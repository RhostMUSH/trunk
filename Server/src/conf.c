/* conf.c:      set up configuration information and static data */

#include <stdio.h>
#include <math.h>


#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "db.h"
#include "externs.h"
#include "interface.h"
#include "command.h"
#include "htab.h"
#include "alloc.h"
#include "flags.h"
#include "udb_defs.h"
#include "debug.h"

/* ---------------------------------------------------------------------------
 * CONFPARM: Data used to find fields in CONFDATA.
 */

typedef struct confparm CONF;
struct confparm {
    char *pname;		/* parm name */
    int (*interpreter) ();	/* routine to interp parameter */
    int flags;			/* control flags */
    int *loc;			/* where to store value */
    long extra;			/* extra data for interpreter */
    long extra2;		/* extra data for interpreter (2nd word) */
    int flags2;			/* Viewable flags */
    char *extrach;		/* Short description of param */
};

/* ---------------------------------------------------------------------------
 * External symbols.
 */

CONFDATA mudconf;
STATEDATA mudstate;

#ifndef STANDALONE
extern int FDECL(pstricmp, (char *, char *, int));
extern NAMETAB logdata_nametab[];
extern NAMETAB logoptions_nametab[];
extern NAMETAB access_nametab[];
extern NAMETAB access_nametab2[];
extern NAMETAB attraccess_nametab[];
extern NAMETAB list_names[];
extern NAMETAB sigactions_nametab[];
extern CONF conftable[];
extern int	FDECL(flagstuff_internal, (char *, char *));
/* Lensy: Not external, but a forward declaration is needed */
CF_HAND(cf_dynstring);
#endif

extern double FDECL(time_ng, (double*));
extern int FDECL(do_flag_and_toggle_def_conf, (dbref, char *, char *, int *, int));
extern double FDECL(safe_atof, (char *));


/* ---------------------------------------------------------------------------
 * cf_init: Initialize mudconf to default values.
 */

void
NDECL(cf_init)
{
#ifndef STANDALONE
    int i;
    strcpy(mudconf.roomlog_path, "roomlogs");
    strcpy(mudconf.data_dir, "./data");
    strcpy(mudconf.txt_dir, "./txt");
    strcpy(mudconf.image_dir, "./image");
    strcpy(mudconf.indb, "netmush.db");
    strcpy(mudconf.outdb, "");
    strcpy(mudconf.crashdb, "");
    strcpy(mudconf.gdbm, "");
    if ( getenv("GAMENAME") != NULL )
       sprintf(mudconf.logdb_name, "%.110s.gamelog", getenv("GAMENAME"));
    else
       strcpy(mudconf.logdb_name, "netrhost.gamelog");
    mudconf.compress_db = 0;
    strcpy(mudconf.compress, "/usr/ucb/compress");
    strcpy(mudconf.uncompress, "/usr/ucb/zcat -c");
    strcpy(mudconf.status_file, "shutdown.status");
	strcpy(mudconf.ip_address, "0.0.0.0");
    mudconf.port = 6250;
    mudconf.html_port = 6251;
    mudconf.api_port = -1;
    mudconf.api_nodns = 0;
    mudconf.debug_id = 44660;
    mudconf.authenticate = 1;
    mudconf.init_size = 1000;
    mudconf.maildelete = 1;
    mudconf.player_dark = 1;
    mudconf.comm_dark = 0;
    mudconf.who_unfindable = 1;
    mudconf.who_default = 0;
    mudconf.start_build = 0;
    strcpy(mudconf.areg_file, "autoreg.txt");
    strcpy(mudconf.aregh_file, "areghost.txt");
    strcpy(mudconf.guest_file, "guest.txt");
    strcpy(mudconf.conn_file, "connect.txt");
    strcpy(mudconf.creg_file, "register.txt");
    strcpy(mudconf.regf_file, "create_reg.txt");
    strcpy(mudconf.motd_file, "motd.txt");
    strcpy(mudconf.wizmotd_file, "wizmotd.txt");
    strcpy(mudconf.quit_file, "quit.txt");
    strcpy(mudconf.down_file, "down.txt");
    strcpy(mudconf.full_file, "full.txt");
    strcpy(mudconf.site_file, "badsite.txt");
    strcpy(mudconf.crea_file, "newuser.txt");
    strcpy(mudconf.help_file, "help.txt");
#ifdef PLUSHELP
    strcpy(mudconf.plushelp_file, "plushelp.txt");
    strcpy(mudconf.plushelp_indx, "plushelp.indx");
#endif
    strcpy(mudconf.help_indx, "help.indx");
    strcpy(mudconf.news_file, "news.txt");
    strcpy(mudconf.news_indx, "news.indx");
    strcpy(mudconf.whelp_file, "wizhelp.txt");
    strcpy(mudconf.whelp_indx, "wizhelp.indx");
    strcpy(mudconf.error_file, "error.txt");
    strcpy(mudconf.error_indx, "error.indx");
    strcpy(mudconf.gconf_file, "noguest.txt");
    strcpy(mudconf.door_file, "doorconf.txt");
    strcpy(mudconf.door_indx, "doorconf.indx");
    strcpy(mudconf.manlog_file, "manual_log");
    strcpy(mudconf.mailinclude_file, "autoreg_include.txt");
    strcpy(mudconf.motd_msg, "");
    strcpy(mudconf.wizmotd_msg, "");
    strcpy(mudconf.downmotd_msg, "");
    strcpy(mudconf.fullmotd_msg, "");
    strcpy(mudconf.dump_msg, "");
    strcpy(mudconf.postdump_msg, "");
    strcpy(mudconf.spam_msg, "Spam: Spam security initiated. Command aborted.");
    strcpy(mudconf.spam_objmsg, "Spam: Spam security initiated. Your object's command aborted.");
    strcpy(mudconf.mailprog, "mail");
    strcpy(mudconf.guild_default, "Wanderer");
    strcpy(mudconf.guild_attrname, "guild");
    memset(mudconf.forbid_host, 0, sizeof(mudconf.forbid_host));
    memset(mudconf.suspect_host, 0, sizeof(mudconf.suspect_host));
    memset(mudconf.register_host, 0, sizeof(mudconf.register_host));
    memset(mudconf.noguest_host, 0, sizeof(mudconf.noguest_host));
    memset(mudconf.autoreg_host, 0, sizeof(mudconf.autoreg_host));
    memset(mudconf.validate_host, 0, sizeof(mudconf.validate_host));
    memset(mudconf.goodmail_host, 0, sizeof(mudconf.goodmail_host));
    memset(mudconf.nobroadcast_host, 0, sizeof(mudconf.nobroadcast_host));
    memset(mudconf.guest_namelist, 0, sizeof(mudconf.guest_namelist));
    memset(mudconf.log_command_list, 0, sizeof(mudconf.log_command_list));
    mudconf.guest_randomize = 0; /* Randomize guests */
    mudconf.mailsub = 1;
    mudconf.mailbox_size = 99;
    mudconf.mailmax_send = 100;
    mudconf.mailmax_fold = 20;
    mudconf.max_num_guests = 0;
    mudconf.name_spaces = 1;
    mudconf.fork_dump = 0;
    mudconf.fork_vfork = 0;
    mudconf.sig_action = SA_DFLT;
    mudconf.paranoid_alloc = 0;
    mudconf.max_players = -1;
    mudconf.max_size = -1;
    mudconf.whereis_notify = 1;
    mudconf.dump_interval = 3600;
    mudconf.check_interval = 600;
    mudconf.dump_offset = 0;
    mudconf.check_offset = 300;
    mudconf.idle_timeout = 3600;
    mudconf.conn_timeout = 120;
    mudconf.idle_interval = 60;
    mudconf.retry_limit = 3;
    mudconf.regtry_limit = 1;
#ifdef QDBM
  #ifdef LBUF64
    mudconf.output_limit = 262144;
  #else
    #ifdef LBUF32
    mudconf.output_limit = 131072;
    #else
      #ifdef LBUF16
    mudconf.output_limit = 65536;
      #else
        #ifdef LBUF8
    mudconf.output_limit = 32768;
        #else
    mudconf.output_limit = 16384;
        #endif
      #endif
    #endif
  #endif
#else
    mudconf.output_limit = 16384;
#endif
    mudconf.paycheck = 0;
    mudconf.paystart = 0;
    mudconf.paylimit = 10000;
    mudconf.start_quota = 10;
    mudconf.payfind = 0;
    mudconf.digcost = 10;
    mudconf.linkcost = 1;
    mudconf.opencost = 1;
    mudconf.wall_cost = 250;
    mudconf.comcost = 5;
    mudconf.createmin = 10;
    mudconf.createmax = 505;
    mudconf.killmin = 10;
    mudconf.killmax = 100;
    mudconf.killguarantee = 100;
    mudconf.robotcost = 1000;
    mudconf.pagecost = 10;
    mudconf.searchcost = 100;
    mudconf.waitcost = 10;
    mudconf.machinecost = 64;
    mudconf.exit_quota = 1;
    mudconf.player_quota = 1;
    mudconf.room_quota = 1;
    mudconf.thing_quota = 1;
    mudconf.queuemax = 100;
    mudconf.wizqueuemax = 1000;
    mudconf.queue_chunk = 3;
    mudconf.active_q_chunk = 0;
    mudconf.sacfactor = 5;
    mudconf.sacadjust = -1;
    mudconf.use_hostname = 1;
    mudconf.recycle = 1;
    mudconf.quotas = 0;
    mudconf.rooms_can_open = 0;
    mudconf.ex_flags = 1;
    mudconf.examine_restrictive = 0;
    mudconf.robot_speak = 1;
    mudconf.clone_copy_cost = 0;
    mudconf.pub_flags = 1;
    mudconf.quiet_look = 1;
    mudconf.exam_public = 1;
    mudconf.read_rem_desc = 0;
    mudconf.read_rem_name = 0;
    mudconf.sweep_dark = 0;
    mudconf.player_listen = 0;
    mudconf.quiet_whisper = 1;
    mudconf.dark_sleepers = 1;
    mudconf.see_own_dark = 1;
    mudconf.idle_wiz_dark = 0;
    mudconf.pemit_players = 0;
    mudconf.pemit_any = 0;
    mudconf.match_mine = 0;
    mudconf.match_mine_pl = 0;
    mudconf.match_pl = 0;
    mudconf.switch_df_all = 1;
    mudconf.fascist_tport = 0;
    mudconf.terse_look = 1;
    mudconf.terse_contents = 1;
    mudconf.terse_exits = 1;
    mudconf.terse_movemsg = 1;
    mudconf.trace_topdown = 1;
    mudconf.trace_limit = 200;
    mudconf.safe_unowned = 0;
    mudconf.parent_control = 0;
    /* -- ??? Running SC on a non-SC DB may cause problems */
    mudconf.space_compress = 1;
    mudconf.intern_comp = 0;
    mudconf.start_room = 0;
    mudconf.start_home = -1;
    mudconf.default_home = -1;
    mudconf.master_room = -1;
    mudconf.switch_substitutions = 0;
    mudconf.ifelse_substitutions = 0;
    mudconf.sideeffects = 32;  /* Enable only list() by default */
    mudconf.sidefx_returnval = 0; /* sideeffects that create return dbref# if enabled */
    mudconf.safer_passwords = 0; /* If enabled, requires tougher to guess passwords */
    mudconf.sidefx_maxcalls = 1000; /* Maximum sideeffects allowed in a command */
    mudconf.max_percentsubs = 1000000; /* Maximum percent substitutions allowed in a command */
    mudconf.oattr_enable_altname = 0; /* Enable/Disable o-attr altnames */
    memset(mudconf.oattr_uses_altname, 0, sizeof(mudconf.oattr_uses_altname));
    strcpy(mudconf.oattr_uses_altname, "_NPC");
    mudconf.ahear_maxcount = 30;
    mudconf.ahear_maxtime = 0;
    mudconf.queue_compatible = 0;
    mudconf.lcon_checks_dark = 0; /* lcon/xcon check dark/unfindable */
    mudconf.mail_hidden = 0;	/* mail/anon hidden who is sent to */
    mudconf.enforce_unfindable = 0; /* Enforce unfindable for locate crap */
    mudconf.zones_like_parents = 0; /* Make zones search like @parents */
    mudconf.sub_override = 0;		/* %-sub override */
    mudconf.round_kludge = 0; /* Kludge workaround to fix rounding 2.5 to 2. [Loki] */
    mudconf.power_objects = 0;		/* non-players can have @powers? */
    mudconf.shs_reverse = 0;
    mudconf.break_compatibility = 0;	/* @break/@assert compatibility */
    mudconf.log_network_errors = 1;	/* Log Network Errors */
    mudconf.old_elist = 0;		/* Use old elist processing */
    mudconf.mux_child_compat = 0;	/* MUX children() compatability */
    mudconf.mux_lcon_compat = 0;	/* MUX lcon() compatability */
    mudconf.switch_search = 0;		/* Switch search and searchng */
    mudconf.signal_crontab = 0;		/* USR1 signals crontab file reading */
    mudconf.max_name_protect = 0;
    mudconf.map_delim_space = 1;      /* output delim is input delim by default */
    mudconf.includenest = 3;		/* Default nesting of @include */
    mudconf.includecnt = 10;		/* Maximum count of @includes per command session */
    mudconf.lfunction_max = 20;		/* Maximum lfunctions allowed per user */
    mudconf.function_max = 1000;	/* Maximum functions allowed period */
    mudconf.blind_snuffs_cons = 0;	/* BLIND flag snuff connect/disconnect */
    mudconf.atrperms_max = 100;		/* Maximum attribute prefix perms */
    mudconf.safer_ufun = 0;		/* are u()'s and the like protected */
    mudconf.listen_parents = 0;		/* ^listens do parents */
    mudconf.icmd_obj = -1;		/* @icmd eval object */
    mudconf.ansi_txtfiles = 0;		/* ANSI textfile support */
    mudconf.list_max_chars = 1000000;	/* Let's allow 1 million characters */
    mudconf.tor_paranoid = 0;
    mudconf.float_precision = 6;		/* Precision of math functions */
    mudconf.file_object = -1;		/* File object for @list_file overloading */
    mudconf.ansi_default = 0;		/* Allow ansi aware functions ansi default */
    mudconf.accent_extend = 0;		/* Can we extend accents from 251-255? */
    mudconf.admin_object = -1;		/* The admin object to define */
    mudconf.enhanced_convtime = 0;	/* Enhanced convtime formatting */
    mudconf.mysql_delay = 0;		/* Toggle to turn on/off delay > 0 sets delay */
    mudconf.name_with_desc = 0;		/* Enable state to allow looking at names of things you can't examine */
    mudconf.proxy_checker = 0;		/* Proxy Checker -- Not very reliable */
    mudconf.idle_stamp = 0;             /* Enable for idle checking on players */
    mudconf.idle_stamp_max = 10;        /* Enable for idle checking on players 10 max default */
    mudconf.penn_setq = 0;		/* Penn compatible setq/setr functions */
    mudconf.delim_null = 0;		/* Allow '@@' for null delims */
    mudconf.parent_follow = 1;		/* Parent allows following if you control target */
    mudconf.objid_localtime = 0;	/* Objid's should use GMtime by default */
    mudconf.objid_offset = 0;		/* seconds of offset that it should use */
    mudconf.hook_offline = 0;		/* Trigger @hook/after on offline player create */
    mudconf.protect_addenh = 0;		/* Enhanced @protect/add */
    mudconf.posesay_funct = 0;		/* Enable functions in SPEECH_PREFIX/SPEECH_SUFFIX */
    mudconf.rollbackmax = 1000;		/* Maximum rollback value (10-10000) */
    mudconf.exec_secure = 1;		/* execscript() escapes out everything by default */
    memset(mudconf.sub_include, '\0', sizeof(mudconf.sub_include));
    memset(mudconf.cap_conjunctions, '\0', sizeof(mudconf.cap_conjunctions));
    memset(mudconf.cap_articles, '\0', sizeof(mudconf.cap_articles));
    memset(mudconf.cap_preposition, '\0', sizeof(mudconf.cap_preposition));
    memset(mudconf.atrperms, '\0', sizeof(mudconf.atrperms));
    memset(mudconf.tor_localhost, '\0', sizeof(mudconf.tor_localhost));
    memset(mudstate.tor_localcache, '\0', sizeof(mudstate.tor_localcache));
    strcpy(mudconf.tree_character, (char *)"`");
#ifdef MYSQL_VERSION
    strcpy(mudconf.mysql_host, (char *)"localhost");
    strcpy(mudconf.mysql_user, (char *)"dbuser");
    strcpy(mudconf.mysql_pass, (char *)"dbpass");
    strcpy(mudconf.mysql_base, (char *)"databasename");
    strcpy(mudconf.mysql_socket, (char *)"/var/lib/mysql/mysql.sock");
    mudconf.mysql_port=3306;
#endif
    mudstate.total_bytesin = 0;		/* Bytes total into the mush */
    mudstate.total_bytesout = 0;	/* Bytes total out of the mush */
    mudstate.daily_bytesin = 0;		/* Bytes total in for current day */
    mudstate.daily_bytesout = 0;	/* Bytes total out for current day */
    mudstate.avg_bytesin = 0;		/* Bytes total in avg */
    mudstate.avg_bytesout = 0;		/* Bytes total out avg */
    mudstate.reset_daily_bytes = time(NULL); /* Reset marker for daily totals */
    mudstate.posesay_dbref = -1;	/* Dbref# of person doing @emit/say/pose */
    mudstate.posesay_fluff = 0;		/* Pose and say fluff */
    mudstate.no_hook = 0;		/* Do not process hooks */
    mudstate.no_hook_count = 0;		/* Count of times hook include can be called per command */
    mudstate.zone_return = 0;		/* State data of zonecmd() */
    mudstate.argtwo_fix = 0;		/* Argument 2 fix for @include and other */
    mudstate.mysql_last = 0;		/* Time of last mysql hang check */
    mudstate.insideaflags = 0;		/* inside @aflags eval check */
    mudstate.insideicmds = 0;		/* inside @icmd eval check */
    mudstate.dumpstatechk = 0;		/* State of the dump state */
    mudstate.forceusr2 = 0;		/* Forcing kill USR2 here */
    mudstate.breakst = 0;
    mudstate.jumpst = 0;
    memset(mudstate.gotolabel, '\0', 16);
    mudstate.gotostate = 0;
    mudstate.rollbackcnt = 0;
    mudstate.rollbackstate = 0;
    mudstate.inlinestate = 0;
    memset(mudstate.rollback, '\0', LBUF_SIZE);
    mudstate.breakdolist = 0;
    mudstate.dolistnest = 0;
    mudstate.twinknum = -1;		/* Dbref of originator if inside a twinklock */
    mudstate.start_of_cmds = 0;		/* hack around zenty ansi parsing */
    mudstate.notrace = 0;		/* Do not trace */
    mudstate.nocodeoverride = 0;	/* Override nocode */
    mudstate.global_regs_wipe = 0;	/* localize variables - wipe if enabled */
    mudstate.includecnt = 0;
    mudstate.includenest = 0;
    mudstate.wipe_state = 0;		/* State of @wipe/wipe() */
    mudstate.blacklist_cnt = 0;		/* Total number of blacklisted elements */
    mudstate.bl_list = NULL;
    mudstate.log_chk_reboot = 0;
    mudstate.f_logfile_name = NULL;
    mudstate.last_network_owner = -1;	/* Last user with network issue */
    mudstate.store_loc = -1;
    mudstate.store_lastcr = -1;
    mudstate.store_lastx1 = -1;
    mudstate.store_lastx2 = -1;
    mudstate.shifted = 0;
    mudstate.trainmode = 0;           /* initialize trainmode variable */
    mudstate.outputflushed = 0;               /* initialize output buffer variable */
    mudstate.allowbypass = 0;		/* Allow bypass() in @functions */
    mudstate.sub_overridestate = 0;	/* %-sub override state */
    mudstate.sub_includestate = 0;	/* %-sub override state */
    mudstate.recurse_rlevel = 0;
    mudstate.func_reverse = 0;
    mudstate.func_ignore = 0;
    mudstate.func_bypass = 0;
    mudstate.nolookie = 1;
    mudstate.reality_notify = 0;
    mudstate.password_nochk = 0;
    mudstate.ahear_count = 0;
    mudstate.ahear_currtime = 0;
    mudstate.ahear_lastplr = -1;
    mudstate.chkcpu_toggle = 0;
    mudstate.chkcpu_inline = 0;
    memset(mudstate.chkcpu_inlinestr, '\0', SBUF_SIZE);
    mudstate.chkcpu_locktog = 0;
    mudstate.chkcpu_stopper = time(NULL);
    mudstate.sidefx_currcalls = 0; /* Counter for sideeffects called */
    mudstate.curr_percentsubs = 0; /* Counter for substitutions called */
    mudstate.cntr_reset = time_ng(NULL);
    mudstate.tog_percentsubs = 0; /* Toggle disabler */
    mudstate.cntr_percentsubs = 0; /* Counter of 3, and then you die */
    mudstate.sidefx_toggle = 0; /* Toggle to show sidefx ceiling reached */
    mudstate.inside_locks = 0;	/* Toggle inside lock check */
    mudconf.max_sitecons = 50; /* Default maximum # of connects from this site */
    mudconf.must_unlquota = 0; /* Must @quota/unlock to give @quota */
    mudconf.partial_conn = 1; /* Enable/Disable @aconnects on partial connect */
    mudconf.partial_deconn = 0; /* Enable/Disable @adisconnects on partial disconnect */
    mudconf.global_parent_obj = -1; /* Generic Global Parent Object for Parenting attrs w/o @parent */
    mudconf.global_parent_room = -1; /* Room Global Parent Object for Parenting attrs w/o @parent */
    mudconf.global_parent_thing = -1; /* Thing Global Parent Object for Parenting attrs w/o @parent */
    mudconf.global_parent_player = -1; /* Player Global Parent Object for Parenting attrs w/o @parent */
    mudconf.global_parent_exit = -1; /* Exit Global Parent Object for Parenting attrs w/o @parent */
    mudconf.global_clone_obj = -1; /* Generic Global Clone for Attribute Cloning */
    mudconf.global_clone_room = -1; /* Room Global Clone for Attribute Cloning */
    mudconf.global_clone_thing = -1; /* Thing Global Clone for Attribute Cloning */
    mudconf.global_clone_player = -1; /* Player Global Clone for Attribute Cloning */
    mudconf.global_clone_exit = -1; /* Exit Global Clone for Attribute Cloning */
    mudconf.global_error_obj = -1; /* Global Error Object for all 'nonexisting' commands - @va */
    mudconf.global_attrdefault = -1; /* Global Attribute Handler for Locks */
    mudconf.signal_object = -1; /* Signal handling object */
    mudconf.signal_object_type = 0; /* Signal object handling type */
    mudconf.zone_parents = 0;	/* Zones inherit attributes to it's children */
    mudconf.restrict_sidefx = 0;
    mudconf.formats_are_local = 0;
    mudconf.descs_are_local = 0;
    mudconf.noregist_onwho = 0;
    mudconf.nospam_connect = 0;
    mudconf.lnum_compat = 0;
    mudconf.nand_compat = 0;
    mudconf.hasattrp_compat = 0; /* Boolean: does hasattrp only check parents */
    mudconf.expand_goto = 0;	/* Expand exit movement to use 'goto' */
    mudconf.global_ansimask = 2097151; /* Always allow all ansi */
    mudconf.imm_nomod = 0; /* Immortal only with NOMODIFY */
    mudconf.paranoid_exit_linking = 0; /* Exit MUST be controlled to link */
    mudconf.notonerr_return = 1; /* If disabled, not() returns '0' for #-1 returns */
    mudconf.nonindxtxt_maxlines = 200; /* Max lines a nonindexed .txt file can have */
    mudconf.hackattr_nowiz = 0;	/* All _attrs defined to be wizard only */
    mudconf.hackattr_see = 0;	/* All _attrs defined to be viewable by wizard only */
    mudconf.penn_playercmds = 0; /* $cmds on players like PENN */
    mudconf.format_compatibility = 0; /* format attributes mux/penn compatible */
    mudconf.brace_compatibility = 0; /* MUX/TM3 brace {} compatibility with parser */
    mudconf.ifelse_compat = 0; /* ifelse() / @ifelse Mux string boolean compatibility */
    mudconf.penn_switches = 0;  /* switch() and switchall() behave like PENN if '1' */
    mudconf.lattr_default_oldstyle = 0;	/* lattr() error's has errors snuffed */
    mudconf.look_moreflags = 0;	/* Show global flags on attributes */
    mudconf.hook_obj = -1;	/* Global HOOK object */
    mudconf.hook_cmd = 0;	/* Global HOOK cmd entry */
    mudconf.stack_limit = 10000; /* Stack limit on commands */
    mudconf.spam_limit = 60;	/* Stack limit of commands per minute */
    mudconf.mail_verbosity = 0;	/* Mail to disconnected players & subjects */
    mudconf.always_blind = 0;	/* BLIND flag always on by default */
    mudconf.empower_fulltel = 0; /* FULLTEL @power */
    mudconf.bcc_hidden = 1;   /* +BCC hides from target who mail is sent to */
    mudconf.exits_conn_rooms = 0; /* If true, then a floating room is one with no exits */
    /* Anonymous player for mail sending */
    strcpy(mudconf.mail_anonymous, "*Anonymous*");
    mudconf.secure_atruselock = 0; /* Global attruselock usage */
    mudconf.player_flags.word1 = 0;
    mudconf.player_flags.word2 = 0;
    mudconf.player_flags.word3 = 0;
    mudconf.player_flags.word4 = 0;
    mudconf.room_flags.word1 = 0;
    mudconf.room_flags.word2 = 0;
    mudconf.room_flags.word3 = 0;
    mudconf.room_flags.word4 = 0;
    mudconf.exit_flags.word1 = 0;
    mudconf.exit_flags.word2 = 0;
    mudconf.exit_flags.word3 = 0;
    mudconf.exit_flags.word4 = 0;
    mudconf.thing_flags.word1 = 0;
    mudconf.thing_flags.word2 = 0;
    mudconf.thing_flags.word3 = 0;
    mudconf.thing_flags.word4 = 0;
    mudconf.robot_flags.word1 = ROBOT;
    mudconf.robot_flags.word2 = 0;
    mudconf.robot_flags.word3 = 0;
    mudconf.robot_flags.word4 = 0;
    mudconf.player_toggles.word1 = 0;
    mudconf.player_toggles.word2 = 0;
    mudconf.player_toggles.word3 = 0;
    mudconf.player_toggles.word4 = 0;
    mudconf.player_toggles.word5 = 0;
    mudconf.player_toggles.word6 = 0;
    mudconf.player_toggles.word7 = 0;
    mudconf.player_toggles.word8 = 0;
    mudconf.room_toggles.word1 = 0;
    mudconf.room_toggles.word2 = 0;
    mudconf.room_toggles.word3 = 0;
    mudconf.room_toggles.word4 = 0;
    mudconf.room_toggles.word5 = 0;
    mudconf.room_toggles.word6 = 0;
    mudconf.room_toggles.word7 = 0;
    mudconf.room_toggles.word8 = 0;
    mudconf.exit_toggles.word1 = 0;
    mudconf.exit_toggles.word2 = 0;
    mudconf.exit_toggles.word3 = 0;
    mudconf.exit_toggles.word4 = 0;
    mudconf.exit_toggles.word5 = 0;
    mudconf.exit_toggles.word6 = 0;
    mudconf.exit_toggles.word7 = 0;
    mudconf.exit_toggles.word8 = 0;
    mudconf.thing_toggles.word1 = 0;
    mudconf.thing_toggles.word2 = 0;
    mudconf.thing_toggles.word3 = 0;
    mudconf.thing_toggles.word4 = 0;
    mudconf.thing_toggles.word5 = 0;
    mudconf.thing_toggles.word6 = 0;
    mudconf.thing_toggles.word7 = 0;
    mudconf.thing_toggles.word8 = 0;
    mudconf.robot_toggles.word1 = 0;
    mudconf.robot_toggles.word2 = 0;
    mudconf.robot_toggles.word3 = 0;
    mudconf.robot_toggles.word4 = 0;
    mudconf.robot_toggles.word5 = 0;
    mudconf.robot_toggles.word6 = 0;
    mudconf.robot_toggles.word7 = 0;
    mudconf.robot_toggles.word8 = 0;
    mudconf.vlimit = 400;
    mudconf.vattr_flags = AF_ODARK;
    mudconf.abort_on_bug = 0;
    mudconf.rwho_transmit = 0;
    strcpy(mudconf.muddb_name, "RhostMUSH");
    strcpy(mudconf.mud_name, "RhostMUSH");
    strcpy(mudconf.rwho_host, "139.78.1.15");	/* riemann.math.okstate.edu */
    strcpy(mudconf.rwho_pass, "get_your_own");
    mudconf.rwho_info_port = 6888;
    mudconf.rwho_data_port = 6889;
    mudconf.rwho_interval = 241;
    strcpy(mudconf.one_coin, "penny");
    strcpy(mudconf.many_coins, "pennies");
    mudconf.global_aconn = 0;
    mudconf.global_adconn = 0;
    mudconf.room_aconn = 0;
    mudconf.room_adconn = 0;
    mudconf.timeslice = 1000;
    mudconf.cmd_quota_max = 100;
    mudconf.cmd_quota_incr = 1;
    mudconf.control_flags = 0xffffffff;		/* Everything for now... */
    mudconf.log_options = LOG_ALWAYS | LOG_BUGS | LOG_SECURITY |
	LOG_NET | LOG_LOGIN | LOG_DBSAVES | LOG_CONFIGMODS |
	LOG_SHOUTS | LOG_STARTUP | LOG_WIZARD |
	LOG_PROBLEMS | LOG_PCREATES | LOG_MAIL | LOG_DOOR;
    mudconf.log_info = LOGOPT_TIMESTAMP | LOGOPT_LOC;
    mudconf.markdata[0] = 0x01;
    mudconf.markdata[1] = 0x02;
    mudconf.markdata[2] = 0x04;
    mudconf.markdata[3] = 0x08;
    mudconf.markdata[4] = 0x10;
    mudconf.markdata[5] = 0x20;
    mudconf.markdata[6] = 0x40;
    mudconf.markdata[7] = 0x80;
    mudconf.func_nest_lim = 50;
    mudconf.func_invk_lim = 2500;
    mudconf.wildmatch_lim = 100000;
    mudconf.ntfy_nest_lim = 20;
    mudconf.lock_nest_lim = 20;
    mudconf.parent_nest_lim = 10;
    mudconf.cache_trim = 0;
    mudconf.cache_depth = CACHE_DEPTH;
    mudconf.cache_width = CACHE_WIDTH;
    mudconf.cache_names = 1;
    mudconf.cache_steal_dirty = 0;
    mudconf.money_limit[0] = 0;
    mudconf.money_limit[1] = 500;
    mudconf.money_limit[2] = 2000;
    mudconf.money_limit[3] = 5000;
    mudconf.offline_reg = 0;
    mudconf.online_reg = 0;
    mudconf.areg_lim = 3;
    mudconf.whohost_size = 0;
    mudconf.who_showwiz = 0;
    mudconf.who_showwiztype = 0;
    mudconf.wiz_override = 1;
    mudconf.wiz_uselock = 1;
    mudconf.idle_wiz_cloak = 0;
    mudconf.idle_message = 1;
    mudconf.no_startup = 0;
    mudconf.newpass_god = 0;
    mudconf.who_wizlevel = 0;
    mudconf.allow_whodark = 1;
    mudconf.allow_ansinames = 15;
    mudconf.who_comment = 1;
    mudconf.safe_wipe = 0;
    mudconf.secure_jumpok = 0;
    mudconf.fmt_contents = 0;
    mudconf.fmt_exits = 0;
    mudconf.fmt_names = 0;
    mudconf.enable_tstamps = 0;
    mudconf.secure_dark = 0;
    mudconf.thing_parent = -1;
    mudconf.room_parent = -1;
    mudconf.exit_parent = -1;
    mudconf.player_parent = -1;
    mudconf.thing_defobj = -1;
    mudconf.room_defobj = -1;
    mudconf.exit_defobj = -1;
    mudconf.player_defobj = -1;
    mudconf.alt_inventories = 0;
    mudconf.altover_inv = 0;
    mudconf.showother_altinv = 0;
    mudconf.restrict_home = 0;
    mudconf.restrict_home2 = 0;
    mudconf.vattr_limit_checkwiz = 0; /* Wizard Vattr Limit Check Enabled? */
    mudconf.wizmax_vattr_limit = 1000000; /* Maximum new attributes wizard can make */
    mudconf.max_vattr_limit = 10000; /* Maximum new attributes player can make */
    mudconf.max_dest_limit = 1000; /* Maximum number of items player can @destroy */
    mudconf.wizmax_dest_limit = 100000; /* Maximum number of items player can @destroy */
    mudconf.hide_nospoof = 0;	/* Don't hide nospoof by default */
/* CPU information */
    /* Number of CPU slams allowed per dbref# in a row - default 3 */
    mudconf.max_cpu_cycles = 3;
    /* Level of security for CPU.
     * 0 - standard (no limit per dbref)
     * 1 - auto-halt enactor
     * 2 - auto-halt enactor and set fubar
     * 3 - auto-halt enactor, set fubar, and set NOCONNECT and boot
     */
    mudconf.cpu_secure_lvl = 0;
    /* 80% cpu usage - range 50-100 */
    mudconf.cpuintervalchk = 80;
    /* One minute - range 60-3600 */
    mudconf.cputimechk = 60;
/* End CPU information */
    /* Default mail object */
    mudconf.mail_def_object = -1;
    /* Enable '@' automatically for mail */
    mudconf.mail_tolist = 0;
    /* Default 'mail' to 'mail/quick' */
    mudconf.mail_default = 0;
    /* Player can log into program */
    mudconf.login_to_prog = 1;
    /* Player (minus immortal) can not shell from program */
    mudconf.noshell_prog = 0;
    /* Autodelete time for mail - default 21 days */
    mudconf.mail_autodeltime = 21;
    strcpy(mudconf.invname, "backpack");
    mudconf.secure_functions = 0;
    mudconf.heavy_cpu_max = 50;	/* Max cycles of a heavy-used cpu-intensive
                                 * function per command
                                 */
    /* Connection security level */
    mudstate.cmp_lastsite = -1;
    mudstate.cmp_lastsite_cnt = 0;
    mudstate.api_lastsite = -1;
    mudstate.api_lastsite_cnt = 0;
    mudconf.lastsite_paranoia = 0; /* 0-off, 1-register, 2-forbid */
    mudconf.pcreate_paranoia = 0; /* 0-off, 1-register, 2-forbid */
    mudconf.max_lastsite_cnt = 20;	/* Shouldn't connect more than 20 times in X period */
    mudconf.max_lastsite_api = 60;	/* API Shouldn't connect more than 60 times in X period */
    mudconf.min_con_attempt = 60;	/* 60 seconds default */
    mudstate.last_con_attempt = 0;
    mudstate.last_apicon_attempt = 0;
    mudconf.max_pcreate_lim = -1;
    mudconf.max_pcreate_time = 60;
    mudstate.last_pcreate_time = 0;
    mudstate.last_pcreate_cnt = 0;
    mudstate.reverse_wild = 0;
    mudstate.stack_val = 0;
    mudstate.stack_toggle = 0;
    mudstate.stack_cntr = 0;
    mudstate.train_cntr = 0;
    mudstate.sudo_cntr = 0;

#ifdef REALITY_LEVELS
    mudconf.no_levels = 0;
    mudconf.wiz_always_real = 0;
    mudconf.reality_locks = 0; /* @Lock/user used as Reality Lock? */
    mudconf.reality_locktype = 0; /* Type of lock */
    mudconf.def_room_rx = 1;
    mudconf.def_room_tx = ~(RLEVEL)0;
    mudconf.def_player_rx = 1;
    mudconf.def_player_tx = 1;
    mudconf.def_exit_rx = 1;
    mudconf.def_exit_tx = 1;
    mudconf.def_thing_rx = 1;
    mudconf.def_thing_tx = 1;
    mudconf.reality_compare = 0;	/* Change how descs are displayed */
#endif /* REALITY_LEVELS */
#ifdef SQLITE
    mudconf.sqlite_query_limit = 5;
    strcpy( mudconf.sqlite_db_path, "sqlite" );
#endif /* SQLITE */

    /* maximum logs allowed per command */
    mudconf.log_maximum = 1;
    mudconf.cluster_cap = 10;	/* Cap of cluster wait in seconds for action */
    mudconf.clusterfunc_cap = 1;/* Cap of cluster wait in seconds for action function */
    mudstate.clust_time = 0;
    mudstate.log_maximum = 0;

    memset(mudstate.last_command, 0, sizeof(mudstate.last_command));
    mudstate.new_vattr = 0;
    mudstate.last_cmd_timestamp = 0;
    mudstate.heavy_cpu_recurse = 0;
    mudstate.heavy_cpu_tmark1 = time(NULL);
    mudstate.heavy_cpu_tmark2 = time(NULL);
    mudstate.heavy_cpu_lockdown = 0;
    mudstate.max_logins_allowed = 0;
    mudstate.iter_inum = -1;
    /* Current CPU slam level */
    mudstate.curr_cpu_cycle=0;
    /* Last dbref# to CPU-SLAM */
    mudstate.curr_cpu_user=NOTHING;
    mudstate.last_cpu_user=NOTHING;
    mudstate.force_halt = 0;
    mudstate.autoreg = 0;
    mudstate.initializing = 0;
    mudstate.panicking = 0;
    mudstate.logging = 0;
    mudstate.epoch = 0;
    mudstate.generation = 0;
    mudstate.curr_player = NOTHING;
    mudstate.curr_enactor = NOTHING;
    mudstate.curr_cmd = (char *) "< none >";
    memset(mudstate.curr_cmd_hook, '\0', LBUF_SIZE);
    mudstate.shutdown_flag = 0;
    mudstate.reboot_flag = 0;
    mudstate.rwho_on = 0;
    mudstate.attr_next = A_USER_START;
    mudstate.debug_cmd = (char *) "< init >";
    strcpy(mudstate.doing_hdr, "Doing");
    strcpy(mudstate.ng_doing_hdr, "Doing");
    strcpy(mudstate.guild_hdr, "Guild");
    mudstate.access_list = NULL;
    mudstate.suspect_list = NULL;
    mudstate.special_list = NULL;
    mudstate.qfirst = NULL;
    mudstate.qlast = NULL;
    mudstate.qlfirst = NULL;
    mudstate.qllast = NULL;
    mudstate.qwait = NULL;
    mudstate.qsemfirst = NULL;
    mudstate.qsemlast = NULL;
    mudstate.fqwait = NULL;
    mudstate.fqsemfirst = NULL;
    mudstate.fqsemlast = NULL;
    mudstate.badname_head = NULL;
    mudstate.protectname_head = NULL;
    mudstate.lbuf_buffer = NULL;
    mudstate.mstat_ixrss[0] = 0;
    mudstate.mstat_ixrss[1] = 0;
    mudstate.mstat_idrss[0] = 0;
    mudstate.mstat_idrss[1] = 0;
    mudstate.mstat_isrss[0] = 0;
    mudstate.mstat_isrss[1] = 0;
    mudstate.mstat_secs[0] = 0;
    mudstate.mstat_secs[1] = 0;
    mudstate.mstat_curr = 0;
    mudstate.iter_alist.data = NULL;
    mudstate.iter_alist.len = 0;
    mudstate.iter_alist.next = NULL;
    mudstate.mod_alist = NULL;
    mudstate.mod_size = 0;
    mudstate.mod_al_id = NOTHING;
    mudstate.min_size = 0;
    mudstate.db_top = 0;
    mudstate.db_size = 0;
    mudstate.freelist = NOTHING;
    mudstate.markbits = NULL;
    mudstate.trace_nest_lev = 0;
    mudstate.func_nest_lev = 0;
    mudstate.ufunc_nest_lev = 0;
    mudstate.func_invk_ctr = 0;
    mudstate.ntfy_nest_lev = 0;
    mudstate.lock_nest_lev = 0;
    mudstate.whisper_state = 0;
    mudstate.nowall_over = 0;
    mudstate.eval_rec = 0;
    for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	mudstate.global_regs[i] = NULL;
	mudstate.global_regsname[i] = NULL;
	mudstate.global_regs_backup[i] = NULL;
    }
    mudstate.remote = NOTHING;
    mudstate.remotep = NOTHING;
#ifdef EXPANDED_QREGS
    strcpy(mudstate.nameofqreg, "0123456789abcdefghijklmnopqrstuvwxyz");
    mudstate.nameofqreg[36]='\0';
#else
    strcpy(mudstate.nameofqreg, "0123456789");
    mudstate.nameofqreg[10]='\0';
#endif
    mudstate.emit_substitute = 0; /* Toggle @emit/substitute */
    mudconf.allow_fancy_quotes = 0;  /* Allow UTF-8 double quote characters */
    mudconf.allow_fullwidth_colon = 0; /* Allow UTF-8 fullwidth colon character */
#else
    mudconf.paylimit = 10000;
    mudconf.digcost = 10;
    mudconf.opencost = 1;
    mudconf.robotcost = 1000;
    mudconf.createmin = 5;
    mudconf.createmax = 505;
    mudconf.sacfactor = 5;
    mudconf.sacadjust = -1;
    mudconf.room_quota = 1;
    mudconf.exit_quota = 1;
    mudconf.thing_quota = 1;
    mudconf.player_quota = 1;
    mudconf.quotas = 0;
    mudconf.start_room = 0;
    mudconf.start_home = -1;
    mudconf.default_home = -1;
    mudconf.vattr_flags = AF_ODARK;
    mudconf.log_options = 0xffffffff;
    mudconf.log_info = 0;
    mudconf.markdata[0] = 0x01;
    mudconf.markdata[1] = 0x02;
    mudconf.markdata[2] = 0x04;
    mudconf.markdata[3] = 0x08;
    mudconf.markdata[4] = 0x10;
    mudconf.markdata[5] = 0x20;
    mudconf.markdata[6] = 0x40;
    mudconf.markdata[7] = 0x80;
    mudconf.ntfy_nest_lim = 20;
    mudconf.cache_trim = 0;
    mudconf.cache_steal_dirty = 1;
    mudconf.vlimit = 750;
    mudconf.safer_passwords = 0; /* If enabled, requires tougher to guess passwords */
    mudconf.vattr_limit_checkwiz = 0; /* Check if wizards check vattr limits */
    mudstate.logging = 0;
    mudstate.attr_next = A_USER_START;
    mudstate.iter_alist.data = NULL;
    mudstate.iter_alist.len = 0;
    mudstate.iter_alist.next = NULL;
    mudstate.mod_alist = NULL;
    mudstate.mod_size = 0;
    mudstate.mod_al_id = NOTHING;
    mudstate.min_size = 0;
    mudstate.db_top = 0;
    mudstate.db_size = 0;
    mudstate.freelist = NOTHING;
    mudstate.markbits = NULL;
    mudstate.remote = NOTHING;
    mudstate.remotep = NOTHING;
#endif /* STANDALONE */
}


#ifndef STANDALONE

#ifdef REALITY_LEVELS
static int
countwordsnew(char *str)
{
    int n;

/* Trim starting white space */
    n = 0;
    while (*str && isspace((int)*str))
       str++;
/* Count words */
    while ( *str ) {
       while (*str && !isspace((int)*str))
          str++;
       n++;
       while (*str && isspace((int)*str))
          str++;
    }
    return n;
}
#endif

/* ---------------------------------------------------------------------------
 * cf_log_notfound: Log a 'parameter not found' error.
 */

void
cf_log_notfound(dbref player, char *cmd, const char *thingname,
		char *thing)
{
    char *buff;

    /* Modified this so buffers are cut off on unrecognized commands */
    if (mudstate.initializing) {
	STARTLOG(LOG_STARTUP, "CNF", "NFND")
	    buff = alloc_lbuf("cf_log_notfound.LOG");
	sprintf(buff, "%.150s: %.1900s %.1900s not found",
		cmd, thingname, thing);
	log_text(buff);
	free_lbuf(buff);
	ENDLOG
    } else {
	buff = alloc_lbuf("cf_log_notfound");
	sprintf(buff, "%.1900s %.1900s not found", thingname, thing);
	notify(player, buff);
	free_lbuf(buff);
    }
}

/* ---------------------------------------------------------------------------
 * cf_log_syntax: Log a syntax error.
 */

void
cf_log_syntax(dbref player, char *cmd, const char *template, char *arg)
{
    char *buff;

    if (mudstate.initializing) {
	STARTLOG(LOG_STARTUP, "CNF", "SYNTX")
	    buff = alloc_lbuf("cf_log_syntax.LOG");
	sprintf(buff, template, arg);
	log_text(cmd);
	log_text((char *) ": ");
	log_text(buff);
	free_lbuf(buff);
	ENDLOG
    } else {
	buff = alloc_lbuf("cf_log_syntax");
	sprintf(buff, template, arg);
	notify(player, buff);
	free_lbuf(buff);
    }
}

/* ---------------------------------------------------------------------------
 * cf_status_from_succfail: Return command status from succ and fail info
 */

int
cf_status_from_succfail(dbref player, char *cmd, int success, int failure)
{
    char *buff;

    /* If any successes, return SUCCESS(0) if no failures or
       PARTIAL_SUCCESS(1) if any failures. */

    if (success > 0)
	return ((failure == 0) ? 0 : 1);

    /* No successes.  If no failures indicate nothing done.
       Always return FAILURE(-1) */

    if (failure == 0) {
	if (mudstate.initializing) {
	    STARTLOG(LOG_STARTUP, "CNF", "NDATA")
		buff = alloc_lbuf("cf_status_from_succfail.LOG");
	    sprintf(buff, "%.3900s: Nothing to set", cmd);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    notify(player, "Nothing to set");
	}
    }
    return -1;
}

#ifdef REALITY_LEVELS
/* ---------------------------------------------------------------------------
 * cf_rlevel: Set reality level
 */
CF_HAND(cf_rlevel)
{
    CONFDATA *mc = (CONFDATA *)vp;
    int i, j, k, cmp_x, cmp_y, cmp_z;
    unsigned int q;
    char dup1[17], dup2[17], *nptr, tbufit[20], *strbuff, *pst;

    cmp_z = countwordsnew(str);
    if ( (cmp_z < 2) || (cmp_z > 3) ) {
	STARTLOG(LOG_STARTUP, "CNF", "LEVEL")
        if ( cmp_z < 2 )
           log_text("Too few arguments for reality_level. 2 to 3 expected, ");
        else
           log_text("Too many arguments for reality_level. 2 to 3 expected, ");
        sprintf(tbufit, "%d", cmp_z);
        log_text(tbufit);
        log_text(" found.\r\n              -->String in question: ");
        log_text(str);
        ENDLOG
        return 1;
    }
    if(mc->no_levels >= 32) {
	STARTLOG(LOG_STARTUP, "CNF", "LEVEL")
        log_text("Too many levels defined. 32 maximum allowed.");
        ENDLOG
        return 1;
    }

    for(i=0; *str && ((*str != ' ') && (*str != '\t')) && (i < 16); ++str) {
        if(i < 16) {
            dup1[i] = tolower(*str);
            mc->reality_level[mc->no_levels].name[i++] = *str;
        }
    }
    dup1[i] = '\0';

    cmp_x = sizeof(mudconf.reality_level);
    cmp_y = sizeof(mudconf.reality_level[0]);
    if ( cmp_y == 0 )
       cmp_z = 0;
    else
       cmp_z = cmp_x / cmp_y;
    for (k = 0; (k < mudconf.no_levels) && (k < cmp_z); ++k) {
      nptr = mudconf.reality_level[k].name;
      j=0;
      while (*nptr) {
        dup2[j++] = tolower(*nptr);
        nptr++;
      }
      dup2[j] = '\0';
      if ( strcmp(dup1, dup2) == 0 ) {
	 STARTLOG(LOG_STARTUP, "CNF", "LEVEL")
         log_text("Duplicate RLEVEL name: ");
         log_text(mudconf.reality_level[mc->no_levels].name);
         ENDLOG
         mudconf.reality_level[mc->no_levels].name[0] = '\0';
         return 0;
      }
    }

    /* If name is over 16 chars, trim off the rest */
    if ( *str && ((*str != ' ') && (*str != '\t')) )
       for(; *str && ((*str != ' ') && (*str != '\t')); ++str);

    mc->reality_level[mc->no_levels].name[i] = '\0';
    mc->reality_level[mc->no_levels].value = 1;
    strcpy(mc->reality_level[mc->no_levels].attr, "DESC");
    for(; *str && (*str == ' ' || *str == '\t'); ++str);
    strbuff = alloc_lbuf("reality_loader");
    memset(strbuff, '\0', LBUF_SIZE);
    pst = strbuff;
    while ( *str && (isxdigit((int)*str) || (ToLower(*str) == 'x')) ) {
       *pst++ = *str++;
    }
    q = (unsigned int)atof(strbuff);
    
    free_lbuf(strbuff);
    if(q)
        mc->reality_level[mc->no_levels].value = (RLEVEL) q;
    for(; *str && ((*str == ' ') || (*str == '\t')); ++str);
    if(*str) {
        strncpy(mc->reality_level[mc->no_levels].attr, str, 32);
        mc->reality_level[mc->no_levels].attr[32] = '\0';
    }
    mc->no_levels++;
    return 0;
}
#endif /* REALITY_LEVELS */


/* ---------------------------------------------------------------------------
 * cf_int: Set integer parameter.
 */

CF_HAND(cf_int)
{
    /* Copy the numeric value to the parameter */

  if (sscanf(str, "%d", vp)) {
    return 0;
  } else {
    if (*str == '#') {
      if (player >= 0) {
	notify(player, unsafe_tprintf("'%s' is not a valid integer. Maybe try without the '#'?",str));
      } else {
	STARTLOG(LOG_CONFIGMODS, "CFG", "ERR")
	  log_name(player);
	  log_text(str);
          log_text("is not valid. Maybe try without the '#'?");
	ENDLOG;
      }
    }
    return -1;
  }
}


CF_HAND(cf_chartoint)
{
  char s_list[]="n#!@lsopartcxfkwm:";
  int  s_mask[]={ 0x00000001, 0x00000002, 0x00000004, 0x00000008,
                  0x00000010, 0x00000020, 0x00000040, 0x00000080,
                  0x00000100, 0x00000200, 0x00000400, 0x00000800,
                  0x00001000, 0x00002000, 0x00004000, 0x00008000,
                  0x00010000, 0x00020000, 0x00040000, 0x00080000,
                  0x00000000 };
  char *s, *t, *fail_str, *fail_strptr;
  int i_return, i_mask;

  if ( (atoi(str) > 0) || (strcmp(str, "0") == 0) ) {
     i_return = cf_int(vp, str, extra, extra2, player, cmd);
     return i_return;
  } 

  s = str;
  i_mask = 0;
  fail_strptr = fail_str = alloc_lbuf("cf_chartoint");
  while ( *s ) {
     t = strchr(s_list, *s);
     if ( t ) {
        i_mask = i_mask | s_mask[t - s_list];
     } else {
        if ( strchr(fail_str, *s) == NULL ) {
           safe_chr(*s, fail_str, &fail_strptr);
        }
     }
     s++;
  }
  if ( !mudstate.initializing && *fail_str ) {
     notify(player, unsafe_tprintf("Invalid substitutions: %s", fail_str));
  }
  free_lbuf(fail_str);
  *vp = i_mask;
  return 0;
}

CF_HAND(cf_int_runtime)
{
   /* Copy the numeric value to the parameter but ONLY on startup */
    if (mudstate.initializing) {
       sscanf(str, "%d", vp);
       return 0;
    }
    else
       return -1;
}

CF_HAND(cf_vint)
{
    int i_ceil = 10000, vp_old=0;
    char s_buf[20];
  
    sscanf(str, "%d", &vp_old);
#ifdef QDBM
    i_ceil = 10000;
    sprintf(s_buf, (char *)"[QDBM Mode]");
#else
#ifdef BIT64
    i_ceil = 400;
    sprintf(s_buf, (char *)"[GDBM 64Bit Mode]");
#else
    sprintf(s_buf, (char *)"[GDBM 32Bit Mode]");
    i_ceil = 750;
#endif
#endif
    if ((vp_old < 0) || (vp_old > i_ceil)) {
        if ( !mudstate.initializing) {
           notify(player, unsafe_tprintf("%s Value must be between 0 and %d.", s_buf, i_ceil));
        }
	return -1;
    } else {
        *vp = vp_old;
	return 0;
    }
}

CF_HAND(cf_recurseint)
{
    int i_ceil = 50, vp_old=0;
  
    sscanf(str, "%d", &vp_old);
    i_ceil = STACKMAX / 10;
    if ((vp_old < 0) || (vp_old > i_ceil)) {
        if ( !mudstate.initializing) {
           notify(player, unsafe_tprintf("Value must be between 0 and %d [Stackmax is %d].", i_ceil, STACKMAX));
        }
	return -1;
    } else {
        *vp = vp_old;
	return 0;
    }
}

CF_HAND(cf_mailint)
{
    int vp_old = 0;

    sscanf(str, "%d", &vp_old);
    if ((vp_old < 10) || (vp_old > 9999)) {
        if ( !mudstate.initializing) 
           notify(player, "Value must be between 10 and 9999.");
	return -1;
    } else {
        *vp = vp_old;
	return 0;
    }
}
CF_HAND(cf_verifyint)
{
    int vp_old = 0;

    sscanf(str, "%d", &vp_old);
    if ((vp_old < extra2) || (vp_old > extra)) {
        if ( !mudstate.initializing) 
           notify(player, unsafe_tprintf("Value must be between %d and %d.", extra2, extra));
	return -1;
    } else {
        *vp = vp_old;
	return 0;
    }
}
CF_HAND(cf_verifyint_mysql)
{
    int vp_old = 0;

    sscanf(str, "%d", &vp_old);
    if ( (vp_old != 0) && ((vp_old < extra2) || (vp_old > extra)) ) {
        if ( !mudstate.initializing) 
           notify(player, unsafe_tprintf("Value must be 0 or between %d and %d.", extra2, extra));
	return -1;
    } else {
        *vp = vp_old;
	return 0;
    }
}

/* ---------------------------------------------------------------------------
 * cf_sidefx: Set up sidefx flags.
 */

/* The value of MUX/PENN/ALL are taken from 'wizhelp sideeffects' */
#define MUXMASK         0x00010A3F /* SET CREATE LINK PEMIT TEL LIST OEMIT PARENT DESTROY */
#define PENNMASK        0x1006FFDF /* SET CREATE LINK PEMIT TEL DIG OPEN EMIT OEMIT CLONE PARENT LOCK LEMIT REMIT WIPE ZEMIT NAME */
#define ALLMASK		0x1FFFFFFF /* 2147483647 would be 'really all', i.e. 32 1s */
#define NO_FLAG_FOUND   -1

/* The conversion function relies on the position of words in this array to match
 * their bit alignment. The list must be NULL terminated */
const char * sideEffects[] = {
  "SET" , "CREATE", "LINK", "PEMIT", "TEL", "LIST", "DIG", "OPEN", "EMIT",
  "OEMIT", "CLONE", "PARENT", "LOCK", "LEMIT", "REMIT", "WIPE", "DESTROY",
  "ZEMIT", "NAME", "TOGGLE", "TXLEVEL", "RXLEVEL", "RSET", "MOVE", "CLUSTER_ADD", 
  "MAILSEND", "EXECSCRIPT", "ZONE", "LSET", NULL
};

/* This function takes an integer mask and converts it to a string list
 * of sideeffect words. It's used by 'config(sideeffects_txt)'
 */
void sideEffectMaskToString(int mask, char *buff, char **bufc) {
  int bFirst = 0;
  int i;
  *buff = '\0';

  /* Go through each sideeffect, seeing if it's defined
   * in the given mask
   */
  for (i = 0 ; sideEffects[i] != NULL ; i++) {
    if (mask & (int) pow(2.0, (double) i)) {
      if (bFirst) {
	safe_chr(' ', buff, bufc);
      }
      safe_str((char *) sideEffects[i], buff, bufc);
      bFirst++;
    }
  }

  /* Exception case - could also check for mask == 0 */
  if (*buff == '\0')
    safe_str( (char *) "NONE", buff, bufc);
}

/* Main config function for sideeffects.. has to be smart as users may expect
 * it to handle integers still!
 */
CF_HAND(cf_sidefx) {
  int mask = 0;
  int i, nErr = 0, flag, bNegate;
  double d_val;
  char *ptr, *eosptr, *tstrtokr;
  int retval, i_mark;

  ptr = str;

  /* If nothing passed in, return nothing */
  if (str == NULL || *str == '\0') {
    if ( player > 0 ) {
      notify(player, "Sideeffects were not changed.");
    }
    return 0;
  }

  *vp = 0;
  i_mark = 0;
  /* Check for an oldstyle integer definition
   * Note: Will not support a mix of old + new
   */
  if (*str >= '0' && *str <= '9') {
    retval = cf_int(vp, str, extra, extra2, player, cmd);
    /* Handle hex */
    if ( strstr(str, "0x") != NULL ) {
       d_val = safe_atof(str);
       *vp = (int)d_val;
    }
    eosptr = ptr = alloc_lbuf("cf_sidefx");
    *eosptr = '\0';

    sideEffectMaskToString(*vp, ptr, &eosptr);
    if (player > 0) {
      notify(player, unsafe_tprintf("Sideeffects set to %s", ptr));
    } else {
      STARTLOG(LOG_CONFIGMODS, "CFG", "INF")
	log_text("Sideeffects set to ");
      log_text(ptr);
      ENDLOG;
    }
    free_lbuf(ptr);
    return retval;
  }

  /* Make sure we have a capitalized string
   * Avoids having to use strcmpi
   */
  while (*ptr != '\0') {
    *ptr = toupper(*ptr);
    ptr++;
  }

  /* Tokenize str, and look at each token to see if it matches */
  ptr = strtok_r(str, " ", &tstrtokr);
  while (ptr) {
    flag = NO_FLAG_FOUND;
    bNegate = (*ptr == '!') ? 1 : 0; /* Did they specify !SIDEEFFECT ? */

    if (strcmp("PENN", &ptr[bNegate]) == 0) {
      flag = PENNMASK;
      i_mark |= 1;
    } else if (strcmp("MUX", &ptr[bNegate]) == 0) {
      flag = MUXMASK;
      i_mark |= 2;
    } else if (strcmp("ALL", &ptr[bNegate]) == 0) {
      flag = ALLMASK;
      i_mark |= 4;
    } else if (strcmp("NONE", &ptr[bNegate]) == 0) {
      flag = 0;
    } else {
      /* If not one of the special words above, try matching against the
       * mask array.
       * This is a linear scan for each token
       */
      for (i = 0 ; sideEffects[i] != NULL ; i++) {
	if (strcmp(&ptr[bNegate], sideEffects[i]) == 0) {
	  flag = (int) pow(2.0, (double) i);
	  break;
	}
      }
    }

    /* If we found a flag, set / unset it as appropriate */
    if (NO_FLAG_FOUND != flag) {
      mask = bNegate ? (mask & ~flag) : (mask | flag);
    } else {
      /* Whine at the user */
      nErr++;
      if (player >= 0) {
	notify(player, unsafe_tprintf("%s is not a valid sideeffect.", &ptr[bNegate]));
      } else {
	STARTLOG(LOG_CONFIGMODS, "CFG", "ERR")
	  log_text(&ptr[bNegate]);
	log_text("is not a valid sideeffect.");
	ENDLOG;
      }
    }
    /* Push to next token */
    ptr = strtok_r(NULL, " ", &tstrtokr);
  }
  /* Set mask in conf structure, and return with error indicator */
  *vp = mask;

  if ( mask != 0 ) {
    eosptr = ptr = alloc_lbuf("cf_sidefx");
    *eosptr = '\0';
    sideEffectMaskToString(*vp, ptr, &eosptr);
    if (player > 0) {
      if ( i_mark & 4 ) {
         notify(player, unsafe_tprintf("Sideeffects set to [ALL] %s", ptr));
      } else if ( i_mark == 3 ) {
         notify(player, unsafe_tprintf("Sideeffects set to [MUX PENN] %s", ptr));
      } else if ( i_mark & 2 ) {
         notify(player, unsafe_tprintf("Sideeffects set to [MUX] %s", ptr));
      } else if ( i_mark & 1 ) {
         notify(player, unsafe_tprintf("Sideeffects set to [PENN] %s", ptr));
      } else {
         notify(player, unsafe_tprintf("Sideeffects set to %s", ptr));
      }
    } else {
       STARTLOG(LOG_CONFIGMODS, "CFG", "INF")
         log_text("Sideeffects set to ");
         log_text(ptr);
       ENDLOG;
    }
    free_lbuf(ptr);
  } else {
    if ( player > 0 ) {
       notify(player, "Sideeffects have been cleared.");
    }
    STARTLOG(LOG_CONFIGMODS, "CFG", "INF")
      log_text("Sideeffects have been cleared.");
    ENDLOG;
  }
  return (nErr > 0) ? 1 : 0;
}


/* ---------------------------------------------------------------------------
 * cf_bool: Set boolean parameter.
 */

NAMETAB hook_names[] =
{
    {(char *) "before", 3, 0, 0, 1},
    {(char *) "after", 3, 0, 0, 2},
    {(char *) "permit", 3, 0, 0, 4},
    {(char *) "ignore", 3, 0, 0, 8},
    {(char *) "igswitch", 3, 0, 0, 16},
    {(char *) "extend", 3, 0, 0, 16},
    {(char *) "afail", 3, 0, 0, 32},
    {(char *) "fail", 3, 0, 0, 256},
    {(char *) "include", 3, 0, 0, 512},
    {NULL, 0, 0, 0, 0}};

CF_HAND(cf_hook)
{
    char *hookcmd, *hookptr, playbuff[201], *tstrtokr;
    int hookflg, retval;
    CMDENT *cmdp;

    retval = -1;
    memset(playbuff, 0, sizeof(playbuff));
    strncpy(playbuff, str, 200);
    hookcmd = strtok_r(playbuff, " \t", &tstrtokr);
    if ( hookcmd != NULL )
       cmdp = (CMDENT *)hashfind(hookcmd, &mudstate.command_htab);
    else
       return retval;
    if ( !cmdp )
       return retval;

    *vp = cmdp->hookmask;
    strncpy(playbuff, str, 200);
    hookptr = strtok_r(NULL, " \t", &tstrtokr);
    while ( hookptr != NULL ) {
       if ( *hookptr == '!' && *(hookptr+1)) {
          hookflg = search_nametab(GOD, hook_names, hookptr+1);
          if ( hookflg != -1 ) {
             retval = 0;
             *vp = *vp & ~hookflg;
          }
       } else {
          hookflg = search_nametab(GOD, hook_names, hookptr);
          if ( hookflg != -1 ) {
             retval = 0;
             *vp = *vp | hookflg;
          }
       }
       hookptr = strtok_r(NULL, " \t", &tstrtokr);
    }
    cmdp->hookmask = *vp;
    return retval;
}

NAMETAB bool_names[] =
{
    {(char *) "true", 1, 0, 0, 1},
    {(char *) "false", 1, 0, 0, 0},
    {(char *) "yes", 1, 0, 0, 1},
    {(char *) "no", 1, 0, 0, 0},
    {(char *) "1", 1, 0, 0, 1},
    {(char *) "0", 1, 0, 0, 0},
    {NULL, 0, 0, 0, 0}};

CF_HAND(cf_bool)
{
    *vp = search_nametab(GOD, bool_names, str);
    if (*vp == -1)
	*vp = 0;
    return 0;
}

CF_HAND(cf_who_bool)
{
  int prev, ret;
  DESC *d;

  prev = *vp;
  ret = cf_bool(vp, str, extra, extra2, player, cmd);
  if (prev && !ret) {
    DESC_ITER_CONN(d) {
      strncpy(d->doing,strip_ansi(d->doing),DOING_SIZE);
      *(d->doing + DOING_SIZE) = '\0';
    }
  }
  return ret;
}

/* ---------------------------------------------------------------------------
 * cf_option: Select one option from many choices.
 */

CF_HAND(cf_option)
{
    int i;

    i = search_nametab(GOD, (NAMETAB *) extra, str);
    if (i == -1) {
	cf_log_notfound(player, cmd, "Value", str);
	return -1;
    }
    *vp = i;
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_dynguest: Set dynamic guests then call dynstring function
 */
CF_HAND(cf_dynguest)
{
   DESC *d;
   char tplayer[6], playbuff[1001], *tplaybuff;
   char *inbufa, *inbufaptr;
   char *inbufb, *inbufbptr;
   char *tpr_buff, *tprp_buff, *tstrtokr;
   int noret_val, first, not;
   dbref lupp;

   if ( str == NULL || (strcmp(str, " ") == 0)) {
      if ( Good_obj(player) )
         notify(player, "Entry not changed.");
      return -1;
   }

   noret_val = 0;

   /* On initialization, lookup_player() doesn't work */
   if (mudstate.initializing) {
      memset(playbuff, 0, sizeof(playbuff));
      strncpy(playbuff, str, 1000);
      tplaybuff = strtok_r(playbuff, " \t", &tstrtokr);
      inbufaptr = inbufa = alloc_lbuf("dynguest.initialize");
      tpr_buff = tprp_buff = alloc_lbuf("dynguest.initialize.2");
      while (tplaybuff != NULL ) {
         first = 0;
         first = atoi(tplaybuff);
         /* We can only do minimal checking here because
          * No database is currently loaded to check against
          */
         if ( first <= 0 ) {
            noret_val = -1;
            break;
         }
         tprp_buff = tpr_buff;
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%s ", tplaybuff), inbufa, &inbufaptr);
         tplaybuff = strtok_r(NULL, " \t", &tstrtokr);
      }
      free_lbuf(tpr_buff);
      if ( noret_val == -1 ) {
         memset(playbuff, 0, sizeof(playbuff));
	 STARTLOG(LOG_STARTUP, "CNF", "DYNGUEST")
	 sprintf(playbuff, "Invalid player list '%.900s' for paramater '%s'.", str, cmd);
	 log_text(playbuff);
	 ENDLOG
      } else {
         noret_val = cf_dynstring(vp, inbufa, extra, extra2, player, cmd);
      }
      free_lbuf(inbufa);
      return noret_val;
   }

   DESC_ITER_CONN(d) {
      memset(tplayer, 0, sizeof(tplayer));
      strncpy(tplayer, Name(d->player), 5);
      if ( stricmp(tplayer, "guest") == 0 ) {
         noret_val = 1;
         break;
      }
      memset(playbuff, 0, sizeof(playbuff));
      strncpy(playbuff, mudconf.guest_namelist, 1000);
      tplaybuff = strtok_r(playbuff, " \t", &tstrtokr);
      while (tplaybuff != NULL ) {
         lupp = lookup_player(NOTHING, tplaybuff, 0);
         if ( lupp == d->player ) {
            noret_val = 1;
            break;
         }
         tplaybuff = strtok_r(NULL, " \t", &tstrtokr);
      }
      if (noret_val == 1)
         break;
      memset(playbuff, 0, sizeof(playbuff));
      strncpy(playbuff, str, 1000);
      tplaybuff = strtok_r(playbuff, " \t", &tstrtokr);
      while (tplaybuff != NULL ) {
         lupp = lookup_player(NOTHING, tplaybuff, 0);
         if ( (lupp < 0) && (*tplaybuff != '!') ) {
            noret_val = 4;
            break;
         }
         if ( lupp == d->player ) {
            noret_val = 2;
            break;
         }
         tplaybuff = strtok_r(NULL, " \t", &tstrtokr);
      }
      if ( noret_val > 0 )
         break;
   }
   if ( noret_val == 1 ) {
      notify_quiet(player, "Can not change value while pre-defined guests connected.");
      return (-1);
   } else if ( noret_val == 2 ) {
      notify_quiet(player, "Can not change value while a target player connected.");
      return (-1);
   } else if ( noret_val == 4 ) {
      notify_quiet(player, "Can not add non-players to the list.");
      return (-1);
   }
   /* Ok, now let's check for matching strings */
   inbufaptr = inbufa = alloc_lbuf("dynguest.1");
   memset(playbuff, 0, sizeof(playbuff));
   strncpy(playbuff, mudconf.guest_namelist, 1000);
   tplaybuff = strtok_r(playbuff, " \t", &tstrtokr);
   safe_chr(' ', inbufa, &inbufaptr);
   tpr_buff = tprp_buff = alloc_lbuf("dynguest.initialize.2");
   while ( tplaybuff != NULL ) {
      lupp = lookup_player(NOTHING, tplaybuff, 0);
      tprp_buff = tpr_buff;
      safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d ", lupp), inbufa, &inbufaptr);
      tplaybuff = strtok_r(NULL, " \t", &tstrtokr);
   }
   inbufbptr = inbufb = alloc_lbuf("dynguest.2");
   memset(playbuff, 0, sizeof(playbuff));
   strncpy(playbuff, str, 1000);
   tplaybuff = strtok_r(playbuff, " \t", &tstrtokr);
   first = 0;
   while ( tplaybuff != NULL ) {
      if (first)
         safe_chr(' ', inbufb, &inbufbptr);
      if ( *tplaybuff == '!' ) {
         lupp = lookup_player(NOTHING, tplaybuff+1, 0);
         not = 1;
      } else {
         lupp = lookup_player(NOTHING, tplaybuff, 0);
         not = 0;
      }
      if ( Good_obj(lupp) ) {
         tprp_buff = tpr_buff;
         if ( not ) {
            safe_str(safe_tprintf(tpr_buff, &tprp_buff, "!#%d", lupp), inbufb, &inbufbptr);
         } else
            safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d", lupp), inbufb, &inbufbptr);
      } else {
         safe_str(tplaybuff, inbufb, &inbufbptr);
         /* Verification on startup can't be done.  Player hash not loaded */
         if ( !not ) {
            noret_val = 2;
            break;
         }
      }
      tprp_buff = tpr_buff;
      if ( (strstr(inbufa, safe_tprintf(tpr_buff, &tprp_buff, " %d ", lupp)) != NULL) && !not ) {
         noret_val = 3;
         break;
      }
      tplaybuff = strtok_r(NULL, " \t", &tstrtokr);
      first = 1;
   }
   free_lbuf(tpr_buff);
   free_lbuf(inbufa);
   if ( noret_val == 3 ) {
      notify_quiet(player, "Can not add duplicated entry.");
      free_lbuf(inbufb);
      return (-1);
   } else if (noret_val == 2) {
      notify_quiet(player, "Can not add non-players to the list.");
      free_lbuf(inbufb);
      return (-1);
   } else {
        noret_val = cf_dynstring(vp, inbufb, extra, extra2, player, cmd);
        free_lbuf(inbufb);
        return noret_val;
   }
}

/* ---------------------------------------------------------------------------
 * cf_dynstring: Set dynamic growth string parameter.
 */
CF_HAND(cf_dynstring)
{
   int retval, chkval, addval, first, second, third, i_skip;
   char *buff, *tbuff, *buff2, *tbuff2, *stkbuff, *abuf1, *abuf2, *abuf3, *tabuf2, *tabuf3;
   char quick_buff[LBUF_SIZE+2], *tstrtokr, *tstval, *tstvalptr, *tstvalstr, *tstvalstrptr, *dup, *dupptr;

   if ( str == NULL || !*str ) {
      if ( Good_obj(player) )
         notify(player, "Entry not changed.");
      return -1;
   }

   chkval = retval = addval = 0;
   if ( strcmp( str, "!ALL" ) == 0 ) {
      if ( Good_obj(player) )
         notify(player, "Entry purged.");
      strcpy((char *) vp, "");
      chkval = 1;
   } else {
      abuf1 = alloc_lbuf("cf_dynstring.BUF1");
      tabuf2 = abuf2 = alloc_lbuf("cf_dynstring.BUF2");
      tabuf3 = abuf3 = alloc_lbuf("cf_dynstring.BUF3");
      strcpy(abuf1, str);
      stkbuff = strtok_r(abuf1, " ", &tstrtokr);
      second = 0;
      third = 0;
      addval = 0;
      i_skip = 0;
      if ( extra2 == 1 ) {
         tstvalptr = tstval = alloc_lbuf("cf_dynstring_checkfordupes");
         dupptr = dup = alloc_lbuf("cf_dynstring_dupliates");
      }
      while (stkbuff) {
         if (*stkbuff == '!') {
            if ( second )
               safe_chr(' ', abuf2, &tabuf2);
            safe_str(stkbuff+1, abuf2, &tabuf2);
            second = 1;
         } else {
            if ( extra2 == 1 ) {
               memset(tstval, '\0', LBUF_SIZE);
               tstvalptr = tstval;
               safe_str((char *)vp, tstval, &tstvalptr);
               safe_chr(' ', tstval, &tstvalptr);
               safe_str(abuf3, tstval, &tstvalptr);
               tstvalstr = strtok_r(tstval, (char *)" ", &tstvalstrptr);
               while ( tstvalstr ) {
                  if ( stricmp(tstvalstr, stkbuff) == 0 ) {
                     i_skip = 1;
                     if ( !dup && !*dup ) {
                        safe_str(stkbuff, dup, &dupptr);
                     } else {
                        safe_chr(' ', dup, &dupptr);
                        safe_str(stkbuff, dup, &dupptr);
                     }
                     break;
                  }
                  tstvalstr = strtok_r(NULL, (char *)" ", &tstvalstrptr);
               }
            }
            if ( !i_skip ) {
               if ( third )
                  safe_chr(' ', abuf3, &tabuf3);
               safe_str(stkbuff, abuf3, &tabuf3);
               third = 1;
               addval++;
            }
         }
         stkbuff = strtok_r(NULL," ", &tstrtokr);
         i_skip = 0;
      }
      if ( extra2 == 1 ) {
         free_lbuf(tstval);
      }
      strcpy(abuf1, (char *) vp);
      stkbuff = strtok_r(abuf1, " ", &tstrtokr);
      chkval = 0;
      first = 0;
      tbuff2 = buff2 = alloc_lbuf("cf_dynstring.ALLOC2");
      while (stkbuff) {
         /* This is always 2 chars above max length stkbuff can be */
         memset(quick_buff, 0, sizeof(quick_buff));
         sprintf(quick_buff, "*%s*", stkbuff);
         if (!quick_wild(quick_buff,abuf2)) {
            if ( first )
               safe_chr(' ', buff2, &tbuff2);
            safe_str(stkbuff, buff2, &tbuff2);
            first = 1;
         } else {
            chkval++;
         }
         stkbuff = strtok_r(NULL," ", &tstrtokr);
      }
      free_lbuf(abuf2);
      tbuff = buff = alloc_lbuf("cf_dynstring.ALLOC");
      safe_str(buff2, buff, &tbuff);
      if ( buff2 )
         safe_chr(' ', buff, &tbuff);
      free_lbuf(buff2);
      stkbuff = strtok_r(abuf3, " ", &tstrtokr);
      first = 0;
      second = 0;
      while ( stkbuff ) {
         if ( (strlen(buff) + strlen(stkbuff) + 1) < extra ) {
            safe_str(stkbuff, buff, &tbuff);
            safe_chr(' ', buff, &tbuff);
            first++;
         }
         second++;
         stkbuff = strtok_r(NULL, " ", &tstrtokr);
      }
      free_lbuf(abuf3);
      strncpy((char *) vp, buff, (extra - 1));
      free_lbuf(abuf1);
      free_lbuf(buff);
      if ( first != second ) {
         if ( chkval && first ) {
            if ( Good_obj(player) )
               notify(player, unsafe_tprintf("String exceeded maximum length.  %d removed, %d added, %d ignored.",
                      chkval, first, second - first));
         } else if ( chkval ) {
            if ( Good_obj(player) )
               notify(player, "String exceeded maximum length.");
         } else {
            if ( Good_obj(player) )
               notify(player, unsafe_tprintf("String exceeded maximum length.  %d added, %d ignored.",
                   first, second - first));
         }
	 STARTLOG(LOG_STARTUP, "CNF", "NFND")
            buff = alloc_lbuf("cf_string.LOG");
	    sprintf(buff, "%s: String buffer exceeded - truncated", cmd);
	    log_text(buff);
	    free_lbuf(buff);
	 ENDLOG
         retval = 1;
      } else {
         if ( !chkval && !addval ) {
            if ( Good_obj(player) )
               notify(player, "Entry not changed.");
         } else {
            if ( chkval && first ) {
               if ( Good_obj(player) )
                  notify(player, unsafe_tprintf("Entry updated.  %d removed, %d added.", chkval, first));
            } else if ( chkval ) {
               if ( Good_obj(player) )
                  notify(player, unsafe_tprintf("Entry updated.  %d removed.", chkval));
            } else {
               if ( Good_obj(player) )
                  notify(player, unsafe_tprintf("Entry updated.  %d added.", first));
            }
         }
         if ( (extra2 == 1) && Good_obj(player) ) {
            if ( dup && *dup ) {
               notify(player, unsafe_tprintf("Duplicate entries ignored: %.*s", LBUF_SIZE - 100, dup));
            }
         }
      }
      if ( extra2 == 1 ) {
         free_lbuf(dup);
      }
   }
   if ( !chkval && !addval  )
      retval = -1;
   return retval;
}

/* ---------------------------------------------------------------------------
 * cf_attriblock
 */
ATRP *atrp_head = NULL;

int
atrpEval(int key, char *s_attr, dbref player, dbref target, int i_type)
{
   int aflags, result;
   dbref aowner;
   char *retval, *atext, *mybuff[4];
   ATTR *ap;

   if ( key != 8 ) {
      return 0;
   }

   if ( !Good_chk(mudconf.hook_obj) || mudstate.insideaflags ) {
      return 1;
   }

   ap = atr_str_atrpeval((char *)"AFLAG_EVAL");
   if ( !ap ) {
      return 1;
   }

   atext = atr_pget(mudconf.hook_obj, ap->number, &aowner, &aflags);

   if ( !atext ) {
      return 1;
   }


   mybuff[0] = alloc_sbuf("atrpEval");
   mybuff[1] = alloc_sbuf("atrpEval2");
   mybuff[2] = alloc_sbuf("atrpEval3");
   mybuff[3] = NULL;
   strncpy(mybuff[0], s_attr, SBUF_SIZE);
   sprintf(mybuff[1], "#%d", target);
   sprintf(mybuff[2], "%d", i_type);
   mudstate.insideaflags = 1;
   retval = cpuexec(player, player, player, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atext, mybuff, 3, (char **)NULL, 0);
   mudstate.insideaflags = 0;
   free_sbuf(mybuff[0]);
   free_sbuf(mybuff[1]);
   free_sbuf(mybuff[2]);
   free_lbuf(atext);
   if ( *retval ) {
      result = atoi(retval);
   } else {
      result = 1;
   }
   free_lbuf(retval);
   return (result ? 1 : 0);
}

int
attrib_cansee(dbref player, const char *name, dbref owner, dbref target)
{
   ATRP *atrp;
   dbref i_player;

   i_player = player;
   if ( Typeof(player) != TYPE_PLAYER )
      i_player = Owner(player);

   for (atrp = atrp_head; atrp; atrp = atrp->next) {
      if ( ((atrp->owner == -1) || (atrp->owner == owner)) && 
           (((atrp->enactor == -1) || (atrp->enactor == i_player)) ||
            ((atrp->enactor == -1) || (atrp->enactor == player))) &&
           ((atrp->target == -1) || (atrp->target == target)) ) {
         if ( pstricmp((char *)name, atrp->name, strlen(atrp->name)) == 0 ) {
            if ( (God(player) && atrpGod(atrp->flag_see)) ||
                 (Immortal(player) && atrpImm(atrp->flag_see)) ||
                 (Wizard(player) && atrpWiz(atrp->flag_see)) ||
                 (Admin(player) && atrpCounc(atrp->flag_see)) ||
                 (Builder(player) && atrpArch(atrp->flag_see)) ||
                 (Guildmaster(player) && atrpGuild(atrp->flag_see)) ||
                 (!(Wanderer(player) || Guest(player)) && atrpPreReg(atrp->flag_see)) ||
                 ((HasPriv(player, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && ExFullWizAttr(player)) && atrvWiz(atrp->flag_see)) ||
                  atrpEval(atrp->flag_see, (char *)name, player, target, 0) ||
                  atrpCit(atrp->flag_see) ) {
               return 1;
            }
            return 0;
         }
      }
   }
   return 1;
}

int
attrib_canset(dbref player, const char *name, dbref owner, dbref target)
{
   ATRP *atrp;
   dbref i_player;

   i_player = player;
   if ( Typeof(player) != TYPE_PLAYER )
      i_player = Owner(player);

   for (atrp = atrp_head; atrp; atrp = atrp->next) {
      if ( ((atrp->owner == -1) || (atrp->owner == owner)) && 
           (((atrp->enactor == -1) || (atrp->enactor == i_player)) ||
            ((atrp->enactor == -1) || (atrp->enactor == player))) &&
           ((atrp->target == -1) || (atrp->target == target)) ) {
         if ( pstricmp((char *)name, atrp->name, strlen(atrp->name)) == 0 ) {
            if ( (God(player) && atrpGod(atrp->flag_set)) ||
                 (Immortal(player) && atrpImm(atrp->flag_set)) ||
                 (Wizard(player) && atrpWiz(atrp->flag_set)) ||
                 (Admin(player) && atrpCounc(atrp->flag_set)) ||
                 (Builder(player) && atrpArch(atrp->flag_set)) ||
                 (Guildmaster(player) && atrpGuild(atrp->flag_set)) ||
                 (!(Wanderer(player) || Guest(player)) && atrpPreReg(atrp->flag_set)) ||
                  atrpEval(atrp->flag_set, (char *)name, player, target, 1) ||
                  atrpCit(atrp->flag_set) ) {
               return 1;
            }
            return 0;
         }
      }
   }
   return 1;
}

char *
attrib_show(char *name, int i_type)
{
   ATRP *atrp;
   char *n_perms[]={"PreReg", "citizen" , "guildmaster", "architect", "councilor", "wizard", "immortal", "god", "Eval", "Error"};
   char *s_buff, *s_my, *s_myptr;

   s_buff = alloc_lbuf("attrib_show");
   s_myptr = s_my = alloc_lbuf("attrib_show2");
   if ( name && *name ) {
      if ( i_type ) {
         for (atrp = atrp_head; atrp; atrp = atrp->next) {
            if ( (atrp->owner != -1) || (atrp->target != -1) || (atrp->enactor != -1) ) {
               if ( pstricmp(name, atrp->name, strlen(atrp->name)) == 0 ) {
                  sprintf(s_buff, "\r\n   ---+ Owner: #%-8d  Target: #%-8d  Enactor: #%-8d CanSee: %-s, CanSet: %-s", 
                                  atrp->owner, atrp->target, atrp->enactor, n_perms[atrp->flag_see], n_perms[atrp->flag_set]);
                  safe_str(s_buff, s_my, &s_myptr);
               }
            }
         }
      } else {
         for (atrp = atrp_head; atrp; atrp = atrp->next) {
            if ( (atrp->owner != -1) || (atrp->target != -1) || (atrp->enactor != -1) )
               continue;
            if ( pstricmp(name, atrp->name, strlen(atrp->name)) == 0 ) {
               sprintf(s_buff, "{CanSee: %-s, CanSet: %-s}", n_perms[atrp->flag_see], n_perms[atrp->flag_set]);
               break;
            }
         }
         strcpy(s_my, s_buff);
      }
   }
   free_lbuf(s_buff);
   return s_my;
}

void
add_perms(dbref player, char *s_input, char *s_output, char **cargs, int ncargs)
{
   ATRP *atrp, *atrp2;
   char *t_strtok, *t_strtokptr, *s_chr;
   int i_owner, i_target, i_enactor, i_see, i_set, i_atrperms_cnt;

   if ( !*s_input || !*s_output ) {
      notify(player, "Require format: @aperms/add prefix=<set> <see> [<owner> <target> <enactor>]");
      return;
   }

   s_chr = s_output;
   i_set = 0;
   while ( s_chr && *s_chr ) {
      if ( isspace(*s_chr) ) {
         i_set++;
      }
      s_chr++;
   }
   if ( (i_set < 1) || (i_set > 4) ) {
      notify(player, "Require format: @aperms/add prefix=<set> <see> [<owner> <target> <enactor>]");
      return;
   }

   i_atrperms_cnt = 0;
   if ( atrp_head ) {
      for (atrp = atrp_head; atrp; atrp = atrp->next)
         i_atrperms_cnt++;
   }

   if ( i_atrperms_cnt >= mudconf.atrperms_max ) {
      notify(player, "Ceiling reached on attribute prefix masking.");
      return;
   }

   i_see = i_set = i_owner = i_target = i_enactor = -1; 
   t_strtok = strtok_r(s_output, " \t", &t_strtokptr);
   if ( t_strtok ) {
      i_see = atoi(t_strtok);
      t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
      if ( t_strtok ) {
         i_set = atoi(t_strtok);
         t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
         if ( t_strtok ) {
            if ( *t_strtok == '#' )
               i_owner = atoi(t_strtok+1);
            else
               i_owner = atoi(t_strtok);
            t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
            if ( t_strtok ) {
               if ( *t_strtok == '#' )
                  i_target = atoi(t_strtok+1);
               else
                  i_target = atoi(t_strtok);
               t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
               if ( t_strtok ) {
                  if ( *t_strtok == '#' )
                     i_enactor = atoi(t_strtok+1);
                  else
                     i_enactor = atoi(t_strtok);
               }
            }
         }
      }
   }
   s_chr = s_input;
   while ( s_chr && *s_chr ) {
      *s_chr = ToUpper(*s_chr);
      if ( isspace(*s_chr) ) {
         notify(player, "Invalid character in prefix.  Non-space string required.");
         return;
      }
      s_chr++;
   }
   for ( atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
      if ( (strcmp(atrp2->name, s_input) == 0) &&
           (((i_owner == atrp2->owner) &&
             (i_target == atrp2->target)) &&
            (i_enactor == atrp2->enactor)) ) {
         notify(player, "Entry already in prefix list.  Use @aflags/mod to modify it.");
         return;
      }
   }
   notify(player, unsafe_tprintf("Entry added [%d of %d used].", i_atrperms_cnt, mudconf.atrperms_max));
   atrp  = (ATRP *) malloc(sizeof(ATRP));
   atrp->name = alloc_sbuf("attribute_perm_array");
   memset(atrp->name, '\0', SBUF_SIZE);
   strncpy(atrp->name, s_input, SBUF_SIZE - 2);
   atrp->next = NULL;
   if ( (i_set < 0) || (i_set > 8) )
      i_set = 1;

   if ( (i_see < 0) || (i_see > 8) )
      i_see = 1;

   atrp->flag_set = i_set;
   atrp->flag_see = i_see;
   atrp->owner = i_owner;
   atrp->target = i_target;
   atrp->controller = player;
   atrp->enactor = i_enactor;

   if ( atrp_head ) {
      for (atrp2 = atrp_head; atrp2->next; atrp2 = atrp2->next);
      atrp2->next = atrp;
   } else {
      atrp_head = atrp;
   }
}

void
del_perms(dbref player, char *s_input, char *s_output, char **cargs, int ncargs)
{
   ATRP *atrp, *atrp2;
   char *t_strtok, *t_strtokptr, *s_chr;
   int i_owner, i_target, i_enactor;

   if ( !*s_input ) {
      notify(player, "Require format: @aperms/del prefix [=<owner> <target> <enactor>]");
      return;
   }

   s_chr = s_output;
   i_owner = 0;
   while ( s_chr && *s_chr ) {
      if ( isspace(*s_chr) ) {
         i_owner++;
      }
      s_chr++;
   }
   if ( i_owner > 2 ) {
      notify(player, "Require format: @aperms/del prefix [=<owner> <target> <enactor>]");
      return;
   }

   i_owner = i_target = i_enactor = -1; 
   if ( s_output && *s_output) {
      t_strtok = strtok_r(s_output, " \t", &t_strtokptr);
      if ( t_strtok ) {
         if ( *t_strtok == '#' )
            i_owner = atoi(t_strtok+1);
         else
            i_owner = atoi(t_strtok);
         t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
         if ( t_strtok ) {
            if ( *t_strtok == '#' )
               i_target = atoi(t_strtok+1);
            else
               i_target = atoi(t_strtok);
            t_strtok = strtok_r(NULL, " \t", &t_strtokptr);
            if ( t_strtok ) {
               if ( *t_strtok == '#' )
                  i_enactor = atoi(t_strtok+1);
               else
                  i_enactor = atoi(t_strtok);
            }
         }
      }
   }
   s_chr = s_input;
   while ( s_chr && *s_chr ) {
      *s_chr = ToUpper(*s_chr);
      if ( isspace(*s_chr) ) {
         notify(player, "Invalid character in prefix.  Non-space string required.");
         return;
      }
      s_chr++;
   }
   atrp = atrp_head;
   for ( atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
      if ( (strcmp(atrp2->name, s_input) == 0) &&
           (i_owner == atrp2->owner) &&
           (i_enactor == atrp2->enactor) &&
           (i_target == atrp2->target) ) {
         if ( (strcmp(atrp_head->name, s_input) == 0) &&
              (i_owner == atrp2->owner) &&
              (i_target == atrp2->target) ) {
            atrp_head = atrp_head->next;
         } else {
            atrp->next = atrp2->next;
         }
         notify(player, "Entry has been deleted.");
         free_sbuf(atrp2->name);
         atrp2->name = NULL;
         free(atrp2);
         atrp2 = NULL;
         return;
      }
      atrp = atrp2;
   }
   notify(player, "Entry not found.");
}

void
mod_perms(dbref player, char *s_input, char *s_output, char **cargs, int ncargs)
{
   ATRP *atrp2;
   char *t_strtok, *t_strtok2, *t_strtokptr, *s_chr, *s_strtok, *s_strtokptr;
   int i_owner, i_target, i_enactor, i_newowner, i_newtarget, i_newenactor, i_newset, i_newsee;

   if ( !*s_input || !*s_output ) {
      notify(player, "Require format: @aperms/mod prefix [<owner> <target> <enactor>]=<set perm> <see perm> [<owner> <target> <enactor>]");
      return;
   }

   s_chr = s_input;
   i_owner = 0;
   while ( s_chr && *s_chr ) {
      if ( isspace(*s_chr) ) {
         i_owner++;
      }
      s_chr++;
   }
   if ( i_owner > 3 ) {
      notify(player, "Require format: @aperms/mod prefix [<owner> <target> <enactor>]=<set perm> <see perm> [<owner> <target> <enactor>]");
      return;
   }
   s_chr = s_output;
   i_owner = 0;
   while ( s_chr && *s_chr ) {
      if ( isspace(*s_chr) ) {
         i_owner++;
      }
      s_chr++;
   }
   if ( i_owner > 4 ) {
      notify(player, "Require format: @aperms/mod prefix [<owner> <target> <enactor>]=<set perm> <see perm> [<owner> <target> <enactor>]");
      return;
   }
   i_owner = i_target = i_enactor = i_newowner = i_newtarget = i_newenactor = i_newset = i_newsee = -1; 
   t_strtok = strtok_r(s_input, " \t", &t_strtokptr);
   if ( t_strtok ) {
      t_strtok2 = strtok_r(NULL, " \t", &t_strtokptr);
      if ( t_strtok2 ) {
         if ( *t_strtok2 == '#' )
            i_owner = atoi(t_strtok2+1);
         else
            i_owner = atoi(t_strtok2);
         t_strtok2 = strtok_r(NULL, " \t", &t_strtokptr);
         if ( t_strtok2 ) {
            if ( *t_strtok2 == '#' )
               i_target = atoi(t_strtok2+1);
            else
               i_target = atoi(t_strtok2);
            t_strtok2 = strtok_r(NULL, " \t", &t_strtokptr);
            if ( t_strtok2 ) {
               if ( *t_strtok2 == '#' )
                  i_enactor = atoi(t_strtok2+1);
               else
                  i_enactor = atoi(t_strtok2);
            }
         }
      }
   }
   i_newowner = i_owner;
   i_newtarget = i_target;
   i_newenactor = i_enactor;
   s_strtok = strtok_r(s_output, " \t", &s_strtokptr);
   if ( s_strtok ) {
      i_newset = atoi(s_strtok);
      s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
      if ( s_strtok ) {
         i_newsee = atoi(s_strtok);
         s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
         if ( s_strtok ) {
            if ( *s_strtok == '#' )
               i_newowner = atoi(s_strtok+1);
            else
               i_newowner = atoi(s_strtok);
            s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
            if ( s_strtok ) {
               if ( *s_strtok == '#' )
                  i_newtarget = atoi(s_strtok+1);
               else
                  i_newtarget = atoi(s_strtok);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               if ( s_strtok ) {
                  if ( *s_strtok == '#' )
                     i_newenactor = atoi(s_strtok+1);
                  else
                     i_newenactor = atoi(s_strtok);
               }
            }
         }
      }
   }
   if ( (i_newset < 0) || (i_newset > 8) )
      i_newset = 1;
   if ( (i_newsee < 0) || (i_newsee > 8) )
      i_newsee = 1;
   s_chr = t_strtok;
   while ( s_chr && *s_chr ) {
      *s_chr = ToUpper(*s_chr);
      if ( isspace(*s_chr) ) {
         notify(player, "Invalid character in prefix.  Non-space string required.");
         return;
      }
      s_chr++;
   }
   for ( atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
      if ( (strcmp(atrp2->name, t_strtok) == 0) &&
           ((i_newowner == atrp2->owner) &&
            (i_newenactor == atrp2->enactor) &&
            (i_newtarget == atrp2->target)) &&
           ((i_newowner != i_owner) &&
            (i_newtarget != i_target) &&
            (i_newenactor != i_enactor)) ) {
         notify(player, "Matching destination attribute with matching owner/target.");
         return;
      }
   }
   for ( atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
      if ( (strcmp(atrp2->name, t_strtok) == 0) &&
           (i_owner == atrp2->owner) &&
           (i_target == atrp2->target) ) {
         notify(player, "Entry has been modified.");
         atrp2->owner = i_newowner;
         atrp2->target = i_newtarget;
         atrp2->enactor = i_newenactor;
         atrp2->controller = player;
         atrp2->flag_set = i_newset;
         atrp2->flag_see = i_newsee;
         return;
      }
   }
   notify(player, "Entry not found.");
}

void 
display_perms(dbref player, int i_page, int i_key, char *fname)
{
    char *n_perms[]={"PreReg", "Cit", "Guild", "Arch", "Counc", "Wiz", "Imm", "God", "Eval", "Error"};
    char *tprbuff, *tprpbuff;
    int i_cnt, i_pagecnt;
    ATRP *atrp;

    tprpbuff = tprbuff = alloc_lbuf("display_perm");
    notify(player, "------------------------------------------------------------------------------");
    notify(player, safe_tprintf(tprbuff, &tprpbuff, "%-64s %-7s %-7s", (char *)"Attribute Prefix", (char *)"Set", (char *)"See"));
    notify(player, "------------------------------------------------------------------------------");
    i_cnt = i_pagecnt = 0;
    if ( atrp_head ) {
       for ( atrp = atrp_head; atrp; atrp = atrp->next ) {
          i_cnt++;
          i_pagecnt++;
          if ( (atrp->owner != -1) || (atrp->target != -1) || (atrp->enactor != -1) )
             i_pagecnt++;
          if ( (i_page != 0) && (i_pagecnt < ((i_page - 1) * 20)) )
             continue;
          if ( (i_page != 0) && (i_pagecnt >= (i_page * 20)) )
             continue;
          if ( (i_page == 0) && i_key && fname && *fname && !quick_wild(fname, atrp->name))
             continue;
          tprpbuff = tprbuff;
          notify(player, safe_tprintf(tprbuff, &tprpbuff, "%-64s %-7s %-7s", atrp->name, n_perms[atrp->flag_set], n_perms[atrp->flag_see]));
          if ( (atrp->owner != -1) || (atrp->target != -1) || (atrp->enactor != -1) ) {
             tprpbuff = tprbuff;
             notify(player, safe_tprintf(tprbuff, &tprpbuff, "   +----- Owner: #%d,  Object: #%d,  Enactor: #%d,  Modifier: #%d", 
                                         atrp->owner, atrp->target, atrp->enactor, atrp->controller));
          }
       }
    }
    tprpbuff = tprbuff;
    if ( i_page != 0 )
       notify(player, safe_tprintf(tprbuff, &tprpbuff, 
                                   "----------------------------[%6d/%6d max]---------- Page %-3d of %-3d ----", 
                                   i_cnt, ((mudconf.atrperms_max > 10000) ? 10000 : mudconf.atrperms_max), i_page, (i_pagecnt / 20) + 1));
    else
       notify(player, safe_tprintf(tprbuff, &tprpbuff, 
                                   "----------------------------[%6d/%6d max]-------------------------------", 
                                   i_cnt, ((mudconf.atrperms_max > 10000) ? 10000 : mudconf.atrperms_max)));
    notify(player, "Note: Immortals are treated as god with regards to seeing attributes.");
    free_lbuf(tprbuff);
}

CF_HAND(cf_atrperms)
{
   int retval, i_del, i_see, i_set, first1, first2, first3, i_atrperms_cnt, i_warn, i_owner, i_target, i_enactor;
   char *s_strtok, *s_strtokptr, *t_strtok, *t_strtokptr, *t_strtokbuf, *s_chr, *s_buff;
   char *sbuff1, *sbuff2, *sbuff3, *sbuff1ptr, *sbuff2ptr, *sbuff3ptr;
   /* Guildmaster, Architect, Councilor, Wizard, Immortal, #1 */
   ATRP *atrp, *atrp2;

   if ( mudconf.atrperms_max < 0 )
      mudconf.atrperms_max = 0;
   if ( mudconf.atrperms_max > 10000 )
      mudconf.atrperms_max = 10000;

   /* Let's count the total attrib prefix masks */
   i_atrperms_cnt = i_del = i_warn = 0;
   if ( atrp_head ) {
      for (atrp = atrp_head; atrp; atrp = atrp->next)
         i_atrperms_cnt++;
   }

   retval = i_set = i_see = first1 = first2 = first3 = 0;
   i_owner = i_target = i_enactor = -1;
   s_strtokptr = strtok_r(str, "\t ", &s_strtok);
   s_buff = alloc_lbuf("cf_atrperms");
   sbuff1ptr = sbuff1 = alloc_lbuf("cf_atrperms2");
   sbuff2ptr = sbuff2 = alloc_lbuf("cf_atrperms3");
   sbuff3ptr = sbuff3 = alloc_lbuf("cf_atrperms3");
   while ( s_strtokptr ) {
      i_owner = i_target = -1;
      memset(s_buff, '\0', LBUF_SIZE);
      strcpy(s_buff, s_strtokptr);
      t_strtokptr = strtok_r(s_buff, ":", &t_strtok);
      if ( t_strtokptr && *t_strtokptr ) {
         s_chr = t_strtokptr;
         while ( s_chr && *s_chr ) {
            *s_chr = ToUpper(*s_chr);
            s_chr++;
         }
         if ( *t_strtokptr == '!' ) {
            atrp = atrp_head;
            for (atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
               t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
               if ( t_strtokbuf ) {
                  i_owner = atoi(t_strtokbuf);
                  t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                  if ( t_strtokbuf ) {
                     i_target = atoi(t_strtokbuf);
                     t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                     if ( t_strtokbuf ) {
                        i_enactor = atoi(t_strtokbuf);
                     }
                  }
               }
               if ( ((i_owner == -1) || (i_owner == atrp2->owner)) &&
                    ((i_enactor == -1) || (i_enactor == atrp2->enactor)) &&
                    ((i_target == -1) || (i_target == atrp2->target)) &&
                    (strcmp(atrp2->name, t_strtokptr+1) == 0) ) {
                  if ( strcmp(t_strtokptr+1, atrp_head->name) == 0) {
                     atrp_head = atrp_head->next;
                  } else {
                     atrp->next = atrp2->next;
                  }
                  if ( first3 )
                     safe_chr(' ', sbuff3, &sbuff3ptr);
                  first3 = 1;
                  safe_str(atrp2->name, sbuff3, &sbuff3ptr);
                  free_sbuf(atrp2->name);
                  atrp2->name = NULL;
                  free(atrp2);
                  atrp2 = NULL;
                  retval = 1;
                  i_del++;
                  break;
               }
               atrp = atrp2;
            }
         } else {
            if ( atrp_head ) {
               t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
               if ( t_strtokbuf ) {
                  i_set = atoi(t_strtokbuf);
                  t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                  if ( t_strtokbuf ) {
                     i_see = atoi(t_strtokbuf);
                     t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                     if ( t_strtokbuf ) {
                        i_owner = atoi(t_strtokbuf);
                        t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                        if ( t_strtokbuf ) {
                           i_target = atoi(t_strtokbuf);
                           t_strtokbuf = strtok_r(NULL, ":", &t_strtok);
                           if ( t_strtokbuf ) {
                              i_enactor = atoi(t_strtokbuf);
                           }
                        }
                     }
                  }
               }
               atrp = atrp_head;
               for (atrp2 = atrp_head; atrp2; atrp2 = atrp2->next) {
                  if ( ((i_owner == -1) || (i_owner == atrp2->owner)) &&
                       ((i_enactor == -1) || (i_enactor == atrp2->enactor)) &&
                       ((i_target == -1) || (i_target == atrp2->target)) &&
                       (stricmp(atrp2->name, t_strtokptr) == 0) ) {
                     if ( (i_set < 0) || (i_set > 8) )
                        i_set = 1;
         
                     if ( (i_see < 0) || (i_see > 8) )
                        i_see = 1;

                     atrp2->flag_set = i_set;
                     atrp2->flag_see = i_see;
                     atrp2->owner = i_owner;
                     atrp2->target = i_target;
                     atrp2->enactor = i_enactor;
                     if ( (i_owner != -1) || (i_target != -1) ) {
                        if ( mudstate.initializing ) 
                           atrp2->controller = 1;
                        else
                           atrp2->controller = player;
                     } else
                        atrp2->controller = -1;
                     retval = 1;
                     if ( first2 )
                        safe_chr(' ', sbuff2, &sbuff2ptr);
                     first2 = 1;
                     safe_str(atrp2->name, sbuff2, &sbuff2ptr);
                        break;
                  }
               }
               atrp_head = atrp;
               if ( retval == 1 ) {
                  retval = 0;
                  s_strtokptr = strtok_r(NULL, "\t ", &s_strtok);
                  continue;
               }
            }
            if ( i_atrperms_cnt >= mudconf.atrperms_max ) {
               if ( Good_obj(player) ) {
                  if ( !i_warn ) 
                     notify(player, "Ceiling reached on attribute prefix masking.");
                  i_warn = 1;
               }
               s_strtokptr = strtok_r(NULL, "\t ", &s_strtok);
               continue;
            }
            i_atrperms_cnt++;
            atrp  = (ATRP *) malloc(sizeof(ATRP));       
            atrp->name = alloc_sbuf("attribute_perm_array");
            memset(atrp->name, '\0', SBUF_SIZE);
            strncpy(atrp->name, t_strtokptr, SBUF_SIZE - 2);
            atrp->next = NULL;
            t_strtokptr = strtok_r(NULL, ":", &t_strtok);
            if ( t_strtokptr ) {
               i_set = atoi(t_strtokptr);
               t_strtokptr = strtok_r(NULL, ":", &t_strtok);
               if ( t_strtokptr ) {
                  i_see = atoi(t_strtokptr);
                  t_strtokptr = strtok_r(NULL, ":", &t_strtok);
                  if ( t_strtokptr ) {
                     i_owner = atoi(t_strtokptr);
                     t_strtokptr = strtok_r(NULL, ":", &t_strtok);
                     if ( t_strtokptr ) {
                        i_target = atoi(t_strtokptr);
                        t_strtokptr = strtok_r(NULL, ":", &t_strtok);
                        if ( t_strtokptr ) {
                           i_enactor = atoi(t_strtokptr);
                        }
                     }
                  }
               }
            }
            if ( (i_set < 0) || (i_set > 8) )
               i_set = 1;

            if ( (i_see < 0) || (i_see > 8) )
               i_see = 1;

            atrp->flag_set = i_set;
            atrp->flag_see = i_see;
            atrp->owner = i_owner;
            atrp->target = i_target;
            atrp->enactor = i_enactor;
            if ( (i_target != -1) || (i_owner != -1) ) {
               if ( mudstate.initializing ) 
                  atrp->controller = 1;
               else
                  atrp->controller = player;
            } else
               atrp->controller = -1;
            if (!atrp_head) {
               atrp_head = atrp;
            } else {
               for (atrp2 = atrp_head; atrp2->next; atrp2 = atrp2->next);
               atrp2->next = atrp;
            }
            if ( atrp && atrp->name ) {
               if ( first1 )
                  safe_chr(' ', sbuff1, &sbuff1ptr);
               first1 = 1;
               safe_str(atrp->name, sbuff1, &sbuff1ptr);
            }
            retval = 5;
         }
      }
      s_strtokptr = strtok_r(NULL, "\t ", &s_strtok);
   }
   free_lbuf(s_buff);
   first1 = 0;
   if ( Good_chk(player) && *sbuff1 ) {
      notify(player, unsafe_tprintf("Added.....: %s", sbuff1));
      first1++;
   }
   if ( Good_chk(player) && *sbuff2 ) {
      notify(player, unsafe_tprintf("Modified..: %s", sbuff2));
      first1++;
   }
   if ( Good_chk(player) && *sbuff3 ) {
      notify(player, unsafe_tprintf("deleted...: %s", sbuff3));
      first1++;
   }
   if ( !first1 ) {
      notify(player, "Unchanged.");
   }
   if ( Good_chk(player) ) {
      if ( i_atrperms_cnt < mudconf.atrperms_max ) {
         notify(player, unsafe_tprintf("You can add %d more prefixes [%d total]", mudconf.atrperms_max - i_atrperms_cnt + i_del, mudconf.atrperms_max));
      } else {
         notify(player, unsafe_tprintf("You are at the maximum number of prefixes [%d].", mudconf.atrperms_max));
      }
   }
   free_lbuf(sbuff1);
   free_lbuf(sbuff2);
   free_lbuf(sbuff3);
   if ( retval == 5 )
      retval = 0;
   return retval;
}

/* ---------------------------------------------------------------------------
 * cf_string: Set string parameter.
 */

CF_HAND(cf_string_chr)
{
    int retval;
    char *buff;

    /* Copy the string to the buffer if it is not too big */

    retval = 0;
    if (strlen(str) >= extra) {
	str[extra - 1] = '\0';
	if (mudstate.initializing) {
	    STARTLOG(LOG_STARTUP, "CNF", "NFND")
		buff = alloc_lbuf("cf_string.LOG");
	    sprintf(buff, "%.3900s: String truncated", cmd);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    notify(player, "String truncated");
	}
	retval = 1;
    }
    if ( *str ) 
       strcpy((char *) vp, str);
    else
       strcpy((char *) vp, (char *)"`");

    return retval;
}

CF_HAND(cf_string)
{
    int retval;
    char *buff;

    /* Copy the string to the buffer if it is not too big */

    retval = 0;
    if (strlen(str) >= extra) {
	str[extra - 1] = '\0';
	if (mudstate.initializing) {
	    STARTLOG(LOG_STARTUP, "CNF", "NFND")
		buff = alloc_lbuf("cf_string.LOG");
	    sprintf(buff, "%.3900s: String truncated", cmd);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    notify(player, "String truncated");
	}
	retval = 1;
    }
    strcpy((char *) vp, str);
    return retval;
}

CF_HAND(cf_string_sub)
{
    int retval;
    char *buff, *s_sublist="abcdfiklnopqrstvwx#!@0123456789+?<-:", *ptr;

    /* Copy the string to the buffer if it is not too big */

    retval = 0;
    if (strlen(str) >= extra) {
	str[extra - 1] = '\0';
	if (mudstate.initializing) {
	    STARTLOG(LOG_STARTUP, "CNF", "NFND")
		buff = alloc_lbuf("cf_string.LOG");
	    sprintf(buff, "%.3900s: String truncated", cmd);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    notify(player, "String truncated");
	}
	retval = 1;
    }
    for (ptr = str; *ptr; ptr++) {
       *ptr = ToLower(*ptr);
       if ( strchr(s_sublist, *ptr) != NULL ) {
          if ( !mudstate.initializing ) {
            notify(player, "String invalid.  Contained reserved percent-sub character.");
          } else {
	    STARTLOG(LOG_STARTUP, "CNF", "NFND")
		buff = alloc_lbuf("cf_string.LOG");
	    sprintf(buff, "%.3900s: String invalid.  Contained one of '%s'.", cmd, s_sublist);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
          }
          return -1;
       }
    }
    strcpy((char *) vp, str);
    return retval;
}

/* ---------------------------------------------------------------------------
 * cf_alias: define a generic hash table alias.
 */

int validate_aliases(dbref player,
		     HASHTAB *htab,
		     char *orig, char *alias,
		     char *label, char *cmd) {
  int *cmdp = NULL, *aliasp = NULL, retval;

  retval = 0;

  if (!orig || !*orig) {
    cf_log_syntax(player, cmd, "Don't know what to alias.", "");
    return -1;
  }

  if (!alias || !*alias) {
    /* Not much we can do here in a generic fashion.. */
    ;
  }
  else if (strcmp("!", orig) == 0) {
    /* We want to remove the alias
     * - orig is assumed to be the alias
     */
    aliasp = hashfind(alias, htab);
    if (!aliasp) {
      cf_log_syntax(player, cmd, "Error: '%s' is not a valid alias.", alias);
      retval = -1;
    } else if (htab->last_entry->bIsOriginal == 1) {
      cf_log_syntax(player, cmd, "Error: '%s' is not an alias, cannot delete.", alias);
      retval = -1;
    } else {
      cf_log_syntax(player, cmd, "Warning: Alias '%s' deleted.", alias);
      hashdelete(alias, htab);
      retval = 1;
    }
  } else {
    cmdp = hashfind(orig, htab);
    if (cmdp == NULL) {
      cf_log_notfound(player, cmd, label, orig);
      retval = -1;
    }
    aliasp = hashfind(alias, htab);
    if (aliasp == NULL) {
      retval = hashadd2(alias, cmdp, htab, 0);
    } else if ( htab->last_entry->bIsOriginal == 1) {
      cf_log_syntax(player, cmd, "Error: '%s' is not an alias, cannot redefine.", alias);
      retval = -1;
    } else {
      cf_log_syntax(player, cmd, "Warning: Redefining alias '%s'", alias);
      retval = hashrepl2(alias, cmdp, htab, 0);
    }
  }
  return retval;
}

CF_HAND(cf_alias)
{
    char *alias, *orig, *tstrtokr;
    int retval;

    alias = strtok_r(str, " \t=,", &tstrtokr);
    orig = strtok_r(NULL, " \t=,", &tstrtokr);

    retval = validate_aliases(player,
			      (HASHTAB *) vp, orig, alias,
			      "Entry", cmd);
    return retval;
}

/* ---------------------------------------------------------------------------
 * The @flagdef and @toggledef command line foo
 */
CF_HAND(cf_flag_access)
{
   int retval;

   retval = do_flag_and_toggle_def_conf(player, str, cmd, vp, 1);
   return retval;
}

CF_HAND(cf_toggle_access) 
{
   int retval;

   retval = do_flag_and_toggle_def_conf(player, str, cmd, vp, 2);
   return retval;
}
/* ---------------------------------------------------------------------------
 * cf_flagalias: define a flag alias.
 */

CF_HAND(cf_flagalias)
{
    char *alias, *orig, *tstrtokr;
    int retval;

    alias = strtok_r(str, " \t=,", &tstrtokr);
    orig = strtok_r(NULL, " \t=,", &tstrtokr);

    retval = validate_aliases(player,
			      &mudstate.flags_htab, orig, alias,
			      "Flag", cmd);
    return retval;
}

CF_HAND(cf_flagname)
{
    char *alias, *orig, *tstrtokr;

    if (mudstate.initializing) {
	alias = strtok_r(str, " \t=,", &tstrtokr);
	orig = strtok_r(NULL, " \t=,", &tstrtokr);
	return (flagstuff_internal(orig,alias));
    }
    else
	return -1;
}

/* ---------------------------------------------------------------------------
 * cf_or_in_bits: OR in bits from namelist to a word.
 */

CF_HAND(cf_or_in_bits)
{
    char *sp, *tstrtokr;
    int f, success, failure;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    while (sp != NULL) {

	/* Set the appropriate bit */

	f = search_nametab(GOD, (NAMETAB *) extra, sp);
	if (f != -1) {
	    *vp |= f;
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    return cf_status_from_succfail(player, cmd, success, failure);
}

int
cf_modify_multibits(int *vp, int *vp2, char *str, long extra, long extra2, dbref player, char *cmd)
{
    char *sp, *tstrtokr;
    int f, negate, success, failure;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    while (sp != NULL) {

	/* Check for negation */

	negate = 0;
	if (*sp == '!') {
	    negate = 1;
	    sp++;
	}
	/* Set or clear the appropriate bit */

	f = search_nametab(GOD, (NAMETAB *) extra, sp);
	if (f != -1) {
	    if (negate)
		*vp &= ~f;
	    else
		*vp |= f;
	    success++;
	} else if ( extra2 ) {
	    f = search_nametab(GOD, (NAMETAB *) extra2, sp);
            if ( f != -1 ) {
	       if (negate)
		   *vp2 &= ~f;
	       else
		   *vp2 |= f;
	       success++;
	   } else {
	       cf_log_notfound(player, cmd, "Entry", sp);
	       failure++;
           }
	} else {
	   cf_log_notfound(player, cmd, "Entry", sp);
	   failure++;
        }

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    return cf_status_from_succfail(player, cmd, success, failure);
}

/* ---------------------------------------------------------------------------
 * cf_modify_bits: set or clear bits in a flag word from a namelist.
 */
CF_HAND(cf_modify_bits)
{
    char *sp, *tstrtokr;
    int f, negate, success, failure;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    while (sp != NULL) {

	/* Check for negation */

	negate = 0;
	if (*sp == '!') {
	    negate = 1;
	    sp++;
	}
	/* Set or clear the appropriate bit */

	f = search_nametab(GOD, (NAMETAB *) extra, sp);
	if (f != -1) {
	    if (negate)
		*vp &= ~f;
	    else
		*vp |= f;
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    return cf_status_from_succfail(player, cmd, success, failure);
}

/* ---------------------------------------------------------------------------
 * cf_set_bits: Clear flag word and then set specified bits from namelist.
 */

CF_HAND(cf_set_bits)
{
    char *sp, *tstrtokr;
    int f, success, failure;

    /* Walk through the tokens */

    success = failure = 0;
    *vp = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    while (sp != NULL) {

	/* Set the appropriate bit */

	f = search_nametab(GOD, (NAMETAB *) extra, sp);
	if (f != -1) {
	    *vp |= f;
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    return cf_status_from_succfail(player, cmd, success, failure);
}

/* ---------------------------------------------------------------------------
 * cf_set_depowers: Clear flag word and then set from a flags htab.
 */

CF_HAND(cf_set_depowers)
{
    char *sp, *tstrtokr;
    POWENT *fp;
    FLAGSET *fset;

    int success, failure, clear;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    fset = (FLAGSET *) vp;

    while (sp != NULL) {

	/* Set the appropriate bit */

	if (*sp == '!') {
	  clear = 1;
	  sp++;
	}
	else
	  clear = 0;

        fp = (POWENT *) hashfind(sp, &mudstate.depowers_htab);
	if (fp != NULL) {
	    if (success == 0) {
		(*fset).word6 = 0;
		(*fset).word7 = 0;
		(*fset).word8 = 0;
	    }
	    if (clear) {
	      if (fp->powerflag & POWER6)
		(*fset).word6 &= ~(fp->powerpos);
	      if (fp->powerflag & POWER7)
		(*fset).word7 &= ~(fp->powerpos);
	      else
		(*fset).word8 &= ~(fp->powerpos);
	    }
	    else {
	      if (fp->powerflag & POWER6)
		(*fset).word6 |= fp->powerpos;
	      if (fp->powerflag & POWER7)
		(*fset).word7 |= fp->powerpos;
	      else
		(*fset).word8 |= fp->powerpos;
	    }
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    if ((success == 0) && (failure == 0)) {
	(*fset).word6 = 0;
	(*fset).word7 = 0;
	(*fset).word8 = 0;

	return 0;
    }
    if (success > 0)
	return ((failure == 0) ? 0 : 1);
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_set_powers: Clear flag word and then set from a flags htab.
 */

CF_HAND(cf_set_powers)
{
    char *sp, *tstrtokr;
    POWENT *fp;
    FLAGSET *fset;

    int success, failure, clear;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    fset = (FLAGSET *) vp;

    while (sp != NULL) {

	/* Set the appropriate bit */

	if (*sp == '!') {
	  clear = 1;
	  sp++;
	}
	else
	  clear = 0;

        fp = (POWENT *) hashfind(sp, &mudstate.powers_htab);
	if (fp != NULL) {
	    if (success == 0) {
		(*fset).word3 = 0;
		(*fset).word4 = 0;
		(*fset).word5 = 0;
	    }
	    if (clear) {
	      if (fp->powerflag & POWER3)
		(*fset).word3 &= ~(fp->powerpos);
	      if (fp->powerflag & POWER4)
		(*fset).word4 &= ~(fp->powerpos);
	      else
		(*fset).word5 &= ~(fp->powerpos);
	    }
	    else {
	      if (fp->powerflag & POWER3)
		(*fset).word3 |= fp->powerpos;
	      if (fp->powerflag & POWER4)
		(*fset).word4 |= fp->powerpos;
	      else
		(*fset).word5 |= fp->powerpos;
	    }
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    if ((success == 0) && (failure == 0)) {
	(*fset).word3 = 0;
	(*fset).word4 = 0;
	(*fset).word5 = 0;

	return 0;
    }
    if (success > 0)
	return ((failure == 0) ? 0 : 1);
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_set_toggles: Clear flag word and then set from a flags htab.
 */

CF_HAND(cf_set_toggles)
{
    char *sp, *tstrtokr;
    TOGENT *fp;
    FLAGSET *fset;

    int success, failure, clear;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    fset = (FLAGSET *) vp;

    while (sp != NULL) {

	/* Set the appropriate bit */

	if (*sp == '!') {
	  clear = 1;
	  sp++;
	}
	else
	  clear = 0;

        fp = (TOGENT *) hashfind(sp, &mudstate.toggles_htab);
	if (fp != NULL) {
	    if (success == 0) {
		(*fset).word1 = 0;
		(*fset).word2 = 0;
	    }
	    if (clear) {
	      if (fp->toggleflag & TOGGLE2)
		(*fset).word2 &= ~(fp->togglevalue);
	      else
		(*fset).word1 &= ~(fp->togglevalue);
	    }
	    else {
	      if (fp->toggleflag & TOGGLE2)
		(*fset).word2 |= fp->togglevalue;
	      else
		(*fset).word1 |= fp->togglevalue;
	    }
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    if ((success == 0) && (failure == 0)) {
	(*fset).word1 = 0;
	(*fset).word2 = 0;

	return 0;
    }
    if (success > 0)
	return ((failure == 0) ? 0 : 1);
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_set_flags: Clear flag word and then set from a flags htab.
 */

CF_HAND(cf_set_flags)
{
    char *sp, *tstrtokr;
    FLAGENT *fp;
    FLAGSET *fset;

    int success, failure, clear;

    /* Walk through the tokens */

    success = failure = 0;
    sp = strtok_r(str, " \t", &tstrtokr);
    fset = (FLAGSET *) vp;

    while (sp != NULL) {

	/* Set the appropriate bit */

	if (*sp == '!') {
	  clear = 1;
	  sp++;
	}
	else
	  clear = 0;

	fp = (FLAGENT *) hashfind(sp, &mudstate.flags_htab);
	if (fp != NULL) {
	    if (success == 0) {
		(*fset).word1 = 0;
		(*fset).word2 = 0;
		(*fset).word3 = 0;
		(*fset).word4 = 0;
	    }
	    if (clear) {
	      if (fp->flagflag & FLAG2)
		(*fset).word2 &= ~(fp->flagvalue);
	      else if (fp->flagflag & FLAG3)
		(*fset).word3 &= ~(fp->flagvalue);
	      else if (fp->flagflag & FLAG4)
		(*fset).word4 &= ~(fp->flagvalue);
	      else
		(*fset).word1 &= ~(fp->flagvalue);
	    }
	    else {
	      if (fp->flagflag & FLAG2)
		(*fset).word2 |= fp->flagvalue;
	      else if (fp->flagflag & FLAG3)
		(*fset).word3 |= fp->flagvalue;
	      else if (fp->flagflag & FLAG4)
		(*fset).word4 |= fp->flagvalue;
	      else
		(*fset).word1 |= fp->flagvalue;
	    }
	    success++;
	} else {
	    cf_log_notfound(player, cmd, "Entry", sp);
	    failure++;
	}

	/* Get the next token */

	sp = strtok_r(NULL, " \t", &tstrtokr);
    }
    if ((success == 0) && (failure == 0)) {
	(*fset).word1 = 0;
	(*fset).word2 = 0;
	(*fset).word3 = 0;
	(*fset).word4 = 0;

	return 0;
    }
    if (success > 0)
	return ((failure == 0) ? 0 : 1);
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_badname: Disallow use of player name/alias.
 */

CF_HAND(cf_badname)
{
    if (extra)
	badname_remove(str);
    else
	badname_add(str);
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_site: Update site information
 */

CF_HAND2(cf_site)
{
    SITE *site, *last, *head, *s_tmp;
    char *addr_txt, *mask_txt, *maxcon_txt, *tstrtokr, *s_val;
    int i_maxcon, i_found;
    struct in_addr addr_num, mask_num;
    unsigned long maskval;

    addr_txt = strtok_r(str, " \t=,", &tstrtokr);
    mask_txt = NULL;
    if (addr_txt)
	mask_txt = strtok_r(NULL, " \t=,", &tstrtokr);
    if (!addr_txt || !*addr_txt || !mask_txt || !*mask_txt) {
	cf_log_syntax(player, cmd, "Missing host address or mask.",
		      (char *) "");
	return -1;
    }
    addr_num.s_addr = inet_addr(addr_txt);
    if ( strchr(mask_txt, '/') != NULL ) {
       maskval = atol(mask_txt+1);
       if (((long)maskval < 0) || (maskval > 32)) {
	  cf_log_syntax(player, cmd, "Bad address mask: %s", mask_txt);
	  return -1;
       }
       if ( maskval != 0 ) {
          maskval = (0xFFFFFFFFUL << (32 - maskval));
       }
       mask_num.s_addr = htonl(maskval);
    } else
       mask_num.s_addr = inet_addr(mask_txt);

    if (addr_num.s_addr == -1) {
	cf_log_syntax(player, cmd, "Bad host address: %s", addr_txt);
	return -1;
    }

    i_maxcon = -1;
    if ( mask_txt ) {
       maxcon_txt = strtok_r(NULL, " \t=,", &tstrtokr);
       if ( maxcon_txt && *maxcon_txt ) {
          i_maxcon = atoi(maxcon_txt);
          if ( i_maxcon < 0 )
             i_maxcon = -1;
       }
    }


    head = (SITE *) (pmath2)*vp;

    i_found = 0;
    for ( s_tmp = head; s_tmp; s_tmp = s_tmp->next ) {
       if ( s_tmp && (s_tmp->maxcon == i_maxcon) &&
            (s_tmp->address.s_addr == addr_num.s_addr) &&
            (s_tmp->mask.s_addr == mask_num.s_addr) &&
            (s_tmp->flag == extra) ) {
          i_found = 1;
          break;
       }
    }
    if ( i_found ) {
       s_val = alloc_lbuf("cf_site");
       sprintf(s_val, "%.200s %.200s", addr_txt, mask_txt);
       cf_log_syntax(player, cmd, "Duplicate entry: %s", s_val);
       free_lbuf(s_val);
       return -1;
    }

    /* Parse the access entry and allocate space for it */
    site = (SITE *) malloc(sizeof(SITE));

    /* Initialize the site entry */

    if ( extra & H_AUTOSITE ) {
       extra &= ~H_AUTOSITE;
       site->key = 1;
    } else {
       site->key = 0;
    }
    site->maxcon = i_maxcon;
    site->address.s_addr = addr_num.s_addr;
    site->mask.s_addr = mask_num.s_addr;
    site->flag = extra;
    site->next = NULL;

    /* Link in the entry.  Link it at the start if not initializing, at
     * the end if initializing.  This is so that entries in the config
     * file are processed as you would think they would be, while entries
     * made while running are processed first.
     */

    if (mudstate.initializing) {
	if (head == NULL) {
	    *vp = (pmath2) site;
	} else {
	    for (last = head; last->next; last = last->next);
	    last->next = site;
	}
    } else {
	site->next = head;
	*vp = (pmath2) site;
    }
    if ( site->key == 1 )
       return 777;
    else
       return 0;
}

/* ---------------------------------------------------------------------------
 * cf_cf_access: Set access on config directives
 */

CF_HAND(cf_cf_access)
{
    CONF *tp;
    char *ap;

    for (ap = str; *ap && !isspace((int)*ap); ap++);
    if (*ap)
	*ap++ = '\0';

    for (tp = conftable; tp->pname; tp++) {
	if (!strcmp(tp->pname, str)) {
	    return (cf_modify_bits(&tp->flags, ap, extra, extra2,
				   player, cmd));
	}
    }
    cf_log_notfound(player, cmd, "Config directive", str);
    return -1;
}

/* ---------------------------------------------------------------------------
 * cf_include: Read another config file.  Only valid during startup.
 */

CF_HAND(cf_include)
{
    FILE *fp;
    char *cp, *ap, *zp, *buf;

    extern int FDECL(cf_set, (char *, char *, dbref));


    if (!mudstate.initializing)
	return -1;

    STARTLOG(LOG_ALWAYS, "CNF", "INCL")
      log_text((char *) "Conf including: ");
      log_text(str);
    ENDLOG

    fp = fopen(str, "r");
    if (fp == NULL) {
	cf_log_notfound(player, cmd, "Config file", str);
	return -1;
    }
    buf = alloc_lbuf("cf_include");
    fgets(buf, LBUF_SIZE, fp);
    while (!feof(fp)) {
	cp = buf;
	if (*cp == '#') {
	    fgets(buf, LBUF_SIZE, fp);
	    continue;
	}
	/* Not a comment line.  Strip off the NL and any characters
	 * following it.  Then, split the line into the command and
	 * argument portions (separated by a space).  Also, trim off
	 * the trailing comment, if any (delimited by #)
	 */

	for (cp = buf; *cp && *cp != '\n'; cp++);
	*cp = '\0';		/* strip \n */
	for (cp = buf; *cp && isspace((int)*cp); cp++);	/* strip spaces */
	for (ap = cp; *ap && !isspace((int)*ap); ap++);	/* skip over command */
	if (*ap)
	    *ap++ = '\0';	/* trim command */
	for (; *ap && isspace((int)*ap); ap++);	/* skip spaces */
	for (zp = ap; *zp && (*zp != '#'); zp++);	/* find comment */
	if (*zp)
	    *zp = '\0';		/* zap comment */
	for (zp = zp - 1; zp >= ap && isspace((int)*zp); zp--)
	    *zp = '\0';		/* zap trailing spcs */

	cf_set(cp, ap, player);
	fgets(buf, LBUF_SIZE, fp);
    }
    free_lbuf(buf);
    fclose(fp);
    return 0;
}

extern CF_HDCL(cf_access);
extern CF_HDCL(cf_cmd_alias);
extern CF_HDCL(cf_cmd_vattr);
extern CF_HDCL(cf_acmd_access);
extern CF_HDCL(cf_attr_access);
extern CF_HDCL(cf_ntab_access);
extern CF_HDCL(cf_func_access);
extern CF_HDCL(cf_cmd_ignore);

/* ---------------------------------------------------------------------------
 * conftable: Table for parsing the configuration file.
 */

CONF conftable[] =
{
    {(char *) "abort_on_bug",
     cf_bool, CA_DISABLED, &mudconf.abort_on_bug, 0, 0, CA_WIZARD,
     (char *)"Bring down game on error?"},
    {(char *) "access",
     cf_access, CA_GOD | CA_IMMORTAL, NULL,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Configure command access permissions."},
    {(char *) "ahear_maxcount",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.ahear_maxcount, 0, 0, CA_PUBLIC,
     (char *) "Maximum listen/ahear iterations?\r\n"\
              "                             Default: 30   Value: %d"},
    {(char *) "ahear_maxtime",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.ahear_maxtime, 0, 0, CA_PUBLIC,
     (char *) "Maximum listen/ahear time laps (in seconds)?\r\n"\
              "                             Default: 60   Value: %d"},
    {(char *) "vattr_command",
     cf_cmd_vattr, CA_GOD | CA_IMMORTAL, (int *) &mudstate.command_vattr_htab, 0, 0, CA_WIZARD,
     (char *) "Define dynamic VATTR commands."},
    {(char *) "alias",
     cf_cmd_alias, CA_GOD | CA_IMMORTAL, (int *) &mudstate.command_htab, 0, 0, CA_WIZARD,
     (char *) "Define command alises."},
    {(char *) "alt_inventories",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.alt_inventories, 0, 0, CA_PUBLIC,
     (char *) "Enable alternate inventories (worn/wield)."},
    {(char *) "altover_inv",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.altover_inv, 0, 0, CA_PUBLIC,
     (char *) "Seperate displays of inventories (worn/wield)."},
    {(char *) "always_blind",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.always_blind, 0, 0, CA_PUBLIC,
     (char *) "BLIND flag reversed (always blind?)"},
    {(char *) "areg_lim",
     cf_int, CA_GOD | CA_IMMORTAL, (int *) &mudconf.areg_lim, 0, 0, CA_WIZARD,
     (char *) "Specify limit on registers from single email.\r\n"\
              "                             Default: 3   Value: %d"},
    {(char *) "attr_access",
     cf_attr_access, CA_GOD | CA_IMMORTAL, NULL,
     (pmath2) attraccess_nametab, 0, CA_WIZARD,
     (char *) "Configure attribute permissions (internal)."},
    {(char *) "attr_alias",
     cf_alias, CA_GOD | CA_IMMORTAL, (int *) &mudstate.attr_name_htab, 0, 0, CA_WIZARD,
     (char *) "Define attribute aliases."},
    {(char *) "attr_cmd_access",
     cf_acmd_access, CA_GOD | CA_IMMORTAL, NULL,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Configure attribute access permissions."},
    {(char *) "allow_ansinames",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.allow_ansinames, 0, 0, CA_PUBLIC,
     (char *) "Allow names to be ansified?\r\n"\
              "                             Default: 15   Value: %d"},
    {(char *) "allow_whodark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.allow_whodark, 0, 0, CA_PUBLIC,
     (char *) "Allow dark players to hide from WHO/DOING?"},
    {(char *) "areg_file",
     cf_string, CA_DISABLED, (int *) mudconf.areg_file, 32, 0, CA_WIZARD,
     (char *) "Specify the auto-register txt file."},
    {(char *) "aregh_file",
     cf_string, CA_DISABLED, (int *) mudconf.aregh_file, 32, 0, CA_WIZARD,
     (char *) "Specify the auto-register with connects file." },
    {(char *) "bad_name",
     cf_badname, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Specify a list of bad player names."},
    {(char *) "badsite_file",
     cf_string, CA_DISABLED, (int *) mudconf.site_file, 32, 0, CA_WIZARD,
     (char *) "Specify the forbid site file."},
    {(char *) "bcc_hidden",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.bcc_hidden, 0, 0, CA_WIZARD,
     (char *) "Does +bcc in mail hide who mail from in target?"},
    {(char *) "blind_snuffs_cons",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.blind_snuffs_cons, 0, 0, CA_PUBLIC,
     (char *) "BLIND flag snuff aconnect/adisconnect?"},
    {(char *) "brace_compatibility",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.brace_compatibility, 0, 0, CA_WIZARD,
     (char *) "Are braces MUX/TM3 compatible?"},
    {(char *) "break_compatibility",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.break_compatibility, 0, 0, CA_WIZARD,
     (char *) "Are @break/@assert double-evalled and earlier 3.9 compatable?"},
     {(char *) "exits_connect_rooms",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.exits_conn_rooms, 0, 0, CA_WIZARD,
     (char *) "Is a room with an exit considered floating?"},
    {(char *) "cache_depth",
     cf_int, CA_DISABLED, &mudconf.cache_depth, 0, 0, CA_WIZARD,
     (char *) "Show what the current cache debth is.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "cache_names",
     cf_bool, CA_DISABLED, &mudconf.cache_names, 0, 0, CA_WIZARD,
     (char *) "Are names being cached?"},
    {(char *) "cache_steal_dirty",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.cache_steal_dirty, 0, 0, CA_WIZARD,
     (char *) "Does the cache do dirty reads?"},
    {(char *) "cache_trim",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.cache_trim, 0, 0, CA_WIZARD,
     (char *) "Is the cache trimmed back to initial value?"},
    {(char *) "cache_width",
     cf_int, CA_DISABLED, &mudconf.cache_width, 0, 0, CA_WIZARD,
     (char *) "What is the current cache width?\r\n"
              "                             Default: 541   Value: %d"},
    {(char *) "atrperms",
     cf_atrperms, CA_GOD | CA_IMMORTAL, (int *) mudconf.atrperms, LBUF_SIZE - 2, 0, CA_WIZARD,
     (char *) "Prefix permission masks for attributes."},
    {(char *) "atrperms_max",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.atrperms_max, 0, 0, CA_WIZARD,
     (char *) "Max Attribute Prefix Permissions?\r\n"
              "     (range: 0-10000)        Default: 100   Value: %d"},
    {(char *) "cap_conjunctions",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.cap_conjunctions, LBUF_SIZE - 2, 0, CA_WIZARD,
     (char *) "Exceptions for caplist -- conjunctions."},
    {(char *) "cap_articles",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.cap_articles, LBUF_SIZE - 2, 0, CA_WIZARD,
     (char *) "Exceptions for caplist -- articles."},
    {(char *) "cap_preposition",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.cap_preposition, LBUF_SIZE - 2, 0, CA_WIZARD,
     (char *) "Exceptions for caplist -- preposition."},
    {(char *) "check_interval",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.check_interval, 0, 0, CA_WIZARD,
     (char *) "What is the DBCK interval?\r\n"\
              "                             Default: 600   Value: %d"},
    {(char *) "check_offset",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.check_offset, 0, 0, CA_WIZARD,
     (char *) "What is the offset of internal checks?\r\n"\
              "                             Default: 300   Value: %d"},
    {(char *) "clone_copies_cost",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.clone_copy_cost, 0, 0, CA_PUBLIC,
     (char *) "Is the cost copied with a @clone?"},
    {(char *) "cluster_cap",
     cf_int, CA_WIZARD, &mudconf.comcost, 0, 0, CA_PUBLIC,
     (char *) "This is the time in seconds action waits\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "clusterfunc_cap",
     cf_int, CA_WIZARD, &mudconf.comcost, 0, 0, CA_PUBLIC,
     (char *) "This is the time in seconds function action waits\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "comcost",
     cf_int, CA_WIZARD, &mudconf.comcost, 0, 0, CA_PUBLIC,
     (char *) "This is the cost per comsystem usage\r\n"\
              "                             Default: 5   Value: %d"},
    {(char *) "command_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.comm_dark, 0, 0, CA_PUBLIC,
     (char *) "Are objects with $commands DARK?"},
    {(char *) "command_quota_increment",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.cmd_quota_incr, 0, 0, CA_WIZARD,
     (char *) "Number of commands per timeslice per user.\r\n"\
              "      -- This is not used    Default: 0   Value: %d"},
    {(char *) "command_quota_max",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.cmd_quota_max, 0, 0, CA_WIZARD,
     (char *) "Max commands allowed per timeslice per user.\r\n"\
              "      -- This is not used    Default: 100   Value: %d"},
    {(char *) "compress_program",
     cf_string, CA_DISABLED, (int *) mudconf.compress, 128, 0, CA_WIZARD,
     (char *) "Compression program for db compression.\r\n"\
              "                                  (compress) is default"},
    {(char *) "compression",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.compress_db, 0, 0, CA_WIZARD,
     (char *) "Is the db compressed on writing/reading?"},
    {(char *) "config_access",
     cf_cf_access, CA_GOD | CA_IMMORTAL, NULL,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Define config paramater access permissions."},
    {(char *) "conn_timeout",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.conn_timeout, 0, 0, CA_WIZARD,
     (char *) "Define what the timeout is on connect screen.\r\n"\
              "                             Default: 120   Value: %d"},
    {(char *) "connect_file",
     cf_string, CA_DISABLED, (int *) mudconf.conn_file, 32, 0, CA_WIZARD,
     (char *) "Define the connect file."},
    {(char *) "connect_reg_file",
     cf_string, CA_DISABLED, (int *) mudconf.creg_file, 32, 0, CA_WIZARD,
     (char *) "Define the registration file."},
    {(char *) "manlog_file",
     cf_string, CA_DISABLED, (int *) mudconf.manlog_file, 32, 0, CA_WIZARD,
     (char *) "Define file used with @log command."},
    {(char *) "mysql_delay",
     cf_verifyint_mysql, CA_GOD | CA_IMMORTAL, &mudconf.mysql_delay, 86400, 60, CA_WIZARD,
     (char *) "MySQL delay before retry connections allowed.\r\n"\
              "                             Default: 0 (off)  Value: %d"},
#ifdef MYSQL_VERSION
    {(char *) "mysql_host",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mysql_host, 126, 0, CA_WIZARD,
     (char *) "MySQL Hostname to connect to."},
    {(char *) "mysql_user",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mysql_user, 126, 0, CA_WIZARD,
     (char *) "MySQL user of database to connect to."},
    {(char *) "mysql_pass",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mysql_pass, 126, 0, CA_WIZARD,
     (char *) "MySQL password of user of database to connect to."},
    {(char *) "mysql_base",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mysql_base, 126, 0, CA_WIZARD,
     (char *) "MySQL database name to connect to."},
    {(char *) "mysql_socket",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mysql_socket, 126, 0, CA_WIZARD,
     (char *) "MySQL database socket lock file."},
    {(char *) "mysql_port",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.mysql_port, 0, 0, CA_WIZARD,
     (char *) "MySQL database port to connect to.\r\n"\
              "                             Default: 3306 Value: %d"},
#endif
    {(char *) "cpuintervalchk",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.cpuintervalchk, 0, 0, CA_WIZARD,
     (char *) "Define percentage before cpu-enforcement.\r\n"\
              "                             Default: 80   Value: %d"},
    {(char *) "cputimechk",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.cputimechk, 0, 0, CA_WIZARD,
     (char *) "Define time in seconds before cpu-enforcement.\r\n"\
              "                             Default: 60   Value: %d"},
    {(char *) "lastsite_paranoia",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.lastsite_paranoia, 0, 0, CA_WIZARD,
     (char *) "Define DoS site paranoia level (0, 1, 2).\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "lattr_default_oldstyle",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.lattr_default_oldstyle, 0, 0, CA_PUBLIC,
     (char *) "Snuff the '#-1' from lattr()."},
    {(char *) "lcon_checks_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.lcon_checks_dark, 0, 0, CA_PUBLIC,
     (char *) "Lcon/Xcon checks dark/unfindable?"},
    {(char *) "lfunction_max",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.lfunction_max, 0, 0, CA_WIZARD,
     (char *) "Max local functions (@lfunctions) per user.\r\n"\
              "                             Default: 20  Value: %d"},
    {(char *) "map_delim_space",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.map_delim_space, 0, 0, CA_PUBLIC,
     (char *) "MAP() uses space/seperator?"},
    {(char *) "max_cpu_cycles",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_cpu_cycles, 0, 0, CA_WIZARD,
     (char *) "Max cpu slams allowed before smackdown.\r\n"\
              "                             Default: 3   Value: %d"},
    {(char *) "max_dest_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_dest_limit, 0, 0, CA_WIZARD,
     (char *) "Max times user allowed to use @destroy.\r\n"\
              "                             Default: 1000   Value: %d"},
    {(char *) "max_lastsite_api",
     cf_verifyint, CA_GOD | CA_IMMORTAL, &mudconf.max_lastsite_api, 500, 1, CA_WIZARD,
     (char *) "Max times API samesite allowed to connect in row.\r\n"\
              "(Range: 1-500 - 120+ warn)   Default: 60   Value: %d"},
    {(char *) "max_lastsite_cnt",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_lastsite_cnt, 0, 0, CA_WIZARD,
     (char *) "Max times samesite allowed to connect in row.\r\n"\
              "                             Default: 20   Value: %d"},
    {(char *) "max_name_protect",
     cf_verifyint, CA_GOD | CA_IMMORTAL, &mudconf.max_name_protect, 100, 0, CA_WIZARD,
     (char *) "Max name protections a player is allowed.\r\n"\
              "     (Range: 0-100)          Default: 0   Value: %d"},
    {(char *) "max_pcreate_lim",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_pcreate_lim, 0, 0, CA_WIZARD,
     (char *) "Max times a player can create on connect screen.\r\n"\
              "         (-1 is unlimited)   Default: -1   Value: %d"},
    {(char *) "max_pcreate_time",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_pcreate_time, 0, 0, CA_WIZARD,
     (char *) "Max timeframe max player creates can occur.\r\n"\
              "        (Value in seconds)   Default: 60   Value: %d"},
    {(char *) "max_percentsubs",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_percentsubs, 0, 0, CA_WIZARD,
     (char *) "Max allowable percent substitutions per command.\r\n"\
              "                             Default: 1000000   Value: %d"},
    {(char *) "max_vattr_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_vattr_limit, 0, 0, CA_WIZARD,
     (char *) "Max times user can create new user attribs.\r\n"\
              "                             Default: 10000   Value: %d"},
    {(char *) "min_con_attempt",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.min_con_attempt, 0, 0, CA_WIZARD,
     (char *) "Time (in seconds) to allow multiple connects.\r\n"\
              "                             Default: 60   Value: %d"},
    {(char *) "cpu_secure_lvl",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.cpu_secure_lvl, 0, 0, CA_WIZARD,
     (char *) "Level of smackdown you want with CPU abusers.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "crash_database",
     cf_string, CA_DISABLED, (int *) mudconf.crashdb, 128, 0, CA_WIZARD,
     (char *) "Database name for attempted write on crash."},
    {(char *) "create_max_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.createmax, 0, 0, CA_PUBLIC,
     (char *) "Max cost of item.\r\n"\
              "                             Default: 505   Value: %d"},
    {(char *) "create_min_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.createmin, 0, 0, CA_PUBLIC,
     (char *) "Min cost of item.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "dark_sleepers",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.dark_sleepers, 0, 0, CA_PUBLIC,
     (char *) "Are disconnected players DARK by default?"},
    {(char *) "debug_id",
     cf_int, CA_DISABLED, &mudconf.debug_id, 0, 0, CA_WIZARD,
     (char *) "Unique key for debug monitor.\r\n"\
              "                             Default: 42010   Value: %d"},
    {(char *) "delim_null",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.delim_null, 0, 0, CA_PUBLIC,
     (char *) "Are @@ in output seperator considered a null?"},
    {(char *) "accent_extend",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.accent_extend, 0, 0, CA_WIZARD,
     (char *) "Are accents extended past 250 to 255??"},
    {(char *) "admin_object",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.admin_object, 0, 0, CA_WIZARD,
     (char *) "The object that will be used to load and save inline conf parameters.   Default: -1   Value: %d"},
    {(char *) "ansi_default",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.ansi_default, 0, 0, CA_WIZARD,
     (char *) "Are functions that are ansi aware made ansi-aware by default?"},
    {(char *) "ansi_txtfiles",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.ansi_txtfiles, 0, 0, CA_WIZARD,
     (char *) "Are .txt files processed for ansi?"},
    {(char *) "api_port",
     cf_int, CA_DISABLED, &mudconf.api_port, 0, 0, CA_WIZARD,
     (char *) "Specifies what the API port is.  '-1' disables this.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "api_nodns",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.api_nodns, 0, 0, CA_WIZARD,
     (char *) "Are site DNS lookups done for the API layer?"},
    {(char *) "authenticate",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.authenticate, 0, 0, CA_WIZARD,
     (char *) "Are site AUTH/IDENT lookups done?"},
    {(char *) "default_home",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.default_home, 0, 0, CA_WIZARD,
     (char *) "Player's default home.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "descs_are_local",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.descs_are_local, 0, 0, CA_PUBLIC,
     (char *) "Are @descs and other did_it calls local?"},
    {(char *) "dig_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.digcost, 0, 0, CA_PUBLIC,
     (char *) "Cost of @dig command.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "door_file",
     cf_string, CA_DISABLED, (int *) mudconf.door_file, 32, 0, CA_WIZARD,
     (char *) "File used for @door command."},
    {(char *) "door_index",
     cf_string, CA_DISABLED, (int *) mudconf.door_indx, 32, 0, CA_WIZARD,
     (char *) "Index used for @door command."},
    {(char *) "down_file",
     cf_string, CA_DISABLED, (int *) mudconf.down_file, 32, 0, CA_WIZARD,
     (char *) "File used @disable logins in effect."},
    {(char *) "down_motd_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.downmotd_msg, 1024, 0, CA_WIZARD,
     (char *) "Message seen when @disable logins in effect."},
    {(char *) "dump_interval",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.dump_interval, 0, 0, CA_WIZARD,
     (char *) "What is the database dump interval?\r\n"\
              "                             Default: 3600   Value: %d"},
    {(char *) "dump_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.dump_msg, 128, 0, CA_WIZARD,
     (char *) "Message seen BEFORE database dumps."},
    {(char *) "posesay_funct",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.posesay_funct, 0, 0, CA_PUBLIC,
     (char *) "Does SPEECH_PREFIX/SPEECH_SUFFIX allow functions?"},
    {(char *) "postdump_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.postdump_msg, 128, 0, CA_WIZARD,
     (char *) "Message seen AFTER database dumps."},
    {(char *) "dump_offset",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.dump_offset, 0, 0, CA_WIZARD,
     (char *) "What is the base offset between dumps?\r\n"\
              " -- If 0 use 'dump_interval' Default: 0   Value: %d"},
    {(char *) "earn_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.paylimit, 0, 0, CA_PUBLIC,
     (char *) "What is the ceiling on money someone can get?\r\n"\
              "                             Default: 10000   Value: %d"},
    {(char *) "empower_fulltel",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.empower_fulltel, 0, 0, CA_PUBLIC,
     (char *) "Does FULLTEL handle more than self?"},
    {(char *) "enable_tstamps",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.enable_tstamps, 0, 0, CA_PUBLIC,
     (char *) "Are timestamps/modifystamps enabled?"},
    {(char *) "enforce_unfindable",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.enforce_unfindable, 0, 0, CA_PUBLIC,
     (char *) "Is UNFINDABLE/DARK enforced for locations?"},
    {(char *) "enhanced_convtime",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.enhanced_convtime, 0, 0, CA_PUBLIC,
     (char *) "Does convtime() handle extra formats?"},
    {(char *) "error_file",
     cf_string, CA_DISABLED, (int *) mudconf.error_file, 32, 0, CA_WIZARD,
     (char *) "File used for huh? messages."},
    {(char *) "error_index",
     cf_string, CA_DISABLED, (int *) mudconf.error_indx, 32, 0, CA_WIZARD,
     (char *) "Index used for huh? messages."},
    {(char *) "examine_flags",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.ex_flags, 0, 0, CA_PUBLIC,
     (char *) "Are flags shown on examine?"},
    {(char *) "examine_public_attrs",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.exam_public, 0, 0, CA_PUBLIC,
     (char *) "Are public attributes seen with examine?"},
    {(char *) "examine_restrictive",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.examine_restrictive, 0, 0, CA_PUBLIC,
     (char *) "What level is restrictions on Examine?\r\n"\
              "  (0-off 1-6 level based)    Default: 0   Value: %d"},
    {(char *) "exec_secure",
     cf_bool, CA_DISABLED, &mudconf.exec_secure, 0, 0, CA_WIZARD,
     (char *) "Is execscript() escaping everything?"},
    {(char *) "exit_flags",
     cf_set_flags, CA_GOD | CA_IMMORTAL, (int *) &mudconf.exit_flags, 0, 0, CA_PUBLIC,
     (char *) "These are default flags set on new exits."},
    {(char *) "exit_toggles",
     cf_set_toggles, CA_GOD | CA_IMMORTAL, (int *) &mudconf.exit_toggles, 0, 0, CA_PUBLIC,
     (char *) "These are default toggles set on new exits."},
    {(char *) "exit_quota",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.exit_quota, 0, 0, CA_PUBLIC,
     (char *) "How much quota does an exit take up?\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "expand_goto",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.expand_goto, 0, 0, CA_PUBLIC,
     (char *) "Does exit movement expand to 'goto'?"},
    {(char *) "fascist_teleport",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fascist_tport, 0, 0, CA_PUBLIC,
     (char *) "Is @teleport limited to jump_ok only?"},
    {(char *) "file_object",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.file_object, 0, 0, CA_PUBLIC,
     (char *) "The object to specify connect screen foo override.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "find_money_chance",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.payfind, 0, 0, CA_PUBLIC,
     (char *) "Chance to find money when you move. (1/X)\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "flag_alias",
     cf_flagalias, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Define flag aliases."},
    {(char *) "flag_name",
     cf_flagname, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Rename flags (in .conf file ONLY!)."},
    {(char *) "flag_access_set",
     cf_flag_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Flag SET Permissions ala @flagdef"},
    {(char *) "flag_access_see",
     cf_flag_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Flag SEE Permissions ala @flagdef"},
    {(char *) "flag_access_unset",
     cf_flag_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Flag UNSET Permissions ala @flagdef"},
    {(char *) "flag_access_type",
     cf_flag_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Flag TYPE Restrictions ala @flagdef"},
    {(char *) "toggle_access_set",
     cf_toggle_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Toggle SET Permissions ala @flagdef"},
    {(char *) "toggle_access_see",
     cf_toggle_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Toggle SEE Permissions ala @flagdef"},
    {(char *) "toggle_access_unset",
     cf_toggle_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Toggle UNSET Permissions ala @flagdef"},
    {(char *) "toggle_access_type",
     cf_toggle_access, CA_GOD | CA_IMMORTAL, NULL, 0, 0, CA_WIZARD,
     (char *) "Override Toggle TYPE Restrictions ala @flagdef"},
    {(char *) "tree_character",
     cf_string_chr, CA_GOD | CA_IMMORTAL, (int *) mudconf.tree_character, 2, 0, CA_WIZARD,
     (char *) "The character for the tree seperator."},
    {(char *) "forbidapi_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list,
     H_FORBIDAPI, 0, CA_WIZARD,
     (char *) "This specifies sites for forbid that are API."},
    {(char *) "forbid_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list,
     H_FORBIDDEN, 0, CA_WIZARD,
     (char *) "This specifies sites for forbid."},
    {(char *) "forbidapi_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.forbidapi_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "This specifies sites by NAME to forbid API."},
    {(char *) "forbid_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.forbid_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "This specifies sites by NAME to forbid."},
    {(char *) "fork_dump",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fork_dump, 0, 0, CA_PUBLIC,
     (char *) "Are dumps done in the background?"},
    {(char *) "fork_vfork",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fork_vfork, 0, 0, CA_WIZARD,
     (char *) "Is vfork used instead of fork?"},
    {(char *) "format_compatibility",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.format_compatibility, 0, 0, CA_PUBLIC,
     (char *) "Are @<attribute> formats MUSH/MUX compatible?"},
    {(char *) "format_contents",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fmt_contents, 0, 0, CA_PUBLIC,
     (char *) "Can you use @conformat for content formatting?"},
    {(char *) "format_exits",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fmt_exits, 0, 0, CA_PUBLIC,
     (char *) "Can you use @exitformat for exit formatting?"},
    {(char *) "format_name",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.fmt_names, 0, 0, CA_PUBLIC,
     (char *) "Can you use @nameformat for @name formatting?"},
    {(char *) "formats_are_local",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.formats_are_local, 0, 0, CA_PUBLIC,
     (char *) "Are @nameformat/@conformat/@exitformat local?"},
    {(char *) "full_file",
     cf_string, CA_DISABLED, (int *) mudconf.full_file, 32, 0, CA_WIZARD,
     (char *) "File used when site is full."},
    {(char *) "full_motd_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.fullmotd_msg, 1024, 0, CA_WIZARD,
     (char *) "This is the current online @motd message."},
    {(char *) "function_access",
     cf_func_access, CA_GOD | CA_IMMORTAL, NULL,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Configure permissions for functions."},
    {(char *) "function_alias",
     cf_alias, CA_GOD | CA_IMMORTAL, (int *) &mudstate.func_htab, 0, 0, CA_WIZARD,
     (char *) "Define aliases for functions."},
    {(char *) "function_invocation_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.func_invk_lim, 0, 0, CA_WIZARD,
     (char *) "The current function invocation limit.\r\n"\
              "                             Default: 2500   Value: %d"},
    {(char *) "function_max",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.function_max, 0, 0, CA_WIZARD,
     (char *) "Max global functions (@functions).\r\n"\
              "                             Default: 1000  Value: %d"},
    {(char *) "function_recursion_limit",
     cf_recurseint, CA_GOD | CA_IMMORTAL, &mudconf.func_nest_lim, 0, 0, CA_WIZARD,
     (char *) "The current function recursion limit.\r\n"\
              "(Max is 300)                 Default: 50   Value: %d"},
    {(char *) "wildmatch_limit",
     cf_int, CA_GOD | CA_IMMORTAL, (int *) &mudconf.wildmatch_lim, 0, 0, CA_WIZARD,
     (char *) "Ceiling on wildmatch recursion (DoS).\r\n"\
              "        -- 0 is all wizards  Default: 100000   Value: %d"},
    {(char *) "gconf_file",
     cf_string, CA_DISABLED, (int *) mudconf.gconf_file, 32, 0, CA_WIZARD,
     (char *) "File for guest from bad sites."},
    {(char *) "gdbm_database",
     cf_string, CA_DISABLED, (int *) mudconf.gdbm, 128, 0, CA_WIZARD,
     (char *) "Name of the gdbm database."},
    {(char *) "global_aconnect",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.global_aconn, 0, 0, CA_PUBLIC,
     (char *) "Are aconnects enabled in master room?"},
    {(char *) "global_adisconnect",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.global_adconn, 0, 0, CA_PUBLIC,
     (char *) "Are adisconnects enabled in master room?"},
    {(char *) "global_ansimask",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_ansimask, 0, 0, CA_PUBLIC,
     (char *) "Mask of ansi allowed to use in ansi()/%%c.\r\n"\
              "                             Default: 2097151   Value: %d"},
    {(char *) "global_attrdefault",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_attrdefault, 0, 0, CA_PUBLIC,
     (char *) "Object that attribute locks are checked on set.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_clone_exit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_clone_exit, 0, 0, CA_PUBLIC,
     (char *) "Object that EXIT attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_clone_obj",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_clone_obj, 0, 0, CA_PUBLIC,
     (char *) "Object that GENERIC attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_clone_player",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_clone_player, 0, 0, CA_PUBLIC,
     (char *) "Object that PLAYER attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_clone_room",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_clone_room, 0, 0, CA_PUBLIC,
     (char *) "Object that ROOM attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_clone_thing",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_clone_thing, 0, 0, CA_PUBLIC,
     (char *) "Object that THING attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_error_obj",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_error_obj, 0, 0, CA_PUBLIC,
     (char *) "Object who's @va is executed for non-cmds.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_parent_exit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_parent_exit, 0, 0, CA_PUBLIC,
     (char *) "Object that EXIT attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_parent_obj",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_parent_obj, 0, 0, CA_PUBLIC,
     (char *) "Object that GENERIC attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_parent_player",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_parent_player, 0, 0, CA_PUBLIC,
     (char *) "Object that PLAYER attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_parent_room",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_parent_room, 0, 0, CA_PUBLIC,
     (char *) "Object that ROOM attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "global_parent_thing",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.global_parent_thing, 0, 0, CA_PUBLIC,
     (char *) "Object that THING attributes are inherited from.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "good_name",
     cf_badname, CA_GOD | CA_IMMORTAL, NULL, 1, 0, CA_WIZARD,
     (char *) "Remove names from the bad_name list."},
    {(char *) "goodmail_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.goodmail_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Mail addresses to ALLOW ALWAYS from autoreg."},
    {(char *) "guest_randomize",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.guest_randomize, 0, 0, CA_WIZARD,
     (char *) "Guests connect in random order?"},
    {(char *) "guest_file",
     cf_string, CA_DISABLED, (int *) mudconf.guest_file, 32, 0, CA_WIZARD,
     (char *) "File used for guest connections."},
    {(char *) "guild_default",
     cf_string, CA_GOD | CA_IMMORTAL | CA_WIZARD, (int *)mudconf.guild_default, 11, 0, CA_WIZARD,
     (char *) "Default guild set on new players."},
    {(char *) "guild_hdr",
     cf_string, CA_GOD | CA_IMMORTAL | CA_WIZARD, (int *)mudstate.guild_hdr, 11, 0, CA_WIZARD,
     (char *) "Default Guild column name in WHO/DOING."},
    {(char *) "guild_attrname",
     cf_string, CA_GOD | CA_IMMORTAL | CA_WIZARD, (int *) mudconf.guild_attrname, 32, 0, CA_WIZARD,
     (char *) "Which attribute to extract the guild info from. (Default: GUILD)"},
    {(char *) "guest_namelist",
     cf_dynguest, CA_GOD | CA_IMMORTAL, (int *) mudconf.guest_namelist, 1000, 0, CA_WIZARD,
     (char *) "List of guests that are dyanmically named."},
    {(char *) "hackattr_nowiz",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.hackattr_nowiz, 0, 0, CA_WIZARD,
     (char *) "Are _attribs NOT wizard owned?"},
    {(char *) "hackattr_see",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.hackattr_see, 0, 0, CA_WIZARD,
     (char *) "Are _attribs NOT wizard viewable?"},
    {(char *) "hasattrp_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.hasattrp_compat, 0, 0, CA_PUBLIC,
     "Should hasattrp() check only parents? (YES=FALSE, NO=TRUE)"},
    {(char *) "heavy_cpu_max",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.heavy_cpu_max, 0, 0, CA_WIZARD,
     (char *) "Repitions allowed of heavy cpu functions.\r\n"\
              "                             Default: 50   Value: %d"},
    {(char *) "help_file",
     cf_string, CA_DISABLED, (int *) mudconf.help_file, 32, 0, CA_WIZARD,
     (char *) "File used for help."},
    {(char *) "help_index",
     cf_string, CA_DISABLED, (int *) mudconf.help_indx, 32, 0, CA_WIZARD,
     (char *) "Index used for help."},
    {(char *) "hide_nospoof",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.hide_nospoof, 0, 0, CA_WIZARD,
     (char *) "Is the NOSPOOF flag hidden from others?"},
    {(char *) "hook_cmd",
     cf_hook, CA_GOD | CA_IMMORTAL, &mudconf.hook_cmd, 0, 0, CA_WIZARD,
     (char *) "Set/remove HOOK bitmasks from commands."},
    {(char *) "hook_obj",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.hook_obj, 0, 0, CA_WIZARD,
     (char *) "Global Hook Object dbref#.\r\n"\
              "    (-1 for no defined obj)  Default: -1   Value: %d"},
    {(char *) "hook_offline",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.hook_offline, 0, 0, CA_WIZARD,
     (char *) "Allow offline 'create' to hook on @pcreate/after?"},
    {(char *) "hostnames",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.use_hostname, 0, 0, CA_WIZARD,
     (char *) "Are hostnames viewable or just numerics?"},
    {(char *) "html_port",
     cf_int, CA_DISABLED, &mudconf.html_port, 0, 0, CA_WIZARD,
     (char *) "Specifies what the HTML port is.\r\n"\
              "                             Default: 6251   Value: %d"},
    {(char *) "icmd_obj",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.icmd_obj, 0, 0, CA_PUBLIC,
     (char *) "The dbref# of the @icmd object.\r\n"\
              "                             Default: -1    Value: %d"},
    {(char *) "idle_message",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.idle_message, 0, 0, CA_WIZARD,
     (char *) "Do wizards receive message when idled out?"},
    {(char *) "idle_stamp",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.idle_stamp, 0, 0, CA_WIZARD,
     (char *) "Enable idle player checking?"},
    {(char *) "idle_stamp_max",
     cf_verifyint, CA_GOD | CA_IMMORTAL, &mudconf.idle_stamp_max, 300, 1, CA_WIZARD,
     (char *) "Max count for idle checking.\r\n"\
              "        [range 1-300]        Default: 10   Value: %d"},
    {(char *) "idle_wiz_cloak",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.idle_wiz_cloak, 0, 0, CA_WIZARD,
     (char *) "Do wizards cloak when idled out?"},
    {(char *) "idle_wiz_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.idle_wiz_dark, 0, 0, CA_WIZARD,
     (char *) "Do wizards go dark when idled out?"},
    {(char *) "idle_interval",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.idle_interval, 0, 0, CA_WIZARD,
     (char *) "Interval between idle checks.\r\n"\
              "                             Default: 60   Value: %d"},
    {(char *) "idle_timeout",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.idle_timeout, 0, 0, CA_WIZARD,
     (char *) "Value in seconds before someone idles out.\r\n"\
              "                             Default: 3600   Value: %d"},
    {(char *) "ifelse_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.ifelse_compat, 0, 0, CA_PUBLIC,
     (char *) "Does @if/if() eval to TRUE with normal strings?"},
    {(char *) "ifelse_substitutions",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.ifelse_substitutions, 0, 0, CA_PUBLIC,
     (char *) "Does @switch/switch()/switchall() allow #$?"},
    {(char *) "image_dir",
     cf_string, CA_DISABLED, (int *) mudconf.image_dir, 128, 0, CA_WIZARD,
     (char *) "Location of dbref# image files."},
    {(char *) "imm_nomod",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.imm_nomod, 0, 0, CA_WIZARD,
     (char *) "Can only immortal set/effect NOMODIFY flag?"},
    {(char *) "include",
     cf_include, CA_DISABLED, NULL, 0, 0, CA_WIZARD,
     (char *) "Process config params by filename on startup."},
    {(char *) "includecnt",
     cf_int, CA_GOD, &mudconf.includecnt, 0, 0, CA_WIZARD,
     (char *) "Include count iterations per command set.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "includenest",
     cf_int, CA_GOD, &mudconf.includenest, 0, 0, CA_WIZARD,
     (char *) "Include recursion iterations per command set.\r\n"\
              "                             Default: 3   Value: %d"},
    {(char *) "initial_size",
     cf_int, CA_DISABLED, &mudconf.init_size, 0, 0, CA_WIZARD,
     (char *) "Initial database size.\r\n"\
              "                             Default: 20000   Value: %d"},
    {(char *) "data_dir",
     cf_string, CA_DISABLED, (int *) mudconf.data_dir, 128, 0, CA_WIZARD,
     (char *) "Location of data files."},
    {(char *) "txt_dir",
     cf_string, CA_DISABLED, (int *) mudconf.txt_dir, 128, 0, CA_WIZARD,
     (char *) "Location of txt files."},
    {(char *) "input_database",
     cf_string, CA_DISABLED, (int *) mudconf.indb, 128, 0, CA_WIZARD,
     (char *) "Name of the input database."},
    {(char *) "internal_compress_string",
     cf_bool, CA_DISABLED, &mudconf.intern_comp, 0, 0, CA_WIZARD,
     (char *) "Are strings compressed internally?"},
    {(char *) "inventory_name",
     cf_string, CA_IMMORTAL, (int *) mudconf.invname, 79, 0, CA_PUBLIC,
     (char *) "What is the inventory name for alt inventory?"},
    {(char *) "ip_address",
     cf_string, CA_DISABLED, (int *) mudconf.ip_address, 15, 0, CA_WIZARD,
     (char *) "IP address for the MUSH to listen on."},
    {(char *) "kill_guarantee_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.killguarantee, 0, 0, CA_PUBLIC,
     (char *) "How much money to guarentee a kill?\r\n"\
              "                             Default: 100   Value: %d"},
    {(char *) "kill_max_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.killmax, 0, 0, CA_PUBLIC,
     (char *) "Maximum money used for kill.\r\n"\
              "                             Default: 100   Value: %d"},
    {(char *) "kill_min_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.killmin, 0, 0, CA_PUBLIC,
     (char *) "Minimum money used for kill.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "link_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.linkcost, 0, 0, CA_PUBLIC,
     (char *) "Cost to @link.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "list_access",
     cf_ntab_access, CA_GOD | CA_IMMORTAL, (int *) list_names,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Change permissions of options with @list."},
    {(char *) "list_max_chars",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.list_max_chars, 0, 0, CA_PUBLIC,
     (char *) "Maximum characters allowed in single list() output.\r\n"\
              "                             Default: 1000000   Value: %d"},
    {(char *) "listen_parents",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.listen_parents, 0, 0, CA_PUBLIC,
     (char *) "Do ^listens follow @parent trees?"},
    {(char *) "lock_recursion_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.lock_nest_lim, 0, 0, CA_WIZARD,
     (char *) "Current recursion limit on @locks.\r\n"\
              "                             Default: 20   Value: %d"},
    {(char *) "log",
     cf_modify_bits, CA_GOD | CA_IMMORTAL, &mudconf.log_options,
     (pmath2) logoptions_nametab, 0, CA_WIZARD,
     (char *) "Current log options enabled."},
    {(char *) "log_maximum",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.log_maximum, 0, 0, CA_WIZARD,
     (char *) "Maximum logstotxt()'s allowed per command.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "log_command_list",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.log_command_list, 1000, 0, CA_WIZARD,
     (char *) "List of commands that are being logged."},
    {(char *) "log_network_errors",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.log_network_errors, 0, 0, CA_PUBLIC,
     (char *) "Do we log network socket errors for connections?"},
    {(char *) "log_options",
     cf_modify_bits, CA_GOD | CA_IMMORTAL, &mudconf.log_info,
     (pmath2) logdata_nametab, 0, CA_WIZARD,
     (char *) "Options to be included with logging."},
    {(char *) "login_to_prog",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.login_to_prog, 0, 0, CA_PUBLIC,
     (char *) "Can a user log into running @program?"},
    {(char *) "logout_cmd_access",
     cf_ntab_access, CA_GOD | CA_IMMORTAL, (int *) logout_cmdtable,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Configure access of commands on connect."},
    {(char *) "logout_cmd_alias",
     cf_alias, CA_GOD | CA_IMMORTAL, (int *) &mudstate.logout_cmd_htab, 0, 0, CA_WIZARD,
     (char *) "Define aliases for commands on connect."},
    {(char *) "look_moreflags",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.look_moreflags, 0, 0, CA_PUBLIC,
     (char *) "Do global flags show up by attributes?"},
    {(char *) "look_obey_terse",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.terse_look, 0, 0, CA_PUBLIC,
     (char *) "Does 'look' obey the TERSE flag?"},
    {(char *) "lnum_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.lnum_compat, 0, 0, CA_PUBLIC,
     (char *) "Is lnum()/lnum2() TinyMUSH/MUX compatible?"},
    {(char *) "name_with_desc",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.name_with_desc, 0, 0, CA_PUBLIC,
     (char *) "Can you see the name of what you look at?"},
    {(char *) "nand_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.nand_compat, 0, 0, CA_PUBLIC,
     (char *) "Use the pre-pl15 nand() behaviour?. (DEPRECATED)"},
    {(char *) "machine_command_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.machinecost, 0, 0, CA_PUBLIC,
     (char *) "Cost of running things from a non-player (1/X)\r\n"\
              "                             Default: 64   Value: %d"},
    {(char *) "mail_anonymous",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mail_anonymous, 30, 0, CA_WIZARD,
     (char *) "Anonymous player name that /anon mail sent has."},
    {(char *) "mail_autodeltime",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.mail_autodeltime, 0, 0, CA_WIZARD,
     (char *) "Default mail autodelete time for messages.\r\n"\
              "          (-1 is inactive)   Default: 21   Value: %d"},
    {(char *) "mail_def_object",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.mail_def_object, 0, 0, CA_WIZARD,
     (char *) "Mail object used for global eval aliases\r\n"\
              "          (-1 is inactive)   Default: -1   Value: %d"},
    {(char *) "mail_default",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mail_default, 0, 0, CA_PUBLIC,
     (char *) "Does 'mail' mimic 'mail/status'?"},
    {(char *) "mail_hidden",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mail_hidden, 0, 0, CA_PUBLIC,
     (char *) "Does mail/anon hide who is sent to?"},
    {(char *) "mail_tolist",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mail_tolist, 0, 0, CA_PUBLIC,
     (char *) "Does 'mail' display a 'To:' list?"},
    {(char *) "mail_verbosity",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.mail_verbosity, 0, 0, CA_WIZARD,
     (char *) "Mail notices show subj. and sent to discon'd players.\r\n"\
              "          (-1 is inactive)   Default: 0    Value: %d"},
    {(char *) "maildelete",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.maildelete, 0, 0, CA_PUBLIC,
     (char *) "Is mail set up for auto-purging?"},
    {(char *) "mailbox_size",
     cf_mailint, CA_GOD | CA_IMMORTAL | CA_WIZARD, &mudconf.mailbox_size, 0, 0, CA_WIZARD,
     (char *) "Current default mailbox size.\r\n"\
              "                             Default: 99   Value: %d"},
    {(char *) "mailinclude_file",
     cf_string, CA_DISABLED, (int *) mudconf.mailinclude_file, 32, 0, CA_WIZARD,
     (char *) "File included when player registers."},
    {(char *) "mailmax_send",
     cf_int, CA_GOD | CA_IMMORTAL | CA_WIZARD, &mudconf.mailmax_send, 0, 0, CA_WIZARD,
     (char *) "Maximum number of players in a single message.\r\n"\
              "                             Default: 100   Value: %d"},
    {(char *) "mailprog",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mailprog, 16, 0, CA_WIZARD,
     (char *) "Program used for autoregistering (Def. elm)."},
    {(char *) "mailsub",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mailsub, 0, 0, CA_WIZARD,
     (char *) "Are subjects included in autoregistration?"},
    {(char *) "master_room",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.master_room, 0, 0, CA_WIZARD,
     (char *) "Dbref# of the master room.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "match_own_commands",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.match_mine, 0, 0, CA_PUBLIC,
     (char *) "Are commands matched on enactor?"},
    {(char *) "max_folders",
     cf_int, CA_GOD | CA_IMMORTAL | CA_WIZARD, &mudconf.mailmax_fold, 0, 0, CA_PUBLIC,
     (char *) "Maximum mail folders allowed.\r\n"\
              "                             Default: 20   Value: %d"},
    {(char *) "max_players",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_players, 0, 0, CA_WIZARD,
     (char *) "Maximum players allowed.\r\n"\
              "   -- '-1' specifies ceiling Default: -1   Value: %d"},
    {(char *) "max_sitecons",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_sitecons, 0, 0, CA_WIZARD,
     (char *) "Maximum connects allowed per unique site.\r\n"\
              "                             Default: 20   Value: %d"},
    {(char *) "maximum_size",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_size, 0, 0, CA_WIZARD,
     (char *) "DB Ceiling for max objects.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "money_lim_off",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.money_limit[0], 0, 0, CA_WIZARD,
     (char *) "Define money for @depower NOSTEAL/NOGIVE.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "money_lim_gm",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.money_limit[1], 0, 0, CA_WIZARD,
     (char *) "Define money for GM level @depower.\r\n"\
              "                             Default: 500   Value: %d"},
    {(char *) "money_lim_arch",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.money_limit[2], 0, 0, CA_WIZARD,
     (char *) "Define money for ARCH level @depower.\r\n"\
              "                             Default: 2000   Value: %d"},
    {(char *) "money_lim_counc",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.money_limit[3], 0, 0, CA_WIZARD,
     (char *) "Define money for Councelor level @depower.\r\n"\
              "                             Default: 5000   Value: %d"},
    {(char *) "money_name_plural",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.many_coins, 32, 0, CA_PUBLIC,
     (char *) "Define plural name for money."},
    {(char *) "money_name_singular",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.one_coin, 32, 0, CA_PUBLIC,
     (char *) "Define singular name for money."},
    {(char *) "motd_file",
     cf_string, CA_DISABLED, (int *) mudconf.motd_file, 32, 0, CA_WIZARD,
     (char *) "File used for motd."},
    {(char *) "motd_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.motd_msg, 1024, 0, CA_WIZARD,
     (char *) "Text used for @motd online."},
    {(char *) "muddb_name",
     cf_string, CA_DISABLED, (int *) mudconf.muddb_name, 32, 0, CA_PUBLIC,
     (char *) "The name used for the RhostMUSH DB's (mail/news)."},
    {(char *) "mud_name",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.mud_name, 32, 0, CA_PUBLIC,
     (char *) "The name used for the RhostMUSH."},
    {(char *) "must_unlquota",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.must_unlquota, 0, 0, CA_WIZARD,
     (char *) "Unlock @quota before giving more?"},
    {(char *) "mux_child_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mux_child_compat, 0, 0, CA_WIZARD,
     (char *) "Does children() behave like MUX's?"},
    {(char *) "mux_lcon_compat",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.mux_lcon_compat, 0, 0, CA_WIZARD,
     (char *) "Does children() behave like MUX's?"},
    {(char *) "tor_localhost",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.tor_localhost, 1000, 0, CA_WIZARD,
     (char *) "This specifies local DNS sites by NAME to do TOR comparisons."},
    {(char *) "tor_paranoid",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.tor_paranoid, 0, 0, CA_WIZARD,
     (char *) "Are TOR Sites aggressively verified on reverse DNS?"},
    {(char *) "news_file",
     cf_string, CA_DISABLED, (int *) mudconf.news_file, 32, 0, CA_WIZARD,
     (char *) "File used for news."},
    {(char *) "news_index",
     cf_string, CA_DISABLED, (int *) mudconf.news_indx, 32, 0, CA_WIZARD,
     (char *) "Index used for news."},
    {(char *) "newuser_file",
     cf_string, CA_DISABLED, (int *) mudconf.crea_file, 32, 0, CA_WIZARD,
     (char *) "File used for new player created."},
    {(char *) "newpass_god",
     cf_int_runtime, CA_GOD | CA_IMMORTAL, (int *) &mudconf.newpass_god, 0, 0, CA_IMMORTAL,
     (char *) "Can you newpassword #1 this reboot?\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "no_startup",
     cf_int_runtime, CA_GOD | CA_IMMORTAL, (int *) &mudconf.no_startup, 0, 0, CA_IMMORTAL,
     (char *) "Are startups de-active?\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "noauth_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.special_list,
     H_NOAUTH, 0, CA_WIZARD,
     (char *) "Specify sites to block AUTH/IDENT lookups."},
    {(char *) "noautoreg_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list,
     H_NOAUTOREG, 0, CA_WIZARD,
     (char *) "Specify sites to block autoregistration."},
    {(char *) "noautoreg_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.autoreg_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Specify site NAMES to block autoregistration."},
    {(char *) "nobroadcast_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.nobroadcast_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "This specifies sites by NAME to not MONITOR."},
    {(char *) "nodns_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.special_list,
     H_NODNS, 0, CA_WIZARD,
     (char *) "Specify sites to block dns lookups."},
    {(char *) "noguest_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list,
     H_NOGUEST, 0, CA_WIZARD,
     (char *) "Specify sites to block guest connections."},
    {(char *) "noguest_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.noguest_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Specify site NAMES to block guest connections"},
    {(char *) "nonindxtxt_maxlines",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.nonindxtxt_maxlines, 0, 0, CA_IMMORTAL,
     (char *) "Maximum lines allowed in non-indx .txt file.\r\n"\
              "                             Default: 200   Value: %d"},
    {(char *) "noregist_onwho",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.noregist_onwho, 0, 0, CA_WIZARD,
     (char *) "Are the 'R's snuffed on DOING/WHO?"},
    {(char *) "noshell_prog",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.noshell_prog, 0, 0, CA_PUBLIC,
     (char *) "Can you issue commands outside a @program?"},
    {(char *) "nospam_connect",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.nospam_connect, 0, 0, CA_WIZARD,
     (char *) "Are forbidden sites logged?"},
    {(char *) "notify_recursion_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.ntfy_nest_lim, 0, 0, CA_WIZARD,
     (char *) "Specify recursion limit on notifies.\r\n"\
              "                             Default: 20   Value: %d"},
    {(char *) "notonerr_return",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.notonerr_return, 0, 0, CA_PUBLIC,
     (char *) "Does #-1 return a '0' on boolean logic?"},
    {(char *) "num_guests",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.max_num_guests, 0, 0, CA_WIZARD,
     (char *) "Maximum number of guests allowed.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "oattr_enable_altname",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.oattr_enable_altname, 0, 0, CA_PUBLIC,
     (char *) "Do o-attribs look at alternate names?"},
    {(char *) "oattr_uses_altname",
     cf_string, CA_GOD, (int *) mudconf.oattr_uses_altname, 31, 0, CA_WIZARD,
     (char *) "Altname used for o-attributes."},
    {(char *) "objid_localtime",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.objid_localtime, 0, 0, CA_WIZARD,
     (char *) "Does objid's use localtime instead of gmtime?"},
    {(char *) "objid_offset",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.objid_offset, 0, 0, CA_WIZARD,
     (char *) "Offset in seconds from GMtime that objid's use.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "offline_reg",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.offline_reg, 0, 0, CA_WIZARD,
     (char *) "Can you autoregister on the connect screen?"},
    {(char *) "old_elist",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.old_elist, 0, 0, CA_WIZARD,
     (char *) "Does elist() double-eval like old versions?"},
    {(char *) "online_reg",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.online_reg, 0, 0, CA_WIZARD,
     (char *) "Can you autoregister from a guest?"},
    {(char *) "open_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.opencost, 0, 0, CA_PUBLIC,
     (char *) "Cost to do an @open.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "output_database",
     cf_string, CA_DISABLED, (int *) mudconf.outdb, 128, 0, CA_WIZARD,
     (char *) "Name of the output database."},
    {(char *) "output_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.output_limit, 0, 0, CA_IMMORTAL,
     (char *) "Limit on output to your port.\r\n"\
              "                             Default: 16384   Value: %d"},
    {(char *) "page_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.pagecost, 0, 0, CA_PUBLIC,
     (char *) "Cost of a page.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "paranoid_allocate",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.paranoid_alloc, 0, 0, CA_WIZARD,
     (char *) "Double check allocated buffers."},
    {(char *) "paranoid_exit_linking",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.paranoid_exit_linking, 0, 0, CA_PUBLIC,
     (char *) "Improve security on unlinked exits?"},
    {(char *) "parentable_control_lock",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.parent_control, 0, 0, CA_PUBLIC,
     (char *) "Do parents follow Locks?"},
    {(char *) "parent_follow",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.parent_follow, 0, 0, CA_PUBLIC,
     (char *) "If you control target do you see entire parent chain?"},
    {(char *) "parent_nest_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.parent_nest_lim, 0, 0, CA_WIZARD,
     (char *) "Maximum nesting allowed on @parents.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "partial_conn",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.partial_conn, 0, 0, CA_PUBLIC,
     (char *) "Partial connects trig master-room @aconnects?"},
    {(char *) "partial_deconn",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.partial_deconn, 0, 0, CA_PUBLIC,
     (char *) "Partial deconn trig master-room @adisconnect?"},
    {(char *) "passproxy_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.suspect_list,
     H_PASSPROXY, 0, CA_WIZARD,
     (char *) "Site list for specifying those who pass proxy checks."},
    {(char *) "passproxy_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.passproxy_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "This specifies sites by NAME to bypass proxy checks."},
    {(char *) "paycheck",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.paycheck, 0, 0, CA_PUBLIC,
     (char *) "Money player receives daily on connect.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "pcreate_paranoia",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.pcreate_paranoia, 0, 0, CA_WIZARD,
     (char *) "Define DoS create paranoia level (0, 1, 2).\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "pemit_far_players",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.pemit_players, 0, 0, CA_PUBLIC,
     (char *) "Can you @pemit remotely to players?"},
    {(char *) "pemit_any_object",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.pemit_any, 0, 0, CA_PUBLIC,
     (char *) "Can you @pemit to anything remotely?"},
    {(char *) "penn_playercmds",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.penn_playercmds, 0, 0, CA_PUBLIC,
     (char *) "Commands on player work only for player?"},
    {(char *) "penn_setq",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.penn_setq, 0, 0, CA_PUBLIC,
     (char *) "Does setq() family mimic penn for labels"},
    {(char *) "penn_switches",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.penn_switches, 0, 0, CA_PUBLIC,
     (char *) "Does switch() understand > and <?"},
    {(char *) "permit_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list, 0, 0, CA_WIZARD,
     (char *) "Site permission for allowing site."},
    {(char *) "player_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.player_dark, 0, 0, CA_PUBLIC,
     (char *) "Can players set themselves/puppets dark?"},
    {(char *) "player_flags",
     cf_set_flags, CA_GOD | CA_IMMORTAL, (int *) &mudconf.player_flags, 0, 0, CA_WIZARD,
     (char *) "Default flags used on new players."},
    {(char *) "player_toggles",
     cf_set_toggles, CA_GOD | CA_IMMORTAL, (int *) &mudconf.player_toggles, 0, 0, CA_WIZARD,
     (char *) "Default toggles used on new players."},
    {(char *) "player_listen",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.player_listen, 0, 0, CA_PUBLIC,
     (char *) "Can players trigger @listen/@hear? DANGEROUS"},
    {(char *) "player_match_own_commands",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.match_mine_pl, 0, 0, CA_PUBLIC,
     (char *) "Players check commands on themselves?"},
    {(char *) "player_match_commands",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.match_pl, 0, 0, CA_PUBLIC,
     (char *) "Commands checked on players?"},
    {(char *) "player_name_spaces",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.name_spaces, 0, 0, CA_PUBLIC,
     (char *) "Spaces allowed in names?"},
    {(char *) "player_queue_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.queuemax, 0, 0, CA_WIZARD,
     (char *) "QUEUE limit for players (Non-Wizards).\r\n"\
              "                             Default: 100   Value: %d"},
    {(char *) "player_quota",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.player_quota, 0, 0, CA_PUBLIC,
     (char *) "Default quota for players.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "player_starting_home",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.start_home, 0, 0, CA_WIZARD,
     (char *) "Default home for new players.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "player_starting_room",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.start_room, 0, 0, CA_WIZARD,
     (char *) "Room new players start in.\r\n"\
              "                             Default: 0   Value: %d"},
#ifdef PLUSHELP
    {(char *) "plushelp_file",
     cf_string, CA_DISABLED, (int *) mudconf.plushelp_file, 32, 0, CA_WIZARD,
     (char *) "Name of +help file."},
    {(char *) "plushelp_index",
     cf_string, CA_DISABLED, (int *) mudconf.plushelp_indx, 32, 0, CA_WIZARD,
     (char *) "Name of +help index."},
#endif
    {(char *) "port",
     cf_int, CA_DISABLED, &mudconf.port, 0, 0, CA_WIZARD,
     (char *) "Port the mush runs on.\r\n"\
              "                             Default: 6250   Value: %d"},
    {(char *) "power_access",
     cf_ntab_access, CA_GOD | CA_IMMORTAL, (int *) powers_nametab,
     (pmath2) access_nametab, (pmath2) access_nametab2, CA_WIZARD,
     (char *) "Change access permissions of a @power."},
    {(char *) "power_objects",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.power_objects, 0, 0, CA_WIZARD,
     (char *) "Can objects have @powers and @depowers?"},
    {(char *) "float_precision",
     cf_verifyint, CA_GOD | CA_IMMORTAL, &mudconf.float_precision, 48, 1, CA_WIZARD,
     (char *) "The decimal placement of float precision for math functions.   Default: 0   Value: %d"},
    {(char *) "protect_addenh",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.protect_addenh, 0, 0, CA_WIZARD,
     (char *) "Will you allow arguments to @protect/add?"},
    {(char *) "proxy_checker",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.proxy_checker, 0, 0, CA_WIZARD,
     (char *) "The Proxy Checker.  0 off, >0 on, 2 block guests, 4 block register.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "public_flags",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.pub_flags, 0, 0, CA_WIZARD,
     (char *) "Players can get flags on anything?"},
    {(char *) "queue_active_chunk",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.active_q_chunk, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "queue_compatible",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.queue_compatible, 0, 0, CA_WIZARD,
     (char *) "Are QUEUES MUSH compatible to go negative (-1)?"},
    {(char *) "queue_idle_chunk",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.queue_chunk, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 3   Value: %d"},
    {(char *) "quiet_look",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.quiet_look, 0, 0, CA_PUBLIC,
     (char *) "Players shown attributes when looking?"},
    {(char *) "quiet_whisper",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.quiet_whisper, 0, 0, CA_PUBLIC,
     (char *) "Are whispers quiet?"},
    {(char *) "quit_file",
     cf_string, CA_DISABLED, (int *) mudconf.quit_file, 32, 0, CA_WIZARD,
     (char *) "File seen when you QUIT/LOGOUT."},
    {(char *) "quotas",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.quotas, 0, 0, CA_PUBLIC,
     (char *) "Are @quotas being used?"},
    {(char *) "rollbackmax",
     cf_verifyint, CA_GOD | CA_IMMORTAL, &mudconf.rollbackmax, 10000, 10, CA_WIZARD,
     (char *) "Max rollback (retry) count allowed.\r\n"\
              "      Range: 10-10000        Default: 1000   Value: %d"},
    {(char *) "rooms_can_open",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.rooms_can_open, 0, 0, CA_PUBLIC,
     (char *) "Are rooms able to use @open/open()?"},
    {(char *) "read_remote_desc",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.read_rem_desc, 0, 0, CA_PUBLIC,
     (char *) "Can you see @descs on remote things?"},
    {(char *) "read_remote_name",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.read_rem_name, 0, 0, CA_PUBLIC,
     (char *) "Can you see names of remote things?"},
    {(char *) "recycling",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.recycle, 0, 0, CA_WIZARD,
     (char *) "Is recyling dbref#'s currently enabled?"},
    {(char *) "register_create_file",
     cf_string, CA_DISABLED, (int *) mudconf.regf_file, 32, 0, CA_WIZARD,
     (char *) "File used for registering."},
    {(char *) "register_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.access_list,
     H_REGISTRATION, 0, CA_WIZARD,
     (char *) "Site permissions for registration."},
    {(char *) "register_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.register_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Site permissions for NAME registration."},
    {(char *) "regtry_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.regtry_limit, 0, 0, CA_WIZARD,
     (char *) "Times a player can try to register per conn.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "restrict_home",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.restrict_home, 0, 0, CA_WIZARD,
     (char *) "Restriction played on 'home' command.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "restrict_home2",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.restrict_home2, 0, 0, CA_WIZARD,
     (char *) "Restriction played on 'home' command (2nd word).\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "restrict_sidefx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.restrict_sidefx, 0, 0, CA_WIZARD,
     (char *) "Restrict sideeffect usage.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "retry_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.retry_limit, 0, 0, CA_WIZARD,
     (char *) "Times a player can attempt to connect.\r\n"\
              "                             Default: 3   Value: %d"},
    {(char *) "robot_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.robotcost, 0, 0, CA_PUBLIC,
     (char *) "Cost of creating a @robot.\r\n"\
              "                             Default: 1000   Value: %d"},
    {(char *) "robot_flags",
     cf_set_flags, CA_GOD | CA_IMMORTAL, (int *) &mudconf.robot_flags, 0, 0, CA_WIZARD,
     (char *) "Flags a robot is set with."},
    {(char *) "robot_toggles",
     cf_set_toggles, CA_GOD | CA_IMMORTAL, (int *) &mudconf.robot_toggles, 0, 0, CA_WIZARD,
     (char *) "Toggles a robot is set with."},
    {(char *) "robot_speech",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.robot_speak, 0, 0, CA_PUBLIC,
     (char *) "Are robots allowed to talk?"},
    {(char *) "room_aconnect",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.room_aconn, 0, 0, CA_WIZARD,
     (char *) "Should rooms trigger aconnects?"},
    {(char *) "room_adisconnect",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.room_adconn, 0, 0, CA_WIZARD,
     (char *) "Should rooms trigger adisconnects?"},
    {(char *) "room_flags",
     cf_set_flags, CA_GOD | CA_IMMORTAL, (int *) &mudconf.room_flags, 0, 0, CA_WIZARD,
     (char *) "Default flags for a new room."},
    {(char *) "room_toggles",
     cf_set_toggles, CA_GOD | CA_IMMORTAL, (int *) &mudconf.room_toggles, 0, 0, CA_WIZARD,
     (char *) "Default toggles for a new room."},
    {(char *) "roomlog_path",
     cf_string, CA_DISABLED, (int *) mudconf.roomlog_path, 128, 0, CA_WIZARD,
     (char *) "Path where LOGROOM logs are sent."},
    {(char *) "room_quota",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.room_quota, 0, 0, CA_WIZARD,
     (char *) "Ammount of quota each room takes.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "rwho_data_port",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.rwho_data_port, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 6889   Value: %d"},
    {(char *) "rwho_dump_interval",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.rwho_interval, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 241   Value: %d"},
    {(char *) "rwho_host",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.rwho_host, 64, 0, CA_WIZARD,
     (char *) NULL},
    {(char *) "rwho_info_port",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.rwho_info_port, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 6888   Value: %d"},
    {(char *) "rwho_password",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.rwho_pass, 32, 0, CA_WIZARD,
     (char *) NULL},
    {(char *) "rwho_transmit",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.rwho_transmit, 0, 0, CA_WIZARD,
     (char *) NULL},
    {(char *) "sacrifice_adjust",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.sacadjust, 0, 0, CA_PUBLIC,
     (char *) "Adjustment applied when sacrificing objects.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "sacrifice_factor",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.sacfactor, 0, 0, CA_PUBLIC,
     (char *) "Numeric factor for sacrificing.\r\n"\
              "                             Default: 5   Value: %d"},
    {(char *) "safe_wipe",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.safe_wipe, 0, 0, CA_PUBLIC,
     (char *) "@wiping SAFE/INDESTRUCTABLE things blocked?"},
    {(char *) "safer_ufun",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.safer_ufun, 0, 0, CA_PUBLIC,
     (char *) "Enforcement of protected u-evals?"},
    {(char *) "safer_passwords",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.safer_passwords, 0, 0, CA_PUBLIC,
     (char *) "Enforcement of harder passwords?"},
    {(char *) "search_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.searchcost, 0, 0, CA_PUBLIC,
     (char *) "Cost of a @search.\r\n"\
              "                             Default: 100   Value: %d"},
    {(char *) "secure_atruselock",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.secure_atruselock, 0, 0, CA_WIZARD,
     (char *) "Attribute USELOCKS require ATRUSE @toggle?"},
    {(char *) "secure_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.secure_dark, 0, 0, CA_WIZARD,
     (char *) "DARK settable by wizards only?"},
    {(char *) "secure_functions",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.secure_functions, 0, 0, CA_WIZARD,
     (char *) "List of functions with added security.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "secure_jumpok",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.secure_jumpok, 0, 0, CA_WIZARD,
     (char *) "Level that the JUMP_OK flag locked down.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "see_owned_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.see_own_dark, 0, 0, CA_PUBLIC,
     (char *) "Can you see dark things you control?"},
    {(char *) "showother_altinv",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.showother_altinv, 0, 0, CA_PUBLIC,
     (char *) "Alt inventories shown when you look?"},
    {(char *) "shs_reverse",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.shs_reverse, 0, 0, CA_PUBLIC,
     (char *) "Are SHS encryption reversed?"},
    {(char *) "sideeffects",
     cf_sidefx, CA_GOD | CA_IMMORTAL, &mudconf.sideeffects, 0, 0, CA_PUBLIC,
     (char *) "Bitmask for enabling sideeffects.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "sidefx_maxcalls",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.sidefx_maxcalls, 0, 0, CA_PUBLIC,
     (char *) "Maximum sideeffect calls per command.\r\n"\
              "                             Default: 1000 Value: %d"},
    {(char *) "sidefx_returnval",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.sidefx_returnval, 0, 0, CA_PUBLIC,
     (char *) "Do creation sideeffects return values?"},
    {(char *) "signal_action",
     cf_option, CA_DISABLED, &mudconf.sig_action,
     (pmath2) sigactions_nametab, 0, CA_WIZARD,
     (char *) "Action taken on receiving signals?"},
    {(char *) "signal_crontab",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.signal_crontab, 0, 0, CA_PUBLIC,
     (char *) "Does signal USR1 optionally read cron file?"},
    {(char *) "space_compress",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.space_compress, 0, 0, CA_PUBLIC,
     (char *) "Spaces compressed?"},
    {(char *) "spam_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.spam_limit, 0, 0, CA_PUBLIC,
     (char *) "Ceiling of commands per minute..\r\n"\
              "                             Default: 60   Value: %d"},
    {(char *) "spam_msg",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.spam_msg, 127, 0, CA_WIZARD,
     (char *) "Message spammer receives when limit reached."},
    {(char *) "spam_objmsg",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.spam_objmsg, 127, 0, CA_WIZARD,
     (char *) "Message spammer receives when object reaches limit."},
    {(char *) "stack_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.stack_limit, 0, 0, CA_PUBLIC,
     (char *) "Ceiling of nested 'command' calls.\r\n"\
              "                             Default: 10000 Value: %d"},
    {(char *) "start_build",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.start_build, 0, 0, CA_WIZARD,
     (char *) "Wanderer flag removed on new players?"},
    {(char *) "starting_money",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.paystart, 0, 0, CA_PUBLIC,
     (char *) "Starting money new player gets.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "starting_quota",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.start_quota, 0, 0, CA_PUBLIC,
     (char *) "Starting quota a new player gets.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "status_file",
     cf_string, CA_DISABLED, (int *) mudconf.status_file, 128, 0, CA_WIZARD,
     (char *) "File where @shutdown commands are sent."},
    {(char *) "sub_include",
     cf_string_sub, CA_GOD|CA_IMMORTAL, (int *) mudconf.sub_include, 26, 0, CA_WIZARD,
     (char *) "Substitutions included for percent-lookups on parser."},
    {(char *) "sub_override",
     cf_chartoint, CA_GOD | CA_IMMORTAL, &mudconf.sub_override, 0, 0, CA_PUBLIC,
     (char *) "Override mask for percent-substitutions.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "suspect_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.suspect_list,
     H_SUSPECT, 0, CA_WIZARD,
     (char *) "Site list for specifying suspects."},
    {(char *) "suspect_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.suspect_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Site NAME list for specifying suspects."},
    {(char *) "sweep_dark",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.sweep_dark, 0, 0, CA_PUBLIC,
     (char *) "Can you sweep dark locations?"},
    {(char *) "switch_default_all",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.switch_df_all, 0, 0, CA_PUBLIC,
     (char *) "Does @switch default to @switch/all?"},
    {(char *) "switch_search",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.switch_search, 0, 0, CA_PUBLIC,
     (char *) "Switch search and searchng execution."},
    {(char *) "switch_substitutions",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.switch_substitutions, 0, 0, CA_PUBLIC,
     (char *) "Does @switch/switch()/switchall() allow #$?"},
    {(char *) "terse_shows_contents",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.terse_contents, 0, 0, CA_PUBLIC,
     (char *) "Do you see contents with the TERSE flag?"},
    {(char *) "terse_shows_exits",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.terse_exits, 0, 0, CA_PUBLIC,
     (char *) "Do you see exits with the TERSE flag?"},
    {(char *) "terse_shows_move_messages",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.terse_movemsg, 0, 0, CA_PUBLIC,
     (char *) "Does the TERSE flag show movement?"},
    {(char *) "thing_flags",
     cf_set_flags, CA_GOD | CA_IMMORTAL, (int *) &mudconf.thing_flags, 0, 0, CA_WIZARD,
     (char *) "Default flags on new things>"},
    {(char *) "thing_toggles",
     cf_set_toggles, CA_GOD | CA_IMMORTAL, (int *) &mudconf.thing_toggles, 0, 0, CA_WIZARD,
     (char *) "Default flags on new things>"},
    {(char *) "thing_quota",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.thing_quota, 0, 0, CA_WIZARD,
     (char *) "Quota that each thing takes up.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *) "timeslice",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.timeslice, 0, 0, CA_WIZARD,
     (char *) "N/A\r\n"\
              "                             Default: 1000   Value: %d"},
    {(char *) "trace_output_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.trace_limit, 0, 0, CA_WIZARD,
     (char *) "Limit on how much output you can have.\r\n"\
              "                             Default: 200   Value: %d"},
    {(char *) "trace_topdown",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.trace_topdown, 0, 0, CA_PUBLIC,
     (char *) "Does trace go top down, or bottom up?"},
    {(char *) "trust_site",
     cf_site, CA_GOD | CA_IMMORTAL, (int *) &mudstate.suspect_list, 0, 0, CA_WIZARD,
     (char *) "Site used to specify trusted people."},
    {(char *) "uncompress_program",
     cf_string, CA_DISABLED, (int *) mudconf.uncompress, 128, 0, CA_WIZARD,
     (char *) "Program used to uncompress database\r\n"\
              "                                 (default compress)"},
    {(char *) "unowned_safe",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.safe_unowned, 0, 0, CA_PUBLIC,
     (char *) "Things you don't own considered SAFE?"},
    {(char *) "user_attr_access",
     cf_modify_bits, CA_GOD | CA_IMMORTAL, &mudconf.vattr_flags,
     (pmath2) attraccess_nametab, 0, CA_WIZARD,
     (char *) "Default permissions for user-attributes."},
    {(char *) "validate_host",
     cf_dynstring, CA_GOD | CA_IMMORTAL, (int *) mudconf.validate_host, LBUF_SIZE-1, 1, CA_WIZARD,
     (char *) "Mail addresses to block from autoreg."},
    {(char *) "vattr_limit_checkwiz",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.vattr_limit_checkwiz, 0, 0, CA_WIZARD,
     (char *) "Are wizards and higher checked for vattr max?"},
    {(char *) "vlimit",
     cf_vint, CA_GOD | CA_IMMORTAL, &mudconf.vlimit, 0, 0, CA_WIZARD,
     (char *) "Maximum user attributes allowed.  Don't touch\r\n"\
              "                             Default: 750   Value: %d"},
    {(char *) "wait_cost",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.waitcost, 0, 0, CA_PUBLIC,
     (char *) "Cost of @waiting.\r\n"\
              "                             Default: 10   Value: %d"},
    {(char *) "wall_cost",
     cf_int, CA_WIZARD, &mudconf.wall_cost, 0, 0, CA_PUBLIC,
     (char *) "Cost of @walling.\r\n"\
              "                             Default: 500   Value: %d"},
    {(char *) "whereis_notify",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.whereis_notify, 0, 0, CA_WIZARD,
     (char *) "Does @whereis by a cloaked wiz notify you?"},
    {(char *) "who_comment",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.who_comment, 0, 0, CA_PUBLIC,
     (char *) "Does the WHO/DOING display cute messages?"},
    {(char *) "who_default",
     cf_who_bool, CA_GOD | CA_IMMORTAL, &mudconf.who_default, 0, 0, CA_PUBLIC,
     (char *) "Is the WHO/DOING like MUSH/MUX?"},
    {(char *) "who_unfindable",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.who_unfindable, 0, 0, CA_PUBLIC,
     (char *) "Unfindable players hidden from WHO/DOING?"},
    {(char *) "whohost_size",
     cf_int, CA_GOD | CA_IMMORTAL, (int *) &mudconf.whohost_size, 0, 0, CA_WIZARD,
     (char *) "Current size of hosts in WHO/DOING.\r\n"\
              "        -- 0 is unlimited    Default: 0   Value: %d"},
    {(char *) "who_showwiz",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.who_showwiz, 0, 0, CA_PUBLIC,
     (char *) "Show a '*' by wizards in WHO/DOING?"},
    {(char *) "who_showwiztype",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.who_showwiztype, 0, 0, CA_PUBLIC,
     (char *) "Show wiz flag by wizards in WHO/DOING?"},
    {(char *) "who_wizlevel",
     cf_int, CA_GOD | CA_IMMORTAL, (int *) &mudconf.who_wizlevel, 0, 0, CA_PUBLIC,
     (char *) "Define level of wiz to show in WHO/DOING.\r\n"\
              "        -- 0 is all wizards  Default: 0   Value: %d"},
    {(char *) "wizmax_vattr_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.wizmax_vattr_limit, 0, 0, CA_WIZARD,
     (char *) "Max times wizard can create new user attribs.\r\n"\
              "                             Default: 1000000 Value: %d"},
    {(char *) "wizmax_dest_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.wizmax_dest_limit, 0, 0, CA_WIZARD,
     (char *) "Max times wizard can @destroy objects/things.\r\n"\
              "                             Default: 100000  Value: %d"},
    {(char *) "wiz_override",
     cf_bool, CA_GOD | CA_IMMORTAL, (int *) &mudconf.wiz_override, 0, 0, CA_WIZARD,
     (char *) "Wizards override locks by default?"},
    {(char *) "wizard_help_file",
     cf_string, CA_DISABLED, (int *) mudconf.whelp_file, 32, 0, CA_WIZARD,
     (char *) "Help file for the wizard help."},
    {(char *) "wizard_help_index",
     cf_string, CA_DISABLED, (int *) mudconf.whelp_indx, 32, 0, CA_WIZARD,
     (char *) "Index file for the wizard help."},
    {(char *) "wizard_motd_file",
     cf_string, CA_DISABLED, (int *) mudconf.wizmotd_file, 32, 0, CA_WIZARD,
     (char *) "Motd file for the wizards."},
    {(char *) "wizard_motd_message",
     cf_string, CA_GOD | CA_IMMORTAL, (int *) mudconf.wizmotd_msg, 1024, 0, CA_WIZARD,
     (char *) "On-line Motd string for the wizards."},
    {(char *) "wizard_queue_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.wizqueuemax, 0, 0, CA_WIZARD,
     (char *) "QUEUE limit for wizards (Architect and higher).\r\n"\
              "                             Default: 1000   Value: %d"},
    {(char *) "wiz_uselock",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.wiz_uselock, 0, 0, CA_WIZARD,
     (char *) "Wizards override uselocks by default?"},
    {(char *) "zone_parents",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.zone_parents, 0, 0, CA_WIZARD,
     (char *) "Zone master attributes are inheritable?"},
    {(char *) "zones_like_parents",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.zones_like_parents, 0, 0, CA_WIZARD,
     (char *) "Do zone members search themselves like @parents?"},
    {(char *) "thing_attr_default",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.thing_defobj, 0, 0, CA_WIZARD,
     (char *) "Default defobj for new things.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "exit_attr_default",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.exit_defobj, 0, 0, CA_WIZARD,
     (char *) "Default defobj for new exits.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "player_attr_default",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.player_defobj, 0, 0, CA_WIZARD,
     (char *) "Default defobj for new players.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "room_attr_default",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.room_defobj, 0, 0, CA_WIZARD,
     (char *) "Default defobj for new rooms.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "thing_parent",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.thing_parent, 0, 0, CA_WIZARD,
     (char *) "Default parent for new things.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "exit_parent",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.exit_parent, 0, 0, CA_WIZARD,
     (char *) "Default parent for new exits.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "player_parent",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.player_parent, 0, 0, CA_WIZARD,
     (char *) "Default parent for new players.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "room_parent",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.room_parent, 0, 0, CA_WIZARD,
     (char *) "Default parent for new rooms.\r\n"\
              "                             Default: -1   Value: %d"},
    {(char *) "signal_object",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.signal_object, 0, 0, CA_WIZARD,
     (char *) "Object that handles signals sent from the shell.\r\n"\
              "                            Default: -1		Value: %d"},
    {(char *) "signal_object_type",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.signal_object_type, 0, 0, CA_WIZARD,
     (char *) "Handler type (function or @trigger) for signal.\r\n"\
              "                            Default: 0		Value: %d"},
    {(char *) "round_kludge",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.round_kludge, 0, 0, CA_PUBLIC,
     (char *) "Kludgy fix for rounding even numbers.\r\n"\
              "                             Default: 0   Value: %d"}, /* [Loki] */
#ifdef SQLITE
    {(char *) "sqlite_query_limit",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.sqlite_query_limit, 0, 0, CA_WIZARD,
     (char *) "Maximum time in seconds that a SQLite query may run.\r\n"\
              "                             Default: 5   Value: %d"},
    {(char *) "sqlite_db_path",
     cf_int, CA_GOD | CA_IMMORTAL, (int*) mudconf.sqlite_db_path, 128, 0, CA_WIZARD,
     (char *) "Path to the directory to store SQLite database files, relative to game/" },
#endif /*SQLITE*/
#ifdef REALITY_LEVELS
    {(char *) "reality_compare",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.reality_compare, 0, 0, CA_WIZARD,
     (char *) "Change how descs are displayed in reality levels.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *) "reality_level",
     cf_rlevel, CA_DISABLED, (int *)&mudconf, 0, 0, CA_WIZARD,
     (char *) "Defined realitylevels."},
    {(char *) "wiz_always_real",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.wiz_always_real, 0, 0, CA_WIZARD,
     (char *) "Wizards always 'real' in reality?"},
    {(char *) "reality_locks",
     cf_bool, CA_GOD | CA_IMMORTAL, &mudconf.reality_locks, 0, 0, CA_WIZARD,
     (char *) "Is @lock/user a reality lock?"},
    {(char *) "reality_locktype",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.reality_locktype, 0, 0, CA_WIZARD,
     (char *) "Type of lock @lock/user is.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *)"def_room_rx",
     cf_int, CA_GOD| CA_IMMORTAL, &mudconf.def_room_rx, 0, 0, CA_WIZARD,
     (char *) "Default room RX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_room_tx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_room_tx, 0, 0, CA_WIZARD,
     (char *) "Default room TX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_player_rx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_player_rx, 0, 0, CA_WIZARD,
     (char *) "Default player RX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_player_tx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_player_tx, 0, 0, CA_WIZARD,
     (char *) "Default player TX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_exit_rx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_exit_rx, 0, 0, CA_WIZARD,
     (char *) "Default exit RX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_exit_tx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_exit_tx, 0, 0, CA_WIZARD,
     (char *) "Default exit TX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_thing_rx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_thing_rx, 0, 0, CA_WIZARD,
     (char *) "Default object RX level.\r\n"\
              "                             Default: 1   Value: %d"},
    {(char *)"def_thing_tx",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.def_thing_tx, 0, 0, CA_WIZARD,
     (char *) "Default object TX level.\r\n"\
              "                             Default: 1   Value: %d"},
#endif /* REALITY_LEVELS */
    {(char *)"allow_fancy_quotes",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.allow_fancy_quotes, 0, 0, CA_WIZARD,
     (char *) "Allow UTF-8 encoded double quotes.\r\n"\
              "                             Default: 0   Value: %d"},
    {(char *)"allow_fullwidth_colon",
     cf_int, CA_GOD | CA_IMMORTAL, &mudconf.allow_fullwidth_colon, 0, 0, CA_WIZARD,
              "Allow UTF-8 encoded full width colon character.\r\n"\
              "                             Default: 0   Value: %d"},
    {NULL,
     NULL, 0, NULL, 0}};

/* ---------------------------------------------------------------------------
 * cf_set: Set config parameter.
 */

int
cf_set(char *cp, char *ap, dbref player)
{
    CONF *tp;
    int i;
    char *buff;

    /* Search the config parameter table for the command.
       If we find it, call the handler to parse the argument. */

    for (tp = conftable; tp->pname; tp++) {
	if (!strcmp(tp->pname, cp)) {
	    if (!mudstate.initializing &&
		!check_access(player, tp->flags, 0, 0)) {
		notify(player,
		       "Permission denied.");
		return (-1);
	    }
	    if (!mudstate.initializing) {
		buff = alloc_lbuf("cf_set");
		strcpy(buff, ap);
	    }
	    i = tp->interpreter(tp->loc, ap, tp->extra, tp->extra2, player, cp);
            if ( i != 777) {
	       if (!mudstate.initializing) {
		   STARTLOG(LOG_CONFIGMODS, "CFG", "UPDAT")
		       log_name(player);
		   log_text((char *) " entered config directive: ");
		   log_text(cp);
		   log_text((char *) " with args '");
		   log_text(buff);
		   log_text((char *) "'.  Status: ");
		   switch (i) {
		   case 0:
		       log_text((char *) "Success.");
		       break;
		   case 1:
		       log_text((char *) "Partial success.");
		       break;
		   case -1:
		       log_text((char *) "Failure.");
		       break;
                   case -2:
		       log_text((char *) "Success (Resetting).");
		       break;
                   case -3:
		       log_text((char *) "Success (Viewing).");
		       break;
		   default:
		       log_text((char *) "Strange.");
		   }
		   ENDLOG
		       free_lbuf(buff);
	       }
            } else {
               i = 0;
            }
	    return i;
	}
    }

    /* Config directive not found.  Complain about it. */

    cf_log_notfound(player, (char *) "Set", "Config directive", cp);
    return (-1);
}

/* ---------------------------------------------------------------------------
 * cf_read: Read in config parameters from named file
 */

int
cf_read(char *fn)
{
    int retval;

    if ( strcmp(fn, "rhost_vattr.conf") == 0 ) {
       mudstate.initializing = -1;
    } else {
       mudstate.initializing = 1;
    }
    retval = cf_include(NULL, fn, 0, 0, 0, (char *) "init");
    mudstate.initializing = 0;

    /* Fill in missing DB file names */

    if (!*mudconf.outdb) {
	strcpy(mudconf.outdb, mudconf.indb);
	strcat(mudconf.outdb, ".out");
    }
    if (!*mudconf.crashdb) {
	strcpy(mudconf.crashdb, mudconf.indb);
	strcat(mudconf.crashdb, ".CRASH");
    }
    if (!*mudconf.gdbm) {
	strcpy(mudconf.gdbm, mudconf.indb);
	strcat(mudconf.gdbm, ".gdbm");
    }
    return retval;
}

/* ---------------------------------------------------------------------------
 * do_admin: Command handler to set config params at runtime */

void
do_admin(dbref player, dbref cause, int extra, char *kw, char *value)
{
    int i, i_tval, i_cntr, i_cntr2, atr, aflags, i_found;
    dbref aowner;
    char *tbuf, *sbuf, *tprbuff, *tprpbuff, *tbuf2, *tb, *tb2;
    FILE *fp;
    ATTR *aname;
    CONF *tp;

    tprpbuff = tprbuff = alloc_lbuf("do_admin_tpr");
    switch( extra ) {
       case ADMIN_LOAD: /* Load parameters into defined dbref# */
          if ( Good_chk(mudconf.admin_object) && Immortal(Owner(mudconf.admin_object)) ) {
             if ( (fp = fopen("rhost_ingame.conf", "r")) != NULL ) {
                tbuf = alloc_lbuf("admin_load");
                sbuf = alloc_sbuf("admin_load");
                notify_quiet(player, "@admin: Loading config parameters to object defined in admin_object param.");
                sprintf(tbuf, "#%d/_LINE*", mudconf.admin_object);
                do_wipe(player, player, (SIDEEFFECT), tbuf);
                i_cntr = 0;
                while ( !feof(fp) ) {
                   if ( i_cntr > 999 ) {
                      notify_quiet(player, "@admin: can not have more than 1000 entries in rhost_ingame.conf.  Sorry.  Ignoring rest.");
                      break;
                   }
                   fgets(tbuf, LBUF_SIZE, fp);
                   if ( feof(fp) )
                      break;
                   if ( !*tbuf || (*tbuf == '\r') || (*tbuf == '\n') ) {
                      strcpy(tbuf, (char *)"#");
                   } else {
                      if ( (tbuf[strlen(tbuf)-1] == '\r') ||
                           (tbuf[strlen(tbuf)-1] == '\n') )
                         tbuf[strlen(tbuf)-1]='\0';
                   }
                   sprintf(sbuf, "_LINE%d", i_cntr);
                   atr = mkattr(sbuf);
                   if ( atr < 0 ) {
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: error attempting to create attribute %s.  Aborting.", sbuf));
                      break;
                   }
                   aname = atr_num3(atr);
                   if ( !aname ) {
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: error attempting to create attribute %s.  Aborting.", sbuf));
                      break;
                   }
                   atr_add_raw(mudconf.admin_object, aname->number, tbuf);
                   i_cntr++;
                }
                notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: completed loading %d lines onto object #%d.", i_cntr, mudconf.admin_object));
                free_lbuf(tbuf);
                free_sbuf(sbuf);
                fclose(fp);
             } else {
                notify_quiet(player, "@admin: There is no 'rhost_ingame.conf' file in your game directory.");
             }
          } else {
             notify_quiet(player, "@admin: Invalid object defined in 'admin_object'.");
          }
          break;
       case ADMIN_SAVE: /* Save parameters into defined dbref# */
          if ( Good_chk(mudconf.admin_object) && Immortal(Owner(mudconf.admin_object)) ) {
             if ( (fp = fopen("rhost_ingame.conf", "w")) != NULL ) {
                i_cntr = i_cntr2 = 0;
                tbuf = alloc_lbuf("admin_save");
                tbuf2 = alloc_lbuf("admin_save");
                sbuf = alloc_sbuf("admin_save");
                while ( 1 ) { /* Go until line not found */
                   if ( i_cntr2 > 999 ) {
                      notify_quiet(player, "@admin: can not have more than 1000 entries in rhost_ingame.conf.  Sorry.  Ignoring rest.");
                      break;
                   }
                   sprintf(sbuf, "_LINE%d", i_cntr2);
                   atr = mkattr(sbuf);
                   if ( atr < 0 ) {
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: error attempting to create attribute %s.  Aborting.", sbuf));
                      break;
                   }
                   aname = atr_num3(atr);
                   if ( !aname ) {
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: error attempting to create attribute %s.  Aborting.", sbuf));
                      break;
                   }
                   memset(tbuf, '\0', LBUF_SIZE);
                   atr_get_str(tbuf, mudconf.admin_object, aname->number, &aowner, &aflags);
                   if ( !*tbuf ) {
                      /* Empty line -- assume we want to end here */
                      break;
                   }
                   if ( *tbuf != '#' ) {
                      tb = tbuf;
                      tb2 = tbuf2;
                      while ( *tb && !isspace(*tb) ) {
                         *(tb2++) = *(tb++);
                      }
                      *tb2 = '\0';
                      i_found = 0;
                      if ( !strcmp(tbuf2, "newpass_god") || !strcmp(tbuf2, "include") ) {
                         /* Do not allow putting newpass_god into this */
                         /* Do not allow putting include into this */
                         i_found = 0;
                      } else {
                         for (tp = conftable; tp->pname; tp++) {
                            if (!strcmp(tp->pname, tbuf2)) {
                               if ( !(tp->flags & CA_DISABLED) ) {
                                  i_found = 1;
                               } else {
                                  i_found = -1;
                               }
                               break;
                            }
                         }
                      }
                   } else {
                      i_found = 1;
                   }
                   if ( i_found != 1 ) {
                      i_cntr2++;
                      tprpbuff = tprbuff;
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: skipping %s config option '%s' in attribute '_LINE%d'.", 
                                   (!i_found ? "invalid" : "restricted"), tbuf2, i_cntr2-1));
                      continue;
                   }
                   fprintf(fp, "%s\n", tbuf);
                   i_cntr++;
                   i_cntr2++;
                }
                notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: completed writing %d (of %d) lines into rhost_ingame.conf.", i_cntr, i_cntr2));
                free_sbuf(sbuf);
                free_lbuf(tbuf);
                free_lbuf(tbuf2);
                fclose(fp);
             } else {
                notify_quiet(player, "@admin: unable to open 'rhost_ingame.conf' file in your game directory.");
             }
          } else {
             notify_quiet(player, "@admin: Invalid object defined in 'admin_object'.");
          }
          break;
       case ADMIN_EXECUTE: /* Execute parameters saved in file */
          if ( Good_chk(mudconf.admin_object) && Immortal(Owner(mudconf.admin_object)) ) {
             if ( (fp = fopen("rhost_ingame.conf", "r")) != NULL ) {
                i_cntr = 0;
                tbuf = alloc_lbuf("admin_save");
                while ( !feof(fp) ) {
                   fgets(tbuf, LBUF_SIZE, fp);
                   if ( !feof(fp) )
                      i_cntr++;
                }
                free_lbuf(tbuf);
                fclose(fp);
                cf_read("rhost_ingame.conf");
                notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "@admin: executed %d lines from rhost_ingame.conf", i_cntr));
             } else {
                notify_quiet(player, "@admin: unable to open 'rhost_ingame.conf' file in your game directory.");
             }
          } else {
             notify_quiet(player, "@admin: Invalid object defined in 'admin_object'.");
          }
          break;
       case ADMIN_LIST: /* List the params in the file */
          if ( Good_chk(mudconf.admin_object) && Immortal(Owner(mudconf.admin_object)) ) {
             if ( (fp = fopen("rhost_ingame.conf", "r")) != NULL ) {
                i_cntr = 0;
                tbuf = alloc_lbuf("admin_save");
                notify_quiet(player, unsafe_tprintf("@admin: listing config params in 'rhost_ingame.conf' [dbref #%d]:", mudconf.admin_object));
                while ( !feof(fp) ) {
                   fgets(tbuf, LBUF_SIZE, fp);
                   if ( !feof(fp) ) {
                      tprpbuff = tprbuff;
                      if ( (tbuf[strlen(tbuf)-1] == '\r') ||
                           (tbuf[strlen(tbuf)-1] == '\n') )
                         tbuf[strlen(tbuf)-1]='\0';
                      notify_quiet(player, safe_tprintf(tprbuff, &tprpbuff, "   %04d : %s", i_cntr, tbuf));
                      i_cntr++;
                   }
                }
                free_lbuf(tbuf);
                notify_quiet(player, "@admin: listing completed.");
             } else {
                notify_quiet(player, "@admin: unable to open 'rhost_ingame.conf' file in your game directory.");
             }
          } else {
             notify_quiet(player, "@admin: Invalid object defined in 'admin_object'.");
          }
          break;
       default: /* Normal set/unset of values */
          i_tval = 0;
          if ( strchr(kw, '!') )
             i_tval = 1;
          i = cf_set(kw, value, player);
          if ((i >= 0) && !Quiet(player))
	      if ( i_tval )
	         notify(player, "Cleared.");
	      else
	         notify(player, "Set.");
          else if (i == -1 && !Quiet(player))
              notify(player, "Failure.");
          else if (i == -2 && !Quiet(player))
              notify(player, "Set. (redefined)");
          break;
    }
    free_lbuf(tprbuff);
    return;
}


/* ---------------------------------------------------------------------------
 * list_cf_access: List access to config directives.
 */

void
list_cf_access(dbref player, char *s_mask, int key)
{
    CONF *tp;
    char *buff;

   if ( key ) {
       notify(player, "--- Config Permissions (wildmatched) ---");
    } else
       notify(player, "--- Config Permissions ---");
    buff = alloc_mbuf("list_cf_access");
    for (tp = conftable; tp->pname; tp++) {
	if (God(player) || check_access(player, tp->flags, 0, 0)) {
            if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)tp->pname)) ) {
	       sprintf(buff, "%s:", tp->pname);
	       listset_nametab(player, access_nametab, 0, tp->flags, 0,
			       buff, 1);
            }
	}
    }
    free_mbuf(buff);
}

/* Idea taken from TinyMUSH 3.0 */
void list_options_boolean(dbref player, int p_val, char *s_val)
{
   CONF *tp;
   int cntr, t_pages;

   cntr = 0;
   for (tp = conftable; tp->pname; tp++) {
      if (((tp->interpreter == cf_bool)) &&
          (check_access(player, tp->flags2, 0, 0))) {
      cntr++;
      }
   }
   t_pages = cntr / 20 + 1;
   if ( !(cntr % 10) )
      t_pages = t_pages - 1;
   if (p_val > t_pages)
      p_val = t_pages;

   cntr = 0;
   for (tp = conftable; tp->pname; tp++) {
      if (((tp->interpreter == cf_bool)) &&
          (check_access(player, tp->flags2, 0, 0))) {
         if ( s_val && *s_val ) {
            if ( !quick_wild(s_val, tp->pname) ) {
               continue;
            }
         } else if ( !s_val && ((cntr < (20 * (p_val-1))) || (cntr >= (20 * p_val))) ) {
            cntr++;
            continue;
         }
         raw_notify(player, unsafe_tprintf("%-28.28s %c %s",
                            tp->pname, (*(tp->loc) ? 'Y' : 'N'),
                            (tp->extrach ? tp->extrach : "N/A")), 0, 1);
         cntr++;
      }
   }
   if ( !s_val )
      raw_notify(player, unsafe_tprintf("--- Page %d of %d ---", p_val, t_pages), 0, 1);
}

void list_options_values(dbref player, int p_val, char *s_val)
{
   CONF *tp;
   int cntr, t_pages;
   static char mybuff[1000];

   cntr = 0;
   for (tp = conftable; tp->pname; tp++) {
      if (((tp->interpreter == cf_int) ||
           (tp->interpreter == cf_int_runtime) ||
           (tp->interpreter == cf_mailint) ||
           (tp->interpreter == cf_vint) ||
           (tp->interpreter == cf_chartoint) ||
           (tp->interpreter == cf_recurseint)) &&
          (check_access(player, tp->flags2, 0, 0))) {
      cntr++;
      }
   }
   t_pages = cntr / 10 + 1;
   if ( !(cntr % 10) )
      t_pages = t_pages - 1;
   if (p_val > t_pages)
      p_val = t_pages;

   cntr = 0;
   memset(mybuff, 0, sizeof(mybuff));
   for (tp = conftable; tp->pname; tp++) {
      if (((tp->interpreter == cf_int) ||
           (tp->interpreter == cf_int_runtime) ||
           (tp->interpreter == cf_mailint) ||
           (tp->interpreter == cf_vint) ||
           (tp->interpreter == cf_chartoint) ||
           (tp->interpreter == cf_recurseint)) &&
          (check_access(player, tp->flags2, 0, 0))) {
         if ( s_val && *s_val ) {
            if ( !quick_wild(s_val, tp->pname) ) {
               continue;
            }
         } else if ( !s_val && ((cntr < (10 * (p_val-1))) ||
                      (cntr >= (10 * p_val))) ) {
            cntr++;
            continue;
         }
         sprintf(mybuff, "%-28.28s %.900s", tp->pname, tp->extrach);
         raw_notify(player, unsafe_tprintf(mybuff, *(tp->loc)), 0, 1);
         cntr++;
      }
   }
   if ( !s_val )
      raw_notify(player, unsafe_tprintf("--- Page %d of %d---", p_val, t_pages), 0, 1);
}

/*---------------------------------------------------------------------------
 * cf_display: Given a config parameter by name, return its value in some
 * sane fashion.
 */

void cf_display(dbref player, char *param_name, int key, char *buff, char **bufc)
{
    CONF *tp;
    int first, bVerboseSideFx = 0, i_wild = 0;
    static char tempbuff[LBUF_SIZE/2];
    char *t_pt, *t_buff;

    if ( *param_name && ((strchr(param_name, '*') != NULL) || (strchr(param_name, '*') != NULL)) ) {
       i_wild = 1;
    }
    if ( key || i_wild || (*param_name == '1') || (*param_name == '0') ) {
       first = 0;
       t_pt = t_buff = alloc_lbuf("cf_display");
       for (tp = conftable; tp->pname; tp++) {
           if (check_access(player, tp->flags2, 0, 0)) {
              if ( !i_wild || (i_wild && quick_wild(param_name, tp->pname)) ) {
                 if ( first )
                    safe_chr(' ', buff, bufc);
                 if ( strlen(buff) > (LBUF_SIZE - 100) ) {
                    if (*param_name == '1')
                       break;
                    safe_str(tp->pname, t_buff, &t_pt);
                    safe_chr(' ', t_buff, &t_pt);
                 } else {
                    safe_str(tp->pname, buff, bufc);
                 }
                 first = 1;
              }
           }
       }
       if ( *t_buff )
          notify(player, t_buff);
       free_lbuf(t_buff);
    } else  if ( stricmp(param_name, (char *)"lbuf_size") == 0 ) {
       sprintf(tempbuff, "%d", LBUF_SIZE);
       safe_str(tempbuff, buff, bufc);
    } else  if ( stricmp(param_name, (char *)"mbuf_size") == 0 ) {
       sprintf(tempbuff, "%d", MBUF_SIZE);
       safe_str(tempbuff, buff, bufc);
    } else  if ( stricmp(param_name, (char *)"sbuf_size") == 0 ) {
       sprintf(tempbuff, "%d", SBUF_SIZE);
       safe_str(tempbuff, buff, bufc);
    } else {
       if (stricmp(param_name, "sideeffects_txt") == 0) {
	 param_name[11] = '\0';
	 bVerboseSideFx = 1;
       }

       for (tp = conftable; tp->pname; tp++) {
           if (!stricmp(tp->pname, param_name)) {
               if (!check_access(player, tp->flags2, 0, 0)) {
                   safe_str("#-1 PERMISSION DENIED", buff, bufc);
                   return;
               }
               if ( (tp->interpreter == cf_int) ||
                    (tp->interpreter == cf_verifyint) ||
                    (tp->interpreter == cf_verifyint_mysql) ||
                    (tp->interpreter == cf_bool) ||
                    (tp->interpreter == cf_who_bool) ||
                    (tp->interpreter == cf_int_runtime) ||
                    (tp->interpreter == cf_mailint) ||
                    (tp->interpreter == cf_vint) ||
                    (tp->interpreter == cf_chartoint) ||
                    (tp->interpreter == cf_recurseint) ||
		    (tp->interpreter == cf_sidefx && !bVerboseSideFx)) {

                   sprintf(tempbuff, "%d", *(tp->loc));
                   safe_str(tempbuff, buff, bufc);
                   return;
               } else if ( (tp->interpreter == cf_string) ||
                         (tp->interpreter == cf_atrperms) ||
                         (tp->interpreter == cf_string_sub) ||
                         (tp->interpreter == cf_string_chr) ||
                         (tp->interpreter == cf_dynstring) ||
                         (tp->interpreter == cf_dynguest) ||
			 (tp->interpreter == cf_sidefx && bVerboseSideFx)) {
    		   if (tp->interpreter == cf_sidefx) {
		     sideEffectMaskToString(*tp->loc, buff, bufc);
		   } else {
		     safe_str((char *)tp->loc, buff, bufc);
		   }
		 return;
               }
               safe_str("#-1 PERMISSION DENIED", buff, bufc);
               return;
           }
       }
       safe_str("#-1 NOT FOUND", buff, bufc);
   }
}

#ifdef _0_
// Incomplete code - Lensman
//    More complex than I'd first thought :P
CMD_ONE_ARG(dump_config) {

  FILE *pConfFile;
  CONF *pConf;

  if (!Good_obj(player)) {
    return;
  }

  if (!God(player)) {
    notify(player, "Permission Denied.");
  } else if (!message || *messages == '\0') {
    notify(player, "No filename specified");
  } else {
    pConfFile = fopen(message, "w");
    if (!pConfFile) {
      notify(player, "Could not open file for writing.");
    } else {
      fprintf("# RhostMUSH config file.\n");
      fprintf("# Autogenerated by %s at %s.\n", Name(player), ctime(&mudstate.now));
      fprintf("#\n");
      for (pConf = conftable, i = 0 ; pConf[i].pname != NULL ; i++) {
	fprintf("# Param: %s\n#   %s\n%s\t",
		pConf[i].pname,
		(!pConf[i].extrach || !*pConf[i].extrach) ? "No description" : pConf[i].extrach,
		pConf[i].pname);
	switch
      }
    }
  }
}
#endif

#endif /* STANDALONE */
