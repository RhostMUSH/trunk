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
 * If you add or reorder fields in `desc_hot_arrays`, you MUST mirror the
 * change in BOTH `dump_reboot_db()` (write) and `load_reboot_db()` (read)
 * in src/netcommon.c.  The serialization order must match exactly between
 * write and read, and must match the struct field order.
 *
 * If you add or reorder fields in `DESC_COLD`, update `sizeof(DESC_COLD)`
 * — the cold bulk-write in `dump_reboot_db()` and cold bulk-read in
 * `load_reboot_db()` handle size mismatches via the `i_file_cold_size`
 * guard (reads the lesser of file-size vs current-size, zero-filling
 * leftover bytes on upgrade).
 *
 * Bump the reboot format version (i_reboot_version) whenever the
 * hot field serialization order changes.  Old .reboot files will be
 * rejected with a clear error message — admin deletes the file and
 * cold-starts.  This is safer than silent data corruption.
 *
 *    - updated 2026-05-26 for v4 SoA serialization
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

/* Forward declarations for hot/cold split */
typedef struct desc_cold DESC_COLD;
typedef struct descriptor_data DESC;

/* ************************************************************************
 * DESC Hot/Cold Split — SoA (Structure of Arrays) layout.
 *
 * struct desc_hot_arrays: 15 parallel arrays indexed by slot_index.
 *   At 150 descriptors: flags[150]=600B fits in L1. At 500: 2KB per field.
 *   Accessor macros: D_FLAGS(d), D_PLAYER(d), etc.
 *
 * DESC_COLD (~500+ bytes) contains rarely-accessed fields:
 *   Embedded arrays (addr, doing, userid, longaddr, account_rawpass, checksum),
 *   Door fields, snooplist, auth fields, websocket fields, etc.
 *
 * NOTE: This breaks reboot compatibility. Old .reboot files will be skipped.
 * Connected players will reconnect automatically on first boot with this change.
 */

/* Maximum simultaneous descriptors (configurable via -DDYN_MAXDESCRIPTORS=N) */
#ifdef DYN_MAXDESCRIPTORS
#define MAX_DESCRIPTORS DYN_MAXDESCRIPTORS
#else
#define MAX_DESCRIPTORS 150
#endif

/* Hot descriptor data — Structure of Arrays (SoA) for cache efficiency.
 * Each field is a parallel array indexed by slot_index.
 * At 500 descriptors: flags[500]=2KB vs AoS 44KB per-iteration working set.
 * Accessor macros: D_FLAGS(d), D_PLAYER(d), etc. */
struct desc_hot_arrays {
    int          descriptor[MAX_DESCRIPTORS];
    int          flags[MAX_DESCRIPTORS];
    int          quota[MAX_DESCRIPTORS];
    int          host_info[MAX_DESCRIPTORS];
    dbref        player[MAX_DESCRIPTORS];
    CBLK        *input_head[MAX_DESCRIPTORS];
    CBLK        *input_tail[MAX_DESCRIPTORS];
    TBLOCK      *output_head[MAX_DESCRIPTORS];
    TBLOCK      *output_tail[MAX_DESCRIPTORS];
    int          input_tot[MAX_DESCRIPTORS];
    int          input_size[MAX_DESCRIPTORS];
    int          output_tot[MAX_DESCRIPTORS];
    int          output_size[MAX_DESCRIPTORS];
    time_t       last_time[MAX_DESCRIPTORS];
};

extern struct desc_hot_arrays desc_hot;

/* Accessor macros — d->hot.X becomes D_X(d) */
#define D_DESCRIPTOR(d)  (desc_hot.descriptor[(d)->slot_index])
#define D_FLAGS(d)       (desc_hot.flags[(d)->slot_index])
#define D_QUOTA(d)       (desc_hot.quota[(d)->slot_index])
#define D_HOST_INFO(d)   (desc_hot.host_info[(d)->slot_index])
#define D_PLAYER(d)      (desc_hot.player[(d)->slot_index])
#define D_INPUT_HEAD(d)  (desc_hot.input_head[(d)->slot_index])
#define D_INPUT_TAIL(d)  (desc_hot.input_tail[(d)->slot_index])
#define D_OUTPUT_HEAD(d) (desc_hot.output_head[(d)->slot_index])
#define D_OUTPUT_TAIL(d) (desc_hot.output_tail[(d)->slot_index])
#define D_INPUT_TOT(d)   (desc_hot.input_tot[(d)->slot_index])
#define D_INPUT_SIZE(d)  (desc_hot.input_size[(d)->slot_index])
#define D_OUTPUT_TOT(d)  (desc_hot.output_tot[(d)->slot_index])
#define D_OUTPUT_SIZE(d) (desc_hot.output_size[(d)->slot_index])
#define D_LAST_TIME(d)   (desc_hot.last_time[(d)->slot_index])

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

    /* Telnet/NAWS state */
    void   *telnet;             /* libtelnet state for this connection */
    unsigned short term_width;  /* NAWS terminal width (0 = unknown) */
    unsigned short term_height; /* NAWS terminal height (0 = unknown) */

    /* WebSocket control */
    int     ws_closing;         /* 1 = close frame received, pending shutdown */
    char    ws_fragmented;      /* 1 = in-progress fragmented message */
    time_t  ws_last_pong;       /* timestamp of last PONG received */

    /* Client detection (TTYPE / capabilities) */
    char    client_name[64];    /* TTYPE terminal type, e.g. "Mudlet" */
    char    client_name_first[64]; /* first TTYPE response in cycle for end-of-list detection */
    int     client_caps;        /* bitmask: 0x01 = unicode detected */
};

/* Client capability flags */
#define CLIENT_CAP_UNICODE    0x01
#define CLIENT_MTTS_SEEN     0x02
#define CLIENT_CAP_ANSI      0x04
#define CLIENT_CAP_256COLOR  0x08
#define CLIENT_CAP_TRUECOLOR 0x10
#define CLIENT_TTYPE_SEND1   0x20
#define CLIENT_TTYPE_SEND2   0x40
#define CLIENT_TTYPE_SEND3   0x80

typedef struct descriptor_data DESC;
struct descriptor_data {
    int          slot_index;
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


extern DESC desc_slots[MAX_DESCRIPTORS];
extern DESC_COLD desc_cold[MAX_DESCRIPTORS];
extern DESC *desc_in_use;

/* from the net interface */

extern void	NDECL(emergency_shutdown);
extern void	FDECL(shutdownsock, (DESC *, int));
extern void	FDECL(shovechars, (int, char *, char *, int));
extern const char *NDECL(get_listening_info);
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

extern DESC *_alloc_desc(const char *);
extern void _free_desc(DESC *);

extern int ndescriptors;    /* total open FDs: main + doors + auth */
extern int ndesc_slots;     /* entries in desc_slots array only */

#define DESC_ITER_PLAYER(p,d) \
	for (int _di = 0; _di < ndesc_slots && ((d) = &desc_slots[_di], (d)->slot_index = _di, 1); _di++) \
	    if (D_PLAYER(d) == (p))
#define DESC_ITER_CONN(d) \
	for (int _di = 0; _di < ndesc_slots && ((d) = &desc_slots[_di], (d)->slot_index = _di, 1); _di++) \
	    if (D_FLAGS(d) & DS_CONNECTED)
#define DESC_ITER_ALL(d) \
	for (int _di = 0; _di < ndesc_slots && ((d) = &desc_slots[_di], (d)->slot_index = _di, 1); _di++)

#define DESC_SAFEITER_PLAYER(p,d,n) \
	for (int _di = ndesc_slots - 1; _di >= 0 && ((d) = &desc_slots[_di], (d)->slot_index = _di, (n) = d, 1); _di--) \
	    if (D_PLAYER(d) == (p))
#define DESC_SAFEITER_CONN(d) \
	for (int _di = ndesc_slots - 1; _di >= 0 && ((d) = &desc_slots[_di], (d)->slot_index = _di, 1); _di--) \
	    if (D_FLAGS(d) & DS_CONNECTED)
#define DESC_SAFEITER_ALL(d) \
	for (int _di = ndesc_slots - 1; _di >= 0 && ((d) = &desc_slots[_di], (d)->slot_index = _di, 1); _di--)

#define MALLOC(result, type, number, where) do { \
	if (!((result)=(type *) XMALLOC (((number) * sizeof (type)), where))) \
		panic("Out of memory", 1);				\
	} while (0)

#endif
