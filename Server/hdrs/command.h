/* command.h - declarations used by the command processor */

#include "copyright.h"

#ifndef _M_COMMAND_H
#define _M_COMMAND_H

#include "db.h"

#define CMD_NO_ARG(name) \
    extern void FDECL(name, (dbref, dbref, int))
#define CMD_ONE_ARG(name) \
    extern void FDECL(name, (dbref, dbref, int, char *))
#define CMD_ONE_ARG_CMDARG(name) \
    extern void FDECL(name, (dbref, dbref, int, char *, char *[], int))
#define CMD_TWO_ARG(name) \
    extern void FDECL(name, (dbref, dbref, int, char *, char *))
#define CMD_TWO_ARG_SEP(name) \
    extern void FDECL(name, (dbref, dbref, int, int, char *, char *))
#define CMD_TWO_ARG_CMDARG(name) \
    extern void FDECL(name, (dbref, dbref, int, char *, char *, char*[], int))
#define CMD_TWO_ARG_ARGV(name) \
    extern void FDECL(name, (dbref, dbref, int, char *, char *[], int))
// player, cause, key, expr, args[], nArgs, cargs[], nCargs
#define CMD_TWO_ARG_ARGV_CMDARG(name) \
    extern void FDECL(name, (dbref, dbref, int, char *, char *[], int, char*[], int))
// Like above, but passing switches
#define CMD_TWO_ARG_ARGV_CMDARG_SWITCHES(name) \
    extern void FDECL(name, (dbref, dbref, char *, char *, char *[], int, char*[], int))

/* Command function handlers */
CMD_ONE_ARG_CMDARG(do_apply_marked);	/* Apply command to marked objects */
CMD_TWO_ARG(do_admin);			/* Change config parameters */
CMD_TWO_ARG_CMDARG(do_aflags);
CMD_TWO_ARG(do_alias);			/* Change the alias of something */
CMD_TWO_ARG(do_areg);
CMD_TWO_ARG(do_attribute);		/* Manage user-named attributes */
CMD_ONE_ARG(do_blacklist);		/* Load/Clear/List blacklist.txt file */
CMD_ONE_ARG(do_boot);			/* Force-disconnect a player */
CMD_TWO_ARG_CMDARG(do_break);
CMD_TWO_ARG_CMDARG(do_assert);
CMD_ONE_ARG(do_progreset);		/* Force reset of the program prompt */
CMD_NO_ARG(do_buff_free);
CMD_TWO_ARG(do_cmdquota);		/* Establish command quota for target player */
CMD_ONE_ARG(do_channel);
CMD_NO_ARG(do_conncheck);
CMD_TWO_ARG(do_chown);			/* Change object or attribute owner */
CMD_TWO_ARG(do_chownall);		/* Give away all of someone's objs */
CMD_TWO_ARG(do_clone);			/* Create a copy of an object */
CMD_TWO_ARG(do_com);
CMD_NO_ARG(do_comment);			/* Ignore argument and do nothing */
CMD_TWO_ARG(do_convert);
CMD_TWO_ARG_ARGV(do_cpattr);
CMD_TWO_ARG(do_create);			/* Create a new object */
CMD_ONE_ARG(do_cut);			/* Truncate contents or exits list */
CMD_NO_ARG(do_dbck);			/* Consistency check */
CMD_NO_ARG(do_dbclean);			/* Clean unused attributes -- piggy as shit */
CMD_TWO_ARG(do_decomp);			/* Reproduce commands to recrete obj */
CMD_ONE_ARG(do_destroy);		/* Destroy an object */
CMD_TWO_ARG_ARGV(do_dig);		/* Dig a new room */
CMD_ONE_ARG(do_doing);			/* Set doing string in WHO report */
CMD_TWO_ARG_CMDARG(do_dolist);		/* Iterate command on list members */
CMD_TWO_ARG_ARGV(do_door);
CMD_ONE_ARG(do_drop);			/* Drop an object */
CMD_ONE_ARG(do_dump);			/* Dump the database */
CMD_TWO_ARG(do_dynhelp);                /* Dynamic help command */
CMD_ONE_ARG(do_pipe);			/* Pipe output to attribute */
CMD_TWO_ARG_ARGV(do_edit);		/* Edit one or more attributes */
CMD_ONE_ARG(do_enter);			/* Enter an object */
CMD_ONE_ARG(do_entrances);		/* List exits and links to loc */
CMD_ONE_ARG(do_eval);			/* Like comment, but eval the arg */
CMD_ONE_ARG(do_examine);		/* Examine an object */
CMD_TWO_ARG(do_extansi);		/* Allow extended ansi in @ansiname */
CMD_ONE_ARG(do_find);			/* Search for name in database */
CMD_TWO_ARG(do_fixdb);			/* Database repair functions */
CMD_TWO_ARG(do_flagstuff);
CMD_TWO_ARG(do_flagdef);                /* Set/unset/see/list paramaters for flags */
CMD_TWO_ARG(do_totem);                /* Set/unset/see/list paramaters for flags */
CMD_TWO_ARG_ARGV(do_teleport);                /* Teleport elsewhere */
CMD_TWO_ARG_CMDARG(do_force);		/* Force someone to do something */
CMD_ONE_ARG_CMDARG(do_force_prefixed);	/* #<num> <cmd> variant of FORCE */
CMD_ONE_ARG(do_freeze);			/* FREEZE a frozen queue entry */
CMD_TWO_ARG(do_function);		/* Make iser-def global function */
CMD_ONE_ARG(do_get);			/* Get an object */
CMD_TWO_ARG(do_give);			/* Give something away */
CMD_ONE_ARG(do_goto);
CMD_TWO_ARG_ARGV(do_grep);
CMD_ONE_ARG(do_global);			/* Enable/disable global flags */
CMD_ONE_ARG(do_halt);			/* Remove commands from the queue */
CMD_ONE_ARG(do_help);			/* Print info from help files */
CMD_ONE_ARG(do_hide);			/* Hide/Unhide from WHO */
CMD_ONE_ARG(do_hook);			/* Warp various timers */
CMD_TWO_ARG_ARGV(do_icmd);
CMD_TWO_ARG_ARGV_CMDARG(do_include);	/* @include attribute into command */
CMD_TWO_ARG_ARGV_CMDARG(do_rollback);	/* @rollback attribute into command */
CMD_NO_ARG(do_inventory);		/* Print what I am carrying */
CMD_TWO_ARG_CMDARG(do_jump);
CMD_TWO_ARG(do_kill);			/* Kill something */
CMD_TWO_ARG(do_label);			/* %_ label adding/removing of attribs */
CMD_ONE_ARG(do_last);			/* Get recent login info */
CMD_TWO_ARG(do_log);			/* Get recent login info */
CMD_NO_ARG(do_logrotate);		/* Rotate or find status of current logfile */
CMD_NO_ARG(do_leave);			/* Leave the current object */
CMD_TWO_ARG_ARGV(do_limit);
CMD_TWO_ARG(do_link);			/* Set home, dropto, or dest */
CMD_ONE_ARG(do_list);			/* List contents of internal tables */
CMD_ONE_ARG(do_list_file);		/* List contents of message files */
CMD_TWO_ARG(do_lock);			/* Set a lock on an object */
CMD_ONE_ARG(do_look);			/* Look here or at something */
CMD_TWO_ARG(do_lset);                    /* Set flags on locks */
/* SENSES */
CMD_ONE_ARG(do_listen);			/* SENSES - listen here or at something */
CMD_ONE_ARG(do_touch);			/* SENSES - touch location or target */
CMD_ONE_ARG(do_taste);			/* SENSES - taste location or target */
CMD_ONE_ARG(do_smell);			/* SENSES - smell location or target */
/* news system */
CMD_TWO_ARG(do_news);
CMD_TWO_ARG(do_newsdb);
/* MMMail */
CMD_TWO_ARG(do_mail);
CMD_ONE_ARG(do_single_mail);
CMD_TWO_ARG(do_wmail);
CMD_TWO_ARG(do_mail2);
CMD_TWO_ARG(do_program);		/* MUX-compatible @program */
CMD_ONE_ARG(do_protect);		/* @protect name convention(s) */
CMD_ONE_ARG(do_purge);
CMD_ONE_ARG(do_reclist);
CMD_ONE_ARG(do_recover);
CMD_NO_ARG(do_markall);			/* Mark or unmark all objects */
CMD_ONE_ARG(do_motd);			/* Set/list MOTD messages */
CMD_ONE_ARG(do_money);                  /* Allows someone with UNLIMITED money to specify what money() shows */
CMD_ONE_ARG(do_move);			/* Move about using exits */
CMD_TWO_ARG_ARGV(do_mvattr);		/* Move attributes on object */
CMD_TWO_ARG(do_mudwho);			/* WHO for inter-mud page/who suppt */
CMD_TWO_ARG(do_name);			/* Change the name of something */
CMD_TWO_ARG(do_newpassword);		/* Change passwords */
CMD_TWO_ARG(do_notify);			/* Notify or drain semaphore */
CMD_ONE_ARG(do_nuke);
CMD_TWO_ARG_ARGV(do_open);		/* Open an exit */
CMD_TWO_ARG(do_page);			/* Send message to faraway player */
CMD_ONE_ARG(do_page_one);
CMD_TWO_ARG(do_parent);			/* Set parent field */
CMD_TWO_ARG(do_zone);			/* handle zone stuff */
CMD_TWO_ARG(do_password);		/* Change my password */
CMD_TWO_ARG(do_pcreate);		/* Create new characters */
CMD_TWO_ARG_CMDARG(do_pemit);			/* Messages to specific player */
CMD_ONE_ARG(do_poor);			/* Reduce wealth of all players */
CMD_TWO_ARG(do_power);
CMD_TWO_ARG(do_depower);
CMD_ONE_ARG(do_ps);			/* List contents of queue */
CMD_ONE_ARG(do_queue);			/* Force queue processing */
CMD_ONE_ARG(do_quitprogram);		/* MUX-compatible @quitprogram */
CMD_TWO_ARG_ARGV(do_quota);		/* Set or display quotas */
CMD_NO_ARG(do_readcache);		/* Reread text file cache */
CMD_NO_ARG(do_reboot);
CMD_TWO_ARG(do_register);
CMD_TWO_ARG_CMDARG(do_remote); /* Run commands at remote location */
CMD_NO_ARG(do_rwho);			/* Open or close conn to rem RWHO */
CMD_ONE_ARG(do_say);			/* Messages to all */
CMD_NO_ARG(do_score);			/* Display my wealth */
CMD_ONE_ARG(do_search);			/* Search for objs matching criteria */
CMD_ONE_ARG(do_search_for_players);
CMD_ONE_ARG(do_selfboot);
CMD_TWO_ARG(do_set);			/* Set flags or attributes */
CMD_TWO_ARG(do_toggle);			/* Set flags or attributes */
CMD_TWO_ARG(do_api);			/* Set flags or attributes */
CMD_NO_ARG(do_tor);			/* TOR cache manipulation */
CMD_TWO_ARG_SEP(do_setattr);		/* Set object attribute */
CMD_TWO_ARG(do_setvattr);		/* Set variable attribute */
CMD_TWO_ARG(do_setvattr_cluster);	/* Set variable attribute on cluster */
CMD_ONE_ARG(do_shutdown);		/* Stop the game */
CMD_TWO_ARG(do_site);
//CMD_TWO_ARG_CMDARG(do_skip);		/* @skip command if boolean true */
CMD_TWO_ARG_ARGV_CMDARG(do_skip);	/* @skip command if boolean true */
CMD_TWO_ARG(do_snapshot);
CMD_ONE_ARG(do_stats);			/* Display object type breakdown */
CMD_TWO_ARG_CMDARG(do_sudo);		/* @sudo someone to do something */
CMD_ONE_ARG(do_sweep);			/* Check for listeners */
CMD_TWO_ARG_ARGV_CMDARG(do_switch);	/* Execute cmd based on match */
CMD_TWO_ARG(do_tag);			/* Manage object tags */
CMD_TWO_ARG(do_ltag);			/* Manage object tags */
CMD_ONE_ARG(do_thaw);			/* THAW out a frozen queue entry */
CMD_ONE_ARG(do_think);			/* Think command (ie: @pemit me=message) */
CMD_ONE_ARG(do_timewarp);		/* Warp various timers */
CMD_TWO_ARG(do_toad);			/* Turn a tinyjerk into a tinytoad */
CMD_TWO_ARG(do_toggledef);              /* Set/unset/see/list paramaters for toggles */
CMD_TWO_ARG(do_totemdef);              /* Set/unset/see/list paramaters for toggles */
CMD_TWO_ARG(do_turtle);			/* Turn a tinyjerk into a tinyturtle */
CMD_ONE_ARG_CMDARG(do_train);		/* Train the player */
CMD_ONE_ARG_CMDARG(do_noparsecmd);      /* noparse the command */
CMD_ONE_ARG_CMDARG(do_idle);		/* Train the player */
CMD_ONE_ARG_CMDARG(do_cluster);		/* Establish the cluster */
CMD_TWO_ARG_ARGV(do_trigger);		/* Trigger an attribute */
CMD_ONE_ARG(do_unlock);			/* Remove a lock from an object */
CMD_ONE_ARG(do_unlink);			/* Unlink exit or remove dropto */
CMD_NO_ARG(do_uptime);
CMD_ONE_ARG(do_use);			/* Use object */
CMD_NO_ARG(do_version);			/* List MUSH version number */
CMD_TWO_ARG_ARGV(do_verb);		/* Execute a user-created verb */
CMD_TWO_ARG_CMDARG(do_wait);		/* Perform command after a wait */
CMD_NO_ARG(do_whereall);
CMD_ONE_ARG(do_whereis);
CMD_NO_ARG(do_wielded);		        /* Print what I am wielding */
CMD_ONE_ARG(do_wipe);			/* Mass-remove attrs from obj */
CMD_NO_ARG(do_worn);		        /* Print what I am wearing */
CMD_TWO_ARG(do_snoop);			/* port redirection for immortals */
/*CMD_NO_ARG(do_dbclean); */		/* Clean db of unused attributes */
#ifdef REALITY_LEVELS
CMD_ONE_ARG(do_leveldefault);           /* Wipe levels */
CMD_TWO_ARG(do_rxlevel);                /* Set RX level */
CMD_TWO_ARG(do_txlevel);                /* Set TX level */
#endif /* REALITY_LEVELS */

typedef enum _cmdtype {
  CMD_BUILTIN_e = 1, CMD_ATTR_e = 2, CMD_ALIAS_e = 4, CMD_LOCAL_e = 8
} cmdtype_t;

typedef struct cmdentry CMDENT;
struct cmdentry {
  char	       *cmdname;
  NAMETAB      *switches;
  unsigned int perms;
  unsigned int perms2;
  unsigned int extra;
  unsigned int callseq;
  unsigned int hookmask;
  void         (*handler)();
  cmdtype_t cmdtype;  // Note we're being cunning here.. the main command
                      // table doesn't actually fill in this field.
                      // But ISO C standards say unfilled fields are 0
};

typedef struct aliasentry {
  char    *aliasname;
  unsigned int extra;     /* Badly named! Stores switch bitmask for alias */
  unsigned int perms;     /* Stores bitmask of switch permissions */
  unsigned int perms2;
  CMDENT  *cmdp;
} ALIASENT;

/* Command handler call conventions */

#define CS_NO_ARGS	 0x0000	/* No arguments */
#define CS_ONE_ARG	 0x0001	/* One argument */
#define CS_TWO_ARG	 0x0002	/* Two arguments */
#define CS_NARG_MASK	 0x0003	/* Argument count mask */
#define CS_ARGV		 0x0004	/* ARG2 is in ARGV form */
#define CS_INTERP	 0x0010	/* Interpret ARG2 if 2 args, ARG1 if 1 */
#define CS_NOINTERP	 0x0020	/* Never interp ARG2 if 2 or ARG1 if 1 */
#define CS_CAUSE	 0x0040	/* Pass cause to old command handler */
#define CS_UNPARSE	 0x0080	/* Pass unparsed cmd to old-style handler */
#define CS_CMDARG	 0x0100	/* Pass in given command args */	
#define CS_STRIP	 0x0200	/* Strip braces even when not interpreting */
#define	CS_STRIP_AROUND	 0x0400	/* Strip braces around entire string only */
#define CS_SEP		 0x0800
#define CS_PASS_SWITCHES 0x1000 /* Pass switches unparsed */
#define CS_ROLLBACK      0x2000 /* Special rollback */
/* Command permission flags */

#define CA_PUBLIC	0x00000000	/* No access restrictions */
#define CA_GOD		0x00000001	/* GOD only... */
#define CA_WIZARD	0x00000002	/* Wizards only */
#define CA_BUILDER	0x00000004	/* Builders only */
#define CA_IMMORTAL	0x00000008	/* Immortals only */
#define CA_ROBOT	0x00000010	/* Robots only */
#define CA_ADMIN	0x00000020
#define CA_GUILDMASTER	0x00000040
#define CA_POWER	0x00000080
#define FA_HANDLER	0x00000100	/* For Flags only */
#define CA_IGNORE	0x00000100
#define CA_IGNORE_MORTAL	0x00000200
#define CA_IGNORE_GM	0x00000400
#define CA_IGNORE_ARCH	0x00000800
#define CA_IGNORE_COUNC	0x00001000
#define CA_IGNORE_ROYAL	0x00002000
#define CA_IGNORE_IM	0x00004000
#define CA_LOGFLAG      0x00008000
#define CA_IGNORE_MASK	0x00007F00

#define CA_NO_HAVEN	0x00010000	/* Not by HAVEN players */
#define CA_NO_ROBOT	0x00020000	/* Not by ROBOT players */
#define CA_NO_SLAVE	0x00040000	/* Not by SLAVE players */
#define CA_NO_SUSPECT	0x00080000	/* Not by SUSPECT players */
#define CA_NO_GUEST	0x00100000	/* Not by GUEST players */
#define CA_NO_WANDER	0x00200000
#define CA_DISABLE_ZONE 0x00400000
#define CA_IGNORE_ZONE  0x00800000

#define CA_GBL_BUILD	0x01000000	/* Requires the global BUILDING flag */
#define CA_GBL_INTERP	0x02000000	/* Requires the global INTERP flag */
#define CA_DISABLED	0x04000000	/* Command completely disabled */
#define	CA_NO_DECOMP	0x08000000	/* Don't include in @decompile */

#define CA_LOCATION	0x10000000	/* Invoker must have location */
#define CA_CONTENTS	0x20000000	/* Invoker must have contents */
#define CA_PLAYER	0x40000000	/* Invoker must be a player */
#define CF_DARK		0x80000000	/* Command doesn't show up in list */

/* Second command permission flags */
#define CA_PUBLIC2      0x00000000      /* No access restrictions */
#define CA_NO_CODE	0x00000001	/* Not by NO_CODE players */
#define CA_NO_EVAL	0x00000002      /* Code doesn't eval? */
#define CA_EVAL		0x00000004      /* Code evals? */
#define CA_CLUSTER	0x00000010	/* Clustered? */
#define CA_NO_PARSE	0x00000020	/* Arguments incoming are not parsed but substitutions are */
#define CA_SB_BYPASS	0x20000000	/* Function can not be bypassed */
#define CA_SB_DENY	0x40000000      /* Function is sandbox denied */
#define CA_SB_IGNORE	0x80000000      /* Function is sandbox ignored */

#define BREAK_INLINE	0x00000001	/* @break/@assert should not go to wait queue */
#define GOTO_LABEL	0x00000001	/* @goto label marker */


#define ADMIN_LOAD	0x00000001	/* @admin/load the parameters */
#define ADMIN_SAVE	0x00000002	/* @admin/save the parameters */
#define ADMIN_EXECUTE	0x00000004	/* @admin/execute (run) the parameters */
#define ADMIN_LIST	0x00000008	/* @admikn/list the config params */

#define HOOK_BEFORE	0x00000001	/* BEFORE hook */
#define HOOK_AFTER	0x00000002	/* AFTER hook */
#define HOOK_PERMIT	0x00000004	/* PERMIT hook */
#define HOOK_IGNORE	0x00000008	/* IGNORE hook */
#define HOOK_IGSWITCH	0x00000010	/* BFAIL hook */
#define HOOK_AFAIL	0x00000020	/* AFAIL hook */
#define HOOK_CLEAR	0x00000040	/* CLEAR hook */
#define HOOK_LIST	0x00000080	/* LIST hooks */
#define HOOK_FAIL       0x00000100      /* FAIL hooks */
#define HOOK_INCLUDE	0x00000200	/* Process hooks as if it's an include */

#define QUITPRG_QUIET	0x00000001	/* silently quitprogram target */

extern int	FDECL(check_access, (dbref, int, int, int));
extern void	FDECL(process_command, (dbref, dbref, int, char *, char *[], int, int, int));

#endif
