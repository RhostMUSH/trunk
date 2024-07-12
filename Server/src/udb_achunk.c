/*
	Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
	Slightly whacked by Andrew Molitor for use with MUSH.
	My apologies to the author..
*/

/* First */
#include	"autoconf.h"
#include	"udb.h"
#include	"udb_defs.h"

#ifdef VMS
#include        <types.h>
#include        <file.h>
#include        <unixio.h>
#include        "vms_dbm.h"
#else
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/file.h>

#include	"myndbm.h"

#endif /* VMS */

extern int	FDECL(attr_siz, (Attr *));
extern void	VDECL(fatal, (char *, ...));
extern void	FDECL(log_db_err, (int, int, const char *));

/*
#define		DBMCHUNK_DEBUG
*/

/* Default database name */
#define DEFAULT_DBMCHUNKFILE "db_dbm"

/* default block size to use for bitmapped chunks */
#define	DDDB_BLOCK	64


/* bitmap growth increment in BLOCKS not bytes (512 is 64 BYTES) */
#define	DDDB_BITBLOCK	512


#define	LOGICAL_BLOCK(off)	((off)/bsiz)
#define	BLOCK_OFFSET(block)	((block)*bsiz)
#define	BLOCKS_NEEDED(siz)	(((siz) + (bsiz-1)) / bsiz)



/*
dbm-based object storage routines. Somewhat trickier than the default
directory hash. a free space list is maintained as a bitmap of free
blocks, and an object-pointer list is maintained in a dbm database.
*/
struct	hrec	{
	off_t	off;
	int	siz;
};

static	int dddb_mark();

static	const char *dbfile = DEFAULT_DBMCHUNKFILE;
static	int	bsiz = DDDB_BLOCK;
static	int	db_initted = 0;
static	int	last_free = 0;	/* last known or suspected free block */

#ifdef	HAVE_DBM
static	int	dbp = 0;
#else
static	DBM	*dbp = (DBM *)0;
#endif

static	FILE *	dbf;
static	struct	hrec	hbuf;

static	char	*bitm = NULL;
static	int	bm_top = 0;
static	datum	dat;
static	datum	key;

static	void	growbit();

extern void mush_logf();

int
dddb_init()
{
	static	const char *copen = "db_init cannot open ";
	char	fnam[MAXPATHLEN];
	struct	stat	sbuf;
	int	fxp;


	/* now open chunk file */
	sprintf(fnam,"%.250s.db",dbfile);
	/* Bah.  Posix-y systems break "a+", so use this gore instead */
	if(((dbf = fopen(fnam,"r+")) == (FILE *)0)
			&& ((dbf = fopen(fnam,"w+")) == (FILE *)0)) {
		mush_logf(copen,fnam," ",(char *)-1,"\n",NULL);
		return(1);
	}

	/* Some systems (like HP-UX don't like nonexistent binary files.
	 * Therefore, create empty files if none exist already.
	 */

	sprintf(fnam,"%.250s.dir",dbfile);
	fxp = open(fnam,O_CREAT|O_RDWR,0600);
	if(fxp < 0) {
		mush_logf(copen,fnam," ",(char *)-1,"\n",NULL);
		return(1);
	}
	(void)close(fxp);

	sprintf(fnam,"%.250s.pag",dbfile);
	fxp = open(fnam,O_CREAT|O_RDWR,0600);
	if(fxp < 0) {
		mush_logf(copen,fnam," ",(char *)-1,"\n",NULL);
		return(1);
	}
	(void)close(fxp);

	/* open hash table */
#ifdef	HAVE_DBM
	if((dbp = dbminit(dbfile)) < 0) {
		mush_logf(copen,dbfile," ",(char *)-1,"\n",NULL);
		return(1);
	}
#else
	if((dbp = dbm_open(dbfile,O_RDWR|O_CREAT,0600)) == (DBM *)0) {
		mush_logf(copen,dbfile," ",(char *)-1,"\n",NULL);
		return(1);
	}
#endif

	/* determine size of chunk file for allocation bitmap */
	sprintf(fnam,"%.250s.db",dbfile);
	if(stat(fnam,&sbuf)) {
		mush_logf("db_init cannot stat ",fnam," ",(char *)-1,"\n",NULL);
		return(1);
	}

	/* allocate bitmap */
	growbit(LOGICAL_BLOCK(sbuf.st_size) + DDDB_BITBLOCK);


#ifdef	HAVE_DBM
	key = firstkey();
#else
	key = dbm_firstkey(dbp);
#endif
	while(key.dptr != NULL) {
#ifdef	HAVE_DBM
		dat = fetch(key);
#else
		dat = dbm_fetch(dbp,key);
#endif
		if(dat.dptr == NULL) {
			mush_logf("db_init index inconsistent\n",NULL);
			return(1);
		}
		bcopy(dat.dptr,(char *)&hbuf,sizeof(hbuf));	/* alignment */

		/* mark it as busy in the bitmap */
		dddb_mark(LOGICAL_BLOCK(hbuf.off),hbuf.siz,1);


#ifdef	HAVE_DBM
		key = nextkey(key);
#else
		key = dbm_nextkey(dbp);
#endif
	}

	db_initted = 1;
	return(0);
}




int dddb_initted()
{
	return(db_initted);
}



int dddb_setbsiz(int nbsiz)
{
	return(bsiz = nbsiz);
}



int dddb_setfile(char *fil)
{
	char	*xp;

	if(db_initted)
		return(1);

	/* KNOWN memory leak. can't help it. it's small */
	xp = (char *)malloc((unsigned)strlen(fil) + 1);
	if(xp == NULL)
		return(1);
	(void)strcpy(xp,fil);
	dbfile = xp;
	return(0);
}



int
dddb_close()
{
	if(dbf != (FILE *)0) {
		fclose(dbf);
		dbf = (FILE *)0;
	}

#ifndef HAVE_DBM
	if(dbp != (DBM *)0) {
		dbm_close(dbp);
		dbp = (DBM *)0;
	}
#endif
	if(bitm != NULL) {
		free((mall_t)bitm);
		bitm = NULL;
		bm_top = 0;
	}
	db_initted = 0;
	return(0);
}



/* grow the bitmap to given size */
static	void
growbit(int maxblok)
{
	int	newtop, newbytes, topbytes;
	char	*nbit;

	/* round up to eight and then some */
	newtop = (maxblok + 15) & 0xfffffff8;

	if (newtop <= bm_top)
		return;

	newbytes = newtop / 8;
	topbytes = bm_top / 8;
	nbit = (char *)malloc(newbytes);
	if(bitm != (char *)0) {
		bcopy(bitm,(char *)nbit, topbytes);
		free((mall_t)bitm);
	}
	bitm = nbit;

	if(bitm == (char *)0)
		fatal("db_init cannot grow bitmap ",(char *)-1,"\n",(char *)0);

	bzero(bitm+topbytes, (newbytes-topbytes));
	bm_top = newtop;
}



static	int
dddb_mark(off_t lbn, int siz, int taken)
{
	int	bcnt;

	if (siz == 0)
		return(0);

	if (siz < 0) {
		mush_logf("Invalid size in dddb_mark.", NULL);
		return(0);
	}

	bcnt = BLOCKS_NEEDED(siz);

	/* remember a free block was here */
	if(!taken)
		last_free = lbn;

	while(bcnt--) {
		if(lbn >= bm_top - 32)
			growbit(lbn + DDDB_BITBLOCK);

		if(taken)
			bitm[lbn >> 3] |= (1 << (lbn & 7));
		else
			bitm[lbn >> 3] &= ~(1 << (lbn & 7));
		lbn++;
	}
	return 0;
}





static	int
dddb_alloc(int siz)
{
	int	bcnt;	/* # of blocks to operate on */
	int	lbn;	/* logical block offset */
	int	tbcnt;
	int	slbn;
	int	overthetop = 0;

	lbn = last_free;
	bcnt = BLOCKS_NEEDED(siz);

	while(1) {
		if(lbn >= bm_top - 32) {
			/* only check here. can't break around the top */
			if(!overthetop) {
				lbn = 0;
				overthetop++;
			} else {
				growbit(lbn + DDDB_BITBLOCK);
			}
		}

		slbn = lbn;
		tbcnt = bcnt;

		while(1) {
			if((bitm[lbn >> 3] & (1 << (lbn & 7))) != 0)
				break;

			/* enough free blocks - mark and done */
			if(--tbcnt == 0) {
				for(tbcnt = slbn; bcnt > 0; tbcnt++, bcnt--)
					bitm[tbcnt >> 3] |= (1 << (tbcnt & 7));

				last_free = lbn;
				return(slbn);
			}

			lbn++;
			if(lbn >= bm_top - 32)
				growbit(lbn + DDDB_BITBLOCK);
		}
		lbn++;
	}
}




Attr	*
dddb_get(Aname *nam)
{
	Attr		*ret;

	if(!db_initted)
		return((Attr *)0);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Aname);
#ifdef	HAVE_DBM
	dat = fetch(key);
#else
	dat = dbm_fetch(dbp,key);
#endif

	if(dat.dptr == NULL)
		return(ANULL);
	bcopy(dat.dptr,(char *)&hbuf,sizeof(hbuf));

	/* seek to location */
	if(fseek(dbf,(long)hbuf.off,0) == -1) {
		log_db_err(nam->object, nam->attrnum,
			"(Get) Seeking within dbfile");
		return(ANULL);
	}

	/* if the file is badly formatted, ret == Attr * 0 */
	ret = attrfromFILE(dbf, hbuf.siz);
	if (ret == ANULL)
		log_db_err(nam->object, nam->attrnum,
			"(Get) Damage detected within dbfile");
	return(ret);
}


int
dddb_put(Attr *obj, Aname *nam)
{
	int	nsiz;

	if(!db_initted)
		return(1);

	nsiz = attr_siz(obj);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Aname);

#ifdef	HAVE_DBM
	dat = fetch(key);
#else
	dat = dbm_fetch(dbp,key);
#endif

	if(dat.dptr != NULL) {

		bcopy(dat.dptr,(char *)&hbuf,sizeof(hbuf));	/* align */

		if(BLOCKS_NEEDED(nsiz) > BLOCKS_NEEDED(hbuf.siz)) {

#ifdef	DBMCHUNK_DEBUG
printf("put: %s old %d < %d - free %d\n",nam,hbuf.siz,nsiz,hbuf.off);
#endif
			/* mark free in bitmap */
			dddb_mark(LOGICAL_BLOCK(hbuf.off),hbuf.siz,0);

			hbuf.off = BLOCK_OFFSET(dddb_alloc(nsiz));
			hbuf.siz = nsiz;
#ifdef	DBMCHUNK_DEBUG
printf("put: %s moved to offset %d, size %d\n",nam,hbuf.off,hbuf.siz);
#endif
		} else {
			hbuf.siz = nsiz;
#ifdef	DBMCHUNK_DEBUG
printf("put: %s replaced within offset %d, size %d\n",nam,hbuf.off,hbuf.siz);
#endif
		}
	} else {
		hbuf.off = BLOCK_OFFSET(dddb_alloc(nsiz));
		hbuf.siz = nsiz;
#ifdef	DBMCHUNK_DEBUG
printf("put: %s (new) at offset %d, size %d\n",nam,hbuf.off,hbuf.siz);
#endif
	}


	/* make table entry */
	dat.dptr = (char *)&hbuf;
	dat.dsize = sizeof(hbuf);

#ifdef	HAVE_DBM
	if(store(key,dat)) {
		log_db_err(nam->object, nam->attrnum,
			"(Put) Storing index entry");
		return(1);
	}
#else
	if(dbm_store(dbp,key,dat,DBM_REPLACE)) {
		log_db_err(nam->object, nam->attrnum,
			"(Put) Storing index entry");
		return(1);
	}
#endif

#ifdef	DBMCHUNK_DEBUG
printf("%s offset %d size %d\n",nam,hbuf.off,hbuf.siz);
#endif
	if(fseek(dbf,(long)hbuf.off,0) == -1) {
		log_db_err(nam->object, nam->attrnum,
			"(Put) Seeking within dbfile");
		return(1);
	}

	if(attrtoFILE(obj,dbf,nsiz) != 0) {
		log_db_err(nam->object, nam->attrnum,
			"(Put) Writing attribute");
		return(1);
	}
	return(0);
}




int
dddb_check(Aname *nam)
{

	if(!db_initted)
		return(0);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Aname);
#ifdef	HAVE_DBM
	dat = fetch(key);
#else
	dat = dbm_fetch(dbp,key);
#endif

	if(dat.dptr == NULL)
		return(0);
	return(1);
}




int
dddb_del(Aname *nam, int flg)
{

	if(!db_initted)
		return(-1);

	key.dptr = (char *)nam;
	key.dsize = sizeof(Aname);
#ifdef	HAVE_DBM
	dat = fetch(key);
#else
	dat = dbm_fetch(dbp,key);
#endif


	/* not there? */
	if(dat.dptr == NULL)
		return(0);
	bcopy(dat.dptr,(char *)&hbuf,sizeof(hbuf));

	/* drop key from db */
#ifdef	HAVE_DBM
	if(delete(key)) {
#else
	if(dbm_delete(dbp,key)) {
#endif
		log_db_err(nam->object, nam->attrnum,
			"(Del) Removing index entry");
		return(1);
	}

	/* mark free space in bitmap */
	dddb_mark(LOGICAL_BLOCK(hbuf.off),hbuf.siz,0);
#ifdef	DBMCHUNK_DEBUG
printf("del: %s free offset %d, size %d\n",nam,hbuf.off,hbuf.siz);
#endif

	return(0);
}
