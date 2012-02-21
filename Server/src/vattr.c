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
static int vstats_name_cnt;
static int vstats_freecnt;
static int vstats_lookups;
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
}

VATTR *vattr_find(char *name)
{
	register VATTR *vp,*vprev;
	int hash;
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
	for (vp = vhash[hash]; vp != NULL;vprev = vp, vp = vp->next) {
		if (!strcmp(name, vp->name))
			break;
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

	if (((number = mudstate.attr_next++) & 0x7f) == 0)
		number = mudstate.attr_next++;
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
	vp->number = number;

	hash = vattr_hash(name);
	vp->next = vhash[hash];
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
void list_vhashstats(dbref player)
{
char	*buff;

	buff = alloc_lbuf("vattr_hashstats");
#ifdef GATHER_STATS
	sprintf(buff, "Vattr stats:  %d size, %d alloc, %d free, %d name lookups," 
                "%d page faults in lookups.\r\n", sizeof(VATTR),
                vstats_count, vstats_freecnt, vstats_lookups, page_faults);
#else
	sprintf(buff, "Vattr stats:  %d size, %d alloc, %d free, %d name lookups\r\n",
		sizeof(VATTR), vstats_count, vstats_freecnt, vstats_lookups);
#endif

	notify(player, buff);
	free_lbuf(buff);
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
