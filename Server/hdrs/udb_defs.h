#ifndef _M_UDB_DEFS_H_
#define _M_UDB_DEFS_H_

/*
	Header file for the UnterMUD DB layer, as applied to MUSH 2.0

	Andrew Molitor, amolitor@eagle.wesleyan.edu
	1991
*/


/*
some machines stdio use different parameters to tell fopen to open
a file for creation and binary read/write. that is why they call it
the "standard" I/O library, I guess. for HP/UX, you need to have
this "rb+", and make sure the file already exists before use. bleah
*/
#define	FOPEN_BINARY_RW	"a+"


#ifdef VMS
#define MAXPATHLEN	256
#define	NOSYSTYPES_H
#define	NOSYSFILE_H
#endif /* VMS */

/* If your malloc() returns void or char pointers... */
/* typedef	void	*mall_t; */
typedef	char	*mall_t;

/* default (runtime-resettable) cache parameters */

#ifdef CACHE_OBJS
#define	CACHE_DEPTH	128
#define	CACHE_WIDTH	1024
#else
#define	CACHE_DEPTH	128
#define	CACHE_WIDTH	4096
#endif

/* Macros for calling the DB layer */

#define	DB_INIT()	dddb_init()
#define	DB_CLOSE()	dddb_close()
#define	DB_SYNC()	dddb_sync()
#define	DB_GET(n)	dddb_get(n)
#define	DB_PUT(o,n)	dddb_put(o,n)
#define	DB_CHECK(s)	dddb_check(s)

#ifdef CACHE_OBJS
#define	DB_DEL(n,f)	  dddb_del((Objname *)n)
#define PROTO_DB_DEL()    int dddb_del(Objname *)
#define PROTO_DB_PUT()    int dddb_put(Obj *obj, Objname *)
#else
#define	DB_DEL(n,f)	  dddb_del((Aname *)n,f)
#define PROTO_DB_DEL()    int dddb_del((Aname *), int )
#define PROTO_DB_PUT()    int dddb_put(Attr *, Aname *)
#endif

#define	CMD__DBCONFIG	cmd__dddbconfig

#endif
