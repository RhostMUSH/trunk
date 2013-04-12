/* patchlevel.h - version information */
#ifndef _M_PATCHLEVEL_H_
#define _M_PATCHLEVEL_H_

#include "copyright.h"

#define MUSH_VERSION            "3.9.4"         /* Base version number*/

#if defined(ZENTY_ANSI) && defined(REALITY_LEVELS)
#define EXT_MUSH_VER "RL(A)"
#elif defined(ZENTY_ANSI)
#define EXT_MUSH_VER "(A)"
#elif defined(REALITY_LEVELS)
#define EXT_MUSH_VER "RL"
#else
#define EXT_MUSH_VER ""
#endif

#define PATCHLEVEL		0		/* Patch sequence number     */
#define PATCHLEVELEXT		""
#define	MUSH_RELEASE_DATE	"09/20/2012"	/* Source release date       */

/* Define if an ALPHA release */
#define ALPHA 1   		

/* Define if a BETA release   */
/* #define BETA 1 */

#endif
