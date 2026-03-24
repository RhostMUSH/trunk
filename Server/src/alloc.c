/* alloc.c - memory allocation subsystem */

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "alloc.h"
#include "mudconf.h"
#include "externs.h"
#include "debug.h"
#include "ansi.h"

/* ensure quad byte divisible length to avoid bus error on some machines */
#define QUADALIGN(x) ((pmath1)(x) % ALLIGN1 ? \
                      (x) + ALLIGN1 - ((pmath1)(x) % ALLIGN1) : (x))

extern int global_timezone_max;

/* this structure size must be x % 4 = 0 */
typedef struct pool_header {
    int magicnum;		/* For consistency check */
    int pool_size;		/* For consistency check */
    struct pool_header *next;	/* Next pool header in chain */
    struct pool_header *nxtfree;	/* Next pool header in freelist */
    char *buf_tag;		/* Debugging/trace tag */
    char *filename;		/* Filename of tag */
    int linenum;		/* Line number of tag */
} POOLHDR;

typedef struct pool_footer {
    int magicnum;               /* For consistency check */
} POOLFTR;

typedef struct pooldata {
    int pool_size;		/* Size in bytes of a buffer */
    POOLHDR *free_head;		/* Buffer freelist head */
    POOLHDR *chain_head;	/* Buffer chain head */
    double tot_alloc;		/* Total buffers allocated */
    double num_alloc;		/* Number of buffers currently allocated */
    double max_alloc;		/* Max # buffers allocated at one time */
    double num_lost;		/* Buffers lost due to corruption */
} POOL;

POOL pools[NUM_POOLS];
const char *poolnames[] =
{"Sbufs", "Mbufs", "Lbufs", "Bools", "Descs", "Qentries", "Pcaches", "ZListNodes", "AtrCache", "AtrName"};

#define POOL_MAGICNUM 0xdeadbeef

int 
rtpool()
{
    return pools[2].num_alloc;
}

void 
pool_init(int poolnum, int poolsize)
{
    pools[poolnum].pool_size = poolsize;
    pools[poolnum].free_head = NULL;
    pools[poolnum].chain_head = NULL;
    pools[poolnum].max_alloc = 0;
    pools[poolnum].num_alloc = 0;
    pools[poolnum].tot_alloc = 0;
    pools[poolnum].num_lost = 0;
    return;
}

static void 
pool_err(const char *logsys, int logflag, int poolnum,
	 const char *tag, POOLHDR * ph, const char *action,
	 const char *reason, int line_num, char *file_name)
{
    if (!mudstate.logging) {
	STARTLOG(logflag, logsys, "ALLOC")
	    sprintf(mudstate.buffer,
#ifdef BIT64
		    "%.10s[%d] (tag %.40s) %.50s at %lx (line %d of %.20s).",
#else
		    "%.10s[%d] (tag %.40s) %.50s at %x (line %d of %.20s).",
#endif
		    action, pools[poolnum].pool_size, tag, reason,
		    (pmath1) ph, line_num, file_name);
	log_text(mudstate.buffer);
	ENDLOG
    } else if (logflag != LOG_ALLOCATE) {
	sprintf(mudstate.buffer,
#ifdef BIT64
		"\r\n***< %.10s[%d] (tag %.40s) %.50s at %lx (line %d of %.20s). >***",
#else
		"\r\n***< %.10s[%d] (tag %.40s) %.50s at %x (line %d of %.20s). >***",
#endif
		action, pools[poolnum].pool_size, tag, reason,
		(pmath1) ph, line_num, file_name);
	log_text(mudstate.buffer);
    }
}

static void 
pool_vfy(int poolnum, const char *tag, int line_num, const char *file_name)
{
    POOLHDR *ph, *lastph;
    POOLFTR *pf;
    char *h;
    int psize;

    lastph = NULL;
    psize = pools[poolnum].pool_size;
    for (ph = pools[poolnum].chain_head; ph; lastph = ph, ph = ph->next) {
	h = (char *) ph;
	h += sizeof(POOLHDR);
	h += pools[poolnum].pool_size;
        h = QUADALIGN(h);
	pf = (POOLFTR *) h;

	if (ph->magicnum != POOL_MAGICNUM) {
	    pool_err("BUG", LOG_ALWAYS, poolnum, tag, ph,
		     "Verify", "header corrupted (clearing freelist)", line_num, (char *)file_name);

	    /* Break the header chain at this point so we don't
	     * generate an error for EVERY alloc and free,
	     * also we can't continue the scan because the next
	     * pointer might be trash too.
	     */

	    if (lastph)
		lastph->next = NULL;
	    else
		pools[poolnum].chain_head = NULL;
	    return;		/* not safe to continue */
	}
	if (pf->magicnum != POOL_MAGICNUM) {
	    pool_err("BUG", LOG_ALWAYS, poolnum, tag, ph,
		     "Verify", "footer corrupted", line_num, (char *)file_name);
	    pf->magicnum = POOL_MAGICNUM;
	}
	if (ph->pool_size != psize) {
	    pool_err("BUG", LOG_ALWAYS, poolnum, tag, ph,
		     "Verify", "header has incorrect size", line_num, (char *)file_name);
	}
    }
}

void 
pool_check(const char *tag, int line_num, char *file_name)
{
    pool_vfy(POOL_LBUF, tag, line_num, file_name);
    pool_vfy(POOL_MBUF, tag, line_num, file_name);
    pool_vfy(POOL_SBUF, tag, line_num, file_name);
    pool_vfy(POOL_BOOL, tag, line_num, file_name);
    pool_vfy(POOL_DESC, tag, line_num, file_name);
    pool_vfy(POOL_QENTRY, tag, line_num, file_name);
    pool_vfy(POOL_ZLISTNODE, tag, line_num, file_name);
    pool_vfy(POOL_ATRCACHE, tag, line_num, file_name);
    pool_vfy(POOL_ATRNAME, tag, line_num, file_name);
}

char *
pool_alloc(int poolnum, const char *tag, int line_num, char *file_name)
{
    int *p;
    char *h;
    POOLHDR *ph;
    POOLFTR *pf;

    mudstate.allocsin++;
    if (mudconf.paranoid_alloc)
	pool_check(tag, line_num, file_name);
    do {
	if (pools[poolnum].free_head == NULL) {
            /* add 3 to size to allow for quad byte alignment */
	    h = (char *) malloc(3 + pools[poolnum].pool_size +
				sizeof(POOLHDR) + sizeof(POOLFTR));
	    if (h == NULL) {
		STARTLOG(LOG_ALWAYS, "MEM", "FATAL")
                  log_text((char *)"pool_alloc failed!");
	          sprintf(mudstate.buffer, " (line %d of file %s)", 
                          line_num, file_name);
                  log_text((char *)mudstate.buffer);
                ENDLOG
		abort();
            }
	    ph = (POOLHDR *) h;
	    h += sizeof(POOLHDR);
	    p = (int *) h;
	    h += pools[poolnum].pool_size;
            h = QUADALIGN(h);
	    pf = (POOLFTR *) h;

	    ph->next = pools[poolnum].chain_head;
	    ph->nxtfree = NULL;
	    ph->magicnum = POOL_MAGICNUM;
	    ph->pool_size = pools[poolnum].pool_size;
	    pf->magicnum = POOL_MAGICNUM;
	    *p = POOL_MAGICNUM;
	    pools[poolnum].chain_head = ph;
	    pools[poolnum].max_alloc++;
	} else {
	    ph = (POOLHDR *) (pools[poolnum].free_head);
	    h = (char *) ph;
	    h += sizeof(POOLHDR);
	    p = (int *) h;
	    h += pools[poolnum].pool_size;
            h = QUADALIGN(h);
	    pf = (POOLFTR *) h;
	    pools[poolnum].free_head = ph->nxtfree;

	    /* If corrupted header we need to throw away the
	     * freelist as the freelist pointer may be corrupt.
	     */

	    if (ph->magicnum != POOL_MAGICNUM) {
		pool_err("BUG", LOG_ALWAYS, poolnum, tag, ph,
			 "Alloc", "corrupted buffer header", line_num, file_name);

		/* Start a new free list and record stats */

		p = NULL;
		pools[poolnum].free_head = NULL;
		pools[poolnum].num_lost +=
		    (pools[poolnum].max_alloc -
		     pools[poolnum].num_alloc);
		pools[poolnum].max_alloc =
		    pools[poolnum].num_alloc;
	    }
	    /* Check for corrupted footer, just report and
	     * fix it */

	    if (pf->magicnum != POOL_MAGICNUM) {
		pool_err("BUG", LOG_ALWAYS, poolnum, tag, ph,
			 "Alloc", "corrupted buffer footer", line_num, file_name);
		pf->magicnum = POOL_MAGICNUM;
	    }
	}
    } while (p == NULL);

    ph->buf_tag = (char *) tag;
    ph->filename = (char *) file_name;
    ph->linenum = line_num;
    pools[poolnum].tot_alloc++;
    pools[poolnum].num_alloc++;

    pool_err("DBG", LOG_ALLOCATE, poolnum, tag, ph, "Alloc", "buffer", line_num, file_name);

    /* If the buffer was modified after it was last freed, log it. */

    if ((*p != POOL_MAGICNUM) && (!mudstate.logging)) {
	pool_err("BUG", LOG_PROBLEMS, poolnum, tag, ph, "Alloc",
		 "buffer modified after free", line_num, file_name);
    }
    *p = 0;
    return (char *) p;
}

int
getBufferSize(char *pBuff) 
{
    POOLHDR *ph;
    ph = (POOLHDR *) (pBuff - sizeof(POOLHDR));
    if (ph->magicnum != POOL_MAGICNUM) {
       return 0;
    } else {
       return ph->pool_size;
    }
}


void 
pool_free(int poolnum, char **buf, int line_num, char *file_name)
{
    int *ibuf;
    char *h;
    POOLHDR *ph;
    POOLFTR *pf;

    ibuf = (int *) *buf;
    h = (char *) ibuf;
    h -= sizeof(POOLHDR);
    ph = (POOLHDR *) h;
    h = (char *) ibuf;    
    h += pools[poolnum].pool_size;
    h = QUADALIGN(h);
    pf = (POOLFTR *) h;
    if (mudconf.paranoid_alloc)
	pool_check(ph->buf_tag, line_num, file_name);

    mudstate.allocsout++;
    /* Make sure the buffer header is good.  If it isn't, log the error and
     * throw away the buffer. */

    if (ph->magicnum != POOL_MAGICNUM) {
	pool_err("BUG", LOG_ALWAYS, poolnum, ph->buf_tag, ph, "Free",
		 "corrupted buffer header", line_num, file_name);
	pools[poolnum].num_lost++;
	pools[poolnum].num_alloc--;
	pools[poolnum].tot_alloc--;
	return;
    }
    /* Verify the buffer footer.  Don't unlink if damaged, just repair */

    if (pf->magicnum != POOL_MAGICNUM) {
	pool_err("BUG", LOG_ALWAYS, poolnum, ph->buf_tag, ph, "Free",
		 "corrupted buffer footer", line_num, file_name);
	pf->magicnum = POOL_MAGICNUM;
    }
    /* Verify that we are not trying to free someone else's buffer */

    if (ph->pool_size != pools[poolnum].pool_size) {
	pool_err("BUG", LOG_ALWAYS, poolnum, ph->buf_tag, ph, "Free",
		 "Attempt to free into a different pool.", line_num, file_name);
	return;
    }
    pool_err("DBG", LOG_ALLOCATE, poolnum, ph->buf_tag, ph, "Free",
	     "buffer", line_num, file_name);

    /* Make sure we aren't freeing an already free buffer.  If we are,
     * log an error, otherwise update the pool header and stats 
     */

    if (*ibuf == POOL_MAGICNUM) {
	pool_err("BUG", LOG_BUGS, poolnum, ph->buf_tag, ph, "Free",
		 "buffer already freed", line_num, file_name);
    } else {
	*ibuf = POOL_MAGICNUM;
	ph->nxtfree = pools[poolnum].free_head;
	pools[poolnum].free_head = ph;
	pools[poolnum].num_alloc--;
    }
}

static char *
pool_stats_extra(int poolnum, const char *text)
{
    char *buf;
    char format_str[80];

    buf = alloc_mbuf("pool_stats_extra");
    if ( !strcmp(text, "Lbufs") || !strcmp(text, "Sbufs") || !strcmp(text, "Mbufs") ) {
       memset(format_str, '\0', sizeof(format_str));
       strcpy(format_str, "%-14s %5d %9d %9d %15s %6s %14.14g");
       if ( !strcmp(text, "Lbufs") ) {
#ifndef NO_GLOBAL_REGBACKUP
          sprintf(buf, format_str, (char *)" \\Lbufs (Regs)", pools[poolnum].pool_size, 
                   ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2), 
                   ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2), 
                   (char *)"^", (char *)"^", 
                   pools[poolnum].pool_size * (double)((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2));
#else
          sprintf(buf, format_str, (char *)" \\Lbufs (Regs)", pools[poolnum].pool_size, 
                   ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1), 
                   ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1), 
                   (char *)"^", (char *)"^", 
                   pools[poolnum].pool_size * (double)((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1));
#endif
       } else if ( !strcmp(text, "Mbufs") ) {
          sprintf(buf, format_str, (char *)" \\Mbufs (TZ)", pools[poolnum].pool_size, 
                   global_timezone_max, global_timezone_max,
                   (char *)"^", (char *)"^", 
                   pools[poolnum].pool_size * (double)global_timezone_max);
       } else {
          sprintf(buf, format_str, (char *)" \\Sbufs (Regs)", pools[poolnum].pool_size,
                   (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST), 
                   (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST), 
                   (char *)"^", (char *)"^", 
                   pools[poolnum].pool_size * (double)(MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST));
       }
    } 
    return buf;
}

static char *
pool_stats(int poolnum, const char *text, int i_color)
{
    char *buf;
    char format_str[80], s_warn[20], s_norm[20];
    double i_check = 0;

    buf = alloc_mbuf("pool_stats");
    memset(format_str, '\0', sizeof(format_str));
    memset(s_warn, '\0', sizeof(s_warn));
    memset(s_norm, '\0', sizeof(s_norm));
    strcpy(format_str, "%-14.14s %5d");
    if ( pools[poolnum].num_alloc > 999999999 )
       strcat(format_str, " %s%9.4g%s");
    else
       strcat(format_str, " %s%9.9g%s");
    if ( pools[poolnum].max_alloc > 999999999 )
       strcat(format_str, " %9.4g");
    else
       strcat(format_str, " %9.9g");
    if ( pools[poolnum].tot_alloc > 9.99999e13 )
       strcat(format_str, " %15.7g");
    else
       strcat(format_str, " %15.15g");
    if ( pools[poolnum].num_lost > 999999)
       strcat(format_str, " %6.4g");
    else
       strcat(format_str, " %6.6g");
    if ( (pools[poolnum].max_alloc * pools[poolnum].pool_size) > 9.99999e13 )
       strcat(format_str, " %14.9g");
    else
       strcat(format_str, " %14.14g");

    if ( strcmp(text, "Lbufs") == 0 ) {
#ifndef NO_GLOBAL_REGBACKUP
       i_check = pools[poolnum].num_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2); 
#else
       i_check = pools[poolnum].num_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1); 
#endif
       if ( i_color ) {
          strcpy(s_norm, ANSI_GREEN);
          if ( i_check > 50 ) {
             strcpy(s_warn, ANSI_RED);
          } else if ( i_check > 20 ) {
             strcpy(s_warn, ANSI_YELLOW);
          }
       }
#ifndef NO_GLOBAL_REGBACKUP
       sprintf(buf, format_str, text, pools[poolnum].pool_size,
               s_warn, i_check, s_norm,
	       // pools[poolnum].num_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2), 
               pools[poolnum].max_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2),
	       pools[poolnum].tot_alloc, pools[poolnum].num_lost,
               (pools[poolnum].max_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2)) * pools[poolnum].pool_size);
#else
       sprintf(buf, format_str, text, pools[poolnum].pool_size,
               s_warn, i_check, s_norm,
	       // pools[poolnum].num_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2), 
               pools[poolnum].max_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1),
	       pools[poolnum].tot_alloc, pools[poolnum].num_lost,
               (pools[poolnum].max_alloc - ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1)) * pools[poolnum].pool_size);
#endif
    } else if ( strcmp(text, "Mbufs") == 0 ) {
       i_check = pools[poolnum].num_alloc - global_timezone_max; 
       if ( i_color ) {
          strcpy(s_norm, ANSI_GREEN);
          if ( i_check > 150 ) {
             strcpy(s_warn, ANSI_RED);
          } else if ( i_check > 100 ) {
             strcpy(s_warn, ANSI_YELLOW);
          }
       }
       sprintf(buf, format_str, text, pools[poolnum].pool_size,
               s_warn, i_check, s_norm,
	       // pools[poolnum].num_alloc - global_timezone_max, 
               pools[poolnum].max_alloc - global_timezone_max,
	       pools[poolnum].tot_alloc, pools[poolnum].num_lost,
               (pools[poolnum].max_alloc - global_timezone_max) * pools[poolnum].pool_size);
    } else if ( strcmp(text, "Sbufs") == 0 ) {
       i_check = pools[poolnum].num_alloc - (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); 
       if ( i_color ) {
          strcpy(s_norm, ANSI_GREEN);
          if ( i_check > 50 ) {
             strcpy(s_warn, ANSI_RED);
          } else if ( i_check > 20 ) {
             strcpy(s_warn, ANSI_YELLOW);
          }
       }
       sprintf(buf, format_str, text, pools[poolnum].pool_size,
               s_warn, i_check, s_norm,
	       // pools[poolnum].num_alloc - (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST), 
               pools[poolnum].max_alloc - (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST),
	       pools[poolnum].tot_alloc, pools[poolnum].num_lost,
               (pools[poolnum].max_alloc - (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST)) * pools[poolnum].pool_size);
    } else {
       sprintf(buf, format_str, text, pools[poolnum].pool_size,
	       s_warn, pools[poolnum].num_alloc, s_norm, pools[poolnum].max_alloc,
	       pools[poolnum].tot_alloc, pools[poolnum].num_lost,
               pools[poolnum].max_alloc * pools[poolnum].pool_size);
    }
    return buf;
}

static void 
pool_trace(dbref player, int poolnum, const char *text, int key)
{
    POOLHDR *ph;
    typedef struct tmp_holder {
              char buff_name[200];
              int i_cntr;
              struct tmp_holder *next;
           } THOLD;
    THOLD *st_holder = NULL, *st_ptr = NULL, *st_ptr2 = NULL;
    int numfree, *ibuf, icount;
    char *h, s_fieldbuff[250];

    numfree = icount = 0;
    notify(player, unsafe_tprintf("----- %s -----", text));
    for (ph = pools[poolnum].chain_head; ph != NULL; ph = ph->next) {
	if (ph->magicnum != POOL_MAGICNUM) {
	    notify(player, "*** CORRUPTED BUFFER HEADER, ABORTING SCAN ***");
	    notify(player,
		   unsafe_tprintf("%d free %s (before corruption)",
			   numfree, text));
            st_ptr = st_holder;
            while ( st_ptr ) {
               st_ptr2 = st_ptr->next;
               free(st_ptr);
               st_ptr = st_ptr2;
            }
	    return;
	}
	h = (char *) ph;
	h += sizeof(POOLHDR);
	ibuf = (int *) h;
	if (*ibuf != POOL_MAGICNUM) {
            if ( key ) {
               for (st_ptr = st_holder; st_ptr; st_ptr = st_ptr->next) {
                  sprintf(s_fieldbuff, "%.90s {%.90s:%d}", ph->buf_tag, ph->filename, ph->linenum);
                  if ( !strcmp( s_fieldbuff, st_ptr->buff_name) ) {
                     st_ptr->i_cntr++;
                     break;
                  }
               }
            } else {
               for (st_ptr = st_holder; st_ptr; st_ptr = st_ptr->next) {
                  if ( !strcmp( ph->buf_tag, st_ptr->buff_name) ) {
                     st_ptr->i_cntr++;
                     break;
                  }
               }
            }
            if ( !st_ptr ) {
               st_ptr2 = malloc(sizeof(THOLD));
               if ( key ) {
                  sprintf(st_ptr2->buff_name, "%.90s {%.90s:%d}", ph->buf_tag, ph->filename, ph->linenum);
               } else {
                  sprintf(st_ptr2->buff_name, "%.199s", ph->buf_tag);
               }
               st_ptr2->i_cntr = 1;
               st_ptr2->next = NULL;
               if ( !st_holder )
                  st_holder = st_ptr2;
               else {
                  for (st_ptr = st_holder; st_ptr && st_ptr->next; st_ptr = st_ptr->next);
                  st_ptr->next = st_ptr2;
               }
            }   
        } else
	    numfree++;
    }
    st_ptr = st_holder;
    while ( st_ptr ) {
       sprintf(s_fieldbuff, "%s  [%d entries]", st_ptr->buff_name, st_ptr->i_cntr);
       icount += st_ptr->i_cntr;
       notify(player, s_fieldbuff);
       st_ptr2 = st_ptr->next;
       free(st_ptr);
       st_ptr = st_ptr2;
    }
    notify(player, unsafe_tprintf("%d free %s, %d allocated", numfree, text, icount));
}

static void 
list_bufstat(dbref player, int poolnum, const char *pool_name, int i_color)
{
    char *buff, *colorbuff;

    buff = pool_stats(poolnum, poolnames[poolnum], i_color);
#ifdef ZENTY_ANSI
    if ( i_color ) {
       colorbuff = alloc_lbuf("list_bufstat");
       sprintf(colorbuff, "%s%s%s%s", ANSI_HILITE, ANSI_GREEN, buff, ANSI_NORMAL);
       notify(player, colorbuff);
       free_lbuf(colorbuff);
    } else {
       notify(player, buff);
    }
#else
    notify(player, buff);
#endif
    free_mbuf(buff);
    buff = pool_stats_extra(poolnum, poolnames[poolnum]);
    /* buff is always allocated */
    if ( *buff ) {
#ifdef ZENTY_ANSI
       if ( i_color ) {
          colorbuff = alloc_lbuf("list_bufstat");
          sprintf(colorbuff, "%s%s%s%s", ANSI_HILITE, ANSI_CYAN, buff, ANSI_NORMAL);
          notify(player, colorbuff);
          free_lbuf(colorbuff);
       } else {
          notify(player, buff);
       }
#else
       notify(player, buff);
#endif
    }
    free_mbuf(buff);
}

void
showTrackedPacketStats(dbref player) 
{
    char *buff;

    buff = unsafe_tprintf("\r\nNetwork Stats (Bytes)   Daily-In:     Average-In:   Total-In:\r\n"
                          "                        %-13.0f %-13.0f %-16.0f"
                          "\r\n\r\n                        Daily-Out:    Average-Out:  Total-Out:\r\n"
                          "                        %-13.0f %-13.0f %-16.0f",
                          mudstate.daily_bytesin, 
                         ((mudstate.avg_bytesin == 0) ? mudstate.daily_bytesin : mudstate.avg_bytesin), 
                          mudstate.total_bytesin,
                          mudstate.daily_bytesout, 
                         ((mudstate.avg_bytesout == 0) ? mudstate.daily_bytesout : mudstate.avg_bytesout), 
                          mudstate.total_bytesout);
   notify(player, buff);
}

double
statsizer(int i_size, char *c_chr) {
   double i_diver = 0.0;

   if ( i_size > 1000000000 ) {
      i_diver = (double) i_size / 1000000000.0;
      *c_chr = 'G';
   } else if ( i_size > 1000000 ) {
      i_diver = (double) i_size / 1000000.0;
      *c_chr = 'M';
   } else if ( i_size > 1000 ) {
      i_diver = (double) i_size / 1000.0;
      *c_chr = 'K';
   } else {
      i_diver = (double) i_size;
      *c_chr = ' ';
   }
   
   return i_diver; 
}

void
showBlacklistStats(dbref player)
{
   int i_blsize, i_ndsize, i_ngsize, i_rgsize;
   double i_diver;
   char *s_buff, c_chr[2];
   
   i_blsize = (int)sizeof(BLACKLIST) * mudstate.blacklist_cnt;
   i_ndsize = (int)sizeof(BLACKLIST) * mudstate.blacklist_nodns_cnt;
   i_rgsize = (int)sizeof(BLACKLIST) * mudstate.blacklist_reg_cnt;
   i_ngsize = (int)sizeof(BLACKLIST) * mudstate.blacklist_nogst_cnt;

   s_buff = alloc_mbuf("blacklist_stats");
   strcpy(c_chr, (char *)" ");
   
   notify(player, "\r\nBlacklist Stats    Size   Inuse     Total Mem (Bytes)");

   /* Black list */
   i_diver = statsizer(i_blsize, c_chr);
   sprintf(s_buff, "%-18s %-6d %-9d %-12d (%.2f%c)", 
           (char *)"Black List", (int)sizeof(BLACKLIST), mudstate.blacklist_cnt, i_blsize, i_diver, c_chr[0]);
   notify(player, s_buff);

   /* NoDNS list */
   i_diver = statsizer(i_ndsize, c_chr);
   sprintf(s_buff, "%-18s %-6d %-9d %-12d (%.2f%c)", 
           (char *)"NoDNS List", (int)sizeof(BLACKLIST), mudstate.blacklist_nodns_cnt, i_ndsize, i_diver, c_chr[0]);
   notify(player, s_buff);

   /* Register list */
   i_diver = statsizer(i_rgsize, c_chr);
   sprintf(s_buff, "%-18s %-6d %-9d %-12d (%.2f%c)", 
           (char *)"Regis List", (int)sizeof(BLACKLIST), mudstate.blacklist_nodns_cnt, i_ndsize, i_diver, c_chr[0]);
   notify(player, s_buff);

   /* NoGuest list */
   i_diver = statsizer(i_ndsize, c_chr);
   sprintf(s_buff, "%-18s %-6d %-9d %-12d (%.2f%c)", 
           (char *)"NoGst List", (int)sizeof(BLACKLIST), mudstate.blacklist_nodns_cnt, i_ndsize, i_diver, c_chr[0]);
   notify(player, s_buff);

   free_mbuf(s_buff); 
}

void
showTotemStats(dbref player)
{
  double i_tot;
  double i_show;
  char c_let;

  i_tot = (double)sizeof(OBJTOTEM) * (double)mudstate.db_top;
  if ( i_tot > 1000000000.0 ) {
     i_show = i_tot / 1000000000.0;
     c_let = 'G';
  } else if ( i_tot > 1000000.0 ) {
     i_show = i_tot / 1000000.0;
     c_let = 'M';
  } else  {
     i_show = i_tot / 1000.0;
     c_let = 'K';
  }
  notify(player, unsafe_tprintf("\r\nTotal overhead of @totems per dbref# -- %d", sizeof(OBJTOTEM)));
  notify(player, "    Size   Slots    Objects   Total Memory");
  notify(player, unsafe_tprintf("    %4d  %6d   %8d   %.0f (%.2f%c)", (sizeof(OBJTOTEM) / TOTEM_SLOTS), TOTEM_SLOTS, mudstate.db_top, i_tot, i_show, c_let));
}

void
showAtrCacheStats(dbref player)
{
  double i_tot;
  double i_show;
  char c_let;

  i_tot = (double)sizeof(ATRCACHE) * mudconf.atrcachemax;
  if ( i_tot > 1000000000.0 ) {
     i_show = i_tot / 1000000000.0;
     c_let = 'G';
  } else if ( i_tot > 1000000.0 ) {
     i_show = i_tot / 1000000.0;
     c_let = 'M';
  } else  {
     i_show = i_tot / 1000.0;
     c_let = 'K';
  }
  notify(player, "\r\nTotal overhead of Atr Caches: (Not Including AtrCache/AtrNames above)");
  notify(player, "    Size   Slots    Total Memory");
  notify(player, unsafe_tprintf("    %4d  %6d     %.0f (%.2f%c)", sizeof(ATRCACHE), mudconf.atrcachemax, i_tot, i_show, c_let));
}


extern int anum_alc_top;
extern int anum_alc_inline_top;

void
showAttrStats(dbref player)
{
   static char s_buff[80];
   int i_attr;
   double i_tot, i_show;
   char c_let;

   memset(s_buff, '\0', 80);
   if ( anum_alc_inline_top == 0 ) {
      i_attr = 255;
   } else {
      i_attr = 255 + anum_alc_inline_top - A_INLINE_START + 1;
   }
   i_tot = (double)i_attr * (double)sizeof(ATTR *);
   if ( i_tot > 1000000000.0 ) {
      i_show = i_tot / 1000000000.0;
      c_let = 'G';
   } else if ( i_tot > 1000000.0 ) {
      i_show = i_tot / 1000000.0;
      c_let = 'M';
   } else  {
      i_show = i_tot / 1000.0;
      c_let = 'K';
   }
   notify(player, "\r\nInline Attributes    Size     Total Memory");
   sprintf(s_buff,"%-10d           %-8d %.0f (%.2f%c)", i_attr, sizeof(ATTR *), i_tot, i_show, c_let);
   notify(player, s_buff);
}

const char *
tf_1(time_t dt)
{
    register struct tm *delta;
    static char buf[64];
    int i_syear;

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
    return(buf);
}

void
showCPUStats(dbref player)
{
   clock_t end;
   time_t now;
   double cpu_time;
   char *s_buff, *buff;

   end = clock();
   time(&now);

   buff = alloc_mbuf("showCPUStats_mbuf");
   strcpy(buff, tf_1(now - mudstate.reboot_time));

   s_buff = alloc_mbuf("showCPUStats");
   cpu_time = (double)(end - mudstate.clock_mush) / CLOCKS_PER_SEC;
   sprintf(s_buff, "\r\nTotal CPU Usage since last reboot: %.2f seconds, Reboot time: %s", cpu_time, buff);

   notify(player, s_buff);
   free_mbuf(buff);
   free_mbuf(s_buff);
}

void 
list_bufstats(dbref player, char *s_key)
{
    int i, i_found, i_color;

    i_found = 0; 
    i_color = 0;

    if ( s_key && *s_key && !strcasecmp(s_key, (char *)"cquick")) {
       i_color = 1;
    }

    if ( !s_key || !*s_key || (!strcasecmp(s_key, (char *)"quick") || 
                               !strcasecmp(s_key, (char *)"cquick")) ) {
       i_found = 1;
#ifdef ZENTY_ANSI
       if ( i_color ) {
          notify(player, unsafe_tprintf("%s%sBuffer Stats    Size     InUse     Total          Allocs   Lost      Total Mem%s",
                         ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL));
       } else {
          notify(player, "Buffer Stats    Size     InUse     Total          Allocs   Lost      Total Mem");
       }
#else
       notify(player, "Buffer Stats    Size     InUse     Total          Allocs   Lost      Total Mem");
#endif
       for (i = 0; i < NUM_POOLS; i++)
	   list_bufstat(player, i, poolnames[i], i_color);
    }

    if ( !s_key || !*s_key || (!strcasecmp(s_key, (char *)"cpu")) ) {
       i_found = 1;
       showCPUStats(player);
    }
    if ( !s_key || !*s_key || (!strcasecmp(s_key, (char *)"quick") ||
                               !strcasecmp(s_key, (char *)"cquick")) ) {
       i_found = 1;
       showTrackedBufferStats(player, i_color);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"blacklist") ) {
       i_found = 1;
       showBlacklistStats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"attrib") ) {
       i_found = 1;
       showAttrStats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"totem") ) {
       i_found = 1;
       showTotemStats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"cache") ) {
       i_found = 1;
       showAtrCacheStats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"network") ) {
       i_found = 1;
       showTrackedPacketStats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"db") ) {
       i_found = 1;
       showdbstats(player);
    }
    if ( !s_key || !*s_key || !strcasecmp(s_key, (char *)"stack") ) {
       i_found = 1;
#ifndef NO_GLOBAL_REGBACKUP
       notify(player, unsafe_tprintf("\r\nTotal Lbufs used in Q-Regs: %d", ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 2)));
#else
       notify(player, unsafe_tprintf("\r\nTotal Lbufs used in Q-Regs: %d", ((MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST) * 1)));
#endif
       notify(player, unsafe_tprintf("Total Sbufs used in Q-Regs: %d", (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST)));
#ifndef NODEBUGMONITOR
       notify(player, unsafe_tprintf("Highest debugmon stack depth was: %d", debugmem->stackval));
       notify(player, unsafe_tprintf("Current debugmon stack depth is: %d", debugmem->stacktop));
#else
       notify(player, "Debug Monitor is disabled.");
#endif
    }

    if ( !i_found && s_key && *s_key ) {
       notify(player, "Unknown sub-option for ALLOC.  Use one of: quick, blacklist, attrib, totem, cache, network, db, stack, cpu, or cquick.");
    }
}

void 
list_buftrace(dbref player, int key)
{
    int i;

    for (i = 0; i < NUM_POOLS; i++)
	pool_trace(player, i, poolnames[i], key);
}

#define BFACT 2
#define BMIN 10

void do_buff_free(dbref player, dbref cause, int key)
{
  int x, y, z;
  POOLHDR *ph;

  for (x = 0; x < NUM_POOLS; x++) {
    if (BFACT)
      y = pools[x].num_alloc + (pools[x].num_alloc / BFACT);
    else
      y = pools[x].num_alloc;
    if (!y)
      y = BMIN;
    if (y < pools[x].max_alloc) {
      for (z = 0; z < (pools[x].max_alloc - y); z++) {
        ph = (POOLHDR *)(pools[x].free_head);
	if (ph == NULL) 
	  break;
	pools[x].free_head = ph->nxtfree;
	if (ph->magicnum != POOL_MAGICNUM) {
	  pool_err("BUG", LOG_ALWAYS, x, "buff_free", ph, "Alloc", "corrupted buffer header", 
                   -1, (char *)"");
	  pools[x].free_head = NULL;
	  pools[x].num_lost += (pools[x].max_alloc - pools[x].num_alloc);
	  pools[x].max_alloc = pools[x].num_alloc;
	  notify(player,"Corrupted buffer header detected");
	  break;
	}
	free ((char *)ph);
	pools[x].max_alloc--;
      }
    }
  }
 notify(player,"Buff free: Done");
}
