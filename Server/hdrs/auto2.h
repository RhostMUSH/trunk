/* autoconf.h.  Generated automatically by configure.  */
/* autoconf.h.in -- System-dependent configuration information */

#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "copyright.h"

/* ---------------------------------------------------------------------------
 * Configuration section:
 *
 * These defines are written by the configure script.
 * Change them if need be
 */

/* Define if we have stdlib.h et al. */
#define STDC_HEADERS 1
/* Define if we have string.h instead of strings.h */
#define USG 1
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
/* #undef HAVE_WAIT3 */
/* Define if struct tm is not in time.h */
/* #undef TM_IN_SYS_TIME */
/* Define if struct tm has a timezone member */
/* #undef HAVE_TM_ZONE */
/* Define if tzname[] exists */
#define HAVE_TZNAME 1
/* Define if setrlimit exists */
#define HAVE_SETRLIMIT 1
/* Define if getrusage exists */
/* #undef HAVE_GETRUSAGE */
/* Define if timelocal exists */
/* #undef HAVE_TIMELOCAL */
/* Define if mktime exists */
#define HAVE_MKTIME 1
/* Define if getdtablesize exists */
/* #undef HAVE_GETDTABLESIZE */
/* Define if getpagesize exists */
/* #undef HAVE_GETPAGESIZE */
/* Define if gettimeofday exists */
#define HAVE_GETTIMEOFDAY 1
/* Define if srandom exists */
/* #undef HAVE_SRANDOM */
/* Define if sys_siglist[] exists */
/* #undef HAVE_SYS_SIGLIST */
/* Define if _sys_siglist[] exists */
#define HAVE__SYS_SIGLIST 1
/* Define if index/rindex/mem??? are defined in string.h */
/* #undef INDEX_IN_STRING_H */
/* Define if malloc/realloc/free are defined in stdlib.h */
/* #undef MALLOC_IN_STDLIB_H */
/* Define if calling signal with SIGCHLD when handling SIGCHLD blows chow */
#define SIGNAL_SIGCHLD_BRAINDAMAGE 1
/* Define if errno.h exists */
#define HAVE_ERRNO_H 1
/* Define if malloc.h exists */
#define HAVE_MALLOC_H 1
/* Define if sys/select.h exists */
#define HAVE_SYS_SELECT_H 1
/* Define if const is broken */
/* #undef const */
/* Define if char type is unsigned */
/* #undef __CHAR_UNSIGNED__ */
/* Define if inline keyword is broken or nonstandard */
/* #undef inline */
/* Define if we need to redef index/bcopy et al. to their SYSV counterparts */
#define NEED_INDEX_DCL 1
/* Define if we need to declare malloc et al. */
/* #undef NEED_MALLOC_DCL */
/* Define if we need to declare vsprintf yourself */
/* #undef NEED_VSPRINTF_DCL */
/* Define if you need to declare sys_siglist yourself */
/* #undef NEED_SYS_SIGLIST_DCL */
/* Define if you need to declare _sys_siglist yourself */
/* #undef NEED__SYS_SIGLIST_DCL */
/* Define if you need to declare sys_errlist yourself */
#define NEED_SYS_ERRLIST_DCL 1
/* Define if you need to declare _sys_errlist yourself */
/* #undef NEED_SYS__ERRLIST_DCL */
/* Define if you need to declare perror yourself */
#define NEED_PERROR_DCL 1
/* Define if you need to declare sprintf yourself */
/* #undef NEED_SPRINTF_DCL */
/* Define if you need to declare getrlimit yourself */
/* #undef NEED_GETRLIMIT_DCL */
/* Define if you need to declare getrusage yourself */
/* #define NEED_GETRUSAGE_DCL 1 */
/* Define if struct linger is defined */
/* #undef HAVE_LINGER */
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

#ifdef STDC_HEADERS
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#else
#include <varargs.h>
extern int	atoi(const char *);
extern double	atof(const char *);
extern long	atol(const char *);
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
extern char *	strtok(char *, char *);
extern char *	strchr(char *, char);
extern void	bcopy(char *, char *, int);
extern void	bzero(char *, int);
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
extern int	getrusage(int, struct rusage *);
#endif
#ifdef NEED_GETRLIMIT_DCL
extern int	getrlimit(int, struct rlimit *);
extern int	setrlimit(int, struct rlimit *);
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
extern int	getpagesize(void);
#endif /* HAVE_UNISTD_H */
#endif /* HAVE_GETPAGESIZE_H */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#ifdef NEED_PERROR_DCL
extern void	perror(const char *);
#endif
#else
extern int errno;
extern void	perror(const char *);
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#ifdef NEED_MALLOC_DCL
extern char *	malloc(int);
extern char *	realloc(char *, int);
extern int	free(char *);
#endif
#endif

#ifdef NEED_SYS_ERRLIST_DCL
extern char *sys_errlist[];
#endif

#ifndef HAVE_TIMELOCAL

#ifndef HAVE_MKTIME
#define NEED_TIMELOCAL
extern time_t	timelocal(struct tm *);
#else
#define timelocal mktime
#endif /* HAVE_MKTIME */

#endif /* HAVE_TIMELOCAL */

#ifndef tolower
extern int	tolower(int);
#endif
#ifndef toupper
extern int	toupper(int);
#endif

#ifndef HAVE_SRANDOM
#define random rand
#define srandom srand
#else
#ifndef random	/* only if not a macro */
extern long	random(void);
#endif
#endif /* HAVE_SRANDOM */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

#ifndef VMS
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#ifdef NEED_SPRINTF_DCL
extern char	*sprintf(char *, const char *, ...);
#endif

#ifndef EXTENDED_STDIO_DCLS
extern int 	fprintf(FILE *, const char *, ...);
extern int	printf(const char *, ...);
extern int	sscanf(const char *, const char *, ...);
extern int	close(int);
extern int	fclose(FILE *);
extern int	fflush(FILE *);
extern int	fgetc(FILE *);
extern int	fputc(int, FILE *);
extern int	fputs(const char *, FILE *);
extern int	fread(void *, size_t, size_t, FILE *);
extern int	fseek(FILE *, long, int);
extern int	fwrite(void *, size_t, size_t, FILE *);
extern pid_t	getpid(void);
extern int	pclose(FILE *);
extern int	rename(char *, char *);
extern time_t	time(time_t *);
extern int	ungetc(int, FILE *);
extern int	unlink(char *);
extern int	write(int, char *, int);
#endif

#include <sys/socket.h>
#ifndef EXTENDED_SOCKET_DCLS
extern int	accept(int, struct sockaddr *, int *);
extern int	bind(int, struct sockaddr *, int);
extern int	listen(int, int);
extern int	sendto(int, void *, int, unsigned int, struct sockaddr *, int);
extern int	setsockopt(int, int, int, void *, int);
extern int	shutdown(int, int);
extern int	socket(int, int, int);
#endif

typedef int	dbref;
typedef int	FLAG;
typedef char	boolexp_type;
typedef char	IBUF[16];

#endif /* AUTOCONF_H */
