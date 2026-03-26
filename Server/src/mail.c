/* mail.c hardcoded mail system by Seawolf.  Requires ndbm.  Will not *
 * work with dbm due to needs of having the mail database and the     *
 * mush database open at the same time.                                *
 * NOTE: mail/write can be called using a single char start like " is *
 * for say. To do so, just call the mail handler with M_WRITE | M_ALL *
 * for the key. The mail_write function is setup for some Rhostshyl   *
 * specific handling of when the intro char is not valid...i.e. it     *
 * can't be used until after the write message is started normally.   *
 * For others, comment out or delete the #define RHOSTSHYL below      */

/************************************************************************
 * Warning:  For full conceptual understanding of the code contained    *
 *           herein, it is highly recommended that you become tired to  *
 *           such a degree as to require caffeine intake and that you   *
 *           get a 12 pack of IBC(r) rootbeer and quickly down one      *
 *           before attempting to read this code.  You have been warned *
 ************************************************************************/

#define RHOSTSHYL

#ifdef SOLARIS
/* Because Solaris is stupid and needs these declared */
char *strtok_r(char *, const char *, char **);
void bcopy(const void *, void *, int);
#endif
#include "autoconf.h"
#ifdef HAVE_NDBM
#include	"redirect_ndbm.h"
#else
#ifdef HAVE_DBM
#include        <dbm.h>
#else
#include        "myndbm.h"
#endif
#endif
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include "mudconf.h"
#include "attrs.h"
#include "db.h"
#include "externs.h"
#include "alloc.h"
#include "mail.h"
#include "command.h"
#include "rhost_ansi.h"

#define FLENGTH 20
#define SUBJLIM 60
#define REJECTLIM 255
#define MAXWRTL 1000
#define MAXNIND 10
#define SECDAY 86400
#define STARTSMAX 10
#define Chk_Cloak(x,y)	((Good_obj(x) && Good_obj(y)) && \
                         ((Immortal(x) && Dark(x) && Unfindable(x) && Immortal(y)) || \
                         (Wizard(x) && Dark(x) && Unfindable(x) && Wizard(y) && !SCloak(x)) || \
                         (!Cloak(x))))
#define Chk_Dark(x,y)	((Good_chk(x) && Good_chk(y)) && \
                         ((Dark(x) && !mudconf.who_unfindable && !mudconf.player_dark) || \
                          (Unfindable(x) && mudconf.who_unfindable) || NoWho(x)) && \
                         !Admin(y))

static char mailname[260];
static char dumpname[260];
static char foldname[260];
static char fdumpname[260];
static char nukename[260];
static char quickfolder[22];
DBM *mailfile;
DBM *foldfile;
static int lastflag, lastwflag, lfoldflag;
static dbref lastplayer, lfoldplayer, lastwplayer;
static char override;
static char recblock;
extern int FDECL(do_convtime, (char *, struct tm *));
extern char * ColorName(dbref player, int key);
extern void fun_testlock(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern dbref FDECL(match_thing_quiet, (dbref, char *));


void mail_read(dbref, char *, dbref, int);

typedef struct mastruct {
    char akey[17];
    struct mastruct *prev;
    struct mastruct *next;
} MAMEM;

static MAMEM *mapt = NULL;

static char *err_verb[]={"Header Receive Record",
			 "Header Send Record",
			 "Message Record",
			 "Index Send Record",
			 "Mail Database"};

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
static char *bufmaster;
static char *sbuf1;
static char *sbuf2;
static char *sbuf3;
static char *sbuf4;
static char *sbuf5;
static char *sbuf6;
static char *sbuf7;
static char *mbuf1;
static char *mbuf2;
static char *lbuf1;
static char *lbuf2;
static char *lbuf3;
static char *lbuf4;
static char *lbuf5;
static char *lbuf6;
static char *lbuf7;
static char *lbuf8;
static char *lbuf9;
static char *lbuf10;
static char *lbuf11;
static char *lbuf12;
static datum keydata, infodata; 
static int absmaxinx;
static int absmaxrcp;
static int absmaxacs;
static int irecsize;
static int hsuboff;
static int htimeoff;
static int hsizeoff;
static int hindoff;
static int hstoff;
static int foldmast;
static int savemax;
static int savecur;
static int saved;
static int toall;
static int totcharinmail;
static short int tomask;

void FDECL(mail_wipe, (dbref, char *));
void NDECL(mnuke_read);
void FDECL(folder_current, (dbref, int, dbref, int));
void FDECL(fname_conv, (char *, char *));
int FDECL(fname_check, (dbref, char *, int));
int FDECL(mail_md2, (dbref, dbref, int));
extern void FDECL(do_mailfix, (dbref));

static int
mail_check_readlock_perms(dbref player, dbref thing, ATTR * attr,
                  int aowner, int aflags, int key)
{
    int see_it;

    /* If we have explicit read permission to the attr, return it */

    if (See_attr_explicit(player, thing, attr, aowner, aflags))
       return 1;

    /* If we are nearby or have examine privs to the attr and it is
     * visible to us, return it.
     */

    if ( thing == GOING || thing == AMBIGUOUS || !Good_obj(thing))
        return 0;
    if ( key ) {
       see_it = Read_attr(player, thing, attr, aowner, aflags, 0);
    } else {
       see_it = See_attr(player, thing, attr, aowner, aflags, 0);
    }
    if ((Examinable(player, thing) || nearby(player, thing))) /* && see_it) */
       return 1;
    /* For any object, we can read its visible attributes, EXCEPT
     * for descs, which are only visible if read_rem_desc is on.
     */

    if (see_it) {
       if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
          return 0;
       } else {
          return 1;
       }
    }
    return 0;
}

/* As defined in functions.c */
static int
check_read_perms2(dbref player, dbref thing, ATTR * attr,
                 int aowner, int aflags)
{
    int see_it;
 
    /* If we have explicit read permission to the attr, return it */
 
    if (See_attr_explicit(player, thing, attr, aowner, aflags))
        return 1;
 
    /* If we are nearby or have examine privs to the attr and it is
     * visible to us, return it.
     */
 
    if ( thing == GOING || thing == AMBIGUOUS || !Good_obj(thing))
        return 0;
    see_it = See_attr(player, thing, attr, aowner, aflags, 0);
    if ((Examinable(player, thing) || nearby(player, thing)))   /* && see_it) */
        return 1;
    /* For any object, we can read its visible attributes, EXCEPT
     * for descs, which are only visible if read_rem_desc is on.
     */
 
    if (see_it) {
        if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
            return 0;
        } else {
            return 1;
        }
    }
    return 0;
}
 
int
subj_dbref(dbref player, char *subj, int key) {
   int target, aflags;
   dbref aowner;
   char *s_buff, *t_buff, *s_pos;
   ATTR *atr;

   if ( !Good_chk(player) )
      return player;

   if ( key )
      return player;

   if ( !subj || !*subj )
      return player;

   target = NOTHING;
   if ( Wizard(player) && (*subj == '<') && ((s_pos = strchr(subj, '>')) != NULL) ) {
      t_buff = alloc_lbuf("subj_dbref");
      strcpy(t_buff, subj);
      *(t_buff + (s_pos - subj)) = '\0';
      atr = atr_str(t_buff+1);
      if ( atr ) {
         s_buff = atr_get(player, atr->number, &aowner, &aflags);
         if ( s_buff && *s_buff ) {
            target = match_thing_quiet(player, s_buff);
         }
         free_lbuf(s_buff);
      }
      free_lbuf(t_buff);
   }
   if ( !Good_chk(target) ) {
      target = player;
   }
   return target;
}

char *
subj_string(dbref player, char *subj) {
   char *s_pos, *s_return;

   s_return = subj;
   if ( (*subj == '<') && ((s_pos = strchr(subj, '>')) != NULL) ) {
      s_return = s_pos+1;
   }

   return s_return;
}

void 
mapurge()
{
    MAMEM *pt1, *pt2;

    pt1 = mapt;
    while (pt1) {
	pt2 = pt1->next;
	free(pt1);
	pt1 = pt2;
    }
    mapt = NULL;
}

void 
maremove(char *target)
{
    MAMEM *pt1;

    if (!mapt)
	return;
    pt1 = mapt;
    while (pt1 && (strcmp(target, pt1->akey)))
	pt1 = pt1->next;
    if (!pt1)
	return;
    if (pt1 == mapt) {
	mapt = pt1->next;
    } else {
	pt1->prev->next = pt1->next;
	if (pt1->next)
	    pt1->next->prev = pt1->prev;
    }
    free(pt1);
}

void 
mapush(char *target)
{
    MAMEM *pt1;

    pt1 = mapt;
    while (pt1 && (strcmp(target, pt1->akey)))
	pt1 = pt1->next;
    if (pt1)
	return;
    pt1 = (MAMEM *) malloc(sizeof(struct mastruct));

    strcpy(pt1->akey, target);
    pt1->prev = NULL;
    pt1->next = mapt;
    if (mapt)
	mapt->prev = pt1;
    mapt = pt1;
}

void 
mainit()
{
    datum keydata;

    if (!mailfile)
	return;
    if (mapt)
	mapurge();
    keydata = dbm_firstkey(mailfile);
    while (keydata.dptr) {
	memcpy(sbuf1,keydata.dptr,keydata.dsize);
	if (*(int *)sbuf1 == MIND_GA)
	    mapush(sbuf1 + sizeof(int));
	keydata = dbm_nextkey(mailfile);
    }
}

char *
mystrchr(char *buf1, char ch1)
{
    char *p1, *p2;

    if (!buf1)
	return NULL;
    p2 = buf1 + strlen(buf1);
    p1 = buf1;
    while ((p1 <= p2) && (*p1 != ch1))
	p1++;
    if (p1 > p2)
	p1 = NULL;
    return p1;
}

int 
stricmp(char *buf1, char *buf2)
{
    char *p1, *p2;

    p1 = buf1;
    p2 = buf2;
    while ( p1 && p2 && (*p1 != '\0') && (*p2 != '\0') && (tolower(*p1) == tolower(*p2))) {
	p1++;
	p2++;
    }
    if ((*p1 == '\0') && (*p2 == '\0'))
	return 0;
    if (*p1 == '\0')
	return -1;
    if (*p2 == '\0')
	return 1;
    if (*p1 < *p2)
	return -1;
    return 1;
}

char *
strstr2(char *buf1, char *buf2)
{
    char *p1, *p2;
    int len, len2;

    len = strlen(buf2);
    p1 = buf1;
    p2 = mystrchr(p1, ' ');
    if (p2 == NULL) {
	p2 = p1 + strlen(p1);
    }
    while (*p1 != '\0') {
	len2 = p2 - p1;
	if ((strncmp(p1, buf2, len) == 0) && (len == len2))
	    break;
	if (*p2 != '\0') {
	    p1 = p2 + 1;
	} else {
	    p1 = p2;
	}
	if (*p1 != '\0') {
	    p2 = mystrchr(p1, ' ');
	    if (p2 == NULL) {
		p2 = p1 + strlen(p1);
	    }
	}
    }
    if (*p1 == '\0') {
	p1 = NULL;
    }
    return p1;
}

char *
strstr3(char *buf1, char *buf2)
{
    char *p1, *p2, *p3;
    int len;

    len = strlen(buf2);
    if (len > strlen(buf1))
	return NULL;
    p2 = NULL;
    p3 = buf1 + strlen(buf1) - len;
    p1 = buf1;
    do {
	if (strncmp(p1, buf2, len) == 0) {
	    p2 = p1;
	    break;
	}
	p1++;
    } while (p1 <= p3);
    return p2;
}

int 
mail_init()
{
    int temp;

    bufmaster = (char *)malloc(32*7 + 2*200 + 12*NDBMBUFSZ + (ALLIGN1-1));
    if (!bufmaster)
      return 0;
    if (!((pmath1)bufmaster % ALLIGN1))
      sbuf1 = bufmaster;
    else
      sbuf1 = bufmaster+ALLIGN1 - ((pmath1)bufmaster % ALLIGN1);
    sbuf2 = sbuf1+32;
    sbuf3 = sbuf2+32;
    sbuf4 = sbuf3+32;
    sbuf5 = sbuf4+32;
    sbuf6 = sbuf5+32;
    sbuf7 = sbuf6+32;
    mbuf1 = sbuf7+32;
    mbuf2 = mbuf1+200;
    lbuf1 = mbuf2+200;
    lbuf2 = lbuf1+NDBMBUFSZ;
    lbuf3 = lbuf2+NDBMBUFSZ;
    lbuf4 = lbuf3+NDBMBUFSZ;
    lbuf5 = lbuf4+NDBMBUFSZ;
    lbuf6 = lbuf5+NDBMBUFSZ;
    lbuf7 = lbuf6+NDBMBUFSZ;
    lbuf8 = lbuf7+NDBMBUFSZ;
    lbuf9 = lbuf8+NDBMBUFSZ;
    lbuf10 = lbuf9+NDBMBUFSZ;
    lbuf11 = lbuf10+NDBMBUFSZ;
    lbuf12 = lbuf11+NDBMBUFSZ;
    lastflag = 0;
    lastplayer = NOTHING;
    lfoldflag = 0;
    lfoldplayer = NOTHING;
    *mailname = '\0';
	memset(mailname, 0, sizeof(mailname));
	memset(dumpname, 0, sizeof(dumpname));
	memset(foldname, 0, sizeof(foldname));
	memset(fdumpname, 0, sizeof(fdumpname));
	memset(nukename, 0, sizeof(nukename));
    sprintf(mailname, "%s/%s.mail", mudconf.data_dir, mudconf.muddb_name);
    sprintf(dumpname, "%s/%s.dump.mail", mudconf.data_dir, mudconf.muddb_name);
    sprintf(foldname, "%s/%s.folder", mudconf.data_dir, mudconf.muddb_name);
    sprintf(fdumpname, "%s/%s.dump.folder", mudconf.data_dir, mudconf.muddb_name);
    sprintf(nukename, "%s/%s.wipe", mudconf.data_dir, mudconf.muddb_name);
    mailfile = dbm_open(mailname, O_RDWR | O_CREAT, 00664);
    foldfile = dbm_open(foldname, O_RDWR | O_CREAT, 00664);
    mainit();
    tomask = 1;
    tomask <<= ((sizeof(short int) << 3) - 1);
    irecsize = sizeof(short int) * 3;
    absmaxinx = (NDBMBUFSZ / sizeof(short int)) - 3;
    absmaxrcp = (NDBMBUFSZ-sizeof(int)) / (sizeof(int) * 2);
    absmaxacs = NDBMBUFSZ / sizeof(int) - 1;
    htimeoff = sizeof(int);
    hindoff = htimeoff + sizeof(time_t);
    hsizeoff = hindoff + sizeof(short int);
    hstoff = hsizeoff + sizeof(short int);
    hsuboff = hstoff + 3;
    if ((mailfile == NULL) || (foldfile == NULL)) {
	free(bufmaster);
        bufmaster = NULL;
	temp = 0;
    }
    else {
	temp = 1;
	mnuke_read();
    }
    return (temp);
}

void 
mail_proc_err()
{
    STARTLOG(LOG_MAIL, "MAIL", "ERROR")
	log_text("Unknown mail error.  Clearing the error flag.");
    log_text("Last mail command/player to follow.");
    if (lastplayer != NOTHING) {
	log_name(lastplayer);
    } else {
	log_text("NOTHING");
    }
    switch (lastflag) {
    case M_SEND:
	log_text("Send");
	break;
    case M_REPLY:
	log_text("Reply");
	break;
    case M_FORWARD:
	log_text("Forward");
	break;
    case M_READM:
	log_text("Read");
	break;
    case M_LOCK:
	log_text("Lock");
	break;
    case M_STATUS:
	log_text("Status");
	break;
    case M_QUICK:
	log_text("Quick");
	break;
    case M_DELETE:
	log_text("Delete");
	break;
    case M_RECALL:
	log_text("Recall");
	break;
    case M_MARK:
	log_text("Mark");
	break;
    case M_SAVE:
	log_text("Save");
	break;
    case M_WRITE:
	log_text("Write");
	break;
    case M_NUMBER:
	log_text("Number");
	break;
    case M_CHECK:
	log_text("Check");
	break;
    case M_UNMARK:
	log_text("Unmark");
	break;
    case M_PASS:
	log_text("Pass");
	break;
    case M_REJECT:
	log_text("Reject");
	break;
    case M_SHARE:
	log_text("Share");
	break;
    case M_PAGE:
	log_text("Page");
	break;
    case M_QUOTA:
	log_text("Quota");
	break;
    case M_ZAP:		/* FSEND not in because it's a switch */
	log_text("Zap");
	break;
    case M_NEXT:
	log_text("Next");
	break;
    case M_ALIAS:
	log_text("Alias");
	break;
    case M_AUTOFOR:
	log_text("Autofor");
	break;
    case M_VERSION:
	log_text("Version");
	break;		/* ALL is a multiple switch */
    default:
	log_text("No key");
    }
    log_text("Last wmail command/player to follow.");
    if (lastwplayer != NOTHING)
	log_name(lastwplayer);
    else
	log_text("NOTHING");
    switch (lastwflag) {
    case WM_CLEAN:
	log_text("Clean");
	break;
    case WM_LOAD:
	log_text("Load");
	break;
    case WM_UNLOAD:
	log_text("Unload");
	break;
    case WM_OVER:
	log_text("Override");
	break;
    case WM_RESTART:
	log_text("Restart");
	break;
    case WM_SIZE:
	log_text("Size");
	break;
    case WM_ACCESS:
	log_text("Access");
	break;
    case WM_ON:
	log_text("On");
	break;
    case WM_OFF:
	log_text("Off");
	break;
    case WM_WIPE:
	log_text("Wipe");
	break;
    case WM_FIX:
	log_text("Fix");
	break;
    case WM_LOADFIX:
	log_text("Loadfix");
	break;
    case WM_ALIAS:
	log_text("Alias");
	break;
    case WM_MTIME:	/* remove and alock not here because they're switches */
	log_text("Mtime");
	break;
    case WM_DTIME:
	log_text("Dtime");
	break;
    case WM_SMAX:
	log_text("Smax");
	break;
    case WM_VERIFY:
	log_text("Verify");
	break;
    default:
	log_text("No key");
    }
    log_text("Last folder command/player to follow.");
    if (lfoldplayer != NOTHING) {
	log_name(lfoldplayer);
    } else {
	log_text("NOTHING");
    }
    switch (lfoldflag) {
    case M2_CREATE:
	log_text("Create");
	break;
    case M2_DELETE:
	log_text("Delete");
	break;
    case M2_RENAME:
	log_text("Rename");
	break;
    case M2_MOVE:
	log_text("Move");
	break;
    case M2_LIST:
	log_text("List");
	break;
    case M2_CURRENT:
	log_text("Current");
	break;
    case M2_CHANGE:
	log_text("Change");
	break;
    case M2_PLIST:
	log_text("Plist");
	break;
    case M2_SHARE:
	log_text("Share");
	break;
    case M2_CSHARE:
	log_text("Cshare");
	break;
    default:
	log_text("No key");
    }
    ENDLOG
    mail_close();
    mudstate.mail_state = mail_init();
}

void mail_rem_dump()
{
  char *tpr_buff, *tprp_buff;
  
  tprp_buff = tpr_buff = alloc_lbuf("mail_rem_dump");
  system(safe_tprintf(tpr_buff, &tprp_buff, "cp -f \"%s\" \"%s.bkup\"",dumpname, dumpname));
  tprp_buff = tpr_buff;
  system(safe_tprintf(tpr_buff, &tprp_buff, "rm \"%s\"",dumpname));
  tprp_buff = tpr_buff;
  system(safe_tprintf(tpr_buff, &tprp_buff, "cp -f \"%s\" \"%s.bkup\"",fdumpname, fdumpname));
  tprp_buff = tpr_buff;
  system(safe_tprintf(tpr_buff, &tprp_buff, "rm \"%s\"",fdumpname));
  free_lbuf(tpr_buff);
}

void 
mail_close()
{
    if (mailfile)
	dbm_close(mailfile);
    if (foldfile)
	dbm_close(foldfile);
    mapurge();
    if ( bufmaster ) {
       free(bufmaster);
       bufmaster = NULL;
    }
}

/* Error messages for stuff that gets logged is generated here */
void mail_error(dbref player,int code)
{
  char *tpr_buff, *tprp_buff;

  tprp_buff = tpr_buff = alloc_lbuf("mail_error");
  notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "MAIL ERROR: %s",err_verb[code]));
  STARTLOG(LOG_MAIL,"MAIL","DB")
    log_text(err_verb[code]);
  ENDLOG
  free_lbuf(tpr_buff);
}

int get_ind_rec(dbref player, char itype, char *rtbuf, int check, dbref wiz, int key)
{
  if (!key && Wizard(wiz))
    check = 0;
  else if (!key)
    check = 2;
  if ((itype == MIND_IRCV) && check) {
    if (check == 1)
      *(int *)sbuf1 = FIND_CURR;
    else
      *(int *)sbuf1 = FIND_CSHR;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    infodata = dbm_fetch(foldfile,keydata);
    if ( *quickfolder ) {
       infodata.dptr = quickfolder;
    }
    if (infodata.dptr) {
      foldmast = 1;
      *(int *)sbuf1 = FIND_BOX;
      *(int *)(sbuf1 + sizeof(int)) = player;
      strcpy(sbuf1 + (sizeof(int) << 1),infodata.dptr);
      keydata.dptr = sbuf1;
      keydata.dsize = 1 + (sizeof(int) << 1) + strlen(infodata.dptr);
      infodata = dbm_fetch(foldfile,keydata);
      if (!infodata.dptr)
	return 0;
      else {
	memcpy(rtbuf,infodata.dptr,infodata.dsize);
	return(infodata.dsize);
      }
    }
  }
  foldmast = 0;
  *(int *)sbuf1 = itype;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (!(infodata.dptr)) {
    return 0;
  } else {
    memcpy(rtbuf,infodata.dptr,infodata.dsize);
    return(infodata.dsize);
  }
}

int get_hd_rcv_rec_nomsg(dbref player, short int index, char *rtbuf, dbref wiz, int key)
{
  *(int *)sbuf1 = MIND_HRCV;
  *(int *)(sbuf1 + sizeof(int)) = player;
  *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf1;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata = dbm_fetch(mailfile,keydata);
  if (!(infodata.dptr)) {
    return 0;
  }
  else {
    memcpy(rtbuf,infodata.dptr,infodata.dsize);
    return(infodata.dsize);
  }
}
int get_hd_rcv_rec(dbref player, short int index, char *rtbuf, dbref wiz, int key)
{
  *(int *)sbuf1 = MIND_HRCV;
  *(int *)(sbuf1 + sizeof(int)) = player;
  *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf1;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata = dbm_fetch(mailfile,keydata);
  if (!(infodata.dptr)) {
    if (key)
      mail_error(player,MERR_HRCV);
    else
      mail_error(wiz,MERR_HRCV);
    return 0;
  }
  else {
    memcpy(rtbuf,infodata.dptr,infodata.dsize);
    return(infodata.dsize);
  }
}

int get_hd_snd_rec(dbref player, short int index, char *rtbuf, dbref wiz, int key)
{
  *(int *)sbuf1 = MIND_HSND;
  *(int *)(sbuf1 + sizeof(int)) = player;
  *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf1;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata = dbm_fetch(mailfile,keydata);
  if (!(infodata.dptr)) {
    if (key)
      mail_error(player,MERR_HSND);
    else
      mail_error(wiz,MERR_HSND);
    return 0;
  }
  else {
    memcpy(rtbuf,infodata.dptr,infodata.dsize);
    return(infodata.dsize);
  }
}

short int get_box_size(dbref target)
{
  short int *pt1, rval;

  *(int *)sbuf2 = MIND_BSIZE;
  *(int *)(sbuf2+sizeof(int)) = target;
  keydata.dptr = sbuf2;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    memcpy(sbuf7,infodata.dptr,infodata.dsize);
    pt1 = (short int *)sbuf7;
    rval = *pt1;
  }
  else
    rval = mudconf.mailbox_size;
//if ((rval < 10) || (rval > 9999)) rval = mudconf.mailbox_size;
  if (rval < 10) rval = mudconf.mailbox_size;
  if (rval > absmaxinx) rval = absmaxinx;
  return rval;
}

short int count_all_msg(dbref player, char *rtbuf, dbref wiz, int key)
{
  short int *pt1;

  if (get_ind_rec(player,MIND_IRCV,rtbuf,0,wiz,key)) {
    pt1 = (short int *)rtbuf;
    return (*(pt1+2));
  }
  else
    return 0;
}

short int get_free_ind(dbref target, char *bpass, char itype)
{
  short int top, nfree, nmsg, index;
  short int *pt1;

  pt1 = (short int *)bpass;
  if (get_ind_rec(target, itype, bpass, 0, NOTHING, 1)) {
    top = *pt1;
    nfree = *(pt1+1);
    nmsg = *(pt1+2);
  }
  else {
    top = 1;
    nfree = 0;
    nmsg = 0;
    *(pt1+1) = nfree;
    *(pt1+2) = nmsg;
  }
  if (((itype == MIND_IRCV) && ((nmsg > get_box_size(target)) && !override)) || 
       ((top > absmaxinx) && nfree == 0))
    return 0;
  if (nfree) {
    nfree--;
    index = *(pt1+3);
    memmove(pt1+3, pt1+4, (nfree + nmsg) * sizeof(short int));
    *(pt1+3+nfree+nmsg) = index;
    nmsg++;
    *(pt1+1) = nfree;
    *(pt1+2) = nmsg;
  }
  else {
    index = top;
    top++;
    *(pt1+3+nfree+nmsg) = index;
    *pt1 = top;
    nmsg++;
    *(pt1+2) = nmsg;
  }
  return (index);
}

short int get_free_nind(int *num)
{
  int x;
  short int rval;

  rval = 0;
  for (x = -1; x >= -MAXNIND; x--) {
    rval = get_free_ind(x, lbuf11, MIND_ISND);
    if (rval) break;
  }
  *num = x;
  return rval;
}

short int get_msg_index(dbref player, int mesg, int check, dbref wiz, int key)
{
  short int rval, *pt1;

  if ((rval = get_ind_rec(player,MIND_IRCV,lbuf1,check,wiz,key))) {
    pt1 = (short int *)lbuf1;
    if (foldmast) {
      if (mesg > *pt1)
	rval = 0;
      else
	rval = *(pt1 + mesg);
    }
    else {
      if (mesg > *(pt1+2)) 
        rval = 0;
      else 
        rval = *(pt1 + 2 + *(pt1+1) + mesg);
    }
  }
  return rval;
}

void unparse_al_num(char *pass, int len, char *rtbuf, int number)
{
  char *pt2, *pt3, *tpr_buff, *tprp_buff;
  int *pt1, count, x;

  *rtbuf = '\0';
  memcpy(lbuf12,pass,len);
  pt1 = (int *)lbuf12;
  count = *pt1;
  pt1++;
  pt3 = alloc_sbuf("unparse_al");
  *pt3 = '#';
  tprp_buff = tpr_buff = alloc_lbuf("unparse_al_num");
  for (x = 0; x < count; x++) {
    if (number) {
      strcpy(pt3+1,myitoa(*pt1));
      pt2 = pt3;
    }
    else {
      tprp_buff = tpr_buff;
      pt2 = safe_tprintf(tpr_buff, &tprp_buff, "#%d", *pt1);
    }
    if ((strlen(rtbuf) + strlen(pt2)) < LBUF_SIZE -22) {
      if (*rtbuf != '\0')
	strcat(rtbuf," ");
      strcat(rtbuf,pt2);
      pt1++;
    }
    else
      break;
  }
  free_lbuf(tpr_buff);
  free_sbuf(pt3);
}

void unparse_al(char *pass, int len, char *rtbuf, int number, int i_key, dbref player)
{
  char *pt2, *pt3;
  int *pt1, count, x, i_first;

  *rtbuf = '\0';
  memcpy(lbuf12,pass,len);
  pt1 = (int *)lbuf12;
  count = *pt1;
  pt1++;
  pt3 = alloc_sbuf("unparse_al");
  *pt3 = '#';
  i_first = 0;
  for (x = 0; x < count; x++) {
    if (number) {
      strcpy(pt3+1,myitoa(*pt1));
      pt2 = pt3;
    } else {
      if ( i_key & ColorMail(player) ) {
         pt2 = ColorName(*pt1,1);
      } else {
         pt2 = Name(*pt1);
      }
    }
    if ((strlen(rtbuf) + strlen(pt2)) < LBUF_SIZE -22) {
      if ( !i_key ) {
         if (*rtbuf != '\0')
  	   strcat(rtbuf," ");
      } else {
         if ( i_first ) {
            strcat(rtbuf, ", ");
         }
      }
      i_first = 1;
      strcat(rtbuf,pt2);
      pt1++;
    }
    else
      break;
  }
  free_sbuf(pt3);
}

int mail_precheck(dbref player, char *s_input)
{
   char *s_retbuff, *s_strtok, *s_strtokr, *s_tbuff, *s_t1;
   int aflags, i_return, i_chk;
   dbref aowner, target;
   ATTR *atr;

   if ( !s_input || !*s_input ) {
      notify(player, "MAIL: Empty send list.");
      return 1;
   }

   s_tbuff = alloc_lbuf("mail_precheck");
   strcpy(s_tbuff, s_input);

   if ( *s_tbuff == '@' ) {
      s_strtok = strtok_r(s_tbuff+1, " \t,", &s_strtokr);
   } else {
      s_strtok = strtok_r(s_tbuff, " \t,", &s_strtokr);
   }

   i_return = i_chk = 0;
   s_retbuff = alloc_lbuf("mail_precheck2");
   while ( s_strtok && *s_strtok ) {
      switch(*s_strtok) {
         case '+': /* Global static */
            s_t1 = (char *)mail_alias_function(player, 0, s_strtok+1, NULL);
            if ( !s_t1 || !*s_t1 ) {
               sprintf(s_retbuff, "MAIL: Invalid static global alias '%s'.", s_t1);
               notify(player, s_retbuff);
               i_return = 1;
            }
            free_lbuf(s_t1);
            break;
         case '$': /* Global dynamic */
            if ( Good_chk(mudconf.mail_def_object) ) {
               sprintf(s_retbuff, "alias.%s", s_strtok+1);
               atr = atr_str(s_retbuff);
               if ( atr && (!(atr->flags & AF_MDARK) || Wizard(player)) ) {
                  s_t1 = atr_get(mudconf.mail_def_object, atr->number, &aowner, &aflags);
                  if ( !s_t1 || !*s_t1 ) {
                     sprintf(s_retbuff, "MAIL: Invalid dynamic global alias '%s'.", s_strtok+1);
                     notify(player, s_retbuff);
                     i_return = 1;
                  }
                  free_lbuf(s_t1);
               } else {
                  sprintf(s_retbuff, "MAIL: Invalid dynamic global alias '%s'.", s_strtok+1);
                  notify(player, s_retbuff);
                  i_return = 1;
               }
            } else {
               if ( !i_chk )
                  notify(player, "MAIL: No dynamic global aliases configured.");
               i_return = 1;
               i_chk = 1;
            }
            break;
         case '&': /* Personal static */
            sprintf(s_retbuff, "%s", s_strtok+1);
            atr = atr_str(s_retbuff);
            if ( atr ) {
               s_t1 = atr_pget(player, atr->number, &aowner, &aflags);
               if ( !s_t1 || !*s_t1 ) {
                  sprintf(s_retbuff, "MAIL: Invalid static personal alias '%s'.", s_strtok+1);
                  notify(player, s_retbuff);
                  i_return = 1;
               }
               free_lbuf(s_t1);
            } else {
               sprintf(s_retbuff, "MAIL: Invalid static personal alias '%s'.", s_strtok+1);
               notify(player, s_retbuff);
               i_return = 1;
            }
            break;
         case '~': /* Personal dynamic */
            sprintf(s_retbuff, "%s", s_strtok+1);
            atr = atr_str(s_retbuff);
            if ( atr ) {
               s_t1 = atr_pget(player, atr->number, &aowner, &aflags);
               if ( !s_t1 || !*s_t1 ) {
                  sprintf(s_retbuff, "MAIL: Invalid dynamic personal alias '%s'.", s_strtok+1);
                  notify(player, s_retbuff);
                  i_return = 1;
               }
               free_lbuf(s_t1);
            } else {
               sprintf(s_retbuff, "MAIL: Invalid dynamic personal alias '%s'.", s_strtok+1);
               notify(player, s_retbuff);
               i_return = 1;
            }
            break;
         default: /* Assume normal player name, number or alias */
            target = lookup_player(player, s_strtok, 0);
            if ( !(Good_chk(target) && isPlayer(target)) ) {
               sprintf(s_retbuff, "MAIL: Invalid player '%s'.", s_strtok);
               notify(player, s_retbuff);
               i_return = 1;
            }
            break;
      }
      s_strtok = strtok_r(NULL, " \t,", &s_strtokr);
   }

   free_lbuf(s_tbuff);
   free_lbuf(s_retbuff);
   return i_return;
}

void parse_tolist(dbref player, char *s_input, char *s_out, char **s_outptr)
{
   char *s_tbuff, *s_strtok, *s_strtokr, *s_t1, *s_t2, *s_t3, *s_b1, *s_b1p,
        *s_strpl, *s_strplr, *x_buff, *s_array[2];
   int i_allow, i_mail_inline, i_first, aflags;
   dbref target, aowner;
   ATTR *atr;

   if ( !s_input || !s_out || !*s_outptr )  {
      return;
   }

   s_tbuff = alloc_lbuf("parse_tolist");
   strcpy(s_tbuff, s_input);

   if ( *s_tbuff == '@' ) {
      s_strtok = strtok_r(s_tbuff+1, " \t,", &s_strtokr);
   } else {
      s_strtok = strtok_r(s_tbuff, " \t,", &s_strtokr);
   }
   s_b1p = s_b1 = alloc_lbuf("parse_tolist");
   x_buff = alloc_lbuf("parse_tolist");
   i_first = 0;
   while ( s_strtok ) {
      i_allow = 1;
      switch(*s_strtok) {
         case '+': /* Global static */
            s_t1 = (char *)mail_alias_function(player, 0, s_strtok+1, NULL);
            s_t2 = (char *)mail_alias_function(player, 1, s_strtok+1, NULL);
            if ( s_t1 && *s_t1 ) {
               if ( s_t2 && *s_t2 ) {
                  memset(s_b1, '\0', LBUF_SIZE);
                  sprintf(x_buff, "#%d", player);
                  s_b1p = s_b1;
                  s_array[0] = s_t1;
                  s_array[1] = x_buff;
                  fun_testlock(s_b1, &s_b1p, player, player, player, s_array, 2, (char **)NULL, 0);
                  i_allow = atoi(s_b1);
               }
            } else {
               /*  Nothing found - disallow */
               i_allow = 0;
            }
            if ( i_allow ) {
               strcpy(s_b1, s_t1);
               /* Walk s_t1 and convert to player names, append to list */
               s_strpl = strtok_r(s_b1, " \t", &s_strplr);
               while ( s_strpl ) {
                  target = lookup_player(player, s_strpl, 0);
                  if ( Good_chk(target) && isPlayer(target) ) {
                     if ( i_first ) {
                        safe_str(", ", s_out, s_outptr);
                     }
                     i_first = 1;
                     if ( ColorMail(player) ) {
                        safe_str(ColorName(target, 1), s_out, s_outptr);
                     } else {
                        safe_str(Name(target), s_out, s_outptr);
                     }
                  } else {
                     if ( i_first ) {
                        safe_str(", ", s_out, s_outptr);
                     }
#ifdef ZENTY_ANSI
                     sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strpl, SAFE_ANSI_NORMAL);
#else
                     sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strpl, ANSI_NORMAL);
#endif
                     safe_str(x_buff, s_out, s_outptr);
                  }
                  s_strpl = strtok_r(NULL, " \t", &s_strplr);
               }
            } else {
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
               i_first = 1;
#ifdef ZENTY_ANSI
               sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
               sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
               safe_str(x_buff, s_out, s_outptr);
               /* Print alias in ansi-red in form !+alias! */
            }
            free_lbuf(s_t1);
            free_lbuf(s_t2);
            break;
         case '$': /* Global dynamic */
            if ( Good_chk(mudconf.mail_def_object) ) {
               sprintf(x_buff, "alias.%s", s_strtok+1);
               atr = atr_str(x_buff);
               /* If attribute not hidden or wizard allow */
               if ( atr && (!(atr->flags & AF_MDARK) || Wizard(player)) ) {
                  s_t1 = atr_get(mudconf.mail_def_object, atr->number, &aowner, &aflags);
                  /* If attribute not hidden or wizard allow */
                  if ( s_t1 && *s_t1 && (!(aflags & AF_MDARK) || Wizard(player)) ) {
                     if ( !((atr->flags & AF_PINVIS) || (aflags & AF_PINVIS)) || Wizard(player) ) {
                        i_mail_inline = mudstate.mail_inline;
                        mudstate.mail_inline = 1;
                        s_t3 = exec(mudconf.mail_def_object, player, player, EV_FCHECK | EV_EVAL, s_t1, (char **) NULL, 0, (char **)NULL, 0);
                        mudstate.mail_inline = i_mail_inline;
                        if ( *s_t3 ) {
                           s_strpl = strtok_r(s_t3, " \t", &s_strplr);
                           while ( s_strpl && *s_strpl) {
                              target = lookup_player(player, s_strpl, 0);
                              if ( Good_chk(target) && isPlayer(target) ) {
                                 if ( i_first ) {
                                    safe_str(", ", s_out, s_outptr);
                                 }
                                 i_first = 1;
                                 if ( ColorMail(player) ) {
                                    safe_str(ColorName(target, 1), s_out, s_outptr);
                                 } else {
                                    safe_str(Name(target), s_out, s_outptr);
                                 }
                              } else {
                                 if ( i_first ) {
                                    safe_str(", ", s_out, s_outptr);
                                 }
#ifdef ZENTY_ANSI
                                 sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strpl, SAFE_ANSI_NORMAL);
#else
                                 sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strpl, ANSI_NORMAL);
#endif
                                 safe_str(x_buff, s_out, s_outptr);
                              }
                              s_strpl = strtok_r(NULL, " \t", &s_strplr);
                           }
                        }
                        free_lbuf(s_t3);
                     } else {
                        if ( i_first ) {
                           safe_str(", ", s_out, s_outptr);
                        }
                     /* Hidden player lists from players */
#ifdef ZENTY_ANSI
                        sprintf(x_buff, "<%s%s%s>", SAFE_ANSI_GREEN, s_strtok, SAFE_ANSI_NORMAL);
#else
                        sprintf(x_buff, "<%s%s%s>", ANSI_GREEN, s_strtok, ANSI_NORMAL);
#endif
                        safe_str(x_buff, s_out, s_outptr);
                     }
                  } else {
                     if ( i_first ) {
                        safe_str(", ", s_out, s_outptr);
                     }
                  /* Cloaked mail lists from players */
#ifdef ZENTY_ANSI
                     sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
                     sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
                     safe_str(x_buff, s_out, s_outptr);
                  }
                  free_lbuf(s_t1);
               } else {
               /* Cloaked mail lists from players */
                  if ( i_first ) {
                     safe_str(", ", s_out, s_outptr);
                  }
                  i_first = 1;
#ifdef ZENTY_ANSI
                  sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
                  sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
                  safe_str(x_buff, s_out, s_outptr);
               }
            } else {
            /* If attribute unuseable or missing print in form !$alias! */
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
               i_first = 1;
#ifdef ZENTY_ANSI
               sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
               sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
               safe_str(x_buff, s_out, s_outptr);
            }
            break;
         case '&': /* Personal static */
            atr = atr_str(s_strtok+1);
            if ( !atr ) {
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
#ifdef ZENTY_ANSI
               sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
               sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
               safe_str(x_buff, s_out, s_outptr);
            } else {
               s_t1 = atr_get(player, atr->number, &aowner, &aflags);
               if (mail_check_readlock_perms(player, player, atr, aowner, aflags, 0)) {
                  s_strpl = strtok_r(s_t1, " \t", &s_strplr);
                  while ( s_strpl ) {
                     target = lookup_player(player, s_strpl, 0);
                     if ( Good_chk(target) && isPlayer(target) ) {
                        if ( i_first ) {
                           safe_str(", ", s_out, s_outptr);
                        }
                        i_first = 1;
                        if ( ColorMail(player) ) {
                           safe_str(ColorName(target, 1), s_out, s_outptr);
                        } else {
                           safe_str(Name(target), s_out, s_outptr);
                        }
                     } else {
                        if ( i_first ) {
                           safe_str(", ", s_out, s_outptr);
                        }
#ifdef ZENTY_ANSI
                        sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strpl, SAFE_ANSI_NORMAL);
#else
                        sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strpl, ANSI_NORMAL);
#endif
                        safe_str(x_buff, s_out, s_outptr);
                     }
                     s_strpl = strtok_r(NULL, " \t", &s_strplr);
                  }
               } else {
                  if ( i_first ) {
                     safe_str(", ", s_out, s_outptr);
                  }
                  i_first = 1;
#ifdef ZENTY_ANSI
                  sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
                  sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
                  safe_str(x_buff, s_out, s_outptr);
               }
               free_lbuf(s_t1);
            }
            break;
         case '~': /* Personal dynamic */
            atr = atr_str(s_strtok+1);
            if ( !atr ) {
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
               i_first = 1;
#ifdef ZENTY_ANSI
               sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
               sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
               safe_str(x_buff, s_out, s_outptr);
            } else {
               s_t1 = atr_get(player, atr->number, &aowner, &aflags);
               if (mail_check_readlock_perms(player, player, atr, aowner, aflags, 0)) {
                  s_t3 = exec(player, player, player, EV_FCHECK | EV_EVAL, s_t1, (char **) NULL, 0, (char **)NULL, 0);
                  if ( *s_t3 ) {
                     s_strpl = strtok_r(s_t3, " \t", &s_strplr);
                     while ( s_strpl ) {
                        target = lookup_player(player, s_strpl, 0);
                        if ( Good_chk(target) && isPlayer(target) ) {
                           if ( i_first ) {
                              safe_str(", ", s_out, s_outptr);
                           }
                           i_first = 1;
                           if ( ColorMail(player) ) {
                              safe_str(ColorName(target, 1), s_out, s_outptr);
                           } else {
                              safe_str(Name(target), s_out, s_outptr);
                           }
                        } else {
                           if ( i_first ) {
                              safe_str(", ", s_out, s_outptr);
                           }
#ifdef ZENTY_ANSI
                           sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strpl, SAFE_ANSI_NORMAL);
#else
                           sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strpl, ANSI_NORMAL);
#endif
                           safe_str(x_buff, s_out, s_outptr);
                        }
                        s_strpl = strtok_r(NULL, " \t", &s_strplr);
                     }
                  } else {
                     if ( i_first ) {
                        safe_str(", ", s_out, s_outptr);
                     }
                     i_first = 1;
#ifdef ZENTY_ANSI
                     sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
                     sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
                     safe_str(x_buff, s_out, s_outptr);
                  }
                  free_lbuf(s_t3);
               } else {
                  if ( i_first ) {
                     safe_str(", ", s_out, s_outptr);
                  }
                  i_first = 1;
#ifdef ZENTY_ANSI
                  sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
                  sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
                  safe_str(x_buff, s_out, s_outptr);
               }
               free_lbuf(s_t1);
            }
            break;
         default: /* Assume normal player name, number or alias */
            target = lookup_player(player, s_strtok, 0);
            if ( Good_chk(target) && isPlayer(target) ) {
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
               i_first = 1;
               if ( ColorMail(player) ) {
                  safe_str(ColorName(target, 1), s_out, s_outptr);
               } else {
                  safe_str(Name(target), s_out, s_outptr);
               }
            } else {
               if ( i_first ) {
                  safe_str(", ", s_out, s_outptr);
               }
               i_first = 1;
#ifdef ZENTY_ANSI
               sprintf(x_buff, "[%s!%s!%s]", SAFE_ANSI_RED, s_strtok, SAFE_ANSI_NORMAL);
#else
               sprintf(x_buff, "[%s!%s!%s]", ANSI_RED, s_strtok, ANSI_NORMAL);
#endif
               safe_str(x_buff, s_out, s_outptr);
            }
            break;
      }
      s_strtok = strtok_r(NULL, " \t,", &s_strtokr);
   }
   free_lbuf(s_tbuff);
   free_lbuf(s_b1);
   free_lbuf(x_buff);
}

void unparse_to(dbref player, short int index, char *rtbuf, dbref *plrdb, int i_mushname)
{
  char *pt2, *mbuftmp, *s_mushname;
  int *pt1, count, x;

  *(int *)sbuf6 = MIND_HSND;
  *(int *)(sbuf6 + sizeof(int)) = player;
  *(short int *)(sbuf6 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf6;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata = dbm_fetch(mailfile,keydata);
  *rtbuf = '\0';
  if (infodata.dptr) {
    s_mushname = alloc_lbuf("unparse_to_mushname");
    if ( i_mushname ) {
       memset(s_mushname, '\0', LBUF_SIZE);
       sprintf(s_mushname, "%s", mudconf.mud_name);
       mbuftmp = s_mushname;
       while ( *mbuftmp ) {
          if ( isspace(*mbuftmp) ) {
             *mbuftmp = '_';
          }
          mbuftmp++;
       }
    }
    memcpy(lbuf12,infodata.dptr,infodata.dsize);
    pt1 = (int *)lbuf12;
    count = *pt1;
    pt1++;
    mbuftmp = alloc_mbuf("unparse_to");
    for (x = 0; x < count; x++) {
      get_hd_rcv_rec(*pt1, (short int)(*(pt1+1)), mbuftmp, NOTHING, 1);
      if ( !Wizard(player) &&
           (*(mbuftmp+1+hstoff) == 'r' ||
            *(mbuftmp+1+hstoff) == 'n' ||
            *(mbuftmp+1+hstoff) == 'f' ||
            *(mbuftmp+1+hstoff) == 'o' ||
            *(mbuftmp+1+hstoff) == 'a') ) {
         if ( strlen(mudconf.mail_anonymous) )
            pt2 = mudconf.mail_anonymous;
         else
            pt2 = (char *)"*Anonymous*";
      } else {
         if ( ColorMail(player) ) {
            pt2 = ColorName(*pt1,1);
         } else {
            pt2 = Name(*pt1);
         }
      }
      *plrdb = *pt1;
      if ((strlen(rtbuf) + strlen(s_mushname) + strlen(pt2)) < (LBUF_SIZE - 12)) {
	if (*rtbuf != '\0') {
          if ( i_mushname ) 
	     strcat(rtbuf,", ");
          else if ( mudconf.name_spaces )
	     strcat(rtbuf,",");
          else
	     strcat(rtbuf," ");
        }
	strcat(rtbuf,pt2);
        if ( i_mushname ) {
	  strcat(rtbuf,"@");
	  strcat(rtbuf,s_mushname);
        }
	pt1 += 2;
      }
      else
	break;
    }
    free_mbuf(mbuftmp);
    free_lbuf(s_mushname);
  }
}

void unparse_to_2(dbref player, short int index, char *rtbuf, dbref *plrdb)
{
  char *pt2, *mbuftmp;
  int *pt1, count, x;

  *(int *)sbuf6 = MIND_HSND;
  *(int *)(sbuf6 + sizeof(int)) = player;
  *(short int *)(sbuf6 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf6;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata = dbm_fetch(mailfile,keydata);
  *rtbuf = '\0';
  if (infodata.dptr) {
    memcpy(lbuf12,infodata.dptr,infodata.dsize);
    pt1 = (int *)lbuf12;
    count = *pt1;
    pt1++;
    mbuftmp = alloc_mbuf("unparse_to_2");
    for (x = 0; x < count; x++) {
      if (!get_hd_rcv_rec(*pt1, (short int)(*(pt1+1)), mbuftmp, NOTHING, 1)) {
	pt1 += 2;
	continue;
      }
      if ((*(mbuftmp+hstoff) != 'U') && (*(mbuftmp+hstoff) != 'N')) {
	pt1 += 2;
	continue;
      }
      if ( !Wizard(player) &&
           (*(mbuftmp+1+hstoff) == 'r' ||
            *(mbuftmp+1+hstoff) == 'n' ||
            *(mbuftmp+1+hstoff) == 'f' ||
            *(mbuftmp+1+hstoff) == 'a') ) {
         if ( strlen(mudconf.mail_anonymous) )
            pt2 = mudconf.mail_anonymous;
         else
            pt2 = (char *)"*Anonymous*";
      } else {
         if ( ColorMail(player) ) {
            pt2 = ColorName(*pt1, 1);
         } else {
            pt2 = Name(*pt1);
         }
      }
      *plrdb = *pt1;
      if ((strlen(rtbuf) + strlen(pt2)) < LBUF_SIZE -12) {
	if (*rtbuf != '\0')
	  strcat(rtbuf," ");
	strcat(rtbuf,pt2);
	pt1 += 2;
      }
      else
	break;
    }
    free_mbuf(mbuftmp);
  }
}

short int insert_msg(dbref player, dbref *toplay, char *subj, char *msg, 
			short int indpass, int fwrd, int *aft, int *listpt, 
                        int lcount, int chk_anon, char *anon_player, int chk_anon3,
                        char *plr_list)
{
  int *pt1, dest, save, loop, chk_anon2, i_foundfolder, s_aflags, i_mail_inline, cmd_bitmask;
  dbref s_aowner;
  short int index, *pt2;
  time_t *pt3;
  char fc, *pt4, *tprp_buff, *tpr_buff, *s_tmparry[5], *s_mailfilter, *s_returnfilter, 
       *s_mail, *s_mailarr[5];
  static char s_plrbuff[35];
  ATTR *s_atr;

  save = -1;
  i_mail_inline = chk_anon2 = i_foundfolder = 0;
  s_mailfilter = tprp_buff = tpr_buff = NULL;
  s_tmparry[0] = s_tmparry[1] = s_tmparry[2] = s_tmparry[3] = s_tmparry[4] = NULL;
  if ( chk_anon && Wizard(*toplay) ) {
     chk_anon2 = 1;
  }
  if (!could_doit(player,*toplay,A_LMAIL,1,0)) {
    s_mailfilter = atr_pget(*toplay, A_MFAIL, &s_aowner, &s_aflags);
    pt4 = NULL;
    if ( *s_mailfilter ) {
       pt4 = exec(*toplay, player, player, EV_FIGNORE | EV_EVAL, s_mailfilter, (char **) NULL, 0, (char **)NULL, 0);
    }
    free_lbuf(s_mailfilter);
    if ( pt4 && *pt4 ) {
       tprp_buff = tpr_buff = alloc_lbuf("insert_msg_mfail");
       safe_str((char *)"Mail: ", tpr_buff, &tprp_buff);
       safe_str(pt4, tpr_buff, &tprp_buff);
       notify_quiet(player, tpr_buff);
       free_lbuf(tpr_buff);
       free_lbuf(pt4);
    } else {
       if ( pt4 ) {
          free_lbuf(pt4);
       }
       *(int *)sbuf1 = MIND_REJM;
       *(int *)(sbuf1+sizeof(int)) = *toplay;
       keydata.dptr = sbuf1;
       keydata.dsize = sizeof(int) << 1;
       infodata = dbm_fetch(mailfile,keydata);
       if (infodata.dptr) {
         pt4 = exec(player, player, player, EV_FIGNORE | EV_EVAL, infodata.dptr, (char **) NULL, 0, (char **)NULL, 0);
         notify_quiet(player,"Mail:");
         notify_quiet(player,pt4);
         free_lbuf(pt4);
       } else {
         tprp_buff = tpr_buff = alloc_lbuf("insert_msg");
         notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Your mail has been rejected by %s",Name(*toplay)));
         free_lbuf(tpr_buff);
       }
    }
    return 0;
  }
  *(int *)sbuf1 = MIND_AFOR;
  *(int *)(sbuf1+sizeof(int)) = *toplay;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  *aft = 0;
  if (!override && infodata.dptr) {
    memcpy(sbuf7,infodata.dptr,sizeof(int));
    dest = *(int *)sbuf7;
    if ( Good_obj(dest) && !Recover(dest) && !Going(dest) ) {
       if (could_doit(*toplay,dest,A_LMAIL,1,0)) {
         save = *toplay;
         *aft = 1;
         *toplay = dest;
       }
    } else {
       notify_quiet(*toplay, "MAIL ERROR: Invalid autoforward setting...removing.");
       dbm_delete(mailfile, keydata);
    }
  }
  for (loop = 0; loop < lcount; loop += 2) {
    if (*(listpt + loop) == *toplay)
      return 0;
  }
  index = get_free_ind(*toplay,lbuf1,MIND_IRCV);
  if (!index) {
    tprp_buff = tpr_buff = alloc_lbuf("insert_msg");
    notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "MAIL ERROR: Mailbox for %s is full.",Name(*toplay)));
    free_lbuf(tpr_buff);
    return 0;
  }
  pt2 = (short int *)lbuf1;
  *(int *)sbuf1 = MIND_IRCV;
  *(int *)(sbuf1 + sizeof(int)) = *toplay;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata.dptr = lbuf1;
  infodata.dsize = sizeof(short int) * (3 + *(pt2+1) + *(pt2+2));
  dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
  if (*aft) fc = 'A';
  else if (fwrd == 1) fc = 'F';
  else if (fwrd == 2) fc = 'R';
  else fc = 'N';

  if ( chk_anon )
     fc = tolower(fc);
  
  pt1 = (int *)mbuf1;
  pt2 = (short int*)(mbuf1+hindoff);
  pt3 = (time_t *)(mbuf1+htimeoff);
  *(mbuf1+hstoff) = 'N';
  if ( chk_anon3 )
     *(mbuf1+1+hstoff) = 'n';
  else
     *(mbuf1+1+hstoff) = 'N';
  *(mbuf1+2+hstoff) = fc;
  *pt1 = player;
  if ( *plr_list ) {
     sprintf(s_plrbuff, " %d ", *toplay);
     if ( strstr(plr_list, s_plrbuff) != NULL ) {
        if ( mudconf.bcc_hidden )
           *pt2 = indpass;
        else 
           *pt2 = indpass | tomask;
     } else {
        if (toall)
          *pt2 = indpass | tomask;
        else
          *pt2 = indpass;
     }
  } else {
     if (toall)
       *pt2 = indpass | tomask;
     else
       *pt2 = indpass;
  }
  *(pt2+1) = (short int)strlen(msg);
  *pt3 = mudstate.now;
  if (subj)  {
    if (*aft) {
      *(mbuf1+hsuboff) = '(';
      strncpy(mbuf1+hsuboff+1,Name(save),22);
      strcat(mbuf1+hsuboff+1,") ");
      strcat(mbuf1+hsuboff+2,subj);
    }
    else
      strcpy(mbuf1+hsuboff,subj);
  }
  else if (*aft) {
    *(mbuf1+hsuboff) = '(';
    strncpy(mbuf1+hsuboff+1,Name(save),22);
    strcat(mbuf1+hsuboff+1,")");
  }
  *(int *)sbuf1 = MIND_HRCV;
  *(int *)(sbuf1 + sizeof(int)) = *toplay;
  *(short int*)(sbuf1 + (sizeof(int) << 1)) = index;
  keydata.dptr = sbuf1;
  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
  infodata.dptr = mbuf1;
  if (subj || (save != -1))
    infodata.dsize = hsuboff + strlen(mbuf1+hsuboff) + 1;
  else
    infodata.dsize = hsuboff;
  dbm_store(mailfile, keydata, infodata, DBM_REPLACE);

  /* Trigger a target's mail notify attribute */
  s_atr = atr_str("mailnotify");
  if ( s_atr && Good_chk(*toplay) ) {
     s_mailfilter = atr_get(*toplay, s_atr->number, &s_aowner, &s_aflags);
     if ( s_mailfilter && *s_mailfilter ) {
        s_tmparry[0] = alloc_lbuf("s_tmparry_0");
        s_tmparry[1] = alloc_lbuf("s_tmparry_1");
        s_tmparry[2] = alloc_lbuf("s_tmparry_1");
        s_tmparry[3] = alloc_lbuf("s_tmparry_1");
        sprintf(s_tmparry[0], "#%d", ((chk_anon && !chk_anon2) ? -1 : player));
        if ( subj && *subj )
           sprintf(s_tmparry[1], "%s", subj_string(*toplay, subj));
        strcpy(s_tmparry[2], msg);
	strcat(s_tmparry[3], ctime((time_t *)pt3));
 	*(s_tmparry[3] + strlen(s_tmparry[3]) - 1) = '\0';
        i_mail_inline = mudstate.mail_inline;
        mudstate.mail_inline = 1;
        s_returnfilter = exec(*toplay, *toplay, *toplay, EV_FCHECK | EV_EVAL, s_mailfilter,
                                 s_tmparry, 4, (char **)NULL, 0);
        mudstate.mail_inline = i_mail_inline;
        if ( *s_returnfilter ) {
           sprintf(s_tmparry[2], "%s", Name(*toplay));
           sprintf(s_tmparry[1], "Mailnotify from %.30s: %.*s", s_tmparry[2], (LBUF_SIZE - 60), s_returnfilter);
           notify_quiet(player, s_tmparry[1]);
        }
        free_lbuf(s_tmparry[0]);
        free_lbuf(s_tmparry[1]);
        free_lbuf(s_tmparry[2]);
        free_lbuf(s_tmparry[3]);
        free_lbuf(s_returnfilter);
     }
     free_lbuf(s_mailfilter);
  }  

  /* Trigger a target's mailfilter attribute to handle folder moving */
  s_atr = atr_str("mailfilter");
  if ( s_atr && Good_obj(*toplay) ) {
     s_mailfilter = atr_get(*toplay, s_atr->number, &s_aowner, &s_aflags);
     if ( s_mailfilter && *s_mailfilter ) {
        s_tmparry[0] = alloc_lbuf("s_tmparry_0");
        s_tmparry[1] = alloc_lbuf("s_tmparry_1");
        s_tmparry[2] = alloc_lbuf("s_tmparry_1");
        s_tmparry[3] = alloc_lbuf("s_tmparry_1");
        sprintf(s_tmparry[0], "#%d", ((chk_anon && !chk_anon2) ? -1 : player));
        if ( subj && *subj )
           sprintf(s_tmparry[1], "%s", subj_string(*toplay, subj));
        strcpy(s_tmparry[2], msg);
	strcat(s_tmparry[3], ctime((time_t *)pt3));
 	*(s_tmparry[3] + strlen(s_tmparry[3]) - 1) = '\0';
        i_mail_inline = mudstate.mail_inline;
        mudstate.mail_inline = 1;
        s_returnfilter = exec(*toplay, *toplay, *toplay, EV_FCHECK | EV_EVAL, s_mailfilter,
                                 s_tmparry, 4, (char **)NULL, 0);
        mudstate.mail_inline = i_mail_inline;
        free_lbuf(s_tmparry[0]);
        free_lbuf(s_tmparry[1]);
        free_lbuf(s_tmparry[2]);
        free_lbuf(s_tmparry[3]);
        if ( *s_returnfilter && fname_check(*toplay, s_returnfilter, 0) ) {
           *(int *)sbuf1 = FIND_LST;
           *(int *)(sbuf1 + sizeof(int)) = *toplay;
           keydata.dptr = sbuf1;
           keydata.dsize = sizeof(int) << 1;
           infodata = dbm_fetch(foldfile, keydata);
           fname_conv(s_returnfilter ,s_plrbuff);
           if ( !infodata.dptr || !strstr2(infodata.dptr, s_plrbuff) ) {
              notify_quiet(player, "MAIL ERROR: Folder does not exist.");
           } else {
              *(int *)sbuf1=FIND_BOX;
              *(int *)(sbuf1 + sizeof(int)) = *toplay;
              strcpy(sbuf1 + (sizeof(int) << 1), s_plrbuff);
              keydata.dptr = sbuf1;
              keydata.dsize = 1 + (sizeof(int) << 1) + strlen(s_plrbuff);
              infodata = dbm_fetch(foldfile,keydata);
              i_foundfolder = 1;
           }
        }
        free_lbuf(s_returnfilter);
     } 
     free_lbuf(s_mailfilter);
  }
  if ( !i_foundfolder ) {
     *(int *)sbuf1=FIND_BOX;
     *(int *)(sbuf1 + sizeof(int)) = *toplay;
     strcpy(sbuf1 + (sizeof(int) << 1),"Incoming");
     keydata.dptr = sbuf1;
     keydata.dsize = 9 + (sizeof(int) << 1);
     infodata = dbm_fetch(foldfile,keydata);
  }
  pt2 = (short int *)lbuf1;
  if (!infodata.dptr) {
    *pt2 = 1;
    *(pt2+1) = index;
  }
  else {
    memcpy(lbuf1,infodata.dptr,infodata.dsize);
    *(pt2 + 1 + *pt2) = index;
    (*pt2)++;
  }
  infodata.dptr = lbuf1;
  infodata.dsize = sizeof(short int) * (1 + *pt2);
  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
  if (Connected(*toplay) || mudconf.mail_verbosity) {
    tprp_buff = tpr_buff = alloc_lbuf("insert_msg");
    if ( mudconf.mail_verbosity && subj ) {
       notify_quiet(*toplay, safe_tprintf(tpr_buff, &tprp_buff, "Mail: You have new mail (#%d) from -> %s%s[Subj: %s]", 
                    index,
                    ((chk_anon && !chk_anon2) ? 
                          anon_player : (ColorMail(*toplay) ? 
                               ColorName(subj_dbref(player,subj,0), 1) : Name(subj_dbref(player,subj,0)))), 
                    (chk_anon2 ? " (Anonymously) " : " "), 
                    subj_string(player, subj)));
    } else {
       notify_quiet(*toplay, safe_tprintf(tpr_buff, &tprp_buff, "Mail: You have new mail (#%d) from -> %s%s", 
                    index, 
                    ((chk_anon && !chk_anon2) ? anon_player : (ColorMail(*toplay) ? ColorName(player, 1) : Name(player))), 
                    (chk_anon2 ? " (Anonymously) " : " ")));
    }
    if ( i_foundfolder ) {
       notify_quiet(*toplay, safe_tprintf(tpr_buff, &tprp_buff, "Mail: Auto-Moved to folder -> %s", s_plrbuff));
    }
    free_lbuf(tpr_buff);
  }
  s_mail = atr_pget(*toplay, A_AMAIL, &s_aowner, &s_aflags);
  if ( *s_mail ) {
     s_mailarr[0] = alloc_lbuf("amail_array_0");
     s_mailarr[1] = alloc_lbuf("amail_array_1");
     s_mailarr[2] = alloc_lbuf("amail_array_2");
     s_mailarr[3] = alloc_lbuf("amail_array_2");
     s_mailarr[4] = alloc_lbuf("amail_array_2");
     sprintf(s_mailarr[0], "%d", index);
     if ( Wizard(*toplay) ) {
        sprintf(s_mailarr[1], "#%d", player);
        sprintf(s_mailarr[2], "%s", (ColorMail(*toplay) ? ColorName(player, 1) : Name(player)));
        sprintf(s_mailarr[3], "#%d", subj_dbref(player, subj, 0));
        sprintf(s_mailarr[4], "%s", 
                 (ColorMail(*toplay) ? ColorName(subj_dbref(player, subj, 0), 1) : Name(subj_dbref(player, subj, 0))));
     } else {
        if ( chk_anon ) {
           sprintf(s_mailarr[1], "#-1");
           sprintf(s_mailarr[2], "%s", anon_player);
        } else {
           sprintf(s_mailarr[1], "#%d", subj_dbref(player, subj, 0));
           sprintf(s_mailarr[2], "%s", 
                 (ColorMail(*toplay) ? ColorName(subj_dbref(player, subj, 0), 1) : Name(subj_dbref(player, subj, 0))));
        }
     }
     cmd_bitmask = mudstate.cmd_bitmask;
     mudstate.cmd_bitmask |= NOMAIL;
     wait_que(*toplay, *toplay, 0, NOTHING, s_mail, s_mailarr, 5, NULL, NULL);
     free_lbuf(s_mailarr[0]);
     free_lbuf(s_mailarr[1]);
     free_lbuf(s_mailarr[2]);
     free_lbuf(s_mailarr[3]);
     free_lbuf(s_mailarr[4]);
     mudstate.cmd_bitmask = cmd_bitmask;
  }
  free_lbuf(s_mail);
  return (index);
}

void 
build_bcc_list(dbref player, char *s_instr, char *s_outstr, char *s_outstrptr)
{
   char *strtok_buff, *s_strtokptr, *s_tbuff, *s_tbuffptr, *s_ptr, *s_plratr,
        *tbuff_malias, *s_plratrretval;
   int in_list, anum_tmp, dummy2, did_alloc, i_mail_inline;
   dbref thing_tmp, dummy1, target;
   MAMEM *ma_ptr;
   ATTR *attr = NULL;
 
   s_tbuffptr = s_tbuff = alloc_lbuf("build_bcc_list");
   s_strtokptr = (char *)strtok_r(s_instr, " ", &strtok_buff);
   i_mail_inline = in_list = did_alloc = 0;
   while ( s_strtokptr && *s_strtokptr ) {
      s_ptr = s_strtokptr;
      if ( *s_ptr == '+' ) {
         s_ptr++;
	 if (strlen(s_ptr) <= 16) {
            ma_ptr = mapt;
	    while (ma_ptr) {
	       if (!strcmp(s_ptr,ma_ptr->akey))
                  break;
	       ma_ptr = ma_ptr->next;
	    }
            if ( ma_ptr ) {
               *(int *)sbuf1 = MIND_GA;
               strcpy(sbuf1 + sizeof(int), ma_ptr->akey);
               keydata.dptr = sbuf1;
               keydata.dsize = strlen(ma_ptr->akey) + 1 + sizeof(int);
               infodata = dbm_fetch(mailfile,keydata);
               if ( infodata.dptr ) {
                  safe_str(infodata.dptr, s_tbuff, &s_tbuffptr);
                  safe_chr(' ', s_tbuff, &s_tbuffptr);
               }
            }
         }
      } else if ( *s_ptr == '&' ) {
         s_ptr++;
	 attr = atr_str(s_ptr);
         if ( attr ) {
	    s_plratr = atr_get(player,attr->number,&dummy1,&dummy2);
            if ( s_plratr && *s_plratr ) {
               safe_str(s_plratr, s_tbuff, &s_tbuffptr);
               safe_chr(' ', s_tbuff, &s_tbuffptr);
            }
            free_lbuf(s_plratr);
         }
      } else if ( *s_ptr == '~' ) {
         s_ptr++;
         did_alloc = 0;
         if (parse_attrib(player, s_ptr, &thing_tmp, &anum_tmp)) {
            if ( (anum_tmp == NOTHING) || (!Good_obj(thing_tmp) ||
                 Going(thing_tmp) || Recover(thing_tmp)) ) {
               attr = NULL;
            } else {
               attr = atr_num(anum_tmp);
            }
         } else {
            thing_tmp = player;
            attr = atr_str(s_ptr);
         }
         if ( attr ) {
            s_plratr = atr_pget(thing_tmp, attr->number, &dummy1, &dummy2);
            if ( !(!s_plratr || !Good_obj(player) || (Cloak(thing_tmp) && !Wizard(player)) ||
                    ((Cloak(thing_tmp) && SCloak(thing_tmp)) && !Immortal(player))) ) {
               if ( check_read_perms2(player, thing_tmp, attr, dummy1, dummy2) ) {
                  i_mail_inline = mudstate.mail_inline;
                  mudstate.mail_inline = 1;
                  s_plratrretval = exec(player, thing_tmp, thing_tmp, EV_FCHECK | EV_EVAL, 
                                        s_plratr, (char **) NULL, 0, (char **)NULL, 0);
                  mudstate.mail_inline = i_mail_inline;
                  did_alloc = 1;
               } else {
                  s_plratrretval = NULL;
               }
               if ( s_plratrretval && *s_plratrretval ) {
                  safe_str(s_plratrretval, s_tbuff, &s_tbuffptr);
                  safe_chr(' ', s_tbuff, &s_tbuffptr);
               }
               if ( did_alloc )
                  free_lbuf(s_plratrretval);
            }
            free_lbuf(s_plratr);
         }
      } else if ( *s_ptr == '$' ) {
         s_ptr++;
         if ( Good_obj(mudconf.mail_def_object) &&
              !Recover(mudconf.mail_def_object) &&
              !Going(mudconf.mail_def_object) ) {
            tbuff_malias = alloc_mbuf("tbuff_malias");
            sprintf(tbuff_malias, "#%d/alias.%.64s", mudconf.mail_def_object, s_ptr);
            if (parse_attrib(mudconf.mail_def_object, tbuff_malias, &thing_tmp, &anum_tmp) ) {
               if ( (anum_tmp == NOTHING) || (!Good_obj(thing_tmp) ||
                    Going(thing_tmp) || Recover(thing_tmp)) ) {
                      attr = NULL;
               } else {
                      attr = atr_num(anum_tmp);
               }
            } else {
               attr = NULL;
            }
            if ( attr ) {
               if ( (attr->flags & AF_MDARK) && !Wizard(player) )
                  attr = NULL;
            }
            free_mbuf(tbuff_malias);
         } else {
            attr = NULL;
         }
         if ( attr ) {
            s_plratr = atr_pget(thing_tmp, attr->number, &dummy1, &dummy2);
            if ( s_plratr && (dummy2 & AF_MDARK) && !Wizard(player) ) {
               *s_plratr = '\0';
            }
            if ( s_plratr && Good_obj(player) ) { 
               i_mail_inline = mudstate.mail_inline;
               mudstate.mail_inline = 1;
               s_plratrretval = exec(thing_tmp, player, player, EV_FCHECK | EV_EVAL, 
                                     s_plratr, (char **) NULL, 0, (char **)NULL, 0);
               mudstate.mail_inline = i_mail_inline;
               did_alloc = 1;
            } else {
               s_plratrretval = NULL;
            }
            if ( s_plratrretval && *s_plratrretval ) {
               safe_str(s_plratrretval, s_tbuff, &s_tbuffptr);
               safe_chr(' ', s_tbuff, &s_tbuffptr);
            }
            if ( did_alloc )
               free_lbuf(s_plratrretval);
            free_lbuf(s_plratr);
         }
      } else {
         safe_str(s_ptr, s_tbuff, &s_tbuffptr);
         safe_chr(' ', s_tbuff, &s_tbuffptr);
      }
      s_strtokptr = (char *)strtok_r(NULL, " ", &strtok_buff);
   }

   /* Build out the dbref# lists */
   if ( *s_tbuff ) {
      s_strtokptr = (char *)strtok_r(s_tbuff, " ", &strtok_buff);
      while (s_strtokptr) {
         target = lookup_player(player, s_strtokptr, 0);
         if ( Good_obj(target) && isPlayer(target) ) {
            if (!in_list)
               safe_chr(' ', s_outstr, &s_outstrptr);
            safe_str(myitoa(target), s_outstr, &s_outstrptr);
            safe_chr(' ', s_outstr, &s_outstrptr);
            in_list = 1;
         }
         s_strtokptr = (char *)strtok_r(NULL, " ", &strtok_buff);
      }
   }
   free_lbuf(s_tbuff);
}

int mail_send(dbref p2, int key, char *buf1, char *buf2, char *subpass)
{
  dbref player, thing_tmp, plrdb, plrtrash;
  char *spt, *mpt, *pt1, *pt2, *apt, *sigpt, *sigpteval, sepchar, tp_chr, *apt_tmp;
  char *tbuff_malias, *bccatr, *bcctmp, *bcctmpptr, *tpr_buff, *tprp_buff;
  int toplay, count, offct, term, dummy1, dummy2, repall, repaal, rmsg, x, atrash, i_nogood;
  short int index, isend;
  int acheck, atest, t1, aft, tolist, anum_tmp, glob_malias, chk_anon, chk_anon2, chk_lbuf_free;
  int comma_exists, i_mail_inline;
  MAMEM *pt3;
  ATTR *attr = NULL;
  static char anon_player[17];
  time_t now1, now2;

  now1 = time(NULL);
  i_mail_inline = comma_exists = 0;
  spt = mpt = NULL;
  memset(anon_player, 0, sizeof(anon_player));
  tolist = mudconf.mail_tolist;
  player = Owner(p2);
  chk_anon = 0;
  chk_lbuf_free = 1;
  if ( strlen(mudconf.mail_anonymous) ) {
     strncpy(anon_player, mudconf.mail_anonymous, 16);
  } else {
     strcpy(anon_player, "*Anonymous*");
  }
  if ( key & M_ANON ) {
     key = (key & ~M_ANON);
     chk_anon = 1;
  }
  isend = get_free_ind(player,lbuf2,MIND_ISND);
  if (!isend) {
     notify_quiet(p2,"MAIL ERROR: Your sent message tracking is full");
     return 1;
  }
  if ((key == M_REPLY) && (*buf1 == '^')) {
    toall = 0;
    repall = 1;
    pt1 = buf1+1;
  }
  else if ((key == M_REPLY) && (*buf1 == '@')) {
    toall = 1;
    repall = 1;
    pt1 = buf1+1;
  }
  else {
    toall = 0;
    repall = 0;
    pt1 = buf1;
  }


  rmsg = 0;
  if ((key == M_REPLY) || (key == M_FORWARD)) {
    pt2 = pt1;
    while ((*pt2) && (*pt2 != ' ') && (*pt2 != ',')) pt2++;
    if ((!*pt2) && (key == M_FORWARD)) {
      notify_quiet(p2,"MAIL ERROR: Improper forward format");
      return 1;
    } else if (*pt2 && (key == M_REPLY)) {
      notify_quiet(p2,"MAIL ERROR: Improper reply format");
      return 1;
    }
    if (*pt2) *pt2 = '\0';
    if (key == M_REPLY) {
      if (*(pt2-1) == '*') {
	*(pt2-1) = '\0';
	rmsg = 1;
      }
    }
    if (!is_number(pt1)) {
      notify_quiet(p2,"MAIL ERROR: Number expected");
      return 1;
    }
    acheck = atoi(pt1);
    acheck = get_msg_index(player,acheck,1,NOTHING,1);
/*  if ((acheck < 1) || (acheck > 9999)) { */
    if (acheck < 1) {
      notify_quiet(p2,"MAIL ERROR: Bad message number specified");
      return 1;
    }
    if (!get_hd_rcv_rec(player,acheck,mbuf1,NOTHING,1)) 
      return 1;
    acheck = *(short int *)(mbuf1 + hindoff);
    if (acheck & tomask) {
      acheck &= ~tomask;
      repaal = 1;
    } else {
      repaal = 0;
    }
    if (rmsg || (key == M_FORWARD)) {
      *(int *)sbuf1 = MIND_MSG;
      *(int *)(sbuf1 + sizeof(int)) = *(int *)mbuf1;
      *(short int *)(sbuf1 + (sizeof(int) << 1)) = acheck;
      keydata.dptr = sbuf1;
      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
      infodata = dbm_fetch(mailfile,keydata);
      if (!infodata.dptr) {
	mail_error(p2,MERR_MSG);
	return 1;
      }
    }
    if (key == M_REPLY) {
      if (rmsg) {
        if (subpass) {
	  spt = subpass;
	  mpt = buf2;
        } else {
          if ( Good_obj(p2) && PennMail(p2) )
  	     spt = strstr(buf2,"/");
          else
  	     spt = strstr(buf2,"//");
  	  if (spt) {
  	    if ((spt - buf2) > SUBJLIM)
  	      *(buf2 + SUBJLIM) = '\0';
  	    else
  	      *spt = '\0';
            if ( Good_obj(p2) && PennMail(p2) )
  	       mpt = spt+1;
            else
  	       mpt = spt+2;
  	    spt = buf2;
  	  } else {
  	    spt = NULL;
  	    mpt = buf2;
  	  }
        }
	if ((strlen(mpt) + strlen(infodata.dptr)) > LBUF_SIZE - 134) {
	  notify_quiet(p2,"MAIL ERROR: Combined reply message too long");
	  return 1;
	} else {
	  strcpy(lbuf8,infodata.dptr);
	  strcat(lbuf8,"\r\n");
	  pt2 = lbuf8 + strlen(lbuf8);
	  for (x = 0; x < 78; x++)
	    *pt2++ = '-';
	  strcpy(pt2,"\r\nReply: (");
	  if ( chk_anon ) {
            if ( strlen(mudconf.mail_anonymous) ) {
               strncat(pt2, mudconf.mail_anonymous, 22);
            } else {
               strcat(pt2, (char *)"*Anonymous*");
            } 
	  } else {
	    strncat(pt2,Name(player),22);
          }
 	  strcat(pt2,") [");
	  strcat(pt2, ctime((time_t *)(mbuf1+htimeoff)));
 	  *(pt2 + strlen(pt2) - 1) = ']';
	  strcat(pt2, "\r\n");
	  strcat(pt2,mpt);
	  mpt = lbuf8;
	}
      }
      t1 = *(int *)mbuf1;
      if (repall && repaal) {
	unparse_to(*(int *)mbuf1,acheck,lbuf3, &plrdb, 0);
	if (strlen(lbuf3) > NDBMBUFSZ - 10) {
	  notify_quiet(p2,"MAIL ERROR: Reply list too long");
	  return 1;
	}
        comma_exists = 0;
        if ( (strchr(lbuf3, ',') != NULL) && (key == M_REPLY) )
           comma_exists = 1;
	if (Good_obj(t1)) {
          if ( comma_exists )
	     strcat(lbuf3,",#");
          else
	     strcat(lbuf3," #");
	  strcat(lbuf3,myitoa(Owner(t1)));
	}
      } else {
	if (Good_obj(t1)) {
	  *lbuf3 = '#';
	  strcpy(lbuf3+1,myitoa(Owner(t1)));
	} else
	  *lbuf3 = '\0';
      }
    }
    else {
      pt1 = pt2 + 1;
      if (subpass) {
	spt = subpass;
	mpt = buf2;
      } else {
        if ( Good_obj(p2) && PennMail(p2) )
  	   spt = strstr(buf2,"/");
        else
  	   spt = strstr(buf2,"//");
  	if (spt) {
  	  if ((spt - buf2) > SUBJLIM)
  	    *(buf2 + SUBJLIM) = '\0';
  	  else
  	    *spt = '\0';
          if ( Good_obj(p2) && PennMail(p2) )
  	     mpt = spt+1;
          else
  	     mpt = spt+2;
  	  spt = buf2;
  	}
  	else {
  	  spt = NULL;
  	  mpt = buf2;
  	}
      }
      if ((strlen(mpt) + strlen(infodata.dptr)) > LBUF_SIZE - 210) {
	notify_quiet(p2,"MAIL ERROR: Forwarded message too long");
	return 1;
      }
      strcpy(lbuf8,"Original Text From: ");
      if (!Good_obj(*(int *)mbuf1) || Recover(*(int *)mbuf1) || Going(*(int *)mbuf1))
	strcat(lbuf8,"(NONE)");
      else {
        if ( *(mbuf1+2+hstoff) == 'r' ||
             *(mbuf1+2+hstoff) == 'n' ||
             *(mbuf1+2+hstoff) == 'f' ||
             *(mbuf1+2+hstoff) == 'a' ) {
           strncat(lbuf8, anon_player, 22);
        } else {
	   strncat(lbuf8,Name(*(int *)mbuf1),22);
        }
      }
      strcat(lbuf8," [");
      strcat(lbuf8, ctime((time_t *)(mbuf1+htimeoff)));
      *(lbuf8 + strlen(lbuf8) - 1) = ']';
      strcat(lbuf8,"\r\n");
      strcat(lbuf8,infodata.dptr);
      strcat(lbuf8,"\r\n");
      pt2 = lbuf8 + strlen(lbuf8);
      for (acheck = 0; acheck < 78; acheck++, pt2++)
	*pt2 = '-';
      strcpy(pt2,"\r\nAdditional Text: (");
      if ( chk_anon )
         strncat(pt2,anon_player,22);
      else
         strncat(pt2,Name(player),22);
      strcat(pt2,") ");
      strcat(pt2,"[");
      strcat(pt2, ctime((time_t *)(&mudstate.now)));
      *(pt2 + strlen(pt2) - 1) = ']';
      strcat(pt2, "\r\n");
      strcat(pt2,mpt);
      mpt = lbuf8;
    }
  }

  if ( (key & M_REPLY) && 
       (*(mbuf1+2+hstoff) == 'r' ||
        *(mbuf1+2+hstoff) == 'n' ||
        *(mbuf1+2+hstoff) == 'f' ||
        *(mbuf1+2+hstoff) == 'a') ) {
    chk_anon2 = 1;
  } else {
    chk_anon2 = 0;
  }
  count = 0;
  if ((key != M_FORWARD) && !rmsg) {
    if (subpass) {
      spt = subpass;
      if (!rmsg)
        mpt = buf2;
    }
    else {
      if ( Good_obj(p2) && PennMail(p2) )
         spt = strstr(buf2,"/");
      else
         spt = strstr(buf2,"//");
      if (spt) {
        if ((spt - buf2) > SUBJLIM)
          *(buf2 + SUBJLIM) = '\0';
        else
          *spt = '\0';
        if ( Good_obj(p2) && PennMail(p2) )
           mpt = spt+1;
        else
           mpt = spt+2;
        spt = buf2;
      }
      else {
        spt = NULL;
	if (!rmsg)
          mpt = buf2;
      }
    }
    if (strlen(mpt) > (LBUF_SIZE - 1)) {
      notify_quiet(p2,"MAIL ERROR: Message too long");
      return 1;
    }
  }
  sigpt = atr_get(p2,A_MAILSIG, &dummy1, &dummy2);
  if (*sigpt) {
    attr = atr_num(A_MAILSIG);
    if ( !((dummy2 & AF_NOPARSE) || (attr->flags & AF_NOPARSE)) ) {
       sigpteval = exec(p2, p2, p2, EV_FCHECK | EV_EVAL, sigpt, (char **) NULL, 0, (char **)NULL, 0);
       free_lbuf(sigpt);
       sigpt = sigpteval;
    }
    if ((strlen(mpt) + strlen(sigpt)) > LBUF_SIZE -2) 
      notify_quiet(p2,"Mail Warning: Signature not added due to insufficient space");
    else {
      strcpy(lbuf9,mpt);
      strcat(lbuf9,"\r\n");
      strcat(lbuf9,sigpt);
      mpt = lbuf9;
    }
  }
  free_lbuf(sigpt);
  if (key == M_REPLY) {
    acheck = 0;
  } else {
    if (*pt1 == '@') {
      toall = 1;
      pt1++;
    } 
    else {
      toall = 0;
    }
    strcpy(lbuf3,pt1);
    acheck = 1;
  }
  if ( tolist )
     toall = !toall;
  sepchar = '\0';
  while (acheck) {
    acheck = 0;
    *lbuf4 = '\0';
    pt1 = lbuf3;
    term = 0;
    comma_exists = 0;
    if ( strchr(lbuf3, ',') != NULL )
       comma_exists = 1;
    while (!term && *pt1) {
      if ( comma_exists )
         while ( *pt1 && (isspace((int)*pt1) || (*pt1 == ','))) pt1++;
      else
         while ( *pt1 && isspace((int)*pt1)) pt1++;
      if (!*pt1) break;
      pt2 = pt1+1;
      if ((sepchar == '\0') || (*pt1 == '#')) {
        while (*pt2 && !isspace((int)*pt2) && (*pt2 != ',')) pt2++;
      } else {
        while (*pt2 && (*pt2 != sepchar) && (*pt2 != ',')) pt2++;
      }
      if (*pt2 && !sepchar) {
        sepchar = *pt2;
        term = 0;
      }
      else if (*pt2)
        term = 0;
      else
        term = 1;
      now2 = time(NULL);
      if ( now2 > (now1 + mudconf.cputimechk) ) {
	  notify_quiet(p2,"MAIL WARNING: Alias recursion exceeded.");
          if ( pt1 && *pt1 ) {
             broadcast_monitor(p2, MF_CPU, "MAIL ALIAS RECURSION REACHED", pt1, NULL, p2, 0, 0, NULL);
          } else {
             broadcast_monitor(p2, MF_CPU, "MAIL ALIAS RECURSION REACHED", (char *)"<aliases>", NULL, p2, 0, 0, NULL);
          }
	  break;
      }
      if ((*pt1 != '+') && (*pt1 != '&') && (*pt1 != '~') && (*pt1 != '$')) {
	if ((strlen(lbuf4) + (pt2-pt1) + 2) > NDBMBUFSZ - 4) {
	  notify_quiet(p2,"MAIL WARNING: Alias expansion truncated");
	  break;
	}
	strncat(lbuf4,pt1,pt2-pt1+1);
      }
      else {
	*pt2 = '\0';
	if (*pt1 == '+') {
	  pt1++;
	  if (strlen(pt1) > 16) {
	    notify_quiet(p2,"MAIL ERROR: Global alias name too long");
	    pt3 = NULL;
	  }
	  else
	    pt3 = mapt;
	  while (pt3) {
	    if (!strcmp(pt1,pt3->akey)) break;
	    pt3 = pt3->next;
	  }
	  if (pt3) {
	    *(int *)sbuf1 = MIND_GAL;
	    strcpy(sbuf1+sizeof(int),pt1);
	    keydata.dptr = sbuf1;
	    keydata.dsize = strlen(pt1) + 1 + sizeof(int);
	    infodata = dbm_fetch(mailfile,keydata);
	    if (infodata.dptr) 
	      atest = eval_boolexp_atr(player,player,player,infodata.dptr,1, 0);
	    else
	      atest = 1;
	    if (atest || Wizard(player)) {
	      *(int *)sbuf1 = MIND_GA;
	      strcpy(sbuf1 + sizeof(int), pt3->akey);
	      keydata.dptr = sbuf1;
	      keydata.dsize = strlen(pt3->akey) + 1 + sizeof(int);
	      infodata = dbm_fetch(mailfile,keydata);
	      if (infodata.dptr) {
		unparse_al(infodata.dptr,infodata.dsize,lbuf11,1, 0, p2);
	        if ((strlen(lbuf4) + strlen(lbuf11)) > NDBMBUFSZ - 4) {
		  notify_quiet(p2,"MAIL ERROR: Alias expansion too long");
	        }
	        else {
		  acheck = 1;
		  strcat(lbuf4,lbuf11);
		  strcat(lbuf4," ");
	        }
	      }
	    }
	  }
	}
	else {
          glob_malias = 0;
          tp_chr = *pt1;
	  pt1++;
          if ( tp_chr == '&' ) {
	     attr = atr_str(pt1);
          } else if ( tp_chr == '$' ) {
             glob_malias = 1;
             if ( Good_obj(mudconf.mail_def_object) && 
                  !Recover(mudconf.mail_def_object) &&
                  !Going(mudconf.mail_def_object) ) {
                tbuff_malias = alloc_mbuf("tbuff_malias");
                sprintf(tbuff_malias, "#%d/alias.%.64s", mudconf.mail_def_object,
                        pt1);
                if (parse_attrib(mudconf.mail_def_object, tbuff_malias, 
                                 &thing_tmp, &anum_tmp) ) {
                   if ( (anum_tmp == NOTHING) || (!Good_obj(thing_tmp) || 
                                                  Going(thing_tmp) || 
                                                  Recover(thing_tmp)) )
                      attr = NULL;
                   else
                      attr = atr_num(anum_tmp);
                } else {
                   attr = NULL;
                }
                if ( attr && (attr->flags & AF_MDARK) && !Wizard(p2) ) {
                   attr = NULL;
                }
                free_mbuf(tbuff_malias);
             } else {
                attr = NULL;
             }
          } else {
             if (parse_attrib(p2, pt1, &thing_tmp, &anum_tmp)) {
                if ( (anum_tmp == NOTHING) || (!Good_obj(thing_tmp) || 
                                               Going(thing_tmp) || 
                                               Recover(thing_tmp)) )
                   attr = NULL;
                else
                   attr = atr_num(anum_tmp);
             } else {
                thing_tmp = p2;
                attr = atr_str(pt1);
             }
          }
	  if ( !attr ) {
            if ( glob_malias )
	       notify_quiet(p2,"MAIL ERROR: Global alias does not exist");
            else
	       notify_quiet(p2,"MAIL ERROR: Attribute alias does not exist");
	  } else {
            if ( tp_chr == '&' ) {
	       apt = atr_get(p2,attr->number,&dummy1,&dummy2);
            } else if ( tp_chr == '$' ) {
               apt_tmp = atr_pget(thing_tmp, attr->number, &dummy1, &dummy2);
               if ( apt_tmp && (dummy2 & AF_MDARK) && !Wizard(p2)) {
                  *apt_tmp = '\0';
               }
               if ( !apt_tmp || !*apt_tmp ) {
                  apt = NULL;
                  chk_lbuf_free = 0;
               } else {
                  i_mail_inline = mudstate.mail_inline;
                  mudstate.mail_inline = 1;
                  apt = exec(thing_tmp, p2, p2, EV_FCHECK | EV_EVAL, apt_tmp, (char **) NULL, 0, (char **)NULL, 0);
                  mudstate.mail_inline = i_mail_inline;
               }
               free_lbuf(apt_tmp);
            } else {
               apt_tmp = atr_pget(thing_tmp, attr->number, &dummy1, &dummy2);
               if ( !apt_tmp || !*apt_tmp || !Good_obj(p2) || (Cloak(thing_tmp) && !Wizard(p2)) ||
                    ((Cloak(thing_tmp) && SCloak(thing_tmp)) && !Immortal(p2)) ) {
                  apt = NULL;
                  chk_lbuf_free = 0;
               } else {
                  if ( check_read_perms2(p2, thing_tmp, attr, dummy1, dummy2) ) {
                     i_mail_inline = mudstate.mail_inline;
                     mudstate.mail_inline = 1;
                     apt = exec(p2, thing_tmp, thing_tmp, EV_FCHECK | EV_EVAL, apt_tmp,
                                (char **) NULL, 0, (char **)NULL, 0);
                     mudstate.mail_inline = i_mail_inline;
                  } else {
                     chk_lbuf_free = 0;
                     apt = NULL;
                  }
               }
               free_lbuf(apt_tmp);
            }
	    if ( !apt || !*apt ) {
	      notify_quiet(p2,"MAIL ERROR: Attribute alias does not exist");
	    } else {
	      if ((strlen(lbuf4) + strlen(apt)) > NDBMBUFSZ - 4) {
		notify_quiet(p2,"MAIL ERROR: Alias expansion too long");
	      }
	      else {
		acheck = 1;
		strcat(lbuf4,apt);
                if ( strchr(apt, ',') != NULL )
		   strcat(lbuf4,",");
                else
		   strcat(lbuf4," ");
	      }
	    }
            if ( chk_lbuf_free )
	       free_lbuf(apt);
	  }
	}
      }
      if (!term) pt1 = pt2+1;
    }
    strcpy(lbuf3,lbuf4);
  }
  term = 0;
  if (key == M_FORWARD)
    atest = 1;
  else if (key == M_REPLY)
    atest = 2;
  else
    atest = 0;
  offct = sizeof(int);
  sepchar = '\0';

  /* Let's grab their BCC list for verification and build out the players */
  bccatr = atr_get(player, A_BCCMAIL, &plrtrash, &atrash);
  bcctmp = bcctmpptr = alloc_lbuf("bcc_send_mail");
  if ( bccatr ) {
     build_bcc_list(player, bccatr, bcctmp, bcctmpptr);
  }
  free_lbuf(bccatr);

  i_nogood = 0;
  tprp_buff = tpr_buff = alloc_lbuf("mail_send");
  if ( Good_chk(p2) && MailValid(p2) ) {
     strcpy(tpr_buff, lbuf3);
     pt1 = tpr_buff;
     comma_exists = 0;
     if ( strchr(tpr_buff, ',') != NULL )
        comma_exists = 1;
     while ( !term && *pt1 ) {
        if ( comma_exists )
           while ( *pt1 && (isspace((int)*pt1) || (*pt1 == ','))) pt1++;
        else
           while (*pt1 && isspace((int)*pt1)) pt1++;
        if (!*pt1) {
           if ( i_nogood != 2 )
              i_nogood = 1;
           break;
        }
        pt2 = pt1+1;
        if ((sepchar == '\0') || (*pt1 == '#')) {
           if ( comma_exists )
              while (*pt2 && (*pt2 != ',')) pt2++;
           else
              while (*pt2 && !isspace((int)*pt2) && (*pt2 != ',')) pt2++;
        } else {
           while (*pt2 && (*pt2 != sepchar) && (*pt2 != ',')) pt2++;
        }
        if (*pt2 && !sepchar) {
           sepchar = *pt2;
           term = 0;
        } else if (*pt2) {
           term = 0;
        } else
           term = 1;
        *pt2 = '\0';
        toplay = lookup_player(player,pt1,0);
        if (toplay == NOTHING) {
           i_nogood = 1;
           break;
        } else {
           if ( !i_nogood )
              i_nogood = 2;
        }
        pt1 = pt2+1;
     }
     if ( i_nogood == 1 ) {
         notify_quiet(p2,"MAIL ERROR: Bad recipient(s) in send.  Mail not sent per user's toggle.");
         free_lbuf(bcctmp);
         free_lbuf(tpr_buff);
         return 1;
     }
  }
  pt1 = lbuf3;
  sepchar = '\0';
  term = 0;
  comma_exists = 0;
  if ( strchr(lbuf3, ',') != NULL )
     comma_exists = 1;
  while (!term && *pt1) {
    if ( comma_exists )
       while ( *pt1 && (isspace((int)*pt1) || (*pt1 == ','))) pt1++;
    else
       while (*pt1 && isspace((int)*pt1)) pt1++;
    if (!*pt1) break;
    pt2 = pt1+1;
    if ((sepchar == '\0') || (*pt1 == '#')) {
      if ( comma_exists )
         while (*pt2 && (*pt2 != ',')) pt2++;
      else
         while (*pt2 && !isspace((int)*pt2) && (*pt2 != ',')) pt2++;
    } else {
      while (*pt2 && (*pt2 != sepchar) && (*pt2 != ',')) pt2++;
    }
    if (*pt2 && !sepchar) {
      sepchar = *pt2;
      term = 0;
    }
    else if (*pt2)
      term = 0;
    else
      term = 1;
    *pt2 = '\0';
    toplay = lookup_player(player,pt1,0);
    if (toplay == NOTHING) {
      notify_quiet(p2,"MAIL ERROR: Bad recipient in send");
    }
    else {
      index = insert_msg(player, &toplay, spt, mpt, isend, atest, &aft, (int *)(lbuf4 + sizeof(int)), 
                         count * 2, chk_anon, anon_player, chk_anon2, bcctmp);
      if (index) {
	count++;
	*(int *)(lbuf4 + offct) = toplay;
	offct += sizeof(int);
	*(int *)(lbuf4 + offct) = (int)index;
	offct += sizeof(int);
        tprp_buff = tpr_buff;
	if (aft) {
          if ( Wizard(p2) && chk_anon2 )  {
             if ( mudconf.mail_hidden )
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s (AutoFor) %s%s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon ? "<HIDDEN> " : " "),
                             (chk_anon2 ? "(Anonymously)" : " ")));
             else
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s (AutoFor) %s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon2 ? "(Anonymously)" : " ")));
          } else {
             if ( mudconf.mail_hidden && Wizard(p2) )
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s %s(AutoFor)",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon ? "<HIDDEN> " : " ")));
             else
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s (AutoFor)",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             ((chk_anon2 | (mudconf.mail_hidden & chk_anon)) ? anon_player : Name(toplay))));
          }
	} else {
          if ( Wizard(p2) && chk_anon2 )  {
             if ( mudconf.mail_hidden )
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s %s%s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon ? "<HIDDEN> " : " "),
                             (chk_anon2 ? "(Anonymously)" : " ")));
             else
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s %s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon2 ? "(Anonymously)" : " ")));
          } else {
             if ( mudconf.mail_hidden && Wizard(p2) )
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s %s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)), 
                             (chk_anon ? "<HIDDEN>" : " ")));
             else
	        notify_quiet(p2,safe_tprintf(tpr_buff, &tprp_buff, "Mail: Message %s to -> %s",
                             (chk_anon ? "sent anonymously" : "sent"), 
                             ((chk_anon2 | (mudconf.mail_hidden & chk_anon)) ? anon_player : (ColorMail(p2) ? ColorName(toplay,1) : Name(toplay)))));
          }
        }
	if (count == absmaxrcp) term = 1;
      }
    }
    if (!term)
      pt1 = pt2+1;
  }
  free_lbuf(tpr_buff);
  free_lbuf(bcctmp);
  if (count) {
    notify_quiet(p2,"Mail: Done");
    *(int *)lbuf4 = (int)count;
    *(int *)sbuf1 = MIND_HSND;
    *(int *)(sbuf1 + sizeof(int)) = player;
    *(short int *)(sbuf1 + (sizeof(int) << 1)) = isend;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    infodata.dptr = lbuf4;
    infodata.dsize = offct;
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
    *(int *)sbuf1 = MIND_ISND;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    infodata.dptr = lbuf2;
    infodata.dsize = sizeof(short int) * (3 + *(short int *)(lbuf2+sizeof(short int)) + *(short int *)(lbuf2+2*sizeof(short int)));
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
    *(int *)sbuf1 = MIND_MSG;
    *(int *)(sbuf1 + sizeof(int)) = player;
    *(short int *)(sbuf1 + (sizeof(int) << 1)) = isend;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    infodata.dptr = mpt;
    infodata.dsize = strlen(mpt) + 1;
    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
  } else {
    notify_quiet(p2,"MAIL ERROR: No valid recipients");
    return 1;
  }
  return 0;
}

char *
mail_quick_function(dbref player, char *fname, int keyval)
{
  int new, old, mark, unread, saved, count, c2;
  short int *step;
  char *ret_buff, *ret_ptr, *tpr_buff, *tprp_buff;

  if (mudstate.mail_state != 1) {
      ret_ptr = ret_buff = alloc_lbuf("mail_quick_function");
      safe_str("#-1 MAIL CURRENTLY DISABLED", ret_buff, &ret_ptr);
      return(ret_buff);
  }

  new = old = mark = unread = saved = 0;
  *quickfolder = '\0';
  if (fname_check(player,fname,0))
    fname_conv(fname,quickfolder);
  else
    notify_quiet(player,"MAIL ERROR: Bad folder in quick function");
  *(int *)sbuf1 = MIND_WRTM;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  folder_current(player,0,NOTHING,32769);
  ret_ptr = ret_buff = alloc_lbuf("mail_quick_function");
  if (get_ind_rec(player,MIND_IRCV,lbuf1,1,NOTHING,1)) {
    step = (short int *)lbuf1;
    if (foldmast) {
      count = *step;
      step++;
    }
    else {
      count = *(step + 2);
      step += *(step + 1) + 3;
    }
    c2 = count;
    do {
      if (get_hd_rcv_rec(player,*step,mbuf1,NOTHING,1)) {
	switch (*(mbuf1+hstoff)) {
	  case 'N': new++; break;
	  case 'O': old++; break;
	  case 'M': mark++; break;
	  case 'U': unread++; break;
	  case 'S': saved++; break;
	}
      }
      step++;
      count--;
    } while (count > 0);
    
    tprp_buff = tpr_buff = alloc_lbuf("mail_quick_function");
    if ( keyval == 2 )
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d", c2), ret_buff, &ret_ptr);
    else if ( keyval == 1 )
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d", old + saved + mark, new + unread, mark), ret_buff, &ret_ptr);
    else if ( keyval == 3 )
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d", old + saved, new + unread, mark), ret_buff, &ret_ptr);
    else if ( keyval == 4 )
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%dT %dN %dU %dO %dM %dS", c2, new, unread, old, mark, saved), ret_buff, &ret_ptr);
    else
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d %d %d %d", c2, new, unread, old, mark, saved), ret_buff, &ret_ptr);
    free_lbuf(tpr_buff);
  }
  else {
    if ( keyval == 2 )
       safe_str("0", ret_buff, &ret_ptr);
    else if ( (keyval == 1) || (keyval == 3) )
       safe_str("0 0 0", ret_buff, &ret_ptr);
    else if ( keyval == 4 )
       safe_str("0T 0N 0U 0O 0M 0S", ret_buff, &ret_ptr);
    else
       safe_str("0 0 0 0 0 0", ret_buff, &ret_ptr);
  }

  *quickfolder = '\0';
  return(ret_buff);
}

void mail_quick(dbref player, char *fname)
{
  int new, old, mark, unread, saved, count, c2;
  short int *step, nmsg, bsize;
  char *tpr_buff, *tprp_buff;

  new = old = mark = unread = saved = 0;
  *quickfolder = '\0';
  if (fname_check(player,fname,0))
    fname_conv(fname,quickfolder);
  else
    notify_quiet(player,"MAIL ERROR: Bad folder in quick command");
  bsize = get_box_size(player);
  nmsg = count_all_msg(player, lbuf1, NOTHING, 1);
  if (nmsg >= bsize)
    notify_quiet(player,"Mail Warning: Your mail box is full");
  *(int *)sbuf1 = MIND_WRTM;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    notify_quiet(player,"Mail: You have a mail/write message in progress");
  folder_current(player,0,NOTHING,1);
  if (get_ind_rec(player,MIND_IRCV,lbuf1,1,NOTHING,1)) {
    step = (short int *)lbuf1;
    if (foldmast) {
      count = *step;
      step++;
    }
    else {
      count = *(step + 2);
      step += *(step + 1) + 3;
    }
    c2 = count;
    do {
      if (get_hd_rcv_rec(player,*step,mbuf1,NOTHING,1)) {
	switch (*(mbuf1+hstoff)) {
	  case 'N': new++; break;
	  case 'O': old++; break;
	  case 'M': mark++; break;
	  case 'U': unread++; break;
	  case 'S': saved++; break;
	}
      }
      step++;
      count--;
    } while (count > 0);
    tprp_buff = tpr_buff = alloc_lbuf("mail_quick");
    notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "Mail: %d Total - %d new, %d unread, %d old, %d marked, %d saved mail message(s)",
	c2, new, unread, old, mark, saved));
    if (mudconf.maildelete) {
      new = mail_md2(player, player, 1);
      if (new > 0) {
        tprp_buff = tpr_buff;
	notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "MAIL WARNING: You have %d message(s) for auto-deletion",new));
      }
    }
    free_lbuf(tpr_buff);
  } else {
    notify_quiet(player,"Mail: You have no mail");
  }
  *quickfolder = '\0';
}

void mail_status(dbref player, char *buf, dbref wiz, int key, int type, char *out_buff, char *out_buffptr)
{
  short int msize, x, y, min, max;
  short int *pt1, *pt2, mlen, nmsg, bsize;
  int w1, len, i, j, page, ml_type, subjsearch, mint_current, i_key,
      itrash, chk_anon, chk_anon2, i_autodelete, mt_flags, i_offset;
  char *pt3, *mail_current, curr_mpos, *tpr_buff, *tprp_buff, *plr_pt1, *s_namebuf;
  dbref player2, pcheck, pcheck2, ptrash, mt_owner;
  double tmp_mval;
  static char anon_player[17];

  i_key = 0;
  if ( (key & M_ANON) && Wizard(player) ) {
     i_key = 1;
     key &= ~M_ANON;
  }

  tmp_mval = 0;
  i_autodelete = 0;
  memset(anon_player, 0, sizeof(anon_player));
  if (key)
    player2 = player;
  else
    player2 = wiz;
  page = 0;
  subjsearch = 0;
  pcheck2 = NOTHING;
  msize = get_box_size(player);	

  s_namebuf = alloc_lbuf("mail_status_namebuf");

  *quickfolder = '\0';
  if ( *buf == '@' ) {
     if (fname_check(player,buf+1,0)) {
       fname_conv(buf+1,quickfolder);
     } else {
       notify_quiet(player,"MAIL ERROR: Bad folder in status function");
     }
  }

  if ( type )
     folder_current(player,0,wiz,key);
  else
     folder_current(player,0,wiz,(key|32769));
  if (get_ind_rec(player,MIND_IRCV,lbuf1,1,wiz,key)) {
    pt1 = (short int *)lbuf1;
    if (foldmast)
      y = *pt1;
    else
      y = *(pt1+2);
    ml_type = 31;
    if (!*buf) {
      min = 1;
      max = y;
    } else if ( toupper(*buf) == 'U' ) {
       ml_type = 1;
       min = 1;
       max = y;
    } else if ( toupper(*buf) == 'N' ) {
       ml_type = 2;
       min = 1;
       max = y;
    } else if ( toupper(*buf) == 'B' ) {
       ml_type = 3;
       min = 1;
       max = y;
    } else if ( toupper(*buf) == 'O' ) {
       ml_type = 4;
       min = 1;
       max = y;
    } else if ( toupper(*buf) == 'S' ) {
       ml_type = 8;
       min = 1;
       max = y;
    } else if ( toupper(*buf) == 'M' ) {
       ml_type = 16;
       min = 1;
       max = y;
    } else if ( *buf == '/' ) {
       subjsearch = 1;
       min = 1;
       max = y;
    }
    else if (toupper(*buf) == 'P') {
      if (!is_number(buf+1)) {
        if ( type )
	   notify_quiet(player2,"MAIL ERROR: Bad paging value given");
        *quickfolder = '\0';
        free_lbuf(s_namebuf);
	return;
      }
      page = atoi(buf+1);
      i = page;
      if ((i > absmaxinx) || (i < 1)) {
        if ( type )
	   notify_quiet(player2,"MAIL ERROR: Bad paging value given");
        *quickfolder = '\0';
        free_lbuf(s_namebuf);
	return;
      }
      *(int *)sbuf1 = MIND_PAGE;
      *(int *)(sbuf1 + sizeof(int)) = player2;
      keydata.dptr = sbuf1;
      keydata.dsize = sizeof(int) << 1;
      infodata = dbm_fetch(mailfile,keydata);
      if (!infodata.dptr) {
        if ( type )
	   notify_quiet(player2,"MAIL ERROR: Bad paging value given");
        *quickfolder = '\0';
        free_lbuf(s_namebuf);
	return;
      }
      memcpy(sbuf2,infodata.dptr,infodata.dsize);
      x = *(short int *)sbuf2;
      j = i * x;
      i = j - x + 1;
      if (i > y) {
        if ( type )
	   notify_quiet(player2,"MAIL ERROR: Bad paging value given");
        *quickfolder = '\0';
        free_lbuf(s_namebuf);
	return;
      }
      min = (short int)i;
      max = (short int)j;
      if (max > y)
	max = y;
    }
    else if (*buf == '*') {
      min = 1;
      max = y;
      pcheck2 = lookup_player(player,buf+1,0);
    }
    else if (*buf == '#') {
      min = 1;
      max = y;
      pcheck2 = lookup_player(player,buf,0);
    }
    else {
      min = 1;
      max = y;
      pt3 = strchr(buf,'-');
      if (pt3) {
	*pt3 = '\0';
	if (is_number(buf) && is_number(pt3+1)) {
	  x = (short int)atoi(buf);
	  if ((x > min) && (x <= max))
	    min = x;
	  x = (short int)atoi(pt3+1);
	  if ((x < max) && (x >= min))
	    max = x;
        } else if ( !*pt3 && is_number(buf) ) {
	   x = (short int)atoi(buf);
           if ( (x >= min) && (x <= max) ) {
              min = x;
           }
	} else if ( !*buf && is_number(pt3+1) ) {
	   x = max - (short int)atoi(pt3+1) + 1;
           if ( (x >= min) && (x <= max) ) {
              min = x;
           }
        }
      } else if ( (*buf == '+') && is_number(buf+1) ) {
	x = min + (short int)atoi(buf+1);
        if ( (x >= min) && (x <= max) ) {
           min = x;
        } else if ( x >= max ) {
           min = max;
        }
      } else if (is_number(buf)) {
	x = (short int)atoi(buf);
	if ((x >= min) && (x <= max))
	  min = max = x;
      }
    }
    if (y) {
      if (foldmast)
	pt2 = pt1 + 1;
      else
        pt2 = pt1 + *(pt1+1) + 3;
      if (msize < 100) w1 = 2;
      else if (msize < 1000) w1 = 3;
      else if (msize < 10000) w1 = 4;
      else w1 = 5;
      for (x = 0; x < 78; x++)
	*(mbuf1+x) = '-';
      *(mbuf1+x) = '\0';
      if (page) {
        if ( type )
	   notify_quiet(player2,unsafe_tprintf("Mail: Page -> %d",page));
      }
      if ( type )
         notify_quiet(player2,mbuf1);
      pt2 += min - 1;
      mail_current = atr_get(player, A_MCURR, &ptrash, &itrash);
      mint_current = 0;
      if (is_number(mail_current)) {
         mint_current = atoi(mail_current);
      } 
      free_lbuf(mail_current);
      if ( (mint_current < 0) || (mint_current > max) )
         mint_current = 0;
      curr_mpos = ' ';
      if ( strlen(mudconf.mail_anonymous) ) {
         strncpy(anon_player, mudconf.mail_anonymous, 16);
      } else {
         strcpy(anon_player, "*Anonymous*");
      }
      tprp_buff = tpr_buff = alloc_lbuf("mail_status");
      for (x = min; x <= max; x++, pt2++) {
        curr_mpos = ' ';
        if (x == mint_current)
           curr_mpos = '-';
	if ((len = get_hd_rcv_rec(player,*pt2,mbuf2,wiz,key))) {
	  pcheck = *(int *)mbuf2;
	  if ((pcheck2 != NOTHING) && (pcheck2 != pcheck)) continue;
          plr_pt1 = atr_get(player, A_MTIME, &mt_owner, &mt_flags);
          if (*plr_pt1) {
             tmp_mval = atof(plr_pt1);
          } else {
             if ( (mudconf.mail_autodeltime > 0) && !Wizard(player) )
                tmp_mval = (double) mudconf.mail_autodeltime;
             else
                tmp_mval = -1.0;
          }
          free_lbuf(plr_pt1);
          tmp_mval *= (double) SECDAY;
          i_autodelete = 0;
          if ( (tmp_mval >= 0) && 
               (difftime(mudstate.now,*(time_t *)(mbuf2+htimeoff)) >= tmp_mval) ) {
             i_autodelete = 1;
          }
	  strcpy(sbuf1,ctime((time_t *)(mbuf2+htimeoff)));
	  *(sbuf1 + strlen(sbuf1) - 1) = '\0';
	  mlen = *(short int *)(mbuf2+hsizeoff);

	  if (mlen == 1)
	    strcpy(sbuf2,"Byte");
	  else
	    strcpy(sbuf2,"Bytes");
          i_offset = 0;
	  if (Good_obj(pcheck)) {
            if ( ColorMail(player2) ) {
               if (len > hsuboff) {
                  i_offset = strlen(ColorName(subj_dbref(pcheck, mbuf2+hsuboff, i_key), 1)) - strlen(Name(subj_dbref(pcheck, mbuf2+hsuboff, i_key)));
	          strncpy(s_namebuf, ColorName(subj_dbref(pcheck, mbuf2+hsuboff, i_key), 1), 400);
               } else {
                  i_offset = strlen(ColorName(pcheck,1)) - strlen(Name(pcheck));
	          strncpy(s_namebuf,ColorName(pcheck,1),400);
               }
            } else {
               if ( len > hsuboff ) {
	          strncpy(s_namebuf, Name(subj_dbref(pcheck, mbuf2+hsuboff, i_key)), 22);
               } else {
	          strncpy(s_namebuf,Name(pcheck),22);
               }
            }
	  } else {
	    strcpy(s_namebuf,"(NONE)");
          }
          if ( ml_type < 31 ) {
             if ( toupper(*(mbuf2+hstoff)) == 'U' ) {
                if ( !(ml_type & 1) )
                   continue;
             } else if ( toupper(*(mbuf2+hstoff)) == 'N' ) {
                if ( !(ml_type & 2) )
                   continue;
             } else if ( toupper(*(mbuf2+hstoff)) == 'O' ) {
                if ( !(ml_type & 4) )
                   continue;
             } else if ( toupper(*(mbuf2+hstoff)) == 'S' ) {
                if ( !(ml_type & 8) )
                   continue;
             } else if ( toupper(*(mbuf2+hstoff)) == 'M' ) {
                if ( !(ml_type & 16) )
                   continue;
             } else
                continue;
          }
          if ( subjsearch && (len > hsuboff) && *buf && *(buf+1)) {
             if ( (strstr( mbuf2+hsuboff, buf+1 ) == NULL) )
                continue;
          } else if ( subjsearch && (len <= hsuboff) ) {
             if ( *buf && (*(buf+1) != '\0') )
                continue;
          } else if ( subjsearch )
             continue;
          chk_anon2 = 0;
          if ( type || (!type && (atoi(buf) == min) && (min == max)) ) {
             if ( *(mbuf2+2+hstoff) == 'r' ||
                  *(mbuf2+2+hstoff) == 'n' ||
                  *(mbuf2+2+hstoff) == 'f' ||
                  *(mbuf2+2+hstoff) == 'a' ) {
                if (Wizard(player2)) {
                   chk_anon = 0;
                   chk_anon2 = 1;
                } else {
                   chk_anon = 1;
                }
             } else {
                chk_anon = 0;
             }

             if ( (toupper(*(mbuf2+hstoff)) == 'S') ||
                  (toupper(*(mbuf2+hstoff)) == 'U') ||
                  (toupper(*(mbuf2+hstoff)) == 'N') )  {
                i_autodelete = 0;
             }

             tprp_buff = tpr_buff;
	     if (toupper(*(mbuf2+2+hstoff)) == 'A') {
                if ( type ) {
	           notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) AutoFwd",
		      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '), 
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2));
                } else {
                      safe_str(safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) AutoFwd",
                      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2), out_buff, &out_buffptr);
                }
	     }
	     else if (toupper(*(mbuf2+2+hstoff)) == 'F') {
                if ( type ) {
	           notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) Fwd",
		      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2));
                } else {
                      safe_str(safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) Fwd",
                      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2), out_buff, &out_buffptr);
                }
	     }
	     else if (toupper(*(mbuf2+2+hstoff)) == 'R') {
                if ( type ) {
	           notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) Reply",
		      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2));
                } else {
                      safe_str(safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s) Reply",
                      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2), out_buff, &out_buffptr);
                }
	     }
	     else {
                if ( type ) {
	           notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s)",
		      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon && 
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2));
                } else {
                      safe_str(safe_tprintf(tpr_buff, &tprp_buff, 
                      "[%c]%c(%*d)%c%cFrom: %-*s <%s> (%d %s)",
                      toupper(*(mbuf2+hstoff)), curr_mpos, w1, x, 
                      (i_autodelete ? 'm' : (chk_anon2 ? 'A' : ' ')), 
                      ((Good_obj(pcheck) && !chk_anon &&
                      (Connected(pcheck) && Chk_Cloak(pcheck,player2) && !Chk_Dark(pcheck,player2))) ? '*':' '),
                      16 + i_offset, (chk_anon ? anon_player : s_namebuf), sbuf1, mlen, sbuf2), out_buff, &out_buffptr);
                }
	     }
             tprp_buff = tpr_buff;
	     if (len > hsuboff) {
                if ( type ) {
	           notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, 
                        "%*s %s",w1+13,"Subj:",subj_string(player2, mbuf2+hsuboff)));
                } else {
                   safe_str(safe_tprintf(tpr_buff, &tprp_buff, 
                        "\n%*s %s", w1+13, "Subj:", subj_string(player2, mbuf2+hsuboff)), out_buff, &out_buffptr);
                }
             }
          }  else {
             tprp_buff = tpr_buff;
             safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d ", x), out_buff, &out_buffptr);
          } /* Type */
	}
      }
      free_lbuf(tpr_buff);
      if ( type )
         notify_quiet(player2,mbuf1);
      bsize = get_box_size(player);
      nmsg = count_all_msg(player, lbuf1, wiz, key);
      if (nmsg >= bsize) {
        if ( type )
           notify_quiet(player,"Mail Warning: Your mail box is full");
      }
      if ( type ) {
         notify_quiet(player2,"Mail: Done");
      }
    } else {
      if (key) {
        if ( type )
           notify_quiet(player,"Mail: You have no mail");
      } else {
        if ( type )
	   notify_quiet(player2,"Mail: That person has no mail");
      }
    }
  } else {
    if (key ) {
      if ( type )
         notify_quiet(player,"Mail: You have no mail");
    } else {
      if ( type )
         notify_quiet(player2,"Mail: That person has no mail");
    }
  }
  *quickfolder = '\0';
  free_lbuf(s_namebuf);
}

void mail_readall(dbref player, char *buf, dbref wiz, int key, int i_type, int i_offload)
{
   char *retval, *s_tmp;
   int a, b, c, d, i_new, i_unread, i_loop, i_chk; 

   if ( i_offload ) {
      key |= 32;
   }
   retval = (char *)mail_quick_function(player, "", 0); 
   sscanf(retval, "%d %d %d %d %d %d", &a, &i_new, &i_unread, &b, &c, &d);
   i_chk = 0;
   if ( i_type == 4 ) {
      if ( strchr(buf, '-') != NULL ) {
         i_unread = atoi(buf);
         i_new = atoi(strchr(buf, '-')+1);
         if ( ((i_unread > 0) && (i_unread <= a) && (i_unread < i_new)) &&
              ((i_new > 0) && (i_new <= a)) ) {
            s_tmp = alloc_sbuf("mail_readall");
            for (i_loop = i_unread; i_loop <= i_new; i_loop++) {
               sprintf(s_tmp, "%d", i_loop);
               mail_read(player, s_tmp, wiz, key);
            }
            free_sbuf(s_tmp);
         } else {
            i_type = 0;
         }
      } else {
         i_type = 0;
      }
   } else if ( i_type == 8 ) {
      s_tmp = alloc_sbuf("mail_readall");
      for ( i_loop = 1; i_loop <= a; i_loop ++ ) {
         i_chk++;
         sprintf(s_tmp, "%d", i_loop);
         mail_read(player, s_tmp, wiz, key);
      }
      if ( !i_chk ) {
         notify_quiet(player,"Mail: You have no mail");
      }
      free_sbuf(s_tmp);
   } else {
      if ( (i_type == 1) || (i_type == 3) ) {
         for (i_loop = 0;i_loop < i_new; i_loop++)
            mail_read(player, (char *)"new", wiz, key);
         i_chk = i_loop;
      } 
      if ( (i_type == 2) || (i_type == 3) ) {
         for (i_loop = 0;i_loop < i_unread; i_loop++)
            mail_read(player, (char *)"unread", wiz, key);
         i_chk += i_loop;
      }
      if ( i_chk < 1 )
         notify_quiet(player,"Mail: You have no new/unread mail");
   }
   if ( (i_type < 1) || (i_type > 8) ) {
      notify_quiet(player,"MAIL ERROR: Bad message number specified");
   }

   free_lbuf(retval);
}

void mail_read_func(dbref player, char *buf, dbref wiz, char *s_type, int key, char *buff, char **bufcx)
{
  int mesg, len, send, x, i_type, chk_anon, chk_anon2;
  short int index, size, *spt1, ind2;
  char *pt1, test, *tpr_buff, *tprp_buff;
  dbref player2, plrdb;
  static char anon_player[17], s_status[10];

  if ((!stricmp(buf,"nall") || !stricmp(buf,"uall") ||
       !stricmp(buf,"ball"))) {
     safe_str("#-1 FEATURE NOT SUPPORTED IN FUNCTION FORMAT", buff, bufcx);
     return;
  }

  mesg = len = send = x = i_type = chk_anon = chk_anon2 = 0;
  memset(anon_player, 0, sizeof(anon_player));
  memset(s_status, 0, sizeof(s_status));

  player2 = player;
  if ( !stricmp(buf,"new") || !stricmp(buf,"unread") || !stricmp(buf, "both")) {
    if (toupper(*buf) == 'N')
      test = 'N';
    else if (toupper(*buf) == 'U')
      test = 'U';
    else
      test = 'B';
    if (get_ind_rec(player,MIND_IRCV,lbuf1,1,wiz,key)) {
      spt1 = (short int *)lbuf1;
      if (foldmast) {
	size = *spt1;
	spt1++;
      }
      else {
        size = *(spt1+2);
        spt1 += 3 + *(spt1+1);
      }
      for (index = 0; index < size; index++,spt1++) {
	if (get_hd_rcv_rec(player,*spt1,mbuf1,wiz,key)) {
          if ( test == 'B' ) {
             if ( (*(mbuf1+hstoff) == 'N') || (*(mbuf1+hstoff) == 'U') ) {
               break;
             }
          } else {
	     if (*(mbuf1+hstoff) == test) {
	       break;
             }
          }
	}
      }
      if (index == size)
	index = 0;
      else {
	mesg = index+1;
	index = *spt1;
      }
    }
    else
      index = 0;
    if (!index) {
      if (test == 'N') 
	safe_str("#-1 YOU HAVE NO NEW MAIL", buff, bufcx);
      else if ( test == 'U')
	safe_str("#-1 YOU HAVE NO UNREAD MAIL", buff, bufcx);
      else
	safe_str("#-1 YOU HAVE NO NEW OR UNREAD MAIL", buff, bufcx);
      return;
    }
  }
  else {
    mesg = atoi(buf);
    if (mesg > 0)
      index = get_msg_index(player,mesg,1, NOTHING, 1);
    if ((mesg < 1) || (!index)) {
      safe_str("#-1 BAD MESSAGE NUMBER SPECIFIED", buff, bufcx);
      return;
    }
  }
  if ((len = get_hd_rcv_rec(player,index,mbuf1,wiz,key))) {
    ind2 = *(short int *)(mbuf1+hindoff);
    if (ind2 & tomask) {
      toall = 1;
      ind2 &= ~tomask;
    }
    else {
      toall = 0;
    }
    if (key)
      atr_add_raw(player, A_MCURR, myitoa(mesg));
    send = *(int *)mbuf1;
    *(int *)sbuf1 = MIND_MSG;
    *(int *)(sbuf1 + sizeof(int)) = send;
    *(short int *)(sbuf1 + (sizeof(int) << 1)) = ind2;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    infodata = dbm_fetch(mailfile,keydata);
    chk_anon2 = 0;
    if ( (*(mbuf1 + 1 +hstoff) == 'a') ||
         (*(mbuf1 + 1 +hstoff) == 'r') ||
         (*(mbuf1 + 1 +hstoff) == 'f') ||
         (*(mbuf1 + 1 +hstoff) == 'o') ||
         (*(mbuf1 + 1 +hstoff) == 'n') ) {
       if ( Wizard(player2) ) {
          chk_anon2 = 1;
       } else {
          chk_anon = 1;
       }
       if ( strlen(mudconf.mail_anonymous) )
          strncpy(anon_player, mudconf.mail_anonymous, 16);
       else
          strcpy(anon_player, "*Anonymous*");
    } else {
       chk_anon = 0;
    }
    if (infodata.dptr) {
      strcpy(lbuf8,infodata.dptr);
      for (x = 0; x < 78; x++)
	*(mbuf2 + x) = '-';
      *(mbuf2 + x) = '\0';
      switch (*(mbuf1+hstoff)) {
        case 'n':
	case 'N': strcpy(s_status,"NEW"); 
		  *(mbuf1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  else
		     *(mbuf1+1+hstoff) = 'O';
		  break;
        case 'u':
	case 'U': strcpy(s_status,"UNREAD"); 
		  *(mbuf1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  else
		     *(mbuf1+1+hstoff) = 'O';
		  break;
        case 'o':
	case 'O': strcpy(s_status,"OLD"); 
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  break;
        case 'm':
	case 'M': strcpy(s_status,"MARKED"); 
		  if ((*(mbuf1+1) == 'N') || (*(mbuf1+1) == 'U'))
		    *(mbuf1+1+hstoff) = 'O';
                  if (*(mbuf1+1) == 'n')
		    *(mbuf1+1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
		  break;
        case 's':
	case 'S': strcpy(s_status,"SAVED");
		  if ((*(mbuf1+1) == 'N') || (*(mbuf1+1) == 'U'))
		    *(mbuf1+1+hstoff) = 'O';
                  if (*(mbuf1+1) == 'n')
		    *(mbuf1+1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
		  break;
      }
      chk_anon2 = 0;
      if ( (*(mbuf1 + 2 +hstoff) == 'a') ||
           (*(mbuf1 + 2 +hstoff) == 'r') ||
           (*(mbuf1 + 2 +hstoff) == 'f') ||
           (*(mbuf1 + 2 +hstoff) == 'n') ) {
         if ( Wizard(player2) ) {
            chk_anon2 = 1;
            chk_anon = 0;
         } else {
            chk_anon = 1;
         }
         if ( strlen(mudconf.mail_anonymous) )
            strncpy(anon_player, mudconf.mail_anonymous, 16);
         else
            strcpy(anon_player, "*Anonymous*");
      } else {
         chk_anon = 0;
      }

      if (toupper(*(mbuf1 + 2 + hstoff)) == 'A')
	strcpy(sbuf2,"AUTO_FORWARD");
      else if (toupper(*(mbuf1 + 2 + hstoff)) == 'F')
	strcpy(sbuf2,"FORWARD");
      else if (toupper(*(mbuf1 + 2 + hstoff)) == 'R')
	strcpy(sbuf2,"REPLY");
      else
	*sbuf2='\0';

      size = *(short int *)(mbuf1+hsizeoff);
      if (size == 1)
	strcpy(sbuf3,"Byte");
      else
	strcpy(sbuf3,"Bytes");
      strcpy(sbuf4,ctime((time_t *)(mbuf1+htimeoff)));
      *(sbuf4 + strlen(sbuf4) - 1) = '\0';
      if (len == hsuboff)
	pt1 = NULL;
      else
	pt1 = mbuf1+hsuboff;
      if (Good_obj(send) && !Recover(send) && !Going(send))
	strncpy(sbuf7,Name(send),22);
      else
	strcpy(sbuf7,"(NONE)");
      tprp_buff = tpr_buff = alloc_lbuf("mail_read");

      if ( key )
         *s_type = 'U';

      unparse_to(send,ind2,lbuf7, &plrdb, 0);
      switch ( ToUpper(*s_type) ) {
        case 'B':  /* Body of Message */
                   safe_str(lbuf8, buff, bufcx);
                   break;
        case 'I':  /* Message Index */
                   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d", mesg), buff, bufcx);
                   break;
        case 'S':  /* Subject */
                   if ( pt1 ) 
                      safe_str(pt1, buff, bufcx);
                   break;
        case 'F':  /* From Name */
                   if ( chk_anon2 ) {
                      safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s [Anonymous]", sbuf7), buff, bufcx);
                   } else if ( chk_anon ) {
                      safe_str(anon_player, buff, bufcx);
                   } else {
                      safe_str(sbuf7, buff, bufcx);
                   }
                   break;
        case 'T':  /* To Name(s) */
                   safe_str(lbuf7, buff, bufcx);
                   break;
        case 'D':  /* Date/Time */
                   safe_str(sbuf4, buff, bufcx);
                   break;
        case 'C':  /* Connect State */
                   if ( Good_obj(send) && !chk_anon && (Connected(send) && Chk_Cloak(send,player2) && !Chk_Dark(send,player2)) )
                      safe_chr('1', buff, bufcx);
                   else
                      safe_chr('0', buff, bufcx);
                   break;
        case 'G':  /* Status Flag */
                   safe_str(s_status, buff, bufcx);
                   break;
        case '?':  /* Message Type (forward/reply/normal) */
                   safe_str(sbuf2, buff, bufcx);
                   break;
        case 'K':  /* Message Size */
                   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d", size), buff, bufcx);
                   break;
        case 'U':  /* Do an update and mark as read -- no output */
                   break;
         default:  /* Help File */
                   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s%s%s%s%s%s%s%s%s%s", 
                                         "H - This Page\r\n",
                                         "I - Message Number/Index\r\n",
                                         "S - Subject\r\n",
                                         "F - From Name\r\n",
                                         "T - To Name List\r\n",
                                         "D - Date/Time\r\n",
                                         "C - Connection State (1 connect, 0 disconnect)\r\n",
                                         "G - Message Status\r\n",
                                         "? - Message Type\r\n",
                                         "K - Message Size (in bytes)\r\n",
                                         "B - Message Body\r\n",
                                         "U - Return nothing (for key type 1 updating status)"), 
                            buff, bufcx);
                   break;
      }
      free_lbuf(tpr_buff);
      if (key) {
        *(int *)sbuf1 = MIND_HRCV;
        *(int *)(sbuf1 + sizeof(int)) = player;
        *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
        keydata.dptr = sbuf1;
        keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
        infodata.dptr = mbuf1;
        infodata.dsize = len;
        dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
      }
    } else {
      safe_str("#-1 THE MAIL DATABASE HAD AN ERROR", buff, bufcx);
    }
  }
}

void mail_read(dbref player, char *buf, dbref wiz, int key)
{
  int mesg, len, send, x, i_type, chk_anon, chk_anon2, i_version, i_offset;
  short int index, size, *spt1, ind2;
  char *pt1, test, *tpr_buff, *tprp_buff, *s_mail, *s_mailptr, *s_namebuf;
  dbref player2, plrdb;
  static char anon_player[17], s_status[10];

  mesg = len = send = x = i_type = chk_anon = chk_anon2 = i_version = 0;

  if ( key & 32 ) {
    key &= ~32;
    i_version = 1;
  }

  memset(anon_player, 0, sizeof(anon_player));
  memset(s_status, 0, sizeof(s_status));
  if ((!stricmp(buf,"nall") || !stricmp(buf,"uall") ||
       !stricmp(buf,"ball") || (strchr(buf, '-') != NULL) ) && key) {
     if ( toupper(*buf) == 'N' )
       i_type = 1;
     else if ( toupper(*buf) == 'U' )
       i_type = 2;
     else if ( strchr(buf, '-') != NULL )
       i_type = 4;
     else
       i_type = 3;
     mail_readall(player, buf, wiz, key, i_type, i_version);
     return;
  }
  if ( !stricmp(buf, "+all") ) {
     i_type = 8;
     mail_readall(player, buf, wiz, key, i_type, i_version);
     return;
  }

  if (key)
    player2 = player;
  else
    player2 = wiz;
  if ((!stricmp(buf,"new") || !stricmp(buf,"unread") || !stricmp(buf, "both")) && key) {
    if (toupper(*buf) == 'N')
      test = 'N';
    else if (toupper(*buf) == 'U')
      test = 'U';
    else
      test = 'B';
    if (get_ind_rec(player,MIND_IRCV,lbuf1,1,wiz,key)) {
      spt1 = (short int *)lbuf1;
      if (foldmast) {
	size = *spt1;
	spt1++;
      }
      else {
        size = *(spt1+2);
        spt1 += 3 + *(spt1+1);
      }
      for (index = 0; index < size; index++,spt1++) {
	if (get_hd_rcv_rec(player,*spt1,mbuf1,wiz,key)) {
          if ( test == 'B' ) {
             if ( (*(mbuf1+hstoff) == 'N') || (*(mbuf1+hstoff) == 'U') ) {
               break;
             }
          } else {
	     if (*(mbuf1+hstoff) == test) {
	       break;
             }
          }
	}
      }
      if (index == size)
	index = 0;
      else {
	mesg = index+1;
	index = *spt1;
      }
    }
    else
      index = 0;
    if (!index) {
      if (test == 'N') 
	notify_quiet(player,"Mail: You have no new mail");
      else if ( test == 'U')
	notify_quiet(player,"Mail: You have no unread mail");
      else
	notify_quiet(player,"Mail: You have no new or unread mail");
      return;
    }
  } else {
    mesg = atoi(buf);
    if (mesg > 0)
      index = get_msg_index(player,mesg,1, NOTHING, 1);
    if ((mesg < 1) || (!index)) {
      notify_quiet(player2,"MAIL ERROR: Bad message number specified");
      return;
    }
  }

  s_namebuf = alloc_lbuf("read_mail_namebuf");

  if ((len = get_hd_rcv_rec(player,index,mbuf1,wiz,key))) {
    ind2 = *(short int *)(mbuf1+hindoff);
    if (ind2 & tomask) {
      toall = 1;
      ind2 &= ~tomask;
    }
    else {
      toall = 0;
    }
    if (key)
      atr_add_raw(player, A_MCURR, myitoa(mesg));
    send = *(int *)mbuf1;
    *(int *)sbuf1 = MIND_MSG;
    *(int *)(sbuf1 + sizeof(int)) = send;
    *(short int *)(sbuf1 + (sizeof(int) << 1)) = ind2;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    infodata = dbm_fetch(mailfile,keydata);
    chk_anon2 = 0;
    if ( (*(mbuf1 + 1 +hstoff) == 'a') ||
         (*(mbuf1 + 1 +hstoff) == 'r') ||
         (*(mbuf1 + 1 +hstoff) == 'f') ||
         (*(mbuf1 + 1 +hstoff) == 'o') ||
         (*(mbuf1 + 1 +hstoff) == 'n') ) {
       if ( Wizard(player2) ) {
          chk_anon2 = 1;
       } else {
          chk_anon = 1;
       }
       if ( strlen(mudconf.mail_anonymous) )
          strncpy(anon_player, mudconf.mail_anonymous, 16);
       else
          strcpy(anon_player, "*Anonymous*");
    } else {
       chk_anon = 0;
    }
    if (infodata.dptr) {
      strcpy(lbuf8,infodata.dptr);
      for (x = 0; x < 78; x++)
	*(mbuf2 + x) = '-';
      *(mbuf2 + x) = '\0';
      switch (*(mbuf1+hstoff)) {
        case 'n':
	case 'N': strcpy(s_status,"NEW"); 
		  *(mbuf1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  else
		     *(mbuf1+1+hstoff) = 'O';
		  break;
        case 'u':
	case 'U': strcpy(s_status,"UNREAD"); 
		  *(mbuf1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  else
		     *(mbuf1+1+hstoff) = 'O';
		  break;
        case 'o':
	case 'O': strcpy(s_status,"OLD"); 
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
                  break;
        case 'm':
	case 'M': strcpy(s_status,"MARKED"); 
		  if ((*(mbuf1+1) == 'N') || (*(mbuf1+1) == 'U'))
		    *(mbuf1+1+hstoff) = 'O';
                  if (*(mbuf1+1) == 'n')
		    *(mbuf1+1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
		  break;
        case 's':
	case 'S': strcpy(s_status,"SAVED");
		  if ((*(mbuf1+1) == 'N') || (*(mbuf1+1) == 'U'))
		    *(mbuf1+1+hstoff) = 'O';
                  if (*(mbuf1+1) == 'n')
		    *(mbuf1+1+hstoff) = 'O';
                  if ( chk_anon )
		     *(mbuf1+1+hstoff) = 'o';
		  break;
      }
      chk_anon2 = 0;
      if ( (*(mbuf1 + 2 +hstoff) == 'a') ||
           (*(mbuf1 + 2 +hstoff) == 'r') ||
           (*(mbuf1 + 2 +hstoff) == 'f') ||
           (*(mbuf1 + 2 +hstoff) == 'n') ) {
         if ( Wizard(player2) ) {
            chk_anon2 = 1;
            chk_anon = 0;
         } else {
            chk_anon = 1;
         }
         if ( strlen(mudconf.mail_anonymous) )
            strncpy(anon_player, mudconf.mail_anonymous, 16);
         else
            strcpy(anon_player, "*Anonymous*");
      } else {
         chk_anon = 0;
      }

      if (toupper(*(mbuf1 + 2 + hstoff)) == 'A')
	strcpy(sbuf2,"AUTO_FORWARD");
      else if (toupper(*(mbuf1 + 2 + hstoff)) == 'F')
	strcpy(sbuf2,"FORWARD");
      else if (toupper(*(mbuf1 + 2 + hstoff)) == 'R')
	strcpy(sbuf2,"REPLY");
      else
	*sbuf2='\0';

      size = *(short int *)(mbuf1+hsizeoff);
      if (size == 1)
	strcpy(sbuf3,"Byte");
      else
	strcpy(sbuf3,"Bytes");
      strcpy(sbuf4,ctime((time_t *)(mbuf1+htimeoff)));
      *(sbuf4 + strlen(sbuf4) - 1) = '\0';
      if (len == hsuboff)
	pt1 = NULL;
      else
	pt1 = mbuf1+hsuboff;

      if ( !i_version )
         notify_quiet(player2,mbuf2);

      i_offset = 0;
      if (Good_obj(send) && !Recover(send) && !Going(send)) {
	strncpy(sbuf7,Name(send),22);
        if ( ColorMail(player2) ) {
           if (pt1 && *pt1 ) {
              i_offset = strlen(ColorName(subj_dbref(send, pt1, 0), 1)) - strlen(Name(subj_dbref(send, pt1, 0)));
	      strncpy(s_namebuf, ColorName(subj_dbref(send, pt1, 0), 1), 400);
           } else {
              i_offset = strlen(ColorName(send, 1)) - strlen(Name(send));
	      strncpy(s_namebuf, ColorName(send, 1), 400);
           }
        } else {
           if (pt1 && *pt1) {
	      strncpy(s_namebuf,Name(subj_dbref(send, pt1, 0)),22);
           } else {
	      strncpy(s_namebuf,Name(send),22);
           }
        }
      } else {
	strcpy(sbuf7,"(NONE)");
	strcpy(s_namebuf,"(NONE)");
      }
      tprp_buff = tpr_buff = alloc_lbuf("mail_read");
      if ( i_version ) {
         s_mail = alloc_lbuf("read_mail_export");
         sprintf(s_mail, "%s", mudconf.mud_name);
         s_mailptr = s_mail;
         while ( *s_mailptr ) {
            if ( isspace(*s_mailptr) ) {
               *s_mailptr = '_';
            }
            s_mailptr++;
         }
	 unparse_to(send, ind2, lbuf7, &plrdb, 1);
         /* MBOX FORMAT */
         notify_quiet(player2, 
                      safe_tprintf(tpr_buff, &tprp_buff,
                                   "From %s@%s  %s\n%s\n%s\nDate: %s\nTo: %s\nSubject: %s\nUser-Agent: %s\n%s\n%s\n%s\nFrom: %s@%s\n%s\n\n%s\n\n", 
                                   (chk_anon ? anon_player : sbuf7), s_mail, sbuf4,
                                   (char *)"Reply-To: donotreply@dev.null",
                                   (char *)"Return-Path: <donotreply@dev.null>", sbuf4, lbuf7, ( pt1 ? pt1 : "(No Subject)"),
                                   (char *)"RhostMUSH mailer", (char *)"MIME-Version: 1.0",
                                   (char *)"Content-Type: text/plain; charset=us-ascii",
                                   (char *)"Content-Transfer-Encoding: 7bit", 
                                   (chk_anon ? anon_player : sbuf7), s_mail, (char *)"Status: N", lbuf8));
         free_lbuf(s_mail);
      } else {
         if (toall) {
	   unparse_to(send,ind2,lbuf7, &plrdb, 0);
           notify_quiet(player2,
                        safe_tprintf(tpr_buff, &tprp_buff, 
                                     "Message #: %d  %s\nStatus: %-11s From: %-*s%s%s\nTo: %s\nDate/Time: %-31s Size: %d %s", mesg, 
                                     ((Good_obj(send) && !chk_anon && (Connected(send) && Chk_Cloak(send,player2) && !Chk_Dark(send,player2))) ? "      (Connected)" : ""), 
                                     s_status, 16 + i_offset, (chk_anon ? anon_player : s_namebuf), (chk_anon2 ? " [Anonymous] " : " "),
                                     sbuf2, lbuf7, sbuf4, size, sbuf3));
         } else {
           notify_quiet(player2,
                        safe_tprintf(tpr_buff, &tprp_buff, 
                                     "Message #: %d  %s\nStatus: %-11s From: %-*s%s%s\nDate/Time: %-31s Size: %d %s", mesg, 
                                     ((Good_obj(send) && !chk_anon && (Connected(send) && Chk_Cloak(send,player2) && !Chk_Dark(send,player2))) ? "      (Connected)" : ""),
                                     s_status, 16 + i_offset, (chk_anon ? anon_player : s_namebuf), (chk_anon2 ? " [Anonymous] " : " "), 
                                     sbuf2, sbuf4, size, sbuf3));
         }
      }
      if (!i_version && pt1) {
        tprp_buff = tpr_buff;
	notify_quiet(player2,safe_tprintf(tpr_buff, &tprp_buff, "Subject: %s",subj_string(player2, pt1)));
      }
      free_lbuf(tpr_buff);
      if ( !i_version ) {
         notify_quiet(player2,mbuf2);
         notify_quiet(player2,lbuf8);
         notify_quiet(player2,mbuf2);
         notify_quiet(player2,"Mail: Done");
      }
      if (key) {
        *(int *)sbuf1 = MIND_HRCV;
        *(int *)(sbuf1 + sizeof(int)) = player;
        *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
        keydata.dptr = sbuf1;
        keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
        infodata.dptr = mbuf1;
        infodata.dsize = len;
        dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
      }
    }
    else
      mail_error(player2,MERR_MSG);
  }
  free_lbuf(s_namebuf);
}

int do_mark(dbref player, short int index, char *buf, int len, int key, int key2)
{
  int test;

  test = 0;
  if (key == M_READM) {
    if (*(buf+hstoff) == 'N') *(buf+hstoff) = 'U';
    if (*(buf+1+hstoff) == 'N') *(buf+1+hstoff) = 'U';
    test = 1;
  }
  else if ((key == M_UNMARK) && ((*(buf+hstoff) == 'M') || (*(buf+hstoff) == 'S'))) {
    if (*(buf+hstoff) == 'S') {
      savecur--;
      saved = 1;
    }
    *(buf+hstoff) = *(buf+1+hstoff);
    test = 1;
  }
  else if (saved && (*(buf+hstoff) != 'S')) {
    if (savecur < savemax) {
      *(buf+hstoff) = 'S';
      test = 1;
      savecur++;
    }
  }
  else if (!key2 || ((*(buf+hstoff) != 'S') && (*(buf+hstoff) != 'M') && (key != M_UNMARK))) {
    *(buf+hstoff) = 'M';
    test = 1;
  }
  if (test) {
    *(int *)sbuf1 = MIND_HRCV;
    *(int *)(sbuf1 + sizeof(int)) = player;
    *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    infodata.dptr = buf;
    infodata.dsize = len;
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
  }
  return test;
}

void mail_mark(dbref player, int key, char *buf, dbref wiz, int key2)
{
/* M_READM passed as key means make new unread, use master index, and
   don't output any messages */

  short int nmsg, *pt1, *pt2, *pt3;
  int len, count, term, low, high, x, check, i_chk;
  char *cpt1, *cpt2, *cpt3;

  if (mudstate.mail_state != 1) {
        return;
  }
  if (mudstate.mail_state != 1 ) return; /* here due to called direct from bsd.c */
  if ((key & M_SAVE) != 0) {
    saved = 1;
    key &= ~M_SAVE;
  }
  else
    saved = 0;
  if (!mudstate.nuke_status && !Wizard(player)) {
    cpt1 = atr_get(player, A_MSAVEMAX, &low, &high);
    if (*cpt1) {
      *(cpt1 + 5) = '\0';
      savemax = atoi(cpt1);
    }
    else {
      *(int *)sbuf1 = MIND_SMAX;
      keydata.dptr = sbuf1;
      keydata.dsize = sizeof(int);
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr) {
	memcpy(sbuf1,infodata.dptr,sizeof(short int));
	savemax = (int)(*(short int *)sbuf1);
      }
      else
	savemax = STARTSMAX;
      atr_add_raw(player, A_MSAVEMAX, myitoa(savemax));
    }
    if (savemax < 0)
      savemax = 0;
    free_lbuf(cpt1);
    cpt1 = atr_get(player, A_MSAVECUR, &low, &high);
    *(cpt1 + 5) = '\0';
    savecur = atoi(cpt1);
    if (savecur < 0)
      savecur = 0;
    free_lbuf(cpt1);
  }
  else {
    savemax = 999999;
    savecur = 0;
  }
  if ((key == M_READM) || !key2)
    check = get_ind_rec(player,MIND_IRCV,lbuf1,0,wiz,key2);
  else
    check = get_ind_rec(player,MIND_IRCV,lbuf1,1,wiz,key2);
  if (check) {
    pt1 = (short int *)lbuf1;
    if (foldmast) {
      pt3 = pt1 + 1;
      nmsg = *pt1;
    }
    else {
      pt3 = pt1+3+*(pt1+1);
      nmsg = *(pt1 + 2);
    }
    if (!nmsg) {
      if ((key != M_READM) && key2)
        notify_quiet(player,"Mail: You have no mail to mark");
      return;
    }
    count = 0;
    if (!stricmp(buf,"all")) {
      for (pt2 = pt3; nmsg > 0; nmsg--, pt2++) {
	if ((len = get_hd_rcv_rec(player,*pt2, mbuf1, wiz, key2))) {
	  count += do_mark(player,*pt2,mbuf1,len,key,key2);
	}
      }
    }
    else if (*buf == '*') {
      check = lookup_player(player,buf+1,0);
      if (check != NOTHING) {
	for (pt2 = pt3; nmsg > 0; nmsg--, pt2++) {
	  if ((len = get_hd_rcv_rec(player,*pt2,mbuf1, wiz, key2))) {
	    if (*(int *)mbuf1 == check) {
	      count += do_mark(player,*pt2,mbuf1,len,key,key2);
	    }
	  }
	}
      }
      else 
	notify_quiet(player,"MAIL ERROR: Bad player name given in mark");
    }
    else {
      cpt1 = buf;
      term = 0;
      pt2 = (short int *)lbuf1;
      while (!term && *cpt1){
	while (isspace((int)*cpt1) && *cpt1) cpt1++;
	if (!*cpt1) break;
	cpt2 = cpt1+1;
	while (!isspace((int)*cpt2) && *cpt2) cpt2++;
	if (!*cpt2) term = 1;
	else *cpt2 = '\0';
	cpt3 = strchr(cpt1,'-');
	if (cpt3) {
	  *cpt3 = '\0';
	  if (is_number(cpt1) && is_number(cpt3+1)) {
	    low = atoi(cpt1);
	    high = atoi(cpt3+1);
	    if (low > high) {
	      notify_quiet(player,"MAIL ERROR: Bad message number");
	    }
	    else {
              if (low < 0 ) 
	         low = 0;
              i_chk = count_all_msg(player, lbuf11, NOTHING, 1);
              if ( high > i_chk )
                 high = i_chk;
	      if ( high < 1 )
		 high = 1;
	      for (x = low; x <= high; x++) {
		if ((x > nmsg) || (x < 1)) continue;
		if ((len = get_hd_rcv_rec(player,*(pt3 + x - 1),mbuf1,NOTHING,1))) {
		  count += do_mark(player,*(pt3 + x - 1),mbuf1,len,key,key2);
		}
	      }
	    }
	  }
	  else
	    notify_quiet(player,"MAIL ERROR: Bad message number");
	}
	else {
	  if (is_number(cpt1)) {
	    x = atoi(cpt1);
	    if ((x <= nmsg) && (x > 0)) {
	      if ((len = get_hd_rcv_rec(player,*(pt3 + x - 1),mbuf1,NOTHING,1))) {
		count += do_mark(player,*(pt3 + x - 1),mbuf1,len,key,key2);
	      }
	    }
	  }
	  else
	    notify_quiet(player,"MAIL ERROR: Bad message number");
	}
	if (!term) 
	  cpt1 = cpt2+1;
      }
    }
    if ((key == M_MARK) && saved)
	strcpy(sbuf1,"saved");
    else if (key == M_MARK)
	strcpy(sbuf1,"marked");
    else
	strcpy(sbuf1,"unmarked");
    if (saved && !mudstate.nuke_status && !Wizard(player)) {
	cpt1 = myitoa(savecur);
	atr_add_raw(player,A_MSAVECUR,cpt1);
    }
    if ((key != M_READM) && key2) {
      if ( saved && (count == 0) && (savecur >= savemax) ) {
         notify_quiet(player,unsafe_tprintf("Mail: save messages at max of %d.  Could not save message.", savemax));
      } else {
         notify_quiet(player,unsafe_tprintf("Mail: %d message(s) %s",count,sbuf1));
      }
    }
  }
}

void folder_ind_del(dbref player, short int index)
{
  char *pt1, *pt2;
  int term;
  short int *pt3, x, num;

  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr) {
    strcpy(lbuf7,"Incoming ");
    strcat(lbuf7,infodata.dptr);
  }
  else
    strcpy(lbuf7,"Incoming");
  pt1 = lbuf7;
  term = 0;
  while (*pt1 && !term) {
    while(*pt1 && isspace((int)*pt1)) pt1++;
    if (!*pt1) break;
    pt2 = pt1 + 1;
    while(*pt2 && !isspace((int)*pt2)) pt2++;
    if (!*pt2) term = 1;
    else *pt2 = '\0';
    *(int *)sbuf1 = FIND_BOX;
    *(int *)(sbuf1 + sizeof(int)) = player;
    strcpy(sbuf1 + (sizeof(int) << 1),pt1);
    keydata.dptr = sbuf1;
    keydata.dsize = 1 + (sizeof(int) << 1) + strlen(pt1);
    infodata = dbm_fetch(foldfile,keydata);
    if (infodata.dptr) {
      memcpy(lbuf12,infodata.dptr,infodata.dsize);
      pt3 = (short int *)lbuf12;
      num = *pt3;
      for(x=0;x<num;x++) {
	if (*(pt3+1+x) == index) break;
      }
      if (x < num) {
	if (num == 1)
	  dbm_delete(foldfile,keydata);
	else {
	  if (x < num-1) 
	    memmove(pt3+1+x,pt3+1+x+1,sizeof(short int) * (num - x + 1));
	  (*pt3)--;
	  infodata.dptr = lbuf12;
	  infodata.dsize -= sizeof(short int);
	  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
	}
      }
    }
    if (!term)
      pt1 = pt2+1;
  }
}

void mail_delete(dbref player, int later1, int later2, dbref wiz, int key, int adel, int i_verbose)
{
  short int *pt1, *pt3, nmsg, nfree, top, icheck, sind;
  short int sndtop, sndfree, sndmsg, *spt1, *spt2, ind2;
  int change, x, y, len, sref, len2, cnt1, cnt2, nsnd;
  char *cpt1, dcheck;

  if (get_ind_rec(player,MIND_IRCV,lbuf1,0,wiz,key)) {
    if (adel) 
      dcheck = 'P';
    else
      dcheck = 'M';
    change = 0;
    pt1 = (short int *)lbuf1;
    top = *pt1;
    nfree = *(pt1+1);
    nmsg = *(pt1+2);
    memset(lbuf2,0,absmaxinx+1);
    for (x = 0; x < nfree; x++) 
      *(lbuf2 + *(pt1 + 3 + x)) = 1;
    pt3 = (short int *)lbuf3;
    for (y = 0; y < nmsg; y++) {
      icheck = *(pt1+3+nfree+y);
      if ((len = get_hd_rcv_rec(player,icheck,mbuf1,wiz,key))) {
	if (*(mbuf1+hstoff) == dcheck) {
	  ind2 = *(short int *)(mbuf1+hindoff);
	  ind2 &= ~tomask;
	  folder_ind_del(player,icheck);
	  change++;
	  sref = *(int *)mbuf1;
	  sind = ind2;
	  if ((len2 = get_hd_snd_rec(sref,sind,lbuf7,wiz,key))) {
	    nsnd = *(int *)lbuf7;
	    if (nsnd == 1) {
	      *(int *)sbuf1 = MIND_HSND;
	      *(int *)(sbuf1 + sizeof(int)) = sref;
	      *(short int *)(sbuf1 + (sizeof(int) << 1)) = sind;
	      keydata.dptr = sbuf1;
	      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	      dbm_delete(mailfile,keydata);
	      *(int *)sbuf1 = MIND_MSG;
	      *(int *)(sbuf1 + sizeof(int)) = sref;
	      *(short int *)(sbuf1 + (sizeof(int) << 1)) = sind;
	      keydata.dptr = sbuf1;
	      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	      dbm_delete(mailfile,keydata);
	      if (get_ind_rec(sref,MIND_ISND,lbuf4,0,wiz,key)) {
		spt1 = (short int *)lbuf4;
		sndmsg = *(spt1+2);
		*(int *)sbuf1 = MIND_ISND;
		*(int *)(sbuf1 + sizeof(int)) = sref;
		keydata.dptr = sbuf1;
		keydata.dsize = sizeof(int) << 1;
		if (sndmsg == 1) {
		  dbm_delete(mailfile,keydata);
		}
		else {
		  sndtop = *spt1;
		  sndfree = *(spt1+1);
		  memset(lbuf5,0,absmaxinx+1);
		  for (x = 0; x < sndfree; x++)
		    *(lbuf5 + *(spt1 + 3 + x)) = 1;
		  *(lbuf5 + sind) = 1;
		  for (x = sndtop; x > 2; x--) {
		    if (!*(lbuf5 + x - 1)) break;
		    else sndtop--;
		  }
		  if (x == 2) sndtop = 2;
		  spt2 = (short int *)lbuf6;
		  for (x = 0; x < sndmsg; x++) {
		    if (*(spt1 + 3 + sndfree + x) != sind) {
		      *spt2 = *(spt1 + 3 + sndfree + x);
		      spt2++;
		    }
		  }
		  *spt1 = sndtop;
		  cnt2 = 3;
		  for (x = sndtop - 1; x > 0; x--) {
		    if (*(lbuf5 + x)) {
		      *(spt1+cnt2) = (short int)x;
		      cnt2++;
		    }
		  }
		  *(spt1+1) = (short int)(cnt2 - 3);
		  *(spt1+2) = sndmsg - 1;
		  spt1 += cnt2;
		  memcpy(spt1,lbuf6,(sndmsg - 1)*sizeof(short int));
		  infodata.dptr = lbuf4;
		  infodata.dsize = sizeof(short int) * (cnt2 + sndmsg - 1);
		  dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
		}
	      }
	      else {
		if (key)
		  mail_error(player,MERR_ISND);
		else
		  mail_error(wiz,MERR_ISND);
	      }
	    }
	    else {
	      (*(int *)lbuf7)--;
	      cpt1 = lbuf7 + sizeof(int);
	      for (x = 0; x < nsnd; x++) {
		if ((*(int *)cpt1 == player) && ((short int)(*(int *)(cpt1 + sizeof(int))) == icheck)) {
		  if (x < (nsnd-1)) {
		    memmove(cpt1,cpt1+(sizeof(int) << 1),(sizeof(int) << 1) * (nsnd - x - 1));
		  }
		  break;
		}
		else {
		  cpt1 += (sizeof(int) << 1);
		}
	      }
	      *(int *)sbuf1 = MIND_HSND;
	      *(int *)(sbuf1 + sizeof(int)) = sref;
	      *(short int *)(sbuf1 + (sizeof(int) << 1)) = sind;
	      keydata.dptr = sbuf1;
	      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	      infodata.dptr = lbuf7;
	      infodata.dsize = len2 - (sizeof(int) << 1);
	      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	    }
	  }
	  *(int *)sbuf1 = MIND_HRCV;
	  *(int *)(sbuf1 + sizeof(int)) = player;
	  *(short int*)(sbuf1 + (sizeof(int) << 1)) = icheck;
	  keydata.dptr = sbuf1;
	  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	  dbm_delete(mailfile,keydata);
	  *(lbuf2 + icheck) = 1;
	}
	else 
	  *pt3++ = icheck;
      }
    }
    if (change == nmsg) {
      *(int *)sbuf1 = MIND_IRCV;
      *(int *)(sbuf1 + sizeof(int)) = player;
      keydata.dptr = sbuf1;
      keydata.dsize = sizeof(int) << 1;
      dbm_delete(mailfile,keydata);
    }
    else if (change) {
      for (x = top; x > 2; x--) {
        if (!*(lbuf2 + x - 1)) break;
        else top--;
      }
      if (x == 2) top = 2;
      *pt1 = top;
      cnt1 = 3;
      for (x = top - 1; x > 0; x--) {
        if (*(lbuf2 + x)) {
          *(pt1+cnt1) = (short int)x;
          cnt1++;
        }
      }
      *(pt1+1) = (short int)(cnt1 - 3);
      *(pt1+2) = (short int)(nmsg - change);
      memcpy(pt1 + cnt1,lbuf3,sizeof(short int) * (nmsg-change));
      *(int *)sbuf1 = MIND_IRCV;
      *(int *)(sbuf1 + sizeof(int)) = player;
      keydata.dptr = sbuf1;
      keydata.dsize = sizeof(int) << 1;
      infodata.dptr = lbuf1;
      infodata.dsize = sizeof(short int) * (cnt1 + nmsg - change);
      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
    }
    else if ((!adel) && key)
      notify_quiet(player,"MAIL ERROR: You have no mail marked for deletion");
    if (change)  {
      if (key) {
        if ( i_verbose && Good_chk(player) ) {
           notify_quiet(player,unsafe_tprintf("Mail: %d message(s) deleted [%s]", change, Name(player)));
        } else {
           notify_quiet(player,unsafe_tprintf("Mail: %d message(s) deleted\r\nMail: Done",change));
        }
      } else {
        if ( i_verbose && Good_chk(player) ) {
           notify_quiet(wiz,unsafe_tprintf("Mail: %d message(s) deleted [%s]", change, Name(player)));
        } else {
           notify_quiet(wiz,unsafe_tprintf("Mail: %d message(s) deleted\r\nMail: Done",change));
        }
      }
    }
  }
  else if ((!adel) && key)
    notify_quiet(player,"MAIL ERROR: You have no mail to delete");
}

void 
write_assemble(dbref player, char *buf1, short int num)
{
    short int count;
    int stripret;

    *buf1 = '\0';
    if ( MailStripRet(player) )
       stripret = 1;
    else
       stripret = 0;
    if (num < 1) {
	notify_quiet(player, "Mail: No write in progress.");
    } else {
	for (count = 1; count <= num; count++) {
	    *(int *)sbuf1 = MIND_WRTL;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    *(short int*)(sbuf1 + (sizeof(int) << 1)) = count;
	    keydata.dptr = sbuf1;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	    infodata = dbm_fetch(mailfile, keydata);
	    if (!infodata.dptr) {
		notify_quiet(player, "MAIL ERROR: Write line missing.");
	    } else {
		if ((strlen(buf1) + strlen(infodata.dptr) + 4) > LBUF_SIZE) {
		    notify_quiet(player, "MAIL ERROR: Write message too long...truncating.");
		    break;
		} else {
		    if (*buf1 != '\0') {
                        if ( stripret )
                           strcat(buf1, " ");
                        else
			   strcat(buf1, "\r\n");
		    }
		    strcat(buf1, infodata.dptr);
		}
	    }
	}
    }
}

void 
write_wipe(dbref player, short int num, int flag)
{
    short int count;

    for (count = 1; count <= num; count++) {
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	dbm_delete(mailfile, keydata);
    }
    *(int *)sbuf1 = MIND_WRTM;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    dbm_delete(mailfile, keydata);
    *(int *)sbuf1 = MIND_WRTS;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    dbm_delete(mailfile, keydata);
    if (flag) {
	notify_quiet(player, "Mail: Write message forgotten.");
    }
}

void 
write_del(dbref player, short int dline, short int num)
{
    short int count;

    for (count = dline; count < num; count++) {
	*lbuf1 = '\0';
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = count + 1;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	infodata = dbm_fetch(mailfile, keydata);
	if (!infodata.dptr) {
	    notify_quiet(player, "MAIL ERROR: Write line missing.");
	} else {
	    strcpy(lbuf1, infodata.dptr);
	}
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	infodata.dptr = lbuf1;
	infodata.dsize = strlen(lbuf1) + 1;
	dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
    }
    *(int *)sbuf1 = MIND_WRTM;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    *(short int *)(sbuf7) = num - 1;
    infodata.dptr = sbuf7;
    infodata.dsize = sizeof(short int);
    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
}

void 
write_insert(dbref player, char *buf1, short int line, short int num)
{
    short int count;

    if (line == MAXWRTL) {
	notify_quiet(player,"MAIL ERROR: Maximum number of write lines reached");
	return;
    }
    if (num > line) {
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = line+1; 
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	infodata.dptr = buf1;
	infodata.dsize = strlen(buf1) + 1;
	dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
    } else {
	for (count = line; count >= num; count--) {
	    *(int *)sbuf1 = MIND_WRTL;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
	    keydata.dptr = sbuf1;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	    infodata = dbm_fetch(mailfile, keydata);
	    *(int *)sbuf1 = MIND_WRTL;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = count+1;
	    keydata.dptr = sbuf1;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	    if (!infodata.dptr) {
		notify_quiet(player, "MAIL ERROR: Write line missing.");
		dbm_delete(mailfile, keydata);
	    } else {
		strcpy(lbuf1, infodata.dptr);
		infodata.dptr = lbuf1;
		infodata.dsize = strlen(lbuf1) + 1;
		dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
	    }
	}
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = num;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	infodata.dptr = buf1;
	infodata.dsize = strlen(buf1) + 1;
	dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
    }
    count = line + 1;
    *(int *)sbuf1 = MIND_WRTM;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    *(short int *)(sbuf7) = count;
    infodata.dptr = sbuf7;
    infodata.dsize = sizeof(short int);
    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
}

char *
mail_funkvar(char *instr)
{
   char *outstr, *outstrptr, *instrptr;
   
   outstrptr = outstr = alloc_lbuf("mailfunk.mail");
   instrptr = instr;
   
   while ( *instrptr ) {
      if ( *instrptr == '\t' ) {
         safe_str("%t", outstr, &outstrptr);
         instrptr++;
         continue;
      }
      if ( *instrptr == '\n' ) {
         safe_str("%r", outstr, &outstrptr);
         instrptr++;
         continue;
      }
      if ( *instrptr < 32 || *instrptr > 126 ) {
         instrptr++;
         continue;
      }
      safe_chr(*instrptr, outstr, &outstrptr);
      instrptr++;
   }
   return outstr;
}

void 
mail_write(dbref player, int key, char *buf1, char *buf2)
{
    char *p1, *p2, *p3, *p4, *atrxxx, *atryyy, *tcim, *tcimptr, *tcimptr2, msubj[SUBJLIM+1];
    char just, *atrtmp, *atrtmpptr, *mailfunkvar, *ztmp, *ztmpptr, *time_tmp;
    char *bcctmp, *bcctmpptr, *bccatr, *tpr_buff, *tprp_buff, *tstrtokr;
    char *s_to, *s_toptr, *s_bcc, *s_bccptr;
//  char *savesend;
    short int line, count, index, min, max, gdcount;
    dbref aowner3, aowner2, owner;
    int aflags2, chk_dash, flags, i_type, valid_flag, type_two, type_three, is_first, i_addkey;
    int aflags3, i_bccacc, no_writie, i_typeval, acheck;
    CMDENT *cmdp;

    p4 = NULL;
    index = 0;
    i_type = i_typeval = 0;
    if ( Good_obj(player) && !isPlayer(player) ) {
       notify(player, "Mail: Only players may use this feature.");
       return;
    }
    i_addkey = 0;
    *(int *)sbuf1 = MIND_WRTM;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    infodata = dbm_fetch(mailfile, keydata);
    if (!infodata.dptr) {
	if (key & M_ALL) {
#ifdef RHOSTSHYL	/* This is here due to - mail/write processing */
	  mudstate.write_status = 0;
#else
	  notify(player,"Huh?  (Type \"help\" for help.)");
#endif
	  return;
	}
	line = 0;
    } else {
	memcpy(sbuf7,infodata.dptr,sizeof(short int));
	line = *(short int *)(sbuf7);
    }
    p3 = mystrchr(buf1, ' ');
    if (p3 != NULL) {
	*p3 = '\0';
    }
    chk_dash = 0;
    atrxxx = atr_get(player, A_SAVESENDMAIL, &aowner2, &aflags2);
    if ( *atrxxx ) {
       chk_dash = 1;
    }
    free_lbuf(atrxxx);
    if ((stricmp(buf1, "+send") == 0) || ((key & M_ALL) && Brandy(player) && 
         ((stricmp(buf1, "--") == 0) || (stricmp(buf1, "-~") == 0)) && chk_dash)) {
        if ( stricmp(buf1, "-~") == 0 ) {
           cmdp = (CMDENT *)hashfind((char *)"mail", &mudstate.command_htab);
           if ( cmdp ) {
              if ( search_nametab(player, cmdp->switches, "anonymous") == -1 ) {
                 notify(player, "Mail: Warning - unable to send anonymously.");
              } else {
                 i_addkey = M_ANON;
              }
           } else
              notify(player, "Mail: Warning - unable to send anonymously.");
        } else
            i_addkey = 0;
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	write_assemble(player, lbuf6, line);
	if (*buf2 == '\0' && *atrxxx == '\0' ) {
	    notify_quiet(player, "MAIL ERROR: No recipient for +send.");
	} else if (*lbuf6) {
            atrxxx = atr_get(player, A_SAVESENDMAIL, &aowner2, &aflags2);
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
            i_type = 0;
            atryyy = atr_get(player, A_TEMPBUFFER, &aowner2, &aflags2);
            i_type = atoi(atryyy);
            free_lbuf(atryyy);
	    if (infodata.dptr) {
	      strcpy(lbuf7,infodata.dptr);
              if ( *buf2 )
	         min = mail_send(player, (i_type == 1 ? (M_REPLY|i_addkey) : 
                                         (i_type == 2 ? (M_FORWARD|i_addkey) : (M_SEND|i_addkey))), 
                                         buf2, lbuf6, lbuf7);
              else
	         min = mail_send(player, (i_type == 1 ? (M_REPLY|i_addkey) : 
                                         (i_type == 2 ? (M_FORWARD|i_addkey) : (M_SEND|i_addkey))), 
                                         atrxxx, lbuf6, lbuf7);
	    }
	    else {
              if ( *buf2 )
	         min = mail_send(player, (i_type == 1 ? (M_REPLY|i_addkey) : 
                                         (i_type == 2 ? (M_FORWARD|i_addkey) : (M_SEND|i_addkey))), 
                                         buf2, lbuf6, NULL);
              else
	         min = mail_send(player, (i_type == 1 ? (M_REPLY|i_addkey) : 
                                         (i_type == 2 ? (M_FORWARD|i_addkey) : (M_SEND|i_addkey))), 
                                         atrxxx, lbuf6, NULL);
            }
	    if (min) {
	      notify_quiet(player, "Mail: Write message kept");
              if (Brandy(player)) {
                 switch(i_type) {
                    case 0 : notify_quiet(player, "      Use 'mail/write +send=<player-list>' to force resend.");
                             notify_quiet(player, "      Use 'mail/write +cc=<player-list> to replace player list.");
                             notify_quiet(player, "      Use 'mail/write +forget' to toss.");
                             break;
                    case 1 : notify_quiet(player, "      Use 'mail/write +reply=<msg#>' to force resend.");
                             notify_quiet(player, "      Use 'mail/write +forget' to toss.");
                             break;
                    case 2 : notify_quiet(player, "      Use 'mail/write +forward=<msg#> <player-list>' to force resend.");
                             notify_quiet(player, "      Use 'mail/write +cc=<player-list> to replace player list.");
                             notify_quiet(player, "      Use 'mail/write +forget' to toss.");
                             break;
                 }
              }
            } else {
	      write_wipe(player, line, 0);
              atr_clr(player, A_SAVESENDMAIL);
              atr_clr(player, A_TEMPBUFFER);
              atr_clr(player, A_BCCMAIL);
            }
            if ( *buf2 && *atrxxx )
               notify_quiet(player, "Mail: Previous send list overridden with new list.");
            free_lbuf(atrxxx);
	}
    } else if (stricmp(buf1,"+subject") == 0) {
	*(int *)sbuf1 = MIND_WRTS;
	*(int *)(sbuf1 + sizeof(int)) = player;
	keydata.dptr = sbuf1;
	keydata.dsize = sizeof(int) << 1;
	if (!*buf2) {
	  dbm_delete(mailfile,keydata);
	  notify_quiet(player,"Mail: Write subject cleared");
	}
	else  {
	  if (strlen(buf2) > SUBJLIM) {
	    notify_quiet(player,"MAIL WARNING: Subject truncated");
	    *(buf2 + SUBJLIM) = '\0';
	  }
	  infodata.dptr = buf2;
	  infodata.dsize = strlen(buf2) + 1;
	  dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	  notify_quiet(player,"Mail: Write subject set");
	}
    } else if (stricmp(buf1, "+reply") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	write_assemble(player, lbuf6, line);
	if (*buf2 == '\0') {
	    notify_quiet(player, "MAIL ERROR: No message specified for +reply.");
	} else if (*lbuf6) {
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
	    if (infodata.dptr) {
	      strcpy(lbuf7,infodata.dptr);
	      min = mail_send(player, M_REPLY, buf2, lbuf6, lbuf7);
	    }
	    else
	      min = mail_send(player, M_REPLY, buf2, lbuf6, NULL);
	    if (min)
	      notify_quiet(player,"Mail: Write message kept.");
	    else {
	      write_wipe(player, line, 0);
              atr_clr(player, A_SAVESENDMAIL);
              atr_clr(player, A_TEMPBUFFER);
              atr_clr(player, A_BCCMAIL);
            }
	}
    } else if (stricmp(buf1, "+forward") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	write_assemble(player, lbuf6, line);
	if (*buf2 == '\0') {
	    notify_quiet(player, "MAIL ERROR: No forward information given.");
	} else if (*lbuf6) {
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
	    if (infodata.dptr) {
	      strcpy(lbuf7,infodata.dptr);
	      min = mail_send(player, M_FORWARD, buf2, lbuf6, lbuf7);
	    }
	    else
	      min = mail_send(player, M_FORWARD, buf2, lbuf6, NULL);
	    if (min)
	      notify_quiet(player,"Mail: Write message kept.");
	    else {
	      write_wipe(player, line, 0);
              atr_clr(player, A_SAVESENDMAIL);
              atr_clr(player, A_TEMPBUFFER);
              atr_clr(player, A_BCCMAIL);
            }
	}
    } else if (stricmp(buf1, "+justify") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	if (buf2 == NULL) {
	    notify_quiet(player, "MAIL ERROR: Improper +justify format.");
	} else {
	    p1 = mystrchr(buf2, ',');
	    if (!p1)
		notify_quiet(player,"MAIL ERROR: Improper +justify format.");
	    else if (!*(p1 + 1))
		notify_quiet(player,"MAIL ERROR: Improper +justify format.");
	    else {
		*p1 = '\0';
		index = (short int)atoi(buf2);
		just = tolower(*(p1 + 1));
		if ((index < 1) || (index > line) || ((just != 'l') && (just != 'c')
				 && (just != 'r')) || (*(p1 + 2) != '\0')) {
		    notify_quiet(player, "MAIL ERROR: Improper +justify format.");
		} else {
		    *(int *)sbuf1 = MIND_WRTL;
		    *(int *)(sbuf1 + sizeof(int)) = player;
		    *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
		    keydata.dptr = sbuf1;
		    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		    infodata = dbm_fetch(mailfile, keydata);
		    if (!infodata.dptr) {
			notify_quiet(player, "MAIL ERROR: Write line missing.");
		    } else {
			p2 = infodata.dptr;
			while (isspace((int)*p2))
			    p2++;
			count = (short int)strlen(p2);
			switch (just) {
			case 'l':
			    strcpy(lbuf1, p2);
			    break;
			case 'c':
			    if (count > 79)
				break;
			    *lbuf1 = '\0';
			    for (min = 1; min < (79 - count) / 2; min++) {
				strcat(lbuf1, " ");
			    }
			    strcat(lbuf1, p2);
			    break;
			case 'r':
			    if (count > 79)
				break;
			    *lbuf1 = '\0';
			    for (min = 0; min < (79 - count); min++) {
				strcat(lbuf1, " ");
			    }
			    strcat(lbuf1, p2);
			}
                        i_type = infodata.dsize;
			infodata.dptr = lbuf1;
			infodata.dsize = strlen(lbuf1) + 1;
	                tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
                        if ( *tcim ) {
                           tcimptr = strtok_r(tcim, " ", &tstrtokr);
                           tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
                           totcharinmail = atoi(tcimptr2) + (infodata.dsize - i_type);
	                   atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
                        } else {
                           totcharinmail = (infodata.dsize - i_type);
	                   atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
                        }
                        free_lbuf(tcim);
			dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
			notify_quiet(player, unsafe_tprintf("Mail: Line justified (%d/%d).",
                                     totcharinmail, LBUF_SIZE - 10));
                        if ( totcharinmail > (LBUF_SIZE-10) )
                           notify_quiet(player, 
                             unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                             (totcharinmail - LBUF_SIZE + 10)));
		    }
		}
	    }
	}
    } else if (stricmp(buf1, "+split") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	p1 = mystrchr(buf2, ',');
	if (!p1) {
	    notify_quiet(player, "MAIL ERROR: Improper +split format.");
	} else if (line >= MAXWRTL) {
	  notify_quiet(player,"MAIL ERROR: Maximum number of lines reached");
	} else {
	    *p1 = '\0';
	    min = (short int)atoi(buf2);
	    max = (short int)atoi(p1 + 1);
	    if ((min < 1) || (min > line) || (max < 1)) {
		notify_quiet(player, "MAIL ERROR: Improper +split format.");
	    } else {
		*(int *)sbuf1 = MIND_WRTL;
		*(int *)(sbuf1 + sizeof(int)) = player;
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = min;
		keydata.dptr = sbuf1;
		keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		infodata = dbm_fetch(mailfile, keydata);
		if (!infodata.dptr) {
		    notify_quiet(player, "MAIL ERROR: Write line missing.");
		} else {
		    index = 0;
		    p2 = infodata.dptr;
		    for (count = 0; count < max; count++) {
			p4 = mystrchr(p2, ' ');
			if (!p4)
			    break;
			p2 = p4 + 1;
		    }
		    if (!p4)
			notify_quiet(player, "MAIL ERROR: Not enough words in line to split there.");
		    else if (!*(p4+1))
			notify_quiet(player, "MAIL ERROR: Not enough words in line to split there.");
		    else {
			*p4 = '\0';
			strcpy(lbuf2, infodata.dptr);
			strcpy(lbuf3, p4 + 1);
			infodata.dptr = lbuf2;
			infodata.dsize = strlen(lbuf2) + 1;
			dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
			write_insert(player, lbuf3, line, min + 1);
			notify_quiet(player, "Mail: Line split.");
		    }
		}
	    }
	}
    } else if (stricmp(buf1, "+insert") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	p1 = mystrchr(buf2, ',');
	if (p1 == NULL) {
	    notify_quiet(player, "MAIL ERROR: Improper +insert format.");
	} else {
	    *p1 = '\0';
	    count = (short int)atoi(buf2);
	    if ((count < 1) || (count > line) || (*(p1 + 1) == '\0')) {
		notify_quiet(player, "MAIL ERROR: Improper +insert format.");
	    } else {
	        tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
                if ( *tcim ) {
                   tcimptr = strtok_r(tcim, " ", &tstrtokr);
                   tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
                   totcharinmail = atoi(tcimptr2) + strlen(p1+1);
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
                } else {
                   totcharinmail = strlen(p1+1);
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
                }
                free_lbuf(tcim);
		write_insert(player, p1 + 1, line, count);
		notify_quiet(player, unsafe_tprintf("Mail: Line inserted (%d/%d).",
                             totcharinmail, LBUF_SIZE - 10));
                if ( totcharinmail > (LBUF_SIZE-10) )
                   notify_quiet(player, 
                             unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                             (totcharinmail - LBUF_SIZE + 10)));
	    }
	}
    } else if (stricmp(buf1, "+validate") == 0 ) {
/*
       bccatr = atr_get(player, A_BCCMAIL, &aowner3, &aflags3);
       savesend = atr_get(player, A_SAVESENDMAIL, &aowner3, &aflags3);
//     tprp_buff = tpr_buff = alloc_lbuf("mail_send");
       strcpy(tpr_buff, lbuf3);
       pt1 = tpr_buff;
       while ( !term && *pt1 ) {
          while (isspace((int)*pt1) && *pt1) pt1++;
          if (!*pt1) {
             if ( i_nogood != 2 )
                i_nogood = 1;
             break;
          }
          pt2 = pt1+1;
          if ((sepchar == '\0') || (*pt1 == '#')) {
             if ( comma_exists )
                while (*pt2 && (*pt2 != ',')) pt2++;
             else
                while (*pt2 && !isspace((int)*pt2) && (*pt2 != ',')) pt2++;
          } else {
             while (*pt2 && (*pt2 != sepchar) && (*pt2 != ',')) pt2++;
          }
          if (*pt2 && !sepchar) {
             sepchar = *pt2;
             term = 0;
          } else if (*pt2) {
             term = 0;
          } else
             term = 1;
          *pt2 = '\0';
          toplay = lookup_player(player,pt1,0);
          if (toplay == NOTHING) {
             i_nogood = 1;
             break;
          } else {
             if ( !i_nogood )
                i_nogood = 2;
          }
          pt1 = pt2+1;
       }
       free_lbuf(tpr_buff);
*/
    } else if (stricmp(buf1, "+proof") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	if (line == 0) {
	    notify_quiet(player, "Mail: No write in progress.");
	} else {
	    if (*buf2 == '\0') {
		min = 1;
		max = line;
	    } else {
		p4 = mystrchr(buf2, ',');
		if (p4 == NULL) {
		    min = atoi(buf2);
		    if (min <= 0)
			min = 1;
                    if (min > line)
                        min = line;
		    max = line;
		} else {
		    *p4 = '\0';
		    min = atoi(buf2);
		    max = atoi(p4 + 1);
		    if (min <= 0)
			min = 1;
                    if (min > line)
                        min = line;
		    if (max < min)
			max = line;
		    if (max > line)
			max = line;
		}
            }
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
            memset(msubj, 0, sizeof(msubj));
	    if (infodata.dptr) {
		strncpy(msubj, infodata.dptr, SUBJLIM);
            }
            *(int *)sbuf1 = MIND_WRTL;
            keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
            notify_quiet(player, "----------------------------------------"\
                                 "--------------------------------------");
            type_three = 0;
            type_two = 1;
            ztmpptr = ztmp = alloc_lbuf("mail_write.proof_listing");
            time_tmp = (char *) ctime(&mudstate.now);
            time_tmp[strlen(time_tmp) - 1] = '\0';
            is_first = 0;
            for (count = min; count <= max; count++) {
                *(int *)(sbuf1 + sizeof(int)) = player;
                *(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
                infodata = dbm_fetch(mailfile, keydata);
                if (!infodata.dptr)
                    type_three++;
                else {
                    if ( MailNoParse(player) ) {
                       mailfunkvar = mail_funkvar(infodata.dptr);
                       type_two += strlen(mailfunkvar);
                       if ( MailStripRet(player) ) {
                          if ( is_first ) {
                             safe_chr(' ', ztmp, &ztmpptr);
                             type_two++;
                          }
                       } else {
                          if ( is_first ) {
                             safe_str("\r\n", ztmp, &ztmpptr);
                             type_two += 2;
                          }
                       }
                       safe_str(mailfunkvar, ztmp, &ztmpptr);
                       free_lbuf(mailfunkvar);
                    } else {
                       type_two += strlen(infodata.dptr);
                       if ( MailStripRet(player) ) {
                          if ( is_first ) {
                             safe_chr(' ', ztmp, &ztmpptr);
                             type_two++;
                          }
                       } else {
                          if ( is_first ) {
                             safe_str("\r\n", ztmp, &ztmpptr);
                             type_two += 2;
                          }
                       }
                       safe_str(infodata.dptr, ztmp, &ztmpptr);
                    }
                }
                is_first = 1;
            }
            atrxxx = atr_get(player, A_SAVESENDMAIL, &aowner2, &aflags2);
            if (Brandy(player) && *atrxxx) {
               atryyy = atr_get(player, A_TEMPBUFFER, &aowner2, &aflags2);
               i_type = atoi(atryyy);
               free_lbuf(atryyy);
               atrtmpptr = atrtmp = alloc_lbuf("mail_write.proof");
               if ( i_type == 1 ) {
                  safe_str("(reply list)", atrtmp, &atrtmpptr);
               } else if ( i_type == 2 ) {
                  if ( *atrxxx ) {
                     if ( strchr(atrxxx, ' ') ) {
                        safe_str(strchr(atrxxx, ' '), atrtmp, &atrtmpptr);
                     } else {
                        safe_str("(empty forward list)", atrtmp, &atrtmpptr);
                     }
                  } else {
                     safe_str("(empty forward list)", atrtmp, &atrtmpptr);
                  }
               } else {
                  i_type = 0;
                  if ( *atrxxx )
                     safe_str(atrxxx, atrtmp, &atrtmpptr);
                  else
                     safe_str("(empty send list)", atrtmp, &atrtmpptr);
               }
               bccatr = atr_get(player, A_BCCMAIL, &aowner3, &aflags3);
               if ( i_type == 1 ) {
                  if ( atrxxx[0] == '@' )
                     i_typeval = atoi(atrxxx+1);
                  else
                     i_typeval = atoi(atrxxx);
                  notify_quiet(player, unsafe_tprintf("Message In Progress [Replying to message %d]", 
                     i_typeval > 0 ? i_typeval : -1));
               } else if ( i_type == 2 ) {
                  if ( atrxxx[0] == '@' )
                     i_typeval = atoi(atrxxx+1);
                  else
                     i_typeval = atoi(atrxxx);
                  notify_quiet(player, unsafe_tprintf("Message In Progress [Forwarding message %d]",
                               i_typeval > 0 ? i_typeval : -1));
               } else {
                  notify_quiet(player, "Message In Progress");
               }
               s_toptr = s_to = alloc_lbuf("mail_pretty1");
               if ( MailValid(player) ) {
                  if ( atrxxx && *atrxxx ) {
                     parse_tolist(player, atrxxx, s_to, &s_toptr);
                  } else {
                     strcpy(s_to, atrtmp);
                  }
               } else {
                  strcpy(s_to, atrtmp);
               }
               if ( bccatr && *bccatr ) {
                 s_bccptr = s_bcc = alloc_lbuf("mail_pretty2");
                 if ( MailValid(player) ) {
                    parse_tolist(player, bccatr, s_bcc, &s_bccptr);
                 } else {
                    strcpy(s_bcc, bccatr);
                 }
		 notify_quiet(player,unsafe_tprintf("Status: %-11s From: %-16s %s\r\n"\
					     "To: %s\r\nBcc: %s\r\nDate/Time: %-31s Size: %d %s", 
					     "EDITING", "You", 
					     (i_type == 0 ? " " : (i_type == 1 ? "REPLY" : "FORWARD")),
					     s_to, s_bcc, time_tmp, type_two, "Bytes"));
                 free_lbuf(s_bcc);
	       } else {
		 notify_quiet(player,unsafe_tprintf("Status: %-11s From: %-16s %s\r\n"\
					     "To: %s\r\nDate/Time: %-31s Size: %d %s",
					     "EDITING", "You",
					     (i_type == 0 ? " " : (i_type == 1 ? "REPLY" : "FORWARD")),
					     s_to, time_tmp, type_two, "Bytes"));		 
	       }
               free_lbuf(s_to);
               free_lbuf(atrtmp);
	       free_lbuf(bccatr);
            } else {
               notify_quiet(player,unsafe_tprintf("Message In Progress\r\nStatus: %-11s From: %-16s\r\n"\
                                            "To: %s\r\nDate/Time: %-31s Size: %d %s", 
                                            "EDITING", "You", "(empty list)", 
                                            time_tmp, type_two, "Bytes"));
            }
            if ( strchr(atrxxx, '*') != NULL )
               no_writie = 1;
            else
               no_writie = 0;
            free_lbuf(atrxxx);
            if ( msubj[0] != '\0' )
               notify_quiet(player, unsafe_tprintf("Subject: %s", msubj));
            else
               notify_quiet(player, "Subject: (None)");
            if ( i_type == 1 ) {
               notify_quiet(player, "----------------------------------------"\
                                    "--------------------------------------");
//             if ( (i_typeval < 1) || (i_typeval > 9999) )
               if ( i_typeval < 1 )
                  no_writie = 0;
               if ( no_writie ) 
                  index = get_msg_index(player, i_typeval, 1, NOTHING, 1);
//             if ( no_writie && (i_typeval > 0) && (index > 0) && (index < 9999) && get_hd_rcv_rec(player,index,mbuf1,0,0)) {
               if ( no_writie && (i_typeval > 0) && (index > 0) && get_hd_rcv_rec(player,index,mbuf1,0,0)) {
                  acheck = *(short int *)(mbuf1 + hindoff);
                  if ( acheck & tomask )
                     acheck &= ~tomask;
                  *(int *)sbuf1 = MIND_MSG;
                  *(int *)(sbuf1 + sizeof(int)) = *(int *)mbuf1;
                  *(short int *)(sbuf1 + (sizeof(int) << 1)) = acheck;
                  keydata.dptr = sbuf1;
                  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
                  infodata = dbm_fetch(mailfile,keydata);
                  if ( infodata.dptr )
                     notify_quiet(player, unsafe_tprintf("%s", infodata.dptr));
                  else
                     notify_quiet(player, "----- <ORIGINAL MESSAGE TEXT WILL BE INSERTED HERE [IF REQUESTED]> -----");
               } else {
                  notify_quiet(player, "----- <ORIGINAL MESSAGE TEXT WILL BE INSERTED HERE [IF REQUESTED]> -----");
               }
            }
            if ( i_type == 2 ) {
               notify_quiet(player, "----------------------------------------"\
                                    "--------------------------------------");
//             if ( (i_typeval > 0) && (i_typeval < 9999) ) { 
               if ( i_typeval > 0 ) {
                  index = get_msg_index(player, i_typeval, 1, NOTHING, 1);
                  no_writie = 1;
               } else {
                  no_writie = 0;
               } 
//             if ( no_writie && (index > 0) && (index < 9999) && get_hd_rcv_rec(player,index,mbuf1,0,0)) {
               if ( no_writie && (index > 0) && get_hd_rcv_rec(player,index,mbuf1,0,0)) {
                  acheck = *(short int *)(mbuf1 + hindoff);
                  if ( acheck & tomask )
                     acheck &= ~tomask;
                  *(int *)sbuf1 = MIND_MSG;
                  *(int *)(sbuf1 + sizeof(int)) = *(int *)mbuf1;
                  *(short int *)(sbuf1 + (sizeof(int) << 1)) = acheck;
                  keydata.dptr = sbuf1;
                  keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
                  infodata = dbm_fetch(mailfile,keydata);
                  if ( infodata.dptr )
                     notify_quiet(player, unsafe_tprintf("%s", infodata.dptr));
                  else
                     notify_quiet(player, "----- <ORIGINAL MESSAGE TEXT WILL BE INSERTED HERE> -----");
               } else {
                  notify_quiet(player, "----- <ORIGINAL MESSAGE TEXT WILL BE INSERTED HERE> -----");
               }
            }
            notify_quiet(player, "----------------------------------------"\
                                 "--------------------------------------");
            notify_quiet(player, ztmp);
            free_lbuf(ztmp);
            notify_quiet(player, "----------------------------------------"\
                                 "--------------------------------------");
            if ( type_two > (LBUF_SIZE-20) )
               notify_quiet(player, unsafe_tprintf("Mail: WARNING - Mail %d bytes over maximum.", (type_two-(LBUF_SIZE-20))));
            if ( type_three > 0 )
               notify_quiet(player, unsafe_tprintf("Mail: ERROR - %d missing line numbers detected.", type_three));
            if ( (type_three > 0) || (type_two > (LBUF_SIZE-20)) )
               notify_quiet(player, "      Please check 'mail/write +list' for details.");
        }
    } else if (stricmp(buf1, "+list") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	if (line == 0) {
	    notify_quiet(player, "Mail: No write in progress.");
	} else {
	    if (*buf2 == '\0') {
		min = 1;
		max = line;
	    } else {
		p4 = mystrchr(buf2, ',');
		if (p4 == NULL) {
		    min = atoi(buf2);
		    if (min <= 0)
			min = 1;
                    if (min > line)
                        min = line;
		    max = line;
		} else {
		    *p4 = '\0';
		    min = atoi(buf2);
		    max = atoi(p4 + 1);
		    if (min <= 0)
			min = 1;
                    if (min > line)
                        min = line;
		    if (max < min)
			max = line;
		    if (max > line)
			max = line;
		}
	    }
	    notify_quiet(player, "Mail: Your write message is:");
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
            atrxxx = atr_get(player, A_SAVESENDMAIL, &aowner2, &aflags2);
            if (Brandy(player) && *atrxxx) {
               atryyy = atr_get(player, A_TEMPBUFFER, &aowner2, &aflags2);
               i_type = atoi(atryyy);
               free_lbuf(atryyy);
               notify_quiet(player, "----------------------------------------"\
                                    "--------------------------------------");
               if ( i_type == 1 ) {
                  if ( *atrxxx ) {
                     if ( atrxxx[0] == '@' )
                        i_type = atoi(atrxxx+1);
                     else
                        i_type = atoi(atrxxx);
                     notify_quiet(player, unsafe_tprintf("Replying to message %d", 
                        i_type > 0 ? i_type : -1));
                  } else
                     notify_quiet(player, "Replying to nothing.");
               } else if ( i_type == 2 ) {
                  if ( *atrxxx ) {
                     atrtmp = strchr(atrxxx, ' ');
                     i_type = atoi(atrxxx);
                     notify_quiet(player, unsafe_tprintf("forwarding %d to: %s",
                                  i_type > 0 ? i_type : -1, 
                                  *atrtmp ? atrtmp+1: "(no one)"));
                  } else
                     notify_quiet(player, "Forwarding (nothing) to: (no one)");
               } else {
                  if ( *atrxxx )
                     notify_quiet(player, unsafe_tprintf("Sending to: %s", atrxxx));
                  else
                     notify_quiet(player, "Sending to: (no one)");
               }
            }
            free_lbuf(atrxxx);
	    if (infodata.dptr)
		notify_quiet(player,unsafe_tprintf("Subject: %s",infodata.dptr));
	    *(int *)sbuf1 = MIND_WRTL;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
            if ( Brandy(player) ) {
               notify_quiet(player, "----------------------------------------"\
                                    "--------------------------------------");
            }
            tprp_buff = tpr_buff = alloc_lbuf("mail_write_+list");
	    for (count = min; count <= max; count++) {
		*(int *)(sbuf1 + sizeof(int)) = player;
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
		infodata = dbm_fetch(mailfile, keydata);
		if (!infodata.dptr)
		    notify_quiet(player, "MAIL ERROR: Write line missing.");
		else {
                    tprp_buff = tpr_buff;
                    if ( MailNoParse(player) ) {
                       mailfunkvar = mail_funkvar(infodata.dptr);
		       notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%-4d: %s", count, mailfunkvar));
                       free_lbuf(mailfunkvar);
                    } else
		       notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%-4d: %s", count, infodata.dptr));
                }
	    }
            free_lbuf(tpr_buff);
            if ( Brandy(player) ) {
               notify_quiet(player, "----------------------------------------"\
                                    "--------------------------------------");
            }
	}
    } else if (stricmp(buf1, "+forget") == 0) {
	if (line == 0) {
	    notify_quiet(player, "Mail: No write in progress.");
            atr_clr(player, A_SAVESENDMAIL);
            atr_clr(player, A_TEMPBUFFER);
            atr_clr(player, A_BCCMAIL);
        } else {
	   if (p3 != NULL) {
	       notify_quiet(player, "Mail: Extra characters ignored.");
	   }
	   write_wipe(player, line, 1);
           atr_clr(player, A_SAVESENDMAIL);
           atr_clr(player, A_TEMPBUFFER);
           atr_clr(player, A_BCCMAIL);
        }
    } else if (stricmp(buf1, "+del") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	index = (short int)atoi(buf2);
	if ((index < 1) || (index > line) || (line == 1)) {
	    notify_quiet(player, "MAIL ERROR: Bad line number in delete command.");
	} else {
	    *(int *)sbuf1 = MIND_WRTS;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    infodata = dbm_fetch(mailfile,keydata);
	    *(int *)sbuf1 = MIND_WRTL;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
 	    *(int *)(sbuf1 + sizeof(int)) = player;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
	    infodata = dbm_fetch(mailfile, keydata);
	    tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
            if ( *tcim ) {
               tcimptr = strtok_r(tcim, " ", &tstrtokr);
               tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
               totcharinmail = atoi(tcimptr2) - infodata.dsize;
	       atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
            } else {
               totcharinmail = 0;
	       atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
            }
            free_lbuf(tcim);
            i_type = infodata.dsize;
	    write_del(player, index, line);
	    notify_quiet(player, unsafe_tprintf("Mail: Write line deleted (%d/%d).",
                                 totcharinmail, LBUF_SIZE - 10));
            if ( totcharinmail > (LBUF_SIZE-10) )
               notify_quiet(player, 
                        unsafe_tprintf("Mail Warning: Message is still %d over limit.  Mail will be cut off.",
                        (totcharinmail - LBUF_SIZE + 10)));
	}
    } else if (stricmp(buf1, "+join") == 0) {
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
	p1 = mystrchr(buf2, ',');
	if (p1 == NULL) {
	    notify_quiet(player, "MAIL ERROR: Improper +join format.");
	} else {
	    *p1 = '\0';
	    index = (short int)atoi(buf2);
	    count = (short int)atoi(p1 + 1);
	    if ((index == count) || (count < 1) || (index < 1) ||
		(count > line) || (index > line)) {
		notify_quiet(player, "MAIL ERROR: Bad line number in +join command.");
	    } else {
		*(int *)sbuf1 = MIND_WRTL;
		*(int *)(sbuf1 + sizeof(int)) = player;
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
		keydata.dptr = sbuf1;
		keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		infodata = dbm_fetch(mailfile, keydata);
		if (!infodata.dptr) {
		    notify_quiet(player, "MAIL ERROR: Write line missing.");
		} else {
		    strcpy(lbuf2, infodata.dptr);
		    *(int *)sbuf1 = MIND_WRTL;
		    *(int *)(sbuf1 + sizeof(int)) = player;
		    *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
		    keydata.dptr = sbuf1;
		    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		    infodata = dbm_fetch(mailfile, keydata);
		    if (!infodata.dptr) {
			notify_quiet(player, "MAIL ERROR: Write line missing.");
		    } else {
			if ((strlen(lbuf2) + strlen(infodata.dptr) + 2) > LBUF_SIZE) {
			    notify_quiet(player, "MAIL ERROR: Joined line too long.");
			} else {
			    p2 = infodata.dptr;
			    strcpy(lbuf3, p2);
			    strcat(lbuf3, " ");
			    strcat(lbuf3, lbuf2);
			    infodata.dptr = lbuf3;
			    infodata.dsize = strlen(lbuf3) + 1;
			    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
			    write_del(player, count, line);
			    notify_quiet(player, "Mail: Line joined.");
			}
		    }
		}
	    }
	}
    } else if ( (stricmp(buf1, "+cc") == 0 ) || (stricmp(buf1, "+bcc") == 0) ||
                (stricmp(buf1, "+acc") == 0) ) {
        i_bccacc=0;
        no_writie = 0;
	if (line == 0) {
           notify_quiet(player, "Mail: No write in progress.");
           return;
        }
	if (!*buf2) {
	   notify_quiet(player,"MAIL ERROR: Improper +cc/+bcc format.");
           return;
	}
        atryyy = atr_get(player, A_TEMPBUFFER, &aowner2, &aflags2);
        if ( *atryyy == 'X' ) {
           notify_quiet(player, "MAIL ERROR: Unable to use in current mode.");
           free_lbuf(atryyy);
           return;
        }
        i_type = atoi(atryyy);
        free_lbuf(atryyy);
        if ( i_type == 1 ) {
           notify_quiet(player, "MAIL ERROR: You can not use +cc/+bcc with a reply.");
           return;
        }
        atrxxx = atr_get(player, A_SAVESENDMAIL, &aowner2, &aflags2);
        atrtmpptr = atrtmp = alloc_lbuf("ccbcc.tempbuffer");
        if ( i_type == 2 ) {
           if ( (stricmp(buf1, "+acc") == 0) || (stricmp(buf1, "+bcc") == 0) ) {
              if ( atrxxx && *atrxxx ) {
                 i_bccacc = 1;
                 safe_str(atrxxx, atrtmp, &atrtmpptr);
                 safe_chr(' ', atrtmp, &atrtmpptr);
              }
           } else {
              safe_chr('@', atrtmp, &atrtmpptr);
           }
           if ( stricmp(buf1, "+bcc") == 0 ) {
              bcctmpptr = bcctmp = alloc_lbuf("bcc.tempbuffer");
              if ( strcmp(buf2, "!ALL") != 0 ) {
                 bccatr = atr_get(player, A_BCCMAIL, &aowner3, &aflags3);
                 if ( bccatr ) {
                    safe_str(bccatr, bcctmp, &bcctmpptr);
                    safe_chr(' ', bcctmp, &bcctmpptr);
                 }
                 free_lbuf(bccatr);
              }
              if ( strcmp(buf2, "!ALL") != 0 ) {
                 safe_str(buf2, bcctmp, &bcctmpptr);
                 atr_add_raw(player, A_BCCMAIL, bcctmp);
              } else {
                 atr_clr(player, A_BCCMAIL);
                 no_writie = 1;
              }
              free_lbuf(bcctmp);
           }
           if ( !no_writie ) {
              safe_str(buf2, atrtmp, &atrtmpptr);
              atr_add_raw(player, A_SAVESENDMAIL, atrtmp);
              if ( i_bccacc )
                 notify_quiet(player, "Mail: Additional forward player list appended.");
              else
                 notify_quiet(player, "Mail: New forward player list stored.");
           } else {
              notify_quiet(player, "Mail: Bcc player list cleared.");
           }
        } else {
           if ( ((stricmp(buf1, "+acc") == 0) || (stricmp(buf1, "+bcc") == 0)) && atrxxx) {
              i_bccacc = 1;
              safe_str( atrxxx, atrtmp, &atrtmpptr);
              safe_chr(' ', atrtmp, &atrtmpptr);
           } else {
              safe_chr('@', atrtmp, &atrtmpptr);
           }
           if ( stricmp(buf1, "+bcc") == 0 ) {
              bcctmpptr = bcctmp = alloc_lbuf("bcc.tempbuffer");
              if ( strcmp(buf2, "!ALL") != 0 ) {
                 bccatr = atr_get(player, A_BCCMAIL, &aowner3, &aflags3);
                 if ( bccatr ) {
                    safe_str(bccatr, bcctmp, &bcctmpptr);
                    safe_chr(' ', bcctmp, &bcctmpptr);
                 }
                 free_lbuf(bccatr);
              }
              if ( strcmp(buf2, "!ALL") != 0 ) {
                 safe_str(buf2, bcctmp, &bcctmpptr);
                 atr_add_raw(player, A_BCCMAIL, bcctmp);
              } else {
                 atr_clr(player, A_BCCMAIL);
                 no_writie = 1;
              }
              free_lbuf(bcctmp);
           }
           if ( !no_writie ) {
              safe_str(buf2, atrtmp, &atrtmpptr);
              atr_add_raw(player, A_SAVESENDMAIL, atrtmp);
              if ( i_bccacc )
                 notify_quiet(player, "Mail: Additional send player list appended.");
              else
                 notify_quiet(player, "Mail: New send player list stored.");
           } else {
              notify_quiet(player, "Mail: Bcc player list cleared.");
           }
        }
        free_lbuf(atrxxx);
        free_lbuf(atrtmp);
    } else if ((stricmp(buf1, "+edit") == 0) || (stricmp(buf1, "+fedit") == 0)) {
	if (p3 == NULL) {
	    notify_quiet(player, "MAIL ERROR: Improper +edit format.");
	    return;
	}
        if ( line < 1 ) {
            notify_quiet(player, "Mail: No write in progress.");
            return;
        }
	count = (short int)atoi(p3 + 1);
	if ((count < 1) || (count > line)) {
	    notify_quiet(player, "MAIL ERROR: Bad line number in +edit.");
	    return;
	}
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	infodata = dbm_fetch(mailfile, keydata);
	if (!infodata.dptr) {
	    notify_quiet(player, "MAIL ERROR: No write line to edit.");
	} else {
	    if (*buf2 == '<') {
		p4 = mystrchr(buf2, '>');
		if (p4 == NULL) {
		    strcpy(lbuf3, buf2);
		    p4 = lbuf3;
		} else {
		    *p4 = '\0';
		    strcpy(lbuf3, buf2 + 1);
		    strcat(lbuf3, p4 + 1);
		    p2 = lbuf3 + (p4 - buf2) - 1;
		    p4 = p2;
		}
	    } else {
		strcpy(lbuf3, buf2);
		p4 = lbuf3;
	    }
	    p1 = mystrchr(p4, ',');
	    if (!p1) {
		notify_quiet(player, unsafe_tprintf("MAIL ERROR: Improper %sedit format.",
                                     stricmp(buf1, "+edit") ? "+f" : "+"));
	    } else {
		*p1 = '\0';
		p2 = strstr3(infodata.dptr, lbuf3);
		if (!p2) {
		    notify_quiet(player, "MAIL ERROR: Original string not found in +edit.");
		} else {
		    if ((strlen(infodata.dptr) + (strlen(p1 + 1) - strlen(lbuf3))) > LBUF_SIZE) {
			notify_quiet(player, "MAIL ERROR: Edited line too long. Edit not performed.");
		    } else {
                        i_type = infodata.dsize;
                        if ( stricmp(buf1, "+edit") == 0 ) {
			   *lbuf2 = '\0';
			   p4 = infodata.dptr;
			   strncpy(lbuf2, p4, p2 - p4);
			   *(lbuf2 + (p2 - p4)) = '\0';
			   strcat(lbuf2, p1 + 1);
			   strcat(lbuf2, p2 + strlen(lbuf3));
			   infodata.dptr = lbuf2;
			   infodata.dsize = strlen(lbuf2) + 1;
                        } else {
                           atrtmp = replace_string(lbuf3, buf2+(int)strlen(lbuf3)+1, infodata.dptr, 0);
                           strcpy(lbuf2, atrtmp);
                           free_lbuf(atrtmp);
                           infodata.dptr = lbuf2;
                           infodata.dsize = strlen(lbuf2) + 1;
                        }
			if (infodata.dsize == 1) {
			    notify_quiet(player, "Mail: Edited message is blank.  Use +del to delete line.");
			} else {
	                    tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
                            if ( *tcim ) {
                               tcimptr = strtok_r(tcim, " ", &tstrtokr);
                               tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
                               totcharinmail = (infodata.dsize - i_type) + atoi(tcimptr2);
	                       atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
                            } else {
                               totcharinmail = (infodata.dsize - i_type);
	                       atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
                            }
                            free_lbuf(tcim);
			    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
		            notify_quiet(player,unsafe_tprintf("Mail: %sedit done (%d/%d)",
                                         (stricmp(buf1, "+edit") ? "+f" : "+"),
                                         totcharinmail, (LBUF_SIZE-10) ));
                            if ( totcharinmail > (LBUF_SIZE-10) )
                              notify_quiet(player, 
                              unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                              (totcharinmail - LBUF_SIZE + 10)));
			}
		    }
		}
	    }
	}
    } else if (stricmp(buf1, "+editall") == 0 || (stricmp(buf1, "+feditall") == 0)) {
        valid_flag = 1;
        if ( line < 1 ) {
            notify_quiet(player, "Mail: No write in progress.");
            return;
        }
        gdcount = 0;
	if (p3 != NULL) {
	    notify_quiet(player, "Mail: Extra characters ignored.");
	}
        i_type = 0;
        tprp_buff = tpr_buff = alloc_lbuf("mail_write_+editall");
        for (count = 1; count <= line; count++) {
	   *(int *)sbuf1 = MIND_WRTL;
	   *(int *)(sbuf1 + sizeof(int)) = player;
	   *(short int *)(sbuf1 + (sizeof(int) << 1)) = count;
	   keydata.dptr = sbuf1;
	   keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	   infodata = dbm_fetch(mailfile, keydata);
	   if (!infodata.dptr) {
	       break;
	   } else {
	       if (*buf2 == '<') {
		   p4 = mystrchr(buf2, '>');
		   if (p4 == NULL) {
		       strcpy(lbuf3, buf2);
		       p4 = lbuf3;
		   } else {
		       *p4 = '\0';
		       strcpy(lbuf3, buf2 + 1);
		       strcat(lbuf3, p4 + 1);
		       p2 = lbuf3 + (p4 - buf2) - 1;
		       p4 = p2;
		   }
	       } else {
		   strcpy(lbuf3, buf2);
		   p4 = lbuf3;
	       }
	       p1 = mystrchr(p4, ',');
	       if (!p1) {
                   tprp_buff = tpr_buff;
		   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "MAIL ERROR: Improper %seditall format.",
                                        stricmp(buf1, "+editall") ? "+f" : "+"));
                   valid_flag = 0;
                   break;
	       } else {
		   *p1 = '\0';
		   p2 = strstr3(infodata.dptr, lbuf3);
		   if (!p2) {
                      i_type += infodata.dsize;
		      continue;
		   } else {
		       if ((strlen(infodata.dptr) + (strlen(p1 + 1) - strlen(lbuf3))) > LBUF_SIZE) {
                          tprp_buff = tpr_buff;
                          notify_quiet(player, 
                              safe_tprintf(tpr_buff, &tprp_buff, "MAIL ERROR: Edited line (line #%d) too long. Line skipped.",
                                       count));
                              i_type += infodata.dsize;
		       } else {
                           if ( stricmp(buf1, "+editall") == 0 ) {
			      *lbuf2 = '\0';
			      p4 = infodata.dptr;
			      strncpy(lbuf2, p4, p2 - p4);
			      *(lbuf2 + (p2 - p4)) = '\0';
			      strcat(lbuf2, p1 + 1);
			      strcat(lbuf2, p2 + strlen(lbuf3));
			      infodata.dptr = lbuf2;
			      infodata.dsize = strlen(lbuf2) + 1;
                              i_type += infodata.dsize;
                           } else {
                              atrtmp = replace_string(lbuf3, buf2+(int)strlen(lbuf3)+1, infodata.dptr, 0);
                              strcpy(lbuf2, atrtmp);
                              free_lbuf(atrtmp);
                              infodata.dptr = lbuf2;
                              infodata.dsize = strlen(lbuf2) + 1;
                              i_type += infodata.dsize;
                           }
			   if (infodata.dsize == 1) {
			      continue;
			   } else {
			       dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
                               gdcount++;
			   }
		       }
		   }
	       }
            }
	}
        free_lbuf(tpr_buff);
        if ( valid_flag ) {
	   tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
           if ( *tcim ) {
              tcimptr = strtok_r(tcim, " ", &tstrtokr);
              tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
              totcharinmail = i_type;
	      atr_add_raw(player, A_TEMPBUFFER, 
                          unsafe_tprintf("%s %d", tcimptr, totcharinmail));
           } else {
              totcharinmail = i_type;
	      atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
           }
           free_lbuf(tcim);
           count--;
           if ( gdcount == count ) {
              notify_quiet(player, 
                           unsafe_tprintf("Mail: %seditall done (%d lines edited) (%d/%d).", 
                                  (stricmp(buf1, "+editall") ? "+f" : "+"), 
                                   gdcount, totcharinmail, LBUF_SIZE - 10));
              if ( totcharinmail > (LBUF_SIZE-10) )
                 notify_quiet(player, 
                              unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                              (totcharinmail - LBUF_SIZE + 10)));
           } else if ( gdcount == 0 ) {
              notify_quiet(player, "Mail: Original string not found in +editall.");
           } else {
              notify_quiet(player, 
                   unsafe_tprintf("Mail: %seditall done (%d out of %d lines edited) (%d/%d).",
                        (stricmp(buf1, "+editall") ? "+f" : "+"), gdcount, count, 
                        totcharinmail, LBUF_SIZE - 10));
              if ( totcharinmail > (LBUF_SIZE-10) )
                 notify_quiet(player, 
                              unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                              (totcharinmail - LBUF_SIZE + 10)));
           }
        }
    } else {
	if (*buf2 != '\0') {
	    notify_quiet(player, "Mail: Extra characters ignored.  Use '\\=' to insert an equals.");
	}
	line++;
	if (line > MAXWRTL) {
	  notify_quiet(player,"MAIL ERROR: Maximum number of lines reached");
	  return;
	}
	*(int *)sbuf1 = MIND_WRTL;
	*(int *)(sbuf1 + sizeof(int)) = player;
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = line;
	keydata.dptr = sbuf1;
	keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	if (p3 != NULL) {
	    *p3 = ' ';
	}
	if (strlen(buf1) > LBUF_SIZE - 10) {
	    notify_quiet(player, "MAIL ERROR: Write line too long.");
	} else {
	    if (key & M_ALL) {
	      infodata.dptr = buf1 + 1;
	      infodata.dsize = strlen(buf1);
	      if (infodata.dsize == 1) {
		notify_quiet(player,"MAIL ERROR: Write line is blank");
		return;
	      }
	    }
	    else {
	      infodata.dptr = buf1;
	      infodata.dsize = strlen(buf1) + 1;
	    }
	    if (line == 1) {
	        tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
                totcharinmail = infodata.dsize;
                if ( *tcim ) {
                   tcimptr = strtok_r(tcim, " ", &tstrtokr);
                   tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
                } else {
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
                }
                free_lbuf(tcim);
		notify_quiet(player, unsafe_tprintf("Mail: Write message started (%d/%d).",
                                             totcharinmail, (LBUF_SIZE-10) ));
	    } else {
	        tcim = atr_get(player, A_TEMPBUFFER, &owner, &flags);
                if ( *tcim ) {
                   tcimptr = strtok_r(tcim, " ", &tstrtokr);
                   tcimptr2 = strtok_r(NULL, " ", &tstrtokr);
                   totcharinmail = infodata.dsize + atoi(tcimptr2);
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("%s %d", tcimptr, totcharinmail));
                } else {
                   totcharinmail = infodata.dsize;
	           atr_add_raw(player, A_TEMPBUFFER, unsafe_tprintf("X %d", totcharinmail));
                }
                free_lbuf(tcim);
		notify_quiet(player,unsafe_tprintf("Mail: Write line %d added (%d/%d)",line, 
                                             totcharinmail, (LBUF_SIZE-10) ));
            }
            if ( totcharinmail > (LBUF_SIZE-10) )
                notify_quiet(player, unsafe_tprintf("Mail Warning: Message is %d over limit.  Mail will be cut off.",
                             (totcharinmail - LBUF_SIZE + 10)));
	    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
	    *(int *)sbuf1 = MIND_WRTM;
	    *(int *)(sbuf1 + sizeof(int)) = player;
	    keydata.dptr = sbuf1;
	    keydata.dsize = sizeof(int) << 1;
	    *(short int *)sbuf7 = line;
	    infodata.dptr = sbuf7;
	    infodata.dsize = sizeof(short int);
	    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
	}
    }
}

void 
mail_lock(dbref player, char *buff)
{
    struct boolexp *okey;

    okey = parse_boolexp(player, buff, 0);
    if (okey == TRUE_BOOLEXP) {
	notify_quiet(player, "MAIL ERROR: I don't understand that lock.");
    } else {
	atr_add_raw(player, A_LMAIL, unparse_boolexp_quiet(player, okey));
	notify_quiet(player, "Mail Lock: Locked.");
    }
    free_boolexp(okey);
}

void mail_unlock(dbref player)
{
  atr_clr(player,A_LMAIL);
  notify_quiet(player,"Mail: Lock cleared.");
}

void mail_reject(dbref player, char *buf)
{
  *(int *)sbuf1 = MIND_REJM;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  if (!*buf) {
    dbm_delete(mailfile,keydata);
    notify_quiet(player,"Mail Reject message cleared");
  }
  else if (!stricmp(buf,"+list")) {
    infodata = dbm_fetch(mailfile,keydata);
    if (infodata.dptr)
      notify_quiet(player,unsafe_tprintf("Mail: Reject message is -> %s",infodata.dptr));
    else
      notify_quiet(player,"Mail: You have no reject message set");
  }
  else {
    if (strlen(buf) > REJECTLIM) {
      *(buf+REJECTLIM) = '\0';
      notify_quiet(player,"MAIL WARNING: Reject message truncated");
    }
    else
      notify_quiet(player,"Mail: Reject message set");
    infodata.dptr = buf;
    infodata.dsize = strlen(buf) + 1;
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
  }
}

void 
mail_share(dbref player, char *buf)
{
    struct boolexp *okey;

    if (*buf == '\0') {
	atr_clr(player, A_LSHARE);
	notify_quiet(player, "Mail: Share cleared.");
    } else {
	okey = parse_boolexp(player, buf, 0);
	if (okey == TRUE_BOOLEXP) {
	    notify_quiet(player, "MAIL ERROR: I don't understand that lock.");
	} else {
	    atr_add_raw(player, A_LSHARE, unparse_boolexp_quiet(player, okey));
	    notify_quiet(player, "Mail: Share set.");
	}
	free_boolexp(okey);
    }
}

void 
mail_time(dbref player, char *buff)
{
    dbref j;
    int i;
    char *pt1, m_buff[6];

    memset(m_buff, 0, sizeof(m_buff));
    pt1 = alloc_lbuf("mail_time");
    sprintf(m_buff, "%.5s", buff);
    i = atoi(m_buff);
    strcpy(pt1,myitoa(i));
    DO_WHOLE_DB(j) {
	if ((j % 25) == 0)
	    cache_reset(0);
	if (isPlayer(j) && !Recover(j)) {
	    if (Wizard(j))
		continue;
	    if (i < 0)
		atr_clr(j, A_MTIME);
	    else
		atr_add_raw(j, A_MTIME, pt1);
	}
    }
    free_lbuf(pt1);
    notify(player, "Mail: Done.");
}

void 
mail_md1(dbref player, dbref wiz, int key, int val, int i_all)
{
    char *pt1;
    dbref owner;
    int flags, x, y;
    short int *spt1, len;
    double mval;

    if (mudstate.mail_state != 1) {
        return;
    }
    if (get_ind_rec(player,MIND_IRCV,lbuf7,0,NOTHING,1)) {
      if (val < 0) {
	pt1 = atr_get(player, A_MTIME, &owner, &flags);
	if (*pt1)
	    mval = atof(pt1);
	else {
           if ( (mudconf.mail_autodeltime > 0) && !Wizard(player) )
              mval = (double) mudconf.mail_autodeltime;
           else
              mval = -1.0;
        }
	free_lbuf(pt1);
      } else {
	mval = (double) val;
      }
      mval *= (double) SECDAY;
      if (mval >= 0) {
	spt1 = (short int *)lbuf7;
	x = *(spt1 + 2);
	spt1 += 3 + *(spt1+1);
	for (y = 0; y < x; y++, spt1++) {
	  if ((len = get_hd_rcv_rec(player,*spt1,mbuf1,NOTHING,1))) {
	    if ( (i_all && ((*(mbuf1+hstoff) == 'N') || (*(mbuf1+hstoff) == 'U'))) || 
                 (*(mbuf1+hstoff) == 'O') || 
                 (*(mbuf1+hstoff) == 'M')) {
	      if (difftime(mudstate.now,*(time_t *)(mbuf1+htimeoff)) >= mval) {
		*(mbuf1+hstoff) = 'P';
		*(int *)sbuf1 = MIND_HRCV;
		*(int *)(sbuf1+sizeof(int)) = player;
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
		keydata.dptr = sbuf1;
		keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		infodata.dptr = mbuf1;
		infodata.dsize = len;
		dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	      }
	    }
	  }
	}
      }
    }
    mail_delete(player, NOTHING, 1, wiz, key, 1, 1);
}

int 
mail_md2(dbref player, dbref wiz, int key)
{
    char *pt1;
    dbref owner;
    int flags, x, y, count;
    short int *spt1;
    double mval;

    if (mudstate.mail_state != 1) {
        return 0;
    }
    count = 0;
    if (get_ind_rec(player,MIND_IRCV,lbuf7,0,NOTHING,1)) {
      pt1 = atr_get(player, A_MTIME, &owner, &flags);
      if (*pt1)
        mval = atof(pt1);
      else {
        if ( (mudconf.mail_autodeltime > 0) && !Wizard(player) )
           mval = (double) mudconf.mail_autodeltime;
        else
           mval = -1.0;
      }
      free_lbuf(pt1);
      mval *= (double) SECDAY;
      count = 0;
      if (mval >= 0) {
	spt1 = (short int *)lbuf7;
	x = *(spt1 + 2);
	spt1 += 3 + *(spt1+1);
	for (y = 0; y < x; y++, spt1++) {
	  if (get_hd_rcv_rec(player,*spt1,mbuf1,NOTHING,1)) {
	    if ((*(mbuf1 + hstoff) == 'O') || (*(mbuf1+hstoff) == 'M')) {
	      if (difftime(mudstate.now,*(time_t *)(mbuf1+htimeoff)) >= mval) {
		count++;
	      }
	    }
	  }
	}
      }
    }
    return count;
}
void 
mail_dtime(dbref player, char *buf1, char *buf2)
{
    dbref pl2;
    char *s_split;
    int val, i_all, i_wall, i_err;

    i_all = i_err = i_wall = 0;
    if ( *buf2 && ((s_split = strchr(buf2, ',')) != NULL) ) {
       *s_split = '\0';
       s_split++;
       if ( !stricmp(s_split, "all") ) {
          i_all = 1;
       } 
    } 
    if (*buf2) {
       *(buf2 + 5) = '\0';
       val = atoi(buf2);
    } else {
	val = -1;
    }
    if ( !stricmp(buf1, "all") || !stricmp(buf1, "wall") ) {
        if ( i_all ) {
           notify_quiet(player, "Mail: Auto-Deleting ALL mail on all players.");
        } else {
           notify_quiet(player, "Mail: Auto-Deleting mail on all players.");
        }
        if ( !stricmp(buf1, "wall") ) {
           i_wall = 1;
        }
	DO_WHOLE_DB(pl2) {
	    if ((pl2 % 25) == 0)
		cache_reset(0);
	    if (!isPlayer(pl2) || Recover(pl2) || 
                (i_wall && Wizard(pl2) && !Immortal(player)) ||
                (!i_wall && Wizard(pl2)) )
		continue;
	    mail_md1(pl2, player, 0, val, i_all);
	}
    } else {
	pl2 = lookup_player(player, buf1, 0);
	if ((pl2 != NOTHING) && (!Wizard(pl2) || (Wizard(pl2) && Immortal(player)))) {
            if ( i_all ) {
               notify_quiet(player, "Mail: Auto-Deleting ALL mail on target player.");
            } else {
               notify_quiet(player, "Mail: Auto-Deleting mail on target player.");
            }
	    mail_md1(pl2, player, 0, val, i_all);
        } else {
           if ( pl2 == NOTHING ) {
              notify_quiet(player, "Mail: Invalid player for Auto-Deletion.");
           } else {
              notify_quiet(player, "Mail: No permission for Auto-Deletion on target player.");
           }
        }
    }
    notify_quiet(player, "Mail: Done.");
}

void 
mail_smax(dbref player, char *buf1)
{
    dbref i;
    int val;
    char *pt1;

    *(buf1 + 5) = '\0';
    val = atoi(buf1);
    if ((val < 1) || (val > 1000)) {
	notify_quiet(player,"MAIL ERROR: Bad value in smax command");
	return;
    }
    *(int *)sbuf1 = MIND_SMAX;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int);
    *(short int *)sbuf2 = (short int)val;
    infodata.dptr = sbuf2;
    infodata.dsize = sizeof(short int);
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
    pt1 = myitoa(val);
    DO_WHOLE_DB(i) {
	if ((i % 25) == 0)
	    cache_reset(0);
	if (!isPlayer(i) || Wizard(i) || Recover(i))
	    continue;
	if (val > 0) {
	    atr_add_raw(i, A_MSAVEMAX, pt1);
	} else {
	    atr_clr(i, A_MSAVEMAX);
	}
    }
    notify_quiet(player, "Mail: Done.");
}

void mail_verify(dbref player)
{
  dbref i;
  int scurr;
  short int *pt1, count, x;

  DO_WHOLE_DB(i) {
    if ((i % 25) == 0)
      cache_reset(0);
    if (!isPlayer(i) || Recover(i) || Wizard(i))
      continue;
    if (get_ind_rec(player,MIND_IRCV,lbuf7,0,NOTHING,1)) {
      pt1 = (short int *)lbuf7;
      count = *(pt1+2);
      scurr = 0;
      pt1 += 3 + *(pt1+1);
      *(int *)sbuf1 = MIND_HRCV;
      *(int *)(sbuf1 + sizeof(int)) = i;
      keydata.dptr = sbuf1;
      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
      for (x = 0; x < count; x++, pt1++) {
	*(short int *)(sbuf1 + (sizeof(int) << 1)) = *pt1;
	infodata = dbm_fetch(mailfile,keydata);
	if (infodata.dptr) {
          if (*(char *)(infodata.dptr + hstoff) == 'S')
	    scurr++;
	}
      }
      if (!scurr)
	atr_clr(i, A_MSAVECUR);
      else
	atr_add_raw(i, A_MSAVECUR, myitoa(scurr));
    }
  }
  notify_quiet(player,"Mail: Done");
}

void 
mail_acheck(dbref player, int key)
{
    MAMEM *pt1;
    ATTR *attr, *attr2;
    OBLOCKMASTER master;
    int atest, cntr, ca, aflags, i_hidden, i_dchk, i_hchk, i_quick, i_len, i_truelen;
    dbref attrib, aowner, tmpdbnum;
    ANSISPLIT outsplit[LBUF_SIZE];
    char *s_shoveattr, *atr_name_ptr, *tpr_buff, *tprp_buff, *s_exec, *s_to, *s_toptr, *s_outbuff, *s_finalbuf,
         *s_fmt="%-4d %-25.25s    %-*.*s %s%s",
         *s_fmthdr="%-4s %-25.25s    %-*.*s %s%s",
         *s_dynfmtwiz="%-4d %-25.25s %c%c %-*.*s %s%s",
         *s_dynfmt="%-4d %-25.25s    %-*.*s %s%s"; 

    cntr = i_quick = 0;

    if ( key & M_QUICK ) {
       i_quick = 1;
       key &= ~M_QUICK;
    }

    *(int *)sbuf1 = MIND_GA;
    *(int *)sbuf2 = MIND_GAL;
    pt1 = mapt;

    if ( i_quick == 0 ) {
       notify_quiet(player, "---------------------------------------"\
                            "---------------------------------------");
    }
    notify_quiet(player, "Listing all static global mail aliases (+<alias>)");
    tprp_buff = tpr_buff = alloc_lbuf("mail_acheck");
    while (pt1) {
	if (Wizard(player))
	    atest = 1;
	else {
	    strcpy(sbuf2 + sizeof(int), pt1->akey);
	    keydata.dptr = sbuf2;
	    keydata.dsize = strlen(pt1->akey) + 1 + sizeof(int);
	    infodata = dbm_fetch(mailfile, keydata);
	    if (infodata.dptr)
		atest = eval_boolexp_atr(player, player, player, infodata.dptr,1, 0);
	    else
		atest = 1;
	}
	if (atest) {
	    strcpy(sbuf1 + sizeof(int), pt1->akey);
	    keydata.dptr = sbuf1;
	    keydata.dsize = strlen(pt1->akey) + 1 + sizeof(int);
	    infodata = dbm_fetch(mailfile, keydata);
	    if (infodata.dptr) {
                if ( i_quick ) {
                /* Do quick listing of mail aliases -- mux compatibility */
                   if ( cntr == 0 ) {
                      notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                            s_fmthdr,
                            (char *)"Num",
                            (char *)"Name",
                            35,35,
                            (char *)"Description",
                            (char *)"",
                            (char *)"Owner"));
                   }
                   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                            s_fmt,
                            cntr, 
                            sbuf1 + sizeof(int),          
                            35,35,
                            (char *)"N/A",
                            (char *)"",
                            (char *)"System"));

                } else {
		   unparse_al(infodata.dptr,infodata.dsize,lbuf11,0,1,player);
                   tprp_buff = tpr_buff;
                   if ( ColorMail(player) ) {
		      notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s%s--+Alias: %s%s%s", 
#ifdef ZENTY_ANSI
                                   SAFE_ANSI_HILITE,
                                   SAFE_ANSI_BLUE,
                                   SAFE_ANSI_GREEN,
                                   sbuf1 + sizeof(int),
                                   SAFE_ANSI_NORMAL
#else
                                   ANSI_HILITE,
                                   ANSI_BLUE,
                                   ANSI_GREEN,
                                   sbuf1 + sizeof(int),
                                   ANSI_NORMAL
#endif
                      ));
                   } else {
		      notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "--+Alias: %s", sbuf1 + sizeof(int)));
                   }
                   tprp_buff = tpr_buff;
		   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Contents: %s",lbuf11));
                }
                cntr++;
	    }
	}
	pt1 = pt1->next;
    }
    if ( !cntr ) {
       notify_quiet(player, "No static aliases defined.");
    }
    if ( i_quick == 0 ) {
       notify_quiet(player, "\r\n---------------------------------------"\
                            "---------------------------------------");
    }
    if ( Wizard(player) ) {
       sprintf(tpr_buff, "Listing all dynamic global mail aliases ($<alias>) [#%d]", mudconf.mail_def_object);
       notify_quiet(player, tpr_buff);
    } else {
       notify_quiet(player, "Listing all dynamic global mail aliases ($<alias>)");
    }
    free_lbuf(tpr_buff);

    cntr = 0;
    if ( Good_obj(mudconf.mail_def_object) && !Going(mudconf.mail_def_object) &&
          !Recover(mudconf.mail_def_object) ) {
       olist_init(&master);
       if (parse_attrib_wild(mudconf.mail_def_object, 
                             unsafe_tprintf("#%d/alias.*", mudconf.mail_def_object),
                             &tmpdbnum, 0, 0, 1, &master, 0, 0, 0)) {
          s_shoveattr = alloc_lbuf("mail_alias_lbuf");
          tprp_buff = tpr_buff = alloc_lbuf("mail_acheck");
          s_outbuff = alloc_lbuf("fun_ljustfill");
          for (ca = olist_first(&master); ca != NOTHING; ca = olist_next(&master)) {
             attr = atr_num(ca);
             if (attr) {
                if ( (attr->flags & AF_MDARK) && !Wizard(player) )
                   continue;
                atr_get_str(s_shoveattr, mudconf.mail_def_object, 
                            attr->number, &aowner, &aflags);
                if ( (aflags & AF_MDARK) && !Wizard(player) )
                   continue;

                i_dchk = i_hchk = i_hidden = 0;
                if ( ((aflags & AF_MDARK) || (attr->flags & AF_MDARK)) ) { 
                   i_dchk = 1;
                }
                if ( ((aflags & AF_PINVIS) || (attr->flags & AF_PINVIS)) ) { 
                   if ( !Wizard(player) ) {
                      i_hidden = 1;
                   }
                   i_hchk = 1;
                }
                if ( (atr_name_ptr = strchr(attr->name, '.')) != NULL ) {
                   tprp_buff = tpr_buff;
                   if ( i_quick ) {
                   /* Do quick listing of mail aliases -- mux compatibility */
                      if ( cntr == 0 ) {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                               s_fmthdr,
                               (char *)"Num",
                               (char *)"Name",
                               35,35,
                               (char *)"Description",
                               (char *)"",
                               (char *)"Owner"));
                      }
                      sprintf(s_shoveattr, "%s", (char *)"N/A");
                      if (parse_attrib(mudconf.mail_def_object, safe_tprintf(tpr_buff, &tprp_buff, "#%d/%s%s", 
                                       mudconf.mail_def_object, "comment", atr_name_ptr), &tmpdbnum, &attrib)) {
                         if ( attrib != NOTHING ) {
                            attr2 = atr_num(attrib);
                            if ( attr2 ) {
                               atr_get_str(s_shoveattr, mudconf.mail_def_object, 
                                           attr2->number, &aowner, &aflags);
                            }
                         }
                      }
                      memset(s_outbuff, '\0', LBUF_SIZE);
                      initialize_ansisplitter(outsplit, LBUF_SIZE);
                      split_ansi(strip_ansi(s_shoveattr), s_outbuff, outsplit);
                      *(s_outbuff+35) = '\0';
                      s_finalbuf = rebuild_ansi(s_outbuff, outsplit, 0);
                      i_truelen = strlen(s_finalbuf);
                      i_len = strlen(s_outbuff);
                      if ( Wizard(player) ) {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                  s_dynfmtwiz,
                                  cntr, 
                                  atr_name_ptr+1,
                                  ( i_dchk ? 'D' : ' '),
                                  ( i_hchk ? 'H' : ' '),
                                  35 + (i_truelen - i_len),
                                  35 + (i_truelen - i_len),
                                  s_finalbuf,
#ifdef ZENTY_ANSI
                                  SAFE_ANSI_NORMAL,
#else
                                  ANSI_NORMAL,
#endif
                                  ( Good_chk(Owner(mudconf.mail_def_object)) ?  Name(Owner(mudconf.mail_def_object)) : "N/A")));
                      } else {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                  s_dynfmt,
                                  cntr, 
                                  atr_name_ptr+1,
                                  35 + (i_truelen - i_len),
                                  35 + (i_truelen - i_len),
                                  s_finalbuf,
#ifdef ZENTY_ANSI
                                  SAFE_ANSI_NORMAL,
#else
                                  ANSI_NORMAL,
#endif
                                  ( Good_chk(Owner(mudconf.mail_def_object)) ?  Name(Owner(mudconf.mail_def_object)) : "N/A")));
                      }
                      free_lbuf(s_finalbuf);
                   } else {
                      if ( ColorMail(player) ) {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s%s--$Alias:%s %s%s", 
#ifdef ZENTY_ANSI
                                      SAFE_ANSI_HILITE,
                                      SAFE_ANSI_BLUE,
                                      SAFE_ANSI_GREEN,
                                      atr_name_ptr+1,
                                      SAFE_ANSI_NORMAL
#else
                                      ANSI_HILITE,
                                      ANSI_BLUE,
                                      ANSI_GREEN,
                                      atr_name_ptr+1,
                                      ANSI_NORMAL
#endif
                         ));
                      } else {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "--$Alias: %s", atr_name_ptr+1));
                      }
                      if ( !i_hidden ) {
                         s_exec = exec(mudconf.mail_def_object, player, player, 
                                       EV_FCHECK | EV_EVAL, s_shoveattr, (char **) NULL, 0, (char **)NULL, 0);
                         tprp_buff = tpr_buff;
                         if ( s_exec && *s_exec ) {
                            s_toptr = s_to = alloc_lbuf("mail_alias");
                            parse_tolist(player, s_exec, s_to, &s_toptr);
                            notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Contents: %s", s_to));
                            free_lbuf(s_to);
                         } else {
                            notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Contents: %s", (char *)"(EMPTY)"));
                         }
                         free_lbuf(s_exec);
                      } else {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Contents: %s", (char *)"[hidden]"));
                      }
                      if ( Wizard(player) ) {
                         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Flags:%s%s",
                              (i_dchk ? " HIDDEN" : " "), (i_hchk ? " PINVISIBLE" : " ")));
                      }
                      tprp_buff = tpr_buff;
                      if (parse_attrib(mudconf.mail_def_object, 
                                       safe_tprintf(tpr_buff, &tprp_buff, "#%d/%s%s", mudconf.mail_def_object,
                                                           "comment", atr_name_ptr), 
                                                           &tmpdbnum, &attrib)) {
                         if ( attrib != NOTHING ) {
                            attr2 = atr_num(attrib);
                            if ( attr2 ) {
                               atr_get_str(s_shoveattr, mudconf.mail_def_object, 
                                           attr2->number, &aowner, &aflags);
                               if ( *s_shoveattr == '\0' ) {
                                  notify_quiet(player, "   Comment: (none)");
                               } else {
                                  tprp_buff = tpr_buff;
                                  notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "   Comment: %.50s", 
                                               strip_returntab(strip_ansi(s_shoveattr),3)));
                               }
                            } else {
                               notify_quiet(player, "   Comment: (none)");
                            }
                         } else {
                            notify_quiet(player, "   Comment: (none)");
                         }
                      } else {
                         notify_quiet(player, "   Comment: (none)");
                      }
                   } /* i_quick */
                   cntr++;
                }
             }
          }
          if ( !cntr )
             notify_quiet(player, "No dynamic aliases defined.");
          free_lbuf(tpr_buff);
          free_lbuf(s_shoveattr);
          free_lbuf(s_outbuff);
       } else {
          notify_quiet(player, "No dynamic aliases defined.");
       }
       olist_cleanup(&master);
    } else {
       notify_quiet(player, "No dynamic aliases defined.");
    }
    if ( i_quick == 0 ) {
       notify_quiet(player, "---------------------------------------"\
                            "---------------------------------------");
    }
    notify_quiet(player, "Mail: Done.");
}
char *
mail_alias_function(dbref player, int key, char *buf1, char *buf2)
{
    datum keydata2, infodata2;
    int clrbool;
    struct boolexp *okey;
    char *outstr, *outstrptr;

    if (mudstate.mail_state != 1) {
        outstrptr = outstr = alloc_lbuf("mail_alias_function");
        safe_str("#-1 MAIL CURRENTLY DISABLED", outstr, &outstrptr);
        return(outstr);
    }

    *(int *)sbuf1 = MIND_GA;
    strcpy(sbuf1+sizeof(int),buf1);
    *(int *)sbuf2 = MIND_GAL;
    strcpy(sbuf2+sizeof(int),buf1);
    keydata.dptr = sbuf1;
    keydata.dsize = strlen(buf1) + 1 + sizeof(int);
    keydata2.dptr = sbuf2;
    keydata2.dsize = keydata.dsize;

    infodata2 = dbm_fetch(mailfile, keydata2);
    clrbool = 0;
    if ((infodata2.dptr) && *(char *)(infodata2.dptr)) {
        okey = parse_boolexp(player, infodata2.dptr, 1);
        strcpy(lbuf1, unparse_boolexp(player, okey));
        clrbool = 1;
    } else {
        strcpy(lbuf1, "");
        okey = NULL;
    }

    infodata = dbm_fetch(mailfile, keydata);
    outstrptr = outstr = alloc_lbuf("mail_alias_function");
    if (infodata.dptr) {
        unparse_al_num(infodata.dptr,infodata.dsize,lbuf11,0);
/*
 *      notify_quiet(player, unsafe_tprintf("Alias: %s\nLock: %s", buf1, lbuf1));
 *      notify_quiet(player, unsafe_tprintf("Contents: %s",lbuf11));
 */
        if ( key == 0 )
           safe_str(lbuf11, outstr, &outstrptr);
        else 
           safe_str((char *) unparse_boolexp_function(player, okey), outstr, &outstrptr);
    }
    if ( clrbool )
        free_boolexp(okey);
    return(outstr);
}

void 
mail_alias(dbref player, int key, char *buf1, char *buf2)
{
    datum keydata2, infodata2;
    int atest, *ipt1, count, dest, cntr;
    struct boolexp *okey;
    MAMEM *pt4;
    char *pt1, *pt2, *tpr_buff, *tprp_buff;

    atest = (key & WM_ALOCK);
    if (!*buf1) {
	if ((key & WM_REMOVE) != 0) {
	    *(int *)sbuf1 = MIND_GA;
	    *(int *)sbuf2 = MIND_GAL;
	    pt4 = mapt;
	    while (pt4) {
		strcpy(sbuf1+sizeof(int), pt4->akey);
		strcpy(sbuf2+sizeof(int), pt4->akey);
		if (atest) {
		    keydata.dptr = sbuf2;
		    keydata.dsize = strlen(pt4->akey) + 1 + sizeof(int);
		    dbm_delete(mailfile, keydata);
		} else {
		    keydata.dptr = sbuf1;
		    keydata.dsize = strlen(pt4->akey) + 1 + sizeof(int);
		    dbm_delete(mailfile, keydata);
		    keydata.dptr = sbuf2;
		    keydata.dsize = strlen(pt4->akey) + 1 + sizeof(int);
		    dbm_delete(mailfile, keydata);
		}
		pt4 = pt4->next;
	    }
	    if (atest) {
		notify_quiet(player, "Mail: All global alias locks removed.");
	    } else {
		mapurge();
		notify_quiet(player, "Mail: All global aliases removed.");
	    }
	} else {
	    pt4 = mapt;
	    *(int *)sbuf1 = MIND_GA;
	    *(int *)sbuf2 = MIND_GAL;
	    keydata.dptr = sbuf1;
	    keydata2.dptr = sbuf2;
            notify_quiet(player, "---------------------------------------"\
                                 "---------------------------------------");
            cntr = 0;
            tprp_buff = tpr_buff = alloc_lbuf("mail_alias");
	    while (pt4) {
		strcpy(sbuf1 + sizeof(int), pt4->akey);
		strcpy(sbuf2 + sizeof(int), pt4->akey);
		keydata.dsize = strlen(pt4->akey) + 1 + sizeof(int);
		keydata2.dsize = keydata.dsize;
		infodata2 = dbm_fetch(mailfile, keydata2);
		if ((infodata2.dptr) && *((char *)infodata2.dptr)) {
		    okey = parse_boolexp(player, infodata2.dptr, 1);
		    strcpy(lbuf1, unparse_boolexp(player, okey));
		    free_boolexp(okey);
		} else {
		    strcpy(lbuf1, "Not Set");
		}
		infodata = dbm_fetch(mailfile, keydata);
		if (infodata.dptr) {
                    cntr++;
		    unparse_al(infodata.dptr,infodata.dsize,lbuf11,0, 1, player);
                    tprp_buff = tpr_buff;
		    notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Alias: %s\r\nLock: %s", keydata.dptr + sizeof(int), lbuf1));
                    tprp_buff = tpr_buff;
		    notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Contents: %s",lbuf11));
		}
		pt4 = pt4->next;
	    }
            free_lbuf(tpr_buff);
            if ( !cntr )
               notify_quiet(player, "No static global mail aliases have been defined.");
            if ( Good_obj(mudconf.mail_def_object) && 
                 !Going(mudconf.mail_def_object) &&
                 !Recover(mudconf.mail_def_object) ) {
              notify_quiet(player, unsafe_tprintf("Dynamic global mail alias object is #%d",
                                            mudconf.mail_def_object));
            } else {
               if ( mudconf.mail_def_object > 0 )
                  notify_quiet(player, unsafe_tprintf("Dynamic global mail alias object is invalid (#%d)",
                                               mudconf.mail_def_object));
               else
                  notify_quiet(player, "There is currently no dynamic global mail alias object.");
            }
            notify_quiet(player, "---------------------------------------"\
                                 "---------------------------------------");
	    notify(player, "Mail: Done.");
	}
	return;
    }
    if (strlen(buf1) > 16) {
	notify_quiet(player, "MAIL ERROR: Global alias name too long.");
	return;
    }
    *(int *)sbuf1 = MIND_GA;
    strcpy(sbuf1+sizeof(int),buf1);
    *(int *)sbuf2 = MIND_GAL;
    strcpy(sbuf2+sizeof(int),buf1);
    keydata.dptr = sbuf1;
    keydata.dsize = strlen(buf1) + 1 + sizeof(int);
    keydata2.dptr = sbuf2;
    keydata2.dsize = keydata.dsize;
    if (strlen(buf2) == 0) {
	if ((key & WM_REMOVE) != 0) {
	    dbm_delete(mailfile, keydata2);
	    if (!atest) {
		maremove(keydata.dptr + 2);
		dbm_delete(mailfile, keydata);
		notify_quiet(player, "Mail: Global alias removed.");
	    } else {
		notify_quiet(player, "Mail: Global alias lock removed.");
	    }
	} else {
	    infodata2 = dbm_fetch(mailfile, keydata2);
	    if ((infodata2.dptr) && *((char *)infodata2.dptr)) {
		okey = parse_boolexp(player, infodata2.dptr, 1);
		strcpy(lbuf1, unparse_boolexp(player, okey));
		free_boolexp(okey);
	    } else {
		strcpy(lbuf1, "Not Set");
	    }
	    infodata = dbm_fetch(mailfile, keydata);
	    if (infodata.dptr) {
		unparse_al(infodata.dptr,infodata.dsize,lbuf11,1,0,player);
		notify_quiet(player, unsafe_tprintf("Alias: %s\r\nLock: %s", buf1, lbuf1));
		notify_quiet(player, unsafe_tprintf("Contents: %s",lbuf11));
	    }
	}
    } else {
	if (atest) {
	    infodata = dbm_fetch(mailfile, keydata);
	    if (infodata.dptr) {
		okey = parse_boolexp(player, buf2, 0);
		if (okey == TRUE_BOOLEXP) {
		    notify_quiet(player, "MAIL ERROR: I don't understand that lock.");
		} else {
		    infodata2.dptr = unparse_boolexp_quiet(player, okey);
		    infodata2.dsize = strlen(infodata2.dptr) + 1;
		    dbm_store(mailfile, keydata2, infodata2, DBM_REPLACE);
		    notify_quiet(player, "Mail: Global alias locked.");
		}
		free_boolexp(okey);
	    } else {
		notify_quiet(player, "MAIL ERROR: Global alias not found.");
	    }
	} else {
	    count = 0;
	    ipt1 = (int *)lbuf1;
	    ipt1++;
	    pt1 = buf2;
	    while (!atest) {
		while (*pt1 && isspace((int)*pt1)) pt1++;
		if (!*pt1) break;
		pt2 = pt1+1;
		while (*pt2 && !isspace((int)*pt2) && (*pt2 != ',')) pt2++;
		if (!*pt2)
		  atest=1;
		else
		  *pt2 = '\0';
		dest = lookup_player(player,pt1,0);
		if (dest != NOTHING) {
		  *ipt1++ = dest;
		  count++;
		}
		else
		  notify_quiet(player,"MAIL ERROR: Bad player in alias");
		pt1 = pt2+1;
		if (count >= absmaxrcp) atest = 1;
	    }
	    if (count) {
		*(int *)lbuf1 = count;
		infodata.dptr = lbuf1;
		infodata.dsize = sizeof(int) * (count+1);
		mapush(keydata.dptr + sizeof(int));
		dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
		notify_quiet(player, "Mail: Global alias set.");
	    }
	}
    }
}

void mail_onoff(dbref player, int key)
{
  if (!mudstate.mail_state) {
    notify_quiet(player,"MAIL ERROR: Mail not active due to error at startup");
  }
  else if (key == WM_ON) {
    mudstate.mail_state = 1;
    mnuke_read();
    notify_quiet(player,"Mail: Mail system turned on");
  }
  else {
    mudstate.mail_state = 2;
    notify_quiet(player,"Mail: Mail system turned off");
  }
}

void mail_unload(dbref player)
{
  FILE *dump1, *dump2;
  short int *pt1, x, y, count;
  char *pt2;
  int *ipt1;

  dump1 = fopen(dumpname,"w");
  dump2 = fopen(fdumpname,"w");
  if (!dump1 || !dump2) {
    mail_error(player,MERR_DUMP);
    return;
  }
  keydata = dbm_firstkey(mailfile);
  if (!keydata.dptr) {
    notify_quiet(player,"Mail: No mail data to dump");
    fclose(dump1);
  }
  else {
    do {
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr) {
	memcpy(sbuf7,keydata.dptr,keydata.dsize);
	switch (*(int *)sbuf7) {
	  case MIND_IRCV: fprintf(dump1,"A%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_ISND: fprintf(dump1,"B%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_HRCV: fprintf(dump1,"C%d/%d\1\n",*(int *)(sbuf7 + sizeof(int)),
			  *(short int *)(sbuf7 + (sizeof(int) << 1)));
			  break;
	  case MIND_HSND: fprintf(dump1,"D%d/%d\1\n",*(int *)(sbuf7 + sizeof(int)),
			  *(short int *)(sbuf7 + (sizeof(int) << 1)));
			  break;
	  case MIND_MSG: fprintf(dump1,"E%d/%d\1\n",*(int *)(sbuf7 + sizeof(int)),
			  *(short int *)(sbuf7 + (sizeof(int) << 1)));
			  break;
	  case MIND_BSIZE: fprintf(dump1,"F%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_REJM: fprintf(dump1,"G%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_WRTM: fprintf(dump1,"H%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_WRTL: fprintf(dump1,"I%d/%d\1\n",*(int *)(sbuf7 + sizeof(int)),
			  *(short int *)(sbuf7 + (sizeof(int) << 1)));
			  break;
	  case MIND_WRTS: fprintf(dump1,"J%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			  break;
	  case MIND_GA: fprintf(dump1,"K%s\1\n",(char *)(keydata.dptr+sizeof(int)));
			break;
	  case MIND_GAL: fprintf(dump1,"L%s\1\n",(char *)(keydata.dptr+sizeof(int)));
			break;
	  case MIND_ACS: fputs("M\1\n",dump1);
			break;
	  case MIND_PAGE: fprintf(dump1,"N%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			break;
	  case MIND_AFOR: fprintf(dump1,"O%d\1\n",*(int *)(sbuf7 + sizeof(int)));
			break;
	  case MIND_SMAX: fputs("P\1\n",dump1);
	}
	memcpy(lbuf12,infodata.dptr,infodata.dsize);
	switch (*(int *)sbuf7) {
	  case MIND_MSG:
	  case MIND_REJM:
	  case MIND_WRTL:
	  case MIND_WRTS:
	  case MIND_GAL:
		fputs(infodata.dptr,dump1);
		fputs("\1\n",dump1);
		break;
	  case MIND_BSIZE:
	  case MIND_WRTM:
	  case MIND_PAGE:
	  case MIND_SMAX:
		fprintf(dump1,"%d\1\n",*(short int *)lbuf12);
		break;
	  case MIND_AFOR:
		fprintf(dump1,"%d\1\n",*(int *)lbuf12);
		break;
	  case MIND_GA:
		ipt1 = (int *)lbuf12;
		count = 1 + *(ipt1);
		for (x = 0; x < count; x++) {
		  fprintf(dump1,"%d",*(ipt1+x));
		  if (x < count-1) fputc('/',dump1);
		}
		fputs("\1\n",dump1);
		break;
	  case MIND_IRCV:
	  case MIND_ISND:
		pt1 = (short int *)lbuf12;
		count = 3 + *(pt1+1) + *(pt1+2);
		for (x = 0; x < count; x++) {
		  fprintf(dump1,"%d",*(pt1+x));
		  if (x < count-1) fputc('/',dump1);
		}
		fputs("\1\n",dump1);
		break;
	  case MIND_HRCV:
		pt2 = lbuf12;
		if (infodata.dsize == hsuboff)
		  fprintf(dump1,"%d/%ld/%d/%d/%c%c%c\1\n",*(int *)pt2,
			*(time_t *)(pt2+htimeoff),
			*(short int *)(pt2+hindoff),*(short int *)(pt2+hsizeoff),
			*(pt2+hstoff),*(pt2+1+hstoff),*(pt2+2+hstoff));
		else
		  fprintf(dump1,"%d/%ld/%d/%d/%c%c%c%s\1\n",*(int *)pt2,
			*(time_t *)(pt2+htimeoff),
			*(short int *)(pt2+hindoff),*(short int *)(pt2+hsizeoff),
			*(pt2+hstoff),*(pt2+1+hstoff),*(pt2+2+hstoff),pt2+hsuboff);
		break;
	  case MIND_HSND:
		pt2 = lbuf12;
		x = sizeof(int) << 1;
		count = (short int)(*(int *)pt2);
		fprintf(dump1,"%d/",count);
		pt2 += sizeof(int);
		for(y = 0; y < count; y++) {
		  fprintf(dump1,"%d/%d",*(int *)pt2,*(int *)(pt2 + sizeof(int)));
		  if (y < count-1) {
		    fputc('/',dump1);
		    pt2 += x;
		  }
		}
		fputs("\1\n",dump1);
		break;
	  case MIND_ACS:
		ipt1 = (int *)lbuf12;
		x = (int)*ipt1;
		x++;
		for (y = 0; y < x; y++, ipt1++) {
		  fputs(myitoa(*ipt1),dump1);
		  if (y < x-1) {
		    fputc('/',dump1);
		  }
		}
		fputs("\1\n",dump1);
	}
      }
      keydata = dbm_nextkey(mailfile);
    } while(keydata.dptr);
    fclose(dump1);
  }
  keydata = dbm_firstkey(foldfile);
  if (!keydata.dptr) {
    notify_quiet(player,"Mail: No folder data to unload");
    fclose(dump2);
  }
  else {
    do {
      infodata = dbm_fetch(foldfile,keydata);
      if (infodata.dptr) {
	memcpy(sbuf7,keydata.dptr,keydata.dsize);
	switch (*(int *)sbuf7) {
	  case FIND_LST: fprintf(dump2,"A%d\1\n",*(int *)(sbuf7+sizeof(int)));
			 break;
	  case FIND_BOX: fprintf(dump2,"B%d/%s\1\n",*(int *)(sbuf7+sizeof(int)),
			 sbuf7 + (sizeof(int) << 1));
			 break;
	  case FIND_CURR: fprintf(dump2,"C%d\1\n",*(int *)(sbuf7+sizeof(int)));
			 break;
	  case FIND_CSHR: fprintf(dump2,"D%d\1\n",*(int *)(sbuf7+sizeof(int)));
	}
	memcpy(lbuf12,infodata.dptr,infodata.dsize);
	switch (*(int *)sbuf7) {
	  case FIND_LST:
	  case FIND_CURR:
	  case FIND_CSHR:
		fputs(lbuf12,dump2);
		fputs("\1\n",dump2);
		break;
	  case FIND_BOX:
		pt1 = (short int *)lbuf12;
		count = *pt1 + 1;
		for (x = 0; x < count; x++) {
		  fprintf(dump2,"%d",*(pt1+x));
		  if (x < count - 1)
		    fputc('/',dump2);
		}
		fputs("\1\n",dump2);
	}
      }
      keydata = dbm_nextkey(foldfile);
    } while (keydata.dptr);
    fclose(dump2);
  }
  notify_quiet(player,"Mail: Mail and folder databases unloaded");
  time(&mudstate.mailflat_time);
}

void myfgets(char *buf, FILE *fpt)
{
  char *pt1, in;

  pt1 = buf;
  in = fgetc(fpt);
  while (!feof(fpt) && (in) && (in != '\1')) {
    *pt1++ = in;
    in = fgetc(fpt);
  }
  *pt1 = '\0';
  if (in == '\1')
    fgetc(fpt);
}

void mnuke_save(dbref player, char *buf)
{
  FILE *nuke1;
  dbref obj;

  if (mudstate.nuke_status)
    obj = atoi(buf);
  else
    obj = lookup_player(player,buf,0);
  if (obj != NOTHING) {
    nuke1 = fopen(nukename, "w+");
    if (!nuke1) {
      notify_quiet(player, "MAIL ERROR: Nuke log open failed.  Do not restart mail system");
      return;
    }
    fputs(myitoa(obj),nuke1);
    fputs("\1\n",nuke1);
    notify_quiet(player,"Mail: User added to wipe list");
    fclose(nuke1);
  }
  else
    notify_quiet(player,"MAIL ERROR: Bad name in wipe command. Name not added to list");
}

void mnuke_read()
{
  FILE *nuke1;
  char buf1[LBUF_SIZE], in1;

  nuke1 = fopen(nukename, "r");
  if (nuke1) {
    mudstate.nuke_status = 1;
    while (1) {
      in1 = fgetc(nuke1);
      if(feof(nuke1))
         break;
      *buf1 = in1;
      myfgets(buf1+1,nuke1);
      mail_wipe(GOD,buf1);
    }
    mudstate.nuke_status = 0;
    fclose(nuke1);
    remove(nukename);
  }
}

void mail_load(dbref player)
{
  FILE *dump1, *dump2;
  char input, *pt1, *pt2, *pt3;
  short int gen, gen2, *spt1;
  int *ipt1;
/* needed due to possibly large input lines */
#if 0
#ifdef QDBM
  #ifdef LBUF64
    char hbuf1[192000];
  #else
    #ifdef LBUF32
      char hbuf1[96000];
    #else
      #ifdef LBUF16
        char hbuf1[48000];
      #else
        #ifdef LBUF8
        	char hbuf1[24000];
        #else
	        char hbuf1[12000];
        #endif
      #endif
    #endif
  #endif
#else
  char hbuf1[12000];
#endif
#endif
  char hbuf1[192000];
  dump1 = fopen(dumpname, "r");
  dump2 = fopen(fdumpname, "r");
  if (!dump1 || !dump2) {
    mail_error(player,MERR_DUMP);
    return;
  }
  mudstate.mail_state = 0;
  dbm_close(mailfile);
  dbm_close(foldfile);
  system(unsafe_tprintf("rm \"%s\"*", mailname));
  system(unsafe_tprintf("rm \"%s\"*", foldname));
  mailfile = dbm_open(mailname, O_RDWR | O_CREAT, 00664);
  foldfile = dbm_open(foldname, O_RDWR | O_CREAT, 00664);
  if (!mailfile || !foldfile) {
    mail_error(player,MERR_DB);
    return;
  }
  while (1) {
    input = fgetc(dump1);
    if(feof(dump1))
       break;
    input = input - 'A' + 1;
    *(int *)sbuf1 = (int)input;
    keydata.dptr = sbuf1;
    myfgets(hbuf1,dump1);
    switch (input) {
	case MIND_IRCV:
	case MIND_ISND:
	case MIND_BSIZE:
	case MIND_REJM:
	case MIND_WRTM:
	case MIND_WRTS:
	case MIND_PAGE:
	case MIND_AFOR:
		*(int *)(sbuf1 + sizeof(int)) = atoi(hbuf1);
		keydata.dsize = sizeof(int) << 1;
		break;
	case MIND_HRCV:
	case MIND_HSND:
	case MIND_MSG:
	case MIND_WRTL:
		pt1 = strchr(hbuf1,'/');
		*pt1 = '\0';
		*(int *)(sbuf1 + sizeof(int)) = atoi(hbuf1);
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = (short int)atoi(pt1+1);
		keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		break;
	case MIND_GA:
	case MIND_GAL:
		strcpy(sbuf1+sizeof(int),hbuf1);
		keydata.dsize = 1 + sizeof(int) + strlen(hbuf1);
		break;
	case MIND_ACS:
	case MIND_SMAX:
		keydata.dsize = sizeof(int);
    }
    myfgets(hbuf1,dump1);
    switch (input) {
	case MIND_MSG:
	case MIND_REJM:
	case MIND_WRTL:
	case MIND_WRTS:
	case MIND_GAL:
		infodata.dptr = hbuf1;
		infodata.dsize = 1 + strlen(hbuf1);
		break;
	case MIND_BSIZE:
	case MIND_WRTM:
	case MIND_PAGE:
	case MIND_SMAX:
		*(short int *)sbuf2 = (short int)atoi(hbuf1);
		infodata.dptr = sbuf2;
		infodata.dsize = sizeof(short int);
		break;
	case MIND_AFOR:
		*(int *)sbuf2 = atoi(hbuf1);
		infodata.dptr = sbuf2;
		infodata.dsize = sizeof(int);
		break;
	case MIND_GA:
		pt1 = hbuf1;
		ipt1 = (int *)lbuf2;
		pt2=strchr(pt1+1,'/');
		*pt2 = '\0';
		*ipt1++ = atoi(pt1);
		pt1 = pt2+1;
		gen = *(ipt1-1);
		for (gen2 = 0; gen2 < gen; gen2++) {
		  pt2=strchr(pt1+1,'/');
		  if (pt2) 
		    *pt2 = '\0';
		  *ipt1++ = atoi(pt1);
		  pt1 = pt2+1;
		}
		ipt1 = (int *)lbuf2;
		infodata.dptr = lbuf2;
		infodata.dsize = sizeof(int) * (1 + *ipt1);
		break;
	case MIND_IRCV:
	case MIND_ISND:
		pt1 = hbuf1;
		spt1 = (short int *)lbuf2;
		for (gen = 0; gen < 3; gen++) {
		  pt2=strchr(pt1+1,'/');
		  *pt2 = '\0';
		  *spt1++ = (short int)atoi(pt1);
		  pt1 = pt2+1;
		}
		gen = *(spt1-1) + *(spt1-2);
		for (gen2 = 0; gen2 < gen; gen2++) {
		  pt2=strchr(pt1+1,'/');
		  if (pt2) 
		    *pt2 = '\0';
		  *spt1++ = (short int)atoi(pt1);
		  pt1 = pt2+1;
		}
		spt1 = (short int *)lbuf2;
		infodata.dptr = lbuf2;
		infodata.dsize = sizeof(short int) * (3 + *(spt1+1) + *(spt1+2));
		break;
	case MIND_HSND:
		pt3 = lbuf2;
		pt1 = hbuf1;
		pt2 = strchr(pt1+1,'/');
		*pt2 = 0;
		*(int *)pt3 = gen = atoi(pt1);
		pt3 += sizeof(int);
		pt1 = pt2+1;
		for (gen2 = 0; gen2 < gen; gen2++) {
		  pt2=strchr(pt1+1,'/');
		  *pt2 = '\0';
		  *(int *)pt3 = atoi(pt1);
		  pt3 += sizeof(int);
		  pt2++;
		  pt1=strchr(pt2,'/');
		  if (pt1) 
		    *pt1 = '\0';
		  *(int *)pt3 = atoi(pt2);
		  pt3 += sizeof(int);
		  pt1++;
		}
		infodata.dptr = lbuf2;
		infodata.dsize = pt3-lbuf2;
		break;
	case MIND_HRCV:
		pt1 = strchr(hbuf1,'/');
		*pt1 = '\0';
		*(int *)(lbuf2) = atoi(hbuf1);
		pt1++;
		pt2 = strchr(pt1,'/');
		*pt2 = '\0';
		*(time_t *)(lbuf2+htimeoff) = (time_t)atoi(pt1);
		pt2++;
		pt1 = strchr(pt2,'/');
		*pt1 = '\0';
		*(short int *)(lbuf2+hindoff) = (short int)atoi(pt2);
		pt1++;
		pt2 = strchr(pt1,'/');
		*pt2 = '\0';
		*(short int *)(lbuf2+hsizeoff) = (short int)atoi(pt1);
		pt2++;
		*(lbuf2+hstoff) = *pt2;
		*(lbuf2+1+hstoff) = *(pt2+1);
		*(lbuf2+2+hstoff) = *(pt2+2);
		pt2 += 3;
		if (*pt2) {
		  strncpy(lbuf2+hsuboff,pt2,LBUF_SIZE - hsuboff - 2);
		  infodata.dsize = hsuboff + strlen(pt2) + 1;
		}
		else
		  infodata.dsize = hsuboff;
		infodata.dptr = lbuf2;
		break;
	case MIND_ACS:
		ipt1 = (int *)lbuf2;
		pt2 = strchr(hbuf1+1,'/');
		*pt2 = '\0';
		*ipt1 = atoi(hbuf1);
		gen = (short int)*ipt1;
		ipt1++;
		pt1 = pt2+1;
		for (gen2 = 0; gen2 < gen; gen2++, ipt1++) {
		  pt2 = strchr(pt1,'/');
		  if (pt2)
		    *pt2 = '\0';
		  *ipt1 = atoi(pt1);
		  pt1 = pt2+1;
		}
		infodata.dptr = lbuf2;
		infodata.dsize = sizeof(int) * (1 + *(int *)lbuf2);
    }
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
  }
  fclose(dump1);
  while (1) {
    input = fgetc(dump2);
    if(feof(dump2))
       break;
    input = input - 'A' + 1;
    *(int *)sbuf1 = (int)input;
    keydata.dptr = sbuf1;
    myfgets(hbuf1,dump2);
    switch (input) {
	case FIND_LST:
	case FIND_CURR:
	case FIND_CSHR:
		*(int *)(sbuf1 + sizeof(int)) = atoi(hbuf1);
		keydata.dsize = sizeof(int) << 1;
		break;
	case FIND_BOX:
		pt1 = strchr(hbuf1,'/');
		*pt1 = '\0';
		pt1++;
		*(int *)(sbuf1 + sizeof(int)) = atoi(hbuf1);
		strcpy(sbuf1 + (sizeof(int) << 1),pt1);
		keydata.dsize = 1 + (sizeof(int) << 1) + strlen(pt1);
    }
    myfgets(hbuf1,dump2);
    switch (input) {
	case FIND_LST:
	case FIND_CURR:
	case FIND_CSHR:
		infodata.dptr = hbuf1;
		infodata.dsize = 1 + strlen(hbuf1);
		break;
	case FIND_BOX:
		pt1 = hbuf1;
		spt1 = (short int *)lbuf2;
		pt2=strchr(pt1+1,'/');
		*pt2 = '\0';
		*spt1++ = (short int)atoi(pt1);
		pt1 = pt2+1;
		gen = *(spt1-1);
		for (gen2 = 0; gen2 < gen; gen2++) {
		  pt2=strchr(pt1+1,'/');
		  if (pt2) 
		    *pt2 = '\0';
		  *spt1++ = (short int)atoi(pt1);
		  pt1 = pt2+1;
		}
		infodata.dptr = lbuf2;
		infodata.dsize = sizeof(short int) * (1 + *(short int *)lbuf2);
    }
    dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
  }
  fclose(dump2);
  mudstate.mail_state = 1; 
  notify_quiet(player,"Mail: Mail and folder databases loaded");
}

void mail_clean(dbref player)
{
  mail_unload(player);
  mail_load(player);
}

void mail_restart(dbref player)
{
  mail_close();
  mudstate.mail_state = mail_init();
  notify_quiet(player,"Mail: Mail system restarted");
}

void 
mail_size(dbref player, char *buf1, char *buf2)
{
    short int mod;
    dbref target;
    char *pt1;

    target = lookup_player(player, buf1, 0);
    mod = (short int)atoi(buf2);
    pt1 = buf2;
    if (*pt1 == '-')
	pt1++;
    while (isdigit((int)*pt1))
	pt1++;
    if (target == NOTHING) {
	notify_quiet(player, "MAIL ERROR: Invalid player name specified in size command.");
    } else if (!Controls(player, target)) {
	notify_quiet(player, "MAIL ERROR: Permission denied.");
//  } else if ((*pt1 != '\0') || ((mod < 10) && (mod)) || (mod > 9999) || (mod > absmaxinx)) {
    } else if ((*pt1 != '\0') || ((mod < 10) && (mod)) || (mod > absmaxinx)) {
//	notify_quiet(player, "MAIL ERROR: Invalid size modifier given.");
        notify_quiet(player, unsafe_tprintf("MAIL ERROR: Invalid size modifier given. [Must be between %d and %d]", 10, absmaxinx));
    } else {
	*(int *)sbuf1 = MIND_BSIZE;
	*(int *)(sbuf1 + sizeof(int)) = target;
	keydata.dptr = sbuf1;
	keydata.dsize = sizeof(int) << 1;
	if (!mod) {
	    dbm_delete(mailfile, keydata);
	    notify_quiet(player, unsafe_tprintf("Mailbox size for: %s has been reset to the default size.", Name(target)));
	} else {
	    *(short int *)sbuf2 = mod;
	    infodata.dptr = sbuf2;
	    infodata.dsize = sizeof(short int);
	    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
	    notify_quiet(player, unsafe_tprintf("Mailbox size for: %s has been changed to %d",
					 Name(target),mod ));
	}
    }
}

void 
mail_quota(dbref player, char *buf1, int key, int *i_used, int *i_saved, int *i_sent, int *i_umax, int *i_samax, int *i_semax)
{
    dbref pcheck;
    int count, smax, dummy1, dummy2, i_pcheck;
    short int *pt1;
    char *pt2;

    count = smax = 0;
    if (*buf1) {
      if (!Wizard(player)) {
        if ( !key )
	   notify_quiet(player,"MAIL ERROR: Improper quota format");
        *i_used = *i_saved = *i_sent = -1;
        *i_umax = *i_samax = *i_semax = -1;
	return;
      }
      else {
	pcheck = lookup_player(player, buf1, 0);
	if ((pcheck == NOTHING) || (!Controls(player,pcheck))) {
           if ( !key )
	      notify_quiet(player,"MAIL ERROR: Bad player or not allowed");
           *i_used = *i_saved = *i_sent = -1;
           *i_umax = *i_samax = *i_semax = -1;
	  return;
	}
      }
    }
    else
      pcheck = player;
    if (get_ind_rec(pcheck,MIND_IRCV,lbuf1,0,NOTHING,1)) {
	pt1 = (short int *)lbuf1;
	count = *(pt1 + 2);
    }
    else
	count = 0;
    i_pcheck = (int)get_box_size(pcheck);
    *i_umax = i_pcheck;
    *i_used = count;
    if ( !key )
       notify_quiet(player, unsafe_tprintf("Mail: %d of %d mail quota used.", count,
                    *i_umax));
    if (!Wizard(pcheck)) {
      pt2 = atr_get(pcheck, A_MSAVEMAX, &dummy1, &dummy2);
      if (*pt2)
	smax = atoi(pt2);
      else {
	*(int *)sbuf1 = MIND_SMAX;
	keydata.dptr = sbuf1;
	keydata.dsize = sizeof(int);
	infodata = dbm_fetch(mailfile,keydata);
	if (infodata.dptr) {
	  bcopy(infodata.dptr, sbuf1, sizeof(short int));
	  smax = (int)(*(short int *)sbuf1);
	}
	else
	  savemax = STARTSMAX;
      }
      free_lbuf(pt2);
      pt2 = atr_get(pcheck, A_MSAVECUR, &dummy1, &dummy2);
      count = atoi(pt2);
      *i_saved = count;
      *i_samax = smax;
      if ( !key )
         notify_quiet(player, unsafe_tprintf("Mail: %d of %d saved messages.", count, smax));
      free_lbuf(pt2);
    }
    if (Wizard(player)) {
      if (get_ind_rec(pcheck, MIND_ISND, lbuf1, 0, player, 0)) {
	pt1 = (short int *)lbuf1;
	count = *(pt1+2);
      }
      else
	count = 0;
      *i_sent = count;
      *i_semax = absmaxinx;
      if ( !key )
         notify_quiet(player, unsafe_tprintf("Mail: %d of %d sent messages.", count, absmaxinx));
    }
}

void mail_page(dbref player, char *buf)
{
  int num;

  *(int *)sbuf1 = MIND_PAGE;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  if (*buf) {
    if (!is_number(buf)) {
      notify_quiet(player,"MAIL ERROR: Bad number in page command");
      return;
    }
    num = atoi(buf);
    if (num > absmaxinx) {
      notify_quiet(player,"MAIL ERROR: Bad number in page command");
      return;
    }
    else if (num < 1) {
      dbm_delete(mailfile,keydata);
      notify_quiet(player,"Mail: Paging reset");
      return;
    }
  }
  else
    num = 0;
  if (!num) {
    infodata = dbm_fetch(mailfile,keydata);
    if (infodata.dptr) {
      memcpy(sbuf2,infodata.dptr,sizeof(short int));
      notify_quiet(player,unsafe_tprintf("Mail: Your paging value is -> %d",*(short int *)sbuf2));
    }
    else
      notify_quiet(player,"Mail: You have no paging value");
  }
  else {
    *(short int *)sbuf2 = (short int)num;
    infodata.dptr = sbuf2;
    infodata.dsize = sizeof(short int);
    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
    notify_quiet(player,"Mail: Paging value set");
  }
}

void 
mail_sread(dbref player, int key)
{
    char *pt1, *pt2;
    dbref owner;
    int flags, prev;

    prev = 0;
    if ( key & M_PREV ) {
       prev = 1;
       key &= ~M_PREV;
    }

    pt1 = atr_get(player, A_MCURR, &owner, &flags);
    if (!*pt1) {
	notify_quiet(player, "MAIL ERROR: You have no current mail message.");
    } else {
	if (key == M_ZAP) {
	    strcpy(sbuf6, pt1);
	    mail_mark(player, M_MARK, sbuf6, NOTHING, 1);
	}
	flags = atoi(pt1);
        if ( prev )
           flags--;
        else 
	   flags++;
	pt2 = myitoa(flags);
	mail_read(player, pt2, NOTHING, 1);
    }
    free_lbuf(pt1);
}

void 
mail_access(dbref player, char *buf1, char *buf2)
{
    char *p1;
    dbref obj;
    int *ipt1, x, y;

    *(int *)sbuf1 = MIND_ACS;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int);
    if (stricmp(buf1, "+list") == 0) {
	infodata = dbm_fetch(mailfile, keydata);
	if (!infodata.dptr) {
	    notify_quiet(player, "Mail: No access list exists.");
	} else {
	    memcpy(lbuf12,infodata.dptr,infodata.dsize);
	    ipt1 = (int *)lbuf12;
	    x = *ipt1;
	    *lbuf1 = '\0';
	    ipt1++;
	    for (y = 0; y < x; y++, ipt1++) {
	      p1 = Name(*ipt1);
	      if ((strlen(lbuf1) + strlen(p1)) > LBUF_SIZE - 2) 
		break;
	      else {
		strcat(lbuf1,p1);
		strcat(lbuf1," ");
	      }
	    }
	    notify_quiet(player,"Mail: Access denied to:");
	    notify_quiet(player,lbuf1);
	}
    } else if (stricmp(buf1, "+add") == 0) {
	if (*buf2 == '\0') {
	    notify_quiet(player, "MAIL ERROR: No user specified in mail access.");
	} else if ((isalnum((int)*buf2)) || (*buf2 == '#')) {
	    if (*buf2 == '#') {
		obj = atoi(buf2 + 1);
		if (obj == 0)
		    obj = NOTHING;
	    } else {
		obj = lookup_player(player, buf2, 0);
	    }
	    if (!Good_obj(obj) || !isPlayer(obj)) {
		notify_quiet(player, "MAIL ERROR: Bad user given in mail access.");
	    } else {
		infodata = dbm_fetch(mailfile, keydata);
		if (!infodata.dptr) {
		  ipt1 = (int *)lbuf1;
		  *ipt1 = 1;
		  *(ipt1 + 1) = obj;
		  infodata.dsize = sizeof(int) << 1;
		}
		else {
		  memcpy(lbuf1,infodata.dptr,infodata.dsize);
		  ipt1 = (int *)lbuf1;
		  if (*ipt1 >= absmaxacs) {
		    notify_quiet(player,"MAIL ERROR: Mail lockout is full");
		    return;
		  }
		  (*ipt1)++;
		  *(ipt1 + *ipt1) = obj;
		  infodata.dsize += sizeof(int);
		}
		infodata.dptr = lbuf1;
		dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
		notify_quiet(player,unsafe_tprintf("Mail: %s locked out",Name(obj)));
	    }
	}

    } else if (stricmp(buf1, "+remove") == 0) {
	if (*buf2 == '\0') {
	    notify_quiet(player, "MAIL ERROR: No user specified in access.");
	} else if ((isalpha((int)*buf2)) || (*buf2 == '#')) {
	    if (*buf2 == '#') {
		obj = atoi(buf2 + 1);
		if (obj == 0)
		    obj = NOTHING;
	    } else {
		obj = lookup_player(player, buf2, 0);
	    }
	    if (!Good_obj(obj)) {
		notify_quiet(player, "MAIL ERROR: Bad object specified in mail access.");
	    } else {
		infodata = dbm_fetch(mailfile, keydata);
		if (!infodata.dptr) {
		    notify_quiet(player, "Mail: No access list exists.");
		} else {
		    memcpy(lbuf12,infodata.dptr,infodata.dsize);
		    ipt1 = (int *)lbuf12;
		    x = *ipt1;
		    ipt1++;
		    for (y = 0; y < x; y++, ipt1++) {
			if (*ipt1 == obj) break;
		    }
		    if (y == x) {
			notify_quiet(player, "Mail: User not in access list");
		    }
		    else {
			if (y < (x-1)) {
			  memcpy(ipt1,ipt1+1,sizeof(int) * (x-y+2));
			}
			if (x == 1) {
			  dbm_delete(mailfile,keydata);
			}
			else {
			  ipt1 = (int *)lbuf12;
			  (*ipt1)--;
			  infodata.dptr = lbuf12;
			  infodata.dsize -= sizeof(int);
			  dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
			}
			notify_quiet(player,unsafe_tprintf("Mail: %s no longer locked out",Name(obj)));
		    }
		}
	    }
	}
    } else {
	notify_quiet(player, "MAIL ERROR: Bad access command.");
    }
}

void 
mail_pass(dbref player, char *buf1, char *buf2)
{
    dbref owner;
    int flags;
    char *p1, ps;

    ps = 0;
    p1 = atr_get(player, A_MPASS, &owner, &flags);
    if (*p1 == '\0') {
	ps = 1;
    } else {
	free_lbuf(p1);
	p1 = atr_get(player, A_MPSET, &owner, &flags);
	if (*p1 != '\0') {
	    ps = 1;
	}
    }
    free_lbuf(p1);
    if (stricmp(buf1, "+set") == 0) {
	if (ps == 0) {
	    notify_quiet(player, "Mail: Permission denied.");
	} else if (*buf2 == '\0') {
	    atr_clr(player, A_MPSET);
	    atr_clr(player, A_MPASS);
	    notify_quiet(player, "Mail: Password cleared.");
	} else {
	    if (!ok_password(buf2, (char *)NULL, player, 1) || (*buf2 == '+')) {
		notify_quiet(player, "MAIL ERROR: Bad password given in +set.");
	    } else {
		atr_add_raw(player, A_MPASS, mush_crypt(buf2, 0));
		atr_add_raw(player, A_MPSET, "SET");
		notify_quiet(player, "Mail: Password set.");
	    }
	}
    } else if (*buf2 != '\0') {
	notify_quiet(player, "MAIL ERROR: Extra characters in password command.");
    } else if (ps == 1) {
	notify_quiet(player, "Mail: Password already entered.");
    } else {
	if ((*buf1 != '+') && (check_pass(player, buf1, 1, 0, NOTHING))) {
	    atr_add_raw(player, A_MPSET, "SET");
	    notify_quiet(player, "Mail: Password accepted.");
	} else {
	    notify_quiet(player, "Mail: Password rejected.");
	}
    }
}

void mail_over(dbref player, char *buf1)
{
  dbref dest;

  dest = lookup_player(player,buf1,0);
  if (dest == NOTHING) {
    notify_quiet(player,"MAIL ERROR: Bad player name");
  }
  else if (!Controls(player,dest) && !God(player)) {
    notify_quiet(player,"MAIL ERROR: Permission denied");
  }
  else {
    atr_clr(dest, A_MPSET);
    atr_clr(dest, A_MPASS);
    notify_quiet(player,"Mail: User mail password reset");
  }
}

void 
mail_check(dbref player, char *buf1, char *buf2)
{
    dbref dest, owner;
    int flags;
    char *p1;

    dest = lookup_player(player, buf1, 0);
    if (dest == NOTHING) {
	notify_quiet(player, "MAIL ERROR: That player does not exist.");
    } else {
	p1 = atr_get(dest, A_LSHARE, &owner, &flags);
	if ( (*p1 == '\0') && ((!Wizard(player) || !Controls(player, dest)) ||
             MailLockDown(player)) ) {
	    notify_quiet(player, "Mail: Permission denied.");
	} else if (!could_doit(player, dest, A_LSHARE,1,0) && (!Immortal(player) || !Controls(player, dest))) {
	    notify_quiet(player, "Mail: Permission denied.");
	} else {
	    mail_read(dest, buf2, player, 0);
	}
	free_lbuf(p1);
    }
}

void 
mail_number(dbref player, char *buf1, char *buf2)
{
    dbref dest, owner;
    int flags;
    char *p1;

    dest = lookup_player(player, buf1, 0);
    if (dest == NOTHING) {
	notify_quiet(player, "MAIL ERROR: That player does not exist.");
    } else {
	p1 = atr_get(dest, A_LSHARE, &owner, &flags);
	if ( (*p1 == '\0') && ((!Wizard(player) || !Controls(player, dest)) ||
             MailLockDown(player)) ) {
	    notify_quiet(player, "Mail: Permission denied.");
	} else if (!could_doit(player, dest, A_LSHARE,1,0) && (!Wizard(player) || !Controls(player, dest))) {
	    notify_quiet(player, "Mail: Permission denied.");
	} else {
	    mail_status(dest, buf2, player, 0, 1, (char *)NULL, (char *)NULL);
	}
	free_lbuf(p1);
    }
}

void mail_wipe(dbref player, char *buf)
{
  dbref dest;
  short int line, *spt1, x, y, index, *spt2, ttest;
  char *pt1, *pt2, toall;
  int term, len, nindnum, t2, ct1, len2, chng1, *ipt1, *ipt2;
  MAMEM *mpt1, *mpt2;

  if (mudstate.nuke_status)
    dest = atoi(buf);
  else {
    dest = lookup_player(player,buf,0);
    if (dest == NOTHING) {
      notify_quiet(player,"MAIL ERROR: That player does not exist.");
      return;
    }
    if (!Controls(player,dest)) {
      notify_quiet(player,"MAIL ERROR: Permission denied.");
      return;
    }
  }
  mail_mark(dest,M_MARK,"all",player,0);
  mail_delete(dest,NOTHING,1,player,0, 0, 0);
  *(int *)sbuf1 = MIND_BSIZE;
  *(int *)(sbuf1+sizeof(int)) = dest;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  dbm_delete(mailfile,keydata);
  *(int *)sbuf1 = MIND_REJM;
  dbm_delete(mailfile,keydata);
  *(int *)sbuf1 = FIND_CURR;
  dbm_delete(foldfile,keydata);
  *(int *)sbuf1 = FIND_CSHR;
  dbm_delete(foldfile,keydata);
  *(int *)sbuf1 = MIND_WRTM;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    memcpy(sbuf7,infodata.dptr,sizeof(short int));
    line = *(short int *)sbuf7;
    write_wipe(dest,line,0);
    atr_clr(dest, A_SAVESENDMAIL);
    atr_clr(player, A_TEMPBUFFER);
    atr_clr(player, A_BCCMAIL);
  }
  *mbuf2 = '#';
  strcpy(mbuf2+1,myitoa(dest));
  mail_access(player,"+remove",mbuf2);
  mpt1 = mapt;
  while (mpt1) {
    *(int *)sbuf1 = MIND_GA;
    strcpy(sbuf1+sizeof(int),mpt1->akey);
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) + strlen(mpt1->akey) + 1;
    infodata = dbm_fetch(mailfile,keydata);
    if (infodata.dptr) {
      memcpy(lbuf1,infodata.dptr,infodata.dsize);
      ipt1 = (int *)lbuf1;
      len = *ipt1++;
      len2 = 0;
      ipt2 = (int *)lbuf2;
      ipt2++;
      for (ct1 = 0; ct1 < len; ct1++, ipt1++) {
	if (dest != *ipt1) {
	  *ipt2++ = *ipt1;
	  len2++;
	}
      }
      if (len2) {
	ipt1 = (int *)lbuf2;
	*ipt1 = len2;
	infodata.dptr = lbuf2;
	infodata.dsize = sizeof(int) * (1 + *ipt1);
	dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	mpt1 = mpt1->next;
      }
      else {
	dbm_delete(mailfile,keydata);
	*(int *)sbuf1 = MIND_GAL;
	dbm_delete(mailfile,keydata);
	if (mpt1 == mapt) {
	  mapt = mpt1->next;
	  if (mapt) mapt->prev = NULL;
	  mpt2 = mapt;
	}
	else if (!mpt1->next) {
	  mpt1->prev->next = NULL;
	  mpt2 = NULL;
	}
	else {
	  mpt2 = mpt1->next;
	  mpt1->prev->next = mpt2;
	  mpt2->prev = mpt1->prev;
	}
	free(mpt1);
	mpt1 = mpt2;
      }
    }
    else
      mpt1 = mpt1->next;
  }
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1+sizeof(int)) = dest;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr) {
    strcpy(lbuf1,"Incoming ");
    strcat(lbuf1,infodata.dptr);
    dbm_delete(foldfile,keydata);
  }
  else
    strcpy(lbuf1,"Incoming");
  pt1 = lbuf1;
  term = 0;
  *(int *)sbuf1 = FIND_BOX;
  *(int *)(sbuf1+sizeof(int)) = dest;
  while (*pt1 && !term) {
    while (isspace((int)*pt1) && *pt1) pt1++;
    if (!*pt1) break;
    pt2 = pt1+1;
    while (!isspace((int)*pt2) && *pt2) pt2++;
    if (!*pt2)
      term = 1;
    else
      *pt2 = '\0';
    strcpy(sbuf1 + (sizeof(int) << 1),pt1);
    keydata.dsize = 1 + (sizeof(int) << 1) + strlen(pt1);
    dbm_delete(foldfile,keydata);
    if (!term)
      pt1 = pt2+1;
  }
  if (get_ind_rec(dest,MIND_ISND,lbuf9,0,player,0)) {
    spt1 = (short int *)lbuf9;
    line = *(spt1+2);
    spt1 += *(spt1+1) + 3;
    term = sizeof(int) << 1;
    t2 = 1;
    for (x = 0; x < line; x++, spt1++) {
      if (get_hd_snd_rec(dest, *spt1, lbuf10, player, 0)) {
	y = *(int *)lbuf10;
	pt1 = lbuf10 + sizeof(int);
	index = get_free_nind(&nindnum);
	chng1 = 0;
	for (ct1 = 0; ct1 < y; ct1++) {
	  if ((len2 = get_hd_rcv_rec(*(int *)pt1,(short int)(*(int *)(pt1 + sizeof(int))),mbuf2,player,0))) {
	    ttest = *(short int *)(mbuf2 + hindoff);
	    if (ttest & tomask) {
	      toall = 1;
	    } else {
	      toall = 0;
            }
	    *(int *)sbuf1 = MIND_HRCV;
	    *(int *)(sbuf1 + sizeof(int)) = *(int *)pt1;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = (short int)(*(int *)(pt1 + sizeof(int)));
	    keydata.dptr = sbuf1;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int); 
	    infodata.dptr = mbuf2;
	    infodata.dsize = len2;
	    if (t2 && index) {
	      *(int *)mbuf2 = nindnum;
	      if (toall)
	        *(short int *)(mbuf2+hindoff) = (index | tomask);
	      else
	        *(short int *)(mbuf2+hindoff) = index;
	      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	      chng1 = 1;
	    }
	    else {
	      t2 = 0;
	      *(mbuf2+hstoff) = 'P';
	      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	      mail_delete(*(int *)pt1,NOTHING,1,player,0,1,0);
	    }
	  }
	  pt1 += term;
	}
	if (chng1) {
	  spt2 = (short int *)lbuf11;
	  *(int *)sbuf1 = MIND_ISND;
	  *(int *)(sbuf1 + sizeof(int)) = nindnum;
	  keydata.dptr = sbuf1;
	  keydata.dsize = sizeof(int) << 1;
	  infodata.dptr = lbuf11;
	  infodata.dsize = sizeof(short int) * (3 + *(spt2+1) + *(spt2+2));
	  dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	  *(int *)(sbuf1 + sizeof(int)) = dest;
	  dbm_delete(mailfile,keydata);
	  if ((len = get_hd_snd_rec(dest, *spt1, lbuf10, player, 0))) {
	    *(int *)sbuf1 = MIND_HSND;
	    *(int *)(sbuf1 + sizeof(int)) = nindnum;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
	    keydata.dptr = sbuf1;
	    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	    infodata.dptr = lbuf10;
	    infodata.dsize = len;
	    dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	    *(int *)(sbuf1 + sizeof(int)) = dest;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
	    dbm_delete(mailfile,keydata);
	    *(int *)sbuf1 = MIND_MSG;
	    infodata = dbm_fetch(mailfile,keydata);
	    if (infodata.dptr) {
	      strcpy(lbuf10,infodata.dptr);
	      *(int *)(sbuf1 + sizeof(int)) = nindnum;
	      *(short int *)(sbuf1 + (sizeof(int) << 1)) = index;
	      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	    }
	    *(int *)(sbuf1 + sizeof(int)) = dest;
	    *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
	    dbm_delete(mailfile,keydata);
	  }
	}
      }
    }
  }
  notify_quiet(player,"Mail: Wipe complete");
}

void 
mail_autofor(dbref player, char *buf1)
{
    dbref dest;

    *(int *)sbuf1 = MIND_AFOR;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    if (!*buf1) {
	infodata = dbm_fetch(mailfile, keydata);
	if (infodata.dptr) {
	    memcpy(sbuf2,infodata.dptr,sizeof(int));
	    dest = *(int *)sbuf2;
	    if (!Good_obj(dest) || !isPlayer(dest) || Recover(dest)) {
		notify_quiet(player, "MAIL ERROR: Invalid autoforward setting...removing.");
		dbm_delete(mailfile, keydata);
	    } else {
		notify_quiet(player, unsafe_tprintf("Mail: Autoforward to -> %s", Name(dest)));
	    }
	} else {
	    notify_quiet(player, "Mail: You have no autoforward set.");
	}
    } else if (!stricmp(buf1, "+off")) {
	dbm_delete(mailfile, keydata);
	notify_quiet(player, "Mail: Autoforwarding removed.");
    } else {
	dest = lookup_player(player, buf1, 0);
	if (dest == NOTHING) {
	    notify_quiet(player, "MAIL ERROR: Bad player name in autoforward.");
	} else if (dest == player) {
	    notify_quiet(player, "MAIL ERROR: You can't autoforward to yourself.");
	} else if (!could_doit(player, dest, A_LMAIL,1,0)) {
	    notify_quiet(player, "MAIL ERROR: Permission denied.");
	} else {
	    *(int *)sbuf2 = dest;
	    infodata.dptr = sbuf2;
	    infodata.dsize = sizeof(int);
	    dbm_store(mailfile, keydata, infodata, DBM_REPLACE);
	    notify_quiet(player, "Mail: Autoforward set.");
	}
    }
}

void mail_recall(dbref player, char *buf1, char *buf2, int key, int later1, int later2)
{
  int *ipt1, all, num, dest, x, y, z, i_chk, do_chk, i_cntr;
  short int *spt1, len, size, *spt2;
  dbref chkplayer, chkplayer2, plrdb;
  struct tm *my_tm;
  char *tpr_buff, *tprp_buff;

  i_cntr = all = num = 0;
  len = size = 0;
  chkplayer = NOTHING;
  chkplayer2 = NOTHING;
  do_chk = 0;
  dest = NOTHING;
  if (!stricmp(buf1,"+all")) {
    if (*buf2) {
      notify_quiet(player,"MAIL ERROR: Improper recall format");
      return;
    }
    all = 1;
  }
  else if (*buf2) {
    if (!*buf2) {
      notify_quiet(player,"MAIL ERROR: Improper recall format");
      return;
    }
    if (!is_number(buf1)) {
      notify_quiet(player,"MAIL ERROR: Bad number in mail recall");
      return;
    }
    num = atoi(buf1);
    if (!stricmp(buf2,"+all")) 
      all = 1;
    else {
      dest = lookup_player(player, buf2, 0);
      if (dest == NOTHING) {
	notify_quiet(player,"MAIL ERROR: Bad name in mail recall");
	return;
      }
    }
  }
  else if (*buf1) {
    if ( !is_number(buf1) && (*buf1 != '*') && 
         ((*buf1 != '@') || ((*buf1 == '@') && !Immortal(player))) ) {
      notify_quiet(player,"MAIL ERROR: Bad number in mail recall");
      return;
    }
    if ( (*buf1 == '@') && Immortal(player) )
       chkplayer2 = lookup_player(player, buf1+1, 0);
    if ( is_number(buf1) )
       num = atoi(buf1);
  } 
  if ( chkplayer2 == NOTHING )
     chkplayer2 = player;
  if (get_ind_rec(chkplayer2,MIND_ISND,lbuf8,0,later1,later2)) {
    spt1 = (short int *)lbuf8;
    if (!all && !num && (dest == NOTHING)) {
      num = (int)(*(spt1+2));
      if ( *buf1 == '*' ) {
         chkplayer = lookup_player(player, buf1+1, 0);
         do_chk = 1;
      }
      spt1 += *(spt1+1) + 3;
      tprp_buff = tpr_buff = alloc_lbuf("mail_recall");
      for (x = 0; x < num; x++, spt1++) {
	if (key & M_ALL)
	  unparse_to(chkplayer2, *spt1, lbuf9, &plrdb, 0);
	else
	  unparse_to_2(chkplayer2,*spt1,lbuf9, &plrdb);
        if ( do_chk ) {
           if ( chkplayer == NOTHING || !Good_obj(chkplayer) ||
                (strstr(lbuf9, Name(chkplayer)) == NULL) )
              continue;
        }
	if (*lbuf9) {
           if ( !i_cntr ) {
              tprp_buff = tpr_buff;
              notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%-8s %-8s %-26s %-s",
                           (char *)"Msg #", (char *)"Date", 
                           (char *)"Subject", (char *)"Sent to"));
              notify_quiet(player, "-------- " \
                                   "-------- " \
                                   "-------------------------- " \
                                   "----------> ...");
           }
           i_cntr++;
           spt2 = (short int *)lbuf8;
           spt2 += *(spt2+1) + 2 + x + 1;
           *(int *)sbuf1 = MIND_MSG;
           *(int *)(sbuf1 + sizeof(int)) = chkplayer2;
           *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt2;
           keydata.dptr = sbuf1;
           keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
           infodata = dbm_fetch(mailfile,keydata);
           if (get_hd_snd_rec(chkplayer2,*spt2,lbuf7,later1,later2)) {
	     ipt1 = (int *)lbuf7;
	     ipt1++; //Lensy: It was *ipt1++
	     if ((len = get_hd_rcv_rec(*ipt1,*(ipt1+1),mbuf1,later1,later2))) {
                my_tm = gmtime((time_t *)(mbuf1+htimeoff));
                sprintf(sbuf3, "%02d/%02d/%02d", (my_tm->tm_mon+1), my_tm->tm_mday, (my_tm->tm_year % 100));
             } else {
                strcpy(sbuf3, "N/A");
             }
           }
           tprp_buff = tpr_buff;
           if ( len > hsuboff )
	       notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "%-8d %-8s %-26.26s %-s", x+1, sbuf3, mbuf1+hsuboff, lbuf9));
           else
	       notify_quiet(player,safe_tprintf(tpr_buff, &tprp_buff, "%-8d %-8s %-26.26s %-s", x+1, sbuf3, (char *)"(NONE)", lbuf9));
        }
      }
      free_lbuf(tpr_buff);
    }
    else if (num && !all && (dest == NOTHING)) {
      i_chk = (int)(*(spt1+2));
      if ( num > i_chk )
	 num = i_chk;
      if ( num < 0 )
	 num = 1;
      spt1 += *(spt1+1) + 2 + num;
      *(int *)sbuf1 = MIND_MSG;
      *(int *)(sbuf1 + sizeof(int)) = player;
      *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
      keydata.dptr = sbuf1;
      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr)
	strcpy(lbuf9,infodata.dptr);
      else
	*lbuf9 = '\0';
      if (get_hd_snd_rec(player,*spt1,lbuf7,later1,later2)) {
	ipt1 = (int *)lbuf7;
	ipt1++; //Lensy: It was *ipt1++
	if ((len = get_hd_rcv_rec(*ipt1,*(ipt1+1),mbuf1,later1,later2))) {
           strcpy(sbuf3, ctime((time_t *)(mbuf1+htimeoff)));
	   *(sbuf3 + strlen(sbuf3) - 1) = '\0';
           size = *(short int *)(mbuf1+hsizeoff);
           if ( size > 1 )
              strcpy(sbuf4, "Bytes");
           else
              strcpy(sbuf4, "Byte");
         } else {
            strcpy(sbuf3, "UNKNOWN");
            strcpy(sbuf4, "N/A");
         }
      } else {
        size = -1;
        strcpy(sbuf3, "UNKNOWN");
        strcpy(sbuf4, "N/A");
      }
      if (key & M_ALL)
	unparse_to(player, *spt1, lbuf10, &plrdb, 0);
      else
        unparse_to_2(player,*spt1,lbuf10, &plrdb);
      /* Enforce a re-read of the player if doesn't exist */
      if ( !*lbuf10 )
	unparse_to(player, *spt1, lbuf10, &plrdb, 0);
      notify_quiet(player, "---------------------------------------"
                           "---------------------------------------");
      notify_quiet(player,
                   unsafe_tprintf("Recall #: %d\nTo: %s\nDate/Time: %-31s Size: %d %s",
                      num, lbuf10, sbuf3, size, sbuf4));
      if ( len > hsuboff )
         notify_quiet(player, unsafe_tprintf("Subject: %s", mbuf1+hsuboff));
      notify_quiet(player, "---------------------------------------"
                           "---------------------------------------");
      notify_quiet(player, unsafe_tprintf("%s", lbuf9));
      notify_quiet(player, "---------------------------------------"
                           "---------------------------------------");
/************************************************************************
 * This is the old method of displaying.  Rather boring.
 *
 *    notify_quiet(player,unsafe_tprintf("Msg #: %d",num));
 *    notify_quiet(player,unsafe_tprintf("To: %s",lbuf10));
 *    notify_quiet(player,unsafe_tprintf("Msg: %s",lbuf9));
 ***********************************************************************/
    }
    else if (dest != NOTHING) {
      spt1 += *(spt1+1) + 2 + num;
      if (get_hd_snd_rec(player,*spt1,lbuf9,later1,later2)) {
	ipt1 = (int *)lbuf9;
	num = *ipt1++;
	y = 0;
	for (x = 0; x < num; x++, ipt1 += 2) {
	  if (*ipt1 != dest) continue;
	  if ((len = get_hd_rcv_rec(*ipt1,*(ipt1+1),mbuf1,later1,later2))) {
	    if ((*(mbuf1+hstoff) == 'N') || (*(mbuf1+hstoff) == 'U')) {
	      *(mbuf1+hstoff) = 'P';
	      *(int *)sbuf1 = MIND_HRCV;
	      *(int *)(sbuf1 + sizeof(int)) = *ipt1;
	      *(short int *)(sbuf1 + (sizeof(int) << 1)) = (short int)(*(ipt1+1));
	      keydata.dptr = sbuf1;
	      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
	      infodata.dptr = mbuf1;
	      infodata.dsize = len;
	      dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
	      y++;
	    }
	  }
	}
	if (y) 
	  mail_delete(dest,NOTHING,1,player,0,1,0);
	notify_quiet(player,unsafe_tprintf("Mail: %d message(s) were able to be recalled",y));
      }
    }
    else {
      y = (int)(*(spt1+2));
      spt1 += *(spt1+1) + 2 + num;
      if (!num)
	spt1++;
      z = 0;
      for (x = 0; x < y; x++, spt1++) {
	if (get_hd_snd_rec(player,*spt1,lbuf9,later1,later2)) {
	  ipt1 = (int *)lbuf9;
	  dest = *ipt1++;
	  for (all = 0; all < dest; all++, ipt1 += 2) {
	    if ((len = get_hd_rcv_rec(*ipt1,*(ipt1+1),mbuf1,later1,later2))) {
	      if ((*(mbuf1+hstoff) == 'N') || (*(mbuf1+hstoff) == 'U')) {
		*(mbuf1+hstoff) = 'P';
		*(int *)sbuf1 = MIND_HRCV;
		*(int *)(sbuf1 + sizeof(int)) = *ipt1;
		*(short int *)(sbuf1 + (sizeof(int) << 1)) = (short int)(*(ipt1+1));
		keydata.dptr = sbuf1;
		keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
		infodata.dptr = mbuf1;
		infodata.dsize = len;
		dbm_store(mailfile,keydata,infodata,DBM_REPLACE);
		mail_delete(*ipt1,NOTHING,1,player,0,1,0);
		z++;
	      }
	    }
	  }
	}
	if (num)
	  break;
      }
      notify_quiet(player,unsafe_tprintf("Mail: %d message(s) were able to be recalled",z));
    }
  }
  if ( i_cntr ) {
     notify_quiet(player, "-------- " \
                          "-------- " \
                          "-------------------------- " \
                          "----------> ...");
     notify_quiet(player, unsafe_tprintf("Mail: %d total - Done", i_cntr));
  } else
     notify_quiet(player,"Mail: Done");
}

long mcount_s1(dbref target, int domsg)
{
  long count;
  int x, y;
  short int *spt1;
  short int i, j;
  char *pt1, *pt2;

  count = 0;
  *(int *)sbuf1 = MIND_IRCV;
  *(int *)(sbuf1 + sizeof(int)) = target;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    count = keydata.dsize + infodata.dsize;
    memcpy(lbuf1,infodata.dptr,infodata.dsize);
    spt1 = (short int *)lbuf1;
    x = *(spt1+2);
    spt1 += 3 + *(spt1+1);
    for (y = 0; y < x; y++, spt1++) {
      *(int *)sbuf1 = MIND_HRCV;
      *(int *)(sbuf1 + sizeof(int)) = target;
      *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
      keydata.dptr = sbuf1;
      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr) {
	count += keydata.dsize + infodata.dsize;
	if (domsg) {
	  memcpy(lbuf2,infodata.dptr,infodata.dsize);
	  count += *(short int *)(lbuf2 + hsizeoff) + (sizeof(int) << 1) + sizeof(short int) + 1;
	}
      }
    }
  }
  *(int *)sbuf1 = MIND_BSIZE;
  *(int *)(sbuf1 + sizeof(int)) = target;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = MIND_REJM;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = MIND_PAGE;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = MIND_AFOR;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = MIND_WRTS;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = FIND_CURR;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = FIND_CSHR;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr)
    count += keydata.dsize + infodata.dsize;
  *(int *)sbuf1 = MIND_WRTM;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    count += keydata.dsize + infodata.dsize;
    memcpy(lbuf1,infodata.dptr,infodata.dsize);
    i = *(short int *)lbuf1;
    *(int *)sbuf1 = MIND_WRTL;
    *(int *)(sbuf1 + sizeof(int)) = target;
    keydata.dptr = sbuf1;
    keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
    for (j = 1; j <= i; j++) {
      *(short int *)(sbuf1 + (sizeof(int) << 1)) = j;
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr)
	count += keydata.dsize + infodata.dsize;
    }
  }
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = target;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr) {
    count += keydata.dsize + infodata.dsize;
    strcpy(lbuf1,"Incoming ");
    strcat(lbuf1,infodata.dptr);
  }
  else
    strcpy(lbuf1,"Incoming");
  pt1 = lbuf1;
  x = 1;
  *(int *)sbuf1 = FIND_BOX;
  while (x) {
    pt2 = pt1+1;
    while (*pt2 && (*pt2 != ' ')) pt2++;
    if (!*pt2)
      x = 0;
    else
      *pt2 = '\0';
    strcpy(sbuf1+(sizeof(int) << 1),pt1);
    keydata.dsize = (sizeof(int) << 1) + 1 + strlen(pt1);
    infodata = dbm_fetch(foldfile,keydata);
    if (infodata.dptr)
      count += keydata.dsize + infodata.dsize;
    pt1 = pt2+1;
  }
  return count;
}

long mcount_s2(dbref target)
{
  short int *spt1;
  short int x, y;
  long count;

  count = 0;
  *(int *)sbuf1 = MIND_ISND;
  *(int *)(sbuf1 + sizeof(int)) = target;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    count = keydata.dsize + infodata.dsize;
    memcpy(lbuf1,infodata.dptr,infodata.dsize);
    spt1 = (short int *)lbuf1;
    x = *(spt1+2);
    spt1 += 3 + *(spt1+1);
    for (y = 0; y < x; y++, spt1++) {
      *(int *)sbuf1 = MIND_HSND;
      *(int *)(sbuf1 + sizeof(int)) = target;
      *(short int *)(sbuf1 + (sizeof(int) << 1)) = *spt1;
      keydata.dptr = sbuf1;
      keydata.dsize = (sizeof(int) << 1) + sizeof(short int);
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr) 
	count += keydata.dsize + infodata.dsize;
      *(int *)sbuf1 = MIND_MSG;
      infodata = dbm_fetch(mailfile,keydata);
      if (infodata.dptr) 
	count += keydata.dsize + infodata.dsize;
    }
  }
  return count;
}

long mcount_size(dbref target, char *buf1)
{
/* type 1: Rcv indx, Rcv hdr, Bsize, Rejm, Write stuff, Page, Afor, Folder stuff
   type 2: Add rcv msg len (stored under sender) to type 1
   type 3: Snd indx, Snd hdr, Snd msg
   type 4: type 1 + type 3
   type 5: type 2 + type 3 */

  long count;
  int typec;

  if (!is_number(buf1)) return -1;
  typec = atoi(buf1);
  if ((typec < 1) || (typec > 5)) return -1;
  if ((typec == 1) || (typec == 4))
    count = mcount_s1(target,0);
  else if ((typec == 2) || (typec == 5))
    count = mcount_s1(target,1);
  else
    count = 0;
  if (typec >= 3)
    count += mcount_s2(target);
  return count;
}

long mcount_size_tot()
{
  long count;

  count = 0;
  keydata = dbm_firstkey(mailfile);
  while (keydata.dptr) {
    count += keydata.dsize;
    infodata = dbm_fetch(mailfile,keydata);
    if (infodata.dptr)
      count += infodata.dsize;
    keydata = dbm_nextkey(mailfile);
  }
  keydata = dbm_firstkey(foldfile);
  while (keydata.dptr) {
    count += keydata.dsize;
    infodata = dbm_fetch(foldfile,keydata);
    if (infodata.dptr)
      count += infodata.dsize;
    keydata = dbm_nextkey(foldfile);
  }
  return count;
}

void 
mail_lfix(dbref player)
{
  memset(dumpname, 0, sizeof(dumpname));
  memset(fdumpname, 0, sizeof(fdumpname));
  sprintf(dumpname,  "%s/mail.fix", mudconf.data_dir);
  sprintf(fdumpname, "%s/folder.fix", mudconf.data_dir);

  mail_load(player);

  memset(dumpname, 0, sizeof(dumpname));
  memset(fdumpname, 0, sizeof(fdumpname));
  sprintf(dumpname,  "%s/%s.dump.mail", mudconf.data_dir, mudconf.muddb_name);
  sprintf(fdumpname, "%s/%s.dump.folder", mudconf.data_dir, mudconf.muddb_name);
}

void mail_gblout(dbref player)
{
  short int smax;

  *(int *)sbuf1 = MIND_SMAX;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int);
  infodata = dbm_fetch(mailfile,keydata);
  if (infodata.dptr) {
    bcopy(infodata.dptr,sbuf1,sizeof(short int));
    smax = *(short int *)sbuf1;
  }
  else
    smax = STARTSMAX;
  if (mudconf.maildelete)
    strcpy(sbuf1,"enabled");
  else
    strcpy(sbuf1,"disabled");
  sprintf(lbuf1,"Mail globals/defaults: autodeletion...%s; boxsize...%d; max save...%d; max folders...%d; max send...%d",
		sbuf1,mudconf.mailbox_size,smax,mudconf.mailmax_fold,mudconf.mailmax_send);
  if (Wizard(player)) {
    sprintf(lbuf2,"; abs max rcv/snd index...%d; abs max rcp...%d; abs max access...%d",
		absmaxinx,absmaxrcp,absmaxacs);
    strcat(lbuf1,lbuf2);
  }
  notify(player,lbuf1);
}

int 
fname_check(dbref player, char *buf1, int itest)
{
    char *p1;
    int rvalue;

    rvalue = 1;
    p1 = buf1;
    while (*p1 && isalnum((int)*p1)) p1++;
    if (*p1) {
	notify_quiet(player, "MAIL ERROR: Bad character(s) in folder name.");
	rvalue = 0;
    } else if (strlen(buf1) > FLENGTH) {
	notify_quiet(player, "MAIL ERROR: Folder name too long.");
	rvalue = 0;
    } else if (itest == 1) {
	if (stricmp(buf1, "Incoming") == 0) {
	    notify_quiet(player, "MAIL ERROR: Incoming is a reserved folder name.");
	    rvalue = 0;
	}
    }
    return rvalue;
}

void 
fname_conv(char *buf1, char *buf2)
{
    int inc;

    inc = 1;
    *buf2 = toupper(*buf1);
    while (*(buf1 + inc) != '\0') {
	*(buf2 + inc) = tolower(*(buf1 + inc));
	inc++;
    }
    *(buf2 + inc) = '\0';
}

void folder_create(dbref player, char *buf)
{
  char *p1;
  int inc;

  if (!fname_check(player,buf,1))
    return;
  fname_conv(buf,lbuf1);
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (!infodata.dptr) {
    infodata.dptr = lbuf1;
    infodata.dsize = strlen(lbuf1) + 1;
    dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
    notify_quiet(player,"Mail: Folder created");
  } else {
    inc = 0;
    p1 = mystrchr(infodata.dptr,' ');
    while (p1) {
      inc++;
      p1 = mystrchr(p1 + 1,' ');
    }
    if (inc > mudconf.mailmax_fold - 1) {
      notify_quiet(player,"MAIL ERROR: Maximum number of folders reached");
    }
    else if (strstr2(infodata.dptr,lbuf1)) {
      notify_quiet(player,"MAIL ERROR: That folder already exists");
    }
    else {
      strcat(lbuf1," ");
      strcat(lbuf1, infodata.dptr);
      infodata.dptr = lbuf1;
      infodata.dsize = strlen(lbuf1) + 1;
      dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
      notify_quiet(player,"Mail: Folder created");
    }
  }
}

void 
folder_current_function(dbref player, int key, char *buf1, char *buff, char *bufcx)
{
  dbref dest;

  dest = lookup_player(player, buf1, 0);
  if (dest == NOTHING) {
     safe_str("#-1 NOT FOUND", buff, &bufcx);
  } else if ((!Wizard(player) || (!Immortal(player) && !Controls(player, dest)))) {
     safe_str("#-1 PERMISSION DENIED", buff, &bufcx);
  } else {
     *(int *)(sbuf1 + sizeof(int)) = dest;
     keydata.dptr = sbuf1;
     keydata.dsize = sizeof(int) << 1;
     if (key)
        *(int *)sbuf1 = FIND_CSHR;
     else
        *(int *)sbuf1 = FIND_CURR;
     if (*quickfolder)
        infodata.dptr = quickfolder;
     else
        infodata = dbm_fetch(foldfile,keydata);
     if (infodata.dptr) {
        safe_str(infodata.dptr, buff, &bufcx);
     }
  }
}

void
folder_list_function(dbref player, char *buff, char *bufcx)
{
  safe_str("Incoming ", buff, &bufcx);
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr) {
    safe_str(infodata.dptr, buff, &bufcx);
    safe_chr(' ', buff, &bufcx);
  }
}

void
folder_plist_function(dbref player, char *buf1, char *buff, char *bufcx)
{
    dbref dest, owner;
    int flags;
    char *p1;

    dest = lookup_player(player, buf1, 0);
    if (dest == NOTHING) {
      safe_str("#-1 NOT FOUND", buff, &bufcx);
    } else {
      p1 = atr_get(dest, A_LSHARE, &owner, &flags);
      if ((*p1 == '\0') && (!Wizard(player) || (!Immortal(player) && !Controls(player, dest)))) {
          safe_str("#-1 PERMISSION DENIED", buff, &bufcx);
      } else if (!could_doit(player, dest, A_LSHARE,1,0) &&
                   (!Wizard(player) || (!Immortal(player) && !Controls(player, dest)))) {
          safe_str("#-1 PERMISSION DENIED", buff, &bufcx);
      } else {
          folder_list_function(dest, buff, bufcx);
      }
      free_lbuf(p1);
    }
}

void 
folder_list(dbref player, dbref wiz, int flag)
{
  char *s_tmp1, *s_buff, *s_strtok, *s_strtokr, *retval;

  strcpy(lbuf1,"Incoming ");
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (infodata.dptr)
    strcat(lbuf1,infodata.dptr);
  s_tmp1 = alloc_lbuf("folder_list_tmp");
  s_buff = alloc_lbuf("folder_list_buff");
  strcpy(s_tmp1, lbuf1);
  if (flag) {
    notify_quiet(player,"Mail: You have the following folders ->");
    s_strtok = strtok_r(s_tmp1, " ", &s_strtokr);
    while ( s_strtok ) {
       retval = mail_quick_function(player, s_strtok, 4);
       sprintf(s_buff, "  %-24s %s", s_strtok, retval);
       free_lbuf(retval);
       notify_quiet(player, s_buff);
       s_strtok = strtok_r(NULL, " ", &s_strtokr);
    }
    folder_current(player, 0, NOTHING, 1);
  } else {
    notify_quiet(wiz,"Mail: That player has the following folders ->");
    s_strtok = strtok_r(s_tmp1, " ", &s_strtokr);
    while ( s_strtok ) {
       retval = mail_quick_function(player, s_strtok, 4);
       sprintf(s_buff, "  %-24s %s", s_strtok, retval);
       free_lbuf(retval);
       notify_quiet(wiz, s_buff);
       s_strtok = strtok_r(NULL, " ", &s_strtokr);
    }	    
    *s_buff = '\0';
    s_strtok = s_buff;
    sprintf(s_tmp1, "#%d", player);
    folder_current_function(GOD, 0, s_tmp1, s_buff, s_strtok);
    if ( *s_buff ) {
       sprintf(s_tmp1, "Mail: That player's current folder is -> %s", s_buff);
    } else {
       sprintf(s_tmp1, "Mail: That player has no current folder");
    }
    notify_quiet(wiz, s_tmp1);
  }
  free_lbuf(s_tmp1);
  free_lbuf(s_buff);
}

void folder_delete(dbref player, char *buf)
{
  datum keydata2, infodata2;
  char *p1, *p2;

  if (!fname_check(player,buf,1))
    return;
  *(int *)sbuf1 = FIND_LST;
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  infodata = dbm_fetch(foldfile,keydata);
  if (!infodata.dptr) {
    notify_quiet(player,"MAIL ERROR: Folder not found");
  }
  else {
    strcpy(lbuf2,infodata.dptr);
    fname_conv(buf,lbuf1);
    p1 = strstr2(lbuf2, lbuf1);
    if (!p1)
      notify_quiet(player,"MAIL ERROR: Folder not found");
    else {
      *(int *)sbuf2 = FIND_BOX;
      *(int *)(sbuf2 + sizeof(int)) = player;
      strcpy(sbuf2 + (sizeof(int) << 1),lbuf1);
      keydata2.dptr = sbuf2;
      keydata2.dsize = 1 + (sizeof(int) << 1) + strlen(lbuf1);
      infodata2 = dbm_fetch(foldfile,keydata2);
      if (infodata2.dptr) 
	notify_quiet(player,"MAIL ERROR: Folder not empty");
      else {
	dbm_delete(foldfile,keydata2);
	*(int *)sbuf2 = FIND_CURR;
	*(int *)(sbuf2 + sizeof(int)) = player;
	keydata2.dsize = sizeof(int) << 1;
	infodata2 = dbm_fetch(foldfile,keydata2);
	if (infodata2.dptr) {
	  if (!strcmp(infodata2.dptr,lbuf1)) {
	    dbm_delete(foldfile,keydata2);
	  }
	}
	*lbuf3 = '\0';
	strncat(lbuf3,lbuf2,p1-lbuf2);
	p2 = p1 + strlen(lbuf1);
	if (!*p2 && !*lbuf3) {
	  dbm_delete(foldfile,keydata);
	}
	else if (!*p2 && *lbuf3) {
	  *(lbuf3 + strlen(lbuf3) - 1) = '\0';
	}
	if (*p2) {
	  strcat(lbuf3,p2+1);
	}
	if (*lbuf3) {
	  infodata.dptr = lbuf3;
	  infodata.dsize = strlen(lbuf3) + 1;
	  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
	}
	notify_quiet(player,"Mail: Folder deleted");
      }
    }
  }
}

void 
folder_rename(dbref player, char *buf1, char *buf2)
{
    datum keydata2, infodata2;
    char *p1;
    int size;

    if (!fname_check(player, buf1, 1))
	return;
    if (!fname_check(player, buf2, 1))
	return;
    *(int *)sbuf1 = FIND_LST;
    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    infodata = dbm_fetch(foldfile, keydata);
    if (!infodata.dptr) {
	notify_quiet(player, "MAIL ERROR: Folder not found.");
    } else {
	strcpy(lbuf3, infodata.dptr);
	fname_conv(buf1, lbuf1);
	fname_conv(buf2, lbuf2);
	if (!strcmp(lbuf1,lbuf2)) {
	  notify_quiet(player,"MAIL ERROR: Folder names are the same");
	  return;
	}
	if (strstr2(lbuf3,lbuf2)) {
	  notify_quiet(player,"MAIL ERROR: New folder name already exists");
	  return;
	}
	p1 = strstr2(lbuf3, lbuf1);
	if (p1 == NULL) {
	    notify_quiet(player, "MAIL ERROR: Folder not found.");
	} else {
	    *(int *)sbuf2 = FIND_CURR;
	    *(int *)(sbuf2 + sizeof(int)) = player;
	    keydata2.dptr = sbuf2;
	    keydata2.dsize = sizeof(int) << 1;
	    infodata2 = dbm_fetch(foldfile, keydata2);
	    if (infodata2.dptr) {
		if (!strcmp(infodata2.dptr, lbuf1)) {
		    infodata2.dptr = lbuf2;
		    infodata2.dsize = strlen(lbuf2) + 1;
		    dbm_store(foldfile, keydata2, infodata2, DBM_REPLACE);
		}
	    }
	    *(int *)sbuf2 = FIND_BOX;
	    *(int *)(sbuf2 + sizeof(int)) = player;
	    strcpy(sbuf2 + (sizeof(int) << 1),lbuf1);
	    keydata2.dptr = sbuf2;
	    keydata2.dsize = 1 + (sizeof(int) << 1) + strlen(lbuf1);
	    infodata2 = dbm_fetch(foldfile, keydata2);
	    if (infodata2.dptr) {
		size = infodata2.dsize;
		memcpy(lbuf5,infodata2.dptr,size);
		dbm_delete(foldfile, keydata2);
		*(int *)sbuf2 = FIND_BOX;
		*(int *)(sbuf2 + sizeof(int)) = player;
		strcpy(sbuf2 + (sizeof(int) << 1),lbuf2);
		keydata2.dptr = sbuf2;
		keydata2.dsize = 1 + (sizeof(int) << 1) + strlen(lbuf2);
		infodata2.dptr = lbuf5;
		infodata2.dsize = size;
		dbm_store(foldfile, keydata2, infodata2, DBM_REPLACE);
	    }
	    *lbuf4 = '\0';
	    strncat(lbuf4, lbuf3, (p1 - lbuf3));
	    strcat(lbuf4, lbuf2);
	    strcat(lbuf4, p1 + strlen(lbuf1));
	    infodata.dptr = lbuf4;
	    infodata.dsize = strlen(lbuf4) + 1;
	    dbm_store(foldfile, keydata, infodata, DBM_REPLACE);
	    notify_quiet(player, "Mail: Folder renamed.");
	}
    }
}

void folder_current(dbref player, int flag, dbref wiz, dbref key)
{
  int silent;

  if (key & 32768) {
     silent = 1;
     key &= ~32767;
  } else
     silent = 0;

  if (!key && !Wizard(wiz))
    flag = 1;
  else if (!key) {
    notify_quiet(wiz,"Mail: Checking all folders");
    return;
  }
  *(int *)(sbuf1 + sizeof(int)) = player;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int) << 1;
  if (flag)
    *(int *)sbuf1 = FIND_CSHR;
  else
    *(int *)sbuf1 = FIND_CURR;
  if (*quickfolder)
    infodata.dptr = quickfolder;
  else
    infodata = dbm_fetch(foldfile,keydata);
  if (!infodata.dptr) {
    if (flag) {
      if (key) {
        if ( !silent )
           notify_quiet(player,"Mail: You have no current share folder");
      } else {
        if ( !silent )
	   notify_quiet(wiz,"Mail: That person has no shared folder");
      }
    }
    else
      if ( !silent )
         notify_quiet(player,"Mail: You have no current folder");
  }
  else {
    if (flag) {
      if (key) {
        if ( !silent )
           notify_quiet(player,unsafe_tprintf("Mail: Your current share folder is -> %s",infodata.dptr));
      } else {
        if ( !silent )
	   notify_quiet(wiz,unsafe_tprintf("Mail: Shared folder is -> %s",infodata.dptr));
      }
    }
    else
      if ( !silent )
         notify_quiet(player,unsafe_tprintf("Mail: Your current folder is -> %s",infodata.dptr));
  }
}

void 
folder_change(dbref player, char *buf1, int flag)
{
    datum keydata2;

    *(int *)(sbuf1 + sizeof(int)) = player;
    keydata.dptr = sbuf1;
    keydata.dsize = sizeof(int) << 1;
    if (!flag) {
	*(int *)sbuf1 = FIND_CURR;
    } else {
	*(int *)sbuf1 = FIND_CSHR;
    }
    if (!*buf1) {
	dbm_delete(foldfile, keydata);
	if (!flag) {
	    notify_quiet(player, "Mail: Folder reset.");
	} else {
	    notify_quiet(player, "Mail: Share folder reset.");
	}
	return;
    }
    if (!fname_check(player, buf1, 0))
	return;
    if (!stricmp(buf1, "Incoming")) {
	infodata.dptr = "Incoming";
	infodata.dsize = strlen(infodata.dptr) + 1;
	dbm_store(foldfile, keydata, infodata, DBM_REPLACE);
	if (!flag) {
	    notify_quiet(player, "Mail: Folder changed.");
	} else {
	    notify_quiet(player, "Mail: Share folder changed.");
	}
    } else {
	*(int *)sbuf2 = FIND_LST;
	*(int *)(sbuf2 + sizeof(int)) = player;
	keydata2.dptr = sbuf2;
	keydata2.dsize = sizeof(int) << 1;
	infodata = dbm_fetch(foldfile, keydata2);
	if (!infodata.dptr) {
	    notify_quiet(player, "MAIL ERROR: That folder does not exist.");
	} else {
	    fname_conv(buf1, lbuf1);
	    if (!strstr2(infodata.dptr, lbuf1)) {
		notify_quiet(player, "MAIL ERROR: That folder does not exist.");
	    } else {
		infodata.dptr = lbuf1;
		infodata.dsize = strlen(lbuf1) + 1;
		dbm_store(foldfile, keydata, infodata, DBM_REPLACE);
		if (!flag) {
		    notify_quiet(player, "Mail: Folder changed.");
		} else {
		    notify_quiet(player, "Mail: Share folder changed.");
		}
	    }
	}
    }
}

void 
folder_plist(dbref player, char *buf1)
{
    dbref dest, owner;
    int flags;
    char *p1;

    dest = lookup_player(player, buf1, 0);
    if (dest == NOTHING) {
	notify_quiet(player, "MAIL ERROR: Player not found.");
    } else {
	p1 = atr_get(dest, A_LSHARE, &owner, &flags);
	if ((*p1 == '\0') && (!Wizard(player) || !Controls(player, dest))) {
	    notify_quiet(player, "MAIL ERROR: Permission denied.");
	} else if (!could_doit(player, dest, A_LSHARE,1,0) && (!Wizard(player) || !Controls(player, dest))) {
	    notify_quiet(player, "MAIL ERROR: Permission denied.");
	} else {
	    folder_list(dest, player, 0);
	}
	free_lbuf(p1);
    }
}

void 
folder_move(dbref player, char *buf1, char *buf2)
{
    short int ct2, num, count, msize, *spt1, *spt2, *spt3, *spt4;
    char *p1, *tstrtokr;
    char all;
    dbref dest;

    count = 0;
    all = 0;
    dest = NOTHING;
    msize = get_box_size(player);
    spt1 = (short int *)lbuf1;
    if (isdigit((int)*buf1)) {
	p1 = strtok_r(buf1, " ", &tstrtokr);
	while (p1 != NULL) {
	    *(spt1+count) = (short int)atoi(p1);
	    count++;
	    if (count == msize)
		break;
	    p1 = strtok_r(NULL, " ", &tstrtokr);
	}
    } else if (stricmp(buf1, "all") == 0) {
	all = 1;
	dest = 0;
    } else {
	dest = lookup_player(player, buf1, 0);
    }
	p1 = mystrchr(buf2, ',');
	if (!p1 || !*(p1 + 1)) {
	    notify_quiet(player, "MAIL ERROR: Bad format in folder move command.");
	} else {
	    *p1 = '\0';
	    if ((fname_check(player, buf2, 0) != 0) && (fname_check(player, p1 + 1, 0) != 0)) {
		fname_conv(buf2, sbuf4);
		fname_conv(p1 + 1, sbuf5);
		if (!strcmp(sbuf4, sbuf5)) {
		    notify_quiet(player, "MAIL ERROR: Folder names the same in move command.");
		} else {
		    *(int *)sbuf3 = FIND_LST;
		    *(int *)(sbuf3 + sizeof(int)) = player;
		    keydata.dptr = sbuf3;
		    keydata.dsize = sizeof(int) << 1;
		    infodata = dbm_fetch(foldfile, keydata);
		    if (!infodata.dptr) {
			notify_quiet(player, "MAIL ERROR: Folder does not exist.");
		    } else {
			if (((strcmp(sbuf4, "Incoming") != 0) && (strstr2(infodata.dptr, sbuf4) == NULL)) ||
			    ((strcmp(sbuf5, "Incoming") != 0) && (strstr2(infodata.dptr, sbuf5) == NULL))) {
			    notify_quiet(player, "MAIL ERROR: Folder does not exist.");
			} else {
			    *(int *)sbuf3 = FIND_BOX;
			    *(int *)(sbuf3+sizeof(int)) = player;
			    strcpy(sbuf3+(sizeof(int) << 1),sbuf4);
			    keydata.dptr = sbuf3;
			    keydata.dsize = 1 + (sizeof(int) << 1) + strlen(sbuf4);
			    infodata = dbm_fetch(foldfile, keydata);
			    if (!infodata.dptr) {
				notify_quiet(player, "MAIL ERROR: Bad message number.");
			    } else {
				memcpy(lbuf2, infodata.dptr,infodata.dsize);
				spt2 = (short int *)lbuf2;
				if (dest != NOTHING) {
				    num = 0;
				    while (num < *spt2) {
					num++;
					if (get_hd_rcv_rec(player,*(spt2+num),lbuf3,NOTHING,1)) {
					    if ((dest == *(int *)lbuf3) || all) {
						*(spt1+count) = num;
						count++;
					    }
					}
					if (count == msize)
					    break;
				    }
				}
				spt3 = (short int *)(lbuf3+sizeof(short int));
				spt4 = (short int *)lbuf4;
				for (ct2 = 1; ct2 <= *spt2; ct2++) {
				  for (num = 0; num < count; num++) {
				    if (ct2 == *(spt1+num)) {
				      *spt4++ = *(spt2+ct2);
				      break;
				    }
				  }
				  if (num == count) {
				    *spt3++ = *(spt2+ct2);
				  }
				}
				num = ((char *)spt4-lbuf4)/sizeof(short int);
				*(int *)sbuf3 = FIND_BOX;
				*(int *)(sbuf3 + sizeof(int)) = player;
				strcpy(sbuf3+(sizeof(int) << 1),sbuf4);
				keydata.dptr = sbuf3;
				keydata.dsize = 1 + (sizeof(int) << 1) + strlen(sbuf4);
				if (num == *spt2) {
				  dbm_delete(foldfile,keydata);
				}
				else {
				  *(short int *)lbuf3 = *spt2-num;
				  infodata.dptr = lbuf3;
				  infodata.dsize = sizeof(short int) * (*spt2-num+1);
				  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
				}
				*(int *)sbuf3 = FIND_BOX;
				*(int *)(sbuf3 + sizeof(int)) = player;
				strcpy(sbuf3+(sizeof(int) << 1),sbuf5);
				keydata.dptr = sbuf3;
				keydata.dsize = 1 + (sizeof(int) << 1) + strlen(sbuf5);
				infodata = dbm_fetch(foldfile,keydata);
				spt3 = (short int *)lbuf3;
				if (infodata.dptr) {
				  memcpy(lbuf3,infodata.dptr,infodata.dsize);
				  memcpy(spt3+*spt3+1,lbuf4,num*sizeof(short int));
				  *spt3 += num;
				  infodata.dptr = lbuf3;
			 	  infodata.dsize = sizeof(short int) * (*spt3 + 1);
				  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
				}
				else {
				  *spt3 = num;
				  memcpy(spt3+1,lbuf4,num*sizeof(short int));
				  infodata.dptr = lbuf3;
				  infodata.dsize = sizeof(short int) * (num+1);
				  dbm_store(foldfile,keydata,infodata,DBM_REPLACE);
				}
				if (num < count)
				  notify_quiet(player,"MAIL ERROR: Bad message number");
				notify_quiet(player,unsafe_tprintf("Mail: %d message(s) moved",num));
			    }
			}
		    }
		}
	    }
	}
}

int acs_check(dbref player)
{
  int x, y, *pt1;

  *(int *)sbuf1 = MIND_ACS;
  keydata.dptr = sbuf1;
  keydata.dsize = sizeof(int);
  infodata = dbm_fetch(mailfile,keydata);
  if (!infodata.dptr) return 1;
  memcpy(lbuf12,infodata.dptr,infodata.dsize);
  pt1 = (int *)lbuf12;
  x = *pt1;
  pt1++;
  for (y = 0; y < x; y++, pt1++) {
    if (*pt1 == player) return 0;
  }
  return 1;
}

void mail_version(dbref player)
{
  notify_quiet(player,MAILVERSION);
}

void 
do_mail2(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
    int flags;
    dbref owner;
    char *p1, *p2;

    if (mudstate.mail_state != 1) {
	notify_quiet(player, "Mail: Mail is currently turned off.");
	return;
    }
    lfoldflag = key;
    lfoldplayer = player;
    if (!Wizard(Owner(player))) {
	if (!acs_check(Owner(player))) {
		notify_quiet(player, "Mail: Sorry, you are locked out of mail.");
		return;
	}
    }
    if ((!mudstate.nuke_status) && (!God(player)) && (isPlayer(player))) {
	p1 = atr_get(player, A_MPASS, &owner, &flags);
	if (*p1 != '\0') {
	    p2 = atr_get(player, A_MPSET, &owner, &flags);
	    if (*p2 == '\0') {
		notify_quiet(player, "Mail: Permission denied.");
		free_lbuf(p1);
		free_lbuf(p2);
		return;
	    }
	    free_lbuf(p2);
	}
	free_lbuf(p1);
    }
    switch (key) {
    case M2_CREATE:
	if ((*buf1 == '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder creation format.");
	} else {
	    folder_create(player, buf1);
	}
	break;
    case M2_DELETE:
	if ((*buf1 == '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder delete format.");
	} else {
	    folder_delete(player, buf1);
	}
	break;
    case M2_RENAME:
	if ((*buf1 == '\0') || (*buf2 == '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder rename format.");
	} else {
	    folder_rename(player, buf1, buf2);
	}
	break;
    case M2_MOVE:
	if ((*buf1 == '\0') || (*buf2 == '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder move format.");
	} else {
	    folder_move(player, buf1, buf2);
	}
	break;
   case M2_LIST:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder list format.");
	} else {
	    folder_list(player, NOTHING, 1);
	}
	break;
    case M2_CSHARE:
    case M2_CURRENT:
	if (key == M2_CSHARE)
	    flags = 1;
	else
	    flags = 0;
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper current folder format.");
	} else {
	    folder_current(player, flags,NOTHING,1);
	}
	break;
    case M2_SHARE:
    case M2_CHANGE:
	if (key == M2_SHARE)
	    flags = 1;
	else
	    flags = 0;
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper folder change format.");
	} else {
	    folder_change(player, buf1, flags);
	}
	break;
    case M2_PLIST:
	if ((*buf1 == '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper folder plist format.");
	} else {
	    folder_plist(player, buf1);
	}
	break;
    default:
	notify_quiet(player, "MAIL ERROR: Unrecognized folder command.");
    }
}

void 
do_single_mail(dbref player, dbref cause, int key, char *string)
{
   char *null_ptr;
   null_ptr = alloc_lbuf("do_single_mail");
   do_mail(player, cause, key, string, null_ptr);
   free_lbuf(null_ptr);
}


void 
do_mail(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
    int key2, flags, flags2, forcesend, i_fill1, i_fill2, i_fill3,
        i_fill4, i_fill5, i_fill6, i_version, acheck, i_pass, i_forced;
    char *p1, *p2, *atrxxx;
    dbref owner, owner2;

    if ( (!key || (key & (M_SEND|M_FORWARD|M_REPLY|M_FSEND|M_ANON))) && (mudstate.cmd_bitmask & NOMAIL) ) {
      notify(player, "MAIL ERROR: Unable to send mail through this method.");
      return;
    }

    recblock = i_version = i_forced = 0;
    i_fill1 = i_fill2 = i_fill3 = 0;
    i_fill4 = i_fill5 = i_fill6 = 0;
    if (key & M_FSEND) {
	key &= ~M_FSEND;
	override = 1;
    } else {
	override = 0;
    }

    if ( (key & M_QUICK) && (key & (M_FORWARD|M_REPLY)) ) {
       i_forced = 1;
       key &=~M_QUICK;
    }
 
    if ( (key & M_SAVE) && (key & M_READM) ) {
       key &= ~M_SAVE;
       i_version = 32;
    }

    if (mudstate.mail_state != 1) {
	notify_quiet(player, "Mail: Mail is currently turned off.");
	return;
    }

    lastplayer = player;
    lastflag = key;
    if (!Wizard(Owner(player))) {
	if (!acs_check(Owner(player))) {
		notify_quiet(player, "Mail: Sorry, you are locked out of mail.");
		return;
	}
    }

    if ( (key & M_QUICK) && (key & ~M_QUICK) && !(key & M_ALIAS) ) {
       notify_quiet(player, "Illegal combination of switches.");
       return;
    }

    if ((!mudstate.nuke_status) && (!God(player)) && (key != M_PASS) && (isPlayer(player)) && (((key != M_QUICK) && (key != 0)) || ((key == 0) && ((*buf1 != '\0') || (*buf2 != '\0'))))) {
	p1 = atr_get(player, A_MPASS, &owner, &flags);
	if (*p1 != '\0') {
	    p2 = atr_get(player, A_MPSET, &owner, &flags);
	    if (*p2 == '\0') {
		notify_quiet(player, "Mail: Permission denied.");
		free_lbuf(p1);
		free_lbuf(p2);
		return;
	    }
	    free_lbuf(p2);
	}
	free_lbuf(p1);
    }
    key2 = key;
    forcesend = 0;
    if ( key & M_SEND ) {
       forcesend = 1;
    }
    if (key2 == 0) {
	if ((*buf1 == '\0') && (*buf2 == '\0')) {
            if (mudconf.mail_default)
	       key2 = M_STATUS;
            else
	       key2 = M_QUICK;
        }
	else if ((*buf1 != '\0') && (*buf2 != '\0'))
	    key2 = M_SEND;
	else if ((*buf1 != '\0') && (*buf2 == '\0'))
	    key2 = M_READM;
    }
    if ((!isPlayer(player)) && (!Wizard(Owner(player)))) {
	notify_quiet(player, "Mail: Permission denied.");
	return;
    }
    if (!isPlayer(player) && (key2 != M_SEND) && (key2 != M_RECALL) && (key2 != M_QUICK)) {
	notify_quiet(player, "Mail: Permission denied.");
	return;
    }
    switch ((key2 &~ M_ANON)) {
    case 0:
        key2 = (key2 | M_SEND);
    case M_REPLY:
    case M_SEND:
        while ( buf1 && isspace(*buf1) ) {
           buf1++;
        }
	if ((*buf2 != '\0') && (*buf1 != '\0')) {
           if ( (Brandy(player) && !i_forced) && !forcesend && isPlayer(player)) {
	      atrxxx = atr_get(player, A_SAVESENDMAIL, &owner2, &flags2);
	      if ( *atrxxx ) {
                 notify_quiet(player, "Mail: You already have a message in progress.");
                 notify_quiet(player, "      Use '-' to add new text, '--' to send, '-~' to send anonymously.");
              } else {
                 i_pass = 1;
                 if ( (key2 &~ M_ANON) == M_REPLY ) {
                    if ( isdigit(*buf1) ) {
                       acheck = atoi(buf1);
                       acheck = get_msg_index(player,acheck,1,NOTHING,1);
                    } else if ( (*buf1 == '@') || (*buf1 == '^') ) {
                       acheck = atoi(buf1+1);
                       acheck = get_msg_index(player,acheck,1,NOTHING,1);
                    } else {
                       notify_quiet(player, "MAIL ERROR: Number expected");
                       i_pass = 0;
                       acheck = -255;
                    }
                    if ( (acheck < 1) && (acheck != -255) ) {
	               notify_quiet(player, "MAIL ERROR: Bad message number specified");
                       i_pass = 0;
                    }
                 }
                 if ( i_pass ) {
                    if ( (key2 & M_REPLY) ) {
                       atr_add_raw(player, A_TEMPBUFFER, "1 0");
                    } else {
                       atr_add_raw(player, A_TEMPBUFFER, "0 0");
                    }

                    atr_add_raw(player, A_SAVESENDMAIL, buf1);
	            mail_write(player, key, "+subject", buf2);
	            mail_write(player, key, "\t", "");
                    notify_quiet(player, "Mail: You begin writing your new mail message.");
                    notify_quiet(player, "      Use '-' to add new text, '--' to send, '-~' to send anonymously.");
                 }
              }
              free_lbuf(atrxxx);
           } else {
	      mail_send(player, key2, buf1, buf2, NULL);
           }
	} else {
	    notify_quiet(player, "MAIL ERROR: Improper format.");
	}
	break;
    case M_FORWARD:
        while ( buf1 && isspace(*buf1) ) {
           buf1++;
        }
	if (*buf1 != '\0') {
           if ( (Brandy(player) && !i_forced) && isPlayer(player)) {
              atrxxx = atr_get(player, A_SAVESENDMAIL, &owner2, &flags2);
              if ( *atrxxx ) {
                 notify_quiet(player, "Mail: You already have a message in progress.");
                 notify_quiet(player, "      Use '-' to add new lines of text, '--' to send.");
              } else {
                 i_pass = 1;
                 if ( isdigit(*buf1) ) {
                    acheck = atoi(buf1);
                    acheck = get_msg_index(player,acheck,1,NOTHING,1);
                 } else {
                    notify_quiet(player, "MAIL ERROR: Number expected");
                    i_pass = 0;
                    acheck = -255;
                 }
                 if ( (acheck < 1) && (acheck != -255) ) {
                    notify_quiet(player, "MAIL ERROR: Bad message number specified");
                    i_pass = 0;
                 }
                 if ( i_pass ) {
                    atr_add_raw(player, A_TEMPBUFFER, "2 0");
                    atr_add_raw(player, A_SAVESENDMAIL, buf1);
	            mail_write(player, key, "+subject", buf2);
	            mail_write(player, key, "\t", "");
                    notify_quiet(player, "Mail: You begin writing your new mail message.");
                    notify_quiet(player, "      Use '-' to add new lines of text, '--' to send.");
                 }
              }
              free_lbuf(atrxxx);
           } else {
	      mail_send(player, key2, buf1, buf2, 0);
           }
	} else {
	    notify_quiet(player, "MAIL ERROR: Improper forward format.");
	}
	break;
    case M_READM:
	if ((*buf2 == '\0') && (*buf1 != '\0')) {
	    mail_read(player, buf1, NOTHING, (1 | i_version));
	} else {
	    notify_quiet(player, "MAIL ERROR: Improper read format or no message specified.");
	}
	break;
    case M_LOCK:
	if (*buf2 == '\0') {
	    if (*buf1 == '\0')
		mail_unlock(player);
	    else
		mail_lock(player, buf1);
	} else {
	    notify_quiet(player, "MAIL ERROR: Improper lock format.");
	}
	break;
    case M_STATUS:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper status format.");
	} else {
            if ( (key & M_ANON) && Wizard(player) ) {
	       mail_status(player, buf1, NOTHING, 1 | M_ANON, 1, (char *)NULL, (char *)NULL);
            } else {
	       mail_status(player, buf1, NOTHING, 1, 1, (char *)NULL, (char *)NULL);
            }
	}
	break;
    case M_DELETE:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper delete format.");
	} else {
	    mail_delete(player, NOTHING, 1, NOTHING, 1, 0, 0);
	}
	break;
    case M_RECALL:
    case (M_RECALL | M_ALL):
	mail_recall(player, buf1, buf2, key2, NOTHING, 1);
	break;
    case M_MARK:
    case (M_MARK | M_SAVE):
    case M_UNMARK:
	if ((*buf2 != '\0') || (*buf1 == '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper mark/unmark format.");
	} else {
	    mail_mark(player, key, buf1, NOTHING, 1);
	}
	break;
    case M_REJECT:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper reject format.");
	} else {
	    mail_reject(player, buf1);
	}
	break;
    case M_WRITE:
    case (M_WRITE | M_ALL) :
	if (*buf1 == '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper write format.");
	} else {
	    mail_write(player, key, buf1, buf2);
	}
	break;
    case M_CHECK:
	if ((*buf1 == '\0') || (*buf2 == '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper check format.");
	} else {
	    mail_check(player, buf1, buf2);
	}
	break;
    case M_NUMBER:
	if (*buf1 == '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper number format.");
	} else {
	    mail_number(player, buf1, buf2);
	}
	break;
    case M_QUICK:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper quick status format.");
	} else {
	    if (isPlayer(player))
		mail_quick(player, buf1);
	    else
		mail_quick(cause, buf1);
	}
	break;
    case M_SHARE:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper share format.");
	} else {
	    mail_share(player, buf1);
	}
	break;
    case M_PASS:
	if (*buf1 == '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper password format.");
	} else {
	    mail_pass(player, buf1, buf2);
	}
	break;
    case M_PAGE:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper page format.");
	} else {
	    mail_page(player, buf1);
	}
	break;
    case M_QUOTA:
	if (*buf2 != '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper quota format.");
	} else {
	    mail_quota(player, buf1, 0, &i_fill1, &i_fill2, &i_fill3, &i_fill4, &i_fill5, &i_fill6);
	}
	break;
    case M_ZAP:
    case M_NEXT:
        if (*buf1 == '-' && strlen(buf1) == 1 )
            mail_sread(player, key2 | M_PREV);
	else if ((*buf1) || (*buf2)) {
	    notify_quiet(player, "MAIL ERROR: Improper next/zap format.");
	} else {
	    mail_sread(player, key2);
	}
	break;
    case (M_ALIAS|M_QUICK):
    case M_ALIAS:
	if ((*buf1) || (*buf2)) {
	    notify_quiet(player, "MAIL ERROR: Improper alias format.");
	} else {
	    mail_acheck(player, key2);
	}
	break;
    case M_AUTOFOR:
	if (*buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper autofor format.");
	} else {
	    mail_autofor(player, buf1);
	}
	break;
    case M_VERSION:
	if (*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper version format.");
	} else {
	    mail_version(player);
	}
	break;
    default:
        if ( key & (M_ALL|M_ANON|M_FSEND|M_QUICK|M_SAVE) ) {
           notify_quiet(player, "Illegal combination of switches.");
        } else {
	   notify_quiet(player, "MAIL ERROR: Unknown mail command.");
        }
    }
}

void 
do_wmail(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
    int key2, flags;
    char *p1, *p2;
    FILE *fp;
    dbref owner;

    recblock = 0;
    if ((mudstate.mail_state != 1) && (key != WM_OFF) && (key != WM_ON)) {
      if (key == WM_WIPE) {
	mnuke_save(player,buf1);
	return;
      } else {
         if ( key == WM_LOAD ) {
            lastwplayer = player;
            lastwflag = key;
            p1 = alloc_lbuf("clobber_mail");
            sprintf(p1, "rm -f data/%s.mail.* data/%s.folder.* >/dev/null 2>&1", mudconf.muddb_name, mudconf.muddb_name); 
            mail_close();
            system(p1);
            mudstate.mail_state = mail_init();
            if ( mudstate.mail_state != 1 ) {
	       notify_quiet(player, "Mail: Mail was unable to be recovered.  Sorry.");
            } else {
               sprintf(p1, "data/%s.dump.mail", mudconf.muddb_name);
               if ( (fp = fopen(p1, "r")) != NULL ) {
                  fclose(fp);
                  mail_load(player);
               } else {
                  sprintf(p1, "cp -f prevflat/%s.dump.* data > /dev/null 2>&1", mudconf.muddb_name);
                  system(p1);
                  sprintf(p1, "data/%s.dump.mail", mudconf.muddb_name);
                  if ( (fp = fopen(p1, "r")) != NULL ) {
                     fclose(fp);
                     mail_load(player);
                  } else {
                     notify_quiet(player, "Mail: no mail flatfile to load.  Using a new mail database.");
                  }
               }
            }
            free_lbuf(p1);
         } else {
	    notify_quiet(player, "Mail: Mail is currently turned off.");
         }
	return;
      }
    }
    if (!key) {
	notify_quiet(player, "Wizard mail has no default switch.");
	return;
    }
    lastwplayer = player;
    lastwflag = key;
    if (!Wizard(Owner(player))) {
	if (!acs_check(Owner(player))) {
		notify_quiet(player, "Mail: Sorry, you are locked out of mail.");
		return;
	}
    }
    if ((!mudstate.nuke_status) && (!God(player)) && (isPlayer(player))) {
	p1 = atr_get(player, A_MPASS, &owner, &flags);
	if (*p1 != '\0') {
	    p2 = atr_get(player, A_MPSET, &owner, &flags);
	    if (*p2 == '\0') {
		notify_quiet(player, "Mail: Permission denied.");
		free_lbuf(p1);
		free_lbuf(p2);
		return;
	    }
	    free_lbuf(p2);
	}
	free_lbuf(p1);
    }
    key2 = key;
    if ((!isPlayer(player)) && (!Wizard(Owner(player)))) {
	notify_quiet(player, "Mail: Permission denied.");
	return;
    }
    if (!isPlayer(player) && (key2 != WM_UNLOAD)) {
	notify_quiet(player, "Mail: Permission denied.");
	return;
    }
    switch (key2) {
    case WM_CLEAN:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper clean format.");
	} else {
	    mail_clean(player);
	}
	break;
    case WM_LOAD:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper load format.");
	} else {
	    mail_load(player);
	}
	break;
    case WM_UNLOAD:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper unload format.");
	} else {
	    mail_unload(player);
	}
	break;
    case WM_WIPE:
	if ((*buf1 == '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper wipe format.");
	} else {
	    mail_wipe(player, buf1);
	}
	break;
    case WM_ON:
    case WM_OFF:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper on/off format.");
	} else {
	    mail_onoff(player, key2);
	}
	break;
    case WM_ACCESS:
	if (*buf1 == '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper access format.");
	} else {
	    mail_access(player, buf1, buf2);
	}
	break;
    case WM_OVER:
	if ((*buf1 == '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper override format.");
	} else {
	    mail_over(player, buf1);
	}
	break;
    case WM_RESTART:
	if ((*buf1 != '\0') || (*buf2 != '\0')) {
	    notify_quiet(player, "MAIL ERROR: Improper restart format.");
	} else {
	    mail_restart(player);
	}
	break;
    case WM_SIZE:
	if (*buf1 == '\0') {
	    notify_quiet(player, "MAIL ERROR: Improper size format.");
	} else {
	    mail_size(player, buf1, buf2);
	}
	break;
    case WM_FIX:
	if (*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper fix format.");
	} else {
	    do_mailfix(player);
	}
	break;
    case WM_LOADFIX:
	if (*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper lfix format.");
	} else {
	    mail_lfix(player);
	}
	break;
    case WM_MTIME:
	if (!*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper time format.");
	} else {
	    mail_time(player, buf1);
	}
	break;
    case WM_SMAX:
	if (!*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper smax format.");
	} else {
	    mail_smax(player, buf1);
	}
	break;
    case WM_VERIFY:
	if (*buf1 || *buf2) {
	    notify_quiet(player, "MAIL ERROR: Improper verify format.");
	} else {
	    mail_verify(player);
	}
	break;
    case WM_DTIME:
	mail_dtime(player, buf1, buf2);
	break;
    case (WM_ALIAS | WM_ALOCK):
    case (WM_ALIAS | WM_REMOVE):
    case (WM_ALIAS | WM_REMOVE | WM_ALOCK):
    case WM_ALIAS:
	mail_alias(player, key, buf1, buf2);
	break;
    default:
	notify_quiet(player, "MAIL ERROR: Unknown mail command.");
    }
}
