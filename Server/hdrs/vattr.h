#ifndef _M_VATTR_H_
#define _M_VATTR_H_

/*
 *  Definitions for user-defined attributes
 */

#define VHASH_SIZE	256		/* Must be a power of 2 */
#define VNHASH_SIZE	256
#define VHASH_MASK	0xff		/* AND mask to get 0..hsize-1 */
#define VNHASH_MASK	0xff
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

extern void	NDECL(vattr_init);
extern VATTR *	FDECL(vattr_rename, (char *, char *));
extern VATTR *	FDECL(vattr_find, (char *));
extern VATTR *	FDECL(vattr_nfind, (int));
extern VATTR *	FDECL(vattr_alloc, (char *, int));
extern VATTR *	FDECL(vattr_define, (char *, int, int));
extern void	FDECL(vattr_delete, (char *));
extern VATTR *	FDECL(attr_rename, (char *, char *));
extern VATTR *	NDECL(vattr_first);
extern VATTR *	FDECL(vattr_next, (VATTR *));
extern void	FDECL(list_vhashstats, (dbref));
extern int	FDECL(sum_vhashstats, (dbref));

#endif

