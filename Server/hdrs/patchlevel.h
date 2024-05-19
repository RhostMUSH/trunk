/* patchlevel.h - version information */
#ifndef _M_PATCHLEVEL_H_
#define _M_PATCHLEVEL_H_

#include "copyright.h"

#define MUSH_VERSION            "4.2.2-180"         /* Base version number*/

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
<<<<<<< HEAD
#define	MUSH_RELEASE_DATE	"05/16/2024"	/* Source release date       */
=======
#define	MUSH_RELEASE_DATE	"05/19/2024"	/* Source release date       */
>>>>>>> 89464e54 (home now honors the BLIND flag)

/* Define if an ALPHA release */
/* #define ALPHA 0 */

/* Define if a BETA release   */
/* #define BETA 1 */

#endif
