#ifdef SQLITE

#include <sqlite3.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "functions.h"
#include "externs.h"
#include "match.h"
#include "flags.h"
#include "alloc.h"
#include "vattr.h"

static char tempbuff[LBUF_SIZE];
static char tempbuff2[LBUF_SIZE];

static int ival(char *buff, char **bufcx, int result) {
   sprintf(tempbuff, "%d", result);
   safe_str(tempbuff, buff, bufcx);
   return 0;
}

FUNCTION(local_fun_sqlite_query)
{
   time_t start;
   char dbFile[LBUF_SIZE], dbFullPath[LBUF_SIZE];
   char * colPtr, *zTail;
   DIR * tmp_dir;
   sqlite3 * sqlite_db;
   sqlite3_stmt * sqlite_stmt;
   regex_t validator;
   unsigned int i, rVal, firstA=1, firstB=1, argIdx, argCount;
   char colDelimit[LBUF_SIZE], rowDelimit[LBUF_SIZE];

   if ( mudstate.heavy_cpu_lockdown == 1 ) {
      safe_str("#-1 FUNCTION HAS BEEN LOCKED DOWN FOR HEAVY CPU USE.", buff, bufcx);
      return;
   }

   if( nfargs < 2 ) {
      safe_str( "#-1 FUNCTION (sqlite_query) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx );
      ival( buff, bufcx, nfargs );
      safe_chr( ']', buff, bufcx );
      return;
   }

   if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + mudconf.cputimechk) ) {
      safe_str("#-1 HEAVY CPU LIMIT ON PROTECTED FUNCTION EXCEEDED", buff, bufcx);
      mudstate.chkcpu_toggle = 1;
      mudstate.heavy_cpu_recurse = mudconf.heavy_cpu_max + 1;
      if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + (mudconf.cputimechk * 3)) ) {
         mudstate.heavy_cpu_lockdown = 1;
      }
      return;
   }

   mudstate.heavy_cpu_tmark2 = time(NULL);
#ifdef DEBUG_SQLITE
   printf( "Construct file paths..\n" );
#endif

   getcwd( tempbuff, LBUF_SIZE );
   snprintf( dbFullPath, LBUF_SIZE, "%s/%s", tempbuff, mudconf.sqlite_db_path );
   snprintf( dbFile, LBUF_SIZE, "%s/%s/%s.sqlite", tempbuff, mudconf.sqlite_db_path, fargs[0] );

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Arrange delimiters..\n" );
#endif

   if( nfargs >= 3 ) {
      strncpy( colDelimit, fargs[2], LBUF_SIZE );
      if( nfargs >= 4 ) {
         strncpy( rowDelimit, fargs[3], LBUF_SIZE );
      }
   } else {
      strcpy( colDelimit, "|" );
      strcpy( rowDelimit, "^" );
   }

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Validate database name..\n" );
#endif

   regcomp( &validator, "^[a-zA-Z0-9_-]+$", REG_EXTENDED | REG_NOSUB );
   if( regexec( &validator, fargs[0], 0, NULL, 0 ) != 0 ) {
      safe_str( "#-1 FUNCTION (sqlite_query) FIRST ARGUMENT MUST CONSIST OF LETTERS, NUMBERS, UNDERSCORE, and DASH [RECEIVED ", buff, bufcx );
      safe_str( fargs[0], buff, bufcx );
      safe_chr( ']', buff, bufcx );
      regfree( &validator );
      return;
   }
   regfree( &validator );

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Check directory access..\n" );
#endif

   if( (tmp_dir = opendir( dbFullPath )) == NULL ) {
      snprintf( tempbuff, LBUF_SIZE, "#-1 FUNCTION (sqlite_query) CANNOT ACCESS DIRECTORY %s: %s", dbFullPath, strerror( errno ) );
      safe_str( tempbuff, buff, bufcx );
      return;
   } else {
      closedir( tmp_dir );
   }

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Check file access..\n" );
#endif

   if( access( dbFile, F_OK ) && !access( dbFile, W_OK | R_OK ) ) {
      snprintf( tempbuff, LBUF_SIZE, "#-1 FUNCTION (sqlite_query) CANNOT ACCESS %s: %s", dbFile, strerror( errno ) );
      safe_str( tempbuff, buff, bufcx );
      return;
   }

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Open database..\n" );
#endif

   if( sqlite3_open_v2( dbFile, &sqlite_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL ) != SQLITE_OK ) {
      if( sqlite_db == NULL ) {
         safe_str( "#-1 FUNCTION (sqlite_query) COULDN'T ALLOCATE MEMORY FOR A SQLITE OBJECT", buff, bufcx );
         return;
      }
      sprintf( tempbuff, "#-1 FUNCTION (sqlite_query) COULDN'T INITIALIZE SQLITE: %s", sqlite3_errmsg( sqlite_db ) );
      safe_str( tempbuff, buff, bufcx );
      sqlite3_close( sqlite_db );
      return;
   }

   sqlite3_exec(sqlite_db, (char *)"PRAGMA foreign_keys = on", 0, (void *)NULL, (char **)NULL);

#ifdef DEBUG_SQLITE
   printf( "Done\n" );
   printf( "Prepare statement..\n" );
#endif

   argIdx = 4;
   zTail = fargs[1];
   start = time( NULL );

   while( zTail[0] != '\0' ) {
      if( ( time( NULL ) - start ) > mudconf.sqlite_query_limit ) {
         safe_str( "#-1 FUNCTION (sqlite_query) EXCEEDED QUERY LIMIT", buff, bufcx );
         break;
      }
      if( sqlite3_prepare_v2( sqlite_db, zTail, LBUF_SIZE, &sqlite_stmt, (const char **) &zTail ) != SQLITE_OK ) {
         sprintf( tempbuff, "#-1 FUNCTION (sqlite_query) FAILED PREPARING STATEMENT: %s", sqlite3_errmsg( sqlite_db ) );
         safe_str( tempbuff, buff, bufcx );
         sqlite3_finalize( sqlite_stmt );
         sqlite3_close( sqlite_db );
         return;
      }

      argCount = sqlite3_bind_parameter_count( sqlite_stmt );
      // sqlite3_bind_parameter_count returns the index of the rightmost parameter, 1-indexed.
      // Arguments to bind are 4 and onward (0-indexed)
      if( argCount > (nfargs - argIdx) ) {
         sprintf( tempbuff, "#-1 FUNCTION (sqlite_query) NOT ENOUGH PARAMETERS (Needed %d more, but only %d remain)", argCount, nfargs - argIdx );
         safe_str( tempbuff, buff, bufcx );
         sqlite3_finalize( sqlite_stmt );
         sqlite3_close( sqlite_db );
         return;
      }

#ifdef DEBUG_SQLITE
      printf( "Done\n" );
      printf( "Bind arguments..\n" );
#endif

      // i and argIdx point to an index in fargs; 4 is the 0th parm, etc.
      for( i = argIdx; (i-argIdx) < argCount; i++ ) {
#ifdef DEBUG_SQLITE
         printf( "       Arg %d..\n", i-3 );
#endif
         if( sqlite3_bind_text( sqlite_stmt, i-(argIdx-1), fargs[i], -1, SQLITE_TRANSIENT ) != SQLITE_OK ) {
            sprintf( tempbuff, "#-1 FUNCTION (sqlite_query) FAILED BINDING PARAMETER %d: %s", i-(argIdx-1), sqlite3_errmsg( sqlite_db ) );
            safe_str( tempbuff, buff, bufcx );
            sqlite3_finalize( sqlite_stmt );
            sqlite3_close( sqlite_db );
            return;
         }
#ifdef DEBUG_SQLITE
         printf( "       OK\n" );
#endif
      }
      argIdx = i;

#ifdef DEBUG_SQLITE
      printf( "Done\n" );
      printf( "Begin executing..\n" );
#endif

      while(1) {
         if( ( time( NULL ) - start ) > mudconf.sqlite_query_limit ) {
            safe_str( "#-1 FUNCTION (sqlite_query) EXCEEDED QUERY LIMIT", buff, bufcx );
            break;
         }
         rVal = sqlite3_step( sqlite_stmt );
         if( rVal == SQLITE_BUSY ) {
#ifdef DEBUG_SQLITE
            printf( "       busy..\n" );
#endif
            continue;
         }
         if( rVal == SQLITE_ROW ) {
#ifdef DEBUG_SQLITE
            printf( "       row ready..\n" );
#endif
            firstB = 1;
#ifdef DEBUG_SQLITE
            printf( "               row delimiter..\n" );
#endif
            if( firstA ) {
#ifdef DEBUG_SQLITE
               printf( "               skipped\n" );
#endif
               firstA=0;
            } else {
               safe_str( rowDelimit, buff, bufcx );
#ifdef DEBUG_SQLITE
               printf( "               sent\n" );
#endif
            }
            for( i = 0; i < sqlite3_column_count( sqlite_stmt ); i++ ) {
#ifdef DEBUG_SQLITE
               printf( "               column %d\n", i );
               printf( "                       column delimiter..\n" );
#endif
               if( firstB ) {
                  firstB = 0;
#ifdef DEBUG_SQLITE
                  printf( "                       skipped\n" );
#endif
               } else {
                  safe_str( colDelimit, buff, bufcx );
#ifdef DEBUG_SQLITE
                  printf( "                       sent\n" );
#endif
               }
#ifdef DEBUG_SQLITE
               printf( "                       copying column text..\n" );
#endif
               colPtr = (char *) sqlite3_column_text( sqlite_stmt, i );
               if( colPtr == NULL ) {
#ifdef DEBUG_SQLITE
                  printf( "                       skipped (null)\n" );
#endif
               } else {
                  strncpy( tempbuff, colPtr, LBUF_SIZE );
#ifdef DEBUG_SQLITE
                  printf( "                       sending column text..\n" );
#endif
                  safe_str( tempbuff, buff, bufcx );
#ifdef DEBUG_SQLITE
                  printf( "                       sent\n" );
#endif
               }
#ifdef DEBUG_SQLITE
               printf( "               OK\n" );
#endif
            }
#ifdef DEBUG_SQLITE
            printf( "       sent\n" );
#endif
            continue;
         }
         if( rVal == SQLITE_DONE ) {
#ifdef DEBUG_SQLITE
            printf( "       finished\n" );
#endif
            break;
         }

         // An error occured.
         sprintf( tempbuff, "#-1 FUNCTION (sqlite_query) FAILED WHILE EXECUTING STATEMENT: %s", sqlite3_errmsg( sqlite_db ) );
         safe_str( tempbuff, buff, bufcx );
         break;
      }
#ifdef DEBUG_SQLITE
      printf( "Done\n" );
#endif

      sqlite3_finalize( sqlite_stmt );
   }
   sqlite3_close( sqlite_db );
   return;
}

void local_sqlite_init(void) {
   FUN *fp;
   char *buff, *cp, *dp;
   struct stat sb;

   if( stat( mudconf.sqlite_db_path, &sb ) == -1 ) {
      getcwd( tempbuff2, LBUF_SIZE );
      STARTLOG(LOG_ALWAYS, "SQL", "FAIL")
         sprintf( tempbuff, "stat - unable to read database path '%s/%s'", tempbuff2, mudconf.sqlite_db_path );
         log_text(tempbuff);
      ENDLOG
      return;
   }

   if( ~sb.st_mode & S_IFDIR ) {
      getcwd( tempbuff2, LBUF_SIZE );
      sprintf( tempbuff, "stat - %s/%s is not a directory", tempbuff2, mudconf.sqlite_db_path );
      STARTLOG(LOG_ALWAYS, "SQL", "FAIL")
         log_text(tempbuff);
      ENDLOG
      return;
   }

   static FUN mysql_fun_table[] = {
      {"SQLITE_QUERY", local_fun_sqlite_query, 0, FN_VARARGS, CA_WIZARD, 0 },
      {NULL, NULL, 0, 0, 0, 0}
   };

/* Register the functions */
   buff = alloc_sbuf("init_sqlite_functab");
   for (fp = mysql_fun_table ; fp->name ; fp++) {
      cp = (char *) fp->name;
      dp = buff;
      while (*cp) {
         *dp = ToLower(*cp);
         cp++;
         dp++;
      }
      *dp = '\0';
      hashadd2(buff, (int *) fp, &mudstate.func_htab, 1);
   }
}

#endif // SQLITE
