/*
	Misc support routines for unter style error management.
	Stolen from mjr.

	Modded to scribble on stderr, for now.

	Andrew Molitor, amolitor@eagle.wesleyan.edu
*/

#include "autoconf.h"
#include "udb_defs.h"
#include "externs.h"

/* Log database errors */

void log_db_err (int obj, int attr, const char *txt)
{
	STARTLOG(LOG_ALWAYS,"DBM","ERROR")
		log_text((char *)"Could not ");
		log_text((char *)txt);
		log_text((char *)" object #");
		log_number(obj);
		if (attr != NOTHING) {
			log_text((char *)" attr #");
			log_number(attr);
		}
	ENDLOG
}

/*
print a series of warnings - do not exit
*/
/* VARARGS */
#ifdef STDC_HEADERS
void mush_logf(char *p, ...)
#else
void mush_logf(va_alist)
va_dcl
#endif

{
va_list	ap;
FILE *f_foo;


#ifdef STDC_HEADERS
	va_start(ap,p);
#else
char	*p;

	va_start(ap);
	p = va_arg(ap,char *);
#endif
       if ( mudstate.f_logfile_name )
          f_foo = mudstate.f_logfile_name;
       else
          f_foo = stderr;

	while(1) {
		if(p == (char *)0)
			break;

		if(p == (char *)-1)
		  p = strerror(errno);
 		 /* p = (char *) sys_errlist[errno]; DEPRECATED */

		(void)fprintf(f_foo,"%s",p);
		p = va_arg(ap,char *);
	}
	va_end(ap);
	(void)fflush(f_foo);
}

/*
print a series of warnings - exit
*/
/* VARARGS */
#ifdef STDC_HEADERS
void fatal(char *p, ...)
#else
void fatal(va_alist)
va_dcl
#endif
{
va_list	ap;

#ifdef STDC_HEADERS
	va_start(ap,p);
#else
	char	*p;

	va_start(ap);
	p = va_arg(ap,char *);
#endif
	while(1) {
		if(p == (char *)0)
			break;

		if(p == (char *)-1)
		  p = strerror(errno);
 		 /* p = (char *) sys_errlist[errno]; DEPRECATED */

		(void)fprintf(stderr,"%s",p);
		p = va_arg(ap,char *);
	}
	va_end(ap);
	(void)fflush(stderr);
	exit(1);
}
