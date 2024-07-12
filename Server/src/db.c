
/* db.c */

#ifdef SOLARIS
/* Solaris has borked header file declarations */
void bcopy(const void *, void *, int);
void bzero(void *, int);
#endif

#include "copyright.h"
#include "autoconf.h"

#include <sys/file.h>

#define __DB_C
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "attrs.h"
#include "vattr.h"
#include "match.h"
#include "alloc.h"
#include "interface.h"

#ifndef O_ACCMODE
#define O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

OBJ *db;
OBJTOTEM *dbtotem;
NAME *names = NULL;

#ifdef TEST_MALLOC
int malloc_count = 0;

#endif /* TEST_MALLOC */

extern double mush_mktime64(struct tm *);
extern double safe_atof(char *);
extern int FDECL(do_convtime, (char *, struct tm *));

/* -------
 * Zone list management
 */

int zlist_inlist( dbref source, dbref target )
{
  ZLISTNODE* ptr = NULL;

  ptr = db[source].zonelist;

  if( !ptr ) 
    return 0;

  while( ptr && ptr->object != target ) {
    ptr = ptr->next;
  }

  if( !ptr )
    return 0;
  return 1;
}

void zlist_destroy(dbref player)
{
  ZLISTNODE* ptr = NULL;
  ZLISTNODE* temp = NULL;

  ptr = db[player].zonelist;

  /* we must remove references to self in other lists */
  /* this can be object -> zmo or zmo -> object       */

  for( ; ptr; ptr = ptr->next ) {
    zlist_del(ptr->object,player);
  }

  while( db[player].zonelist ) {
    temp = db[player].zonelist->next;
    free_zlistnode(db[player].zonelist);
    db[player].zonelist = temp;
  }
}

void zlist_del(dbref player, dbref object)
{
  ZLISTNODE* ptr = NULL;
  ZLISTNODE* temp = NULL;

  ptr = db[player].zonelist;

  if( !ptr )
    return;

  if( ptr->object == object ) { /* head deletion */
    temp = db[player].zonelist;
    db[player].zonelist = db[player].zonelist->next;
    free_zlistnode(temp); 
    return;
  }

  temp = db[player].zonelist;
  ptr = temp->next;

  while( ptr && ptr->object != object ) {
    temp = ptr;
    ptr = ptr->next;
  }

  if( !ptr )
    return;

  temp->next = ptr->next;
  free_zlistnode(ptr);
}
 
void zlist_add(dbref player, dbref object)
{
  ZLISTNODE* ptr = NULL;
  ZLISTNODE* node = NULL;
  ZLISTNODE* temp = NULL;

  ptr = db[player].zonelist;

  node = alloc_zlistnode("zlist_add");
  node->next = NULL;
  node->object = object;

  if( !ptr ) {
    db[player].zonelist = node;
    return;
  }

  while( ptr ) {
    temp = ptr;
    ptr = ptr->next;
  }

  temp->next = node;
}

/* ---------------------------------------------------------------------------
 * Temp file management, used to get around static limits in some versions
 * of libc.
 */

FILE *t_fd;
int t_is_pipe;

#ifdef TLI
int t_is_tli;

#endif

static void 
tf_xclose(FILE * fd)
{
    if (fd) {
	if (t_is_pipe)
	    pclose(fd);
#ifdef TLI
	else if (t_is_tli)
	    t_close(fd);
#endif
	else
	    fclose(fd);
    } else {
	close(0);
    }
    t_fd = NULL;
    t_is_pipe = 0;
}

static int 
tf_fiddle(int tfd)
{
    if (tfd < 0) {
	tfd = open(DEV_NULL, O_RDONLY, 0);
	return -1;
    }
    if (tfd != 0) {
	dup2(tfd, 0);
	close(tfd);
    }
    return 0;
}

static int 
tf_xopen(char *fname, int mode)
{
    int fd;

    fd = open(fname, mode, 0600);
    fd = tf_fiddle(fd);
    return fd;
}

/* #define t_xopen(f,m) t_fiddle(open(f, m, 0600)) */

static const char *
mode_txt(int mode)
{
    switch (mode & O_ACCMODE) {
    case O_RDONLY:
	return "r";
    case O_WRONLY:
	return "w";
    }
    return "r+";
}

void 
NDECL(tf_init)
{
    fclose(stdin); 
    tf_xopen(DEV_NULL, O_RDONLY);
    t_fd = NULL;
    t_is_pipe = 0;
}

int 
tf_open(char *fname, int mode)
{
    tf_xclose(t_fd);
    return tf_xopen(fname, mode);
}

#ifndef STANDALONE

int 
tf_socket(int fam, int typ)
{
    tf_xclose(t_fd);
    return tf_fiddle(socket(fam, typ, 0));
}

#ifdef TLI
int 
tf_topen(int fam, int mode)
{
    tf_xclose(t_fd);
    return tf_fiddle(t_open(fam, mode, NULL));
}

#endif
#endif

void 
tf_close(int fdes)
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

FILE *
tf_fopen(char *fname, int mode)
{
    tf_xclose(t_fd);
    if (tf_xopen(fname, mode) >= 0) {
	t_fd = fdopen(0, mode_txt(mode));
	return t_fd;
    }
    return NULL;
}

void 
tf_fclose(FILE * fd)
{
    tf_xclose(t_fd);
    tf_xopen(DEV_NULL, O_RDONLY);
}

FILE *
tf_popen(char *fname, int mode)
{
    tf_xclose(t_fd);
    t_fd = popen(fname, mode_txt(mode));
    if (t_fd != NULL) {
	t_is_pipe = 1;
    }
    return t_fd;
}

/* #define GNU_MALLOC_TEST 1 */

#ifdef GNU_MALLOC_TEST
extern unsigned int malloc_sbrk_used;	/* amount of data space used now */

#endif

/* Check routine forward declaration. */
extern int FDECL(fwdlist_ck, (int, dbref, dbref, int, char *));

/* Other declarations */
extern int FDECL(ansiname_ck, (int, dbref, dbref, int, char *));
extern int FDECL(progprompt_ck, (int, dbref, dbref, int, char *));

extern void FDECL(pcache_reload, (dbref));
extern void FDECL(desc_reload, (dbref));
extern void FDECL(do_reboot, (dbref, dbref, int));
extern int FDECL(process_output, (DESC * d));
extern int FDECL(list_vcount, ());


AFLAGENT attrflag[] =
{
  {"ARCHITECT", 1, AF_BUILDER, 'A'},
  {"COUNCILOR", 1, AF_ADMIN, 'C'},
  {"DARK", 2, AF_DARK, 'D'},
  {"DELETED", 2, AF_DELETED, 'd'},
  {"GOD", 2, AF_GOD, 'G'},
  {"GUILDMASTER", 2, AF_GUILDMASTER, 'g'},
  {"IMMORTAL", 2, AF_IMMORTAL, 'I'},
  {"INTERNAL", 2, AF_INTERNAL,'X'},
  {"ISLOCK", 2, AF_IS_LOCK, 'i'},
  {"LOCK", 1, AF_LOCK, '+'},
  {"MDARK", 1, AF_MDARK, 'M'},
  {"NOANSI", 3, AF_NOANSI, 'a'},
  {"NOCMD", 3, AF_NOCMD, 'c'},
  {"NONBLOCKING", 3, AF_NONBLOCKING, 'B'},
  {"NOPROG", 3, AF_NOPROG, '$'},
  {"PRIVATE", 1, AF_ODARK, 'O'},
  {"PINVISIBLE", 2, AF_PINVIS, 'p'},
  {"NO_INHERIT", 4, AF_PRIVATE, 'P'},
  {"ROYALTY", 1, AF_WIZARD, 'R'},
  {"VISUAL", 1, AF_VISUAL, 'V'},
  {"NO_CLONE", 4, AF_NOCLONE, 'N'},
  {"NO_PARSE", 4, AF_NOPARSE, 'n'},
  {"SAFE", 2, AF_SAFE, 's'},
  {"USELOCK", 3, AF_USELOCK, 'u'},
  {"SINGLETHREAD", 3, AF_SINGLETHREAD, 'S'},
  {"DEFAULT", 3, AF_DEFAULT, 'F'},
  {"ATRLOCK", 3, AF_ATRLOCK, 'l'},
  {"LOGGED", 3, AF_LOGGED, 'm'},
/* These two add dynamically when turned into a hash */
/* Check look.c for the attrib lookup stuff as well */
  {"REGEXP", 3, AF_REGEXP, 'R'},
  {"UNSAFE", 3, AF_UNSAFE, 'U'},
  {NULL, 0, 0}};

/* list of attributes */
ATTR attr[] =
{
    {"Aahear", A_AAHEAR, AF_ODARK, NULL},
    {"Aclone", A_ACLONE, AF_ODARK, NULL},
    {"Aconnect", A_ACONNECT, AF_ODARK, NULL},
    {"Adesc", A_ADESC, AF_ODARK, NULL},
    {"Adesc2", A_ADESC2, AF_MDARK | AF_WIZARD, NULL},
    {"Adfail", A_ADFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Adisconnect", A_ADISCONNECT, AF_ODARK, NULL},
    {"Adrop", A_ADROP, AF_ODARK, NULL},
    {"Aefail", A_AEFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Aenter", A_AENTER, AF_ODARK, NULL},
    {"Afail", A_AFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Agfail", A_AGFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Ahear", A_AHEAR, AF_ODARK, NULL},
    {"Akill", A_AKILL, AF_ODARK, NULL},
    {"Aleave", A_ALEAVE, AF_ODARK, NULL},
/*  {"AllowPageLock", A_LALLOWPAGE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK | AF_WIZARD | AF_MDARK,
     NULL}, */
    {"Alfail", A_ALFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Alias", A_ALIAS, AF_NOPROG | AF_NOCMD | AF_GOD, NULL},
    {"Allowance", A_ALLOWANCE, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"AltName", A_ALTNAME, AF_ODARK | AF_NOPROG | AF_NOANSI | AF_NORETURN | AF_WIZARD | AF_MDARK,
     NULL},
    {"AltNameLock", A_LALTNAME, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK | AF_WIZARD | AF_MDARK, 
     NULL},
    {"Amhear", A_AMHEAR, AF_ODARK, NULL},
    {"Amove", A_AMOVE, AF_ODARK, NULL},
    {"AnsiName", A_ANSINAME, AF_ODARK | AF_NOPROG | AF_NOANSI, ansiname_ck},
    {"Apay", A_APAY, AF_ODARK, NULL},
    {"Arfail", A_ARFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Asfail", A_ASFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Asucc", A_ASUCC, AF_ODARK, NULL},
    {"Atfail", A_ATFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Atofail", A_ATOFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Atport", A_ATPORT, AF_ODARK | AF_NOPROG, NULL},
    {"Aufail", A_AUFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Ause", A_AUSE, AF_ODARK, NULL},
    {"Autoreg", A_AUTOREG, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"Away", A_AWAY, AF_ODARK | AF_NOPROG, NULL},
    {"BadSite", A_SITEBAD, AF_DARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"BCCMail", A_BCCMAIL, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"Caption", A_CAPTION, AF_ODARK | AF_NOPROG | AF_NOANSI | AF_NORETURN, NULL},
    {"Charges", A_CHARGES, AF_ODARK | AF_NOPROG, NULL},
    {"ChownLock", A_LCHOWN, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, 
     NULL},
    {"Channels", A_CHANNEL, AF_WIZARD, NULL},
    {"Cmdcheck", A_CMDCHECK, AF_IMMORTAL | AF_MDARK | AF_NOPROG, NULL},
    {"Comment", A_COMMENT, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
    {"ConFormat", A_LCON_FMT, AF_ODARK | AF_NOPROG, NULL},
    {"ConnInfo", A_CONNINFO, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"ConnRecord", A_CONNRECORD, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"ControlLock", A_LCONTROL, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Cost", A_COST, AF_ODARK, NULL},
    {"CreatedStamp", A_CREATED_TIME, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"Desc", A_DESC, AF_NOPROG, NULL},
    {"DestVattrMax", A_DESTVATTRMAX, AF_DARK|AF_NOPROG|AF_INTERNAL|AF_NOCMD|AF_GOD, NULL},
    {"DarkLock", A_LDARK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"DarkExitFormat", A_LDEXIT_FMT, AF_ODARK | AF_NOPROG | AF_WIZARD, NULL},
    {"DefaultLock", A_LOCK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Dfail", A_DFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Drop", A_DROP, AF_ODARK | AF_NOPROG, NULL},
    {"DropLock", A_LDROP, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"DropToLock", A_LDROPTO, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Ealias", A_EALIAS, AF_ODARK | AF_NOPROG, NULL},
    {"Efail", A_EFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Enter", A_ENTER, AF_ODARK, NULL},
    {"EnterLock", A_LENTER, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"ExitFormat", A_LEXIT_FMT, AF_ODARK | AF_NOPROG, NULL},
    {"ExitTo", A_EXITTO, AF_ODARK | AF_NOPROG, NULL},
    {"Fail", A_FAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Filter", A_FILTER, AF_ODARK | AF_NOPROG, NULL},
    {"Forwardlist", A_FORWARDLIST, AF_ODARK | AF_NOPROG, fwdlist_ck},
    {"GetFromLock", A_LGETFROM, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Gfail", A_GFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"GiveLock", A_LGIVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"GiveToLock", A_LGIVETO, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"GoodSite", A_SITEGOOD, AF_DARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"Guild", A_GUILD, AF_NOPROG | AF_GUILDMASTER | AF_NOANSI,
     NULL},
    {"Idesc", A_IDESC, AF_ODARK | AF_NOPROG, NULL},
    {"Idle", A_IDLE, AF_ODARK | AF_NOPROG, NULL},
    {"Infilter", A_INFILTER, AF_ODARK | AF_NOPROG, NULL},
    {"Inprefix", A_INPREFIX, AF_ODARK | AF_NOPROG, NULL},
    {"InvType", A_INVTYPE, AF_ODARK | AF_NOPROG | AF_NOANSI | AF_NORETURN | AF_WIZARD | AF_MDARK,
     NULL},
    {"Kill", A_KILL, AF_ODARK, NULL},
    {"Lalias", A_LALIAS, AF_ODARK | AF_NOPROG, NULL},
    {"lambda_internal_foo", A_LAMBDA, AF_PINVIS | AF_NOCLONE | AF_NOPROG | AF_GOD | AF_NOCMD | AF_PRIVATE, NULL},
    {"Last", A_LAST, AF_WIZARD | AF_NOCMD | AF_NOPROG, NULL},
    {"LastCreate", A_LASTCREATE, AF_DARK | AF_INTERNAL | AF_NOPROG | AF_NOCMD, NULL},
    {"LastIP", A_LASTIP, AF_MDARK | AF_NOPROG | AF_NOCMD | AF_GOD, NULL},
    {"LastPage", A_LASTPAGE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_WIZARD, NULL},
    {"Lastsite", A_LASTSITE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_GOD, NULL},
    {"Leave", A_LEAVE, AF_ODARK, NULL},
    {"LeaveLock", A_LLEAVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Lfail", A_LFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"LinkLock", A_LLINK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Listen", A_LISTEN, AF_ODARK, NULL},
    {"Logindata", A_LOGINDATA, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
     NULL},
    {"Lquota", A_LQUOTA, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"MailCurrent", A_MCURR, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IMMORTAL,
     NULL},
    {"MailLock", A_LMAIL, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"MailSig", A_MAILSIG, AF_ODARK | AF_NOPROG, NULL},
    {"MailSMax", A_MSAVEMAX, AF_ODARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"MailSCur", A_MSAVECUR, AF_ODARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"MailTime", A_MTIME, AF_ODARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"ModifiedStamp", A_MODIFY_TIME, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"Move", A_MOVE, AF_ODARK, NULL},
    {"Mquota", A_MQUOTA, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"Name", A_NAME, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
     NULL},
    {"NameFormat", A_NAME_FMT, AF_ODARK | AF_NOPROG, NULL},
    {"FlagLevel", A_FLAGLEVEL, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD,
     NULL},
    {"Odesc", A_ODESC, AF_ODARK | AF_NOPROG, NULL},
    {"Odfail", A_ODFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Odrop", A_ODROP, AF_ODARK | AF_NOPROG, NULL},
    {"Oefail", A_OEFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Oenter", A_OENTER, AF_ODARK, NULL},
    {"Ofail", A_OFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Ogfail", A_OGFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Okill", A_OKILL, AF_ODARK, NULL},
    {"Oleave", A_OLEAVE, AF_ODARK, NULL},
    {"Olfail", A_OLFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Omove", A_OMOVE, AF_ODARK, NULL},
    {"Opay", A_OPAY, AF_ODARK, NULL},
    {"OpenLock", A_LOPEN, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Orfail", A_ORFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Osucc", A_OSUCC, AF_ODARK | AF_NOPROG, NULL},
    {"Otfail", A_OTFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Otofail", A_OTOFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Otport", A_OTPORT, AF_ODARK | AF_NOPROG, NULL},
    {"Oufail", A_OUFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Ouse", A_OUSE, AF_ODARK, NULL},
    {"Oxenter", A_OXENTER, AF_ODARK, NULL},
    {"Oxleave", A_OXLEAVE, AF_ODARK, NULL},
    {"Oxtport", A_OXTPORT, AF_ODARK | AF_NOPROG, NULL},
    {"PageLock", A_LPAGE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"ParentLock", A_LPARENT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Pay", A_PAY, AF_ODARK, NULL},
    {"PayLim", A_PAYLIM, AF_MDARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"Prefix", A_PREFIX, AF_ODARK | AF_NOPROG, NULL},
    {"ProgBuffer", A_PROGBUFFER, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"ProgPrompt", A_PROGPROMPT, AF_ODARK | AF_NOPROG | AF_NOANSI | AF_NORETURN, progprompt_ck},
    {"ProgPromptBuffer", A_PROGPROMPTBUF, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"ProtectName", A_PROTECTNAME, AF_DARK | AF_MDARK | AF_INTERNAL | AF_GOD | AF_NOCMD, NULL},
    {"QueueMax", A_QUEUEMAX, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
    {"Quota", A_QUOTA, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"Race", A_RACE, AF_NOPROG | AF_BUILDER, NULL},
    {"ReceiveLock", A_LRECEIVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"ReceiveLim", A_RECEIVELIM, AF_MDARK | AF_NOPROG | AF_IMMORTAL, NULL},
    {"Rectime", A_RECTIME, AF_MDARK | AF_NOPROG | AF_NOCMD | AF_IMMORTAL, NULL},
    {"ReturnPage", A_RETPAGE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_WIZARD, NULL},
    {"Reject", A_REJECT, AF_ODARK | AF_NOPROG, NULL},
    {"Rfail", A_RFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Rquota", A_RQUOTA, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"RsrvDesc2", A_DESC2, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"Runout", A_RUNOUT, AF_ODARK, NULL},
    {"SAListen", A_SALISTEN, AF_ODARK | AF_NOPROG, NULL},
    {"SASmell", A_SASMELL, AF_ODARK | AF_NOPROG, NULL},
    {"SATaste", A_SATASTE, AF_ODARK | AF_NOPROG, NULL},
    {"SATouch", A_SATOUCH, AF_ODARK | AF_NOPROG, NULL},
    {"SaveSendMail", A_SAVESENDMAIL, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"Semaphore", A_SEMAPHORE, AF_ODARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"Sex", A_SEX, AF_NOPROG, NULL},
    {"SayString", A_SAYSTRING, AF_ODARK | AF_NOPROG | AF_NOANSI | AF_NORETURN, NULL},
    {"Sfail", A_SFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"ShareLock", A_LSHARE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
    {"SListen", A_SLISTEN, AF_ODARK | AF_NOPROG, NULL},
    {"SOListen", A_SOLISTEN, AF_ODARK | AF_NOPROG, NULL},
    {"SOSmell", A_SOSMELL, AF_ODARK | AF_NOPROG, NULL},
    {"SOTaste", A_SOTASTE, AF_ODARK | AF_NOPROG, NULL},
    {"SOTouch", A_SOTOUCH, AF_ODARK | AF_NOPROG, NULL},
    {"SSmell", A_SSMELL, AF_ODARK | AF_NOPROG, NULL},
    {"SpamProtect", A_SPAMPROTECT, AF_DARK|AF_NOPROG|AF_INTERNAL|AF_NOCMD|AF_GOD, NULL},
    {"SpeechLock", A_LSPEECH, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Startup", A_STARTUP, AF_ODARK, NULL},
    {"STaste", A_STASTE, AF_ODARK | AF_NOPROG, NULL},
    {"STouch", A_STOUCH, AF_ODARK | AF_NOPROG, NULL},
    {"Succ", A_SUCC, AF_ODARK | AF_NOPROG, NULL},
    {"TeloutLock", A_LTELOUT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"TempBuffer", A_TEMPBUFFER, AF_DARK | AF_NOPROG | AF_INTERNAL | AF_NOCMD, NULL},
    {"TitleCaption", A_TITLE, AF_ODARK | AF_NOPROG | AF_NORETURN, NULL},
    {"Tfail", A_TFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Timeout", A_TIMEOUT, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
    {"Tofail", A_TOFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"TotCharIn", A_TOTCHARIN, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOANSI | AF_NOCMD, NULL},
    {"TotCharOut", A_TOTCHAROUT, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOANSI | AF_NOCMD, NULL},
    {"TotCmds", A_TOTCMDS, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOANSI | AF_NOCMD, NULL},
    {"LstCmds", A_LSTCMDS, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOANSI | AF_NOCMD, NULL},
    {"Tport", A_TPORT, AF_ODARK | AF_NOPROG, NULL},
    {"TportLock", A_LTPORT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"Tquota", A_TQUOTA, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"Ufail", A_UFAIL, AF_ODARK | AF_NOPROG, NULL},
    {"Use", A_USE, AF_ODARK, NULL},
    {"UseLock", A_LUSE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"UserLock", A_LUSER, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"VA", A_VA, AF_ODARK, NULL},
    {"VB", A_VA + 1, AF_ODARK, NULL},
    {"VC", A_VA + 2, AF_ODARK, NULL},
    {"VD", A_VA + 3, AF_ODARK, NULL},
    {"VE", A_VA + 4, AF_ODARK, NULL},
    {"VF", A_VA + 5, AF_ODARK, NULL},
    {"VG", A_VA + 6, AF_ODARK, NULL},
    {"VH", A_VA + 7, AF_ODARK, NULL},
    {"VI", A_VA + 8, AF_ODARK, NULL},
    {"VJ", A_VA + 9, AF_ODARK, NULL},
    {"VK", A_VA + 10, AF_ODARK, NULL},
    {"VL", A_VA + 11, AF_ODARK, NULL},
    {"VM", A_VA + 12, AF_ODARK, NULL},
    {"VN", A_VA + 13, AF_ODARK, NULL},
    {"VO", A_VA + 14, AF_ODARK, NULL},
    {"VP", A_VA + 15, AF_ODARK, NULL},
    {"VQ", A_VA + 16, AF_ODARK, NULL},
    {"VR", A_VA + 17, AF_ODARK, NULL},
    {"VS", A_VA + 18, AF_ODARK, NULL},
    {"VT", A_VA + 19, AF_ODARK, NULL},
    {"VU", A_VA + 20, AF_ODARK, NULL},
    {"VV", A_VA + 21, AF_ODARK, NULL},
    {"VW", A_VA + 22, AF_ODARK, NULL},
    {"VX", A_VA + 23, AF_ODARK, NULL},
    {"VY", A_VA + 24, AF_ODARK, NULL},
    {"VZ", A_VA + 25, AF_ODARK, NULL},
    {"ZA", A_ZA, AF_ODARK, NULL},
    {"ZB", A_ZA + 1, AF_ODARK, NULL},
    {"ZC", A_ZA + 2, AF_ODARK, NULL},
    {"ZD", A_ZA + 3, AF_ODARK, NULL},
    {"ZE", A_ZA + 4, AF_ODARK, NULL},
    {"ZF", A_ZA + 5, AF_ODARK, NULL},
    {"ZG", A_ZA + 6, AF_ODARK, NULL},
    {"ZH", A_ZA + 7, AF_ODARK, NULL},
    {"ZI", A_ZA + 8, AF_ODARK, NULL},
    {"ZJ", A_ZA + 9, AF_ODARK, NULL},
    {"ZK", A_ZA + 10, AF_ODARK, NULL},
    {"ZL", A_ZA + 11, AF_ODARK, NULL},
    {"ZM", A_ZA + 12, AF_ODARK, NULL},
    {"ZN", A_ZA + 13, AF_ODARK, NULL},
    {"ZO", A_ZA + 14, AF_ODARK, NULL},
    {"ZP", A_ZA + 15, AF_ODARK, NULL},
    {"ZQ", A_ZA + 16, AF_ODARK, NULL},
    {"ZR", A_ZA + 17, AF_ODARK, NULL},
    {"ZS", A_ZA + 18, AF_ODARK, NULL},
    {"ZT", A_ZA + 19, AF_ODARK, NULL},
    {"ZU", A_ZA + 20, AF_ODARK, NULL},
    {"ZV", A_ZA + 21, AF_ODARK, NULL},
    {"ZW", A_ZA + 22, AF_ODARK, NULL},
    {"ZX", A_ZA + 23, AF_ODARK, NULL},
    {"ZY", A_ZA + 24, AF_ODARK, NULL},
    {"ZZ", A_ZA + 25, AF_ODARK, NULL},
    {"*Password", A_PASS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
     NULL},
    {"*Totems", A_PRIVS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
     NULL},
    {"*Money", A_MONEY, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
     NULL},
 {"*MailPass", A_MPASS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
  {"*MailSet", A_MPSET, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
  {"Ident", A_IDENT, AF_DARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
    {"ZoneWizLock", A_LZONEWIZ, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"ZoneToLock", A_LZONETO, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"TwinkLock", A_LTWINK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
     NULL},
    {"*Email", A_EMAIL, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"*Pfail", A_PFAIL, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
    {"RLevel", A_RLEVEL, AF_DARK | AF_NOPROG | AF_NOCMD | AF_PRIVATE | AF_INTERNAL, NULL},
    {"____ObjectTag", A_OBJECTTAG, AF_DARK | AF_NOPROG | AF_NOCMD | AF_PRIVATE | AF_INTERNAL, NULL},
    {NULL, 0, 0, NULL}};

#ifndef STANDALONE

char *give_name_aflags(dbref player, dbref thing, int flagkey)
{
  AFLAGENT *afp;
  
  for (afp = attrflag; afp->flagname; afp++) {
     if (flagkey & afp->flagvalue) 
        return (char *)afp->flagname;
  }
  return (char *)NULL;
}

int parse_aflags(dbref player, dbref thing, int anum, char *bp, char **bpx, int full)
{
  AFLAGENT *afp;
  ATTR *ap;
  dbref aowner;
  int aflags, rt;

  ap = atr_num(anum);
  if (ap == NULL)
    rt = 0;
  else if (atr_get_info(thing, anum, &aowner, &aflags)) {
    if (Read_attr(player, thing, ap, aowner, aflags, 0)) {
      rt = 0;
      aflags |= ap->flags;
      if (!Wizard(player))
	aflags &= ~AF_PINVIS;
      for (afp = attrflag; afp->flagname; afp++) {
	if (aflags & afp->flagvalue) {
	  if (full) {
	    if (rt)
	      safe_chr(' ', bp, bpx);
	    safe_str((char *)(afp->flagname), bp, bpx);
	  }
	  else
	    safe_chr(afp->flaglett, bp, bpx);
	  rt = 1;
	}
      }
    }
    else
      rt = 0;
  }
  else
    rt = 0;
  return rt;
}

int has_aflag(dbref player, dbref thing, int anum, char *fname)
{
  AFLAGENT *afp;
  ATTR *ap;
  dbref aowner;
  int aflags, rt;

  ap = atr_num(anum);
  if (ap == NULL)
    rt = 0;
  else if (atr_get_info(thing, anum, &aowner, &aflags)) {
    if (Read_attr(player, thing, ap, aowner, aflags, 1)) {
      rt = 0;
      aflags |= ap->flags;
      if (!Wizard(player))
	aflags &= ~AF_PINVIS;
      for (afp = attrflag; afp->flagname; afp++) {
	if ((aflags & afp->flagvalue) && minmatch(fname, (char *)afp->flagname, afp->flagmatch)) {
	  rt = 1;
	  break;
	}
      }
    }
    else
      rt = 0;
  }
  else
    rt = 0;
  return rt;
}

/* ---------------------------------------------------------------------------
 * fwdlist_set, fwdlist_clr: Manage cached forwarding lists
 */

void 
fwdlist_set(dbref thing, FWDLIST * ifp)
{
   FWDLIST *fp, *xfp;
   int i, stat;
   char *logbuf;

/* If fwdlist is null, clear */

   if (!ifp || (ifp->count <= 0)) {
      fwdlist_clr(thing);
      return;
   }
   /* Copy input forwardlist to a correctly-sized buffer */

   fp = (FWDLIST *) XMALLOC(sizeof(int) * ((ifp->count) + 1), "fwdlist_set");

   for (i = 0; i < ifp->count; i++) {
      fp->data[i] = ifp->data[i];
   }
   fp->count = ifp->count;

   /* Replace an existing forwardlist, or add a new one */

   xfp = fwdlist_get(thing);
   if (xfp) {
      XFREE(xfp, "fwdlist_set");
      nhashrepl(thing, (int *) fp, &mudstate.fwdlist_htab);
   } else {
       stat = nhashadd(thing, (int *) fp, &mudstate.fwdlist_htab);
       stat = (stat < 0) ? 0 : 1;
       if(!stat) {
          logbuf = alloc_lbuf("fwdlist_add");
          sprintf(logbuf,"UNABLE TO ADD FWDLIST HASH FOR #%d", thing);
	  STARTLOG(LOG_ALWAYS, "BUG", "FWDLIST")
             log_text(logbuf);
          ENDLOG
          free(fp);
          free(logbuf);
       }
   }
}

void 
fwdlist_clr(dbref thing)
{
    FWDLIST *xfp;

    /* If a forwardlist exists, delete it */

    xfp = fwdlist_get(thing);
    if (xfp) {
	XFREE(xfp, "fwdlist_clr");
	nhashdelete(thing, &mudstate.fwdlist_htab);
    }
}

#endif

/* ---------------------------------------------------------------------------
 * fwdlist_load: Load text into a forwardlist.
 */

int 
fwdlist_load(FWDLIST * fp, dbref player, char *atext)
{
    dbref target;
    char *tp, *bp, *dp;
    int count, errors, fail;

    count = 0;
    errors = 0;
    bp = tp = alloc_lbuf("fwdlist_load.str");
    strcpy(tp, atext);
    do {
	for (; *bp && isspace((int)*bp); bp++);	/* skip spaces */
	for (dp = bp; *bp && !isspace((int)*bp); bp++);	/* remember string */
	if (*bp)
	    *bp++ = '\0';	/* terminate string */
	if ((*dp++ == '#') && isdigit((int)*dp)) {
	    target = atoi(dp);
#ifndef STANDALONE
	    fail = (!Good_obj(target) ||
		    (!God(player) &&
		     !controls(player, target) &&
		     (!Link_ok(target) ||
		      !could_doit(player, target, A_LLINK, 1, 0))));
#else
	    fail = !Good_obj(target);
#endif
	    if (fail) {
#ifndef STANDALONE
		notify(player,
		       unsafe_tprintf("Cannot forward to #%d: Permission denied.",
			       target));
#endif
		errors++;
	    } else {
		fp->data[count++] = target;
	    }
	}
    } while (*bp);
    free_lbuf(tp);
    fp->count = count;
    return errors;
}

/* ---------------------------------------------------------------------------
 * fwdlist_rewrite: Generate a text string from a FWDLIST buffer.
 */

int 
fwdlist_rewrite(FWDLIST * fp, char *atext)
{
    char *tp, *bp;
    int i, count;

    if (fp && fp->count) {
	count = fp->count;
	tp = alloc_sbuf("fwdlist_rewrite.errors");
	bp = atext;
	for (i = 0; i < fp->count; i++) {
	    if (Good_obj(fp->data[i])) {
		sprintf(tp, "#%d ", fp->data[i]);
		safe_str(tp, atext, &bp);
	    } else {
		count--;
	    }
	}
	*bp = '\0';
	free_sbuf(tp);
    } else {
	count = 0;
	*atext = '\0';
    }
    return count;
}

/* ---------------------------------------------------------------------------
 * progprompt_ck :  don't exceed 80 characters
 */

int
progprompt_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
#ifndef STANDALONE
   if ( strlen(strip_all_ansi(atext)) > 80 ) {
      notify(player, "ProgPrompt can not exceed 80 characters.");
      return 0;
   } else {
      return 1;
   }
#else
   return 1;
#endif
}

/* ---------------------------------------------------------------------------
 * ansiname_ck:  don't allow if player set noansiname
 */

int 
ansiname_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
   if ( NoAnsiName(Owner(player)) ) {
#ifndef STANDALONE
      notify(player, "Permission denied.");
#endif
      return 0;
   }
   else
      return 1;
}

/* ---------------------------------------------------------------------------
 * fwdlist_ck:  Check a list of dbref numbers to forward to for AUDIBLE
 */

int 
fwdlist_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
#ifndef STANDALONE

    FWDLIST *fp;
    int count;

    count = 0;

    if (atext && *atext) {
	fp = (FWDLIST *) alloc_lbuf("fwdlist_ck.fp");
	fwdlist_load(fp, player, atext);
    } else {
	fp = NULL;
    }

    /* Set the cached forwardlist */

    fwdlist_set(thing, fp);
    count = fwdlist_rewrite(fp, atext);
    if (fp)
	free_lbuf(fp);
    return ((count > 0) || !atext || !*atext);
#else
    return 1;
#endif
}

FWDLIST *
fwdlist_get(dbref thing)
{
#ifdef STANDALONE

    dbref aowner;
    int aflags;
    char *tp;

    static FWDLIST *fp = NULL;

    if (!fp)
	fp = (FWDLIST *) alloc_lbuf("fwdlist_get");
    tp = atr_get(thing, A_FORWARDLIST, &aowner, &aflags);
    fwdlist_load(fp, GOD, tp);
    free_lbuf(tp);
#else
    FWDLIST *fp;

    fp = ((FWDLIST *) nhashfind(thing, &mudstate.fwdlist_htab));
#endif
    return fp;
}

static char *
set_string(char **ptr, char *new)
{
    /* if pointer not null unalloc it */

    if (*ptr)
	XFREE(*ptr, "set_string");

    /* if new string is not null allocate space for it and copy it */

    if (!new)			/* || !*new */
	return (*ptr = NULL);	/* Check with GAC about this */
    *ptr = (char *) XMALLOC(strlen(new) + 1, "set_string");
    strcpy(*ptr, new);
    return (*ptr);
}

/* ---------------------------------------------------------------------------
 * Name, s_Name: Get or set an object's name.
 */

INLINE char *
Name(dbref thing)
{
    dbref aowner;
    int aflags;
    char *buff;
    static char *tbuff[LBUF_SIZE];

    if (mudconf.cache_names) {
	if (!names[thing]) {
	    buff = atr_get(thing, A_NAME, &aowner, &aflags);
	    set_string(&names[thing], buff);
	    free_lbuf(buff);
	}
	return (names[thing]);
    }
    atr_get_str((char *) tbuff, thing, A_NAME, &aowner, &aflags);
    return ((char *) tbuff);
}

#ifndef STANDALONE

INLINE char *
Guild(dbref thing)
{
    dbref aowner;
    int aflags;
    char *attrval = NULL, *retval = NULL;
    static char temp[LBUF_SIZE];
    ATTR *attr;

    attr = atr_str(mudconf.guild_attrname);
    if (!attr) {
       retval = mudconf.guild_default;
    } else {
       attrval = atr_get_str(temp, thing, attr->number, &aowner, &aflags);
     
       if (attrval == NULL || *attrval == '\0') {
         retval = mudconf.guild_default;
       } else {
         retval = attrval;
       }
    }
  return strip_all_special(strip_returntab(retval,3));
}
#endif 

INLINE void 
s_Name(dbref thing, const char *s)
{
    atr_add_raw(thing, A_NAME, (char *) s);
    if (mudconf.cache_names) {
	set_string(&names[thing], (char *) s);
    }
}

void 
s_Pass(dbref thing, const char *s)
{
    atr_add_raw(thing, A_PASS, (char *) s);
}

void
s_MPass(dbref thing, const char *s)
{
  atr_add_raw(thing, A_MPASS, (char *) s);
}


#ifndef STANDALONE

/* First 100k attributes only -- allow overhead buffering */
void
do_recache_vattrs( dbref player, int key, char *s_type, int i_internal )
{
   int i, i_min, i_max, i_repeat, i_pass;
   char *s_buff, *s_ptr, *s_tmp;
   ATTR *va;

   i_min = 513; /* Let's skip past 512 just to be safe and give some buffer */
   i_pass = i_repeat = 1;
   i_max = ((mudstate.attr_next - 1) < (MAXVATTRCACHE - 1) ? (mudstate.attr_next - 1) : (MAXVATTRCACHE - 1));
   switch(key) {
      case 0: /* load cache */
         /* Clear the current cache for reloading */
         for ( i = 0; i < MAXVATTRCACHE + 1; i++ ) {
            mudstate.vattr_reuse[i] = 0;
         }
         mudstate.vattr_reuseptr = NOTHING;
         mudstate.vattr_reusecnt = 0;
         /* Start at 512 to be 'safe' */
         if ( !i_internal ) 
            notify(player, "Loading free attribute cache.  Please wait...");
         while ( i_repeat ) {
            if ( !i_internal ) 
               notify(player, unsafe_tprintf("...pass %d", i_pass));
            i_pass++;
            for ( i = i_min; i < i_max; i++ ) {
               if ( i >= mudstate.attr_next || mudstate.vattr_reusecnt > (MAXVATTRCACHE - 1) )
                  break;
               va = atr_num_vattr(i);
               if ( !va ) {
                  mudstate.vattr_reuse[mudstate.vattr_reusecnt] = i;
                  if ( mudstate.vattr_reuseptr == NOTHING ) {
                     mudstate.vattr_reuseptr = mudstate.vattr_reusecnt;
                  }
                  mudstate.vattr_reusecnt++;
               }
               va = NULL;
               if ( mudstate.vattr_reusecnt >= (MAXVATTRCACHE - 1) ) {
                  i_repeat = 0;
                  break;
               }
            }
            if ( i_repeat && (mudstate.vattr_reusecnt < (MAXVATTRCACHE - 1)) ) {
               i_min += (MAXVATTRCACHE - 1);
               i_max += (MAXVATTRCACHE - 1);
               if ( i_min >= mudstate.attr_next ) {
                  i_repeat = 0;
               }
               if ( i_max >= mudstate.attr_next ) {
                  i_max = (mudstate.attr_next - 1);
               }
            }
            if ( mudstate.vattr_reusecnt >= (MAXVATTRCACHE - 1)) {
               i_repeat = 0;
            }
         }
         if ( !i_internal ) {
            notify(player, "Completed.");
            if ( mudstate.vattr_reusecnt == 0 ) {
               notify(player, "There are no attributes to be re-used.");
            } else {
               notify(player, unsafe_tprintf("Total re-useable attributes cached: %d, Next to be used: %d", 
                      mudstate.vattr_reusecnt, mudstate.vattr_reuse[mudstate.vattr_reuseptr]));
            }
         }
         break;
      case 1: /* list cache */
         if ( !i_internal && !mudstate.vattr_reusecnt ) {
            notify(player, "Free attribute-number cache has not been populated.");
         } else if ( !i_internal ) {
            notify(player, "Showing free attrib-numbers.");
            notify(player, "------------------------------------------------------------------------------");
            i_max=(mudstate.vattr_reusecnt / 180) + 1;
            i_min = atoi(s_type);
            if ( i_min < 1 )
               i_min = 1;
            if ( i_min > i_max )
               i_min = i_max;
            s_tmp = alloc_sbuf("attribute_cachesb");
            s_ptr = s_buff = alloc_mbuf("attribute_cachemb");
            i_repeat = 0;
            i_pass = (i_min - 1) * 180;
            for ( i = i_pass; i < i_pass + 180; i++ ) {
               if ( (i >= (MAXVATTRCACHE - 1)) || (i >= mudstate.vattr_reusecnt) ) {
                  break;
               }
               if ( mudstate.vattr_reuse[i] ) {
                  sprintf(s_tmp, "%12d", mudstate.vattr_reuse[i]);
               } else {
                  sprintf(s_tmp, "%12s", (char *)" ");
               }
               if ( i_repeat % 6 ) {
                  safe_str(s_tmp, s_buff, &s_ptr);
               } else if ( i_repeat != 0 ) {
                  if ( i_repeat != 6 )
                     safe_str(s_tmp, s_buff, &s_ptr);
                  notify(player, s_buff);
                  memset(s_buff, '\0', MBUF_SIZE);
                  s_ptr = s_buff;
               } else {
                  safe_str(s_tmp, s_buff, &s_ptr);
               }
               i_repeat++;
            }
            if ( *s_buff ) {
               notify(player, s_buff);
            }
            free_sbuf(s_tmp);
            free_mbuf(s_buff);
            notify(player, "------------------------------------------------------------------------------");
            notify(player, unsafe_tprintf("Page %d out of %d", i_min, i_max));
            notify(player, unsafe_tprintf("Total re-useable attributes cached: %d, Next to be used: %d", 
                   mudstate.vattr_reusecnt, mudstate.vattr_reuse[mudstate.vattr_reuseptr]));
         }
         break;
   }
}

/* ---------------------------------------------------------------------------
 * do_attrib: Manage user-named attributes.
 */

extern NAMETAB attraccess_nametab[];

void 
do_attribute(dbref player, dbref cause, int key, char *aname, char *value)
{
    int success, negate, f, delcnt, aflags;
    char *buff, *sp, *p, *q, *lbuff, *s_chkattr, *tstrtokr;
    dbref i, aowner;
    VATTR *va;
    ATTR *va2;

    /* If cache is specified, cache the attribute reuse cache */
    /* Look up the user-named attribute we want to play with */
    if ( key == ATTRIB_CACHELD ) {
       do_recache_vattrs(player, 0, aname, 0);
       return;
    }
   
    if ( key == ATTRIB_CACHESH ) {
       do_recache_vattrs(player, 1, aname, 0);
       return;
    }

    i = delcnt = 0;
    buff = alloc_sbuf("do_attribute");
    for (p = buff, q = aname; *q && ((p - buff) < (SBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    va = (VATTR *) vattr_find(buff);
    if (!va) {
	notify(player, "No such user-named attribute.");
	free_sbuf(buff);
	return;
    }
    if (*value && (!key || key == ATTRIB_DELETE)) {
	notify(player, "Illegal combination of switches and arguments.");
	free_sbuf(buff);
	return;
    }
    switch (key) {
    case ATTRIB_ACCESS:

	/* Modify access to user-named attribute */

	for (sp = value; *sp; sp++)
	    *sp = ToLower((int)*sp);
	sp = strtok_r(value, " ", &tstrtokr);
	success = 0;
	while (sp != NULL) {

	    /* Check for negation */

	    negate = 0;
	    if (*sp == '!') {
		negate = 1;
		sp++;
	    }
	    /* Set or clear the appropriate bit */

	    f = search_nametab(player, attraccess_nametab, sp);
	    if (f != -1) {
		success = 1;
		if (negate)
		    va->flags &= ~f;
		else
		    va->flags |= f;
	    } else {
		notify(player,
		       unsafe_tprintf("Unknown permission: %s.", sp));
	    }

	    /* Get the next token */

	    sp = strtok_r(NULL, " ", &tstrtokr);
	}
	if (success && !Quiet(player))
	    notify(player, "Attribute access changed.");
	break;

    case ATTRIB_RENAME:

	/* Make sure the new name doesn't already exist */

	va2 = atr_str(value);
	if (va2) {
	    notify(player,
		   "An attribute with that name already exists.");
	    free_sbuf(buff);
	    return;
	}
	if (vattr_rename(va->name, value) == NULL)
	    notify(player, "Attribute rename failed.");
	else
	    notify(player, "Attribute renamed.");
	break;

    case ATTRIB_DELETE:
        if ( va->command_flag > 0 ) {
           notify(player, unsafe_tprintf("Attribute %s is command-accessed %d times.  Can not delete.", va->name, va->command_flag));
           break;
        }

        /* Let's verify the attribute is not in use first */
        s_chkattr = alloc_lbuf("attribute_delete");
        DO_WHOLE_DB(i) {
           atr_get_str(s_chkattr, i, va->number, &aowner, &aflags);
           if ( s_chkattr && *s_chkattr ) {
              delcnt++;
           }
        }
        free_lbuf(s_chkattr);

	/* Remove the attribute */

        if ( delcnt == 0 ) {
	   vattr_delete(buff);
	   notify(player, "Attribute deleted.");
        } else {
           if ( (va->number < (A_USER_START - 1)) || (va->number >= A_INLINE_START) ) {
              notify(player, unsafe_tprintf("Attribute %s in use by %d dbref#'s, but is listed internal.  Deleting.", va->name, delcnt));
	      vattr_delete(buff);
	      notify(player, "Attribute deleted.");
           } else {
              notify(player, unsafe_tprintf("Attribute %s in use by %d dbref#'s.  Unable to delete.", va->name, delcnt));
           }
        }
	break;
    }

    if (key != ATTRIB_DELETE) {
	lbuff = alloc_lbuf("attrib_access");
        if ( va->command_flag > 0 ) {
	   sprintf(lbuff, "Access for %s [%d commands linked]:",
		   va->name, va->command_flag);
        } else {
	   sprintf(lbuff, "Access for %s:",
		   va->name);
        }
	listset_nametab(player, attraccess_nametab, 0,
			va->flags, 0, lbuff, 1);
	free_lbuf(lbuff);
    }
    free_sbuf(buff);
    return;
}
/* ---------------------------------------------------------------------------
 * do_fixdb: Directly edit database fields
 */

void 
do_fixdb(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
    dbref thing, res, aowner;
    char *s;
    char *s_types[]={ "room", "thing", "exit", "player", "zone", "garbage", "unknown type 1", "unknown type 2", NULL };
    int aflags, i_type;

    if ( mudstate.remotep != NOTHING ) {
       notify(player, "You can't fix the db remotely.");
       return;
    }
    init_match(player, arg1, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
	return;

    res = NOTHING;
    switch (key) {
    case FIXDB_OWNER:
    case FIXDB_LOC:
    case FIXDB_CON:
    case FIXDB_EXITS:
    case FIXDB_NEXT:
	init_match(player, arg2, NOTYPE);
	match_everything(0);
	res = noisy_match_result();
	break;
    case FIXDB_PENNIES:
	res = atoi(arg2);
	break;
    }

    switch (key) {
    case FIXDB_TYPE:
        i_type = atoi(arg2);
        if ( (i_type < 0) || (i_type > TYPE_MASK) ) {
           notify(player, "Unrecognized type definition specified.");
           break;
        }
        if ( (i_type == TYPE_PLAYER) || (Typeof(thing) == TYPE_PLAYER) ) {
	   s = atr_get(thing, A_PASS, &aowner, &aflags);
           if ( !s || !*s ) {
              notify(player, "You can not change a player into a non-player.");
              free_lbuf(s);
              break;
           } else if ( i_type != TYPE_PLAYER ) {
              notify(player, "You can not change a non-player into a player.");
              free_lbuf(s);
              break;
           }
           s_Flags(thing, (Flags(thing) & ~TYPE_MASK) | TYPE_PLAYER);
           notify(player, unsafe_tprintf("Converted #%d from type %s [id-type %d] to %s [id-type %d]", 
                  thing, s_types[Flags(thing) & TYPE_MASK], Flags(thing) & TYPE_MASK, 
                  s_types[i_type], i_type));
           free_lbuf(s);
        } else {
           if ( i_type <= 5 ) {
              notify(player, unsafe_tprintf("Converted #%d from type %s [id-type %d] to %s [id-type %d]", 
                     thing, s_types[Flags(thing) & TYPE_MASK], Flags(thing) & TYPE_MASK, 
                     s_types[i_type], i_type));
           s_Flags(thing, (Flags(thing) & ~TYPE_MASK) | i_type);
           } else {
              notify(player, "Invalid type specified.  Can not convert.");
           }
        }
        break;
    case FIXDB_OWNER:
	s_Owner(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Owner set to #%d", res));
	break;
    case FIXDB_LOC:
	s_Location(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Location set to #%d", res));
	break;
    case FIXDB_CON:
	s_Contents(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Contents set to #%d", res));
	break;
    case FIXDB_EXITS:
	s_Exits(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Exits set to #%d", res));
	break;
    case FIXDB_NEXT:
	s_Next(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Next set to #%d", res));
	break;
    case FIXDB_PENNIES:
	s_Pennies(thing, res);
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Pennies set to %d", res));
	break;
    case FIXDB_NAME:
	if (Typeof(thing) == TYPE_PLAYER) {
	    if (!ok_player_name(arg2)) {
		notify(player,
		       "That's not a good name for a player.");
		return;
	    }
	    if (lookup_player(NOTHING, arg2, 0) != NOTHING) {
		notify(player,
		       "That name is already in use.");
		return;
	    }
	    STARTLOG(LOG_SECURITY, "SEC", "CNAME")
		log_name(thing),
		log_text((char *) " renamed to ");
	        log_text(arg2);
	    ENDLOG
		if (Suspect(player)) {
		raw_broadcast(0, WIZARD,
			      "[Suspect] %s renamed to %s",
			      Name(thing), arg2);
	    }
	    delete_player_name(thing, Name(thing));
	    s_Name(thing, arg2);
	    add_player_name(thing, arg2);
	} else {
	    if (!ok_name(arg2)) {
		notify(player,
		       "Warning: That is not a reasonable name.");
	    }
	    s_Name(thing, arg2);
	}
	if (!Quiet(player))
	    notify(player, unsafe_tprintf("Name set to %s", arg2));
	break;
    }
}

/* ---------------------------------------------------------------------------
 * init_attrtab: Initialize the attribute hash tables.
 */

void 
NDECL(init_attrtab)
{
    ATTR *a;
    char *buff, *p, *q;

    hashinit(&mudstate.attr_name_htab, 521);
    buff = alloc_sbuf("init_attrtab");
    for (a = attr; a->number; a++) {
        /* Anum extend handles outside ranges but won't abort */
	anum_extend(a->number);
        if ( (a->number < 0) || (a->number > A_INLINE_END) ) {
           continue;
        }
	anum_set(a->number, a);
	for (p = buff, q = (char *) a->name; *q; p++, q++)
	    *p = ToLower((int)*q);
	*p = '\0';
	hashadd2(buff, (int *) a, &mudstate.attr_name_htab, 1);
    }
    free_sbuf(buff);
}

/* ---------------------------------------------------------------------------
 * atr_str: Look up an attribute by name.
 */

ATTR *
atr_str_cluster(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_objid");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_objid(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_objid");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_exec(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_exec");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_atrpeval(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_atrpeval");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_parseatr(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_parseatr");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_notify(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str_notify");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_mtch(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str4");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str4(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str4");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str3(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str3");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str2(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str2");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr and return a pointer to it. */

    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_str_bool(char *s)
{
    char *buff, *p, *q;
    ATTR *a;
    VATTR *va;
    static ATTR tattr2;

    /* Convert the buffer name to lowercase */

    buff = alloc_mbuf("atr_str");
    for (p = buff, q = s; *q && ((p - buff) < (MBUF_SIZE - 1)); p++, q++)
	*p = ToLower((int)*q);
    *p = '\0';

    /* Look for a predefined attribute */

    a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
    if (a != NULL) {
	free_mbuf(buff);
	return a;
    }
    /* Nope, look for a user attribute */

    if ( mudstate.nolookie )
       va = NULL;
    else
       va = (VATTR *) vattr_find(buff);
    free_mbuf(buff);

    /* If we got one, load tattr2 and return a pointer to it. */

    if (va != NULL) {
	tattr2.name = va->name;
	tattr2.number = va->number;
	tattr2.flags = va->flags;
	tattr2.check = NULL;
	return &tattr2;
    }
    /* All failed, return NULL */

    return NULL;
}

#else /* STANDALONE */

/* We don't have the hash tables, do it by hand */

/* ---------------------------------------------------------------------------
 * atr_str: Look up an attribute by name.
 */

ATTR *
atr_str(char *s)
{
    ATTR *ap;
    VATTR *va;
    static ATTR tattr;

    /* Check for an exact match on a predefined attribute */
    for (ap = attr; ap->name; ap++) {
	if (!string_compare(ap->name, s))
	    return ap;
    }

    /* Check for an exact match on a user-named attribute */

    va = (VATTR *) vattr_find(s);
    if (va != NULL) {

	/* Got it */

	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* No exact match, try for a prefix match on predefined attribs.
     * Check for both longer versions and shorter versions.
     */

    for (ap = attr; ap->name; ap++) {
	if (string_prefix(s, ap->name))
	    return ap;
	if (string_prefix(ap->name, s))
	    return ap;
    }

    return NULL;
}

ATTR *
atr_str_bool(char *s)
{
   return atr_str(s);
}

#endif /* STANDALONE */

/* ---------------------------------------------------------------------------
 * anum_extend: Grow the attr num lookup table.
 */

ATTR **anum_table = NULL;
ATTR **anum_table_inline = NULL;
int anum_alc_top = 0;
int anum_alc_inline_top = 0;

void 
anum_extend(int newtop)
{
    ATTR **anum_table2;
    int delta, i;

#ifndef STANDALONE
    delta = mudconf.init_size;
#else
    delta = 1000;
#endif
    if ( newtop >= A_INLINE_START ) {
       if ( newtop <= anum_alc_inline_top )
          return;

       /* This shouldn't happen but if the src is modified for this abort it */
       if ( newtop >= A_INLINE_END) {
	  STARTLOG(LOG_ALWAYS, "ATTR", "INLINE")
             log_text("Built-in attribute number ");
             log_unsigned(newtop);
             log_text(" outside of range.");
          ENDLOG
          return;
       }
       if (anum_table_inline == NULL) {
	   anum_table_inline = (ATTR **) malloc(((newtop - A_INLINE_START) + 1) * sizeof(ATTR *));
	   for (i = 0; i <= (newtop - A_INLINE_START); i++)
	       anum_table_inline[i] = NULL;
       } else {
	   anum_table2 = (ATTR **) malloc(((newtop - A_INLINE_START) + 1) * sizeof(ATTR *));
	   for (i = 0; i <= (anum_alc_inline_top - A_INLINE_START); i++)
	       anum_table2[i] = anum_table_inline[i];
	   for (i = anum_alc_inline_top + 1; i <= (newtop - A_INLINE_START); i++)
	       anum_table2[i] = NULL;
	   free((char *) anum_table_inline);
	   anum_table_inline = anum_table2;
       }
       anum_alc_inline_top = newtop;
    } else if ( newtop < 0 ) {
       /* This shouldn't happen but if the src is modified for this abort it */
       STARTLOG(LOG_ALWAYS, "ATTR", "INLINE")
          log_text("Built-in attribute number ");
          log_unsigned(newtop);
          log_text(" outside of range.");
       ENDLOG
       return;
    } else {
       if (newtop <= anum_alc_top)
	   return;
       if (newtop < anum_alc_top + delta)
	   newtop = anum_alc_top + delta;
       if (anum_table == NULL) {
	   anum_table = (ATTR **) malloc((newtop + 1) * sizeof(ATTR *));
	   for (i = 0; i <= newtop; i++)
	       anum_table[i] = NULL;
       } else {
	   anum_table2 = (ATTR **) malloc((newtop + 1) * sizeof(ATTR *));
	   for (i = 0; i <= anum_alc_top; i++)
	       anum_table2[i] = anum_table[i];
	   for (i = anum_alc_top + 1; i <= newtop; i++)
	       anum_table2[i] = NULL;
	   free((char *) anum_table);
	   anum_table = anum_table2;
       }
       anum_alc_top = newtop;
    }
}

/* ---------------------------------------------------------------------------
 * atr_num: Look up an attribute by number.
 */
ATTR *
atr_num_mtch(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_chkpass(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_objid(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_exec(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_aladd(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_pinfo(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_ex(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num4(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num3(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num2(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_lattr(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_vattr(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num(int anum)
{
    VATTR *va;
    static ATTR tattr;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr.name = va->name;
	tattr.number = va->number;
	tattr.flags = va->flags;
	tattr.check = NULL;
	return &tattr;
    }
    /* All failed, return NULL */

    return NULL;
}

ATTR *
atr_num_bool(int anum)
{
    VATTR *va;
    static ATTR tattr2;

    /* Look for a predefined attribute */
    if ((anum < 0) || (anum >= A_INLINE_END))
	return NULL;

    if (anum < A_USER_START)
	return anum_get(anum);
    if (anum >= A_INLINE_START)
	return anum_get(anum);

    if (anum > anum_alc_top)
	return NULL;

    /* It's a user-defined attribute, we need to copy data */

    va = (VATTR *) anum_get(anum);
    if (va != NULL) {
	tattr2.name = va->name;
	tattr2.number = va->number;
	tattr2.flags = va->flags;
	tattr2.check = NULL;
	return &tattr2;
    }
    /* All failed, return NULL */

    return NULL;
}

/* ---------------------------------------------------------------------------
 * mkattr: Lookup attribute by name, creating if needed.
 */

int 
mkattr(char *buff)
{
    ATTR *ap;
    VATTR *va;
    mudstate.new_vattr = 0;
    if (!(ap = atr_str(buff))) {
	/* Unknown attr, create a new one */
        mudstate.new_vattr = 1;
	va = vattr_alloc(buff, mudconf.vattr_flags);
	if (!va || !(va->number))
	    return -1;
	return va->number;
    }
    if (!(ap->number))
	return -1;
    return ap->number;
}

/* ---------------------------------------------------------------------------
 * al_decode: Fetch an attribute number from an alist.
 */

static int 
al_decode(char **app)
{
    int atrnum = 0, anum, atrshft = 0;
    char *ap;

    ap = *app;

    for (;;) {
	anum = ((*ap) & 0x7f);
	if (atrshft > 0)
	    atrnum += (anum << atrshft);
	else
	    atrnum = anum;
	if (!(*ap++ & 0x80)) {
	    *app = ap;
	    return atrnum;
	}
	atrshft += 7;
    }
    /*NOTREACHED */
}

/* ---------------------------------------------------------------------------
 * al_code: Store an attribute number in an alist.
 */

static void 
al_code(char **app, int atrnum)
{
    char *ap;

    ap = *app;

    for (;;) {
	*ap = atrnum & 0x7f;
	atrnum = atrnum >> 7;
	if (!atrnum) {
	    *app = ++ap;
	    return;
	}
	*ap++ |= 0x80;
    }
}

/* ---------------------------------------------------------------------------
 * Commer: check if an object has any $-commands in its attributes.
 */

int 
Commer(dbref thing)
{
    char *s, *as, c;
    int attr, aflags;
    dbref aowner;
    ATTR *ap;

    atr_push();
    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
	ap = atr_num(attr);
	if (!ap || (ap->flags & AF_NOPROG))
	    continue;

	s = atr_get(thing, attr, &aowner, &aflags);
	c = *s;
	free_lbuf(s);
	if ((c == '$') && !(aflags & AF_NOPROG)) {
	    atr_pop();
	    return 1;
	}
    }
    atr_pop();
    return 0;
}

/* routines to handle object attribute lists */

/* ---------------------------------------------------------------------------
 * al_size, al_fetch, al_store, al_add, al_delete: Manipulate attribute lists
 */

/* al_extend: Get more space for attributes, if needed */

void 
al_extend(char **buffer, int *bufsiz, int len, int copy)
{
    char *tbuff;
    int  i_newsize;

    if (len > *bufsiz) {
	i_newsize = len + ATR_BUF_CHUNK;
	tbuff = XMALLOC(i_newsize, "al_extend");
        if ( !tbuff ) {
	   STARTLOG(LOG_ALWAYS, "MEMORY", "ALLOCATION")
	      log_text("al_extend: Unable to allocate chunk memory for new attribute.  Rebooting...");
           ENDLOG
#ifndef STANDALONE
           raw_broadcast(0, 0, "Game: Memory allocation error.  Attempting reboot...");
           do_reboot(GOD, GOD, 0);
#endif
        }
	if (*buffer) {
	    if (copy)
               memcpy(tbuff, *buffer, *bufsiz);
/*		bcopy(*buffer, tbuff, *bufsiz); */
	    XFREE(*buffer, "al_extend");
	}
	*buffer = tbuff;
        *bufsiz = i_newsize;
    }
}

/* al_size: Return length of attribute list in chars */

int 
al_size(char *astr)
{
    if (!astr)
	return 0;
    return (strlen(astr) + 1);
}

/* al_store: Write modified attribute list */

void 
NDECL(al_store)
{
    if (mudstate.mod_al_id != NOTHING) {
	if (mudstate.mod_alist && *mudstate.mod_alist) {
	    atr_add_raw(mudstate.mod_al_id, A_LIST,
			mudstate.mod_alist);
	} else {
	    atr_clr(mudstate.mod_al_id, A_LIST);
	}
    }
    mudstate.mod_al_id = NOTHING;
}

/* al_fetch: Load attribute list */

char *
al_fetch(dbref thing)
{
    char *astr;
    int len;

    /* We only need fetch if we change things */

    if (mudstate.mod_al_id == thing)
	return mudstate.mod_alist;

    /* Save old list, then fetch and set up the attribute list */

    al_store();
    astr = atr_get_raw(thing, A_LIST);
    if (astr) {
	len = al_size(astr);
	al_extend(&mudstate.mod_alist, &mudstate.mod_size, len, 0);
        memcpy((char *) mudstate.mod_alist, (char *) astr, len);
/*	bcopy((char *) astr, (char *) mudstate.mod_alist, len); */
    } else {
	al_extend(&mudstate.mod_alist, &mudstate.mod_size, 1, 0);
	*mudstate.mod_alist = '\0';
    }
    mudstate.mod_al_id = thing;
    return mudstate.mod_alist;
}

/* al_add: Add an attribute to an attribute list */

void 
al_add(dbref thing, int attrnum)
{
    char *abuf, *cp, *s_mbuf;
    int anum, do_limit_add;
    dbref player;
#ifndef STANDALONE
    ATTR *attr;
    dbref aowner2;
    int i, i_array[LIMIT_MAX], aflags2;
    char *s_chkattr, *s_buffptr, *tstrtokr;
#endif
    /* If trying to modify List attrib, return.  Otherwise, get the
     * attribute list. */

    player = NOTHING;
    s_mbuf = NULL;
    if (attrnum == A_LIST)
	return;
    abuf = al_fetch(thing);

    /* See if attr is in the list.  If so, exit (need not do anything) */

    cp = abuf;
    while (*cp) {
	anum = al_decode(&cp);
	if (anum == attrnum)
	    return;
    }
    if ((attrnum >= A_USER_START) && (attrnum < A_INLINE_START) && (db[thing].nvattr >= mudconf.vlimit)) {
      if ( 
#ifndef STANDALONE
          !mudstate.dbloading &&
#endif
          (mudstate.vlplay != NOTHING) ) {
#ifndef STANDALONE
	attr = atr_num_aladd(attrnum);
        notify_quiet(mudstate.vlplay,"Variable attribute limit reached.");
	STARTLOG(LOG_SECURITY, "SEC", "VLIMIT")
	  log_text("Variable attribute limit reached -> Player: ");
	  log_name(mudstate.vlplay);
	  log_text(" Object: ");
	  log_name(thing);
	  if (attr) {
	    log_text(", Attribute: ");
	    log_text((char *)attr->name);
	  }
	ENDLOG

	if (attr) {
	  broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"VARIABLE ATTRIBUTE LIMIT",
		(char *)attr->name, NULL, thing, 0, 0, NULL);
	}
	else {
	  broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"VARIABLE ATTRIBUTE LIMIT",
		NULL, NULL, thing, 0, 0, NULL);
	}
        if ( attr && mudstate.new_vattr) {
           vattr_delete((char *)attr->name);
           mudstate.new_vattr = 0;
        }
#endif
        mudstate.vlplay = NOTHING;
      }
      return;
    }
    do_limit_add = 0;
    if ( 
#ifndef STANDALONE
         !mudstate.dbloading && 
#endif
         (attrnum > A_USER_START) && (attrnum < A_INLINE_START) && mudstate.new_vattr && 
         !((mudstate.vlplay != NOTHING) && 
          ((Wizard(mudstate.vlplay) || (Good_obj(Owner(mudstate.vlplay)) && Wizard(Owner(mudstate.vlplay)))) && 
         !mudconf.vattr_limit_checkwiz)) ) {
#ifndef STANDALONE
       player = NOTHING;
       if ( Good_obj(mudstate.vlplay) ) {
          if ( isPlayer(mudstate.vlplay) ) {
             player = mudstate.vlplay;
          } else {
             player = Owner(mudstate.vlplay);
             if ( !(Good_obj(player) && isPlayer(player)) )
                player = NOTHING;
          }
       }
       if ( player != NOTHING ) {
	  attr = atr_num_aladd(attrnum);
          s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
          if ( *s_chkattr ) {
             i_array[0] = i_array[2] = 0;
             i_array[4] = i_array[1] = i_array[3] = -2;
             for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &tstrtokr), i = 0;
                  s_buffptr && (i < LIMIT_MAX);
                  s_buffptr = (char *) strtok_r(NULL, " ", &tstrtokr), i++) {
                 i_array[i] = atoi(s_buffptr);
             }
             if ( (i_array[1] != -1) && !((i_array[1] == -2) && ((Wizard(player) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit) == -1)) ) {
                if ( (i_array[0]+1) > 
                     (i_array[1] == -2 ? (Wizard(player) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit) : i_array[1]) ) {
                   notify_quiet(mudstate.vlplay,"Variable attribute new creation maximum reached.");
                   STARTLOG(LOG_SECURITY, "SEC", "VMAXIMUM")
                     log_text("Variable attribute new creation maximum reached -> Player: ");
                     log_name(mudstate.vlplay);
                     log_text(" Object: ");
                     log_name(thing);
                     if (attr) {
                       log_text(", Attribute: ");
                       log_text((char *)attr->name);
                     }
                   ENDLOG
                   if (attr) {
                     broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"CREATE V-ATTRIBUTE MAXIMUM",
                           (char *)attr->name, NULL, thing, 0, 0, NULL);
                   }
                   else {
                     broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"CREATE V-ATTRIBUTE MAXIMUM",
                           NULL, NULL, thing, 0, 0, NULL);
                   }
                   if ( attr && mudstate.new_vattr) {
                      vattr_delete((char *)attr->name);
                      mudstate.new_vattr = 0;
                   }
                   mudstate.vlplay = NOTHING;
                   free_lbuf(s_chkattr); 
                   return;
                }
             }
             s_mbuf = alloc_mbuf("vattr_check");
             sprintf(s_mbuf, "%d %d %d %d %d", i_array[0]+1, i_array[1], 
                                                  i_array[2], i_array[3], i_array[4]);
             do_limit_add = 1;
          } else {
             s_mbuf = alloc_mbuf("vattr_check");
             sprintf(s_mbuf, "1 -2 0 -2 %d", -2);
             do_limit_add = 1;
          }
          free_lbuf(s_chkattr);
       }
    }
    if ( !mudstate.dbloading && (attrnum > A_USER_MAXIMUM) && (attrnum < A_INLINE_START) ) {
       attr = atr_num_aladd(attrnum);
       broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"V-ATTRIBUTE CEILING REACHED",
                         NULL, NULL, thing, 0, 0, NULL);
       STARTLOG(LOG_SECURITY, "SEC", "VCEILING")
       log_text("Variable attribute new creation maximum reached -> Player: ");
       log_name(mudstate.vlplay);
       log_text(" Object: ");
       log_name(thing);
       if (attr) {
          log_text(", Attribute: ");
          log_text((char *)attr->name);
       }
       ENDLOG
       if ( attr && mudstate.new_vattr) {
          vattr_delete((char *)attr->name);
          mudstate.new_vattr = 0;
       }
    }
#else
       mudstate.vlplay = NOTHING;
       return;
    }
#endif
    /* Nope, extend it */

    al_extend(&mudstate.mod_alist, &mudstate.mod_size,
	      (cp - abuf + ATR_BUF_INCR), 1);
    if (mudstate.mod_alist != abuf) {

	/* extend returned different buffer, re-find the end */

	abuf = mudstate.mod_alist;
	for (cp = abuf; *cp; anum = al_decode(&cp));
    }
    /* Add the new attribute on to the end */

    al_code(&cp, attrnum);
    *cp = '\0';
    if ( (attrnum >= A_USER_START) && (attrnum < A_INLINE_START) )
      (db[thing].nvattr)++;
    if ( do_limit_add ) {
       atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
       free_mbuf(s_mbuf);
    }
    return;
}

/* al_delete: Remove an attribute from an attribute list */

void 
al_delete(dbref thing, int attrnum)
{
    int anum;
    char *abuf, *cp, *dp;
#ifndef STANDALONE
    ATTR *attr;
    char *s_chkattr, *s_buffptr, *s_mbuf, *tstrtokr;
    dbref aowner2, player;
    int aflags2, i_array[LIMIT_MAX], i;
#endif
    /* If trying to modify List attrib, return.  Otherwise, get the
     * attribute list. */

    if (attrnum == A_LIST)
	return;
    abuf = al_fetch(thing);
    if (!abuf)
	return;

    cp = abuf;
    while (*cp) {
	dp = cp;
	anum = al_decode(&cp);
	if (anum == attrnum) {
	    if ( (anum >= A_USER_START) && (anum < A_INLINE_START) )
	      (db[thing].nvattr)--;
	    while (*cp) {
		anum = al_decode(&cp);
		al_code(&dp, anum);
	    }
	    *dp = '\0';
	    break;
	}
    }

    if ( (attrnum > A_USER_START) && (attrnum < A_INLINE_START) && mudstate.new_vattr && 
         !((mudstate.vlplay != NOTHING) && 
          ((Wizard(mudstate.vlplay) || (Good_obj(Owner(mudstate.vlplay)) && Wizard(Owner(mudstate.vlplay)))) && 
         !mudconf.vattr_limit_checkwiz)) ) {
#ifndef STANDALONE
       player = NOTHING;
       if ( Good_obj(mudstate.vlplay) ) {
          if ( isPlayer(mudstate.vlplay) ) {
             player = mudstate.vlplay;
          } else {
             player = Owner(mudstate.vlplay);
             if ( !(Good_obj(player) && isPlayer(player)) )
                player = NOTHING;
          }
       }
       if ( player != NOTHING ) {
	  attr = atr_num_aladd(attrnum);
          s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
          if ( *s_chkattr ) {
             i_array[0] = i_array[2] = 0;
             i_array[4] = i_array[1] = i_array[3] = -2;
             for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &tstrtokr), i = 0;
                  s_buffptr && (i < LIMIT_MAX);
                  s_buffptr = (char *) strtok_r(NULL, " ", &tstrtokr), i++) {
                 i_array[i] = atoi(s_buffptr);
             }
             if ( (i_array[1] != -1) && !((i_array[1] == -2) && ((Wizard(mudstate.vlplay) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit) == -1)) ) {
                if ( (i_array[0]+1) > 
                     (i_array[1] == -2 ? (Wizard(mudstate.vlplay) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit) : i_array[1]) ) {
                   notify_quiet(mudstate.vlplay,"Variable attribute new creation (del) maximum reached.");
                   STARTLOG(LOG_SECURITY, "SEC", "VMAXIMUM")
                     log_text("Variable attribute new creation (del) maximum reached -> Player: ");
                     log_name(mudstate.vlplay);
                     log_text(" Object: ");
                     log_name(thing);
                     if (attr) {
                       log_text(", Attribute: ");
                       log_text((char *)attr->name);
                     }
                   ENDLOG
                   if (attr) {
                     broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"CREATE(DEL) V-ATTRIBUTE MAXIMUM",
                           (char *)attr->name, NULL, thing, 0, 0, NULL);
                   }
                   else {
                     broadcast_monitor(mudstate.vlplay,MF_VLIMIT,"CREATE(DEL) V-ATTRIBUTE MAXIMUM",
                           NULL, NULL, thing, 0, 0, NULL);
                   }
                   if ( attr && mudstate.new_vattr) {
                      vattr_delete((char *)attr->name);
                      mudstate.new_vattr = 0;
                   }
                   mudstate.vlplay = NOTHING;
                   free_lbuf(s_chkattr); 
                   return;
                }
             }
             s_mbuf = alloc_mbuf("vattr_check");
             sprintf(s_mbuf, "%d %d %d %d %d", i_array[0]+1, i_array[1], 
                                                  i_array[2], i_array[3], i_array[4]);
             atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
             free_mbuf(s_mbuf);
          } else {
             s_mbuf = alloc_mbuf("vattr_check");
             sprintf(s_mbuf, "1 -2 0 -2 %d", -2);
             atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
             free_mbuf(s_mbuf);
          }
          free_lbuf(s_chkattr);
       }
    }
#else
       mudstate.vlplay = NOTHING;
       return;
    }
#endif


    return;
}

INLINE static void 
makekey(dbref thing, int atr, Aname * abuff)
{
    abuff->object = thing;
    abuff->attrnum = atr;
    return;
}

/* ---------------------------------------------------------------------------
 * al_destroy: wipe out an object's attribute list.
 */

void 
al_destroy(dbref thing)
{
    if (mudstate.mod_al_id == thing)
	al_store();		/* remove from cache */
    atr_clr(thing, A_LIST);
    if (Good_obj(thing))
      db[thing].nvattr = 0; 
}

/* ---------------------------------------------------------------------------
 * atr_encode: Encode an attribute string.
 */

static char *
atr_encode(char *iattr, dbref thing, dbref owner, int flags,
	   int atr)
{
    /* Compress the attribute string */

    iattr = (char *) compress(iattr, atr);

    /* If using the default owner and flags (almost all attributes will),
     * just store the string.
     */

    if (((owner == Owner(thing)) || (owner == NOTHING)) && !flags)
	return iattr;

    /* Encode owner and flags into the attribute text */

    if (owner == NOTHING)
	owner = Owner(thing);
    return unsafe_tprintf("%c%d:%d:%s", ATR_INFO_CHAR, owner, flags, iattr);
}

/* ---------------------------------------------------------------------------
 * atr_decode: Decode an attribute string.
 */

static void 
atr_decode(char *iattr, char *oattr, dbref thing, dbref * owner,
	   int *flags, int atr)
{
    char *cp;
    int neg;

    /* See if the first char of the attribute is the special character */

    if (*iattr == ATR_INFO_CHAR) {

	/* It is, crack the attr apart */

	cp = &iattr[1];

	/* Get the attribute owner */

	*owner = 0;
	neg = 0;
	if (*cp == '-') {
	    neg = 1;
	    cp++;
	}
	while (isdigit((int)*cp)) {
	    *owner = (*owner * 10) + (*cp++ - '0');
	}
	if (neg)
	    *owner = 0 - *owner;

	/* If delimiter is not ':', just return attribute */

	if (*cp++ != ':') {
	    *owner = Owner(thing);
	    *flags = 0;
	    if (oattr)
		uncompress_str(oattr, iattr, atr);
	    return;
	}
	/* Get the attribute flags */

	*flags = 0;
	while (isdigit((int)*cp)) {
	    *flags = (*flags * 10) + (*cp++ - '0');
	}

	/* If delimiter is not ':', just return attribute */

	if (*cp++ != ':') {
	    *owner = Owner(thing);
	    *flags = 0;
	    if (oattr)
		uncompress_str(oattr, iattr, atr);

	}
	/* Get the attribute text */

	if (oattr)
	    uncompress_str(oattr, cp, atr);

	if (*owner == NOTHING)
	    *owner = Owner(thing);
    } else {

	/* Not the special character, return normal info */

	*owner = Owner(thing);
	*flags = 0;
	if (oattr)
	    uncompress_str(oattr, iattr, atr);
    }
}

/* ---------------------------------------------------------------------------
 * atr_clr: clear an attribute in the list.
 */

void 
atr_clr(dbref thing, int atr)
{
    Aname okey;

    makekey(thing, atr, &okey);
    DELETE(&okey);
    al_delete(thing, atr);
    switch (atr) {
    case A_STARTUP:
	s_Flags(thing, Flags(thing) & ~HAS_STARTUP);
	break;
    case A_PROTECTNAME:
	s_Flags4(thing, Flags4(thing) & ~HAS_PROTECT);
	break;
    case A_FORWARDLIST:
	s_Flags2(thing, Flags2(thing) & ~HAS_FWDLIST);
#ifndef STANDALONE
        fwdlist_clr(thing);
#endif
	break;
    case A_LISTEN:
	s_Flags2(thing, Flags2(thing) & ~HAS_LISTEN);
	break;
#ifndef STANDALONE
    case A_TIMEOUT:
	desc_reload(thing);
	break;
    case A_QUEUEMAX:
	pcache_reload(thing);
	break;
#endif
    case A_OBJECTTAG:
	s_Flags4(thing, Flags4(thing) & ~HAS_OBJECTTAG);
	break;
    }
}

/* ---------------------------------------------------------------------------
 * atr_add_raw, atr_add: add attribute of type atr to list
 */

void 
atr_add_raw(dbref thing, int atr, char *buff)
{
    Attr *a;
    Aname okey;

    makekey(thing, atr, &okey);
    if (!buff || !*buff) {
	DELETE(&okey);
	al_delete(thing, atr);
	return;
    }
    if ((a = (Attr *) malloc(strlen(buff) + 1)) == (char *) 0) {
	return;
    }
    strcpy(a, buff);

    STORE(&okey, a);
    al_add(thing, atr);
    switch (atr) {
    case A_STARTUP:
	s_Flags(thing, Flags(thing) | HAS_STARTUP);
	break;
    case A_PROTECTNAME:
	s_Flags4(thing, Flags4(thing) | HAS_PROTECT);
	break;
    case A_FORWARDLIST:
	s_Flags2(thing, Flags2(thing) | HAS_FWDLIST);
	break;
    case A_LISTEN:
	s_Flags2(thing, Flags2(thing) | HAS_LISTEN);
	break;
#ifndef STANDALONE
    case A_TIMEOUT:
	desc_reload(thing);
	break;
    case A_QUEUEMAX:
	pcache_reload(thing);
	break;
#endif
    case A_OBJECTTAG:
	s_Flags4(thing, Flags4(thing) | HAS_OBJECTTAG);
	break;
    }
}

void 
atr_add(dbref thing, int atr, char *buff, dbref owner, int flags)
{
    char *tbuff;
#ifndef STANDALONE
    char *bufr;
    time_t tt;

    if ( mudconf.enable_tstamps && !NoTimestamp(thing) ) {
       time(&tt);
       bufr = (char *) ctime(&tt);
       bufr[strlen(bufr) - 1] = '\0';
       atr_add_raw(thing, A_MODIFY_TIME, bufr);
    }
#endif

    if (!buff || !*buff) {
	atr_clr(thing, atr);
    } else {
	tbuff = atr_encode(buff, thing, owner, flags, atr);
	atr_add_raw(thing, atr, tbuff);
    }
}

void 
atr_set_owner(dbref thing, int atr, dbref owner)
{
    dbref aowner;
    int aflags;
    char *buff;

    buff = atr_get(thing, atr, &aowner, &aflags);
    atr_add(thing, atr, buff, owner, aflags);
    free_lbuf(buff);
}

void 
atr_set_flags(dbref thing, int atr, dbref flags)
{
    dbref aowner;
    int aflags;
    char *buff;

    buff = atr_get(thing, atr, &aowner, &aflags);
    atr_add(thing, atr, buff, aowner, flags);
    free_lbuf(buff);
}

/* ---------------------------------------------------------------------------
 * atr_get_raw, atr_get_str, atr_get: Get an attribute from the database.
 */

// Global LBUF size static buffer for large LBUF -> small LBUF conversion.
Attr tmp_lbuf[LBUF_SIZE];
//
char *
atr_get_raw(dbref thing, int atr)
{
    Attr *a;
    Aname okey;

#ifndef STANDALONE
    mudstate.attribfetchcount++;
#endif

    makekey(thing, atr, &okey);
    a = FETCH(&okey);
    if(a != NULL)
      if(strlen(a) > (LBUF_SIZE-1))
      {
        memset(tmp_lbuf, '\0', LBUF_SIZE);
        strncpy(tmp_lbuf,a,LBUF_SIZE-1);
        return tmp_lbuf;
      }
    return a;
}

char *
atr_get_str(char *s, dbref thing, int atr, dbref * owner, int *flags)
{
    char *buff;

    buff = atr_get_raw(thing, atr);
    if (!buff) {
      *owner = Owner(thing);
      *flags = 0;
      *s = '\0';
    } else {
	atr_decode(buff, s, thing, owner, flags, atr);
    }
    return s;
}

char *
atr_get_ash(dbref thing, int atr, dbref * owner, int *flags, int line_num, char *file_name)
{
    char *buff;

/*  buff = alloc_lbuf("atr_get"); */
    buff = alloc_lbuf_new("atr_get", line_num, file_name);
    return atr_get_str(buff, thing, atr, owner, flags);
}

int 
atr_get_info(dbref thing, int atr, dbref * owner, int *flags)
{
    char *buff;

    buff = atr_get_raw(thing, atr);
    if (!buff) {
	*owner = Owner(thing);
	*flags = 0;
	return 0;
    }
    atr_decode(buff, NULL, thing, owner, flags, atr);
    return 1;
}

#ifndef STANDALONE
char *
atr_pget_str_globalchk(char *s, dbref thing, int atr, dbref * owner, int *flags, int *retobj)
{
    char *buff;
    dbref parent;
    int lev, i_player, i_chk, i_wiz=0, i_lev=-1;
    ATTR *ap;
    ZLISTNODE *z_ptr;

    i_chk = 0;
    if ( *retobj != -1 ) {
       i_player = *retobj;
       if ( Good_obj(i_player))
       {
          if(Wizard(i_player))
             i_wiz=1;
          i_lev = obj_bitlevel(i_player);
          i_chk = 1;
       }
       *retobj = -1;
    }

    ITER_PARENTS(thing, parent, lev) {
        if ( i_chk && NoEx(parent) && !i_wiz && (obj_noexlevel(parent) > i_lev)  && (parent != thing) )
            break;
	buff = atr_get_raw(parent, atr);
	if (buff && *buff) {
	    atr_decode(buff, s, thing, owner, flags, atr);
	    if ((lev == 0) || !(*flags & AF_PRIVATE)) {
                *retobj = parent;
		return s;
            }
	}
	if ((lev == 0) && Good_obj(Parent(parent))) {
	    ap = atr_num_pinfo(atr);
	    if (!ap || ap->flags & AF_PRIVATE)
		break;
	}
    }
#ifndef STANDALONE
    /* First, inherit from the zonemaster, if enabled */
    if ( mudconf.zone_parents && Good_obj(thing) && !ZoneMaster(thing) && !NoZoneParent(thing) ) {
       for ( z_ptr = db[thing].zonelist; z_ptr; z_ptr = z_ptr->next ) {
          if ( ZoneParent(z_ptr->object) ) {
	     buff = atr_get_raw(z_ptr->object, atr);
	     if (buff && *buff) {
	         atr_decode(buff, s, thing, owner, flags, atr);
		 if ( !(*flags & AF_PRIVATE)) {
		   return s;
		 }
	     }
          }
       }
    }
#endif
    *s = '\0';
    return s;
}

char *
atr_pget_str(char *s, dbref thing, int atr, dbref * owner, int *flags, int *retobj)
{
    char *buff;
    dbref parent, gbl_parent;
    int lev, i_player, i_chk, i_wiz=0, i_lev=-1;
    ATTR *ap;
    ZLISTNODE *z_ptr;

    i_chk = 0;
    if ( *retobj != -1 ) {
       i_player = *retobj;
       if ( Good_obj(i_player))
       {
          if(Wizard(i_player))
             i_wiz=1;
          i_lev = obj_bitlevel(i_player);
          i_chk = 1;
       }
       *retobj = -1;
    }

    ITER_PARENTS(thing, parent, lev) {
        if ( i_chk && NoEx(parent) && !i_wiz && (obj_noexlevel(parent) > i_lev)  && (parent != thing) )
            break;
	buff = atr_get_raw(parent, atr);
	if (buff && *buff) {
	    atr_decode(buff, s, thing, owner, flags, atr);
	    if ((lev == 0) || !(*flags & AF_PRIVATE)) {
                *retobj = parent;
		return s;
            }
	}
	if ((lev == 0) && Good_obj(Parent(parent))) {
	    ap = atr_num_pinfo(atr);
	    if (!ap || ap->flags & AF_PRIVATE)
		break;
	}
    }
#ifndef STANDALONE
    /* First, inherit from the zonemaster, if enabled */
    if ( mudconf.zone_parents && Good_obj(thing) && !ZoneMaster(thing) && !NoZoneParent(thing) ) {
       for ( z_ptr = db[thing].zonelist; z_ptr; z_ptr = z_ptr->next ) {
          if ( ZoneParent(z_ptr->object) ) {
	     buff = atr_get_raw(z_ptr->object, atr);
	     if (buff && *buff) {
	         atr_decode(buff, s, thing, owner, flags, atr);
		 if ( !(*flags & AF_PRIVATE)) {
		   return s;
		 }
	     }
          }
       }
    }
    if ( Good_obj(thing) && !NoGlobParent(thing) ) {
       switch ( Typeof(thing) ) {
          case TYPE_ROOM :   if ( Good_obj(mudconf.global_parent_room) )
                           gbl_parent = mudconf.global_parent_room;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_PLAYER : if ( Good_obj(mudconf.global_parent_player) )
                           gbl_parent = mudconf.global_parent_player;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_THING :  if ( Good_obj(mudconf.global_parent_thing) )
                           gbl_parent = mudconf.global_parent_thing;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_EXIT :   if ( Good_obj(mudconf.global_parent_exit) )
                           gbl_parent = mudconf.global_parent_exit;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          default:      gbl_parent = mudconf.global_parent_obj;
                        break;
       }
    } else {
       gbl_parent = NOTHING;
    }
    /* Now, check if there's a global parent */
    if ( Good_obj(gbl_parent) &&
         !Recover(gbl_parent) &&
         !Going(gbl_parent) &&
         (gbl_parent != thing) ) {
	buff = atr_get_raw(gbl_parent, atr);
        if ( buff && *buff ) {
	    atr_decode(buff, s, thing, owner, flags, atr);
            if ( !(*flags & AF_PRIVATE)) {
               return s;
            }
        } else {
           s = atr_pget_str_globalchk(s, gbl_parent, atr, owner, flags, retobj);
           if ( s && *s ) {
              return s;
           }
        }
    }
#endif
    *owner = Owner(thing);
    *flags = 0;
    *s = '\0';
    return s;
}

char *
atr_pget_ash(dbref thing, int atr, dbref * owner, int *flags, int line_num, char *file_name)
{
    char *buff;
    int ibf=-1;

    buff = alloc_lbuf_new("atr_pget", line_num, file_name);
    return atr_pget_str(buff, thing, atr, owner, flags, &ibf);
}

int 
atr_pget_info_globalchk(dbref thing, int atr, dbref * owner, int *flags)
{
    char *buff;
    dbref parent;
    int lev;
    ATTR *ap;
    ZLISTNODE *z_ptr;

    ITER_PARENTS(thing, parent, lev) {
	buff = atr_get_raw(parent, atr);
	if (buff && *buff) {
	    atr_decode(buff, NULL, thing, owner, flags, atr);
	    if ((lev == 0) || !(*flags & AF_PRIVATE))
		return 1;
	}
	if ((lev == 0) && Good_obj(Parent(parent))) {
	    ap = atr_num_pinfo(atr);
	    if (!ap || ap->flags & AF_PRIVATE)
		break;
	}
    }
#ifndef STANDALONE
    /* First, inherit from the zonemaster, if enabled */
    if ( mudconf.zone_parents && Good_obj(thing) && !ZoneMaster(thing) && !NoZoneParent(thing) ) {
       for ( z_ptr = db[thing].zonelist; z_ptr; z_ptr = z_ptr->next ) {
          if ( ZoneParent(z_ptr->object) ) {
	     buff = atr_get_raw(z_ptr->object, atr);
	     if (buff && *buff) {
	         atr_decode(buff, NULL, thing, owner, flags, atr);
		 if ( !(*flags & AF_PRIVATE) ) {
		   return 1;
		 }
	     }
          }
       }
    }
#endif
    return 0;
}

int 
atr_pget_info(dbref thing, int atr, dbref * owner, int *flags)
{
    char *buff;
    dbref parent, gbl_parent;
    int lev, i_return;
    ATTR *ap;
    ZLISTNODE *z_ptr;

    i_return = 0;

    ITER_PARENTS(thing, parent, lev) {
	buff = atr_get_raw(parent, atr);
	if (buff && *buff) {
	    atr_decode(buff, NULL, thing, owner, flags, atr);
	    if ((lev == 0) || !(*flags & AF_PRIVATE))
		return 1;
	}
	if ((lev == 0) && Good_obj(Parent(parent))) {
	    ap = atr_num_pinfo(atr);
	    if (!ap || ap->flags & AF_PRIVATE)
		break;
	}
    }
#ifndef STANDALONE
    /* First, inherit from the zonemaster, if enabled */
    if ( mudconf.zone_parents && Good_obj(thing) && !ZoneMaster(thing) && !NoZoneParent(thing) ) {
       for ( z_ptr = db[thing].zonelist; z_ptr; z_ptr = z_ptr->next ) {
          if ( ZoneParent(z_ptr->object) ) {
	     buff = atr_get_raw(z_ptr->object, atr);
	     if (buff && *buff) {
	         atr_decode(buff, NULL, thing, owner, flags, atr);
		 if ( !(*flags & AF_PRIVATE) ) {
		   return 1;
		 }
	     }
          }
       }
    }
    /* Now, check if there's a global parent */
    if ( Good_obj(thing) && !NoGlobParent(thing)) {
       switch ( Typeof(thing) ) {
          case TYPE_ROOM :   if ( Good_obj(mudconf.global_parent_room) )
                           gbl_parent = mudconf.global_parent_room;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_PLAYER : if ( Good_obj(mudconf.global_parent_player) )
                           gbl_parent = mudconf.global_parent_player;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_THING :  if ( Good_obj(mudconf.global_parent_thing) )
                           gbl_parent = mudconf.global_parent_thing;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          case TYPE_EXIT :   if ( Good_obj(mudconf.global_parent_exit) )
                           gbl_parent = mudconf.global_parent_exit;
                        else
                           gbl_parent = mudconf.global_parent_obj;
                        break;
          default:      gbl_parent = mudconf.global_parent_obj;
                        break;
       }
    } else {
       gbl_parent = NOTHING;
    }
    if ( Good_obj(gbl_parent) &&
         !Recover(gbl_parent) &&
         !Going(gbl_parent) &&
         (gbl_parent != thing) ) {
	buff = atr_get_raw(gbl_parent, atr);
        if ( buff && *buff ) {
	    atr_decode(buff, NULL, thing, owner, flags, atr);
	    return 1;
        } else {
           i_return = atr_pget_info_globalchk(gbl_parent, atr, owner, flags);
           if ( i_return ) {
              return i_return;
           }
        }
    }
#endif
    *owner = Owner(thing);
    *flags = 0;
    return 0;
}

#endif	/* STANDALONE */

/* ---------------------------------------------------------------------------
 * atr_free: Return all attributes of an object.
 */

void 
atr_free(dbref thing)
{
    int attr;
    char *as;

    atr_push();
    for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
	atr_clr(thing, attr);
    }
    atr_pop();
    al_destroy(thing);		/* Just to be on the safe side */
}

/* garbage collect an attribute list */

void 
atr_collect(dbref thing)
{
    /* Nada.  qdbm takes care of us.  I hope ;-) */
}

/* ---------------------------------------------------------------------------
 * atr_cpy: Copy all attributes from one object to another.  Takes the
 * player argument to ensure that only attributes that COULD be set by
 * the player are copied.
 */

void 
atr_cpy(dbref player, dbref dest, dbref source)
{
    int attr, aflags;
    dbref owner, aowner;
    char *as, *buf;
    ATTR *at;

    owner = Owner(dest);
    atr_push();
    for (attr = atr_head(source, &as); attr; attr = atr_next(&as)) {
	buf = atr_get(source, attr, &aowner, &aflags);
	if (!(aflags & AF_LOCK))
	    aowner = owner;	/* chg owner */
	at = atr_num(attr);
	if (attr) {
	    if ((attr != A_MONEY) && Write_attr(owner, dest, at, aflags)) {
		/* Only set attrs that owner has perm to set */
    		mudstate.vlplay = player;
                if ( !((aflags & AF_NOCLONE) || (at && (at->flags & AF_NOCLONE))) ) {
		   atr_add(dest, attr, buf, aowner, aflags);
                }
            }
	}
	free_lbuf(buf);
    }
    atr_pop();
}

/* ---------------------------------------------------------------------------
 * atr_chown: Change the ownership of the attributes of an object to the
 * current owner if they are not locked.
 */

void 
atr_chown(dbref obj)
{
    int attr, aflags;
    dbref owner, aowner;
    char *as, *buf;

    owner = Owner(obj);
    atr_push();
    for (attr = atr_head(obj, &as); attr; attr = atr_next(&as)) {
	buf = atr_get(obj, attr, &aowner, &aflags);
	if ((aowner != owner) && !(aflags & AF_LOCK))
	    atr_add(obj, attr, buf, owner, aflags);
	free_lbuf(buf);
    }
    atr_pop();
}

/* ---------------------------------------------------------------------------
 * atr_next: Return next attribute in attribute list.
 */

int 
atr_next(char **attrp)
{
    if (!*attrp || !**attrp) {
	return 0;
    } else {
	return al_decode(attrp);
    }
}

/* ---------------------------------------------------------------------------
 * atr_push, atr_pop: Push and pop attr lists.
 */

void 
NDECL(atr_push)
{
    ALIST *new_alist;

    new_alist = (ALIST *) alloc_sbuf("atr_push");
    new_alist->data = mudstate.iter_alist.data;
    new_alist->len = mudstate.iter_alist.len;
    new_alist->next = mudstate.iter_alist.next;

    mudstate.iter_alist.data = NULL;
    mudstate.iter_alist.len = 0;
    mudstate.iter_alist.next = new_alist;
    return;
}

void 
NDECL(atr_pop)
{
    ALIST *old_alist;
    char *cp;

    old_alist = mudstate.iter_alist.next;

    if (mudstate.iter_alist.data) {
	free(mudstate.iter_alist.data);
    }
    if (old_alist) {
	mudstate.iter_alist.data = old_alist->data;
	mudstate.iter_alist.len = old_alist->len;
	mudstate.iter_alist.next = old_alist->next;
	cp = (char *) old_alist;
	free_sbuf(cp);
    } else {
	mudstate.iter_alist.data = NULL;
	mudstate.iter_alist.len = 0;
	mudstate.iter_alist.next = NULL;
    }
    return;
}

/* ---------------------------------------------------------------------------
 * atr_head: Returns the head of the attr list for object 'thing'
 */

int 
atr_head(dbref thing, char **attrp)
{
    char *astr;
    int alen;

    /* Get attribute list.  Save a read if it is in the modify atr list */

    if (thing == mudstate.mod_al_id) {
	astr = mudstate.mod_alist;
    } else {
	astr = atr_get_raw(thing, A_LIST);
    }
    alen = al_size(astr);

    /* If no list, return nothing */

    if (!alen)
	return 0;

    /* Set up the list and return the first entry */

    al_extend(&mudstate.iter_alist.data, &mudstate.iter_alist.len,
	      alen, 0);
    memcpy((char *) mudstate.iter_alist.data, (char *) astr, alen);
/*  bcopy((char *) astr, (char *) mudstate.iter_alist.data, alen); */
    *attrp = mudstate.iter_alist.data;
    return atr_next(attrp);
}

void val_count()
{
  dbref d;
  char *cp;
  int anum, count;

  DO_WHOLE_DB(d) {
    if ((d % 100) == 0)
      cache_reset(0);
    count = 0;
    for (anum = atr_head(d, &cp); anum; anum = atr_next(&cp)) {
      if ((anum >= A_USER_START) && (anum < A_INLINE_START)) count++;
    }
    db[d].nvattr = count; 
  }
}

int atrcint(dbref player, dbref thing, int key, char *s_wild)
{
   char *cp, *newatr, *s_tprintf, *s_tprintfptr;
   int anum, aflags, count;
   dbref aowner;
   ATTR *attr, *attr2;

   count = 0;
   s_tprintfptr = s_tprintf = alloc_lbuf("atrcint");
   for (anum = atr_head(thing, &cp); anum; anum = atr_next(&cp)) {
      if (atr_get_info(thing, anum, &aowner, &aflags)) {
         attr = atr_num(anum);
         if (!attr) { /* if we get here, something is wrong */
            STARTLOG(LOG_ALWAYS, "OBJECT", "DAMAGE")
               s_tprintfptr = s_tprintf;
               log_text((char *)safe_tprintf(s_tprintf, &s_tprintfptr, "Object damaged: %d, Attribute number: %d\n"
                        ,thing,anum));
               log_text("Attr pointer is NULL\n");
               s_tprintfptr = s_tprintf;
               if (attr && attr->name)
                  log_text((char *)safe_tprintf(s_tprintf, &s_tprintfptr, "Attribute Name is: %s\n",attr->name));
               if ( (key == 1) && (anum > (A_USER_START - 1)) && (anum < A_INLINE_START) ) {
                  newatr = alloc_sbuf("atrcint");
#ifndef STANDALONE
                  sprintf(newatr, "XYZZY_%d_%d", (int)(mudstate.now), anum);
#else
                  sprintf(newatr, "XYZZY_%d", anum);
#endif
                  attr2 = atr_str(newatr);
                  if ( !attr2 ) {
                     vattr_define(newatr, anum, AF_PRIVATE);
                     log_text("Attr pointer redefined to ");
                     log_text(newatr);
#ifndef STANDALONE
                     s_tprintfptr = s_tprintf;
                     notify(player, safe_tprintf(s_tprintf, &s_tprintfptr, "Attribute %d recovered.  Renamed to %s", anum, newatr));
#endif
                  } else {
#ifndef STANDALONE
                     s_tprintfptr = s_tprintf;
                     notify(player, safe_tprintf(s_tprintf, &s_tprintfptr, "Attribute %d unable to be renamed.  Cleared.", anum, newatr));
#endif
                     log_text("Attribute can not be recovered and was removed from attribute list");
                     atr_clr(thing,anum);
                 }
                 free_sbuf(newatr);
              } else if ( key == 0 ) {
                 atr_clr(thing,anum);
                 log_text("Attribute removed from attribute list");
              } else if ( key == 3 ) {
                 atr_clr(thing,anum);
                 log_text("Attribute removed from attribute list");
              } else if ( key == 2 ) {
                 log_text("Attribute was NOT removed.");
              }
           ENDLOG
           free_lbuf(s_tprintf);
           return -1;
        } else if ( key == 3 ) {
           if ( aflags & AF_IS_LOCK ) {
              atr_set_flags(thing, anum, (aflags & ~AF_IS_LOCK));
#ifndef STANDALONE
              s_tprintfptr = s_tprintf;
              notify(player, safe_tprintf(s_tprintf, &s_tprintfptr, "Attribute %s cleared of ISLOCK", attr->name));
#endif
           }
        }
        if ( Read_attr(player, thing, attr, aowner, aflags, 0) ) {
#ifndef STANDALONE
           if ( !s_wild || !*s_wild ) {
              count++;
           } else if (s_wild && *s_wild ) {
              if ( ((*s_wild == '^' ) && quick_regexp_match(s_wild, (char *)attr->name, 0)) ||
                   ((*s_wild != '^' ) && quick_wild(s_wild, (char *)attr->name)) ) {
                 count++;
              }
           }
#else
           count++;
#endif
        }
     }
   }
   free_lbuf(s_tprintf);
   return count;
}

/* ---------------------------------------------------------------------------
 * db_grow: Extend the struct database.
 */

#define	SIZE_HACK	1	/* So mistaken refs to #-1 won't die. */

void 
db_grow(dbref newtop)
{
    int newsize, marksize, delta, i, i_totem;
    MARKBUF *newmarkbuf;
    OBJ *newdb;
    OBJTOTEM *newtotem;
    NAME *newnames;
    char *cp;

#ifndef STANDALONE
    delta = mudconf.init_size;
#else
    delta = 1000;
#endif

    /* Determine what to do based on requested size, current top and 
     * size.  Make sure we grow in reasonable-sized chunks to prevent
     * frequent reallocations of the db array.
     */

    /* If requested size is smaller than the current db size, ignore it */

    if (newtop <= mudstate.db_top) {
	return;
    }
    /* If requested size is greater than the current db size but smaller
     * than the amount of space we have allocated, raise the db size and
     * initialize the new area.
     */

    if (newtop <= mudstate.db_size) {
	for (i = mudstate.db_top; i < newtop; i++) {
	    s_Owner(i, GOD);
	    s_Flags(i, (TYPE_THING | GOING));
	    s_Flags2(i, 0);
	    s_Flags3(i, 0);
	    s_Flags4(i, 0);
	    s_Toggles(i, 0);
	    s_Toggles2(i, 0);
	    s_Toggles3(i, 0);
	    s_Toggles4(i, 0);
	    s_Toggles5(i, 0);
	    s_Toggles6(i, 0);
	    s_Toggles7(i, 0);
	    s_Toggles8(i, 0);
	    s_Location(i, NOTHING);
	    s_Contents(i, NOTHING);
	    s_Exits(i, NOTHING);
	    s_Link(i, NOTHING);
	    s_Next(i, NOTHING);
	    s_Parent(i, NOTHING);
	    if (mudconf.cache_names)
		i_Name(i);
            db[i].zonelist = NULL;
	    db[i].nvattr = 0;
            for ( i_totem = 0; i_totem < TOTEM_SLOTS; i_totem++ ) {
               dbtotem[i].flags[i_totem] = 0;
            }
            dbtotem[i].modified = 0;
	}
	mudstate.db_top = newtop;
	return;
    }
    /* Grow by a minimum of delta objects */

    if (newtop <= mudstate.db_size + delta) {
	newsize = mudstate.db_size + delta;
    } else {
	newsize = newtop;
    }

    /* Enforce minimumdatabase size */

    if (newsize < mudstate.min_size)
	newsize = mudstate.min_size + delta;;

    /* Grow the name table */

    if (mudconf.cache_names) {
	newnames = (NAME *) XMALLOC((newsize + SIZE_HACK) * sizeof(NAME),
				    "db_grow.names");
	if (!newnames) {
	    LOG_SIMPLE(LOG_ALWAYS, "ALC", "DB",
		 unsafe_tprintf("Could not allocate space for %d item name cache.",
			 newsize));
	    abort();
	}
	bzero((char *) newnames, (newsize + SIZE_HACK) * sizeof(NAME));

	if (names) {

	    /* An old name cache exists.  Copy it. */

	    names -= SIZE_HACK;
/*	    bcopy((char *) names, (char *) newnames,
		  (newtop + SIZE_HACK) * sizeof(NAME)); */
            memcpy((char *) newnames, (char *) names, (newtop + SIZE_HACK) * sizeof(NAME));
	    cp = (char *) names;
	    XFREE(cp, "db_grow.name");
	} else {

	    /* Creating a brand new struct database.  Fill in the
	     * 'reserved' area in case it gets referenced.
	     */

	    names = newnames;
	    for (i = 0; i < SIZE_HACK; i++) {
		i_Name(i);
	    }
	}
	names = newnames + SIZE_HACK;
	newnames = NULL;
    }
    /* Grow the db array */

    newdb = (OBJ *) XMALLOC((newsize + SIZE_HACK) * sizeof(OBJ), "db_grow.db");
    newtotem = (OBJTOTEM *) XMALLOC((newsize + SIZE_HACK) * sizeof(OBJTOTEM), "db_grow.db");
    if (!newdb || !newtotem) {

	LOG_SIMPLE(LOG_ALWAYS, "ALC", "DB",
	    unsafe_tprintf("Could not allocate space for %d item struct database.",
		    newsize));
	abort();
    }
    if (db) {

	/* An old struct database exists.  Copy it to the new buffer */

	db -= SIZE_HACK;
	dbtotem -= SIZE_HACK;
/*	bcopy((char *) db, (char *) newdb,
	      (mudstate.db_top + SIZE_HACK) * sizeof(OBJ)); */
        memcpy((char *) newdb, (char *) db, (mudstate.db_top + SIZE_HACK) * sizeof(OBJ));
        memcpy((char *) newtotem, (char *) dbtotem, (mudstate.db_top + SIZE_HACK) * sizeof(OBJTOTEM));
	cp = (char *) db;
	XFREE(cp, "db_grow.db");
        cp = (char *) dbtotem;
	XFREE(cp, "db_grow.db");
    } else {

	/* Creating a brand new struct database.  Fill in the
	 * 'reserved' area in case it gets referenced.
	 */

	db = newdb;
        dbtotem = newtotem;
	for (i = 0; i < SIZE_HACK; i++) {
	    s_Owner(i, GOD);
	    s_Flags(i, (TYPE_THING | GOING));
	    s_Flags2(i, 0);
	    s_Flags3(i, 0);
	    s_Flags4(i, 0);
	    s_Toggles(i, 0);
	    s_Toggles2(i, 0);
	    s_Toggles3(i, 0);
	    s_Toggles4(i, 0);
	    s_Toggles5(i, 0);
	    s_Toggles6(i, 0);
	    s_Toggles7(i, 0);
	    s_Toggles8(i, 0);
	    s_Location(i, NOTHING);
	    s_Contents(i, NOTHING);
	    s_Exits(i, NOTHING);
	    s_Link(i, NOTHING);
	    s_Next(i, NOTHING);
	    s_Pennies(i, NOTHING);
/*                      s_Zone(i, NOTHING); */
	    s_Parent(i, NOTHING);
	    s_Pennies(i, 0);
            db[i].zonelist = NULL;
	    db[i].nvattr = 0;
            for ( i_totem = 0; i_totem < TOTEM_SLOTS; i_totem++ ) {
               dbtotem[i].flags[i_totem] = 0;
            }
            dbtotem[i].modified = 0;
	}
    }
    db = newdb + SIZE_HACK;
    dbtotem = newtotem + SIZE_HACK;
    newdb = NULL;
    newtotem = NULL;

    for (i = mudstate.db_top; i < newtop; i++) {
	if (mudconf.cache_names)
	    i_Name(i);
	s_Flags(i, TYPE_THING | GOING);
	s_Owner(i, GOD);
	s_Flags2(i, 0);
	s_Flags3(i, 0);
	s_Flags4(i, 0);
	s_Toggles(i, 0);
	s_Toggles2(i, 0);
	s_Toggles3(i, 0);
	s_Toggles4(i, 0);
	s_Toggles5(i, 0);
	s_Toggles6(i, 0);
	s_Toggles7(i, 0);
	s_Toggles8(i, 0);
	s_Location(i, NOTHING);
	s_Contents(i, NOTHING);
	s_Exits(i, NOTHING);
	s_Link(i, NOTHING);
	s_Next(i, NOTHING);
	s_Parent(i, NOTHING);
        db[i].zonelist = NULL;
	db[i].nvattr = 0;
        for ( i_totem = 0; i_totem < TOTEM_SLOTS; i_totem++ ) {
           dbtotem[i].flags[i_totem] = 0;
        }
        dbtotem[i].modified = 0;
    }
    mudstate.db_top = newtop;
    mudstate.db_size = newsize;

    /* Grow the db mark buffer */

    marksize = (newsize + 7) >> 3;
    newmarkbuf = (MARKBUF *) XMALLOC(marksize, "db_grow");
    bzero((char *) newmarkbuf, marksize);
    if (mudstate.markbits) {
	marksize = (newtop + 7) >> 3;
/*	bcopy((char *) mudstate.markbits, (char *) newmarkbuf, marksize); */
        memcpy((char *) newmarkbuf, (char *) mudstate.markbits, marksize);
	cp = (char *) mudstate.markbits;
	XFREE(cp, "db_grow");
    }
    mudstate.markbits = newmarkbuf;
}

#ifndef STANDALONE 
void showdbstats(dbref player) 
{
    int sum;

   sum = 0;
 /* This logic doesn't work.  We'd have to maintain a total count */
/* sum = ((&mudstate.command_htab)->hashsize * (&mudstate.command_htab)->entries);
 * sum = sum + ((&mudstate.logout_cmd_htab)->hashsize * (&mudstate.logout_cmd_htab)->entries);
 * sum = sum + ((&mudstate.func_htab)->hashsize * (&mudstate.func_htab)->entries);
 * sum = sum + ((&mudstate.flags_htab)->hashsize * (&mudstate.flags_htab)->entries);
 * sum = sum + ((&mudstate.attr_name_htab)->hashsize * (&mudstate.attr_name_htab)->entries);
 * sum = sum + ((&mudstate.player_htab)->hashsize * (&mudstate.player_htab)->entries);
 * sum = sum + ((&mudstate.desc_htab)->hashsize * (&mudstate.desc_htab)->entries);
 * sum = sum + ((&mudstate.fwdlist_htab)->hashsize * (&mudstate.fwdlist_htab)->entries);
 * sum = sum + ((&mudstate.parent_htab)->hashsize * (&mudstate.parent_htab)->entries);
 * sum = sum + ((&mudstate.news_htab)->hashsize * (&mudstate.news_htab)->entries);
 * sum = sum + ((&mudstate.help_htab)->hashsize * (&mudstate.help_htab)->entries);
 * sum = sum + ((&mudstate.wizhelp_htab)->hashsize * (&mudstate.wizhelp_htab)->entries);
 * #ifdef PLUSHELP
 * sum = sum + ((&mudstate.plushelp_htab)->hashsize * (&mudstate.plushelp_htab)->entries);
 * #endif
 */
  /* This has to be a guess as we don't know the size of all attribs.  Mean Avg is 15 */
  sum = sum + sum_vhashstats(player);

  notify(player, " ");
  notify(player, "DB Size         DB Top          DB Object Size  VHash Size      Total Mem");
  notify(player, unsafe_tprintf("%-16d%-16d%-16d%-16d%dK", 
                         mudstate.db_size,
                         mudstate.db_top,
                         sizeof(OBJ),
                         sum,
                         ((mudstate.db_size * sizeof(OBJ) + sum) / 1024)));
}
#endif


void 
NDECL(db_free)
{
    dbref i;
    
    char *cp;

    if (db != NULL) {
        for( i = 0; i < mudstate.db_top; i++ ) {
           zlist_destroy(i);
        }
	db -= SIZE_HACK;
	cp = (char *) db;
	XFREE(cp, "db_grow");
	db = NULL;
    }
    mudstate.db_top = 0;
    mudstate.db_size = 0;
    mudstate.freelist = NOTHING;
    mudstate.recoverlist = NOTHING;
}

#ifndef STANDALONE
void 
NDECL(db_make_minimal)
{
    dbref obj;
    int safepwd_chk = 0;

    db_free();
    db_grow(1);
    s_Name(0, "Limbo");
    s_Flags(0, TYPE_ROOM | mudconf.room_flags.word1);
    s_Location(0, NOTHING);
    s_Exits(0, NOTHING);
    s_Link(0, NOTHING);
    s_Parent(0, NOTHING);
/*      s_Zone(0, NOTHING); */
    s_Pennies(0, 1);
    s_Owner(0, 1);
    s_Flags2(0,mudconf.room_flags.word2);
    s_Flags3(0,mudconf.room_flags.word3);
    s_Flags4(0,mudconf.room_flags.word4);
    s_Toggles(0,mudconf.room_toggles.word1);
    s_Toggles2(0,mudconf.room_toggles.word2);
    s_Toggles3(0,mudconf.room_toggles.word3);
    s_Toggles4(0,mudconf.room_toggles.word4);
    s_Toggles5(0,mudconf.room_toggles.word5);
    s_Toggles6(0,mudconf.room_toggles.word6);
    s_Toggles7(0,mudconf.room_toggles.word7);
    s_Toggles8(0,mudconf.room_toggles.word8);
    db[0].zonelist = NULL;

    /* should be #1 */
    load_player_names();
    
    /* Safer passwords don't allow #1 creation here.  Bad bad mojo */
    safepwd_chk = mudconf.safer_passwords;
    mudconf.safer_passwords = 0;
    obj = create_player((char *) "Wizard", (char *) "Nyctasia", NOTHING, 0, (char *)"Wizard", 0);
    mudconf.safer_passwords = safepwd_chk;
    s_Flags(obj, Flags(obj) | IMMORTAL);
    s_Flags2(obj, Flags2(obj) & ~WANDERER);
    s_Pennies(obj, 1000);

    /* Manually link to Limbo, just in case */
    s_Location(obj, 0);
    s_Next(obj, NOTHING);
    s_Contents(0, obj);
    s_Link(obj, 0);
}

#endif

dbref
parse_dbref_special(char *s) {
   char *p, *q, *r;
   int x;
#ifndef STANDALONE
   char *atext, *buff;
   int aflags, i_id, i_id_found;
   double y, z;
   struct tm *ttm;
   long l_offset, mynow;
   dbref aowner;
   ATTR *a_id;
#endif

   r = q = strchr(s, ':');
   *q = '\0';
   q++;

   for (p = s; *p; p++) {
      if (!isdigit((int)*p)) {
         *r = ':';
         return NOTHING;
      }
   }
   x = atoi(s); 
   *r = ':';
#ifndef STANDALONE
   if ( Good_chk(x) ) {
      if ( NoTimestamp(x) ) {
         return ((x >= 0) ? x : NOTHING);
      }
      i_id = mkattr("__OBJID_INTERNAL");
      i_id_found = 0;
      if (i_id > 0) {
         a_id = atr_num_objid(i_id);
         if (a_id) {
            i_id_found = 1;
            atext = atr_get(x, a_id->number, &aowner, &aflags);
            if ( atext && *atext ) {
               i_id_found = 2;
               y = safe_atof(atext);
            }
            free_lbuf(atext);
         }
      }

      if ( i_id_found == 2 ) {
         z = safe_atof(q);
         if ( y == z ) {
            return ((x >= 0) ? x : NOTHING);
         }
         x = -1;
      } else {
         atext = atr_get(x, A_CREATED_TIME, &aowner, &aflags);
         if ( atext && *atext ) {
            if ( mudconf.objid_localtime ) {
               ttm = localtime(&mudstate.now);
            } else {
               ttm = localtime(&mudstate.now);
               mynow = mktime(ttm);
               ttm = gmtime(&mudstate.now);
               mynow -= mktime(ttm);
            }
            l_offset = (long) mktime(ttm) - (long) mush_mktime64(ttm);
            if (do_convtime(atext, ttm)) {
               if ( mudconf.objid_localtime ) {
                  y = (double)(mush_mktime64(ttm) + l_offset);
               } else {
                  y = (double)(mush_mktime64(ttm) + l_offset + mynow + mudconf.objid_offset);
               }
               if ( i_id_found == 1 ) {
                  buff = alloc_sbuf("create_objid");
                  sprintf(buff, "%.0f", y);
                  atr_add_raw(x, a_id->number, buff);
                  atr_set_flags(x, a_id->number, AF_INTERNAL|AF_GOD);
                  free_sbuf(buff);
               }
               z = safe_atof(q);
               if ( y == z ) {
                  free_lbuf(atext);
                  return ((x >= 0) ? x : NOTHING);
               }
            }
         }
         free_lbuf(atext);
         x = -1;
      }
   }
#endif
   return ((x >= 0) ? x : NOTHING);
}

dbref 
parse_dbref(const char *s)
{
    const char *p;
    int x;

    /* Enforce completely numeric dbrefs */
    if ( !*s )
       return NOTHING;

#ifndef STANDALONE
    if ( mudconf.enable_tstamps && (strchr(s, ':') != NULL) ) {
       return parse_dbref_special((char *)s);
    }
#endif
    for (p = s; *p; p++) {
	if (!isdigit((int)*p))
	    return NOTHING;
    }

    x = atoi(s);
    return ((x >= 0) ? x : NOTHING);
}

void 
putref(FILE * f, dbref ref)
{
    fprintf(f, "%d\n", ref);
}

void 
putstring(FILE * f, const char *s)
{
    if (s) {
	fputs(s, f);
    }
    putc('\n', f);
}

const char *
getstring_noalloc(FILE * f)
{
    static char buf[LBUF_SIZE];
    char *p;
    int c, lastc;

    p = buf;
    c = '\0';
    for (;;) {
	lastc = c;
	c = fgetc(f);

	/* If EOF or null, return */

	if (!c || (c == EOF)) {
	    *p = '\0';
	    return buf;
	}
	/* If a newline, return if prior char is not a cr.  Otherwise
	 * keep on truckin'
	 */

	if ((c == '\n') && (lastc != '\r')) {
	    *p = '\0';
	    return buf;
	}
	safe_chr(c, buf, &p);
    }
}

dbref 
getref(FILE * f)
{
    return (atoi(getstring_noalloc(f)));
}

void 
free_boolexp(BOOLEXP * b)
{
    if (b == TRUE_BOOLEXP)
	return;

    switch (b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
	free_boolexp(b->sub1);
	free_boolexp(b->sub2);
	free_bool(b);
	break;
    case BOOLEXP_NOT:
    case BOOLEXP_CARRY:
    case BOOLEXP_IS:
    case BOOLEXP_OWNER:
    case BOOLEXP_INDIR:
	free_boolexp(b->sub1);
	free_bool(b);
	break;
    case BOOLEXP_CONST:
	free_bool(b);
	break;
    case BOOLEXP_ATR:
    case BOOLEXP_EVAL:
	free((char *) b->sub1);
	free_bool(b);
	break;
    }
}

BOOLEXP *
dup_bool(BOOLEXP * b)
{
    BOOLEXP *r;

    if (b == TRUE_BOOLEXP)
	return (TRUE_BOOLEXP);

    r = alloc_bool("dup_bool");
    switch (r->type = b->type) {
    case BOOLEXP_AND:
    case BOOLEXP_OR:
	r->sub2 = dup_bool(b->sub2);
    case BOOLEXP_NOT:
    case BOOLEXP_CARRY:
    case BOOLEXP_IS:
    case BOOLEXP_OWNER:
    case BOOLEXP_INDIR:
	r->sub1 = dup_bool(b->sub1);
    case BOOLEXP_CONST:
	r->thing = b->thing;
	break;
    case BOOLEXP_EVAL:
    case BOOLEXP_ATR:
	r->thing = b->thing;
	r->sub1 = (BOOLEXP *) strsave((char *) b->sub1);
	break;
    default:
	fprintf(stderr, "bad bool type!!\n");
	return (TRUE_BOOLEXP);
    }
    return (r);
}

void 
clone_object(dbref a, dbref b)
{
/*    bcopy((char *) &db[b], (char *) &db[a], sizeof(struct object)); */
   memcpy((char *) &db[a], (char *) &db[b], sizeof(struct object)); 
}

int 
init_qdbm_db(char *qdbmfile)
{
#ifdef STANDALONE
    fprintf(stderr, "Opening %s\n", qdbmfile);
#endif
    cache_init(mudconf.cache_width, mudconf.cache_depth);
    dddb_setfile(qdbmfile);
    dddb_init();
#ifdef STANDALONE
    fprintf(stderr, "Done opening %s.\n", qdbmfile);
#else
    STARTLOG(LOG_ALWAYS, "INI", "LOAD")
	log_text((char *) "Using qdbm file: ");
    log_text(qdbmfile);
    ENDLOG
#endif
	db_free();
    mudstate.nolookie = 0;
    vattr_init();
    return (0);
}

#ifndef STANDALONE
void do_dbclean(dbref player, dbref cause, int key)
{
   char *s_chkattr, *s_buff, *cs;
   int ca, i, i_walkie, i_cntr, i_max, i_del;
   VATTR *va, *va2;
   ATTR *atr;
   DESC *d;

   DESC_ITER_CONN(d) {
      if ( d->player == player ) {
         if ( key & DBCLEAN_CHECK )
            queue_string(d,"Checking database of empty attributes.  Please wait...");
         else
            queue_string(d,"Purging database of empty attributes.  Please wait...");
         queue_write(d, "\r\n", 2);
         process_output(d);
      }
   }
   s_buff = alloc_sbuf("do_dbclean");
   s_chkattr = alloc_lbuf("attribute_delete");

   DESC_ITER_CONN(d) {
      if ( d->player == player ) {
         queue_string(d, "---> Hashing values...");
         queue_write(d, "\r\n", 2);
         process_output(d);
      }
   }
   DO_WHOLE_DB(i) {
      for (ca=atr_head(i, &cs); ca; ca=atr_next(&cs)) {
         if ( (ca > A_USER_START) && (ca < A_INLINE_START) ) {
            atr = atr_num2(ca);
            if ( atr ) {
               va = (VATTR *) vattr_find((char *)atr->name);
               if ( va && !(va->flags & AF_DELETED) ) {
                  if ( !(va->flags & AF_NONBLOCKING) ) {
                     va->flags |= AF_NONBLOCKING;
                  }
               }
            }
         }
      }
   }
   DESC_ITER_CONN(d) {
      if ( d->player == player ) {
         if ( key & DBCLEAN_CHECK )
            queue_string(d, "---> Initializing check routines...");
         else
            queue_string(d, "---> Initializing purge routines...");
         queue_write(d, "\r\n", 2);
         process_output(d);
      }
   }

   
   va = vattr_first();
   i_max = list_vcount();
   i_walkie = i_del = i_cntr = 0;

   for (va = vattr_first(); va; va = va2) {
       if ( va )
          va2 = vattr_next(va);
       if ( va && !(va->flags & AF_DELETED)) {
          if ( va->command_flag == 0 ) {
             if ( !(va->flags & AF_NONBLOCKING) && va->name ) {
                strcpy(s_buff, va->name);
                if ( key & DBCLEAN_CHECK )
                   va->flags &= ~AF_NONBLOCKING;
                else
	           vattr_delete(s_buff);
                i_del++;
                i_cntr++;
             } else {
                va->flags &= ~AF_NONBLOCKING;
             }
          } else {
             va->flags &= ~AF_NONBLOCKING;
          }
       }
       i_walkie++;
       if ( !(i_walkie % 5000) ) {
          sprintf(s_chkattr, "---> %10d/%10d attributes processed [%d %s]", 
                  i_walkie, i_max, i_del, ((key & DBCLEAN_CHECK) ? "would be deleted" : "deleted") );
          i_del = 0;
          DESC_ITER_CONN(d) {
             if ( d->player == player ) {
                queue_string(d, s_chkattr);
                queue_write(d, "\r\n", 2);
                process_output(d);
             }
          }
       }
   }
   DESC_ITER_CONN(d) {
      sprintf(s_chkattr, "---> %10d/%10d attributes processed [%d %s]\r\n---> Completed!  %d total attributes have been %s.", 
                          i_walkie, i_max, i_del, ((key & DBCLEAN_CHECK) ? "would be deleted" : "deleted"),
                          i_cntr, ((key & DBCLEAN_CHECK) ? "verified deleteable" : "deleted") );
      if ( d->player == player ) {
         queue_string(d, s_chkattr);
         queue_write(d, "\r\n", 2);
         process_output(d);
      }
   }
   free_lbuf(s_chkattr);
   free_sbuf(s_buff);
}

#endif
