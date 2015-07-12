
/****************************************************************************
 * Rhost MYSQL connection.
 *
 * Written by Lensman, 09 Nov, 03.
 * Adapted from PennMUSH mySQL Contrib by Hans Engelen and Javelin.
 */
 /* #define MYSQL_VER "Version 1.1 Beta" */
 /*
 * NOTE: This is _NOT_ supported or recommended by the RhostMUSH team. No
 *       gurantees are made with regards to performance or stability.
 *       In short, use it at your own risk.
 *
 *       Queries are _NOT_ run in the background, as such any long-running
 *       database access _WILL_ pause your mush until it finishes.
 *
 * Send comments, bug reports or fixes to: lensman@the-wyvern.net
 *
 */
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <stdlib.h>
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


/* Edit this if needed */
#define MYSQL_RETRY_TIMES 3

extern double NDECL(next_timer);
extern int FDECL(alarm_msec, (double));

/************* DON'T EDIT ANYTHING BELOW HERE **********/

static MYSQL *mysql_struct = NULL;
static long  queryCount = 0;
static long  numConnectionsMade = 0;
static long  numRowsRetrieved = 0;
static dbref lastConnectMadeBy = -1;

int sql_shutdown(dbref player);
static int sql_init(dbref player);
static int sql_query(dbref player, 
		     char *q_string, char row_delim, char field_delim, char *buff, char **bp);

/*************************************************************
 *
 * Okay, in this block we define all of the functions and
 * commands that get added to the mush.
 * There are three commands: @sql, @sqlconnect, @sqldisconnect
 * There are four functions: sql, sqlescape, sqlon, sqloff
 *
 * Description of the commands and functions :
 * - SqlOn()  : Returns 1 on success, 0 on failure. Connects to Database
 * - SqlOff() : Guess.
 * - Sql()    : Use like Sql(Query_String[,Row_Delimiter[,Field_Delimiter]]).
 *              Query_String is parsed so you can use U(), V(), %vx etc.
 *              Best is to put your query in an attrib and U() or V() it in
 *              the Query_String. SQL statements after al can have confusing
 *              characters like commas etc. Both delimiters are parsed as well
 *              so they could have some logic in them as well.
 * - @sqlconnect    : same as 'Th SqlOn()'
 * - @sqldisconnect : same as 'Th SqlOff()'
 * - @sql           : Similar to Sql() but with different outputformat
 *************************************************************/
void do_sql(dbref player, dbref cause, int key, char *arg_left) {
  if (!*arg_left) {
    notify(player, unsafe_tprintf("@SQL for RhostMUSH (%s [MySQL %s]) based on the PennMUSH mySQL Contrib patch", MYSQL_VER, MYSQL_VERSION));
    notify(player, unsafe_tprintf("Status is %s. (Last connection made by %s)", 
			   mysql_struct ? "CONNECTED" : "DISCONNECTED",
			   lastConnectMadeBy < 0 ? "SYSTEM" : Name(lastConnectMadeBy)));
    notify(player, unsafe_tprintf("%ld connections to database made.", numConnectionsMade));
    notify(player, unsafe_tprintf("Executed %ld queries bringing back a total of %ld rows.", 
			   queryCount, numRowsRetrieved));
  } else {
    sql_query(player, arg_left, ' ', ' ', NULL, NULL);  
  }
}

void do_sql_connect(dbref player, dbref cause, int key) {
   if (sql_init(player) < 0) {
    notify(player, "Database connection attempt failed.");
  } else {
    notify(player, "Database connection succeeded.");
  }   
}
 
void do_sql_shutdown(dbref player, dbref cause, int key) {
  if (sql_shutdown(player) < 0) {
    notify(player, "Not connected to database");
  } else {
    notify(player, "Database connection shutdown");
  }
}

FUNCTION(local_fun_sql)
{
  char row_delim, field_delim;

  if (!fn_range_check("SQL", nfargs, 1, 3, buff, bufcx)) {
    return;
  }

  if ( !*fargs[0] )
     return;

  row_delim = field_delim = ' ';
  if ( nfargs > 1 ) {
     if ( *fargs[1] ) {
        row_delim = *fargs[1];
     }
     field_delim = row_delim;
  }
  if (nfargs > 2) {
     if ( *fargs[2] ) {
        field_delim = *fargs[2];
     }
  }
  
  sql_query(player, fargs[0], row_delim, field_delim, buff, bufcx);

}

FUNCTION(local_fun_sql_connect) {
  if (sql_init(player) < 0) {
      safe_str("0", buff, bufcx);
  } else {
      safe_str("1", buff, bufcx);
  }
}

FUNCTION(local_fun_sql_disconnect) {
  if (sql_shutdown(player) < 0) {
      safe_str("0", buff, bufcx);
  } else {
      safe_str("1", buff, bufcx);

  }
}

FUNCTION(local_fun_sql_escape) {
  int retries;
  static char bigbuff[LBUF_SIZE * 2 + 1], *s_localchr;

  if (!fargs[0] || !*fargs[0])
    return;

  memset(bigbuff, '\0', sizeof(bigbuff));

  if (!mysql_struct) {
    /* Try to reconnect. */
    retries = 0;
    while ((retries < MYSQL_RETRY_TIMES) && !mysql_struct) {
      nanosleep((struct timespec[]){{0, 900000000}}, NULL);
      sql_init(cause);
      retries++;
    }
  }

  if (!mysql_struct || (mysql_ping(mysql_struct) != 0)) {
    safe_str("#-1 NO CONNECTION", buff, bufcx);
    mysql_struct = NULL;
    return;
  }
  
  s_localchr = alloc_lbuf("local_fun_sql_escape");
  memset(s_localchr, '\0', LBUF_SIZE);
  strncpy(s_localchr, fargs[0], LBUF_SIZE-2);
  if (mysql_real_escape_string(mysql_struct, bigbuff, s_localchr, strlen(s_localchr)) < LBUF_SIZE) {
    bigbuff[LBUF_SIZE - 1] = '\0';
    bigbuff[LBUF_SIZE - 2] = '\0';
    safe_str(bigbuff, buff, bufcx);
  } else {
    safe_str("#-1 TOO LONG", buff, bufcx);
  }
  free_lbuf(s_localchr);
}

void local_mysql_init(void) {
  /* Setup our local command and function tables */
  static CMDENT mysql_cmd_table[] = {
    {(char *) "@sql", NULL, CA_WIZARD, 0, 0, CS_ONE_ARG, 0, do_sql},
    {(char *) "@sqlconnect", NULL, CA_WIZARD, 0, 0, CS_NO_ARGS, 0, do_sql_connect},
    {(char *) "@sqldisconnect", NULL, CA_WIZARD, 0, 0, CS_NO_ARGS, 0, do_sql_shutdown},
    {(char *) NULL, NULL, 0, 0, 0, 0, 0, NULL}

  };
  
  static FUN mysql_fun_table[] = {
    {"SQL", local_fun_sql, 0, FN_VARARGS, CA_WIZARD, 0},
    {"SQLESCAPE", local_fun_sql_escape, 1, 0, CA_WIZARD, 0},
    {"SQLON", local_fun_sql_connect, 0, 0, CA_WIZARD, 0},
    {"SQLOFF", local_fun_sql_disconnect, 0, 0, CA_WIZARD, 0},
    {NULL, NULL, 0, 0, 0, 0}
  };

  CMDENT *cmdp;
  FUN *fp;
  char *buff, *cp, *dp;

  /* Add the commands to the command table */
  for (cmdp = mysql_cmd_table; cmdp->cmdname; cmdp++) {
    cmdp->cmdtype = CMD_LOCAL_e;
    hashadd(cmdp->cmdname, (int *) cmdp, &mudstate.command_htab);
  }

  /* Register the functions */
  buff = alloc_sbuf("init_mysql_functab");
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
  free_sbuf(buff);

  sql_init(-1);
}


int sql_shutdown(dbref player) {

  if (!mysql_struct)
    return -1;

  mysql_close(mysql_struct);
  mysql_struct = NULL;
  return 0;
}
 
static int sql_init(dbref player) {
  MYSQL *result;
  
  /* If we are already connected, drop and retry the connection, in
   * case for some reason the server went away.
   */
  
  if (mysql_struct)
    sql_shutdown(player);
  
  /* Try to connect to the database host. If we have specified
   * localhost, use the Unix domain socket instead.
   */
  
  mysql_struct = mysql_init(mysql_struct);
  
  result = mysql_real_connect(mysql_struct, mudconf.mysql_host, mudconf.mysql_user,
                              mudconf.mysql_pass, mudconf.mysql_base, mudconf.mysql_port,
                              mudconf.mysql_socket, 0);

  if (!result) {
    STARTLOG(LOG_PROBLEMS, "SQL", "ERR");
    log_text(unsafe_tprintf("DB connect by %s : ", player < 0 ? "SYSTEM" : Name(player)));
    log_text("Failed to connect to SQL database.");
    ENDLOG
    return -1;
  } else {
    STARTLOG(LOG_PROBLEMS, "SQL", "INF");
    log_text(unsafe_tprintf("DB connect by %s : ", player < 0 ? "SYSTEM" : Name(player)));
    log_text("Connected to SQL database.");
    ENDLOG
  }
  
  numConnectionsMade++;
  lastConnectMadeBy = player;
  numRowsRetrieved = queryCount = 0;
  return 1;
}

#define print_sep(s,b,p) \
 if (s) { \
     if (s != '\n') { \
       safe_chr(s,b,p); \
     } else { \
       safe_str((char *) "\n",b,p); \
     } \
 }
 
static int sql_query(dbref player, 
		     char *q_string, char row_delim, char field_delim, char *buff, char **bp) {
  MYSQL_RES *qres;
  MYSQL_ROW row_p;
  int num_rows, got_rows, got_fields;
  int i, j;
  int retries;
  char *tpr_buff, *tprp_buff, *s_qstr;
  
  /* If we have no connection, and we don't have auto-reconnect on
   * (or we try to auto-reconnect and we fail), this is an error
   * generating a #-1. Notify the player, too, and set the return code.
   */
  
  if ( mysql_struct && !mysql_ping(mysql_struct) ) {
     sql_shutdown(player);
  }
  if (!mysql_struct) {
    /* Try to reconnect. */
    retries = 0;
    while ((retries < MYSQL_RETRY_TIMES) && !mysql_struct) {
      nanosleep((struct timespec[]){{0, 900000000}}, NULL);
      sql_init(player);
      retries++;
    }
  }
  /* If there's a valid structure, but it's not responding yet, wait until it does */
  if (mysql_struct && (mysql_ping(mysql_struct) != 0) ) {
      retries = 0;
      while ((retries < MYSQL_RETRY_TIMES) && mysql_struct && (mysql_ping(mysql_struct) != 0) ) {
         nanosleep((struct timespec[]){{0, 900000000}}, NULL);
         retries++;
      }
  }
  if (!mysql_struct || (mysql_ping(mysql_struct) != 0)) {
    notify(player, "No SQL database connection.");
    if (buff)
      safe_str("#-1", buff, bp);
    sql_shutdown(player);
    return -1;
  }

  if (!q_string || !*q_string)
    return 0;
  
  /* Send the query. */
  
  alarm_msec(5);
  s_qstr = alloc_lbuf("tmp_q_string");
  memset(s_qstr, '\0', LBUF_SIZE);
  strncpy(s_qstr, q_string, LBUF_SIZE - 2);
  got_rows = mysql_real_query(mysql_struct, s_qstr, strlen(s_qstr));
  if ( mudstate.alarm_triggered ) {
     notify(player, "The SQL engine forced a failure on a timeout.");
     sql_shutdown(player);
     mudstate.alarm_triggered = 0;
     alarm_msec(next_timer());
     free_lbuf(s_qstr);
     return 0;
  }
  free_lbuf(s_qstr);
  mudstate.alarm_triggered = 0;
  alarm_msec(next_timer());


  if ((got_rows) && (mysql_errno(mysql_struct) == CR_SERVER_GONE_ERROR)) {
    
    /* We got this error because the server died unexpectedly
     * and it shouldn't have. Try repeatedly to reconnect before
     * giving up and failing. This induces a few seconds of lag,
     * depending on number of retries; we put in the sleep() here
     * to see if waiting a little bit helps.
     */
    
    retries = 0;
    sql_shutdown(player);
    
    while ((retries < MYSQL_RETRY_TIMES) && (!mysql_struct)) {
      nanosleep((struct timespec[]){{0, 900000000}}, NULL);
      sql_init(player);
      retries++;
    }
    
    if (mysql_struct) {
      alarm_msec(5);
      s_qstr = alloc_lbuf("tmp_q_string");
      memset(s_qstr, '\0', LBUF_SIZE);
      strncpy(s_qstr, q_string, LBUF_SIZE - 2);
      got_rows = mysql_real_query(mysql_struct, s_qstr, strlen(s_qstr));
      if ( mudstate.alarm_triggered ) {
         notify(player, "The SQL engine forced a failure on a timeout.");
         sql_shutdown(player);
         mudstate.alarm_triggered = 0;
         alarm_msec(next_timer());
         free_lbuf(s_qstr);
         return 0;
      }
      free_lbuf(s_qstr);
      mudstate.alarm_triggered = 0;
      alarm_msec(next_timer());
    }
  }
  if (got_rows) {
    notify(player, mysql_error(mysql_struct));
    if (buff)
      safe_str("#-1", buff, bp);
    return -1;
  }
  
  /* A number of affected rows greater than 0 means it wasnt a SELECT */
  
  tprp_buff = tpr_buff = alloc_lbuf("sql_query");

  num_rows = mysql_affected_rows(mysql_struct);
  if (num_rows > 0) {
    tprp_buff = tpr_buff;
    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "SQL query touched %d %s.",
			   num_rows, (num_rows == 1) ? "row" : "rows"));
    free_lbuf(tpr_buff);
    return 0;
  } else if (num_rows == 0) {
    free_lbuf(tpr_buff);
    return 0;
  }
  
  /* Check to make sure we got rows back. */
  
  qres = mysql_store_result(mysql_struct);
  got_rows = mysql_num_rows(qres);
  if (got_rows == 0) {
    mysql_free_result(qres);
    free_lbuf(tpr_buff);
    return 0;
  }

  /* Construct properly-delimited data. */
  
  if (buff) {
    for (i = 0; i < got_rows; i++) {
      if (i > 0) {
        if ( row_delim != '\0' ) {
	   print_sep(row_delim, buff, bp);
        } else {
	   print_sep(' ', buff, bp);
        }
      }
      row_p = mysql_fetch_row(qres);
      if (row_p) {
	got_fields = mysql_num_fields(qres);
	for (j = 0; j < got_fields; j++) {
	  if (j > 0) {
             if ( field_delim != '\0' ) {
	       print_sep(field_delim, buff, bp);
             } else {
	       print_sep(' ', buff, bp);
             }
	  }
	  if (row_p[j] && *row_p[j]) {
	    safe_str(row_p[j], buff, bp);
          } else if ( !row_p[j] ) {
            break;
          }
	}
      }
    }
  } else {
    for (i = 0; i < got_rows; i++) {
      row_p = mysql_fetch_row(qres);
      if (row_p) {
	got_fields = mysql_num_fields(qres);
	for (j = 0; j < got_fields; j++) {
          tprp_buff = tpr_buff;
	  if (row_p[j] && *row_p[j]) {
	    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Row %d, Field %d: %.3900s",
				   i + 1, j + 1, row_p[j]));
          } else if ( !row_p[j] ) {
            break;
	  } else {
	    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Row %d, Field %d: NULL", i + 1, j + 1));
	  }
	}
      } else {
        tprp_buff = tpr_buff;
	notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Row %d: NULL", i + 1));
      }
    }
  }

  numRowsRetrieved += got_rows;
  queryCount++;

  mysql_free_result(qres);
  free_lbuf(tpr_buff);
  return 0;
}


