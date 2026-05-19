/* htab.h - Structures and declarations needed for table hashing */

#include "copyright.h"

#ifndef _M_HTAB_H
#define _M_HTAB_H

#include "db.h"

typedef struct hashentry HASHENT;
struct hashentry {
	char			*target;
	int			*data;
        int                      bIsOriginal;
	struct hashentry	*next;
};

typedef struct num_hashentry NHSHENT;
struct num_hashentry {
	int			target;
	int			*data;
	struct num_hashentry	*next;
};

typedef struct hasharray HASHARR;
struct hasharray {
	HASHENT		*element[500];
};

typedef struct num_hasharray NHSHARR;
struct num_hasharray {
	NHSHENT		*element[500];
};

typedef struct hashtable HASHTAB;
struct hashtable {
	int		hashsize;
	int		mask;
	double		checks;
	double		scans;
	int		max_scan;
	double		hits;
	int		entries;
	int		deletes;
	int		nulls;
	HASHARR		*entry;
	int		last_hval;      /* Used for hashfirst & hashnext. */
	HASHENT		*last_entry;    /* Used for hashfirst & hashnext. */
        int             bOnlyOriginals; /* Used for hashfirst & hashnext. */
};

typedef struct num_hashtable NHSHTAB;
struct num_hashtable {
	int		hashsize;
	int		mask;
	double		checks;
	double		scans;
	int		max_scan;
	double		hits;
	int		entries;
	int		deletes;
	int		nulls;
	NHSHARR		*entry;
};

typedef struct name_table NAMETAB;
struct name_table {
	char	*name;
	int	minlen;
	unsigned int	perm;
	unsigned int	perm2;
	unsigned int	flag;
};

/* BQUE - Command queue - Hot/Cold split for cache efficiency.
 *
 * BQUE (~56 bytes, 1 cache line) contains hot fields accessed during
 * queue traversal (do_second, halt_que, etc.):
 *   next, player, cause, sem, waittime, pid, stop_bool, stop_bool_val, plr_type
 *
 * BQUE_COLD (~692 bytes) contains fields accessed only during command execution:
 *   text, comm, env[], scr[], scrname[], nargs, shellprg, text_len, etc.
 */

typedef struct bque_cold BQUE_COLD;
struct bque_cold {
	char	*text;
	char	*comm;
	char	*env[NUM_ENV_VARS];
	char	*scr[MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST];
	char	*scrname[MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST];
	int	nargs;
	int	shellprg;
	int	text_len;
	int	hooked_command;
	int	bitwise_flags;
};

typedef struct bque BQUE;
struct bque {
	BQUE	*next;
	dbref	player;
	dbref	cause;
	dbref	sem;
	double	waittime;
	int	pid;
	int	stop_bool;
	double	stop_bool_val;
	int	plr_type;
	BQUE_COLD *cold;
};

/* BQUE cold accessor macros */
#define Q_TEXT(d)       ((d)->cold->text)
#define Q_COMM(d)       ((d)->cold->comm)
#define Q_ENV(d)        ((d)->cold->env)
#define Q_SCR(d)        ((d)->cold->scr)
#define Q_SCRNAME(d)    ((d)->cold->scrname)
#define Q_NARGS(d)      ((d)->cold->nargs)
#define Q_SHELLPRG(d)   ((d)->cold->shellprg)
#define Q_TEXT_LEN(d)   ((d)->cold->text_len)
#define Q_HOOKED(d)     ((d)->cold->hooked_command)
#define Q_BITWISE(d)    ((d)->cold->bitwise_flags)


extern void	FDECL(real_hashinit, (HASHTAB *, int, const char *, int));
extern void	FDECL(real_hashreset, (HASHTAB *, const char *, int));
extern int	FDECL(real_hashval, (char *, int, const char *, int));
extern int	FDECL(real_get_hashmask, (int *, const char *, int));
extern int *	FDECL(real_hashfind, (char *, HASHTAB *, const char *, int));
extern int *	FDECL(real_hashfind2, (char *, HASHTAB *, int, const char *, int));
extern int	FDECL(real_hashadd, (char *, int *, HASHTAB *, const char *, int));
extern int	FDECL(real_hashadd2, (char *, int *, HASHTAB *, int, const char *, int));
extern void	FDECL(real_hashdelete, (char *, HASHTAB *, const char *, int));
extern void	FDECL(real_hashflush, (HASHTAB *, int, const char *, int));
extern int	FDECL(real_hashrepl, (char *, int *, HASHTAB *, const char *, int));
extern int	FDECL(real_hashrepl2, (char *, int *, HASHTAB *, int, const char *, int));
extern char *	FDECL(real_hashinfo, (const char *, HASHTAB *, const char *, int));
extern int *	FDECL(real_nhashfind, (int, NHSHTAB *, const char *, int));
extern int	FDECL(real_nhashadd, (int, int *, NHSHTAB *, const char *, int));
extern void	FDECL(real_nhashdelete, (int, NHSHTAB *, const char *, int));
extern void	FDECL(real_nhashflush, (NHSHTAB *, int, const char *, int));
extern int	FDECL(real_nhashrepl, (int, int *, NHSHTAB *, const char *, int));
extern int	FDECL(real_search_nametab, (dbref, NAMETAB *, char *, const char *, int));
extern NAMETAB * FDECL(real_find_nametab_ent, (dbref, NAMETAB *, char *, const char *, int));
extern void	FDECL(real_display_nametab, (dbref, NAMETAB *, char *, int, const char *, int));
extern void	FDECL(real_interp_nametab, (dbref, NAMETAB *, int, char *, char *, char *, const char *, int));
extern void	FDECL(real_listset_nametab, (dbref, NAMETAB *, NAMETAB *, int, int, char *, int, const char *, int));
extern int *	FDECL(real_hash_nextentry, (HASHTAB *htab, const char *, int));
extern int *	FDECL(real_hash_firstentry, (HASHTAB *htab, const char *, int));
extern int *	FDECL(real_hash_firstentry2, (HASHTAB *htab, int, const char *, int));

#define hashinit(a,b)                    real_hashinit(a, b ,__FILE__, __LINE__)
#define hashreset(a)                     real_hashreset(a ,__FILE__, __LINE__)
#define hashval(a, b)                    real_hashval(a, b, __FILE__, __LINE__)
#define get_hashmask(a)                  real_get_hashmask, (a, __FILE__, __LINE__)
#define hashfind(a, b)                   real_hashfind(a, b, __FILE__, __LINE__)
#define hashfind2(a, b, c)               real_hashfind2(a, b, c, __FILE__, __LINE__)
#define hashadd(a, b, c)                 real_hashadd(a, b, c, __FILE__, __LINE__)
#define hashadd2(a, b, c, d)             real_hashadd2(a, b, c, d, __FILE__, __LINE__)
#define hashdelete(a, b)                 real_hashdelete(a, b, __FILE__, __LINE__)
#define hashflush(a, b)                  real_hashflush(a, b, __FILE__, __LINE__)
#define hashrepl(a, b, c)                real_hashrepl(a, b, c, __FILE__, __LINE__)
#define hashrepl2(a, b, c, d)            real_hashrepl2(a, b, c, d, __FILE__, __LINE__)
#define hashinfo(a, b)                   real_hashinfo(a, b, __FILE__, __LINE__)
#define nhashfind(a, b)                  real_nhashfind(a, b, __FILE__, __LINE__)
#define nhashadd(a, b, c)                real_nhashadd(a, b, c, __FILE__, __LINE__)
#define nhashdelete(a, b)                real_nhashdelete(a, b, __FILE__, __LINE__)
#define nhashflush(a, b)                 real_nhashflush(a, b, __FILE__, __LINE__)
#define nhashrepl(a, b, c)               real_nhashrepl(a, b, c, __FILE__, __LINE__)
#define search_nametab(a, b, c)          real_search_nametab(a, b, c, __FILE__, __LINE__)
#define find_nametab_ent(a, b, c)        real_find_nametab_ent(a, b, c, __FILE__, __LINE__)
#define display_nametab(a, b, c, d)      real_display_nametab(a, b, c, d, __FILE__, __LINE__)
#define interp_nametab(a, b, c, d, e, f) real_interp_nametab(a, b, c, d, e, f, __FILE__, __LINE__)
#define listset_nametab(a, b, c, d, e, f, g) real_listset_nametab(a, b, c, d, e, f, g, __FILE__, __LINE__)
#define hash_nextentry(a)                real_hash_nextentry(a, __FILE__, __LINE__)
#define hash_firstentry(a)               real_hash_firstentry(a, __FILE__, __LINE__)
#define hash_firstentry2(a, b)           real_hash_firstentry2(a, b, __FILE__, __LINE__)

extern NAMETAB powers_nametab[];
#define nhashinit(h,s) hashinit((HASHTAB *)h, s)
#define nhashreset(h) hashreset((HASHTAB *)h)
#define nhashinfo(t,h) hashinfo(t,(HASHTAB *)h)

#endif
