/* mudconf.h:	runtime configuration structure */

#ifndef _M_CONF_H
#define _M_CONF_H

#ifdef VMS
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#else
#include <netinet/in.h>
#endif
#include "config.h"
#include "htab.h"
#include "alloc.h"
#include "flags.h"

/* CONFDATA:	runtime configurable parameters */

#define MAXEVLEVEL	10
typedef unsigned char Uchar;

typedef struct confdata CONFDATA;
struct confdata {
	int	cache_trim;	/* Should cache be shrunk to original size */
	int	cache_steal_dirty; /* Should cache code write dirty attrs */
	int	cache_depth;	/* Number of entries in each cache cell */
	int	cache_width;	/* Number of cache cells */
	int	cache_names;	/* Should object names be cached separately */
#ifndef STANDALONE
        char    data_dir[128];   /* Directory for database files */
        char    txt_dir[128];    /* Directory for txt and help files */
	char	image_dir[128];	/* Snapshot image directory */
	char	indb[128];	/* database file name */
	char	outdb[128];	/* checkpoint the database to here */
	char	crashdb[128];	/* write database here on crash */
	char	gdbm[128];	/* use this gdbm file if we need one */
	int	compress_db;	/* should we use compress */
	char	compress[128];	/* program to run to compress */
	char	uncompress[128];/* program to run to uncompress */
	char	status_file[128]; /* Where to write arg to @shutdown */
	char	roomlog_path[128]; /* Path where LOGROOM and LOGROOMENH is sent */
        char	logdb_name[128];/* Name of log db */
	int	round_kludge; /* Kludge workaround to fix rounding 2.5 to 2. [Loki] */
	char ip_address[15];
	int	port;		/* user port */
	int	html_port;	/* html port - Thorin 6/97 */
        int     debug_id;       /* shared memory key for debug monitor */
	int	authenticate;	/* Do we wish to use AUTH protocol? */
	int	init_size;	/* initial db size */
	int	have_guest;	/* Do we wish to allow a GUEST character? */
	int	max_num_guests;
	int	wall_cost;
	int	comcost;
	int	maildelete;
	int	mailsub;
	int	start_build;
	char	guest_file[32];	/* display if guest connects */
	char	conn_file[32];	/* display on connect if no registration */
	char	creg_file[32];	/* display on connect if registration */
	char	regf_file[32];	/* display on (failed) create if reg is on */
	char	motd_file[32];	/* display this file on login */
	char	wizmotd_file[32]; /* display this file on login to wizards */
	char	quit_file[32];	/* display on quit */
	char	down_file[32];	/* display this file if no logins */
	char	full_file[32];	/* display when max users exceeded */
	char	site_file[32];	/* display if conn from bad site */
	char	crea_file[32];	/* display this on login for new users */
	char	help_file[32];	/* HELP text file */
#ifdef PLUSHELP
	char	plushelp_file[32];	/* +HELP text file */
	char	plushelp_indx[32];	/* +HELP index file */
#endif
	char	help_indx[32];	/* HELP index file */
	char	news_file[32];	/* NEWS text file */
	char	news_indx[32];	/* NEWS index file */
	char	whelp_file[32];	/* Wizard help text file */
	char	whelp_indx[32];	/* Wizard help index file */
	char	error_file[32]; /* huh? file */
	char	error_indx[32]; /* huh? index */
	char	gconf_file[32];
	char	areg_file[32];
	char	aregh_file[32];
	char	door_file[32];
	char	door_indx[32];
        char    manlog_file[32]; /* Manual log file if /manual switch used */
        char    mailinclude_file[32]; /* File to include with autoregistration */
	char	mailprog[16];
	char	guild_default[12];
	char    guild_attrname[33];
	char	motd_msg[1024];	/* Wizard-settable login message */
	char	wizmotd_msg[1024];  /* Login message for wizards only */
	char	downmotd_msg[1024];  /* Settable 'logins disabled' message */
	char	fullmotd_msg[1024];  /* Settable 'Too many players' message */
	char	dump_msg[128];	/* Message displayed when @dump-ing */
	char	postdump_msg[128];  /* Message displayed after @dump-ing */
        char  spam_msg[128];    /* Message displayed when spammonitor kicks in */
        char  spam_objmsg[128]; /* Message displayed when object spammonitor kicks in */
	int	whereis_notify;
	int	max_size;
	int	name_spaces;	/* allow player names to have spaces */
	int	fork_dump;	/* perform dump in a forked process */
	int	fork_vfork;	/* use vfork to fork */
	int	sig_action;	/* What to do with fatal signals */
	int	paranoid_alloc;	/* Rigorous buffer integrity checks */
	int	max_players;	/* Max # of connected players */
	int	dump_interval;	/* interval between ckp dumps in seconds */
	int	check_interval;	/* interval between db check/cleans in secs */
	int	dump_offset;	/* when to take first checkpoint dump */
	int	check_offset;	/* when to perform first check and clean */
	int	idle_timeout;	/* Boot off players idle this long in secs */
	int	conn_timeout;	/* Allow this long to connect before booting */
	int	idle_interval;	/* when to check for idle users */
	int	retry_limit;	/* close conn after this many bad logins */
	int	regtry_limit;
	int	output_limit;	/* Max # chars queued for output */
	int	paycheck;	/* players earn this much each day connected */
	int	paystart;	/* new players start with this much money */
	int	paylimit;	/* getting money gets hard over this much */
	int	start_quota;	/* Quota for new players */
	int	payfind;	/* chance to find a penny with wandering */
	int	digcost;	/* cost of @dig command */
	int	linkcost;	/* cost of @link command */
	int	opencost;	/* cost of @open command */
	int	createmin;	/* default (and minimum) cost of @create cmd */
	int	createmax;	/* max cost of @create command */
	int	killmin;	/* default (and minimum) cost of kill cmd */
	int	killmax;	/* max cost of kill command */
	int	killguarantee;	/* cost of kill cmd that guarantees success */
	int	robotcost;	/* cost of @robot command */
	int	pagecost;	/* cost of @page command */
	int	searchcost;	/* cost of commands that search the whole DB */
	int	waitcost;	/* cost of @wait (refunded when finishes) */
	int	queuemax;	/* max commands a player (nonwiz) may have in queue */
	int	wizqueuemax;	/* Max commands a wizard may have in queue */
	int	queue_chunk;	/* # cmds to run from queue when idle */
	int	active_q_chunk;	/* # cmds to run from queue when active */
	int	machinecost;	/* One in mc+1 cmds costs 1 penny (POW2-1) */
	int	room_quota;	/* quota needed to make a room */
	int	exit_quota;	/* quota needed to make an exit */
	int	thing_quota;	/* quota needed to make a thing */
	int	player_quota;	/* quota needed to make a robot player */
	int	sacfactor;	/* sacrifice earns (obj_cost/sfactor) + sadj */
	int	sacadjust;	/* ... */
	int	clone_copy_cost;/* Does @clone copy value? */
	int	use_hostname;	/* TRUE = use machine NAME rather than quad */
	int	recycle;	/* allow object recycling */
	int	quotas;		/* TRUE = have building quotas */
	int	rooms_can_open; /* TRUE = rooms are able to use @open/open() */
	int	ex_flags;	/* TRUE = show flags on examine */
	int	robot_speak;	/* TRUE = allow robots to speak */
	int	pub_flags;	/* TRUE = flags() works on anything */
	int	quiet_look;	/* TRUE = don't see attribs when looking */
	int	exam_public;	/* Does EXAM show public attrs by default? */
	int	read_rem_desc;	/* Can the DESCs of nonlocal objs be read? */
	int	read_rem_name;	/* Can the NAMEs of nonlocal objs be read? */
	int	sweep_dark;	/* Can you sweep dark places? */
	int	player_listen;	/* Are AxHEAR triggered on players? */
	int	quiet_whisper;	/* Can others tell when you whisper? */
	int	dark_sleepers;	/* Are sleeping players 'dark'? */
	int	see_own_dark;	/* Do you see your own dark stuff? */
	int	idle_wiz_dark;	/* Do idling wizards get set dark? */
	int	pemit_players;	/* Can you @pemit to faraway players? */
	int	pemit_any;	/* Can you @pemit to ANY remote object? */
	int	match_mine;	/* Should you check yourself for $-commands? */
	int	match_mine_pl;	/* Should players check selves for $-cmds? */
	int	match_pl;
	int	switch_df_all;	/* Should @switch match all by default? */
	int	fascist_tport;	/* Src of teleport must be owned/JUMP_OK */
	int	terse_look;	/* Does manual look obey TERSE */
	int	terse_contents;	/* Does TERSE look show exits */
	int	terse_exits;	/* Does TERSE look show obvious exits */
	int	terse_movemsg;	/* Show move msgs (SUCC/LEAVE/etc) if TERSE? */
	int	trace_topdown;	/* Is TRACE output top-down or bottom-up? */
	int	trace_limit;	/* Max lines of trace output if top-down */
	int	safe_unowned;	/* Are objects not owned by you safe? */
	int	parent_control;	/* Obey ControlLock on parent */
	int	space_compress;	/* Convert multiple spaces into one space */
	int	start_room;	/* initial location and home for players */
	int	start_home;	/* initial HOME for players */
	int	default_home;	/* HOME when home is inaccessable */
	int	master_room;	/* Room containing default cmds/exits/etc */
	int	intern_comp;	/* compress internal strings */
	FLAGSET	player_flags;	/* Flags players start with */
	FLAGSET	room_flags;	/* Flags rooms start with */
	FLAGSET	exit_flags;	/* Flags exits start with */
	FLAGSET	thing_flags;	/* Flags things start with */
	FLAGSET	robot_flags;	/* Flags robots start with */
	FLAGSET	player_toggles;	/* Flags players start with */
	FLAGSET	room_toggles;	/* Flags rooms start with */
	FLAGSET	exit_toggles;	/* Flags exits start with */
	FLAGSET	thing_toggles;	/* Flags things start with */
	FLAGSET	robot_toggles;	/* Flags robots start with */
	int	vattr_flags;	/* Attr flags for all user-defined attrs */
	int	abort_on_bug;	/* Dump core after logging a bug  DBG ONLY */
	char	mud_name[32];	/* Name of the mud */
        char	muddb_name[32];	/* Name of the DB (mail and news) files */
	int	rwho_transmit;	/* Should we transmit RWHO data */
	char	rwho_host[64];	/* Remote RWHO host */
	char	rwho_pass[32];	/* Remote RWHO password */
	int	rwho_info_port;	/* Port to which we dump WHO information */
	int	rwho_data_port;	/* Port to which remote RWHO dumps data */
	int	rwho_interval;	/* seconds between RWHO dumps to remote */
	char	one_coin[32];	/* name of one coin (ie. "penny") */
	char	many_coins[32];	/* name of many coins (ie. "pennies") */
	int	timeslice;	/* How often do we bump people's cmd quotas? */
	int	cmd_quota_max;	/* Max commands at one time */
	int	cmd_quota_incr;	/* Bump #cmds allowed by this each timeslice */
	int	control_flags;	/* Global runtime control flags */
	int	log_options;	/* What gets logged */
	int	log_info;	/* Info that goes into log entries */
	Uchar	markdata[8];	/* Masks for marking/unmarking */
	int	func_nest_lim;	/* Max nesting of functions */
	int	func_invk_lim;	/* Max funcs invoked by a command */
        int     wildmatch_lim;    /* Ceiling on the wildmatching recursion */
	int	ntfy_nest_lim;	/* Max nesting of notifys */
	int	lock_nest_lim;	/* Max nesting of lock evals */
	int	parent_nest_lim;/* Max levels of parents */
	int	global_aconn; 	/* Turn on/off global aconnects */
	int	global_adconn;	/* Turn on/off global adisconnects */
        int     room_aconn;     /* Turn on/off room aconnects */
        int     room_adconn;    /* Turn on/off room adisconnects */
	int	mailbox_size;
	int	mailmax_send;
	int	mailmax_fold;
	int	vlimit;
	int	player_dark;
	int	comm_dark;
	int	who_unfindable;
	int	who_default;
	int	money_limit[4];
	int	offline_reg;
	int	online_reg;
	int	areg_lim;
        int     whohost_size;
        int     who_showwiz;
        int     who_showwiztype;
        int     wiz_override;   /* Enable/disable wizzes overriding locks */
        int     wiz_uselock;    /* Enable/disable wizzes overriding uselocks */
        int     idle_wiz_cloak; /* Enable/disable wizzes cloaking when idle */
        int     idle_message;   /* Enable/disable wizzes getting idle messages */
        int     no_startup;     /* Disable/enable triggering startups */
        int     newpass_god;    /* Allow immortals to newpassword #1 */
        int     who_wizlevel;   /* Specifies what level show up with WHO */
        int     allow_whodark;  /* Allows players set DARK from not showing on WHO */
        int     allow_ansinames;/* Allows names of all dbtypes to be ansified */
                                /* 0:none/1:player/2:thing/4:room/8:exit/15:everything */
        int     who_comment;    /* Allows the (Bummer) and other messages in WHO */
        int     safe_wipe;      /* Anything set SAFE or INDESTRUCTABLE can't be @wiped */
        int     secure_jumpok;  /* Sorry, only arch and higher can set jump_ok on non-rooms */
	int	fmt_contents;	/* Check for content formats @conformat */
	int	fmt_exits;	/* Check for exit formats @exitformat and @darkexitformat */
	int	fmt_names;	/* Check for name format @nameformat */
	int	enable_tstamps;	/* Enable modify/created timestamps */
        int     secure_dark;    /* Players can't set dark on themselves but only themselves */
        int     room_parent;    /* Default global that all new rooms are @parented to */
        int     thing_parent;   /* Default global that all new objects are @parented to */
        int     exit_parent;    /* Default global that all new exits are @parented to */
        int     player_parent;  /* Default global that all new players are @parented to */
	int	room_defobj;	/* Default global that room built-in attribs inherit */
	int	thing_defobj;	/* Default global that thing built-in attribs inherit */
	int	exit_defobj;	/* Default global that exit built-in attribs inherit */
	int	player_defobj;	/* Default global that player built-in attribs inherit */
        int     alt_inventories; /* Define alternate inventory types (worn/wielded) */
        int     altover_inv;    /* Define if alt inventories override 'inventory' */
        int     showother_altinv; /* Define if others see 'equipment' when looking at target */
        int     restrict_home;  /* Define level of restriction */
        int     restrict_home2; /* Define level of restriction (2nd word) */
	char    invname[80];    /* Define name of inventory type - default 'backpack' */
	int     sideeffects;	/* Define sideeffects (set-1,create-2,link-4,pemit-8,tel-16) */
	int     restrict_sidefx; /* Restrict setting side-effects to bitlevel (0 default/any) */
        int     cpuintervalchk; /* CPU level to check for overflowing CPU processes */
        int     cputimechk;     /* Time notification of time elapses from start of command */
        int     mail_tolist;	/* have the '@' in mail be the default */
        int     mail_default;   /* Switch 'mail' from mail/quick to mail/status */
        int     login_to_prog;  /* When you log in you're still in a program (if 1) */
        int     noshell_prog;   /* Can not use | to exit programs (minus immortal) */
        int     sidefx_returnval; /* Sideeffects return dbref# when it creates things */
        int     noregist_onwho; /* Axe the 'R' on the WHO for registration enabled */
        int     nospam_connect; /* Disable logging from forbid sites */
        int     lnum_compat;    /* PENN/MUX lwho() compatibility */
        int     nand_compat;    /* Old (BROKEN) pre-p15 Rhost nand compatibility */
        int     hasattrp_compat; /* Boolean: does hasattrp only check parents */
        int     must_unlquota;  /* Forces you to @quota/unlock before you give @quota */
        char    forbid_host[LBUF_SIZE]; /* forbid host names */
        char    suspect_host[LBUF_SIZE]; /* suspect host names */
        char    register_host[LBUF_SIZE]; /* register host names */
        char    noguest_host[LBUF_SIZE]; /* noguest host names */
        char    autoreg_host[LBUF_SIZE]; /* noguest host names */
        char    validate_host[LBUF_SIZE]; /* Invalidate autoregister email masks */
        char    goodmail_host[LBUF_SIZE]; /* Good hosts to allow to autoregister ALWAYS */
        char    log_command_list[1000]; /* List of commands to log */
        char    nobroadcast_host[LBUF_SIZE]; /* Don't broadcast to sites in this host */
        char    tor_localhost[1000];	/* Localhost name for TOR lookup */
        int	tor_paranoid;	/* Paranoid option for TOR enable Checkig */
        int     imm_nomod;	/* Change NOMODIFY to immortal only perm */
        int     paranoid_exit_linking; /* unlinked exits can't be linked unless controlled */
        int     notonerr_return; /* If function returns '#-1' not() returns '0' if disabled */
        int     safer_passwords; /* Taken from TinyMUSH - requires harder passwords */
        int     max_sitecons;	/* Define maximum number of connects from a site */
	int	mail_def_object;	/* Default object to use global mail eval aliases */
	int	mail_autodeltime;	/* Autodelete on mail - time in days */
        char    guest_namelist[1000]; /* A string of guest names that can be used */
        int	nonindxtxt_maxlines; /* Maximum lines a non-indexed .txt file can have */
        int     hackattr_nowiz;	/* _attributes are not wiz only by default */
	int     hackattr_see;   /* _attributes can be viewable by nonwizzes */
        int	penn_playercmds; /* Do $commands on players like PENN */
	int	format_compatibility;	/* Mush/mux compatibility */
	int	brace_compatibility;	/* Mux compatibility */
	int	max_cpu_cycles;  /* Maximum allowed CPU slams allowed in a row */
	int	cpu_secure_lvl;	/* Action to take when max_cpu_cycles reached */
	int	expand_goto;	/* Toggle on/off expanding exit names to use 'goto' */
	int	global_ansimask;	/* Ansi mask on what to allow/deny */
	int	wizmax_vattr_limit;	/* Maximum vattrs set by wizard+ */
	int	max_vattr_limit;	/* Maximum vattrs set by player */
	int	max_dest_limit;	/* Maximum number you can @destroy */
        int     wizmax_dest_limit; 	/* Maximum number wiz can @destroy */
	int	vattr_limit_checkwiz;	/* Is wizard checking enabled? */
	int	hide_nospoof;	/* Hide the nospoof flag - default no */
        int	partial_conn;	/* Trigger @aconnects on partial connect */
        int	partial_deconn;	/* Trigger @adisconnects on partial disconnect */
	int	secure_functions; /* Fix functions to be safer on evaluation - mask */
        int     penn_switches; /* PENN like switch()/switchall() */
	int	heavy_cpu_max; /* Specify maximum for heavy cpu function-usage */
	int	lastsite_paranoia;	/* Enable paranoia level on connections */
	int	pcreate_paranoia;	/* Enable paranoia level on creations */
	int	max_lastsite_cnt;	/* Count of maximum lastsite information */
	int	min_con_attempt;	/* Minimum ammount of time between connections */
        int	lattr_default_oldstyle;	/* lattr's output is snuffed? */
	int	formats_are_local;	/* A_*_FMT's are local to self */
	int	descs_are_local;	/* All did_it() stuff is localized */
	int	max_pcreate_lim;	/* Maximum times a player can 'create' from con screen */
	int	max_pcreate_time;	/* Time (in seconds) that a player can issue create from screen */
	int	global_parent_obj;	/* Global parent generic object */
	int	global_parent_room;	/* Global parent room object */
	int	global_parent_thing;	/* Global parent thing object */
	int	global_parent_player;	/* Global parent player object */
	int	global_parent_exit;	/* Global parent exit object */
	int	global_clone_obj;	/* Global that all things are @cloned from */
	int	global_clone_room;	/* Global that rooms are @cloned from */
	int	global_clone_thing;	/* Global that things are @cloned from */
	int	global_clone_player;	/* Global that players are @cloned from */
	int	global_clone_exit;	/* Global that exits are @cloned from */
	int	zone_parents;		/* Zones inherit attributes to children */
	int	global_error_obj;	/* Object that handles all 'unfound' commands */
	int	signal_object;		/* Signal Handling Object */
	int	signal_object_type;	/* functions or trigger like a startup */
        int	look_moreflags;		/* Show global flags on attributes */
	int	secure_atruselock;	/* Make attribute uselocks work only with @toggle */
	int	hook_obj;		/* Global command hook object */
	int 	hook_cmd;		/* Issue hook command value */
	int	stack_limit;		/* Ceiling for command stack value */
	int	spam_limit;		/* Ceiling on #commands per minute */
        int     mail_verbosity;         /* Allow messages to disconnected players and subject info */
        int     always_blind;           /* Does movement through exits always work by default? */
        char    mail_anonymous[32];     /* Anonymous player mail is sent to */
        int     sidefx_maxcalls;        /* Maximum sideeffects allowed in a command */
        char    oattr_uses_altname[32]; /* o-attribs use this name optionally */
        int     oattr_enable_altname;   /* Enable/disable alternate names through exits */
        int     empower_fulltel;        /* FULLTEL power handles more than 'self' */
        int     bcc_hidden;             /* +bcc hides who mail is sent to on target */
	int     exits_conn_rooms;       /* If defined, rooms with an exit are not considered floating */
	int	global_attrdefault;	/* Global Attribute Default Handler for Objects */
	int	ahear_maxcount;		/* Maximum loop with ahear counts */
	int	ahear_maxtime;		/* Max time in seconds between ahears */
	int	switch_substitutions;	/* Do @switch/switch()/switchall() allow #$ subs? */
	int	ifelse_substitutions;	/* Do @switch/switch()/switchall() allow #$ subs? */
	int	examine_restrictive;	/* Is examine restrictive  */
	int	queue_compatible;	/* Is the QUEUE mush compatible */
        int     max_percentsubs;	/* Maximum %-subs per command */
	int	lcon_checks_dark;	/* Does lcon/xcon check dark? */
	int	mail_hidden;		/* Does mail/anon hide who it is originally sent to */
	int	enforce_unfindable;	/* Enforce unfindable on target */
	int	zones_like_parents;	/* Make zones behave like @parents for self/inventory/contents */
	int	sub_override;		/* override %-substitutions */
	int	log_maximum;		/* Maximum logtotext() allowed */
	int	power_objects;		/* Objects can have powers */
	int	shs_reverse;		/* SHS reversed for sha function */
	int	break_compatibility;	/* @break/@assert double-eval compatibility */
	char	sub_include[200];	/* Include specified characters for %-subs */
	int	log_network_errors;	/* Turn on or off network error logging */
	int	old_elist;		/* old elist processing */
	int	cluster_cap;		/* Cluster cap for processing */
	int	clusterfunc_cap;	/* Cluster cap for processing (function) */
	int	mux_child_compat;	/* Is it MUX/TM3 compatable for children() */
	int	mux_lcon_compat;	/* Is it MUX/TM3 compatable for children() */
	int	ansi_default;		/* Allow functions to be ansi-default aware that can do so */
	int	accent_extend;		/* Expand accents from 251-255 */
	int	switch_search;		/* Switch search() and searchng() */
	int	signal_crontab;		/* Signal the crontab via USR1 */
        int 	max_name_protect;	/* Maximum name protects allowed */
	int	map_delim_space;	/* map() delimitats space if specified null */
	char	cap_conjunctions[LBUF_SIZE];	/* caplist exceptions */
	char	cap_articles[LBUF_SIZE];	/* caplist exceptions */
	char	cap_preposition[LBUF_SIZE];	/* caplist exceptions */
        char    atrperms[LBUF_SIZE];
        int	atrperms_max;
        int	safer_ufun;
	int	includenest;	/* Max number of nesting of @include */
	int	includecnt;	/* Total number of @includes in the command caller */
	int	lfunction_max;	/* Maximum lfunctions allowed */
        int	blind_snuffs_cons;	/* Does the BLIND flag snuff aconnect/adisconnect */
	int	listen_parents;	/* ^listens handle parents */
	int     icmd_obj;        /* The object for the icmd evaluation */
	int	ansi_txtfiles;	/* Do allthe various connect files parse %-ansi subs */
	int	list_max_chars;	/* Maximum characters allowed to be shoved in a list */
	int	float_precision;	/* Float percision for math functions() -- default 6 */
	int	admin_object;	/* The admin object */
	int	enhanced_convtime;	/* Enhanced convtime format */
	int	mysql_delay;	/* MySql Retry Delay Value (in seconds) */
	char	tree_character[2];	/* The Tree Character */
	int	proxy_checker;	/* Proxy Checker -- Not very reliable */
	int	idle_stamp;	/* Idle stamp to use for comparing 10 past commands */
	int	idle_stamp_max;	/* Idle stamp count max to use for comparing X past commands */
	dbref	file_object;	/* The file object to override @list_file foo */
#ifdef REALITY_LEVELS
        int reality_compare;	/* How descs are displayed in reality */
        int no_levels;          /* # of reality levels */
        struct rlevel_def {
            char name[17];	/* name of level */
            RLEVEL value;	/* bitmask for level */
            char attr[33];	/* RLevel desc attribute */
        } reality_level[32];	/* Reality levels */
        int wiz_always_real;	/* Wizards are always real */
	int reality_locks;	/* Allow user-lock to be reality lock */
        int reality_locktype;	/* What type of lock will the user-lock be? */
        RLEVEL  def_room_rx;	/* Default room RX level */
        RLEVEL  def_room_tx;	/* Default room TX level */
        RLEVEL  def_player_rx;	/* Default player RX level */
        RLEVEL  def_player_tx;	/* Default player RX level */
        RLEVEL  def_exit_rx;	/* Default exit RX level */
        RLEVEL  def_exit_tx;	/* Default exit TX level */
        RLEVEL  def_thing_rx;	/* Default thing RX level */
        RLEVEL  def_thing_tx;	/* Default thing TX level */
#endif /* REALITY_LEVELS */
#ifdef SQLITE
        int     sqlite_query_limit;
        char    sqlite_db_path[128];
#endif /* SQLITE */
#ifdef MYSQL_VERSION
        char	mysql_host[128];
        char	mysql_user[128];
        char	mysql_pass[128];
        char	mysql_base[128];
        char	mysql_socket[128];
	int	mysql_port;
#endif
	int	name_with_desc;	/* Toggle to enable names with descs when looking (if not-examinable) */
#else
	int	paylimit;	/* getting money gets hard over this much */
	int	digcost;	/* cost of @dig command */
	int	opencost;	/* cost of @open command */
	int	robotcost;	/* cost of @robot command */
	int	createmin;	/* default (and minimum) cost of @create cmd */
	int	createmax;	/* max cost of @create command */
	int	sacfactor;	/* sacrifice earns (obj_cost/sfactor) + sadj */
	int	sacadjust;	/* ... */
	int	room_quota;	/* quota needed to make a room */
	int	exit_quota;	/* quota needed to make an exit */
	int	thing_quota;	/* quota needed to make a thing */
	int	player_quota;	/* quota needed to make a robot player */
	int	quotas;		/* TRUE = have building quotas */
	int	start_room;	/* initial location and home for players */
	int	start_home;	/* initial HOME for players */
	int	default_home;	/* HOME when home is inaccessable */
	int	vattr_flags;	/* Attr flags for all user-defined attrs */
	int	log_options;	/* What gets logged */
	int	log_info;	/* Info that goes into log entries */
	Uchar	markdata[8];	/* Masks for marking/unmarking */
	int	ntfy_nest_lim;	/* Max nesting of notifys */
	int	vlimit;
        int     safer_passwords; /* Taken from TinyMUSH - requires harder passwords */
	int	vattr_limit_checkwiz;	/* Is wizard checking enabled? */
	int	switch_substitutions;	/* Do @switch/switch()/switchall() allow #$ subs? */
	int	ifelse_substitutions;	/* Do @switch/switch()/switchall() allow #$ subs? */
	int	enforce_unfindable;	/* Enforce unfindable on target */
	int	power_objects;		/* Objects can have powers */
	int	lfunction_max;	/* Maximum lfunctions allowed */
        int	blind_snuffs_cons;	/* Does the BLIND flag snuff aconnect/adisconnect */
	char	sub_include[200];
	int	old_elist;		/* Old elist processing */
#endif	/* STANDALONE */
};

extern CONFDATA mudconf;

typedef struct site_data SITE;
struct site_data {
	struct site_data *next;		/* Next site in chain */
	struct in_addr address;		/* Host or network address */
	struct in_addr mask;		/* Mask to apply before comparing */
	int	flag;			/* Value to return on match */
	int	key;			/* Auto sited or not? */
	int	maxcon;			/* Maximum connections allowed from site */
};


typedef struct markbuf MARKBUF;
struct markbuf {
	char	chunk[5000];
};

typedef struct alist ALIST;
struct alist {
	char	*data;
	int	len;
	struct alist *next;
};

typedef struct protectname_struc PROTECTNAME;
struct protectname_struc {
	char	*name;
	dbref	i_name;
	int	i_key;
	struct protectname_struc	*next;
};
typedef struct badname_struc BADNAME;
struct badname_struc {
	char	*name;
	struct badname_struc	*next;
};

typedef struct forward_list FWDLIST;
struct forward_list {
	int	count;
	int	data[1000];
};

typedef struct blacklist_list BLACKLIST;
struct blacklist_list {
	char	s_site[20];
        struct	in_addr	site_addr;
	struct	in_addr mask_addr;
	struct	blacklist_list	*next;
};

/* For a future mod to split up HIGH cpu based on function 
 * This will be smarter as we'll compare it to last time
 * executed as well, or at least some wiggy logic. */
struct struct_cpu_recurse {
	int	chk_nslookup;
	int	chk_textfile;
	int	chk_dynhelp;
	int	chk_entrances;
	int	chk_lrooms;
	int	chk_parenmatch;
	int	chk_search;
	int	chk_stats;
};

typedef struct statedata STATEDATA;
struct statedata {
#ifndef STANDALONE
        /* command profiling */
        int     objevalst;
	int	breakst;
	int	breakdolist;
  dbref remote; /* Remote location for @remote */
  dbref remotep;/* Remote location for @remote player*/
	int	dolistnest;
        int     shell_program;  /* Shelled out of @program */
        dbref   store_lastcr;   /* Store the last created dbref# for functions */
	dbref	store_lastx1;	/* Store the last created exit# for dig */
	dbref	store_lastx2;	/* Store the previous created exit# for dig */
	dbref	store_loc;	/* Store target location for open/dig */
        int     evalcount;
        int     funccount;
        int     attribfetchcount;
	int	func_ignore;
        int     func_bypass;
	int	initializing;	/* are we reading config file at startup? */
	int	panicking;	/* are we in the middle of dying horribly? */
	int	logging;	/* Are we in the middle of logging? */
	int	epoch;		/* Generation number for dumps */
	int	generation;	/* DB global generation number */
	dbref	curr_enactor;	/* Who initiated the current command */
	dbref	curr_player;	/* Who is running the current command */
        char    *curr_cmd;      /* The current command */
        char    *iter_arr[50];   /* Iter recursive memory - text*/
        int     iter_inumarr[50];/* Iter recursive memory - number*/
        int     iter_inumbrk[50];/* Iter recursive memory - break*/
        int     iter_inum;      /* Iter inum value */
	int	dol_inumarr[50];/* Dolist array */
	char	*dol_arr[50];	/* Dolist Array */
	int	alarm_triggered;/* Has periodic alarm signal occurred? */
	time_t	now;		/* What time is it now? */
	double  nowmsec; /* What time is it now, with msecs */
	time_t	lastnow;	/* What time was it last? */
	double  lastnowmsec; /* What time was it last, with msecs */
	double	dump_counter;	/* Countdown to next db dump */
	double	check_counter;	/* Countdown to next db check */
	double	idle_counter;	/* Countdown to next idle check */
	double	rwho_counter;	/* Countdown to next RWHO dump */
	double	mstats_counter;	/* Countdown to next mstats snapshot */
	time_t  chkcpu_stopper; /* What time was it when command started */
	int     chkcpu_toggle;  /* Toggles the chkcpu to notify if aborted */
	int	chkcpu_locktog;	/* Toggles the chkcpu to notify if aborted via locks */
	int	ahear_count;	/* Current ahear nest count */
	dbref	ahear_lastplr;	/* Last Player to try the ahear thingy */
	int	ahear_currtime;	/* Time the ahear was issued */
        int     force_halt;     /* Can you force the halted person? */
	int	rwho_on;	/* Have we connected to an RWHO server? */
	int	shutdown_flag;	/* Should interface be shut down? */
	int	reboot_flag;
	char	version[128];	/* MUSH version string */
	char	short_ver[64];	/* Short version number (for RWHO) */
	time_t	start_time;	/* When was MUSH started */
	time_t	reboot_time;
        time_t  mushflat_time;  /* When was MUSH last @dump/flatted */
        time_t  aregflat_time;  /* When was @reg last flatfiled */
        time_t  newsflat_time;  /* When was news last flatfiled */
        time_t  mailflat_time;  /* When was mail last flatfiled */
	char	buffer[256];	/* A buffer for holding temp stuff */
        char    *lbuf_buffer;	/* An lbuf buffer we can globally use */
	char	*debug_cmd;	/* The command we are executing (if any) */
	char	doing_hdr[81];	/* Doing column header in the WHO display */
	char	ng_doing_hdr[81];
	char	guild_hdr[12];
        char    last_command[LBUF_SIZE]; /* Easy buffer of last command */
	SITE	*access_list;	/* Access states for sites */
	SITE	*suspect_list;	/* Sites that are suspect */
	SITE	*special_list;	/* Sites that have special requirements */
        HASHTAB cmd_alias_htab; /* Command alias hashtable */
	HASHTAB	command_htab;	/* Commands hashtable */
	HASHTAB	logout_cmd_htab;/* Logged-out commands hashtable (WHO, etc) */
	HASHTAB func_htab;	/* Functions hashtable */
	HASHTAB ufunc_htab;	/* Local functions hashtable */
	HASHTAB ulfunc_htab;	/* User-Defiend Local functions hashtable */
	HASHTAB flags_htab;	/* Flags hashtable */
	HASHTAB toggles_htab;	/* Toggles hashtable */
	HASHTAB powers_htab;
	HASHTAB depowers_htab;
	HASHTAB	attr_name_htab;	/* Attribute names hashtable */
	NHSHTAB	attr_num_htab;	/* Attribute numbers hashtable */
	HASHTAB player_htab;	/* Player name->number hashtable */
	NHSHTAB	desc_htab;	/* Socket descriptor hashtable */
	NHSHTAB	fwdlist_htab;	/* Room forwardlists */
	NHSHTAB	parent_htab;	/* Parent $-command exclusion */
	HASHTAB	news_htab;	/* News topics hashtable */
	HASHTAB	help_htab;	/* Help topics hashtable */
	HASHTAB	wizhelp_htab;	/* Wizard help topics hashtable */
	HASHTAB error_htab;
        HASHTAB ansi_htab;	/* 256 colortab */
#ifdef PLUSHELP
	HASHTAB	plushelp_htab;	/* PlusHelp topics hashtable */
#endif
	int	errornum;
	int	attr_next;	/* Next attr to alloc when freelist is empty */
	BQUE	*qfirst;	/* Head of player queue */
	BQUE	*qlast;		/* Tail of player queue */
	BQUE	*qlfirst;	/* Head of object queue */
	BQUE	*qllast;	/* Tail of object queue */
	BQUE	*qwait;		/* Head of wait queue */
	BQUE	*qsemfirst;	/* Head of semaphore queue */
	BQUE	*qsemlast;	/* Tail of semaphore queue */
	BQUE	*fqwait;	/* Head of freeze wait queue */
	BQUE	*fqsemfirst;	/* Head of freeze semaphore queue */
	BQUE	*fqsemlast;	/* Tail of freeze semaphore queue */
	BADNAME	*badname_head;	/* List of disallowed names */
	PROTECTNAME	*protectname_head;	/* List of protected names */
	int	mstat_ixrss[2];	/* Summed shared size */
	int	mstat_idrss[2];	/* Summed private data size */
	int	mstat_isrss[2];	/* Summed private stack size */
	int	mstat_secs[2];	/* Time of samples */
	int	mstat_curr;	/* Which sample is latest */
	ALIST	iter_alist;	/* Attribute list for iterations */
	char	*mod_alist;	/* Attribute list for modifying */
	int	mod_size;	/* Length of modified buffer */
	dbref	mod_al_id;	/* Where did mod_alist come from? */
	dbref	freelist;	/* Head of object freelist */
	int	min_size;	/* Minimum db size (from file header) */
	int	db_top;		/* Number of items in the db */
	int	db_size;	/* Allocated size of db structure */
	MARKBUF	*markbits;	/* temp storage for marking/unmarking */
        int	trace_nest_lev;	/* The trace nest level */
	int	func_nest_lev;	/* Current nesting of functions */
	int	func_invk_ctr;	/* Functions invoked so far by this command */
	int	ntfy_nest_lev;	/* Current nesting of notifys */
	int	lock_nest_lev;	/* Current nesting of lock evals */
        int     ufunc_nest_lev; /* Current nesting of USER functions */
	char	*global_regs[MAX_GLOBAL_REGS];	/* Global registers */
	char	*global_regsname[MAX_GLOBAL_REGS];	/* Global register names */
        char    *global_regs_backup[MAX_GLOBAL_REGS];
        char    nameofqreg[37]; /* Buffer to hold qregs */
        int	global_regs_wipe;	/* Toggle to wipe localized regs */
	int	mail_state;
	int	whisper_state;
	int	eval_rec;
	int	guest_num;
	int	guest_status;
	dbref	free_num;
	dbref	recoverlist;
	char	nuke_status;
	char	write_status;
	int	nowall_over;
	int	evalnum;
	int	evalresult;
	int	evalstate[MAXEVLEVEL];
	int	evaldb[MAXEVLEVEL];
	dbref	vlplay;
	int	exitcheck;
	dbref	pageref;
	int	droveride;
	int	scheck;
	int	autoreg;
	int	curr_cpu_cycle;/* Current CPU slam count per dbref# */
	int	curr_cpu_user;  /* Current CPU slam count per dbref# */
        int     last_cpu_user;  /* Last user to CPU-slam and be punished with ceiling */
        int     new_vattr;	/* New Vattr Defined */
	int	max_logins_allowed; /* Total logins allowed based on free OS descriptors */
	int	last_cmd_timestamp;
	int	heavy_cpu_recurse;	/* functions with heavy CPU usage */
        time_t	heavy_cpu_tmark1;	/* Time maker */
        time_t	heavy_cpu_tmark2;	/* Time maker */
        int	heavy_cpu_lockdown;	/* Lock down a function if heavilly abused */
	int	cmp_lastsite;    	/* Last site that connected */
	int	cmp_lastsite_cnt; 	/* Number of times last site connected */
	int	last_con_attempt;
        int     last_pcreate_cnt;
        int     last_pcreate_time;
	int	reverse_wild;	/* @wipe and such have wildmatches REVERSED */
	int	stack_val;	/* Current Stack Value of command */
	int	stack_toggle;	/* Toggle to show if stack was hit or not */
	int	stack_cntr;	/* Counter of how many consecutive stack overflows there were */
        int     train_cntr;     /* Counter for train nest limit */
        int     sudo_cntr;      /* Counter for sudo nest limit */
        int     sidefx_currcalls; /* Current sidefx calls */
        int     sidefx_toggle;  /* Toggle to show sidefx ceiling was hit */
	int     emit_substitute;  /* Toggle @emit substitutions */
        int     inside_locks;	/* Enable/disable 'inside locks' */
	int	nolookie;	/* Don't look into the vattr table at this point */
	int	reality_notify;	/* Reality level check inside notify */
	int	password_nochk;	/* Don't allow @hooks to pass/execute %x/%c to find password */
	int	curr_percentsubs;	/* Current percent sub tree */
	int	tog_percentsubs;	/* Ok, you hit the max percent sub ceiling.  Bad boy */
	int	cntr_percentsubs;	/* Counter to kill the little pecker */
        double  cntr_reset;	/* Reset the basic counters after 60 seconds */
	int	recurse_rlevel;	/* Allow recurse limit for reality levels */
	int	sub_overridestate; /* state information for sub_overrides */
	int	sub_includestate; /* state information for sub_overrides */
	int	log_maximum;	/* State to count logtotext() calls per command */
	int	trainmode;	/* ] enabled */
        int     outputflushed;  /* Is output flushed on command? */
	int     allowbypass;	/* Is bypass() allowed?  Ergo, in @function? */
	int	shifted;	/* Is 0-9 registers shifted? */
	int	clust_time;	/* Time for cluster */
	dbref	last_network_owner;	/* The last network owner who had network issues */	
	FILE	*f_logfile_name;
        int	log_chk_reboot;
	int	blacklist_cnt;
	int	wipe_state;	/* do_wipe state counter */
	int	includecnt;	/* @include count */
	int	includenest;	/* @include nest count */
	int	nocodeoverride;	/* Override NO_CODE flag for objeval() */
	int	notrace;	/* Do not trace */
	int	start_of_cmds;	/* Start of command -- hack around zenty ansi */
        int	twinknum;	/* Dbref of twink object if inside twinklock */
	int	dumpstatechk;	/* Dump state check */
	int	forceusr2;	/* Dump state check */
        BLACKLIST *bl_list; 	/* The black list */
	char	tor_localcache[1000]; /* Cache for the tor local host */
	int 	insideaflags; 	/* Inside @aflag eval check */
	int	insideicmds;	/* Inside ICMD evaluation */
	time_t	mysql_last;	/* Last mysql hang time */
	int	argtwo_fix;	/* Arg 2 fix test for '=' */
#else
  dbref remote; /* Remote location for @remote */
  dbref remotep;/* Remote location for @remote player */
	int	logging;	/* Are we in the middle of logging? */
	char	buffer[256];	/* A buffer for holding temp stuff */
        char    *lbuf_buffer;	/* An lbuf buffer we can globally use */
	int	attr_next;	/* Next attr to alloc when freelist is empty */
	ALIST	iter_alist;	/* Attribute list for iterations */
	char	*mod_alist;	/* Attribute list for modifying */
	int	mod_size;	/* Length of modified buffer */
	dbref	mod_al_id;	/* Where did mod_alist come from? */
	int	min_size;	/* Minimum db size (from file header) */
	int	db_top;		/* Number of items in the db */
	int	db_size;	/* Allocated size of db structure */
        int     new_vattr;	/* New Vattr Defined */
	int	nolookie;	/* Don't look into the vattr table at this point */
	dbref	freelist;	/* Head of object freelist */
	dbref	recoverlist;
	dbref	vlplay;
	FILE	*f_logfile_name;
        int	log_chk_reboot;
	MARKBUF	*markbits;	/* temp storage for marking/unmarking */
#endif	/* STANDALONE */
};

extern STATEDATA mudstate;

/* Configuration parameter handler definition */

#define CF_HAND(proc)	int proc (int *vp, char *str, long extra, long extra2, \
 			          dbref player, char *cmd)
#define CF_HDCL(proc)	int FDECL(proc, (int *, char *, long, long, dbref, char *))

/* Global flags */

/* Game control flags in mudconf.control_flags */

#define	CF_LOGIN	0x0001		/* Allow nonwiz logins to the mush */
#define CF_BUILD	0x0002		/* Allow building commands */
#define CF_INTERP	0x0004		/* Allow object triggering */
#define CF_CHECKPOINT	0x0008		/* Perform auto-checkpointing */
#define	CF_DBCHECK	0x0010		/* Periodically check/clean the DB */
#define CF_IDLECHECK	0x0020		/* Periodically check for idle users */
#define CF_RWHO_XMIT	0x0040		/* Update remote RWHO data */
#define CF_ALLOW_RWHO	0x0080		/* Allow the RWHO command */
#define CF_DEQUEUE	0x0100		/* Remove entries from the queue */

/* Host information codes */

#define H_REGISTRATION	0x0001	/* Registration ALWAYS on */
#define H_FORBIDDEN	0x0002	/* Reject all connects */
#define H_SUSPECT	0x0004	/* Notify wizards of connects/disconnects */
#define H_NOGUEST	0x0008
#define H_NOAUTOREG	0x0010
#define H_NOAUTH	0x0020  /* Don't use AUTH protocol - Thorin 5/00 */
#define H_NODNS		0x0040  /* Don't do reverse DNS lookups - Thorin 5/00 */
#define H_AUTOSITE	0x0080  /* Site was automatically updated */

/* Logging options */

#define LOG_ALLCOMMANDS	0x00000001	/* Log all commands */
#define LOG_ACCOUNTING	0x00000002	/* Write accounting info on logout */
#define LOG_BADCOMMANDS	0x00000004	/* Log bad commands */
#define LOG_BUGS	0x00000008	/* Log program bugs found */
#define LOG_DBSAVES	0x00000010	/* Log database dumps */
#define LOG_CONFIGMODS	0x00000020	/* Log changes to configuration */
#define LOG_PCREATES	0x00000040	/* Log character creations */
#define LOG_KILLS	0x00000080	/* Log KILLs */
#define LOG_LOGIN	0x00000100	/* Log logins and logouts */
#define LOG_NET		0x00000200	/* Log net connects and disconnects */
#define LOG_SECURITY	0x00000400	/* Log security-related events */
#define LOG_SHOUTS	0x00000800	/* Log shouts */
#define LOG_STARTUP	0x00001000	/* Log nonfatal errors in startup */
#define LOG_WIZARD	0x00002000	/* Log dangerous things */
#define LOG_ALLOCATE	0x00004000	/* Log alloc/free from buffer pools */
#define LOG_PROBLEMS	0x00008000	/* Log runtime problems */
#define LOG_MAIL	0x00010000      /* Log mail problems/wizard stuff */
#define LOG_SUSPECT     0x00020000      /* Log SUSPECT players */
#define LOG_WANDERER    0x00040000      /* Log WANDERER players */
#define LOG_GUEST       0x00080000      /* Log GUEST players */
#define LOG_CONNECT     0x00100000      /* Log everything on the connect screen */
#define LOG_GHOD        0x00200000
#define LOG_DOOR        0x00400000      /* Log all internal door activity */
#define LOG_ALWAYS	0x80000000	/* Always log it */

#define LOGOPT_FLAGS		0x01	/* Report flags on object */
#define LOGOPT_LOC		0x02	/* Report loc of obj when requested */
#define LOGOPT_OWNER		0x04	/* Report owner of obj if not obj */
#define LOGOPT_TIMESTAMP	0x08	/* Timestamp log entries */

#endif
