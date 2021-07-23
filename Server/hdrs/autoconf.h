/* autoconf.h.  Generated automatically by configure.  */
/* autoconf.h.in -- System-dependent configuration information */

#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "copyright.h"

/* Define the codebase as RhostMUSH */
#define RHOSTMUSH 1

/* Define the number of Totem Slots
 * This value must be between 1 and X where X is based on LBUF size
 *
 * Suggested values:
 * 10 slots (for  320 flags)
 * 20 slots (for  640 flags)
 * 40 slots (for 1280 flags)
 * Max values:
 *** 4K  lbufs -  300 max slots  (for   9600 flags)
 *** 8K  lbufs -  600 max slots  (for  19200 flags)
 *** 16K lbufs - 1200 max slots  (for  38400 flags)
 *** 32K lbufs - 2500 max slots  (for  80000 flags)
 *** 64K lbufs - 5000 max slots  (for 160000 flags)
 *
 * Overhead: ((slots * 8) * total objects) + (flags * name len) + (slots * 8)
 * Flags are based on how many are used only.
 *
 * Assuming all flags possible used with max length of name.
 * 5000 obj db, 300 slots: (300 * 8) * 5000) + (9600 * 20) + (300 * 8)
 *                         12,194,400 (12 MB)
 * 20000 obj db, 2500 slots: (2500 * 8) * 20000) + (80000 * 20) + (2500 * 8)
 *                         401,620,000 (400 MB)
 * 20000 obj db, 40 slots: (40 * 8) * 20000) + (1280 * 20) + (40 * 8)
 *                         6,425,920 (6.4 MB)
 * 20000 obj db, 10 slots: (10 * 8) * 20000) + (320 * 20) + (10 * 8) (default)
 *                         1,606,480 (1.6 MB)
 *
 * Please be aware of the overhead.  This will also drastically increase stack
 * space.  Drastically raising this value will require raising stack of your
 * shell process.  ulimit -s
 * For high numbers, suggest 32,000 or 64,000 stack size
 * Strongly suggest keeping this to a reasonable number.
 *
 * 10 slot default gives you 320 flags.  Nearly twice what Rhost has in use.
 * Unless you are really insane to want enough flags to choke a unicorn, it
 * should be plenty.
 */

 /* Warning:
  * Do NOT exceed ((LBUF_SIZE / 9) - 10) total slots or you will CRASH.
  * Maximums per LBUFS will be: (rounded down to closest 100)
  *** 4K  lbufs -  300 max slots  (for   9600 flags)
  *** 8K  lbufs -  600 max slots  (for  19200 flags)
  *** 16K lbufs - 1200 max slots  (for  38400 flags)
  *** 32K lbufs - 2500 max slots  (for  80000 flags)
  *** 64K lbufs - 5000 max slots  (for 160000 flags)
  */
#define TOTEM_SLOTS 10

/* ---------------------------------------------------------------------------
 * Configuration section:
 *
 * These defines are written by the configure script.
 * Change them if need be
 */

/* define header information for restricted pointers */
#ifndef __GNUC_PREREQ
#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
        ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif
#endif
/* Define if values required (like MAXINT) */
#define NEED_VALUES 1
/* Define if we have stdlib.h et al. */
#define STDC_HEADERS 1
/* Define if we have string.h instead of strings.h */
/* #undef USG */
/* Define if we have unistd.h */
#define HAVE_UNISTD_H 1
/* Define if we have memory.h and need it to get memcmp et al. */
/* #undef NEED_MEMORY_H */
/* Decl for pid_t */
/* #undef pid_t */
/* signal() return type */
#define RETSIGTYPE void
/* Define if we have vfork.h */
/* #undef HAVE_VFORK_H */
/* Define if vfork is broken */
/* #undef vfork */
/* Define if wait3 exists and works */
#define HAVE_WAIT3 1
/* Define if struct tm is not in time.h */
/* #undef TM_IN_SYS_TIME */
/* Define if struct tm has a timezone member */
/* #undef HAVE_TM_ZONE */
/* Define if tzname[] exists */
#define HAVE_TZNAME 1
/* Define if setrlimit exists */
#define HAVE_SETRLIMIT 1
/* Define if getrusage exists */
#define HAVE_GETRUSAGE 1
/* Define if timelocal exists */
/* #undef HAVE_TIMELOCAL */
/* Define if mktime exists */
#define HAVE_MKTIME 1
/* Define if getdtablesize exists */
#define HAVE_GETDTABLESIZE 1
/* Define if getpagesize exists */
#define HAVE_GETPAGESIZE 1
/* Define if gettimeofday exists */
#define HAVE_GETTIMEOFDAY 1
/* Define if srandom exists */
#define HAVE_SRANDOM 1
/* Define if sys_siglist[] exists */
/* #undef HAVE_SYS_SIGLIST */
/* Define if _sys_siglist[] exists */
/* #undef HAVE__SYS_SIGLIST */
/* Define if index/rindex/mem??? are defined in string.h */
/* #undef INDEX_IN_STRING_H */
/* Define if malloc/realloc/free are defined in stdlib.h */
/* #undef MALLOC_IN_STDLIB_H */
/* Define if calling signal with SIGCHLD when handling SIGCHLD blows chow */
/* #undef SIGNAL_SIGCHLD_BRAINDAMAGE */
/* Define if errno.h exists */
/* #undef HAVE_ERRNO_H */
/* Define if malloc.h exists */
/* #undef HAVE_MALLOC_H */
/* Define if sys/select.h exists */
/* #undef HAVE_SYS_SELECT_H */
/* Define if const is broken */
/* #undef const */
/* Define if char type is unsigned */
/* #undef __CHAR_UNSIGNED__ */
/* Define if inline keyword is broken or nonstandard */
/* #undef inline */
/* Define if we need to redef index/bcopy et al. to their SYSV counterparts */
/* #undef NEED_INDEX_DCL */
/* Define if we need to declare malloc et al. */
/* #undef NEED_MALLOC_DCL */
/* Define if we need to declare vsprintf yourself */
/* #undef NEED_VSPRINTF_DCL */
/* Define if you need to declare sys_siglist yourself */
/* #undef NEED_SYS_SIGLIST_DCL */
/* Define if you need to declare _sys_siglist yourself */
/* #undef NEED__SYS_SIGLIST_DCL */
/* Define if you need to declare sys_errlist yourself */
/* #undef NEED_SYS_ERRLIST_DCL */
/* Define if you need to declare _sys_errlist yourself */
/* #undef NEED_SYS__ERRLIST_DCL */
/* Define if you need to declare perror yourself */
/* #undef NEED_PERROR_DCL */
/* Define if you need to declare sprintf yourself */
/* #undef NEED_SPRINTF_DCL */
/* Define if you need to declare getrlimit yourself */
/* #undef NEED_GETRLIMIT_DCL */
/* Define if you need to declare getrusage yourself */
/* #undef NEED_GETRUSAGE_DCL */
/* Define if struct linger is defined */
#define HAVE_LINGER 1
/* Define if signal handlers have a struct sigcontext as their third arg */
/* #undef HAVE_STRUCT_SIGCONTEXT */
/* Define if stdio.h defines lots of extra functions */
#define EXTENDED_STDIO_DCLS 1
/* Define if sys/socket.h defines lots of extra functions */
#define EXTENDED_SOCKET_DCLS 1
/* Define if sys/wait.h defines union wait. */
/* #undef HAVE_UNION_WAIT */
/* Define if we have system-supplied ndbm routines */
#define HAVE_NDBM 1
/* Define if we have system-supplied (old) dbm routines */
/* #undef HAVE_DBM */
/* Define if we may safely include both time.h and sys/time.h */
#define TIME_WITH_SYS_TIME 1
/* Define if sys/time.h exists */
/* #undef HAVE_SYS_TIME_H */

/* ---------------------------------------------------------------------------
 * Setup section:
 *
 * Load system-dependent header files.
 */

/* Prototype templates for ANSI C and traditional C */

#ifdef __STDC__
#define	NDECL(f)	f(void)
#define	FDECL(f,p)	f p
#ifdef STDC_HEADERS
#define	VDECL(f,p)	f p
#else
#define VDECL(f,p)	f()
#endif
#else
#define NDECL(f)	f()
#define FDECL(f,p)	f()
#define VDECL(f,p)	f()
#endif

#ifdef STDC_HEADERS
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#else
#include <varargs.h>
extern int	FDECL(atoi, (const char *));
extern double	FDECL(atof, (const char *));
extern long	FDECL(atol, (const char *));
#endif

#ifdef NEED_MEMORY_H
#include <memory.h>
#endif

#if defined(USG) || defined(STDC_HEADERS)
#include <string.h>
#ifdef NEED_INDEX_DCL
#define	index		strchr
#define	rindex		strrchr
#define	bcopy(s,d,n)	memcpy(d,s,n)
#define	bcmp(s1,s2,n)	memcmp(s1,s2,n)
#define	bzero(s,n)	memset(s,0,n)
#endif
#else
#include <strings.h>
extern char *	FDECL(strtok, (char *, char *));
extern char *	FDECL(strchr, (char *, char));
extern void	FDECL(bcopy, (char *, char *, int));
extern void	FDECL(bzero, (char *, int));
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRUSAGE)
#include <sys/resource.h>
#ifdef NEED_GETRUSAGE_DCL
extern int	FDECL(getrusage, (int, struct rusage *));
#endif
#ifdef NEED_GETRLIMIT_DCL
extern int	FDECL(getrlimit, (int, struct rlimit *));
extern int	FDECL(setrlimit, (int, struct rlimit *));
#endif
#endif

#include <sys/param.h>
#ifndef HAVE_GETPAGESIZE
#ifdef EXEC_PAGESIZE
#define getpagesize()	EXEC_PAGESIZE
#else
#ifdef NBPG
#ifndef CLSIZE
#define CLSIZE 1
#endif /* no CLSIZE */
#define getpagesize() NBPG * CLSIZE
#else /* no NBPG */
#define getpagesize() NBPC
#endif /* no NBPG */
#endif /* no EXEC_PAGESIZE */
#else
#ifndef HAVE_UNISTD_H
extern int	NDECL(getpagesize);
#endif /* HAVE_UNISTD_H */
#endif /* HAVE_GETPAGESIZE_H */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#ifdef NEED_PERROR_DCL
extern void	FDECL(perror, (const char *));
#endif
#else
extern int errno;
extern void	FDECL(perror, (const char *));
#endif
#ifdef BROKEN_ERRNO
#include <asm/errno.h>
#endif
#ifdef BROKEN_ERRNO_SYS
#include <sys/errno.h>
#endif
#ifdef NEED_SYSERRLIST
#ifdef  __USE_BSD
   extern int sys_nerr;
   extern char *sys_errlist[];
#endif
#ifdef  __USE_GNU
   extern int _sys_nerr;
   extern char *_sys_errlist[];
#endif
#endif

#ifdef MALLOC_IN_STDLIB_H
#include <stdlib.h>
#else
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#ifdef NEED_MALLOC_DCL
extern char *	FDECL(malloc, (int));
extern char *	FDECL(realloc, (char *, int));
extern int	FDECL(free, (char *));
#endif
#endif
#endif

#ifdef NEED_SYS_ERRLIST_DCL
extern char *sys_errlist[];
#endif

#ifndef HAVE_TIMELOCAL

#ifndef HAVE_MKTIME
#define NEED_TIMELOCAL
extern time_t	FDECL(timelocal, (struct tm *));
#else
#define timelocal mktime
#endif /* HAVE_MKTIME */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

#endif /* HAVE_TIMELOCAL */

#ifndef tolower
extern int	FDECL(tolower, (int));
#endif
#ifndef toupper
extern int	FDECL(toupper, (int));
#endif

#ifndef HAVE_SRANDOM
#define random rand
#define srandom srand
#else
#ifndef random	/* only if not a macro */
/* extern long	NDECL(random); */
#endif
#endif /* HAVE_SRANDOM */
#ifdef SOLARIS
#ifndef projid_t
#define projid_t int
#endif
#endif

#ifndef VMS
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#ifdef NEED_SPRINTF_DCL
extern char	*VDECL(sprintf, (char *, const char *, ...));
#endif

#ifndef EXTENDED_STDIO_DCLS
extern int 	VDECL(fprintf, (FILE *, const char *, ...));
extern int	VDECL(printf, (const char *, ...));
extern int	VDECL(sscanf, (const char *, const char *, ...));
extern int	FDECL(close, (int));
extern int	FDECL(fclose, (FILE *));
extern int	FDECL(fflush, (FILE *));
extern int	FDECL(fgetc, (FILE *));
extern int	FDECL(fputc, (int, FILE *));
extern int	FDECL(fputs, (const char *, FILE *));
extern int	FDECL(fread, (void *, size_t, size_t, FILE *));
extern int	FDECL(fseek, (FILE *, long, int));
extern int	FDECL(fwrite, (void *, size_t, size_t, FILE *));
extern pid_t	FDECL(getpid, (void));
extern int	FDECL(pclose, (FILE *));
extern int	FDECL(rename, (char *, char *));
extern time_t	FDECL(time, (time_t *));
extern int	FDECL(ungetc, (int, FILE *));
extern int	FDECL(unlink, (char *));
extern int	FDECL(write, (int, char *, int));
#endif

#include <sys/socket.h>
#ifndef EXTENDED_SOCKET_DCLS
extern int	FDECL(accept, (int, struct sockaddr *, int *));
extern int	FDECL(bind, (int, struct sockaddr *, int));
extern int	FDECL(listen, (int, int));
extern int	FDECL(sendto, (int, void *, int, unsigned int,
			struct sockaddr *, int));
extern int	FDECL(setsockopt, (int, int, int, void *, int));
extern int	FDECL(shutdown, (int, int));
extern int	FDECL(socket, (int, int, int));
#endif

typedef int	dbref;
typedef int	FLAG;

#ifdef REALITY_LEVELS
typedef int RLEVEL;
#endif /* REALITY_LEVELS */

typedef char	boolexp_type;
typedef char	IBUF[16];

/* Byte-offset code */
#ifndef BIT64
typedef unsigned int    pmath1;
typedef int     pmath2;
#define ALLIGN1 4
#else
typedef unsigned long    pmath1;
typedef long     pmath2;
#define ALLIGN1 8
#endif

/* Lensman: Cygwin doesn't need many tweaks,
 *          but it does need a few.
 */
#ifdef BSD_LIKE
#define MAXINT INT_MAX
#ifdef stricmp
#undef stricmp
#endif
#ifdef MIN
#undef MIN
#endif
#endif
#endif /* AUTOCONF_H */
