#include "autoconf.h"
#include "config.h"
#include "externs.h"
#include "local.h"


/******************************************************************
 *
 *                  LOCAL CALLBACK FUNCTIONS
 *
 * Author  : Lensman
 *
 * Purpose : People writing 3rd party code for Rhost may need to
 *           insert hooks into parts of the source such that funcs
 *           within their code get called at the appropriate time.
 *           This is to help :)
 *
 * V2
 *******************************************************************/

/* Called when the mush starts up, immediatly prior to the main game
 * loop being entered. By this point all databases are loaded and
 * all variables configured.
 */
#ifdef MYSQL_VERSION
   extern void local_mysql_init(void);
   extern int sql_shutdown(dbref player);
#endif

#ifdef SQLITE
   extern void local_sqlite_init(void);
#endif /* SQLITE */

void local_startup(void) {
#ifdef SQLITE
   local_sqlite_init();
#endif /* SQLITE */
#ifdef MYSQL_VERSION
   local_mysql_init();
#endif
   load_regexp_functions();
}

/* Called immediatly after the main game loop exits. At this point
 * all databases and variables are still configured
 */
void local_shutdown(void) {
#ifdef MYSQL_VERSION
   sql_shutdown(-1);
#endif
}

/* Called after Rhost has written out the reboot file */
void local_dump_reboot(void) {

}

/* Called after Rhost has finishesd loading the reboot file */
void local_load_reboot(void) {

}

/* Called after Rhost has finished dumping her databases */
void local_dump(int bPanicDump) {

}

/* Called after Rhost has finished her internal db checks */
void local_dbck(void) {

}
/* Called once per game cycle, at the very bottom of the main
 * game loop.
 */
void local_tick(void) {

}

/* Called once per second
 */
void local_second(void) {

}

/* Called when a player connects and after all connect/aconnect
 * messages and calls have been handled
 */
void local_player_connect(dbref player, int bNew) {

}

/* Called when a player disconnects and after all disconnect/adisconnect
 * messages and calls have been handled
 */
void local_player_disconnect(dbref player) {

}

/* Called when you issue localfunc() with the name of the function and args
 * 
 */
void local_function(char *funcname, char *buff, char **bufcx, dbref player, dbref cause,
                    char *fargs[], int nfargs, char *cargs[], int ncargs) {

   if ( !funcname || !*funcname ) {
      safe_str("#-1 NO FUNCTION", buff, bufcx);
      return;
   }
   if ( stricmp(funcname, "test") == 0 ) {
      safe_str("This was a test. Function: '", buff, bufcx);
      safe_str(funcname, buff, bufcx);
      safe_str("' First Arg: '", buff, bufcx);
      safe_str(fargs[0], buff, bufcx);
      safe_chr('\'', buff, bufcx);
   } else {
      safe_str("#-1 NO FUNCTION", buff, bufcx);
   }
}

