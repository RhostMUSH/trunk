/* file_c.h -- File cache header file */

#include "copyright.h"
#ifndef _M_FILE_C
#define	_M_FILE_C

#include "db.h"
#include "interface.h"

/* File caches.  These _must_ track the fcache array in file_c.c */

#define	FC_CONN		0
#define	FC_CONN_SITE	1
#define	FC_CONN_DOWN	2
#define	FC_CONN_FULL	3
#define	FC_CONN_GUEST	4
#define	FC_CONN_REG	5
#define	FC_CREA_NEW	6
#define	FC_CREA_REG	7
#define	FC_MOTD		8
#define	FC_WIZMOTD	9
#define	FC_QUIT		10
#define FC_GUEST_FAIL	11
#define FC_CONN_AUTO	12
#define FC_CONN_AUTO_H	13
#define	FC_LAST		13

/* File cache routines */

extern void	fcache_rawdump(int fd, int num, struct in_addr host, char *s_site);
extern void	fcache_dump(DESC *d, int num, char *s_site);
extern void	fcache_send(dbref, int);
extern void	fcache_load(dbref);
extern void	fcache_init(void);

#endif
