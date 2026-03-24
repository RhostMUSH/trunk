/* version.c - version information */

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "mudconf.h"
#include "alloc.h"
#include "externs.h"
#include "patchlevel.h"

/* Converted mush 2 to muse code */

/*
   2.0
   All known bugs fixed with disk-based.  Played with gdbm, it
   sucked.  Now using bsd 4.4 hash stuff.
*/

/* 1.12
   All known bugs fixed after several days of debugging 1.10/1.11.
   Much string-handling braindeath patched, but needs a big overhaul,
   really.   GAC 2/10/91
*/

/* 1.11
   Fixes for 1.10.  (@name didn't call do_name, etc.)
   Added dexamine (debugging examine, dumps the struct, lots of info.)
*/

/* 1.10
  Finally got db2newdb working well enough to run from the big (30000
  object) db with ATR_KEY and ATR_NAME defined.   GAC 2/3/91
*/

/* TinyCWRU version.c file.  Add a comment here any time you've made a
   big enough revision to increment the TinyCWRU version #.
*/

void do_version (dbref player, dbref cause, int extra)
{
char	*buff;

	notify(player, mudstate.version);
	buff=alloc_mbuf("do_version");
	sprintf(buff, "Build date: %.150s", MUSH_BUILD_DATE);
	notify(player, buff);
	free_mbuf(buff);
}

void NDECL(init_version)
{
    if(strcmp(VERSION_EXT,"")) {
	sprintf(mudstate.version, "RhostMUSH %.2s.%.2s.%.2s-%.8s [%.30s] #%.10s",
		MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, VERSION_EXT, MUSH_RELEASE_DATE, MUSH_BUILD_NUM);
	sprintf(mudstate.short_ver, "RhostMUSH %.2s.%.2s.%.2s-%.8s",
		MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, VERSION_EXT);
    } else {
	sprintf(mudstate.version, "RhostMUSH %.2s.%.2s.%.2s [%.30s] #%.10s",
		MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, MUSH_RELEASE_DATE, MUSH_BUILD_NUM);
	sprintf(mudstate.short_ver, "RhostMUSH %.2s.%.2s.%.2s",
		MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
    }
	STARTLOG(LOG_ALWAYS,"INI","START")
		log_text((char *)"Starting: ");
		log_text(mudstate.version);
	ENDLOG
	STARTLOG(LOG_ALWAYS,"INI","START")
		log_text((char *)"Build date: ");
		log_text((char *)MUSH_BUILD_DATE);
	ENDLOG	
}
