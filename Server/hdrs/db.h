/* db.h */

#include "copyright.h"

#ifndef	_M_DB_H
#define	_M_DB_H

#include "config.h"

#include <sys/file.h>

/* Eliminate link error on Linux - Thorin 3/99 */
#ifndef dbm_error
#define dbm_error(x) (0)
#endif

#define STORE(key, attr)	cache_put(key, attr)
#define DELETE(key)		cache_del(key)
#define FETCH(key)		cache_get(key)
#define SYNC			cache_sync()
#define CLOSE			{ cache_sync(); dddb_close(); }

#include "udb.h"

#define	ITER_PARENTS(t,p,l)	for ((l)=0, (p)=(t); \
				     (Good_obj(p) && \
				      ((l) < mudconf.parent_nest_lim)); \
				     (p)=Parent(p), (l)++)

/* Player classes and pseudo-classes for determining access to commands */
#define	CLASS_SLAVE	0
#define	CLASS_GUEST	1
#define	CLASS_VISITOR	2
#define	CLASS_PLAYER	4
#define	CLASS_ARCHITECT	8
#define	CLASS_MAGE	12
#define	CLASS_WIZARD	14
#define	CLASS_MAINT	15
#define	CLASS_GOD	16	/* Cannot assign players to this class */
#define	CLASS_DISABLED	17	/* Cannot assign players to this class */

typedef struct attr ATTR;
struct attr {
	const char *name;	/* This has to be first.  braindeath. */
	int	number;		/* attr number */
	int	flags;
	int	FDECL((*check),(int, dbref, dbref, int, char *));
};

extern ATTR *	FDECL(atr_num, (int anum));
extern ATTR *	FDECL(atr_num2, (int anum));
extern ATTR *	FDECL(atr_num3, (int anum));
extern ATTR *	FDECL(atr_num4, (int anum));
extern ATTR *	FDECL(atr_num_ex, (int anum));
extern ATTR *	FDECL(atr_num_pinfo, (int anum));
extern ATTR *	FDECL(atr_num_aladd, (int anum));
extern ATTR *	FDECL(atr_num_exec, (int anum));
extern ATTR *	FDECL(atr_num_objid, (int anum));
extern ATTR *	FDECL(atr_num_lattr, (int anum));
extern ATTR *	FDECL(atr_num_vattr, (int anum));
extern ATTR *	FDECL(atr_num_chkpass, (int anum));
extern ATTR *	FDECL(atr_num_mtch, (int anum));
extern ATTR *	FDECL(atr_str, (char *s));
extern ATTR *	FDECL(atr_str2, (char *s));
extern ATTR *	FDECL(atr_str3, (char *s));
extern ATTR *	FDECL(atr_str4, (char *s));
extern ATTR *	FDECL(atr_str_atrpeval, (char *s));
extern ATTR *	FDECL(atr_str_notify, (char *s));
extern ATTR *	FDECL(atr_str_parseatr, (char *s));
extern ATTR *	FDECL(atr_str_exec, (char *s));
extern ATTR *	FDECL(atr_str_objid, (char *s));
extern ATTR *	FDECL(atr_num_bool, (int anum));
extern ATTR *	FDECL(atr_str_bool, (char *s));
extern ATTR *	FDECL(atr_str_cluster, (char *s));
extern ATTR *	FDECL(atr_str_mtch, (char *s));

extern ATTR attr[];

extern ATTR **anum_table;
extern ATTR **anum_table_inline;

extern ATTR *	FDECL(anum_get_f, (long x));
extern void	FDECL(anum_set_f, (long x, ATTR *v));
#define anum_get(x) anum_get_f(x)
#define anum_set(x,v) anum_set_f(x,v)
extern void	FDECL(anum_extend,(int));

#define	ATR_INFO_CHAR	'\1'	/* Leading char for attr control data */

/* Attribute handler keys */
#define	AH_READ		0	/* Read the attribute from the hash db */
#define	AH_WRITE	1	/* Write the attribute to the hash db */
#define	AH_RWMASK	1	/* Mask for read/write bit */
#define	AH_RAW		2	/* Don't encode/decode tag info */
#define	AH_NOCHECK	4	/* Don't check permissions */
#define	AH_NOSPECIAL	8	/* Ignore special processing */

/* Boolean expressions, for locks */
#define	BOOLEXP_AND	0
#define	BOOLEXP_OR	1
#define	BOOLEXP_NOT	2
#define	BOOLEXP_CONST	3
#define	BOOLEXP_ATR	4
#define	BOOLEXP_INDIR	5
#define	BOOLEXP_CARRY	6
#define	BOOLEXP_IS	7
#define	BOOLEXP_OWNER	8
#define	BOOLEXP_EVAL	9

typedef struct boolexp BOOLEXP;
struct boolexp {
  boolexp_type type;
  struct boolexp *sub1;
  struct boolexp *sub2;
  dbref thing;			/* thing refers to an object */
};

#define	TRUE_BOOLEXP ((BOOLEXP *) 0)

#define	Astr(alist) ((unsigned char *)(&((alist)[1])))

/* Database format information */

#define	F_UNKNOWN	0	/* Unknown database format */
#define	F_MUSH		1	/* MUSH format (many variants) */
#define	F_MUSE		2	/* MUSE format */
#define	F_MUD		3	/* Old TinyMUD format */
#define	F_MUCK		4	/* TinyMUCK format */

#define	V_MASK		0x000000ff	/* Database version */
#define	V_ZONE		0x00000100	/* ZONE/DOMAIN field */
#define	V_LINK		0x00000200	/* LINK field (exits from objs) */
#define	V_GDBM		0x00000400	/* attrs are in a GDBM db, not here */
#define	V_ATRNAME	0x00000800	/* NAME is an attr, not in the hdr */
#define	V_ATRKEY	0x00001000	/* KEY is an attr, not in the hdr */
#define	V_PERNKEY	0x00001000	/* PERN: Extra locks in object hdr */
#define	V_PARENT	0x00002000	/* db has the PARENT field */
#define	V_COMM		0x00004000	/* PERN: Comm status in header */
#define	V_ATRMONEY	0x00008000	/* Money is kept in an attribute */
#define	V_XFLAGS	0x00010000	/* An extra word of flags */

/* special dbref's */
#define	NOTHING		(-1)	/* null dbref */
#define	AMBIGUOUS	(-2)	/* multiple possibilities, for matches */
#define	HOME		(-3)	/* virtual room, represents mover's home */
#define	NOPERM		(-4)	/* Error status, no permission */

typedef struct zlistnode ZLISTNODE;
struct zlistnode {
   dbref object;
   ZLISTNODE* next;
};

typedef struct objtotem OBJTOTEM;
struct objtotem {
   int flags[TOTEM_SLOTS];
   int modified;
};

typedef struct livewire LWIRE;
struct livewire {
   int funceval;
   int funceval_override;
   int queuemax;
};

typedef struct object OBJ;
struct object {
	dbref	location;	/* PLAYER, THING: where it is */
				/* ROOM: dropto: */
				/* EXIT: where it goes to */
	dbref	contents;	/* PLAYER, THING, ROOM: head of contentslist */
				/* EXIT: unused */
	dbref	exits;		/* PLAYER, THING, ROOM: head of exitslist */
				/* EXIT: where it is */
	dbref	next;		/* PLAYER, THING: next in contentslist */
				/* EXIT: next in exitslist */
				/* ROOM: unused */
	dbref	link;		/* PLAYER, THING: home location */
				/* ROOM, EXIT: unused */
	dbref	parent;		/* ALL: defaults for attrs, exits, $cmds, */
	dbref	owner;		/* PLAYER: domain number + class + moreflags */
				/* THING, ROOM, EXIT: owning player number */
	int	nvattr;		/* added 11/26 Seawolf.  */
        ZLISTNODE* zonelist;    /* ZONEMASTER: list of objects in zone */
                                /* OTHERS: list of zones belonged to */
	FLAG	flags;		/* ALL: Flags set on the object */
	FLAG	flags2;		/* ALL: even more flags */
	FLAG	flags3;		/* ALL: yet more flags - Thorin 3/95*/
	FLAG	flags4;		/* ALL: ditto */
	FLAG	toggles;	/* ALL: power toggles - Thorin 3/95*/
	FLAG	toggles2;	/* ALL: power toggles part 2 - Thorin 3/95*/
	FLAG	toggles3;	/* ALL: power toggles part 3 - Thorin 3/95*/
	FLAG	toggles4;	/* ALL: power toggles part 4 - Thorin 3/95*/
	FLAG	toggles5;
	FLAG	toggles6;
	FLAG	toggles7;
	FLAG	toggles8;
};

typedef char *NAME;

extern OBJTOTEM *dbtotem;
extern OBJ *db;
extern NAME *names;
extern LWIRE *dblwire;

#define Totem(t,x)		(((x >= 0) && (x < TOTEM_SLOTS)) ? dbtotem[t].flags[x] : 0)
#define TotemChk(t)		dbtotem[t].modified;

#define SYSTEM -1

//#define	Location(t)		db[t].location
#define	Zone(t)			NOTHING
#define	Contents(t)		db[t].contents
#define	Exits(t)		db[t].exits
#define	Next(t)			db[t].next
#define	Link(t)			db[t].link
#define	Owner(t)		db[t].owner
#define	Parent(t)		db[t].parent
#define	Flags(t)		db[t].flags
#define	Flags2(t)		db[t].flags2
#define Flags3(t)		db[t].flags3
#define Flags4(t)		db[t].flags4
#define Toggles(t)		db[t].toggles
#define Toggles2(t)		db[t].toggles2
#define Toggles3(t)		db[t].toggles3
#define Toggles4(t)		db[t].toggles4
#define Toggles5(t)		db[t].toggles5
#define Toggles6(t)		db[t].toggles6
#define Toggles7(t)		db[t].toggles7
#define Toggles8(t)		db[t].toggles8
#define	Home(t)			Link(t)
#define	Dropto(t)		Location(t)

#define	i_Name(t)		names[t] = NULL;
#define	s_Location(t,n)		db[t].location = (n)
#define	s_Zone(t,n)		((void)(n))
#define	s_Contents(t,n)		db[t].contents = (n)
#define	s_Exits(t,n)		db[t].exits = (n)
#define	s_Next(t,n)		db[t].next = (n)
#define	s_Link(t,n)		db[t].link = (n)
#define	s_Owner(t,n)		db[t].owner = (n)
#define	s_Parent(t,n)		db[t].parent = (n)
#define	s_Flags(t,n)		db[t].flags = (n)
#define	s_Flags2(t,n)		db[t].flags2 = (n)
#define s_Flags3(t,n)		db[t].flags3 = (n)
#define s_Flags4(t,n)		db[t].flags4 = (n)
#define s_Toggles(t,n)		db[t].toggles = (n)
#define s_Toggles2(t,n)		db[t].toggles2 = (n)
#define s_Toggles3(t,n)		db[t].toggles3 = (n)
#define s_Toggles4(t,n)		db[t].toggles4 = (n)
#define s_Toggles5(t,n)		db[t].toggles5 = (n)
#define s_Toggles6(t,n)		db[t].toggles6 = (n)
#define s_Toggles7(t,n)		db[t].toggles7 = (n)
#define s_Toggles8(t,n)		db[t].toggles8 = (n)

#define	s_Home(t,n)		s_Link(t,n)
#define	s_Dropto(t,n)		s_Location(t,n)

extern int	FDECL(Pennies, (dbref));
extern void	FDECL(s_Pennies, (dbref, int));

extern int 	FDECL(zlist_inlist, (dbref, dbref));
extern void	FDECL(zlist_destroy, (dbref));
extern void 	FDECL(zlist_del, (dbref,dbref));
extern void 	FDECL(zlist_add, (dbref, dbref));

extern void	NDECL(tf_init);
extern int	FDECL(tf_open, (char *, int));
extern int	FDECL(tf_socket, (int, int));
extern void	FDECL(tf_close, (int));
extern FILE *	FDECL(tf_fopen, (char *, int));
extern void	FDECL(tf_fclose, (FILE *));
extern FILE *	FDECL(tf_popen, (char *, int));
#define tf_pclose(f)	tf_fclose(f)

extern dbref	FDECL(getref, (FILE *));
extern void	FDECL(putref, (FILE *, dbref));
extern BOOLEXP *FDECL(dup_bool, (BOOLEXP *));
extern void	FDECL(free_boolexp, (BOOLEXP *));
extern dbref	FDECL(parse_dbref, (const char *));
extern int	FDECL(mkattr, (char *));
extern void	FDECL(al_add, (dbref, int));
extern void	FDECL(al_delete, (dbref, int));
extern void	FDECL(al_destroy, (dbref));
extern void	NDECL(al_store);
extern void	NDECL(val_count);
extern int	FDECL(atrcint, (dbref, dbref, int, char *));
extern void	FDECL(db_grow, (dbref));
extern void	NDECL(db_free);
extern void	NDECL(db_make_minimal);
extern dbref	FDECL(db_read, (FILE *, int *, int *, int *));
extern dbref	FDECL(db_write, (FILE *, int, int));
extern void	FDECL(showdbstats, (dbref));

#define	DOLIST(thing,list) \
	for ((thing)=(list); \
	     ((thing)!=NOTHING) && (Next(thing)!=(thing)); \
	     (thing)=Next(thing))
#define	SAFE_DOLIST(thing,next,list) \
	for ((thing)=(list),(next)=((thing)==NOTHING ? NOTHING: Next(thing)); \
	     (thing)!=NOTHING && (Next(thing)!=(thing)); \
	     (thing)=(next), (next)=Next(next))
#define	DO_WHOLE_DB(thing) \
	for ((thing)=0; (thing)<mudstate.db_top; (thing)++)

#define	Dropper(thing)	(Connected(Owner(thing)) && Hearer(thing))

#endif				/* __DB_H */
