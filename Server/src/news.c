/* news.c - Hardcoded version of MushNews by Thorin (Mike McDermott) 01/1997 */

#include "autoconf.h"
#include "myndbm.h"
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "externs.h"
#include "interface.h"
#include "command.h"
#include "db.h"
#ifdef NEED_VALUES
#ifndef BSD_LIKE
#include <values.h>
#else
#include <limits.h>
#endif
#endif
/* #include "values.h" */

#include "news.h"
#include "mail.h"

#ifdef MAXINT
#define MYMAXINT        MAXINT
#else
#define MYMAXINT        2147483647
#endif

/* Any thoughts on where this should live? */
extern void FDECL(putstring, (FILE *, const char *));

/* ==================== Module Scope Macros ============================ */

#define news_assert(x) { if( !(x) ) { \
                           news_halt("Assertion failure '%s', line %d, file '%s'.", #x, __LINE__, __FILE__); \
                           return; \
                         } \
                       }

#define news_assert_retnonevoid(x,y) { if( !(x) ) { \
                           news_halt("Assertion failure '%s', line %d, file '%s'.", #x, __LINE__, __FILE__); \
                           return (y); \
                         } \
                       }

/* ==================== Additional Externs ============================= */

extern const char *getstring_noalloc(FILE * f);

/* ==================== Module Scope Data ============================== */

DBM* ndb = NULL;
int news_system_active = 0;
int news_internal_failure = 0;

/* ==================== Additional Features By Ash ===================== */

char* news_function_group( dbref player, dbref cause, int key ); /* Call to functions.c */

/* ==================== Module Scope Prototypes ======================== */

char* ez_ctime( time_t timemark );
char* groupfmt( dbref player, char* group_name );

int news_dbck_internal( void );

void news_halt(char* fmt, ...);

int articles_in_group(char* group_name);
int articles_by_user(dbref user);

int user_subscribed(dbref user, char* group_name);

int create_user_if_new(dbref player, dbref user);
int user_passes_post(dbref user, char* group_name);
int user_passes_read(dbref user, char* group_name);
int user_passes_admin(dbref user, char* group_name);

int add_user_to_group(char* group_name, dbref user);
int add_group_to_user(dbref user, char* group_name);

void news_readall_group( dbref player, char* groupkeyptr );

int delete_article_from_user(dbref user, char* group_name, int seq);
int delete_article_from_group(char* group_name, int seq);
int delete_group_from_user(dbref user, char* group_name);
int delete_user_from_group(char* group_name, dbref user);

int news_convert_db( int src_ver );

int get_VV(NewsVersionRec* vv);
int get_XX(NewsGlobalRec* xx);
int get_XX_v1(NewsGlobalRec_v1* xx);

int get_GI(NewsGroupInfoRec* gi,        char* group_name);
int get_GI_v1(NewsGroupInfoRec_v1* gi,  char* group_name);
int get_GR(NewsGroupReadLockRec* gr,    char* group_name);
int get_GP(NewsGroupPostLockRec* gp,    char* group_name);
int get_GX(NewsGroupAdminLockRec* gx,   char* group_name);
int get_GA(NewsGroupArticleInfoRec* ga, char* group_name, int seq);
int get_GT(NewsGroupArticleTextRec* gt, char* group_name, int seq);
int get_GU(NewsGroupUserRec* gu,        char* group_name, dbref user_dbref);

int get_UI(NewsUserInfoRec* ui,    dbref user_dbref);
int get_UG(NewsUserGroupRec* ug,   dbref user_dbref, char* group_name);
int get_UA(NewsUserArticleRec* ua, dbref user_dbref, char* group_name, int seq);

int put_VV(NewsVersionRec* vv);
int put_XX(NewsGlobalRec* xx);

int put_GI(NewsGroupInfoRec* gi);
int put_GR(NewsGroupReadLockRec* gr);
int put_GP(NewsGroupPostLockRec* gp);
int put_GX(NewsGroupAdminLockRec* gx);
int put_GA(NewsGroupArticleInfoRec* ga);
int put_GT(NewsGroupArticleTextRec* gt);
int put_GU(NewsGroupUserRec* gu);

int put_UI(NewsUserInfoRec* ui);
int put_UG(NewsUserGroupRec* ug);
int put_UA(NewsUserArticleRec* ua);

int del_VV(NewsVersionRec* vv);
int del_XX(NewsGlobalRec* xx);

int del_GI(NewsGroupInfoRec* gi);
int del_GR(NewsGroupReadLockRec* gr);
int del_GP(NewsGroupPostLockRec* gp);
int del_GX(NewsGroupAdminLockRec* gx);
int del_GA(NewsGroupArticleInfoRec* ga);
int del_GT(NewsGroupArticleTextRec* gt);
int del_GU(NewsGroupUserRec* gu);

int del_UI(NewsUserInfoRec* ui);
int del_UG(NewsUserGroupRec* ug);
int del_UA(NewsUserArticleRec* ua);

CMD_TWO_ARG(news_articlelife);
CMD_TWO_ARG(news_post);
CMD_TWO_ARG(news_mailto);
CMD_TWO_ARG(news_postlist);
CMD_TWO_ARG(news_repost);
CMD_TWO_ARG(news_read);
CMD_TWO_ARG(news_readall);
CMD_TWO_ARG(news_jump);
CMD_TWO_ARG(news_check);
CMD_TWO_ARG(news_status);
CMD_TWO_ARG(news_yank);
CMD_TWO_ARG(news_extend);
CMD_TWO_ARG(news_subscribe);
CMD_TWO_ARG(news_unsubscribe);
CMD_TWO_ARG(news_groupadd);
CMD_TWO_ARG(news_groupdel);
CMD_TWO_ARG(news_groupinfo);
CMD_TWO_ARG(news_grouplist);
CMD_TWO_ARG(news_groupmem);
CMD_TWO_ARG(news_groupadmin);
CMD_TWO_ARG(news_grouplimit);
CMD_TWO_ARG(news_userlimit);
CMD_TWO_ARG(news_userinfo);
CMD_TWO_ARG(news_usermem);
CMD_TWO_ARG(news_lock);
CMD_TWO_ARG(news_expire);
CMD_TWO_ARG(newsdb_unload);
CMD_TWO_ARG(newsdb_load);
CMD_TWO_ARG(newsdb_dbinfo);
CMD_TWO_ARG(newsdb_dbck);
CMD_TWO_ARG(news_off);

/* ==================== Functions ============================= */

/* -------------------- Externally accessible functions ------- */

void start_news_system( void )
{
  char news_db_name[129 + 32 + 5];
  char news_db_dir_name[129 + 32 + 5 + 4];
  NewsVersionRec vv;
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  time_t now;
  struct stat st;
  int statretval;

  time(&now);

  sprintf(news_db_name, 
	  "%s/%s.news", mudconf.data_dir, mudconf.muddb_name);
  sprintf(news_db_dir_name, "%s.dir", news_db_name);

  /* check for new db */

  statretval = stat(news_db_dir_name, &st); 

  if( statretval == -1 ) { /* failed stat */
    if( errno != ENOENT ) {
      STARTLOG(LOG_STARTUP, "INI", "ERROR")
        log_text((char *) "Unable to stat news database.");
      ENDLOG
      news_internal_failure = 1;
      return;
    }
    else { /* no db file exists */
      ndb = dbm_open(news_db_name, O_RDWR | O_CREAT, 00664);

      if( !ndb ) {
        STARTLOG(LOG_STARTUP, "INI", "ERROR")
          log_text((char *) "Unable to create news database.");
        ENDLOG
        news_internal_failure = 1;
        return;
      }
      STARTLOG(LOG_STARTUP, "INI", "START")
        log_text((char *) "News System Started/New DB.");
      ENDLOG

      /* create an initial database including the special groups
         'info' and 'general' of which all users are forced to belong. */

      memset(&vv,0,sizeof(vv));
      vv.version = LATEST_NEWS_DB_VERSION;

      put_VV(&vv);

      memset(&xx,0,sizeof(xx));
      xx.db_creation_time = now;
      xx.last_expire_time = 0;
      xx.def_article_life = DEFAULT_EXPIRE_OFFSET;
      xx.user_limit  = 100;
      strcpy(xx.first_group, "general");
      xx.first_user_dbref  = -1;

      put_XX(&xx);

      memset(&gi,0,sizeof(gi));
      strcpy(gi.name, "general");
      strcpy(gi.desc, "General Discussion");
      gi.adder = -1;
      gi.posting_limit = 1000;
      gi.def_article_life = -1;
      gi.create_time = now;
      gi.last_activity_time = 0;
      gi.next_post_seq = 0;
      gi.first_seq = -1;
      gi.first_user_dbref = -1;
      strcpy(gi.next_group, "info");

      put_GI(&gi);

      memset(&gi,0,sizeof(gi));
      strcpy(gi.name, "info");
      strcpy(gi.desc, "Information");
      gi.adder = -1;
      gi.posting_limit = 1000;
      gi.def_article_life = -1;
      gi.create_time = now;
      gi.last_activity_time = 0;
      gi.next_post_seq = 0;
      gi.first_seq = -1;
      gi.first_user_dbref = -1;
      strcpy(gi.next_group, "");
  
      put_GI(&gi);
    }
  }
  else { /* db exists */
    ndb = dbm_open(news_db_name, O_RDWR, 00664);

    if( !ndb ) {
      STARTLOG(LOG_STARTUP, "INI", "ERROR")
        log_text((char *) "Unable to open existing news database.");
      ENDLOG
      news_internal_failure = 1;
      return;
    }
    else {
      STARTLOG(LOG_STARTUP, "INI", "START")
        log_text((char *) "News System started.");
      ENDLOG
    }
  }

  /* check db version and upgrade if needed */
  if( !get_VV(&vv) ) {
    /* version 1 database didn't have VV record */

    STARTLOG(LOG_STARTUP, "INI", "WARN")
      log_text((char *) "News db is an old version, converting.");
    ENDLOG
    news_assert( news_convert_db( 1 ) );
  }
  else {
    if( vv.version > 1 &&
        vv.version < LATEST_NEWS_DB_VERSION ) {
      STARTLOG(LOG_STARTUP, "INI", "WARN")
        log_text((char *) "News db is an old version, converting.");
      ENDLOG
      news_assert( news_convert_db( vv.version ) );
    }
    else if( vv.version != LATEST_NEWS_DB_VERSION ) {
      STARTLOG(LOG_STARTUP, "INI", "ERROR")
        log_text((char *) "News database version is unknown.");
      ENDLOG
      news_internal_failure = 1;
    }
  }
}

void shutdown_news_system( void )
{
  STARTLOG(LOG_STARTUP, "WIZ", "SHTDN")
    log_text((char *) "News System shutdown.");
  ENDLOG

  if( !ndb ) {
    return;
  }

  dbm_close(ndb);
  news_system_active = 0;
}

void do_newsdb(dbref player, dbref cause, int key, char *buf1, char *buf2)
{

  /* check for dbck which can happen w/o turning on system */
  /* and in the presence of an active internal failure */

  if( key == NEWSDB_DBCK ) {
    newsdb_dbck(player, cause, key, buf1, buf2); 
    return;
  }

  /* check for load request which can happen even if the system is
     not turned on and in the presence of an internal failure */

  if( key == NEWSDB_LOAD ) {
    newsdb_load(player, cause, key, buf1, buf2); 
    return;
  }

  /* block all other activity unless system is turned on */

  if( !news_system_active ) {
    notify(player, "News: The news system is currently deactivated.");
    return;
  }

  switch(key) {
    case NEWSDB_UNLOAD:
    newsdb_unload(player, cause, key, buf1, buf2);
    break;

    case NEWSDB_DBINFO:
    newsdb_dbinfo(player, cause, key, buf1, buf2);
    break;

    default:
    notify(player,"Illegal combination of switches.");
    break;
  } /* switch */
}

void do_news(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  /* first check to see if player is requesting old style news */
  if( !key ) {
    do_help(player, cause, (HELP_NEWS|key), buf1);
    return;
  }

  /* no guests beyond this point */

  if( Guest(player) ) {
    notify(player, "Sorry, guests may not use this command.");
    return;
  }

  /* check for activation request */

  if( key == NEWS_ON ) {
    if( news_system_active ) {
      notify(player, "News: System already active.");
    } 
    else if( news_internal_failure ) {
      notify(player, "News: An internal error is preventing the system from starting.");
      notify(player, "News: Notify a server administrator.");
    }
    else {
      notify(player, "News: Performing pre-activation DB integrity check.");
      if( news_dbck_internal() ) {
        notify(player, "News: System activated.");
        news_system_active = 1;
      }
      else {
        notify(player, "News: System startup failed.");
        notify(player, "News: Notify a server administrator.");
      }
    }
    return;
  }

  /* block all other activity unless system is turned on */

  if( !news_system_active ) {
    notify(player, "News: The news system is currently deactivated.");
    return;
  }

  /* news system is active, dispatch all valid commands */

  switch( key ) {
    case NEWS_ADMINLOCK: 
    news_groupadmin(player, cause, key, buf1, buf2); 
    break;

    case NEWS_ARTICLELIFE: 
    news_articlelife(player, cause, key, buf1, buf2); 
    break;

    case NEWS_POST: 
    news_post(player, cause, key, buf1, buf2); 
    break;

    case NEWS_MAILTO: 
    news_mailto(player, cause, key, buf1, buf2); 
    break;
 
    case NEWS_POSTLIST: 
    news_postlist(player, cause, key, buf1, buf2); 
    break;

    case NEWS_REPOST: 
    news_repost(player, cause, key, buf1, buf2); 
    break;

    case NEWS_READ: 
    news_read(player, cause, key, buf1, buf2); 
    break;

    case NEWS_READALL: 
    news_readall(player, cause, key, buf1, buf2); 
    break;

    case NEWS_JUMP: 
    news_jump(player, cause, key, buf1, buf2); 
    break;

    case NEWS_CHECK: 
    news_check(player, cause, key, buf1, buf2); 
    break;

    case (NEWS_CHECK + NEWS_VERBOSE): 
    news_check(player, cause, key, buf1, buf2); 
    break;

    case NEWS_LOGIN: 
    news_check(player, cause, key, buf1, buf2); 
    break;

    case NEWS_STATUS: 
    news_status(player, cause, key, buf1, buf2); 
    break;

    case (NEWS_STATUS + NEWS_VERBOSE): 
    news_status(player, cause, key, buf1, buf2); 
    break;

    case NEWS_YANK: 
    news_yank(player, cause, key, buf1, buf2); 
    break;

    case NEWS_EXTEND: 
    news_extend(player, cause, key, buf1, buf2); 
    break;

    case NEWS_SUBSCRIBE: 
    news_subscribe(player, cause, key, buf1, buf2); 
    break;

    case NEWS_UNSUBSCRIBE: 
    news_unsubscribe(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPADD: 
    news_groupadd(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPDEL: 
    news_groupdel(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPINFO: 
    news_groupinfo(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPLIST: 
    news_grouplist(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPMEM: 
    news_groupmem(player, cause, key, buf1, buf2); 
    break;

    case NEWS_GROUPLIMIT: 
    news_grouplimit(player, cause, key, buf1, buf2); 
    break;

    case NEWS_USERLIMIT: 
    news_userlimit(player, cause, key, buf1, buf2); 
    break;

    case NEWS_USERINFO: 
    news_userinfo(player, cause, key, buf1, buf2); 
    break;

    case NEWS_USERMEM: 
    news_usermem(player, cause, key, buf1, buf2); 
    break;

    case NEWS_READLOCK: 
    news_lock(player, cause, key, buf1, buf2); 
    break;

    case NEWS_POSTLOCK: 
    news_lock(player, cause, key, buf1, buf2); 
    break;

    case NEWS_EXPIRE: 
    news_expire(player, cause, key, buf1, buf2); 
    break;

    case NEWS_OFF: 
    news_off(player, cause, key, buf1, buf2); 
    break;

    default:
    notify(player,"Illegal combination of switches.");
    break;
  } /* switch */
}

void news_player_nuke_hook( dbref player, dbref target )
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsUserInfoRec ui;
  NewsUserInfoRec tempui;
  NewsUserGroupRec ug;
  NewsUserArticleRec ua;
  NewsGroupArticleInfoRec ga;

  char* groupkeyptr;
  int seqkey;
  dbref userkey;

  int found = 0;
  int ug_count = 0;
  int ua_count = 0;
  char *tpr_buff, *tprp_buff;

  /* Added Sep 15 2005 - Caused problems if news system was disabled
   * and a player was nuked.  --Odin
   */
  if ( !news_system_active ){ 
    return;
  }
  
  news_assert(get_XX(&xx));

  /* if player is a group adder, remove them from being an adder */

  tprp_buff = tpr_buff = alloc_lbuf("news_player_nuke_hook");
  for( groupkeyptr = xx.first_group;
       *groupkeyptr;
       groupkeyptr = gi.next_group ) {
    news_assert(get_GI(&gi, groupkeyptr));

    if( gi.adder == target ) {
      gi.adder = -1;
      put_GI(&gi);
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: Group '%s' adder removed.",
                             gi.name));
    }

    /* check to see if player is an admin of the group, and warn */
    if( user_passes_admin( target, gi.name ) ) {
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: WARNING: Player is an admin for group '%s'.", gi.name));
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: WARNING: Please correct admin lock on group '%s'.", gi.name));
    }
  }
  free_lbuf(tpr_buff);

  if( !get_UI(&ui, target) ) {
    return;
  }

  /* delete user's groups */
  for( groupkeyptr = ui.first_group;
       *groupkeyptr;
       groupkeyptr = ug.next_group ) {
    news_assert(get_UG(&ug, target, groupkeyptr));
    
    delete_user_from_group(ug.group_name, target);
    delete_group_from_user(target, ug.group_name);
    ug_count++;
  }

  /* delete user's article references, leave actual articles */
  for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
       *groupkeyptr;
       groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
    news_assert(get_UA(&ua, target, groupkeyptr, seqkey));

    news_assert(get_GA(&ga, ua.group_name, ua.seq));

    ga.owner_nuked_flag = OWNER_NUKE_DEAD;
 
    put_GA(&ga);

    delete_article_from_user(target, ua.group_name, ua.seq);
 
    ua_count++;
  }

  /* delete user's info rec */

  /* check for head delete */
  if( xx.first_user_dbref == target ) {
    xx.first_user_dbref = ui.next_user_dbref;
    put_XX(&xx);
  }
  else {
    for( userkey = xx.first_user_dbref;
         userkey != -1;
         userkey = tempui.next_user_dbref ) {
      news_assert(get_UI(&tempui, userkey));

      if( tempui.next_user_dbref == target ) {
        tempui.next_user_dbref = ui.next_user_dbref;
        put_UI(&tempui);
        found = 1;
        break;
      }
    }

    news_assert( found );
  }

  del_UI(&ui);

  notify(player, unsafe_tprintf("News: %d group(s) unsubscribed, %d article(s) cleaned.",
                         ug_count, ua_count));
}

/* ------------------- Sub command functions --------------------- */

void news_post(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  NewsGroupArticleInfoRec newga;
  static NewsGroupArticleTextRec newgt;
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserArticleRec newua;
  char *tpr_buff, *tprp_buff;
  DESC* d;

  time_t now;
  int found = 0;

  char *grouptok;
  char *groupkeyptr;
  char *titletok;

  char *titlebuffexec;
  char *textbuffexec;

  int tempseq;
  char *tempgroupkeyptr;

  time(&now);

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }

  if( !*buf1 ||
      !*buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  grouptok = buf1;
  titletok = strchr(buf1, '/');
  if( !titletok ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  *titletok = '\0';
  titletok++;

  groupkeyptr = groupfmt(player, grouptok);

  if( !groupkeyptr ) {
    return;
  }

  if( !*titletok ) {
    notify(player, "News: You must supply a title.");
    return;
  }

  if( !user_subscribed(player, groupkeyptr) ) {
    notify(player, "News: You are not subscribed to that group.");
    return;
  }

  if( !user_passes_post(player, groupkeyptr) ) {
    notify(player, "News: This group is locked in such a way as to prevent you from posting to it.");
    return;
  }

  news_assert(get_XX(&xx));
  news_assert(get_GI(&gi, groupkeyptr));
  news_assert(get_UI(&ui, player));

  if( articles_in_group(groupkeyptr) >= gi.posting_limit ) {
    notify(player, "News: This group is currently full. Please try again later.");
    return;
  }

  if( articles_by_user(player) >= xx.user_limit ) {
    notify(player, "News: You have reached your maximum posting limit. Please try again later.");
    return;
  }

  titlebuffexec = exec(player, player, player, EV_FCHECK|EV_STRIP|EV_EVAL, 
                       titletok, NULL, 0, (char **)NULL, 0);

  if( !*titlebuffexec ) {
    notify(player, "News: Your title must evaluate to something that isn't an empty string.");
    free_lbuf(titlebuffexec);
    return;
  }

  if( strlen(titlebuffexec) > ART_TITLE_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Article titles may be %d characters at most.",
                           ART_TITLE_LEN - 1));
    free_lbuf(titlebuffexec);
    return;
  }

  textbuffexec = exec(player, player, player, EV_FCHECK|EV_STRIP|EV_EVAL,
                      buf2, NULL, 0, (char **)NULL, 0);

  if( !*textbuffexec ) {
    notify(player, "News: Your text must evaluate to something that isn't an empty string.");
    free_lbuf(titlebuffexec);
    free_lbuf(textbuffexec);
    return;
  }

  if( strlen(textbuffexec) > ART_TEXT_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Article text may be %d characters at most.",
                           ART_TEXT_LEN - 1));
    free_lbuf(titlebuffexec);
    free_lbuf(textbuffexec);
    return;
  }

  /* All checks have passed, stick it in the db */

  memset(&newga, 0, sizeof(newga));
  strcpy(newga.group_name, groupkeyptr);
  newga.seq = gi.next_post_seq;
  strcpy(newga.title, titlebuffexec);
  strncpy(newga.owner_name, Name(player), USER_NAME_LEN);
  newga.owner_name[USER_NAME_LEN - 1] = '\0';
  newga.owner_dbref = player;
  newga.owner_nuked_flag = OWNER_NUKE_ALIVE;
  newga.post_time = now;

  /* set expiration time, check order is GI, XX */
  if( gi.def_article_life > 0 ) {
    newga.expire_time = now + gi.def_article_life;
  } else if ( gi.def_article_life == 0 ) {
    newga.expire_time = 0;
  } else {
    newga.expire_time = now + xx.def_article_life;
  }

  newga.next_seq = -1;

  memset(&newua, 0, sizeof(newua));
  newua.user_dbref = player;
  strcpy(newua.group_name, groupkeyptr);
  newua.seq = gi.next_post_seq;
  strcpy(newua.next_art_group, "");
  newua.next_art_seq = -1;

  memset(&newgt, 0, sizeof(newgt));
  strcpy(newgt.group_name, groupkeyptr);
  newgt.seq = gi.next_post_seq;
  strcpy(newgt.text, textbuffexec);

  free_lbuf(titlebuffexec);
  free_lbuf(textbuffexec);

  /* GA & GT: check for head insertion */
  if( gi.first_seq == -1 ) {
    newga.next_seq = gi.first_seq;
    gi.first_seq = newga.seq;
    put_GA(&newga);
    put_GT(&newgt);
    put_GI(&gi);
  }
  else {
    /* non-head insertion */
    found = 0;
    for( tempseq = gi.first_seq;
         tempseq != -1;
         tempseq = ga.next_seq ) {
      news_assert(get_GA(&ga, groupkeyptr, tempseq));

      if( ga.next_seq > newga.seq ||
          ga.next_seq == -1 ) {
        newga.next_seq = ga.next_seq;
        ga.next_seq = newga.seq;
        put_GA(&newga);
        put_GT(&newgt);
        put_GA(&ga);
        found = 1;
        break;
      }
    }
    news_assert( found );
  }

  /* UA: check for head insertion */
  if( !*ui.first_art_group ) {
    strcpy(newua.next_art_group, ui.first_art_group);
    newua.next_art_seq = ui.first_art_seq;
    strcpy(ui.first_art_group, groupkeyptr);
    ui.first_art_seq = newua.seq;
    put_UA(&newua);
    put_UI(&ui);
  }
  else {
    /* UA: non-head insertion */
    found = 0;
    for( tempgroupkeyptr = ui.first_art_group, tempseq = ui.first_art_seq;
         *tempgroupkeyptr;
         tempgroupkeyptr = ua.next_art_group, tempseq = ua.next_art_seq ) {
      news_assert(get_UA(&ua, player, tempgroupkeyptr, tempseq));

      if( strcmp(ua.next_art_group, groupkeyptr) > 0 ||
          !*ua.next_art_group ) {
        strcpy(newua.next_art_group, ua.next_art_group);
        newua.next_art_seq = ua.next_art_seq;
        strcpy(ua.next_art_group, groupkeyptr);
        ua.next_art_seq = newua.seq;
        put_UA(&newua);
        put_UA(&ua);
        found = 1;
        break;
      }
    }
    news_assert(found);
  }

  /* adjust GI */

  gi.next_post_seq++;
  gi.last_activity_time = now;

  put_GI(&gi);

  /* adjust UI */

  ui.total_posts++;
  
  put_UI(&ui);
    
  notify(player, unsafe_tprintf("News: Article %d posted to group '%s', expires %s.",
                         newga.seq, groupkeyptr, 
                         (newga.expire_time == 0 ? "never" : ez_ctime(newga.expire_time))));

  tprp_buff = tpr_buff = alloc_lbuf("news_post");
  DESC_ITER_CONN(d) {
    if( d->player != player &&
        user_subscribed(d->player, groupkeyptr) ) {
      tprp_buff = tpr_buff;
      notify(d->player, safe_tprintf(tpr_buff, &tprp_buff, "News: New article posted to group '%s' by %s: %s",
                             groupkeyptr, Name(player), newga.title));
    }
  }
  free_lbuf(tpr_buff);
}

void news_repost(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  NewsGroupArticleInfoRec newga;
  static NewsGroupArticleTextRec newgt;
  NewsUserArticleRec ua;
  char *tpr_buff, *tprp_buff;
  DESC* d;

  time_t now;
  int found = 0;

  char *grouptok;
  char *groupkeyptr;
  char *titletok;
  char *seqtok;

  char *titlebuffexec;
  char *textbuffexec;

  int tempseq;
  
  int seqkey;

  time(&now);

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }

  if( !*buf1 ||
      !*buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  grouptok = buf1;
  seqtok = strchr(buf1, '/');
  if( !seqtok ) {
    notify(player, "News: Invalid command format.");
    return;
  }
  *seqtok = '\0';
  seqtok++;
  titletok = strchr(seqtok, '/');
  if( !titletok ) {
    notify(player, "News: Invalid command format.");
    return;
  }
  *titletok = '\0';
  titletok++;

  seqkey = atoi(seqtok);

  groupkeyptr = groupfmt(player, grouptok);

  if( !groupkeyptr ) {
    return;
  }

  if( !*titletok ) {
    notify(player, "News: You must supply a title.");
    return;
  }

  if( !user_subscribed(player, groupkeyptr) ) {
    notify(player, "News: You are not subscribed to that group.");
    return;
  }

  if( !user_passes_post(player, groupkeyptr) ) {
    notify(player, "News: This group is locked in such a way as to prevent you from reposting to it.");
    return;
  }

  if( !get_UA(&ua, player, groupkeyptr, seqkey) ) {
    notify(player, "News: You do have an active posting with that id.");
    return;
  }

  news_assert(get_GA(&ga, groupkeyptr, seqkey));

  if( ga.post_time < now - (60 * 30) &&
      !Wizard(player) ) { /* thirty minutes ago */
    notify(player, "News: You posted this article more than 30 minutes ago.");
    notify(player, "News: Permission denied to repost.");
    return;
  }

  news_assert(get_XX(&xx));
  news_assert(get_GI(&gi, groupkeyptr));

  if( seqkey != gi.next_post_seq - 1 &&
      !Wizard(player) ) {
    notify(player, "News: Another posting has been made to this group");
    notify(player, "News: which is possibly in response to your article.");
    notify(player, "News: Reposting of your article is not permitted.");
    return;
  }

  titlebuffexec = exec(player, player, player, EV_FCHECK|EV_STRIP|EV_EVAL, 
                       titletok, NULL, 0, (char **)NULL, 0);

  if( !*titlebuffexec ) {
    notify(player, "News: Your title must evaluate to something that isn't an empty string.");
    free_lbuf(titlebuffexec);
    return;
  }

  if( strlen(titlebuffexec) > ART_TITLE_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Article titles may be %d characters at most.",
                           ART_TITLE_LEN - 1));
    free_lbuf(titlebuffexec);
    return;
  }

  textbuffexec = exec(player, player, player, EV_FCHECK|EV_STRIP|EV_EVAL,
                      buf2, NULL, 0, (char **)NULL, 0);

  if( !*textbuffexec ) {
    notify(player, "News: Your text must evaluate to something that isn't an empty string.");
    free_lbuf(titlebuffexec);
    free_lbuf(textbuffexec);
    return;
  }

  if( strlen(textbuffexec) > ART_TEXT_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Article text may be %d characters at most.",
                           ART_TEXT_LEN - 1));
    free_lbuf(titlebuffexec);
    free_lbuf(textbuffexec);
    return;
  }

  /* All checks have passed, stick it in the db */

  memset(&newga, 0, sizeof(newga));
  strcpy(newga.group_name, groupkeyptr);
  newga.seq = seqkey; 
  strcpy(newga.title, titlebuffexec);
  strncpy(newga.owner_name, Name(player), USER_NAME_LEN);
  newga.owner_name[USER_NAME_LEN - 1] = '\0';
  newga.owner_dbref = player;
  newga.owner_nuked_flag = OWNER_NUKE_ALIVE;
  newga.post_time = now;

  /* set expiration time, check order is GI, XX */
  if( gi.def_article_life > 0 ) {
    newga.expire_time = now + gi.def_article_life;
  } else if ( gi.def_article_life == 0 ) {
    newga.expire_time = 0;
  } else {
    newga.expire_time = now + xx.def_article_life;
  }

  newga.next_seq = -1;

  memset(&newgt, 0, sizeof(newgt));
  strcpy(newgt.group_name, groupkeyptr);
  newgt.seq = seqkey;
  strcpy(newgt.text, textbuffexec);

  free_lbuf(titlebuffexec);
  free_lbuf(textbuffexec);

  /* GA & GT: check for head insertion */
  news_assert(gi.first_seq != -1);

  /* non-head insertion */
  found = 0;
  for( tempseq = gi.first_seq;
       tempseq != -1;
       tempseq = ga.next_seq ) {
    news_assert(get_GA(&ga, groupkeyptr, tempseq));

    if( ga.seq == seqkey ) {
        newga.next_seq = ga.next_seq;
        put_GA(&newga);
        put_GT(&newgt);
        found = 1;
        break;
    }
  }
  news_assert( found );

  /* adjust GI */

  gi.last_activity_time = now;

  put_GI(&gi);

  notify(player, unsafe_tprintf("News: Article %d reposted to group '%s', expires %s.",
                         newga.seq, groupkeyptr, 
                         (newga.expire_time == 0 ? "never" : ez_ctime(newga.expire_time))));

  tprp_buff = tpr_buff = alloc_lbuf("news_repost");
  DESC_ITER_CONN(d) {
    if( d->player != player &&
        user_subscribed(d->player, groupkeyptr) ) {
      notify(d->player, safe_tprintf(tpr_buff, &tprp_buff, "News: Article %d reposted to group '%s' by %s: %s",
                             newga.seq,
                             groupkeyptr, Name(player), newga.title));
    }
  }
  free_lbuf(tpr_buff);
}

void news_postlist(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsGroupArticleInfoRec ga;
  dbref usertarget;
  char* uobuff;
  char* groupkeyptr, *tpr_buff, *tprp_buff;
  int seqkey;

  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !*buf1 ) { /* user command: postlist */
    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Sorry, only players may use this command.");
      return;
    }

    usertarget = player;
    notify(player, "-------------------------------------------------------------------------------");
    notify(player, "Your active articles:");
  }
  else { /* admin command: postlist <player> */
    if( !Wizard(player) ) {
      notify(player, "News: Permission Denied.");
      return;
    }

    usertarget = lookup_player(player, buf1, 1);

    if( usertarget == NOTHING ) {
      notify(player, "News: Unknown player specified.");
      return;
    }

    /* check for player */
    if( !isPlayer(usertarget) ) {
      notify(player, "News: Please specify a player.");
      return;
    }

    uobuff = unparse_object(player, usertarget, 1);

    notify(player, "-------------------------------------------------------------------------------");
    notify(player, unsafe_tprintf("Active articles for %s:", uobuff));
    free_lbuf(uobuff);
  }
  notify(player, "-------------------------------------------------------------------------------");

  if( get_UI(&ui, usertarget) ) {
    tprp_buff = tpr_buff = alloc_lbuf("news_postlist");
    for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
         *groupkeyptr;
         groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
      news_assert(get_GA(&ga, groupkeyptr, seqkey));
      news_assert(get_UA(&ua, usertarget, groupkeyptr, seqkey));

      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "(%8d) %-31.31s %-36.36s", 
                             ga.seq, ga.group_name, ga.title));
    }
    free_lbuf(tpr_buff);
  }
  notify(player, "-------------------------------------------------------------------------------");
}


void news_mailto(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  NewsGroupArticleTextRec gt;
  NewsUserInfoRec ui;

  int seqkey;
  char* buf1_end;
  char* groupkeyptr;
  char* tstrtokr;

  char* seqtok;
  char* grouptok;
  char* usertok;
  char* subjtok;
  char* texttok;

  char* mailcmd_buf1;
  char* mailcmd_buf1ptr;
  char* mailcmd_buf2;
  char* mailcmd_buf2ptr;
  char* header_buf;
  char* textbuffexec;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }

  /* check for buf2 */
  if( !*buf2 ) {
    notify( player, "News: Invalid command format." );
    return;
  }

  if( !get_UI(&ui, player) ) {
    notify(player, "News: You must first subscribe to a group.");
    return;
  }

  /* command format: mailto <group>/<seq> <address list>=[<subject>//]<text> */

  /* blast any trailing spaces in buf1 */
  buf1_end = buf1 + strlen( buf1 );

  while( isspace((int)*buf1_end) && buf1_end >= buf1 ) {
    *buf1_end = '\0';
    if( buf1_end > buf1 ) {
     buf1_end--;
    }
  }

  /* get group/seq */
  grouptok = strtok_r( buf1, " ", &tstrtokr );

  if( !grouptok ) {
    notify( player, "News: Invalid command format." );
    return;
  }

  /* get sequence */
  seqtok = strchr(buf1, '/');

  if( !seqtok ) {
    notify( player, "News: Invalid command format." );
    return;
  }

  /* separate group & sequence number */
  *seqtok = '\0';
  seqtok++;

  /* get user */
  usertok = seqtok + strlen( seqtok ) + 1;

  if( usertok >= buf1_end ) {
    notify( player, "News: Invalid command format." );
    return;
  }

  groupkeyptr = groupfmt(player, grouptok);

  if( !groupkeyptr ) {
    return;
  }

  if( !user_subscribed(player, groupkeyptr) ) {
    notify(player, "News: You are not subscribed to that group.");
    return;
  }

  if( !is_integer(seqtok) ) {
    notify(player, "News: Please specify an integer sequence number.");
    return;
  }

  seqkey = atoi(seqtok);

  /* we should now have at least a group key, and possibly a sequence key */
  /* the group has been checked for subscription but not readlock */
  /* the sequence key if there is one has not been tested for bounds */

  if( !user_passes_read(player, groupkeyptr) ) {
    notify(player, "News: You do not pass the read lock for this group.");
    return;
  }

  news_assert(get_GI(&gi, groupkeyptr));

  if( seqkey < 0 ||
      seqkey > gi.next_post_seq - 1 ) {
    notify(player, "News: Sequence number out of range for this group.");
    return;
  }

  if( gi.first_seq > seqkey ||
      !get_GA(&ga, groupkeyptr, seqkey) ) {
    notify(player, "News: The requested article has expired." );
    return;
  }

  /* send article and comment text to user */
  mailcmd_buf1ptr = mailcmd_buf1 = alloc_lbuf( "news_mailto" );

  if( !mailcmd_buf1 ) {
    notify(player, "News: Out of memory" );
    return;
  }

  mailcmd_buf2ptr = mailcmd_buf2 = alloc_lbuf( "news_mailto" );

  if( !mailcmd_buf2 ) {
    notify(player, "News: Out of memory" );
    free_lbuf(mailcmd_buf1);
    return;
  }

  header_buf = alloc_lbuf( "news_mailto" );

  if( !header_buf ) {
    notify(player, "News: Out of memory" );
    free_lbuf(mailcmd_buf2);
    free_lbuf(mailcmd_buf1);
    return;
  }

  /* prepare address list */
  safe_str( usertok, mailcmd_buf1, &mailcmd_buf1ptr );

  /* prepare players reply */
  textbuffexec = exec(player, player, player, EV_FCHECK|EV_STRIP|EV_EVAL,
                      buf2, NULL, 0, (char **)NULL, 0);

  /* check for subject line */
  texttok = strstr( textbuffexec, "//" );

  if( texttok ) {
    subjtok = textbuffexec;

    /* wipe out the // and position just beyond */
    *texttok = '\0';
    texttok++;
    *texttok = '\0';
    texttok++;
  }
  else {
    /* no subject line */
    subjtok = NULL;
    texttok = textbuffexec;
  }
  
  news_assert(get_GT(&gt, groupkeyptr, seqkey));

  /* start out with subject if specified */
  if( subjtok ) {
    safe_str( subjtok, mailcmd_buf2, &mailcmd_buf2ptr );
    safe_str( "//", mailcmd_buf2, &mailcmd_buf2ptr );
  }
  
  /* insert the header */
  /* Note: This sprintf will not overflow an lbuf */
  sprintf( header_buf, 
           "In regards to news article: %-.31s  by: %-.20s\r\n"
           "in group: %-.31s  posted on: %-24.24s\r\n", 
           ga.title, ga.owner_name,
           groupkeyptr, ez_ctime(ga.post_time) );

  safe_str( header_buf, mailcmd_buf2, &mailcmd_buf2ptr );

  /* insert header separator */
  safe_str( "---------------------------------------"
            "---------------------------------------\r\n", 
            mailcmd_buf2,
            &mailcmd_buf2ptr);

  if(get_GT(&gt, groupkeyptr, seqkey)) {
    safe_str( gt.text, mailcmd_buf2, &mailcmd_buf2ptr);
  }
  else {
    safe_str( "(Missing article text)\r\n", mailcmd_buf2, &mailcmd_buf2ptr);
  }

  safe_str( "\r\n--------------------------------------"
            "----------------------------------------\r\n", 
            mailcmd_buf2,
            &mailcmd_buf2ptr);

  /* append players reply */
  safe_str( texttok, mailcmd_buf2, &mailcmd_buf2ptr );

  free_lbuf( textbuffexec );
  free_lbuf( header_buf );

  /* if there was no subject line specified, knock down
  ** any '//' substrings, or the mailer will interpret
  ** them as a subject specifier. Don't like modifying
  ** the user's text, but there's no good solution
  ** to this problem.
  */
  if( !subjtok ) {
    subjtok = mailcmd_buf2;
    while( (subjtok = strstr(subjtok, "//")) ) {
      /* make second slash into a bar character */
      subjtok++;
      *subjtok = '|';
      subjtok++;
    }
  }

  notify(player, "News: Sending mail..." );

  // now ship it to the mail system
  do_mail( player, cause, M_SEND, mailcmd_buf1, mailcmd_buf2 );

  free_lbuf( mailcmd_buf1 );
  free_lbuf( mailcmd_buf2 );
}


void news_readall(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;

  char* groupkeyptr;

  int iterate = 0;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !get_UI(&ui, player) ) {
    notify(player, "News: You must first subscribe to a group.");
    return;
  }

  if( !*buf1 ) { /* command format: readall */
    groupkeyptr = ui.first_group;

    iterate = 1;
  }
  else { /* command format: readall <group> */
    groupkeyptr = groupfmt(player, buf1);

    if( !groupkeyptr ) {
      return;
    }

    if( !user_subscribed(player, groupkeyptr) ) {
      notify(player, "News: You are not subscribed to that group.");
      return;
    }

    iterate = 0;
  }

  /* we should now have a valid group key */
  /* the group has been checked for subscription but not readlock */

  if( iterate ) {
    do {
      news_readall_group( player, groupkeyptr );

      news_assert(get_UG(&ug, player, groupkeyptr));

      groupkeyptr = ug.next_group;
    } while( *groupkeyptr );
  }
  else {
    news_readall_group( player, groupkeyptr );

    /* update user info */
    strcpy(ui.last_read_group, groupkeyptr); 

    put_UI(&ui);
  }
}


/* NOTE: This function changes information in the UG record
** for the player in the db.
*/
void news_readall_group( dbref player, char* groupkeyptr )
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  static NewsGroupArticleTextRec gt;

  NewsUserGroupRec ug;

  char* workbuff, *tpr_buff, *tprp_buff;

  int found = 0;
  int seqkey;
  int tempseq;

  if( !user_passes_read(player, groupkeyptr) ) {
    notify(player, unsafe_tprintf(
      "News: You do not pass the read lock for the group '%s'.", groupkeyptr));
    return;
  }

  /* get group info */
  news_assert(get_GI(&gi, groupkeyptr));

  /* get user group info */
  news_assert(get_UG(&ug, player, groupkeyptr));

  seqkey = ug.seq_marker;

  if( seqkey > gi.next_post_seq - 1 ) {
    notify(player, unsafe_tprintf("News: No unread news in group '%s'.",
                           groupkeyptr));
    return;
  }

  /* now we have a sequence key and group that are all confirmed */

  /* loop through all articles in the group */
  tprp_buff = tpr_buff = alloc_lbuf("news_readall_group");
  do {
    /* grab this article, or seek to the next unexpired one */
    found = 0;

    if( !get_GA(&ga, groupkeyptr, seqkey) ) {
      for( tempseq = gi.first_seq;
           tempseq != -1;
           tempseq = ga.next_seq ) {
        news_assert(get_GA(&ga, groupkeyptr, tempseq));

        if( tempseq > seqkey ) {
          tprp_buff = tpr_buff;
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: Skipping %d expired %s in group '%s'.",
                                 tempseq - seqkey,
                                 tempseq - seqkey == 1 ? "article" : 
                                 "articles",
                                 groupkeyptr));
          seqkey = tempseq;
          found = 1;
          break;
        }
      }
    }
    else {
      found = 1;
    }

    if( !found ) {
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: All remaining articles have expired from group '%s'.",
                             groupkeyptr));
      seqkey = gi.next_post_seq;
    }
    else {
      /* now we have a GA and our seqkey has been adjusted if necessary */

      /* read article */

      notify(player, "-------------------------------------------------------------------------------");
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Group:   %-31.31s Posted:  %-24.24s",
                             groupkeyptr, ez_ctime(ga.post_time)));

      workbuff = alloc_lbuf("news/read");
      sprintf(workbuff, "%d of %d", seqkey, gi.next_post_seq - 1); 
      tprp_buff = tpr_buff;
      if( ga.expire_time ) {
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Article: %-31.31s Expires: %-24.24s",
                               workbuff, ez_ctime(ga.expire_time)));
      }
      else {
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Article: %-31.31s Expires: [Never]",
                               workbuff));
      }
      free_lbuf(workbuff);

      tprp_buff = tpr_buff;
      if( ga.owner_nuked_flag == OWNER_NUKE_DEAD ) {
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Post By: %s [NUKED]",
                               ga.owner_name));
      }
      else if( strcmp(ga.owner_name, Name(ga.owner_dbref)) != 0 ) {
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Post By: %s [NAME CHANGED TO : %s]",
                               ga.owner_name, 
                               Name(ga.owner_dbref)));
      }
      else {
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Post By: %s",
                               ga.owner_name));
      }
  
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Title:   %s", ga.title));
      notify(player, "-------------------------------------------------------------------------------");

      if( get_GT(&gt, groupkeyptr, seqkey) ) {
        notify(player, gt.text);
      }
      else {
        notify(player, "(Missing article text)");
      }
      notify(player, "-------------------------------------------------------------------------------");

      seqkey++;
    }
  } while( seqkey < gi.next_post_seq );
  free_lbuf(tpr_buff);

  /* update user group info */
  ug.seq_marker = seqkey;

  put_UG(&ug);
}


void news_read(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  NewsGroupArticleTextRec gt;
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;

  int seqkey = 0;
  int tempseq = 0;
  char* groupkeyptr;

  char* seqtok;
  char* grouptok;

  char* workbuff, *tpr_buff, *tprp_buff;

  int update_marker = 0;
  int gotseq = 0;

  int found = 0;
  int group_read = 0;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !get_UI(&ui, player) ) {
    notify(player, "News: You must first subscribe to a group.");
    return;
  }

  if( !*buf1 ) { /* command format: read */
    if( !user_subscribed(player, ui.last_read_group) ) {
      notify(player, "News: You do not have a valid default group.");
      return;
    }
 
    groupkeyptr = ui.last_read_group;

    update_marker = 1;
  }
  else { /* command format: read <group>, read <seq>, read <group>/<seq> */
    grouptok = buf1;
    seqtok = strchr(buf1, '/');

    if( !seqtok ) { /* command format: read <group>, read <seq> */
      if( is_integer(grouptok) ) { /* command format: read <seq> */
        if( !user_subscribed(player, ui.last_read_group) ) {
          notify(player, "News: You do not have a valid default group.");
          return;
        }
       
        groupkeyptr = ui.last_read_group;
        seqkey = atoi(grouptok);  
        gotseq = 1;
      }
      else { /* command format: read <group> */
        groupkeyptr = groupfmt(player, grouptok);

        if( !groupkeyptr ) {
          return;
        }

        if( !user_subscribed(player, groupkeyptr) ) {
          notify(player, "News: You are not subscribed to that group.");
          return;
        }

        update_marker = 1;
      }
    }
    else { /* command format: read <group>/<seq> */
      *seqtok = '\0';
      seqtok++;

      groupkeyptr = groupfmt(player, grouptok);

      if( !groupkeyptr ) {
        return;
      }

      if( !user_subscribed(player, groupkeyptr) ) {
        notify(player, "News: You are not subscribed to that group.");
        return;
      }

      if( !is_integer(seqtok) ) {
        notify(player, "News: Please specify an integer sequence number.");
        return;
      }

      seqkey = atoi(seqtok);
      gotseq = 1;
      update_marker = 1;
/* Jeh
 *    group_read = 1;
 */
    }
  }

  /* we should now have at least a group key, and possibly a sequence key */
  /* the group has been checked for subscription but not readlock */
  /* the sequence key if there is one has not been tested for bounds */

  if( !user_passes_read(player, groupkeyptr) ) {
    notify(player, "News: You do not pass the read lock for this group.");
    return;
  }

  news_assert(get_GI(&gi, groupkeyptr));

  if( gotseq &&
      (seqkey < 0 ||
       seqkey > gi.next_post_seq - 1) ) {
    notify(player, "News: Sequence number out of range for this group.");
    return;
  }

  news_assert(get_UG(&ug, player, groupkeyptr));

  if( !gotseq ) {
    seqkey = ug.seq_marker;

    if( seqkey > gi.next_post_seq - 1 ) {
      notify(player, unsafe_tprintf("News: No more unread news in group '%s'.",
                             groupkeyptr));
      return;
    }
  }

  /* now we have a sequence key and group that are all confirmed */

  /* grab this article, or seek to the next unexpired one */

  if( !get_GA(&ga, groupkeyptr, seqkey) ) {
    for( tempseq = gi.first_seq;
         tempseq != -1;
         tempseq = ga.next_seq ) {
      news_assert(get_GA(&ga, groupkeyptr, tempseq));

      if( tempseq > seqkey ) {
        tprp_buff = tpr_buff = alloc_lbuf("news_read");
        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: Skipping %d expired %s.",
                               tempseq - seqkey,
                               tempseq - seqkey == 1 ? "article" : 
                               "articles"));
        seqkey = tempseq;
        found = 1;
        free_lbuf(tpr_buff);
        break;
      }
    }

    if( !found ) {
      if( update_marker ) {
        notify(player, unsafe_tprintf("News: All remaining articles have expired from group '%s'.",
                               groupkeyptr));
        seqkey = gi.next_post_seq;
      }
      else {
        notify(player, "News: The specified article has expired.");
        return;
      }
    }
  }

  /* now we have a GA and our seqkey has been adjusted if necessary */

  /* check again to see if we're still in bounds, because all articles
     may have expired */

  if( seqkey < gi.next_post_seq ) {
    /* read article */

    notify(player, "-------------------------------------------------------------------------------");
    notify(player, unsafe_tprintf("Group:   %-31.31s Posted:  %-24.24s",
                           groupkeyptr, ez_ctime(ga.post_time)));

    workbuff = alloc_lbuf("news/read");
    sprintf(workbuff, "%d of %d", seqkey, gi.next_post_seq - 1); 
    if( ga.expire_time ) {
      notify(player, unsafe_tprintf("Article: %-31.31s Expires: %-24.24s",
                             workbuff, ez_ctime(ga.expire_time)));
    }
    else {
      notify(player, unsafe_tprintf("Article: %-31.31s Expires: [Never]",
                             workbuff));
    }
    free_lbuf(workbuff);

    if( ga.owner_nuked_flag == OWNER_NUKE_DEAD ) {
      notify(player, unsafe_tprintf("Post By: %s [NUKED]",
                             ga.owner_name));
    }
    else if( strcmp(ga.owner_name, Name(ga.owner_dbref)) != 0 ) {
      notify(player, unsafe_tprintf("Post By: %s [NAME CHANGED TO : %s]",
                             ga.owner_name, 
                             Name(ga.owner_dbref)));
    }
    else {
      notify(player, unsafe_tprintf("Post By: %s",
                             ga.owner_name));
    }

    notify(player, unsafe_tprintf("Title:   %s",
                           ga.title));
    notify(player, "-------------------------------------------------------------------------------");

    if(get_GT(&gt, groupkeyptr, seqkey)) {
      notify(player, gt.text);
    }
    else {
      notify(player, "(Missing article text)");
    }
    notify(player, "-------------------------------------------------------------------------------");
  }


  /* update the marker if we should */
  if( update_marker ) { 
    seqkey++;

    if ( group_read  && ((ug.seq_marker + 1) == seqkey) )
       ug.seq_marker = seqkey;
    else if ( !group_read && (seqkey >= ug.seq_marker) )
       ug.seq_marker = seqkey;

    put_UG(&ug);

    strcpy(ui.last_read_group, groupkeyptr); 

    put_UI(&ui);
  }
}

void news_jump(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  NewsGroupInfoRec gi;
  char* groupkeyptr;
  char* tok;
  char* tstrtokr;
  int jumpseq = 0;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !get_UI(&ui, player) ) {
    notify(player, "News: You must first subscribe to a group.");
    return;
  }

  if( !*buf1 ) { /* jump to end of current group */
    if( !user_subscribed(player, ui.last_read_group) ) {
      notify(player, "News: You do not have a valid default group.");
      return;
    }
    news_assert(get_GI(&gi, ui.last_read_group));
    groupkeyptr = ui.last_read_group;
    jumpseq = gi.next_post_seq;
  }
  else { /* one of: jump <group>, jump <seq>, jump <group>/<seq> */
    if( strchr(buf1, '/') ) { /* jump <group>/<seq> */
      tok = strtok_r(buf1, "/", &tstrtokr);
      tok = strtok_r(NULL, "/", &tstrtokr);

      groupkeyptr = groupfmt(player, buf1);

      if( !groupkeyptr ) {
        return;
      }

      if( !user_subscribed(player, groupkeyptr) ) {
        notify(player, "News: You are not subscribed to that group.");
        return;
      }

      if( !is_integer(tok) ) {
        notify(player, "News: Please specify an integer for the sequence number.");
        return;
      }

      jumpseq = atoi(tok);

      if( jumpseq < 0 ) {
        notify(player, "News: Please specify a positive integer.");
        return;
      }

      news_assert(get_GI(&gi, groupkeyptr));

      if( jumpseq > gi.next_post_seq ) {
        notify(player, "News: The sequence number specified is past the end of the group.");
        notify(player, "News: The actual end of group will be substituted instead.");
        jumpseq = gi.next_post_seq;
      }
    }
    else {
      if( is_integer(buf1) ) { /* jump <seq> */
        if( !user_subscribed(player, ui.last_read_group) ) {
          notify(player, "News: You do not have a valid default group.");
          return;
        }
        groupkeyptr = ui.last_read_group;

        jumpseq = atoi(buf1);

        if( jumpseq < 0 ) {
          notify(player, "News: Please specify a positive integer.");
          return;
        }

        news_assert(get_GI(&gi, ui.last_read_group));

        if( jumpseq > gi.next_post_seq ) {
          notify(player, "News: The sequence number specified is past the end of the group.");
          notify(player, "News: The actual end of group will be substituted instead.");
          jumpseq = gi.next_post_seq;
        }
      }
      else { /* jump <group> */
        groupkeyptr = groupfmt(player, buf1);

        if( !groupkeyptr ) {
          return;
        }

        if( !user_subscribed(player, groupkeyptr) ) {
          notify(player, "News: You are not subscribed to that group.");
          return;
        }

        news_assert(get_GI(&gi, groupkeyptr));
        jumpseq = gi.next_post_seq;
      }
    }
  }

  if( !user_passes_read(player, groupkeyptr) ) {
    notify(player, "News: You do not pass the read lock of that group.");
    return;
  }

  news_assert(get_UG(&ug, player, groupkeyptr));

  ug.seq_marker = jumpseq;

  put_UG(&ug);

  notify(player, unsafe_tprintf("News: Jumped to position %d of group '%s'.",
                       jumpseq < 0 ? 0 : jumpseq, groupkeyptr));
}

void news_check(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  char *groupkeyptr;
  int seqkey;
  int silent = 0;
  int verbose = 0;
  int unreadcount = 0;
  int printcount = 0;

  dbref usertarget;

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( key == NEWS_LOGIN ) {

    /* special case, cause of invocation becomes target */
    usertarget = cause;

    /* fail silently for guest login */
    if( Guest(cause) ) {
      return;
    }

    /* check for player */
    if( !isPlayer(cause) ) {
      notify(cause, "News: Sorry, only players may use this command.");
      return;
    }
    create_user_if_new(usertarget, usertarget);
    silent = 1;
  }
  else if( key & NEWS_VERBOSE ) {
    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Sorry, only players may use this command.");
      return;
    }

    usertarget = player;

    verbose = 1;
  }
  else {
    usertarget = player;
  }

  /* iterate through the user's groups and display unread stats */

  if( !get_UI(&ui, usertarget) ) {
    return;
  }

  for( groupkeyptr = ui.first_group;
       *groupkeyptr;
       groupkeyptr = ug.next_group ) {
    news_assert(get_UG(&ug, usertarget, groupkeyptr));

    news_assert(get_GI(&gi, ug.group_name));

    unreadcount = 0;
    for( seqkey = gi.first_seq;
         seqkey != -1;
         seqkey = ga.next_seq ) {
      news_assert(get_GA(&ga, gi.name, seqkey));

      if( ug.seq_marker <= ga.seq ) {
        unreadcount++;
      }
    }

    if( unreadcount ||
        verbose ) {
      notify(usertarget, unsafe_tprintf("News: %d unread %s in group '%s'%s",
             unreadcount, unreadcount == 1 ? "article" : "articles",
             gi.name, 
             strcmp(gi.name, ui.last_read_group) ? "." :
             " [default]."));
      printcount++;
    }
  }

  if( !printcount &&
      !silent ) {
    notify(usertarget, "News: No unread news.");
  }
}

void news_status(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  char* groupkeyptr, *tpr_buff, *tprp_buff;
  int seqkey;
  int verbose = 0;
  int printcount = 0;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }

  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( key & NEWS_VERBOSE ) {
    verbose = 1;
  }

  if( !get_UI(&ui, player) ) {
    notify(player, "News: You must first subscribe to a group.");
    return;
  }

  if( !*buf1 ) { /* command format: news/status <group> */
    if( !user_subscribed(player, ui.last_read_group) ) {
      notify(player, "News: You do not have a valid default group.");
      return;
    }

    groupkeyptr = ui.last_read_group;
  }
  else {
    groupkeyptr = groupfmt(player, buf1); 

    if( !groupkeyptr ) {
      return;
    }

    if( !user_subscribed(player, groupkeyptr) ) {
      notify(player, "News: You are not subscribed to that group.");
      return;
    }
  }

  if( !user_passes_read(player, groupkeyptr) ) {
    notify(player, "News: You do not pass the read lock for that group.");
    return;
  }

  notify(player, "-------------------------------------------------------------------------------");

  if( verbose ) {
    notify(player, unsafe_tprintf("Active articles in group '%s':",
                           groupkeyptr));
  }
  else {
    notify(player, unsafe_tprintf("Unread articles in group '%s':",
                           groupkeyptr));
  }

  notify(player, "-------------------------------------------------------------------------------");

  news_assert(get_UG(&ug, player, groupkeyptr));
  news_assert(get_GI(&gi, groupkeyptr));

  tprp_buff = tpr_buff = alloc_lbuf("news_check");
  for( seqkey = gi.first_seq;
       seqkey != -1;
       seqkey = ga.next_seq ) {
    news_assert(get_GA(&ga, groupkeyptr, seqkey));

   tprp_buff = tpr_buff;
    if( ug.seq_marker > ga.seq &&
        verbose) {
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "[R] (%8d) %-15.15s Title: %-39.39s",
                             ga.seq,
                             ga.owner_nuked_flag == OWNER_NUKE_DEAD ?
                             ga.owner_name : Name(ga.owner_dbref),
                             ga.title));
      printcount++;
    }
    else if( ug.seq_marker <= ga.seq ) {
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "[U] (%8d) %-15.15s Title: %-39.39s",
                             ga.seq,
                             ga.owner_nuked_flag == OWNER_NUKE_DEAD ?
                             ga.owner_name : Name(ga.owner_dbref),
                             ga.title));
      printcount++;
    }
  }
  free_lbuf(tpr_buff);

  if( !printcount ) {
    notify(player, "None.");
  }

  notify(player, "-------------------------------------------------------------------------------");
}

void news_yank(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  DESC* d;
  char* groupkeyptr;
  char* grouptok;
  char* seqtok, *tpr_buff, *tprp_buff;
  int seqkey;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  grouptok = buf1;
  seqtok = strchr(buf1, '/');
  if( !seqtok ) {
    notify(player, "News: Invalid command format.");
    return;
  }
  *seqtok = '\0';
  seqtok++;

  groupkeyptr = groupfmt(player, grouptok);
   
  if( !groupkeyptr ) {
    return;
  }

  if( !is_integer(seqtok) ) {
    notify(player, "News: Please specify an integer sequence number.");
    return;
  }

  seqkey = atoi(seqtok);

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }

  if( !get_GA(&ga, groupkeyptr, seqkey) ) {
    notify(player, "News: That is not an active posting.");
    return;
  }

  if( user_passes_admin( player, groupkeyptr ) ||
      Wizard(player) ||
      (ga.owner_dbref == player &&
       gi.next_post_seq - 1 == ga.seq &&
       user_passes_post(player, groupkeyptr)) ) {
    /* ok to yank article */
    delete_article_from_group(groupkeyptr, seqkey);
    delete_article_from_user(ga.owner_dbref, groupkeyptr, seqkey);

    notify(player, unsafe_tprintf("News: Article %d yanked from group '%s'.",
                           seqkey, groupkeyptr));

    tprp_buff = tpr_buff = alloc_lbuf("news_yank");
    DESC_ITER_CONN(d) {
      if( d->player != player &&
          user_subscribed(d->player, groupkeyptr) ) {
        tprp_buff = tpr_buff;
        notify(d->player, safe_tprintf(tpr_buff, &tprp_buff, "News: Article %d yanked from group '%s' by %s.",
                               seqkey,
                               groupkeyptr, Name(player)));
      }
    }
    free_lbuf(tpr_buff);
  }
  else {
    if( ga.owner_dbref != player ) {
      notify(player, "News: Permission denied.");
      return;
    }
    if( !user_passes_post(player, groupkeyptr) ) {
      notify(player, "News: You no longer pass the post lock on that group.");
      return;
    }
    notify(player, "News: Another posting has been made to this group");
    notify(player, "News: which is possibly in response to your article.");
    notify(player, "News: Yanking of your article is not permitted.");
  }
}

void news_extend(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  char* grouptok;
  char* seqtok;
  int seqkey;
  char* groupkeyptr;
  int extendval;
  time_t now;

  time(&now);

  grouptok = buf1;
  seqtok = strchr(buf1, '/');

  if( !seqtok ) {
    notify(player, "News: Invalid command format.");
    return;
  }
  *seqtok = '\0';
  seqtok++;

  groupkeyptr = groupfmt(player, grouptok);

  if( !groupkeyptr ) {
    return;
  }

  if( !is_integer(seqtok) ) {
    notify(player, "News: Please specify an integer sequence number.");
    return;
  }

  if( !is_integer(buf2) &&
      strcasecmp(buf2, "forever") != 0 ) {
    notify(player, "News: Please specify an integer number of days or 'forever'.");
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }

  if( !user_passes_admin(player, groupkeyptr) &&
      !Wizard(player) ) {
    notify(player, "News: Permission Denied.");
    return;
  }

  seqkey = atoi(seqtok);

  if( !get_GA(&ga, groupkeyptr, seqkey) ) {
    notify(player, "News: The specified sequence number is not active.");
    return;
  }

  if( strcasecmp(buf2, "forever") == 0 ) {
    ga.expire_time = 0;
    notify(player, unsafe_tprintf("News: Extending expiration forever on article %d of group '%s'.", seqkey, groupkeyptr));
  }
  else {
    extendval = atoi(buf2);
    if( !ga.expire_time ) {
      notify(player, "News: Article was set to never expire, offset taken from current time.");
      ga.expire_time = now + extendval * (60 * 60 * 24);
    }
    else {
      ga.expire_time = ga.expire_time + extendval * (60 * 60 * 24);
    }
   
    if( ga.expire_time == 0 ) { /* 0 is special value for forever */
      ga.expire_time = 1;
    }
 
    notify(player, unsafe_tprintf("News: Expiration on article %d of group '%s' now %s.", seqkey, groupkeyptr, ez_ctime(ga.expire_time)));

    if( ga.expire_time < now ) {
      notify(player, "News: Expiration date is in the past, expiration is now pending.");
    }
  }
  put_GA(&ga);
}

char *news_function_group(dbref player, dbref cause, int key)
{
  NewsGroupInfoRec gi;
  NewsGlobalRec xx;
  char *groupkeyptr, *grouplbuf, *grouplbufptr;
  int first;

  grouplbufptr = grouplbuf = alloc_lbuf("news_function_group");

  if( !news_system_active || news_internal_failure) {
     return(grouplbuf);
  }
 
  first = 0;
  news_assert_retnonevoid(get_XX(&xx), NULL);
  for( groupkeyptr = xx.first_group;
       *groupkeyptr;
       groupkeyptr = gi.next_group ) {
    news_assert_retnonevoid(get_GI(&gi, groupkeyptr), NULL);

    if ( !key || (key && (user_passes_post(player, gi.name) || 
           user_passes_read(player, gi.name))) ) {
       if ( first )
           safe_chr(' ', grouplbuf, &grouplbufptr);
       safe_str(gi.name, grouplbuf, &grouplbufptr);
       first = 1;
    }
  } 

  return(grouplbuf);
}

void news_subscribe(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  char* groupkeyptr;
  char* uobuff;
  dbref usertarget;

  if( !*buf1 ) {
    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Sorry, only players may use this command.");
      return;
    }

    create_user_if_new(player, player);
    /* list user's subscribed groups */
    if( !get_UI(&ui, player) ||
        !*ui.first_group ) {
      notify(player, "News: You are not subscribed to any groups.");
      return;
    }
    else {
      notify(player, "-------------------------------------------------------------------------------");
      notify(player, "Group Name                       Current Article #");
      notify(player, "-------------------------------------------------------------------------------");
      for( groupkeyptr = ui.first_group;
           *groupkeyptr;
           groupkeyptr = ug.next_group ) {
        news_assert(get_UG(&ug, player, groupkeyptr));

        notify(player, unsafe_tprintf("%-31s  %d", ug.group_name, 
                               ug.seq_marker));
      }
      notify(player, "-------------------------------------------------------------------------------");
      return;
    }
  }
  else {
    /* attempt to subscribe user to group */

    if( *buf2 ) {   /* admin version: subscribe <player>=<group> */
      groupkeyptr = groupfmt(player, buf2);
       
      if( !groupkeyptr ) {
        return;
      }

      if( !get_GI(&gi, groupkeyptr) ) {
        notify(player, "News: Unknown group specified.");
        return;
      }

      if( !user_passes_admin(player, groupkeyptr) &&
          !Wizard(player) ) {
        notify(player, "News: Improper command format, refer to help.");
        return;
      }

      usertarget = lookup_player(player, buf1, 1);

      if( usertarget == NOTHING ) {
        notify(player, "News: Unknown player specified.");
        return;
      }

      /* check for player */
      if( !isPlayer(usertarget) ) {
        notify(player, "News: Please specify a player.");
        return;
      }

      create_user_if_new(player, usertarget);

      uobuff = unparse_object(player, usertarget, 1);

      if( user_subscribed(usertarget, groupkeyptr) ) {
        notify(player, unsafe_tprintf(
               "News: %s is already subscribed to that group.",
               uobuff));
        free_lbuf(uobuff); 
        return;
      }
      /* admins and royalty can add players to groups without them having
         to pass the post or read locks */
      add_user_to_group(groupkeyptr, usertarget);
      add_group_to_user(usertarget, groupkeyptr);

      notify(player, unsafe_tprintf(
             "News: %s subscribed to group '%s'.",
             uobuff, groupkeyptr));
      free_lbuf(uobuff); 
      return;
    }
    else {  /* player version of command: subscribe <group> */
      /* check for player */
      if( !isPlayer(player) ) {
        notify(player, "News: Sorry, only players may use this command.");
        return;
      }
      groupkeyptr = groupfmt(player,buf1);

      if( !groupkeyptr ) {
        return;
      }

      create_user_if_new(player, player);

      if( !get_GI(&gi, groupkeyptr) ) {
        notify(player, "News: Unknown group specified.");
        return;
      }
    
      if( user_subscribed(player, groupkeyptr) ) {
        notify(player, "News: You are already subscribed to that group.");
        return;
      }

      if( !user_passes_read(player, groupkeyptr) &&
          !user_passes_post(player, groupkeyptr) ) {
        notify(player, "News: This group is locked in a way that excludes your membership."); 
        return;
      }

      add_user_to_group(groupkeyptr, player);
      add_group_to_user(player, groupkeyptr);
      notify(player, unsafe_tprintf("News: You are now subscribed to group '%s'.",
                             groupkeyptr));
    }
  }
}

void news_unsubscribe(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  char* groupkeyptr;
  char* uobuff;
  dbref usertarget;

  /* attempt to unsubscribe user from group */

  if( *buf2 ) {   /* admin version: unsubscribe <player>=<group> */
    groupkeyptr = groupfmt(player, buf2);
     
    if( !groupkeyptr ) {
      return;
    }

    if( !get_GI(&gi, groupkeyptr) ) {
      notify(player, "News: Unknown group specified.");
      return;
    }

    if( !user_passes_admin( player, groupkeyptr) &&
        !Wizard(player) ) {
      notify(player, "News: Improper command format, refer to help.");
      return;
    }

    if( strcmp(groupkeyptr, "info") == 0 ) {
      notify(player, "News: Users cannot be unsubscribed from the 'info' group.");
      return;
    }

    usertarget = lookup_player(player, buf1, 1);

    if( usertarget == NOTHING ) {
      notify(player, "News: Unknown player specified.");
      return;
    }

    /* check for player */
    if( !isPlayer(usertarget) ) {
      notify(player, "News: Please specify a player.");
      return;
    }

    uobuff = unparse_object(player, usertarget, 1);

    if( !user_subscribed(usertarget, groupkeyptr) ) {
      notify(player, unsafe_tprintf(
             "News: %s is not subscribed to that group.",
             uobuff));
      free_lbuf(uobuff); 
      return;
    }

    delete_user_from_group(groupkeyptr, usertarget);
    delete_group_from_user(usertarget, groupkeyptr);

    notify(player, unsafe_tprintf(
           "News: %s unsubscribed from group '%s'.",
           uobuff, groupkeyptr));
    free_lbuf(uobuff);
    return;
  }
  else {  /* player version of command: unsubscribe <group> */
    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Sorry, only players may use this command.");
      return;
    }
    groupkeyptr = groupfmt(player,buf1);

    if( !groupkeyptr ) {
      return;
    }

    if( !get_GI(&gi, groupkeyptr) ) {
      notify(player, "News: Unknown group specified.");
      return;
    }

    if( !user_subscribed(player, groupkeyptr) ) {
      notify(player, "News: You are not subscribed to that group.");
      return;
    }

    if( strcmp(groupkeyptr, "info") == 0 ) {
      notify(player, "News: You cannot unsubscribe from the 'info' group.");
      return;
    }

    delete_user_from_group(groupkeyptr, player);
    delete_group_from_user(player, groupkeyptr);
    notify(player, unsafe_tprintf("News: You are now unsubscribed from group '%s'.",
                           groupkeyptr));
  }
}

void news_groupadd(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupInfoRec newgi;
  char *groupkeyptr;
  time_t now;
  int found = 0;

  time(&now);

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }

  groupkeyptr = groupfmt(player, buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !*buf2 ) {
    notify(player, "News: You must specify a group description.");
    return;
  }
  
  if( strlen(buf2) > GROUP_DESC_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Group descriptions can have a maximum of %d characters.", GROUP_DESC_LEN - 1));
    return;
  }

  if( get_GI(&gi, groupkeyptr) ) {
/*  notify(player, unsafe_tprintf("News: The group '%s' already exists.", 
                           groupkeyptr)); */
    notify(player, unsafe_tprintf("News: Group '%s' given new description of '%s'.", gi.name, buf2));
    strcpy(gi.desc, buf2);
    put_GI(&gi);
    return;
  }

  news_assert(get_XX(&xx));

  /* initialize record */
  memset(&newgi, 0, sizeof(newgi));
  strcpy(newgi.name, groupkeyptr);
  strcpy(newgi.desc, buf2);
  newgi.adder = player;
  newgi.posting_limit = 1000;
  newgi.create_time = now;
  newgi.def_article_life = -1;
  newgi.last_activity_time = 0;
  newgi.next_post_seq = 0;
  newgi.first_seq = -1;
  newgi.first_user_dbref = -1;
  strcpy(newgi.next_group, "");

  /* position it in the db in-order */
  if( strcmp(xx.first_group,groupkeyptr) > 0 ||
      !*xx.first_group ) {
    /* first existing group name is lexically higher, so we have a new first */
    strcpy(newgi.next_group, xx.first_group);
    strcpy(xx.first_group, groupkeyptr);
    put_GI(&newgi);
    put_XX(&xx);
  }
  else {
    for( groupkeyptr = xx.first_group; 
         *groupkeyptr; 
         groupkeyptr = gi.next_group ) {
      news_assert(get_GI(&gi,groupkeyptr)); 

      if( strcmp(gi.next_group, newgi.name) > 0 ||
          !*gi.next_group ) {
        /* new record goes after this one */
        strcpy(newgi.next_group, gi.next_group);
        strcpy(gi.next_group, newgi.name);
        put_GI(&newgi);
        put_GI(&gi);
        found = 1;
        break;
      }
    }
    news_assert(found);
  }
  notify(player, unsafe_tprintf("News: Group '%s' added.", newgi.name));
}

void news_groupdel(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupInfoRec tempgi;
  NewsGroupArticleInfoRec ga;
  NewsGroupReadLockRec gr;
  NewsGroupPostLockRec gp;
  NewsGroupAdminLockRec gx;
  char *groupkeyptr;
  char *tempkeyptr;
  dbref userkey;
  int seqkey;
  int found = 0;

  int ga_count = 0;
  int gu_count = 0;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  groupkeyptr = groupfmt(player, buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !strcmp(groupkeyptr, "info") ||
      !strcmp(groupkeyptr, "general") ) {
    notify(player, unsafe_tprintf("News: The group '%s' is a system group, and cannot be deleted.", groupkeyptr));
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, unsafe_tprintf("News: The group '%s' does not exist.", 
                           groupkeyptr));
    return;
  }

  news_assert(get_XX(&xx));

  /* clean up GroupArticleInfo/GroupArticleText and 
     UserArticles */

  /* GI is constantly changing in this loop due to the call to 
     delete_article_from_group */
  for( seqkey = gi.first_seq;
       seqkey != -1;
       seqkey = gi.first_seq ) {
    news_assert(get_GA(&ga, groupkeyptr, seqkey));
    delete_article_from_user(ga.owner_dbref, groupkeyptr, seqkey);
    delete_article_from_group(groupkeyptr, seqkey);
    ga_count++;
   
    news_assert(get_GI(&gi, groupkeyptr));
  }

  notify(player, unsafe_tprintf("News: %d articles deleted from '%s'.",
                         ga_count, groupkeyptr));

  /* clean up GroupUsers and UserGroups */

  /* GI is constantly changing in this loop due to the call to 
     delete_user_from_group */
  for( userkey = gi.first_user_dbref;
       userkey != -1;
       userkey = gi.first_user_dbref ) {
    delete_group_from_user(userkey, groupkeyptr);
    delete_user_from_group(groupkeyptr, userkey);
    gu_count++;

    news_assert(get_GI(&gi, groupkeyptr));
  }

  notify(player, unsafe_tprintf("News: %d users removed from '%s'.",
                         gu_count, groupkeyptr));

  if( get_GR(&gr, groupkeyptr) ) {
    del_GR(&gr);
    notify(player, unsafe_tprintf("News: Read lock removed from '%s'.", groupkeyptr));
  }

  if( get_GP(&gp, groupkeyptr) ) {
    del_GP(&gp);
    notify(player, unsafe_tprintf("News: Post lock removed from '%s'.", groupkeyptr));
  }

  if( get_GX(&gx, groupkeyptr) ) {
    del_GX(&gx);
    notify(player, unsafe_tprintf("News: Admin lock removed from '%s'.", groupkeyptr));
  }

  /* check for gi as first group */
  if( strcmp(xx.first_group,groupkeyptr) == 0 ) {
    /* first existing group name is our group */
    strcpy(xx.first_group, gi.next_group);
    put_XX(&xx);
  }
  else {
    /* gi is in the rest of the list */
    for( tempkeyptr = xx.first_group; 
         *tempkeyptr; 
         tempkeyptr = tempgi.next_group ) {
      news_assert(get_GI(&tempgi,tempkeyptr)); 

      if( strcmp(tempgi.next_group, groupkeyptr) == 0 ) {
        /* record of interest is after this one */
        strcpy(tempgi.next_group, gi.next_group);
        put_GI(&tempgi);
        found = 1;
        break;
      }
    }
    news_assert(found);
  }
  del_GI(&gi);

  notify(player, unsafe_tprintf("News: Group '%s' deleted.", groupkeyptr));
  return;
}

void news_groupinfo(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupReadLockRec gr;
  NewsGroupPostLockRec gp;
  NewsGroupAdminLockRec gx;

  char *groupkeyptr;
  dbref userkey;

  int restrct = 1; /* restrict information shown to player */

  char *buff;

  int ga_count = 0;
  int gu_count = 0;

  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  groupkeyptr = groupfmt(player, buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }
 
  if( !user_passes_admin( player, groupkeyptr ) &&
      !Wizard(player) ) {
    restrct = 1;
  }
  else {
    restrct = 0;
  }

  notify(player, "-------------------------------------------------------------------------------");

  notify(player, unsafe_tprintf(  "Group:                %s", gi.name));
  notify(player, unsafe_tprintf(  "Desc:                 %s", gi.desc));

  if( gi.adder != -1 ) {
    buff = unparse_object(player, gi.adder, 1);
    notify(player, unsafe_tprintf("Created:              %-24s  Creator:  %s", 
                           ez_ctime(gi.create_time), buff));
    free_lbuf(buff);
  }
  else {
    notify(player, unsafe_tprintf("Created:              %-24s  Creator:  [System]", 
                           ez_ctime(gi.create_time)));
  }

  if ( gi.def_article_life == 0 ) {
    notify(player, "Default Article Life: [Infinite]");
  }
  else if( gi.def_article_life != -1 ) {
    notify(player, unsafe_tprintf("Default Article Life: %d days", 
           gi.def_article_life / 60 / 60 / 24 ));
  }
  else {
    news_assert(get_XX(&xx));
    notify(player, unsafe_tprintf("Default Article Life: %d days (Global Default)", xx.def_article_life / 60 / 60 / 24 ));
  }

  if( gi.last_activity_time ) {
    notify(player, unsafe_tprintf("Activity:             %s", 
                             ez_ctime(gi.last_activity_time)));
  }
  else {
    notify(player,         "Activity:             [Never]");
  }
    
  ga_count = articles_in_group(groupkeyptr);

  for( userkey = gi.first_user_dbref;
       userkey != -1;
       userkey = gu.next_user_dbref ) {
    news_assert(get_GU(&gu, groupkeyptr, userkey));
    gu_count++;
  }

  notify(player, unsafe_tprintf(  "Users:                %-24d", 
                           gu_count));

  notify(player, unsafe_tprintf(  "Max Active Articles:  %-24d  Articles: %d [%d Active]",
                           gi.posting_limit, gi.next_post_seq, ga_count));

  if( !restrct ) {
    if( get_GR(&gr, groupkeyptr) ) {
      notify(player, unsafe_tprintf("Read Lock:            %s", gr.text));
    }
    else {
      notify(player,         "Read Lock:            [None]");
    }
   
    if( get_GP(&gp, groupkeyptr) ) {
      notify(player, unsafe_tprintf("Post Lock:            %s", gp.text));
    }
    else {
      notify(player,         "Post Lock:            [None]");
    }

    if( get_GX(&gx, groupkeyptr) ) {
      notify(player, unsafe_tprintf(  "Admin Lock:           %s", gx.text));
    }
    else {
      notify(player,           "Admin Lock:           [None]");
    }
  }

  notify(player, "-------------------------------------------------------------------------------");
}

void news_grouplist(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGlobalRec xx;
  char* groupkeyptr, *tpr_buff, *tprp_buff; 

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  news_assert(get_XX(&xx));

  notify(player, "-------------------------------------------------------------------------------");
  notify(player, "Access  Group Name                      Group Description");
  notify(player, "-------------------------------------------------------------------------------");

  tprp_buff = tpr_buff = alloc_lbuf("news_grouplist");
  for( groupkeyptr = xx.first_group; 
       *groupkeyptr; 
       groupkeyptr = gi.next_group ) {
    news_assert(get_GI(&gi, groupkeyptr));
    tprp_buff = tpr_buff;
    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-6.6s  %-31.31s %-39.39s",
           (user_passes_post(player, gi.name) || 
           user_passes_read(player, gi.name)) ? "Yes" : "No",
           gi.name, gi.desc ));
  }
  free_lbuf(tpr_buff);

  notify(player, "-------------------------------------------------------------------------------");
}

void news_groupmem(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  char* groupkeyptr;
  dbref userkey;
  char* buff;
  char* uobuff;
  int count;

  /* check for player */
  if( !isPlayer(player) ) {
    notify(player, "News: Sorry, only players may use this command.");
    return;
  }


  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  groupkeyptr = groupfmt(player,buf1);

  if( !groupkeyptr ) {
    return;
  }
  
  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }

  if( !user_passes_admin( player, groupkeyptr ) &&
      !Wizard(player) &&
      !user_subscribed(player, groupkeyptr) ) {
    notify(player, "News: You are not subscribed to this group.");
    return;
  }

  notify(player, "-------------------------------------------------------------------------------");
  notify(player, unsafe_tprintf("Users of group '%s':", groupkeyptr));
  notify(player, "-------------------------------------------------------------------------------");

  buff = alloc_lbuf("news/groupmem");
  
  count = 0;
  *buff = '\0';
  for( userkey = gi.first_user_dbref;
       userkey != -1;
       userkey = gu.next_user_dbref ) {
    news_assert(get_GU(&gu, groupkeyptr, userkey));

    uobuff = unparse_object(player, userkey, 1);
    sprintf(buff + strlen(buff), "%-37.37s  ", uobuff);
    free_lbuf(uobuff);

    if( count++ >= 1 ) {
      notify(player, buff);
      count = 0;
      *buff = '\0';
    }
  }

  if( count ) {
    notify(player, buff);
  }

  free_lbuf(buff);

  notify(player, "-------------------------------------------------------------------------------");
}

void news_groupadmin(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupAdminLockRec gx;
  NewsGroupInfoRec gi;
  char *groupkeyptr; 
  char *bp;

  groupkeyptr = groupfmt(player,buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
  }

  if( !buf2 || !*buf2 ) {
    /* remove group admin */
    if( get_GX(&gx, groupkeyptr) ) {
      del_GX(&gx);
    }

    notify(player, unsafe_tprintf("News: Admin lock removed from group '%s'.", 
                           groupkeyptr)); 
  }
  else {
    notify(player, unsafe_tprintf("News: Admin lock group '%s' is now '%s'.",
                                  groupkeyptr, buf2));

    memset( &gx, 0, sizeof(gx) );
    strcpy( gx.group_name, groupkeyptr );
    bp = gx.text;
    safe_str(buf2,gx.text, &bp);

    put_GX( &gx );
  }
}

void news_grouplimit(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  char* groupkeyptr;
  int newlimit;

  groupkeyptr = groupfmt(player, buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }

  if( !is_integer(buf2) ) {
    notify(player, "News: Please specify an integer limit.");
    return;
  }

  newlimit = atoi(buf2);

  if( newlimit < 0 ) {
    notify(player, "News: Limits must be positive integers.");
    return;
  }

  gi.posting_limit = newlimit;

  put_GI(&gi);

  notify(player, unsafe_tprintf("News: Posting limit for group '%s' set to %d.",
                 groupkeyptr, newlimit));
}

void news_articlelife(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  int new_def_article_life;
  char* groupkeyptr;

  if( !buf2 || !*buf2 ) {
    /* Set global default article life */
    /* command format: news/articlelife <days> */
 
    new_def_article_life = atoi( buf1 );

    if( strcasecmp(buf1, "forever") == 0 ) {
       news_assert( get_XX(&xx) );
       notify(player, "News: Global default article life is now infinite days.");
       xx.def_article_life = 0;
    } else {
       if( new_def_article_life < 1 ) {
         notify(player, "News: Please enter a positive integer number of days." );
         return;
       }

       /* This will give a warning on Solaris, because their header files are screwed */
       if ( (int)(new_def_article_life) > (MYMAXINT / 60 / 60 / 24) ) {
         notify(player, "News: Number of days is too great, be reasonable." );
         return;
       }

       notify(player, unsafe_tprintf("News: Global default article life is now %d days.", 
              new_def_article_life));

       news_assert( get_XX(&xx) );

       xx.def_article_life = new_def_article_life * 60 * 60 * 24;

    }
    put_XX(&xx);
  }
  else {
    /* Set group default article life */
    /* command format: news/articlelife <group>=<days> */
    groupkeyptr = groupfmt(player, buf1);

    if( !groupkeyptr ) {
      return;
    }

    if( !get_GI(&gi, groupkeyptr) ) {
      notify(player, "News: Unknown group specified.");
      return;
    }

    new_def_article_life = atoi( buf2 );

    if( strcasecmp(buf2, "forever") == 0 ) {
       gi.def_article_life = 0;
       notify(player, unsafe_tprintf("News: Default article life for group '%s' is now infinite days.",gi.name));
    } else {
       if( new_def_article_life == 0 ||
           new_def_article_life < -1 ) {
         notify(player, "News: Please enter a positive integer number of days (-1 to use global value)." );
         return;
       }
   
       /* This will give a warning on Solaris, because their header files are screwed */
       if( (int)(new_def_article_life) > (MYMAXINT / 60 / 60 / 24) ) {
         notify(player, "News: Number of days is too great, be reasonable." );
         return;
       }
   
       if( new_def_article_life == -1 ) {
         notify(player, unsafe_tprintf("News: Default article life for group '%s' now follows the global setting.", gi.name) );
/*        gi.def_article_life = DEFAULT_EXPIRE_OFFSET; */
          news_assert( get_XX(&xx) );
          gi.def_article_life = xx.def_article_life;
       }
       else {
         notify(player, unsafe_tprintf("News: Default article life for group '%s' is now %d days.", 
                gi.name,
                new_def_article_life));
          gi.def_article_life = new_def_article_life * 60 * 60 * 24;
       }
   
    }    
    put_GI( &gi );
  }
}

void news_userlimit(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  int newlimit;

  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !is_integer(buf1) ) {
    notify(player, "News: Please specify an integer limit.");
    return;
  }

  newlimit = atoi(buf1);

  if( newlimit < 0 ) {
    notify(player, "News: Limits must be positive integers.");
    return;
  }

  news_assert(get_XX(&xx));

  xx.user_limit = newlimit;

  put_XX(&xx);

  notify(player, unsafe_tprintf("News: User posting limit set to %d.", newlimit));
}

void news_usermem(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  char* groupkeyptr;

  dbref usertarget;

  char* uobuff, *tpr_buff, *tprp_buff;

  usertarget = lookup_player(player, buf1, 1);

  if( usertarget == NOTHING ) {
    notify(player, "News: Unknown player specified.");
    return;
  }

  uobuff = unparse_object(player, usertarget, 1);

  if( !get_UI(&ui, usertarget) ) {
    notify(player, unsafe_tprintf("News: %s has never used the news system.", uobuff));
  }
  else {
    notify(player, "-------------------------------------------------------------------------------");
    notify(player, unsafe_tprintf("Group membership for %s:", uobuff));
    notify(player, "-------------------------------------------------------------------------------");
    tprp_buff = tpr_buff = alloc_lbuf("news_usermem");
    for( groupkeyptr = ui.first_group;
         *groupkeyptr;
         groupkeyptr = ug.next_group ) {
      news_assert(get_UG(&ug, usertarget, groupkeyptr));

      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-31s  %d", ug.group_name, 
                             ug.seq_marker));
    }
    free_lbuf(tpr_buff);
    notify(player, "-------------------------------------------------------------------------------");
  }
  free_lbuf(uobuff);
}
  
  
void news_userinfo(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;

  dbref usertarget;

  char* groupkeyptr;
  char* uobuff;
  int ua_count = 0; 
  int ug_count = 0;

  if( *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  if( !*buf1 ) {
    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Sorry, only players may use this command.");
      return;
    }

    usertarget = player;
    if( !get_UI(&ui, usertarget) ) {
      notify(player, "News: You have never used the news system.");
      return;
    }
  }
  else {
    if( !Wizard(player) ) {
      notify(player, "News: You cannot obtain information on other users.");
      return;
    }

    usertarget = lookup_player(player, buf1, 1);

    if( usertarget == NOTHING ) {
      notify(player, "News: Unknown player specified.");
      return;
    }

    /* check for player */
    if( !isPlayer(player) ) {
      notify(player, "News: Please specify a player.");
      return;
    }

    if( !get_UI(&ui, usertarget) ) {
      uobuff = unparse_object(player, usertarget, 1);
      notify(player, unsafe_tprintf(
             "News: %s has never used the news system.", uobuff));
      free_lbuf(uobuff);
      return;
    }
  }

  uobuff = unparse_object(player, usertarget, 1);

  for( groupkeyptr = ui.first_group;
       *groupkeyptr;
       groupkeyptr = ug.next_group ) {
    news_assert(get_UG(&ug, usertarget, groupkeyptr));
    ug_count++;
  }
  ua_count = articles_by_user(player);
  notify(player, "-------------------------------------------------------------------------------");
  notify(player, unsafe_tprintf("User:                %s", uobuff));
  notify(player, unsafe_tprintf("Group Subscriptions: %d", ug_count));
  notify(player, unsafe_tprintf("Postings:            %d [%d Active]",  
                         ui.total_posts,
                         ua_count));
  notify(player, "-------------------------------------------------------------------------------");
 
  free_lbuf(uobuff);
}

void news_lock(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGroupInfoRec gi;
  NewsGroupReadLockRec gr;
  NewsGroupPostLockRec gp;

  char* groupkeyptr;
  char* bp;

  groupkeyptr = groupfmt(player, buf1);

  if( !groupkeyptr ) {
    return;
  }

  if( !get_GI(&gi, groupkeyptr) ) {
    notify(player, "News: Unknown group specified.");
    return;
  }

  if( !user_passes_admin( player, groupkeyptr ) &&
      !Wizard(player) ) {
    notify(player, "News: Permission denied.");
    return;
  }

  if( key == NEWS_READLOCK ) {
    if( strcmp(groupkeyptr, "info") == 0 ||   
        strcmp(groupkeyptr, "general") == 0 ) {
      notify(player, unsafe_tprintf("News: The group '%s' is a system group, and cannot be readlocked.", groupkeyptr));
      return;
    }
    if( !*buf2 ) {
      if( get_GR(&gr, groupkeyptr) ) {
        del_GR(&gr);
      }
      notify(player, unsafe_tprintf("News: Read lock removed for group '%s'.",
                             groupkeyptr));
    }
    else {
      memset(&gr, 0, sizeof(gr));
      strcpy(gr.group_name, groupkeyptr);
      bp = gr.text;
      safe_str(buf2,gr.text, &bp);
      put_GR(&gr);
      notify(player, unsafe_tprintf("News: Read lock set for group '%s'.",
                             groupkeyptr));
    }
  }
  else if( key == NEWS_POSTLOCK ) {
    if( !*buf2 ) {
      if( get_GP(&gp, groupkeyptr) ) {
        del_GP(&gp);
      }
      notify(player, unsafe_tprintf("News: Post lock removed for group '%s'.",
                             groupkeyptr));
    }
    else {
      memset(&gp, 0, sizeof(gp));
      strcpy(gp.group_name, groupkeyptr);
      bp = gp.text;
      safe_str(buf2,gp.text, &bp);
      put_GP(&gp);
      notify(player, unsafe_tprintf("News: Post lock set for group '%s'.",
                             groupkeyptr));
    }
  }
}

void news_expire(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;

  char* groupkeyptr;
  int seqkey;

  time_t now;

  int ga_count = 0;
  
  time(&now);

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  notify(player, "News: Performing expiration...");

  STARTLOG(LOG_ALWAYS, "EXP", "NEWS")
    log_text((char *) "Expiration started.");
  ENDLOG

  news_assert(get_XX(&xx));

  for( groupkeyptr = xx.first_group;
       *groupkeyptr;
       groupkeyptr = gi.next_group ) {
    news_assert(get_GI(&gi, groupkeyptr));

    for( seqkey = gi.first_seq; 
         seqkey != -1;
         seqkey = ga.next_seq ) {
      news_assert(get_GA(&ga, gi.name, seqkey));
      
      if( ga.expire_time != 0 &&
          ga.expire_time <= now ) {
        delete_article_from_user(ga.owner_dbref, gi.name, seqkey);
        delete_article_from_group(gi.name, seqkey);
        ga_count++;
      }
    }
  }

  xx.last_expire_time = now;

  put_XX(&xx);

  STARTLOG(LOG_ALWAYS, "EXP", "NEWS")
    log_text((char *) "Expiration done. (");
    log_number(ga_count);
    log_text((char *) " article(s) deleted)");
  ENDLOG

  notify(player, unsafe_tprintf("News: Done. (%d article(s) deleted)",
                         ga_count));
}

void newsdb_unload(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  char flatfile_name[129 + 32 + 5 + 5];
  char oldflatfile_name[129 + 32 + 5 + 5 + 4];
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupArticleInfoRec ga;
  static NewsGroupArticleTextRec gt;
  static NewsGroupReadLockRec gr;
  static NewsGroupPostLockRec gp;
  static NewsGroupAdminLockRec gx;
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserGroupRec ug;

  char *groupkeyptr;
  dbref userkey;
  int seqkey;

  FILE* flatfile;

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  STARTLOG(LOG_ALWAYS, "DMP", "NEWS")
    log_text((char *) "Unloading news database...");
  ENDLOG

  notify(player, "News: Performing pre-unload DB integrity check...");
  if( !news_dbck_internal() ) {
    notify(player, "News: Failed. News system deactivated.");
    return;
  }
  else {
    notify(player, "News: Passed.");
  }

  notify(player,"News: Unloading...");

  sprintf(flatfile_name, 
	  "%s/%s.news.flat", mudconf.data_dir, mudconf.muddb_name);

  sprintf(oldflatfile_name, "%s.old", flatfile_name);

  /* save the old flatfile if there is one */
  rename(flatfile_name, oldflatfile_name);

  /* open new flat file */
  flatfile = fopen(flatfile_name, "w");

  if( !flatfile ) {
    notify(player, "News: Unable to create flat file.");
    return;
  }

  /* write out version number */
  fprintf(flatfile, "VV\n%d\n", LATEST_NEWS_DB_VERSION);

  news_assert(get_XX(&xx));

  fprintf(flatfile, "XX\n");
  fprintf(flatfile, "%ld\n", xx.db_creation_time);
  fprintf(flatfile, "%ld\n", xx.last_expire_time);
  fprintf(flatfile, "%d\n", xx.def_article_life);
  fprintf(flatfile, "%d\n", xx.user_limit);
  putstring(flatfile, xx.first_group);
  fprintf(flatfile, "%d\n", xx.first_user_dbref);

  for( groupkeyptr = xx.first_group; 
       *groupkeyptr; 
       groupkeyptr = gi.next_group ) {
    news_assert(get_GI(&gi, groupkeyptr));
    
    fprintf(flatfile, "GI\n");
    putstring(flatfile, gi.name);
    putstring(flatfile, gi.desc);
    fprintf(flatfile, "%d\n", gi.adder);
    fprintf(flatfile, "%d\n", gi.posting_limit);
    fprintf(flatfile, "%d\n", gi.def_article_life);
    fprintf(flatfile, "%ld\n", gi.create_time);
    fprintf(flatfile, "%ld\n", gi.last_activity_time);
    fprintf(flatfile, "%d\n", gi.next_post_seq);
    fprintf(flatfile, "%d\n", gi.first_seq);
    fprintf(flatfile, "%d\n", gi.first_user_dbref);
    putstring(flatfile, gi.next_group);
 
    if( get_GP(&gp, gi.name) ) {
      fprintf(flatfile, "GP\n");
      putstring(flatfile, gp.group_name);
      putstring(flatfile, gp.text);
    }

    if( get_GR(&gr, gi.name) ) {
      fprintf(flatfile, "GR\n");
      putstring(flatfile, gr.group_name);
      putstring(flatfile, gr.text);
    }

    if( get_GX(&gx, gi.name) ) {
      fprintf(flatfile, "GX\n");
      putstring(flatfile, gx.group_name);
      putstring(flatfile, gx.text);
    }

    for( userkey = gi.first_user_dbref;
         userkey != -1;
         userkey = gu.next_user_dbref ) {
      news_assert(get_GU(&gu, gi.name, userkey));
      fprintf(flatfile, "GU\n");
      putstring(flatfile, gu.group_name);
      fprintf(flatfile, "%d\n", gu.user_dbref);
      fprintf(flatfile, "%d\n", gu.next_user_dbref);
    }

    for( seqkey = gi.first_seq;
         seqkey != -1;
         seqkey = ga.next_seq ) {
      news_assert(get_GA(&ga, gi.name, seqkey));
      fprintf(flatfile, "GA\n");
      putstring(flatfile, ga.group_name);
      fprintf(flatfile, "%d\n", ga.seq);
      putstring(flatfile, ga.title);
      putstring(flatfile, ga.owner_name);
      fprintf(flatfile, "%d\n", ga.owner_dbref);
      fprintf(flatfile, "%c\n", ga.owner_nuked_flag);
      fprintf(flatfile, "%ld\n", ga.post_time);
      fprintf(flatfile, "%ld\n", ga.expire_time);
      fprintf(flatfile, "%d\n", ga.next_seq);

      if( get_GT(&gt, gi.name, seqkey) ) {
        fprintf(flatfile, "GT\n");
        putstring(flatfile, gt.group_name);
        fprintf(flatfile, "%d\n", gt.seq);
        putstring(flatfile, gt.text);
      }
    }
  }

  for( userkey = xx.first_user_dbref;
       userkey != -1;
       userkey = ui.next_user_dbref ) {
    news_assert(get_UI(&ui, userkey));

    fprintf(flatfile, "UI\n");
    fprintf(flatfile, "%d\n", ui.user_dbref);
    fprintf(flatfile, "%d\n", ui.total_posts);
    putstring(flatfile, ui.first_group);
    putstring(flatfile, ui.last_read_group);
    putstring(flatfile, ui.first_art_group);
    fprintf(flatfile, "%d\n", ui.first_art_seq);
    fprintf(flatfile, "%d\n", ui.next_user_dbref);

    for( groupkeyptr = ui.first_group; 
         *groupkeyptr;
         groupkeyptr = ug.next_group ) {
      news_assert(get_UG(&ug, userkey, groupkeyptr));
      fprintf(flatfile, "UG\n");
      fprintf(flatfile, "%d\n", ug.user_dbref);
      putstring(flatfile, ug.group_name);
      fprintf(flatfile, "%d\n", ug.seq_marker);
      putstring(flatfile, ug.next_group);
    }

    for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
         *groupkeyptr;
         groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
      news_assert(get_UA(&ua, userkey, groupkeyptr, seqkey));
      fprintf(flatfile, "UA\n");
      fprintf(flatfile, "%d\n", ua.user_dbref);
      putstring(flatfile, ua.group_name);
      fprintf(flatfile, "%d\n", ua.seq);
      putstring(flatfile, ua.next_art_group);
      fprintf(flatfile, "%d\n", ua.next_art_seq);
      
    }
  }
  fprintf(flatfile, "**\n");
  fclose(flatfile);

  STARTLOG(LOG_ALWAYS, "DMP", "NEWS")
    log_text((char *) "Done.");
  ENDLOG

  notify(player, "News: Done.");
  time(&mudstate.newsflat_time);
}

void newsdb_load(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  char flatfile_name[129 + 32 + 5 + 5];
  char news_db_name[129 + 32 + 5];
  char news_db_dir_name[129 + 32 + 5 + 4];
  char news_db_pag_name[129 + 32 + 5 + 4];
  char tag[1024];
  int line = 0;
  NewsVersionRec vv;
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupArticleInfoRec ga;
  static NewsGroupArticleTextRec gt;
  static NewsGroupReadLockRec gr;
  static NewsGroupPostLockRec gp;
  static NewsGroupAdminLockRec gx;
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserGroupRec ug;

  FILE* flatfile;

  int version = 1;
  int done = 0;

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  notify(player,"News: Loading...");

  sprintf(flatfile_name, 
	  "%s/%s.news.flat", mudconf.data_dir, mudconf.muddb_name);

  flatfile = fopen(flatfile_name, "r");

  if( !flatfile ) {
    notify(player, "News: Unable to open flat file.");
    return;
  }

  sprintf(news_db_name, "%s/%s.news", mudconf.data_dir, mudconf.muddb_name);
  sprintf(news_db_dir_name, "%s.dir", news_db_name);
  sprintf(news_db_pag_name, "%s.pag", news_db_name);

  if( ndb ) {
    dbm_close(ndb);
  }

  unlink(news_db_dir_name);
  unlink(news_db_pag_name);

  /* re-enable the system */
  news_internal_failure = 0;

  ndb = dbm_open(news_db_name, O_RDWR | O_CREAT, 00664);

  if( !ndb ) {
    news_system_active = 0;
    news_internal_failure = 1;
    STARTLOG(LOG_STARTUP, "WIZ", "ERROR")
      log_text((char *) "Unable to open news database for load.");
    ENDLOG
    notify(player, "News: Database creation error. System deactivated.");
    fclose(flatfile);
    return;
  }

  while( !done && !feof(flatfile) ) {
    fgets(tag, 1024, flatfile);
    if( *tag ) {
      tag[strlen(tag) - 1] = '\0';
    }
    line++;

    if( !strcmp(tag, "**") ) {
      done = 1;
    }
    else if( !strcmp(tag, "VV") ) {
      version = getref(flatfile);
      line++;

      if( version > LATEST_NEWS_DB_VERSION || version < 1 ) {
        news_system_active = 0;
        news_internal_failure = 1;
        STARTLOG(LOG_STARTUP, "WIZ", "ERROR")
          log_text((char *) "Unknown news flatfile database version!");
        ENDLOG
        fclose(flatfile);
        return;
      }
      else if( version != LATEST_NEWS_DB_VERSION ) {
        STARTLOG(LOG_STARTUP, "WIZ", "WARN")
          log_text((char *) "News flatfile is an old version, transcoding...");
        ENDLOG
      }

      /* if version is not latest, we'll convert as we go */
      memset(&vv, 0, sizeof(vv));
      vv.version = LATEST_NEWS_DB_VERSION;
      put_VV(&vv);
    }
    else if( !strcmp(tag, "XX") ) {
      memset(&xx, 0, sizeof(xx));
      xx.db_creation_time = getref(flatfile);
      line++;
      xx.last_expire_time = getref(flatfile);
      line++;
      if( version > 1 ) {
        xx.def_article_life = getref(flatfile);
        line++;
      }
      else {
        xx.def_article_life = DEFAULT_EXPIRE_OFFSET;
      }
      xx.user_limit = getref(flatfile);
      line++;
      strcpy(xx.first_group, getstring_noalloc(flatfile));
      line++;
      xx.first_user_dbref = getref(flatfile);
      line++;
      put_XX(&xx);
    }
    else if( !strcmp(tag, "GI") ) {
      memset(&gi, 0, sizeof(gi));
      strcpy(gi.name, getstring_noalloc(flatfile));
      line++;
      strcpy(gi.desc, getstring_noalloc(flatfile));
      line++;
      gi.adder = getref(flatfile);
      line++;
      if( version < 2 ) {
        int admin;
        admin = getref(flatfile);
        line++;

        if( admin != -1 ) {
          /* convert admin field to admin lock record */
          memset(&gx, 0, sizeof(gx));
          strcpy(gx.group_name, gi.name);
          sprintf(gx.text, "match(#%d,%%#)", admin);
          put_GX(&gx);
        }
      }
      gi.posting_limit = getref(flatfile);
      line++;
      if( version > 1 ) {
        gi.def_article_life = getref(flatfile);
        line++;
      }
      else {
        gi.def_article_life = -1;
      }
      gi.create_time = getref(flatfile);
      line++;
      gi.last_activity_time = getref(flatfile);
      line++;
      gi.next_post_seq = getref(flatfile);
      line++;
      gi.first_seq = getref(flatfile);
      line++;
      gi.first_user_dbref = getref(flatfile);
      line++;
      strcpy(gi.next_group, getstring_noalloc(flatfile));
      line++;
      put_GI(&gi);
    }
    else if( !strcmp(tag, "GP") ) {
      memset(&gp, 0, sizeof(gp));
      strcpy(gp.group_name, getstring_noalloc(flatfile));
      line++;
      strcpy(gp.text, getstring_noalloc(flatfile));
      line++;
      put_GP(&gp);
    }
    else if( !strcmp(tag, "GR") ) {
      memset(&gr, 0, sizeof(gr));
      strcpy(gr.group_name, getstring_noalloc(flatfile));
      line++;
      strcpy(gr.text, getstring_noalloc(flatfile));
      line++;
      put_GR(&gr);
    }
    else if( !strcmp(tag, "GX") ) {
      memset(&gx, 0, sizeof(gx));
      strcpy(gx.group_name, getstring_noalloc(flatfile));
      line++;
      strcpy(gx.text, getstring_noalloc(flatfile));
      line++;
      put_GX(&gx);
    }
    else if( !strcmp(tag, "GU") ) {
      memset(&gu, 0, sizeof(gu));
      strcpy(gu.group_name, getstring_noalloc(flatfile));
      line++;
      gu.user_dbref = getref(flatfile);
      line++;
      gu.next_user_dbref = getref(flatfile);
      line++;
      put_GU(&gu);
    }
    else if( !strcmp(tag, "GA") ) {
      memset(&ga, 0, sizeof(ga));
      strcpy(ga.group_name, getstring_noalloc(flatfile));
      line++;
      ga.seq = getref(flatfile);
      line++;
      strcpy(ga.title, getstring_noalloc(flatfile));
      line++;
      strcpy(ga.owner_name, getstring_noalloc(flatfile));
      line++;
      ga.owner_dbref = getref(flatfile);
      line++;
      ga.owner_nuked_flag = fgetc(flatfile);
      fgetc(flatfile);
      line++;
      ga.post_time = getref(flatfile);
      line++;
      ga.expire_time = getref(flatfile);
      line++;
      ga.next_seq = getref(flatfile);
      line++;
      put_GA(&ga);
    }
    else if( !strcmp(tag, "GT") ) {
      memset(&gt, 0, sizeof(gt));
      strcpy(gt.group_name, getstring_noalloc(flatfile));
      line++;
      gt.seq = getref(flatfile);
      line++;
      strcpy(gt.text, getstring_noalloc(flatfile));
      line++;
      put_GT(&gt);
    }
    else if( !strcmp(tag, "UI") ) {
      memset(&ui, 0, sizeof(ui));
      ui.user_dbref = getref(flatfile);
      line++;
      ui.total_posts = getref(flatfile);
      line++;
      strcpy(ui.first_group, getstring_noalloc(flatfile));
      line++;
      strcpy(ui.last_read_group, getstring_noalloc(flatfile));
      line++;
      strcpy(ui.first_art_group, getstring_noalloc(flatfile));
      line++;
      ui.first_art_seq = getref(flatfile);
      line++;
      ui.next_user_dbref = getref(flatfile);
      line++;
      put_UI(&ui);
    }
    else if( !strcmp(tag, "UG") ) {
      memset(&ug, 0, sizeof(ug));
      ug.user_dbref = getref(flatfile);
      line++;
      strcpy(ug.group_name, getstring_noalloc(flatfile));
      line++;
      ug.seq_marker = getref(flatfile);
      line++;
      strcpy(ug.next_group, getstring_noalloc(flatfile));
      line++;
      put_UG(&ug);
    }
    else if( !strcmp(tag, "UA") ) {
      memset(&ua, 0, sizeof(ua));
      ua.user_dbref = getref(flatfile);
      line++;
      strcpy(ua.group_name, getstring_noalloc(flatfile));
      line++;
      ua.seq = getref(flatfile);
      line++;
      strcpy(ua.next_art_group, getstring_noalloc(flatfile));
      line++;
      ua.next_art_seq = getref(flatfile);
      line++;
      put_UA(&ua);
    }
    else {
      news_system_active = 0;
      news_internal_failure = 1;
      STARTLOG(LOG_STARTUP, "WIZ", "ERROR")
        log_text((char *) "News: Corrupted flat file during load. Line = ");
        log_number( line );
        log_text((char *) ", Offset = ");
        log_number( ftell(flatfile) );
        log_text((char *) ", Bad Tag = '");
        log_text((char *) tag);
        log_text((char *) "'");
      ENDLOG
      notify(player, "News: Corrupt flat file. System deactivated.");
      fclose(flatfile);
      return;
    }
  }
  fclose(flatfile);

  if( !done ) {
    news_system_active = 0;
    news_internal_failure = 1;
    STARTLOG(LOG_STARTUP, "WIZ", "ERROR")
      log_text((char *) "News: Corrupted flat file during load. EOF before End marker.");
    ENDLOG
    notify(player, "News: Corrupt flat file. System deactivated.");
    return;
  }

  notify(player, "News: Load Complete.");

  notify(player, "News: Performing post-load DB integrity check...");
  if( !news_dbck_internal() ) {
    notify(player, "News: Failed. News system deactivated.");
  }
  else {
    notify(player, "News: Passed.");
  }
}

void newsdb_dbck(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  notify(player, "News: Performing DB integrity check...");
  
  if( news_dbck_internal() ) {
    notify(player, "News: Passed.");
  }
  else {
    notify(player, "News: Failed. News system deactivated.");
  }
}

void newsdb_dbinfo(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  char news_db_name[129 + 32 + 5];
  char news_db_dir_name[129 + 32 + 5 + 4 + 1];
  char news_db_pag_name[129 + 32 + 5 + 4 + 1];
  struct stat filestat;
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupArticleInfoRec ga;
  static NewsGroupArticleTextRec gt;
  static NewsGroupReadLockRec gr;
  static NewsGroupPostLockRec gp;
  static NewsGroupAdminLockRec gx;
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserGroupRec ug;

  char *groupkeyptr;
  dbref userkey;
  int seqkey;

  int gi_count = 0;
  int gu_count = 0;
  int ga_count = 0;
  int gt_count = 0;
  int gr_count = 0;
  int gp_count = 0;
  int gx_count = 0;
  int ui_count = 0;
  int ua_count = 0;
  int ug_count = 0;

  if( *buf1 ||
      *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  notify(player,"-------------------------------------------------------------------------------");

  news_assert(get_XX(&xx));

  notify(player,unsafe_tprintf( "Database Created:                        %s", 
         ez_ctime(xx.db_creation_time)));

  if( xx.last_expire_time ) {
    notify(player,unsafe_tprintf("Last Expiration Run:                     %s", 
           ez_ctime(xx.last_expire_time)));
  }
  else {
    notify(player,       "Last Expiration Run:                     [Never]");
  }

  if ( xx.def_article_life == 0 )
     notify(player, "Default Article Life:                    [Infinite]");
  else {
     notify(player, unsafe_tprintf("Default Article Life:                    %d days",
            xx.def_article_life / 60 / 60 / 24) );
  }

  notify(player,unsafe_tprintf( "User Posting Limit:                      %d", 
                        xx.user_limit));

  for( groupkeyptr = xx.first_group; 
       *groupkeyptr; 
       groupkeyptr = gi.next_group ) {
    gi_count++;
    news_assert(get_GI(&gi, groupkeyptr));
 
    if( get_GP(&gp, gi.name) ) {
      gp_count++;
    }

    if( get_GR(&gr, gi.name) ) {
      gr_count++;
    }

    if( get_GX(&gx, gi.name) ) {
      gx_count++;
    }

    for( userkey = gi.first_user_dbref;
         userkey != -1;
         userkey = gu.next_user_dbref ) {
      gu_count++;
      news_assert(get_GU(&gu, gi.name, userkey));
    }

    for( seqkey = gi.first_seq;
         seqkey != -1;
         seqkey = ga.next_seq ) {
      ga_count++;
      news_assert(get_GA(&ga, gi.name, seqkey));

      if( get_GT(&gt, gi.name, seqkey) ) {
        gt_count++;
      }
    }
  }

  notify(player,unsafe_tprintf("Group Info Segs:                         %d", 
                        gi_count));
  notify(player,unsafe_tprintf("Group Read Lock Segs:                    %d", 
                        gr_count));
  notify(player,unsafe_tprintf("Group Post Lock Segs:                    %d", 
                        gp_count));
  notify(player,unsafe_tprintf("Group Admin Lock Segs:                   %d", 
                        gx_count));
  notify(player,unsafe_tprintf("Group User Segs:                         %d", 
                        gu_count));
  notify(player,unsafe_tprintf("Group Article Info Segs:                 %d", 
                        ga_count));
  notify(player,unsafe_tprintf("Group Article Text Segs:                 %d", 
                        gt_count));
 
  for( userkey = xx.first_user_dbref;
       userkey != -1;
       userkey = ui.next_user_dbref ) {
    news_assert(get_UI(&ui, userkey));
    ui_count++;

    for( groupkeyptr = ui.first_group; 
         *groupkeyptr;
         groupkeyptr = ug.next_group ) {
      news_assert(get_UG(&ug, userkey, groupkeyptr));
      ug_count++;
    }

    for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
         *groupkeyptr;
         groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
      news_assert(get_UA(&ua, userkey, groupkeyptr, seqkey));
      ua_count++;
    }
  }
  notify(player,unsafe_tprintf("User Info Segs:                          %d", 
                        ui_count));
  notify(player,unsafe_tprintf("User Group Segs:                         %d", 
                        ug_count));
  notify(player,unsafe_tprintf("User Article Segs:                       %d", 
                        ua_count));

  sprintf(news_db_name, "%s/%s.news", mudconf.data_dir, mudconf.muddb_name);
  sprintf(news_db_dir_name, "%s.dir", news_db_name);
  sprintf(news_db_pag_name, "%s.pag", news_db_name);

  if( stat(news_db_dir_name, &filestat) != -1 ) {
    strcat(news_db_dir_name, ":");
    notify(player,unsafe_tprintf("%-40.40s %d blocks (%d bytes)", 
                          news_db_dir_name, filestat.st_blocks, 
                          filestat.st_blocks * 512));
  }

  if( stat(news_db_pag_name, &filestat) != -1 ) {
    strcat(news_db_pag_name, ":");
    notify(player,unsafe_tprintf("%-40.40s %d blocks (%d bytes)", 
                          news_db_pag_name, filestat.st_blocks, 
                          filestat.st_blocks * 512));
  }

  notify(player,"-------------------------------------------------------------------------------");
}

void news_off(dbref player, dbref cause, int key, char *buf1, char *buf2)
{
  char *p;
  char flatfile_name[129 + 32 + 5 + 5], newname_name[129 + 32 + 5 + 5];
  int retval;

  for (p = buf1; *p; p++)
    *p = ToLower((int)*p);
 
  if ( *buf1 && strcmp(buf1, "initialize") == 0 ) {
    news_system_active = 0;
    notify(player, "News: System Deactivated.");
    memset(flatfile_name, 0, sizeof(flatfile_name));
    sprintf(flatfile_name, "%s/%s.news.dir", mudconf.data_dir, mudconf.muddb_name);
    remove(flatfile_name);
    memset(flatfile_name, 0, sizeof(flatfile_name));
    sprintf(flatfile_name, "%s/%s.news.pag", mudconf.data_dir, mudconf.muddb_name);
    remove(flatfile_name);
    notify(player, "News: News database purged.");
    memset(flatfile_name, 0, sizeof(flatfile_name));
    memset(newname_name, 0, sizeof(newname_name));

    sprintf(flatfile_name, "%s/%s.news.flat", mudconf.data_dir, mudconf.muddb_name);
    sprintf(newname_name, "%s/%s.news.del", mudconf.data_dir, mudconf.muddb_name);

    retval = rename(flatfile_name, newname_name);
    if ( retval == 0 )
       notify(player, "News: Existing flatfile archived.");
    notify(player, "News: System has been re-initialized.  You must @reboot to take effect.");
    return;
  } else if( *buf1 || *buf2 ) {
    notify(player, "News: Invalid command format.");
    return;
  }

  news_system_active = 0;
  notify(player, "News: System Deactivated.");
}


/* ------------------- support functions ------------------------- */

int news_convert_db( int src_ver )
{
  NewsVersionRec vv;
  NewsGlobalRec_v1 xx_v1;
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupInfoRec_v1 gi_v1;
  char* groupkeyptr = NULL;

  if( src_ver <= 1 ) {
    /* Add VV segment */
 
    memset( &vv, 0, sizeof(vv) );
    vv.version = LATEST_NEWS_DB_VERSION;

    put_VV( &vv );

    /* Convert XX segment */
    get_XX_v1( &xx_v1 );
    
    memset( &xx, 0, sizeof(xx) );
    xx.db_creation_time =   xx_v1.db_creation_time;
    xx.last_expire_time =   xx_v1.last_expire_time;
    xx.def_article_life =   DEFAULT_EXPIRE_OFFSET;   /* new in v2 */
    xx.user_limit =         xx_v1.user_limit;
    strcpy( xx.first_group, xx_v1.first_group );
    xx.first_user_dbref =   xx_v1.first_user_dbref;
    
    put_XX( &xx );

    /* Convert GI segments */

    for( groupkeyptr = xx.first_group;
         *groupkeyptr;
         groupkeyptr = gi.next_group ) {
      news_assert_retnonevoid( get_GI_v1( &gi_v1, groupkeyptr ), 0);

      memset( &gi, 0, sizeof( gi ) );
      strcpy( gi.name,        gi_v1.name );
      strcpy( gi.desc,        gi_v1.desc );
      gi.adder =              gi_v1.adder;
      
      if( gi_v1.admin != -1 ) {
        /* convert admin field into admin lock seg */
        NewsGroupAdminLockRec gx;

        memset( &gx, 0, sizeof( gx ) );
        strcpy( gx.group_name, gi_v1.name );
        sprintf(gx.text, "match(#%d,%%#)", gi_v1.admin);

        put_GX( &gx );
      }

      gi.posting_limit =      gi_v1.posting_limit;
      gi.def_article_life =   -1;                 /* new in v2 */
      gi.create_time =        gi_v1.create_time;
      gi.last_activity_time = gi_v1.last_activity_time;
      gi.next_post_seq =      gi_v1.next_post_seq;
      gi.first_seq =          gi_v1.first_seq;
      gi.first_user_dbref =   gi_v1.first_user_dbref;
      strcpy( gi.next_group,  gi_v1.next_group );

      put_GI( &gi );
    }
  }

  return 1;
}


void news_halt(char* fmt, ...)
{
  va_list va;
  char* buff;

  news_internal_failure = 1;
  news_system_active = 0;

  va_start(va, fmt);

  buff = alloc_lbuf("news_halt");

  vsprintf( buff, fmt, va );
  va_end(va);

  STARTLOG(LOG_PROBLEMS, "NEWS", "HALT")
    log_text(buff);
    log_text((char *) " News system halted.");
  ENDLOG
  
  free_lbuf(buff);
}

char* ez_ctime( time_t timemark )
{
  static char timebuff[26];

  strcpy(timebuff,ctime(&timemark));
  timebuff[strlen(timebuff) - 1] = '\0';

  return timebuff;
}

char* groupfmt( dbref player, char *group_name )
{
  static char cleaned[GROUP_NAME_LEN];
  char *tpr_buff, *tprp_buff;
  int idx;

  if( !*group_name ) {
    notify(player, "News: You must specify a group name.");
    return NULL;
  }

  if( !isalpha((int)*group_name) ) {
    notify(player, "News: Group names must start with an alpha character.");
    return NULL;
  }

  if( strlen(group_name) > GROUP_NAME_LEN - 1 ) {
    notify(player, unsafe_tprintf("News: Group names cannot exceed %d characters.",
                           GROUP_NAME_LEN - 1));
    return NULL;
  }

  tprp_buff = tpr_buff = alloc_lbuf("groupfmt");
  for( idx = 0; idx < GROUP_NAME_LEN - 1 && group_name[idx]; idx++ ) {
    if( !isalpha((int)group_name[idx]) &&
        !isdigit((int)group_name[idx]) &&
        group_name[idx] != '_' &&
        group_name[idx] != '.' ) {
      tprp_buff = tpr_buff;
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "News: Illegal character in group name [%c].",
                             group_name[idx]));
      free_lbuf(tpr_buff);
      return NULL;
    }
    cleaned[idx] = tolower(group_name[idx]);
  }
  free_lbuf(tpr_buff);
  cleaned[idx] = '\0';

  return cleaned;
}

int articles_in_group(char* group_name)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  int seqkey;
  int count = 0;

  if( !get_GI(&gi, group_name) ) {
    return 0;
  }

  for( seqkey = gi.first_seq; 
       seqkey != -1;
       seqkey = ga.next_seq ) {
    news_assert_retnonevoid(get_GA(&ga, group_name, seqkey), 0);
    count++;
  }
  return count;
}
  
int articles_by_user(dbref user)
{
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  char* groupkeyptr;
  int seqkey;
  int count = 0;

  if( !get_UI(&ui, user) ) {
    return 0;
  }

  for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
       *groupkeyptr;
       groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
    news_assert_retnonevoid(get_UA(&ua, user, groupkeyptr, seqkey), 0);
    count++;
  }
  return count;
}

int create_user_if_new(dbref player, dbref user)
{
  NewsUserInfoRec newui;
  NewsUserInfoRec ui;
  NewsGlobalRec xx;
  dbref userkey;

  if( get_UI(&ui, user) ) {
    return 0;
  }

  news_assert_retnonevoid(get_XX(&xx), 0);

  /* initialize record */
  memset(&newui, 0, sizeof(newui));
  newui.user_dbref = user;
  newui.total_posts = 0;
  strcpy(newui.first_group, "");
  strcpy(newui.last_read_group, "info");
  strcpy(newui.first_art_group, "");
  newui.first_art_seq = -1;
  newui.next_user_dbref = -1;

  /* check for head insertion */

  if( xx.first_user_dbref > user ||
      xx.first_user_dbref == -1 ) {
    newui.next_user_dbref = xx.first_user_dbref;
    xx.first_user_dbref = user;
    put_XX(&xx);
    put_UI(&newui);
  }
  else {
    /* non-head insertion */
    for( userkey = xx.first_user_dbref;
         userkey != -1;
         userkey = ui.next_user_dbref ) {
      news_assert_retnonevoid(get_UI(&ui, userkey), 0);

      if( ui.next_user_dbref > user ||
          ui.next_user_dbref == -1 ) {
        newui.next_user_dbref = ui.next_user_dbref;
        ui.next_user_dbref = user;
        put_UI(&ui);
        put_UI(&newui);
        break;
      }
    }
    news_assert_retnonevoid( userkey != -1, 0 );
  }
  add_user_to_group("info", user);
  add_group_to_user(user, "info");
  add_user_to_group("general", user);
  add_group_to_user(user, "general");
  if( player == user ) {
    notify(player, unsafe_tprintf("News: Welcome to the %s news system.", 
           mudconf.mud_name));
    notify(player, "News: New members join the groups 'general' and 'info' by default.");
  }
  else {
    notify(player, "News: New user initialized to news system.");
  }
  return 1;
}

int user_subscribed(dbref user, char* group_name)
{
  NewsUserGroupRec ug;

  if( !get_UG(&ug, user, group_name) ) {
    return 0;
  }
  return 1;
}

int user_passes_post(dbref user, char* group_name)
{
  NewsGroupPostLockRec gp;
  char* resultbuff;
  int result;

  if( !get_GP(&gp, group_name) ) {
    return 1;
  }

  resultbuff = exec(user, user, user, EV_FCHECK|EV_STRIP|EV_EVAL, 
                    gp.text, NULL, 0, (char **)NULL, 0);

  result = atoi(resultbuff);
  free_lbuf(resultbuff);

  if( Wizard(user) ||
      result ) {
    return 1;
  }
  return 0;
}

int user_passes_read(dbref user, char* group_name)
{
  NewsGroupReadLockRec gr;
  char* resultbuff;
  int result;

  if( !get_GR(&gr, group_name) ) {
    return 1;
  }

  resultbuff = exec(user, user, user, EV_FCHECK|EV_STRIP|EV_EVAL, 
                    gr.text, NULL, 0, (char **)NULL, 0);

  result = atoi(resultbuff);
  free_lbuf(resultbuff);

  if( Wizard(user) ||
      result ) {
    return 1;
  }
  return 0;
}

int user_passes_admin(dbref user, char* group_name)
{
  NewsGroupAdminLockRec gx;
  char* resultbuff;
  int result;

  if( !get_GX(&gx, group_name) ) {
    return 0;  /* Default for admin lock is nobody allowed if not set */
  }

  resultbuff = exec(user, user, user, EV_FCHECK|EV_STRIP|EV_EVAL, 
                    gx.text, NULL, 0, (char **)NULL, 0);

  result = atoi(resultbuff);
  free_lbuf(resultbuff);

  if( Wizard(user) ||
      result ) {
    return 1;
  }
  return 0;
}

int delete_article_from_user(dbref user, char* group_name, int seq)
{
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserArticleRec tempua;
  char *tempgroupkeyptr;
  int tempseq;
  int found = 0;

  if( !get_UI(&ui, user) ) {
    /* user has been nuked */
    return 1;
  }

  if( !get_UA(&ua, user, group_name, seq) ) {
    /* user's article has already been nuked */
    return 1;
  }

  /* check for first article */
  if( strcmp(ui.first_art_group, group_name) == 0 &&
      ui.first_art_seq == seq ) {
    strcpy(ui.first_art_group, ua.next_art_group);
    ui.first_art_seq = ua.next_art_seq;
    
    put_UI(&ui);
  }
  else {
    for(tempgroupkeyptr = ui.first_art_group, tempseq = ui.first_art_seq;
        *tempgroupkeyptr;
        tempgroupkeyptr = tempua.next_art_group, 
        tempseq = tempua.next_art_seq ) {
      news_assert_retnonevoid(get_UA(&tempua, user, tempgroupkeyptr, tempseq), 0);
      if( strcmp(tempua.next_art_group, group_name) == 0 &&
          tempua.next_art_seq == seq ) {
        strcpy(tempua.next_art_group, ua.next_art_group);
        tempua.next_art_seq = ua.next_art_seq;
    
        put_UA(&tempua);
        found = 1;
        break;
      }
    }
    news_assert_retnonevoid(found, 0);
  }
  del_UA(&ua);

  return 1;
}

int delete_article_from_group(char* group_name, int seq)
{
  NewsGroupInfoRec gi;
  NewsGroupArticleInfoRec ga;
  NewsGroupArticleInfoRec tempga;
  NewsGroupArticleTextRec gt;
  int tempseq;

  news_assert_retnonevoid(get_GI(&gi, group_name), 0);
  news_assert_retnonevoid(get_GA(&ga, group_name, seq), 0);

  /* check for first article */
  if( gi.first_seq == seq ) {
    gi.first_seq = ga.next_seq;
    put_GI(&gi);
  }
  else {
    for( tempseq = gi.first_seq;
         tempseq != -1;
         tempseq = tempga.next_seq ) {
      news_assert_retnonevoid(get_GA(&tempga, group_name, tempseq), 0);

      if( tempga.next_seq == seq ) {
        tempga.next_seq = ga.next_seq;
        put_GA(&tempga);
        break;
      }
    }
    news_assert_retnonevoid(tempseq != -1, 0);
  }

  if( get_GT(&gt, group_name, seq) ) {
    del_GT(&gt);
  }
  del_GA(&ga);

  return 1;
}

int add_user_to_group(char* group_name, dbref user)
{
  NewsGroupInfoRec gi;
  NewsGroupUserRec newgu;
  NewsGroupUserRec gu;
  dbref tempuserkey;

  news_assert_retnonevoid(get_GI(&gi, group_name), 0);

  /* initialize new GU record */

  memset(&newgu, 0, sizeof(newgu));
  strcpy(newgu.group_name, group_name);
  newgu.user_dbref = user;
  newgu.next_user_dbref = -1;

  /* check for head insertion */
  if( gi.first_user_dbref > user ||
      gi.first_user_dbref == -1 ) {
    newgu.next_user_dbref = gi.first_user_dbref;
    gi.first_user_dbref = user;
    put_GI(&gi);
    put_GU(&newgu);
    return 0;
  }
  else {
    /* non-head insertion */
    for( tempuserkey = gi.first_user_dbref;
         tempuserkey != -1;
         tempuserkey = gu.next_user_dbref ) {
      news_assert_retnonevoid(get_GU(&gu, group_name, tempuserkey), 0);

      if( gu.next_user_dbref > user ||
          gu.next_user_dbref == -1 ) {
        newgu.next_user_dbref = gu.next_user_dbref;
        gu.next_user_dbref = user;
        put_GU(&gu);
        put_GU(&newgu);
        break;
      }
    }
    news_assert_retnonevoid( tempuserkey != -1, 0);
  }

  return 1;
}

int add_group_to_user(dbref user, char* group_name)
{
  NewsUserInfoRec ui;
  NewsUserGroupRec newug;
  NewsUserGroupRec ug;
  NewsGroupInfoRec gi;
  char* tempgroupkeyptr;
  int found = 0;

  news_assert_retnonevoid(get_UI(&ui, user), 0);
  news_assert_retnonevoid(get_GI(&gi, group_name), 0);

  /* Initialize new UG record */
  memset(&newug, 0, sizeof(newug));
  newug.user_dbref = user;
  strcpy(newug.group_name, group_name);
  newug.seq_marker = gi.first_seq == -1 ? 0 : gi.first_seq;
  strcpy(newug.next_group, "");

  /* check for head insertion */
  if( strcmp(ui.first_group, group_name) > 0 ||
      !*ui.first_group ) {
    strcpy(newug.next_group, ui.first_group);
    strcpy(ui.first_group, group_name);
    put_UI(&ui);
    put_UG(&newug);
    return 0;
  }
  else {
    /* non-head insertion */
    for( tempgroupkeyptr = ui.first_group;
         *tempgroupkeyptr;
         tempgroupkeyptr = ug.next_group ) {
      news_assert_retnonevoid(get_UG(&ug, user, tempgroupkeyptr), 0);
    
      if( strcmp(ug.next_group, group_name) > 0 ||
          !*ug.next_group ) {
        strcpy(newug.next_group, ug.next_group);
        strcpy(ug.next_group, group_name);
        put_UG(&ug);
        put_UG(&newug);
        found = 1;
        break;
      }
    }
    news_assert_retnonevoid(found, 0);
  }
  return 1;
}

  
int delete_group_from_user(dbref user, char* group_name) 
{
  NewsUserInfoRec ui;
  NewsUserGroupRec ug;
  NewsUserGroupRec tempug;
  char* tempgroupkeyptr;
  int found = 0;

  news_assert_retnonevoid(get_UI(&ui, user), 0);
  news_assert_retnonevoid(get_UG(&ug, user, group_name), 0);

  if( strcmp(ui.first_group, group_name) == 0 ) {
    strcpy(ui.first_group, ug.next_group);
    put_UI(&ui);
  }
  else {
    for( tempgroupkeyptr = ui.first_group;
         *tempgroupkeyptr;
         tempgroupkeyptr = tempug.next_group ) {
      news_assert_retnonevoid(get_UG(&tempug, user, tempgroupkeyptr), 0);
    
      if( strcmp(tempug.next_group, group_name) == 0 ) {
        strcpy(tempug.next_group, ug.next_group);
        put_UG(&tempug);
        found = 1;
        break;
      }
    }
    news_assert_retnonevoid(found, 0);
  }
  del_UG(&ug);

  return 1;
}
 
int delete_user_from_group(char* group_name, dbref user)
{
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupUserRec tempgu;
  dbref tempuserkey;

  news_assert_retnonevoid(get_GI(&gi, group_name), 0);
  news_assert_retnonevoid(get_GU(&gu, group_name, user), 0);

  if( gi.first_user_dbref == user ) {
    gi.first_user_dbref = gu.next_user_dbref;
    put_GI(&gi);
  }
  else {
    for( tempuserkey = gi.first_user_dbref;
         tempuserkey != -1;
         tempuserkey = tempgu.next_user_dbref ) {
      news_assert_retnonevoid(get_GU(&tempgu, group_name, tempuserkey), 0);

      if( tempgu.next_user_dbref == user ) {
        tempgu.next_user_dbref = gu.next_user_dbref;
        put_GU(&tempgu);
        break;
      }
    }
    news_assert_retnonevoid( tempuserkey != -1, 0 );
  }

  del_GU(&gu);

  return 1;
}

int news_dbck_internal( void )
{
  NewsVersionRec vv;
  NewsGlobalRec xx;
  NewsGroupInfoRec gi;
  NewsGroupUserRec gu;
  NewsGroupArticleInfoRec ga;
  static NewsGroupArticleTextRec gt;
  NewsUserInfoRec ui;
  NewsUserArticleRec ua;
  NewsUserGroupRec ug;

  char *groupkeyptr;
  dbref userkey;
  int seqkey;

  if( !get_VV(&vv) ) {
    news_halt("DBCK/Missing VV record.");
    return 0;
  }

  if( !get_XX(&xx) ) {
    news_halt("DBCK/Missing XX record.");
    return 0;
  }

  for( groupkeyptr = xx.first_group; 
       *groupkeyptr; 
       groupkeyptr = gi.next_group ) {
    if( !get_GI(&gi, groupkeyptr) ) {
      news_halt("DBCK/Missing GI or corrupted GI key '%s'.", groupkeyptr);
      return 0;
    }

    if( gi.adder != -1 &&
        !isPlayer(gi.adder) ) {
      news_halt("DBCK/Group '%s' adder #%d is not a player anymore.",
                gi.name, gi.adder);
      return 0;
    }

    for( userkey = gi.first_user_dbref;
         userkey != -1;
         userkey = gu.next_user_dbref ) {
      if( !get_GU(&gu, gi.name, userkey) ) {
        news_halt("DBCK/Missing GU or corrupted GU key '%s/%d'.", 
                  gi.name, userkey);
        return 0;
      }

      if( !get_UG(&ug, userkey, gi.name) ) {
        news_halt("DBCK/Bad ref to UG from GU '%s/%d'.", 
                  gi.name, userkey);
        return 0;
      }
    }

    for( seqkey = gi.first_seq;
         seqkey != -1;
         seqkey = ga.next_seq ) {
      if( !get_GA(&ga, gi.name, seqkey) ) {
        news_halt("DBCK/Missing GA or corrupted GA key '%s/%d'.",
                  gi.name, seqkey);
        return 0;
      }

      if( !get_GT(&gt, gi.name, seqkey) ) {
        news_halt("DBCK/Missing GT or corrupted GT key '%s/%d'.",
                  gi.name, seqkey);
        return 0;
      }
 
      if( ga.owner_nuked_flag != OWNER_NUKE_DEAD &&
          !get_UA(&ua, ga.owner_dbref, gi.name, seqkey) ) {
        news_halt("DBCK/Bad ref to UA '%d' from GA '%s/%d'.",
                  ga.owner_dbref, gi.name, seqkey);
        return 0;
      }
    }
  }

  for( userkey = xx.first_user_dbref;
       userkey != -1;
       userkey = ui.next_user_dbref ) {
    if( !get_UI(&ui, userkey) ) {
      news_halt("DBCK/Missing UI or corrupted UI key '%d'.", userkey);
      return 0;
    }

    for( groupkeyptr = ui.first_group; 
         *groupkeyptr;
         groupkeyptr = ug.next_group ) {
      if( !get_UG(&ug, userkey, groupkeyptr) ) {
        news_halt("DBCK/Missing UG or corrupted UG key '%d/%s'.",
                  userkey, groupkeyptr);
        return 0;
      }

      if( !get_GU(&gu, ug.group_name, userkey) ) {
        news_halt("DBCK/Bad ref to GU from UG '%d/%s'.", 
                  userkey, ug.group_name);
        return 0;
      }
    }

    for( groupkeyptr = ui.first_art_group, seqkey = ui.first_art_seq;
         *groupkeyptr;
         groupkeyptr = ua.next_art_group, seqkey = ua.next_art_seq ) {
      if( !get_UA(&ua, userkey, groupkeyptr, seqkey) ) {
        news_halt("DBCK/Missing UA or corrupted UA key '%d/%s/%d'.",
                  userkey, groupkeyptr, seqkey);
        return 0;
      }

      if( !get_GA(&ga, ua.group_name, seqkey) ) {
        news_halt("DBCK/Bad ref to GA from UA '%d/%s/%d'.", 
                  userkey, ua.group_name, seqkey);
        return 0;
      }
    }
  }
  return 1;
}

/* ------------------- low level db functions -------------------- */

int get_VV(NewsVersionRec* vv)
{
  datum key;
  datum data;
  char keybuf[VV_KEY_LEN];

  memset(keybuf, 0, VV_KEY_LEN);
  memcpy(keybuf, "VV", 2);
  key.dptr = keybuf;
  key.dsize = VV_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(vv, data.dptr, data.dsize);

  return 1;
}

int get_XX_v1(NewsGlobalRec_v1* xx)
{
  datum key;
  datum data;
  char keybuf[XX_KEY_LEN_V1];

  memset(keybuf, 0, XX_KEY_LEN_V1);
  memcpy(keybuf, "XX", 2);
  key.dptr = keybuf;
  key.dsize = XX_KEY_LEN_V1;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(xx, data.dptr, data.dsize);

  return 1;
}

int get_XX(NewsGlobalRec* xx)
{
  datum key;
  datum data;
  char keybuf[XX_KEY_LEN];

  memset(keybuf, 0, XX_KEY_LEN);
  memcpy(keybuf, "XX", 2);
  key.dptr = keybuf;
  key.dsize = XX_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(xx, data.dptr, data.dsize);

  return 1;
}

int get_GI_v1(NewsGroupInfoRec_v1* gi,        char* group_name)
{
  datum key;
  datum data;
  char keybuf[GI_KEY_LEN_V1];

  memset(keybuf, 0, GI_KEY_LEN_V1);
  memcpy(keybuf, "GI", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GI_KEY_LEN_V1;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gi, data.dptr, data.dsize);

  return 1;
}

int get_GI(NewsGroupInfoRec* gi,        char* group_name)
{
  datum key;
  datum data;
  char keybuf[GI_KEY_LEN];

  memset(keybuf, 0, GI_KEY_LEN);
  memcpy(keybuf, "GI", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GI_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gi, data.dptr, data.dsize);

  return 1;
}

int get_GR(NewsGroupReadLockRec* gr,    char* group_name)
{
  datum key;
  datum data;
  char keybuf[GR_KEY_LEN];

  memset(keybuf, 0, GR_KEY_LEN);
  memcpy(keybuf, "GR", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GR_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gr, data.dptr, data.dsize);

  return 1;
}

int get_GP(NewsGroupPostLockRec* gp,    char* group_name)
{
  datum key;
  datum data;
  char keybuf[GP_KEY_LEN];

  memset(keybuf, 0, GP_KEY_LEN);
  memcpy(keybuf, "GP", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GP_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gp, data.dptr, data.dsize);

  return 1;
}

int get_GX(NewsGroupAdminLockRec* gx,    char* group_name)
{
  datum key;
  datum data;
  char keybuf[GX_KEY_LEN];

  memset(keybuf, 0, GX_KEY_LEN);
  memcpy(keybuf, "GX", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GX_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gx, data.dptr, data.dsize);

  return 1;
}

int get_GA(NewsGroupArticleInfoRec* ga, char* group_name, int seq)
{
  datum key;
  datum data;
  char keybuf[GA_KEY_LEN];

  memset(keybuf, 0, GA_KEY_LEN);
  memcpy(keybuf, "GA", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GA_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(ga, data.dptr, data.dsize);

  return 1;
}

int get_GT(NewsGroupArticleTextRec* gt, char* group_name, int seq)
{
  datum key;
  datum data;
  char keybuf[GT_KEY_LEN];

  memset(keybuf, 0, GT_KEY_LEN);
  memcpy(keybuf, "GT", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GT_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gt, data.dptr, data.dsize);

  return 1;
}

int get_GU(NewsGroupUserRec* gu,        char* group_name, dbref user_dbref)
{
  datum key;
  datum data;
  char keybuf[GU_KEY_LEN];

  memset(keybuf, 0, GU_KEY_LEN);
  memcpy(keybuf, "GU", 2);
  strncpy(keybuf + 2, group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = GU_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(gu, data.dptr, data.dsize);

  return 1;
}

int get_UI(NewsUserInfoRec* ui,    dbref user_dbref)
{
  datum key;
  datum data;
  char keybuf[UI_KEY_LEN];

  memset(keybuf, 0, UI_KEY_LEN);
  memcpy(keybuf, "UI", 2);
  memcpy(keybuf + 2, &user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = UI_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(ui, data.dptr, data.dsize);

  return 1;
}

int get_UG(NewsUserGroupRec* ug,   dbref user_dbref, char* group_name)
{
  datum key;
  datum data;
  char keybuf[UG_KEY_LEN];

  memset(keybuf, 0, UG_KEY_LEN);
  memcpy(keybuf, "UG", 2);
  memcpy(keybuf + 2, &user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = UG_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(ug, data.dptr, data.dsize);

  return 1;
}

int get_UA(NewsUserArticleRec* ua, dbref user_dbref, char* group_name, int seq)
{
  datum key;
  datum data;
  char keybuf[UA_KEY_LEN];

  memset(keybuf, 0, UA_KEY_LEN);
  memcpy(keybuf, "UA", 2);
  memcpy(keybuf + 2, &user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + sizeof(dbref) + GROUP_NAME_LEN, &seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = UA_KEY_LEN;

  data = dbm_fetch(ndb, key);

  if( !data.dptr ) {
    return 0;
  }

  memcpy(ua, data.dptr, data.dsize);

  return 1;
}

int put_XX(NewsGlobalRec* xx)
{
  datum key;
  datum data;
  char keybuf[XX_KEY_LEN];

  memset(keybuf, 0, XX_KEY_LEN);
  memcpy(keybuf, "XX", 2);
  key.dptr = keybuf;
  key.dsize = XX_KEY_LEN;

  data.dptr = (char*)xx;
  data.dsize = sizeof(NewsGlobalRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_VV(NewsVersionRec* vv)
{
  datum key;
  datum data;
  char keybuf[VV_KEY_LEN];

  memset(keybuf, 0, VV_KEY_LEN);
  memcpy(keybuf, "VV", 2);
  key.dptr = keybuf;
  key.dsize = VV_KEY_LEN;

  data.dptr = (char*)vv;
  data.dsize = sizeof(NewsVersionRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GI(NewsGroupInfoRec* gi)
{
  datum key;
  datum data;
  char keybuf[GI_KEY_LEN];

  memset(keybuf, 0, GI_KEY_LEN);
  memcpy(keybuf, "GI", 2);
  strncpy(keybuf + 2, gi->name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GI_KEY_LEN;

  data.dptr = (char*)gi;
  data.dsize = sizeof(NewsGroupInfoRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GR(NewsGroupReadLockRec* gr)
{
  datum key;
  datum data;
  char keybuf[GR_KEY_LEN];

  memset(keybuf, 0, GR_KEY_LEN);
  memcpy(keybuf, "GR", 2);
  strncpy(keybuf + 2, gr->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GR_KEY_LEN;

  data.dptr = (char*)gr;
  data.dsize = sizeof(NewsGroupReadLockRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GP(NewsGroupPostLockRec* gp)
{
  datum key;
  datum data;
  char keybuf[GP_KEY_LEN];

  memset(keybuf, 0, GP_KEY_LEN);
  memcpy(keybuf, "GP", 2);
  strncpy(keybuf + 2, gp->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GP_KEY_LEN;

  data.dptr = (char*)gp;
  data.dsize = sizeof(NewsGroupPostLockRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GX(NewsGroupAdminLockRec* gx)
{
  datum key;
  datum data;
  char keybuf[GX_KEY_LEN];

  memset(keybuf, 0, GX_KEY_LEN);
  memcpy(keybuf, "GX", 2);
  strncpy(keybuf + 2, gx->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GX_KEY_LEN;

  data.dptr = (char*)gx;
  data.dsize = sizeof(NewsGroupAdminLockRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GA(NewsGroupArticleInfoRec* ga)
{
  datum key;
  datum data;
  char keybuf[GA_KEY_LEN];

  memset(keybuf, 0, GA_KEY_LEN);
  memcpy(keybuf, "GA", 2);
  strncpy(keybuf + 2, ga->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &ga->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GA_KEY_LEN;

  data.dptr = (char*)ga;
  data.dsize = sizeof(NewsGroupArticleInfoRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GT(NewsGroupArticleTextRec* gt)
{
  datum key;
  datum data;
  char keybuf[GT_KEY_LEN];

  memset(keybuf, 0, GT_KEY_LEN);
  memcpy(keybuf, "GT", 2);
  strncpy(keybuf + 2, gt->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &gt->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GT_KEY_LEN;

  data.dptr = (char*)gt;
  /* save some space in the db, no sense sticking mostly empty LBUF's in 
     there */
  data.dsize = sizeof(NewsGroupArticleTextRec) - 
               (ART_TEXT_LEN - (strlen(gt->text) + 1));

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_GU(NewsGroupUserRec* gu)
{
  datum key;
  datum data;
  char keybuf[GU_KEY_LEN];

  memset(keybuf, 0, GU_KEY_LEN);
  memcpy(keybuf, "GU", 2);
  strncpy(keybuf + 2, gu->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &gu->user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = GU_KEY_LEN;

  data.dptr = (char*)gu;
  data.dsize = sizeof(NewsGroupUserRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}


int put_UI(NewsUserInfoRec* ui)
{
  datum key;
  datum data;
  char keybuf[UI_KEY_LEN];

  memset(keybuf, 0, UI_KEY_LEN);
  memcpy(keybuf, "UI", 2);
  memcpy(keybuf + 2, &ui->user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = UI_KEY_LEN;

  data.dptr = (char*)ui;
  data.dsize = sizeof(NewsUserInfoRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_UG(NewsUserGroupRec* ug)
{
  datum key;
  datum data;
  char keybuf[UG_KEY_LEN];

  memset(keybuf, 0, UG_KEY_LEN);
  memcpy(keybuf, "UG", 2);
  memcpy(keybuf + 2, &ug->user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), ug->group_name, GROUP_NAME_LEN);
  key.dptr = (char*)keybuf;
  key.dsize = UG_KEY_LEN;

  data.dptr = (char*)ug;
  data.dsize = sizeof(NewsUserGroupRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int put_UA(NewsUserArticleRec* ua)
{
  datum key;
  datum data;
  char keybuf[UA_KEY_LEN];

  memset(keybuf, 0, UA_KEY_LEN);
  memcpy(keybuf, "UA", 2);
  memcpy(keybuf + 2, &ua->user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), ua->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + sizeof(dbref) + GROUP_NAME_LEN, 
         &ua->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = UA_KEY_LEN;

  data.dptr = (char*)ua;
  data.dsize = sizeof(NewsUserArticleRec);

  news_assert_retnonevoid(!dbm_store(ndb, key, data, DBM_REPLACE), 0);

  return 1;
}

int del_VV(NewsVersionRec* vv)
{
  datum key;
  char keybuf[VV_KEY_LEN];

  memset(keybuf, 0, VV_KEY_LEN);
  memcpy(keybuf, "VV", 2);
  key.dptr = keybuf;
  key.dsize = VV_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_XX(NewsGlobalRec* xx)
{
  datum key;
  char keybuf[XX_KEY_LEN];

  memset(keybuf, 0, XX_KEY_LEN);
  memcpy(keybuf, "XX", 2);
  key.dptr = keybuf;
  key.dsize = XX_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GI(NewsGroupInfoRec* gi)
{
  datum key;
  char keybuf[GI_KEY_LEN];

  memset(keybuf, 0, GI_KEY_LEN);
  memcpy(keybuf, "GI", 2);
  strncpy(keybuf + 2, gi->name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GI_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GR(NewsGroupReadLockRec* gr)
{
  datum key;
  char keybuf[GR_KEY_LEN];

  memset(keybuf, 0, GR_KEY_LEN);
  memcpy(keybuf, "GR", 2);
  strncpy(keybuf + 2, gr->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GR_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GP(NewsGroupPostLockRec* gp)
{
  datum key;
  char keybuf[GP_KEY_LEN];

  memset(keybuf, 0, GP_KEY_LEN);
  memcpy(keybuf, "GP", 2);
  strncpy(keybuf + 2, gp->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GP_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GX(NewsGroupAdminLockRec* gx)
{
  datum key;
  char keybuf[GX_KEY_LEN];

  memset(keybuf, 0, GX_KEY_LEN);
  memcpy(keybuf, "GX", 2);
  strncpy(keybuf + 2, gx->group_name, GROUP_NAME_LEN);
  key.dptr = keybuf;
  key.dsize = GX_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GA(NewsGroupArticleInfoRec* ga)
{
  datum key;
  char keybuf[GA_KEY_LEN];

  memset(keybuf, 0, GA_KEY_LEN);
  memcpy(keybuf, "GA", 2);
  strncpy(keybuf + 2, ga->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &ga->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GA_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GT(NewsGroupArticleTextRec* gt)
{
  datum key;
  char keybuf[GT_KEY_LEN];

  memset(keybuf, 0, GT_KEY_LEN);
  memcpy(keybuf, "GT", 2);
  strncpy(keybuf + 2, gt->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &gt->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = GT_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_GU(NewsGroupUserRec* gu)
{
  datum key;
  char keybuf[GU_KEY_LEN];

  memset(keybuf, 0, GU_KEY_LEN);
  memcpy(keybuf, "GU", 2);
  strncpy(keybuf + 2, gu->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + GROUP_NAME_LEN, &gu->user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = GU_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}


int del_UI(NewsUserInfoRec* ui)
{
  datum key;
  char keybuf[UI_KEY_LEN];

  memset(keybuf, 0, UI_KEY_LEN);
  memcpy(keybuf, "UI", 2);
  memcpy(keybuf + 2, &ui->user_dbref, sizeof(dbref));
  key.dptr = keybuf;
  key.dsize = UI_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_UG(NewsUserGroupRec* ug)
{
  datum key;
  char keybuf[UG_KEY_LEN];

  memset(keybuf, 0, UG_KEY_LEN);
  memcpy(keybuf, "UG", 2);
  memcpy(keybuf + 2, &ug->user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), ug->group_name, GROUP_NAME_LEN);
  key.dptr = (char*)keybuf;
  key.dsize = UG_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}

int del_UA(NewsUserArticleRec* ua)
{
  datum key;
  char keybuf[UA_KEY_LEN];

  memset(keybuf, 0, UA_KEY_LEN);
  memcpy(keybuf, "UA", 2);
  memcpy(keybuf + 2, &ua->user_dbref, sizeof(dbref));
  strncpy(keybuf + 2 + sizeof(dbref), ua->group_name, GROUP_NAME_LEN);
  memcpy(keybuf + 2 + sizeof(dbref) + GROUP_NAME_LEN, 
         &ua->seq, sizeof(int));
  key.dptr = keybuf;
  key.dsize = UA_KEY_LEN;

  news_assert_retnonevoid(!dbm_delete(ndb, key), 0);

  return 1;
}
