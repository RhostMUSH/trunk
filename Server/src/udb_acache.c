/*
	Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
	Slightly whacked by Andrew Molitor for use with MUSH.
	My apologies to the author..
*/

/*
#define CACHE_DEBUG
#define CACHE_VERBOSE
*/
/* First */
#include	"autoconf.h"
#include	"udb_defs.h"
#include	"mudconf.h"
#include	"externs.h"
#ifdef	NOSYSTYPES_H
#include	<types.h>
#else
#include	<sys/types.h>
#endif

extern unsigned int	attr_hash(Aname *, int);
extern int		attrfree(Attr *);
extern int		dddb_check(Aname *);
extern int		dddb_del(Aname *, int);
extern Attr *		dddb_get(Aname *);
extern int		dddb_put(Attr *, Aname *);
extern void		fatal(const char *,...);
extern void		mush_logf(const char *,...);

/*
This is by far the most complex and kinky code in UnterMUD. You should
never need to mess with anything in here - if you value your sanity.
*/


typedef	struct	cache	{
	Aname	onm;
	Attr	*op;
	struct	cache	*nxt;
	struct	cache	*prv;
} Cache;

typedef struct {
	Cache	*head;
	Cache	*tail;
} Chain;

typedef	struct	{
	Chain	active;
	Chain	mactive;
	Chain	old;
	Chain	mold;
	int	count;
} CacheLst;

#define NAMECMP(x, y) ((x->onm.object == y->object) && (x->onm.attrnum == y->attrnum))

#define DEQUEUE(q, e)	if(e->nxt == CNULL) \
				q.tail = e->prv; \
			else \
				e->nxt->prv = e->prv; \
			if(e->prv == CNULL) \
				q.head = e->nxt; \
			else \
				e->prv->nxt = e->nxt;

#define INSHEAD(q, e)	if (q.head == CNULL) { \
				q.tail = e; \
				e->nxt = CNULL; \
			} else { \
				q.head->prv = e; \
				e->nxt = q.head; \
			} \
			q.head = e; \
			e->prv = CNULL;

#define INSTAIL(q, e)	if (q.head == CNULL) { \
				q.head = q.tail = e; \
				e->prv = CNULL; \
			} else { \
				q.tail->nxt = e; \
				e->prv = q.tail; \
			} \
			q.tail = e; \
			e->nxt = CNULL;

static Cache *	get_free_entry(CacheLst *);

/* initial settings for cache sizes */
static	int	cwidth = CACHE_WIDTH;
static	int	cdepth = CACHE_DEPTH;


/* ntbfw - main cache pointer and list of things to kill off */
static	CacheLst	*sys_c;

static	int	cache_initted = 0;
static	int	cache_frozen = 0;
static	int	cache_busy = 0;

/* cache stats gathering stuff. you don't like it? comment it out */
time_t	cs_ltime;
int	cs_writes = 0;	/* total writes */
int	cs_reads = 0;	/* total reads */
int	cs_dbreads = 0;	/* total read-throughs */
int	cs_dbwrites = 0;	/* total write-throughs */
int	cs_dels = 0;		/* total deletes */
int	cs_checks = 0;	/* total checks */
int	cs_rhits = 0;	/* total reads filled from cache */
int	cs_ahits = 0;	/* total reads filled active cache */
int	cs_whits = 0;	/* total writes to dirty cache */
int	cs_fails = 0;	/* attempts to grab nonexistent */
int	cs_resets = 0;	/* total cache resets */
int	cs_syncs = 0;	/* total cache syncs */
int	cs_objects = 0;	/* total cache size */


/* Replace a cache element */

void cache_repl (Cache *cp, char *new)
{
	if (cp->op != ANULL)
		attrfree(cp->op);
	cp->op = new;
}

int
cache_init(int width, int depth)
{
int	x;
int	y;
Cache	*np, *cp;
CacheLst *sp;
static const char *ncmsg = "cache_init: cannot allocate cache: ";


	if(cache_initted || sys_c != (CacheLst *)0)
		return(0);

	/* If either of these is non-zero, change cache dimension,	*/
	/* otherwise leave it at the default size.			*/

	if(width)
		cwidth = width;
	if(depth)
		cdepth = depth;

	sp = sys_c = (CacheLst *)malloc((unsigned)cwidth * sizeof(CacheLst));
	if(sys_c == (CacheLst *)0) {
		mush_logf(ncmsg,(char *)-1,"\n",(char *)0);
		return(-1);
	}

	/* Pre-allocate the initial cache entries in one chunk */
	cp = (Cache *)malloc(cwidth * cdepth * sizeof(Cache));
	if (cp == (Cache *)0) {
		mush_logf(ncmsg,(char *)-1,"\n",(char *)0);
		return(-1);
	}

	for(x = 0; x < cwidth; x++, sp++) {
		sp->active.head = sp->old.head = CNULL;
		sp->old.tail = sp->old.tail = CNULL;
		sp->mactive.head = sp->mold.head = CNULL;
		sp->mactive.tail = sp->mold.tail = CNULL;
		sp->count = cdepth;

		for(y = 0; y < cdepth; y++) {

			np = cp++;
			if((np->nxt = sp->old.head) != CNULL)
				np->nxt->prv = np;
			else
				sp->old.tail = np;

			np->prv = CNULL;
			np->onm.object = -1;
			np->op = ANULL;

			sp->old.head = np;
			cs_objects++;
		}
	}

	/* mark caching system live */
	cache_initted++;

	time(&cs_ltime);

	return(0);
}


static void
cache_trim(CacheLst *sp)
{
	Cache *cp;

	while ((sp->count > cdepth) && ((cp = sp->old.tail) != CNULL)) {
		cp->prv->nxt = CNULL;
		sp->old.tail = cp->prv;
		cache_repl(cp, ANULL);
		free(cp);
		sp->count--;
		cs_objects--;
	}
}

void
cache_reset(int trim)
{
	int	x;
	CacheLst *sp;

	if(!cache_initted)
		return;

	/* unchain and rechain each active list at head of old chain */
	sp = sys_c;
	for(x = 0; x < cwidth; x++, sp++) {
		if(sp->active.head != CNULL) {
			sp->active.tail->nxt = sp->old.head;

			if(sp->old.head != CNULL)
				sp->old.head->prv = sp->active.tail;
			if (sp->old.tail == CNULL)
				sp->old.tail = sp->active.tail;

			sp->old.head = sp->active.head;
			sp->active.head = CNULL;
		}
		if(!cache_busy && sp->mactive.head != CNULL) {
			sp->mactive.tail->nxt = sp->mold.head;

			if(sp->mold.head != CNULL)
				sp->mold.head->prv = sp->mactive.tail;
			if (sp->mold.tail == CNULL)
				sp->mold.tail = sp->mactive.tail;

			sp->mold.head = sp->mactive.head;
			sp->mactive.head = CNULL;
		}
		if (trim && mudconf.cache_trim)
			cache_trim(sp);
	}
	cs_resets++;
}




/*
search the cache for an object, and if it is not found, thaw it.
this code is probably a little bigger than it needs be because I
had fun and unrolled all the pointer juggling inline.
*/
Attr	*
cache_get(Aname *nam)
{
	Cache		*cp;
	CacheLst	*sp;
	int		hv = 0;
	Attr		*ret;

	/* firewall */
	if(nam == NNULL || !cache_initted) {
#ifdef	CACHE_VERBOSE
		mush_logf("cache_get: NULL object name - programmer error\n",(char *)0);
#endif
		return(ANULL);
	}

#ifdef	CACHE_DEBUG
	printf("get %s\n",nam);
#endif

	cs_reads++;

	hv = attr_hash(nam,cwidth);
	sp = &sys_c[hv];

	/* search active chain first */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			cs_rhits++;
			cs_ahits++;
#ifdef	CACHE_DEBUG
			printf("return %d,%d active cache %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			return(cp->op);
		}
	}

	/* search modified active chain next. */
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			cs_rhits++;
			cs_ahits++;

#ifdef	CACHE_DEBUG
			printf("return %d,%d modified active cache %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			return(cp->op);
		}
	}

	/* search in-active chain now */
	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {

			/* dechain from in-active chain */
			DEQUEUE(sp->old, cp);
			/* insert at head of active chain */
			INSHEAD(sp->active, cp);

			/* done */
			cs_rhits++;
#ifdef	CACHE_DEBUG
			printf("return %d,%d old cache %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			return(cp->op);
		}
	}

	/* search modified in-active chain now */
	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {

			/* dechain from modified in-active chain */
			DEQUEUE(sp->mold, cp);

			/* insert at head of modified active chain */
			INSHEAD(sp->mactive, cp);

			/* done */
			cs_rhits++;

#ifdef	CACHE_DEBUG
			printf("return %d,%d modified old cache %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			return(cp->op);
		}
	}

	/* DARN IT - at this point we have a certified, type-A cache miss */

	/* thaw the object from wherever. */
	/* If it's not there, save this fact too. */

	ret = DB_GET(nam);
	if(ret == ANULL) {
		cs_fails++;
		cs_dbreads++;
#ifdef	CACHE_DEBUG
		printf("%s not in db\n",nam);
#endif
	} else
		cs_dbreads++;

	cp = get_free_entry(sp);
	if (cp == CNULL) {
		log_db_err(nam->object, nam->attrnum,
			"allocate cache entry for");
		return(ANULL);
	}

	cp->op = ret;
	cp->onm = *nam;

	/* relink at head of active chain */
	INSHEAD(sp->active, cp);

#ifdef	CACHE_DEBUG
	printf("return %d,%d loaded into cache %d\n",
	       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
	return(ret);
}




/*
put an object back into the cache. this is complicated by the
fact that when a function calls this with an object, the object
is *already* in the cache, since calling functions operate on
pointers to the cached objects, and may or may not be actively
modifying them. in other words, by the time the object is handed
to cache_put, it has probably already been modified, and the cached
version probably already reflects those modifications!

so - we do a couple of things: we make sure that the cached
object is actually there, and set its dirty bit. if we can't
find it - either we have a (major) programming error, or the
*name* of the object has been changed, or the object is a totally
new creation someone made and is inserting into the world.

in the case of totally new creations, we simply accept the pointer
to the object and add it into our environment. freeing it becomes
the responsibility of the cache code. DO NOT HAND A POINTER TO
CACHE_PUT AND THEN FREE IT YOURSELF!!!!

There are other sticky issues about changing the object pointers
of MUDs and their names. This is left as an exercise for the
reader.
*/
int
cache_put(Aname *nam, Attr *obj)
{
	Cache	*cp;
	CacheLst *sp;
	int	hv = 0;

	/* firewall */
	if(obj == ANULL || nam == NNULL || !cache_initted) {
#ifdef	CACHE_VERBOSE
		mush_logf("cache_put: NULL object/name - programmer error\n",(char *)0);
#endif
		return(1);
	}

	cs_writes++;

	/* generate hash */
	hv = attr_hash(nam, cwidth);
	sp = &sys_c[hv];

	/* step one, search active chain, and if we find the obj, dirty it */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if(cp->op != obj) {
				cache_repl(cp, obj);
			}

#ifdef	CACHE_DEBUG
			printf("cache_put %d,%d %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			DEQUEUE(sp->active, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			return(0);
		}
	}

	/* step two, search modified active chain, and if we find the obj, we're done */
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if(cp->op != obj)
				cache_repl(cp, obj);

#ifdef	CACHE_DEBUG
			printf("cache_put %d,%d %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			cs_whits++;
			return(0);
		}
	}

	/* step three, search in-active chain.  If we find it, move it to the mod active */
	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if(cp->op != obj)
				cache_repl(cp, obj);

#ifdef	CACHE_DEBUG
			printf("cache_put %d,%d %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			DEQUEUE(sp->old, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			return(0);
		}
	}

	/* step four, search modified in-active chain */
	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {
			if(cp->op != obj)
				cache_repl(cp, obj);
#ifdef	CACHE_DEBUG
			printf("cache_put %d,%d %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			DEQUEUE(sp->mold, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			return(0);
		}
	}

	/* add it to the cache */
	cp = get_free_entry(sp);
	if (cp == CNULL) {
		log_db_err(nam->object, nam->attrnum,
			"allocate cache entry for");
		return(0);
	}

	cp->op = obj;
	cp->onm = *nam;

	/* link at head of modified active chain */
	INSHEAD(sp->mactive, cp);

#ifdef	CACHE_DEBUG
	printf("cache_put %d,%d new in cache %d\n",cp->onm.object,cp->onm.attrnum,cp->op);
#endif

	/* e finito ! */
	return(0);
}

static Cache *get_free_entry(CacheLst *sp)
{
	Cache *cp;

	/* if there are no old cache object holders left, allocate one */
	if((cp = sp->old.tail) == CNULL) {

		if (!cache_busy && !cache_frozen && mudconf.cache_steal_dirty &&
		    (cp = sp->mold.tail) != CNULL) {
			/* unlink old cache chain tail */
			if(cp->prv != CNULL) {
				sp->mold.tail = cp->prv;
				cp->prv->nxt = cp->nxt;
			} else	/* took last one */
				sp->mold.head = sp->mold.tail = CNULL;

#ifdef	CACHE_DEBUG
			printf("clean %d,%d from cache %d\n",
			       cp->onm.object,cp->onm.attrnum,cp->op);
#endif
			if (cp->op == ANULL) {
				if (DB_DEL(&cp->onm, 0)) {
					log_db_err(cp->onm.object,
						cp->onm.attrnum, "delete");
					return(CNULL);
				}
				cs_dels++;
			} else {
				if(DB_PUT(cp->op,&cp->onm)) {
					log_db_err(cp->onm.object,
						 cp->onm.attrnum, "write");
					return(CNULL);
				}
				cs_dbwrites++;
			}
		} else {
			if((cp = (Cache *)malloc(sizeof(Cache))) == CNULL)
				fatal("cache get_free_entry: malloc failed",(char *)-1,(char *)0);

			cp->op = ANULL;
			sp->count++;
			cs_objects++;
		}
	} else {

		/* unlink old cache chain tail */
		if(cp->prv != CNULL) {
			sp->old.tail = cp->prv;
			cp->prv->nxt = cp->nxt;
		} else	/* took last one */
			sp->old.head = sp->old.tail = CNULL;
	}

	/* free object's data */
	cache_repl(cp, NULL);
	return(cp);
}

static int cache_write(Cache *cp)
{
	while (cp != CNULL) {
#ifdef	CACHE_DEBUG
		printf("sync %d,%d -- %d\n",cp->onm.object,cp->onm.attrnum,
			cp->op;);
#endif
		if (cp->op == ANULL) {
			if (DB_DEL(&cp->onm, 0)) {
				log_db_err(cp->onm.object, cp->onm.attrnum,
					"delete");
				return(1);
			}
			cs_dels++;
		} else {
			if(DB_PUT(cp->op,&cp->onm)) {
				log_db_err(cp->onm.object, cp->onm.attrnum,
					"write");
				return(1);
			}
			cs_dbwrites++;
		}
		cp = cp->nxt;
	}
	return(0);
}

static void
cache_clean(CacheLst *sp)
{
	/* move the modified inactive chain to the inactive chain */
	if (sp->mold.head != CNULL) {
		if (sp->old.head == CNULL) {
			sp->old.head = sp->mold.head;
			sp->old.tail = sp->mold.tail;
		} else {
			sp->old.tail->nxt = sp->mold.head;
			sp->mold.head->prv = sp->old.tail;
			sp->old.tail = sp->mold.tail;
		}
		sp->mold.head = sp->mold.tail = CNULL;
	}
	/* same for modified active chain */
	if (sp->mactive.head != CNULL) {
		if (sp->active.head == CNULL) {
			sp->active.head = sp->mactive.head;
			sp->active.tail = sp->mactive.tail;
		} else {
			sp->active.tail->nxt = sp->mactive.head;
			sp->mactive.head->prv = sp->active.tail;
			sp->active.tail = sp->mactive.tail;
		}
		sp->mactive.head = sp->mactive.tail = CNULL;
	}
}

int cache_sync(void)
{
int	x;
CacheLst *sp;

	cs_syncs++;

	if(!cache_initted)
		return(1);

	if (cache_frozen)
		return(0);

	cache_reset(0);

	for(x = 0, sp = sys_c; x < cwidth; x++, sp++) {
		if (cache_write(sp->mold.head))
			return(1);
		if (cache_write(sp->mactive.head))
			return(1);
		cache_clean(sp);
		if (mudconf.cache_trim)
			cache_trim(sp);
	}
	return(0);
}

/*
probe the cache and the database (if needed) for the existence of an
object. return nonzero if the object is in cache or database
*/
int cache_check(Aname *nam)
{
	Cache	*cp;
	CacheLst *sp;
	int	hv = 0;

	if(nam == NNULL || !cache_initted)
		return(0);

	cs_checks++;

	hv = attr_hash(nam, cwidth);
	sp = &sys_c[hv];

	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt)
		if(NAMECMP(cp,nam))
			return(1);

	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt)
		if(NAMECMP(cp,nam))
			return(1);

	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt)
		if(NAMECMP(cp,nam))
			return(1);

	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt)
		if(NAMECMP(cp,nam))
			return(1);

	/* no ? */
	return(DB_CHECK(nam));
}

/*
Mark this attr as deleted in the cache.
*/
void
cache_del(Aname *nam)
{
	Cache	*cp;
	CacheLst *sp;
	int	hv = 0;

	if(nam == NNULL || !cache_initted)
		return;

	cs_dels++;

	hv = attr_hash(nam ,cwidth);
	sp = &sys_c[hv];

	/* mark dead in cache */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->active, cp);
			INSHEAD(sp->mactive, cp);
			cache_repl(cp, ANULL);
			return;
		}
	}
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			cache_repl(cp, ANULL);
			return;
		}
	}

	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->old, cp);
			INSHEAD(sp->mactive, cp);
			cache_repl(cp, ANULL);
			return;
		}
	}
	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->mold, cp);
			INSHEAD(sp->mactive, cp);
			cache_repl(cp, ANULL);
			return;
		}
	}
	cp = get_free_entry(sp);
	if (cp == CNULL) {
		log_db_err(nam->object, nam->attrnum,
			"allocate cache entry for");
		return;
	}

	cp->onm = *nam;
	cp->op = ANULL;

	INSHEAD(sp->mactive, cp);
	return;
}
