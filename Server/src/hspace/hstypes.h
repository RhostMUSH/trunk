#ifndef __HSTYPES_INCLUDED__
#define __HSTYPES_INCLUDED__

extern "C" {
#include "autoconf.h"
#include "db.h"
#include "flags.h"

};

typedef int BOOL;
typedef unsigned int UINT;
//typedef int dbref;

#define TRUE	1
#define FALSE	0

#ifdef RHOSTMUSH
#define THING_HSPACE_SIM	MARKER0
#define THING_HSPACE_CONSOLE	MARKER1
#define THING_HSPACE_TERRITORY	MARKER2
#define THING_HSPACE_OBJECT	MARKER3
#define ROOM_HSPACE_LANDINGLOC	MARKER4
#define ROOM_HSPACE_UNIVERSE	MARKER5
#define THING_HSPACE_C_LOCKED	MARKER6
#define EXIT_HSPACE_HATCH       MARKER7
#define THING_HSPACE_COMM	MARKER8
#define PLAYER_HSPACE_ADMIN	MARKER9

// Ick, Penn-style flag names. Let's give them Tiny-style aliases
#define HS_SIM              THING_HSPACE_SIM
#define HS_CONSOLE          THING_HSPACE_CONSOLE
#define HS_TERRITYRY 	    THING_HSPACE_TERRITORY
#define HS_OBJECT	    THING_HSPACE_OBJECT
#define HS_LANDINGLOC	    ROOM_HSPACE_LANDINGLOC
#define HS_UNIVERSE	    ROOM_HSPACE_UNIVERSE
#define HS_C_LOCKED         THING_HSPACE_C_LOCKED
#define HS_HATCH            EXIT_HSPACE_HATCH
#define HS_COMM	            THING_HSPACE_COMM
#define HS_ADMIN	    PLAYER_HSPACE_ADMIN
#endif

#ifdef WIN32
#include <string.h>
#define strcasecmp stricmp
#endif

#endif // __HSTYPES_INCLUDED__
