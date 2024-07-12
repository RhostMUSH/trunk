/* config.h */

#ifndef CONFIG_H
#define CONFIG_H

#include "copyright.h"

/* TEST_MALLOC:	Defining this makes a malloc that keeps track of the number
 *		of blocks allocated.  Good for testing for Memory leaks.
 * MSTATS:	Defining the following allows you to get stats and usage info
 *		if you use the gnu-malloc package.  It can be used in
 *		conjunction with the above, but why?
 * ATR_NAME:	Define if you want name to be stored as an attribute on the
 *		object rather than in the object structure.
 * USE_QDBM:	Define if you plan on using the disk based database scheme.
 *		The code will not work if this is not #defined.
 * rcheck:	Define if you are a) using the gnu_malloc package, and b)
 *		want to enable it's memory range checking features.  Useful for
 *		helping to track memory leaks.
 */

/* Compile time options */

#define CONF_FILE "Rhostshyl.conf"	/* Default config file */
#define FILEDIR "files/"		/* Source for @cat */

/* #define TEST_MALLOC */		/* Keep track of block allocs */
/* #define MSTATS */			/* Gnu-malloc stats */
#define USE_QDBM			/* Store text in disk-based db */
/* #define rcheck */			/* Gnu memory range checking */

#define PLAYER_NAME_LIMIT	22	/* Max length for player names */
#define NUM_ENV_VARS		10	/* Number of env vars (%0 et al.) */
#define MAX_ARG			100	/* max # args from command processor */
#ifdef EXPANDED_QREGS
#define MAX_GLOBAL_REGS		36	/* r() registers */
#else
#define MAX_GLOBAL_REGS		10	/* r() registers */
#endif

/* 
 * This option will requiure either penn_setq enabled or by using the
 * +/! options to setq to specify 'next in line' for setting labels.
 *
 * The only way to access these additional registers is with labels
 * Default behavior is '0' for no additional registers
 *
 * The expected overhead for each addition register is:
 *    (LBUF_SIZE * 2) + SBUF_SIZE
 *
 * This has been tested vigeriously with upwards to 10k extra.
 * The overhead in setting all 10k registers took 3-5 seconds.
 * After that, using them was as fast as normal.
 *
 * I would suggest keeping the value under a few hundred due to overhead.
 *
 * Keep in mind extra overhead in the stack with queued processing.
 *
 * The default value for this is 0
 */
#ifdef DYN_MAX_GLOBAL_BOOST
#define MAX_GLOBAL_BOOST	DYN_MAX_GLOBAL_BOOST
#else
#define MAX_GLOBAL_BOOST	0    	/* Boost the q-regs */
#endif

/* 
 * ATRCACHE_MAX is the global buffers that can be assigned for
 * function evaluation.  Generally 200 is more than enough but
 * on games that you may want more, you are welcome to use this.
 *
 * Keep in mind that expected overhead for atrcaches are:
 *    (LBUF_SIZE * 2) + SBUF_SIZE
 *
 * The default value for this is 200
 */
#ifdef DYN_ATRCACHE_MAX
#define ATRCACHE_MAX		DYN_ATRCACHE_MAX
#else
#define ATRCACHE_MAX		200 	/* @atrcache max possible */
#endif

/* ---------------------------------------------------------------------------
 * Database R/W flags.
 */

#define MANDFLAGS	(V_LINK|V_PARENT|V_XFLAGS)	/* Everyone has these */
#define OFLAGS1		(V_GDBM|V_ATRKEY)	/* QDBM has these */

#define OFLAGS2		(V_ATRNAME|V_ATRMONEY)

#define OUTPUT_VERSION	7			/* Version 6 */
#define OUTPUT_FLAGS	(MANDFLAGS|OFLAGS1|OFLAGS2)
						/* format for dumps */
#define UNLOAD_VERSION	7			/* version for export */
#define UNLOAD_OUTFLAGS	(MANDFLAGS)		/* format for export */

/* magic lock cookies */
#define NOT_TOKEN	'!'
#define AND_TOKEN	'&'
#define OR_TOKEN	'|'
#define LOOKUP_TOKEN	'*'
#define NUMBER_TOKEN	'#'
#define INDIR_TOKEN	'@'		/* One of these two should go. */
#define CARRY_TOKEN	'+'		/* One of these two should go. */
#define IS_TOKEN	'='
#define OWNER_TOKEN	'$'

/* matching attribute tokens */
#define AMATCH_CMD	'$'
#define AMATCH_LISTEN	'^'

/* delimiters for various things */
#define EXIT_DELIMITER	';'
#define ARG_DELIMITER	'='
#define ARG_LIST_DELIM	','

/* These chars get replaced by the current item from a list in commands and
 * functions that do iterative replacement, such as @apply_marked, dolist,
 * the eval= operator for @search, and iter().
 */

#define BOUND_VAR	"##"
#define NUMERIC_VAR	"#@"

/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)/mudconf.sacfactor) +mudconf.sacadjust)

/* !!! added for recycling, return value of object */
#define OBJECT_DEPOSIT(pennies) \
    (((pennies)-mudconf.sacadjust)*mudconf.sacfactor)


#ifdef VMS
#define unlink delete
#define gmtime localtime
#define DEV_NULL "NL:"
#define READ socket_read
#define WRITE socket_write
#else
#define DEV_NULL "/dev/null"
#define READ read
#define WRITE write
#endif

#ifdef BRAIN_DAMAGE		/* a kludge to get it to work on a mutant
				 * DENIX system */
#undef toupper
#endif

#ifdef TEST_MALLOC
extern int malloc_count;
#define XMALLOC(x,y) (fprintf(stderr,"Malloc: %s\n", (y)), malloc_count++, \
                    (char *)malloc((x)))
#define XFREE(x,y) (fprintf(stderr, "Free: %s\n", (y)), \
                    ((x) ? malloc_count--, free((x)), (x)=NULL : (x)))
#else
#define XMALLOC(x,y) (char *)malloc((x))
#define XFREE(x,y) (free((x)), (x) = NULL)
#endif  /* TEST_MALLOC */

#ifdef MSTATS
struct mstats_value
{
  int blocksize;
  int nfree;
  int nused;
};
#endif  /* MSTATS */

#define MA_CACHE_WIDTH	4	/* Number of leaves in a MA cache entry */

#endif	/* CONFIG_H */
