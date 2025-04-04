/* log.c - logging routines */

#ifdef SOLARIS
/* Borked header files in Solaris - No declarations */
char *index(const char *, int);
#endif

#include <stdio.h>
#include "copyright.h"
#include "autoconf.h"

#include <sys/types.h>

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "flags.h"
#include "alloc.h"
#include "htab.h"
#include "rhost_ansi.h"

#ifndef STANDALONE

NAMETAB logdata_nametab[] =
{
    {(char *) "flags", 1, 0, 0, LOGOPT_FLAGS},
    {(char *) "location", 1, 0, 0, LOGOPT_LOC},
    {(char *) "owner", 1, 0, 0, LOGOPT_OWNER},
    {(char *) "timestamp", 1, 0, 0, LOGOPT_TIMESTAMP},
    {NULL, 0, 0, 0, 0}};

NAMETAB logoptions_nametab[] =
{
    {(char *) "accounting", 2, 0, 0, LOG_ACCOUNTING},
    {(char *) "all_commands", 2, 0, 0, LOG_ALLCOMMANDS},
    {(char *) "bad_commands", 2, 0, 0, LOG_BADCOMMANDS},
    {(char *) "buffer_alloc", 3, 0, 0, LOG_ALLOCATE},
    {(char *) "bugs", 3, 0, 0, LOG_BUGS},
    {(char *) "checkpoints", 2, 0, 0, LOG_DBSAVES},
    {(char *) "config_changes", 4, 0, 0, LOG_CONFIGMODS},
    {(char *) "create", 2, 0, 0, LOG_PCREATES},
    {(char *) "door", 2, 0, 0, LOG_DOOR},
    {(char *) "killing", 1, 0, 0, LOG_KILLS},
    {(char *) "logins", 1, 0, 0, LOG_LOGIN},
    {(char *) "network", 1, 0, 0, LOG_NET},
    {(char *) "problems", 1, 0, 0, LOG_PROBLEMS},
    {(char *) "security", 2, 0, 0, LOG_SECURITY},
    {(char *) "shouts", 2, 0, 0, LOG_SHOUTS},
    {(char *) "startup", 2, 0, 0, LOG_STARTUP},
    {(char *) "wizard", 2, 0, 0, LOG_WIZARD},
    {(char *) "mail", 1, 0, 0, LOG_MAIL},
    {(char *) "suspect", 2, 0, 0, LOG_SUSPECT},
    {(char *) "wanderer", 2, 0, 0, LOG_WANDERER},
    {(char *) "guest", 2, 0, 0, LOG_GUEST},
    {(char *) "god", 2, 0, 0, LOG_GHOD},
/*  This commented out as it gives chance to log passwords! */
/*  {(char *) "connect", 4, 0, 0, LOG_CONNECT}, */
    {NULL, 0, 0, 0, 0}};

#endif

void
close_logfile( void )
{
#ifndef STANDALONE
   struct tm *tp;
   time_t now;

   time((time_t *) (&now));
   tp = localtime((time_t *) (&now));
   sprintf(mudstate.buffer, "./oldlogs/%.200s.%02d%02d%02d%02d%02d%02d",
           mudconf.logdb_name,
           (tp->tm_year % 100), tp->tm_mon + 1,
           tp->tm_mday, tp->tm_hour,
           tp->tm_min, tp->tm_sec);

   /* Backup the old filename */
#endif
   if ( mudstate.f_logfile_name )
      fclose(mudstate.f_logfile_name);
   mudstate.f_logfile_name = NULL;

#ifndef STANDALONE
   if ( ! mudstate.reboot_flag ) {
      (void) rename(mudconf.logdb_name, mudstate.buffer);
   }
#endif
}

void
init_logfile( void )
{
#ifndef STANDALONE
   struct tm *tp;
   time_t now;

   time((time_t *) (&now));
   tp = localtime((time_t *) (&now));
   sprintf(mudstate.buffer, "./oldlogs/%.200s.%02d%02d%02d%02d%02d%02d",
           mudconf.logdb_name,
           (tp->tm_year % 100), tp->tm_mon + 1,
           tp->tm_mday, tp->tm_hour,
           tp->tm_min, tp->tm_sec);

   /* Backup the old filename */
   if ( ! mudstate.log_chk_reboot ) {
      (void) rename(mudconf.logdb_name, mudstate.buffer);
   }

   if ((mudconf.log_info & LOGOPT_TIMESTAMP) != 0) {
      time((time_t *) (&now));
      tp = localtime((time_t *) (&now));
      sprintf(mudstate.buffer, "%02d%d%d%d%d.%d%d%d%d%d%d ",
              (tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
              (((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
              (tp->tm_mday % 10),
              (tp->tm_hour / 10), (tp->tm_hour % 10),
              (tp->tm_min / 10), (tp->tm_min % 10),
              (tp->tm_sec / 10), (tp->tm_sec % 10));
   } else {
      mudstate.buffer[0] = '\0';
   }
   if ( (mudstate.f_logfile_name = fopen(mudconf.logdb_name, "a")) != NULL ) {
      fprintf(stderr, "%s%s: Game log file opened as %s.\n", 
              mudstate.buffer, mudconf.mud_name, mudconf.logdb_name);
   } else {
      mudstate.f_logfile_name = NULL;
      fprintf(stderr, "%s%s: Game log could not be opened.  Will write log files to stderr (this file)\n",
              mudstate.buffer, mudconf.mud_name);
   }
#endif
}
/* ---------------------------------------------------------------------------
 * start_log: see if it is OK to log something, and if so, start writing the
 * log entry.
 */

int 
start_log(const char *primary, const char *secondary)
{
    struct tm *tp;
    time_t now;
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    mudstate.logging++;
    switch (mudstate.logging) {
    case 1:
    case 2:

	/* Format the timestamp */

	if ((mudconf.log_info & LOGOPT_TIMESTAMP) != 0) {
	    time((time_t *) (&now));
	    tp = localtime((time_t *) (&now));
	    sprintf(mudstate.buffer, "%02d%d%d%d%d.%d%d%d%d%d%d ",
		    (tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
		    (((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
		    (tp->tm_mday % 10),
		    (tp->tm_hour / 10), (tp->tm_hour % 10),
		    (tp->tm_min / 10), (tp->tm_min % 10),
		    (tp->tm_sec / 10), (tp->tm_sec % 10));
	} else {
	    mudstate.buffer[0] = '\0';
	}
#ifndef STANDALONE
	/* Write the header to the log */

	if (secondary && *secondary)
	    fprintf(f_foo, "%s%s %3s/%-5s: ", mudstate.buffer,
		    mudconf.mud_name, primary, secondary);
	else
	    fprintf(f_foo, "%s%s %-9s: ", mudstate.buffer,
		    mudconf.mud_name, primary);
#endif
	/* If a recursive call, log it and return indicating no log */

	if (mudstate.logging == 1)
	    return 1;
	fprintf(f_foo, "Recursive logging request.\r\n");
    default:
	mudstate.logging--;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * end_log: Finish up writing a log entry
 */

void 
end_log(void)
{
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    fprintf(f_foo, "\n");
    fflush(f_foo);
    mudstate.logging--;
}

/* ---------------------------------------------------------------------------
 * log_perror: Write perror message to the log
 */

void 
log_perror(const char *primary, const char *secondary,
	   const char *extra, const char *failing_object)
{
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    start_log(primary, secondary);
    if (extra && *extra) {
	log_text((char *) "(");
	log_text((char *) extra);
	log_text((char *) ") ");
    }
    perror((char *) failing_object);
    fflush(f_foo);
    mudstate.logging--;
}

/* ---------------------------------------------------------------------------
 * log_text, log_number: Write text or number to the log file.
 */

void 
log_text(char *text)
{
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    /* Write only 3900 characters.  Notify if it was cut off */
    if ( strlen(text) > 3900 )
       if ( index(text,ESC_CHAR) )
          fprintf(f_foo, "%.3900s%s [overflow cut]", text, ANSI_NORMAL);
       else
          fprintf(f_foo, "%.3900s [overflow cut]", text);
    else
       fprintf(f_foo, "%.3900s", text);
}

void 
log_number(int num)
{
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    fprintf(f_foo, "%d", num);
}

void 
log_unsigned(int num)
{
    FILE *f_foo;

    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

    fprintf(f_foo, "%u", (unsigned int)num);
}


/* ---------------------------------------------------------------------------
 * log_name: write the name, db number, and flags of an object to the log.
 * If the object does not own itself, append the name, db number, and flags
 * of the owner.
 */

void 
log_name(dbref target)
{
    FILE *f_foo;
#ifndef STANDALONE
    char *tp;

    tp = NULL;
#endif
    if ( mudstate.f_logfile_name )
       f_foo = mudstate.f_logfile_name;
    else
       f_foo = stderr;

#ifndef STANDALONE
    if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
	tp = unparse_object((dbref) GOD, target, 0);
    else
	tp = unparse_object_numonly(target);
    fprintf(f_foo, "%s", tp);
    free_lbuf(tp);
    if (Good_obj(target) && ((mudconf.log_info & LOGOPT_OWNER) != 0) &&
	(target != Owner(target))) {
	if ((mudconf.log_info & LOGOPT_FLAGS) != 0)
	    tp = unparse_object((dbref) GOD, Owner(target), 0);
	else
	    tp = unparse_object_numonly(Owner(target));
	fprintf(f_foo, "[%s]", tp);
	free_lbuf(tp);
    }
#else
    if (Good_obj(target))
      fprintf(f_foo, "%.3900s(#%d)", Name(target), target);
    else
      fprintf(f_foo, "(#%d)", target);
#endif
    return;
}

/* ---------------------------------------------------------------------------
 * log_name_and_loc: Log both the name and location of an object
 */

void 
log_name_and_loc(dbref player)
{
    log_name(player);
    if ((mudconf.log_info & LOGOPT_LOC) && Has_location(player)) {
	log_text((char *) " in ");
	log_name(Location(player));
    }
    return;
}

char *
OBJTYP(dbref thing)
{
    if (!Good_obj(thing)) {
	return (char *) "??OUT-OF-RANGE??";
    }
    switch (Typeof(thing)) {
    case TYPE_PLAYER:
	return (char *) "PLAYER";
    case TYPE_THING:
	return (char *) "THING";
    case TYPE_ROOM:
	return (char *) "ROOM";
    case TYPE_EXIT:
	return (char *) "EXIT";
    default:
	return (char *) "??ILLEGAL??";
    }
    /* NOTREACHED */
    return NULL;
}

void 
log_type_and_name(dbref thing)
{
    char nbuf[16];

    log_text(OBJTYP(thing));
    sprintf(nbuf, " #%d(", thing);
    log_text(nbuf);
    if (Good_obj(thing))
	log_text(Name(thing));
    log_text((char *) ")");
    return;
}

void 
log_type_and_num(dbref thing)
{
    char nbuf[16];

    log_text(OBJTYP(thing));
    sprintf(nbuf, " #%d", thing);
    log_text(nbuf);
    return;
}
