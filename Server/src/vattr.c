/* vattr.c -- Manages the user-defined attributes. */

#include "copyright.h"
#include "autoconf.h"

/* Gather page fault stats */

#undef GATHER_STATS

#include "copyright.h"
#include "externs.h"
#include "mudconf.h"
#include "vattr.h"
#include "alloc.h"

#ifndef NULL
#define NULL 0
#endif

static VATTR **vhash;

static VATTR *vfreelist;

static int vhash_index;

static int vstats_count;
static double vstats_hits;
static int vstats_name_cnt;
static int vstats_freecnt;
static double vstats_lookups;
static int vstats_maxlookup;
static double vstats_checks;
static int vstats_nulls;
#ifdef GATHER_STATS
static int page_faults = 0;
#endif

#define vattr_hash(n)		hashval((n), VHASH_MASK)
static void 	FDECL(fixcase, (char *));
static char	FDECL(*store_string, (char *));

/* Allocate space for strings in lumps this big. */

#define STRINGBLOCK 1000

/* Current block we're putting stuff in */

static char	*stringblock = (char *)0;

/* High water mark. */

static int	stringblock_hwm = 0;

void NDECL(vattr_init)
{
	VATTR **vpp;
	int i;

	vhash = (VATTR **)malloc(VHASH_SIZE*sizeof(VATTR *));
	vfreelist = NULL;

	for (i = 0, vpp = vhash; i < VHASH_SIZE; i++)
		*vpp++ = NULL;

	vstats_count = 0;
	vstats_name_cnt = 0;
	vstats_lookups = 0;
	vstats_hits = 0;
	vstats_checks = 0;
	vstats_maxlookup = 0;
	vstats_nulls = VHASH_SIZE;
}

VATTR *vattr_find(char *name)
{
	register VATTR *vp,*vprev;
	int hash, maxlooks;
#if defined(GATHER_STATS) && defined(HAVE_GETRUSAGE)
	struct rusage foo;
	int	old_majflt;

	getrusage(RUSAGE_SELF,&foo);
	old_majflt = foo.ru_majflt;
#endif
	fixcase(name);
	if (!ok_attr_name(name))
		return(NULL);
	hash = vattr_hash(name);
	vstats_lookups++;

	vprev = NULL;
        maxlooks = 0;
	for (vp = vhash[hash]; vp != NULL;vprev = vp, vp = vp->next) {
		maxlooks++;
		if (!strcmp(name, vp->name)) {
			vstats_hits++;
			vstats_checks += (double)maxlooks;
			if ( maxlooks > vstats_maxlookup ) {
				vstats_maxlookup = maxlooks;
			}
			break;
                }
	}
	if(vp && vprev){ /* Rechain */
		vprev->next = vp->next;
		vp->next = vhash[hash];
		vhash[hash] = vp;
	}
#if defined(GATHER_STATS) && defined(HAVE_GETRUSAGE)
	getrusage(RUSAGE_SELF,&foo);
	page_faults += foo.ru_majflt - old_majflt;
#endif

	/* vp is NULL or the right thing. It's right, either way. */
	return(vp);
}

VATTR *vattr_alloc(char *name, int flags)
{
	int number;

#ifndef STANDALONE
        if ( (mudconf.control_flags & CF_VATTRCHECK) &&
#else
        if (
#endif
           (mudstate.vattr_reuseptr >= 0) && mudstate.vattr_reusecnt && (mudstate.vattr_reuse[mudstate.vattr_reuseptr] > 0) ) {
           number = mudstate.vattr_reuse[mudstate.vattr_reuseptr];
           mudstate.vattr_reuse[mudstate.vattr_reuseptr] = 0;
           if ( mudstate.vattr_reuseptr >= mudstate.vattr_reusecnt ) {
              mudstate.vattr_reuseptr = NOTHING;
           } else {
              mudstate.vattr_reuseptr++;
           }
        } else {
	   if (((number = mudstate.attr_next++) & 0x7f) == 0)
		   number = mudstate.attr_next++;
        }
	anum_extend(number);
	return(vattr_define(name, number, flags));
}
	
VATTR *vattr_define(char *name, int number, int flags)
{
	VATTR *vp;
	int hash;

	/* Be ruthless. */

	if(strlen(name) > VNAME_SIZE)
		name[VNAME_SIZE - 1] = '\0';

	fixcase(name);
	if(!ok_attr_name(name))
		return(NULL);

	if ((vp = vattr_find(name)) != NULL)
		return(NULL);

	if (vfreelist == NULL) {
		vfreelist = vp = (VATTR *)malloc(VALLOC_SIZE * sizeof(VATTR));
		for (hash = 0; hash < (VALLOC_SIZE-1); hash++, vp++)
			vp->next = vp + 1;
		vp->next = NULL;
		vstats_freecnt = VALLOC_SIZE;
	}
	vp = vfreelist;
	vfreelist = vp->next;
	vstats_freecnt--;
	vstats_count++;
	vstats_name_cnt += strlen(name);

	vp->name = store_string(name);
	vp->flags = flags;
	vp->command_flag = 0;
	vp->number = number;

	hash = vattr_hash(name);
	vp->next = vhash[hash];
        if ( vhash[hash] == NULL ) {
           vstats_nulls--;
        }
	vhash[hash] = vp;

	anum_extend(vp->number);
	anum_set(vp->number, (ATTR *)vp);
	return(vp);
}

void vattr_delete(char *name)
{
	VATTR *vp, *vpo;
	int number, hash;

	fixcase(name);
	if (!ok_attr_name(name))
		return;

	number = 0;
	hash = vattr_hash(name);
	for (vp = vhash[hash], vpo = NULL; vp != NULL;) {
		if (!strcmp(name, vp->name)) {
			number = vp->number;
			if (vpo == NULL)
				vhash[hash] = vp->next;
			else
				vpo->next = vp->next;
                 	vstats_name_cnt-=strlen(vp->name);
			vp->next = vfreelist;
			vfreelist = vp;
			vstats_count--;
			vstats_freecnt++;
			break;
		}
		vpo = vp;
		vp = vp->next;
	}
	if (number)
		anum_set(number, NULL);
	return;
}

VATTR *vattr_rename(char *name, char *newname)
{
	VATTR *vp, *vpo;
	int hash;

	fixcase(name);
	if (!ok_attr_name(name))
		return(NULL);

	/* Be ruthless. */

	if(strlen(newname) > VNAME_SIZE)
		newname[VNAME_SIZE - 1] = '\0';

	fixcase(newname);
	if (!ok_attr_name(newname))
		return(NULL);

	hash = vattr_hash(name);
	for (vp = vhash[hash], vpo = NULL; vp != NULL;) {
		if (!strcmp(name, vp->name)) {
			if (vpo == NULL)
				vhash[hash] = vp->next;
			else
				vpo->next = vp->next;
			break;
		}
		vpo = vp;
		vp = vp->next;
	}
	vp->name = store_string(newname);

	hash = vattr_hash(newname);
	vp->next = vhash[hash];
	vhash[hash] = vp;

	return(vp);
}

VATTR *NDECL(vattr_first)
{
	for (vhash_index=0; vhash_index < VHASH_SIZE; vhash_index++) {
		if (vhash[vhash_index])
			return vhash[vhash_index];
	}
	return NULL;
}

VATTR *vattr_next(VATTR *vp)
{
	if (vp == NULL)
		return(vattr_first());

	if (vp->next == NULL) {
		while (++vhash_index < VHASH_SIZE) {
			if (vhash[vhash_index])
				return vhash[vhash_index];
		}
		return(NULL);
	}

	return(vp->next);
}

#ifndef STANDALONE
int list_vcount()
{
   return vstats_count;
}

void list_vhashstats(dbref player)
{
   char  *buff, *s_format, *s_pt;
   int i_max = 99999999;

   buff = alloc_mbuf("vattr_hashstats");

   s_pt = s_format = alloc_mbuf("hashformat");

   /* name, VHASH_SIZE, vstats_count, vstats_freecnt, vstats_nulls */
   safe_str((char *)"%-14.14s %6d %8d %5d %5d ", s_format, &s_pt);

   /* Special padding for total hashes scanned */
   if ( vstats_lookups > i_max ) {
      safe_str((char *)"%8.2e ", s_format, &s_pt);
   } else {
      safe_str((char *)"%8.0f ", s_format, &s_pt);
   }

   /* Special padding for total hashes hit */
   if ( vstats_hits > i_max ) {
      safe_str((char *)"%8.2e ", s_format, &s_pt);
   } else {
      safe_str((char *)"%8.0f ", s_format, &s_pt);
   }

   /* Special padding for total hashes checked */
   if ( vstats_checks > i_max ) {
      safe_str((char *)"%8.2e ", s_format, &s_pt);
   } else {
      safe_str((char *)"%8.0f ", s_format, &s_pt);
   }

   /* vstats_maxlookup */
   safe_str((char *)"%8d", s_format, &s_pt);

#ifdef GATHER_STATS
   /* page_faults */
   safe_str((char *)"%8d", s_format, &s_pt);

   //sprintf(buff, "%-14.14s %6d %8d %5d %5d %8d %8d %8d %8d %8d",
   sprintf(buff, s_format,
           (char *)"Vattr Stats:", VHASH_SIZE, vstats_count, vstats_freecnt, 
           vstats_nulls, vstats_lookups, vstats_hits, vstats_checks, vstats_maxlookup, page_faults);
#else
   //sprintf(buff, "%-14.14s %6d %8d %5d %5d %8d %8d %8d %8d",
   sprintf(buff, s_format,
           (char *)"Vattr Stats:", VHASH_SIZE, vstats_count, vstats_freecnt, 
           vstats_nulls, vstats_lookups, vstats_hits, vstats_checks, vstats_maxlookup);
#endif

   notify(player, buff);
   free_mbuf(buff);
   free_mbuf(s_format);
}

int sum_vhashstats(dbref player)
{
   int val;
   val = (sizeof(VATTR) * vstats_count) + vstats_name_cnt;

   return(val);
}

#endif

static void fixcase(char *name)
{
	char *cp = name;

	while (*cp) {
		if (islower((int)*cp)) *cp = toupper((int)*cp);
		cp++;
	}

	return;
}


/*
	Some goop for efficiently storing strings we expect to
	keep forever. There is no freeing mechanism.
*/

static char *store_string(char *str)
{
	int	len;
	char	*ret;

	len = strlen(str);

	/* If we have no block, or there's not enough room left in the
		current one, get a new one. */

	if(!stringblock || (STRINGBLOCK - stringblock_hwm) < (len+1)){
		stringblock = (char *) malloc(STRINGBLOCK);
		if(!stringblock)
			return((char *)0);
		stringblock_hwm = 0;
	}

	ret = stringblock + stringblock_hwm;
	strcpy(ret, str);
	stringblock_hwm += (len + 1);
	return(ret);
}
