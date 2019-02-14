/* inerface.h */

#include "copyright.h"

#ifndef _M_INTERFACE__H
#define _M_INTERFACE__H

#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* these symbols must be defined by the interface */

extern int shutdown_flag; /* if non-zero, interface should shut down */

/* Disconnection reason codes */

#define	R_QUIT		1	/* User quit */
#define	R_TIMEOUT	2	/* Inactivity timeout */
#define	R_BOOT		3	/* Victim of @boot, @toad, or @destroy */
#define	R_SOCKDIED	4	/* Other end of socked closed it */
#define	R_GOING_DOWN	5	/* Game is going down */
#define	R_BADLOGIN	6	/* Too many failed login attempts */
#define	R_GAMEDOWN	7	/* Not admitting users now */
#define	R_LOGOUT	8	/* Logged out w/o disconnecting */
#define R_GAMEFULL	9	/* Too many players logged in */
#define R_REBOOT	10	/* @reboot done */
#define R_HACKER        11      /* User tried to hack connect screen */
#define R_NODESCRIPTOR  12      /* No 'free' descriptors - actually buffer zone */
#define R_API		13	/* API Connection */

extern NAMETAB logout_cmdtable[];

typedef struct cmd_block_hdr CBLKHDR;
typedef struct cmd_block CBLK;

struct cmd_block {
	struct cmd_block_hdr {
		struct cmd_block *nxt;
	}	hdr;
	char	cmd[LBUF_SIZE - sizeof(CBLKHDR)];
};

typedef struct text_block_hdr TBLKHDR;
typedef struct text_block TBLOCK;

struct text_block {
	struct text_block_hdr {
		struct text_block *nxt;
		char	*start;
		char	*end;
		int	nchars;
	}	hdr;
	char	data[LBUF_SIZE - sizeof(TBLKHDR)];
};

struct SNOOPLISTNODE {
	dbref snooper;
	FILE *sfile;
	int logonly;
	struct SNOOPLISTNODE *next;
};

/* ************************************************************************
 * BIG NOTE!!!!
 *
 * If you change anything in the DESC structure be sure to analyze the
 * effects of your change as it pertains to the @reboot code in bsd.c,
 * netcommon.c, and game.c. Also note that you MUST use @shutdown to
 * bring in any new code that has had the DESC structure changed. If not
 * then an @reboot will write out the old records to a file, then read
 * them in with the new structure and everything will be hosed!
 *    - Thorin 01/1997
*/

/* OK, let's make a temporary player desc data descriptor here */
typedef struct descriptor_data_online DESC_ONLINE;
struct descriptor_data_online {
  int version;
  int descriptor;
  int width;
  int height;
  char addr[255];
  dbref player;
};

typedef struct descriptor_data DESC;
struct descriptor_data {
  int descriptor;
  int flags;
  int retries_left;
  int regtries_left;
  int command_count;
  int timeout;
  int host_info;
  char addr[51];
  char doing[256];
  dbref player;
  char *output_prefix;
  char *output_suffix;
  int output_size;
  int output_tot;
  int output_lost;
  TBLOCK *output_head;
  TBLOCK *output_tail;
  int input_size;
  int input_tot;
  int input_lost;
  CBLK *input_head;
  CBLK *input_tail;
  CBLK *raw_input;
  char *raw_input_at;
  time_t connected_at;
  time_t last_time;
  int quota;
  struct sockaddr_in address;	/* added 3/6/90 SCG */
  struct descriptor_data *hashnext;
  struct descriptor_data *next;
  struct descriptor_data **prev;
  struct SNOOPLISTNODE *snooplist;  /* added 2/95 Thorin */
  int logged;
  int authdescriptor;		    /* added 2/95 Thorin */
  char userid[MBUF_SIZE];	    /* added 2/95 thorin */
  int door_desc;		/* added 11/15/97 Seawolf */
  int door_num;			/* added 11/15/97 Seawolf */
  TBLOCK *door_output_head;
  TBLOCK *door_output_tail;
  int door_output_size;
  CBLK *door_input_head;
  CBLK *door_input_tail;
  int door_int1;
  int door_int2;
  int door_int3;
  char *door_lbuf;
  char *door_mbuf;
  char *door_raw;
};

/* flags in the flag field */
#define	DS_CONNECTED		0x0001		/* player is connected */
#define	DS_AUTODARK		0x0002		/* Wizard was auto set dark. */
#define DS_AUTH_IN_PROGRESS	0x0004		/* Working to grab userid */
#define DS_NEED_AUTH_WRITE  	0x0008		/* send req when ok */
#define DS_HAS_DOOR		0x0010		/* player has door socket open */
#define DS_AUTOUNF              0x0020          /* Wizard was auto set unfindable. */
#define DS_AUTH_CONNECTING      0x0040          /* AUTH doing non-blocking connect */
#define DS_HAVEpFX		0x0080		/* Target has prefix */
#define DS_HAVEsFX		0x0100		/* Target has suffix */
#define DS_API			0x0200		/* Target is an API handler */
#define DS_CMDQUOTA		0x0400		/* Target is an CMDQUOTA handler */
#define DS_SSL      		0x0800		/* Target is on an SSL handler */

extern DESC *descriptor_list;
extern DESC *desc_in_use;

/* from the net interface */

extern void	NDECL(emergency_shutdown);
extern void	FDECL(shutdownsock, (DESC *, int));
extern void	FDECL(shovechars, (int, char*));
extern void	NDECL(set_signals);
extern void	FDECL(start_auth, (DESC *));
extern void 	FDECL(check_auth_connect, (DESC *));
extern void 	FDECL(check_auth, (DESC *));
extern void	FDECL(write_auth, (DESC *));

/* from netcommon.c */

extern struct timeval	FDECL(timeval_sub, (struct timeval, struct timeval));
extern int	FDECL(msec_diff, (struct timeval now, struct timeval then));
extern struct timeval	FDECL(msec_add, (struct timeval, int));
extern struct timeval	FDECL(update_quotas, (struct timeval, struct timeval));
extern void	FDECL(raw_notify, (dbref, const char *, int, int));
extern void	FDECL(clearstrings, (DESC *));
extern void	FDECL(queue_write, (DESC *, const char *, int));
extern void	FDECL(queue_string, (DESC *, const char *));
extern void	FDECL(freeqs, (DESC *, int));
extern void	FDECL(welcome_user, (DESC *));
extern void	FDECL(save_command, (DESC *, CBLK *));
extern void	FDECL(announce_disconnect, (dbref, DESC *, const char *));
extern int	FDECL(boot_off, (dbref, char *));
extern int	FDECL(boot_by_port, (int, int, int, char *));
extern int	FDECL(fetch_idle, (dbref));
extern int	FDECL(fetch_connect, (dbref));
extern void	NDECL(check_idle);
extern void	NDECL(process_commands);
extern int	FDECL(site_check, (struct in_addr, SITE *, int, int, int));
extern int	FDECL(blacklist_check, (struct in_addr host, int));
extern void	FDECL(make_ulist, (dbref, char *, char **, int, dbref, int));
extern dbref	FDECL(find_connected_name, (dbref, char *));

/* From predicates.c */

#define alloc_desc(s) (DESC *)pool_alloc(POOL_DESC,s,__LINE__,__FILE__)
#define free_desc(b) pool_free(POOL_DESC,((char **)&(b)),__LINE__,__FILE__)

#define DESC_ITER_PLAYER(p,d) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab);d;d=d->hashnext)
#define DESC_ITER_CONN(d) \
	for (d=descriptor_list;(d);d=(d)->next) \
		if ((d)->flags & DS_CONNECTED)
#define DESC_ITER_ALL(d) \
	for (d=descriptor_list;(d);d=(d)->next)

#define DESC_SAFEITER_PLAYER(p,d,n) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab), \
        	n=((d!=NULL) ? d->hashnext : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->hashnext : NULL))
#define DESC_SAFEITER_CONN(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->next : NULL)) \
		if ((d)->flags & DS_CONNECTED)
#define DESC_SAFEITER_ALL(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->next : NULL))

#define MALLOC(result, type, number, where) do { \
	if (!((result)=(type *) XMALLOC (((number) * sizeof (type)), where))) \
		panic("Out of memory", 1);				\
	} while (0)

#endif
