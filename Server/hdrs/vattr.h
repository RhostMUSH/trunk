#ifndef _M_VATTR_H_
#define _M_VATTR_H_

/*
 *  Definitions for user-defined attributes
 */

// #define VHASH_SIZE	256		/* Must be a power of 2 */
// #define VNHASH_SIZE	256
// #define VHASH_MASK	0xff		/* AND mask to get 0..hsize-1 */
// #define VNHASH_MASK	0xff

/* Set to 4096 as 256 was too damn low */
#define VHASH_SIZE	16384		/* Must be a power of 2 */
#define VNHASH_SIZE	16384
/* This is HASH_SIZE - 1 */
#define VHASH_MASK	0x3fff		/* AND mask to get 0..hsize-1 */
#define VNHASH_MASK	0x3FFF

/* This number will allocate entries 32k bytes at a time. */
#define VALLOC_SIZE	630
#ifdef SBUF64
#define VNAME_SIZE	64
#else
#define VNAME_SIZE	32
#endif

typedef struct user_attribute VATTR;
struct user_attribute {
	char	*name; /* Name of user attribute */
	int	number;		/* Assigned attribute number */
	int	flags;		/* Attribute flags */
	int	command_flag;	/* turned into @command */
	struct user_attribute *next;	/* name hash chain. */
};

extern void	vattr_init(void);
extern VATTR *	vattr_rename(char *, char *);
extern VATTR *	vattr_find(char *);
extern VATTR *	vattr_nfind(int);
extern VATTR *	vattr_alloc(char *, int);
extern VATTR *	vattr_define(char *, int, int);
extern void	vattr_delete(char *);
extern VATTR *	attr_rename(char *, char *);
extern VATTR *	vattr_first(void);
extern VATTR *	vattr_next(VATTR *);
extern void	list_vhashstats(dbref);
extern int	sum_vhashstats(dbref);

#endif

