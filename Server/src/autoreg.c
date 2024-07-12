#include "autoconf.h"
#include "myndbm.h"
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include "mudconf.h"
#include "attrs.h"
#include "db.h"
#include "externs.h"
#include "alloc.h"


/* 4 less to be safe */
#ifdef QDBM
  #ifdef LBUF64
    #define NDBMBUFSZ 65532
  #else
    #ifdef LBUF32
      #define NDBMBUFSZ 32764
    #else
      #ifdef LBUF16
        #define NDBMBUFSZ 16380
      #else
        #ifdef LBUF8
          #define NDBMBUFSZ 8188
        #else
          #define NDBMBUFSZ 4092
        #endif
      #endif
    #endif
  #endif
#else
#define NDBMBUFSZ 4092
#endif

static char aregname[129 + 64];
static char dumpname[129 + 64 + 6];
static datum keydata, infodata;
static int maxreg;
static char bigbuffer[NDBMBUFSZ];
static char *nbuffer;
static char nflag;
DBM *aregfile;

int areg_init()
{
  int rtemp;

  rtemp =((pmath2) bigbuffer) % 4;
  if (rtemp)
    nbuffer = bigbuffer + 4 - rtemp;
  else
    nbuffer = bigbuffer;
  maxreg = (LBUF_SIZE/sizeof(int)) - 2;

  sprintf(aregname, "%s/%s.areg", mudconf.data_dir, mudconf.muddb_name);
  sprintf(dumpname, "%s.dump", aregname);

  aregfile = dbm_open(aregname, O_RDWR | O_CREAT, 00664);
  if (aregfile == NULL)
    rtemp = 0;
  else
    rtemp = 1;
  return rtemp;
}

void areg_close()
{
  if (mudstate.autoreg)
    dbm_close(aregfile);
}

int email_check(char *email)
{
  char *pt1;

  pt1 = email;
  while (*pt1 && !isspace((int)*pt1))
    pt1++;
  if (*pt1)
    return 0;
  else
    return 1;
}

int areg_check(char *email, int force)
{
  int rcheck, offset, *intpt;

  rcheck = 1;
  if (mudstate.autoreg && *email) {
    nflag = 1;
    keydata.dptr = email;
    keydata.dsize = strlen(email) + 1;
    infodata = dbm_fetch(aregfile,keydata);
    if (infodata.dptr) {
      memcpy(nbuffer,infodata.dptr,infodata.dsize);
      intpt = (int *)nbuffer;
      offset = *(intpt + 1);
      if ((offset >= maxreg) || (!force && (*intpt >= 0) && (offset >= *intpt)))
        rcheck = 0;
      else 
        nflag = 0;
    }
  }
  return rcheck;
}

void areg_add(char *email, dbref newp)
{
  int *intpt, offset;

  intpt = (int *)nbuffer;
  if (nflag) {
    *intpt = mudconf.areg_lim;
    *(intpt + 1) = 1;
  }
  else
    (*(intpt + 1))++;
  offset = *(intpt + 1);
  *(intpt + offset + 1) = newp;
  keydata.dptr = email;
  keydata.dsize = strlen(email) + 1;
  infodata.dptr = nbuffer;
  infodata.dsize = sizeof(int) * (offset + 2);
  dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
}

int areg_del_player(dbref delp)
{
  int *intpt, offset, dcheck, flags;
  dbref owner;
  char *atrbuf, *pt1;

  dcheck = 0;
  if (mudstate.autoreg) {
    atrbuf = atr_get(delp, A_AUTOREG, &owner, &flags);
    if (*atrbuf) {
      pt1 = atrbuf + 7;
      while (!isspace((int)*pt1))
        pt1++;
      *(pt1 - 1) = '\0';
      pt1 = atrbuf + 7;
      keydata.dptr = pt1;
      keydata.dsize = strlen(pt1) + 1;
      infodata = dbm_fetch(aregfile,keydata);
      if (infodata.dptr) {
        memcpy(nbuffer,infodata.dptr,infodata.dsize);
        intpt = (int *)nbuffer;
        for (flags = 0; flags < *(intpt + 1); flags++) {
	  if (*(intpt + 2 + flags) == delp)
	    break;
        }
        if (flags < *(intpt + 1)) {
	  if ((*(intpt + 1) == 1) && (*intpt == mudconf.areg_lim)) {
	    dbm_delete(aregfile,keydata);
	    dcheck = 1;
	  }
	  else {
	    (*(intpt + 1))--;
	    offset = *(intpt + 1);
	    offset -= flags;
	    if (offset > 0)
	      memmove((intpt + 2 + flags),(intpt + 3 + flags),sizeof(int) * offset);
	    infodata.dptr = nbuffer;
	    infodata.dsize = sizeof(int) * (*(intpt + 1) + 2);
	    dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
	    dcheck = 1;
	  }
        }
      }
    }
    free_lbuf(atrbuf);
  }
  return dcheck;
}

void areg_unload(dbref player)
{
  FILE *dump1;
  int *intpt, x;

  dump1 = fopen(dumpname,"w");
  if (!dump1) {
    notify(player,"Areg: Unable to open autoreg dump file.");
    return;
  }
  keydata = dbm_firstkey(aregfile);
  if (!keydata.dptr) {
    notify(player,"Areg: Database error or no data to dump.");
  }
  else {
    do {
      infodata = dbm_fetch(aregfile,keydata);
      if (infodata.dptr) {
	fprintf(dump1,"%s\n",(char *)keydata.dptr);
	memcpy(nbuffer,infodata.dptr,infodata.dsize);
	intpt = (int *)nbuffer;
	for (x = 0; x < (*(intpt + 1) + 2); x++) {
	  if (x > 0)
	    fprintf(dump1,"/%d",*(intpt + x));
	  else
	    fprintf(dump1,"%d",*(intpt + x));
	}
	fputc('\n',dump1);
      }
      keydata = dbm_nextkey(aregfile);
    } while (keydata.dptr);
  }
  fclose(dump1);
  notify(player, "Areg: Dump complete.");
  time(&mudstate.aregflat_time);
}

int mush_getline(char *buf, FILE *fpt)
{
  char *pt1, in;

  pt1 = buf;
  in = fgetc(fpt);
  while (!feof(fpt) && in && (in != '\n')) {
    *pt1++ = in;
    in = fgetc(fpt);
  }
  *pt1 = '\0';
  if (feof(fpt))
    return 0;
  else
    return 1;
}

void areg_wipe(dbref player)
{
  mudstate.autoreg = 0;
  dbm_close(aregfile);
  system(unsafe_tprintf("rm %s*", aregname));
  mudstate.autoreg = areg_init();
  notify(player,"Areg: Database wiped.");
}

void areg_load(dbref player)
{
  FILE *dump1;
  int *intpt, x, count;
  char rbuff1[5000], rbuff2[5000], *pt1, *pt2;

  dump1 = fopen(dumpname, "r");
  if (!dump1) {
    notify(player, "Areg: Unable to open autoreg dump file.");
    return;
  }
  mudstate.autoreg = 0;
  dbm_close(aregfile);
  system(unsafe_tprintf("rm %s*", aregname));
  aregfile = dbm_open(aregname, O_RDWR | O_CREAT, 00664);
  if (!aregfile) {
    notify(player, "Areg: Unable to re-open autoreg data file.");
    return;
  }
  intpt = (int *)nbuffer;
  while (mush_getline(rbuff1,dump1)) {
    mush_getline(rbuff2,dump1);
    pt1 = strchr(rbuff2,'/');
    *pt1 = '\0';
    *intpt = atoi(rbuff2);
    pt2 = strchr(pt1+1,'/');
    *pt2 = '\0';
    count = atoi(pt1+1);
    *(intpt+1) = count;
    pt1 = pt2 + 1;
    for (x = 0; x < count; x++) {
      pt2 = strchr(pt1+1,'/');
      if (pt2)
	*pt2 = '/';
      *(intpt + 2 + x) = atoi(pt1);
      if (pt2)
	pt1 = pt2 + 1;
    }
    keydata.dptr = rbuff1;
    keydata.dsize = strlen(rbuff1) + 1;
    infodata.dptr = nbuffer;
    infodata.dsize = sizeof(int) * (count + 2);
    dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
  }
  fclose(dump1);
  mudstate.autoreg = 1;
  notify(player,"Areg: Load complete.");
}

void areg_list(dbref player, char *email)
{
  int *intpt, check;
  char *tpr_buff, *tprp_buff;

  if (*email) {
    if (email_check(email))
      check = 1;
    else {
      check = 0;
      notify(player,"Areg: Bad email...ignored.");
    }
  }
  else
    check = 0;
  keydata = dbm_firstkey(aregfile);
  if (!keydata.dptr) {
    notify(player, "Areg: Database error or no data.");
    return;
  }
  tprp_buff = tpr_buff = alloc_lbuf("areg_list");
  do {
    infodata = dbm_fetch(aregfile,keydata);
    if (infodata.dptr) {
      if (!check || (check && !stricmp(keydata.dptr,email))) {
	memcpy(nbuffer,infodata.dptr,infodata.dsize);
        intpt = (int *)nbuffer;
        tprp_buff = tpr_buff;
        notify(player,safe_tprintf(tpr_buff, &tprp_buff, "Limit: %d, #: %d, Email: %s",*intpt,*(intpt+1),keydata.dptr));
      }
    }
    keydata = dbm_nextkey(aregfile);
  } while (keydata.dptr);
  free_lbuf(tpr_buff);
  notify(player,"Areg: Done.");
}

void areg_com_add(dbref player, char *newp, char *email, int key)
{
  dbref target, owner;
  char *buff1;
  time_t now;
  int force;

  target = lookup_player(player,newp,0);
  if (target == NOTHING) {
    notify(player,"Areg: Bad player name given.");
    return;
  }
  if (!*email || !email_check(email)) {
    notify(player,"Areg: Bad email given.");
    return;
  }
  if (key == AREG_ADD)
    force = 0;
  else
    force = 1;
  if (!areg_check(email,force)) 
    notify(player,"Areg: Limit reached.");
  else {
    areg_add(email,target);
    buff1 = atr_get(target,A_AUTOREG,&owner,&force);
    if (!*buff1) {
      time(&now);
      sprintf(buff1,"Email: %s, Site: , Id: , Time: %s",email,ctime(&now));
      *(buff1 + strlen(buff1) - 1) = '\0';
      atr_add_raw(target,A_AUTOREG,buff1);
    }
    free_lbuf(buff1);
  }
  notify(player,"Areg: Done.");
}

void areg_com_del_play(dbref player, char *pname)
{
  dbref target;

  target = lookup_player(player,pname,0);
  if (target == NOTHING)
    notify(player,"Areg: Bad player.");
  else {
    if (areg_del_player(target)) 
      atr_clr(target,A_AUTOREG);
    else
      notify(player,"Areg: Missing attribute or player not under email.");
  }
  notify(player,"Areg: Done.");
}

void areg_com_del_email(dbref player, char *email, int key)
{
  int *intpt;

  if (!email_check(email))
    notify(player,"Areg: Bad email.");
  else {
    keydata.dptr = email;
    keydata.dsize = strlen(email) + 1;
    infodata = dbm_fetch(aregfile,keydata);
    if (infodata.dptr) {
      memcpy(nbuffer,infodata.dptr,infodata.dsize);
      intpt = (int *)nbuffer;
      if ((*intpt == mudconf.areg_lim) || (key == AREG_DEL_AEMAIL))
        dbm_delete(aregfile,keydata);
      else {
	*(intpt + 1) = 0;
	infodata.dptr = nbuffer;
	infodata.dsize = sizeof(int) * 2;
	dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
      }
    }
    else
      notify(player,"Areg: Email not found.");
  }
  notify(player,"Areg: Done.");
}

void areg_limit(dbref player, char *email, char *limit)
{
  int *intpt, val;

  if (!*email || !email_check(email))
    notify(player,"Areg: Bad email.");
  else if (!*limit || !is_number(limit))
    notify(player,"Areg: Bad number for limit.");
  else {
    val = atoi(limit);
    if (val < -1)
      val = -1;
    else if (val > maxreg)
      val = maxreg;
    keydata.dptr = email;
    keydata.dsize = strlen(email) + 1;
    infodata = dbm_fetch(aregfile,keydata);
    intpt = (int *)nbuffer;
    if (infodata.dptr) {
      memcpy(nbuffer,infodata.dptr,infodata.dsize);
      *intpt = val;
      infodata.dptr = nbuffer;
      dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
    }
    else {
      *intpt = val;
      *(intpt + 1) = 0;
      infodata.dptr = nbuffer;
      infodata.dsize = sizeof(int) * 2;
      dbm_store(aregfile,keydata,infodata,DBM_REPLACE);
    }
  }
  notify(player,"Areg: Done.");
}

void do_areg(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
  if (!mudstate.autoreg) {
    notify(player, "Areg: autoreg is disabled.");
    return;
  }
  switch(key) {
    case AREG_LOAD:
	areg_load(player);
	break;
    case AREG_UNLOAD:
	areg_unload(player);
	break;
    case AREG_LIST:
	areg_list(player, arg1);
	break;
    case AREG_ADD:
    case AREG_ADD_FORCE:
	areg_com_add(player,arg1,arg2,key);
	break;
    case AREG_DEL_PLAY:
	areg_com_del_play(player,arg1);
	break;
    case AREG_DEL_EMAIL:
    case AREG_DEL_AEMAIL:
	areg_com_del_email(player,arg1,key);
	break;
    case AREG_LIMIT:
	areg_limit(player,arg1,arg2);
	break;
    case AREG_WIPE:
	areg_wipe(player);
	break;
    case AREG_CLEAN:
	areg_unload(player);
	areg_load(player);
	break;
  }
}
