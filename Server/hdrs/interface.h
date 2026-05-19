/* interface.h */

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

/* Define WebSocket handler checksum length -- IF YOU CHANGE YOU MUST @shutdown then Startmush */
#define WEBSOCKETS_CHECKSUM_LEN 40

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
#define R_WEBSOCKETS	14	/* WebSocket connection */
#define R_SU            15      /* Account was SU'd */

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
 *
 * This has been modified.  Going forward from Rhost 4.0.0p4 any changes
 * to this file is allowed but ONLY FOR ADDING NEW DATA AT THE END.
 * You can not:
 *   1.  Alter any of the field values including sizes
 *   2.  Change the order of any of the values.
 *   3.  Shrink/remove fields after adding them
 *
 * If you do any of the two with the DESC data, a @reboot WILL CRASH.
 *    - Ashen-Shugar 12/2019
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

/* We define this to allow people who are on older systems to boot new desc data w/o crashing */
typedef struct descriptor_data_orig DESCORIG;
struct descriptor_data_orig {
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
  char userid[MBUF_SIZE];	    /* added 2/95 Thorin */
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

/* Forward declarations for hot/cold split */
typedef struct desc_hot DESC_HOT;
typedef struct desc_cold DESC_COLD;
typedef struct descriptor_data DESC;

/* ************************************************************************
 * DESC Hot/Cold Split — Cache-optimized descriptor layout.
 *
 * DESC_HOT (~104 bytes, 2 cache lines) contains fields accessed every tick:
 *   flags, descriptor, player, quota, input/output queues, host_info,
 *   input/output sizes, last_time, hashnext, next, prev.
 *
 * DESC_COLD (~500+ bytes) contains rarely-accessed fields:
 *   Embedded arrays (addr, doing, userid, longaddr, account_rawpass, checksum),
 *   Door fields, snooplist, auth fields, websocket fields, etc.
 *
 * NOTE: This breaks reboot compatibility. Old .reboot files will be skipped.
 * Connected players will reconnect automatically on first boot with this change.
 */

/* Hot descriptor data — accessed every game tick */
struct desc_hot {
    int          descriptor;      /* socket fd */
    int          flags;           /* DS_CONNECTED, DS_API, etc. */
    int          quota;           /* command quota */
    int          host_info;       /* site flags */
    dbref        player;          /* connected player */
    /* 4 bytes padding on 64-bit */
    CBLK        *input_head;      /* input queue */
    CBLK        *input_tail;      /* input tail */
    TBLOCK      *output_head;     /* output queue */
    TBLOCK      *output_tail;     /* output tail */
    int          input_tot;       /* total input bytes */
    int          input_size;      /* input buffer size */
    int          output_tot;      /* total output bytes */
    int          output_size;     /* output buffer size */
    time_t       last_time;       /* last activity */
    DESC        *hashnext;        /* hash chain */
    DESC        *next;            /* list forward */
    DESC       **prev;            /* list back-pointer */
};

/* Cold descriptor data — accessed infrequently */
struct desc_cold {
    /* Embedded arrays (~680 bytes) */
    char  addr[51];
    char  doing[256];
    char  userid[MBUF_SIZE];
    char  longaddr[256];
    char  account_rawpass[100];
    char  checksum[WEBSOCKETS_CHECKSUM_LEN + 1];

    /* Pointers */
    char  *output_prefix;
    char  *output_suffix;
    CBLK  *raw_input;
    char  *raw_input_at;
    struct SNOOPLISTNODE *snooplist;
    char  *door_lbuf;
    char  *door_mbuf;
    char  *door_raw;

    /* Door fields */
    TBLOCK *door_output_head;
    TBLOCK *door_output_tail;
    CBLK   *door_input_head;
    CBLK   *door_input_tail;
    int     door_desc;
    int     door_num;
    int     door_output_size;
    int     door_int1;
    int     door_int2;
    int     door_int3;

    /* Integers */
    int     retries_left;
    int     regtries_left;
    int     command_count;
    int     timeout;
    int     output_lost;
    int     input_lost;
    int     logged;
    int     authdescriptor;
    int     longaddrcheck;
    int     addr_family;
    long    ws_frame_len;
    unsigned short remote_port;

    /* Other */
    time_t  connected_at;
    dbref   account_owner;
    struct sockaddr_in address;
};

typedef struct descriptor_data DESC;
struct descriptor_data {
    DESC_HOT     hot;
    DESC_COLD   *cold;
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
#define DS_WEBSOCKETS_REQUEST   0x1000          /* Target is negotiating WebSockets */
#define DS_WEBSOCKETS           0x2000          /* Target is a WebSocket */


extern DESC *descriptor_list;
extern DESC *desc_in_use;

/* from the net interface */

extern void	NDECL(emergency_shutdown);
extern void	FDECL(shutdownsock, (DESC *, int));
extern void	FDECL(shovechars, (int, char *, char *, int));
extern void	NDECL(set_signals);
extern void	FDECL(start_auth, (DESC *));
extern void 	FDECL(check_auth_connect, (DESC *));
extern void 	FDECL(check_auth, (DESC *));
extern void	FDECL(write_auth, (DESC *));

/* from utils.c */
#define CONN_TIME       1       /* Total Connected Time in seconds */
#define CONN_LONGEST    2       /* Longest Connection in seconds */
#define CONN_LAST       4       /* Duration of Last Connection */
#define CONN_TOTAL      8       /* Total number of connections */
#define CONN_LOGOUT     16      /* EPOCH (time_t) of last logout */
#define CONN_ALL        32      /* Do everything */
extern void handle_conninfo_write(DESC *, dbref, int);
extern int handle_conninfo_read(char *, dbref, int);

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
extern int	FDECL(site_check_str, (const char *, int, SITE *, int, int, int));
extern int	FDECL(blacklist_check, (struct in_addr host, int));
extern int	FDECL(blacklist_check_str, (const char *, int, int));
extern void	FDECL(make_ulist, (dbref, char *, char **, int, dbref, int));
extern dbref	FDECL(find_connected_name, (dbref, char *));

/* From predicates.c */

#define alloc_desc(s) _alloc_desc(s)
#define free_desc(b) _free_desc(b)
#define alloc_desc_cold(s) (DESC_COLD *)pool_alloc(POOL_DESC_COLD,s,__LINE__,__FILE__)
#define free_desc_cold(b) pool_free(POOL_DESC_COLD,((char **)&(b)),__LINE__,__FILE__)

extern DESC *_alloc_desc(const char *);
extern void _free_desc(DESC *);

#define DESC_ITER_PLAYER(p,d) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab);d;d=d->hot.hashnext)
#define DESC_ITER_CONN(d) \
	for (d=descriptor_list;(d);d=(d)->hot.next) \
		if ((d)->hot.flags & DS_CONNECTED)
#define DESC_ITER_ALL(d) \
	for (d=descriptor_list;(d);d=(d)->hot.next)

#define DESC_SAFEITER_PLAYER(p,d,n) \
	for (d=(DESC *)nhashfind((int)p,&mudstate.desc_htab), \
        	n=((d!=NULL) ? d->hot.hashnext : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->hot.hashnext : NULL))
#define DESC_SAFEITER_CONN(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->hot.next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->hot.next : NULL)) \
		if ((d)->hot.flags & DS_CONNECTED)
#define DESC_SAFEITER_ALL(d,n) \
	for (d=descriptor_list,n=((d!=NULL) ? d->hot.next : NULL); \
	     d; \
	     d=n,n=((n!=NULL) ? n->hot.next : NULL))

#define MALLOC(result, type, number, where) do { \
	if (!((result)=(type *) XMALLOC (((number) * sizeof (type)), where))) \
		panic("Out of memory", 1);				\
	} while (0)

#endif
