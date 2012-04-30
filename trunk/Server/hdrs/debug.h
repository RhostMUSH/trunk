#ifndef _M_DEBUG_H__
#define _M_DEBUG_H__

#define STACKMAX 1000

#define RETURN(x)   {DPOP; return (x);}
#define VOIDRETURN   {DPOP; return;}

typedef struct Debugmem Debugmem;
struct Debugmem {
  int mushport;
  int debugport;
  int lastdbfetch;
  int stacktop;
  int stackval;
  struct {
    int filenum;
    int linenum;
  } callstack[STACKMAX];
};

extern Debugmem *debugmem;

Debugmem * shmConnect(int debug_id, int create, int *pShmid);

#define INITDEBUG(x) { x->lastdbfetch = -1; x->stacktop = 0; x->stackval = 0; }

#ifndef NODEBUGMONITOR
#define DPUSH   {if(debugmem && debugmem->stacktop < STACKMAX) { \
                  debugmem->callstack[debugmem->stacktop].filenum = \
                    FILENUM; \
                  debugmem->callstack[debugmem->stacktop++].linenum = \
                    __LINE__; \
                  debugmem->stackval = \
                    (debugmem->stacktop > debugmem->stackval ? debugmem->stacktop : debugmem->stackval); \
                } \
                else if(debugmem) { \
                  printf("Debug Stack Overflow! (line %d of fp %d)\n", __LINE__, FILENUM); \
                  exit(1); \
                }}
#define DPOP    {if(debugmem && debugmem->stacktop > 0) { \
                  debugmem->stacktop--; \
                } \
                else if(debugmem) { \
                  printf("Debug Stack Underflow! (line %d of fp %d)\n",__LINE__, FILENUM); \
                  exit(1); \
                }}
#define DPOPCONDITIONAL   {if(debugmem && debugmem->stacktop > 0) { \
                  debugmem->stacktop--; \
                }}

#else
#define DPUSH
#define DPOP
#define DPOPCONDITIONAL
#endif

/* only add files to the end of this list, never in the middle or at
   the beginning.

   Make sure you keep the filename array in sync with the FILENUMBERS
   enumeration */

enum FILENUMBERS { ALLOC_C = 1,
                   BOOLEXP_C, 
                   BSD_C, 
                   COMMAND_C, 
                   COMPAT_C, 
                   COMPRESS_C, 
                   CONF_C, 
                   CQUE_C, 
                   CREATE_C, 
                   CREATE2_C, 
                   CRYPT_C, 
                   DB_C, 
                   DB_RW_C, 
                   DBCONVERT_C, 
		   DOOR_C,
		   EVAL_C, 
                   FILE_C_C, 
                   FLAGS_C, 
                   FUNCTIONS_C, 
                   GAME_C, 
                   GOVERN_C, 
                   HELP_C, 
                   HTAB_C, 
                   LOG_C, 
                   LOOK_C, 
                   MAIL_C, 
                   MAILFIX_C, 
                   MATCH_C, 
                   MKINDX_C, 
                   MOVE_C, 
		   MUSHCRYPT_C,
                   MYNDBM_C, 
                   NETCOMMON_C, 
                   OBJECT_C, 
                   OBJECT2_C, 
                   PLAYER_C, 
                   PLAYER_C_C, 
                   PREDICATES_C, 
		   REGEXP_C,
                   ROB_C, 
                   RWHO_CLILIB_C, 
                   SET_C,
                   SPEECH_C, 
		   SHS_C,
                   STRINGUTIL_C, 
                   STRIP_C, 
                   TIMER_C, 
                   TRIAL_C, 
                   UDB_ACACHE_C, 
                   UDB_ACHUNK_C, 
                   UDB_ATTR_C, 
                   UDB_MISC_C, 
                   UDB_OBJ_C, 
                   UDB_OCACHE_C, 
                   UDB_OCHUNK_C, 
                   UNPARSE_C, 
                   UNSPLIT_C, 
		   UTILS_C,
                   VATTR_C, 
                   VERSION_C, 
                   VMS_DBM_C, 
                   WALKDB_C, 
                   WILD_C, 
                   WIZ_C };

#ifdef __DEBUGMON_INCLUDE__ 
/* Only debug.c should define this.
 * It's left in as a #ifdef because it's handy to have this stuff
 * in the same file as the enumerations above.
 */
char *aDebugFilenameArray[] = { "*** UNKNOWN ***",
				"alloc.c",  
				"boolexp.c",
				"bsd.c",
				"command.c",
				"compat.c",
				"compress.c",
				"conf.c",
				"cque.c",
				"create.c",
				"create2.c",
				"crypt.c",
				"db.c",
				"db_rw.c",
				"dbconvert.c",
				"door.c",
				"eval.c",
				"file_c.c",
				"flags.c",
				"functions.c",
				"game.c",
				"govern.c",
				"help.c",
				"htab.c",
				"log.c",
				"look.c",
				"mail.c",
				"mailfix.c",
				"match.c",
				"mkindx.c",
				"move.c",
				"mushcrypt.c",
				"myndbm.c",
				"netcommon.c",
				"object.c",
				"object2.c",
				"player.c",
				"player_c.c",
				"predicates.c",
				"regexp.c",
				"rob.c",
				"rwho_clilib.c",
				"set.c",
				"speech.c",
				"shs.c",
				"stringutil.c",
				"strip.c",
				"timer.c",
				"trial.c",
				"udb_acache.c", 
				"udb_achunk.c", 
				"udb_attr.c", 
				"udb_misc.c", 
				"udb_obj.c", 
				"udb_ocache.c", 
				"udb_ochunk.c", 
				"unparse.c", 
				"unsplit.c", 
				"utils.c",
				"vattr.c", 
				"version.c", 
				"vms_dbm.c", 
				"walkdb.c", 
				"wild.c", 
				"wiz.c"};
#else
extern char *aDebugFilenameArray[];
#endif /* __DEBUGMON_INCLUDE__ */

#endif /* __DEBUG_H__ */
