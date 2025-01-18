/*
 * Code originally written by Lensman, 2005
 * Contact: lensman@the-wyvern.net
 *
 * This code is to be considered fully open source. It can be used for any
 * purpose providing that this credit is maintained and any fixes made
 * are returned to Lensman for inclusion in other projects 
 *
 *
 * TODO:
 *   - Change the printf's to Rhost log messages
 *   - Write an admin debug command for internal functions/macros
 *   - Testing the code, bp stuff, etc.
 *   - Include a vnsprintf function for systems that don't have it
 *   - 
 */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"

#include "interface.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"

#include "utils.h"
#include "tprintf.h"

#include <assert.h>
#include "ansi.h"

#define TPRINTF_LIST_SIZE 10
typedef struct _tprintf_list {
  char *pBuff[TPRINTF_LIST_SIZE];
  int nInUse;
  struct _tprintf_list *pNext;
} tprintf_list_t;


typedef struct _tprintf_stats {
  int maxLength;
  int current;
  long cummulativeLength;
  long numberOfCalls;
} tprintf_stats_t;

#ifdef STANDALONE
extern int getBufferSize(char *buffer);
#else
static tprintf_list_t bufferList = { {0}, 0, NULL };
static tprintf_stats_t bufferStats = { 0, 0 };
#endif

char *getTrackedBuffer(void);

#ifdef NEED_VSNPRINTF
extern char *	vsnprintf(char *, char *, va_list);
#endif

#ifdef STDC_HEADERS
char *real_tprintf(char *filename, int lineNo,
		   char *buff, char **bp, char *format, ...)
#else
char *real_tprintf(char *filename, int lineNo,
		   char *buff, char **bp, va_alist)
va_dcl
#endif
{
  int length, end, ret;
  char *bp2;
  va_list ap;
#ifdef STDC_HEADERS
  va_start(ap, format);
#else
  char	*format;
  va_start(ap);
  format = va_arg(ap, char *);
#endif

  if (buff == NULL) {
    buff = getTrackedBuffer();
    bp2 = buff;
    bp = &bp2;
  }

  length = getBufferSize(buff);

  end = length - 1; /* In other words buff[end] */
  
  if (length > 0) {
    length -= (*bp - buff) + 1;
  } else {
    /* TODO: Change to log msg */
    fprintf(stderr, 
	    "ERR: (%s:%d) Unknown pool passed to safe_tprintf.\n",
	    filename, lineNo);
  }

  if (length < 0) {
    length = 0;
    fprintf(stderr, 
	    "WRN: (%s:%d) Zero length buffer in safe_tprintf.\n",
	    filename, lineNo);
  }

  ret = vsnprintf(buff, length, format, ap);
  va_end(ap);

  buff[end] = '\0';
  
  if (ret < 0) {
     fprintf(stderr, 
	     "ERR: (%s:%d) vsnprintf returned error code (%d).\n", 
	     filename, lineNo, ret);
  }

  
  /* Unfortunately not all vsnprintf implementations return the number of
   * characters written to the buffer.
   * Solaris, for example, states:
   *
   * The  vsnprintf()  function  returns the  number  of characters formatted, 
   * that is, the number of characters that would have been written to the
   * buffer if it were large enough.
   *
   * ... This is the most efficient way of finding the \0 I could think of.
   */
  if (ret <= length) {
     *bp += ret + 1;
  } else {
     for (bp2 = *bp; (*bp2 != '\0') && (bp2 != &buff[end]); bp2++);
     *bp = bp2;
  }
  return buff;
}

#ifdef OLD_TPRINTF
{
static char buff[10000];
va_list	ap;

#ifdef STDC_HEADERS
	va_start(ap, format);
#else
char	*format;
	va_start(ap);
	format = va_arg(ap, char *);
#endif
	vsprintf(buff, format, ap);
	va_end(ap);
	buff[9999] = '\0';
	return buff;
}
#endif

#ifndef STANDALONE
char *getTrackedBuffer(void) {
  tprintf_list_t *pList = &bufferList;

  while (1) {
    if (pList->nInUse >= TPRINTF_LIST_SIZE) {

      if (pList->pNext == NULL) {
	pList->pNext = malloc(sizeof(tprintf_list_t));

	if (pList->pNext == NULL) {
	  /* TODO: Handle out of memory cleanly */
	  RHOSTpanic("Failed to allocate memory.");
	} else {
	  pList = pList->pNext;
	  pList->nInUse = 0;
	  pList->pNext = NULL;
	}

      }  else { /* if (pList->pNext != NULL) */
	pList = pList->pNext;
      } 
    } else { /* pList->nInUse < TPRINTF_LIST_SIZE */
      pList->pBuff[pList->nInUse] = alloc_lbuf("tprintf");
      pList->nInUse++;

      bufferStats.current++;

      if (bufferStats.current > bufferStats.maxLength) {
	bufferStats.maxLength = bufferStats.current;
      }

      return pList->pBuff[(pList->nInUse - 1)];
    }
  }
}

void freeTrackedBuffers(void) {
  tprintf_list_t *pList = &bufferList;
  tprintf_list_t *pCurrentList = NULL;
  int nFreed = 0;
  int i;

  while (pList != NULL) {

    /* Cycle through and free these buffers */  
    for ( i = 0 ; i < pList->nInUse ; i++) {
      free_lbuf(pList->pBuff[i]);
      nFreed++;
    }
    pList->nInUse -= i;
    RHOSTassert(pList->nInUse == 0);

    pList = pList->pNext;

    /* First time through loop, this will be null
     * - so we never try free initial element.
     */
    if (pCurrentList != NULL) {
      free(pCurrentList);
    } else {
      /* Although we don't free the initial element,
       * we do need to reset it's next ptr.
       * Otherwise say hello to my friend Mr. Coredump.
       */
      bufferList.pNext = NULL; 
    }
    pCurrentList = pList;

  };

  if ( nFreed > 0 ) {
    bufferStats.cummulativeLength += nFreed;
    bufferStats.numberOfCalls++;
    bufferStats.current -= nFreed;
    assert( bufferStats.current == 0);
  }


}

void showTrackedBufferStats(dbref player, int color) {
  char *buff = NULL;
  int avg = 0;
  
  if (bufferStats.numberOfCalls > 0) {
      avg = bufferStats.cummulativeLength / bufferStats.numberOfCalls;
  } else {
      /* This can only happen at mush startup, before the first free iteration */
      avg = bufferStats.current;
  }
  
#ifdef ZENTY_ANSI
  buff = unsafe_tprintf(
	                 "%s%s\nTprintf Stats   Current  Maximum   Average\n"
			 "                %-5d    %-5d     %-5d%s",
                         (char *)(color ? ANSI_HILITE : ""),
                         (char *)(color ? ANSI_GREEN : ""),
	                 bufferStats.current,
		         bufferStats.maxLength,
			 avg,
                         (char *)(color ? ANSI_NORMAL : ""));
#else
  buff = unsafe_tprintf(
	                 "\nTprintf Stats   Current  Maximum   Average\n"
			 "                %-5d    %-5d     %-5d",
	                 bufferStats.current,
		         bufferStats.maxLength,
			 avg);
#endif

  
  notify(player, buff);

}
#else

/* Nasty, nasty hack for if we're building in
 * 'standalone' mode (e.g. dbconvert )
 *
 * This should be no more dangerous than the
 * original tprintf call
 *
 * (Not that this really reassures me!)
 */
char *getTrackedBuffer(void) {
  static char buffer[10000];
  return buffer;
}

int getBufferSize(char *buffer) {
  return 10000;
}
#endif
