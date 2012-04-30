/*
	Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
	Slightly whacked by Andrew Molitor for use with MUSH.
	My apologies to the author..

	Also banged on by dcm@somewhere.or.other, Jellan on the muds,
	to be more complex and more efficient in the context of
	MUSH.

	This version then banged on by Andrew Molitor, 1992, to
	do MUSH but object-by-object, hiding the object part inside
	the cache layer. Upper layers think it's an attribute cache.

*/

/*
#define CACHE_DEBUG
#define CACHE_VERBOSE
*/

/* First */
#include	"autoconf.h"
#include	"udb_defs.h"
#include	"mudconf.h"
#include        "debug.h"

#define  FILENUM UDB_OCACHE_C

#ifdef	NOSYSTYPES_H
#include	<types.h>
#else
#include	<sys/types.h>
#endif

extern struct Obj *dddb_get();
extern void mush_logf();

/* Lensy: Hacktastic fix to avoid warnings.
 *        whoever forced me to do this needs their balls removed
 */
extern PROTO_DB_DEL();
extern PROTO_DB_PUT();

extern void VDECL(fatal, (const char *,...));
extern void FDECL(log_db_err, (int, int, const char *));

/*
This is by far the most complex and kinky code in UnterMUD. You should
never need to mess with anything in here - if you value your sanity.
*/


typedef	struct	cache	{
	Obj	*op;
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

/* This is a MUSH specific definition, depending on Objname being an
	integer type of some sort. */

#define NAMECMP(a,b) ((a)->op) && (((a)->op)->name == (b)->object)

#define DEQUEUE(q, e)	if(e->nxt == (Cache *)0) \
				q.tail = e->prv; \
			else \
				e->nxt->prv = e->prv; \
			if(e->prv == (Cache *)0) \
				q.head = e->nxt; \
			else \
				e->prv->nxt = e->nxt;

#define INSHEAD(q, e)	if (q.head == (Cache *)0) { \
				q.tail = e; \
				e->nxt = (Cache *)0; \
			} else { \
				q.head->prv = e; \
				e->nxt = q.head; \
			} \
			q.head = e; \
			e->prv = (Cache *)0;

#define INSTAIL(q, e)	if (q.head == (Cache *)0) { \
				q.head = q.tail = e; \
				e->prv = (Cache *)0; \
			} else { \
				q.tail->nxt = e; \
				e->prv = q.tail; \
			} \
			q.tail = e; \
			e->nxt = (Cache *)0;

static Cache *get_free_entry();
static int cache_write();
static void cache_clean();

static Attr *get_attrib();
static void set_attrib();
static void del_attrib();
static void	FDECL(objfree, (Obj *));

/* initial settings for cache sizes */
static	int	cwidth;
static	int	cdepth;


/* ntbfw - main cache pointer and list of things to kill off */
static	CacheLst	*sys_c;
static Cache *initsave;

static	int	cache_initted;
static	int	cache_frozen;
static	int	cache_busy;

/* cache stats gathering stuff. you don't like it? comment it out */
time_t	cs_ltime;
int	cs_writes;	/* total writes */
int	cs_reads;	/* total reads */
int	cs_dbreads;	/* total read-throughs */
int	cs_dbwrites;	/* total write-throughs */
int	cs_dels;		/* total deletes */
int	cs_checks;	/* total checks */
int	cs_rhits;	/* total reads filled from cache */
int	cs_ahits;	/* total reads filled active cache */
int	cs_whits;	/* total writes to dirty cache */
int	cs_fails;	/* attempts to grab nonexistent */
int	cs_resets;	/* total cache resets */
int	cs_syncs;	/* total cache syncs */
int	cs_objects;	/* total cache size */

void cache_var_init()
{
  cwidth = CACHE_WIDTH;
  cdepth = CACHE_DEPTH;
  cache_initted = 0;
  cache_frozen = 0;
  cache_busy = 0;
  cs_writes = 0;
  cs_reads = 0;
  cs_dbreads = 0;
  cs_dbwrites = 0;
  cs_dels = 0;
  cs_checks = 0;
  cs_rhits = 0;
  cs_ahits = 0;
  cs_whits = 0;
  cs_fails = 0;
  cs_syncs = 0;
  cs_objects = 0;
  sys_c = (CacheLst *)0;
}

void cache_fchn(Chain pass1)
{
  Cache *pt1;

  pt1 = pass1.head;
  while (pt1 && (pt1 != pass1.tail)) {
    if (pt1->op)
      objfree(pt1->op);
    pt1 = pt1->nxt;
  }
  if ((pt1) && (pt1->op))
    objfree(pt1->op);
}

void cache_repl (Cache *cp, Obj *new)
{
        DPUSH; /* #161 */
	if(cp->op != ONULL)
		objfree(cp->op);
	cp->op = new;
        VOIDRETURN; /* #161 */
}

int
cache_init(int width, int depth)
{
	int	x;
	int	y;
	Cache	*np, *cp;
	CacheLst *sp;
	static char *ncmsg = "cache_init: cannot allocate cache: ";

        DPUSH; /* #162 */

	if(cache_initted || sys_c != (CacheLst *)0) {
		RETURN(0); /* #162 */
        }

	/* If either dimension is specified as non-zero, change it to */
	/* that, otherwise use default. Treat dimensions deparately.  */

	if(width)
		cwidth = width;
	if(depth)
		cdepth = depth;

	sp = sys_c = (CacheLst *)malloc((unsigned)cwidth * sizeof(CacheLst));
	if(sys_c == (CacheLst *)0) {
		mush_logf(ncmsg,(char *)-1,"\n",(char *)0);
		RETURN(-1); /* #162 */
	}

	/* Pre-allocate the initial cache entries in one chunk */
	initsave = cp = (Cache *)malloc(cwidth * cdepth * sizeof(Cache));
	if (cp == (Cache *)0) {
		mush_logf(ncmsg,(char *)-1,"\n",(char *)0);
		RETURN(-1); /* #162 */
	}

	for(x = 0; x < cwidth; x++, sp++) {
		sp->active.head = sp->old.head = (Cache *)0;
		/* sp->old.tail = sp->old.tail = (Cache *)0; */
		sp->active.tail = sp->old.tail = (Cache *)0;
		sp->mactive.head = sp->mold.head = (Cache *)0;
		sp->mactive.tail = sp->mold.tail = (Cache *)0;
		sp->count = cdepth;

		for(y = 0; y < cdepth; y++) {

			np = cp++;
			if((np->nxt = sp->old.head) != (Cache *)0)
				np->nxt->prv = np;
			else
				sp->old.tail = np;

			np->prv = (Cache *)0;
			np->op = (Obj *)0;

			sp->old.head = np;
			cs_objects++;
		}
	}

	/* mark caching system live */
	cache_initted++;

	time(&cs_ltime);

	RETURN(0); /* #162 */
}



static void cache_trim(CacheLst *sp)
{
	Cache *cp, *cp2, *cp3;

        DPUSH; /* #163 */

	cp2 = initsave + (cwidth * cdepth);
	cp = sp->old.head;
	while (cp != CNULL) {
	    if ((cp < initsave) || (cp > cp2)) {
		if ((cp == sp->old.head) && (cp == sp->old.tail)) {
		    sp->old.head = sp->old.tail = CNULL;
		    cp3 = CNULL;
		}
		else if (cp == sp->old.head) {
		    cp->nxt->prv = CNULL;
		    sp->old.head = cp->nxt;
		    cp3 = cp->nxt;
		}
		else if (cp == sp->old.tail) {
		    cp->prv->nxt = CNULL;
		    sp->old.tail = cp->prv;
		    cp3 = CNULL;
		}
		else {
		    cp->prv->nxt = cp->nxt;
		    cp->nxt->prv = cp->prv;
		    cp3 = cp->nxt;
		}
		cache_repl(cp, ONULL);
		free(cp);
		sp->count--;
		cs_objects--;
		cp = cp3;
	    }
	    else
		cp = cp->nxt;
	}
        VOIDRETURN; /* #163 */
}

void cache_reset(int trim)
{
	int	x;
	CacheLst *sp;

        DPUSH; /* #164 */

	if(!cache_initted) {
		VOIDRETURN; /* #164 */
        }

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
        VOIDRETURN; /* #164 */
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
	Obj		*ret;

        DPUSH; /* #165 */

	/* firewall */
	if(nam == (Aname *)0 || !cache_initted) {
#ifdef	CACHE_VERBOSE
		mush_logf("cache_get: NULL object name - programmer error\n",(char *)0);
#endif
		RETURN((Attr *)0); /* #165 */
	}

#ifdef	CACHE_DEBUG
	printf("get %d/%d\n",nam->object,nam->attrnum);
#endif

	cs_reads++;

	/* We search the cache for the right Obj, then find the Attrib inside
		that. */

	hv = nam->object % cwidth;
	sp = &sys_c[hv];

	/* search active chain first */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			cs_rhits++;
			cs_ahits++;
#ifdef	CACHE_DEBUG
			printf("return active cache -- %d\n",cp->op);
#endif
			/* Found the Obj, return the Attr within it */

			RETURN(get_attrib(nam,cp->op)); /* #165 */
		}
	}

	/* search modified active chain next. */
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			cs_rhits++;
			cs_ahits++;

#ifdef	CACHE_DEBUG
			printf("return modified active cache -- %d\n",cp->op);
#endif
			RETURN(get_attrib(nam,cp->op)); /* #165 */
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
			printf("return old cache -- %d\n",cp->op);
#endif
			RETURN(get_attrib(nam,cp->op)); /* #165 */
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
			printf("return modified old cache -- %d\n",cp->op);
#endif
			RETURN(get_attrib(nam,cp->op)); /* #165 */
		}
	}

	/* DARN IT - at this point we have a certified, type-A cache miss */

	/* thaw the object from wherever. */

        /* shared memory debugging with debugmon */
#ifndef NODEBUGMONITOR
        debugmem->lastdbfetch = nam->object;
#endif

	if((ret = DB_GET(&(nam->object))) == (Obj *)0) {
		cs_fails++;
		cs_dbreads++;
#ifdef	CACHE_DEBUG
		printf("Object %d not in db\n",nam->object);
#endif
		RETURN((Attr *)0); /* #165 */
	} else
		cs_dbreads++;

	if ((cp = get_free_entry(sp)) == CNULL) {
		RETURN((Attr *)0); /* #165 */
        }

	cp->op = ret;

	/* relink at head of active chain */
	INSHEAD(sp->active, cp);

#ifdef	CACHE_DEBUG
	printf("returning attr %d, object %d loaded into cache -- %d\n",
	       nam->attrnum,nam->object,cp->op);
#endif
	RETURN(get_attrib(nam,ret)); /* #165 */
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
	Obj	*newobj;
	int	hv = 0;

        DPUSH; /* #166 */

	/* firewall */
	if(obj == (Attr *)0 || nam == (Aname *)0 || !cache_initted) {
#ifdef	CACHE_VERBOSE
		mush_logf("cache_put: NULL object/name - programmer error\n",(char *)0);
#endif
		RETURN(1); /* #166 */
	}

	cs_writes++;

	/* generate hash */
	hv = nam->object % cwidth;
	sp = &sys_c[hv];

	/* step one, search active chain, and if we find the obj, dirty it */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {

			/* Right object, set the attribute */

			set_attrib(nam,cp->op,obj);

#ifdef	CACHE_DEBUG
			printf("cache_put object %d, attr %d -- %d\n",
			       nam->object,nam->attrnum, cp->op);
#endif
			DEQUEUE(sp->active, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			RETURN(0); /* #166 */
		}
	}

	/* step two, search modified active chain, and if we find the obj, we're done */
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {

			/* Right object, set the attribute */

			set_attrib(nam,cp->op,obj);

#ifdef	CACHE_DEBUG
			printf("cache_put object %d,attr %d -- %d\n",
			       nam->object, nam->attrnum, cp->op);
#endif
			cs_whits++;
			RETURN(0); /* #166 */
		}
	}

	/* step three, search in-active chain.  If we find it, move it to the mod active */
	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {

			/* Right obj. Set the attrib. */

			set_attrib(nam,cp->op,obj);

#ifdef	CACHE_DEBUG
			printf("cache_put object %d, attr %d -- %d\n",
			       nam->object,nam->attrnum,cp->op);
#endif
			DEQUEUE(sp->old, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			RETURN(0); /* #166 */
		}
	}

	/* step four, search modified in-active chain */
	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt) {
		if (NAMECMP(cp, nam)) {

			/* Right object. Set the attribute. */

			set_attrib(nam,cp->op,obj);

#ifdef	CACHE_DEBUG
			printf("cache_put %d,%d %d\n",
			       nam->object,nam->attrnum,cp->op);
#endif
			DEQUEUE(sp->mold, cp);
			INSHEAD(sp->mactive, cp);
			cs_whits++;
			RETURN(0); /* #166 */
		}
	}

	/* Ok, now the fact that we're caching objects, and pretending
		to cache attributes starts to *hurt*. We gotta try to
		get the object in to cache, see.. */

	newobj = DB_GET(&(nam->object));
	if(newobj == (Obj *)0){

		/* SHIT! Totally NEW object! */

		newobj = (Obj *) malloc(sizeof(Obj));
		if(newobj == (Obj *)0)
			RETURN(1); /* #166 */

		newobj->atrs = ATNULL;
		newobj->name = nam->object;
	}

	/* Now we got the thing, hang the new version of the attrib on it. */

	set_attrib(nam,newobj,obj);

	/* add it to the cache */
	if ((cp = get_free_entry(sp)) == CNULL) {
		RETURN(1); /* #166 */
        }

	cp->op = newobj;

	/* link at head of modified active chain */
	INSHEAD(sp->mactive, cp);

#ifdef	CACHE_DEBUG
	printf("cache_put %d/%d new in cache %d\n",nam->object,nam->attrnum,cp->op);
#endif

	/* e finito ! */
	RETURN(0); /* #166 */
}

static Cache *
get_free_entry(CacheLst *sp)
{
	Cache *cp;

        DPUSH; /* #167 */

	/* if there are no old cache object holders left, allocate one */
	if((cp = sp->old.tail) == CNULL) {

		if (!cache_busy && !cache_frozen &&
				(cp = sp->mold.tail) != CNULL) {
			/* unlink old cache chain tail */
			if(cp->prv != CNULL) {
				sp->mold.tail = cp->prv;
				cp->prv->nxt = cp->nxt;
			} else	/* took last one */
				sp->mold.head = sp->mold.tail = CNULL;

#ifdef	CACHE_DEBUG
			printf("clean object %d from cache %d\n",
			       (cp->op)->name,cp->op);
#endif
			if ((cp->op)->at_count == 0) {
  			        if (DB_DEL(&((cp->op)->name),x))
					RETURN(CNULL); /* #167 */
				cs_dels++;
			} else {
				if(DB_PUT(cp->op,&((cp->op)->name)))
					RETURN(CNULL); /* #167 */
				cs_dbwrites++;
			}
		} else {
			if((cp = (Cache *)malloc(sizeof(Cache))) == CNULL)
				fatal("cache get_free_entry: malloc failed",(char *)-1,(char *)0);

			cp->op = (Obj *)0;
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

	cache_repl(cp, ONULL);
	RETURN(cp); /* #167 */
}

static int cache_write(Cache *cp)
{
        DPUSH; /* #168 */
	while (cp != CNULL) {
#ifdef	CACHE_DEBUG
		printf("sync %d -- %d\n",cp->op->name, cp->op);
#endif
		if (cp->op->at_count == 0) {
			if(DB_DEL(&((cp->op)->name),x)) {
				log_db_err(cp->op->name, 0, "delete");
				RETURN(1); /* #168 */
			}
			cs_dels++;
		} else {
			if(DB_PUT(cp->op,&((cp->op)->name))) {
				log_db_err(cp->op->name, 0, "write");
				RETURN(1); /* #168 */
			}
			cs_dbwrites++;
		}
		cp = cp->nxt;
	}
	RETURN(0); /* #168 */
}

static void cache_clean(CacheLst *sp)
{
        DPUSH; /* #169 */
	/* move the modified inactive chain to the inactive chain */
	if (sp->mold.head != NULL) {
		if (sp->old.head == NULL) {
			sp->old.head = sp->mold.head;
			sp->old.tail = sp->mold.tail;
		} else {
			sp->old.tail->nxt = sp->mold.head;
			sp->mold.head->prv = sp->old.tail;
			sp->old.tail = sp->mold.tail;
		}
		sp->mold.head = sp->mold.tail = CNULL;
	}
	/* same for the modified active chain */
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
        VOIDRETURN; /* #169 */
}

int NDECL(cache_sync)
{
int	x;
CacheLst *sp;

        DPUSH; /* #170 */

	cs_syncs++;

	if(!cache_initted) {
		RETURN(1); /* #170 */
        }

	if (cache_frozen) {
		RETURN(0); /* #170 */
        }

	cache_reset(0);

	for(x = 0, sp = sys_c; x < cwidth; x++, sp++) {
		if (cache_write(sp->mold.head)) {
			RETURN(1); /* #170 */
                }
		if (cache_write(sp->mactive.head)) {
			RETURN(1); /* #170 */
                }
		cache_clean(sp);
		if (mudconf.cache_trim)
			cache_trim(sp);
	}
	RETURN(0); /* #170 */
}

/*
Mark this attr as deleted in the cache. The object will flush back to
disk without it, eventually.
*/
void
cache_del(Aname *nam)
{
	Cache	*cp;
	CacheLst *sp;
	Obj	*obj;
	int	hv = 0;

        DPUSH; /* #171 */

	if(nam == (Aname *)0 || !cache_initted) {
		VOIDRETURN; /* #171 */
        }

#ifdef CACHE_DEBUG
	printf("cache_del: object %d, attr %d\n",nam->object,nam->attrnum);
#endif

	cs_dels++;

	hv = nam->object % cwidth;
	sp = &sys_c[hv];

	/* mark dead in cache */
	for(cp = sp->active.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->active, cp);
			INSHEAD(sp->mactive, cp);
			del_attrib(nam,cp->op);
			VOIDRETURN; /* #171 */
		}
	}
	for(cp = sp->mactive.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			del_attrib(nam,cp->op);
			VOIDRETURN; /* #171 */
		}
	}

	for(cp = sp->old.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->old, cp);
			INSHEAD(sp->mactive, cp);
			del_attrib(nam,cp->op);
			VOIDRETURN; /* #171 */
		}
	}
	for(cp = sp->mold.head; cp != CNULL; cp = cp->nxt) {
		if(NAMECMP(cp,nam)) {
			DEQUEUE(sp->mold, cp);
			INSHEAD(sp->mactive, cp);
			del_attrib(nam,cp->op);
			VOIDRETURN; /* #171 */
		}
	}

	/* If we got here, the object the attribute's on isn't in cache */
	/* At all.  So we get to fish it off of disk, nuke the attrib,  */
	/* and shove the object in cache, so it'll be flushed back later */

	obj = DB_GET(&(nam->object));
	if(obj == (Obj *)0) {
		VOIDRETURN; /* #171 */
        }

	if ((cp = get_free_entry(sp)) == CNULL) {
		VOIDRETURN; /* #171 */
        }

	del_attrib(nam,obj);

	cp->op = obj; 

	INSHEAD(sp->mactive, cp);
	VOIDRETURN; /* #171 */
}

/*
	And now a totally new suite of functions for manipulating
attributes within an Obj. Woo. Woo. Andrew.

*/

static Attr *
get_attrib(Aname *anam, Obj *obj)
{
	int	i;
	Attrib	*a;

        DPUSH; /* #172 */

#ifdef CACHE_DEBUG
	printf("get_attrib: object %d, attr %d, obj ptr %d\n",
		anam->object,anam->attrnum,obj);
#endif

	/* Loop down the object's attribute list */

	a = obj->atrs;
	for(i = 0; i < obj->at_count; i++){
		if(a[i].attrnum == anam->attrnum) { 
			RETURN((Attr *) a[i].data); /* #172 */
                }
	}
#ifdef CACHE_DEBUG
	printf("get_attrib: not found.\n");
#endif
	RETURN((Attr *)0); /* Not found */ /* #172 */
}

static void
set_attrib(Aname *anam, Obj *obj, Attr *value)
{
	int	i;
	Attrib	*a;

        DPUSH; /* #173 */

	/* Demands made elsewhere insist that we cope with the
		case of an empty object. */

	if(obj->atrs == ATNULL){
		a = (Attrib *) malloc(sizeof(Attrib));
		if(a == ATNULL) { /* Fail silently. It's a game. */
			VOIDRETURN; /* #173 */
                }

		obj->atrs = a;
		obj->at_count = 1;
		a[0].attrnum = anam->attrnum;
		a[0].data = (char *) value;
		a[0].size = ATTR_SIZE(value);
		VOIDRETURN; /* #173 */
	}


	/* Loop down the object's attribute list. */

	a = obj->atrs;
	for(i = 0; i < obj->at_count; i++){
		if(a[i].attrnum == anam->attrnum){
			/* Found it, so change it.. */

			free(a[i].data);
			a[i].data = (char *) value;
			a[i].size = ATTR_SIZE(value);
			VOIDRETURN; /* #173 */
		}
	}

	/* Not found on the object, so make a new attribute. */

	a = (Attrib *) realloc(obj->atrs, (obj->at_count + 1) * sizeof(Attrib));

	if(!a){
		/* Silently fail. It's just a game. */

		VOIDRETURN; /* #173 */
	}

	a[obj->at_count].data = value;
	a[obj->at_count].attrnum = anam->attrnum;
	a[obj->at_count].size = ATTR_SIZE(value);
	obj->at_count = obj->at_count + 1;
	obj->atrs = a;
        VOIDRETURN; /* #173 */
}

static void
del_attrib(Aname *anam, Obj *obj)
{
	int	i;
	Attrib	*a;

        DPUSH; /* #174 */
#ifdef CACHE_DEBUG
	printf("del_attrib: deleteing attr %d on object %d (%d)\n",anam->attrnum,
		obj->name,anam->object);
#endif

	if(!obj->at_count || !obj->atrs) { 
		VOIDRETURN; /* #174 */
        }

	if(obj->at_count < 0)
		abort();

	a = obj->atrs;
	for(i = 0; i < obj->at_count; i++){
		if(anam->attrnum == a[i].attrnum){

			/* Now nuke this one, and pack the rest down. */
	
			free(a[i].data);
	
			obj->at_count -= 1;
			if(i != obj->at_count)
				bcopy(a+i+1, a+i,
					(obj->at_count - i) * sizeof(Attrib));
			VOIDRETURN; /* #174 */
		}
	}
        VOIDRETURN; /* #174 */
}

/* And something to free all the goo on an Obj, as well as the Obj. */

static void
objfree(Obj *o)
{
	int	i;
	Attrib	*a;

        DPUSH; /* #175 */

	if(!o->atrs){
		free(o);
		VOIDRETURN; /* #175 */
	}
	a = o->atrs;
	for(i = 0; i < o->at_count; i++)
		free(a[i].data);

	free(a);
	free(o);
        VOIDRETURN; /* #175 */
}
