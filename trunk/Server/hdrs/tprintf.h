#ifndef _TPRINTF_H_
#define _TPRINTF_H_

/* Macros:
 *    These macros wrap around the tprintf function and
 *    abstract out the safe and unsafe implementations.
 *
 *    Filename and line number are passed in for debugging
 *    purposes.
 */
#define safe_tprintf(buff, bp, format, ...) \
      real_tprintf(__FILE__, __LINE__, \
		   buff, bp, format, __VA_ARGS__)

#define unsafe_tprintf(format, ...) \
      real_tprintf(__FILE__, __LINE__, \
		   NULL, NULL, format, __VA_ARGS__)

#ifdef STDC_HEADERS
char *real_tprintf(char *filename, int lineNo,
		   char *buff, char **bp, char *format, ...);
#else
char *real_tprintf(char *filename, int lineNo,
		   char *buff, char **bp, va_alist);
#endif

/* This should only be called in one place 
 *  - at the end of the main Rhost work loop
 */
void freeTrackedBuffers(void);
void showTrackedBufferStats(dbref player);

#endif
