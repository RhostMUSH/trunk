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
#ifdef BETA
#define INDEVEL_TAG "Beta"
#endif
#ifdef ALPHA
#define INDEVEL_TAG "Alpha"
#endif

#ifdef INDEVEL_TAG
#if PATCHLEVEL > 0
	sprintf(mudstate.version, "RhostMUSH %.10s version %.30s %.10s patchlevel %d%.10s #%.10s%.20s",
		INDEVEL_TAG, MUSH_VERSION, EXT_MUSH_VER, PATCHLEVEL, PATCHLEVELEXT, MUSH_BUILD_NUM, mudconf.vercustomstr);
	sprintf(mudstate.short_ver, "RhostMUSH %.10s %.30s %.10s.p%d%.10s%.20s",
		INDEVEL_TAG, MUSH_VERSION, EXT_MUSH_VER, PATCHLEVEL, PATCHLEVELEXT, mudconf.vercustomstr);
#else
	sprintf(mudstate.version, "RhostMUSH %.10s version %.30s %.10s #%.10s%.20s",
		INDEVEL_TAG, MUSH_VERSION, EXT_MUSH_VER, MUSH_BUILD_NUM, mudconf.vercustomstr);
	sprintf(mudstate.short_ver, "RhostMUSH %.10s %.30s %.10s%.20s", INDEVEL_TAG,
		MUSH_VERSION, EXT_MUSH_VER, mudconf.vercustomstr);
#endif	/* PATCHLEVEL */
#else	/* not BETA or ALPHA */
#if PATCHLEVEL > 0
	sprintf(mudstate.version, "RhostMUSH version %.30s %.10s patchlevel %d%.10s #%.10s [%.30s]%.20s",
		MUSH_VERSION, EXT_MUSH_VER, PATCHLEVEL, PATCHLEVELEXT, MUSH_BUILD_NUM, MUSH_RELEASE_DATE, mudconf.vercustomstr);
	sprintf(mudstate.short_ver, "RhostMUSH %.30s %.10s.p%d%.10s%.20s",
		MUSH_VERSION, EXT_MUSH_VER, PATCHLEVEL, PATCHLEVELEXT, mudconf.vercustomstr);
#else
	sprintf(mudstate.version, "RhostMUSH version %.30s %.10s #%.10s [%.30s]%.20s",
		MUSH_VERSION, EXT_MUSH_VER, MUSH_BUILD_NUM, MUSH_RELEASE_DATE, mudconf.vercustomstr);
	sprintf(mudstate.short_ver, "RhostMUSH %.30s%.10s%.20s",
		MUSH_VERSION, EXT_MUSH_VER, mudconf.vercustomstr);
#endif	/* PATCHLEVEL */
#endif	/* BETA */
	STARTLOG(LOG_ALWAYS,"INI","START")
		log_text((char *)"Starting: ");
		log_text(mudstate.version);
	ENDLOG
	STARTLOG(LOG_ALWAYS,"INI","START")
		log_text((char *)"Build date: ");
		log_text((char *)MUSH_BUILD_DATE);
	ENDLOG	
}
