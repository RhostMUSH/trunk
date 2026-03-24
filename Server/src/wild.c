/* wild.c - wildcard routines
 *
 * Written by T. Alexander Popiel, 24 June 1993
 * Last modified by T. Alexander Popiel, 19 August 1993
 *
 * Thanks go to Andrew Molitor for debugging
 * Thanks also go to Rich $alz for code to benchmark against
 *
 * Copyright (c) 1993 by T. Alexander Popiel
 * This code is hereby placed under GNU copyleft,
 * see copyright.h for details.
 */
#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "alloc.h"

#define FIXCASE(a) (ToLower((int)a))
#define EQUAL(a,b) ((a == b) || (FIXCASE(a) == FIXCASE(b)))
#define NOTEQUAL(a,b) ((a != b) && (FIXCASE(a) != FIXCASE(b)))

extern double safe_atof(char *);

static char **arglist;	/* argument return space */
static int numargs;	/* argument return size */
/* Code ifdef'd out: 
 * static int check_literals(const char *tstr, const char *dstr); 
 */

/* ---------------------------------------------------------------------------
 * quick_wild: do a wildcard match, without remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 */

int quick_wild_internal(char *tstr, char *dstr, int *pLoopCntr)
{
	
	if (*pLoopCntr > mudconf.wildmatch_lim) {
	   return 0;
	}
	(*pLoopCntr)++;

	/* We need to handle nulls */
	if ( (*tstr == '\0') && (*dstr == '\0') ) {
           return 1;
        }
        if ( *tstr == '\0' ) {
           return 0;
        }
	while (*tstr != '*') {
		switch (*tstr) {
		case '?':
			/* Single character match.  Return false if at
			 * end of data.
			 */
			if (!*dstr) 
			  return 0;
			break;
		case '\\':
			/* Escape character.  Move up, and force literal
			 * match of next character.
			 */
			tstr++;
			/* FALL THROUGH */
		default:
			/* Literal character.  Check for a match.
			 * If matching end of data, return true.
			 */
			if (NOTEQUAL(*dstr, *tstr)) 
			  return 0;
			if (!*dstr) 
			  return 1;
		}
                if ( *tstr )
		   tstr++;
                if ( *dstr )
		   dstr++;
	}

	/* Skip over '*'. */

        if ( *tstr )
	   tstr++;

	/* Return true on trailing '*'. */

	if (!*tstr) 
	  return 1;

	/* Skip over wildcards. */

	while ((*tstr == '?') || (*tstr == '*')) {
	  if (*tstr == '?') {
	    if (!*dstr) 
	      return 0;
	    dstr++;
	  }
	  tstr++;
	}

	/* Skip over a backslash in the pattern string if it is there. */

	if (*tstr == '\\') 
	  tstr++;

	/* Return true on trailing '*'. */

	if (!*tstr)
	  return 1;

	/* Scan for possible matches. */

	while (*dstr) {
		if (EQUAL(*dstr, *tstr) &&
		    quick_wild_internal(tstr+1, dstr+1, pLoopCntr)) return 1;
		dstr++;
	}
	return 0;
}

int quick_wild(char *tstr, char *dstr) {
  int loopCntr = 0;
  return quick_wild_internal(tstr, dstr, &loopCntr);
}

/* ---------------------------------------------------------------------------
 * wild1: INTERNAL: do a wildcard match, remembering the wild data.
 *
 * DO NOT CALL THIS FUNCTION DIRECTLY - DOING SO MAY RESULT IN
 * SERVER CRASHES AND IMPROPER ARGUMENT RETURN.
 *
 * Side Effect: this routine modifies the 'arglist' static global
 * variable.
 */
int wild1(char *tstr, char *dstr, int arg, int *pLoopCntr)
{
  char	*datapos;
  int	argpos, numextra;
 
        /* DoS Protection */
        if ( *pLoopCntr > mudconf.wildmatch_lim )
           return 0;
	(*pLoopCntr)++;

	while (*tstr != '*') {
		switch (*tstr) {
		case '?':
			/* Single character match.  Return false if at
			 * end of data.
			 */
			if (!*dstr) 
			  return 0;
			arglist[arg][0] = *dstr;
			arglist[arg][1] = '\0';
			arg++;

			/* Jump to the fast routine if we can. */

			if (arg >= numargs) 
			  return quick_wild_internal(tstr+1, dstr+1, pLoopCntr);
			break;
		case '\\':
			/* Escape character.  Move up, and force literal
			 * match of next character.
			 */
			tstr++;
			/* FALL THROUGH */
		default:
			/* Literal character.  Check for a match.
			 * If matching end of data, return true.
			 */
			if (NOTEQUAL(*dstr, *tstr)) 
			  return 0;
			if (!*dstr) 
			  return 1;
		}
		tstr++;
		dstr++;
	}

	/* If at end of pattern, slurp the rest, and leave. */

	if (!tstr[1]) {
	  strncpy(arglist[arg], dstr, LBUF_SIZE - 1);
	  arglist[arg][LBUF_SIZE - 1] = '\0';
	  return 1;
	}

	/* Remember current position for filling in the '*' return. */

	datapos = dstr;
	argpos = arg;

	/* Scan forward until we find a non-wildcard. */

	do {
	  if (argpos < arg) {
	    /* Fill in arguments if someone put another '*'
	     * before a fixed string.
	     */
	    arglist[argpos][0] = '\0';
	    argpos++;

	    /* Jump to the fast routine if we can. */
	    
	    if (argpos >= numargs) 
	      return quick_wild_internal(tstr, dstr, pLoopCntr);

	    /* Fill in any intervening '?'s */

	    while (argpos < arg) {
	      arglist[argpos][0] = *datapos;
	      arglist[argpos][1] = '\0';
	      datapos++;
	      argpos++;
	      
	      /* Jump to the fast routine if we can. */

	      if (argpos >= numargs)
		return quick_wild_internal(tstr, dstr, pLoopCntr);
	    }
	  }

	  /* Skip over the '*' for now... */
	  
	  tstr++;
	  arg++;
	  
	  /* Skip over '?'s for now... */
	  
	  numextra = 0;
	  while (*tstr == '?') {
	    if (!*dstr) 
	      return 0;
	    tstr++;
	    dstr++;
	    arg++;
	    numextra++;
	  }
	} while (*tstr == '*');

	/* Skip over a backslash in the pattern string if it is there. */

	if (*tstr == '\\')
	  tstr++;

	/* Check for possible matches.  This loop terminates either at
	 * end of data (resulting in failure), or at a successful match.
	 */
#ifndef WILD_LIKE_PENN
	while (1) {

	  /* Scan forward until first character matches. */
	  
	  if (*tstr)
	    while (NOTEQUAL(*dstr, *tstr)) {
	      if (!*dstr) return 0;
	      dstr++;
	    }
	  else 
	    while (*dstr) dstr++;

	  /* The first character matches, now.  Check if the rest
	   * does, using the fastest method, as usual.
	   */
	  if (!*dstr ||
	      ((arg<numargs) ? wild1(tstr+1, dstr+1, arg, pLoopCntr)
	       : quick_wild_internal(tstr+1, dstr+1, pLoopCntr))) {
	    
	    /* Found a match!  Fill in all remaining arguments.
	     * First do the '*'...
	     */
	    strncpy(arglist[argpos], datapos,
		    (dstr - datapos) - numextra);
	    arglist[argpos][(dstr - datapos) - numextra] = '\0';
	    datapos = dstr - numextra;
	    argpos++;
	    
	    /* Fill in any trailing '?'s that are left. */
	    
	    while (numextra) {
	      if (argpos >= numargs) return 1;
	      arglist[argpos][0] = *datapos;
	      arglist[argpos][1] = '\0';
	      datapos++;
	      argpos++;
	      numextra--;
	    }
	    
	    /* It's done! */
	    
	    return 1;
	  } else {
	    dstr++;
	  }
	}
#else
  if (!*tstr)
    while (*dstr)
      dstr++;
  else {
    argpos++;
    while (1) {
      if (EQUAL(*dstr, *tstr) &&
	  ((arg < numargs) ? wild1(tstr, dstr, arg, pLoopCntr)
	   : quick_wild_internal(tstr, dstr, pLoopCntr)))
	break;
      if (!*dstr)
	return 0;
      dstr++;
      argpos++;
    }
  }

  /* Found a match!  Fill in all remaining arguments.
   * First do the '*'...
   */
  strncpy(arglist[argpos], datapos,
	  (dstr - datapos) - numextra);
  arglist[argpos][(dstr - datapos) - numextra] = '\0';
  datapos = dstr - numextra;
  argpos++;

  /* Fill in any trailing '?'s that are left. */
  while (numextra) {
    if (argpos >= numargs)
      return 1;
    arglist[argpos][0] = *datapos;
    arglist[argpos][1] = '\0';
    datapos++;
    argpos++;
    numextra--;
  }

  /* It's done! */
  return 1;
#endif
}

/* ---------------------------------------------------------------------------
 * wild: do a wildcard match, remembering the wild data.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 *
 * This function may crash if alloc_lbuf() fails.
 *
 * Side Effect: this routine modifies the 'arglist' and 'numargs'
 * static global variables.
 */
int wild(char *tstr, char *dstr, char *args[], int nargs)
{
  int	i, value;
  char	*scan;
  int   loopCntr = 0;
	/* Initialize the return array. */

	for (i=0; i<nargs; i++) args[i] = NULL;

	/* Do fast match. */

	while ((*tstr != '*') && (*tstr != '?')) {
		if (*tstr == '\\') tstr++;
		if (NOTEQUAL(*dstr, *tstr)) return 0;
		if (!*dstr) return 1;
		tstr++;
		dstr++;
	}

	/* Allocate space for the return args. */

	i = 0;
	scan = tstr;
	while (*scan && (i < nargs)) {
		switch (*scan) {
		case '?':
			args[i] = alloc_lbuf("wild.?");
			i++;
			break;
		case '*':
			args[i] = alloc_lbuf("wild.*");
			i++;
		}
		scan++;
	}

	/* Put stuff in globals for quick recursion. */

	arglist = args;
	numargs = nargs;
#if  0
	MattRhost had some funky ass bug where this blew up.
	Until I can figure out why, this code is removed.
	Lensy.

	/* Do sanity check - ported from Penn */
	if (!check_literals(tstr, dstr)) {
	  /* Clean out any fake match data left by wild1. */
	  for (i=0; i<nargs; i++)
	    if ((args[i] != NULL) && (!*args[i] || !value)) {
	      free_lbuf(args[i]);
	      args[i] = NULL;
	    }
	  return 0;
	}
#endif
	/* Do the match. */

	value = nargs ? wild1(tstr, dstr, 0, &loopCntr) : quick_wild_internal(tstr, dstr, &loopCntr);

	/* Clean out any fake match data left by wild1. */
	for (i=0; i<nargs; i++)
		if ((args[i] != NULL) && (!*args[i] || !value)) {
			free_lbuf(args[i]);
			args[i] = NULL;
		}

	return value;
}

/* ---------------------------------------------------------------------------
 * wild_match: do either an order comparison or a wildcard match,
 * remembering the wild data, if wildcard match is done.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 */
int wild_match(char *tstr, char *dstr, char *args[], int nargs, int ck_arith)
{
   int i, i_loop, i_type;

   if ( ck_arith ) {
      i_loop = 1;
      i_type = 0;
      while ( i_loop ) {
         switch (*tstr) {
            case '>': /* Greater-than */
               if ( i_type & 3 ) {
                  i_loop = 0;
               } else {
                  i_type |= 1;
                  tstr++;
               }
               break;
            case '<': /* Less-than */
               if ( i_type & 3 ) {
                  i_loop = 0;
               } else {
                  i_type |= 2;
                  tstr++;
               }
               break;
            case '=': /* Equal */
               if ( i_type & 4 ) {
                  i_loop = 0;
               } else {
                  i_type |= 4;
                  tstr++;
               }
               break;
            default: /* Break out */
               i_loop = 0;
         }
      }
      if ( i_type ) {
         for  (i=0; i<nargs; i++) args[i] = NULL;

         i_loop = 0;
         if ( mudconf.float_switches ) {
            if ( isdigit((int)*tstr) || (*tstr == '-') || (*tstr == '.') ) {
               i_loop = 1;
            }
         } else {
            if ( isdigit((int)*tstr) || (*tstr == '-') ) {
               i_loop = 1;
            }
         }
         switch (i_type ) {
            case 1: /* gt */
               if ( i_loop ) {
                  if ( mudconf.float_switches ) {
                     return (safe_atof(tstr) < safe_atof(dstr));
                  } else {
                     return (atoi(tstr) < atoi(dstr));
                  }
               } else {
                  return (strcmp(tstr, dstr) < 0);
               }
               break;
            case 2: /* lt */
               if ( i_loop ) {
                  if ( mudconf.float_switches ) {
                     return (safe_atof(tstr) > safe_atof(dstr));
                  } else {
                     return (atoi(tstr) > atoi(dstr));
                  }
               } else {
                  return (strcmp(tstr, dstr) > 0);
               }
               break;
            case 4: /* eq */
               if ( i_loop ) {
                  if ( mudconf.float_switches ) {
                     return (safe_atof(tstr) == safe_atof(dstr));
                  } else {
                     return (atoi(tstr) == atoi(dstr));
                  }
               } else {
                  return (strcmp(tstr, dstr) == 0);
               }
               break;
            case 5: /* gte */
               if ( i_loop ) {
                  if ( mudconf.float_switches ) {
                     return (safe_atof(tstr) <= safe_atof(dstr));
                  } else {
                     return (atoi(tstr) <= atoi(dstr));
                  }
               } else {
                  return (strcmp(tstr, dstr) <= 0);
               }
               break;
            case 6: /* lte */
               if ( i_loop ) {
                  if ( mudconf.float_switches ) {
                     return (safe_atof(tstr) >= safe_atof(dstr));
                  } else {
                     return (atoi(tstr) >= atoi(dstr));
                  }
               } else {
                  return (strcmp(tstr, dstr) >= 0);
               }
               break;
         }
      }
   }

   return nargs ? wild(tstr, dstr, args, nargs) : quick_wild(tstr, dstr);
}


#if 0
/* check_literals: Taken from the PennMUSH code */
static int check_literals(const char *tstr, const char *dstr)
{
   /* Every literal string in tstr must appear, in order, in dstr,
    * or no match can happen. That is, tstr is the pattern and dstr
    * is the string-to-match
    */
  char *tbuf1;
  char *dbuf1, *tstrtokr;
  const char delims[] = "?*";
  char *sp, *dp;

  tbuf1 = alloc_lbuf("check_literals");
  dbuf1 = alloc_lbuf("check_literals");

  sp = strtok_r(tbuf1,delims,&tstrtokr);
  while (sp) {
    if (!dp) {
      free_lbuf(dbuf1);
      free_lbuf(tbuf1);
      return 0;
    }
    if (!(dp = strstr(dp,sp))) {
      free_lbuf(dbuf1);
      free_lbuf(tbuf1);
      return 0;
    }
    dp += strlen(sp);
    sp = strtok_r(NULL,delims,&tstrtokr);
  }
  free_lbuf(dbuf1);
  free_lbuf(tbuf1);
  return 1;
}
#endif

