/* netcommon.c - Network utility routines */

/* This file contains routines used by the networking code that do not
 * depend on the implementation of the networking code.  The network-specific
 * portions of the descriptor data structure are not used.
 */

#ifdef SOLARIS
/* Solaris has borked header file declarations */
char *index(const char *, int);
#endif
#ifndef UTMP_MISSING
#ifdef UTMP_ONLY
#include <utmp.h>
#else
#include <utmpx.h>
#endif
#endif

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
#include "telnet_io.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */
#include "door.h"
#ifdef ENABLE_WEBSOCKETS
///// NEW WEBSOCK
#include "websock2.h"
///// END NEW WEBSOCK
#endif
#ifdef ENABLE_LUA
#include "lua.h"
#endif

#include "debug.h"
#define FILENUM NETCOMMON_C

extern int ndescriptors;
extern int FDECL(alarm_msec, (double));

/* ---------------------------------------------------------------------------
 * DESC_OLD — Legacy descriptor struct for reboot format migration.
 * Exact replica of struct descriptor_data from pre hot/cold split (interface.old:167-222).
 * Used only in load_reboot_db() to read old-format .reboot files and migrate to v3.
 */
typedef struct desc_old DESC_OLD;
struct desc_old {
    int descriptor;
    int flags;
    int retries_left;
    int regtries_left;
    int command_count;
    int timeout;
    int host_info;
    char addr[51];
    char doing[256];
    dbref player;
    char *output_prefix;
    char *output_suffix;
    int output_size;
    int output_tot;
    int output_lost;
    TBLOCK *output_head;
    TBLOCK *output_tail;
    int input_size;
    int input_tot;
    int input_lost;
    CBLK *input_head;
    CBLK *input_tail;
    CBLK *raw_input;
    char *raw_input_at;
    time_t connected_at;
    time_t last_time;
    int quota;
    struct sockaddr_in address;
    struct descriptor_data *hashnext;
    struct descriptor_data *next;
    struct descriptor_data **prev;
    struct SNOOPLISTNODE *snooplist;
    int logged;
    int authdescriptor;
    char userid[MBUF_SIZE];
    int door_desc;
    int door_num;
    TBLOCK *door_output_head;
    TBLOCK *door_output_tail;
    int door_output_size;
    CBLK *door_input_head;
    CBLK *door_input_tail;
    int door_int1;
    int door_int2;
    int door_int3;
    char *door_lbuf;
    char *door_mbuf;
    char *door_raw;
    char checksum[WEBSOCKETS_CHECKSUM_LEN + 1];
    long ws_frame_len;
    dbref account_owner;
    char account_rawpass[100];
    char longaddr[256];
    int longaddrcheck;
};


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
#define CMD_HEAD        11
#define CMD_POST	12
#define CMD_PUT 	13

#define	CMD_MASK	0xff
#define	CMD_NOxFIX	0x100

#define INFO_VERSION    "1"

extern int FDECL(process_output, (DESC * d));
extern int decode_base64(const char *, int, char *, char **, int);
extern CF_HAND(cf_site);
extern int FDECL(lookup, (char *, char *, int, int *));
static void set_userstring(char **, const char *);
extern const char *addrout(const char *, int, int);
extern void fun_objid(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern CMDENT * lookup_command(char *);
extern int process_hook(dbref, dbref, char *, ATTR *, int, int, char *);
extern int encode_base64(const char *, int, char *, char **);
extern int check_tor_str(const char *, int, int);

extern char * skip_mux_ansi(char *, char *, char **);
extern int count_mux_ansi(char *);
extern int down_ansi(int, int, int);
extern char *ansi_translate_fg[257];
extern char *ansi_translate_bg[257];


#ifdef LOCAL_RWHO_SERVER
void FDECL(dump_rusers, (DESC * call_by));

#endif

/* These functions only strip RAW ansi.  Leave this as it is or shit breaks */
extern char *
strip_ansi2(const char *raw)
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

/* These functions only strip RAW ansi.  Leave this as it is or shit breaks */
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

/* These functions only strip RAW ansi.  Leave this as it is or shit breaks */
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

        if( (*p == '%') && ( (*(p+1) == SAFE_CHR)  || (*(p+1) == SAFE_UCHR)
#ifdef SAFE_CHR2
                        ||   (*(p+1) == SAFE_CHR2) || (*(p+1) == SAFE_UCHR2)
#endif
#ifdef SAFE_CHR3
                        ||   (*(p+1) == SAFE_CHR3) || (*(p+1) == SAFE_UCHR3)
#endif
)) {
           if ( isAnsi[(int) *(p+2)] ) {
              p+=3;
              continue;
           }
             if ( *(p+2) == '0' && ((*(p+3) == 'x') || (*(p+3) == 'X')) &&
                  *(p+4) && *(p+5) && isxdigit(*(p+4)) && isxdigit(*(p+5)) ) {
               p+=6; // strip safe XTERM ansi
               continue;
            }
              if ( *(p+2) == '<' && (strchr(p+2, '>') != NULL) ) {
                p = skip_mux_ansi(p+2, (char *)NULL, (char **)NULL);
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
strip_all_special2(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #100 */

    while (p && *p) {
        if ( (*p == '%') && ( (*(p+1) == SAFE_CHR)  || (*(p+1) == SAFE_UCHR) 
#ifdef SAFE_CHR2
                         ||   (*(p+1) == SAFE_CHR2) || (*(p+1) == SAFE_UCHR2)
#endif
#ifdef SAFE_CHR3
                         ||   (*(p+1) == SAFE_CHR3) || (*(p+1) == SAFE_UCHR3)
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
             if ( *(p+2) == '<' && (strchr(p+2, '>') != NULL) ) {
               p = skip_mux_ansi(p+2, (char *)NULL, (char **)NULL);
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
strip_all_special(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    DPUSH; /* #100 */

    while (p && *p) {
        if ( (*p == '%') && ( (*(p+1) == SAFE_CHR)  || (*(p+1) == SAFE_UCHR) 
#ifdef SAFE_CHR2
                         ||   (*(p+1) == SAFE_CHR2) || (*(p+1) == SAFE_UCHR2)
#endif
#ifdef SAFE_CHR3
                         ||   (*(p+1) == SAFE_CHR3) || (*(p+1) == SAFE_UCHR3)
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
             /* Strip mux ansi */
            if ( *(p+2) == '<' && (strchr(p+2, '>') != NULL) ) {
               p = skip_mux_ansi(p+2, (char *)NULL, (char **)NULL);
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
        if( (*p == '%') && ( (*(p+1) == SAFE_CHR)  || (*(p+1) == SAFE_UCHR) 
#ifdef SAFE_CHR2
                        ||   (*(p+1) == SAFE_CHR2) || (*(p+1) == SAFE_UCHR2)
#endif
#ifdef SAFE_CHR3
                        ||   (*(p+1) == SAFE_CHR3) || (*(p+1) == SAFE_UCHR3)
#endif
)) {
           if ( isAnsi[(int) *(p+2)] ) {
              p+=3; // strip safe ansi
              continue;
           }
            if ( *(p+2) == '0' && ((*(p+3) == 'x') || (*(p+3) == 'X')) &&
                 *(p+4) && *(p+5) && isxdigit(*(p+4)) && isxdigit(*(p+5)) ) {
              p+=6; // strip safe XTERM ansi
              continue;
           }
             /* Strip mux ansi */
             if ( *(p+2) == '<' && (strchr(p+2, '>') != NULL) ) {
                p = skip_mux_ansi(p+2, (char *)NULL, (char **)NULL);
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

static void
ival(char *buff, char **bufcx, int result)
{
  static char tempbuff[LBUF_SIZE/2];

  sprintf(tempbuff, "%d", result);
  safe_str(tempbuff, buff, bufcx);
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
	      while (*p && !isalpha((int)*p)) {
                 p++;
              }
              if (*p) {
                 p++;
              }
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
   ANSI_BYELLOW, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BMAGENTA, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BYELLOW, ANSI_BWHITE,
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
   ANSI_YELLOW, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_MAGENTA_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_YELLOW_H, ANSI_WHITE_H,
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
	    /* We've got an ANSI code here -- verify it's XTERM/TrueColor codes */
            if ( (*(p+1) && (*(p+1) == '[')) && (*(p+2) && ((*(p+2) == '4') || (*(p+2) == '3'))) && (*(p+3) && (*(p+3) == '8')) ) {
               if ( *(p+2) == '3' ) {
                  i_type = 0;
               } else {
                  i_type = 1;
               }
               /* Check mode: ;2; = TrueColor (pass through), ;5; = 256-color (downconvert) */
               if ( *(p+4) == ';' && *(p+5) == '2' && *(p+6) == ';' ) {
                  /* TrueColor: copy the entire escape as-is */
                  while (*p && *p != 'm')
                     *q++ = *p++;
                  if (*p)
                     *q++ = *p++;
               } else {
                  /* 256-color: downconvert to 16-color */
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
               }
            } else {
 	       *q++ = *p++;
            }
        }
    }
    *q = '\0';
    RETURN(buf); /* #104 */
}

static char *
strip_ansi_truecolor(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;
    int i_r, i_g, i_b, i_type, i_256;

    DPUSH; /* #106 */

    while (p && *p) {
        if (*p != ESC_CHAR) {
            *q++ = *p++;
        } else {
            /* Check for TrueColor: \x1B[38;2;R;G;Bm or \x1B[48;2;R;G;Bm */
            if ((*(p+1) == '[') &&
                ((*(p+2) == '3' || *(p+2) == '4') && *(p+3) == '8') &&
                *(p+4) == ';' && *(p+5) == '2' && *(p+6) == ';') {

                i_type = (*(p+2) == '3') ? 0 : 1;
                p += 7;

                i_r = atoi(p);
                while (*p && isdigit(*p)) p++;
                if (*p == ';') p++;
                i_g = atoi(p);
                while (*p && isdigit(*p)) p++;
                if (*p == ';') p++;
                i_b = atoi(p);
                while (*p && isdigit(*p)) p++;

                /* Downsample to nearest 256-color */
                i_256 = down_ansi(i_r, i_g, i_b);

                /* Output 256-color escape */
                if (i_type)
                    q += sprintf(q, "\x1B[48;5;%dm", i_256);
                else
                    q += sprintf(q, "\x1B[38;5;%dm", i_256);

                /* Skip to end of ANSI sequence */
                while (*p && *p != 'm') p++;
                if (*p) p++;
            } else {
                *q++ = *p++;
            }
        }
    }
    *q = '\0';
    RETURN(buf);
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

/* html handler for API -- cleanup and rewrite */
void  
handle_html(DESC *d, int code, char *buf1, char *buf2, char *buf3, char *buf4)
{
   char *s_dtime;

   s_dtime = (char *) ctime(&mudstate_hot.now);

   switch(code) {
      case 200: /* OK */
         queue_string(d, "HTTP/1.1 200 OK\r\n");
         break;
      case 400: /* Bad Request */
         queue_string(d, "HTTP/1.1 400 Bad Request\r\n");
         break;
      case 403: /* Forbidden */
         queue_string(d, "HTTP/1.1 403 Forbidden\r\n");
         break;
      case 404: /* Not Found */
         queue_string(d, "HTTP/1.1 404 Not Found\r\n");
         break;
      case 500: /* Internal Server Error */
         queue_string(d, "HTTP/1.1 500 Internal Server Error\r\n");
         break;
      case 501: /* SSL not enabled */
       queue_string(d, "HTTP/1.1 501 Not Implemented\r\n");
         break;
   }

   /* Standard header */
   queue_string(d, "Content-type: text/plain\r\n");
   queue_string(d, unsafe_tprintf("Date: %s", s_dtime));

   /* Additional fields */
   if ( buf1 && *buf1 ) {
      queue_string(d, buf1);
   }

   if ( buf2 && *buf2 ) {
      queue_string(d, buf2);
   }

   if ( buf3 && *buf3 ) {
      queue_string(d, buf3);
   }

   if ( buf4 && *buf4 ) {
      queue_string(d, buf4);
   }
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
	    /* freed-slot safety: d->cold is NULL for freed slots */
	    if (!d->cold) continue;
	    if (d->cold->snooplist) {
		if (found == 0)
		    notify(player, "Snooped Players:");
		found++;
		notify(player, unsafe_tprintf("  %s [port %d]", Name(D_PLAYER(d)), D_DESCRIPTOR(d)));
		for (node = d->cold->snooplist; node; node = node->next) {
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
	    notify(player, "There are no players being snooped on.");
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
	DESC *d_tmp1;
	DESC_SAFEITER_PLAYER(victim, d, d_tmp1) {
	    (void)d_tmp1;
	    for (node = d->cold->snooplist; node && node->snooper != Owner(oper);
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
                                 Name(victim), Name(d->cold->snooplist->snooper)));
		    VOIDRETURN; /* #106 */
		}
	    }
	}
	DESC *d_tmp2;
	DESC_SAFEITER_PLAYER(victim, d, d_tmp2) {
	    (void)d_tmp2;
	    node = (struct SNOOPLISTNODE *) malloc(sizeof(
						     struct SNOOPLISTNODE));

	    if (node == NULL)
		abort();
	    node->snooper = Owner(oper);
	    if (sstat && !(d->cold->logged)) {
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
	    if (node->sfile && !(d->cold->logged)) {
		d->cold->logged = 1;
		fprintf(node->sfile, "Snoop attached: [%s/%s] (%ld) %s", 
                        d->cold->addr, d->cold->longaddr, mudstate_hot.now, ctime(&mudstate_hot.now));
	    }
	    node->next = d->cold->snooplist;
	    d->cold->snooplist = node;
	}
	notify_quiet(player, "Snoop attached.");
	VOIDRETURN; /* #106 */
    } else if ( key == SNOOP_OFF ) {
	found = 0;
	DESC_ITER_PLAYER(victim, d) {
	    if (d->cold->snooplist != NULL) {
		if (d->cold->snooplist->snooper == Owner(oper)) {
		    node = d->cold->snooplist->next;
		    if (d->cold->snooplist->sfile) {
			fprintf(d->cold->snooplist->sfile, 
                                "Snoop detached: (%ld) %s", mudstate_hot.now, 
                                ctime(&mudstate_hot.now));
			fclose(d->cold->snooplist->sfile);
			d->cold->logged = 0;
		    }
		    free(d->cold->snooplist);
		    d->cold->snooplist = node;
		    found = 1;
		} else {
		    for (prev = d->cold->snooplist, node = d->cold->snooplist;
			 node && node->snooper != Owner(oper);
			 prev = node, node = node->next) {
		    }
		    if (node != NULL) {
			prev->next = node->next;
			if (node->sfile) {
			    fprintf(node->sfile, "Snoop detached: (%ld) %s", mudstate_hot.now, ctime(&mudstate_hot.now));
			    fclose(node->sfile);
			    d->cold->logged = 0;
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
  int i_hot_size, i_cold_size;

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

  /* Reboot format v4: SoA — write version marker, then hot fields (struct order), then cold bulk */
  i_hot_size = 8 * (int)sizeof(int) + 4 * (int)sizeof(void *) + (int)sizeof(time_t) + (int)sizeof(dbref);
  i_cold_size = sizeof(DESC_COLD);
  int i_reboot_version = 4;
  if (!fwrite(&i_reboot_version, sizeof(i_reboot_version), 1, rebootfile) ||
      !fwrite(&i_hot_size, sizeof(i_hot_size), 1, rebootfile) ||
      !fwrite(&i_cold_size, sizeof(i_cold_size), 1, rebootfile)) {
    STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
      log_text((char *) "Error writing DESC sizes to reboot file.");
    ENDLOG
    fclose(rebootfile);
    fclose(suffixfile);
    RETURN(0); /* #107 */
  }

  i_prefix = mkattr("____OUTPUTPREFIX");
  i_suffix = mkattr("____OUTPUTSUFFIX");
  /* Pass 1 — save non-API connections.
   * Must use SAFEITER (backward) to avoid the forward-iteration compaction
   * bug: shutdownsock in pass 2 would compact slot-i with slot-last, losing
   * slot-last's data from the reboot save under forward iteration. */
  DESC_SAFEITER_ALL(d) {
    if (!d->cold) continue;
    if (D_FLAGS(d) & DS_API) continue;
    if (D_FLAGS(d) & DS_HAS_DOOR) {
      closeDoorWithId(d, d->cold->door_num);
    }
    if ( (i_prefix > 0 ) && d->cold->output_prefix && *(d->cold->output_prefix) && Good_chk(D_PLAYER(d)) ) {
       if (!fwrite(d->cold->output_prefix, LBUF_SIZE, 1, suffixfile)) {
          STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
            log_text((char *) "Error writing to reboot file.");
          ENDLOG
          fclose(rebootfile);
          fclose(suffixfile);
          RETURN(0); /* #107 */
       }
       D_FLAGS(d) |= DS_HAVEpFX;
    }
    if ( (i_suffix > 0 ) && d->cold->output_suffix && *(d->cold->output_suffix) && Good_chk(D_PLAYER(d)) ) {
       if (!fwrite(d->cold->output_suffix, LBUF_SIZE, 1, suffixfile)) {
          STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
            log_text((char *) "Error writing to reboot file.");
          ENDLOG
          fclose(rebootfile);
          fclose(suffixfile);
          RETURN(0); /* #107 */
       }
       D_FLAGS(d) |= DS_HAVEsFX;
    }
    /* Null telnet pointer before writing cold struct (stale across processes) */
    void *saved_telnet = d->cold->telnet;
    d->cold->telnet = NULL;
    /* Write hot fields individually (SoA format v4) */
    if( !fwrite(&D_DESCRIPTOR(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_FLAGS(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_QUOTA(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_HOST_INFO(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_PLAYER(d), sizeof(dbref), 1, rebootfile) ||
        !fwrite(&D_INPUT_HEAD(d), sizeof(CBLK *), 1, rebootfile) ||
        !fwrite(&D_INPUT_TAIL(d), sizeof(CBLK *), 1, rebootfile) ||
        !fwrite(&D_OUTPUT_HEAD(d), sizeof(TBLOCK *), 1, rebootfile) ||
        !fwrite(&D_OUTPUT_TAIL(d), sizeof(TBLOCK *), 1, rebootfile) ||
        !fwrite(&D_INPUT_TOT(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_INPUT_SIZE(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_OUTPUT_TOT(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_OUTPUT_SIZE(d), sizeof(int), 1, rebootfile) ||
        !fwrite(&D_LAST_TIME(d), sizeof(time_t), 1, rebootfile) ||
        !fwrite(d->cold, sizeof(DESC_COLD), 1, rebootfile) ) {
      d->cold->telnet = saved_telnet;
      STARTLOG(LOG_PROBLEMS, "RBT", "DUMP")
        log_text((char *) "Error writing to reboot file.");
      ENDLOG
      fclose(rebootfile);
      fclose(suffixfile);
      RETURN(0); /* #107 */
    }

    /* clean up logged snoops */
    for( slnptr = d->cold->snooplist; slnptr; slnptr = slnptr->next ) {
      if( slnptr->sfile ) { 
        fprintf(slnptr->sfile, "*** Log closed due to reboot ***\n");
        fclose(slnptr->sfile);
      }
    }
    /* Restore telnet pointer (was nulled for serialization) */
    d->cold->telnet = saved_telnet;
  }

  /* Pass 2 — close API connections. SAFEITER handles compaction safely:
   * backward iteration means slot-last is already visited, and the compacted
   * data at slot-i is safely skipped by the continue below. */
  DESC_SAFEITER_ALL(d) {
    if (!d->cold) continue;
    if (!(D_FLAGS(d) & DS_API)) continue;
    if (D_FLAGS(d) & DS_HAS_DOOR) {
      closeDoorWithId(d, d->cold->door_num);
    }
    shutdownsock(d, R_API);
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
  FILE* rebootfile = NULL, *suffixfile = NULL, *sizefile = NULL;
  char rebootfilename[32 + 8], suffixfilename[32 + 8], rebootsizeref[32 + 8], *s_text, *s_god;
  int i_prefix, i_suffix, i_fxchk;
  int i_file_hot_size, i_file_cold_size;
  int i_cold_read;
  int i_is_old_format = 0;
  int i_old_descsize = 0;

  DPUSH; /* #108 */

  STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
    log_text((char *) "Loading reboot data...");
  ENDLOG

  i_fxchk = ndescriptors = ndesc_slots = 0;

  strcpy(rebootfilename, mudconf.muddb_name);
  strcpy(suffixfilename, mudconf.muddb_name);
  strcpy(rebootsizeref, mudconf.muddb_name);
  strcat(rebootfilename, ".reboot");
  strcat(suffixfilename, ".fx");
  strcat(rebootsizeref, ".size");

  rebootfile = fopen(rebootfilename, "rb");
  if( !rebootfile ) {
    STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
      log_text((char *) "Unable to open reboot file for reading.");
    ENDLOG
    RETURN(0); /* #108 */
  }

  /* Detect old format: if .size file exists, it's pre hot/cold split */
  sizefile = fopen(rebootsizeref, "rb");
  if ( sizefile ) {
     if ( !fread(&i_old_descsize, sizeof(i_old_descsize), 1, sizefile) ) {
        STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
           log_text((char *) "Error reading DESC SIZE file. Assuming baseline DESC.");
        ENDLOG
        i_old_descsize = sizeof(DESC_OLD);
     }
     fclose(sizefile);
     sizefile = NULL;
     if ( i_old_descsize < (int)sizeof(DESC_OLD) ) {
        i_old_descsize = sizeof(DESC_OLD);
     }
     i_is_old_format = 1;
     STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
       log_text((char *) "Detected legacy reboot format (pre hot/cold split). Migrating to v3.");
     ENDLOG
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

  if ( i_is_old_format ) {
     /* Legacy format: no version marker, descriptors are raw binary structs */
     i_prefix = mkattr("____OUTPUTPREFIX");
     i_suffix = mkattr("____OUTPUTSUFFIX");
     s_text = alloc_lbuf("reboot_fx");

     while(!feof(rebootfile)) {
        DESC_OLD old_desc;
        memset(&old_desc, 0, sizeof(old_desc));

        if( !fread(&old_desc, i_old_descsize, 1, rebootfile) ) {
           if( feof(rebootfile) ) {
              break;
           }
           STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
             log_text((char *) "Error reading from reboot file (legacy format).");
           ENDLOG
           fclose(rebootfile);
           if ( !i_fxchk ) fclose(suffixfile);
           free_lbuf(s_text);
           RETURN(0); /* #108 */
        }

        /* Skip empty/invalid descriptors */
        if ( old_desc.descriptor < 0 ) {
           continue;
        }

        d = alloc_desc("reboot_migrate");
        if (!d) {
           STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
             log_text((char *) "Too many descriptors in reboot file for current MAX_DESCRIPTORS. Skipping excess connections.");
           ENDLOG
           break;
        }

        /* Map hot fields */
        D_DESCRIPTOR(d) = old_desc.descriptor;
        D_FLAGS(d) = old_desc.flags;
        D_QUOTA(d) = old_desc.quota;
        D_HOST_INFO(d) = old_desc.host_info;
        D_PLAYER(d) = old_desc.player;
        D_INPUT_HEAD(d) = old_desc.input_head;   /* stale pointer, nulled below */
        D_INPUT_TAIL(d) = old_desc.input_tail;   /* stale pointer, nulled below */
        D_OUTPUT_HEAD(d) = old_desc.output_head; /* stale pointer, nulled below */
        D_OUTPUT_TAIL(d) = old_desc.output_tail; /* stale pointer, nulled below */
        D_INPUT_TOT(d) = old_desc.input_tot;
        D_INPUT_SIZE(d) = old_desc.input_size;
        D_OUTPUT_TOT(d) = old_desc.output_tot;
        D_OUTPUT_SIZE(d) = old_desc.output_size;
        D_LAST_TIME(d) = old_desc.last_time;

        /* Map cold fields via memcpy for contiguous arrays and scalars */
        memcpy(d->cold->addr, old_desc.addr, sizeof(old_desc.addr));
        memcpy(d->cold->doing, old_desc.doing, sizeof(old_desc.doing));
        memcpy(d->cold->userid, old_desc.userid, sizeof(old_desc.userid));
        memcpy(d->cold->longaddr, old_desc.longaddr, sizeof(old_desc.longaddr));
        memcpy(d->cold->account_rawpass, old_desc.account_rawpass, sizeof(old_desc.account_rawpass));
        memcpy(d->cold->checksum, old_desc.checksum, sizeof(old_desc.checksum));
        d->cold->retries_left = old_desc.retries_left;
        d->cold->regtries_left = old_desc.regtries_left;
        d->cold->command_count = old_desc.command_count;
        d->cold->timeout = old_desc.timeout;
        d->cold->output_lost = old_desc.output_lost;
        d->cold->input_lost = old_desc.input_lost;
        d->cold->logged = old_desc.logged;
        d->cold->authdescriptor = old_desc.authdescriptor;
        d->cold->longaddrcheck = old_desc.longaddrcheck;
        d->cold->ws_frame_len = old_desc.ws_frame_len;
        d->cold->connected_at = old_desc.connected_at;
        d->cold->account_owner = old_desc.account_owner;
        memcpy(&d->cold->address, &old_desc.address, sizeof(old_desc.address));

        /* Old format was IPv4-only */
        d->cold->addr_family = AF_INET;
        d->cold->remote_port = 0;

        /* Null all stale pointers from old process */
        d->cold->output_prefix = NULL;
        d->cold->output_suffix = NULL;
        d->cold->raw_input = NULL;
        d->cold->raw_input_at = NULL;
        d->cold->snooplist = NULL;
        d->cold->door_lbuf = NULL;
        d->cold->door_mbuf = NULL;
        d->cold->door_raw = NULL;
        d->cold->door_output_head = NULL;
        d->cold->door_output_tail = NULL;
        d->cold->door_input_head = NULL;
        d->cold->door_input_tail = NULL;

        /* Read prefix/suffix from .fx file */
        if ( (i_prefix > 0) && (old_desc.flags & DS_HAVEpFX) && Good_chk(old_desc.player) ) {
           if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
              set_userstring(&d->cold->output_prefix, s_text);
        }
        if ( (i_suffix > 0) && Good_chk(old_desc.player) && (old_desc.flags & DS_HAVEsFX) ) {
           if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
              set_userstring(&d->cold->output_suffix, s_text);
        }

        /* Clear transient state */
        D_FLAGS(d) &= ~(DS_HAVEpFX|DS_HAVEsFX);
        D_OUTPUT_SIZE(d) = 0;
        D_OUTPUT_HEAD(d) = NULL;
        D_OUTPUT_TAIL(d) = NULL;
        D_INPUT_SIZE(d) = 0;
        D_INPUT_HEAD(d) = NULL;
        D_INPUT_TAIL(d) = NULL;
        d->cold->door_desc = 0;
        d->cold->door_output_size = 0;
        d->cold->snooplist = NULL;
        d->cold->telnet = NULL;
        telnet_init_desc(d);
        d->cold->logged = 0;

        ndescriptors++;
        ndesc_slots++;

        s_Connected(D_PLAYER(d));
        STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
          log_text((char*) "Reconnecting (migrated): ");
          log_name(D_PLAYER(d));
        ENDLOG
        mudstate.recordcurrconn++;
     }
     free_lbuf(s_text);
     /* Clean up legacy .size file */
     unlink(rebootsizeref);
  } else {
     /* v4 format: read version marker and sizes */
     int i_reboot_version = 0;
     if( !fread(&i_reboot_version, sizeof(i_reboot_version), 1, rebootfile) ||
         !fread(&i_file_hot_size, sizeof(i_file_hot_size), 1, rebootfile) ||
         !fread(&i_file_cold_size, sizeof(i_file_cold_size), 1, rebootfile) ) {
       STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
         log_text((char *) "Error reading reboot file header.");
       ENDLOG
       fclose(rebootfile);
       if ( !i_fxchk ) fclose(suffixfile);
       RETURN(0); /* #108 */
     }
     (void)i_file_hot_size; /* read for file position only — hot fields are individually serialized, sizes are implicit from types */

     if ( i_reboot_version != 4 ) {
       STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
         log_text((char *) "Reboot file format is not v4. Delete .reboot file and restart.");
       ENDLOG
       fclose(rebootfile);
       if ( !i_fxchk ) fclose(suffixfile);
       RETURN(0); /* #108 */
     }

     if ( i_file_cold_size > sizeof(DESC_COLD) ) {
       STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
         log_text((char *) "Reboot file DESC_COLD struct is larger than current. Cannot load safely.");
       ENDLOG
       fclose(rebootfile);
       if ( !i_fxchk ) fclose(suffixfile);
       RETURN(0); /* #108 */
     }

     i_cold_read = (i_file_cold_size < sizeof(DESC_COLD)) ? i_file_cold_size : sizeof(DESC_COLD);

     i_prefix = mkattr("____OUTPUTPREFIX");
     i_suffix = mkattr("____OUTPUTSUFFIX");
     s_text = alloc_lbuf("reboot_fx");
     while(!feof(rebootfile)) {
       d = alloc_desc("reboot_sock");
       if (!d) {
         STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
           log_text((char *) "Too many descriptors in reboot file for current MAX_DESCRIPTORS. Skipping excess connections.");
         ENDLOG
         break;
       }
        /* Read hot fields individually (SoA format v4 — struct field order) */
        if( !fread(&D_DESCRIPTOR(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_FLAGS(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_QUOTA(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_HOST_INFO(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_PLAYER(d), sizeof(dbref), 1, rebootfile) ||
            !fread(&D_INPUT_HEAD(d), sizeof(CBLK *), 1, rebootfile) ||
            !fread(&D_INPUT_TAIL(d), sizeof(CBLK *), 1, rebootfile) ||
            !fread(&D_OUTPUT_HEAD(d), sizeof(TBLOCK *), 1, rebootfile) ||
            !fread(&D_OUTPUT_TAIL(d), sizeof(TBLOCK *), 1, rebootfile) ||
            !fread(&D_INPUT_TOT(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_INPUT_SIZE(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_OUTPUT_TOT(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_OUTPUT_SIZE(d), sizeof(int), 1, rebootfile) ||
            !fread(&D_LAST_TIME(d), sizeof(time_t), 1, rebootfile) ||
            !fread(d->cold, i_cold_read, 1, rebootfile) ) {
         if( feof(rebootfile) ) {
           free_desc(d);
           break;
         }
         STARTLOG(LOG_PROBLEMS, "RBT", "LOAD")
           log_text((char *) "Error reading from reboot file.");
         ENDLOG
         free_desc(d);
         fclose(rebootfile);
         RETURN(0); /* #108 */
       }
       d->cold->output_prefix = NULL;
       if ( (i_prefix > 0) && (D_FLAGS(d) & DS_HAVEpFX) && Good_chk(D_PLAYER(d)) ) {
          if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
             set_userstring(&d->cold->output_prefix, s_text);
       }
       d->cold->output_suffix = NULL;
       if ( (i_suffix > 0) && Good_chk(D_PLAYER(d)) && (D_FLAGS(d) & DS_HAVEsFX) ) {
          if ( !i_fxchk && fread(s_text, LBUF_SIZE, 1, suffixfile) )
             set_userstring(&d->cold->output_suffix, s_text);
       }
       D_FLAGS(d) &= ~(DS_HAVEpFX|DS_HAVEsFX);
       D_OUTPUT_SIZE(d) = 0;
       D_OUTPUT_HEAD(d) = NULL;
       D_OUTPUT_TAIL(d) = NULL;
       D_INPUT_SIZE(d) = 0;
       D_INPUT_HEAD(d) = NULL;
       D_INPUT_TAIL(d) = NULL;
       d->cold->door_desc = 0;
       d->cold->door_output_head = NULL;
       d->cold->door_output_tail = NULL;
       d->cold->door_input_head = NULL;
       d->cold->door_input_tail = NULL;
       d->cold->door_raw = NULL;
       d->cold->door_output_size = 0;
       d->cold->door_lbuf = NULL;
       d->cold->door_mbuf = NULL;
        d->cold->raw_input = NULL;
        d->cold->raw_input_at = NULL;
        d->cold->snooplist = NULL;
        d->cold->telnet = NULL;
        telnet_init_desc(d);
        d->cold->logged = 0;

        ndescriptors++;
        ndesc_slots++;

        s_Connected(D_PLAYER(d));
        STARTLOG(LOG_ALWAYS, "RBT", "LOAD")
          log_text((char*) "Reconnecting: ");
         log_name(D_PLAYER(d));
       ENDLOG
       mudstate.recordcurrconn++;
     }
     free_lbuf(s_text);
  }
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
  if ( mudstate.recordcurrconn > mudstate.recordconn ) {
     mudstate.recordconn = mudstate.recordcurrconn;
     s_god = alloc_lbuf("record_connections_reboot");
     sprintf(s_god, "%d", mudstate.recordcurrconn);
     atr_add_raw(GOD, A_CONNRECORD, s_god);
     free_lbuf(s_god);
  }
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

    if ( mudconf.timeslice <= 0 ) {
       mudconf.timeslice = 1;
    }
    nslices = msec_diff(current, last) / mudconf.timeslice;

    if (nslices > 0) {
	DESC_ITER_ALL(d) {
            /* IF customquotas exceed the max current values, ignore it and carry on */
            if ( (D_FLAGS(d) & DS_CMDQUOTA) ) {
               if ( Good_chk(D_PLAYER(d)) && 
                    ((Wizard(D_PLAYER(d)) && D_QUOTA(d) > mudconf.wizcmd_quota_max) ||
                     (!Wizard(D_PLAYER(d)) && D_QUOTA(d) > mudconf.cmd_quota_max)) ) {
                  continue;
               }
               /* Cleanup the flag */
               D_FLAGS(d) &= ~DS_CMDQUOTA;
            }
	    D_QUOTA(d) += mudconf.cmd_quota_incr * nslices;
            if ( Good_chk(D_PLAYER(d)) && Wizard(D_PLAYER(d)) ) {
	       if (D_QUOTA(d) > mudconf.wizcmd_quota_max)
		   D_QUOTA(d) = mudconf.wizcmd_quota_max;
            } else {
	       if (D_QUOTA(d) > mudconf.cmd_quota_max)
		   D_QUOTA(d) = mudconf.cmd_quota_max;
            }
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
    DESC *d, *sd;
    struct SNOOPLISTNODE *node;
    char antemp[20];
    int found;

    DPUSH; /* #112 */

    if (!msg || !*msg) {
	VOIDRETURN; /* #112 */
    }

/* Potential fix for API */
//  if (!Connected(player) && !port) {
//	VOIDRETURN; /* #112 */
//  }
    if ( !Good_obj(player) && !port ) {
       VOIDRETURN; /* #112 */
    }

    strcpy(antemp, ANSI_NORMAL);
    if (!port) {
      DESC *d_tmp;
      DESC_SAFEITER_PLAYER(player, d, d_tmp) {
      (void)d_tmp;
	if ((D_FLAGS(d) & DS_HAS_DOOR) && !mudstate.droveride && (Flags3(D_PLAYER(d)) & DOORRED)) {
	  continue;
        }
	queue_string(d, msg);
	if (ShowAnsi(D_PLAYER(d)) && index(msg, ESC_CHAR)) {
	    queue_string(d, antemp);
        }
        if ( type ) {
	   queue_write(d, "\r\n", 2);
        }
	if (d->cold->snooplist) {
	    for (node = d->cold->snooplist; node; node = node->next) {
		if (node->sfile) {
		    fprintf(node->sfile, "%ld|", mudstate_hot.now);
		    fputs(Name(player), node->sfile);
		    fputs("> ", node->sfile);
		    fputs(strip_ansi(msg), node->sfile);
		    fputc('\n', node->sfile);
                    fflush(node->sfile);
		}
		if (!Connected(node->snooper) || node->logonly) {
		    continue;
                }
		DESC *sd_tmp;
		DESC_SAFEITER_PLAYER(node->snooper, sd, sd_tmp) {
		(void)sd_tmp;
		    if ((D_FLAGS(sd) & DS_HAS_DOOR) && (Flags3(D_PLAYER(sd)) & DOORRED)) {
			continue;
                    }
                    if( ShowAnsi(D_PLAYER(sd)) ) {
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
                    } else {
		      queue_string(sd, "|");
		      queue_string(sd, Name(player));
		      queue_string(sd, "> ");
		      queue_string(sd, msg);
                    }
                    if ( type ) {
		       queue_write(sd, "\r\n", 2);
                    }
                    process_output(sd);
		}
	    }
	}
        process_output(d);
      }
    } else {
      found = 0;
      DESC_SAFEITER_ALL(d) {
	if (!d->cold) continue;
	if (D_DESCRIPTOR(d) != port) {
	  continue;
        }
	if (!(D_PLAYER(d)) && !Immortal(mudstate.pageref)) {
	  mudstate.pageref = NOTHING;
	  break;
	}
	if (Immortal(D_PLAYER(d)) && SCloak(D_PLAYER(d)) && !Immortal(mudstate.pageref)) {
	  mudstate.pageref = NOTHING;
	  break;
	}
	found = 1;
	if ((D_FLAGS(d) & DS_HAS_DOOR) && !mudstate.droveride && (Flags3(D_PLAYER(d)) & DOORRED)) {
	  continue;
        }
	queue_string(d, msg);
	if (D_PLAYER(d) && ShowAnsi(D_PLAYER(d)) && index(msg, ESC_CHAR)) {
	    queue_string(d, antemp);
        }
	queue_write(d, "\r\n", 2);
	if (d->cold->snooplist) {
	    for (node = d->cold->snooplist; node; node = node->next) {
		if (node->sfile) {
		    fprintf(node->sfile, "%ld|", mudstate_hot.now);
		    fputs(Name(D_PLAYER(d)), node->sfile);
		    fputs("> ", node->sfile);
		    fputs(strip_ansi(msg), node->sfile);
		    fputc('\n', node->sfile);
                    fflush(node->sfile);
		}
		if (!Connected(node->snooper) || node->logonly) {
		    continue;
                }
		DESC *sd_tmp;
		DESC_SAFEITER_PLAYER(node->snooper, sd, sd_tmp) {
		(void)sd_tmp;
		    if ((D_FLAGS(sd) & DS_HAS_DOOR) && (Flags3(D_PLAYER(sd)) & DOORRED)) {
			continue;
                    }
                    if( ShowAnsi(D_PLAYER(sd)) ) {
                      queue_string(sd, ANSI_HILITE);
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "|");
                      queue_string(sd, ANSI_GREEN);
		      queue_string(sd, Name(D_PLAYER(d)));
                      queue_string(sd, ANSI_WHITE);
		      queue_string(sd, "| ");
                      queue_string(sd, ANSI_NORMAL);
		      queue_string(sd, msg);
		      queue_string(sd, antemp);
                    } else {
		      queue_string(sd, "|");
		      queue_string(sd, Name(D_PLAYER(d)));
		      queue_string(sd, "> ");
		      queue_string(sd, msg);
                    }
		    queue_write(sd, "\r\n", 2);
                    process_output(sd);
		}
	    }
	}
        process_output(d);
	break;
      }
      if (!found) {
        mudstate.pageref=NOTHING;
      }
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
       if ( site && (inflags & (MF_CONN|MF_DCONN|MF_FAIL|MF_BFAIL|MF_GFAIL|MF_COMPFAIL)) && *(mudconf.nobroadcast_host) ) {
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
        if ((inflags & MF_API) && !HasPriv(D_PLAYER(d), NOTHING, POWER_MONITORAPI, POWER5, NOTHING))
            continue;
        if ((inflags & MF_COMPFAIL) && (Toggles2(D_PLAYER(d)) & TOG_MONITOR_BFAIL))
            continue;
        if ((inflags & MF_BFAIL) && !(Toggles2(D_PLAYER(d)) & TOG_MONITOR_BFAIL))
            continue;
	if ((inflags & MF_FAIL) && !(Toggles(D_PLAYER(d)) & TOG_MONITOR_FAIL))
	    continue;
	if ((inflags & MF_VLIMIT) && !(Toggles(D_PLAYER(d)) & TOG_MONITOR_VLIMIT))
	    continue;
	if ((inflags & MF_CPU) && !(Toggles(D_PLAYER(d)) & TOG_MONITOR_CPU))
            continue;
        if ((inflags & MF_CPUEXT) && !(Toggles(D_PLAYER(d)) & TOG_MONITOR_CPU))
            continue;
	if ((D_FLAGS(d) & DS_HAS_DOOR) && (Flags3(D_PLAYER(d)) & DOORRED))
	    continue;
	if ((inflags & MF_DCONN) && !Immortal(D_PLAYER(d)))
	    continue;
	if (inflags & MF_DCONN)
	    inflags &= ~MF_DCONN;
	if (inflags & MF_CONN) {
	    if ( (Toggles(D_PLAYER(d)) & TOG_MONITOR_CONN) && (Toggles(D_PLAYER(d)) & TOG_MONITOR) ) {
		if( ShowAnsi(D_PLAYER(d)) ) 
                  queue_string(d, ANSI_HILITE);
                if (inflags & MF_API ) {
		   queue_string(d, "[MONITOR] API ");
                } else {
		   queue_string(d, "[MONITOR] ");
                }
		if( ShowAnsi(D_PLAYER(d)) ) 
                  queue_string(d, ANSI_NORMAL);
		if (Wizard(D_PLAYER(d)) || (pt1 == NULL) || SeeSuspect(D_PLAYER(d)))
		  queue_string(d, type);
		else
		  queue_string(d, b3);
		if (reason && TogReason(D_PLAYER(d))) {
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
                if ( Toggles(D_PLAYER(d)) & TOG_MONITOR_TIME ) {
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
	  if ( (Toggles(D_PLAYER(d)) & TOG_MONITOR_AREG) && (Toggles(D_PLAYER(d)) & TOG_MONITOR) )  {
		if( ShowAnsi(D_PLAYER(d)) ) 
                  queue_string(d, ANSI_HILITE);
		queue_string(d, "[MONITOR] ");
		if( ShowAnsi(D_PLAYER(d)) ) 
                  queue_string(d, ANSI_NORMAL);
		queue_string(d, type);
		if (userid && *userid)
		  sprintf(buff, " | Port: %d, Site: %s@%s, Char/Email: %s", succ, userid, site, reason);
		else
		  sprintf(buff, " | Port: %d, Site: %s, Char/Email: %s", succ, site, reason);
		queue_string(d, buff);
                if ( Toggles(D_PLAYER(d)) & TOG_MONITOR_TIME ) {
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
	if (((Toggles(D_PLAYER(d)) & TOG_MONITOR) &&
	((((player != NOTHING) && !Cloak(player)) || Immortal(D_PLAYER(d)) || (inflags & (MF_FAIL|MF_BFAIL)))))) {
	    if( ShowAnsi(D_PLAYER(d)) ) 
              queue_string(d, ANSI_HILITE);
	    queue_string(d, "[MONITOR] ");
	    if( ShowAnsi(D_PLAYER(d)) ) 
              queue_string(d, ANSI_NORMAL);
	    if (Wizard(D_PLAYER(d)) || (pt1 == NULL) || SeeSuspect(D_PLAYER(d)))
	      queue_string(d, type);
	    else
	      queue_string(d, b3);
	    if (reason && TogReason(D_PLAYER(d))) {
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
                if ( Toggles(D_PLAYER(d)) & TOG_MONITOR_TIME ) {
                   t = time(NULL);
                   tp = localtime(&t);
                   sprintf(buff, " [%02d:%02d:%02d]", tp->tm_hour, tp->tm_min, tp->tm_sec );
	           queue_string(d, buff);
                }
		queue_write(d, "\r\n", 2);
		process_output(d);
		continue;
	    }
	    if (((inflags & (MF_SITE)) && Toggles(D_PLAYER(d)) & (TOG_MONITOR_USERID | TOG_MONITOR_SITE)))
		queue_string(d, " | ");
	    if ((Toggles(D_PLAYER(d)) & TOG_MONITOR_USERID) &&
		(*userid)) {
		queue_string(d, userid);
		queue_string(d, "@");
	    }
	    if (Toggles(D_PLAYER(d)) & (TOG_MONITOR_SITE | TOG_MONITOR_USERID))
		queue_string(d, site);
	    if ((inflags & MF_STATS) &&
		(Toggles(D_PLAYER(d)) & TOG_MONITOR_STATS)) {
		sprintf(buff, " (Succ: %d | Fail: %d/%d)",
			succ, recentfail, fail);
		queue_string(d, buff);
	    }
            if ( Toggles(D_PLAYER(d)) & TOG_MONITOR_TIME ) {
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
    char *message, *mptr, *msg_ns2, *mp_ns2, *msg_utf, *mp_utf;
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
	mp_utf = msg_utf = alloc_lbuf("notify_check_utf");
    parse_ansi( (char *) buff, message, &mptr, msg_ns2, &mp_ns2, msg_utf, &mp_utf);
    *mp_ns2 = '\0';
	*mp_utf = '\0';
#endif   
    strcpy(antemp, ANSI_NORMAL);
    DESC_ITER_CONN(d) {
	if ((!(((Flags2(D_PLAYER(d)) & NO_WALLS) || ((D_FLAGS(d) & DS_HAS_DOOR) && (Flags3(D_PLAYER(d)) & DOORRED))) 
	  && sender != 0 && !Wizard(sender)) || mudstate.nowall_over) &&
	    ((inflags == 0) ||
	     ((inflags & GUILDMASTER) && Guildmaster(D_PLAYER(d))) ||
	     ((inflags & BUILDER) && Builder(D_PLAYER(d))) ||
	     ((inflags & ADMIN) && Admin(D_PLAYER(d))) ||
	     ((inflags & WIZARD) && Wizard(D_PLAYER(d))) ||
	     ((inflags & IMMORTAL) && Immortal(D_PLAYER(d))))) {
           if ( i_nowalls && (Flags2(D_PLAYER(d)) & NO_WALLS) ) 
              continue;
#ifdef ZENTY_ANSI
		   if ( UTF8(D_PLAYER(d)) )
			  queue_string(d, msg_utf);
           else if ( Accents(D_PLAYER(d) ) )
              queue_string(d, msg_ns2);
           else
	      queue_string(d, strip_safe_accents(message));
#else	   
	   queue_string(d, buff);
#endif	   
	    if (ShowAnsi(D_PLAYER(d)) && index(buff, ESC_CHAR))
		queue_string(d, antemp);
	    queue_write(d, "\r\n", 2);
	    process_output(d);
	}
    }
    free_lbuf(buff);
#ifdef ZENTY_ANSI
    free_lbuf(message);
    free_lbuf(msg_ns2);
	free_lbuf(msg_utf);
#endif
    va_end(ap);
    VOIDRETURN; /* #114 */
}

/* ---------------------------------------------------------------------------
 * raw_broadcast2: Send message to players who have indicated flags
 * DONT USE THIS ANYMORE, USE THE ABOVE -2/95 Thorin
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
	if ((inflags != WIZARD) && ((Flags2(D_PLAYER(d)) & inflags) == inflags)) {
	    if ((((!(Flags2(D_PLAYER(d)) & NO_WALLS)) || mudstate.nowall_over) && (!Wizard(D_PLAYER(d)))) || (inflags == 0)) {
		if (((inflags == BUILDER) && !Builder(D_PLAYER(d))) || (inflags == ADMIN) || (inflags == 0)) {
		    queue_string(d, buff);
		    if (ShowAnsi(D_PLAYER(d)) && index(buff, ESC_CHAR))
			queue_string(d, antemp);
		    queue_write(d, "\r\n", 2);
		    process_output(d);
		}
	    }
	} else if (Immortal(D_PLAYER(d)) && (inflags == WIZARD)) {
	    queue_string(d, buff);
	    if (ShowAnsi(D_PLAYER(d)) && index(buff, ESC_CHAR))
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
    if (d->cold->output_prefix) {
	free_lbuf(d->cold->output_prefix);
	d->cold->output_prefix = NULL;
    }
    if (d->cold->output_suffix) {
	free_lbuf(d->cold->output_suffix);
	d->cold->output_suffix = NULL;
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

    if (!d) {
        VOIDRETURN;
    }

    if (!d->cold) {
        VOIDRETURN;
    }

    if (D_OUTPUT_SIZE(d) + n > mudconf.output_limit)
	process_output(d);

    left = mudconf.output_limit - D_OUTPUT_SIZE(d) - n;
    if (left < 0) {
        mudstate_hot.outputflushed = 1;
	tp = D_OUTPUT_HEAD(d);
	if (tp == NULL) {
	    STARTLOG(LOG_PROBLEMS, "QUE", "WRITE")
		log_text((char *) "Flushing when output_head is null!");
	    ENDLOG
	} else {
	    STARTLOG(LOG_NET, "NET", "WRITE")
            buf = alloc_lbuf("queue_write.LOG");
	    sprintf(buf,
		    "[%d/%s] Output buffer overflow, %d chars discarded by ",
		    D_DESCRIPTOR(d), d->cold->longaddr, tp->hdr.nchars);
	    log_text(buf);
	    free_lbuf(buf);
	    log_name(D_PLAYER(d));
	    ENDLOG
            D_OUTPUT_SIZE(d) -= tp->hdr.nchars;
	    D_OUTPUT_HEAD(d) = tp->hdr.nxt;
	    d->cold->output_lost += tp->hdr.nchars;
	    if (D_OUTPUT_HEAD(d) == NULL)
		D_OUTPUT_TAIL(d) = NULL;
	    free_lbuf(tp);
	}
    }


#ifdef ENABLE_WEBSOCKETS
    ///// NEW WEBSOCK
    if (D_FLAGS(d) & DS_WEBSOCKETS) {
        /* Format output for websockets */
        websocket_write(d, b, n);
        VOIDRETURN;
    }
    ///// END NEW WEBSOCK
#endif


    /* Allocate an output buffer if needed */
    if (D_OUTPUT_HEAD(d) == NULL) {
	tp = (TBLOCK *) alloc_lbuf("queue_write.new");
	tp->hdr.nxt = NULL;
	tp->hdr.start = tp->data;
	tp->hdr.end = tp->data;
	tp->hdr.nchars = 0;
	D_OUTPUT_HEAD(d) = tp;
	D_OUTPUT_TAIL(d) = tp;
    } else {
	tp = D_OUTPUT_TAIL(d);
    }

    /* Now tp points to the last buffer in the chain */
    D_OUTPUT_SIZE(d) += n;
    D_OUTPUT_TOT(d) += n;
    mudstate.total_bytesout += n;
    if ( (mudstate.reset_daily_bytes + 86400) < mudstate_hot.now ) {
       if ( mudstate.avg_bytesin == 0 ) {
          mudstate.avg_bytesin = mudstate.daily_bytesin;
       } else {
          mudstate.avg_bytesin = (mudstate.avg_bytesin + mudstate.daily_bytesin) / 2;
       }
       if ( mudstate.avg_bytesout == 0 ) {
          mudstate.avg_bytesout = mudstate.daily_bytesout;
       } else {
          mudstate.avg_bytesout = (mudstate.avg_bytesout + mudstate.daily_bytesout) / 2;
       }
       mudstate.daily_bytesin = 0;
       mudstate.daily_bytesout = n;
       mudstate.reset_daily_bytes = time(NULL);
    } else {
       mudstate.daily_bytesout += n;
    }

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
	    D_OUTPUT_TAIL(d)->hdr.nxt = tp;
	    D_OUTPUT_TAIL(d) = tp;
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
    new = (char *) s;
    if (new && (!D_PLAYER(d) || !ShowAnsi(D_PLAYER(d))) && index(new, ESC_CHAR))
        new = strip_ansi(new);
    else if (new && !ShowAnsiTrueColor(D_PLAYER(d)) && index(new, ESC_CHAR))
        new = strip_ansi_truecolor(new);
    if (new && !ShowAnsiXterm(D_PLAYER(d)) && index(new, ESC_CHAR))
        new = strip_ansi_xterm(new);
    if (new && !ShowAnsiColor(D_PLAYER(d)) && index(new, ESC_CHAR))
        new = strip_ansi_color(new);
    if (NoFlash(D_PLAYER(d)) && index(new, ESC_CHAR))
	new = strip_ansi_flash(new);
    if (NoUnderline(D_PLAYER(d)) && index(new, ESC_CHAR))
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

  if (!d || !d->cold) {
    VOIDRETURN;
  }

  if (!dooronly) {
    tb = D_OUTPUT_HEAD(d);
    while (tb) {
	tnext = tb->hdr.nxt;
	free_lbuf(tb);
	tb = tnext;
    }
    D_OUTPUT_HEAD(d) = NULL;
    D_OUTPUT_TAIL(d) = NULL;

    cb = D_INPUT_HEAD(d);
    while (cb) {
	cnext = (CBLK *) cb->hdr.nxt;
	free_lbuf(cb);
	cb = cnext;
    }
    D_INPUT_HEAD(d) = NULL;
    D_INPUT_TAIL(d) = NULL;

    if (d->cold->raw_input)
	free_lbuf(d->cold->raw_input);
    d->cold->raw_input = NULL;
    d->cold->raw_input_at = NULL;
  }

  cb = d->cold->door_input_head;
  while (cb) {
    cnext = (CBLK *) cb->hdr.nxt;
    free_lbuf(cb);
    cb = cnext;
  }
  d->cold->door_input_head = NULL;
  d->cold->door_input_tail = NULL;

  tb = d->cold->door_output_head;
  while (tb) {
    tnext = tb->hdr.nxt;
    free_lbuf(tb);
    tb = tnext;
  }
  d->cold->door_output_head = NULL;
  d->cold->door_output_tail = NULL;
  VOIDRETURN; /* #121 */
}

void 
welcome_user(DESC * d)
{
    DPUSH; /* #124 */
    if (mudconf.offline_reg && !(D_HOST_INFO(d) & H_PERMIT) && (D_HOST_INFO(d) & H_REGISTRATION))
	fcache_dump(d, FC_CONN_AUTO_H, (char *)NULL);
    else if (mudconf.offline_reg)
	fcache_dump(d, FC_CONN_AUTO, (char *)NULL);
    else if (!(D_HOST_INFO(d) & H_PERMIT) && (D_HOST_INFO(d) & H_REGISTRATION))
	fcache_dump(d, FC_CONN_REG, (char *)NULL);
    else
	fcache_dump(d, FC_CONN, (char *)NULL);

    VOIDRETURN; /* #124 */
}

void 
save_command(DESC * d, CBLK * command)
{
    DPUSH; /* #125 */
    command->hdr.nxt = NULL;
    if (D_INPUT_TAIL(d) == NULL)
	D_INPUT_HEAD(d) = command;
    else
	D_INPUT_TAIL(d)->hdr.nxt = command;
    D_INPUT_TAIL(d) = command;
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
    char *argv[1], *s_pmt, *s_pmtptr;
    int totsucc, totfail, newfail;
#ifdef ZENTY_ANSI
    char *s_buff, *s_buff2, *s_buff3, *s_buffptr, *s_buff2ptr, *s_buff3ptr;
#endif

    DPUSH; /* #130 */

    buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags);
    if (buf) {
	d->cold->timeout = atoi(buf);
	if ( (d->cold->timeout <= 0) && (d->cold->timeout != -1) )
	    d->cold->timeout = mudconf.idle_timeout;
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
        if(mudconf.pagelock_notify)
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

    time_str = ctime(&mudstate_hot.now);
    time_str[strlen(time_str) - 1] = '\0';
    record_login(player, 1, time_str, d->cold->longaddr, &totsucc, &totfail, &newfail);
    if ( !(D_FLAGS(d) & DS_SSL) ) {
        atr_add_raw(player, A_LASTIP, d->cold->addr);
    } else {
       if ( d->cold->doing[0] != '\0' ) {
          atr_add_raw(player, A_LASTIP, d->cold->doing);
          d->cold->doing[0] = '\0';
       } else {
          atr_add_raw(player, A_LASTIP, d->cold->longaddr);
       }
    }
    if ( num > 1 ) {
       if (Suspect(player))
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "RECONNECT [SUSPECT]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (D_HOST_INFO(d) & H_SUSPECT)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "RECONNECT [SUSPECT SITE]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (D_HOST_INFO(d) & H_PASSPROXY)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "RECONNECT [PROXY BYPASS]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (dc)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_DCONN, "RECONNECT",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else 
	   broadcast_monitor(player, MF_SITE | MF_STATS, "RECONNECT",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
    } else {
       if (Suspect(player))
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "CONNECT [SUSPECT]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (D_HOST_INFO(d) & H_SUSPECT)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "CONNECT [SUSPECT SITE]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (D_HOST_INFO(d) & H_PASSPROXY)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_TRIM, "CONNECT [PROXY BYPASS]",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else if (dc)
	   broadcast_monitor(player, MF_SITE | MF_STATS | MF_DCONN, "CONNECT",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
       else 
	   broadcast_monitor(player, MF_SITE | MF_STATS, "CONNECT",
			     d->cold->userid, d->cold->longaddr, totsucc, newfail, totfail, NULL);
    }
    temp = mudstate_hot.curr_enactor;
    mudstate_hot.curr_enactor = player;
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


    look_in(player, player, Location(player), (LK_SHOWEXIT | LK_OBEYTERSE));
    if ( d && Prompt(D_PLAYER(d)) ) {
       progatr = atr_get(D_PLAYER(d), A_PROGPROMPT, &aowner, &aflags);
       if ( *progatr ) {
          s_pmt = s_pmtptr = alloc_lbuf("process_ic_command");
          queue_string(d, safe_tprintf(s_pmt, &s_pmtptr, "%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
          free_lbuf(s_pmt);
       }
       free_lbuf(progatr);
    }
    if ( InProgram(player) ) {
       if ( (mudconf.login_to_prog && !(ProgCon(player))) || 
            (!mudconf.login_to_prog && ProgCon(player)) ) {
          notify(player, "You are still in a @program.");
          DESC_ITER_CONN(d) {
             if ( D_PLAYER(d) == player ) {
                progatr = atr_get(player, A_PROGPROMPTBUF, &aowner2, &aflags2);
                if ( progatr && *progatr ) {
                   if ( strcmp(progatr, "NULL") != 0 ) {
#ifdef ZENTY_ANSI
                      s_buffptr = s_buff = alloc_lbuf("parse_ansi_prompt");
                      s_buff2ptr = s_buff2 = alloc_lbuf("parse_ansi_prompt2");
                      s_buff3ptr = s_buff3 = alloc_lbuf("parse_ansi_prompt3");
                      parse_ansi((char *) progatr, s_buff, &s_buffptr, s_buff2, &s_buff2ptr, s_buff3, &s_buff3ptr);
                      queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, s_buff, ANSI_NORMAL));
                      free_lbuf(s_buff);
                      free_lbuf(s_buff2);
                      free_lbuf(s_buff3);
#else
                      queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
#endif
                   }
                } else {
                   queue_string(d, unsafe_tprintf("%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
                }
                free_lbuf(progatr);
                process_output(d);
                break;
             }
          }
       } else {
          notify(player, "Your @program was aborted from disconnecting.");
          s_Flags4(player, (Flags4(player) & (~INPROGRAM)));
          DESC_ITER_CONN(d) {
             if ( D_PLAYER(d) == player ) {
                queue_string(d, "\377\371");
             }
             process_output(d);
          }
          mudstate.shell_program = 0;
          atr_clr(player, A_PROGBUFFER);
          atr_clr(player, A_PROGPROMPTBUF);
       }
    }
    mudstate_hot.curr_enactor = temp;
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
    cmdcnt += d->cold->command_count;
    if ( cmdcnt > 2000000000 )
       cmdcnt = 2000000000;
    atr_add_raw(player, A_TOTCMDS, unsafe_tprintf("%d",cmdcnt));
    atr_add_raw(player, A_LSTCMDS, unsafe_tprintf("%d", d->cold->command_count));
    atr_add_raw(player, A_TOTCHARIN, unsafe_tprintf("%d", D_INPUT_TOT(d)));
    atr_add_raw(player, A_TOTCHAROUT, unsafe_tprintf("%d", D_OUTPUT_TOT(d)));


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
    } else if (D_HOST_INFO(d) & H_SUSPECT) {
        if ( num < 2 )
	   broadcast_monitor(player, MF_TRIM, "DISCONN [SUSPECT SITE]", 
                             d->cold->userid, d->cold->longaddr, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, MF_TRIM, "PARTIAL DISCONN [SUSPECT SITE]", 
                             d->cold->userid, d->cold->longaddr, 0, 0, 0, (char *)reason);
    } else if (D_HOST_INFO(d) & H_PASSPROXY) {
        if ( num < 2 )
	   broadcast_monitor(player, MF_TRIM, "DISCONN [PROXY BYPASS]", 
                             d->cold->userid, d->cold->longaddr, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, MF_TRIM, "PARTIAL DISCONN [PROXY BYPASS]", 
                             d->cold->userid, d->cold->longaddr, 0, 0, 0, (char *)reason);
    } else {
        if ( num < 2 )
	   broadcast_monitor(player, 0, "DISCONN", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
        else
	   broadcast_monitor(player, 0, "PARTIAL DISCONN", 
                             NULL, NULL, 0, 0, 0, (char *)reason);
    }
    temp = mudstate_hot.curr_enactor;
    mudstate_hot.curr_enactor = player;

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

        sprintf(s_timeon, "%ld", (mudstate_hot.now - d->cold->connected_at));
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
	if (D_FLAGS(d) & DS_AUTODARK) {
	    s_Flags(D_PLAYER(d), Flags(D_PLAYER(d)) & ~DARK);
	    D_FLAGS(d) &= ~DS_AUTODARK;
	}
	if (D_FLAGS(d) & DS_AUTOUNF) {
	    s_Flags2(D_PLAYER(d), Flags2(D_PLAYER(d)) & ~UNFINDABLE);
	    D_FLAGS(d) &= ~DS_AUTOUNF;
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
           sprintf(s_timeon, "%ld", (mudstate_hot.now - d->cold->connected_at));
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
    mudstate_hot.curr_enactor = temp;
    local_player_disconnect(player);
    VOIDRETURN; /* #131 */
}

int 
boot_off(dbref player, char *message)
{
    DESC *d, *dnext;
    (void)dnext;
    int count;

    DPUSH; /* #132 */
    count = 0;
    DESC_SAFEITER_PLAYER(player, d, dnext) {
	if (message && *message) {
	    queue_string(d, message);
	    queue_string(d, "\r\n");
            process_output(d);
	}
	shutdownsock(d, R_BOOT);
	count++;
    }
    RETURN(count); /* #132 */
}

int 
boot_by_port(int port, int no_god, int who, char *message)
{
    DESC *d;
    int count;
    char *buf;

    DPUSH; /* #133 */
    count = 0;
    DESC_SAFEITER_ALL(d) {
	if (!d->cold) continue;
	if ((D_DESCRIPTOR(d) == port) && (!no_god || !God(D_PLAYER(d)))) {
	    if ((D_PLAYER(d) == who) || !DePriv(who, D_PLAYER(d), DP_BOOT, POWER6, NOTHING)) {
	      if (Controls(who,D_PLAYER(d))) {
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
                    process_output(d);
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
	    d->cold->timeout = atoi(buf);
	    if ( (d->cold->timeout <= 0) && (d->cold->timeout != -1) )
		d->cold->timeout = mudconf.idle_timeout;
	}
	free_lbuf(buf);
    }
    VOIDRETURN; /* #134 */
}

/* ---------------------------------------------------------------------------
 * fetch_idle, fetch_connect: Return smallest idle time/largest connect time
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
	idletime = (mudstate_hot.now - D_LAST_TIME(d));
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
	conntime = (mudstate_hot.now - d->cold->connected_at);
	if (conntime > result)
	    result = conntime;
    }
    RETURN(result); /* #136 */
}

void 
NDECL(check_idle)
{
    DESC *d;
    time_t idletime;

    DPUSH; /* #137 */

    DESC_SAFEITER_ALL(d) {
	if (D_FLAGS(d) & DS_CONNECTED) {


	#ifdef ENABLE_WEBSOCKETS
	    /* Websockets don't like these random bytes. They want Unicode. */
	    if (!(D_FLAGS(d) & DS_WEBSOCKETS)) {
	#endif
            if ( KeepAlive(D_PLAYER(d)) ) {
               /* Send NOP code to players to keep NAT/routers/firewalls happy */
               queue_string(d, "\377\361");
	       process_output(d);
            }
	#ifdef ENABLE_WEBSOCKETS
	    }
	#endif
            if ( D_LAST_TIME(d) > mudstate_hot.now )
	       idletime = 0;
            else
	       idletime = mudstate_hot.now - D_LAST_TIME(d);
	    if (( (idletime > d->cold->timeout) && (d->cold->timeout != -1) ) && !Wizard(D_PLAYER(d))) {
		queue_string(d, "*** Inactivity Timeout ***\r\n");
                process_output(d);
		shutdownsock(d, R_TIMEOUT);
		/* freed-slot safety: shutdownsock compacts the slot, skip to next */
		continue;
            } else if (mudconf.idle_wiz_cloak &&
		       ( (idletime > d->cold->timeout) && (d->cold->timeout != -1) ) &&
		       Wizard(D_PLAYER(d)) && (!Dark(D_PLAYER(d)) || !Unfindable(D_PLAYER(d)))) {
                if ( !Dark(D_PLAYER(d)) ) {
		   s_Flags(D_PLAYER(d), Flags(D_PLAYER(d)) | DARK);
		   D_FLAGS(d) |= DS_AUTODARK;
                }
                if ( !Unfindable(D_PLAYER(d)) ) {
		   s_Flags2(D_PLAYER(d), Flags2(D_PLAYER(d)) | UNFINDABLE);
		   D_FLAGS(d) |= DS_AUTOUNF;
                }
                if (mudconf.idle_message)
                   notify(D_PLAYER(d), "You have exceeded the timeout and been CLOAKED.");
	    } else if (mudconf.idle_wiz_dark &&
		       ( (idletime > d->cold->timeout) && (d->cold->timeout != -1) ) &&
		       Wizard(D_PLAYER(d)) && !Dark(D_PLAYER(d))) {
		s_Flags(D_PLAYER(d), Flags(D_PLAYER(d)) | DARK);
		D_FLAGS(d) |= DS_AUTODARK;
                if (mudconf.idle_message)
                   notify(D_PLAYER(d), "You have exceeded the timeout and set DARK.");
	    }
	} else {
            if ( (d->cold->connected_at > mudstate_hot.now) ||
                 ((d->cold->connected_at + mudconf.conn_timeout + 60) < mudstate_hot.now) ) {
               idletime = 0;
            } else {
	       idletime = mudstate_hot.now - d->cold->connected_at;
            }
            if ( d->cold->account_owner != NOTHING ) {
               if ( idletime < mudconf.idle_timeout ) {
                  idletime = 0;
               }
            }
	    if (idletime > mudconf.conn_timeout) {
		queue_string(d, "*** Login Timeout ***\r\n");
                process_output(d);
		shutdownsock(d, R_TIMEOUT);
		/* freed-slot safety: shutdownsock compacts the slot, skip to next */
		continue;
	    } else if ( D_FLAGS(d) & DS_API ) {
               /* Force API disconnecting after d->cold->timeout from connection */
               if ( (idletime > (d->cold->timeout)) || (mudstate_hot.now > (d->cold->connected_at + d->cold->timeout)) ) {
                  process_output(d);
		  shutdownsock(d, R_API);
		  /* freed-slot safety: shutdownsock compacts the slot, skip to next */
		  continue;
               }
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
    char *buf, *fp, *gp, *doingAnsiBuf, *doingAccentBuf, *doingUtfBuf, *pDoing, *atext, *atext2, *atextptr, *a_tee; 
#ifdef ZENTY_ANSI
    char *doingAnsiBufp, *abuf, *abufp, *msg_ns2, *mp2, *doingAccentBufp, *doingUtfBufp, *msg_utf, *mp_utf;
#endif
    char smallbuf[25];
    char antemp[20];
    char buf_format[80];
    char one_chr_holder = ' ';
    char *tpr_buff, *tprp_buff;
    ATTR *atr;

    DPUSH; /* #139 */
    memset(buf_format, 0, sizeof(buf_format));
    if ( Good_chk(D_PLAYER(e)) && Fubar(D_PLAYER(e))) {
	notify(D_PLAYER(e), "You can't do that...");
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
	doingUtfBuf = alloc_lbuf("dump_users_utf");
    tprp_buff = tpr_buff = alloc_lbuf("dump_users_tprintf");

    i_pipetype = 1;
    if ( Good_chk(D_PLAYER(e)) && H_Attrpipe(D_PLAYER(e)) ) {
       atr = atr_str3("___ATTRPIPE");
       if ( atr ) {
          atext2 = atr_get(D_PLAYER(e), atr->number, &aowner, &aflags);
          if ( *atext2 ) {
             if ( (a_tee = strchr(atext2, ' ')) != NULL ) {
                *a_tee = '\0';
                i_pipetype = atoi(a_tee+1);
             }
             atr = atr_str3(atext2);
             if ( atr ) {
                if ( Controlsforattr(D_PLAYER(e), D_PLAYER(e), atr, aflags) ) {
                   i_attrpipe = 1;
                }
             }
          }
          free_lbuf(atext2);
          if ( i_attrpipe ) {
             atext2 = atr_get(D_PLAYER(e), atr->number, &aowner, &aflags);
             atextptr = atext = alloc_lbuf("dump_users_pipe");
             safe_str(atext2, atext, &atextptr);
             free_lbuf(atext2);
          }
       }
       if ( !i_attrpipe ) {
          queue_string(e, (char *)"Notice: Piping attribute no longer exists.  Piping has been auto-disabled.\r\n");
          s_Flags4(D_PLAYER(e), Flags4(D_PLAYER(e)) & ~HAS_ATTRPIPE);
          atr = atr_str3("___ATTRPIPE");
          if ( atr ) {
             atr_clr(D_PLAYER(e), atr->number);
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
    } else {
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
    }
    if (key == CMD_SESSION) {
        if ( i_attrpipe ) {
           safe_str((char *)"Pend  Lost     Total  Pend  Lost     Total\r\n", atext, &atextptr);
        } 
        if ( i_pipetype ) {
	   queue_string(e, "Pend  Lost     Total  Pend  Lost     Total\r\n");
        }
    } else if ((D_FLAGS(e) & DS_CONNECTED) && (Wizard(D_PLAYER(e)) || HasPriv(D_PLAYER(e), NOTHING, POWER_WIZ_WHO, POWER3, POWER_LEVEL_NA))
               && (!DePriv(D_PLAYER(e), NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF))
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
		   mp_utf = msg_utf = alloc_lbuf("notify_check_utf");
           parse_ansi(mudstate.ng_doing_hdr, abuf, &abufp, msg_ns2, &mp2, msg_utf, &mp_utf);
           *mp2 = '\0';
		   *mp_utf = '\0';
		   if ( UTF8(D_PLAYER(e)) ) {
                if (i_attrpipe) {
                   safe_str(msg_utf, atext, &atextptr);
                }
                if (i_pipetype) {
                    queue_string(e, msg_utf);   
                }			  
           } else if ( Accents(D_PLAYER(e)) ) {
	          if (i_attrpipe) {
                   safe_str(msg_ns2, atext, &atextptr);
                }
                if (i_pipetype) {
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
		   free_lbuf(msg_utf);
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
		   mp_utf = msg_utf = alloc_lbuf("notify_check_utf");
           parse_ansi(mudstate.doing_hdr, abuf, &abufp, msg_ns2, &mp2, msg_utf, &mp_utf);
           *mp2 = '\0';
		   *mp_utf = '\0';
		   if ( UTF8(D_PLAYER(e)) ) {
			  if ( i_attrpipe ) {
                 safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", mudstate.guild_hdr, msg_utf),
                          atext, &atextptr);
              } 
              if ( i_pipetype ) {
	         queue_string(e, safe_tprintf(tpr_buff, &tprp_buff, "%-11s %s", 
                              mudstate.guild_hdr, msg_utf));
              }
           } else if ( Accents(D_PLAYER(e)) ) {
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
		   free_lbuf(msg_utf);
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
    /* Collect phase: gather matching player slots */
    {
        int slots[MAX_DESCRIPTORS], nslots = 0;
        DESC_SAFEITER_CONN(d) {
            int slot;

	    rcount++;

	    if (Cloak(D_PLAYER(d)) && rcount)
	        rcount--;
            else if ((Dark(D_PLAYER(d)) && !mudconf.who_unfindable && !mudconf.player_dark && rcount && 
                      mudconf.allow_whodark))
	        rcount--;
            else if (NoWho(D_PLAYER(d)) && rcount && !(D_PLAYER(d) == D_PLAYER(e) || Wizard(D_PLAYER(e))) && rcount)
                rcount--;
            else if (NoWho(D_PLAYER(d)) && !(D_FLAGS(e) & DS_CONNECTED) && rcount)
                rcount--;

            if ( ( !(Unfindable(D_PLAYER(d)) && mudconf.who_unfindable) || 
                   (Wizard(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED)) || 
                   (Unfindable(D_PLAYER(d)) && HasPriv(D_PLAYER(e),D_PLAYER(d),POWER_WHO_UNFIND,POWER4,NOTHING)) ||
	            HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING) ||
                   (!mudconf.who_unfindable && mudconf.player_dark) ||
	           (Admin(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED) && !Wizard(D_PLAYER(d))) 
                 ) &&
                 !( Dark(D_PLAYER(d)) && 
                    !mudconf.who_unfindable && 
                    !mudconf.player_dark && 
                    !(Admin(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED)) && 
                    mudconf.allow_whodark
                  ) ) {
	        if (!Unfindable(D_PLAYER(d)))
	            count++;
	        if (match && !(string_prefix(Name(D_PLAYER(d)), match)))
	            continue;
	        if ((key == CMD_SESSION) && !Wizard(D_PLAYER(e)) &&
		    !Admin(D_PLAYER(e)) && !Builder(D_PLAYER(e)) &&
		    (D_PLAYER(d) != D_PLAYER(e)))
	            continue;
	        if ((key == CMD_SESSION) && !Wizard(D_PLAYER(e)) &&
		    Wizard(D_PLAYER(d)))
	            continue;
	        if ((key == CMD_SESSION) && !Admin(D_PLAYER(e)) &&
		    Admin(D_PLAYER(d)))
	            continue;
	        if (SCloak(D_PLAYER(d)) && Dark(D_PLAYER(d)) && Unfindable(D_PLAYER(d)) && !Immortal(D_PLAYER(e)))
	            continue;
	        if (Cloak(D_PLAYER(d)) && !Wizard(D_PLAYER(e)))
	            continue;
	        if ((Cloak(D_PLAYER(d)) || NoWho(D_PLAYER(d))) && !(D_FLAGS(e) & DS_CONNECTED))
	            continue;
                if (NoWho(D_PLAYER(d)) && (D_PLAYER(d) != D_PLAYER(e)) &&
                    !(Wizard(D_PLAYER(e)) || HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING)))
                    continue;
                if (NoWho(D_PLAYER(d)) && !(D_FLAGS(e) & DS_CONNECTED))
                    continue;
                if (Dark(D_PLAYER(d)) && !mudconf.who_unfindable && !mudconf.player_dark && 
                    !(D_FLAGS(e) & DS_CONNECTED) && mudconf.allow_whodark)
                    continue;
                slot = d->slot_index;
                if (nslots < MAX_DESCRIPTORS) {
                    int _p;
                    for (_p = 0; _p < nslots; _p++)
                        if (slots[_p] == slot) break;
                    if (_p >= nslots)
                        slots[nslots++] = slot;
                }
	    }
	}
        /* Sort by connected_at descending (newest first) */
        for (int _i = 0; _i < nslots - 1; _i++) {
            for (int _j = _i + 1; _j < nslots; _j++) {
                time_t ti = desc_cold[slots[_i]].connected_at;
                time_t tj = desc_cold[slots[_j]].connected_at;
                if (ti < tj) {
                    int _t = slots[_i]; slots[_i] = slots[_j]; slots[_j] = _t;
                }
            }
        }
        /* Output phase: iterate sorted entries */
        for (int _wi = 0; _wi < nslots; _wi++) {
            d = &desc_slots[slots[_wi]];
#ifdef ZENTY_ANSI
            *doingAccentBuf='\0';
            *doingAnsiBuf='\0';
            *doingUtfBuf='\0';
	    doingAnsiBufp = doingAnsiBuf;
	    doingAccentBufp = doingAccentBuf;
	    doingUtfBufp = doingUtfBuf;
	    parse_ansi(d->cold->doing, doingAnsiBuf, &doingAnsiBufp, doingAccentBuf, &doingAccentBufp, doingUtfBuf, &doingUtfBufp);
            if ( !Accents(D_PLAYER(e)) ) {
               strcpy(doingAccentBuf, strip_safe_accents(doingAnsiBuf));
            }
	    pDoing = doingAccentBuf;
#else
	    pDoing = d->cold->doing;
#endif
	    if ((D_FLAGS(e) & DS_CONNECTED) &&
		(Wizard(D_PLAYER(e)) || HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING)) &&
		(!DePriv(D_PLAYER(e), D_PLAYER(d), DP_WIZ_WHO, POWER6, NOTHING) || (D_PLAYER(e) == D_PLAYER(d))) &&
		(!DePriv(D_PLAYER(e), NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF)) &&
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
			Name(D_PLAYER(d)),
			time_format_1(now - d->cold->connected_at),
			time_format_2(now - D_LAST_TIME(d)),
#ifdef PARIS
                        Location(D_PLAYER(d)), d->cold->command_count,
#else
			Location(D_PLAYER(d)), D_DESCRIPTOR(d),
#endif
			d->cold->longaddr);

#ifdef PARIS
		fp = &buf[16];
#else
		fp = &buf[14];
#endif

                if ( !(HasPriv(D_PLAYER(d), D_PLAYER(e), POWER_HIDEBIT, POWER5, NOTHING) && 
                       (D_PLAYER(d) != D_PLAYER(e) || mudstate_hot.objevalst) && (D_FLAGS(e) & DS_CONNECTED))) {
                   if (mudconf.who_showwiz && Guildmaster(D_PLAYER(d)) ) {
                      if ( Immortal(D_PLAYER(d)) ||
                           ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 ) ||
                           ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 ) ||
                           ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 ) ||
                           ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 ) )
                          *fp++ = '*';
                   }
                   else if (mudconf.who_showwiztype ) {
                      if ( Immortal(D_PLAYER(d)) )
                         *fp++ = 'W';
                      else if ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 )
                         *fp++ = 'W';
                      else if ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 )
                         *fp++ = 'a';
                      else if ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 )
                         *fp++ = 'B';
                      else if ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 )
                         *fp++ = 'g';
                   }
                }
                if ( (D_FLAGS(e) & DS_CONNECTED) && Wizard(D_PLAYER(e)) && (D_FLAGS(d) & DS_SSL) ) {
                   *fp++ = '$';
                }
		if (Dark(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED)) {
		    if (D_FLAGS(d) & DS_AUTODARK)
			*fp++ = 'd';
		    else
			*fp++ = 'D';
		}
		if (Unfindable(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED)) {
		  if (D_FLAGS(d) & DS_AUTOUNF) {
		    *fp++ = 'h';
		  } else {
		    *fp++ = 'H';
		  }
		}
                if ( NoWho(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED) )
                    *fp++ = 'U';
		if (Suspect(D_PLAYER(d)) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
                    (D_FLAGS(e) & DS_CONNECTED))
		    *fp++ = '+';
		if ((D_HOST_INFO(d) & H_FORBIDDEN) && Wizard(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED))
		    *fp++ = 'F';
		if ((D_HOST_INFO(d) & H_PERMIT) && Wizard(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED))
		    *fp++ = 'P';
		if (D_HOST_INFO(d) & H_REGISTRATION && (D_FLAGS(e) & DS_CONNECTED) && 
                    !mudconf.noregist_onwho)
		    *fp++ = 'R';
		if ((D_HOST_INFO(d) & H_SUSPECT) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
		    (D_FLAGS(e) & DS_CONNECTED))
		    *fp++ = '+';
		if ((D_HOST_INFO(d) & H_PASSPROXY) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
		    (D_FLAGS(e) & DS_CONNECTED))
		    *fp++ = '^';
	    } else if (key == CMD_SESSION) {
		sprintf(buf,
			"%-16.16s %10d %4d  %4d%6d%10d%6d%6d%10d\r\n",
			Name(D_PLAYER(d)),
			d->cold->command_count,
			D_DESCRIPTOR(d),
			D_INPUT_SIZE(d), d->cold->input_lost,
			D_INPUT_TOT(d),
			D_OUTPUT_SIZE(d), d->cold->output_lost,
			D_OUTPUT_TOT(d));
	    } else {
		if ((key == CMD_WHO) && ((HasPriv(D_PLAYER(e), NOTHING, POWER_WIZ_WHO, POWER3, POWER_LEVEL_NA)) ||
					 (Wizard(D_PLAYER(e)) && DePriv(D_PLAYER(e), NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_NA))) &&
		    !DePriv(D_PLAYER(e), NOTHING, DP_WIZ_WHO, POWER6, POWER_LEVEL_OFF))
		    continue;
		if (mudconf.who_default) {
#ifdef PARIS
                  sprintf(buf, "%-16.16s   %10s %4s  %-s\r\n",
#else
		  sprintf(buf, "%-14.14s   %10s %4s  %-s\r\n",
#endif
			Name(D_PLAYER(d)),
			time_format_1(now - d->cold->connected_at),
			time_format_2(now - D_LAST_TIME(d)),
			pDoing);
		}
		else {
		  gp = Guild(D_PLAYER(d));
		  if (!*gp)
		    gp = mudconf.guild_default;
#ifdef PARIS
                  sprintf(buf, "%-16.16s   %10s %4s  %-11.11s %-s\r\n",
#else
		  sprintf(buf, "%-14.14s   %10s %4s  %-11.11s %-s\r\n",
#endif
			Name(D_PLAYER(d)),
			time_format_1(now - d->cold->connected_at),
			time_format_2(now - D_LAST_TIME(d)),
			gp, pDoing);
		}
		if (Admin(D_PLAYER(e)) || HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING)) {
#ifdef PARIS
                    fp = &buf[16];
#else
                    fp = &buf[14];
#endif
                    if ( !(HasPriv(D_PLAYER(d), D_PLAYER(e), POWER_HIDEBIT, POWER5, NOTHING) &&
                           (D_PLAYER(d) != D_PLAYER(e) || mudstate_hot.objevalst)) && (D_FLAGS(e) & DS_CONNECTED)) {
                        if (mudconf.who_showwiz && Guildmaster(D_PLAYER(d)) ) {
                           if ( Immortal(D_PLAYER(d)) ||
                                ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 ) ||
                                ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 ) ||
                                ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 ) ||
                                ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 ) )
                               *fp++ = '*';
                        }
                        else if (mudconf.who_showwiztype ) {
                           if ( Immortal(D_PLAYER(d)) )
                              *fp++ = 'W';
                           else if ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 )
                              *fp++ = 'W';
                           else if ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 )
                              *fp++ = 'a';
                           else if ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 )
                              *fp++ = 'B';
                           else if ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 )
                              *fp++ = 'g';
                        }
                    }
                    if ( (D_FLAGS(e) & DS_CONNECTED) && Wizard(D_PLAYER(e)) && (D_FLAGS(d) & DS_SSL) ) {
                       *fp++ = '$';
                    }
		    if (Dark(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED)) {
			if (D_FLAGS(d) & DS_AUTODARK)
			    *fp++ = 'd';
			else
			    *fp++ = 'D';
		    }
		    if (Unfindable(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED)) {
		        if (D_FLAGS(d) & DS_AUTOUNF)
			    *fp++ = 'h';
		        else
			    *fp++ = 'H';
		    }
                    if ( NoWho(D_PLAYER(d)) && (D_FLAGS(e) & DS_CONNECTED) )
                        *fp++ = 'U';
		    if (Suspect(D_PLAYER(d)) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
                       (D_FLAGS(e) & DS_CONNECTED))
			*fp++ = '+';
		    if ((D_HOST_INFO(d) & H_FORBIDDEN) && Wizard(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED))
			*fp++ = 'F';
		    if ((D_HOST_INFO(d) & H_PERMIT) && Wizard(D_PLAYER(e)) && (D_FLAGS(e) & DS_CONNECTED))
			*fp++ = 'P';
		    if (D_HOST_INFO(d) & H_REGISTRATION && (D_FLAGS(e) & DS_CONNECTED) &&
                        !mudconf.noregist_onwho)
			*fp++ = 'R';
		    if ((D_HOST_INFO(d) & H_SUSPECT) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
                        (D_FLAGS(e) & DS_CONNECTED))
			*fp++ = '+';
		    if ((D_HOST_INFO(d) & H_PASSPROXY) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
                        (D_FLAGS(e) & DS_CONNECTED))
			*fp++ = '^';
		}
		else if (HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WHO_UNFIND, POWER4, NOTHING) && 
				(D_FLAGS(e) & DS_CONNECTED))
		   one_chr_holder = 'H';
#ifdef PARIS
                fp = &buf[16];
#else
                fp = &buf[14];
#endif
                if ( !(HasPriv(D_PLAYER(d), D_PLAYER(e), POWER_HIDEBIT, POWER5, NOTHING) &&
                       (D_PLAYER(d) != D_PLAYER(e) || mudstate_hot.objevalst)) && (D_FLAGS(e) & DS_CONNECTED)) {
                   if (mudconf.who_showwiz && Guildmaster(D_PLAYER(d)) ) {
                      if ( Immortal(D_PLAYER(d)) ||
                           ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 ) ||
                           ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 ) ||
                           ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 ) ||
                           ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 ) )
                          *fp++ = '*';
                   }
                   else if (mudconf.who_showwiztype ) {
                      if ( Immortal(D_PLAYER(d)) )
                         *fp++ = 'W';
                      else if ( Wizard(D_PLAYER(d)) && mudconf.who_wizlevel < 5 )
                         *fp++ = 'W';
                      else if ( Admin(D_PLAYER(d)) && mudconf.who_wizlevel < 4 )
                         *fp++ = 'a';
                      else if ( Builder(D_PLAYER(d)) && mudconf.who_wizlevel < 3 )
                         *fp++ = 'B';
                      else if ( Guildmaster(D_PLAYER(d)) && mudconf.who_wizlevel < 2 )
                         *fp++ = 'g';
                   }
                }
		if (!(Admin(D_PLAYER(e)) || HasPriv(D_PLAYER(e), D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING))) {
		   if (Suspect(D_PLAYER(d)) && (Wizard(D_PLAYER(e)) || SeeSuspect(D_PLAYER(e))) && 
                      (D_FLAGS(e) & DS_CONNECTED))
	              *fp++ = '+';
		}
                if ( one_chr_holder == 'H' && (D_FLAGS(e) & DS_CONNECTED))
                   *fp++ = one_chr_holder;
                one_chr_holder=' ';
	    }
            if ( i_attrpipe ) {
               safe_str(buf, atext, &atextptr);
            } 
            if ( i_pipetype ) {
	       queue_string(e, buf);
            }
	    if (ShowAnsi(D_PLAYER(e)) && index(buf, ESC_CHAR)) {
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
    if ( NoWho(D_PLAYER(e)) ) {
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
	free_lbuf(doingUtfBuf);
    free_lbuf(tpr_buff);
    if ( i_attrpipe ) {
       atr_add_raw(D_PLAYER(e), atr->number, atext);
       if ( !Quiet(D_PLAYER(e)) && TogNoisy(D_PLAYER(e)) && !i_pipetype )
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
        if (Dark(D_PLAYER(d)) && !mudconf.who_unfindable && !mudconf.player_dark &&
                mudconf.allow_whodark)
                continue;
	if (!(Unfindable(D_PLAYER(d)) && mudconf.who_unfindable)) {
	    notify(player,
		   unsafe_tprintf("@inwho %s@%s=%-16.16s %10s %4s %s",
			   name, mud,
			   Name(D_PLAYER(d)),
			   time_format_1(now - d->cold->connected_at),
			   time_format_2(now - D_LAST_TIME(d)),
			   *(d->cold->doing) ? d->cold->doing : ""));
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
  char *s_uptime;
#ifndef UTMP_MISSING
#ifdef UTMP_ONLY
  struct utmp *ut;
#else
  struct utmpx *ut;
#endif
#endif

  DPUSH; /* #141 */
  
  time(&now);

  buff = alloc_mbuf("uptime");
  s_uptime = alloc_mbuf("uptime_sys");

#ifndef UTMP_MISSING
#ifdef UTMP_ONLY
  setutent();
  ut = getutent();
  while ( ut && (ut->ut_type != BOOT_TIME) )
     ut = getutent();

  memset(s_uptime, '\0', MBUF_SIZE);
  if ( ut ) {
     strcpy(s_uptime, time_format_1(mudstate_hot.now - ut->ut_tv.tv_sec));
  }
  endutent();
#else
  setutxent();
  ut = getutxent();
  while ( ut && (ut->ut_type != BOOT_TIME) )
     ut = getutxent();

  memset(s_uptime, '\0', MBUF_SIZE);
  if ( ut ) {
     strcpy(s_uptime, time_format_1(mudstate_hot.now - ut->ut_tv.tv_sec));
  }
  endutxent();
#endif
#endif

  strcpy(buff,time_format_1(now - mudstate.reboot_time));

  notify(player, unsafe_tprintf("%s has been up for %s, Reboot: %s",
			   mudconf.mud_name,
			   time_format_1(now - mudstate.start_time), buff));
  if ( *s_uptime )
       notify(player, unsafe_tprintf("System Uptime: %s", s_uptime));

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
	  DESC *d_tmp;
	  DESC_SAFEITER_PLAYER(player, d, d_tmp) {
	  (void)d_tmp;
	    c = d->cold->doing;
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
	    over = safe_copy_str(pt1, d->cold->doing, &c, copylen);
	    *c = '\0';
	    foundany = 1;
	  }
	}
	else {
	  time(&now);
	  gotone = 0;
	  dtime = 0;
	  DESC_ITER_PLAYER(player, d) {
	    if (!gotone || (now - D_LAST_TIME(d)) < dtime) {
	      e = d;
	      dtime = now - D_LAST_TIME(d);
	      gotone= 1;
	    }
	  }
	  if (gotone > 0) {
	    c = e->cold->doing;
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
	    over = safe_copy_str(pt1, e->cold->doing, &c, copylen);
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
    {(char *) "POST", 4, CA_PUBLIC, 0, CMD_POST},
    {(char *) "HEAD", 4, CA_PUBLIC, 0, CMD_HEAD},
    {(char *) "PUT", 3, CA_PUBLIC, 0, CMD_PUT},
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
	buff = alloc_lbuf("failconn.LOG");
    sprintf(buff, "[%d/%s] %s rejected to ",
	    D_DESCRIPTOR(d), d->cold->longaddr, logtype);
    log_text(buff);
    free_lbuf(buff);
    if (player != NOTHING)
	log_name(player);
    else
	log_text(user);
    log_text((char *) " (");
    log_text((char *) logreason);
    log_text((char *) ")");
    ENDLOG
	fcache_dump(d, filecache, (char *)NULL);
    if (motd_msg && *motd_msg) {
	queue_string(d, motd_msg);
	queue_write(d, "\r\n", 2);
        process_output(d);
    }
    free_mbuf(command);
    free_mbuf(user);
    free_mbuf(password);
    shutdownsock(d, disconnect_reason);
    mudstate_hot.debug_cmd = cmdsave;
    VOIDRETURN; /* #145 */
}

static const char *connect_fail =
"Either that player does not exist, or has a different password.\r\n";
static const char *create_fail =
"Either there is already a player with that name, or that name is illegal.\r\n";

int
softcode_trigger(DESC *d, const char *msg) {
    ATTR *atr;
    char *s_text, *s_return, *s_strtok, *s_strtokr, *s_buff, *s_array[10], *s_ptr;
#ifdef ZENTY_ANSI
    char *lbuf1ptr, *lbuf1, *lbuf2ptr, *lbuf2, *lbuf3ptr, *lbuf3;
#endif
    int aflags, i_found;
    dbref aowner;

    if ( !Good_chk(mudconf.file_object) ) {
       return(0);
    }

    atr = atr_str3("FAKECOMMANDS");
    if ( !atr ) {
       return(0);
    }

    s_text = atr_get(mudconf.file_object, atr->number, &aowner, &aflags);
    if ( !s_text || !*s_text ) {
       free_lbuf(s_text);
       return(0);
    }

    for ( i_found = 0; i_found < 10; i_found++ ) 
       s_array[i_found] = NULL;

    i_found = 0;

    /* Stack the arrays */
    s_array[0] = alloc_lbuf("s_array_0");
    s_array[1] = alloc_lbuf("s_array_1");
    s_array[2] = alloc_lbuf("s_array_2");
    s_array[3] = alloc_lbuf("s_array_3");
    s_array[4] = alloc_lbuf("s_array_4");

    /* Store last command as %0 */
    s_ptr = s_array[0];
    safe_str((char *)msg, s_array[0], &s_ptr);
    if ( strchr(s_array[0], ' ') != NULL )
       *(strchr(s_array[0], ' ')) = '\0';

    /* Arguments to command as %1 */
    s_ptr = s_array[1];
    if ( strchr((char *)msg, ' ') != NULL )
       safe_str(strchr((char *)msg, ' ')+1, s_array[1], &s_ptr);

    /* DNS name as %2 */
    s_ptr = s_array[2];
    safe_str(d->cold->addr, s_array[2], &s_ptr);

    /* IP number as %3 */
    s_ptr = s_array[3];
    safe_str(d->cold->longaddr, s_array[3], &s_ptr);

    /* Port  as %4*/
    s_ptr = s_array[4];
    ival(s_array[4], &s_ptr, D_DESCRIPTOR(d));

    s_return = cpuexec(mudconf.file_object, mudconf.file_object, mudconf.file_object, 
                       EV_FCHECK | EV_EVAL, s_text, s_array, 5, (char **)NULL, 0);
    if ( mudstate_hot.chkcpu_toggle ) {
       broadcast_monitor(mudconf.file_object, MF_CPU, "CPU RUNAWAY LOGIN CUSTOMCMD",
                         (char *)atr->name, NULL, mudconf.file_object, 0, 0, NULL);
       STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
          log_name_and_loc(mudconf.file_object);
          sprintf(s_text, " CPU login customcmd overload on attribute %s", (char *)atr->name);
          log_text(s_text);
       ENDLOG
       mudstate_hot.chkcpu_toggle = 0;
    }
    free_lbuf(s_text);
    if ( !s_return || !*s_return ) {
        free_lbuf(s_return);
        free_lbuf(s_array[0]);
        free_lbuf(s_array[1]);
        free_lbuf(s_array[2]);
        free_lbuf(s_array[3]);
        free_lbuf(s_array[4]);
        return(0);
    }
 
    s_strtok = strtok_r(s_return, "|", &s_strtokr);
    i_found = 0;
    while ( s_strtok ) {
       if ( stricmp(s_strtok, s_array[0]) == 0 ) {
          i_found = 1;
          s_buff = alloc_lbuf("softcode_trigger");
          sprintf(s_buff, "RUN_%s", s_strtok);
          atr = atr_str3(s_buff);
          free_lbuf(s_buff);
          if ( !atr ) {
             i_found = 0;
             break;
          }

          s_text = atr_get(mudconf.file_object, atr->number, &aowner, &aflags);
          if ( !s_text || !*s_text ) {
             i_found = 0;
             free_lbuf(s_text);
             break;
          }
          
          s_buff = cpuexec(mudconf.file_object, mudconf.file_object, mudconf.file_object, 
                        EV_FCHECK | EV_EVAL, s_text, s_array, 5, (char **)NULL, 0);
          if ( mudstate_hot.chkcpu_toggle ) {
             broadcast_monitor(mudconf.file_object, MF_CPU, "CPU RUNAWAY LOGIN CUSTOMCMD",
                               unsafe_tprintf("run_%s", s_strtok), NULL, mudconf.file_object, 0, 0, NULL);
             STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                log_name_and_loc(mudconf.file_object);
                sprintf(s_text, " CPU login customcmd overload on attribute run_%s", s_strtok);
                log_text(s_text);
             ENDLOG
             mudstate_hot.chkcpu_toggle = 0;
          }
          free_lbuf(s_text);
          if ( !s_buff || !*s_buff ) {
             i_found = 0;
             free_lbuf(s_buff);
             break;
          }

#ifdef ZENTY_ANSI
          lbuf1 = alloc_lbuf("fcache_dump3");
          lbuf2 = alloc_lbuf("fcache_dump4");
          lbuf3 = alloc_lbuf("fcache_dump5");
          lbuf1ptr = lbuf1;
          lbuf2ptr = lbuf2;
          lbuf3ptr = lbuf3;
          parse_ansi(s_buff, lbuf1, &lbuf1ptr, lbuf2, &lbuf2ptr, lbuf3, &lbuf3ptr);
          queue_write(d, lbuf1, strlen(lbuf1));
          free_lbuf(lbuf1);
          free_lbuf(lbuf2);
          free_lbuf(lbuf3);
#else
          queue_string(d, s_buff);
#endif
	  queue_write(d, "\r\n", 2);
          free_lbuf(s_buff);
          process_output(d);
          break;
       }
       s_strtok = strtok_r(NULL, "|", &s_strtokr);
    }
    free_lbuf(s_array[0]);
    free_lbuf(s_array[1]);
    free_lbuf(s_array[2]);
    free_lbuf(s_array[3]);
    free_lbuf(s_array[4]);
    free_lbuf(s_return);
    return(i_found);
}

/* Connect types
 * co - connect normal
 * cd - connect dark
 * ch - connect hidden
 * zz - use functional account via account_login()
 * reg - registration connect
 */

/* This is perm restriction only for connection command only */
int chkbit(dbref player)
{
   int i_ret;

   i_ret = -1;

   if ( !Good_chk(player) ) {
      return i_ret;
   }

   if ( Good_chk(player) ) {
      if ( mudstate.account_subsys_inuse ) {
         i_ret = 777; /* Always work */
      } else if ( God(player) ) {
         i_ret = 7;
      } else if (Immortal(player) ) {
         i_ret = 6;
      } else if (Wizard(player) ) {
         i_ret = 5;
      } else if (Admin(player) ) {
         i_ret = 4;
      } else if (Builder(player) ) {
         i_ret = 3;
      } else if (Guildmaster(player) ) {
         i_ret = 2;
      } else if (Wanderer(player) || Guest(player)) {
         if (Guest(player) && (mudconf.connect_perm & 100)) {
            i_ret = 777; /* Always work */
         } else {
            i_ret = 0;
         }
      } else {
         i_ret = 1;
      }
   }
   return i_ret;
}

static int 
check_connect(DESC * d, const char *msg, int key, int i_attr)
{
   char *command, *user, *password, *buff, *cmdsave, *buff3, *addroutbuf, *tsite_buff, *s_god,  
        buff2[10], cchk[SBUF_SIZE], *in_tchr, tchar_buffer[600], *tstrtokr, *s_uselock, *sarray[5];
   int aflags, nplayers, comptest, gnum, bittemp, bitcmp, postest, overf, dc, tchar_num, is_guest, i_return, i_size, i_chk,
       ok_to_login, i_sitemax, postestcnt, i_atr, chk_tog, guest_randomize[32], guest_bits[32], guest_randcount, i_allow;
#ifdef ZENTY_ANSI
   char *lbuf1, *lbuf1ptr, *lbuf2, *lbuf2ptr, *lbuf3, *lbuf3ptr;
#endif
   time_t chk_stop;
   dbref player, aowner, player2, victim;
   DESC *d2, *d3;
   CMDENT *cmdp;
   ATTR *hk_ap2, *atr;

   DPUSH; /* #146 */

   bittemp = bitcmp = i_chk = 0;
   i_allow = i_return = 1;
   cmdsave = mudstate_hot.debug_cmd;
   mudstate_hot.debug_cmd = (char *) "< check_connect >";


   /* Hide the password length from SESSION */
   D_INPUT_TOT(d) -= (strlen(msg) + 1);

   /* Crack the command apart */
   memset(cchk, '\0', SBUF_SIZE);
   i_size = 1;
   if ( *msg ) {
      buff = (char *)msg;
      buff3 = (char *)cchk;
      dc = 0;
      /* the connect strings are hardcoded max 31 chars */
      while ( *buff && (dc < 31) ) {
         if ( isspace(*buff) ) {
            break;
         }
         *buff3 = ToLower(*buff);   
         buff++;
         buff3++;
         dc++;
      }
      i_size = strlen(cchk);
   }

   if ( mudconf.connect_methods > 0 ) {
       addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, (D_FLAGS(d) & DS_API));
       tsite_buff = alloc_lbuf("hardconn_check");
      strcpy(tsite_buff, mudconf.hardconn_host);
      if ( (mudconf.connect_methods & 1) && 
           (!strncmp(cchk, mudconf.string_conn, i_size) ||
            !strncmp(cchk, mudconf.string_conndark, i_size) ||
            !strncmp(cchk, mudconf.string_connhide, i_size)) ) {
           if ( !(mudconf.hardconn_host[0] && lookup(addroutbuf, tsite_buff, 1, &i_sitemax)) &&
                 !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_HARDCONN) & H_HARDCONN) ) {
               i_allow = 0;
            }
         }
         if ( i_allow && (mudconf.connect_methods & 2) && !strncmp(cchk, mudconf.string_create, i_size) ) {
            if ( !(mudconf.hardconn_host[0] && lookup(addroutbuf, tsite_buff, 1, &i_sitemax)) &&
                 !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_HARDCONN) & H_HARDCONN) ) {
               i_allow = 0;
            }
         }
         if ( i_allow && (mudconf.connect_methods & 4) && !strncmp(cchk, mudconf.string_register, i_size) ) {
            if ( !(mudconf.hardconn_host[0] && lookup(addroutbuf, tsite_buff, 1, &i_sitemax)) &&
                 !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_HARDCONN) & H_HARDCONN) ) {
             i_allow = 0;
          }
       }
      free_lbuf(tsite_buff);
   }

        i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_NOGUEST);
    dc = 0;
   command = alloc_mbuf("check_conn.cmd");
   user = alloc_mbuf("check_conn.user");
   password = alloc_mbuf("check_conn.pass");
   overf = parse_connect(msg, command, user, password);
   if ( strlen(user) > 120 )
      overf = 0;
   if ( !(!strncmp(cchk, mudconf.string_conn, i_size) || !strncmp(cchk, mudconf.string_conndark, i_size) || 
          !strncmp(cchk, mudconf.string_connhide, i_size) || ((key == 1) && !strncmp(cchk, "zz", 2))) )
      overf = 1;
   if ( strlen(msg) > 2000 )
      overf = 0;
   if (!overf) {
      queue_string(d,"Your attempt to crash this mush has been noted and logged.\r\n");
      STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "OVERFLOW")
         *(((char *)msg)+LBUF_SIZE-MBUF_SIZE-1) = '\0';
         buff = alloc_lbuf("check_conn.LOG.over");
         sprintf(buff, "[%d/%s] Attempted overflow -> %.3800s", D_DESCRIPTOR(d), d->cold->longaddr, msg);
         log_text(buff);
         free_lbuf(buff);
      ENDLOG
      broadcast_monitor(NOTHING, MF_CONN, "ATTEMPTED OVERFLOW", d->cold->userid, d->cold->longaddr, D_DESCRIPTOR(d), 0, 0, NULL);
      free_mbuf(command);
      free_mbuf(user);
      free_mbuf(password);
      shutdownsock(d, R_HACKER);
      mudstate_hot.debug_cmd = cmdsave;
      RETURN(0); /* #146 */
   }

/* PUT IN TOR PROTECTION HERE FOR CO, CD, CR and REG */

/*
    STARTLOG(LOG_CONNECT, "CMD", "BAD")
       buff = alloc_lbuf("check_conn.LOG.bad");
       sprintf(buff, "[%d/%s] On connect screen -> %.3900s",
               D_DESCRIPTOR(d), d->cold->longaddr, msg);
       log_text(buff);
       free_lbuf(buff);
    ENDLOG
*/

   /* Guest determination */
   strncpy(buff2, user, 5);
   *(buff2 + 5) = '\0';
   if (!strncmp(cchk, mudconf.string_conn, i_size)) {
      comptest = stricmp(buff2, "guest");
   } else {
      comptest = 1;
   }

   if ( (key == 1) && !strncmp(cchk, "zz", 2) && (stricmp(buff2, "guest") == 0) ) {
      queue_string(d,"Guests can not connect with this method.\r\n");
      free_mbuf(command);
      free_mbuf(user);
      free_mbuf(password);
      mudstate_hot.debug_cmd = cmdsave;
      RETURN(1); /* #146 */
   }
   player2 = lookup_player(NOTHING, user, 0);
   is_guest = 0;
   if ( player2 != NOTHING && Guest(player2) )
      is_guest = 1;
   if ( (!comptest && ((strlen(user) > 5) || (strcmp(password, "guest")))) ||
        (is_guest && !strcmp(password, "guest") && comptest) ) {
      queue_string(d, "Use '");
      queue_string(d, mudconf.string_conn);
      queue_string(d, " guest guest' to connect to a guest character.\r\n");
      STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
         buff = alloc_lbuf("check_conn.LOG.bad");
         sprintf(buff, "[%d/%s] Failed connect to '%s'", D_DESCRIPTOR(d), d->cold->longaddr, user);
         log_text(buff);
         free_lbuf(buff);
      ENDLOG
      if (player2 == NOTHING) {
         broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD GUEST CONNECT)", 
                           d->cold->userid, d->cold->longaddr, 0, 0, 0, user);
      } else {
         broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD GUEST CONNECT)", 
                           d->cold->userid, d->cold->longaddr, 0, 0, 0, 
                           (char *)unsafe_tprintf("%s  [%s]", user, Name(player2)));
      }
      is_guest = 1; /* Enforce guest check */
      if ( --(d->cold->retries_left) <= 0 ) {
         free_mbuf(command);
         free_mbuf(user);
         free_mbuf(password);
         shutdownsock(d, R_BADLOGIN);
         mudstate_hot.debug_cmd = cmdsave;
         RETURN(0); /* #146 */
      }
      *command = 'a';
      comptest = 1;
    } else if (!comptest && (D_HOST_INFO(d) & H_NOGUEST)) {
       i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_NOGUEST);
       if ( i_sitemax == -1 ) {
          tsite_buff = alloc_lbuf("noguest_check");
          addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, 0);
          strcpy(tsite_buff, mudconf.noguest_host);
          lookup(addroutbuf, tsite_buff, 1, &i_sitemax);
          free_lbuf(tsite_buff);
       }
      if ( i_sitemax == -1 ) {
         broadcast_monitor(NOTHING, MF_SITE | MF_FAIL | MF_GFAIL, "NO GUEST FAIL", 
                           d->cold->userid, d->cold->longaddr, 0, 0, 0, NULL);
         fcache_dump(d, FC_GUEST_FAIL, (char *)NULL);
      } else {
         broadcast_monitor(NOTHING, MF_SITE | MF_FAIL | MF_GFAIL, unsafe_tprintf("NO GUEST FAIL[%d max]", i_sitemax), 
                           d->cold->userid, d->cold->longaddr, 0, 0, 0, NULL);
         fcache_dump(d, FC_GUEST_FAIL, (char *)"SITE_NOGUEST");
      }
 
      STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "GUESTFAIL")
         buff = alloc_lbuf("check_conn.LOG,badguest");
         sprintf(buff, "[%d/%s] Attempt to connect to guest char from registered site",
                 D_DESCRIPTOR(d), d->cold->longaddr);
         log_text(buff);
         free_lbuf(buff);
      ENDLOG
      if ( --(d->cold->retries_left) <= 0 ) {
         free_mbuf(command);
         free_mbuf(user);
         free_mbuf(password);
	 shutdownsock(d, R_BADLOGIN);
	 mudstate_hot.debug_cmd = cmdsave;
	 RETURN(0); /* #146 */
      }
      *command = 'a';
      comptest = 1;
   } else if (!comptest && (mudstate.guest_num >= mudconf.max_num_guests)) {
      if ( mudconf.max_num_guests <= 0 ) {
         queue_string(d, "Guests have been disabled.\r\n");
      } else {
         queue_string(d, "Maximum number of guests has been reached.\r\n");
      }
      STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
         buff = alloc_lbuf("check_conn.LOG.bad");
         sprintf(buff, "[%d/%s] Failed connect to '%s'",
                 D_DESCRIPTOR(d), d->cold->longaddr, user);
         log_text(buff);
         free_lbuf(buff);
      ENDLOG
      if ( --(d->cold->retries_left) <= 0 ) {
         free_mbuf(command);
         free_mbuf(user);
         free_mbuf(password);
         shutdownsock(d, R_BADLOGIN);
         mudstate_hot.debug_cmd = cmdsave;
         RETURN(0); /* #146 */
      }
      *command = 'a';
      comptest = 1;
   } else if (!comptest) {
      bittemp = bitcmp = 0x00000001;
      gnum = 1;
      if ( mudconf.guest_randomize ) {
         guest_randcount = 0;
         while ( gnum < 32 ) {
            guest_randomize[gnum-1] = 1;
            guest_bits[gnum-1] = 0x00000001;
            if ( !(bitcmp & mudstate.guest_status) && (gnum <= mudconf.max_num_guests) ) {
               guest_randomize[guest_randcount] = gnum;
               guest_bits[guest_randcount] = bitcmp;
               guest_randcount++;
            }
            bitcmp <<= 1;
            gnum++;
         }
         if ( guest_randcount > 0 ) {
            guest_randcount = random() % guest_randcount;
            gnum = guest_randomize[guest_randcount];
            bitcmp = guest_bits[guest_randcount];
         } else {
            gnum = 1;
            bitcmp = 0x00000001;
         }
         bittemp = bitcmp;
      } else {
         while ((bittemp & (mudstate.guest_status)) && (gnum < 32)) {
            bittemp <<= 1;
            gnum++;
         }
      }
      if ( strlen(mudconf.guest_namelist) > 0 ) {
         memset(tchar_buffer, 0, sizeof(tchar_buffer));
         strcpy(tchar_buffer, mudconf.guest_namelist);
         in_tchr = strtok_r(tchar_buffer, " \t", &tstrtokr);
         tchar_num = 1;
         while ( (tchar_num < gnum) && (tchar_num < 32) ) {
            in_tchr = strtok_r(NULL, " \t", &tstrtokr);
            if ( in_tchr == NULL ) {
               break;
            }
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
   if ( (i_allow && ((!strncmp(cchk, mudconf.string_conn, i_size)) || 
          (!strncmp(cchk, mudconf.string_conndark, i_size)) || (!strncmp(cchk, mudconf.string_connhide, i_size)))) ||
        ((key == 1) && !strncmp(cchk, "zz", 2)) ) {
      /* See if this connection would exceed the max #players */
      if (mudconf.max_players > mudstate.max_logins_allowed)
         mudconf.max_players = mudstate.max_logins_allowed;
      if (mudconf.max_players < 0) {
         /* Can't be helped.  Have to check conn players regardless */
         nplayers = 0;
         DESC_ITER_CONN(d2) {
            nplayers++;
         }
         if ( nplayers > mudstate.max_logins_allowed ) {
            nplayers = mudconf.max_players - 1;
         }
      } else {
         nplayers = 0;
         DESC_ITER_CONN(d2) {
            nplayers++;
         }
      }

      ok_to_login = (((nplayers < mudconf.max_players) || (mudconf.max_players == -1)) && (nplayers < mudstate.max_logins_allowed));
      if (!strncmp(cchk, mudconf.string_conndark, i_size))
         dc = 1;
      else if ( !strncmp(cchk, mudconf.string_connhide, i_size))
         dc = 2;
      else
         dc = 0;
      player = connect_player(user, password, (char *)d, key, i_attr);
      player2 = lookup_player(NOTHING, user, 0);
      if ( mudconf.connect_perm > 100 ) {
         i_chk = mudconf.connect_perm - 100;
      } else {
         i_chk = mudconf.connect_perm;
      }

      /* Trying to connect to player below specified bitlevel */
      if (  Good_chk(player2) && (chkbit(player2) < i_chk) ) {
         queue_string(d, "Connections to that player are unable to use the standard connect command.\r\n");
         queue_string(d, "Try account management?\r\n");
         broadcast_monitor(player, MF_SITE | MF_FAIL, "FAIL (CONNECT PERM RESTRICTED)", 
                           d->cold->userid, d->cold->longaddr, 0, 0, 0, NULL);
         STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "RESTRICT")
            buff = alloc_lbuf("check_conn.LOG.restrict");
            sprintf(buff, "[%d/%s] (RESTRICTED) Failed connect to '%s'",
                    D_DESCRIPTOR(d), d->cold->longaddr, user);
            log_text(buff);
            free_lbuf(buff);
         ENDLOG
         if ( --(d->cold->retries_left) <= 0 ) {
            free_mbuf(command);
            free_mbuf(user);
            free_mbuf(password);
            shutdownsock(d, R_BADLOGIN);
            mudstate_hot.debug_cmd = cmdsave;
            RETURN(0); /* #146 */
         }
      } else if (player == NOPERM) {
         queue_string(d, "Connections to that player are not allowed from your site.\r\n");
         STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BADSITE")
            buff = alloc_lbuf("check_conn.LOG.bad");
            sprintf(buff, "[%d/%s] Failed connect to '%s'",
                    D_DESCRIPTOR(d), d->cold->longaddr, user);
            log_text(buff);
            free_lbuf(buff);
         ENDLOG
         if ( --(d->cold->retries_left) <= 0 ) {
            free_mbuf(command);
            free_mbuf(user);
            free_mbuf(password);
            shutdownsock(d, R_BADLOGIN);
            mudstate_hot.debug_cmd = cmdsave;
            RETURN(0); /* #146 */
         }
      } else {
         postest = postestcnt = 0;
         if ((player != NOTHING) && NoPossess(player)) {
            DESC_ITER_CONN(d3) {
               if (D_PLAYER(d3) == player) {
                  if ( strcmp(d3->cold->longaddr, d->cold->longaddr) || strcmp(d3->cold->userid, d->cold->userid)) {
                     postest = 1;
                     break;
                  }
                  if ( !strcmp(d3->cold->longaddr, d->cold->longaddr) ) {
                     postestcnt++;
                  }
               }
            }
         }
         if ( postestcnt >= 2 ) {
            postest = 1;
         }
         if ((player == NOTHING) || (Flags3(player) & NOCONNECT) || postest ) {
            if ((player != NOTHING) && (Flags3(player) & NOCONNECT))
               broadcast_monitor(player, MF_SITE | MF_FAIL, "FAIL (NOCONNECT)", 
                                 d->cold->userid, d->cold->longaddr, 0, 0, 0, NULL);
            if (postest)
               broadcast_monitor(player, MF_SITE | MF_FAIL, "FAIL (NOPOSSESS)", 
                                 d->cold->userid, d->cold->longaddr, 0, 0, 0, NULL);
            if ( !is_guest && (player == NOTHING) && (player2 == NOTHING))
               broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CONNECT)", 
                                 d->cold->userid, d->cold->longaddr, 0, 0, 0, user);
            if ( !is_guest && (player == NOTHING) && (player2 != NOTHING))
               broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CONNECT)", 
                                 d->cold->userid, d->cold->longaddr, 0, 0, 0, 
                                 (char *)unsafe_tprintf("%s  [%s]", user, Name(player2)));

            /* Not a player, or wrong password */

            atr = NULL;
            i_atr = -1;
            if ( Good_chk(player) && (Flags3(player) & NOCONNECT) ) {
               i_atr = mkattr("_NOCONNECT_MSG");
               if ( i_atr > 0 ) {
                  atr = atr_num(i_atr);
                  if ( atr ) {
                     attr_wizhidden("_NOCONNECT_MSG");
                     buff = atr_get(player, atr->number, &aowner, &aflags);
                     if ( !buff || !*buff ) {
                        free_lbuf(buff);
                        i_atr = -1;
                     }
                  }
               }
            } else if ( postest ) {
               i_atr = mkattr("_NOPOSSESS_MSG");
               if ( i_atr > 0 ) {
                  atr = atr_num(i_atr);
                  if ( atr ) {
                     attr_wizhidden("_NOPOSSESS_MSG");
                     buff = atr_get(player, atr->number, &aowner, &aflags);
                     if ( !buff || !*buff ) {
                        free_lbuf(buff);
                        i_atr = -1;
                     }
                  }
               }
            }
            if ( (i_atr <= 0 ) && Good_chk(mudconf.file_object) && Immortal(Owner(mudconf.file_object)) ) {
               if ( postest ) {
                  atr = atr_str("SITE_NOPOSSESS");
               } else if ( Good_chk(player) && (Flags3(player) & NOCONNECT) ) {
                  atr = atr_str("SITE_NOCONNECT");
               }
            }
            if ( atr ) {
               if ( i_atr == -1 ) {
                  buff = atr_get(mudconf.file_object, atr->number, &aowner, &aflags);
               }
               if ( buff && *buff ) {
                  sarray[0] = alloc_lbuf("noconnect1");
                  sarray[1] = alloc_lbuf("noconnect2");
                  sarray[2] = alloc_lbuf("noconnect3");
                  sarray[3] = alloc_lbuf("noconnect4");
                  sarray[4] = NULL;
                   strcpy(sarray[0], d->cold->addr);
                  strcpy(sarray[1], d->cold->longaddr);
                  sprintf(sarray[2], "%d", D_DESCRIPTOR(d));
                  if ( !Good_chk(player ) )
                     sprintf(sarray[3], "#%d", NOTHING);
                  else
                     sprintf(sarray[3], "#%d", player);
                  if ( i_atr == -1 ) {
                     buff3 = cpuexec(mudconf.file_object, mudconf.file_object, mudconf.file_object, 
                                     EV_STRIP | EV_FCHECK | EV_EVAL, buff, sarray, 4, (char **)NULL, 0);
                     if ( mudstate_hot.chkcpu_toggle ) {
                        broadcast_monitor(mudconf.file_object, MF_CPU, "CPU RUNAWAY LOGIN PROCESS",
                                          (postest ? (char *)"SITE_NOPOSSESS" : (char *)"SITE_NOCONNECT"), 
                                          NULL, mudconf.file_object, 0, 0, NULL);
                        STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                           log_name_and_loc(mudconf.file_object);
                           sprintf(buff, " CPU login process overload on attribute %s", 
                                   (postest ? (char *)"SITE_NOPOSSESS" : (char *)"SITE_NOCONNECT"));
                           log_text(buff);
                        ENDLOG
                        mudstate_hot.chkcpu_toggle = 0;
                     }
                  } else {
                     buff3 = cpuexec(player, player, player,
                                     EV_STRIP | EV_FCHECK | EV_EVAL, buff, sarray, 4, (char **)NULL, 0);
                     if ( mudstate_hot.chkcpu_toggle ) {
                        broadcast_monitor(player, MF_CPU, "CPU RUNAWAY LOGIN PROCESS",
                                          (postest ? (char *)"SITE_NOPOSSESS" : (char *)"SITE_NOCONNECT"), 
                                          NULL, player, 0, 0, NULL);
                        STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                           log_name_and_loc(player);
                           sprintf(buff, " CPU login process overload on attribute %s", 
                                   (postest ? (char *)"SITE_NOPOSSESS" : (char *)"SITE_NOCONNECT"));
                           log_text(buff);
                        ENDLOG
                        mudstate_hot.chkcpu_toggle = 0;
                     }
                  }
                  if ( buff3 && *buff3 ) {
#ifdef ZENTY_ANSI
                     lbuf1ptr = lbuf1 = alloc_lbuf("noconnect_ansi1");
                     lbuf2ptr = lbuf2 = alloc_lbuf("noconnect_ansi2");
                     lbuf3ptr = lbuf3 = alloc_lbuf("noconnect_ansi3");
                     parse_ansi(buff3, lbuf1, &lbuf1ptr, lbuf2, &lbuf2ptr, lbuf3, &lbuf3ptr);
                     queue_write(d, lbuf1, strlen(lbuf1));
                     queue_string(d, "\r\n");
                     free_lbuf(lbuf1);
                     free_lbuf(lbuf2);
                     free_lbuf(lbuf3);
#else
                     queue_string(d, buff3);
                     queue_string(d, "\r\n");
#endif
                  } else {
                     queue_string(d, connect_fail);
                  }
                  free_lbuf(buff3);
                  free_lbuf(sarray[0]);
                  free_lbuf(sarray[1]);
                  free_lbuf(sarray[2]);
                  free_lbuf(sarray[3]);
               } else {
                  queue_string(d, connect_fail);
               }
               free_lbuf(buff);
            } else {
               queue_string(d, connect_fail);
            }
            STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD")
               buff = alloc_lbuf("check_conn.LOG.bad");
               sprintf(buff, "[%d/%s] Failed connect to '%s'",
                       D_DESCRIPTOR(d), d->cold->longaddr, user);
               log_text(buff);
               free_lbuf(buff);
            ENDLOG
            if ( --(d->cold->retries_left) <= 0 ) {
               free_mbuf(command);
               free_mbuf(user);
               free_mbuf(password);
               shutdownsock(d, R_BADLOGIN);
               mudstate_hot.debug_cmd = cmdsave;
               RETURN(0); /* #146 */
            }
         } else if ( ((mudconf.control_flags & CF_LOGIN) && ok_to_login) ||
                     Wizard(player) || God(player) || (Login(player) & ok_to_login)) {
            /* Logins are enabled, or wiz or god */
            STARTLOG(LOG_LOGIN, "CON", "LOGIN")
               buff = alloc_lbuf("check_conn.LOG.login");
               sprintf(buff, "[%d/%s] Connected to ",
                       D_DESCRIPTOR(d), d->cold->longaddr);
               log_text(buff);
               log_name_and_loc(player);
               free_lbuf(buff);
            ENDLOG
            D_FLAGS(d) |= DS_CONNECTED;
            d->cold->connected_at = time(0);
            D_PLAYER(d) = player;
            mudstate.recordcurrconn++;
            if ( mudstate.recordcurrconn > mudstate.recordconn ) {
               mudstate.recordconn = mudstate.recordcurrconn;
               s_god = alloc_lbuf("record_connections");
               sprintf(s_god, "%d", mudstate.recordcurrconn);
               atr_add_raw(GOD, A_CONNRECORD, s_god);
               free_lbuf(s_god);
            }
            /* Give the player the MOTD file and the settable MOTD
             * message(s). Use raw notifies so the player doesn't
             * try to match on the text. */

            if (Guest(player)) {
               fcache_dump(d, FC_CONN_GUEST, (char *)NULL);
            } else {
               buff = atr_get(player, A_LAST, &aowner, &aflags);
               if ((buff == NULL) || (*buff == '\0')) {
                  fcache_dump(d, FC_CREA_NEW, (char *)NULL);
               } else {
                  fcache_dump(d, FC_MOTD, (char *)NULL);
               }
               if (Builder(player)) {
                  fcache_dump(d, FC_WIZMOTD, (char *)NULL);
               }
               free_lbuf(buff);
            }
            if (dc == 1) {
               if (Immortal(player)) {
                  buff = alloc_lbuf("connect_dark");
                  sprintf(buff, "%s", (char *)"scloak unfindable dark");
                  flag_set(player, player, buff, SET_MUFFLE);
                  free_lbuf(buff);
               } else if (Wizard(player)) {
                  buff = alloc_lbuf("connect_dark");
                  sprintf(buff, "%s", (char *)"unfindable dark");
                  flag_set(player, player, buff, SET_MUFFLE);
                  free_lbuf(buff);
               }
               if ( (!Immortal(player) && !Wizard(player)) || 
                    (Wizard(player) && DePriv(player, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA))) {
                  dc = 0;
               }
            } else if (dc == 2) { 
               if ( Wizard(player) || HasPriv(player, NOTHING, POWER_NOWHO, POWER5, NOTHING) ) {
                  s_Flags3(player, Flags3(player) | NOWHO);
               } else {
                  dc = 0;
               }
            }
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
   } else if ( i_allow && !strncmp(cchk, mudconf.string_create, i_size) ) {
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
              ((mudstate.last_pcreate_time + mudconf.max_pcreate_time) > mudstate_hot.now) ) {
            broadcast_monitor(NOTHING, (MF_SITE | MF_BFAIL), "FAIL (CREATE CEILING)", d->cold->userid,
                              d->cold->longaddr, 0, 0, 0, user);
            buff = alloc_mbuf("check_conn.LOG.badcrea");
             if ( mudconf.pcreate_paranoia > 0 ) {
                 sprintf(buff, "%.100s %s", d->cold->addr, (d->cold->addr_family == AF_INET6) ? "/128" : "255.255.255.255");
                  if ( (mudconf.pcreate_paranoia == 1) && 
                       ( !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION) &&
                       ( site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_PERMIT) != H_PERMIT) ) &&
                      !(D_HOST_INFO(d) & H_REGISTRATION) ) {
                    cf_site((int *)&mudstate.access_list, buff, 
                            H_REGISTRATION|H_AUTOSITE, 0, 1, "register_site");
                    sprintf(buff, "(PCREATE) [%.100s] marked for autoregistration.  (Remote port %d)",
                            d->cold->addr, d->cold->remote_port);
                 } else if ( (mudconf.pcreate_paranoia == 2) &&
                             ( !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN) &&
                               !(site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_PERMIT) == H_PERMIT) ) ) {
                    cf_site((int *)&mudstate.access_list, buff, 
                             H_FORBIDDEN|H_AUTOSITE, 0, 1, "forbid_site");
                    sprintf(buff, "(PCREATE) [%.100s] marked for autoforbid.  (Remote port %d)",
                            d->cold->addr, d->cold->remote_port);
                }
               broadcast_monitor(NOTHING, MF_CONN, unsafe_tprintf("PCREATE-SITE AUTO-BLOCKED[%d attempts/%ds time]",
                                 mudstate.last_pcreate_cnt,
                                 (mudstate_hot.now - mudstate.last_pcreate_time)),
                                 d->cold->userid, d->cold->longaddr, 0, 0, 0, user);
               STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOR")
                  log_text(buff);
               ENDLOG
            }
            free_mbuf(buff);
            failconn("CRE", "Create", unsafe_tprintf("[%d/%ds] Max Create Reached", mudconf.max_pcreate_lim, mudconf.max_pcreate_time),
		     d, R_HACKER, NOTHING, FC_QUIT,
		     (char *)NULL, command, user, password,
		     cmdsave);
            RETURN(0); /* #146 */
         } else {
            if ( (mudstate.last_pcreate_time + mudconf.max_pcreate_time) < mudstate_hot.now ) {
               mudstate.last_pcreate_cnt = 0;
            }
            mudstate.last_pcreate_time = mudstate_hot.now;
         } 
         mudstate.last_pcreate_cnt++;
      }
      /* Enforce max #players */
      if (mudconf.max_players > mudstate.max_logins_allowed)
         mudconf.max_players = mudstate.max_logins_allowed;
      if (mudconf.max_players < 0) {
         /* Sorry man, we need to do this */
         nplayers = 0;
         DESC_ITER_CONN(d2) {
            nplayers++;
         }
         if ( nplayers > (mudstate.max_logins_allowed) ) {
            nplayers = mudconf.max_players - 1;
         }
      } else {
         nplayers = 0;
         DESC_ITER_CONN(d2) {
            nplayers++;
         }
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
      if ((D_HOST_INFO(d) & H_REGISTRATION) && !(D_HOST_INFO(d) & H_PERMIT)) {
           i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_REGISTRATION);
          if ( i_sitemax == -1 ) {
             tsite_buff = alloc_lbuf("register_check");
             addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, 0);
            strcpy(tsite_buff, mudconf.register_host);
            lookup(addroutbuf, tsite_buff, 1, &i_sitemax);
            free_lbuf(tsite_buff);
         }
         if ( i_sitemax != -1 ) {
            fcache_dump(d, FC_CREA_REG, (char *)"SITE_REGISTER");
         } else {
            fcache_dump(d, FC_CREA_REG, (char *)NULL);
         }
      } else {
         player = create_player(user, password, NOTHING, 0, user, 0);
         if (player == NOTHING) {
            if ( !ok_password(password, (char *)NULL, NOTHING, 0) ) {
               queue_string(d, (char *)"Invalid password specified.\r\n");
               if ( mudconf.safer_passwords ) {
                  queue_string(d, (char *)"Passwords must have 1 upper, 1 lower, and 1 non-alpha and be 5+ chars long.\r\n");
               }
            } 
            broadcast_monitor(NOTHING, MF_SITE | MF_BFAIL, "FAIL (BAD CREATE)", d->cold->userid, 
                              d->cold->longaddr, 0, 0, 0, user);
            queue_string(d, create_fail);
            STARTLOG(LOG_SECURITY | LOG_PCREATES, "CON", "BAD")
               buff = alloc_lbuf("check_conn.LOG.badcrea");
               sprintf(buff, "[%d/%s] Create of '%s' failed",
                       D_DESCRIPTOR(d), d->cold->longaddr, user);
               log_text(buff);
               free_lbuf(buff);
            ENDLOG
            if (!mudconf.pcreate_paranoia_fail) {
               if (mudconf.max_pcreate_lim != -1) 
                  if(mudstate.last_pcreate_cnt > 0)
                     mudstate.last_pcreate_cnt--;
            }
         } else {
            STARTLOG(LOG_LOGIN | LOG_PCREATES, "CON", "CREA")
               buff = alloc_lbuf("check_conn.LOG.create");
               sprintf(buff, "[%d/%s] Created ", D_DESCRIPTOR(d), d->cold->longaddr);
               log_text(buff);
               log_name(player);
               free_lbuf(buff);
            ENDLOG

            move_object(player, mudconf.start_room);
            D_FLAGS(d) |= DS_CONNECTED;
            d->cold->connected_at = time(0);
            D_PLAYER(d) = player;
            mudstate.recordcurrconn++;
            if ( mudstate.recordcurrconn > mudstate.recordconn ) {
               mudstate.recordconn = mudstate.recordcurrconn;
               s_god = alloc_lbuf("record_connections");
               sprintf(s_god, "%d", mudstate.recordcurrconn);
               atr_add_raw(GOD, A_CONNRECORD, s_god);
               free_lbuf(s_god);
            }
            fcache_dump(d, FC_CREA_NEW, (char *)NULL);
            announce_connect(player, d, 0);

            /* Trigger the hook for player creation */
            if ( Good_chk(mudconf.hook_obj) ) {
               cmdp = lookup_command((char *)"@pcreate");
               if ( (cmdp->hookmask & HOOK_AFTER) ) {
                  s_uselock = alloc_sbuf("offline_create");
                  sprintf(s_uselock, "%s", (char *)"AO_@PCREATE");
                  hk_ap2 = atr_str(s_uselock);
                  chk_stop = mudstate_hot.chkcpu_stopper;
                  chk_tog  = mudstate_hot.chkcpu_toggle;
                  mudstate_hot.chkcpu_stopper = time(NULL);
                  mudstate_hot.chkcpu_toggle = 0;
                  mudstate_hot.chkcpu_locktog = 0;
                  process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0, cmdp->hookmask, (char *)NULL);
                  mudstate_hot.chkcpu_toggle = chk_tog;
                  mudstate_hot.chkcpu_stopper = chk_stop;
                  free_sbuf(s_uselock);
               }
            }
         }
      }
   } else if ( i_allow && mudconf.offline_reg && !strncmp(cchk, mudconf.string_register, i_size)) {
      if (D_HOST_INFO(d) & H_NOAUTOREG) {
         buff3 = alloc_lbuf("reg.fail");
         queue_string(d, "Permission denied.\r\n");
         process_output(d);
         strcpy(buff3,user);
         strcat(buff3,"/");
         strcat(buff3,password);
         broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL (NoAutoReg)", d->cold->userid, d->cold->longaddr, D_DESCRIPTOR(d), 0, 0, buff3);
         free_lbuf(buff3);
      } else if (d->cold->regtries_left <= 0) {
         buff3 = alloc_lbuf("reg.fail");
         queue_string(d, "Registration limit reached.\r\n");
         process_output(d);
         strcpy(buff3,user);
         strcat(buff3,"/");
         strcat(buff3,password);
         broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL LIMIT", d->cold->userid, d->cold->longaddr, D_DESCRIPTOR(d), 0, 0, buff3);
         free_lbuf(buff3);
      } else {
         switch (reg_internal(user, password, (char *)d, 0, NULL, user, 0)) {
            case 0: /* Success -- Email out */
               (d->cold->regtries_left)--;
               queue_string(d, "Autoregistration password emailed.\r\n");
               victim = lookup_player(GOD, user, 1);
               if ( Good_chk(mudconf.hook_obj) && Good_chk(victim) ) {
                  cmdp = lookup_command((char *)"@register");
                  if ( (cmdp->hookmask & HOOK_AFTER) ) {
                     s_uselock = alloc_sbuf("offline_register");
                     sprintf(s_uselock, "%s", (char *)"AO_@REGISTER");
                     hk_ap2 = atr_str(s_uselock);
                     chk_stop = mudstate_hot.chkcpu_stopper;
                     chk_tog  = mudstate_hot.chkcpu_toggle;
                     mudstate_hot.chkcpu_stopper = time(NULL);
                     mudstate_hot.chkcpu_toggle = 0;
                     mudstate_hot.chkcpu_locktog = 0;
                     process_hook(victim, mudconf.hook_obj, s_uselock, hk_ap2, 0, cmdp->hookmask, (char *)NULL);
                     mudstate_hot.chkcpu_toggle = chk_tog;
                     mudstate_hot.chkcpu_stopper = chk_stop;
                     free_sbuf(s_uselock);
                  }
               }
               break;
            case 1: /* Invalid character name or name exists already */
               queue_string(d, "Autoregistration character invalid or already exists.\r\n");
               break;
            case 2: /* Failing in emailing the password to player */
               queue_string(d, "Email of password failed.\r\n");
               break;
            case 3: /* Invalid email address due to white space */
               queue_string(d, "Space in email address.\r\n");
               break;
            case 4: /* No permission to register */
               queue_string(d, "Permission denied.\r\n");
               break;
            case 5: /* Invalid email address */
               queue_string(d, "Invalid email specified.\r\n");
               break;
            case 6: /* Email is not allowed due to matching config disallowing email addy */
               queue_string(d, "That email address is not allowed.\r\n");
               break;
            case 7: /* Invalid character(s) in email address */
               queue_string(d, "Invalid character detected in email.\r\n");
               break;
         }
      }
   } else {
      if ( !softcode_trigger(d, msg) ) {
         welcome_user(d);
         i_return = 0;
      }
      if ( Good_obj(D_PLAYER(d)) && !TogHideIdle(D_PLAYER(d)) ) {
         d->cold->command_count++;
      }
   }
   free_mbuf(command);
   free_mbuf(user);
   free_mbuf(password);
   if (!comptest) {
      mudstate.guest_num++;
      mudstate.guest_status |= bittemp;
   }
   mudstate_hot.debug_cmd = cmdsave;
   RETURN(i_return); /* #146 */
}
int
check_connect_ex(DESC * d, char *msg, int key, int i_attr)
{
   int i_return, i_override;

   
   i_override = mudstate.account_subsys_inuse;
   mudstate.account_subsys_inuse = 1;
   i_return = check_connect(d, msg, key, i_attr);
   mudstate.account_subsys_inuse = i_override;
   return(i_return);
}

extern int igcheck(dbref, int);

int 
do_command(DESC * d, char *command)
{
#ifdef HAS_OPENSSL
#ifdef ZENTY_ANSI
    char *s_ansi1, *s_ansi2, *s_ansi3, *s_ansi1p, *s_ansi2p, *s_ansi3p;
#endif
    char *s_usepass, *s_usepassptr, 
         *s_user, *s_snarfing, *s_snarfing2, *s_snarfing3, *s_snarfing4, *s_snarfheader, *s_snarfvalue, *s_buffer,
         *s_get, *s_pass, *s_enc64, *s_enc64ptr;
    double i_time;
    int i_cputog, i_encode64, i_snarfing, i_enc64, i_parse, i_usepass, i_snarfing4, i_snarfheaders;
    dbref thing;
    ATTR *atrp;
#endif
    dbref aowner;
    char *s_strtok, *s_strtokr, s_msg[80];
#ifdef ENABLE_LUA
    char *s_lua, *s_luaptr;
    int i_lualength;
    lua_t *lua;
#endif
    char *arg, *cmdsave, *time_str, *s_rollback, *addroutbuf, *addrsav,
         *s_sitetmp, *s_sitebuff, *haproxy_srcip, *haproxy_rest;
    int retval, cval, gotone, store_perm, chk_perm, i_rollback, i_jump,
        maxsitecon, i_retvar, i_valid, aflags, no_space, i_do_proxy;
    struct SNOOPLISTNODE *node;
#ifdef HAS_OPENSSL
    struct itimerval itimer;
    int i_timeout;
#endif
    DESC *sd, *d2, *dssl;
    NAMETAB *cp;

    DPUSH; /* #147 */

    memset(s_msg, '\0', sizeof(s_msg));
    i_do_proxy = 0;
    time_str = NULL;
    chk_perm = store_perm = 0;
    cmdsave = mudstate_hot.debug_cmd;
    mudstate_hot.debug_cmd = (char *) "< do_command >";
    mudstate_hot.breakst = 0;
    mudstate_hot.jumpst = 0;
    if( mudstate.gotostate > 0) {
       memset(mudstate.gotolabel, '\0', 16);
       mudstate.gotostate = 0;
    }
    mudstate.rollbackcnt = 0;
    if ( !mudstate.rollbackstate ) {
       mudstate.rollbackcnt = 0;
       memset(mudstate.rollback, '\0', LBUF_SIZE);
    }
    mudstate.breakdolist = 0;
    
    if ( d && Prompt(D_PLAYER(d)) ) {
       s_rollback = atr_get(D_PLAYER(d), A_PROGPROMPT, &aowner, &aflags);
       if ( *s_rollback ) {
          s_strtokr = s_strtok = alloc_lbuf("process_ic_command");
          queue_string(d, safe_tprintf(s_strtok, &s_strtokr, "%s%s%s \377\371", ANSI_HILITE, s_rollback, ANSI_NORMAL));
          free_lbuf(s_strtok);
       }
       free_lbuf(s_rollback);
    }

#ifdef HAS_OPENSSL
    i_timeout = 1;
#endif
    /* snoop on player input -Thorin */
    if (d->cold->snooplist) {
	for (node = d->cold->snooplist; node; node = node->next) {
	    if (node->sfile) {
		fprintf(node->sfile, "%ld>", mudstate_hot.now);
		fputs(Name(D_PLAYER(d)), node->sfile);
		fputs("> ", node->sfile);
		fputs(command, node->sfile);
		fputc('\n', node->sfile);
	    }
	    if (!Connected(node->snooper) || node->logonly)
		continue;
	    DESC_ITER_PLAYER(node->snooper, sd) {
                if( ShowAnsi(D_PLAYER(sd)) ) {
                  queue_string(sd, ANSI_HILITE);
                  queue_string(sd, ANSI_WHITE);
                }
		queue_string(sd, ">");
                if( ShowAnsi(D_PLAYER(sd)) ) {
                  queue_string(sd, ANSI_YELLOW);
                }
		queue_string(sd, Name(D_PLAYER(d)));
                if( ShowAnsi(D_PLAYER(sd)) ) {
                  queue_string(sd, ANSI_WHITE);
                }
		queue_string(sd, "> ");
                if( ShowAnsi(D_PLAYER(sd)) ) {
                  queue_string(sd, ANSI_NORMAL);
                }
		queue_string(sd, command);
		queue_write(sd, "\r\n", 2);
                process_output(sd);
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
    
    addroutbuf = NULL;
    if ( !D_PLAYER(d) && *arg && *command && !mudconf.sconnect_reip && *(mudconf.sconnect_cmd) &&
         !strcmp(mudconf.sconnect_cmd, command) ) {
       addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, (D_FLAGS(d) & DS_API));
       s_rollback = alloc_lbuf("sconnect_handler");
       queue_string(d, "SSL attempt to negotiate without SSL enabled.\r\n");
       process_output(d);
       sprintf(s_rollback, "%.50s -> %.50s {%s} ", addroutbuf, arg, arg);
       broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [SSL DISABLED]", NULL, s_rollback,
                          D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
        STARTLOG(LOG_ALWAYS, "NET", "SSL");
           log_text("[DISABLED] ");
          log_text(s_rollback);
       ENDLOG
       free_lbuf(s_rollback);
       shutdownsock(d, R_BOOT);
       RETURN(0); /* #147 */
    }
    /* Support haproxy PROXY as though it were our own */
    haproxy_rest = 0;
    if ( !D_PLAYER(d) && *arg && *command && mudconf.sconnect_reip &&
         !strcmp("PROXY", command) ) {
       strtok_r(arg, " ", &haproxy_rest); /* TCP4 */
       haproxy_srcip = strtok_r(NULL, " ", &haproxy_rest); /* Source IP */
       if(haproxy_srcip) {
           /* Put the source IP in arg to simulate the stunnel command */
           arg = haproxy_srcip;
           i_do_proxy = 1;
           STARTLOG(LOG_ALWAYS, "NET", "PROXY");
            log_text("Received HAPROXY IP ");
            log_text(arg);
           ENDLOG
       } else {
           STARTLOG(LOG_ALWAYS, "NET", "PROXY");
            log_text("HAPROXY attempt without IP");
           ENDLOG
           RETURN(0);
       }
    }
    else if ( !D_PLAYER(d) && *arg && *command && mudconf.sconnect_reip && *(mudconf.sconnect_cmd) &&
         !strcmp(mudconf.sconnect_cmd, command) ) {
       i_do_proxy = 1;
    }

    if ( i_do_proxy ) {
       s_rollback = alloc_lbuf("sconnect_handler");
       addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, (D_FLAGS(d) & DS_API));
       if ( D_FLAGS(d) & DS_SSL ) {
          queue_string(d, "SSL attempt to negotiate twice.\r\n");
          process_output(d);
          sprintf(s_rollback, "%.50s -> %.50s {%s} ", addroutbuf, arg, arg);
          broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [HACKING]", NULL, s_rollback,
                            D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
          STARTLOG(LOG_ALWAYS, "NET", "SSL");
             log_text("[HACKING] ");
             log_text(s_rollback);
          ENDLOG
          /* We're disabling the SSL handler at this point as it's been compromised */
          STARTLOG(LOG_ALWAYS, "NET", "SSL");
             log_text("SSL handler [sconnect_reip] has been disabled as secret [sconnect_cmd] was guessed");
          ENDLOG
          mudconf.sconnect_reip = 0;
          shutdownsock(d, R_BOOT);
          free_lbuf(s_rollback);
          RETURN(0); /* #147 */
       }
       if ( !*(mudconf.sconnect_host) ) {
          sprintf(s_rollback, "%s", (char *)"localhost 127.0.0.1");
       } else {
          strcpy(s_rollback, mudconf.sconnect_host);
       }

       addrsav = alloc_mbuf("address_save");
       addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, (D_FLAGS(d) & DS_API));
       sprintf(addrsav, "%.*s", (MBUF_SIZE - 10), addroutbuf);

       i_valid = 0;
       if ( lookup(addroutbuf, s_rollback, 1, &aflags) ) {
           /* We need to be repetitive and do site check on the SSL redirected IP now */
           /* This will always be valid because of what is going to it -- but check it anyway */
           int ssl_addr_family = AF_INET;
           struct in_addr ssl_ipv4;
           struct in6_addr ssl_ipv6;
           if ( inet_pton(AF_INET6, arg, &ssl_ipv6) == 1 ) {
              ssl_addr_family = AF_INET6;
              i_valid = 1;
           } else if ( inet_pton(AF_INET, arg, &ssl_ipv4) == 1 ) {
              ssl_addr_family = AF_INET;
              i_valid = 1;
           }
           if ( i_valid ) {
              /* Blacklist check and TOR check first */
                if ( blacklist_check_str(arg, ssl_addr_family, 0) || check_tor_str(arg, ssl_addr_family, mudconf.port) ) {
                  if ( mudconf.ssl_welcome ) {
                     D_HOST_INFO(d) |= H_FORBIDDEN;
                     fcache_rawdump(D_DESCRIPTOR(d), FC_CONN_SITE, arg, (char *)NULL);
                  }
                 queue_string(d, "SSL Connections are not allowed from your site.\r\n");
                 process_output(d);
                 sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, arg, arg);
                 broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [BLACKLIST/TOR]", NULL, s_rollback,
                                   D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                 STARTLOG(LOG_ALWAYS, "NET", "SSL");
                    log_text("[BLACKLIST/TOR] ");
                    log_text(s_rollback);
                 ENDLOG
                 free_lbuf(s_rollback);
                 shutdownsock(d, R_BOOT);
                 free_mbuf(addrsav);
                 RETURN(0); /* #147 */
              }
           }
          /* Allocate tmp buffering */
          s_sitetmp = alloc_lbuf("SSL_site_tmp");
          i_retvar = maxsitecon = 0;

           /* Convert IP to DNS if we can */
           if ( i_valid && mudconf.use_hostname) {
              struct sockaddr_storage p_sock_storage;
              memset(&p_sock_storage, 0, sizeof(p_sock_storage));
              if ( ssl_addr_family == AF_INET6 ) {
                 struct sockaddr_in6 *p_sin6 = (struct sockaddr_in6 *)&p_sock_storage;
                 p_sin6->sin6_family = AF_INET6;
                 p_sin6->sin6_addr = ssl_ipv6;
                 if ( getnameinfo((struct sockaddr*)&p_sock_storage, sizeof(struct sockaddr_in6), s_sitetmp, LBUF_SIZE - 1, NULL, 0, NI_NAMEREQD) ) {
                    sprintf(s_sitetmp, "%.*s", (LBUF_SIZE-1), arg);
                 }
              } else if ( ssl_addr_family == AF_INET ) {
                 struct sockaddr_in *p_sin = (struct sockaddr_in *)&p_sock_storage;
                 p_sin->sin_family = AF_INET;
                 p_sin->sin_addr = ssl_ipv4;
                 if ( getnameinfo((struct sockaddr*)&p_sock_storage, sizeof(struct sockaddr_in), s_sitetmp, LBUF_SIZE - 1, NULL, 0, NI_NAMEREQD) ) {
                    sprintf(s_sitetmp, "%.*s", (LBUF_SIZE-1), arg);
                 }
              } else {
                 sprintf(s_sitetmp, "%.*s", (LBUF_SIZE-1), arg);
              }
              /* If site is set not to do DNS, don't do it */
              if ( site_check_str(arg, ssl_addr_family, mudstate.special_list, 1, 0, H_NODNS) & H_NODNS || blacklist_check_str(arg, ssl_addr_family, 1) ) {
                 sprintf(s_sitetmp, "%.*s", (LBUF_SIZE-1), arg);
              }
           } else {
              sprintf(s_sitetmp, "%.*s", (LBUF_SIZE-1), arg);
           }

          /* Count connections */
          DESC_SAFEITER_ALL(dssl) {
	     if (!dssl->cold) continue;
             if ( strcmp(s_sitetmp, dssl->cold->addr) == 0 ) {
                maxsitecon++;
             }
          }

          /* Do site checks if incoming remap site is valid */
          if ( i_valid ) {
             s_sitebuff = alloc_lbuf("SSL_site_buff");

              /* Do forbid site checks */
              strcpy(s_sitebuff, mudconf.permit_host);
              if ( !(site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, H_PERMIT) == H_PERMIT) &&
                   !(lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar)) ) {
                     strcpy(s_sitebuff, mudconf.forbid_host);
                     if ( (site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN) ||
                       lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar)) {
                        if ( mudconf.ssl_welcome ) {
                           D_HOST_INFO(d) |= H_FORBIDDEN;
                           fcache_rawdump(D_DESCRIPTOR(d), FC_CONN_SITE, arg, (char *)NULL);
                        }
                       queue_string(d, "SSL Connections are not allowed from your site.\r\n");
                       process_output(d);
                       sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
                       broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [FORBID]", NULL, s_rollback,
                                         D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                       STARTLOG(LOG_ALWAYS, "NET", "SSL");
                          log_text("[FORBIDDEN] ");
                          log_text(s_rollback);
                       ENDLOG
                       free_lbuf(s_rollback);
                       free_lbuf(s_sitebuff);
                       free_lbuf(s_sitetmp);
                       shutdownsock(d, R_BOOT);
                       free_mbuf(addrsav);
                       RETURN(0); /* #147 */
                    }
              }

               /* Do register site checks -- flag register, log, continue on */
               strcpy(s_sitebuff, mudconf.permit_host);
               if ( !(site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, H_PERMIT) == H_PERMIT) &&
                    !(lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar)) ) {
                      strcpy(s_sitebuff, mudconf.register_host);
                      if ( (site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION) ||
                         lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar) ||
                         blacklist_check_str(arg, ssl_addr_family, 2) ) {
                           i_valid = 2;
                     }
               }
               /* Do noguest site checks -- flag noguest, log, continue on */
               strcpy(s_sitebuff, mudconf.noguest_host);
               if ( (site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, H_NOGUEST) == H_NOGUEST) ||
                    (lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar)) ||
                     blacklist_check_str(arg, ssl_addr_family, 3) ) {
                 if ( i_valid == 2 ) {
                    i_valid |= 4;
                 } else {
                    i_valid = 4;
                 }
              }

              /* Do suspect site checks -- flag suspect, log, continue on */
              strcpy(s_sitebuff, mudconf.suspect_host);
              if ( ( (site_check_str(arg, ssl_addr_family, mudstate.access_list, 1, 0, 0) & ~0x4) | 
                    site_check_str(arg, ssl_addr_family, mudstate.suspect_list, 0, 0, 0)) ||
                   (lookup(s_sitetmp, s_sitebuff, maxsitecon, &i_retvar)) ) {
                 if ( (i_valid == 2) || (i_valid == 4) || (i_valid == 6) ) {
                    i_valid |= 8;
                 } else {
                    i_valid = 8;
                 }
              }
            
             switch(i_valid) {
                case 2: /* Register */
                case 10: /* Suspect * Register */
                   // queue_string(d, "SSL Connections are flagged REGISTER ONLY from your site.\r\n");
                   // process_output(d);
                   strcpy(s_msg, (char *)"SSL Connections are flagged REGISTER ONLY from your site.\r\n");
                   D_HOST_INFO(d) |= H_REGISTRATION;
                   sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
                   if ( i_valid == 10 ) {
                      D_HOST_INFO(d) |= H_SUSPECT;
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [REGISTER & SUSPECT]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   } else {
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [REGISTER]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   }
                   STARTLOG(LOG_ALWAYS, "NET", "SSL");
                      if ( i_valid == 10 ) {
                         log_text("[REGISTERED & SUSPECT] ");
                      } else {
                         log_text("[REGISTERED] ");
                      }
                      log_text(s_rollback);
                   ENDLOG
                   i_valid = 2;
                   break;
                case 4: /* Noguest */
                case 12: /* Suspect * Noguest */
                   // queue_string(d, "SSL Connections are flagged NOGUEST from your site.\r\n");
                   // process_output(d);
                   strcpy(s_msg, (char *)"SSL Connections are flagged NOGUEST from your site.\r\n");
                   D_HOST_INFO(d) |= H_NOGUEST;
                   sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
                   if ( i_valid == 12 ) {
                      D_HOST_INFO(d) |= H_SUSPECT;
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [NOGUEST & SUSPECT]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   } else {
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [NOGUEST]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   }
                   STARTLOG(LOG_ALWAYS, "NET", "SSL");
                      if ( i_valid == 12 ) {
                         log_text("[NOGUEST & SUSPECT] ");
                      } else {
                         log_text("[NOGUEST] ");
                      }
                      log_text(s_rollback);
                   ENDLOG
                   i_valid = 2;
                   break;
                case 6: /* Register & NoGuest */
                case 14: /* Suspect, Register, Noguest */
                   // queue_string(d, "SSL Connections are flagged REGISTER and NOGUEST from your site.\r\n");
                   // process_output(d);
                   strcpy(s_msg, (char *)"SSL Connections are flagged REGISTER and NOGUEST from your site.\r\n");
                   D_HOST_INFO(d) |= H_NOGUEST | H_REGISTRATION;
                   sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
                   if ( i_valid == 14 ) {
                      D_HOST_INFO(d) |= H_SUSPECT;
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [REGISTER, NOGUEST & SUSPECT]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   } else {
                      broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [REGISTER & NOGUEST]", NULL, s_rollback,
                                        D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   }
                   STARTLOG(LOG_ALWAYS, "NET", "SSL");
                      if ( i_valid == 14 ) {
                         log_text("[REGISTER, NOGUEST & SUSPECT] ");
                      } else {
                         log_text("[REGISTER & NOGUEST] ");
                      }
                      log_text(s_rollback);
                   ENDLOG
                   i_valid = 2;
                   break;
                case 8: /* Suspect */
                   /* We don't notify them of site being suspect */
                   D_HOST_INFO(d) |= H_SUSPECT;
                   sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
                   broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [SUSPECT]", NULL, s_rollback,
                                     D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
                   STARTLOG(LOG_ALWAYS, "NET", "SSL");
                      log_text("[SUSPECT] ");
                      log_text(s_rollback);
                   ENDLOG
                   i_valid = 2;
                   break;
             }
             
             free_lbuf(s_sitebuff);
          }

          /* If they gave us a bad IP, we should push it in lastsite but log it anyway */
          if ( !i_valid ) {
             /* We're doing a bit of tricky-dicky here because d->cold->doing is unused on new connections (until connected) */
             /* This will be the IP address stored in &LASTIP */
             memset(d->cold->doing, '\0', sizeof(d->cold->doing));
             strncpy(d->cold->doing, arg, 50);

             /* d->cold->addr is the legacy DNS string */
             memset(d->cold->addr, '\0', sizeof(d->cold->addr));
             strncpy(d->cold->addr, arg, 50);

             /* d->cold->longaddr is a string that shows on the WHO and stored in &LASTSITE */
             memset(d->cold->longaddr, '\0', sizeof(d->cold->longaddr));
             strncpy(d->cold->longaddr, arg, 255);

          } else {
             /* We're doing a bit of tricky-dicky here because d->cold->doing is unused on new connections (until connected) */
             /* This will be the IP address stored in &LASTIP */
             memset(d->cold->doing, '\0', sizeof(d->cold->doing));
             strncpy(d->cold->doing, arg, 50);

             /* d->cold->addr is the legacy DNS string */
             memset(d->cold->addr, '\0', sizeof(d->cold->addr));
             strncpy(d->cold->addr, s_sitetmp, 50);

             /* d->cold->longaddr is a string that shows on the WHO and stored in &LASTSITE */
             memset(d->cold->longaddr, '\0', sizeof(d->cold->longaddr));
             strncpy(d->cold->longaddr, s_sitetmp, 255);
          }

          /* Flag the connection SSL as we're successful at this point and log and continue */
          D_FLAGS(d) |= DS_SSL;

          /* All set up, send the welcome message to user */
          if ( mudconf.ssl_welcome ) {
             welcome_user(d);
          }
          if ( *s_msg ) {
             queue_string(d, s_msg);
             process_output(d);
          }

          /* i_valid = 2 if it's register only, we don't want to do double logging here */
          if ( !i_valid ) {
             sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, arg, arg);
             broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [BADIP]", NULL, s_rollback,
                               D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
             STARTLOG(LOG_ALWAYS, "NET", "SSL");
                log_text("[BADIP] ");
                log_text(s_rollback);
             ENDLOG
          } else if ( i_valid == 1 ) { 
             sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, s_sitetmp, arg);
             broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [OK]", NULL, s_rollback,
                               D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
             STARTLOG(LOG_ALWAYS, "NET", "SSL");
                log_text("[OK] ");
                log_text(s_rollback);
             ENDLOG
          }
          free_lbuf(s_rollback);
          free_lbuf(s_sitetmp);
          free_mbuf(addrsav);

          if ( haproxy_rest && (D_FLAGS(d) & DS_API) ) {
             arg = strstr(haproxy_rest, "\r\n");
             if(arg) {
                arg += 2; /* Skip the \r\n */
                do_command(d, arg);
             } else {
                arg = strstr(haproxy_rest, "\n");
                if(arg) {
                   arg += 1; /* Skip the \n */
                   do_command(d, arg);
                }
             }
          }
          RETURN(0); /* #147 */
       } else {

          /* We shouldn't get here unless you misconfigured your SSL connector or someone guessed your secret */
          queue_string(d, "SSL Connections are not allowed from your site.\r\n");
          process_output(d);
          sprintf(s_rollback, "%.50s -> %.50s {%s} ", addrsav, arg, arg);
          broadcast_monitor(NOTHING, MF_CONN | MF_SITE, "SCONNECT PROXY [DENIED]", NULL, s_rollback,
                            D_DESCRIPTOR(d), 0, d->cold->remote_port, NULL);
          STARTLOG(LOG_ALWAYS, "NET", "SSL");
             log_text("[DENIED] ");
             log_text(s_rollback);
          ENDLOG
          free_lbuf(s_rollback);
          shutdownsock(d, R_BOOT);
          free_mbuf(addrsav);
          RETURN(0); /* #147 */
       }
       free_lbuf(s_rollback);
    }

    if ( D_PLAYER(d) && InProgram(D_PLAYER(d)) && (command[0] == '|') && 
         !((NoShProg(D_PLAYER(d)) && !mudconf.noshell_prog) || 
           (!Immortal(D_PLAYER(d)) && mudconf.noshell_prog && !NoShProg(D_PLAYER(d)))) ) 
       cp = (NAMETAB *) hashfind(command+1, &mudstate.logout_cmd_htab);
    else
       cp = (NAMETAB *) hashfind(command, &mudstate.logout_cmd_htab);
    /* Make modification here for zone/location icmd check and global icmd check */
    cval = 0; 
    if ( cp && D_PLAYER(d) )
       cval = (igcheck(D_PLAYER(d), cp->perm) ? 2 : 0);
    if ( (cval == 0) && cp && D_PLAYER(d) && CmdCheck(D_PLAYER(d)) )
      cval = cmdtest(D_PLAYER(d), cp->name);
    /* Let's do the zone check */
    if ( (cval == 0) && cp && D_PLAYER(d) && Good_obj(Location(D_PLAYER(d))) ) {
       cval = zonecmdtest(D_PLAYER(d), cp->name);
    }
    /* If ignored on WHO or DOING, then allow on connect screen but not in-game */
    if ( cp && (((cp->flag & CMD_MASK) == CMD_WHO) || ((cp->flag & CMD_MASK) == CMD_DOING)) ) {
      if ( cp->perm & CA_IGNORE ) {
	if ( D_PLAYER(d) && Connected(D_PLAYER(d)) )
	  cval = 2;
	else {
             chk_perm = 1;
             store_perm = cp->perm;
             cval = 0;
             cp->perm = (cp->perm & ~CA_IGNORE);
	}
      }
    }

    if (cval == 2) {
      cp = NULL;
    }

    if ( !(D_FLAGS(d) & DS_CONNECTED) && cp && (cp->perm & CF_DARK) ) {
      if ( chk_perm && cp )
         cp->perm = store_perm;
      cp = NULL;
    }

    if ( (D_FLAGS(d) & DS_API) && (cp == NULL) ) {
       handle_html(d, 400, (char *) "Exec: Error - Invalid Headers Supplied\r\n\r\n", NULL, NULL, NULL);
       process_output(d);
       shutdownsock(d, R_API);
       mudstate_hot.debug_cmd = cmdsave;
       if ( chk_perm && cp ) {
          cp->perm = store_perm;
       }
       RETURN(0); /* #147 */
    }

    if ( !(D_FLAGS(d) & DS_API) ) {
       if (cp == NULL || (cp != NULL && InProgram(D_PLAYER(d)) && command[0] != '|') || 
           (((NoShProg(D_PLAYER(d)) && !mudconf.noshell_prog) ||
             (!NoShProg(D_PLAYER(d)) && mudconf.noshell_prog && !Immortal(D_PLAYER(d)))) && 
            InProgram(D_PLAYER(d)))) {
	   if (*arg)
	       *--arg = ' ';	/* restore nullified space */
	   if (D_FLAGS(d) & DS_CONNECTED) {
               if ( Good_obj(D_PLAYER(d)) && !TogHideIdle(D_PLAYER(d)) )
	          d->cold->command_count++;
	       if (d->cold->output_prefix) {
		   queue_string(d, d->cold->output_prefix);
		   queue_write(d, "\r\n", 2);
	       }
	       mudstate_hot.curr_player = D_PLAYER(d);
	       mudstate_hot.curr_enactor = D_PLAYER(d);
               mudstate_hot.curr_pid = -1;
               memset(mudstate.curr_pidcmd, '\0', LBUF_SIZE);
	       desc_in_use = d;
               s_rollback = alloc_lbuf("s_rollback_docommand");
               strcpy(s_rollback, mudstate.rollback);
               i_rollback = mudstate.rollbackcnt;
               i_jump = mudstate_hot.jumpst;
               mudstate_hot.jumpst = mudstate.rollbackcnt = 0;
               strcpy(mudstate.rollback, command);
               no_space = mudstate_hot.no_space_compress;
               if ( *command == '}' ) {
                  mudstate_hot.no_space_compress = 1;
               }
	       process_command(D_PLAYER(d), D_PLAYER(d), 1,
			       command, (char **) NULL, 0, mudstate.shell_program, mudstate_hot.no_hook, mudstate_hot.no_space_compress);
               mudstate_hot.no_space_compress = no_space;
               mudstate.rollbackcnt = i_rollback;
               mudstate_hot.jumpst = i_jump;
               strcpy(mudstate.rollback, s_rollback);
               free_lbuf(s_rollback);
               mudstate_hot.curr_cmd = (char *) "";
	       if (d->cold->output_suffix) {
		   queue_string(d, d->cold->output_suffix);
		   queue_write(d, "\r\n", 2);
	       }
	       mudstate_hot.debug_cmd = cmdsave;
	       if ( chk_perm && cp ) {
	         cp->perm = store_perm;
               }
               process_output(d);
	       RETURN(1); /* #147 */
	   } else {
	       mudstate_hot.debug_cmd = cmdsave;
               retval = check_connect(d, command, 0, NOTHING);
	       if ( chk_perm && cp )
	         cp->perm = store_perm;
	       RETURN(retval); /* #147 */
	   }
       }
    }
    /* The command was in the logged-out command table.  Perform prefix
     * and suffix processing, and invoke the command handler.
     */

    if ( Good_obj(D_PLAYER(d)) && !TogHideIdle(D_PLAYER(d)) )
       d->cold->command_count++;
    if (cp && !(cp->flag & CMD_NOxFIX)) {
	if (d->cold->output_prefix) {
	    queue_string(d, d->cold->output_prefix);
	    queue_write(d, "\r\n", 2);
	}
    }
    if ( cp && !(D_FLAGS(d) & DS_API) && ((Good_chk(D_PLAYER(d)) && !check_access(D_PLAYER(d), cp->perm, cp->perm2, 0)) || cval ||
	((cp->perm & CA_PLAYER) && !(D_FLAGS(d) & DS_CONNECTED))) ) {
	queue_string(d, "Permission denied.\r\n");
    } else if ( cp ) {
	mudstate_hot.debug_cmd = cp->name;
	switch (cp->flag & CMD_MASK) {
	case CMD_QUIT:
            if ( !(D_FLAGS(d) & DS_CONNECTED) ) {
               process_output(d);
               if ( d && (D_FLAGS(d) & DS_API) ) {
		  shutdownsock(d, R_API);
               } else {
		  shutdownsock(d, R_QUIT);
               }
		mudstate_hot.debug_cmd = cmdsave;
		if ( chk_perm && cp )
		  cp->perm = store_perm;
		RETURN(0); /* #147 */
	    } else if (Good_chk(D_PLAYER(d)) && !Fubar(D_PLAYER(d))) {
                process_output(d);
                if ( d && (D_FLAGS(d) & DS_API) ) {
		   shutdownsock(d, R_API);
                } else {
		   shutdownsock(d, R_QUIT);
                }
		mudstate_hot.debug_cmd = cmdsave;
		if ( chk_perm && cp )
		  cp->perm = store_perm;
		RETURN(0); /* #147 */
	    } else {
		notify_quiet(D_PLAYER(d), "Permission denied.");
		break;
	    }
	case CMD_LOGOUT:
            if ( !(D_FLAGS(d) & DS_CONNECTED) ) {
               process_output(d);
               if ( d && (D_FLAGS(d) & DS_API) ) {
		  shutdownsock(d, R_API);
               } else {
		  shutdownsock(d, R_LOGOUT);
               }
		break;
	    } else if (!Fubar(D_PLAYER(d))) {
                process_output(d);
                if ( d && (D_FLAGS(d) & DS_API) ) {
		   shutdownsock(d, R_API);
                } else {
		   shutdownsock(d, R_LOGOUT);
                }
		break;
	    } else {
		notify_quiet(D_PLAYER(d), "Permission denied.");
		break;
	    }
	case CMD_WHO:
	    dump_users(d, arg, CMD_WHO);
	    break;
	case CMD_DOING:
	    dump_users(d, arg, CMD_DOING);
	    break;
	case CMD_SESSION:
            if ( D_FLAGS(d) & DS_CONNECTED )
	       dump_users(d, arg, CMD_SESSION);
            else
	       notify_quiet(D_PLAYER(d), "Permission denied.");
	    break;
#ifdef LOCAL_RWHO_SERVER
	case CMD_RWHO:
	    dump_rusers(d);
	    break;
#endif
	case CMD_PREFIX:
	    set_userstring(&d->cold->output_prefix, arg);
	    break;
	case CMD_SUFFIX:
	    set_userstring(&d->cold->output_suffix, arg);
	    break;
        case CMD_PUT:
        case CMD_HEAD:
            if ( (D_FLAGS(d) & DS_CONNECTED) || !(D_FLAGS(d) & DS_API) ) {
               if ( D_FLAGS(d) & DS_CONNECTED ) {
                  notify_quiet(D_PLAYER(d), "Not supported.");
               } else {
                  queue_string(d, "Not on API port and not supported.\r\n");
                  process_output(d);
                  if ( D_FLAGS(d) & DS_API ) {
                     shutdownsock(d, R_API);
                     mudstate_hot.debug_cmd = cmdsave;
                     if ( chk_perm && cp )
                        cp->perm = store_perm;
                     RETURN(0); /* #147 */
                  }
               }
            }
            break;
        case CMD_GET:
            if ( !(D_FLAGS(d) & DS_API) || (D_FLAGS(d) & DS_CONNECTED) ) {
               if ( D_FLAGS(d) & DS_CONNECTED ) {
                  notify_quiet(D_PLAYER(d), "Permission denied.");
               } else {
                  queue_string(d, "Not on API port.\r\n");
                  process_output(d);
               }
                break;
             }
#ifndef HAS_OPENSSL
             handle_html(d, 501, (char *)"Exec: Error - SSL not compiled in RhostMUSH\r\n",
                       (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
            process_output(d);
            shutdownsock(d, R_API);
            mudstate_hot.debug_cmd = cmdsave;
            if ( chk_perm && cp )
               cp->perm = store_perm;
            RETURN(0); /* #147 */
            break;
#else
            i_encode64 = 0;
#ifdef ENABLE_LUA
            s_lua = NULL;
#endif
/* ---- this is the initialization chunk required for profiling/cpu handling */
            /* As this is not part of the 'main' command set, we must initialize all the
             * things that would normally be initialized for normal commands for this
             */
            /* For all valid commands we must initialize the timers */
            mudstate_hot.heavy_cpu_recurse = 0;
            mudstate_hot.heavy_cpu_tmark1 = time(NULL);
            mudstate_hot.chkcpu_toggle = 0;
            mudstate_hot.chkcpu_stopper = time(NULL);
            mudstate_hot.chkcpu_locktog = 0;
            mudstate_hot.stack_val = 0;
            mudstate_hot.stack_toggle = 0;
            mudstate.sidefx_currcalls = 0;
            mudstate_hot.curr_percentsubs = 0;
            mudstate_hot.tog_percentsubs = 0;
            mudstate.sidefx_toggle = 0;
            mudstate.log_maximum = 0;

            /* profiling */
            mudstate_hot.evalcount = 0;
            mudstate_hot.funccount = 0;
            mudstate.allocsin = 0;
            mudstate.allocsout = 0;
            mudstate.attribfetchcount = 0;
            itimer.it_interval.tv_sec = 0;
            itimer.it_interval.tv_usec = 0;
            itimer.it_value.tv_usec = 0;
            itimer.it_value.tv_sec = 1000;
            setitimer(ITIMER_PROF, &itimer, NULL);
/* End of the profile/cpu chunk */

            s_snarfing = alloc_lbuf("cmd_get");
            s_snarfing2 = alloc_lbuf("cmd_get2");
            s_snarfing3 = alloc_lbuf("cmd_get3");
            s_snarfing4 = alloc_lbuf("cmd_get4");
            s_snarfheader = alloc_lbuf("cmd_getheader");
            s_snarfvalue = alloc_lbuf("cmd_getvalue");
            s_buffer = alloc_lbuf("cmd_get_buff");
            s_usepassptr = s_usepass = alloc_lbuf("cmd_get_userpass");
            s_user = alloc_lbuf("cmd_get_user");
            strcpy(s_buffer, arg);
            s_strtok = strtok_r(s_buffer, "\n", &s_strtokr);
            i_enc64 = i_parse = i_snarfing = i_usepass = i_snarfing4 = 0;
            ///// NEW WEBSOCK
            int ws_upgrade = 0, ws_connection = 0, ws_host = 0, ws_version = 0, ws_key = 0;
            char *s_sockhost = alloc_lbuf("cmd_sockhost");
            char *s_sockkey = alloc_lbuf("cmd_sockkey");
            char *s_sockver = alloc_lbuf("cmd_sockver");
            ///// END NEW WEBSOCK

            s_enc64 = alloc_lbuf("cmd_get_exec64");
            while ( s_strtok ) {
               // Check if we have a header
               i_snarfheaders = sscanf(s_strtok, (char *)"%[^:]: %[^\n]", s_snarfheader, s_snarfvalue);
               if ( 2 == i_snarfheaders ) {

               ////////   NEW WEBSOCK
               ///// TODO: Improve logic here

                   if ( !stricmp(s_snarfheader, (char *)"Upgrade" ) ) {
                      if ( !stricmp(s_snarfvalue, (char *)"Websocket" ) ) {
                         ws_upgrade = 1;
                      }
                   }

                   if ( !stricmp(s_snarfheader, (char *)"Connection" ) ) {
                      /* Check for "upgrade" anywhere in the value (case-insensitive) */
                      char *cp;
                      for (cp = s_snarfvalue; *cp; cp++) {
                         if (strncasecmp(cp, "upgrade", 7) == 0) {
                            ws_connection = 1;
                            break;
                         }
                      }
                   }

                   if ( !stricmp(s_snarfheader, (char *)"Host" ) ) {
                      strcpy(s_sockhost, s_snarfvalue);
                      ws_host = 1;
                   }

                   if ( !stricmp(s_snarfheader, (char *)"Sec-WebSocket-Version" ) ) {
                      strcpy(s_sockver, s_snarfvalue);
                      if ( !stricmp(s_sockver, (char *)"13" ) ) {
                         ws_version = 1;
                      }
                   }

                   if ( !stricmp(s_snarfheader, (char *)"Sec-WebSocket-Key" ) ) {
                      strcpy(s_sockkey, s_snarfvalue);
#ifdef ENABLE_WEBSOCKETS
                      if (validate_websocket_key(s_sockkey)) {
                         ws_key = 1;
                      }
#else
                      ws_key = 1;
#endif
                   }

                ////////   END NEW WEBSOCK

                  if ( (d->cold->timeout == 1) && !stricmp(s_snarfheader, (char *)"Keep-Alive" ) ) {
                     if ( !strncasecmp(s_snarfvalue, (char *)"timeout=", 8) ) {
                        i_timeout = atoi(strchr(s_snarfvalue, '=')+1);
                        if ( i_timeout < 1 )
                           i_timeout = 1;
                        if ( i_timeout > mudconf.max_api_timeout ) 
                           i_timeout = mudconf.max_api_timeout;
                        d->cold->timeout = i_timeout;
                     }
                  }

                  if ( !i_snarfing && !stricmp(s_snarfheader, (char *)"Exec" ) ) {
                     strcpy(s_snarfing, s_snarfvalue);
                     i_snarfing = 1;
                  }

                  if ( !i_enc64 && !stricmp(s_snarfheader, (char *)"Exec64" ) ) {
                     strcpy(s_enc64, s_snarfvalue);
                     i_enc64 = i_snarfing = 1;
                  }

#ifdef ENABLE_LUA
                  if ( !s_lua && !stricmp(s_snarfheader, (char *)"X-Lua" ) ) {
                     s_lua = alloc_lbuf("cmd_lua");
                     strcpy(s_lua, s_snarfvalue);
                  }

                  if ( !stricmp(s_snarfheader, (char *)"X-Lua64" ) ) {
                     if(s_lua) {
                         free_lbuf(s_lua);
                     }
                     s_lua = alloc_lbuf("cmd_lua");
                     s_luaptr = s_lua;
                     decode_base64((const char*)s_snarfvalue, strlen(s_snarfvalue), s_lua, &s_luaptr, 0);
                  }
#endif

                  if ( !i_snarfing4 && !stricmp(s_snarfheader, (char *)"Origin" ) ) {
                     strcpy(s_snarfing4, s_snarfvalue);
                     i_snarfing4 = 1;
                  }

                  if ( !stricmp(s_snarfheader, (char *)"Parse" ) ) {
                     /* Default behavior -- set to 0 */
                     if ( stricmp( s_snarfvalue, (char *)"parse") == 0 ) {
                        i_parse = 0;
                     } else if ( stricmp( s_snarfvalue, (char *)"ansiparse") == 0 ) {
                        i_parse = 3;
                     /* Do not parse -- ergo, only percent subs */
                     } else if ( stricmp( s_snarfvalue, (char *)"noparse") == 0 ) {
                        i_parse = 1;
                     } else if ( stricmp( s_snarfvalue, (char *)"ansinoparse") == 0 ) {
                        i_parse = 4;
                     } else if ( stricmp( s_snarfvalue, (char *)"ansionly") == 0 ) {
                     /* Take the string and only process it through the ansi processor */
                        i_parse = 2;
                     /* Illegal value so just set to default */
                     } else {
                        i_parse = 0;
                     }
                  }

                  if ( !stricmp(s_snarfheader, (char *)"Encode" ) ) {
                     if ( !stricmp(s_snarfvalue, (char *)"yes" ) ) {
                        i_encode64 = 1;
                     }
                  }

                  if ( !i_usepass && !stricmp(s_snarfheader, (char *)"Authorization" ) ) {
                     if ( sscanf(s_snarfvalue, "Basic %[^\n]", s_user) == 1 ) {
                        i_usepass = strlen(s_user);
                        decode_base64((const char*)s_user, i_usepass, s_usepass, &s_usepassptr, 0);
                        i_usepass = 1;
                     }
                  }
               }
               s_strtok = strtok_r(NULL, "\n", &s_strtokr);
            }

            ///// NEW WEBSOCK
            if (ws_upgrade && ws_connection && ws_host && ws_version && ws_key) {
                free_lbuf(s_user); // Was allocated but will not be used
#ifdef ENABLE_WEBSOCKETS
                STARTLOG(LOG_ALWAYS, "NET", "WS");
                log_text("Received WebSocket opening handshake.");
                ENDLOG;
                complete_handshake(d, s_sockkey);
#else
                 handle_html(d, 501, (char *)"Return: Error - Websockets not enabled\r\n\r\n", NULL, NULL, NULL);
#endif
            } else
            ///// END NEW WEBSOCK
            if ( ((*s_usepass == '#') && isdigit(*(s_usepass+1))) && (strchr(s_usepass, ':') != NULL) ) {
               free_lbuf(s_user);
               s_user = s_usepass+1;
               s_pass = strchr(s_usepass, ':');
               *s_pass = '\0';
               s_pass++;
               thing = atoi(s_user);
               if ( !Good_chk(thing) ) {
                  handle_html(d, 404, (char *)"Exec: Error - Invalid target\r\n", 
                              (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               } else if ( !HasPriv(thing, NOTHING, POWER_API, POWER5, NOTHING) )  {
                  handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n", 
                              (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               } else {
                  atrp = atr_str3("_APIIP");
                  i_snarfing = 0;
                  if ( atrp ) {
                     s_get = atr_get(thing, atrp->number, &aowner, &aflags);
                     if ( !*s_get ) {
                        sprintf(s_get, "%s", (char *)"127.0.0.1");
                     }
                  } else {
                     s_get = alloc_lbuf("GET_fetchip");
                     sprintf(s_get, "%s", (char *)"127.0.0.1");
                  }
                   if ( !lookup(d->cold->addr, s_get, 1, &aflags) ) {
                      handle_html(d, 403, (char *)"Exec: Error - IP not allowed\r\n", 
                                  (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                      i_snarfing = 1;
                   }
                   free_lbuf(s_get);

#ifdef ENABLE_LUA
                  if ( s_lua )  {
                     if ( Totem(atoi(s_user),9) & TOTEM_API_LUA ) {
                        lua = open_lua_interpreter(thing);
                        if(!lua) {
                           handle_html(d, 500, (char *)"Exec: Error - Timeout opening interpreter\r\n",
                                       (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                           broadcast_monitor(thing, MF_CPU, "LUA INTERPRETER TIMEOUT REACHED", (char *)"LUA", NULL, thing, 0, 0, NULL);
                           goto end_lua; /* Abort, abort! */
                        }

                        free_lbuf(s_buffer);
                        s_buffer = exec_lua_script(lua, s_lua, &i_lualength);
                        if(!s_buffer) {
                           handle_html(d, 500, (char *)"Exec: Error - Timeout running script\r\n",
                                       (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                           /* We need to re-cache the buffer here */
                           s_buffer = alloc_lbuf("cmd_post_buff");
                           broadcast_monitor(thing, MF_CPU, "LUA EXECUTION TIMEOUT REACHED", (char *)"LUA API->GET", NULL, thing, 0, 0, NULL);
                           goto end_lua; /* Abort, abort! */
                        }
      
                        close_lua_interpreter(lua);

                        /* This is a one off for various handlings for LUA */
                        handle_html(d, 200, NULL, NULL, NULL, NULL);
                        if ( i_snarfing4 ) {
                           queue_string(d, unsafe_tprintf("Access-Control-Allow-Origin: %s\r\n", s_snarfing4));
                           queue_string(d, "Access-Control-Allow-Methods: POST, GET\r\n");
                           queue_string(d, "Vary: Accept-Encoding, Origin\r\n");
                        }
                        queue_string(d, "X-Lua: Ok - Executed\r\n");
                        if ( i_encode64 ) {
                           queue_string(d, "X-Lua-Warning: Base64 Encoding not supported for output\r\n");
                        }
                        queue_string(d, unsafe_tprintf("Content-Length: %i\r\n\r\n", i_lualength));
                        s_luaptr = s_buffer;
                        while(i_lualength > LBUF_SIZE - 1) {
                            i_lualength -= LBUF_SIZE - 1;
                            queue_write(d, s_luaptr, LBUF_SIZE - 1);
                            s_luaptr += LBUF_SIZE - 1;
                        }
                        if(i_lualength) {
                            queue_write(d, s_luaptr, i_lualength);
                        }
                        free(s_buffer);
                        s_buffer = alloc_lbuf("cmd_post_buff");
                     } else {
                        handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                                    (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                     }
                     end_lua: /* I'm using goto as a poor man's exception system. Fite me. --polk */
                     free_lbuf(s_lua);
                     s_lua = NULL;
                     i_snarfing = 1; /* We already handled this, move on */
                  }
#endif

                  if ( !i_snarfing ) {
                     atrp = atr_str3("_APIPASSWD");
                     if ( atrp ) {
                        s_get = atr_get(thing, atrp->number, &aowner, &aflags);
                        if ( *s_get && mush_crypt_validate(thing, s_pass, s_get, 0)) {
                           if ( i_enc64 ) {
                              s_enc64ptr = s_snarfing;
                              decode_base64((const char*)s_enc64, strlen(s_enc64), s_snarfing, &s_enc64ptr, 0);
                           } 
                           if ( *s_snarfing ) {
                              free_lbuf(s_buffer);
                              i_cputog = mudstate_hot.chkcpu_toggle;
                               sprintf(s_snarfing2, "[%s]->%.*s", d->cold->addr, (LBUF_SIZE-30), s_snarfing);
                              switch(i_parse) {
                                 case 1: /* Do not parse -- percent args only */
                                 case 4: /* Do not parse -- percent args only -- show ansi */
                                         s_buffer = cpuexec(thing, thing, thing,
                                                            EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_snarfing, (char **)NULL, 0, (char **)NULL, 0);
#ifdef ZENTY_ANSI
                                         if ( i_parse == 4 ) {
                                            strcpy(s_snarfing, s_buffer);
                                            s_ansi1p = s_ansi1 = alloc_lbuf("get_parse_ansibuf1");
                                            s_ansi2p = s_ansi2 = alloc_lbuf("get_parse_ansibuf2");
                                            s_ansi3p = s_ansi3 = alloc_lbuf("get_parse_ansibuf3");
                                            parse_ansi(s_snarfing, s_ansi1, &s_ansi1p, s_ansi2, &s_ansi2p, s_ansi3, &s_ansi3p);
                                            strcpy(s_buffer, s_ansi3); 
                                            free_lbuf(s_ansi1);
                                            free_lbuf(s_ansi2);
                                            free_lbuf(s_ansi3);
                                         }
#endif
                                         break;
                                 case 2: /* Just return the string as an ansi handler */
#ifdef ZENTY_ANSI
                                         s_buffer = alloc_lbuf("get_parse_ansi");
                                         s_ansi1p = s_ansi1 = alloc_lbuf("get_parse_ansibuf1");
                                         s_ansi2p = s_ansi2 = alloc_lbuf("get_parse_ansibuf2");
                                         s_ansi3p = s_ansi3 = alloc_lbuf("get_parse_ansibuf3");
                                         parse_ansi(s_snarfing, s_ansi1, &s_ansi1p, s_ansi2, &s_ansi2p, s_ansi3, &s_ansi3p);
                                         strcpy(s_buffer, s_ansi3); 
                                         free_lbuf(s_ansi1);
                                         free_lbuf(s_ansi2);
                                         free_lbuf(s_ansi3);
#else
                                         strcpy(s_buffer, s_snarfing);
#endif
                                         break;
                                 case 3:
                                 default: /* Default case */
                                         s_buffer = cpuexec(thing, thing, thing,
                                                            EV_FCHECK | EV_EVAL, s_snarfing, (char **)NULL, 0, (char **)NULL, 0);
#ifdef ZENTY_ANSI
                                         if ( i_parse == 3 ) {
                                            strcpy(s_snarfing, s_buffer);
                                            s_ansi1p = s_ansi1 = alloc_lbuf("get_parse_ansibuf1");
                                            s_ansi2p = s_ansi2 = alloc_lbuf("get_parse_ansibuf2");
                                            s_ansi3p = s_ansi3 = alloc_lbuf("get_parse_ansibuf3");
                                            parse_ansi(s_snarfing, s_ansi1, &s_ansi1p, s_ansi2, &s_ansi2p, s_ansi3, &s_ansi3p);
                                            strcpy(s_buffer, s_ansi3); 
                                            free_lbuf(s_ansi1);
                                            free_lbuf(s_ansi2);
                                            free_lbuf(s_ansi3);
                                         }
#endif
                                         break;
                              }
                              if ( mudstate_hot.chkcpu_toggle && !i_cputog ) {
                                 /* Ok, API hit the cpu -- remove the API @power and sort out later */
                                 sprintf(s_snarfing, "%s", (char *)"API");
                                 power_set(thing, GOD, s_snarfing, POWER_LEVEL_OFF);
                                 broadcast_monitor(thing, MF_CPU | MF_API, "CPU RUNAWAY API GET", s_snarfing2,
                                                   NULL, thing, 0, 0, NULL);
                                 STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                                    log_name_and_loc(thing);
                                    sprintf(s_snarfing, " CPU API GET overload on attribute %.*s", 
                                            (LBUF_SIZE - 100), s_snarfing2);
                                    log_text(s_snarfing);
                                 ENDLOG
                              }
                              mudstate_hot.chkcpu_toggle = i_cputog;

                              /* This is a one off handler */
                              handle_html(d, 200, NULL, NULL, NULL, NULL);
                              if ( i_snarfing4 ) {
                                 queue_string(d, unsafe_tprintf("Access-Control-Allow-Origin: %s\r\n", s_snarfing4));
                                 queue_string(d, "Access-Control-Allow-Methods: POST, GET\r\n");
                                 queue_string(d, "Vary: Accept-Encoding, Origin\r\n");
                              }
                              queue_string(d, "Exec: Ok - Executed\r\n");
                              if ( i_encode64 ) {
                                 free_lbuf(s_snarfing2);
                                 s_snarfing2 = s_snarfing;
                                 i_encode64 = strlen(s_buffer);
                                 encode_base64((const char*)s_buffer, i_encode64, s_snarfing, &s_snarfing2);
                                 queue_string(d, unsafe_tprintf("Return: %.*s\r\n\r\n", (LBUF_SIZE - 14), s_snarfing));
                                 s_snarfing2 = alloc_lbuf("tmp_get_buffer");
                              } else {
                                 queue_string(d, unsafe_tprintf("Return: %.*s\r\n\r\n", (LBUF_SIZE - 14), s_buffer));
                              }
                           } else {
                              handle_html(d, 400, (char *)"Exec: Error - Empty String\r\n",
                                          (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                           }
                        } else {
                           handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                                       (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                        }
                        free_lbuf(s_get);
                     } else {
                        handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                                    (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                     }
                  }
               }
            } else {
               handle_html(d, 403, (char *)"Exec: Error - Malformed User or Password\r\n",
                           (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               free_lbuf(s_user);
            }

            free_lbuf(s_enc64);
            free_lbuf(s_snarfing);
            free_lbuf(s_snarfing2);
            free_lbuf(s_snarfing3);
            free_lbuf(s_snarfing4);
            free_lbuf(s_snarfheader);
            free_lbuf(s_snarfvalue);
            free_lbuf(s_buffer);
            free_lbuf(s_usepass);
            ///// NEW WEBSOCK
            free_lbuf(s_sockhost);
            free_lbuf(s_sockkey);
            free_lbuf(s_sockver);
            ///// END NEW WEBSOCK
            process_output(d);
            if ((D_FLAGS(d) & DS_API) && (d->cold->timeout == 1) )
                shutdownsock(d, R_API);
            mudstate_hot.debug_cmd = cmdsave;
            if ( chk_perm && cp )
               cp->perm = store_perm;
            RETURN(0); /* #147 */
            break;
#endif

        case CMD_POST:
            if ( !(D_FLAGS(d) & DS_API) || (D_FLAGS(d) & DS_CONNECTED) ) {
               if ( D_FLAGS(d) & DS_CONNECTED ) {
                  notify_quiet(D_PLAYER(d), "Permission denied.");
               } else {
                  queue_string(d, "Not on API port.\r\n");
                  process_output(d);
               }
                break;
             }
#ifndef HAS_OPENSSL
             handle_html(d, 501, (char *)"Exec: Error - SSL not compiled in RhostMUSH\r\n",
                        (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
            process_output(d);
            shutdownsock(d, R_API);
            mudstate_hot.debug_cmd = cmdsave;
            if ( chk_perm && cp )
               cp->perm = store_perm;
            RETURN(0); /* #147 */
            break;
#else
/* ---- this is the initialization chunk required for profiling/cpu handling */
            /* As this is not part of the 'main' command set, we must initialize all the
             * things that would normally be initialized for normal commands for this
             */
            /* For all valid commands we must initialize the timers */
            mudstate_hot.heavy_cpu_recurse = 0;
            mudstate_hot.heavy_cpu_tmark1 = time(NULL);
            mudstate_hot.chkcpu_toggle = 0;
            mudstate_hot.chkcpu_stopper = time(NULL);
            mudstate_hot.chkcpu_locktog = 0;
            mudstate_hot.stack_val = 0;
            mudstate_hot.stack_toggle = 0;
            mudstate.sidefx_currcalls = 0;
            mudstate_hot.curr_percentsubs = 0;
            mudstate_hot.tog_percentsubs = 0;
            mudstate.sidefx_toggle = 0;
            mudstate.log_maximum = 0;

            /* profiling */
            mudstate_hot.evalcount = 0;
            mudstate_hot.funccount = 0;
            mudstate.allocsin = 0;
            mudstate.allocsout = 0;
            mudstate.attribfetchcount = 0;
            itimer.it_interval.tv_sec = 0;
            itimer.it_interval.tv_usec = 0;
            itimer.it_value.tv_usec = 0;
            itimer.it_value.tv_sec = 1000;
            setitimer(ITIMER_PROF, &itimer, NULL);
/* End of the profile/cpu chunk */

            s_snarfing = alloc_lbuf("cmd_post");
            s_snarfing4 = alloc_lbuf("cmd_post2");
            s_snarfheader = alloc_lbuf("cmd_post_snarfheader");
            s_snarfvalue = alloc_lbuf("cmd_post_snarfvalue");
            s_buffer = alloc_lbuf("cmd_post_buff");
            s_usepassptr = s_usepass = alloc_lbuf("cmd_post_userpass");
            s_user = alloc_lbuf("cmd_post_user");
            strcpy(s_buffer, arg);
            s_strtok = strtok_r(s_buffer, "\n", &s_strtokr);
            i_enc64 = i_snarfing = i_usepass = i_snarfing4 = 0;
            i_time = 0.0;
            s_enc64 = alloc_lbuf("cmd_post_exec64");
            while ( s_strtok ) {
               // Check if we have a header
               i_snarfheaders = sscanf(s_strtok, (char *)"%[^:]: %[^\n]", s_snarfheader, s_snarfvalue);
               if ( 2 == i_snarfheaders ) {
                  if ( (d->cold->timeout == 1) && !stricmp(s_snarfheader, (char *)"Keep-Alive" ) ) {
                     if ( !strncasecmp(s_snarfvalue, (char *)"timeout=", 8) ) {
                        i_timeout = atoi(strchr(s_snarfvalue, '=')+1);
                        if ( i_timeout < 1 )
                           i_timeout = 1;
                        if ( i_timeout > mudconf.max_api_timeout ) 
                           i_timeout = mudconf.max_api_timeout;
                        d->cold->timeout = i_timeout;
                     }
                  }

                  if ( !i_snarfing && !stricmp(s_snarfheader, (char *)"Exec" ) ) {
                     strcpy(s_snarfing, s_snarfvalue);
                     i_snarfing = 1;
                  }

                  if ( !i_enc64 && !stricmp(s_snarfheader, (char *)"Exec64" ) ) {
                     strcpy(s_enc64, s_snarfvalue);
                     i_enc64 = i_snarfing = 1;
                  }

                  if ( !i_snarfing4 && !stricmp(s_snarfheader, (char *)"Origin" ) ) {
                     strcpy(s_snarfing4, s_snarfvalue);
                     i_snarfing4 = 1;
                  }

                  if ( !stricmp(s_snarfheader, (char *)"Time" ) ) {
                     if ( 1 == sscanf(s_snarfvalue, "%lf", &i_time) ) {
                        if ( i_time < 0 )
                           i_time = 0;
                        if ( i_time > 2000000000 )
                           i_time = 2000000000;
                     }
                  }

                  if ( !i_usepass && !stricmp(s_snarfheader, (char *)"Authorization" ) ) {
                     if ( sscanf(s_snarfvalue, "Basic %[^\n]", s_user) == 1 ) {
                        i_usepass = strlen(s_user);
                        decode_base64((const char*)s_user, i_usepass, s_usepass, &s_usepassptr, 0);
                        i_usepass = 1;
                     }
                  }
               }
               s_strtok = strtok_r(NULL, "\n", &s_strtokr);
            }
            if ( ((*s_usepass == '#') && isdigit(*(s_usepass+1))) && (strchr(s_usepass, ':') != NULL) ) {
               free_lbuf(s_user);
               s_user = s_usepass+1;
               s_pass = strchr(s_usepass, ':');
               *s_pass = '\0';
               s_pass++;
               thing = atoi(s_user);
               if ( !Good_chk(thing) ) {
                  handle_html(d, 404, (char *)"Exec: Error - Invalid Target\r\n",
                              (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               } else if ( !HasPriv(thing, NOTHING, POWER_API, POWER5, NOTHING) )  {
                  handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                              (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               } else {
                  atrp = atr_str3("_APIIP");
                  i_snarfing = 0;
                  if ( atrp ) {
                     s_get = atr_get(thing, atrp->number, &aowner, &aflags);
                     if ( !*s_get ) {
                        sprintf(s_get, "%s", (char *)"127.0.0.1");
                     }
                  } else {
                     s_get = alloc_lbuf("GET_fetchip");
                     sprintf(s_get, "%s", (char *)"127.0.0.1");
                  }
                   if ( !lookup(d->cold->addr, s_get, 1, &aflags) ) {
                      handle_html(d, 403, (char *)"Exec: Error - IP not allowed\r\n",
                                  (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                      i_snarfing = 1;
                   }
                   free_lbuf(s_get);
                   if ( !i_snarfing ) {
                      atrp = atr_str3("_APIPASSWD");
                     if ( atrp ) {
                        s_get = atr_get(thing, atrp->number, &aowner, &aflags);
                        if ( *s_get && mush_crypt_validate(thing, s_pass, s_get, 0)) {
                           if ( i_enc64 ) {
                              s_enc64ptr = s_snarfing;
                              decode_base64((const char*)s_enc64, strlen(s_enc64), s_snarfing, &s_enc64ptr, 0);
                           } 
                           if ( *s_snarfing ) {
	                      wait_que(thing, thing, i_time, NOTHING, s_snarfing, (char **)NULL, 0, NULL, NULL);

                              /* One off handler */
                              handle_html(d, 200, NULL, NULL, NULL, NULL);
                              if ( i_snarfing4 ) {
                                 queue_string(d, unsafe_tprintf("Access-Control-Allow-Origin: %s\r\n", s_snarfing4));
                                 queue_string(d, "Access-Control-Allow-Methods: POST, GET\r\n");
                                 queue_string(d, "Vary: Accept-Encoding, Origin\r\n");
                              }
                              queue_string(d, "Exec: Ok - Queued\r\n\r\n");
                           } else {
                              handle_html(d, 400, (char *)"Exec: Error - Empty String\r\n",
                                          (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                           }
                        } else {
                           handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                                       (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                        }
                        free_lbuf(s_get);
                     } else {
                        handle_html(d, 403, (char *)"Exec: Error - Permission Denied\r\n",
                                    (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
                     }
                  }
               }
            } else {
               handle_html(d, 403, (char *)"Exec: Error - Malformed User or Password\r\n",
                           (char *)"Return: <NULL>\r\n\r\n", NULL, NULL);
               free_lbuf(s_user);
            }

            free_lbuf(s_enc64);
            free_lbuf(s_snarfing);
            free_lbuf(s_snarfing4);
            free_lbuf(s_snarfheader);
            free_lbuf(s_snarfvalue);
            free_lbuf(s_buffer);
            free_lbuf(s_usepass);
            process_output(d);
            if ((D_FLAGS(d) & DS_API) && d->cold && (d->cold->timeout == 1) )
               shutdownsock(d, R_API);
            mudstate_hot.debug_cmd = cmdsave;
            if ( chk_perm && cp )
               cp->perm = store_perm;
            RETURN(0); /* #147 */
            break;
#endif

        case CMD_INFO:
          gotone = 0;
          DESC_ITER_CONN(d2) {
             if (Cloak(D_PLAYER(d2)))
	        continue;
             if (!(mudconf.who_unfindable) && Dark(D_PLAYER(d2)) && 
                 !(mudconf.player_dark) && mudconf.allow_whodark )
                continue;
             if (NoWho(D_PLAYER(d2)))
                continue;
             gotone += 1;
          }
          time_str = ctime(&mudstate.start_time);
          time_str[strlen(time_str) - 1] = '\0';
          queue_string(d, unsafe_tprintf("### Begin INFO %s\r\n", INFO_VERSION));
          queue_string(d, unsafe_tprintf("Name: %s\r\n", mudconf.mud_name));
          queue_string(d, unsafe_tprintf("Uptime: %s\r\n", time_str));
          queue_string(d, unsafe_tprintf("Connected: %d\r\n", gotone));
          queue_string(d, unsafe_tprintf("Size: %d\r\n", mudstate_hot.db_top));
          queue_string(d, unsafe_tprintf("Version: %s\r\n", mudstate.short_ver));
          queue_string(d, "### End INFO");
          queue_write(d, "\r\n", 2); 
          process_output(d);
          break;

	default:
            if ( D_FLAGS(d) & DS_API ) {
               process_output(d);
               shutdownsock(d, R_API);
               mudstate_hot.debug_cmd = cmdsave;
               if ( chk_perm && cp )
                  cp->perm = store_perm;
               RETURN(0); /* #147 */
            }
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
    /* Any API foo should just drop here as we have nothing for them to do */
    if ( D_FLAGS(d) & DS_API ) {
       handle_html(d, 400, (char *)"Exec: Error - Unrecognized Input\r\n", NULL, NULL,  NULL);
       if ( cp ) {
          queue_string(d, unsafe_tprintf("Return: Bad command -> %s\r\n\r\n", cp->name));
       } else { 
          queue_string(d, "Return: <NULL>\r\n\r\n");
       }
       process_output(d);
       shutdownsock(d, R_API);
       mudstate_hot.debug_cmd = cmdsave;
       if ( chk_perm && cp )
          cp->perm = store_perm;
       RETURN(0); /* #147 */
    }
    if (cp && !(cp->flag & CMD_NOxFIX) && d->cold) {
	if (d->cold->output_suffix) {
	    queue_string(d, d->cold->output_suffix);
	    queue_write(d, "\r\n", 2);
	}
    }
    mudstate_hot.debug_cmd = cmdsave;
    if ( chk_perm && cp )
      cp->perm = store_perm;
    process_output(d);
    RETURN(1); /* #147 */
}

void 
NDECL(process_commands)
{
    int nprocessed, aflags2, i;
    DESC *d;
    CBLK *t;
    char *cmdsave, *progatr;
    dbref aowner2;
#ifdef ZENTY_ANSI
    char *s_buff, *s_buff2, *s_buff3, *s_buffptr, *s_buff2ptr, *s_buff3ptr;
#endif

    DPUSH; /* #148 */

    cmdsave = mudstate_hot.debug_cmd;
    mudstate_hot.debug_cmd = (char *) "process_commands";

    do {
	nprocessed = 0;
	DESC_SAFEITER_ALL(d) {
	    if (!d->cold) continue;
	    if ((D_FLAGS(d) & DS_AUTH_IN_PROGRESS) == 0) {
		if ( ((D_QUOTA(d) > 0) || (D_FLAGS(d) & DS_API)) && (t = D_INPUT_HEAD(d))) {
                    if ( !(D_FLAGS(d) & DS_API) ) {
		       D_QUOTA(d)--;
                    }
		    nprocessed++;
		    D_INPUT_HEAD(d) = (CBLK *) t->hdr.nxt;
		    if (!D_INPUT_HEAD(d))
			D_INPUT_TAIL(d) = NULL;
		    D_INPUT_SIZE(d) -= (strlen(t->cmd) + 1);
		    if (D_FLAGS(d) & DS_HAS_DOOR && t->cmd[0] != '!')
			door_raw_input(d, t->cmd); 
		    else {
                        for (i = 0; i < (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); i++) {
                           *mudstate_hot.global_regs[i] = '\0';
                           *mudstate_hot.global_regsname[i] = '\0';
#ifndef NO_GLOBAL_REGBACKUP
                           *mudstate_hot.global_regs_backup[i] = '\0';
#endif
                        }
                        mudstate.global_regs_wipe = 0;
			do_command(d, (D_FLAGS(d) & DS_HAS_DOOR) ? &t->cmd[1] : t->cmd);
                        if ( InProgram(D_PLAYER(d)) ) {
                           progatr = atr_get(D_PLAYER(d), A_PROGPROMPTBUF, &aowner2, &aflags2);
                           if ( progatr && *progatr ) {
                              if ( strcmp(progatr, "NULL") != 0 ) {
#ifdef ZENTY_ANSI
                                 s_buffptr = s_buff = alloc_lbuf("parse_ansi_prompt");
                                 s_buff2ptr = s_buff2 = alloc_lbuf("parse_ansi_prompt2");
                                 s_buff3ptr = s_buff3 = alloc_lbuf("parse_ansi_prompt3");
                                 parse_ansi((char *) progatr, s_buff, &s_buffptr, s_buff2, &s_buff2ptr, s_buff3, &s_buff3ptr);
                                 queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, s_buff, ANSI_NORMAL));
                                 free_lbuf(s_buff);
                                 free_lbuf(s_buff2);
                                 free_lbuf(s_buff3);
#else
                                 queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
#endif
                              }
                           } else {
                              queue_string(d, unsafe_tprintf("%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
                           }
                           free_lbuf(progatr);
                        }
                        mudstate.shell_program = 0;
                    }
		    free_lbuf(t);
		}
	    }
	    if (d->cold && (t = d->cold->door_input_head)) {
		nprocessed++;
		d->cold->door_input_head = (CBLK *) t->hdr.nxt;
		if (!d->cold->door_input_head)
		    d->cold->door_input_tail = NULL;
		door_raw_output(d, t->cmd);
		free_lbuf(t);
	    }
	}
    } while (nprocessed > 0);
    mudstate_hot.debug_cmd = cmdsave;
    VOIDRETURN; /* #148 */
}

/* Types are:
 * 0 - Blacklist
 * 1 - NoDNS
 * 2 - Register
 * 3 - NoGuest
 */
int
blacklist_check(struct in_addr host, int i_type)
{
   int i_return;
   BLACKLIST *b_host;

   DPUSH;
   i_return=0;

   if ( ((i_type == 0) && (mudstate.blacklist_cnt < 1)) ||
        ((i_type == 1) && (mudstate.blacklist_nodns_cnt < 1)) ||
        ((i_type == 2) && (mudstate.blacklist_reg_cnt < 1)) ||
        ((i_type == 3) && (mudstate.blacklist_nogst_cnt < 1)) ) {
      RETURN(i_return);
   }
   b_host = NULL;
   switch ( i_type ) {
      case 0: /* Blacklist */
         b_host = mudstate.bl_list;
         break;
      case 1: /* NoDNS */
         b_host = mudstate.nd_list;
         break;
      case 2: /* Register */
         b_host = mudstate.rg_list;
         break;
      case 3: /* NoGuest */
         b_host = mudstate.ng_list;
         break;
   }

   while ( b_host ) {
      if ( (host.s_addr & b_host->mask_addr.s_addr) == b_host->site_addr.s_addr ) {
         i_return=1;
         break;
      }
      b_host = b_host->next;
   }
   RETURN(i_return);
}

int
blacklist_check_str(const char *addr_str, int addr_family, int i_type)
{
   int i_return;
   BLACKLIST *b_host;
   struct in_addr addr4, site4, mask4;
   struct in6_addr addr6, site6, mask6;
   int effective_family;

   DPUSH;
   i_return = 0;

   if ( ((i_type == 0) && (mudstate.blacklist_cnt < 1)) ||
        ((i_type == 1) && (mudstate.blacklist_nodns_cnt < 1)) ||
        ((i_type == 2) && (mudstate.blacklist_reg_cnt < 1)) ||
        ((i_type == 3) && (mudstate.blacklist_nogst_cnt < 1)) ) {
      RETURN(i_return);
   }
   b_host = NULL;
   switch ( i_type ) {
      case 0: /* Blacklist */
         b_host = mudstate.bl_list;
         break;
      case 1: /* NoDNS */
         b_host = mudstate.nd_list;
         break;
      case 2: /* Register */
         b_host = mudstate.rg_list;
         break;
      case 3: /* NoGuest */
         b_host = mudstate.ng_list;
         break;
   }

   if (addr_family == AF_INET6) {
      if (inet_pton(AF_INET6, addr_str, &addr6) != 1)
         RETURN(0);
      effective_family = AF_INET6;
   } else if (addr_family == AF_INET) {
      addr4.s_addr = inet_addr(addr_str);
      if (addr4.s_addr == (in_addr_t)-1)
         RETURN(0);
      effective_family = AF_INET;
   } else {
      /* addr_family == 0: legacy reboot entry, try both families */
      addr4.s_addr = inet_addr(addr_str);
      if (addr4.s_addr != (in_addr_t)-1) {
         effective_family = AF_INET;
      } else if (inet_pton(AF_INET6, addr_str, &addr6) == 1) {
         effective_family = AF_INET6;
      } else {
         RETURN(0);
      }
   }

    while ( b_host ) {
       if (b_host->addr_family != effective_family && b_host->addr_family != 0) {
          b_host = b_host->next;
          continue;
       }

      if (effective_family == AF_INET6) {
         if (inet_pton(AF_INET6, b_host->site_addr_str, &site6) != 1) {
            b_host = b_host->next;
            continue;
         }
         if (strchr(b_host->mask_addr_str, '/') != NULL) {
            int prefix = atoi(b_host->mask_addr_str + 1);
            memset(&mask6, 0, sizeof(mask6));
            if (prefix > 0) {
               int full_bytes = prefix / 8;
               int remaining_bits = prefix % 8;
               memset(&mask6, 0xFF, full_bytes);
               if (remaining_bits > 0)
                  mask6.s6_addr[full_bytes] = (unsigned char)(0xFF << (8 - remaining_bits));
            }
         } else {
            if (inet_pton(AF_INET6, b_host->mask_addr_str, &mask6) != 1) {
               b_host = b_host->next;
               continue;
            }
         }
         {
            int i;
            int match = 1;
            for (i = 0; i < 16; i++) {
               if ((addr6.s6_addr[i] & mask6.s6_addr[i]) != site6.s6_addr[i]) {
                  match = 0;
                  break;
               }
            }
            if (match) {
               i_return = 1;
               break;
            }
         }
      } else {
         site4.s_addr = b_host->site_addr.s_addr;
         mask4.s_addr = b_host->mask_addr.s_addr;
         if ( (addr4.s_addr & mask4.s_addr) == site4.s_addr ) {
            i_return = 1;
            break;
         }
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

int
site_check_str(const char *addr_str, int addr_family, SITE *site_list, int stop, int key, int flag)
{
   SITE *this;
   int rflag, rkey, i_found;
   struct in_addr addr4, site4, mask4;
   struct in6_addr addr6, site6, mask6;
   int effective_family;

   DPUSH; /* #149 */
   rflag = i_found = 0;
   rkey = 2000000000;

   if (addr_family == AF_INET6) {
      if (inet_pton(AF_INET6, addr_str, &addr6) != 1)
         RETURN(0);
      effective_family = AF_INET6;
   } else if (addr_family == AF_INET) {
      addr4.s_addr = inet_addr(addr_str);
      if (addr4.s_addr == (in_addr_t)-1)
         RETURN(0);
      effective_family = AF_INET;
   } else {
      /* addr_family == 0: legacy reboot entry, try both families */
      addr4.s_addr = inet_addr(addr_str);
      if (addr4.s_addr != (in_addr_t)-1) {
         effective_family = AF_INET;
      } else if (inet_pton(AF_INET6, addr_str, &addr6) == 1) {
         effective_family = AF_INET6;
      } else {
         RETURN(0);
      }
   }

    for (this = site_list; this; this = this->next) {
       if (this->addr_family != effective_family && this->addr_family != 0)
          continue;

      if (effective_family == AF_INET6) {
         if (inet_pton(AF_INET6, this->address_str, &site6) != 1)
            continue;
         if (strchr(this->mask_str, '/') != NULL) {
            int prefix = atoi(this->mask_str + 1);
            memset(&mask6, 0, sizeof(mask6));
            if (prefix > 0) {
               int full_bytes = prefix / 8;
               int remaining_bits = prefix % 8;
               memset(&mask6, 0xFF, full_bytes);
               if (remaining_bits > 0)
                  mask6.s6_addr[full_bytes] = (unsigned char)(0xFF << (8 - remaining_bits));
            }
         } else {
            if (inet_pton(AF_INET6, this->mask_str, &mask6) != 1)
               continue;
         }
         {
            int i;
            int match = 1;
            for (i = 0; i < 16; i++) {
               if ((addr6.s6_addr[i] & mask6.s6_addr[i]) != site6.s6_addr[i]) {
                  match = 0;
                  break;
               }
            }
            if (!match)
               continue;
         }
      } else {
         site4.s_addr = this->address.s_addr;
         mask4.s_addr = this->mask.s_addr;
         if ((addr4.s_addr & mask4.s_addr) != site4.s_addr)
            continue;
      }

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
	if (flag == H_SUSPECT)
	    str = "Suspected";
	else if (flag == H_PASSPROXY)
	    str = "Proxy Bypass";
	else
	    str = "Trusted";
	break;
    case S_ACCESS:
	switch (flag) {
        case H_HARDCONN:
            str = "Hard Connect";
            break;
        case H_PASSAPI:
            str = "API Bypass";
            break;
        case H_FORBIDAPI:
            if ( key )
               str = "Forbid API (AutoSite)";
            else
               str = "Forbid API";
            break;
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
	case H_PERMIT:
	    str = "Unrestricted";
	    break;
	default:
	    str = "Strange";
	}
	break;
    case S_SPECIAL:
        switch( flag ) {
        case H_NOAUTH:
            if ( key ) 
               str = "NoAUTH (AutoSite)";
            else
               str = "NoAUTH";
            break;
        case H_NODNS:
            if ( key )
               str = "NoDNS (AutoSite)";
            else
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
    notify(player, "Address                                      Mask                                     Max-Conns      Status");
    str2 = alloc_sbuf("list_sites");
    for (this = site_list; this; this = this->next) {
	str = (char *) stat_string(stat_type, this->flag, this->key);
        if ( (this->flag & H_REGISTRATION) ||
             (this->flag & H_FORBIDDEN) ||
             (this->flag & H_FORBIDAPI) ||
             (this->flag & H_PASSAPI) ||
             (this->flag & H_HARDCONN) ||
             (this->flag & H_NOGUEST) ) {
           if ( this->maxcon == -1 ) 
              strcpy(str2, (char *)"Restricted");
           else
              sprintf(str2, "%d cons", this->maxcon);
        } else {
              strcpy(str2, (char *)"N/A (Ignored)");
        }
	if (this->addr_family == AF_INET6) {
	   sprintf(buff, "%-44s %-44s %-14s %s",
		   this->address_str, this->mask_str, str2, str);
	} else {
	   strcpy(buff1, inet_ntoa(this->mask));
	   sprintf(buff, "%-44s %-44s %-14s %s",
		   inet_ntoa(this->address), buff1, str2, str);
	}
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
    char *tmp_word, tbuff[LBUF_SIZE], sbuff[SBUF_SIZE], *tpr_buff, *tprp_buff, *tmp_wordptr, *tstrtokr;
    int i_maxcons, i_found;

    memset(tbuff, '\0', sizeof(tbuff));
    memset(sbuff, '\0', sizeof(sbuff));
    strncpy(tbuff, hostchrtype, (LBUF_SIZE > strlen(hostchrtype) ? strlen(hostchrtype) : LBUF_SIZE - 1));
    if ( hostchrtype ) {
       tmp_word = strtok_r(tbuff, " ", &tstrtokr);
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
          tmp_word = strtok_r( NULL, " ", &tstrtokr);
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
    if ( (mudstate.blacklist_cnt > 0) || (mudstate.blacklist_nodns_cnt > 0) ||
         (mudstate.blacklist_nogst_cnt > 0) || (mudstate.blacklist_reg_cnt > 0) ) {
       sprintf(s_buff, "Blacklist: blacklist %d, NoDNS %d, Reg %d, NoGuest %d",
               mudstate.blacklist_cnt, mudstate.blacklist_nodns_cnt, 
               mudstate.blacklist_reg_cnt, mudstate.blacklist_nogst_cnt);
       notify(player, s_buff);
    } else {
       notify(player, "Blacklist: All blacklists are currently not populated.");
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
    list_hosts(player, mudconf.forbidapi_host, "Forbid API");
    list_hosts(player, mudconf.suspect_host, "Suspected");
    list_hosts(player, mudconf.register_host, "Registration");
    list_hosts(player, mudconf.noguest_host, "Noguest");
    list_hosts(player, mudconf.autoreg_host, "NoAutoReg");
    list_hosts(player, mudconf.validate_host, "InvalidAReg");
    list_hosts(player, mudconf.goodmail_host, "ValidAReg");
    list_hosts(player, mudconf.nobroadcast_host, "NoMonitor");
    list_hosts(player, mudconf.passproxy_host, "Proxy Bypass");
    list_hosts(player, mudconf.passapi_host, "API Bypass");
    list_hosts(player, mudconf.hardconn_host, "Hard Connect");
    list_hosts(player, mudconf.permit_host, "Allowed");

    VOIDRETURN; /* #152 */
}

/* ---------------------------------------------------------------------------
 * make_ulist: Make a list of connected user numbers for the LWHO function.
 */

void 
make_ulist(dbref player, char *buff, char **bufcx, int i_type, dbref victim, int i_objid)
{
    DESC *d;
    int gotone = 0, i_port;
    char *tpr_buff, *tprp_buff, *s_array[2], *tbuf, *array_buff, *array_buffptr;
    dbref target;

    DPUSH; /* #153 */

    if ( Good_obj(victim) )
      target = victim;
    else
      target = player;

    if (!Controls(player, target))
      target = player;

    tprp_buff = tpr_buff = alloc_lbuf("make_ulist_tprintf");
    if ( i_objid ) {
       tbuf = alloc_sbuf("exec.invoker");
       array_buffptr = array_buff = alloc_lbuf("buffer_for_objid");
       s_array[1] = NULL;
    }
    /* Collect phase: gather matching player slots */
    {
        int slots[MAX_DESCRIPTORS], nslots = 0;
        DESC_SAFEITER_CONN(d) {
            if (!Wizard(target) && Cloak(D_PLAYER(d)))
                continue;
            if (!Admin(target) && !(mudconf.who_unfindable) && Dark(D_PLAYER(d)) && 
                !(mudconf.player_dark) && mudconf.allow_whodark )
                continue;
            if (Immortal(D_PLAYER(d)) && Cloak(D_PLAYER(d)) && SCloak(D_PLAYER(d)) && !Immortal(target))
                continue;
            if (!Admin(target) && Unfindable(D_PLAYER(d)) && mudconf.who_unfindable
                    && !HasPriv(target,D_PLAYER(d),POWER_WHO_UNFIND,POWER4,NOTHING))
                continue;
            if (NoWho(D_PLAYER(d)) && (D_PLAYER(d) != target) &&
                !(Wizard(target) || HasPriv(target, D_PLAYER(d), POWER_WIZ_WHO, POWER3, NOTHING)))
                continue;
            if (nslots < MAX_DESCRIPTORS) {
                slots[nslots] = d->slot_index;
                nslots++;
            }
        }
        /* Sort by connected_at descending (newest first) */
        for (int _i = 0; _i < nslots - 1; _i++) {
            for (int _j = _i + 1; _j < nslots; _j++) {
                time_t ti = desc_cold[slots[_i]].connected_at;
                time_t tj = desc_cold[slots[_j]].connected_at;
                if (ti < tj) {
                    int _t = slots[_i]; slots[_i] = slots[_j]; slots[_j] = _t;
                }
            }
        }
        /* Output phase: iterate sorted entries */
        for (int _k = 0; _k < nslots; _k++) {
            d = &desc_slots[slots[_k]];
            if (gotone)
                safe_chr(' ', buff, bufcx);
            if ( i_type == 2 ) {
               if ( (target == D_PLAYER(d)) || Wizard(player) ) 
                  i_port = D_DESCRIPTOR(d);
               else
                  i_port = -1;
               tprp_buff = tpr_buff;
               safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d", i_port), buff, bufcx);
            } else if ( i_type == 1 ) {
               if ( (target == D_PLAYER(d)) || Wizard(player) ) 
                  i_port = D_DESCRIPTOR(d);
               else
                  i_port = -1;
               tprp_buff = tpr_buff;
               if ( i_objid ) {
                  sprintf(tbuf, "#%d", D_PLAYER(d));
                  s_array[0] = tbuf;
                  array_buffptr = array_buff;
                  fun_objid(array_buff, &array_buffptr, player, D_PLAYER(d), D_PLAYER(d), s_array, 1, (char **)NULL, 0);
                  safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s|%d", array_buff, i_port), buff, bufcx);
               } else {
                  safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d:%d", D_PLAYER(d), i_port), buff, bufcx);
               }
            } else {
               tprp_buff = tpr_buff;
               if ( i_objid ) {
                  sprintf(tbuf, "#%d", D_PLAYER(d));
                  array_buffptr = array_buff;
                  s_array[0] = tbuf;
                  fun_objid(array_buff, &array_buffptr, player, D_PLAYER(d), D_PLAYER(d), s_array, 1, (char **)NULL, 0);
                  safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s", array_buff), buff, bufcx);
               } else {
                  safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d", D_PLAYER(d)), buff, bufcx);
               }
            }
            gotone = 1;
        }
    }
    if ( i_objid ) {
       free_sbuf(tbuf);
       free_lbuf(array_buff);
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
	if (Good_obj(player) && !Wizard(player) && Hidden(D_PLAYER(d)))
	    continue;
	if (!string_prefix(Name(D_PLAYER(d)), name))
	    continue;
	if ((found != NOTHING) && (found != D_PLAYER(d))) {
	    RETURN(NOTHING); /* #154 */
        }
	found = D_PLAYER(d);
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
	if ((D_FLAGS(d) & DS_CONNECTED) && !Hidden(D_PLAYER(d))) {
	    sprintf(buf, "%d@%s", D_PLAYER(d), mudconf.mud_name);
	    rwhocli_userlogin(buf, Name(D_PLAYER(d)), d->cold->connected_at);
	}
    }
    free_mbuf(buf);
    VOIDRETURN; /* #155 */
}
#endif
