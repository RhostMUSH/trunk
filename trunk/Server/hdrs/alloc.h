/* alloc.h - External definitions for memory allocation subsystem */

#include "copyright.h"

#ifndef M_ALLOC_H
#define M_ALLOC_H

#define	POOL_SBUF	0
#define	POOL_MBUF	1
#define	POOL_LBUF	2
#define	POOL_BOOL	3
#define	POOL_DESC	4
#define	POOL_QENTRY	5
#define POOL_PCACHE	6
#define POOL_ZLISTNODE  7
#define	NUM_POOLS       8

#define LBUF_SIZE	4000
#define MBUF_SIZE	200
#ifdef SBUF64
#define SBUF_SIZE	64
#else
#define SBUF_SIZE	32
#endif

#ifndef STANDALONE

extern void	FDECL(pool_init, (int, int));
extern char *	FDECL(pool_alloc, (int, const char *, int, char *));
extern void	FDECL(pool_free, (int, char **, int, char *));
extern void	FDECL(list_bufstats, (dbref));
extern void	FDECL(list_buftrace, (dbref));
extern void	FDECL(do_buff_free, (dbref, dbref, int));
extern int      FDECL(getBufferSize, (char *));

#define	alloc_lbuf(s)	pool_alloc(POOL_LBUF,s,__LINE__,__FILE__)
#define	alloc_lbuf_new(s,t,u)	pool_alloc(POOL_LBUF,s,t,u)
#define	free_lbuf(b)	pool_free(POOL_LBUF,((char **)&(b)),__LINE__,__FILE__)
#define	alloc_mbuf(s)	pool_alloc(POOL_MBUF,s,__LINE__,__FILE__)
#define	free_mbuf(b)	pool_free(POOL_MBUF,((char **)&(b)),__LINE__,__FILE__)
#define	alloc_sbuf(s)	pool_alloc(POOL_SBUF,s,__LINE__,__FILE__)
#define	free_sbuf(b)	pool_free(POOL_SBUF,((char **)&(b)),__LINE__,__FILE__)
#define	alloc_bool(s)	(struct boolexp *)pool_alloc(POOL_BOOL,s,__LINE__,__FILE__)
#define	free_bool(b)	pool_free(POOL_BOOL,((char **)&(b)),__LINE__,__FILE__)
#define	alloc_qentry(s)	(BQUE *)pool_alloc(POOL_QENTRY,s,__LINE__,__FILE__)
#define	free_qentry(b)	pool_free(POOL_QENTRY,((char **)&(b)),__LINE__,__FILE__)
#define alloc_pcache(s)	(PCACHE *)pool_alloc(POOL_PCACHE,s,__LINE__,__FILE__)
#define free_pcache(b)	pool_free(POOL_PCACHE,((char **)&(b)),__LINE__,__FILE__)
#define alloc_zlistnode(s)	(ZLISTNODE *)pool_alloc(POOL_ZLISTNODE,s,__LINE__,__FILE__)
#define free_zlistnode(b)	pool_free(POOL_ZLISTNODE,((char **)&(b)),__LINE__,__FILE__)

#else

#define	alloc_lbuf(s)	(char *)malloc(LBUF_SIZE)
#define	alloc_lbuf_new(s,t,u)	(char *)malloc(LBUF_SIZE)
#define	free_lbuf(b)	if (b) free(b)
#define	alloc_mbuf(s)	(char *)malloc(MBUF_SIZE)
#define	free_mbuf(b)	if (b) free(b)
#define	alloc_sbuf(s)	(char *)malloc(SBUF_SIZE)
#define	free_sbuf(b)	if (b) free(b)
#define	alloc_bool(s)	(struct boolexp *)malloc(sizeof(struct boolexp))
#define	free_bool(b)	if (b) free(b)
#define	alloc_qentry(s)	(BQUE *)malloc(sizeof(BQUE))
#define	free_qentry(b)	if (b) free(b)
#define	alloc_pcache(s)	(PCACHE *)malloc(sizeof(PCACHE))
#define free_pcache(b)	if (b) free(b)
#define	alloc_zlistnode(s)	(ZLISTNODE *)malloc(sizeof(ZLISTNODE))
#define free_zlistnode(b)	if (b) free(b)
#endif

#define	safe_str(s,b,p)		safe_copy_str(s,b,p,(LBUF_SIZE-2))
#define safe_strmax(s,b,p)	safe_copy_strmax(s,b,p,(LBUF_SIZE-8))
#define	safe_chr(c,b,p)		safe_copy_chr(c,b,p,(LBUF_SIZE-2))
#define	safe_sb_str(s,b,p)	safe_copy_str(s,b,p,(SBUF_SIZE-2))
#define	safe_sb_chr(c,b,p)	safe_copy_chr(c,b,p,(SBUF_SIZE-2))
#define	safe_mb_str(s,b,p)	safe_copy_str(s,b,p,(MBUF_SIZE-2))
#define	safe_mb_chr(c,b,p)	safe_copy_chr(c,b,p,(MBUF_SIZE-2))

typedef struct objlist_block OBLOCK;
struct objlist_block {
  struct objlist_block *next;
  dbref   data[(LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref)];
};

#define OBLOCK_SIZE ((LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref))

typedef struct objlist_blockmaster OBLOCKMASTER;
struct objlist_blockmaster {
  OBLOCK *olist_head;
  OBLOCK *olist_tail;
  OBLOCK *olist_cblock;
  int olist_count;
  int olist_citm;
};

#endif	/* M_ALLOC_H */
