/* functions.h - declarations for functions & function processing */

#include "copyright.h"

#ifndef _M_FUNCTIONS_H
#define _M_FUNCTIONS_H

/* This is the prototype for functions, be sure to update the 
   prototype in struct fun if this is changed */

#define FUNCTION(x)     \
  void x(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, \
         char *fargs[], int nfargs, char *cargs[], int ncargs)

typedef struct fun {
	const char *name;	/* function name */
	void	(*fun)(char*,char**,dbref,dbref,dbref,
                       char*[],int, char*[], int);	/* handler */
	int	nargs;		/* Number of args needed or expected */
	int	flags;		/* Function flags */
	int	perms;		/* Access to function */
	int	perms2;
} FUN;

typedef struct ufun {
	const char *name;	/* function name */
	dbref	obj;		/* Object ID */
	dbref 	owner;		/* NOT USED except for local functions */
        dbref   orig_owner;	/* NOT USED except for local functions */
	int	atr;		/* Attribute ID */
	int	flags;		/* Function flags */
	int	perms;		/* Access to function */
	int	perms2;
	int	minargs;	/* minimum args expected - optional */
	int	maxargs;	/* maximum args expected - optional */
	struct ufun *next;	/* Next ufun in chain */
} UFUN;

#define	FN_VARARGS	1	/* Function allows a variable # of args */
#define	FN_NO_EVAL	2	/* Don't evaluate args to function */
#define	FN_PRIV		4	/* Perform user-def function as holding obj */
#define FN_LIST         8       /* List user-def functions */
#define FN_PRES        16       /* Function is preserved */
#define FN_DEL	       32	/* Delete function */
#define FN_BYPASS      64       /* Bypass function restriction */
#define FN_DISPLAY    128	/* Display function */
#define FN_PROTECT    256	/* Private the variables - assumes FN_PRES */
#define FN_MIN        512	/* Min value */
#define FN_MAX       1024       /* Max value */
#define FN_NOTRACE   2048	/* Do not trace @function */
#define FN_LOCAL     4096       /* @function is a local user function */

extern void	NDECL(init_functab);
extern void	FDECL(list_functable, (dbref, char *));
extern void	FDECL(list_functable2, (dbref, char*, char**, int));

/* (Moved from functions.c by Lensman)
 * This is for functions that take an optional delimiter character */
#define varargs_preamble(xname,xnargs)                                  \
	if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufcx, 0,   \
		player, cause, caller, cargs, ncargs))                          \
		return;

#define evarargs_preamble(xname,xnargs)                                 \
	if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufcx, 1,   \
	    player, cause, caller, cargs, ncargs))                              \
		return;

#define mvarargs_preamble(xname,xminargs,xnargs)                        \
	if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufcx, 0,   \
	    player, cause, caller, cargs, ncargs))                              \
		return;

#define evarargs_preamble2(xname,xnargs,xnargs2)                        \
	if (!fn_range_check(xname, nfargs, xnargs, xnargs2, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufcx, 1,   \
	    player, cause, caller, cargs, ncargs))                              \
		return;

#define svarargs_preamble(xname,xnargs)                                 \
	if (!fn_range_check(xname, nfargs, xnargs-2, xnargs, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs-1, &sep, buff, bufcx, 0,        \
    			 player, cause, caller, cargs, ncargs))                 \
		return;                                                 \
	if (nfargs < xnargs)                                            \
		osep = sep;                                             \
	else if (!delim_check(fargs, nfargs, xnargs, &osep, buff, bufcx, 0,    \
    		 player, cause, caller, cargs, ncargs))                         \
		return;

#define sevarargs_preamble(xname,xnargs)                                 \
	if (!fn_range_check(xname, nfargs, xnargs-2, xnargs, buff, bufcx))     \
		return;                                                 \
	if (!delim_check(fargs, nfargs, xnargs-1, &sep, buff, bufcx, 1,        \
    			 player, cause, caller, cargs, ncargs))                 \
		return;                                                 \
	if (nfargs < xnargs)                                            \
		osep = sep;                                             \
	else if (!delim_check(fargs, nfargs, xnargs, &osep, buff, bufcx, 1,    \
    		 player, cause, caller, cargs, ncargs))                         \
		return;

/* Utility function prototypes */
char * trim_space_sep(char *, char);
double safe_atof(char *);
char *upcasestr(char *);
char *next_token(char *, char);
char * split_token(char **, char);
int fn_range_check_real(const char *, int, int, int, char *, char **, int);
int delim_check(char *[], int, int, char *, char *, char **, int, dbref, dbref, dbref, char *[], int);
int list2arr(char *arr[], int maxlen, char *list, char sep);
#define fn_range_check(a, b, c, d, e, f)	fn_range_check_real(a, b, c, d, e, f, 0)

#endif
