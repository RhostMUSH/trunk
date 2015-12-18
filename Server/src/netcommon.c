/* netcommon.c - Network utility routines */

/* This file contains routines used by the networking code that do not
 * depend on the implementation of the networking code.  The network-specific
 * portions of the descriptor data structure are not used.
 */

#ifdef SOLARIS
/* Solaris has borked header file declarations */
char *index(const char *, int);
#endif
#include <utmpx.h>

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "mudconf.h"
#include "file_c.h"
#include "interface.h"
#include "command.h"
#include "externs.h"
#include "rwho_clilib.h"
#include "alloc.h"
#include "attrs.h"
#define INCLUDE_ASCII_TABLE
#include "rhost_ansi.h"
#include "local.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */
#include "door.h"

#include "debug.h"
#define FILENUM NETCOMMON_C

extern int ndescriptors;

/* Logged out command table definitions */

#define	CMD_QUIT	1
#define	CMD_WHO		2
#define	CMD_DOING	3
#define	CMD_RWHO	4
#define	CMD_PREFIX	5
#define	CMD_SUFFIX	6
#define	CMD_LOGOUT	7
#define	CMD_SESSION	8
#define	CMD_GET         9
#define CMD_INFO        10

#define	CMD_MASK	0xff
#define	CMD_NOxFIX	0x100

#define INFO_VERSION    "1"

extern int FDECL(process_output, (DESC * d));
extern CF_HAND(cf_site);
static void desc_addhash(DESC * d);
extern int FDECL(lookup, (char *, char *, int, int *));
static void set_userstring(char **, const char *);
extern const char *addrout(struct in_addr);


/* for aconnect: player = room, target = connecting player */
/*
#define CANSEE(player, target) \
   (!Cloak(target) || Wizard(player)) \
   && (!(Dark(target) && mudconf.player_dark) 
   && (!SCloak(target) || Immortal(player))
*/
#define CANSEE(player, target) \
   (!Cloak(target) || Wizard(player)) && \
   (!(Dark(target) && mudconf.player_dark) || Wizard(player)) && \
   (!SCloak(target) || Immortal(player))
    
#ifdef LOCAL_RWHO_SERVER
void FDECL(dump_rusers, (DESC * call_by));

#endif

extern char *
strip_ansi(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #98 */

    while (p && *p) {
	if (*p == ESC_CHAR) {
	    /* Start of ANSI code. Skip to end. */
	    while (*p && !isalpha((int)*p))
		p++;
	    if (*p)
		p++;
	} else
	    *q++ = *p++;
    }
    *q = '\0';
    RETURN(buf); /* #98 */
}

#ifdef ZENTY_ANSI
extern char *
strip_safe_accents(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #99 */

    while (p && *p) {
        if(((*p == '\\') || (*p == '%')) &&
           ((*(p+1) == '\\') || (*(p+1) == '%')))
            p++;

        if((*p == '%') && (*(p+1) == 'f') && isprint(*(p+2)))
            p+=3;
        else
            *q++ = *p++;
    }
    *q = '\0';
    RETURN(buf); /* #99 */
}

extern char *
strip_safe_ansi(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #99 */

    while (p && *p) {
        if(((*p == '\\') || (*p == '%')) &&
           ((*(p+1) == '\\') || (*(p+1) == '%')))
            p++;

        if( (*p == '%') && ((*(p+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                        || (*(p+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                        || (*(p+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(p+2)] ) {
              p+=3;
              continue;
           }
           if ( (p+2) && *(p+2) == '0' && ((p+3) && ((*(p+3) == 'x') || (*(p+3) == 'X'))) &&
                (p+4) && *(p+4) && (p+5) && *(p+5) && isxdigit(*(p+4)) && isxdigit(*(p+5)) ) {
              p+=6; // strip safe XTERM ansi
              continue;
           }
           *q++ = *p++;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
    RETURN(buf); /* #99 */
}


extern char *
strip_all_special(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #100 */

    while (p && *p) {
        if ( (*p == '%') && ((*(p+1) == SAFE_CHR) 
#ifdef SAFE_CHR2
                         || (*(p+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                         || (*(p+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(p+2)]) {
              p+=3; // strip safe ansi
              continue;
           }
           if ( *(p+2) == '0' && ((*(p+3) == 'x') || (*(p+3) == 'X')) &&
                *(p+4) && *(p+5) && isxdigit(*(p+4)) && isxdigit(*(p+5)) ) {
              p+=6; // strip safe XTERM ansi
              continue;
           }
           *q++ = *p++;
        } else if (*p == ESC_CHAR) {
            // Strip normal ansi
            while (*p && !isalpha((int)*p))
                p++;
            if (*p)
                p++;
        } else if ( (*p == '%') && (*(p+1) == 'f') && isprint(*(p+2)) ) {
           p+=3;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
    RETURN(buf); /* #100 */
}
extern char *
strip_all_ansi(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #100 */

    while (p && *p) {
        if( (*p == '%') && ((*(p+1) == SAFE_CHR) 
#ifdef SAFE_CHR2
                        || (*(p+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                        || (*(p+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(p+2)] ) {
              p+=3; // strip safe ansi
              continue;
           }
           if ( (p+2) && *(p+2) == '0' && ((p+3) && ((*(p+3) == 'x') || (*(p+3) == 'X'))) &&
                (p+4) && *(p+4) && (p+5) && *(p+5) && isxdigit(*(p+4)) && isxdigit(*(p+5)) ) {
              p+=6; // strip safe XTERM ansi
              continue;
           }
           *q++ = *p++;
        } else if (*p == ESC_CHAR) {
            // Strip normal ansi
            while (*p && !isalpha((int)*p))
                p++;
            if (*p)
                p++;
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
    RETURN(buf); /* #100 */
}
#endif


extern char *
strip_nonprint(const char *raw)
{
   static char buf[LBUF_SIZE];
   char *p = (char *) raw;
   char *q = buf;

   DPUSH; /* #101 */

   while (p && *p) {
      if (!isprint((int)*p)) {
         p++;
      } else {
         *q++ = *p++;
      }
   }
   *q = '\0';

   RETURN(buf); /* #101 */
}

static char *
strip_ansi_flash(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;
   
    DPUSH; /* #102 */
    while (p && *p) {
	if (*p != ESC_CHAR) {
	    *q++ = *p++;
	}
	else if (*(p + 1) && *(p + 2) && *(p + 3) && (*(p + 1) == '[') &&
	  (*(p + 2) == '5') && (*(p + 3) == 'm')) {
	  p += 4;
	}
	else {
	  *q++ = *p++;
	}
    }
    *q = '\0';
    RETURN(buf); /* #102 */
}

static char *
strip_ansi_underline(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;
   
    DPUSH; /* #103 */
    while (p && *p) {
	if (*p != ESC_CHAR) {
	    *q++ = *p++;
	}
	else if (*(p + 1) && *(p + 2) && *(p + 3) && (*(p + 1) == '[') &&
	  (*(p + 2) == '4') && (*(p + 3) == 'm')) {
	  p += 4;
	}
	else {
	  *q++ = *p++;
	}
    }
    *q = '\0';
    RETURN(buf); /* #103 */
}

static char *
strip_ansi_color(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #104 */

    while (p && *p) {
	if (*p != ESC_CHAR) {
	    *q++ = *p++;
	} else {
	   /* We've got an ANSI code here */
	   if (*(p + 1) && *(p + 2) && *(p + 3) && (*(p + 1) == '[') && (*(p + 3) == 'm')) {
	       /* This code is ok to pass */
	       *q++ = *p++;
	   } else {
	      /* Skip to end. */
	      while (*p && !isalpha((int)*p))
                 p++;
                 if (*p)
                    p++;
           }
        }
    }
    *q = '\0';
    RETURN(buf); /* #104 */
}

char *ansi_translate_bg[257]={
   ANSI_BBLACK, ANSI_BRED, ANSI_BGREEN, ANSI_BYELLOW, ANSI_BBLUE, ANSI_BMAGENTA, ANSI_BCYAN, ANSI_BWHITE,
   ANSI_BBLACK, ANSI_BRED, ANSI_BGREEN, ANSI_BYELLOW, ANSI_BBLUE, ANSI_BMAGENTA, ANSI_BCYAN, ANSI_BWHITE,
   ANSI_BBLACK, ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE, ANSI_BGREEN, ANSI_BCYAN,
   ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BBLUE,
   ANSI_BBLUE, ANSI_BBLUE, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BBLUE, ANSI_BBLUE,
   ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BGREEN, ANSI_BGREEN,
   ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA,
   ANSI_BBLUE, ANSI_BBLUE, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BBLUE, ANSI_BBLUE, ANSI_BBLUE,
   ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BBLUE, ANSI_BGREEN, ANSI_BGREEN,
   ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN,
   ANSI_BCYAN, ANSI_BCYAN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN,
   ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BRED, ANSI_BRED,
   ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BGREEN, ANSI_BCYAN,
   ANSI_BBLUE, ANSI_BBLUE, ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BCYAN,
   ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BGREEN, ANSI_BCYAN, ANSI_BCYAN, ANSI_BGREEN, ANSI_BGREEN,
   ANSI_BGREEN, ANSI_BGREEN, ANSI_BWHITE, ANSI_BWHITE, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA,
   ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BRED, ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA,
   ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW,
   ANSI_BYELLOW, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BGREEN, ANSI_BWHITE,
   ANSI_BWHITE, ANSI_BWHITE, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BWHITE, ANSI_BWHITE,
   ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BRED, ANSI_BRED,
   ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BMAGENTA,
   ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA,
   ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BYELLOW, ANSI_BYELLOW,
   ANSI_BYELLOW, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BRED, ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA,
   ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BRED, ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA,
   ANSI_BRED, ANSI_BRED, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW,
   ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BWHITE,
   ANSI_BWHITE, ANSI_BWHITE, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BWHITE, ANSI_BWHITE,
   ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK, ANSI_BBLACK,
   ANSI_BBLACK, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE,
   ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, ANSI_BWHITE, (char *)NULL,
};

char *ansi_translate_fg[257]={
   ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_WHITE,
   ANSI_BLACK_H, ANSI_RED_H, ANSI_GREEN_H, ANSI_YELLOW_H, ANSI_BLUE_H, ANSI_MAGENTA_H, ANSI_CYAN_H, ANSI_WHITE_H,
   ANSI_BLACK, ANSI_BLUE, ANSI_BLUE, ANSI_BLUE, ANSI_BLUE_H, ANSI_BLUE_H, ANSI_GREEN, ANSI_CYAN,
   ANSI_BLUE, ANSI_BLUE_H, ANSI_BLUE_H, ANSI_BLUE_H, ANSI_GREEN_H, ANSI_GREEN, ANSI_CYAN, ANSI_BLUE_H,
   ANSI_BLUE_H, ANSI_BLUE_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_BLUE_H, ANSI_BLUE_H,
   ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_GREEN_H, ANSI_GREEN_H,
   ANSI_GREEN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_RED, ANSI_MAGENTA, ANSI_MAGENTA, ANSI_MAGENTA_H,
   ANSI_BLUE_H, ANSI_BLUE_H, ANSI_GREEN, ANSI_GREEN, ANSI_CYAN, ANSI_BLUE_H, ANSI_BLUE_H, ANSI_BLUE_H,
   ANSI_GREEN, ANSI_GREEN, ANSI_GREEN, ANSI_CYAN, ANSI_CYAN, ANSI_BLUE_H, ANSI_GREEN_H, ANSI_GREEN,
   ANSI_GREEN, ANSI_CYAN, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_CYAN_H,
   ANSI_CYAN_H, ANSI_CYAN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_CYAN_H, ANSI_CYAN_H,
   ANSI_RED, ANSI_RED, ANSI_MAGENTA, ANSI_MAGENTA, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_RED, ANSI_RED,
   ANSI_MAGENTA, ANSI_MAGENTA, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW, ANSI_YELLOW, ANSI_GREEN, ANSI_CYAN,
   ANSI_BLUE_H, ANSI_BLUE_H, ANSI_GREEN, ANSI_GREEN, ANSI_GREEN, ANSI_CYAN, ANSI_CYAN_H, ANSI_CYAN_H,
   ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_GREEN_H, ANSI_CYAN_H, ANSI_CYAN_H, ANSI_GREEN_H, ANSI_GREEN_H,
   ANSI_GREEN_H, ANSI_GREEN_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_RED, ANSI_MAGENTA, ANSI_MAGENTA, ANSI_MAGENTA_H,
   ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_RED_H, ANSI_RED_H, ANSI_RED_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H,
   ANSI_YELLOW, ANSI_YELLOW, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW, ANSI_YELLOW,
   ANSI_YELLOW, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_GREEN_H, ANSI_WHITE_H,
   ANSI_WHITE_H, ANSI_WHITE_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_WHITE_H, ANSI_WHITE_H,
   ANSI_RED, ANSI_RED_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_RED_H, ANSI_RED_H,
   ANSI_RED_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW, ANSI_YELLOW, ANSI_YELLOW, ANSI_MAGENTA_H,
   ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW, ANSI_YELLOW, ANSI_YELLOW_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H,
   ANSI_YELLOW, ANSI_YELLOW, ANSI_YELLOW_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_YELLOW_H, ANSI_YELLOW_H,
   ANSI_YELLOW_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_RED_H, ANSI_RED_H, ANSI_RED_H, ANSI_MAGENTA_H,
   ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_RED_H, ANSI_RED_H, ANSI_RED_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H,
   ANSI_RED_H, ANSI_RED_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW_H, ANSI_YELLOW_H,
   ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_WHITE_H,
   ANSI_WHITE_H, ANSI_WHITE_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_WHITE_H, ANSI_WHITE_H,
   ANSI_BLACK, ANSI_BLACK, ANSI_BLACK_H, ANSI_BLACK_H, ANSI_BLACK_H, ANSI_BLACK_H, ANSI_BLACK_H, ANSI_BLACK_H,
   ANSI_BLACK_H, ANSI_WHITE, ANSI_WHITE, ANSI_WHITE, ANSI_WHITE, ANSI_WHITE, ANSI_WHITE, ANSI_WHITE,
   ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, ANSI_WHITE_H, (char *)NULL,
};

static char *
strip_ansi_xterm(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;
    char *r;
    int i_chk, i_type, i_chk2;

    DPUSH; /* #104 */

    i_chk = i_type = 0;
    r = NULL;

    while (p && *p) {
	if (*p != ESC_CHAR) {
	    *q++ = *p++;
	} else {
	    /* We've got an ANSI code here -- verify it's XTERM codes */
            if ( (*(p+1) && (*(p+1) == '[')) && (*(p+2) && ((*(p+2) == '4') || (*(p+2) == '3'))) && (*(p+3) && (*(p+3) == '8')) ) {
               if ( *(p+2) == '3' ) {
                  i_type = 0;
               } else {
                  i_type = 1;
               }
               i_chk = 0;
	       while (*p && !isalpha((int)*p)) {
                  if ( *p == ';' )
                     i_chk++;
                  if ( i_chk == 2 ) {
                     i_chk2 = -1;
                     sscanf( p+1, "%d", &i_chk2);
                     if ( (i_chk2 >= 0) && (i_chk2 < 256) ) {
                        if ( i_type ) {
                           r = ansi_translate_bg[i_chk2];
                        } else {
                           r = ansi_translate_fg[i_chk2];
                        }
                        while ( r && *r ) {
	                   *q++ = *r++;
                        }
                     }
                     i_chk++;
                  }
		  p++;
               }
               if ( *p )
                  p++;
            } else {
	       *q++ = *p++;
            }
        }
    }
    *q = '\0';
    RETURN(buf); /* #104 */
}

extern char *
strip_returntab(const char *raw, int key)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #105 */
    while (p && *p) {
        if ((*p == '\t' && (key & 1)) || ((*p == '\n' || *p == '\r') && (key & 2)) ) {
           p++;
        } else
            *q++ = *p++;
    }
    *q = '\0';
    RETURN(buf); /* #105 */
}

/* --------------------------------------------------
 * do_snoop: turns snooping on and off on a player for an immortal
 * [Thorin - 02/95]
 */

void 
do_snoop(dbref player, dbref cause, int key, char *name, char *arg2)
{
    dbref victim;
    dbref oper;
    struct SNOOPLISTNODE *prev, *node;
    int found = 0;
    char sstat;
    DESC *d;

    DPUSH; /* #106 */

    if (!Immortal(player)) {
	notify(player, errmsg(player));
	VOIDRETURN; /* #106 */
    }

#ifdef SNOOPLOG_ONLY
    if ( !(key & SNOOP_LOG) && !(key & SNOOP_OFF) && !(key & SNOOP_STAT) ) {
       notify_quiet(player, "Unknown combination of switches.");
       VOIDRETURN; /* #106 */
    }
#endif

    if (key & SNOOP_LOG) {
	key &= ~SNOOP_LOG;
	sstat = 1;
    } else
	sstat = 0;
    if ((key == 0) && !sstat)
	key = SNOOP_ON;
    if (key != SNOOP_ON &&
	key != SNOOP_OFF &&
	key != 0 &&
	key != SNOOP_STAT) {
	notify_quiet(player, "Unknown combination of switches.");
    }
    if (key == SNOOP_STAT) {
	if (*name != '\0' || *arg2 != '\0') {
	    notify_quiet(player, "Too many arguments for given switch.");
	    VOIDRETURN; /* #106 */
	}
	found = 0;
	DESC_ITER_ALL(d) {
	    if (d->snooplist) {
		if (found == 0)
		    notify(player, "Snooped Players:");
		found++;
		notify(player, unsafe_tprintf("  %s [port %d]", Name(d->player), d->descriptor));
		for (node = d->snooplist; node; node = node->next) {
		    if (node->sfile) {
			if (node->logonly)
			    notify(player, unsafe_tprintf("    * %s (Logged Only)", Name(node->snooper)));
			else
			    notify(player, unsafe_tprintf("    * %s (Logged)", Name(node->snooper)));
		    } else
			notify(player, unsafe_tprintf("    * %s", Name(node->snooper)));
		}
	    }
	}
	if (found == 0)
	    notify(player, "There are no players beeing snooped on.");
	VOIDRETURN; /* #106 */
    }
    if (*name == '\0') {
	notify_quiet(player, "You must specify a player.");
	VOIDRETURN; /* #106 */
    }
    victim = lookup_player(player, name, 1);
    if (victim == NOTHING) {
	notify_quiet(player, unsafe_tprintf("Unknown player '%s'.", name));
	VOIDRETURN; /* #106 */
    }
    if (arg2 && *arg2 != '\0') {
	oper = lookup_player(player, arg2, 1);
	if (oper == NOTHING) {
	    notify_quiet(player, unsafe_tprintf("Unknown player '%s'.", arg2));
	    VOIDRETURN; /* #106 */
	}
    } else {
	oper = player;
    }

    if (God(victim) || Immortal(victim)) {
	notify_quiet(player, "You can't snoop on that player!");
	VOIDRETURN; /* #106 */
    }
    if (!Connected(victim)) {
	notify_quiet(player, "That player isn't connected.");
	VOIDRETURN; /* #106 */
    }
    if (!Connected(oper) && key && (key != SNOOP_OFF)) {
	notify_quiet(player, unsafe_tprintf("%s isn't connected.", Name(oper)));
	VOIDRETURN; /* #106 */
    }
    if (victim == oper) {
	notify_quiet(player, "You can't snoop on yourself.");
	VOIDRETURN; /* #106 */
    }
    if ((key == SNOOP_ON) || key == 0) {
	DESC_ITER_PLAYER(victim, d) {
	    for (node = d->snooplist; node && node->snooper != Owner(oper);
		 node = node->next) {
	    }
	    if (node != NULL) {
		if (!sstat && node->logonly) {
		    node->logonly = 0;
		    notify_quiet(player, "Snoop modified.");
		    VOIDRETURN; /* #106 */
		} else {
		    notify_quiet(player, 
                                 unsafe_tprintf("%s is already snooped by %s.", 
                                 Name(victim), Name(d->snooplist->snooper)));
		    VOIDRETURN; /* #106 */
		}
	    }
	}
	DESC_ITER_PLAYER(victim, d) {
	    node = (struct SNOOPLISTNODE *) malloc(sizeof(
						     struct SNOOPLISTNODE));

	    if (node == NULL)
		abort();
	    node->snooper = Owner(oper);
	    if (sstat && !(d->logged)) {
		if (!key)
		    node->logonly = 1;
		node->sfile = fopen(unsafe_tprintf("snoop%d.txt", victim), "a");
	    } else {
		if (sstat) {
		    notify_quiet(player, unsafe_tprintf("%s is already being logged.", Name(victim)));
		    if (!key) {
			free(node);
			VOIDRETURN; /* #106 */
		    }
		}
		node->logonly = 0;
		node->sfile = NULL;
	    }
	    if (node->sfile && !(d->logged)) {
		d->logged = 1;
		fprintf(node->sfile, "Snoop attached: [%s/%s] (%ld) %s", 
                        inet_ntoa(d->address.sin_addr), d->addr, mudstate.now, ctime(&mudstate.now));
	    }
	    node->next = d->snooplist;
	    d->snooplist = node;
	}
	notify_quiet(player, "Snoop attached.");
	VOIDRETURN; /* #106 */
    } else if ( key == SNOOP_OFF ) {
	found = 0;
	DESC_ITER_PLAYER(victim, d) {
	    if (d->snooplist != NULL) {
		if (d->snooplist->snooper == Owner(oper)) {
		    node = d->snooplist->next;
		    if (d->snooplist->sfile) {
			fprintf(d->snooplist->sfile, 
                                "Snoop detached: (%ld) %s", mudstate.now, 
                                ctime(&mudstate.now));
			fclose(d->snooplist->sfile);
			d->logged = 0;
		    }
		    free(d->snooplist);
		    d->snooplist = node;
		    found = 1;
		} else {
		    for (prev = d->snooplist, node = d->snooplist;
			 node && node->snooper != Owner(oper);
			 prev = node, node = node->next) {
		    }
		    if (node != NULL) {
			prev->next = node->next;
			if (node->sfile) {
			    fprintf(node->sfile, "Snoop detached: (%ld) %s", mudstate.now, ctime(&mudstate.now));
			    fclose(node->sfile);
			    d->logged = 0;
			}
			free(node);
			found = 1;
		    }
		}
	    }
	}
	if (found) {
	    notify_quiet(player, "Snoop detached.");
	} else {
	    notify_quiet(player, "No match found.");
	}
	VOIDRETURN; /* #106 */
    }
    VOIDRETURN; /* #106 */
}

int dump_reboot_db( void )
{
  DESC* d;
  struct SNOOPLISTNODE* slnptr;
  FILE* rebootfile = NULL, *suffixfile = NULL;
  char rebootfilename[32 + 8], suffixfilename[32 + 8];
  int i_prefix, i_suffix;

  DPUSH; /* #107 */

  STARTLOG(LOG_ALWAYS, "RBT", "DUMP")
    log_text((char *) "Dumping reboot data...");
  ENDLOG

  strcpy(rebootfilename, mudconf.muddb_name);
  strcpy(suffixfilename, mudconf.muddb_name);
  strcat(rebootfilename, ".reboot");
  strcat(suffixfilename, ".fx");

  rebootfile = fopen(rebootfilename, "wb");
  suffixfile = fopen(suffixfilename, "wb");

  if( !rebootfile || !suffixfile) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Unable to open reboot file for writing.");
    ENDLOG
    if (rebootfile) fclose(rebootfile);
    if (suffixfile) fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudconf.who_default), sizeof(mudconf.who_default), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudstate.start_time), sizeof(mudstate.start_time), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudstate.mushflat_time), sizeof(mudstate.mushflat_time), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudstate.newsflat_time), sizeof(mudstate.newsflat_time), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudstate.mailflat_time), sizeof(mudstate.mailflat_time), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }
  if (!fwrite(&(mudstate.aregflat_time), sizeof(mudstate.aregflat_time), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }

  i_prefix = mkattr("____OUTPUTPREFIX");
  i_suffix = mkattr("____OUTPUTSUFFIX");
  DESC_ITER_ALL(d) {
    /* Lensy, bugfix 29/02/04
     * Doors should NOT be carried through on a reboot, at least not here! */
    if (d->flags & DS_HAS_DOOR) {
      d->flags &= ~DS_HAS_DOOR;
    }
    if ( (i_prefix > 0 ) && d->output_prefix && *(d->output_prefix) && Good_chk(d->player) ) {
       if (!fwrite(d->output_prefix, LBUF_SIZE, 1, suffixfile)) {
          STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
            log_text((char *) "Error writing to reboot file.");
          ENDLOG
          fclose(rebootfile);
          fclose(suffixfile);
          RETURN(0); /* #107 */
       }
       d->flags |= DS_HAVEpFX;
    }
    if ( (i_suffix > 0 ) && d->output_suffix && *(d->output_suffix) && Good_chk(d->player) ) {
       if (!fwrite(d->output_suffix, LBUF_SIZE, 1, suffixfile)) {
          STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
            log_text((char *) "Error writing to reboot file.");
          ENDLOG
          fclose(rebootfile);
          fclose(suffixfile);
          RETURN(0); /* #107 */
       }
       d->flags |= DS_HAVEsFX;
    }
    if( !fwrite(d, sizeof(DESC), 1, rebootfile) ) {
      STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
        log_text((char *) "Error writing to reboot file.");
      ENDLOG
      fclose(rebootfile);
      fclose(suffixfile);
      RETURN(0); /* #107 */
    }

    /* clean up logged snoops */
    for( slnptr = d->snooplist; slnptr; slnptr = slnptr->next ) {
      if( slnptr->sfile ) { 
        fprintf(slnptr->sfile, "*** Log closed due to reboot ***\n");
        fclose(slnptr->sfile);
      }
    }
  }

  fclose(rebootfile);
  fclose(suffixfile);
  local_dump_reboot();
  STARTLOG(LOG_ALWAYS, "RBT", "DUMP")
    log_text((char *) "Dump complete.");
  ENDLOG
  RETURN(1); /* #107 */
}

int load_reboot_db( void )
{
  DESC* d;
  DESC* prev;
  DESC* tempd;
  FILE* rebootfile = NULL, *suffixfile = NULL;
  char rebootfilename[32 + 8], suffixfilename[32 + 8], *s_text;
  int i_prefix, i_suffix, i_fxchk;

  DPUSH; /* #108 */

  STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
    log_text((char *) "Loading reboot data...");
  ENDLOG

  i_fxchk = ndescriptors = 0;

  strcpy(rebootfilename, mudconf.muddb_name);
  strcat(rebootfilename, ".reboot");
  strcpy(suffixfilename, mudconf.muddb_name);
  strcat(suffixfilename, ".fx");

  rebootfile = fopen(rebootfilename, "rb");

  if( !rebootfile ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Unable to open reboot file for reading.");
    ENDLOG
    RETURN(0); /* #108 */
  }

  suffixfile = fopen(suffixfilename, "rb");
  if ( !suffixfile ) 
     i_fxchk = 1;

  if( !fread(&(mudconf.who_default), sizeof(mudconf.who_default), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  if( !fread(&(mudstate.start_time), sizeof(mudstate.start_time), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  if( !fread(&(mudstate.mushflat_time), sizeof(mudstate.mushflat_time), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  if( !fread(&(mudstate.newsflat_time), sizeof(mudstate.newsflat_time), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  if( !fread(&(mudstate.mailflat_time), sizeof(mudstate.mailflat_time), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  if( !fread(&(mudstate.aregflat_time), sizeof(mudstate.aregflat_time), 1, rebootfile) ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Error reading from reboot file.");
    ENDLOG
    fclose(rebootfile);
    if ( !i_fxchk ) fclose(suffixfile);
    RETURN(0); /* #108 */
  } 
  i_prefix = mkattr("____OUTPUTPREFIX");
  i_suffix = mkattr("____OUTPUTSUFFIX");
  s_text = alloc_lbuf("reboot_fx");
  while(!feof(rebootfile)) {
    d = alloc_desc("reboot_sock");
    if( !fread(d, sizeof(DESC), 1, rebootfile) ) {
      if( feof(rebootfile) ) {
        break;
      }
      STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
        log_text((char *) "Error reading from reboot file.");
      ENDLOG
      fclose(rebootfile);
      RETURN(0); /* #108 */
    }
    d->output_prefix = NULL;
    if ( (i_prefix > 0) && (d->flags & DS_HAVEpFX) && Good_chk(d->player) ) {
       if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
          set_userstring(&d->output_prefix, s_text);
    }
    d->output_suffix = NULL;
    if ( (i_suffix > 0) && Good_chk(d->player) && (d->flags & DS_HAVEsFX) ) {
       if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
          set_userstring(&d->output_suffix, s_text);
    }
    d->flags &= ~(DS_HAVEpFX|DS_HAVEsFX);
    d->output_size = 0;
    d->output_head = NULL;
    d->output_tail = NULL;
    d->input_size = 0;
    d->input_head = NULL;
    d->input_tail = NULL;
    d->door_desc = 0;
    d->door_output_head = NULL;
    d->door_output_tail = NULL;
    d->door_input_head = NULL;
    d->door_input_tail = NULL;
    d->door_raw = NULL;
    d->door_output_size = 0;
    d->door_lbuf = NULL;
    d->door_mbuf = NULL;
    d->raw_input = NULL;
    d->raw_input_at = NULL;
    d->hashnext = NULL;
    d->next = NULL;
    d->prev = NULL;
    d->snooplist = NULL;
    d->logged = 0;

    /* place the nodes in their original order */
    prev = descriptor_list;
    DESC_ITER_ALL(tempd) {
      /* seek end of list */
      prev = tempd;
    }

    if( !prev ) {
      d->prev = &descriptor_list;
      descriptor_list = d;
    }
    else {
      d->prev = &(prev->next);
      prev->next = d;
    }

    desc_addhash(d);
    s_Connected(d->player);
    STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
      log_text((char*) "Reconnecting: ");
      log_name(d->player);
    ENDLOG
  }
  free_lbuf(s_text);
  fclose(rebootfile);
  if ( suffixfile) {
     fclose(suffixfile);
     unlink(suffixfilename);
  }
  unlink(rebootfilename);
  local_load_reboot();
  STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
    log_text((char *) "Load complete.");
  ENDLOG
  RETURN(1); /* #108 */
}


/* ---------------------------------------------------------------------------
 * timeval_sub: return difference between two times as a timeval
 */

struct timeval 
timeval_sub(struct timeval now, struct timeval then)
{
    DPUSH; /* #109 */
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if (now.tv_usec < 0) {
	now.tv_usec += 1000000;
	now.tv_sec--;
    }
    RETURN(now); /* #109 */
}

/* ---------------------------------------------------------------------------
 * msec_diff: return difference between two times in msec
 */

int 
msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000 +
	    (now.tv_usec - then.tv_usec) / 1000);
}

/* ---------------------------------------------------------------------------
 * msec_add: add milliseconds to a timeval
 */

struct timeval 
msec_add(struct timeval t, int x)
{
    DPUSH; /* #110 */
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;
    if (t.tv_usec >= 1000000) {
	t.tv_sec += t.tv_usec / 1000000;
	t.tv_usec = t.tv_usec % 1000000;
    }
    RETURN(t); /* #110 */
}

/* ---------------------------------------------------------------------------
 * update_quotas: Update timeslice quotas
 */

struct timeval 
update_quotas(struct timeval last, struct timeval current)
{
    int nslices;
    DESC *d;
    struct timeval retval;

    DPUSH; /* #111 */

    nslices = msec_diff(current, last) / mudconf.timeslice;

    if (nslices > 0) {
	DESC_ITER_ALL(d) {
	    d->quota += mudconf.cmd_quota_incr * nslices;
	    if (d->quota > mudconf.cmd_quota_max)
		d->quota = mudconf.cmd_quota_max;
	}
    }
    retval = msec_add(last, nslices * mudconf.timeslice);
    RETURN(retval); /* #111 */
}

/* ---------------------------------------------------------------------------
 * raw_notify: write a message to a player
 */

void 
raw_notify(dbref player, const char *msg, int port, int type)
{
    DESC *d, *sd, *dnext;
    struct SNOOPLISTNODE *node;
    char antemp[20];
    int found;

    DPUSH; /* #112 */

    if (!msg || !*msg) {
	VOIDRETURN; /* #112 */
    }
    if (!Connected(player) && !port) {
	VOIDRETURN; /* #112 */
    }

    strcpy(antemp, ANSI_NORMAL);
    if (!port) {
      DESC_ITER_PLAYER(player, d) {
	if ((d->flags & DS_HAS_DOOR) && !mudstate.droveride && (Flags3(d->player) & DOORRED))
	  continue;
	queue_string(d, msg);
	if (ShowAnsi(d->player) && index(msg, ESC_CHAR))
	    queue_string(d, antemp);
        if ( type )
	   queue_write(d, "\r\n", 2);
	if (d->snooplist) {
	    for (node = d->snooplist; node; node = node->next) {
		if (node->sfile) {
		    fprintf(node->sfile, "%ld|", mudstate.now);
		    fputs(Name(player), node->sfile);
		    fputs("> ", node->sfile);
		    fputs(strip_ansi(msg), node->sfile);
		    fputc('\n', node->sfile);
                    fflush(node->sfile);
		}
		if (!Connected(node->snooper) || node->logonly)
		    continue;
		DESC_ITER_PLAYER(node->snooper, sd) {
		    if ((sd->flags & DS_HAS_DOOR) && (Flags3(sd->player) & DOORRED))
			continue;
                    if( ShowAnsi(sd->player) ) {
                      queue_string(sd, ANSI_HILITE);
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "|");
                      queue_string(sd, ANSI_GREEN);
		      queue_string(sd, Name(player));
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "| ");
                      queue_string(sd, ANSI_NORMAL);
		      queue_string(sd, msg);
		      queue_string(sd, antemp);
                    }
                    else {
		      queue_string(sd, "|");
		      queue_string(sd, Name(player));
		      queue_string(sd, "> ");
		      queue_string(sd, msg);
                    }
                    if ( type )
		       queue_write(sd, "\r\n", 2);
		}
	    }
	}
      }
    }
    else {
      found = 0;
      DESC_SAFEITER_ALL(d,dnext) {
	if (d->descriptor != port)
	  continue;
	if (!(d->player) && !Immortal(mudstate.pageref)) {
	  mudstate.pageref = NOTHING;
	  break;
	}
	if (Immortal(d->player) && SCloak(d->player) && !Immortal(mudstate.pageref)) {
	  mudstate.pageref = NOTHING;
	  break;
	}
	found = 1;
	if ((d->flags & DS_HAS_DOOR) && !mudstate.droveride && (Flags3(d->player) & DOORRED))
	  continue;
	queue_string(d, msg);
	if (d->player && ShowAnsi(d->player) && index(msg, ESC_CHAR))
	    queue_string(d, antemp);
	queue_write(d, "\r\n", 2);
	if (d->snooplist) {
	    for (node = d->snooplist; node; node = node->next) {
		if (node->sfile) {
		    fprintf(node->sfile, "%ld|", mudstate.now);
		    fputs(Name(d->player), node->sfile);
		    fputs("> ", node->sfile);
		    fputs(strip_ansi(msg), node->sfile);
		    fputc('\n', node->sfile);
                    fflush(node->sfile);
		}
		if (!Connected(node->snooper) || node->logonly)
		    continue;
		DESC_ITER_PLAYER(node->snooper, sd) {
		    if ((sd->flags & DS_HAS_DOOR) && (Flags3(sd->player) & DOORRED))
			continue;
                    if( ShowAnsi(sd->player) ) {
                      queue_string(sd, ANSI_HILITE);
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "|");
                      queue_string(sd, ANSI_GREEN);
		      queue_string(sd, Name(d->player));
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "| ");
                      queue_string(sd, ANSI_NORMAL);
		      queue_string(sd, msg);
		      queue_string(sd, antemp);
                    }
                    else {
		      queue_string(sd, "|");
		      queue_string(sd, Name(d->player));
		      queue_string(sd, "> ");
		      queue_string(sd, msg);
                    }
		    queue_write(sd, "\r\n", 2);
		}
	    }
	}
	break;
      }
      if (!found)
        mudstate.pageref=NOTHING;
    }
    VOIDRETURN; /* #112 */
}

void 
broadcast_monitor(dbref player, int inflags, char *type,
		  char *userid,
		  char *site, int succ, int recentfail, int fail, char *reason)
{
    char *buff, *buff2, b3[80], *pt1;
    int i, i_retvar = -1;
    DESC *d;
    struct tm *tp;
    time_t  t;

    DPUSH; /* #113 */

    buff = alloc_lbuf("broadcast_monitor");
    if ( (inflags & (MF_CONN|MF_DCONN|MF_FAIL|MF_BFAIL|MF_GFAIL|MF_COMPFAIL)) && *(mudconf.nobroadcast_host) ) {
       buff2 = alloc_lbuf("nobroadcast_monitor_chk");
       sprintf(buff2, "%s", mudconf.nobroadcast_host);
       if ( lookup(site, buff2, -1, &i_retvar) ) {
          free_lbuf(buff);
          free_lbuf(buff2);
          VOIDRETURN;   
       }
       free_lbuf(buff2);
    }

    if (inflags & MF_TRIM) {
      pt1 = type;
      while (*pt1 && !isspace((int)*pt1))
	pt1++;
      if (*pt1) {
	i = pt1-type;
	strncpy(b3,type,i);
	*(b3+i) = '\0';
      }
      else
	pt1 = NULL;
    }
    else
      pt1 = NULL;
     
    DESC_ITER_CONN(d) {
        if ((inflags & MF_COMPFAIL) && (Toggles2(d->player) & TOG_MONITOR_BFAIL))
            continue;
        if ((inflags & MF_BFAIL) && !(Toggles2(d->player) & TOG_MONITOR_BFAIL))
            continue;
	if ((inflags & MF_FAIL) && !(Toggles(d->player) & TOG_MONITOR_FAIL))
	    continue;
	if ((inflags & MF_VLIMIT) && !(Toggles(d->player) & TOG_MONITOR_VLIMIT))
	    continue;
	if ((inflags & MF_CPU) && !(Toggles(d->player) & TOG_MONITOR_CPU))
            continue;
        if ((inflags & MF_CPUEXT) && !(Toggles(d->player) & TOG_MONITOR_CPU))
            continue;
	if ((d->flags & DS_HAS_DOOR) && (Flags3(d->player) & DOORRED))
	    continue;
	if ((inflags & MF_DCONN) && !Immortal(d->player))
	    continue;
	if (inflags & MF_DCONN)
	    inflags &= ~MF_DCONN;
	if (inflags & MF_CONN) {
	    if ( (Toggles(d->player) & TOG_MONITOR_CONN) && (Toggles(d->player) & TOG_MONITOR) ) {
		if( ShowAnsi(d->player) ) 
                  queue_string(d, ANSI_HILITE);
		queue_string(d, "[MONITOR] ");
		if( ShowAnsi(d->player) ) 
                  queue_string(d, ANSI_NORMAL);
		if (Wizard(d->player) || (pt1 == NULL) || SeeSuspect(d->player))
		  queue_string(d, type);
		else
		  queue_string(d, b3);
		if (reason && TogReason(d->player)) {
		  queue_string(d, " (");
		  queue_string(d, reason);
		  queue_string(d, ")");
		}
		queue_string(d, " | ");
		if (userid && *userid) {
                  if ( fail ) 
		     sprintf(buff, "Port: %d <%d>, Site: %s@%s", succ, fail, userid, site);
                  else
		     sprintf(buff, "Port: %d, Site: %s@%s", succ, userid, site);
		} else {
                  if ( fail ) 
		     sprintf(buff, "Port: %d <%d>, Site: %s", succ, fail, site);
                  else
		     sprintf(buff, "Port: %d, Site: %s", succ, site);
                }
		queue_string(d, buff);
                if ( Toggles(d->player) & TOG_MONITOR_TIME ) {
                   t = time(NULL);
                   tp = localtime(&t);
                   sprintf(buff, " [%02d:%02d:%02d]", tp->tm_hour, tp->tm_min, tp->tm_sec );
	           queue_string(d, buff);
                }
		queue_write(d, "\r\n", 2);
		process_output(d);
	    }
	    continue;
	}
	if (inflags & MF_AREG) {
	  if ( (Toggles(d->player) & TOG_MONITOR_AREG) && (Toggles(d->player) & TOG_MONITOR) )  {
		if( ShowAnsi(d->player) ) 
                  queue_string(d, ANSI_HILITE);
		queue_string(d, "[MONITOR] ");
		if( ShowAnsi(d->player) ) 
                  queue_string(d, ANSI_NORMAL);
		queue_string(d, type);
		if (userid && *userid)
		  sprintf(buff, " | Port: %d, Site: %s@%s, Char/Email: %s", succ, userid, site, reason);
		else
		  sprintf(buff, " | Port: %d, Site: %s, Char/Email: %s", succ, site, reason);
		queue_string(d, buff);
                if ( Toggles(d->player) & TOG_MONITOR_TIME ) {
                   t = time(NULL);
                   tp = localtime(&t);
                   sprintf(buff, " [%02d:%02d:%02d]", tp->tm_hour, tp->tm_min, tp->tm_sec );
	           queue_string(d, buff);
                }
                queue_string(d, "\r\n");
		process_output(d);
	  }
	  continue;
	}
	if (((Toggles(d->player) & TOG_MONITOR) &&
	((((player != NOTHING) && !Cloak(player)) || Immortal(d->player) || (inflags & (MF_FAIL|MF_BFAIL)))))) {
	    if( ShowAnsi(d->player) ) 
              queue_string(d, ANSI_HILITE);
	    queue_string(d, "[MONITOR] ");
	    if( ShowAnsi(d->player) ) 
              queue_string(d, ANSI_NORMAL);
	    if (Wizard(d->player) || (pt1 == NULL) || SeeSuspect(d->player))
	      queue_string(d, type);
	    else
	      queue_string(d, b3);
	    if (reason && TogReason(d->player)) {
	      queue_string(d, " (");
	      queue_string(d, reason);
	      queue_string(d, ")");
	    }
	    if (!(inflags & (MF_GFAIL|MF_BFAIL))) {
	      queue_string(d, " | ");
              if ( (player != NOTHING) && Good_obj(player) )
	         queue_string(d, Name(player));
              else
	         queue_string(d, "(Unknown Player)");
	    }
	    if (inflags & MF_VLIMIT) {
	      queue_string(d, " | ");
              if ( (succ != NOTHING) && Good_obj(succ) )
	         queue_string(d, Name(succ));
              else
	         queue_string(d, "(Unknown Player)");
	      queue_string(d, "(#");
	      queue_string(d, myitoa(succ));
	      if (userid) {
		queue_string(d, "->");
		queue_string(d, userid);
	      }
	      queue_string(d, ")");
	    }
	    if (inflags & MF_CPU) {
	      queue_string(d, " | ");
              if ( (succ != NOTHING) && Good_obj(succ) )
	         queue_string(d, Name(succ));
              else
	         queue_string(d, "(Unknown Player)");
	      queue_string(d, "(#");
	      queue_string(d, myitoa(succ));
	      if (userid) {
		queue_string(d, "->'");
		queue_string(d, userid);
	      }
	      queue_string(d, "')");
            }
            if (inflags & MF_CPUEXT) {
                if (userid) {
                   queue_string(d, " | ");
                   queue_string(d, userid);
                }
            }
	    if (!(inflags & (MF_SITE | MF_STATS))) {
                if ( Toggles(d->player) & TOG_MONITOR_TIME ) {
                   t = time(NULL);
                   tp = localtime(&t);
                   sprintf(buff, " [%02d:%02d:%02d]", tp->tm_hour, tp->tm_min, tp->tm_sec );
	           queue_string(d, buff);
                }
		queue_write(d, "\r\n", 2);
		process_output(d);
		continue;
	    }
	    if (((inflags & (MF_SITE)) && Toggles(d->player) & (TOG_MONITOR_USERID | TOG_MONITOR_SITE)))
		queue_string(d, " | ");
	    if ((Toggles(d->player) & TOG_MONITOR_USERID) &&
		(*userid)) {
		queue_string(d, userid);
		queue_string(d, "@");
	    }
	    if (Toggles(d->player) & (TOG_MONITOR_SITE | TOG_MONITOR_USERID))
		queue_string(d, site);
	    if ((inflags & MF_STATS) &&
		(Toggles(d->player) & TOG_MONITOR_STATS)) {
		sprintf(buff, " (Succ: %d | Fail: %d/%d)",
			succ, recentfail, fail);
		queue_string(d, buff);
	    }
            if ( Toggles(d->player) & TOG_MONITOR_TIME ) {
               t = time(NULL);
               tp = localtime(&t);
               sprintf(buff, " [%02d:%02d:%02d]", tp->tm_hour, tp->tm_min, tp->tm_sec );
	       queue_string(d, buff);
            }
	    queue_write(d, "\r\n", 2);
	    process_output(d);
	}
    }
    free_lbuf(buff);
    VOIDRETURN; /* #113 */
}


/* ---------------------------------------------------------------------------
 * raw_broadcast: Send message to players who have indicated flags
 */

#ifdef NEED_VSPRINTF_DCL
extern char *FDECL(vsprintf, (char *, char *, va_list));

#endif

#ifdef STDC_HEADERS
void 
raw_broadcast(dbref sender, int inflags, char *template,...)
#else
void 
raw_broadcast(va_alist)
    va_dcl

#endif

{
    char *buff;
    char antemp[20];
#ifdef ZENTY_ANSI
    char *message, *mptr, *msg_ns2, *mp_ns2;
#endif
    DESC *d;
    va_list ap;
    int i_nowalls;

    DPUSH; /* #114 */

#ifdef STDC_HEADERS
    va_start(ap, template);
#else
    dbref sender;
    int inflags;
    char *template;

    va_start(ap);
    sender = va_arg(ap, dbref);
    inflags = va_arg(ap, int);
    template = va_arg(ap, char *);

#endif
    if (!template || !*template){
	VOIDRETURN; /* #114 */
    }

    i_nowalls = 0;
    if ( inflags & NO_WALLS ) {
       i_nowalls = 1;
       inflags &= ~NO_WALLS;
    }
    buff = alloc_lbuf("raw_broadcast");
    vsprintf(buff, template, ap);

#ifdef ZENTY_ANSI   
    mptr = message = alloc_lbuf("raw_broadcast_message");
    mp_ns2 = msg_ns2 = alloc_lbuf("notify_check_accents");
    parse_ansi( (char *) buff, message, &mptr, msg_ns2, &mp_ns2);
    *mp_ns2 = '\0';
#endif   
    strcpy(antemp, ANSI_NORMAL);
    DESC_ITER_CONN(d) {
	if ((!(((Flags2(d->player) & NO_WALLS) || ((d->flags & DS_HAS_DOOR) && (Flags3(d->player) & DOORRED))) 
	  && sender != 0 && !Wizard(sender)) || mudstate.nowall_over) &&
	    ((inflags == 0) ||
	     ((inflags & GUILDMASTER) && Guildmaster(d->player)) ||
	     ((inflags & BUILDER) && Builder(d->player)) ||
	     ((inflags & ADMIN) && Admin(d->player)) ||
	     ((inflags & WIZARD) && Wizard(d->player)) ||
	     ((inflags & IMMORTAL) && Immortal(d->player)))) {
           if ( i_nowalls && (Flags2(d->player) & NO_WALLS) ) 
              continue;
#ifdef ZENTY_ANSI	   
           if ( Accents(d->player ) )
              queue_string(d, msg_ns2);
           else
	      queue_string(d, strip_safe_accents(message));
#else	   
	   queue_string(d, buff);
#endif	   
	    if (ShowAnsi(d->player) && index(buff, ESC_CHAR))
		queue_string(d, antemp);
	    queue_write(d, "\r\n", 2);
	    process_output(d);
	}
    }
    free_lbuf(buff);
#ifdef ZENTY_ANSI
    free_lbuf(message);
    free_lbuf(msg_ns2);
#endif
    va_end(ap);
    VOIDRETURN; /* #114 */
}

/* ---------------------------------------------------------------------------
 * raw_broadcast2: Send message to players who have indicated flags
 * DONT USE THIS ANYMORE, USE THE ABOVE -2/95 thorin
 */

#ifdef NEED_VSPRINTF_DCL
extern char *FDECL(vsprintf, (char *, char *, va_list));

#endif

#ifdef STDC_HEADERS
void 
raw_broadcast2(int inflags, char *template,...)
#else
void 
raw_broadcast2(va_alist)
    va_dcl

#endif

{
    char *buff;
    char antemp[20];
    DESC *d;
    va_list ap;
 
    DPUSH; /* #115 */

#ifdef STDC_HEADERS
    va_start(ap, template);
#else
    int inflags;
    char *template;

    va_start(ap);
    inflags = va_arg(ap, int);
    template = va_arg(ap, char *);

#endif
    if (!template || !*template) {
	VOIDRETURN; /* #115 */
    }

    buff = alloc_lbuf("raw_broadcast");
    vsprintf(buff, template, ap);

    strcpy(antemp, ANSI_NORMAL);
    DESC_ITER_CONN(d) {
	if ((inflags != WIZARD) && ((Flags2(d->player) & inflags) == inflags)) {
	    if ((((!(Flags2(d->player) & NO_WALLS)) || mudstate.nowall_over) && (!Wizard(d->player))) || (inflags == 0)) {
		if (((inflags == BUILDER) && !Builder(d->player)) || (inflags == ADMIN) || (inflags == 0)) {
		    queue_string(d, buff);
		    if (ShowAnsi(d->player) && index(buff, ESC_CHAR))
			queue_string(d, antemp);
		    queue_write(d, "\r\n", 2);
		    process_output(d);
		}
	    }
	} else if (Immortal(d->player) && (inflags == WIZARD)) {
	    queue_string(d, buff);
	    if (ShowAnsi(d->player) && index(buff, ESC_CHAR))
		queue_string(d, antemp);
	    queue_write(d, "\r\n", 2);
	    process_output(d);
	}
    }
    free_lbuf(buff);
    va_end(ap);
    VOIDRETURN; /* #115 */
}

/* ---------------------------------------------------------------------------
 * clearstrings: clear out prefix and suffix strings
 */

void 
clearstrings(DESC * d)
{
    DPUSH; /* #116 */
    if (d->output_prefix) {
	free_lbuf(d->output_prefix);
	d->output_prefix = NULL;
    }
    if (d->output_suffix) {
	free_lbuf(d->output_suffix);
	d->output_suffix = NULL;
    }
    VOIDRETURN; /* #116 */
}

/* ---------------------------------------------------------------------------
 * queue_write: Add text to the output queue for the indicated descriptor.
 */

void 
queue_write(DESC * d, const char *b, int n)
{
    int left;
    char *buf;
    TBLOCK *tp;

    DPUSH; /* #117 */

    if (n <= 0) {
	VOIDRETURN; /* #117 */
    }

    if (d->output_size + n > mudconf.output_limit)
	process_output(d);

    left = mudconf.output_limit - d->output_size - n;
    if (left < 0) {
        mudstate.outputflushed = 1;
	tp = d->output_head;
	if (tp == NULL) {
	    STARTLOG(LOG_PROBLEMS, "QUE", "WRITE")
		log_text((char *) "Flushing when output_head is null!");
	    ENDLOG
	} else {
	    STARTLOG(LOG_NET, "NET", "WRITE")
            buf = alloc_lbuf("queue_write.LOG");
	    sprintf(buf,
		    "[%d/%s] Output buffer overflow, %d chars discarded by ",
		    d->descriptor, d->addr, tp->hdr.nchars);
	    log_text(buf);
	    free_lbuf(buf);
	    log_name(d->player);
	    ENDLOG
            d->output_size -= tp->hdr.nchars;
	    d->output_head = tp->hdr.nxt;
	    d->output_lost += tp->hdr.nchars;
	    if (d->output_head == NULL)
		d->output_tail = NULL;
	    free_lbuf(tp);
	}
    }
    /* Allocate an output buffer if needed */

    if (d->output_head == NULL) {
	tp = (TBLOCK *) alloc_lbuf("queue_write.new");
	tp->hdr.nxt = NULL;
	tp->hdr.start = tp->data;
	tp->hdr.end = tp->data;
	tp->hdr.nchars = 0;
	d->output_head = tp;
	d->output_tail = tp;
    } else {
	tp = d->output_tail;
    }

    /* Now tp points to the last buffer in the chain */

    d->output_size += n;
    d->output_tot += n;
    do {

	/* See if there is enough space in the buffer to hold the
	 * string.  If so, copy it and update the pointers..
	 */

        /* BUGFIX 1/4/1997 - Thorin */
	/* left = LBUF_SIZE - (tp->hdr.end - (char *) tp); */
	left = LBUF_SIZE - sizeof(TBLKHDR) - (tp->hdr.end - (char *) tp);

	if (n <= left) {
	    strncpy(tp->hdr.end, b, n);
	    tp->hdr.end += n;
	    tp->hdr.nchars += n;
	    n = 0;
	} else {

	    /* It didn't fit.  Copy what will fit and then allocate
	     * another buffer and retry.
	     */

	    if (left > 0) {
		strncpy(tp->hdr.end, b, left);
		tp->hdr.end += left;
		tp->hdr.nchars += left;
		b += left;
		n -= left;
	    }
	    tp = (TBLOCK *) alloc_lbuf("queue_write.extend");
	    tp->hdr.nxt = NULL;
	    tp->hdr.start = tp->data;
	    tp->hdr.end = tp->data;
	    tp->hdr.nchars = 0;
	    d->output_tail->hdr.nxt = tp;
	    d->output_tail = tp;
	}
    } while (n > 0);
    VOIDRETURN; /* #117 */
}


void 
queue_string(DESC * d, const char *s)
{
    char *new;

    DPUSH; /* #119 */

    if ( !d )
       VOIDRETURN;
    if ((!d->player || !ShowAnsi(d->player)) && index(s, ESC_CHAR))
	new = strip_ansi(s);
    else if (!ShowAnsiColor(d->player) && index(s, ESC_CHAR))
	new = strip_ansi_color(s);
    else if (!ShowAnsiXterm(d->player) && index(s, ESC_CHAR))
        new = strip_ansi_xterm(s);
    else
	new = (char *) s;
    if (NoFlash(d->player) && index(new, ESC_CHAR))
	new = strip_ansi_flash(new);
    if (NoUnderline(d->player) && index(new, ESC_CHAR))
	new = strip_ansi_underline(new);
    if (new)
	queue_write(d, new, strlen(new));
    VOIDRETURN; /* #119 */
}

void 
freeqs(DESC * d, int dooronly)
{
    TBLOCK *tb, *tnext;
    CBLK *cb, *cnext;

    DPUSH; /* #121 */

  if (!dooronly) {
    tb = d->output_head;
    while (tb) {
	tnext = tb->hdr.nxt;
	free_lbuf(tb);
	tb = tnext;
    }
    d->output_head = NULL;
    d->output_tail = NULL;

    cb = d->input_head;
    while (cb) {
	cnext = (CBLK *) cb->hdr.nxt;
	free_lbuf(cb);
	cb = cnext;
    }
    d->input_head = NULL;
    d->input_tail = NULL;

    if (d->raw_input)
	free_lbuf(d->raw_input);
    d->raw_input = NULL;
    d->raw_input_at = NULL;
  }

  cb = d->door_input_head;
  while (cb) {
    cnext = (CBLK *) cb->hdr.nxt;
    free_lbuf(cb);
    cb = cnext;
  }
  d->door_input_head = NULL;
  d->door_input_tail = NULL;

  tb = d->door_output_head;
  while (tb) {
    tnext = tb->hdr.nxt;
    free_lbuf(tb);
    tb = tnext;
  }
  d->door_output_head = NULL;
  d->door_output_tail = NULL;
  VOIDRETURN; /* #121 */
}

/* ---------------------------------------------------------------------------
 * desc_addhash: Add a net descriptor to its player hash list.
 */

static void 
desc_addhash(DESC * d)
{
    dbref player;
    DESC *hdesc;

    DPUSH; /* #122 */

    player = d->player;
    hdesc = (DESC *) nhashfind((int) player, &mudstate.desc_htab);
    if (hdesc == NULL) {
	d->hashnext = NULL;
	nhashadd((int) player, (int *) d, &mudstate.desc_htab);
    } else {
	d->hashnext = hdesc;
	nhashrepl((int) player, (int *) d, &mudstate.desc_htab);
    }
    VOIDRETURN; /* #122 */
}

/* ---------------------------------------------------------------------------
 * desc_delhash: Remove a net descriptor from its player hash list.
 */

static void 
desc_delhash(DESC * d)
{
    DESC *hdesc, *last;
    dbref player;

    DPUSH; /* #123 */

    player = d->player;
    last = NULL;
    hdesc = (DESC *) nhashfind((int) player, &mudstate.desc_htab);
    while (hdesc != NULL) {
	if (d == hdesc) {
	    if (last == NULL) {
		if (d->hashnext == NULL) {
		    nhashdelete((int) player,
				&mudstate.desc_htab);
		} else {
		    nhashrepl((int) player,
			      (int *) d->hashnext,
			      &mudstate.desc_htab);
		}
	    } else {
		last->hashnext = d->hashnext;
	    }
	    break;
	}
	last = hdesc;
	hdesc = hdesc->hashnext;
    }
    d->hashnext = NULL;
    VOIDRETURN; /* #123 */
}

void 
welcome_user(DESC * d)
{
    DPUSH; /* #124 */
    if (mudconf.offline_reg && (d->host_info & H_REGISTRATION))
	fcache_dump(d, FC_CONN_AUTO_H);
    else if (mudconf.offline_reg)
	fcache_dump(d, FC_CONN_AUTO);
    else if (d->host_info & H_REGISTRATION)
	fcache_dump(d, FC_CONN_REG);
    else
	fcache_dump(d, FC_CONN);
    VOIDRETURN; /* #124 */
}

void 
save_command(DESC * d, CBLK * command)
{
    DPUSH; /* #125 */
    command->hdr.nxt = NULL;
    if (d->input_tail == NULL)
	d->input_head = command;
    else
	d->input_tail->hdr.nxt = command;
    d->input_tail = command;
    VOIDRETURN; /* #125 */
}

static void 
set_userstring(char **userstring, const char *command)
{
    DPUSH; /* #126 */
    while (*command && isascii((int)*command) && isspace((int)*command))
	command++;
    if (!*command) {
	if (*userstring != NULL) {
	    free_lbuf(*userstring);
	    *userstring = NULL;
	}
    } else {
	if (*userstring == NULL)
	    *userstring = alloc_lbuf("set_userstring");
	strcpy(*userstring, command);
    }
    VOIDRETURN; /* #126 */
}

static int 
parse_connect(const char *msg, char *command, char *user, char *pass)
{
    char *p, *c1, *c2, d1, d2;
    int count;

    DPUSH; /* #127 */

    while (*msg && isascii((int)*msg) && isspace((int)*msg))
	msg++;
    p = command;
    count = 0;
    while (*msg && isascii((int)*msg) && !isspace((int)*msg) && (count < MBUF_SIZE)) {
	count++;
	*p++ = *msg++;
    }
    if (count >= (MBUF_SIZE -1)) {
	RETURN(0); /* #127 */
    }
    d1 = *p;
    *p = '\0';
    c1 = p;
    while (*msg && isascii((int)*msg) && isspace((int)*msg))
	msg++;
    p = user;
    count = 0;
    if (mudconf.name_spaces && (*msg == '\"')) {
	for (; *msg && (*msg == '\"' || isspace((int)*msg)); msg++);
	while (*msg && *msg != '\"' && (count < MBUF_SIZE)) {
	    while (*msg && !isspace((int)*msg) && (*msg != '\"') && (count < MBUF_SIZE)) {
		*p++ = *msg++;
		count++;
	    }
    	    if (count >= (MBUF_SIZE -1)) {
		*c1 = d1;
		RETURN(0); /* #127 */
    	    }
	    if (*msg == '\"')
		break;
	    while (*msg && isspace((int)*msg))
		msg++;
	    if (*msg && (*msg != '\"')) {
		count++;
		*p++ = ' ';
	    }
	}
        if (count >= (MBUF_SIZE -1)) {
	    *c1 = d1;
	    RETURN(0); /* #127 */
        }
	for (; *msg && *msg == '\"'; msg++);
    } else {
	while (*msg && isascii((int)*msg) && !isspace((int)*msg) && (count < MBUF_SIZE)) {
	    count++;
	    *p++ = *msg++;
	}
        if (count >= (MBUF_SIZE -1)) {
	    *c1 = d1;
	    RETURN(0); /* #127 */
        }
    }
    d2 = *p;
    c2 = p;
    *p = '\0';
    while (*msg && isascii((int)*msg) && isspace((int)*msg))
	msg++;
    p = pass;
    count = 0;
    while (*msg && isascii((int)*msg) && !isspace((int)*msg) && (count < MBUF_SIZE)) {
	count++;
	*p++ = *msg++;
    }
    if (count >= (MBUF_SIZE -1)) {
	*c1 = d1;
	*c2 = d2;
	RETURN(0); /* #127 */
    }
    *p = '\0';
    RETURN(1); /* #127 */
}

const char *
time_format_1(time_t dt)
{
    register struct tm *delta;
    static char buf[64];
    int i_syear;

    DPUSH; /* #128 */

    if (dt < 0)
	dt = 0;

    i_syear = ((int)dt / 31536000);
    delta = gmtime(&dt);
    if ( i_syear > 0 ) {
	sprintf(buf, "%dy %02d:%02d",
		i_syear, delta->tm_hour, delta->tm_min);
    } else if (delta->tm_yday > 0) {
	sprintf(buf, "%dd %02d:%02d",
		delta->tm_yday, delta->tm_hour, delta->tm_min);
    } else {
	sprintf(buf, "%02d:%02d",
		delta->tm_hour, delta->tm_min);
    }
    RETURN(buf); /* #128 */
}

static const char *
time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];
    int i_syear;

    DPUSH; /* #129 */

    if (dt < 0)
	dt = 0;

    delta = gmtime(&dt);
    i_syear = ((int)dt / 31536000);
    if ( i_syear > 0 ) {
	sprintf(buf, "%dy", i_syear);
    } else if (delta->tm_yday > 0) {
	sprintf(buf, "%dd", delta->tm_yday);
    } else if (delta->tm_hour > 0) {
	sprintf(buf, "%dh", delta->tm_hour);
    } else if (delta->tm_min > 0) {
	sprintf(buf, "%dm", delta->tm_min);
    } else {
	sprintf(buf, "%ds", delta->tm_sec);
    }
    RETURN(buf); /* #129 */
}

static void 
announce_connect(dbref player, DESC * d, int dc)
{
    dbref loc, aowner, temp, obj, aowner2;
    int aflags, num, key, aflags2;
    char *buf, *tbuf, *time_str, *progatr;
    DESC *dtemp;
    char *argv[1];
    int totsucc, totfail, newfail;

    DPUSH; /* #130 */

    desc_addhash(d);
    buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags);
    if (buf) {
	d->timeout = atoi(buf);
	if (d->timeout <= 0 && d->timeout != -1)
	    d->timeout = mudconf.idle_timeout;
    }
    free_lbuf(buf);

    buf = atr_get(player, A_GUILD, &aowner, &aflags);
    if (!(*buf))
	atr_add_raw(player, A_GUILD, (char *) "Citizen");
    free_lbuf(buf);

    loc = Location(player);
    s_Connected(player);

    raw_notify(player, mudconf.motd_msg, 0, 1);
    if (Builder(player)) {
	raw_notify(player, mudconf.wizmotd_msg, 0, 1);
	if (!(mudconf.control_flags & CF_LOGIN)) {
	    raw_notify(player, "*** Logins are disabled.", 0, 1);
	}
    }
    buf = atr_get(player, A_LPAGE, &aowner, &aflags);
    if (buf && *buf) {
	raw_notify(player, "Your PAGE LOCK is set.  You may be unable to receive some pages.", 0, 1);
    }
    tbuf = alloc_mbuf("connect_rwho");
#ifdef RWHO_IN_USE
    if (mudconf.rwho_transmit && (mudconf.control_flags & CF_RWHO_XMIT)) {
	sprintf(buf, "%d@%s", player, mudconf.mud_name);
	sprintf(tbuf, "%s(%s)", Name(player), Guild(player));
	rwhocli_userlogin(buf, tbuf, time((time_t *) 0));
    }
#endif
    num = 0;
    DESC_ITER_PLAYER(player, dtemp) num++;

    if (num < 2)
	sprintf(buf, "%s has connected.", Name(player));
    else
	sprintf(buf, "%s has reconnected.", Name(player));
    key = MSG_INV;
    if ((loc != NOTHING) && !(Dark(player) && Wizard(player)))
	key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);

    time_str = ctime(&mudstate.now);
    time_str[strlen(time_str) - 1] = '\0';
    record_login(player, 1, time_str, d->addr, &totsucc, &totfail, &newfail);
    atr_add_raw(player, A_LASTIP, inet_ntoa(d->address.sin_addr));
    if ( num > 1 ) {
       if (Suspect(player))
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "RECONNECT [SUSPECT]",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else if (d->host_info & H_SUSPECT)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "RECONNECT [SUSPECT SITE]",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else if (dc)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_DCONN, "RECONNECT",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else 
	   broadcast_monitor(player, MF_SITE | MF_STATS, "RECONNECT",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
    } else {
       if (Suspect(player))
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "CONNECT [SUSPECT]",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else if (d->host_info & H_SUSPECT)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "CONNECT [SUSPECT SITE]",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else if (dc)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_DCONN, "CONNECT",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
       else 
	   broadcast_monitor(player, MF_SITE | MF_STATS, "CONNECT",
			     d->userid, d->addr, totsucc, newfail, totfail, NULL);
    }
    temp = mudstate.curr_enactor;
    mudstate.curr_enactor = player;
    if ( (!dc && !Cloak(player) && !NCloak(player) && !(Wizard(player) && Dark(player)) &&
         !(mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc)))) ) ||
	DePriv(player, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA)) {
#ifdef REALITY_LEVELS
        if (loc == NOTHING)
           notify_check(player, player, buf, 0, key, 0);
        else
           notify_except_rlevel(loc, player, player, buf, 0);
#else
        notify_check(player, player, buf, 0, key, 0);
#endif /* REALITY_LEVELS */
    } else {
	if (!Wizard(player)) {
	    if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if (dc == 2) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (@hide Connect)"));
	    } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	} else if (!Immortal(player)) {
	    if (Cloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Wizcloaked)"));
	    } else if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if (dc == 2) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (@hide Connect)"));
	    } else if (Dark(player)) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
	    } else if (dc) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark Connect)"));
            } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	} else {
	    if (Cloak(player)) {
		if (SCloak(player)) {
		    notify_except3(loc, player, player, player, 1,
				   strcat(buf, " (Supercloaked)"));
		} else {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Wizcloaked)"));
		}
	    } else if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if (Dark(player)) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
	    } else if (dc == 2) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (@hide Connect)"));
	    } else if (dc) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark Connect)"));
            } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	}
    }
    free_lbuf(buf);
    free_mbuf(tbuf);

    if ( (num < 2) || ((num > 1) && mudconf.partial_conn) ) {

       if ( num < 2 )
          argv[0] = (char *) "0";
       else
          argv[0] = (char *) "1";

       buf = atr_pget(player, A_ACONNECT, &aowner, &aflags);
       if (buf && *buf) {
	 wait_que(player, player, 0, NOTHING, buf, argv, 1, NULL, NULL);
       }
       free_lbuf(buf);

       /* Handle room aconnect */
       if (Good_obj(Location(player))
	   && mudconf.room_aconn 
	   && Location(player) != mudconf.master_room
	   && CANSEE(Location(player), player)) { 
	 buf = atr_pget(Location(player), A_ACONNECT, &aowner, &aflags);
	 if (buf && *buf) {
	   wait_que(Location(player), player, 0, NOTHING, buf, argv, 1, NULL, NULL);
	 }
	 free_lbuf(buf);
       }


       /* Do GLOBAL ACONNECT */
       if( Good_obj(mudconf.master_room) ) {
	   if(mudconf.global_aconn) {
	       DOLIST(obj, Contents(mudconf.master_room)) {
#ifdef REALITY_LEVELS
                   if(!isPlayer(obj) && IsReal(obj, player)) {
#else
                   if(!isPlayer(obj)) {
#endif /* REALITY_LEVELS */
		       buf = atr_pget(obj, A_ACONNECT, &aowner, &aflags);
		       if(buf && *buf && !God(player))
			   wait_que(obj, player, 0, NOTHING, buf, argv, 
                                    1, NULL, NULL);
		       free_lbuf(buf);
		   }
	       }
	   }
       }
    }

    /* Initialize cpu for connected players */
    mudstate.chkcpu_stopper = time(NULL);
    mudstate.chkcpu_toggle = 0;
    mudstate.chkcpu_locktog = 0;

    look_in(player, Location(player), (LK_SHOWEXIT | LK_OBEYTERSE));
    if ( InProgram(player) ) {
       if ( (mudconf.login_to_prog && !(ProgCon(player))) || 
            (!mudconf.login_to_prog && ProgCon(player)) ) {
          notify(player, "You are still in a @program.");
          DESC_ITER_CONN(d) {
             if ( d->player == player ) {
                progatr = atr_get(player, A_PROGPROMPTBUF, &aowner2, &aflags2);
                if ( progatr && *progatr ) {
                   if ( strcmp(progatr, "NULL") != 0 )
                      queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
                } else
                   queue_string(d, unsafe_tprintf("%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
                free_lbuf(progatr);
                break;
             }
          }
       } else {
          notify(player, "Your @program was aborted from disconnecting.");
          s_Flags4(player, (Flags4(player) & (~INPROGRAM)));
          DESC_ITER_CONN(d) {
             if ( d->player == player ) {
                queue_string(d, "\377\371");
             }
          }
          mudstate.shell_program = 0;
          atr_clr(player, A_PROGBUFFER);
          atr_clr(player, A_PROGPROMPTBUF);
       }
    }
    mudstate.curr_enactor = temp;
    local_player_connect(player, num);
    VOIDRETURN; /* #130 */
}

void 
announce_disconnect(dbref player, DESC *d, const char *reason)
{
    dbref loc, aowner, temp, obj;
    int num, aflags, key, cmdcnt;
    char *buf, *atr_temp, *cmdbuf, s_timeon[11];
    DESC *dtemp;
    char *argv[3];

    DPUSH; /* #131 */

    cmdcnt=0;
    memset(s_timeon, 0, sizeof(s_timeon));
    cmdbuf = atr_get(player, A_TOTCMDS, &aowner, &aflags);
    if (*cmdbuf)
       cmdcnt = atoi(cmdbuf);
    free_lbuf(cmdbuf);
    cmdcnt += d->command_count;
    if ( cmdcnt > 2000000000 )
       cmdcnt = 2000000000;
    atr_add_raw(player, A_TOTCMDS, unsafe_tprintf("%d",cmdcnt));
    atr_add_raw(player, A_LSTCMDS, unsafe_tprintf("%d", d->command_count));
    atr_add_raw(player, A_TOTCHARIN, unsafe_tprintf("%d", d->input_tot));
    atr_add_raw(player, A_TOTCHAROUT, unsafe_tprintf("%d", d->output_tot));


    loc = Location(player);
    num = 0;
    DESC_ITER_PLAYER(player, dtemp) num++;

    if (Suspect(player)) {
        if ( num < 2 )
	   broadcast_monitor(player, MF_TRIM, "DISCONN [SUSPECT]", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, MF_TRIM, "PARTIAL DISCONN [SUSPECT]", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
    } else if (d->host_info & H_SUSPECT) {
        if ( num < 2 )
	   broadcast_monitor(player, MF_TRIM, "DISCONN [SUSPECT SITE]", 
                             d->userid, d->addr, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, MF_TRIM, "PARTIAL DISCONN [SUSPECT SITE]", 
                             d->userid, d->addr, 0, 0, 0, (char *)reason);
    } else {
        if ( num < 2 )
	   broadcast_monitor(player, 0, "DISCONN", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, 0, "PARTIAL DISCONN", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
    }
    temp = mudstate.curr_enactor;
    mudstate.curr_enactor = player;

    if (num < 2) {
	buf = alloc_mbuf("announce_disconnect.only");

#ifdef RWHO_IN_USE
	if (mudconf.rwho_transmit &&
	    (mudconf.control_flags & CF_RWHO_XMIT)) {
	    sprintf(buf, "%d@%s", player, mudconf.mud_name);
	    rwhocli_userlogout(buf);
	}
#endif
	sprintf(buf, "%s has disconnected.", Name(player));
	key = MSG_INV;
	if ((loc != NOTHING) && !(Dark(player) && Wizard(player)))
	    key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);
	if ( (!Cloak(player) && !NCloak(player) && !(Wizard(player) && Dark(player)) &&
              !(mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc)))) ) ||
	    DePriv(player, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA))
#ifdef REALITY_LEVELS
            if (loc == NOTHING)
               notify_check(player, player, buf, 0, key, 0);
            else
               notify_except_rlevel(loc, player, player, buf, 0);
#else
            notify_check(player, player, buf, 0, key, 0);
#endif /* REALITY_LEVELS */
	else {
	    if (!Wizard(player)) {
		if (NCloak(player)) {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Cloaked)"));
		} else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
                }
	    } else if (!Immortal(player)) {
		if (Cloak(player)) {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Wizcloaked)"));
		} else if (NCloak(player)) {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Cloaked)"));
		} else if (Dark(player)) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
		} else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
                }
	    } else {
		if (Cloak(player)) {
		    if (SCloak(player)) {
			notify_except3(loc, player, player, player, 1,
				       strcat(buf, " (Supercloaked)"));
		    } else {
			notify_except3(loc, player, player, player, 0,
				       strcat(buf, " (Wizcloaked)"));
		    }
		} else if (NCloak(player)) {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Cloaked)"));
		} else if (Dark(player)) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
		} else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
                }
	    }
	}
	free_mbuf(buf);

        sprintf(s_timeon, "%ld", (mudstate.now - d->connected_at));
	argv[0] = (char *) reason;
        argv[1] = (char *) s_timeon;
        argv[2] = (char *) "0";
        c_Connected(player);
	atr_temp = atr_pget(player, A_ADISCONNECT, &aowner, &aflags);
	if (atr_temp && *atr_temp) {
	  wait_que(player, player, 0, NOTHING, atr_temp, argv, 3, NULL, NULL);
	}
	free_lbuf(atr_temp);

	/* Room level adisconnect */
        if (Good_obj(Location(player))
	    && mudconf.room_adconn 
	    && Location(player) != mudconf.master_room
	    && CANSEE(Location(player), player)) { 
	  atr_temp = atr_pget(Location(player), A_ADISCONNECT, &aowner, &aflags);
	  if (atr_temp && *atr_temp) {
	     wait_que(Location(player), player, 0, NOTHING, atr_temp , argv, 3, NULL, NULL);
	  }
	  free_lbuf(atr_temp);
	}

	/* DO GLOBAL ADISCONNECTS */

	if( Good_obj(mudconf.master_room) ) {
	    if (mudconf.global_adconn) {
		DOLIST(obj, Contents(mudconf.master_room)) {
#ifdef REALITY_LEVELS
                    if (!isPlayer(obj) && IsReal(obj,player)) {
#else
                    if (!isPlayer(obj)) {
#endif /* REALITY_LEVELS */
			atr_temp = atr_pget(obj, A_ADISCONNECT,
					    &aowner, &aflags);
			if (atr_temp && *atr_temp && !God(player))
			    wait_que(obj, player, 0, NOTHING,
				     atr_temp, argv, 3, NULL, NULL);
			free_lbuf(atr_temp);
		    }
		}
	    }
	}
	if (d->flags & DS_AUTODARK) {
	    s_Flags(d->player, Flags(d->player) & ~DARK);
	    d->flags &= ~DS_AUTODARK;
	}
	if (d->flags & DS_AUTOUNF) {
	    s_Flags2(d->player, Flags2(d->player) & ~UNFINDABLE);
	    d->flags &= ~DS_AUTOUNF;
	}
    } else {
	buf = alloc_mbuf("announce_disconnect.partial");
	sprintf(buf, "%s has partially disconnected.", Name(player));
	key = MSG_INV;
	if ((loc != NOTHING) && !(Dark(player) && Wizard(player))) {
	  key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);
	}

	if ( (!Cloak(player) && !NCloak(player) && !(Wizard(player) && Dark(player)) &&
              !(mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc)))) ) ||
	    DePriv(player, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA)) {
#ifdef REALITY_LEVELS
	  if (loc == NOTHING) {
	    notify_check(player, player, buf, 0, key, 0);
	  } else {
	    notify_except_rlevel(loc, player, player, buf, 0);
	  }
#else
            notify_check(player, player, buf, 0, key, 0);
#endif /* REALITY_LEVELS */
	}

	if (!Wizard(player)) {
	    if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
		    notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	} else if (!Immortal(player)) {
	    if (Cloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Wizcloaked)"));
	    } else if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if (Dark(player)) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
            } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
                notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	} else {
	    if (Cloak(player)) {
		if (SCloak(player)) {
		    notify_except3(loc, player, player, player, 1,
				   strcat(buf, " (Supercloaked)"));
		} else {
		    notify_except3(loc, player, player, player, 0,
				   strcat(buf, " (Wizcloaked)"));
		}
	    } else if (NCloak(player)) {
		notify_except3(loc, player, player, player, 0,
			       strcat(buf, " (Cloaked)"));
	    } else if (Dark(player)) {
		notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Dark)"));
            } else if ( mudconf.blind_snuffs_cons && (Blind(player) || (Good_chk(loc) && Blind(loc))) ) {
                notify_except3(loc, player, player, player, 0,
				strcat(buf, " (Blind)"));
            }
	}

	free_mbuf(buf);

        if ( mudconf.partial_deconn ) {
           sprintf(s_timeon, "%ld", (mudstate.now - d->connected_at));
           argv[0] = (char *) reason;
           argv[1] = (char *) s_timeon;
           argv[2] = (char *) "1";
           atr_temp = atr_pget(player, A_ADISCONNECT, &aowner, &aflags);
           if (atr_temp && *atr_temp)
               wait_que(player, player, 0, NOTHING, atr_temp, argv, 3,
                        NULL, NULL);
           free_lbuf(atr_temp);
 
           /* DO GLOBAL ADISCONNECTS */
 
           if( Good_obj(mudconf.master_room) ) {
               if (mudconf.global_adconn) {
                   DOLIST(obj, Contents(mudconf.master_room)) {
#ifdef REALITY_LEVELS
                       if (!isPlayer(obj) && IsReal(obj,player)) {
#else
                       if (!isPlayer(obj)) {
#endif /* REALITY_LEVELS */
                           atr_temp = atr_pget(obj, A_ADISCONNECT,
                                               &aowner, &aflags);
                           if (atr_temp && *atr_temp && !God(player))
                               wait_que(obj, player, 0, NOTHING,
                                        atr_temp, argv, 3, NULL, NULL);
                           free_lbuf(atr_temp);
                       }
                   }
               }
           }   
       }
    }
    mudstate.curr_enactor = temp;
    desc_delhash(d);
    local_player_disconnect(player);
    VOIDRETURN; /* #131 */
}

int 
boot_off(dbref player, char *message)
{
    DESC *d, *dnext;
    int count;

    DPUSH; /* #132 */
    count = 0;
    DESC_SAFEITER_PLAYER(player, d, dnext) {
	if (message && *message) {
	    queue_string(d, message);
	    queue_string(d, "\r\n");
	}
	shutdownsock(d, R_BOOT);
	count++;
    }
    RETURN(count); /* #132 */
}

int 
boot_by_port(int port, int no_god, int who, char *message)
{
    DESC *d, *dnext;
    int count;
    char *buf;

    DPUSH; /* #133 */
    count = 0;
    DESC_SAFEITER_ALL(d, dnext) {
	if ((d->descriptor == port) && (!no_god || !God(d->player))) {
	    if ((d->player == who) || !DePriv(who, d->player, DP_BOOT, POWER6, NOTHING)) {
	      if (Controls(who,d->player)) {
		STARTLOG(LOG_WIZARD,"WIZ","BOOT")
		  buf = alloc_sbuf("do_boot.port");
		  sprintf(buf, "Port %d", port);
		  log_text(buf);
		  log_text((char *)" was @booted by ");
		  log_name(who);
		  free_sbuf(buf);
		ENDLOG
		if (message && *message) {
		    queue_string(d, message);
		    queue_string(d, "\r\n");
		}
		shutdownsock(d, R_BOOT);
		count++;
	      }
	    }
	}
    }
    RETURN(count); /* #133 */
}

/* ---------------------------------------------------------------------------
 * desc_reload: Reload parts of net descriptor that are based on db info.
 */

void 
desc_reload(dbref player)
{
    DESC *d;
    char *buf;
    dbref aowner;
    FLAG aflags;

    DPUSH; /* #134 */

    DESC_ITER_PLAYER(player, d) {
	buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags);
	if (buf) {
	    d->timeout = atoi(buf);
	    if (d->timeout <= 0 && d->timeout != -1)
		d->timeout = mudconf.idle_timeout;
	}
	free_lbuf(buf);
    }
    VOIDRETURN; /* #134 */
}

/* ---------------------------------------------------------------------------
 * fetch_idle, fetch_connect: Return smallest idle time/largest connec time
 * for a player (or -1 if not logged in)
 */

int 
fetch_idle(dbref target)
{
    DESC *d;
    int result, idletime;
   
    DPUSH; /* #135 */

    result = -1;
    DESC_ITER_PLAYER(target, d) {
	idletime = (mudstate.now - d->last_time);
	if ((result == -1) || (idletime < result))
	   result = idletime;
    }
    RETURN(result); /* #135 */
}

int 
fetch_connect(dbref target)
{
    DESC *d;
    int result, conntime;

    DPUSH; /* #136 */

    result = -1;
    DESC_ITER_PLAYER(target, d) {
	conntime = (mudstate.now - d->connected_at);
	if (conntime > result)
	    result = conntime;
    }
    RETURN(result); /* #136 */
}

void 
NDECL(check_idle)
{
    DESC *d, *dnext;
    time_t idletime;

    DPUSH; /* #137 */

    DESC_SAFEITER_ALL(d, dnext) {
	if (d->flags & DS_CONNECTED) {
            if ( KeepAlive(d->player) ) {
               /* Send NOP code to players to keep NAT/routers/firewalls happy */
               queue_string(d, "\377\361");
	       process_output(d);
            }
            if ( d->last_time > mudstate.now )
	       idletime = 0;
            else
	       idletime = mudstate.now - d->last_time;
	    if ((idletime > d->timeout && d->timeout != -1) && !Wizard(d->player)) {
		queue_string(d,
			     "*** Inactivity Timeout ***\r\n");
		shutdownsock(d, R_TIMEOUT);
            } else if (mudconf.idle_wiz_cloak &&
		       (idletime > d->timeout && d->timeout != -1) &&
		       Wizard(d->player) && (!Dark(d->player) || !Unfindable(d->player))) {
                if ( !Dark(d->player) ) {
		   s_Flags(d->player, Flags(d->player) | DARK);
		   d->flags |= DS_AUTODARK;
                }
                if ( !Unfindable(d->player) ) {
		   s_Flags2(d->player, Flags2(d->player) | UNFINDABLE);
		   d->flags |= DS_AUTOUNF;
                }
                if (mudconf.idle_message)
                   notify(d->player, "You have exceeded the timeout and been CLOAKED.");
	    } else if (mudconf.idle_wiz_dark &&
		       (idletime > d->timeout && d->timeout != -1) &&
		       Wizard(d->player) && !Dark(d->player)) {
		s_Flags(d->player, Flags(d->player) | DARK);
		d->flags |= DS_AUTODARK;
                if (mudconf.idle_message)
                   notify(d->player, "You have exceeded the timeout and set DARK.");
	    }
	} else {
            if ( (d->connected_at > mudstate.now) ||
                 ((d->connected_at + mudconf.conn_timeout + 60) < mudstate.now) )
               idletime = 0;
            else
	       idletime = mudstate.now - d->connected_at;
	    if (idletime > mudconf.conn_timeout) {
		queue_string(d,
			     "*** Login Timeout ***\r\n");
		shutdownsock(d, R_TIMEOUT);
	    }
	}
    }
    VOIDRETURN; /* #137 */
}

static void 
dump_users(DESC * e, char *match, int key)
{
    DESC *d;
    dbref aowner;
    int count, rcount = 0, i_attrpipe = 0, aflags, i_pipetype;
    time_t now;
    char *buf, *fp, *gp, *doingAnsiBuf, *doingAccentBuf, *pDoing, *atext, *atext2, *atextptr, *a_tee; 
#ifdef ZENTY_ANSI
    char *doingAnsiBufp, *abuf, *abufp, *msg_ns2, *mp2, *doingAccentBufp;
#endif
    char smallbuf[25];
    char antemp[20];
    char buf_format[80];
    char one_chr_holder = ' ';
    char *tpr_buff, *tprp_buff;
    ATTR *atr;

    DPUSH; /* #139 */
    memset(buf_format, 0, sizeof(buf_format));
    if ( Good_chk(e->player) && Fubar(e->player)) {
	notify(e->player, "You can't do that...");
	VOIDRETURN; /* #139 */
    }
    while (match && *match && isspace((int)*match))
	match++;
    if (!match || !*match)
	match = NULL;

    strcpy(antemp, ANSI_NORMAL);
    buf = alloc_lbuf("dump_users");
    doingAnsiBuf = alloc_lbuf("dump_users_ansi");
    doingAccentBuf = alloc_lbuf("dump_users_accents");
    tprp_buff = tpr_buff = alloc_lbuf("dump_users_tprintf");

    i_pipetype = 1;
    if ( Good_chk(e->player) && H_Attrpipe(e->player) ) {
       atr = atr_str3("___ATTRPIPE");
       if ( atr ) {
          atext2 = atr_get(e->player, atr->number, &aowner, &aflags);
          if ( *atext2 ) {
             if ( (a_tee = strchr(atext2, ' ')) != NULL ) {
                *a_tee = '\0';
                i_pipetype = atoi(a_tee+1);
             }
             atr = atr_str3(atext2);
             if ( atr ) {
                if ( Controlsforattr(e->player, e->player, atr, aflags) ) {
                   i_attrpipe = 1;
                }
             }
          }
          free_lbuf(atext2);
          if ( i_attrpipe ) {
             atext2 = atr_get(e->player, atr->number, &aowner, &aflags);
             atextptr = atext = alloc_lbuf("dump_users_pipe");
             safe_str(atext2, atext, &atextptr);
             free_lbuf(atext2);
          }
       }
       if ( !i_attrpipe ) {
          queue_string(e, (char *)"Notice: Piping attribute no longer exists.  Piping has been auto-disabled.\r\n");
          s_Flags4(e->player, Flags4(e->player) & ~HAS_ATTRPIPE);
          atr = atr_str3("___ATTRPIPE");
          if ( atr ) {
             atr_clr(e->player, atr->number);
          }
       }
    }
    time(&now);

    if (key == CMD_SESSION) {
        if ( i_attrpipe ) {
           safe_str((char *)"                                  ", atext, &atextptr);
           safe_str((char *)"Characters Input----  Characters Output---\r\n", atext, &atextptr);
           safe_str((char *)"Player Name          #Cmnds Port  ", atext, &atextptr);
        } 
        if ( i_pipetype ) {
	   queue_string(e, "                                  ");
	   queue_string(e, "Characters Input----  Characters Output---\r\n");
           queue_string(e, "Player Name          #Cmnds Port  ");
        }
    }
    else
#ifdef PARIS
      if ( i_attrpipe ) {
         safe_str((char *)"Player Name            On For Idle  ", atext, &atextptr);
      } 
      if ( i_pipetype ) {
         queue_string(e, "Player Name            On For Idle  ");
      }
#else
      if ( i_attrpipe ) {
         safe_str((char *)"Player Name          On For Idle  ", atext, &atextptr);
      } 
      if ( i_pipetype ) {
         queue_string(e, "Player Name          On For Idle  ");
      }
#endif
    if (key == CMD_SESSION) {
        if ( i_attrpipe ) {
           safe_str((char *)"Pend  Lost     Total  Pend  Lost     Total\r\n", atext, &atextptr);
        } 
        if ( i_pipetype ) {
	   queue_string(e, "Pend  Lost     Total  Pend  Lost     Total\r\n");
        }
    } else if ((e->flags & DS_CONNECTED) && (Wizard(e->player) || HasPriv(e->player, NOTHING, POWER_WIZ_WHO, POWER3, POWER_LEVEL_NA))
	&& (!DePriv(e->player, NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF))
	       && (key == CMD_WHO)) {
#ifdef PARIS
        if ( i_attrpipe ) {
           safe_str((char *)"Room      Cmds Host\r\n", atext, &atextptr);
        } 
        if ( i_pipetype ) {
           queue_string(e, "Room      Cmds Host\r\n");
        }
#else
        if ( i_attrpipe ) {
           safe_str((char *)"Room     Ports Host\r\n", atext, &atextptr);
        } 
        if ( i_pipetype ) {
	   queue_string(e, "Room     Ports Host\r\n");
        }
#endif
    } else {
	if (mudconf.who_default) {
#ifdef ZENTY_ANSI
           abufp = abuf = alloc_lbuf("doing_header");
           mp2 = msg_ns2 = alloc_lbuf("notify_check_accents");
	   parse_ansi(mudstate.ng_doing_hdr, abuf, &abufp, msg_ns2, &mp2);
           *mp2 = '\0';
           if ( Accents(e->player) ) {
              if ( i_attrpipe ) {
                 safe_str(msg_ns2, atext, &atextptr);
              } 
              if ( i_pipetype ) {
	         queue_string(e, msg_ns2);
              }
           } else {
              if ( i_attrpipe ) {
                 safe_str(strip_safe_accents(abuf), atext, &atextptr);
              } 
              if ( i_pipetype ) {
	         queue_string(e, strip_safe_accents(abuf));
              }
           }
           free_lbuf(msg_ns2);
           free_lbuf(abuf);
#else
           if ( i_attrpipe ) {
              safe_str(mudstate.ng_doing_hdr, atext, &atextptr);
           } 
           if ( i_pipetype ) {
	      queue_string(e, mudstate.ng_doing_hdr);
           }
#endif
	} else  {
           tprp_buff = tpr_buff;
#ifdef ZENTY_ANSI
           abufp = abuf = alloc_lbuf("doing_header");
           mp2 = msg_ns2 = alloc_lbuf("notify_check_accents");
	   parse_ansi(mudstate.doing_hdr, abuf, &abufp, msg_ns2, &mp2);
           *mp2 = '\0';
           if ( Accents(e->player) ) {
              if ( i_attrpipe ) {
                 safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", mudstate.guild_hdr, msg_ns2),
                          atext, &atextptr);
              } 
              if ( i_pipetype ) {
	         queue_string(e, safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", 
                              mudstate.guild_hdr, msg_ns2));
              }
           } else {
              if ( i_attrpipe ) {
                 safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", mudstate.guild_hdr, strip_safe_accents(abuf)),
                          atext, &atextptr);
              } 
              if ( i_pipetype ) {
	         queue_string(e, safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", 
                              mudstate.guild_hdr, strip_safe_accents(abuf)));
              }
           }
           free_lbuf(msg_ns2);
           free_lbuf(abuf);
#else
           if ( i_attrpipe ) {
              safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", mudstate.guild_hdr,mudstate.doing_hdr),
                       atext, &atextptr);
           } 
           if ( i_pipetype ) {
	      queue_string(e, safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s",
                           mudstate.guild_hdr,mudstate.doing_hdr));
           }
#endif
        }
        if ( i_attrpipe ) {
           safe_str("\r\n", atext, &atextptr);
        } 
        if ( i_pipetype ) {
	   queue_string(e, "\r\n");
        }
    }
    count = 0;
    DESC_ITER_CONN(d) {
	rcount++;

#ifdef ZENTY_ANSI
	doingAnsiBufp = doingAnsiBuf;
	doingAccentBufp = doingAccentBuf;
	parse_ansi(d->doing, doingAnsiBuf, &doingAnsiBufp, doingAccentBuf, &doingAccentBufp);
        if ( !Accents(e->player) ) {
           strcpy(doingAccentBuf, strip_safe_accents(doingAnsiBuf));
        }
	pDoing = doingAccentBuf;
#else
	pDoing = d->doing;
#endif

	if (Cloak(d->player) && rcount)
	    rcount--;
        else if ((Dark(d->player) && !mudconf.who_unfindable && !mudconf.player_dark && rcount && 
                  mudconf.allow_whodark))
	    rcount--;
        else if (NoWho(d->player) && rcount && !(d->player == e->player || Wizard(e->player)) && rcount)
            rcount--;
        else if (NoWho(d->player) && !(e->flags & DS_CONNECTED) && rcount)
            rcount--;

        /* Rewrote this big honking if() because it was an eye sore and pissed me off -- Ash */
	if ( ( !(Unfindable(d->player) && mudconf.who_unfindable) || 
               (Wizard(e->player) && (e->flags & DS_CONNECTED)) || 
               (Unfindable(d->player) && HasPriv(e->player,d->player,POWER_WHO_UNFIND,POWER4,NOTHING)) ||
	        HasPriv(e->player, d->player, POWER_WIZ_WHO, POWER3, NOTHING) ||
               (!mudconf.who_unfindable && mudconf.player_dark) ||
	       (Admin(e->player) && (e->flags & DS_CONNECTED) && !Wizard(d->player)) 
             ) &&
             !( Dark(d->player) && 
                !mudconf.who_unfindable && 
                !mudconf.player_dark && 
                !(Admin(e->player) && (e->flags & DS_CONNECTED)) && 
                mudconf.allow_whodark
              ) ) {
	    if (!Unfindable(d->player))
		count++;
	    if (match && !(string_prefix(Name(d->player), match)))
		continue;
	    if ((key == CMD_SESSION) && !Wizard(e->player) &&
		!Admin(e->player) && !Builder(e->player) &&
		(d->player != e->player))
		continue;
	    if ((key == CMD_SESSION) && !Wizard(e->player) &&
		Wizard(d->player))
		continue;
	    if ((key == CMD_SESSION) && !Admin(e->player) &&
		Admin(d->player))
		continue;
	    if (SCloak(d->player) && Dark(d->player) && Unfindable(d->player) && !Immortal(e->player))
		continue;
	    if (Cloak(d->player) && !Wizard(e->player))
		continue;
	    if ((Cloak(d->player) || NoWho(d->player)) && !(e->flags & DS_CONNECTED))
		continue;
            if (NoWho(d->player) && (d->player != e->player) &&
                !(Wizard(e->player) || HasPriv(e->player, d->player, POWER_WIZ_WHO, POWER3, NOTHING)))
                continue;
            if (NoWho(d->player) && !(e->flags & DS_CONNECTED))
                continue;
            if (Dark(d->player) && !mudconf.who_unfindable && !mudconf.player_dark && 
                !(e->flags & DS_CONNECTED) && mudconf.allow_whodark)
                continue;
	    if ((e->flags & DS_CONNECTED) &&
		(Wizard(e->player) || HasPriv(e->player, d->player, POWER_WIZ_WHO, POWER3, NOTHING)) &&
		(!DePriv(e->player, d->player, DP_WIZ_WHO, POWER6, NOTHING) || (e->player == d->player)) &&
		(!DePriv(e->player, NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF)) &&
		(key == CMD_WHO)) {
                if ( mudconf.whohost_size <= 0 || mudconf.whohost_size > 28 )
#ifdef PARIS
                   sprintf( buf_format, "%%-16.16s   %%10s %%4s  #%%-6d %%6d [%%s]\r\n");
#else
                   sprintf( buf_format, "%%-14.14s   %%10s %%4s  #%%-6d %%6d [%%s]\r\n");
#endif
                else
#ifdef PARIS
                   sprintf( buf_format, "%%-16.16s   %%10s %%4s  #%%-6d %%6d [%%.%ds]\r\n", mudconf.whohost_size);
#else
                   sprintf( buf_format, "%%-14.14s   %%10s %%4s  #%%-6d %%6d [%%.%ds]\r\n", mudconf.whohost_size);
#endif
		sprintf(buf,
                        buf_format,
			Name(d->player),
			time_format_1(now - d->connected_at),
			time_format_2(now - d->last_time),
#ifdef PARIS
                        Location(d->player), d->command_count,
#else
			Location(d->player), d->descriptor,
#endif
			d->addr);

#ifdef PARIS
		fp = &buf[16];
#else
		fp = &buf[14];
#endif

                if ( !(HasPriv(d->player, e->player, POWER_HIDEBIT, POWER5, NOTHING) && 
                       (d->player != e->player || mudstate.objevalst) && (e->flags & DS_CONNECTED))) {
                   if (mudconf.who_showwiz && Guildmaster(d->player) ) {
                      if ( Immortal(d->player) ||
                           ( Wizard(d->player) && mudconf.who_wizlevel < 5 ) ||
                           ( Admin(d->player) && mudconf.who_wizlevel < 4 ) ||
                           ( Builder(d->player) && mudconf.who_wizlevel < 3 ) ||
                           ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 ) )
                          *fp++ = '*';
                   }
                   else if (mudconf.who_showwiztype ) {
                      if ( Immortal(d->player) )
                         *fp++ = 'W';
                      else if ( Wizard(d->player) && mudconf.who_wizlevel < 5 )
                         *fp++ = 'W';
                      else if ( Admin(d->player) && mudconf.who_wizlevel < 4 )
                         *fp++ = 'a';
                      else if ( Builder(d->player) && mudconf.who_wizlevel < 3 )
                         *fp++ = 'B';
                      else if ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 )
                         *fp++ = 'g';
                   }
                }
		if (Dark(d->player) && (e->flags & DS_CONNECTED)) {
		    if (d->flags & DS_AUTODARK)
			*fp++ = 'd';
		    else
			*fp++ = 'D';
		}
		if (Unfindable(d->player) && (e->flags & DS_CONNECTED)) {
		  if (d->flags & DS_AUTOUNF) {
		    *fp++ = 'h';
		  } else {
		    *fp++ = 'H';
		  }
		}
                if ( NoWho(d->player) && (e->flags & DS_CONNECTED) )
                    *fp++ = 'U';
		if (Suspect(d->player) && (Wizard(e->player) || SeeSuspect(e->player)) && 
                    (e->flags & DS_CONNECTED))
		    *fp++ = '+';
		if ((d->host_info & H_FORBIDDEN) && Wizard(e->player) && (e->flags & DS_CONNECTED))
		    *fp++ = 'F';
		if (d->host_info & H_REGISTRATION && (e->flags & DS_CONNECTED) && 
                    !mudconf.noregist_onwho)
		    *fp++ = 'R';
		if ((d->host_info & H_SUSPECT) && (Wizard(e->player) || SeeSuspect(e->player)) && 
		    (e->flags & DS_CONNECTED))
		    *fp++ = '+';
	    } else if (key == CMD_SESSION) {
		sprintf(buf,
			"%-16.16s %10d %4d  %4d%6d%10d%6d%6d%10d\r\n",
			Name(d->player),
			d->command_count,
			d->descriptor,
			d->input_size, d->input_lost,
			d->input_tot,
			d->output_size, d->output_lost,
			d->output_tot);
	    } else {
		if ((key == CMD_WHO) && ((HasPriv(e->player, NOTHING, POWER_WIZ_WHO, POWER3, POWER_LEVEL_NA)) ||
					 (Wizard(e->player) && DePriv(e->player, NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_NA))) &&
		    !DePriv(e->player, NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF))
		    continue;
		if (mudconf.who_default) {
#ifdef PARIS
                  sprintf(buf, "%-16.16s   %10s %4s  %-s\r\n",
#else
		  sprintf(buf, "%-14.14s   %10s %4s  %-s\r\n",
#endif
			Name(d->player),
			time_format_1(now - d->connected_at),
			time_format_2(now - d->last_time),
			pDoing);
		}
		else {
		  gp = Guild(d->player);
		  if (!*gp)
		    gp = mudconf.guild_default;
#ifdef PARIS
                  sprintf(buf, "%-16.16s   %10s %4s  %-11.11s %-s\r\n",
#else
		  sprintf(buf, "%-14.14s   %10s %4s  %-11.11s %-s\r\n",
#endif
			Name(d->player),
			time_format_1(now - d->connected_at),
			time_format_2(now - d->last_time),
			gp, pDoing);
		}
		if (Admin(e->player) || HasPriv(e->player, d->player, POWER_WIZ_WHO, POWER3, NOTHING)) {
#ifdef PARIS
                    fp = &buf[16];
#else
                    fp = &buf[14];
#endif
                    if ( !(HasPriv(d->player, e->player, POWER_HIDEBIT, POWER5, NOTHING) &&
                           (d->player != e->player || mudstate.objevalst)) && (e->flags & DS_CONNECTED)) {
                        if (mudconf.who_showwiz && Guildmaster(d->player) ) {
                           if ( Immortal(d->player) ||
                                ( Wizard(d->player) && mudconf.who_wizlevel < 5 ) ||
                                ( Admin(d->player) && mudconf.who_wizlevel < 4 ) ||
                                ( Builder(d->player) && mudconf.who_wizlevel < 3 ) ||
                                ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 ) )
                               *fp++ = '*';
                        }
                        else if (mudconf.who_showwiztype ) {
                           if ( Immortal(d->player) )
                              *fp++ = 'W';
                           else if ( Wizard(d->player) && mudconf.who_wizlevel < 5 )
                              *fp++ = 'W';
                           else if ( Admin(d->player) && mudconf.who_wizlevel < 4 )
                              *fp++ = 'a';
                           else if ( Builder(d->player) && mudconf.who_wizlevel < 3 )
                              *fp++ = 'B';
                           else if ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 )
                              *fp++ = 'g';
                        }
                    }
		    if (Dark(d->player) && (e->flags & DS_CONNECTED)) {
			if (d->flags & DS_AUTODARK)
			    *fp++ = 'd';
			else
			    *fp++ = 'D';
		    }
		    if (Unfindable(d->player) && (e->flags & DS_CONNECTED)) {
		        if (d->flags & DS_AUTOUNF)
			    *fp++ = 'h';
		        else
			    *fp++ = 'H';
		    }
                    if ( NoWho(d->player) && (e->flags & DS_CONNECTED) )
                        *fp++ = 'U';
		    if (Suspect(d->player) && (Wizard(e->player) || SeeSuspect(e->player)) && 
                       (e->flags & DS_CONNECTED))
			*fp++ = '+';
		    if ((d->host_info & H_FORBIDDEN) && Wizard(e->player) && (e->flags & DS_CONNECTED))
			*fp++ = 'F';
		    if (d->host_info & H_REGISTRATION && (e->flags & DS_CONNECTED) &&
                        !mudconf.noregist_onwho)
			*fp++ = 'R';
		    if ((d->host_info & H_SUSPECT) && (Wizard(e->player) || SeeSuspect(e->player)) && 
                        (e->flags & DS_CONNECTED))
			*fp++ = '+';
		}
		else if (HasPriv(e->player, d->player, POWER_WHO_UNFIND, POWER4, NOTHING) && 
				(e->flags & DS_CONNECTED))
		   one_chr_holder = 'H';
#ifdef PARIS
                fp = &buf[16];
#else
                fp = &buf[14];
#endif
                if ( !(HasPriv(d->player, e->player, POWER_HIDEBIT, POWER5, NOTHING) &&
                       (d->player != e->player || mudstate.objevalst)) && (e->flags & DS_CONNECTED)) {
                   if (mudconf.who_showwiz && Guildmaster(d->player) ) {
                      if ( Immortal(d->player) ||
                           ( Wizard(d->player) && mudconf.who_wizlevel < 5 ) ||
                           ( Admin(d->player) && mudconf.who_wizlevel < 4 ) ||
                           ( Builder(d->player) && mudconf.who_wizlevel < 3 ) ||
                           ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 ) )
                          *fp++ = '*';
                   }
                   else if (mudconf.who_showwiztype ) {
                      if ( Immortal(d->player) )
                         *fp++ = 'W';
                      else if ( Wizard(d->player) && mudconf.who_wizlevel < 5 )
                         *fp++ = 'W';
                      else if ( Admin(d->player) && mudconf.who_wizlevel < 4 )
                         *fp++ = 'a';
                      else if ( Builder(d->player) && mudconf.who_wizlevel < 3 )
                         *fp++ = 'B';
                      else if ( Guildmaster(d->player) && mudconf.who_wizlevel < 2 )
                         *fp++ = 'g';
                   }
                }
		if (!(Admin(e->player) || HasPriv(e->player, d->player, POWER_WIZ_WHO, POWER3, NOTHING))) {
		   if (Suspect(d->player) && (Wizard(e->player) || SeeSuspect(e->player)) && 
                      (e->flags & DS_CONNECTED))
	              *fp++ = '+';
		}
                if ( one_chr_holder == 'H' && (e->flags & DS_CONNECTED))
                   *fp++ = one_chr_holder;
                one_chr_holder=' ';
	    }
            if ( i_attrpipe ) {
               safe_str(buf, atext, &atextptr);
            } 
            if ( i_pipetype ) {
	       queue_string(e, buf);
            }
	    if (ShowAnsi(e->player) && index(buf, ESC_CHAR)) {
                if ( i_attrpipe ) {
                   safe_str(antemp, atext, &atextptr);
                } 
                if ( i_pipetype ) {
		   queue_string(e, antemp);
                }
            }
	}
    }

    /* sometimes I like the ternary operator.... */

    
    if ( mudconf.who_comment ) {
       if (rcount < 10)
	   strcpy(smallbuf, "Bummer.");
       else if (rcount > 9 && rcount < 16)
	   strcpy(smallbuf, "Not bad.");
       else if (rcount > 15 && rcount < 20)
	   strcpy(smallbuf, "Getting there.");
       else if (rcount > 19 && rcount < 26)
	   strcpy(smallbuf, "Yea!");
       else
	   strcpy(smallbuf, "Fantastic!");

       sprintf(buf, "%d Player%slogged in. (%s)\r\n", rcount,
   	       (rcount == 1) ? " " : "s ", smallbuf);
    }
    else
       sprintf(buf, "%d Player%slogged in.\r\n", rcount,
   	       (rcount == 1) ? " " : "s ");

    if ( i_attrpipe ) {
       safe_str(buf, atext, &atextptr);
    } 
    if ( i_pipetype ) {
       queue_string(e, buf);
    }
    if ((rcount - count > 0) && mudconf.who_unfindable) {
	sprintf(buf, "%d Player%s hidden.\r\n", rcount - count,
		(rcount - count == 1) ? " is" : "s are");
        if ( i_attrpipe ) {
           safe_str(buf, atext, &atextptr);
        } else {
	   queue_string(e, buf);
        }
    }
    if ( NoWho(e->player) ) {
       if ( i_attrpipe ) {
          safe_str(buf, atext, &atextptr);
       } 
       if ( i_pipetype ) {
          queue_string(e, "You are @hidden from the WHO.\r\n");
       }
    }
    free_lbuf(buf);
    free_lbuf(doingAnsiBuf);
    free_lbuf(doingAccentBuf);
    free_lbuf(tpr_buff);
    if ( i_attrpipe ) {
       atr_add_raw(e->player, atr->number, atext);
       if ( TogNoisy(e->player) && !i_pipetype )
          queue_string(e, "Piping output to attribute.\r\n");
       free_lbuf(atext);
    }
    VOIDRETURN; /* #139 */
}
/* ---------------------------------------------------------------------------
 * do_mudwho: Special WHO for Howard's intermud paging/who server.
 * Original code by Howard.
 */

void 
do_mudwho(dbref player, dbref cause, int key, char *name, char *mud)
{
    DESC *d;
    int players;
    time_t now;

    DPUSH; /* #140 */

    time(&now);
    players = 0;
#ifdef PARIS
      notify(player,
           unsafe_tprintf("@inwho %s@%s=Player                 On For Idle %s",
                   name, mud, mudstate.doing_hdr));
#else
    notify(player,
	   unsafe_tprintf("@inwho %s@%s=Player               On For Idle %s",
		   name, mud, mudstate.doing_hdr));
#endif
    DESC_ITER_CONN(d) {
        if (Dark(d->player) && !mudconf.who_unfindable && !mudconf.player_dark &&
                mudconf.allow_whodark)
                continue;
	if (!(Unfindable(d->player) && mudconf.who_unfindable)) {
	    notify(player,
		   unsafe_tprintf("@inwho %s@%s=%-16.16s %10s %4s %s",
			   name, mud,
			   Name(d->player),
			   time_format_1(now - d->connected_at),
			   time_format_2(now - d->last_time),
			   d->doing ? d->doing : ""));
	    players++;
	}
    }
    notify(player,
	   unsafe_tprintf("@inwho %s@%s=%d player%s %s connected.",
		   name, mud, players, (players == 1) ? "" : "s",
		   (players == 1) ? "is" : "are"));
    VOIDRETURN; /* #140 */
}


/* ---------------------------------------------------------------------------
 * Idea from Nyctasia@Rhostshyl.
 */

void
do_uptime(dbref player, dbref cause, int key)
{
  time_t now;
  char *buff;
  int i_syear, i_ryear;
  char *s_uptime;
  struct utmpx *ut;
/*
  FILE *fp;
  char *s_chk;
*/

  DPUSH; /* #141 */
  
  time(&now);

  buff = alloc_mbuf("uptime");
  s_uptime = alloc_mbuf("uptime_sys");

  /* Open the command for reading. */
/*
  fp = popen("/usr/bin/uptime", "r");
  if (fp != NULL) {
     memset(s_uptime, '\0', MBUF_SIZE);
     fgets(s_uptime, MBUF_SIZE - 1, fp);
     s_chk = strchr(s_uptime, ',');
     if ( s_chk ) {
        s_chk = strchr(s_chk+1, ',');
        if ( s_chk )
           *s_chk = '\0';
        else
           memset(s_uptime, '\0', MBUF_SIZE);
     } else {
        memset(s_uptime, '\0', MBUF_SIZE);
     }
     pclose(fp);
  }
*/

  setutxent();
  ut = getutxent();
  while ( ut && (ut->ut_type != BOOT_TIME) )
     ut = getutxent();

  memset(s_uptime, '\0', MBUF_SIZE);
  if ( ut ) {
     strcpy(s_uptime, time_format_1(mudstate.now - ut->ut_tv.tv_sec));
  }
  endutxent();

  strcpy(buff,time_format_1(now - mudstate.reboot_time));
  i_syear = ((mudstate.now - mudstate.start_time) / 31536000);
  i_ryear = ((mudstate.now - mudstate.reboot_time) / 31536000);
  if ( i_syear || i_ryear ) {
    notify(player, unsafe_tprintf("%s has been up for %dy %s, Reboot: %dy %s",
			   mudconf.mud_name, i_syear,
			   time_format_1(now - mudstate.start_time), i_ryear, buff));
    if ( *s_uptime )
       notify(player, unsafe_tprintf("System Uptime: %s", s_uptime));
  } else {
    notify(player, unsafe_tprintf("%s has been up for %s, Reboot: %s",
			   mudconf.mud_name,
			   time_format_1(now - mudstate.start_time), buff));
    if ( *s_uptime )
       notify(player, unsafe_tprintf("System Uptime: %s", s_uptime));
  }
  free_mbuf(buff);
  free_mbuf(s_uptime);
  VOIDRETURN; /* #141 */
}

int 
len_noansi(char *buff)
{
    char *pt1;
    int count;

    DPUSH; /* #142 */

    count = 0;
    pt1 = strip_safe_ansi(buff);
    while (*pt1) {
	if (*pt1 == ESC_CHAR)
	    while (*pt1++ != 'm');
	else {
	    pt1++;
	    count++;
	}
    }
    RETURN(count); /* #142 */
}

/* ---------------------------------------------------------------------------
 * do_doing: Set the doing string that appears in the WHO report.
 * Idea from R'nice@TinyTIM.
 */

void 
do_doing(dbref player, dbref cause, int key, char *arg)
{
    DESC *d = NULL, 
         *e = NULL;
    char *c, *pt1, *pt2;
    int foundany, over, copylen, addlen, gotone;
    time_t now, dtime;

    DPUSH; /* #143 */

    if (mudconf.who_default)
#ifdef PARIS
	addlen = 15;
#else
	addlen = 12;
#endif
    else
	addlen = 0;
    if ((key == DOING_MESSAGE) || (key == DOING_UNIQUE)) {
	foundany = 0;
	over = 0;
	pt2 = alloc_lbuf("do_doing");
	c = pt2;
	pt1 = arg;
	while (*pt1) {
	    if ((*pt1 != '\r') && (*pt1 != '\n') && (*pt1 != '\t'))
		*c++ = *pt1++;
	    else
		pt1++;
	}
	*c = '\0';
	if (key == DOING_MESSAGE) {
	  DESC_ITER_PLAYER(player, d) {
	    c = d->doing;
	    if (len_noansi(pt2) > (DOING_SIZE + addlen)) {
		pt1 = strip_all_special(pt2);
		copylen = DOING_SIZE + addlen;
		notify(player, "Warning: ansi codes stripped.");
	    } else {
		copylen = strlen(pt2);
		if (copylen > 255) {
		    pt1 = strip_ansi(pt2);
		    copylen = DOING_SIZE + addlen;
		    notify(player, "Warning: ansi codes stripped.");
		} else 
		    pt1 = pt2;
	    }
	    over = safe_copy_str(pt1, d->doing, &c, copylen);
	    *c = '\0';
	    foundany = 1;
	  }
	}
	else {
	  time(&now);
	  gotone = 0;
	  dtime = 0;
	  DESC_ITER_PLAYER(player, d) {
	    if (!gotone || (now - d->last_time) < dtime) {
	      e = d;
	      dtime = now - d->last_time;
	      gotone= 1;
	    }
	  }
	  if (gotone > 0) {
	    c = e->doing;
	    if (len_noansi(pt2) > (DOING_SIZE + addlen)) {
		pt1 = strip_ansi(pt2);
		copylen = DOING_SIZE + addlen;
		notify(player, "Warning: ansi codes stripped.");
	    } else {
		copylen = strlen(pt2);
		if (copylen > 255) {
		    pt1 = strip_ansi(pt2);
		    copylen = DOING_SIZE + addlen;
		    notify(player, "Warning: ansi codes stripped.");
		} else 
		    pt1 = pt2;
	    }
	    over = safe_copy_str(pt1, e->doing, &c, copylen);
	    *c = '\0';
	    foundany = 1;
	  }
	}
	free_lbuf(pt2);
	if (foundany) {
	    if (over) {
		notify(player,
		       unsafe_tprintf("Warning: %d characters lost.",
			       over));
	    }
	    if (!Quiet(player))
		notify(player, "Set.");
	} else {
	    notify(player, "Not connected.");
	}
    } else if (key == DOING_HEADER) {
	if (!arg || !*arg) {
	  if (mudconf.who_default)
	    strcpy(mudstate.ng_doing_hdr, "Doing");
	  else
	    strcpy(mudstate.doing_hdr, "Doing");
	  over = 0;
	} else {
	    if (mudconf.who_default)
	      c = mudstate.ng_doing_hdr;
	    else
	      c = mudstate.doing_hdr;
	    if (len_noansi(arg) > (DOING_SIZE + addlen)) {
		pt1 = strip_ansi(arg);
		copylen = DOING_SIZE + addlen;
		notify(player, "Warning: ansi codes stripped.");
	    } else {
		copylen = strlen(arg);
		if (copylen > 69) {
		    pt1 = strip_ansi(arg);
		    copylen = DOING_SIZE + addlen;
		    notify(player, "Warning: ansi codes stripped.");
		} else
		    pt1 = arg;
	    }
	    if (mudconf.who_default)
	      over = safe_copy_str(pt1, mudstate.ng_doing_hdr, &c, copylen);
	    else
	      over = safe_copy_str(pt1, mudstate.doing_hdr, &c, copylen);
	    *c = '\0';
	}
	if (over) {
	    notify(player,
		   unsafe_tprintf("Warning: %d characters lost.", over));
	}
	if (!Quiet(player))
	    notify(player, "Set.");
    } else {
	if (mudconf.who_default)
	  notify(player, unsafe_tprintf("Poll: %s", mudstate.ng_doing_hdr));
	else
	  notify(player, unsafe_tprintf("Poll: %s", mudstate.doing_hdr));
    }
    VOIDRETURN; /* #143 */
}

NAMETAB logout_cmdtable[] =
{
    {(char *) "DOING", 5, CA_PUBLIC, 0, CMD_DOING},
    {(char *) "LOGOUT", 6, CA_PUBLIC, 0, CMD_LOGOUT},
    {(char *) "OUTPUTPREFIX", 12, CA_PUBLIC, 0, CMD_PREFIX | CMD_NOxFIX},
    {(char *) "OUTPUTSUFFIX", 12, CA_PUBLIC, 0, CMD_SUFFIX | CMD_NOxFIX},
    {(char *) "QUIT", 4, CA_PUBLIC, 0, CMD_QUIT},
#ifdef LOCAL_RWHO_SERVER
    {(char *) "RWHO", 4, CA_PUBLIC, 0, CMD_RWHO},
#endif
    {(char *) "SESSION", 7, CA_PUBLIC, 0, CMD_SESSION},
    {(char *) "WHO", 3, CA_PUBLIC, 0, CMD_WHO},
    {(char *) "GET", 3, CA_PUBLIC, 0, CMD_GET},
    {(char *) "INFO", 4, CA_PUBLIC, 0, CMD_INFO},
    {NULL, 0, 0, 0, 0}};

void 
NDECL(init_logout_cmdtab)
{
    NAMETAB *cp;

    DPUSH; /* #144 */

    /* Make the htab bigger than the number of entries so that we find
     * things on the first check.  Remember that the admin can add aliases.
     */

    hashinit(&mudstate.logout_cmd_htab, 19);
    for (cp = logout_cmdtable; cp->flag; cp++)
	hashadd2(cp->name, (int *) cp, &mudstate.logout_cmd_htab, 1);
    VOIDRETURN; /* #144 */
}

static void 
failconn(const char *logcode, const char *logtype,
	 const char *logreason, DESC * d, int disconnect_reason,
	 dbref player, int filecache, char *motd_msg, char *command,
	 char *user, char *password, char *cmdsave)
{
    char *buff;

    DPUSH; /* #145 */

    STARTLOG(LOG_LOGIN | LOG_SECURITY, logcode, "RJCT")
	buff = alloc_mbuf("failconn.LOG");
    sprintf(buff, "[%d/%s] %s rejected to ",
	    d->descriptor, d->addr, logtype);
    log_text(buff);
    free_mbuf(buff);
    if (player != NOTHING)
	log_name(player);
    else
	log_text(user);
    log_text((char *) " (");
    log_text((char *) logreason);
    log_text((char *) ")");
    ENDLOG
	fcache_dump(d, filecache);
    if (motd_msg && *motd_msg) {
	queue_string(d, motd_msg);
	queue_write(d, "\r\n", 2);
    }
    free_mbuf(command);
    free_mbuf(user);
    free_mbuf(password);
    shutdownsock(d, disconnect_reason);
    mudstate.debug_cmd = cmdsave;
    VOIDRETURN; /* #145 */
}

static const char *connect_fail =
"Either that player does not exist, or has a different password.\r\n";
static const char *create_fail =
"Either there is already a player with that name, or that name is illegal.\r\n";

static int 
check_connect(DESC * d, const char *msg)
{
    char *command, *user, *password, *buff, *cmdsave, *buff3, *addroutbuf, *tsite_buff;
    dbref player, aowner, player2;
    int aflags, nplayers, comptest, gnum, bittemp, postest, overf, dc, tchar_num, is_guest;
    int ok_to_login, i_sitemax;
    DESC *d2, *d3;
    char buff2[10], *in_tchr, tchar_buffer[600];

    DPUSH; /* #146 */

    bittemp = 0;
    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< check_connect >";

    /* Hide the password length from SESSION */

    d->input_tot -= (strlen(msg) + 1);

    /* Crack the command apart */

    command = alloc_mbuf("check_conn.cmd");
    user = alloc_mbuf("check_conn.user");
    password = alloc_mbuf("check_conn.pass");
    overf = parse_connect(msg, command, user, password);
    if ( strlen(user) > 120 )
       overf = 0;
    if (!overf) {
	queue_string(d,"Your attempt to crash this mush has been noted and logged.\r\n");
	STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "OVERFLOW")
	  *(((char *)msg)+LBUF_SIZE-MBUF_SIZE-1) = '\0';
	  buff = alloc_lbuf("check_conn.LOG.over");
	  sprintf(buff, "[%d/%s] Attempted overflow -> %.3800s",
		d->descriptor, d->addr, msg);
	  log_text(buff);
	  free_lbuf(buff);
	ENDLOG
	broadcast_monitor(NOTHING, MF_CONN, "ATTEMPTED OVERFLOW", d->userid, d->addr, d->descriptor, 0, 0, NULL);
	free_mbuf(command);
	free_mbuf(user);
	free_mbuf(password);
	shutdownsock(d, R_HACKER);
	mudstate.debug_cmd = cmdsave;
	RETURN(0); /* #146 */
    }

/* PUT IN TOR PROTECTION HERE FOR CO, CD, CR and REG */

/*
    STARTLOG(LOG_CONNECT, "CMD", "BAD")
       buff = alloc_lbuf("check_conn.LOG.bad");
       sprintf(buff, "[%d/%s] On connect screen -> %.3900s",
               d->descriptor, d->addr, msg);
       log_text(buff);
       free_lbuf(buff);
    ENDLOG
*/

    /* Guest determination */
    strncpy(buff2, user, 5);
    *(buff2 + 5) = '\0';
    if (!strncmp(command, "co", 2)) {
	comptest = stricmp(buff2, "guest");
    } else {
	comptest = 1;
    }
    player2 = lookup_player(NOTHING, user, 0);
    is_guest = 0;
    if ( player2 != NOTHING && Guest(player2) )
       is_guest = 1;
    if ( (!comptest && ((strlen(user) > 5) || (strcmp(password, "guest")))) ||
         (is_guest && !strcmp(password, "guest") && comptest) ) {
	queue_string(d, "Use 'co guest guest' to connect to a guest character.\r\n");
	STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
	    buff = alloc_mbuf("check_conn.LOG.bad");
	sprintf(buff, "[%d/%s] Failed connect to '%s'",
		d->descriptor, d->addr, user);
	log_text(buff);
	free_mbuf(buff);
	ENDLOG
        if (player2 == NOTHING) {
		broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD GUEST CONNECT)", 
                                  d->userid, d->addr, 0, 0, 0, user);
        } else {
		broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD GUEST CONNECT)", 
                                  d->userid, d->addr, 0, 0, 0, 
                                  (char *)unsafe_tprintf("%s  [%s]", user, Name(player2)));
        }
	if (--(d->retries_left) <= 0) {
	    free_mbuf(command);
	    free_mbuf(user);
	    free_mbuf(password);
	    shutdownsock(d, R_BADLOGIN);
	    mudstate.debug_cmd = cmdsave;
	    RETURN(0); /* #146 */
	}
	*command = 'a';
	comptest = 1;
    } else if (!comptest && (d->host_info & H_NOGUEST)) {
	fcache_dump(d, FC_GUEST_FAIL);
        i_sitemax = site_check((d->address).sin_addr, mudstate.access_list, 1, 1, H_NOGUEST);
        if ( i_sitemax == -1 ) {
           tsite_buff = alloc_lbuf("noguest_check");
           addroutbuf = (char *) addrout((d->address).sin_addr);
           strcpy(tsite_buff, addroutbuf);
           strcpy(tsite_buff, mudconf.noguest_host);
           lookup(addroutbuf, tsite_buff, 1, &i_sitemax);
           free_lbuf(tsite_buff);
        }
        if ( i_sitemax == -1 ) {
	   broadcast_monitor(NOTHING, MF_SITE | MF_FAIL | MF_GFAIL, "NO GUEST FAIL", d->userid, d->addr, 0, 0, 0, NULL);
        } else {
	   broadcast_monitor(NOTHING, MF_SITE | MF_FAIL | MF_GFAIL, unsafe_tprintf("NO GUEST FAIL[%d max]", i_sitemax), d->userid, d->addr, 0, 0, 0, NULL);
        }
	STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "GUESTFAIL")
	  buff = alloc_mbuf("check_conn.LOG,badguest");
	  sprintf(buff, "[%d/%s] Attempt to connect to guest char from registered site",
		d->descriptor, d->addr);
	  log_text(buff);
	  free_mbuf(buff);
	ENDLOG
	if (--(d->retries_left) <= 0) {
	    free_mbuf(command);
	    free_mbuf(user);
	    free_mbuf(password);
	    shutdownsock(d, R_BADLOGIN);
	    mudstate.debug_cmd = cmdsave;
	    RETURN(0); /* #146 */
	}
	*command = 'a';
	comptest = 1;
    } else if (!comptest && (mudstate.guest_num >= mudconf.max_num_guests)) {
	queue_string(d, "Maximum number of guests has been reached.\r\n");
	STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
	    buff = alloc_mbuf("check_conn.LOG.bad");
	sprintf(buff, "[%d/%s] Failed connect to '%s'",
		d->descriptor, d->addr, user);
	log_text(buff);
	free_mbuf(buff);
	ENDLOG
	if (--(d->retries_left) <= 0) {
	    free_mbuf(command);
	    free_mbuf(user);
	    free_mbuf(password);
	    shutdownsock(d, R_BADLOGIN);
	    mudstate.debug_cmd = cmdsave;
	    RETURN(0); /* #146 */
	}
	*command = 'a';
	comptest = 1;
    } else if (!comptest) {
	bittemp = 0x00000001;
	gnum = 1;
	while ((bittemp & (mudstate.guest_status)) && (gnum < 32)) {
	    bittemp <<= 1;
	    gnum++;
	}
        if ( strlen(mudconf.guest_namelist) > 0 ) {
           memset(tchar_buffer, 0, sizeof(tchar_buffer));
           strcpy(tchar_buffer, mudconf.guest_namelist);
           in_tchr = strtok(tchar_buffer, " \t");
           tchar_num = 1;
           while ( (tchar_num < gnum) && (tchar_num < 32) ) {
              in_tchr = strtok(NULL, " \t");
              if ( in_tchr == NULL )
                 break;
              tchar_num++;
           }
           if ( (tchar_num == gnum) && in_tchr != NULL ) {
              strncpy(user, (char *)in_tchr, 16);
              *(user + 16) = '\0';
           } else { 
	      sprintf(buff2, "%d", gnum);
	      strcat(user, buff2);
           }
        } else {
	   sprintf(buff2, "%d", gnum);
	   strcat(user, buff2);
        }
    }
    if ( (!strncmp(command, "co", 2)) || (!strncmp(command, "cd", 2)) || (!strncmp(command, "ch", 2)) ) {

	/* See if this connection would exceed the max #players */
        
        if (mudconf.max_players > mudstate.max_logins_allowed)
           mudconf.max_players = mudstate.max_logins_allowed;
	if (mudconf.max_players < 0) {
            /* Can't be helped.  Have to check conn players regardless */
            nplayers = 0;
            DESC_ITER_CONN(d2)
                nplayers++;
            if ( nplayers > mudstate.max_logins_allowed )
	       nplayers = mudconf.max_players - 1;
	} else {
	    nplayers = 0;
	    DESC_ITER_CONN(d2)
		nplayers++;
	}

        ok_to_login = (((nplayers < mudconf.max_players) || (mudconf.max_players == -1)) && (nplayers < mudstate.max_logins_allowed));
	if (!strncmp(command, "cd", 2))
	  dc = 1;
	else if ( !strncmp(command, "ch", 2))
          dc = 2;
        else
	  dc =0;
	player = connect_player(user, password, (char *)d);
        player2 = lookup_player(NOTHING, user, 0);
	if (player == NOPERM) {
		queue_string(d, "Connections to that player are not allowed from your site.\r\n");
	    STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BADSITE")
		buff = alloc_mbuf("check_conn.LOG.bad");
	    sprintf(buff, "[%d/%s] Failed connect to '%s'",
		    d->descriptor, d->addr, user);
	    log_text(buff);
	    free_mbuf(buff);
	    ENDLOG
		if (--(d->retries_left) <= 0) {
		  free_mbuf(command);
		  free_mbuf(user);
		  free_mbuf(password);
		  shutdownsock(d, R_BADLOGIN);
		  mudstate.debug_cmd = cmdsave;
		  RETURN(0); /* #146 */
		}
	} else {

	  postest = 0;
	  if ((player != NOTHING) && NoPossess(player)) {
	    DESC_ITER_CONN(d3) {
		if (d3->player == player) {
		    if (strcmp(d3->addr, d->addr) || strcmp(d3->userid, d->userid)) {
			postest = 1;
			break;
		    }
		}
	    }
	  }
	  if ((player == NOTHING) || (Flags3(player) & NOCONNECT) || postest ) {
	    if ((player != NOTHING) && (Flags3(player) & NOCONNECT))
		broadcast_monitor(player, MF_SITE | MF_FAIL, "FAIL (NOCONNECT)", 
                                  d->userid, d->addr, 0, 0, 0, NULL);
	    if (postest)
		broadcast_monitor(player, MF_SITE | MF_FAIL, "FAIL (NOPOSSESS)", 
                                  d->userid, d->addr, 0, 0, 0, NULL);
            if ((player == NOTHING) && (player2 == NOTHING))
		broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CONNECT)", 
                                  d->userid, d->addr, 0, 0, 0, user);
            if (player == NOTHING && (player2 != NOTHING))
		broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CONNECT)", 
                                  d->userid, d->addr, 0, 0, 0, 
                                  (char *)unsafe_tprintf("%s  [%s]", user, Name(player2)));

	    /* Not a player, or wrong password */

	    queue_string(d, connect_fail);
	    STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
		buff = alloc_mbuf("check_conn.LOG.bad");
	    sprintf(buff, "[%d/%s] Failed connect to '%s'",
		    d->descriptor, d->addr, user);
	    log_text(buff);
	    free_mbuf(buff);
	    ENDLOG
		if (--(d->retries_left) <= 0) {
		free_mbuf(command);
		free_mbuf(user);
		free_mbuf(password);
		shutdownsock(d, R_BADLOGIN);
		mudstate.debug_cmd = cmdsave;
		RETURN(0); /* #146 */
	    }
          } else if (((mudconf.control_flags & CF_LOGIN) && ok_to_login) ||
                    Wizard(player) || God(player) || (Login(player) & ok_to_login)) {

	    /* Logins are enabled, or wiz or god */

            STARTLOG(LOG_LOGIN, "CON", "LOGIN")
               buff = alloc_mbuf("check_conn.LOG.login");
               sprintf(buff, "[%d/%s] Connected to ",
                       d->descriptor, d->addr);
               log_text(buff);
               log_name_and_loc(player);
               free_mbuf(buff);
            ENDLOG
            d->flags |= DS_CONNECTED;
	    d->connected_at = time(0);
	    d->player = player;

	    /* Give the player the MOTD file and the settable MOTD
	     * message(s). Use raw notifies so the player doesn't
	     * try to match on the text. */

	    if (Guest(player)) {
		fcache_dump(d, FC_CONN_GUEST);
	    } else {
		buff = atr_get(player, A_LAST, &aowner, &aflags);
		if ((buff == NULL) || (*buff == '\0'))
		    fcache_dump(d, FC_CREA_NEW);
		else {
		    fcache_dump(d, FC_MOTD);
		}
		if (Builder(player)) {
		    fcache_dump(d, FC_WIZMOTD);
		}
		free_lbuf(buff);
	    }
	    if (dc == 1) {
	      if (Immortal(player)) {
                buff = alloc_lbuf("connect_dark");
                sprintf(buff, "%s", (char *)"scloak unfindable dark");
                flag_set(player, player, buff, SET_MUFFLE);
                free_lbuf(buff);
	      }
	      else if (Wizard(player)) {
                buff = alloc_lbuf("connect_dark");
                sprintf(buff, "%s", (char *)"unfindable dark");
                flag_set(player, player, buff, SET_MUFFLE);
                free_lbuf(buff);
	      }
	      if ((!Immortal(player) && !Wizard(player)) || (Wizard(player) && DePriv(player, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA)))
		dc = 0;
	    } else if (dc == 2) { 
               if ( Wizard(player) || HasPriv(player, NOTHING, POWER_NOWHO, POWER5, NOTHING) ) {
                  s_Flags3(player, Flags3(player) | NOWHO);
               } else {
                  dc = 0;
               }
            }
            mudstate.chkcpu_stopper = time(NULL);
            mudstate.chkcpu_toggle = 0;
            mudstate.chkcpu_locktog = 0;
	    announce_connect(player, d, dc);

            if ( (Wizard(player) || God(player)) && (nplayers >= mudstate.max_logins_allowed) ) {
               buff = alloc_lbuf("warn_logins");
               raw_notify(player, "WARNING: You, Wizard, have logged in past user descriptor maximums.", 0, 1);
               sprintf(buff, "         There are only %d free descriptors left.",
                       (mudstate.max_logins_allowed + 7 - nplayers));
               raw_notify(player, buff, 0, 1);
               raw_notify(player, "         You will have serious issues if this number is reached.", 0, 1);
               free_lbuf(buff);
            }
	  } else if (!(mudconf.control_flags & CF_LOGIN)) {
	    failconn("CON", "Connect", "Logins Disabled", d,
		     R_GAMEDOWN, player, FC_CONN_DOWN,
		     mudconf.downmotd_msg, command, user, password,
		     cmdsave);
	    RETURN(0); /* #146 */
	  } else {
            if (nplayers >= mudstate.max_logins_allowed) {
	       failconn("CON", "Connect", "No Free Descriptors", d,
		        R_NODESCRIPTOR, player, FC_CONN_FULL,
		        mudconf.fullmotd_msg, command, user, password,
		        cmdsave);
	       RETURN(0); /* #146 */
            } else {
	       failconn("CON", "Connect", "Game Full", d,
		        R_GAMEFULL, player, FC_CONN_FULL,
		        mudconf.fullmotd_msg, command, user, password,
		        cmdsave);
	       RETURN(0); /* #146 */
            }
	  }
	}
    } else if (!strncmp(command, "cr", 2)) {

	/* Enforce game down */

	if (!(mudconf.control_flags & CF_LOGIN)) {
	    failconn("CRE", "Create", "Logins Disabled", d,
		     R_GAMEDOWN, NOTHING, FC_CONN_DOWN,
		     mudconf.downmotd_msg, command, user, password,
		     cmdsave);
	    RETURN(0); /* #146 */
	}

	/* Enforce ceiling on creates */
        if (mudconf.max_pcreate_lim != -1) {
           if ( (mudstate.last_pcreate_cnt >= mudconf.max_pcreate_lim) &&
                ((mudstate.last_pcreate_time + mudconf.max_pcreate_time) > mudstate.now) ) {
              broadcast_monitor(NOTHING, (MF_SITE | MF_BFAIL), "FAIL (CREATE CEILING)", d->userid,
                                d->addr, 0, 0, 0, user);
              buff = alloc_mbuf("check_conn.LOG.badcrea");
              if ( mudconf.pcreate_paranoia > 0 ) {
                 sprintf(buff, "%.100s 255.255.255.255", inet_ntoa(d->address.sin_addr));
                 if ( (mudconf.pcreate_paranoia == 1) && 
                      !(site_check(d->address.sin_addr, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION) &&
                      !(d->host_info & H_REGISTRATION) ) {
                    cf_site((int *)&mudstate.access_list, buff, 
                            H_REGISTRATION|H_AUTOSITE, 0, 1, "register_site");
                    sprintf(buff, "(PCREATE) [%.100s] marked for autoregistration.  (Remote port %d)",
                            inet_ntoa(d->address.sin_addr), ntohs(d->address.sin_port));
                 } else if ( (mudconf.pcreate_paranoia == 2) &&
                            !(site_check(d->address.sin_addr, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN) ) {
                    cf_site((int *)&mudstate.access_list, buff, 
                            H_FORBIDDEN|H_AUTOSITE, 0, 1, "forbid_site");
                    sprintf(buff, "(PCREATE) [%.100s] marked for autoforbid.  (Remote port %d)",
                            inet_ntoa(d->address.sin_addr), ntohs(d->address.sin_port));
                 }
                 broadcast_monitor(NOTHING, MF_CONN, unsafe_tprintf("PCREATE-SITE AUTO-BLOCKED[%d attempts/%ds time]",
                                   mudstate.last_pcreate_cnt,
                                   (mudstate.now - mudstate.last_pcreate_time)),
                                   d->userid, d->addr, 0, 0, 0, user);
                 STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOR")
                 log_text(buff);
                 ENDLOG
              }
              free_mbuf(buff);
	      failconn("CRE", "Create", unsafe_tprintf("[%d/%ds] Max Create Reached", 
                                                mudconf.max_pcreate_lim,
                                                mudconf.max_pcreate_time),
		       d, R_HACKER, NOTHING, FC_QUIT,
		       (char *)NULL, command, user, password,
		       cmdsave);
              RETURN(0); /* #146 */
           } else {
              if ( (mudstate.last_pcreate_time + mudconf.max_pcreate_time) < mudstate.now ) {
                 mudstate.last_pcreate_cnt = 0;
              }
              mudstate.last_pcreate_time = mudstate.now;
           } 
           mudstate.last_pcreate_cnt++;
        }
	/* Enforce max #players */

        if (mudconf.max_players > mudstate.max_logins_allowed)
           mudconf.max_players = mudstate.max_logins_allowed;
	if (mudconf.max_players < 0) {
            /* Sorry man, we need to do this */
            nplayers = 0;
	    DESC_ITER_CONN(d2)
		nplayers++;
            if ( nplayers > (mudstate.max_logins_allowed) )
	       nplayers = mudconf.max_players - 1;
	} else {
	    nplayers = 0;
	    DESC_ITER_CONN(d2)
		nplayers++;
	}
        ok_to_login = (((nplayers < mudconf.max_players) || (mudconf.max_players == -1)) && (nplayers < mudstate.max_logins_allowed));
	if ((nplayers > mudconf.max_players) && (mudconf.max_players >= 0)) {

	    /* Too many players on, reject the attempt */

	    failconn("CRE", "Create", "Game Full", d,
		     R_GAMEFULL, NOTHING, FC_CONN_FULL,
		     mudconf.fullmotd_msg, command, user, password,
		     cmdsave);
	    RETURN(0); /* #146 */
	}
        if (nplayers >= mudstate.max_logins_allowed) {
            /* More players than the OS can handle for descriptors */
	    failconn("CRE", "Create", "No Free Descriptors", d,
		     R_NODESCRIPTOR, NOTHING, FC_CONN_FULL,
		     mudconf.fullmotd_msg, command, user, password,
		     cmdsave);
	    RETURN(0); /* #146 */
        }
	if (d->host_info & H_REGISTRATION) {
	    fcache_dump(d, FC_CREA_REG);
	} else {
	    player = create_player(user, password, NOTHING, 0);
	    if (player == NOTHING) {
                if ( !ok_password(password, NOTHING, 0) ) {
                   queue_string(d, (char *)"Invalid password specified.\r\n");
                   if ( mudconf.safer_passwords ) {
                      queue_string(d, (char *)"Passwords must have 1 upper, 1 lower, and 1 non-alpha and be 5+ chars long.\r\n");
                   }
                } 
		broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CREATE)", d->userid, 
                                  d->addr, 0, 0, 0, user);
		queue_string(d, create_fail);
		STARTLOG(LOG_SECURITY | LOG_PCREATES, "CON", "BAD")
		    buff = alloc_mbuf("check_conn.LOG.badcrea");
		sprintf(buff,
			"[%d/%s] Create of '%s' failed",
			d->descriptor, d->addr, user);
		log_text(buff);
		free_mbuf(buff);
		ENDLOG
	    } else {
		STARTLOG(LOG_LOGIN | LOG_PCREATES, "CON", "CREA")
		    buff = alloc_mbuf("check_conn.LOG.create");
		sprintf(buff, "[%d/%s] Created ",
			d->descriptor, d->addr);
		log_text(buff);
		log_name(player);
		free_mbuf(buff);
		ENDLOG

                mudstate.chkcpu_stopper = time(NULL);
                mudstate.chkcpu_toggle = 0;
                mudstate.chkcpu_locktog = 0;

		move_object(player, mudconf.start_room);
		d->flags |= DS_CONNECTED;
		d->connected_at = time(0);
		d->player = player;
		fcache_dump(d, FC_CREA_NEW);
		announce_connect(player, d, 0);
	    }
	}
    } else if (mudconf.offline_reg && !strncmp(command, "reg", 3)) {
      if (d->host_info & H_NOAUTOREG) {
	buff3 = alloc_lbuf("reg.fail");
	queue_string(d, "Permission denied.\r\n");
	process_output(d);
	strcpy(buff3,user);
	strcat(buff3,"/");
	strcat(buff3,password);
	broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL (NoAutoReg)", d->userid, d->addr, d->descriptor, 0, 0, buff3);
	free_lbuf(buff3);
      }
      else if (d->regtries_left <= 0) {
	buff3 = alloc_lbuf("reg.fail");
	queue_string(d, "Registration limit reached.\r\n");
	process_output(d);
	strcpy(buff3,user);
	strcat(buff3,"/");
	strcat(buff3,password);
	broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL LIMIT", d->userid, d->addr, d->descriptor, 0, 0, buff3);
	free_lbuf(buff3);
      }
      else {
        switch (reg_internal(user, password, (char *)d, 0)) {
	  case 0:
	    (d->regtries_left)--;
	    queue_string(d, "Autoregistration password emailed.\r\n");
	    break;
	  case 1:
	    queue_string(d, "Autoregistration character invalid or already exists.\r\n");
	    break;
	  case 2:
	    queue_string(d, "Email of password failed.\r\n");
	    break;
	  case 3:
	    queue_string(d, "Space in email address.\r\n");
	    break;
	  case 4:
	    queue_string(d, "Permission denied.\r\n");
	    break;
          case 5:
            queue_string(d, "Invalid email specified.\r\n");
            break;
          case 6:
            queue_string(d, "That email address is not allowed.\r\n");
            break;
          case 7:
            queue_string(d, "Invalid character detected in email.\r\n");
            break;
        }
      }
    } else {
	welcome_user(d);
        if ( Good_obj(d->player) && !TogHideIdle(d->player) )
	   d->command_count++;
    }
    free_mbuf(command);
    free_mbuf(user);
    free_mbuf(password);
    if (!comptest) {
	mudstate.guest_num++;
	mudstate.guest_status |= bittemp;
    }
    mudstate.debug_cmd = cmdsave;
    RETURN(1); /* #146 */
}

extern int igcheck(dbref, int);

int 
do_command(DESC * d, char *command)
{
    char *arg, *cmdsave, *time_str;
    struct SNOOPLISTNODE *node;
    DESC *sd, *d2;
    NAMETAB *cp;
    int retval, cval, gotone, store_perm, chk_perm;

    DPUSH; /* #147 */

    time_str = NULL;
    chk_perm = store_perm = 0;
    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< do_command >";
    mudstate.breakst = 0;
    mudstate.breakdolist = 0;
    
    /* snoop on player input -Thorin */
    if (d->snooplist) {
	for (node = d->snooplist; node; node = node->next) {
	    if (node->sfile) {
		fprintf(node->sfile, "%ld>", mudstate.now);
		fputs(Name(d->player), node->sfile);
		fputs("> ", node->sfile);
		fputs(command, node->sfile);
		fputc('\n', node->sfile);
	    }
	    if (!Connected(node->snooper) || node->logonly)
		continue;
	    DESC_ITER_PLAYER(node->snooper, sd) {
                if( ShowAnsi(sd->player) ) {
                  queue_string(sd, ANSI_HILITE);
                  queue_string(sd, ANSI_WHITE);
                }
		queue_string(sd, ">");
                if( ShowAnsi(sd->player) ) {
                  queue_string(sd, ANSI_YELLOW);
                }
		queue_string(sd, Name(d->player));
                if( ShowAnsi(sd->player) ) {
                  queue_string(sd, ANSI_WHITE);
                }
		queue_string(sd, "> ");
                if( ShowAnsi(sd->player) ) {
                  queue_string(sd, ANSI_NORMAL);
                }
		queue_string(sd, command);
		queue_write(sd, "\r\n", 2);
	    }
	}
    }
    /* Split off the command from the arguments */

    arg = command;
    while (*arg && !isspace((int)*arg))
	arg++;
    if (*arg)
	*arg++ = '\0';

    /* Look up the command.  If we don't find it, turn it over to the
     * normal logged-in command processor or to create/connect
     * cval: 0 normal, 1 disable, 2 ignore
     */

    if ( InProgram(d->player) && (command[0] == '|') && 
         !((NoShProg(d->player) && !mudconf.noshell_prog) || 
           (!Immortal(d->player) && mudconf.noshell_prog && !NoShProg(d->player))) ) 
       cp = (NAMETAB *) hashfind(command+1, &mudstate.logout_cmd_htab);
    else
       cp = (NAMETAB *) hashfind(command, &mudstate.logout_cmd_htab);
    /* Make modification here for zone/location icmd check and global icmd check */
    cval = 0; 
    if ( cp && d->player )
       cval = (igcheck(d->player, cp->perm) ? 2 : 0);
    if ( (cval == 0) && cp && d->player && CmdCheck(d->player) )
      cval = cmdtest(d->player, cp->name);
    /* Let's do the zone check */
    if ( (cval == 0) && cp && d->player && Good_obj(Location(d->player)) ) {
       cval = zonecmdtest(d->player, cp->name);
    }
    /* If ignored on WHO or DOING, then allow on connect screen but not in-game */
    if ( cp && (((cp->flag & CMD_MASK) == CMD_WHO) || ((cp->flag & CMD_MASK) == CMD_DOING)) ) {
      if ( cp->perm & CA_IGNORE ) {
	if ( d->player && Connected(d->player) )
	  cval = 2;
	else {
             chk_perm = 1;
             store_perm = cp->perm;
             cval = 0;
             cp->perm = (cp->perm & ~CA_IGNORE);
	}
      }
    }

    if (cval == 2)
      cp = NULL;

    if (cp == NULL || (cp != NULL && InProgram(d->player) && command[0] != '|') || 
        (((NoShProg(d->player) && !mudconf.noshell_prog) ||
          (!NoShProg(d->player) && mudconf.noshell_prog && !Immortal(d->player))) && 
         InProgram(d->player))) {
	if (*arg)
	    *--arg = ' ';	/* restore nullified space */
	if (d->flags & DS_CONNECTED) {
            if ( Good_obj(d->player) && !TogHideIdle(d->player) )
	       d->command_count++;
	    if (d->output_prefix) {
		queue_string(d, d->output_prefix);
		queue_write(d, "\r\n", 2);
	    }
	    mudstate.curr_player = d->player;
	    mudstate.curr_enactor = d->player;
	    desc_in_use = d;
	    process_command(d->player, d->player, 1,
			    command, (char **) NULL, 0, mudstate.shell_program);
            mudstate.curr_cmd = (char *) "";
	    if (d->output_suffix) {
		queue_string(d, d->output_suffix);
		queue_write(d, "\r\n", 2);
	    }
	    mudstate.debug_cmd = cmdsave;
	    if ( chk_perm) 
	      cp->perm = store_perm;
	    RETURN(1); /* #147 */
	} else {
	    mudstate.debug_cmd = cmdsave;
            retval = check_connect(d, command);
	    if ( chk_perm)
	      cp->perm = store_perm;
	    RETURN(retval); /* #147 */
	}
    }
    /* The command was in the logged-out command table.  Perform prefix
     * and suffix processing, and invoke the command handler.
     */

    if ( Good_obj(d->player) && !TogHideIdle(d->player) )
       d->command_count++;
    if (cp && !(cp->flag & CMD_NOxFIX)) {
	if (d->output_prefix) {
	    queue_string(d, d->output_prefix);
	    queue_write(d, "\r\n", 2);
	}
    }
    if ( cp && (!check_access(d->player, cp->perm, cp->perm2, 0) || cval ||
	((cp->perm & CA_PLAYER) && !(d->flags & DS_CONNECTED))) ) {
	queue_string(d, "Permission denied.\r\n");
    } else if ( cp ){
	mudstate.debug_cmd = cp->name;
	switch (cp->flag & CMD_MASK) {
	case CMD_QUIT:
	    if (!Fubar(d->player)) {
		shutdownsock(d, R_QUIT);
		mudstate.debug_cmd = cmdsave;
		if ( chk_perm )
		  cp->perm = store_perm;
		RETURN(0); /* #147 */
	    } else {
		notify_quiet(d->player, "Permission denied.");
		break;
	    }
	case CMD_LOGOUT:
	    if (!Fubar(d->player)) {
		shutdownsock(d, R_LOGOUT);
		break;
	    } else {
		notify_quiet(d->player, "Permission denied.");
		break;
	    }
	case CMD_WHO:
	    dump_users(d, arg, CMD_WHO);
	    break;
	case CMD_DOING:
	    dump_users(d, arg, CMD_DOING);
	    break;
	case CMD_SESSION:
            if ( d->flags & DS_CONNECTED )
	       dump_users(d, arg, CMD_SESSION);
            else
	       notify_quiet(d->player, "Permission denied.");
	    break;
#ifdef LOCAL_RWHO_SERVER
	case CMD_RWHO:
	    dump_rusers(d);
	    break;
#endif
	case CMD_PREFIX:
	    set_userstring(&d->output_prefix, arg);
	    break;
	case CMD_SUFFIX:
	    set_userstring(&d->output_suffix, arg);
	    break;
        case CMD_GET:
          queue_string(d, "Content-type: text/html\r\n"
                          "\r\n"
                          "<H1>Rhostshyl HTML Test</H1>\r\n"
                          "This is a test.");
          queue_write(d, "\r\n", 2);
          break;
        case CMD_INFO:
          gotone = 0;
          DESC_ITER_CONN(d2) {
             if (Cloak(d2->player))
	        continue;
             if (!(mudconf.who_unfindable) && Dark(d2->player) && 
                 !(mudconf.player_dark) && mudconf.allow_whodark )
                continue;
             if (NoWho(d2->player))
                continue;
             gotone += 1;
          }
          time_str = ctime(&mudstate.start_time);
          time_str[strlen(time_str) - 1] = '\0';
          queue_string(d, unsafe_tprintf("### Begin INFO %s\r\n", INFO_VERSION));
          queue_string(d, unsafe_tprintf("Name: %s\r\n", mudconf.mud_name));
          queue_string(d, unsafe_tprintf("Uptime: %s\r\n", time_str));
          queue_string(d, unsafe_tprintf("Connected: %d\r\n", gotone));
          queue_string(d, unsafe_tprintf("Size: %d\r\n", mudstate.db_top));
          queue_string(d, unsafe_tprintf("Version: %s\r\n", mudstate.short_ver));
          queue_string(d, "### End INFO");
          queue_write(d, "\r\n", 2); 
          break;
	default:
	    STARTLOG(LOG_BUGS, "BUG", "PARSE")
		arg = alloc_lbuf("do_command.LOG");
	    sprintf(arg,
		    "Prefix command with no handler: '%.3900s'",
		    command);
	    log_text(arg);
	    free_lbuf(arg);
	    ENDLOG
	}
    }
    if (cp && !(cp->flag & CMD_NOxFIX)) {
	if (d->output_suffix) {
	    queue_string(d, d->output_suffix);
	    queue_write(d, "\r\n", 2);
	}
    }
    mudstate.debug_cmd = cmdsave;
    if ( chk_perm )
      cp->perm = store_perm;
    RETURN(1); /* #147 */
}

void 
NDECL(process_commands)
{
    int nprocessed, aflags2, i;
    DESC *d, *dnext;
    CBLK *t;
    char *cmdsave, *progatr;
    dbref aowner2;

    DPUSH; /* #148 */

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "process_commands";

    do {
	nprocessed = 0;
	DESC_SAFEITER_ALL(d, dnext) {
	    if ((d->flags & DS_AUTH_IN_PROGRESS) == 0) {
		if (d->quota > 0 && (t = d->input_head)) {
		    d->quota--;
		    nprocessed++;
		    d->input_head = (CBLK *) t->hdr.nxt;
		    if (!d->input_head)
			d->input_tail = NULL;
		    d->input_size -= (strlen(t->cmd) + 1);
		    if (d->flags & DS_HAS_DOOR && t->cmd[0] != '!')
			door_raw_input(d, t->cmd); 
		    else {
                        for (i = 0; i < MAX_GLOBAL_REGS; i++) {
                           *mudstate.global_regs[i] = '\0';
                           *mudstate.global_regsname[i] = '\0';
                        }
                        mudstate.global_regs_wipe = 0;
			do_command(d, (d->flags & DS_HAS_DOOR) ? &t->cmd[1] : t->cmd);
                        if ( InProgram(d->player) ) {
                           progatr = atr_get(d->player, A_PROGPROMPTBUF, &aowner2, &aflags2);
                           if ( progatr && *progatr ) {
                              if ( strcmp(progatr, "NULL") != 0 )
                                 queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, 
                                              progatr, ANSI_NORMAL));
                           } else
                              queue_string(d, unsafe_tprintf("%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
                           free_lbuf(progatr);
                        }
                        mudstate.shell_program = 0;
                    }
		    free_lbuf(t);
		}
	    }
	    if ((t = d->door_input_head)) {
		nprocessed++;
		d->door_input_head = (CBLK *) t->hdr.nxt;
		if (!d->door_input_head)
		    d->door_input_tail = NULL;
		door_raw_output(d, t->cmd);
		free_lbuf(t);
	    }
	}
    } while (nprocessed > 0);
    mudstate.debug_cmd = cmdsave;
    VOIDRETURN; /* #148 */
}

int
blacklist_check(struct in_addr host)
{
   int i_return;
   BLACKLIST *b_host;

   DPUSH;
   i_return=0;
   if ( mudstate.blacklist_cnt < 1 ) {
      RETURN(i_return);
   }
   b_host = mudstate.bl_list;
   while ( b_host ) {
      if ( (host.s_addr & b_host->mask_addr.s_addr) == b_host->site_addr.s_addr ) {
         i_return=1;
         break;
      }
      b_host = b_host->next;
   }
   RETURN(i_return);
}
/* ---------------------------------------------------------------------------
 * site_check: Check for site flags in a site list.
 */

int 
site_check(struct in_addr host, SITE * site_list, int stop, int key, int flag)
{
   SITE *this;
   int rflag, rkey, i_found;

   DPUSH; /* #149 */
   rflag = i_found = 0;
   rkey = 2000000000;
   for (this = site_list; this; this = this->next) {
      if ((host.s_addr & this->mask.s_addr) == this->address.s_addr) {
         if ( key && (this->flag == flag) ) {
            i_found = 1;
            if ( rkey > this->maxcon ) {
               rkey = this->maxcon;
            }
            if ( !stop ) {
               RETURN(rkey);
            }
         } else if (!stop) {
            RETURN(this->flag); /* #149 */
         } else if (this->flag == H_FORBIDDEN) {
            if (rflag) {
               RETURN(rflag); /* #149 */
            } else {
               RETURN(this->flag); /* #149 */
            }
         }
         rflag |= this->flag;
      }
   }
   if ( key ) {
      if ( !i_found ) 
         rkey = -1;
      RETURN(rkey);
   } else {
      RETURN(rflag); /* #149 */
   }
}

/* --------------------------------------------------------------------------
 * list_sites: Display information in a site list
 */

#define	S_SUSPECT	1
#define	S_ACCESS	2
#define	S_SPECIAL	3

static const char *
stat_string(int strtype, int flag, int key)
{
    const char *str;

    DPUSH; /* #150 */

    switch (strtype) {
    case S_SUSPECT:
	if (flag)
	    str = "Suspected";
	else
	    str = "Trusted";
	break;
    case S_ACCESS:
	switch (flag) {
	case H_FORBIDDEN:
            if ( key )
	       str = "Forbidden (AutoSite)";
            else
	       str = "Forbidden";
	    break;
	case H_REGISTRATION:
            if ( key )
	       str = "Registration (AutoSite)";
            else
	       str = "Registration";
	    break;
	case H_NOAUTOREG:
	    str = "NoAutoReg";
	    break;
	case H_NOGUEST:
	    str = "Noguest";
	    break;
	case 0:
	    str = "Unrestricted";
	    break;
	default:
	    str = "Strange";
	}
	break;
    case S_SPECIAL:
        switch( flag ) {
        case H_NOAUTH:
            str = "NoAUTH";
            break;
        case H_NODNS:
            str = "NoDNS";
            break;
        default:
            str = "Strange";
        }
        break;
    default:
	str = "Strange";
    }
    RETURN(str); /* #150 */
}

static void 
list_sites(dbref player, SITE * site_list,
	   const char *header_txt, int stat_type)
{
    char *buff, *buff1, *str, *str2;
    SITE *this;

    DPUSH; /* #151 */

    buff = alloc_mbuf("list_sites.buff");
    buff1 = alloc_sbuf("list_sites.addr");
    sprintf(buff, "----- %s -----", header_txt);
    notify(player, buff);
    notify(player, "Address              Mask                 Max-Conns      Status");
    str2 = alloc_sbuf("list_sites");
    for (this = site_list; this; this = this->next) {
	str = (char *) stat_string(stat_type, this->flag, this->key);
        if ( (this->flag & H_REGISTRATION) ||
             (this->flag & H_FORBIDDEN) ||
             (this->flag & H_NOGUEST) ) {
           if ( this->maxcon == -1 ) 
              strcpy(str2, (char *)"Restricted");
           else
              sprintf(str2, "%d cons", this->maxcon);
        } else {
              strcpy(str2, (char *)"N/A (Ignored)");
        }
	strcpy(buff1, inet_ntoa(this->mask));
	sprintf(buff, "%-20s %-20s %-14s %s",
		inet_ntoa(this->address), buff1, str2, str);
	notify(player, buff);
    }
    free_sbuf(str2);

    free_mbuf(buff);
    free_sbuf(buff1);
    VOIDRETURN; /* #151 */
}

void
list_hosts(dbref player, char *hostchrtype, char *hostchrmeth)
{
    char *tmp_word, tbuff[LBUF_SIZE], sbuff[SBUF_SIZE], *tpr_buff, *tprp_buff, *tmp_wordptr;
    int i_maxcons, i_found;

    memset(tbuff, '\0', sizeof(tbuff));
    memset(sbuff, '\0', sizeof(sbuff));
    strncpy(tbuff, hostchrtype, (LBUF_SIZE > strlen(hostchrtype) ? strlen(hostchrtype) : LBUF_SIZE - 1));
    if ( hostchrtype ) {
       tmp_word = strtok(tbuff, " ");
       tprp_buff = tpr_buff = alloc_lbuf("list_hosts_tprintf");
       while( tmp_word ) {
          i_found = 0;
          i_maxcons = -1;
          if ( (tmp_wordptr = strchr(tmp_word, '|')) != NULL ) {
             i_found = 1;
             *tmp_wordptr = '\0';
             i_maxcons = atoi(tmp_wordptr+1);
          }
          if ( (strcmp(hostchrmeth, "Forbidden") == 0) ||
               (strcmp(hostchrmeth, "Noguest") == 0) ||
               (strcmp(hostchrmeth, "Registration") == 0) ) {
             if ( i_maxcons == -1 ) {
                strcpy(sbuff, (char *)"Restricted");                
             } else {
                sprintf(sbuff, "%d cons", i_maxcons);
             }
          } else {
             if ( i_maxcons != -1 )
                sprintf(sbuff, (char *)"N/A (%d)", i_maxcons);                
             else
                strcpy(sbuff, (char *)"N/A (Ignored)");
          }
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-40.40s  %-14s %-s", tmp_word, sbuff, hostchrmeth));
          if ( i_found )
             *tmp_wordptr = '|';
          tmp_word = strtok( NULL, " " );
       }
       free_lbuf(tpr_buff);
    }
}
/* ---------------------------------------------------------------------------
 * list_siteinfo: List information about specially-marked sites.
 */

void 
list_siteinfo(dbref player)
{
    char *s_buff;

    DPUSH; /* #152 */
    
    s_buff = alloc_mbuf("list_siteinfo");
    if ( mudstate.blacklist_cnt > 0 ) {
       sprintf(s_buff, "Blacklist: There are currently %d entries in the blacklist.",
               mudstate.blacklist_cnt); 
       notify(player, s_buff);
    } else {
       notify(player, "Blacklist: Blacklists are currently not populated.");
    }
    free_mbuf(s_buff);
    list_sites(player, mudstate.access_list, "Site Access",
	       S_ACCESS);
    list_sites(player, mudstate.suspect_list, "Suspected Sites",
	       S_SUSPECT);
    list_sites(player, mudstate.special_list, "Special Sites",
	       S_SPECIAL);

    notify(player, "----- DNS Access Permissions -----");
    notify(player, unsafe_tprintf("%-40s  %-14s %-s", "DNS Name Convention", "Max-Conns", "Status"));
    list_hosts(player, mudconf.forbid_host, "Forbidden");
    list_hosts(player, mudconf.suspect_host, "Suspected");
    list_hosts(player, mudconf.register_host, "Registration");
    list_hosts(player, mudconf.noguest_host, "Noguest");
    list_hosts(player, mudconf.autoreg_host, "NoAutoReg");
    list_hosts(player, mudconf.validate_host, "InvalidAReg");
    list_hosts(player, mudconf.goodmail_host, "ValidAReg");
    list_hosts(player, mudconf.nobroadcast_host, "NoMonitor");

    VOIDRETURN; /* #152 */
}

/* ---------------------------------------------------------------------------
 * make_ulist: Make a list of connected user numbers for the LWHO function.
 */

void 
make_ulist(dbref player, char *buff, char **bufcx, int i_type, dbref victim)
{
    DESC *d;
    int gotone = 0, i_port;
    char *tpr_buff, *tprp_buff;
    dbref target;

    DPUSH; /* #153 */

    if ( Good_obj(victim) )
      target = victim;
    else
      target = player;

    if (!Controls(player, target))
      target = player;

    tprp_buff = tpr_buff = alloc_lbuf("make_ulist_tprintf");
    DESC_ITER_CONN(d) {
	if (!Wizard(target) && Cloak(d->player))
	    continue;
        if (!Admin(target) && !(mudconf.who_unfindable) && Dark(d->player) && 
            !(mudconf.player_dark) && mudconf.allow_whodark )
            continue;
	if (Immortal(d->player) && Cloak(d->player) && SCloak(d->player) && !Immortal(target))
	    continue;
	if (!Admin(target) && Unfindable(d->player) && mudconf.who_unfindable
		&& !HasPriv(target,d->player,POWER_WHO_UNFIND,POWER4,NOTHING))
	    continue;
        if (NoWho(d->player) && (d->player != target) &&
            !(Wizard(target) || HasPriv(target, d->player, POWER_WIZ_WHO, POWER3, NOTHING)))
            continue;
	if (gotone)
	    safe_chr(' ', buff, bufcx);
        if ( i_type == 2 ) {
           /* This is player and not target as it needs the immediate enactor, not 'target' */
           if ( (target == d->player) || Wizard(player) ) 
              i_port = d->descriptor;
           else
              i_port = -1;
           tprp_buff = tpr_buff;
	   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d", i_port), buff, bufcx);
        } else if ( i_type == 1 ) {
           /* This is player and not target as it needs the immediate enactor, not 'target' */
           if ( (target == d->player) || Wizard(player) ) 
              i_port = d->descriptor;
           else
              i_port = -1;
           tprp_buff = tpr_buff;
	   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d:%d", d->player, i_port), buff, bufcx);
        } else {
           tprp_buff = tpr_buff;
	   safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d", d->player), buff, bufcx);
        }
        gotone = 1;
    }
    free_lbuf(tpr_buff);
    VOIDRETURN; /* #153 */
}

/* ---------------------------------------------------------------------------
 * find_connected_name: Resolve a playername from the list of connected
 * players using prefix matching.  We only return a match if the prefix
 * was unique.
 */

dbref 
find_connected_name(dbref player, char *name)
{
    DESC *d;
    dbref found;

    DPUSH; /* #154 */

    found = NOTHING;
    DESC_ITER_CONN(d) {
	if (Good_obj(player) && !Wizard(player) && Hidden(d->player))
	    continue;
	if (!string_prefix(Name(d->player), name))
	    continue;
	if ((found != NOTHING) && (found != d->player)) {
	    RETURN(NOTHING); /* #154 */
        }
	found = d->player;
    }
    RETURN(found); /* #154 */
}

/* ---------------------------------------------------------------------------
 * rwho_update: Send RWHO info to the remote RWHO server.
 */


#ifdef RWHO_IN_USE
void 
NDECL(rwho_update)
{
    DESC *d;
    char *buf;
 
    DPUSH; /* #155 */

    if (!(mudconf.rwho_transmit && (mudconf.control_flags & CF_RWHO_XMIT))) {
	VOIDRETURN; /* #155 */
    }
    buf = alloc_mbuf("rwho_update");
    rwhocli_pingalive();
    DESC_ITER_ALL(d) {
	if ((d->flags & DS_CONNECTED) && !Hidden(d->player)) {
	    sprintf(buf, "%d@%s", d->player, mudconf.mud_name);
	    rwhocli_userlogin(buf, Name(d->player), d->connected_at);
	}
    }
    free_mbuf(buf);
    VOIDRETURN; /* #155 */
}
#endif
