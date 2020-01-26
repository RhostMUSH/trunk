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

#define HS_TYPE_ATR        "_HSTYPE"

#ifdef RHOSTMUSH
#define THING_HSPACE_SIM	"SIMULATOR"
#define THING_HSPACE_CONSOLE	"CONSOLE"
#define THING_HSPACE_TERRITORY	"TERRITORY"
#define THING_HSPACE_OBJECT	"OBJECT"
#define ROOM_HSPACE_LANDINGLOC	"LANDINGLOC"
#define ROOM_HSPACE_UNIVERSE	"UNIVERSE"
#define THING_HSPACE_C_LOCKED	"C_LOCKED"
#define EXIT_HSPACE_HATCH       "HATCH"
#define THING_HSPACE_COMM	"COMM"
#define PLAYER_HSPACE_ADMIN	"ADMIN"
#endif

#ifdef WIN32
#include <string.h>
#define strcasecmp stricmp
#endif

#endif // __HSTYPES_INCLUDED__
