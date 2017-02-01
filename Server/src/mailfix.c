/* Program to rebuild mail and folder databases offline. */

#ifdef SOLARIS
/* Solaris requires these to be delcared because it's stupid. */
void bcopy(const void *, void *dest, int);
void bzero(void *, int);
#endif
#include "autoconf.h"
#include <stdio.h>
#include <sys/file.h>
#ifdef HAVE_NDBM
#include	"redirect_ndbm.h"
#else
#ifdef HAVE_DBM
#include        <dbm.h>
#else
#include        "myndbm.h"
#endif
#endif
#include "config.h"
#include "db.h"
#include "externs.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "mail.h"

#ifdef STANDALONE
#include "debug.h"
#endif

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
#define MAXNUKEPLY	-10
#define PMNUKEPLY	10
#define MAXWRTLN	1000
#define BSMIN		10
#define BSMAX		((NDBMBUFSZ > 4092) ? ((NDBMBUFSZ / sizeof(short int)) - 3) : 9999)

#define Useg_obj(x)	(Good_obj(x) && !Going(x) && !Recover(x))

typedef struct ptrlst {
  void *ptr;
  struct ptrlst *next;
} PTRLST;

typedef struct ptrlst2 {
  char *ptr;
  int player;
  short int index;
  int len;
  struct ptrlst2 *next;
} PTRLST2;

typedef struct intlst {
  int *intlst;
  int player;
  char *used;
  short int index;
  struct intlst *next;
} INTLST;

typedef struct ntrack {
  int newplay;
  short int newind;
  PTRLST2 *msg;
  INTLST *hsnd;
  struct ntrack *next;
} NTRACK;

typedef struct blist {
  char *name;
  short int *list;
  struct blist *next;
} BLIST;

typedef struct gastruct {
  char *name;
  char *lock;
  int locklen;
  int *list;
  struct gastruct *next;
} GASTRUCT;

typedef struct plyst {
  int player;
  short int *ircv;
  short int *isnd;
  PTRLST2 *hrcv;
  INTLST *hsnd;
  PTRLST2 *msg;
  short int bsize;
  char *rejm;
  short int rejlen;
  int wrtm;
  PTRLST2 *wrtl;
  char *wrts;
  int wrtlen;
  short int page;
  int afor;
  char *flist;
  short int fllen;
  BLIST *box;
  char *cshare;
  short int cslen;
  char *curr;
  short int culen;
  struct plyst *next;
} PLYST;

#ifdef STANDALONE
DBM *foldfile;
DBM *mailfile;

#else
extern DBM *foldfile;
extern DBM *mailfile;
extern void mail_proc_err(); 

#endif
static FILE *output1, *output2;
static int errcount;
static PTRLST *allptr;
static PLYST *allplay;
static datum keydata, infodata;
static short int smax;
static int *maccess;
static GASTRUCT *alias;
static INTLST *nukehsnd;
static PTRLST2 *nukemsg;
static NTRACK *nuketrack;
static char *buf1;
static char *buf2;
static char *buf3;
static char *hdlst, *mslst;
static int absmaxinx;
static int absmaxrcp;
static int absmaxacs;
static int htimeoff;
static int hsizeoff;
static int hindoff;
static int hsuboff;
static int hstoff;
static short int tomask;

static char *err_msg[]={"Corrupted key in mail database",
			"Bad player in fix readin",
			"Duplicate receive index",
			"Insufficient receive record index size",
			"Corrupted receive index record",
/* 5 */			"Duplicate send index",
			"Duplicate send index for nuke tracking",
			"Duplicate box size",
			"Duplicate Write_M record",
			"Duplicate page",
/* 10 */		"Duplicate autofor",
			"Duplicate write subject",
			"Duplicate reject message",
			"Duplicate smax",
			"Duplicate access",
/* 15 */		"Duplicate alias",
			"Duplicate alias lock",
			"Unknown mail key",
			"Bad player in folder information",
			"Extra information in folder database",
/* 20 */		"Duplicate folder list records",
			"Duplicate folder cshare records",
			"Duplicate folder current records",
			"Duplicate box",
			"Unknown folder key",
/* 25 */		"mail.fix open failed",
			"folder.fix open failed",
			"Buffer allocation failed",
			"Memory allocation failed",
			"Insufficient send record index size",
/* 30 */		"Corrupted receive index record",
			"Corrupted receive header record",
			"Mail data record overflow",
			"Corrupted box size record",
			"Corrupted wrt_m record",
/* 35 */		"Corrupted page record",
			"Corrupted autofor record",
			"Corrupted smax record",
			"Corrupted access record",
			"Corrupted global alias record",
/* 40 */		"Corrupted key in folder database",
			"Folder data record overflow",
			"Corrupted box/folder record",
			"Duplicate message record index",
			"Stored message length too long. Adjusting to fit actual",
/* 45 */		"Actual message length is zero. Removing message record",
			"Duplicate header send record index",
			"Bad player in header send record",
			"Index out of range in header receive record",
			"Index out of range in header send record",
/* 50 */		"Index out of range in message record",
			"Index out of range in write line record",
			"No matching header send record for message record",
			"No matching message record for header send record",
			"No matching header send record for message record in nuke index assignment",
/* 55 */		"No matching message record for header send record in nuke index assignment",
			"No send header or message record for receive header",
			"Index for message out of range in header receive record",
			"Index in header receive not found in header send",
			"Index for header receive record out of range",
/* 60 */		"Index for header receive record not found in header send record",
			"Header send record full",
			"Message record not found",
			"Bad size in header receive record",
			"Bad message type in receive header backup slot",
/* 65 */		"Incomplete wipe detected in receive header",
			"Bad message type in receive header slot",
			"Bad type in receive header forward slot",
			"Revised send header empty",
			"Index in send index record out of range",
/* 70 */		"Duplicate index in send index record",
			"Extra index in send index record",
			"No valid indecies in send index record",
			"Index not in send index record",
			"Incorrect top index in send index record",
/* 75 */		"Free index in send index record out of range",
			"Bad free index value in send index record",
			"Duplicate free index value in send index record",
			"Index in receive index record out of range",
			"Duplicate index in receive index record",
/* 80 */		"Extra index in receive index record",
			"No valid indecies in receive index record",
			"Index not in receive index record",
			"Incorrect top index in receive index record",
			"Free index in receive index record out of range",
/* 85 */		"Bad free index value in receive index record",
			"Duplicate free index value in receive index record",
			"Invalid wrtm record for nuke tracking player id",
			"Invalid wrts record for nuke tracking player id",
			"Invalid wrtl record(s) for nuke tracking player id",
/* 90 */		"No wrtm record for wrts record",
			"No wrtm record for wrtl record(s)",
			"Invalid wrtm entry",
			"Bad wrtl index",
			"Duplicate wrtl index",
/* 95 */		"Bad wrtl length",
			"Stored wrtl length doesn't match actual",
			"Stored write subject length doesn't match actual",
			"Bad write subject length",
			"Bad box size",
/* 100 */		"Invalid reject message length stored",
			"No reject message pointer, but stored length",
			"Stored reject message length doesn't match actual",
			"Correct reject message length is invalid",
			"Message length is too long.  Truncating",
/* 105 */		"Bad page value",
			"Bad player in autofor",
			"Folder list length out of range",
			"Folder list pointer, but no length set",
			"Stored folder list length doesn't match actual",
/* 110 */		"Corrected folder list length out of range",
			"Shared folder length out of range",
			"Shared folder pointer, but no length set",
			"Stored shared folder length doesn't match actual",
			"Corrected shared folder length out of range",
/* 115 */		"Current folder length out of range",
			"Current folder pointer, but no length set",
			"Stored current folder length doesn't match actual",
			"Corrected current folder length out of range",
			"Bad folder in folder list",
/* 120 */		"Share folder invalid",
			"Current folder invalid",
			"Bad folder name in box list",
			"Index in folder out of range",
			"Duplicate index in folder",
/* 125 */		"Index in folder not in master index",
			"Index in master index not in any folder index",
			"Bad smax record",
			"Bad access record",
			"Invalid player in access list",
/* 130 */		"Repaired access list is empty",
			"Global alias name too long",
			"Corrupted global alias lock length",
			"Lock length, but no lock",
			"Stored global alias lock length doesn't match actual",
/* 135 */		"Global alias lock, but no alias present",
			"Corrupted global alias list",
			"Bad player in global alias",
			"Corrected global alias is blank",
			"Stored wrtm value doesn't match actual",
/* 140 */		"Bad write line index",
			"Message reassigned to nuke tracking"};

void err_out(int errnum, char *info1, char *info2)
{
  errcount++;
  if ( (errnum < 0) || (errnum > 141) ) {
     if ( info2 ) {
        fprintf(stderr, "FIX ERROR: Unrecognized error code %d -> %s,%s\n", errnum, info1, info2);
     } else if ( info1 ) {
        fprintf(stderr, "FIX ERROR: Unrecognized error code %d -> %s\n", errnum, info1);
     } else {
        fprintf(stderr, "FIX ERROR: Unrecognized error code %d\n", errnum);
     }
     return;
  }

  if (info2)
    fprintf(stderr,"FIX ERROR: %s -> %s,%s\n",err_msg[errnum],info1,info2);
  else if (info1)
    fprintf(stderr,"FIX ERROR: %s -> %s\n",err_msg[errnum],info1);
  else
    fprintf(stderr,"FIX ERROR: %s\n",err_msg[errnum]);
}

void *myapush(int len)
{
  void *pt1, *pt2;
  PTRLST *pt3;

  pt1 = malloc(len + 3);
  pt3 = (PTRLST *)malloc(sizeof(PTRLST));
  if (!pt1 || !pt3) return NULL;
  if (!((pmath1)pt1 % 4))
    pt2 = pt1;
  else
    pt2 = pt1 + 4 - ((pmath1)pt1 % 4);
  pt3->ptr = pt1;
  pt3->next = allptr;
  allptr = pt3;
  return pt2;
}

void myapurge()
{
  PTRLST *pt1;

  while(allptr) {
    pt1 = allptr;
    allptr = pt1->next;
    free((void *)(pt1->ptr));
    free((void *)pt1);
  }
}

PLYST *findplay(int player)
{
  PLYST *pt1;

  pt1 = allplay;
  while (pt1) {
    if (player == pt1->player) break;
    pt1 = pt1->next;
  }
  return pt1;
}

#ifdef STANDALONE
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
#else
extern char *mystrchr(char *, char);
#endif

#ifdef STANDALONE
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
#else
extern char *strstr2(char *, char *);
#endif

GASTRUCT *asearch(char *nck)
{
  GASTRUCT *pt1;

  pt1 = alias;
  while (pt1) {
    if (!strcmp(pt1->name,nck)) break;
    pt1 = pt1->next;
  }
  return pt1;
}

int boxsearch(BLIST *ckpt, char *bck)
{
  BLIST *pt1;

  pt1 = ckpt;
  while (pt1) {
    if (!strcmp(pt1->name,bck)) break;
    pt1 = pt1->next;
  }
  return (pt1 != NULL);
}

void initplay(PLYST *pt1)
{
  pt1->ircv = NULL;
  pt1->isnd = NULL; 
  pt1->hrcv = NULL; 
  pt1->hsnd = NULL; 
  pt1->msg = NULL;
  pt1->rejm = NULL;
  pt1->wrtl = NULL;
  pt1->wrts = NULL; 
  pt1->flist = NULL;
  pt1->cshare = NULL;
  pt1->curr = NULL;
  pt1->box = NULL;
  pt1->fllen = pt1->cslen = pt1->culen = 0;
  pt1->rejlen = pt1->wrtlen = pt1->bsize = pt1->wrtm = pt1->page = 0;
  pt1->afor = -1;
}

int readin()
{
  PLYST *pt1 = NULL;
  PTRLST2 *pt2 = NULL;
  INTLST *pt3 = NULL;
  GASTRUCT *pt4 = NULL;
  char *pt5 = NULL;
  BLIST *pt6 = NULL;
  int player, key, ntest;
  short int index;

  player = key = ntest = 0;
  index = 0;
  keydata = dbm_firstkey(mailfile);
  while (keydata.dptr) {
    if (keydata.dsize < sizeof(int)) {
      err_out(0,NULL,NULL);
      keydata = dbm_nextkey(mailfile);
      continue;
    }
    infodata = dbm_fetch(mailfile,keydata);
    if (infodata.dptr) {
      if (infodata.dsize > NDBMBUFSZ) {
	err_out(32,NULL,NULL);
	keydata = dbm_nextkey(mailfile);
	continue;
      }
      bcopy(keydata.dptr,buf1,keydata.dsize);
      key = *(int *)buf1;
      if ((key != MIND_SMAX) && (key != MIND_ACS) && (key != MIND_GA) && (key != MIND_GAL)) {
	if ((key == MIND_HRCV) || (key == MIND_HSND) || (key == MIND_MSG) || (key == MIND_WRTL)) {
	  if (keydata.dsize != ((sizeof(int) << 1) + sizeof(short int))) {
	    err_out(0,NULL,NULL);
            keydata = dbm_nextkey(mailfile);
            continue;
          }
	}
        else if (keydata.dsize != (sizeof(int) << 1)) {
	  err_out(0,NULL,NULL);
          keydata = dbm_nextkey(mailfile);
          continue;
        }
	ntest = 0;
	player = *(int *)(buf1 + sizeof(int));
	if ((player < 0) || (!Useg_obj(player)))
	  strcpy(buf3,"(Nuked)");
	else
	  strncpy(buf3,Name(player),22);
	if (((player >= 0) && (!Useg_obj(player) || !isPlayer(player))) && (key != MIND_HSND) && (key != MIND_MSG)) {
	  err_out(1,NULL,NULL);
	  keydata = dbm_nextkey(mailfile);
	  continue;
	}
	else if (((player >= 0) && (!Useg_obj(player) || !isPlayer(player))) && ((key == MIND_HSND) || (key == MIND_MSG))) {
	  ntest = 1;
	}
	if (!ntest) {
	  pt1 = findplay(player);
	  if (!pt1) {
	    pt1 = (PLYST *)myapush(sizeof(PLYST));
	    if (!pt1) return 0;
	    pt1->next = allplay;
	    allplay = pt1;
	    pt1->player = player;
	    initplay(pt1);
	  }
	}
      }
      switch (key) {
	case MIND_IRCV:
	  if (pt1->ircv) 
	    err_out(2,buf3,NULL);
	  else {
	    if (infodata.dsize < (sizeof(short int) << 2)) 
	      err_out(3,buf3,NULL);
	    else {
	      pt1->ircv = (short int *)myapush(infodata.dsize);
	      if (!pt1->ircv) return 0;
	      bcopy(infodata.dptr,pt1->ircv,infodata.dsize);
	      if (((*(pt1->ircv + 1) + *(pt1->ircv + 2) + 3) * sizeof(short int)) != infodata.dsize) {
		err_out(4,buf3,NULL);
		pt1->ircv = NULL;
	      }
	    }
	  }
	  break;
	case MIND_ISND:
	  if (pt1->isnd) {
	    if (player >= 0)
	      err_out(5,buf3,NULL);
	    else
	      err_out(6,buf3,NULL);
	  }
	  else {
	    if (infodata.dsize < (sizeof(short int) << 2)) 
	      err_out(29,buf3,NULL);
	    else {
	      pt1->isnd = (short int *)myapush(infodata.dsize);
	      if (!pt1->isnd) return 0;
	      bcopy(infodata.dptr,pt1->isnd,infodata.dsize);
	      if (((*(pt1->isnd + 1) + *(pt1->isnd + 2) + 3) * sizeof(short int)) != infodata.dsize) {
		err_out(30,buf3,NULL);
		pt1->isnd = NULL;
	      }
	    }
	  }
	  break;
	case MIND_HRCV:
	  index = *(short int *)(buf1 + (sizeof(int) << 1));
	  if (infodata.dsize < hsuboff)
	    err_out(31,buf3,NULL);
	  else if ((index < 1) || (index > absmaxinx))
	    err_out(48,buf3,NULL);
	  else {
	    pt2 = (PTRLST2 *)myapush(sizeof(PTRLST2));
	    if (!pt2) return 0;
	    pt2->ptr = myapush(infodata.dsize);
	    if (!pt2->ptr) return 0;
	    bcopy(infodata.dptr,pt2->ptr,infodata.dsize);
	    pt2->index = index;
	    pt2->len = infodata.dsize;
	    pt2->next = pt1->hrcv;
	    pt1->hrcv = pt2;
	  }
	  break;
	case MIND_HSND:
	  pt3 = (INTLST *)myapush(sizeof(INTLST));
	  if (!pt3) return 0;
	  pt3->intlst = (int *)myapush(infodata.dsize);
	  if (!pt3->intlst) return 0;
	  bcopy(infodata.dptr,pt3->intlst,infodata.dsize);
	  index = *(short int *)(buf1 + (sizeof(int) << 1));
	  if (infodata.dsize != (((*(pt3->intlst) * 2) + 1) * sizeof(int)))
	    err_out(32,buf3,NULL);
	  else if ((index < 1) || (index > absmaxinx))
	    err_out(49,buf3,NULL);
	  else {
	    pt3->index = index;
	    if (ntest) {
	      pt3->player = player;
	      pt3->next = nukehsnd;
	      nukehsnd = pt3;
	    }
	    else {
	      pt3->player = NOTHING;
	      pt3->next = pt1->hsnd;
	      pt1->hsnd = pt3;
	    }
	  }
	  break;
	case MIND_MSG:
	  index = *(short int *)(buf1 + (sizeof(int) << 1));
	  if ((index < 1) || (index > absmaxinx))
	    err_out(50,buf3,NULL);
	  else {
	    pt2 = (PTRLST2 *)myapush(sizeof(PTRLST2));
	    if (!pt2) return 0;
	    pt2->ptr = myapush(infodata.dsize);
	    if (!pt2->ptr) return 0;
	    bcopy(infodata.dptr,pt2->ptr,infodata.dsize);
	    pt2->index = index;
	    pt2->len = infodata.dsize;
	    if (ntest) {
	      pt2->player = player;
	      pt2->next = nukemsg;
	      nukemsg = pt2;
	    }
	    else {
	      pt2->player = NOTHING;
	      pt2->next = pt1->msg;
	      pt1->msg = pt2;
	    }
	  }
	  break;
	case MIND_WRTL:
	  index = *(short int *)(buf1 + (sizeof(int) << 1));
	  if ((index < 1) || (index > MAXWRTLN))
	    err_out(51,buf3,NULL);
	  else {
	    pt2 = (PTRLST2 *)myapush(sizeof(PTRLST2));
	    if (!pt2) return 0;
	    pt2->ptr = myapush(infodata.dsize);
	    if (!pt2->ptr) return 0;
	    bcopy(infodata.dptr,pt2->ptr,infodata.dsize);
	    pt2->index = index;
	    pt2->len = infodata.dsize;
	    pt2->next = pt1->wrtl;
	    pt1->wrtl = pt2;
	  }
	  break;
	case MIND_BSIZE:
	  if (pt1->bsize)
	    err_out(7,buf3,NULL);
	  else if (infodata.dsize != sizeof(short int))
	    err_out(33,buf3,NULL);
	  else {
	    bcopy(infodata.dptr,buf2,sizeof(short int));
	    pt1->bsize = *(short int *)buf2;
	  }
	  break;
	case MIND_WRTM:
	  if (pt1->wrtm)
	    err_out(8,buf3,NULL);
	  else if (infodata.dsize != sizeof(short int))
	    err_out(34,buf3,NULL);
	  else {
	    bcopy(infodata.dptr,buf2,sizeof(short int));
	    pt1->wrtm = *(short int *)buf2;
	  }
	  break;
	case MIND_PAGE:
	  if (pt1->page)
	    err_out(9,buf3,NULL);
	  else if (infodata.dsize != sizeof(short int))
	    err_out(35,buf3,NULL);
	  else {
	    bcopy(infodata.dptr,buf2,sizeof(short int));
	    pt1->page = *(short int *)buf2;
	  }
	  break;
	case MIND_AFOR:
	  if (pt1->afor >= 0)
	    err_out(10,buf3,NULL);
	  else if (infodata.dsize != sizeof(int))
	    err_out(36,buf3,NULL);
	  else {
	    bcopy(infodata.dptr,buf2,sizeof(int));
	    pt1->afor = *(int *)buf2;
	  }
	  break;
	case MIND_WRTS:
	  if (pt1->wrts)
	    err_out(11,buf3,NULL);
	  else {
	    pt1->wrts = myapush(infodata.dsize);
	    if (!pt1->wrts) return 0;
	    pt1->wrtlen = infodata.dsize;
	    bcopy(infodata.dptr,pt1->wrts,infodata.dsize);
	  }
	  break;
	case MIND_REJM:
	  if (pt1->rejm)
	    err_out(12,buf3,NULL);
	  else {
	    pt1->rejm = myapush(infodata.dsize);
	    if (!pt1->rejm) return 0;
	    pt1->rejlen = infodata.dsize;
	    bcopy(infodata.dptr,pt1->rejm,infodata.dsize);
	  }
	  break;
	case MIND_SMAX:
	  if (smax)
	    err_out(13,buf3,NULL);
	  else if (infodata.dsize != sizeof(short int))
	    err_out(37,buf3,NULL);
	  else {
	    bcopy(infodata.dptr,buf2,sizeof(short int));
	    smax = *(short int *)buf2;
	  }
	  break;
	case MIND_ACS:
	  if (maccess)
	    err_out(14,NULL,NULL);
	  else {
	    maccess = (int *)myapush(infodata.dsize);
	    if (!maccess) return 0;
	    bcopy(infodata.dptr,maccess,infodata.dsize);
	    if (((*maccess + 1) * sizeof(int)) != infodata.dsize) {
	      err_out(38,NULL,NULL);
	      maccess = NULL;
	    }
	  }
	  break;
	case MIND_GA:
	  if ((keydata.dsize <= 5) || *((char *)(keydata.dptr + (keydata.dsize-1))))
	    err_out(0,NULL,NULL);
	  else {
	    pt5 = keydata.dptr + sizeof(int);
	    pt4 = asearch(pt5);
	    if (pt4 && (pt4->list))
	      err_out(15,pt5,NULL);
	    else if (pt4) {
	      pt4->list = (int *)myapush(infodata.dsize);
	      if (!pt4->list) return 0;
	      bcopy(infodata.dptr,pt4->list,infodata.dsize);
	      if (((*(pt4->list) + 1) * sizeof(int)) != infodata.dsize) {
		err_out(39,NULL,NULL);
		pt4->list = NULL;
	      }
	    }
	    else {
	      pt4 = (GASTRUCT *)myapush(sizeof(GASTRUCT));
	      if (!pt4) return 0;
	      pt4->list = (int *)myapush(infodata.dsize);
	      if (!pt4->list) return 0;
	      bcopy(infodata.dptr,pt4->list,infodata.dsize);
	      if (((*(pt4->list) + 1) * sizeof(int)) != infodata.dsize) {
		err_out(39,NULL,NULL);
		pt4->list = NULL;
	      }
	      else {
		pt4->name = myapush(strlen(pt5) + 1);
		if (!(pt4->name)) return 0;
		strcpy(pt4->name,pt5);
	        pt4->next = alias;
	        pt4->lock = NULL;
		pt4->locklen = 0;
		alias = pt4;
	      }
	    } 
	  }
	  break;
	case MIND_GAL:
	  if ((keydata.dsize <= 5) || *((char *)(keydata.dptr + (keydata.dsize-1))))
	    err_out(0,NULL,NULL);
	  else {
	    pt5 = keydata.dptr + sizeof(int);
	    pt4 = asearch(pt5);
	    if (pt4 && (pt4->lock))
	      err_out(16,pt5,NULL);
	    else if (pt4) {
	      pt4->lock = myapush(infodata.dsize);
	      if (!pt4->lock) return 0;
	      bcopy(infodata.dptr,pt4->lock,infodata.dsize);
	      pt4->locklen = infodata.dsize;
	    }
	    else {
	      pt4 = (GASTRUCT *)myapush(sizeof(GASTRUCT));
	      if (!pt4) return 0;
	      pt4->lock = myapush(infodata.dsize);
	      if (!pt4->lock) return 0;
	      bcopy(infodata.dptr,pt4->lock,infodata.dsize);
	      pt4->name = myapush(strlen(pt5) + 1);
	      if (!(pt4->name)) return 0;
	      strcpy(pt4->name,pt5);
	      pt4->locklen = 0;
	      pt4->next = alias;
	      pt4->list = NULL;
	      alias = pt4;
	    }
	  }
	  break;
	default:
	  err_out(17,NULL,NULL);
      }
    }
    keydata = dbm_nextkey(mailfile);
  }
  keydata = dbm_firstkey(foldfile);
  while (keydata.dptr) {
    if (keydata.dsize < (sizeof(int) << 1)) {
      err_out(40,NULL,NULL);
      keydata = dbm_nextkey(foldfile);
      continue;
    }
    infodata = dbm_fetch(foldfile,keydata);
    if (infodata.dptr) {
      bcopy(keydata.dptr,buf1,keydata.dsize);
      key = *(int *)buf1;
      player = *(int *)(buf1 + sizeof(int));
      if (!Useg_obj(player) || !isPlayer(player)) {
	err_out(18,NULL,NULL);
	keydata = dbm_nextkey(foldfile);
	continue;
      }
      if ((key == FIND_BOX) && (keydata.dsize < ((sizeof(int) << 1) + 2))) {
        if ( Good_obj(player) ) {
	   err_out(40,Name(player),NULL);
        } else {
	   err_out(40,"(INVALID PLAYER)",NULL);
        }
	keydata = dbm_nextkey(foldfile);
	continue;
      }
      if (infodata.dsize > NDBMBUFSZ) {
        if ( Good_obj(player) ) {
	   err_out(41,Name(player),NULL);
        } else {
	   err_out(41,"(INVALID PLAYER)",NULL);
        }
	keydata = dbm_nextkey(foldfile);
	continue;
      }
      pt1 = findplay(player);
      if (!pt1) {
         if ( Good_chk(player) ) {
            pt1 = (PLYST *)myapush(sizeof(PLYST));
            if (pt1) {
               pt1->next = allplay;
               allplay = pt1;
               pt1->player = player;
               initplay(pt1);
            } else {
               if ( Good_obj(player) ) {
	          err_out(19,Name(player),NULL);
               } else {
	          err_out(19,"(INVALID PLAYER)",NULL);
               }
	       keydata = dbm_nextkey(foldfile);
	       continue;
            }
        } else {
           if ( Good_obj(player) ) {
	      err_out(19,Name(player),NULL);
           } else {
	      err_out(19,"(INVALID PLAYER)",NULL);
           }
	   keydata = dbm_nextkey(foldfile);
	   continue;
        }
      }
      switch (key) {
	case FIND_LST:
	  if (pt1->flist) {
            if ( Good_obj(player) ) {
	       err_out(20,Name(player),NULL);
            } else {
	       err_out(20,"(INVALID PLAYER)",NULL);
            }
	  } else {
	    pt1->flist = myapush(infodata.dsize);
	    if (!pt1->flist) return 0;
	    bcopy(infodata.dptr,pt1->flist,infodata.dsize);
	    pt1->fllen = infodata.dsize;
	  }
	  break;
	case FIND_CSHR:
	  if (pt1->cshare) {
            if ( Good_obj(player) ) {
	       err_out(21,Name(player),NULL);
            } else {
	       err_out(21,"(INVALID PLAYER)",NULL);
            }
	  } else {
	    pt1->cshare = myapush(infodata.dsize);
	    if (!pt1->cshare) return 0;
	    bcopy(infodata.dptr,pt1->cshare,infodata.dsize);
	    pt1->cslen = infodata.dsize;
	  }
	  break;
	case FIND_CURR:
	  if (pt1->curr) {
            if ( Good_obj(player) ) {
	       err_out(22,Name(player),NULL);
            } else {
	       err_out(22,"(INVALID PLAYER)",NULL);
            }
	  } else {
	    pt1->curr = myapush(infodata.dsize);
	    if (!pt1->curr) return 0;
	    bcopy(infodata.dptr,pt1->curr,infodata.dsize);
	    pt1->culen = infodata.dsize;
	  }
	  break;
	case FIND_BOX:
	  pt5 = keydata.dptr + (sizeof(int) << 1);
	  if (boxsearch(pt1->box,pt5)) {
             if ( Good_obj(player) ) {
	       err_out(23,Name(player),pt5);
             } else {
	       err_out(23,"(INVALID PLAYER)",pt5);
             }
	  } else {
	    pt6 = (BLIST *)myapush(sizeof(BLIST));
	    if (!pt6) return 0;
	    pt6->name = myapush(strlen(pt5) + 1);
	    if (!pt6->name) return 0;
	    strcpy(pt6->name,pt5);
	    pt6->list = (short int *)myapush(infodata.dsize);
	    if (!pt6->list) return 0;
	    bcopy(infodata.dptr,pt6->list,infodata.dsize);
	    if (infodata.dsize != (sizeof(short int) * (*(pt6->list) + 1))) {
              if ( Good_obj(player) ) {
	         err_out(42,Name(player),pt5);
              } else {
	         err_out(42,"(INVALID PLAYER)",pt5);
              }
	    } else {
	      pt6->next = pt1->box;
	      pt1->box = pt6;
	    }
	  }
	  break;
	default:
	  err_out(24,NULL,NULL);
      }
    }
    keydata = dbm_nextkey(foldfile);
  }
  return 1;
}

int setupall()
{
    char *name = NULL;
    int c;
    errcount = 0;    
    name = alloc_lbuf("mailfix_filename");
    if (!name) {
      err_out(27,NULL,NULL);
      return 0;
    }
    // All this because Ashen doesn't like sprintf :)
    strcpy(name, mudconf.data_dir);
    strcat(name, "/");
    c = strlen(name);

    strcat(name, "mail.fix"); 
    output1 = fopen(name, "w+");
    if (!output1) {
      err_out(25,NULL,NULL);
      return 0;
    }

    name[c] = '\0';

    strcat(name, "folder.fix");
    output2 = fopen(name, "w+");
    if (!output2) {
      err_out(26,NULL,NULL);
      fclose(output1);
      return 0;
    }

    free_lbuf(name);

    buf1 = myapush(NDBMBUFSZ);
    buf2 = myapush(NDBMBUFSZ);
    buf3 = myapush(NDBMBUFSZ);
    if (!buf1 || !buf2 || !buf3) {
      err_out(27,NULL,NULL);
      return 0;
    }
    allptr = NULL;
    allplay = NULL;
    smax = 0;
    alias = NULL;
    maccess = NULL;
    nukehsnd = NULL;
    nukemsg = NULL;
    nuketrack = NULL;
    absmaxinx = (NDBMBUFSZ / sizeof(short int)) - 3;
    absmaxrcp = (NDBMBUFSZ-sizeof(int)) / (sizeof(int) * 2);
    absmaxacs = NDBMBUFSZ / sizeof(int) - 1;
    hdlst = myapush(absmaxinx);
    if (!hdlst) return 0;
    mslst = myapush(absmaxinx);
    if (!mslst) return 0;
    htimeoff = sizeof(int);
    hindoff = htimeoff + sizeof(time_t);
    hsizeoff = hindoff + sizeof(short int);
    hstoff = hsizeoff + sizeof(short int);
    hsuboff = hstoff + 3;
    tomask = 1;
    tomask <<= ((sizeof(short int) << 3) - 1);
    return 1;
}

int ptrlst2_sch(PTRLST2 *pass1, short int chk)
{
  PTRLST2 *t1;

  t1 = pass1;
  while (t1) {
    if (t1->index == chk) return 1;
    t1 = t1->next;
  }
  return 0;
}

int verlen(char *st1, int tlen)
{
  char *t1, *t2;

  t1 = st1 + (tlen - 1);
  for (t2 = t1; (t2 > st1) && *t2; t2--) {};
  if (t2 == st1)
    return 0;
  else
    return ((int)(t2 - st1 + 1));
}

PTRLST2 *fixmsg1(PTRLST2 *pass1)
{
  PTRLST2 *pt1, *pt3, *pt4, *pt5, *pt6;
  PLYST *pt2;
  int lcheck;

  pt2 = allplay;
  while (pt2) {
    if (pass1)
      pt1 = pass1;
    else
      pt1 = pt2->msg;
    pt3 = NULL;
    while (pt1) {
      if (ptrlst2_sch(pt1->next,pt1->index)) {
        if ( Good_obj(pt2->player) ) {
	   err_out(43,Name(pt2->player),NULL);
        } else {
	   err_out(43,"(INVALID PLAYER)",NULL);
        }
	pt4 = NULL;
	pt5 = pt1->next;
	while (pt5) {
	  if (pt5->index != pt1->index) {
	    pt6 = pt5;
	    pt5 = pt5->next;
	    pt6->next = pt4;
	    pt4 = pt6;
	  }
	  else 
	    pt5 = pt5->next;
	}
	pt1 = pt4;
	continue;
      }
      lcheck = verlen(pt1->ptr,pt1->len);
      if (lcheck < pt1->len) {
	if (lcheck) {
          if ( Good_obj(pt2->player) ) {
	     err_out(44,Name(pt2->player),NULL);
          } else {
	     err_out(44,"(INVALID PLAYER)",NULL);
          }
	  pt1->len = lcheck;
	} else {
          if ( Good_obj(pt2->player) ) {
	     err_out(45,Name(pt2->player),NULL);
          } else {
	     err_out(45,"(INVALID PLAYER)",NULL);
          }
        }
      }
      if (lcheck > LBUF_SIZE) {
        if ( Good_obj(pt2->player) ) {
	   err_out(104,Name(pt2->player),NULL);
        } else {
	   err_out(104,"(INVALID PLAYER)",NULL);
        }
	pt1->len = LBUF_SIZE;
	*(pt1->ptr + (LBUF_SIZE - 1)) = '\0';
      }
      pt4 = pt1;
      pt1 = pt1->next;
      if (lcheck) {
        pt4->next = pt3;
        pt3 = pt4;
      }
    }
    if (pass1)
      return pt3;
    pt2->msg = pt3;
    pt2 = pt2->next;
  }
  return NULL;
}

int intlst_sch(INTLST *pass1, short int chk)
{
  INTLST *t1;

  t1 = pass1;
  while (t1) {
    if (t1->index == chk) return 1;
    t1 = t1->next;
  }
  return 0;
}

INTLST *fixsndhd1(INTLST *pass1)
{
  PLYST *pt1;
  INTLST *pt2, *pt3, *pt4, *pt5, *pt6;
  int *ipt1, *ipt2;
  int x, y, count;

  pt1 = allplay;
  while (pt1) {
    if (pass1)
      pt2 = pass1;
    else
      pt2 = pt1->hsnd;
    pt3 = NULL;
    while (pt2) {
      if (intlst_sch(pt2->next,pt2->index)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(46,Name(pt1->player),NULL);
        } else {
	   err_out(46,"(INVALID PLAYER)",NULL);
        }
	pt4 = NULL;
	pt5 = pt2->next;
	while (pt5) {
	  if (pt5->index != pt2->index) {
	    pt6 = pt5;
	    pt5 = pt5->next;
	    pt6->next = pt4;
	    pt4 = pt6;
	  }
	  else
	    pt5 = pt5->next;
	}
	pt2 = pt4;
	continue;
      }
      ipt1 = pt2->intlst;
      y = *ipt1++;
      ipt2 = (int *)buf1;
      count = 0;
      for (x = 0; x < y; x++, ipt1 += 2) {
	if (Useg_obj(*ipt1) && isPlayer(*ipt1)) {
	  count++;
	  *ipt2++ = *ipt1;
	  *ipt2++ = *(ipt1+1);
	}
      }
      if (count && (count < *(pt2->intlst))) {
	bcopy(buf1,pt2->intlst + 1,count * 2 * sizeof(int));
	*(pt2->intlst) = count;
      }
      pt4 = pt2;
      pt2 = pt2->next;
      if (count) {
	pt4->next = pt3;
	pt3 = pt4;
      } else {
        if ( Good_obj(pt1->player) ) {
	   err_out(47,Name(pt1->player),NULL);
        } else {
	   err_out(47,"(INVALID PLAYER)",NULL);
        }
      }
    }
    if (pass1) 
      return pt3;
    pt1->hsnd = pt3;
    pt1 = pt1->next;
  }
  return NULL;
}

void hsndmsgck(PTRLST2 **pass1, INTLST **pass2, char *pass3)
{
  PTRLST2 *pt1, *pt2, *pt5;
  INTLST *pt3, *pt4, *pt6, *pt7;

  pt2 = NULL;
  pt4 = NULL;
  pt1 = *pass1;
  pt6 = *pass2;
  while (pt1) {
    pt3 = pt6;
    pt6 = NULL;
    while (pt3) {
      if (pt3->index == pt1->index) {
	if (!pt6)
	  pt6 = pt3->next;
	else {
	  pt7 = pt6;
	  while (pt7->next)
	    pt7 = pt7->next;
	  pt7->next = pt3->next;
	}
	pt3->next = pt4;
	pt4 = pt3;
	pt5 = pt1;
	pt1 = pt1->next;
	pt5->next = pt2;
	pt2 = pt5;
	break;
      }
      else {
	pt7 = pt3;
	pt3 = pt3->next;
	pt7->next = pt6;
	pt6 = pt7;
      }
    }
    if (pt3) continue;
    err_out(52,pass3,NULL);
    pt1 = pt1->next;
  }
  while (pt6) {
    err_out(53,pass3,NULL);
    pt6 = pt6->next;
  }
  *pass1 = pt2;
  *pass2 = pt4;
}

void fixhsndmsg()
{
  PLYST *pt1;

  pt1 = allplay;
  while (pt1) {
    if (pt1->player >= 0)
      hsndmsgck(&(pt1->msg),&(pt1->hsnd),Name(pt1->player));
    pt1 = pt1->next;
  }
}

int nukeinddet()
{
  PTRLST2 *pt1;
  INTLST *pt2, *pt5;
  PLYST *pt3;
  NTRACK *pt4;
  int plynum, x;
  short int maxind;

  for (plynum = -1; plynum >= MAXNUKEPLY; plynum--) {
    pt3 = findplay(plynum);
    if (pt3)
      hsndmsgck(&(pt3->msg),&(pt3->hsnd),"(NUKED)");
  }
  pt1 = NULL;
  while (nukemsg) {
    pt2 = nukehsnd;
    nukehsnd = NULL;
    while (pt2) {
      if ((pt2->player == nukemsg->player) && (pt2->index == nukemsg->index)) {
	pt4 = (NTRACK *)myapush(sizeof(NTRACK));
	if (!pt4) return 0;
	pt4->msg = nukemsg;
	nukemsg = nukemsg->next;
	pt4->hsnd = pt2;
	pt4->next = nuketrack;
	nuketrack = pt4;
	if (!nukehsnd)
	  nukehsnd = pt2->next;
	else {
	  pt5 = nukehsnd;
	  while (pt5->next)
	    pt5 = pt5->next;
	  pt5->next = pt2->next;
	}
	break;
      }
      else {
	pt5 = pt2;
	pt2 = pt2->next;
	pt5->next = nukehsnd;
	nukehsnd = pt5;
      }
    }
    if (pt2) continue;
    err_out(54,"(NUKED)",NULL);
    nukemsg = nukemsg->next;
  }
  while (nukehsnd) {
    err_out(55,"(NUKED)",NULL);
    nukehsnd = nukehsnd->next;
  }
  if (nuketrack) {
    pt4 = nuketrack;
    bzero(mslst,absmaxinx);
    for (plynum = -1; plynum >= MAXNUKEPLY; plynum--) {
      pt3 = findplay(plynum);
      if (!pt3) break;
      pt1 = pt3->msg;
      while (pt1) {
        *(mslst+(pt1->index - 1)) = 1;
        pt1 = pt1->next;
      }
      for (x = 0; x < absmaxinx; x++) {
	if (!*(mslst+x)) {
	  err_out(141,NULL,NULL);
	  *(mslst+x) = 1;
	  pt4->newplay = plynum;
	  pt4->newind = (short int)(x + 1);
	  pt4 = pt4->next;
	  if (!pt4) break;
	}
      }
      if (!pt4) break;
    }
    maxind = 1;
    while (pt4 && (plynum >= MAXNUKEPLY)) {
      pt4->newplay = plynum;
      pt4->newind = maxind++;
      pt4 = pt4->next;
      if (maxind > absmaxinx) {
	plynum--;
	maxind = 1;
      }
    }
    while (pt4) {
      err_out(56,"(NUKED)",NULL);
      pt4->newplay = MAXNUKEPLY - 1;
      pt4->newind = 0;
      pt4 = pt4->next;
    }
  }
  return 1;
}

int fixrcvhd()
{
  PLYST *pt1 = NULL, 
        *pt6 = NULL;
  INTLST *pt2 = NULL;
  NTRACK *pt3 = NULL;
  PTRLST2 *pt4 = NULL, 
          *pt5 = NULL,
          *pt8 = NULL;
  int pcheck, pcheck2, x, y, *pt7;
  short int icheck, icheck2;
  char *pt9, toall;

  pt1 = allplay;
  while (pt1) {
    pt2 = pt1->hsnd;
    while (pt2) {
      pt2->used = myapush(absmaxrcp);
      if (!(pt2->used)) return 0;
      bzero(pt2->used,absmaxrcp);
      pt2 = pt2->next;
    }
    pt1 = pt1->next;
  }
  pt3 = nuketrack;
  while (pt3) {
    pt3->hsnd->used = myapush(absmaxrcp);
    if (!(pt3->hsnd->used)) return 0;
    pt3 = pt3->next;
  }
  pt1 = allplay;
  while (pt1) {
    pcheck2 = pt1->player;
    if (((pcheck2) >= 0) && Useg_obj(pcheck2))
      strncpy(buf3,Name(pcheck2),22);
    else
      strcpy(buf3,"(NUKED)");
    pt4 = NULL;
    pt5 = pt1->hrcv;
    while (pt5) {
      icheck2 = pt5->index;
      if ((icheck2 < 1) || (icheck2 > absmaxinx)) {
	err_out(59,buf3,NULL);
	pt5 = pt5->next;
	continue;
      }
      icheck = *(short int *)(pt5->ptr + hindoff);
      if (icheck & tomask) {
	toall = 1;
        icheck &= ~tomask;
      }
      else
	toall = 0;
      if ((icheck < 1) || (icheck > absmaxinx)) {
	err_out(57,buf3,NULL);
	pt5 = pt5->next;
	continue;
      }
      pcheck = *(int *)(pt5->ptr);
      if (((pcheck >= 0) && Useg_obj(pcheck) && isPlayer(pcheck)) || ((pcheck >= MAXNUKEPLY) && (pcheck < 0))) {
	pt6 = findplay(pcheck);
	if (!pt6) {
	  err_out(56,buf3,NULL);
	  pt5 = pt5->next;
	  continue;
	}
	pt2 = pt6->hsnd;
	while (pt2) {
	  if (pt2->index == icheck) break;
	  pt2 = pt2->next;
	}
	if (!pt2) {
	  err_out(58,buf3,NULL);
	  pt5 = pt5->next;
	  continue;
	}
	pt3 = NULL;
      }
      else {
	pt3 = nuketrack;
	while (pt3) {
	  if ((pt3->hsnd->player == pcheck) && (pt3->hsnd->index == icheck)) break;
	  pt3 = pt3->next;
	}
	if (!pt3) {
	  err_out(58,buf3,NULL);
	  pt5 = pt5->next;
	  continue;
	}
	else
	  pt2 = pt3->hsnd;
      }
      pt7 = pt2->intlst;
      y = *pt7++;
      for (x = 0; x < y; x++, pt7 += 2) 
	if ((pcheck2 == *pt7) && (icheck2 == (short int)(*(pt7+1)))) break;
      if (x == y) {
	err_out(60,buf3,NULL);
	if (y >= absmaxrcp)
	  err_out(61,buf3,NULL);
	else {
	  pt7 = (int *)myapush((((y + 1) * 2) + 1) * sizeof(int));
	  if (!pt7) return 0;
	  bcopy(pt2->intlst + 1, pt7+1, y * 2 * sizeof(int));
	  *pt7 = y + 1;
	  *(pt7 + 1 + (y * 2)) = pcheck2;
	  *(pt7 + 2 + (y * 2)) = y+1;
	  pt2->intlst = pt7;
	  *(pt2->used + y) = 1;
	}
      }
      else 
	*(pt2->used + x) = 1;
      if (pt3) {
	*(int *)(pt5->ptr) = pt3->newplay;
	if (toall)
	  *(short int *)(pt5->ptr + hindoff) = ((pt3->newind) | tomask);
	else
	  *(short int *)(pt5->ptr + hindoff) = pt3->newind;
	icheck2 = pt3->msg->len;
      }
      else {
	pt8 = pt6->msg;
	while (pt8) {
	  if (pt8->index == icheck) break;
	  pt8 = pt8->next;
	}
	if (!pt8) {
	  err_out(62,buf3,NULL);
	  pt5 = pt5->next;
	  continue;
	}
	icheck2 = pt8->len;
      }
      if (*(short int *)(pt5->ptr + hsizeoff) != (icheck2 - 1)) {
	err_out(63,buf3,NULL);
	*(short int *)(pt5->ptr + hsizeoff) = (icheck2 - 1);
      }
      pt9 = pt5->ptr + hstoff;
      if ((*(pt9 + 1) != 'N') && (*(pt9 + 1) != 'O') && (*(pt9 + 1) != 'U')) {
         if ((*(pt9 + 1) != 'n') && (*(pt9 + 1) != 'o') && (*(pt9 + 1) != 'u')) {
	   err_out(64,buf3,NULL);
	   *(pt9 + 1) = 'O';
	   *pt9 = 'O';
         }
      }
      if (*pt9 == 'P') {
	err_out(65,buf3,NULL);
	*pt9 = *(pt9 + 1);
      }
      else if ((*pt9 != 'N') && (*pt9 != 'O') && (*pt9 != 'M') && (*pt9 != 'U') && (*pt9 != 'S')) {
	err_out(66,buf3,NULL);
	*pt9 = *(pt9 + 1);
      }
      if ((*(pt9 + 2) != 'N') && (*(pt9 + 2) != 'F') && (*(pt9 + 2) != 'R') && (*(pt9 + 2) != 'A')) {
         if ((*(pt9 + 2) != 'n') && (*(pt9 + 2) != 'f') && (*(pt9 + 2) != 'r') && (*(pt9 + 2) != 'a')) {
	   err_out(67,buf3,NULL);
	   *(pt9 + 2) = 'N';
         }
      }
      pt8 = pt5;
      pt5 = pt5->next;
      pt8->next = pt4;
      pt4 = pt8;
    }
    pt1->hrcv = pt4;
    pt1 = pt1->next;
  }
  return 1;
}

INTLST *fixsndhd3(INTLST *pass1)
{
  int x, y, count, *pt1;
  char *pt2;

  count = 0;
  pt1 = pass1->intlst;
  y = *pt1++;
  pt2 = buf2 + sizeof(int);
  for (x = 0; x < y; x++, pt1 += 2) {
    if (*(pass1->used + x)) {
      bcopy(pt1,pt2,sizeof(int) << 1);
      count++;
      pt2 += sizeof(int) << 1;
    }
  }
  if (!count) return NULL;
  bcopy(buf2+sizeof(int),pass1->intlst + 1,count * (sizeof(int) << 1));
  *(pass1->intlst) = count;
  return pass1;
}

void fixsndhd2()
{
  PLYST *pt1;
  INTLST *pt2, *pt3, *pt4;

  pt1 = allplay;
  while (pt1) {
    if ((pt1->player >= 0) && Useg_obj(pt1->player))
      strncpy(buf3,Name(pt1->player),22);
    else
      strcpy(buf3,"(NUKED)");
    pt2 = pt1->hsnd;
    pt3 = NULL;
    while (pt2) {
      pt4 = fixsndhd3(pt2);
      pt2 = pt2->next;
      if (pt4) {
	pt4->next = pt3;
	pt3 = pt4;
      }
      else
	err_out(68,buf3,NULL);
    }
    pt1->hsnd = pt3;
    pt1 = pt1->next;
  }
}

void fixsndhdnk()
{
  NTRACK *pt1, *pt3, *pt4;
  INTLST *pt2;

  pt1 = nuketrack;
  pt3 = NULL;
  while (pt1) {
    pt2 = fixsndhd3(pt1->hsnd);
    if (pt2) {
      pt4 = pt1;
      pt1 = pt1->next;
      pt4->hsnd = pt2;
      pt4->next = pt3;
      pt3 = pt4;
    }
    else {
      err_out(68,"(PRENUKE)",NULL);
      pt1 = pt1->next;
    }
  }
  nuketrack = pt3;
}

int fixsndix()
{
  PLYST *pt1;
  INTLST *pt2, *pt5, *pt6, *pt7;
  short int *pt3, *pt4;
  short int top, nmsg, x, count, count2;

  pt1 = allplay;
  while (pt1) {
    if ((pt1->player >= 0) && Useg_obj(pt1->player))
      strncpy(buf3,Name(pt1->player),22);
    else
      strcpy(buf3,"(NUKED)");
    bzero(hdlst,absmaxinx);
    count = 0;
    top = 0;
    pt4 = (short int *)buf2;
    if (pt1->isnd) {
      pt3 = pt1->isnd;
      nmsg = *(pt3+2);
      pt3 += 3 + *(pt3+1);
      pt2 = pt1->hsnd;
      pt7 = NULL;
      for (x = 0; x < nmsg; x++, pt3++) {
	if ((*pt3 < 1) || (*pt3 > absmaxinx)) {
	  err_out(69,buf3,NULL);
	  continue;
	}
	if (*(hdlst + (*pt3 - 1))) {
	  err_out(70,buf3,NULL);
	  continue;
	}
        pt5 = NULL;
	while (pt2) {
	  if (pt2->index == *pt3) break;
	  pt6 = pt2;
	  pt2 = pt2->next;
	  pt6->next = pt5;
	  pt5 = pt6;
	}
	if (pt2) {
	  if (pt5) {
	    pt6 = pt5;
	    while (pt6->next)
	      pt6 = pt6->next;
	    pt6->next = pt2->next;
	  }
	  else
	    pt5 = pt2->next;
	  *(hdlst + (*pt3 - 1)) = 1;
	  pt2->next = pt7;
	  pt7 = pt2;
	  *pt4++ = *pt3;
	  count++;
	  if (*pt3 > top)
	    top = *pt3;
	}
	else
	  err_out(71,buf3,NULL);
	pt2 = pt5;
      }
      if (pt2) {
	while (pt2) {
	  err_out(73,buf3,NULL);
	  *(hdlst + (pt2->index - 1)) = 1;
	  pt6 = pt2;
	  pt2 = pt2->next;
	  pt6->next = pt7;
	  pt7 = pt6;
	  *pt4++ = pt7->index;
	  if (pt7->index > top)
	    top = pt7->index;
	  count++;
	}
      }
      if (!count) 
	err_out(72,buf3,NULL);
      else {
        top++;
	if (*(pt1->isnd) != top)
	  err_out(74,buf3,NULL);
	pt3 = pt1->isnd;
	nmsg = *(pt3 + 1);
	pt3 += 3;
	bzero(mslst,absmaxinx);
	for (x = 0; x < nmsg; x++, pt3++) {
	  if ((*pt3 < 1) || (*pt3 >= absmaxinx))
	    err_out(75,buf3,NULL);
	  else if ((*pt3 >= top) || (*(hdlst + (*pt3 - 1))))
	    err_out(76,buf3,NULL);
	  else if (*(mslst + (*pt3 - 1)))
	    err_out(77,buf3,NULL);
	  else
	    *(mslst + (*pt3 - 1)) = 1;
	}
      }
      pt1->hsnd = pt7;
    }
    else {
      pt2 = pt1->hsnd;
      while (pt2) {
	*pt4++ = pt2->index;
	if (pt2->index > top)
	  top = pt2->index;
	count++;
	*(hdlst + (pt2->index - 1)) = 1;
	pt2 = pt2->next;
      }
    }
    if (count) {
      pt4 = (short int *)buf1;
      count2 = 0;
      for (x = 0; x < top - 1; x++) {
	if (!*(hdlst + x)) {
	  *pt4++ = x+1;
	  count2++;
	}
      }
      if (!(pt1->isnd)) {
	pt1->isnd = (short int *)myapush((3 + count + count2) * sizeof(short int));
	if (!(pt1->isnd)) return 0;
      }
      else {
	pt3 = pt1->isnd;
	if ((*(pt3+1) + *(pt3+2)) < (count + count2)) {
	  pt1->isnd = (short int *)myapush((3 + count + count2) * sizeof(short int));
	  if (!(pt1->isnd)) return 0;
	}
      }
      pt3 = pt1->isnd;
      *pt3 = top;
      *(pt3 + 1) = count2;
      *(pt3 + 2) = count;
      pt3 += 3;
      pt4 = (short int *)buf1;
      for (x = 0; x < count2; x++)
	*pt3++ = *pt4++;
      pt4 = (short int *)buf2;
      for (x = 0; x < count; x++)
	*pt3++ = *pt4++;
    }
    else
      pt1->isnd = NULL;
    pt1 = pt1->next;
  }
  return 1;
}

int fixrcvix()
{
  PLYST *pt1;
  PTRLST2 *pt2, *pt5, *pt6, *pt7;
  short int *pt3, *pt4;
  short int top, nmsg, x, count, count2;

  pt1 = allplay;
  while (pt1) {
    if ((pt1->player >= 0) && Useg_obj(pt1->player))
       strncpy(buf3,Name(pt1->player),22);
    else
      strcpy(buf3,"(NUKED)");
    bzero(hdlst,absmaxinx);
    count = 0;
    top = 0;
    pt4 = (short int *)buf2;
    if (pt1->ircv) {
      pt3 = pt1->ircv;
      nmsg = *(pt3+2);
      pt3 += 3 + *(pt3+1);
      pt2 = pt1->hrcv;
      pt7 = NULL;
      for (x = 0; x < nmsg; x++, pt3++) {
	if ((*pt3 < 1) || (*pt3 > absmaxinx)) {
	  err_out(78,buf3,NULL);
	  continue;
	}
	if (*(hdlst + (*pt3 - 1))) {
	  err_out(79,buf3,NULL);
	  continue;
	}
        pt5 = NULL;
	while (pt2) {
	  if (pt2->index == *pt3) break;
	  pt6 = pt2;
	  pt2 = pt2->next;
	  pt6->next = pt5;
	  pt5 = pt6;
	}
	if (pt2) {
	  if (pt5) {
	    pt6 = pt5;
	    while (pt6->next)
	      pt6 = pt6->next;
	    pt6->next = pt2->next;
	  }
	  else
	    pt5 = pt2->next;
	  *(hdlst + (*pt3 - 1)) = 1;
	  pt2->next = pt7;
	  pt7 = pt2;
	  *pt4++ = *pt3;
	  count++;
	  if (*pt3 > top)
	    top = *pt3;
	}
	else
	  err_out(80,buf3,NULL);
	pt2 = pt5;
      }
      if (pt2) {
	while (pt2) {
	  err_out(82,buf3,NULL);
	  *(hdlst + (pt2->index - 1)) = 1;
	  pt6 = pt2;
	  pt2 = pt2->next;
	  pt6->next = pt7;
	  pt7 = pt6;
	  *pt4++ = pt7->index;
	  if (pt7->index > top)
	    top = pt7->index;
	  count++;
	}
      }
      if (!count) 
	err_out(81,buf3,NULL);
      else {
        top++;
	if (*(pt1->ircv) != top)
	  err_out(83,buf3,NULL);
	pt3 = pt1->ircv;
	nmsg = *(pt3 + 1);
	pt3 += 3;
	bzero(mslst,absmaxinx);
	for (x = 0; x < nmsg; x++, pt3++) {
	  if ((*pt3 < 1) || (*pt3 >= absmaxinx))
	    err_out(84,buf3,NULL);
	  else if ((*pt3 >= top) || (*(hdlst + (*pt3 - 1))))
	    err_out(85,buf3,NULL);
	  else if (*(mslst + (*pt3 - 1)))
	    err_out(86,buf3,NULL);
	  else
	    *(mslst + (*pt3 - 1)) = 1;
	}
      }
      pt1->hrcv = pt7;
    }
    else {
      pt2 = pt1->hrcv;
      while (pt2) {
	*pt4++ = pt2->index;
	if (pt2->index > top)
	  top = pt2->index;
	count++;
	*(hdlst + (pt2->index - 1)) = 1;
	pt2 = pt2->next;
      }
    }
    if (count) {
      pt4 = (short int *)buf1;
      count2 = 0;
      for (x = 0; x < top - 1; x++) {
	if (!*(hdlst + x)) {
	  *pt4++ = x+1;
	  count2++;
	}
      }
      if (!(pt1->ircv)) {
	pt1->ircv = (short int *)myapush((3 + count + count2) * sizeof(short int));
	if (!(pt1->ircv)) return 0;
      }
      else {
	pt3 = pt1->ircv;
	if ((*(pt3+1) + *(pt3+2)) < (count + count2)) {
	  pt1->ircv = (short int *)myapush((3 + count + count2) * sizeof(short int));
	  if (!(pt1->ircv)) return 0;
	}
      }
      pt3 = pt1->ircv;
      *pt3 = top;
      *(pt3 + 1) = count2;
      *(pt3 + 2) = count;
      pt3 += 3;
      pt4 = (short int *)buf1;
      for (x = 0; x < count2; x++)
	*pt3++ = *pt4++;
      pt4 = (short int *)buf2;
      for (x = 0; x < count; x++)
	*pt3++ = *pt4++;
    }
    else
      pt1->ircv = NULL;
    pt1 = pt1->next;
  }
  return 1;
}

int tacnuke()
{
  NTRACK *pt1;
  short int x, *pt2, /* *pt4, */ count, top, count2, nmsg, nfree;
  PLYST *pt3;
  int y, change;

  for (y = -1; y >= MAXNUKEPLY; y--) {
    pt3 = findplay(y);
    pt1 = nuketrack;
    pt2 = (short int *)buf2;
    if (pt3 && pt3->isnd) {
      top = *(pt3->isnd) - 1;
      count = *(pt3->isnd + 2);
      nmsg = count;
      nfree = *(pt3->isnd + 1);
      bcopy(pt3->isnd + 3 + nfree,buf2,count * sizeof(short int));
      pt2 += count;
    }
    else {
      top = 0;
      count = 0;
      nmsg = 0;
      nfree = 0;
    }
    change = 0;
    while (pt1) {
      if (pt1->newplay == y) {
	if (!pt3) {
	  pt3 = (PLYST *)myapush(sizeof(PLYST));
	  if (!pt3) return 0;
	  initplay(pt3);
	  pt3->player = y;
	  pt3->next = allplay;
	  allplay = pt3;
	}
	if (pt1->newind > top)
	  top = pt1->newind;
	*pt2++ = pt1->newind;
	count++;
	change = 1;
      }
      pt1 = pt1->next;
    }
    if (change) {
      bzero(hdlst,absmaxinx);
      count2 = 0;
      pt2 = (short int *)buf2;
      for (x = 0; x < count; x++, pt2++)
	*(hdlst + (*pt2 - 1)) = 1;
      for (x = 0; x < top - 1; x++) {
	if (!*(hdlst + x) && !(hdlst + x)) {
/*	  *pt4++ = x+1; */
	  count2++;
	}
      }
      if ((count + count2) > (nmsg + nfree)) {
	pt3->isnd = (short int *)myapush((3 + count + count2) * sizeof(short int));
	if (!pt3->isnd) return 0;
      }
      pt2 = pt3->isnd;
      *pt2++ = top+1;
      *pt2++ = count2;
      *pt2++ = count;
      if (count2) {
	bcopy(buf1,pt2,count2 * sizeof(short int));
	pt2 += count2;
      }
      bcopy(buf2,pt2,count * sizeof(short int));
    }
  }
  return 1;
}

void fixwrt()
{
  PLYST *pt1;
  PTRLST2 *pt2, *pt3, *pt4;
  int count, x, lcheck, imax;
  short int ilst[MAXWRTLN];

  pt1 = allplay;
  while (pt1) {
    if (pt1->player < 0) {
      if (pt1->wrtm) {
	err_out(87,NULL,NULL);
	pt1->wrtm = 0;
      }
      if (pt1->wrts || pt1->wrtlen) {
	err_out(88,NULL,NULL);
	pt1->wrts = NULL;
	pt1->wrtlen = 0;
      }
      if (pt1->wrtl) {
	err_out(89,NULL,NULL);
	pt1->wrtl = NULL;
      }
    }
    else if (!(pt1->wrtm)) {
      if (pt1->wrts || pt1->wrtlen) {
        if ( Good_obj(pt1->player) ) {
	   err_out(90,Name(pt1->player),NULL);
        } else {
	   err_out(90,"(INVALID PLAYER)",NULL);
        }
	pt1->wrtlen = 0;
	pt1->wrts = NULL;
      }
      if (pt1->wrtl) {
        if ( Good_obj(pt1->player) ) {
	   err_out(91,Name(pt1->player),NULL);
        } else {
	   err_out(91,"(INVALID PLAYER)",NULL);
        }
	pt1->wrtl = NULL;
      }
    }
    else if (pt1->wrtm < 0) {
      if ( Good_obj(pt1->player) ) {
         err_out(92,Name(pt1->player),NULL);
      } else {
         err_out(92,"(INVALID PLAYER)",NULL);
      }
      pt1->wrtm = 0;
      pt1->wrtl = NULL;
      pt1->wrts = NULL;
      pt1->wrtlen = 0;
    }
    else {
      pt3 = NULL;
      pt2 = pt1->wrtl;
      for (count = 0; count < MAXWRTLN; count++)
	ilst[count] = 0;
      count = 0;
      imax = 0;
      while (pt2) {
	if (pt2->index <= pt1->wrtm) {
	  if (ilst[pt2->index - 1]) {
            if ( Good_obj(pt1->player) ) {
	       err_out(94,Name(pt1->player),NULL);
            } else {
	       err_out(94,"(INVALID PLAYER)",NULL);
            }
	    pt2 = pt2->next;
	  }
	  else if (pt2->len < 2) {
            if ( Good_obj(pt1->player) ) {
	       err_out(95,Name(pt1->player),NULL);
            } else {
	       err_out(95,"(INVALID PLAYER)",NULL);
            }
	    pt2 = pt2->next;
	  }
	  else {
	    lcheck = verlen(pt2->ptr,pt2->len);
	    if (lcheck != pt2->len) {
              if ( Good_obj(pt1->player) ) {
	         err_out(96,Name(pt1->player),NULL);
              } else {
	         err_out(96,"(INVALID PLAYER)",NULL);
              }
	      pt2->len = lcheck;
	    }
	    if ((pt2->len < 2) || (pt2->len > LBUF_SIZE)) {
              if ( Good_obj(pt1->player) ) {
	         err_out(95,Name(pt1->player),NULL);
              } else {
	         err_out(95,"(INVALID PLAYER)",NULL);
              }
	      pt2 = pt2->next;
	    }
	    else {
	      ilst[pt2->index - 1] = 1;
	      if (pt2->index > imax)
		imax = pt2->index;
	      pt4 = pt2;
	      pt2 = pt2->next;
	      pt4->next = pt3;
	      pt3 = pt4;
	      count++;
	    }
	  }
	} else {
          if ( Good_obj(pt1->player) ) {
	     err_out(93,Name(pt1->player),NULL);
          } else {
	     err_out(93,"(INVALID PLAYER)",NULL);
          }
	  pt2 = pt2->next;
	}
      }
      pt1->wrtl = pt3;
      if (pt1->wrtm != count) {
        if ( Good_obj(pt1->player) ) {
	   err_out(139,Name(pt1->player),NULL);
        } else {
	   err_out(139,"(INVALID PLAYER)",NULL);
        }
        pt1->wrtm = count;
      }
      count = 0;
      for (x = 0; x < imax; x++) {
	if (!(ilst[x]))
	  count--;
	else
	  ilst[x] = count;
      }
      pt2 = pt1->wrtl;
      while (pt2) {
	if (ilst[pt2->index - 1]) {
	  pt2->index += ilst[pt2->index - 1];
          if ( Good_obj(pt1->player) ) {
	     err_out(140,Name(pt1->player),NULL);
          } else {
	     err_out(140,"(INVALID PLAYER)",NULL);
          }
	}
	pt2 = pt2->next;
      }
      if (pt1->wrts) {
	lcheck = verlen(pt1->wrts,pt1->wrtlen);
	if (lcheck != pt1->wrtlen) {
          if ( Good_obj(pt1->player) ) {
	     err_out(97,Name(pt1->player),NULL);
          } else {
	     err_out(97,"(INVALID PLAYER)",NULL);
          }
	  pt1->wrtlen = lcheck;
	}
	if ((pt1->wrtlen < 2) || (pt1->wrtlen > LBUF_SIZE)) {
          if ( Good_obj(pt1->player) ) {
	     err_out(98,Name(pt1->player),NULL);
          } else {
	     err_out(98,"(INVALID PLAYER)",NULL);
          }
	  pt1->wrtlen = 0;
	  pt1->wrts = NULL;
	}
      } else if (pt1->wrtlen) {
        if ( Good_obj(pt1->player) ) {
	   err_out(97,Name(pt1->player),NULL);
        } else {
	   err_out(97,"(INVALID PLAYER)",NULL);
        }
      }
    }
    pt1 = pt1->next;
  }
}

void fixother()
{
  PLYST *pt1;
  int lcheck;

  pt1 = allplay;
  while (pt1) {
    if (pt1->player < 0) {
      pt1 = pt1->next;
      continue;
    }
    if (pt1->bsize) {
      if ((pt1->bsize < BSMIN) || (pt1->bsize > BSMAX)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(99,Name(pt1->player),NULL);
        } else {
	   err_out(99,"(INVALID PLAYER)",NULL);
        }
	pt1->bsize = 0;
      }
    }
    if ((pt1->rejm) && (pt1->rejlen < 2)) {
      if ( Good_obj(pt1->player) ) {
         err_out(100,Name(pt1->player),NULL);
      } else {
         err_out(100,"(INVALID PLAYER)",NULL);
      }
      pt1->rejm = NULL;
      pt1->rejlen = 0;
    }
    else if ((pt1->rejlen > 1) && !(pt1->rejm)) {
      if ( Good_obj(pt1->player) ) {
         err_out(101,Name(pt1->player),NULL);
      } else {
         err_out(101,"(INVALID PLAYER)",NULL);
      }
      pt1->rejlen = 0;
    }
    else if (pt1->rejm) {
      lcheck = verlen(pt1->rejm,pt1->rejlen);
      if (lcheck != pt1->rejlen) {
        if ( Good_obj(pt1->player) ) {
           err_out(102,Name(pt1->player),NULL);
        } else {
           err_out(102,"(INVALID PLAYER)",NULL);
        }
        pt1->rejlen = lcheck;
      }
      if ((lcheck < 2)  || (lcheck > LBUF_SIZE)) {
	pt1->rejm = NULL;
        if ( Good_obj(pt1->player) ) {
	   err_out(103,Name(pt1->player),NULL);
        } else {
	   err_out(103,"(INVALID PLAYER)",NULL);
        }
      }
    }
    if (pt1->page) {
      if ((pt1->page < 1) || (pt1->page > BSMAX)) {
        if ( Good_obj(pt1->player) ) {
           err_out(105,Name(pt1->player),NULL);
        } else {
           err_out(105,"(INVALID PLAYER)",NULL);
        }
	pt1->page = 0;
      }
    }
    if (pt1->afor != NOTHING) {
      if (!Useg_obj(pt1->afor) || !isPlayer(pt1->afor)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(106,Name(pt1->player),NULL);
        } else {
	   err_out(106,"(INVALID PLAYER)",NULL);
        }
	pt1->afor = NOTHING;
      }
    }
    pt1 = pt1->next;
  }
}

int checkfold(char *pass1, int inc)
{
  char *pt1;

  if ((!*pass1) || (strlen(pass1) > 16))
    return 0;
  pt1 = pass1;
  if (((*pt1 < 'A') || (*pt1 > 'Z')) && ((*pt1 < '0') || (*pt1 > '9')))
    return 0;
  pt1++;
  while (*pt1) {
    if (((*pt1 < 'a') || (*pt1 > 'z')) && ((*pt1 < '0') || (*pt1 > '9')))
      return 0;
    pt1++;
  }
  if (inc) {
    if (!strcmp(pass1,"Incoming"))
      return 0;
  }
  return 1;
}

int fixfolder()
{
  PLYST *pt1;
  BLIST *pt2, *pt3, *pt6, *pt9;
  char *pt4, *pt5;
  short int lcheck, *pt7, *pt8, count, x, top;
  int test;

  pt1 = allplay;
  while (pt1) {
    if (pt1->player < 0) {
      pt1 = pt1->next;
      continue;
    }
    if (pt1->flist && ((pt1->fllen < 2) || (pt1->fllen > NDBMBUFSZ))) {
      if ( Good_obj(pt1->player) ) {
         err_out(107,Name(pt1->player),NULL);
      } else {
         err_out(107,"(INVALID PLAYER)",NULL);
      }
      pt1->flist = NULL;
      pt1->fllen = 0;
    }
    else if (pt1->fllen && !pt1->flist) {
      if ( Good_obj(pt1->player) ) {
         err_out(108,Name(pt1->player),NULL);
      } else {
         err_out(108,"(INVALID PLAYER)",NULL);
      }
      pt1->fllen = 0;
    }
    else if (pt1->flist) {
      lcheck = verlen(pt1->flist,pt1->fllen);
      if (lcheck != pt1->fllen) {
        if ( Good_obj(pt1->player) ) {
	   err_out(109,Name(pt1->player),NULL);
        } else {
	   err_out(109,"(INVALID PLAYER)",NULL);
        }
	pt1->fllen = lcheck;
      }
      if (lcheck < 2) {
        if ( Good_obj(pt1->player) ) {
	   err_out(110,Name(pt1->player),NULL);
        } else {
	   err_out(110,"(INVALID PLAYER)",NULL);
        }
	pt1->fllen = 0;
	pt1->flist = NULL;
      }
    }
    if (pt1->cshare && ((pt1->cslen < 2) || (pt1->cslen > NDBMBUFSZ))) {
      if ( Good_obj(pt1->player) ) {
         err_out(111,Name(pt1->player),NULL);
      } else {
         err_out(111,"(INVALID PLAYER)",NULL);
      }
      pt1->cshare = NULL;
      pt1->cslen = 0;
    } else if (pt1->cslen && !pt1->cshare) {
      if ( Good_obj(pt1->player) ) {
         err_out(112,Name(pt1->player),NULL);
      } else {
         err_out(112,"(INVALID PLAYER)",NULL);
      }
      pt1->cslen = 0;
    }
    else if (pt1->cshare) {
      lcheck = verlen(pt1->cshare,pt1->cslen);
      if (lcheck != pt1->cslen) {
        if ( Good_obj(pt1->player) ) {
	   err_out(113,Name(pt1->player),NULL);
        } else {
	   err_out(113,"(INVALID PLAYER)",NULL);
        }
	pt1->cslen = lcheck;
      }
      if (lcheck < 2) {
        if ( Good_obj(pt1->player) ) {
	   err_out(114,Name(pt1->player),NULL);
        } else {
	   err_out(114,"(INVALID PLAYER)",NULL);
        }
	pt1->cslen = 0;
	pt1->cshare = NULL;
      }
    }
    if (pt1->curr && ((pt1->culen < 2) || (pt1->culen > NDBMBUFSZ))) {
      if ( Good_obj(pt1->player) ) {
         err_out(115,Name(pt1->player),NULL);
      } else {
         err_out(115,"(INVALID PLAYER)",NULL);
      }
      pt1->curr = NULL;
      pt1->culen = 0;
    }
    else if (pt1->culen && !pt1->curr) {
      if ( Good_obj(pt1->player) ) {
         err_out(116,Name(pt1->player),NULL);
      } else {
         err_out(116,"(INVALID PLAYER)",NULL);
      }
      pt1->culen = 0;
    } else if (pt1->culen) {
      lcheck = verlen(pt1->curr,pt1->culen);
      if (lcheck != pt1->culen) {
        if ( Good_obj(pt1->player) ) {
	   err_out(117,Name(pt1->player),NULL);
        } else {
	   err_out(117,"(INVALID PLAYER)",NULL);
        }
	pt1->culen = lcheck;
      }
      if (lcheck < 2) {
        if ( Good_obj(pt1->player) ) {
	   err_out(118,Name(pt1->player),NULL);
        } else {
	   err_out(118,"(INVALID PLAYER)",NULL);
        }
	pt1->culen = 0;
	pt1->curr = NULL;
      }
    }
    strcpy(buf2,"Incoming");
    if (pt1->flist) {
      strcpy(buf1,pt1->flist);
      pt4 = buf1;
      test = 0;
      while (*pt4 && !test) {
        while (*pt4 && isspace((int)*pt4)) pt4++;
        if (!*pt4) break;
        pt5 = pt4+1;
        while (*pt5 && !isspace((int)*pt5)) pt5++;
        if (!*pt5)
  	  test = 1;
        else
	  *pt5 = '\0';
        if (checkfold(pt4,1)) {
	  strcat(buf2," ");
	  strcat(buf2,pt4);
        } else {
          if ( Good_obj(pt1->player) ) {
	     err_out(119,Name(pt1->player),NULL);
          } else {
	     err_out(119,"(INVALID PLAYER)",NULL);
          }
        }
        if (!test)
	  pt4 = pt5+1;
      }
      pt4 = buf2;
      while (*pt4 && (*pt4 != ' ')) pt4++;
      if (*pt4) {
	strcpy(pt1->flist,pt4+1);
	pt1->fllen = strlen(pt4+1) + 1;
      }
    }
    if (pt1->cshare) {
      if (!strstr2(buf2,pt1->cshare)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(120,Name(pt1->player),NULL);
        } else {
	   err_out(120,"(INVALID PLAYER)",NULL);
        }
	pt1->cshare = NULL;
	pt1->cslen = 0;
      }
    }
    if (pt1->curr) {
      if (!strstr2(buf2,pt1->curr)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(121,Name(pt1->player),NULL);
        } else {
	   err_out(121,"(INVALID PLAYER)",NULL);
        }
	pt1->curr = NULL;
	pt1->culen = 0;
      }
    }
    bzero(mslst,absmaxinx);
    if (pt1->ircv) {
      pt8 = pt1->ircv;
      top = *pt8 - 1;
      count = *(pt8+2);
      pt8 += *(pt8+1) + 3;
      for (x = 0; x < count; x++, pt8++)
	*(mslst + (*pt8 - 1)) = 1;
    }
    else
      top = 0;
    pt3 = NULL;
    pt9 = NULL;
    pt2 = pt1->box;
    bzero(hdlst,absmaxinx);
    while (pt2) {
      count = 0;
      pt7 = (short int *)buf3;
      if ((!checkfold(pt2->name,0)) || (!strstr2(buf2,pt2->name))) {
        if ( Good_obj(pt1->player) ) {
	   err_out(122,Name(pt1->player),NULL);
        } else {
	   err_out(122,"(INVALID PLAYER)",NULL);
        }
	pt2 = pt2->next;
      }
      else {
	if (!strcmp(pt2->name,"Incoming"))
	  pt9 = pt2;
	pt8 = pt2->list;
	lcheck = *pt8++;
	for (x = 0; x < lcheck; x++, pt8++) {
	  if ((*pt8 < 1) || (*pt8 > absmaxinx)) {
            if ( Good_obj(pt1->player) ) {
	       err_out(123,Name(pt1->player),pt2->name);
            } else {
	       err_out(123,"(INVALID PLAYER)",pt2->name);
            }
	  } else if (*(hdlst + (*pt8 - 1))) {
            if ( Good_obj(pt1->player) ) {
	       err_out(124,Name(pt1->player),pt2->name);
            } else {
	       err_out(124,"(INVALID PLAYER)",pt2->name);
            }
	  } else if (!*(mslst + (*pt8 - 1))) {
            if ( Good_obj(pt1->player) ) {
	       err_out(125,Name(pt1->player),pt2->name);
            } else {
	       err_out(125,"(INVALID PLAYER)",pt2->name);
            }
	  } else {
	    *(hdlst + (*pt8 - 1)) = 1;
	    *(mslst + (*pt8 - 1)) = 0;
	    *pt7++ = *pt8;
	    count++;
	  }
	}
	if (count) {
	  if (count != *(pt2->list)) {
	    *(pt2->list) = count;
	    bcopy(buf3,pt2->list+1,count * sizeof(short int));
	  }
	  pt6 = pt2;
	  pt2 = pt2->next;
	  pt6->next = pt3;
	  pt3 = pt6;
	}
	else
	  pt2 = pt2->next;
      }
    }
    pt1->box = pt3;
    count = 0;
    pt7 = (short int *)buf1;
    for (x = 0; x < top; x++) {
      if (*(mslst + x)) {
        if ( Good_obj(pt1->player) ) {
	   err_out(126,Name(pt1->player),NULL);
        } else {
	   err_out(126,"(INVALID PLAYER)",NULL);
        }
	count++;
	*pt7++ = x+1;
      }
    }
    if (count) {
      if (!pt9) {
	pt9 = (BLIST *)myapush(sizeof(BLIST));
	if (!pt9) return 0;
	pt9->list = NULL;
	pt9->name = myapush(9);
	if (!pt9->name) return 0;
	strcpy(pt9->name,"Incoming");
	pt9->next = pt1->box;
	pt1->box = pt9;
      }
      if (pt9->list) {
	pt8 = pt9->list;
	lcheck = *pt8++;
	for (x = 0; x < lcheck; x++, pt8++, pt7++)
	  *pt7 = *pt8;
	count += lcheck;
      }
      pt9->list = (short int *)myapush((count + 1) * sizeof(short int));
      if (!pt9->list) return 0;
      pt7 = pt9->list;
      *pt7++ = count;
      bcopy(buf1,pt7,count * sizeof(short int));
    }
    pt1 = pt1->next;
  }
  return 1;
}

int fixplay()
{
  fixmsg1(NULL);
  if (nukemsg)
    nukemsg = fixmsg1(nukemsg);
  fixsndhd1(NULL);
  if (nukehsnd)
    nukehsnd = fixsndhd1(nukehsnd);
  if (!nukeinddet()) {fprintf(stderr, "MAIL: nukeinddet\n");return 0;}
  fixhsndmsg();
  if (!fixrcvhd()) {fprintf(stderr, "MAIL: fixrcvhd\n");return 0;}
  fixsndhd2();
  fixsndhdnk();
  fixhsndmsg();
  if (!fixsndix()) {fprintf(stderr, "MAIL: fixsndix\n");return 0;}
  if (!fixrcvix()) {fprintf(stderr, "MAIL: fixrcvix\n");return 0;}
  if (!tacnuke()) {fprintf(stderr, "MAIL: tacnuke\n");return 0;}
  fixwrt();
  fixother();
  if (!fixfolder()) {fprintf(stderr, "MAIL: fixfolder\n");return 0;}
  return 1;
}

void fixacs()
{
  int num, count, *pt1, *pt2, x;

  if (maccess) {
    num = *maccess;
    if (num < 1) {
      err_out(128,NULL,NULL);
      maccess = NULL;
    }
    else {
      pt2 = (int *)buf1;
      pt1 = maccess + 1;
      count = 0;
      for (x = 0; x < num; x++, pt1++) {
	if (!Useg_obj(*pt1) || !isPlayer(*pt1))
	  err_out(129,NULL,NULL);
	else {
	  *pt2++ = *pt1;
	  count++;
	}
      }
      if (count && (count != num)) {
	*maccess = count;
	bcopy(buf1,maccess+1,count * sizeof(int));
      }
      else if (!count) {
	err_out(130,NULL,NULL);
	maccess = NULL;
      }
    }
  }
}

void fixgal()
{
  int num, count, *pt1, *pt2, x;
  GASTRUCT *pt4, *pt5, *pt6;
  short int lcheck;

  pt4 = alias;
  pt5 = NULL;
  while (pt4) {
    if (strlen(pt4->name) > 16) {
      err_out(131,NULL,NULL);
      pt4 = pt4->next;
      continue;
    }
    if ((pt4->lock) && ((pt4->locklen) < 2)) {
      err_out(132,pt4->name,NULL);
      pt4 = pt4->next;
      continue;
    }
    if (!(pt4->lock) && (pt4->locklen)) {
      err_out(133,pt4->name,NULL);
      pt4 = pt4->next;
      continue;
    }
    if (pt4->lock) {
      lcheck = verlen(pt4->lock,pt4->locklen);
      if (lcheck != pt4->locklen) {
        err_out(134,pt4->name,NULL);
        pt4 = pt4->next;
        continue;
      }
    }
    else if (pt4->locklen)
      err_out(134,pt4->name,NULL);
    if (!(pt4->list)) {
      err_out(135,pt4->name,NULL);
      pt4 = pt4->next;
      continue;
    }
    num = *(pt4->list);
    if (num < 1) {
      err_out(136,pt4->name,NULL);
      pt4->list = NULL;
    }
    else {
      pt2 = (int *)buf1;
      pt1 = pt4->list + 1;
      count = 0;
      for (x = 0; x < num; x++, pt1++) {
	if (!Useg_obj(*pt1) || !isPlayer(*pt1))
	  err_out(137,pt4->name,NULL);
	else {
	  *pt2++ = *pt1;
	  count++;
	}
      }
      if (count && (count != num)) {
	*(pt4->list) = count;
	bcopy(buf1,pt4->list+1,count * sizeof(int));
      }
      else if (!count) {
	err_out(138,pt4->name,NULL);
	pt4->list = NULL;
      }
    }
    if (pt4->list) {
      pt6 = pt4;
      pt4 = pt4->next;
      pt6->next = pt5;
      pt5 = pt6;
    }
    else
      pt4 = pt4->next;
  }
  alias = pt5;
}

void fixglobal()
{
  if (smax) {
    if ((smax < 1) || (smax > absmaxinx)) {
      err_out(127,NULL,NULL);
      smax = 0;
    }
  }
  fixacs();
  fixgal();
}

void writesind(short int *list)
{
  short int x, y;

  y = 3 + *(list + 1) + *(list + 2);
  for (x = 0; x < y; x++) {
    fprintf(output1,"%d",*(list+x));
    if (x < y - 1)
      fputc('/',output1);
  }
  fputs("\1\n",output1);
}

void writeind(int *list, int count)
{
  int x;

  for (x = 0; x < count; x++) {
    fprintf(output1,"%d",*(list+x));
    if (x < count - 1)
      fputc('/',output1);
  }
  fputs("\1\n",output1);
}

void procout()
{
  PLYST *pt1;
  PTRLST2 *pt2;
  INTLST *pt3;
  BLIST *pt4;
  NTRACK *pt5;
  GASTRUCT *pt6;
  short int count, x;

  pt1 = allplay;
  while (pt1) {
    if (pt1->ircv) {
      fprintf(output1,"A%d\1\n",pt1->player);
      writesind(pt1->ircv);
    }
    if (pt1->isnd) {
      fprintf(output1,"B%d\1\n",pt1->player);
      writesind(pt1->isnd);
    }
    pt2 = pt1->hrcv;
    while (pt2) {
      fprintf(output1,"C%d/%d\1\n",pt1->player,pt2->index);
      if (pt2->len == hsuboff)
	fprintf(output1,"%d/%ld/%d/%d/%c%c%c\1\n",*(int *)(pt2->ptr),
		*(time_t *)(pt2->ptr + htimeoff), *(short int *)(pt2->ptr + hindoff),
		*(short int *)(pt2->ptr + hsizeoff), *(pt2->ptr + hstoff),
		*(pt2->ptr + hstoff + 1), *(pt2->ptr + hstoff + 2));
      else
	fprintf(output1,"%d/%ld/%d/%d/%c%c%c%s\1\n",*(int *)(pt2->ptr),
		*(time_t *)(pt2->ptr + htimeoff), *(short int *)(pt2->ptr + hindoff),
		*(short int *)(pt2->ptr + hsizeoff), *(pt2->ptr + hstoff),
		*(pt2->ptr + hstoff + 1), *(pt2->ptr + hstoff + 2),
		pt2->ptr + hsuboff);
      pt2 = pt2->next;
    }
    pt3 = pt1->hsnd;
    while (pt3) {
      fprintf(output1,"D%d/%d\1\n",pt1->player,pt3->index);
      writeind(pt3->intlst,(*(pt3->intlst) * 2) + 1);
      pt3 = pt3->next;
    }
    pt2 = pt1->msg;
    while (pt2) {
      fprintf(output1,"E%d/%d\1\n",pt1->player,pt2->index);
      fputs(pt2->ptr,output1);
      fputs("\1\n",output1);
      pt2 = pt2->next;
    }
    if (pt1->bsize) {
      fprintf(output1,"F%d\1\n",pt1->player);
      fprintf(output1,"%d\1\n",pt1->bsize);
    }
    if (pt1->rejm) {
      fprintf(output1,"G%d\1\n",pt1->player);
      fputs(pt1->rejm,output1);
      fputs("\1\n",output1);
    }
    if (pt1->wrtm) {
      fprintf(output1,"H%d\1\n",pt1->player);
      fprintf(output1,"%d\1\n",pt1->wrtm);
    }
    pt2 = pt1->wrtl;
    while (pt2) {
      fprintf(output1,"I%d/%d\1\n",pt1->player,pt2->index);
      fputs(pt2->ptr,output1);
      fputs("\1\n",output1);
      pt2 = pt2->next;
    }
    if (pt1->wrts) {
      fprintf(output1,"J%d\1\n",pt1->player);
      fputs(pt1->wrts,output1);
      fputs("\1\n",output1);
    }
    if (pt1->page) {
      fprintf(output1,"N%d\1\n",pt1->player);
      fprintf(output1,"%d\1\n",pt1->page);
    }
    if (pt1->afor != NOTHING) {
      fprintf(output1,"O%d\1\n",pt1->player);
      fprintf(output1,"%d\1\n",pt1->afor);
    }
    if (pt1->flist) {
      fprintf(output2,"A%d\1\n",pt1->player);
      fputs(pt1->flist,output2);
      fputs("\1\n",output2);
    }
    pt4 = pt1->box;
    while (pt4) {
      fprintf(output2,"B%d/%s\1\n",pt1->player,pt4->name);
      count = *(pt4->list) + 1;
      for (x = 0; x < count; x++) {
	fprintf(output2,"%d",*(pt4->list + x));
	if (x < count - 1)
	  fputc('/',output2);
      }
      fputs("\1\n",output2);
      pt4 = pt4->next;
    }
    if (pt1->cshare) {
      fprintf(output2,"D%d\1\n",pt1->player);
      fputs(pt1->cshare,output2);
      fputs("\1\n",output2);
    }
    if (pt1->curr) {
      fprintf(output2,"C%d\1\n",pt1->player);
      fputs(pt1->curr,output2);
      fputs("\1\n",output2);
    }
    pt1 = pt1->next;
  }
  pt5 = nuketrack;
  while (pt5) {
    fprintf(output1,"D%d/%d\1\n",pt5->newplay,pt5->newind);
    writeind(pt5->hsnd->intlst,(*(pt5->hsnd->intlst) * 2) + 1);
    fprintf(output1,"E%d/%d\1\n",pt5->newplay,pt5->newind);
    fputs(pt5->msg->ptr,output1);
    fputs("\1\n",output1);
    pt5 = pt5->next;
  }
  if (smax) {
    fputs("P\1\n",output1);
    fprintf(output1,"%d\1\n",smax);
  }
  if (maccess) {
    fputs("M\1\n",output1);
    writeind(maccess,*maccess + 1);
  }
  pt6 = alias;
  while (pt6) {
    fprintf(output1,"K%s\1\n",pt6->name);
    writeind(pt6->list,*(pt6->list) + 1);
    if (pt6->lock) {
      fprintf(output1,"L%s\1\n",pt6->name);
      fputs(pt6->lock,output1);
      fputs("\1\n",output1);
    }
    pt6 = pt6->next;
  }
}

#ifdef STANDALONE
int 
openmaildb(char *prefix)
{
    char nbuf[100];

    strcpy(nbuf, prefix);
    strcat(nbuf, ".mail");
    mailfile = dbm_open(nbuf, O_RDWR | O_CREAT, 00664);
    strcpy(nbuf, prefix);
    strcat(nbuf, ".folder");
    foldfile = dbm_open(nbuf, O_RDWR | O_CREAT, 00664);
    if (!mailfile || !foldfile)
	return -1;
    else
	return 0;
}

void 
main(int argc, char *argv[])
{
    dbref i, test;
    int db_format, db_ver, db_flags, x;
    FILE *fp;

    debugmem = (Debugmem *)malloc(sizeof(Debugmem));
    if (!debugmem)
      abort();
    INITDEBUG(debugmem);
    if (argc != 4) {
	fprintf(stderr,"Try again with the right number of arguments.\n");
	exit(1);
    }
    dddb_var_init();
    cache_var_init();
    cf_init();
    if (init_gdbm_db(argv[1]) < 0) {
	fprintf(stderr,"Couldn't open gdbm file.\n");
	exit(2);
    }
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
	fprintf(stderr,"Couldn't open db file.");
	exit(4);
    }
    if (openmaildb(argv[3]) == -1) {
	fprintf(stderr,"Couldn't open mail or folder database.");
	fclose(fp);
	exit(3);
    }
    test = db_read(fp, &db_format, &db_ver, &db_flags);
    mudstate.db_top = test;
    fprintf(stderr,"Got past open and read: %d %d\n", mudstate.db_top, test);
    setupall();
    if (!readin()) {
      err_out(28,NULL,NULL);
      fprintf(stderr,"FIX ERROR (readin): Memory allocation failed");
      myapurge();
      return;
    }
    if (!fixplay()) {
      err_out(28,NULL,NULL);
      fprintf(stderr,"FIX ERROR (fixplay): Memory allocation failed");
      myapurge();
      return;
    }
    fixglobal();
    procout();
    myapurge();
    fprintf(stderr,"Mail and folder correction complete.\n");
    fprintf(stderr,"A total of: %d error(s) were found and corrected.\n", errcount);
    fclose(output1);
    fclose(output2);
    fclose(fp);
    CLOSE;
}
#else
void 
do_mailfix(dbref player)
{
    fprintf(stderr,"MAILFIX STARTED\n");
    if (dbm_error(mailfile) || dbm_error(foldfile))
	mail_proc_err();
    setupall();
    if (!readin()) {
      err_out(28,NULL,NULL);
      notify_quiet(player,"FIX ERROR (readin): Memory allocation failed");
      myapurge();
      return;
    }
    if (!fixplay()) {
      err_out(28,NULL,NULL);
      notify_quiet(player,"FIX ERROR (fixplay): Memory allocation failed");
      myapurge();
      return;
    }
    fixglobal();
    procout();
    myapurge();
    notify_quiet(player, "Mail and folder correction complete.");
    notify_quiet(player, unsafe_tprintf("A total of: %d error(s) were found and corrected.", errcount));
    fclose(output1);
    fclose(output2);
    fprintf(stderr,"MAILFIX COMPLETED\n");
}
#endif
