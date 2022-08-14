/* flags.h - object flags */

#include "copyright.h"

#ifndef _M_FLAGS_H
#define _M_FLAGS_H

#define IS_TYPE_FLAG    1
#define IS_TYPE_TOGGLE  2
#define IS_TYPE_TOTEM   4

#include "htab.h"
#include "attrs.h"
#include "utils.h"

#define FLAG2	        0x1     /* Lives in extended flag word */
#define FLAG3		0x2
#define FLAG4		0x4
#define TOGGLE2		0x1
#define TOGGLE3         0x2
#define TOGGLE4         0x4
#define TOGGLE5         0x8
#define TOGGLE6         0x10
#define TOGGLE7         0x20
#define TOGGLE8         0x40
#define POWER3		0x2
#define POWER4		0x4
#define POWER5		0x8
#define POWER6		0x10
#define POWER7		0x20
#define POWER8		0x40

#define MF_SITE		0x0001
#define MF_STATS	0x0002
#define MF_FAIL		0x0004
#define MF_CONN		0x0008
#define MF_GFAIL	0x0010
#define MF_VLIMIT	0x0020
#define MF_TRIM		0x0040
#define MF_AREG		0x0080
#define MF_DCONN	0x0100
#define MF_CPU          0x0200
#define MF_BFAIL        0x0400
#define MF_COMPFAIL     0x0800
#define MF_CPUEXT       0x1000
#define MF_API		0x2000

/* Object types */
#define TYPE_ROOM       0x0
#define TYPE_THING      0x1
#define TYPE_EXIT       0x2
#define TYPE_PLAYER     0x3
#define TYPE_ZONE       0x4
#define TYPE_GARBAGE    0x5
#define NOTYPE          0x7
#define TYPE_MASK       0x7

/* First word of flags */
#define SEETHRU         0x00000008      /* Can see through to the other side */
#define WIZARD          0x00000010      /* gets automatic control */
#define LINK_OK         0x00000020      /* anybody can link to this room */
#define DARK            0x00000040      /* Don't show contents or presence */
#define JUMP_OK         0x00000080      /* Others may @tel here */
#define STICKY          0x00000100      /* Object goes home when dropped */
#define DESTROY_OK      0x00000200      /* Others may @destroy */
#define HAVEN           0x00000400      /* No killing here, or no pages */
#define QUIET           0x00000800      /* Prevent 'feelgood' messages */
#define HALT            0x00001000      /* object cannot perform actions */
#define TRACE           0x00002000      /* Generate evaluation trace output */
#define GOING           0x00004000      /* object is available for recycling */
#define MONITOR         0x00008000      /* Process ^x:action listens on obj? */
#define MYOPIC          0x00010000      /* See things as nonowner/nonwizard */
#define PUPPET          0x00020000      /* Relays ALL messages to owner */
#define CHOWN_OK        0x00040000      /* Object may be @chowned freely */
#define ENTER_OK        0x00080000      /* Object may be ENTER'd */
#define VISUAL          0x00100000      /* Everyone can see properties */
#define IMMORTAL        0x00200000      /* Object can't be killed */
#define HAS_STARTUP     0x00400000      /* Load some attrs at startup */
#define OPAQUE          0x00800000      /* Can't see inside */
#define VERBOSE         0x01000000      /* Tells owner everything it does. */
#define INHERIT         0x02000000      /* Gets owner's privs. (i.e. Wiz) */
#define NOSPOOF         0x04000000      /* Report originator of all actions. */
#define ROBOT           0x08000000      /* Player is a ROBOT */
#define SAFE            0x10000000      /* Need /override to @destroy */
#define CONTROL_OK      0x20000000      /* Control_OK specifies who ctrls me */
#define HEARTHRU        0x40000000      /* Can hear out of this obj or exit */
#define TERSE           0x80000000      /* Only show room name on look */

/* Second word of flags */
#define KEY             0x00000001      /* No puppets */
#define ABODE           0x00000002      /* May @set home here */
#define FLOATING        0x00000004      /* Inhibit Floating room.. msgs */
#define UNFINDABLE      0x00000008      /* Cant loc() from afar */
#define PARENT_OK       0x00000010      /* Others may @parent to me */
#define LIGHT           0x00000020      /* Visible in dark places */
#define HAS_LISTEN      0x00000040      /* Internal: LISTEN attr set */
#define HAS_FWDLIST     0x00000080      /* Internal: FORWARDLIST attr set */
#define ADMIN           0x00000100      /* Player has admin privs */
#define GUILDOBJ        0x00000200
#define GUILDMASTER     0x00000400      /* Player has gm privs */
#define NO_WALLS        0x00000800      /* So to stop normal walls */
#define REQUIRE_TREES 	0x00001000	/* Trees are required on this target for attrib sets */
/* ----FREE----         0x00002000 */   /* #define OLD_NOROBOT	0x00002000 */
#define SCLOAK		0x00004000
#define CLOAK		0x00008000
#define FUBAR		0x00010000
#define INDESTRUCTABLE	0x00020000	/* object can't be nuked */
#define NO_YELL		0x00040000	/* player can't @wall */
#define NO_TEL		0x00080000	/* player can't @tel or be @tel'd */
#define FREE		0x00100000	/* object/player has unlim money */
#define GUEST_FLAG	0x00200000
#define RECOVER		0x00400000
#define BYEROOM		0x00800000
#define WANDERER	0x01000000
#define ANSI		0x02000000
#define ANSICOLOR	0x04000000
#define NOFLASH		0x08000000
#define SUSPECT         0x10000000      /* Report some activities to wizards */
#define BUILDER         0x20000000      /* Player has architect privs */
#define CONNECTED       0x40000000      /* Player is connected */
#define SLAVE           0x80000000      /* Disallow most commands */

/* Third word of flags - Thorin 3/95 */
#define NOCONNECT	0x00000001
#define DPSHIFT		0x00000002
#define NOPOSSESS	0x00000004
#define COMBAT		0x00000008
#define IC		0x00000010
#define ZONEMASTER	0x00000020
#define ALTQUOTA	0x00000040
#define NOEXAMINE	0x00000080
#define NOMODIFY	0x00000100
#define CMDCHECK	0x00000200
#define DOORRED		0x00000400
#define PRIVATE		0x00000800	/* For exits only */
#define NOMOVE		0x00001000
#define STOP		0x00002000
#define NOSTOP		0x00004000
#define NOCOMMAND	0x00008000
#define AUDIT		0x00010000
#define SEE_OEMIT	0x00020000
#define NO_GOBJ		0x00040000
#define NO_PESTER	0x00080000
#define LRFLAG		0x00100000
#define TELOK		0x00200000
#define NO_OVERRIDE	0x00400000
#define NO_USELOCK      0x00800000
#define DR_PURGE	0x01000000	/* For rooms only...internal */
#define NO_ANSINAME     0x02000000      /* Remove the ability to set @ansiname */
#define SPOOF           0x04000000
#define SIDEFX          0x08000000      /* Allow enactor to use side-effects */
#define ZONECONTENTS    0x10000000      /* Search contents of zonemaster for $commands */
#define NOWHO           0x20000000      /* Player in WHO doesn't show up - use with @hide */
#define ANONYMOUS       0x40000000      /* Player set shows up as 'Someone' when talking */
#define BACKSTAGE       0x80000000      /* Immortal toggle for items on control */

/* Forth word of flags - Thorin 3/95 */
#define NOBACKSTAGE     0x00000001      /* Immortal toggle to control no-backstage */
#define LOGIN           0x00000002      /* Enable player to login past @disable logins */
#define INPROGRAM       0x00000004      /* Player is inside a program */
#define COMMANDS        0x00000008      /* Optional define for $commands */
#define MARKER0         0x00000010      /* TM 3.0 marker flags */
#define MARKER1         0x00000020
#define MARKER2         0x00000040
#define MARKER3         0x00000080
#define MARKER4         0x00000100
#define MARKER5         0x00000200
#define MARKER6         0x00000400
#define MARKER7         0x00000800
#define MARKER8         0x00001000
#define MARKER9         0x00002000
#define BOUNCE		0x00004000	/* That lovely TM 3.0 Bouncy thingy */
#define SHOWFAILCMD     0x00008000      /* Show failed $commands default error */
#define NOUNDERLINE     0x00010000      /* Strip UNDERLINE character from ANSI */
#define NONAME		0x00020000	/* Target does not display name with look */
#define ZONEPARENT	0x00040000	/* Target zone allows inheritance of attribs */
#define SPAMMONITOR	0x00080000	/* Monitor the target for spam */
#define BLIND           0x00100000      /* Exits and locations snuff arrived/left */
#define NOCODE		0x00200000	/* Players may not code */
#define HAS_PROTECT	0x00400000	/* Player target has protect name data */
#define XTERMCOLOR      0x00800000      /* Extended ANSI Xterm colors */
#define HAS_ATTRPIPE    0x01000000      /* Attribute piping via @pipe */
#define HAS_OBJECTTAG   0x02000000      /* Has ____ObjectTag attribute set */
/* 0x04000000 free */
/* 0x08000000 free */
/* 0x10000000 free */
/* 0x20000000 free */
/* 0x40000000 free */
/* 0x80000000 free */

/* First word of toggles - Thorin 3/95 */

#define TOG_MONITOR		0x00000001	/* Active monitor on player */
#define TOG_MONITOR_USERID	0x00000002 	/* show userid */
#define TOG_MONITOR_SITE	0x00000004	/* show site */
#define TOG_MONITOR_STATS	0x00000008	/* show stats */
#define TOG_MONITOR_FAIL	0x00000010	/* show fails */
#define TOG_MONITOR_CONN	0x00000020
#define TOG_VANILLA_ERRORS      0x00000040      /* show normal error msg */
#define TOG_NO_ANSI_EX          0x00000080      /* suppress ansi stuff in ex */
#define TOG_CPUTIME		0x00000100	/* show CPU time for cmds */
#define TOG_MONITOR_DISREASON	0x00000200
#define TOG_MONITOR_VLIMIT	0x00000400
#define TOG_NOTIFY_LINK		0x00000800
#define TOG_MONITOR_AREG	0x00001000
#define TOG_MONITOR_TIME        0x00002000
#define TOG_CLUSTER		0x00004000	/* Object is part of a cluster */
#define TOG_SNUFFDARK           0x00008000	/* Snuff Dark Exit Viewing */
#define TOG_NOANSI_PLAYER       0x00010000      /* Do not show ansi player names */
#define TOG_NOANSI_THING        0x00020000      /* ... things */
#define TOG_NOANSI_ROOM         0x00040000      /* ... rooms */
#define TOG_NOANSI_EXIT         0x00080000      /* ... exits */
#define TOG_NO_TIMESTAMP	0x00100000      /* Do not modify timestamps on target */
#define TOG_NO_FORMAT           0x00200000      /* Override @conformat/@exitformat */
#define TOG_ZONE_AUTOADD        0x00400000      /* Automatically add FIRST zone in list */
#define TOG_ZONE_AUTOADDALL     0x00800000      /* Automatically add ALL zones in list */
#define TOG_WIELDABLE           0x01000000      /* Marker to specify if object is wieldable */
#define TOG_WEARABLE            0x02000000      /* Marker to specify if object is wearable */
#define TOG_SEE_SUSPECT         0x04000000      /* Specify who sees suspect in WHO/MONITOR */
#define TOG_MONITOR_CPU         0x08000000      /* Specify who sees CPU overflow alerts */
#define TOG_BRANDY_MAIL         0x10000000      /* Define brandy like mail interface */
#define TOG_FORCEHALTED         0x20000000      /* The item toggled can @force halted things */
#define TOG_PROG                0x40000000      /* Can use @program on other people/things */
#define TOG_NOSHELLPROG         0x80000000      /* Target can not issue commands inside a prog */

/* Second word of toggles -- Ash */

#define TOG_EXTANSI             0x00000001      /* Specify if target can used extended ansi naming */
#define TOG_IMMPROG             0x00000002      /* Only an immortal can @quitprogram them */
#define TOG_MONITOR_BFAIL       0x00000004      /* Monitor if a failed connect on bad character */
#define TOG_PROG_ON_CONNECT	0x00000008	/* Reverse logic of program on connect */
#define TOG_MAIL_STRIPRETURN    0x00000010      /* Strip carriage return in mail combining */
#define TOG_PENN_MAIL           0x00000020      /* Use PENN style syntax */
#define TOG_SILENTEFFECTS       0x00000040	/* Silents did_it() functionality on target */
#define TOG_IGNOREZONE          0x00000080      /* Target is set to @icmd zones */
#define TOG_VPAGE               0x00000100      /* Target sees alias in pages */
#define TOG_PAGELOCK            0x00000200	/* Target issues pagelocks as normal */
#define TOG_MAIL_NOPARSE	0x00000400	/* Don't parse %t/%b/%r in mail */
#define TOG_MAIL_LOCKDOWN	0x00000800	/* Mortal-accessed mail/number and mail/check */
#define TOG_MUXPAGE		0x00001000	/* Have 'page' work like MUX */
#define TOG_NOZONEPARENT	0x00002000	/* Zone Child does NOT inherit parent attribs */
#define TOG_ATRUSE		0x00004000	/* Enactor can use Attribute based USELOCKS */
#define TOG_VARIABLE		0x00008000	/* Set exit to be variable */
#define TOG_KEEPALIVE		0x00010000	/* Send 'keepalives' to the target player */
#define TOG_CHKREALITY		0x00020000	/* Target checks @lock/user for Reality passes */
#define TOG_NOISY		0x00040000	/* Always do noisy sets */
#define TOG_ZONECMDCHK          0x00080000	/* Zone commands checked on target like @parent */
#define TOG_HIDEIDLE		0x00100000	/* Allow wizards/immortals to hide their idle time */
#define TOG_MORTALREALITY	0x00200000	/* Override the wiz_always_real setting */
#define TOG_ACCENTS		0x00400000	/* Accents being displayed */
#define TOG_PREMAILVALIDATE	0x00800000	/* Pre-Validate the mail send list before sending mail */
#define TOG_SAFELOG             0x01000000	/* Allow 'clean logging' by the player */
#define TOG_UTF8                0x02000000	/* UTF8 being displayed */
/* 0x04000000 free */
#define TOG_NODEFAULT		0x08000000	/* Allow target to inherit default attribs */
#define TOG_EXFULLWIZATTR	0x10000000	/* Examine Wiz attribs */
#ifdef ENH_LOGROOM
#define TOG_LOGROOMENH		0x20000000	/* Enhanced Room Logging */
#endif
#define TOG_LOGROOM		0x40000000	/* Log Room's location/contents */
#define TOG_NOGLOBPARENT        0x80000000      /* Target does not inherit global attributes */

/* Power level markers...correspond to guildmaster, arch, counc. */

#define POWER_LEVEL_OFF		0
#define POWER_LEVEL_GUILD	1
#define POWER_LEVEL_ARCH	2
#define POWER_LEVEL_COUNC	3
#define POWER_LEVEL_SPC		98
#define POWER_LEVEL_NA		99
#define POWER_PURGE		100
#define POWER_CHECK		101
#define POWER_REMOVE		102
#define POWER_CHECK_OWNER	128

/* First word of power positions.  Each position is 2 bits so the
   number here is how far over to shift the 2 bit pattern	  */

#define POWER_CHANGE_QUOTAS	0
#define POWER_CHOWN_ME  	2
#define POWER_CHOWN_ANYWHERE	4
#define POWER_CHOWN_OTHER	6
#define POWER_WIZ_WHO		8
#define POWER_EX_ALL		10
#define POWER_NOFORCE		12
#define POWER_SEE_QUEUE_ALL	14
#define POWER_FREE_QUOTA	16
#define POWER_GRAB_PLAYER	18
#define POWER_JOIN_PLAYER	20
#define POWER_LONG_FINGERS	22
#define POWER_NO_BOOT		24
#define POWER_BOOT		26
#define POWER_STEAL		28
#define POWER_SEE_QUEUE		30

/* Second word of power positions. */
#define POWER_SHUTDOWN		0
#define POWER_TEL_ANYWHERE	2
#define POWER_TEL_ANYTHING	4
#define POWER_PCREATE		6
#define POWER_STAT_ANY		8
#define POWER_FREE_WALL		10
#define POWER_EXECSCRIPT	12
#define POWER_FREE_PAGE		14
#define POWER_HALT_QUEUE	16
#define POWER_HALT_QUEUE_ALL	18
#define POWER_FORMATTING	20
#define POWER_NOKILL		22
#define POWER_SEARCH_ANY	24
#define POWER_SECURITY		26
#define POWER_WHO_UNFIND	28
#define POWER_WRAITH		30

/* Third word of power positions. */
#define POWER_OPURGE		0
#define POWER_HIDEBIT           2
#define POWER_NOWHO             4
#define POWER_FULLTEL_ANYWHERE  6
#define POWER_EX_FULL           8
#define POWER_API		10
#define POWER_MONITORAPI	12
#define POWER_WIZ_IDLE		14
#define POWER_WIZ_SPOOF		16
#define POWER_USE_FREELIST	18
/* 20 free */
/* 22 free */
/* 24 free */
/* 26 free */
/* 28 free */
/* 30 free */

/* DEPOWER flags */
/* First word */
#define DP_WALL			0
#define DP_LONG_FINGERS		2
#define DP_STEAL		4
#define DP_CREATE		6
#define DP_WIZ_WHO		8
#define DP_CLOAK		10
#define DP_BOOT			12
#define DP_PAGE			14
#define DP_FORCE		16
#define DP_LOCKS		18
#define DP_COM			20
#define DP_COMMAND		22
#define DP_MASTER		24
#define DP_EXAMINE		26
#define DP_NUKE			28
#define DP_FREE			30

/* Second word */
#define DP_OVERRIDE		0
#define DP_TEL_ANYWHERE		2
#define DP_TEL_ANYTHING		4
#define DP_PCREATE		6
#define DP_POWER		8
#define DP_QUOTA		10
#define DP_MODIFY		12
#define DP_CHOWN_ME		14
#define DP_CHOWN_OTHER		16
#define DP_ABUSE		18
#define DP_UNL_QUOTA		20
#define DP_SEARCH_ANY		22
#define DP_GIVE			24
#define DP_RECEIVE		26
#define DP_NOGOLD		28
#define DP_NOSTEAL		30

/* Third word...and there was much rejoicing */
#define DP_PASSWORD		0
#define DP_MORTAL_EXAMINE	2
#define DP_PERSONAL_COMMANDS	4
/* 06 free */
#define DP_DARK			8
/* 10 free */
/* 12 free */
/* 14 free */
/* 16 free */
/* 18 free */
/* 20 free */
/* 22 free */
/* 24 free */
/* 26 free */
/* 28 free */
/* 30 free */

/* Flags from prior versions of MUSH */
#define PERN_SLAVE      0x00000080
#define PERN_KEY        0x00000008
#define PERN_NOSPOOF    0x00200000
#define PERN_SUSPECT    0x04000000
#define PERN_VERBOSE    0x00000080
#define PERN_IMMORTAL   0x00002000
#define PERN_MONITOR    0x00200000
#define PERN_SAFE       0x04000000
#define PERN_UNFIND     0x02000000
#define V2_ACCESSED     0x00008000
#define V3_MARKED       0x00010000
#define V4_GAGGED       0x00000080
#define V4_SUSPECT      0x20000000
#define V6OBJ_KEY       0x00000008
#define V6_BUILDER      0x00000008
#define V6_FLOATING     0x00000008
#define V6EXIT_KEY      0x00000080
#define V6_SUSPECT      0x00000080
#define V6_CONNECT      0x00000200
#define V6_ABODE        0x00000200
#define V6ROOM_JUMPOK   0x00002000
#define V6PLYR_UNFIND   0x00002000
#define V6ROOM_UNFIND   0x08000000
#define V6_SLAVE        0x10000000

/* Flags from MUSE */
#define MUSE_BUILD      0x00000010
#define MUSE_SLAVE      0x00000080
#define MUSE_KEY        0x00000010
#define MUSE_DEST_OK    0x00000200
#define MUSE_ABODE      0x00000200
#define MUSE_SEETHRU    0x00000200
#define MUSE_UNFIND     0x00001000

#define MUSE_CHOWN_OK   0x00000020
#define MUSE_DARK       0x00000040
#define MUSE_STICKY     0x00000100
#define MUSE_HAVEN      0x00000400
#define MUSE_INHERIT    0x00002000
#define MUSE_GOING      0x00004000
#define MUSE_PUPPET     0x00020000
#define MUSE_LINK_OK    0x00040000
#define MUSE_ENTER_OK   0x00080000
#define MUSE_VISUAL     0x00100000
#define MUSE_OPAQUE     0x00800000
#define MUSE_QUIET      0x01000000

/* Flags from TinyMUD */
#define MUD_ABODE       0x00000800
#define MUD_ROBOT       0x00004000
#define MUD_CHOWN_OK    0x00008000

/* Flags for use with ANSI name colors stuff */
#define ANSI_PLAYER	1
#define ANSI_THING      2
#define ANSI_ROOM	4
#define ANSI_EXIT	8

/* ---------- First word of totems */
#define	TOTEM_MARKER0	 0x00000001 /* slot 9 */
#define TOTEM_MARKER0_SLOT	9
#define	TOTEM_MARKER1	 0x00000002 /* slot 9 */
#define TOTEM_MARKER1_SLOT	9
#define	TOTEM_MARKER2	 0x00000004 /* slot 9 */
#define TOTEM_MARKER2_SLOT	9
#define	TOTEM_MARKER3	 0x00000008 /* slot 9 */
#define TOTEM_MARKER3_SLOT	9
#define	TOTEM_MARKER4	 0x00000010 /* slot 9 */
#define TOTEM_MARKER4_SLOT	9
#define	TOTEM_MARKER5	 0x00000020 /* slot 9 */
#define TOTEM_MARKER5_SLOT	9
#define	TOTEM_MARKER6	 0x00000040 /* slot 9 */
#define TOTEM_MARKER6_SLOT	9
#define	TOTEM_MARKER7	 0x00000080 /* slot 9 */
#define TOTEM_MARKER7_SLOT	9
#define	TOTEM_MARKER8	 0x00000100 /* slot 9 */
#define TOTEM_MARKER8_SLOT	9
/* 0x00000200 free */
/* 0x00000400 free */
/* 0x00000800 free */
/* 0x00001000 free */
/* 0x00002000 free */
/* 0x00004000 free */
/* 0x00008000 free */
/* 0x00010000 free */
/* 0x00020000 free */
/* 0x00040000 free */
/* 0x00080000 free */
/* 0x00100000 free */
/* 0x00200000 free */
/* 0x00400000 free */
/* 0x00800000 free */
/* 0x01000000 free */
/* 0x02000000 free */
/* 0x04000000 free */
/* 0x08000000 free */
/* 0x10000000 free */
/* 0x20000000 free */
/* 0x40000000 free */
/* 0x80000000 free */

/* ---------- Second word of totems */
/* 0x00000001 free */
/* 0x00000002 free */
/* 0x00000004 free */
/* 0x00000008 free */
/* 0x00000010 free */
/* 0x00000020 free */
/* 0x00000040 free */
/* 0x00000080 free */
/* 0x00000100 free */
/* 0x00000200 free */
/* 0x00000400 free */
/* 0x00000800 free */
/* 0x00001000 free */
/* 0x00002000 free */
/* 0x00004000 free */
/* 0x00008000 free */
/* 0x00010000 free */
/* 0x00020000 free */
/* 0x00040000 free */
/* 0x00080000 free */
/* 0x00100000 free */
/* 0x00200000 free */
/* 0x00400000 free */
/* 0x00800000 free */
/* 0x01000000 free */
/* 0x02000000 free */
/* 0x04000000 free */
/* 0x08000000 free */
/* 0x10000000 free */
/* 0x20000000 free */
/* 0x40000000 free */
/* 0x80000000 free */

/* ---------- Third word of totems */
/* 0x00000001 free */
/* 0x00000002 free */
/* 0x00000004 free */
/* 0x00000008 free */
/* 0x00000010 free */
/* 0x00000020 free */
/* 0x00000040 free */
/* 0x00000080 free */
/* 0x00000100 free */
/* 0x00000200 free */
/* 0x00000400 free */
/* 0x00000800 free */
/* 0x00001000 free */
/* 0x00002000 free */
/* 0x00004000 free */
/* 0x00008000 free */
/* 0x00010000 free */
/* 0x00020000 free */
/* 0x00040000 free */
/* 0x00080000 free */
/* 0x00100000 free */
/* 0x00200000 free */
/* 0x00400000 free */
/* 0x00800000 free */
/* 0x01000000 free */
/* 0x02000000 free */
/* 0x04000000 free */
/* 0x08000000 free */
/* 0x10000000 free */
/* 0x20000000 free */
/* 0x40000000 free */
#define	TOTEM_SETQLABEL	0x80000000 /* slot 9 */
#define	TOTEM_SETQLABEL_SLOT	9
#define TOTEM_API_LUA 	0x00000400 /* slot 9 */
#define	TOTEM_API_LUA_SLOT  	9


/* Undeclared words of totems (outside of reserved 3 slots) */

/* ---------------------------------------------------------------------------
 * TOTEMENT: Information about object totem flags.
 */

typedef struct totem_entry {
	char	*flagname;	/* Name of totem */
	int	flagvalue;	/* Which bit is the totem flag */
	char    flaglett;       /* Flag letter for listing */
	int	flagtier;	/* The flag tier 0, 1, 2 */
	int	flagpos;	/* 0-9 totem flag position for structure */
	int	totemflag;	/* Ctrl Flags for this flag (recursive? :-) */
	int	listperm;	/* Who can see Totem */
	int	setovperm;	/* Who can set Totem */
	int	usetovperm;	/* Who can unset Totem */
	int	typeperm;	/* Type permission */
	int	permanent;	/* Perm handle: 0 - dyn, 1 - perm, 2 - builtin */
	int	aliased;	/* Is this entry aliased?  0 - no, 1 - yes */
	int     (*handler)();   /* Handler for setting/clearing this flag */
} TOTEMENT;

/* ---------------------------------------------------------------------------
 * FLAGENT: Information about object flags.
 */

typedef struct flag_entry {
	char 	flagname[16];   /* Name of the flag */
	int     flagvalue;      /* Which bit in the object is the flag */
	char    flaglett;       /* Flag letter for listing */
	int     flagflag;       /* Ctrl flags for this flag (recursive? :-) */
	int     listperm;       /* Who sees this flag when set */
        int     setovperm;      /* Override who can set the flag */
        int     usetovperm;     /* Override who can unset the flag */
	int	typeperm;	/* Type permission */
	int     (*handler)();   /* Handler for setting/clearing this flag */
} FLAGENT;

typedef struct toggle_entry {
	const char *togglename;   /* Name of the flag */
	int     togglevalue;      /* Which bit in the object is the flag */
	char    togglelett;       /* Flag letter for listing */
	int     toggleflag;       /* Ctrl flags for this flag (recursive? :-) */
	int     listperm;       /* Who sees this flag when set */
	int	setovperm;	/* Override who can set the toggle */
	int	usetovperm;	/* Override who can unset the toggle */
	int	typeperm;	/* Type permission */
	int     (*handler)();   /* Handler for setting/clearing this flag */
} TOGENT;

typedef struct power_entry {
	const char *powername;	/* Name of power */
	int	powerpos;	/* Which position (each pos is 2 bits) */
	char	powerlet;
	int	powerflag;
	int	powerperm;
	int	powerlev;
	int	(*handler)();
} POWENT;

/* ---------------------------------------------------------------------------
 * OBJENT: Fundamental object types
 */

typedef struct object_entry {
	const char *name;
	char    lett;
	int     perm;
	int     flags;
} OBJENT;
extern OBJENT object_types[8];

#define OF_CONTENTS     0x0001          /* Object has contents: Contents() */
#define OF_LOCATION     0x0002          /* Object has a location: Location() */
#define OF_EXITS        0x0004          /* Object has exits: Exits() */
#define OF_HOME         0x0008          /* Object has a home: Home() */
#define OF_DROPTO       0x0010          /* Object has a dropto: Dropto() */
#define OF_OWNER        0x0020          /* Object can own other objects */
#define OF_SIBLINGS     0x0040          /* Object has siblings: Next() */

typedef struct flagset {
	FLAG    word1;
	FLAG    word2;
	FLAG	word3;
	FLAG	word4;
	FLAG	word5;
	FLAG	word6;
	FLAG	word7;
	FLAG	word8;
} FLAGSET;

extern void     NDECL(init_flagtab);
extern void     NDECL(init_totemtab);
extern void     NDECL(init_toggletab);
extern void	NDECL(init_powertab);
extern void     FDECL(display_flagtab, (dbref));
extern void     FDECL(display_toggletab, (dbref));
extern void     FDECL(display_totemtab, (dbref, char *));
extern int	FDECL(totem_cansee, (dbref, dbref, char *));
extern int	FDECL(totem_cansee_bit, (dbref, dbref, int));
extern int	FDECL(totem_flags, (char *, dbref, dbref, char *));

extern void     FDECL(display_flagtab2, (dbref, char *, char **));
extern void     FDECL(display_toggletab2, (dbref, char *, char **));
extern void     FDECL(display_totemtab2, (dbref, char *, char **, int, char *));
extern void     FDECL(flag_set, (dbref, dbref, char *, int));
extern void     FDECL(toggle_set, (dbref, dbref, char *, int));
extern void     FDECL(power_set, (dbref, dbref, char *, int));
extern char *   FDECL(flag_description, (dbref, dbref, int, int *, int));
extern char *   FDECL(toggle_description, (dbref, dbref, int, int, int *));
extern char *   FDECL(power_description, (dbref, dbref, int, int));
extern char *   FDECL(depower_description, (dbref, dbref, int, int));
extern FLAGENT *FDECL(find_flag, (dbref, char *));
extern TOGENT *FDECL(find_toggle, (dbref, char *));
extern TOTEMENT *FDECL(find_totem, (dbref, char *));
extern int	FDECL(check_totem, (dbref, dbref, char *));
extern char *   FDECL(decode_flags, (dbref, dbref, FLAG, FLAG, FLAG, FLAG));
extern void     FDECL(decode_flags_func, (dbref, dbref, FLAG, FLAG, FLAG, FLAG, char *, char *));
extern int      FDECL(has_flag, (dbref, dbref, char *));
extern char *   FDECL(unparse_object, (dbref, dbref, int));
extern char *   FDECL(unparse_object_altname, (dbref, dbref, int));
extern char *   FDECL(unparse_object_ansi, (dbref, dbref, int));
extern char *   FDECL(unparse_object_ansi_altname, (dbref, dbref, int));
extern char *   FDECL(unparse_object2, (dbref, dbref, int));
extern char *   FDECL(unparse_object_numonly, (dbref));
extern char *   FDECL(ansi_exitname, (dbref));
extern int      FDECL(convert_flags, (dbref, char *, FLAGSET *, FLAG *, int));
extern void     FDECL(decompile_flags, (dbref, dbref, char *, char *, int));
extern void     FDECL(decompile_toggles, (dbref, dbref, char *, char *, int));
extern void     FDECL(decompile_totems, (dbref, dbref, char *, char *, int));
extern int	FDECL(HasPriv, (dbref, dbref, int, int, int));

extern int	FDECL(parse_aflags, (dbref, dbref, int, char *, char **, int));
extern char *	FDECL(give_name_aflags, (dbref, dbref, int));
extern int	FDECL(has_aflag, (dbref, dbref, int, char *));

#define unparse_flags(p,t) decode_flags(p,t,Flags(t),Flags2(t),Flags3(t),Flags4(t))
#define unparse_fun_flags(p,t,c1,c2) decode_flags_func(p,t,Flags(t),Flags2(t),Flags3(t),Flags4(t),c1,c2)

#define GOD ((dbref) 1)

/* ---------------------- Object Permission/Attribute Macros */
/* IS(X,T,F)            - Is X of type T and have flag F set? */
/* Typeof(X)            - What object type is X */
/* God(X)               - Is X player #1 */
/* Guest(X)             - Is X the GUEST player */
/* Robot(X)             - Is X a robot player */
/* Wizard(X)            - Does X have wizard privs */
/* Immortal(X)          - Is X unkillable */
/* Alive(X)             - Is X a player or a puppet */
/* Dark(X)              - Is X dark */
/* WHODark(X)           - Should X be hidden from the WHO report */
/* Builder(X)           - Is X allowed to add on to the db */
/* Floating(X)          - Prevent 'disconnected room' msgs for room X */
/* Quiet(X)             - Should 'Set.' messages et al. from X be disabled */
/* Verbose(X)           - Should owner receive all commands executed? */
/* Trace(X)             - Should owner receive eval trace output? */
/* Player_haven(X)      - Is the owner of X no-page */
/* Haven(X)             - Is X no-kill(rooms) or no-page(players) */
/* Halted(X)            - Is X halted (not allowed to run commands)? */
/* Suspect(X)           - Is X someone the wizzes should keep an eye on */
/* Slave(X)             - Should X be prevented from db-changing commands */
/* Safe(X,P)            - Does P need the /OVERRIDE switch to @destroy X? */
/* Monitor(X)          - Should we check for ^xxx:xxx listens on player? */
/* Terse(X)             - Should we only show the room name on a look? */
/* Myopic(X)            - Should things as if we were nonowner/nonwiz */
/* Audible(X)           - Should X forward messages? */
/* Findroom(X)          - Can players in room X be found via @whereis? */
/* Unfindroom(X)        - Is @whereis blocked for players in room X? */
/* Findable(X)          - Can @whereis find X */
/* Unfindable(X)        - Is @whereis blocked for X */
/* No_robots(X)         - Does X disallow robot players from using */
/* Has_location(X)      - Is X something with a location (i.e. plyr or obj) */
/* Has_home(X)          - Is X something with a home (i.e. plyr or obj) */
/* Has_contents(X)      - Is X something with contents (i.e. plyr/obj/room) */
/* Good_obj(X)          - Is X inside the DB and have a valid type? */
/* Good_owner(X)        - Is X a good owner value? */
/* Going(X)             - Is X marked GOING? */
/* Inherits(X)          - Does X inherit the privs of its owner */
/* Examinable(P,X)      - Can P look at attribs of X */
/* MyopicExam(P,X)      - Can P look at attribs of X (obeys MYOPIC) */
/* Controls(P,X)        - Can P force X to do something */
/* Affects(P,X)         - (Controls in MUSH V1) Is P wiz or same owner as X */
/* Abode(X)             - Is X an ABODE room */
/* Link_exit(P,X)       - Can P link from exit X */
/* Linkable(P,X)        - Can P link to X */
/* Mark(x)              - Set marked flag on X */
/* Unmark(x)            - Clear marked flag on X */
/* Marked(x)            - Check marked flag on X */
/* See_attr(P,X.A,O,F,Z)- Can P see text attr A on X if attr has owner O */
/* Set_attr(P,X,A,F)    - Can P set/change text attr A (with flags F) on X */
/* Read_attr(P,X,A,O,F,Z) - Can P see attr A on X if attr has owner O */
/* Write_attr(P,X,A,F)  - Can P set/change attr A (with flags F) on X */

/* Define totem functionaries for totem flags */
#define SetqLabel(x)	(mudconf.setqlabel || (Good_obj(x) && (dbtotem[x].flags[TOTEM_SETQLABEL_SLOT] & TOTEM_SETQLABEL)))
#define LuaAPI(x)	(mudconf.setqlabel || (Good_obj(x) && (dbtotem[x].flags[TOTEM_API_LUA_SLOT] & TOTEM_API_LUA)))

#define InProgram(x)    ((Flags4(x) & INPROGRAM) != 0)
#define Login(x)	((Flags4(x) & LOGIN) != 0)
#define Bouncer(x)	((Flags4(x) & BOUNCE) != 0)
#define Bouncy(x)	((Flags4(x) & BOUNCE) != 0)
#define ShowFailCmd(x)  ((Flags4(x) & SHOWFAILCMD) != 0)
#define Backstage(x)    (((Flags3(x) & BACKSTAGE) != 0) || ((Flags3(Owner(x)) & BACKSTAGE) != 0))
#define NoBackstage(x)  ((Flags4(x) & NOBACKSTAGE) != 0)
#define Private(x)	((Flags3(x) & PRIVATE) != 0)
#define Anonymous(x)    ((Flags3(x) & ANONYMOUS) != 0)
#define s_Lrused(x)	s_Flags3((x), Flags3(x) | LRFLAG)
#define c_Lrused(x)	s_Flags3((x), Flags3(x) & ~LRFLAG)
#define Lrused(x)	((Flags3(x) & LRFLAG) != 0)
#define Ic(x)		((Flags3(x) & IC) != 0)
#define Combat(x)	((Flags3(x) & COMBAT) != 0)
#define ZoneMaster(x)	((Flags3(x) & ZONEMASTER) != 0)
#define ZoneContents(x) ((Flags3(x) & ZONECONTENTS) != 0)
#define NoWho(x)        ((Flags3(x) & NOWHO) != 0)
#define NoEx(x)		((Flags3(x) & NOEXAMINE) != 0)
#define NoMod(x)	((Flags3(x) & NOMODIFY) != 0)
#define TelOK(x)	((Flags3(x) & TELOK) != 0)
#define NoOverride(x)	((Flags3(x) & NO_OVERRIDE) != 0)
#define NoUselock(x)    ((Flags3(x) & NO_USELOCK) != 0)
#define NoAnsiName(x)   ((Flags3(x) & NO_ANSINAME) != 0)
#define Spoof(x)        ((Flags3(x) & SPOOF) != 0)
#define SideFX(x)	((Flags3(x) & SIDEFX) != 0)
#define VanillaErrors(x) ((Toggles(x) & TOG_VANILLA_ERRORS) != 0)
#define No_Ansi_Ex(x)	((Toggles(x) & TOG_NO_ANSI_EX) != 0)
#define CpuTime(x)	((Toggles(x) & TOG_CPUTIME) != 0)
#define Cluster(x)	((Toggles(x) & TOG_CLUSTER) != 0)
#define SnuffDark(x)	((Toggles(x) & TOG_SNUFFDARK) != 0)
#define NoAnsiPlayer(x) ((Toggles(x) & TOG_NOANSI_PLAYER) != 0)
#define NoAnsiThing(x)  ((Toggles(x) & TOG_NOANSI_THING) != 0)
#define NoAnsiRoom(x)   ((Toggles(x) & TOG_NOANSI_ROOM) != 0)
#define NoAnsiExit(x)   ((Toggles(x) & TOG_NOANSI_EXIT) != 0)
#define NoFormat(x)     ((Toggles(x) & TOG_NO_FORMAT) != 0)
#define NoTimestamp(x)  ((Toggles(x) & TOG_NO_TIMESTAMP) != 0)
#define AutoZoneAdd(x)  ((Toggles(x) & TOG_ZONE_AUTOADD) != 0)
#define AutoZoneAll(x)  ((Toggles(x) & TOG_ZONE_AUTOADDALL) != 0)
#define Wieldable(x)    ((Toggles(x) & TOG_WIELDABLE) != 0)
#define Wearable(x)     ((Toggles(x) & TOG_WEARABLE) != 0)
#define SeeSuspect(x)   ((Toggles(x) & TOG_SEE_SUSPECT) != 0)
#define Brandy(x)       ((Toggles(x) & TOG_BRANDY_MAIL) != 0)
#define ForceHalted(x)  ((Toggles(x) & TOG_FORCEHALTED) != 0)
#define Prog(x)         ((Toggles(x) & TOG_PROG) != 0)
#define NoShProg(x)     ((Toggles(x) & TOG_NOSHELLPROG) != 0)
#define ExtAnsi(x)      ((Toggles2(x) & TOG_EXTANSI) != 0)
#define ImmProg(x)      ((Toggles2(x) & TOG_IMMPROG) != 0)
#define ProgCon(x)	((Toggles2(x) & TOG_PROG_ON_CONNECT) != 0)
#define MailStripRet(x) ((Toggles2(x) & TOG_MAIL_STRIPRETURN) != 0)
#define PennMail(x)     ((Toggles2(x) & TOG_PENN_MAIL) != 0)
#define SilentEff(x)	((Toggles2(x) & TOG_SILENTEFFECT) != 0)
#define IgnoreZone(x)   ((Toggles2(x) & TOG_IGNOREZONE) != 0)
#define ZoneCmdChk(x)	((Toggles2(x) & TOG_ZONECMDCHK) != 0)
#define VPage(x)	((Toggles2(x) & TOG_VPAGE) != 0)
#define PageLock(x)     ((Toggles2(x) & TOG_PAGELOCK) != 0)
#define MailNoParse(x)  ((Toggles2(x) & TOG_MAIL_NOPARSE) != 0)
#define MailLockDown(x) ((Toggles2(x) & TOG_MAIL_LOCKDOWN) != 0)
#define MuxPage(x)	((Toggles2(x) & TOG_MUXPAGE) != 0)
#define NoZoneParent(x)	((Toggles2(x) & TOG_NOZONEPARENT) != 0)
#define AtrUse(x)	((Toggles2(x) & TOG_ATRUSE) != 0)
#define NoGlobParent(x) ((Toggles2(x) & TOG_NOGLOBPARENT) != 0)
#define LogRoom(x)      ((Toggles2(x) & TOG_LOGROOM) != 0)
#define ExFullWizAttr(x) ((Toggles2(x) & TOG_EXFULLWIZATTR) != 0)
#define TogNoDefault(x)	((Toggles2(x) & TOG_NODEFAULT) != 0)
#define SafeLog(x)	((Toggles2(x) & TOG_SAFELOG) != 0)
#define TogHideIdle(x)	((Toggles2(x) & TOG_HIDEIDLE) != 0)
#define TogMortReal(x)	((Toggles2(x) & TOG_MORTALREALITY) != 0)
#define Accents(x)	((Toggles2(x) & TOG_ACCENTS) != 0)
#define UTF8(x)		((Toggles2(x) & TOG_UTF8) != 0)
#define MailValid(x)	((Toggles2(x) & TOG_PREMAILVALIDATE) != 0)
#define KeepAlive(x)	((Toggles2(x) & TOG_KEEPALIVE) != 0)
#define ChkReality(x)	((Toggles2(x) & TOG_CHKREALITY) != 0)
#define TogNoisy(x)	((Toggles2(x) & TOG_NOISY) != 0)
#ifdef ENH_LOGROOM
#define LogRoomEnh(x)	((Toggles2(x) & TOG_LOGROOMENH) != 0)
#endif
#define VariableExit(x) ((Toggles2(x) & TOG_VARIABLE) != 0)
#define DPShift(x)	((Flags3(x) & DPSHIFT) != 0)
#define IS(thing,type,flag) ((Typeof(thing)==(type)) && (Flags(thing) & (flag)))
#define Typeof(x)       (Flags(x) & TYPE_MASK)
#define God(x)          ((x) == GOD)
#define Guest(x)        (((Flags2(x) & GUEST_FLAG) != 0) || (Good_obj(Owner(x)) && (Flags2(Owner(x)) & GUEST_FLAG) != 0))
#define Wanderer(x)	(((Flags2(x) & WANDERER) != 0) || (Good_obj(Owner(x)) && (Flags2(Owner(x)) & WANDERER) != 0))
#define NoPossess(x)	((Flags3(x) & NOPOSSESS) != 0)
#define NoCode(x)	(((Flags4(x) & NOCODE) != 0) || (Good_obj(Owner(x)) && (Flags4(Owner(x)) & NOCODE) != 0))
#define CmdCheck(x)	((Flags3(x) & CMDCHECK) != 0)
#define Robot(x)        (isPlayer(x) && ((Flags(x) & ROBOT) != 0))
#define Alive(x)        (isPlayer(x) || (Puppet(x) && Has_contents(x)))

#define Recover(x)	((Flags2(x) & RECOVER) != 0)

#define OwnsOthers(x)   ((object_types[Typeof(x)].flags & OF_OWNER) != 0)
#define Has_location(x) ((object_types[Typeof(x)].flags & OF_LOCATION) != 0)
#define Has_contents(x) ((object_types[Typeof(x)].flags & OF_CONTENTS) != 0)
#define Has_exits(x)    ((object_types[Typeof(x)].flags & OF_EXITS) != 0)
#define Has_siblings(x) ((object_types[Typeof(x)].flags & OF_SIBLINGS) != 0)
#define Has_home(x)     ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define Has_dropto(x)   ((object_types[Typeof(x)].flags & OF_DROPTO) != 0)
#define Home_ok(x)      ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define isPlayer(x)     (Good_obj(x) && (Typeof(x) == TYPE_PLAYER))
#define isRoom(x)       (Good_obj(x) && (Typeof(x) == TYPE_ROOM))
#define isExit(x)       (Good_obj(x) && (Typeof(x) == TYPE_EXIT))
#define isThing(x)      (Good_obj(x) && (Typeof(x) == TYPE_THING))
#define isZone(x)       (Good_obj(x) && (Typeof(x) == TYPE_ZONE))

#define Good_obj(x)     (((x) >= 0) && ((x) < mudstate.db_top) && \
			 (Typeof(x) <= TYPE_ZONE))
#define Good_owner(x)   (Good_obj(x) && OwnsOthers(x))

#define Good_chk(x)	(Good_obj(x) && !Going(x) && !Recover(x))
#define Transparent(x)  ((Flags(x) & SEETHRU) != 0)
#define Link_ok(x)      (((Flags(x) & LINK_OK) != 0) && Has_contents(x))
#ifndef STANDALONE
#define Wizard(x)       (evlevchk(x,4) ? mudstate.evalresult : \
			((Flags(x) & WIZARD) || Immortal(x) || God(x) ||\
			 ((Flags(Owner(x)) & WIZARD) && Inherits(x))))
#else
#define Wizard(x)	((Flags(x) & WIZARD) || Immortal(x) || God(x) ||\
			 ((Flags(Owner(x)) & WIZARD) && Inherits(x)))
#endif
#define ReqTrees(x)	((Flags2(x) & REQUIRE_TREES) != 0)
#define Recover(x)	((Flags2(x) & RECOVER) != 0)
#define Byeroom(x)	((Flags2(x) & BYEROOM) != 0)
#define Dr_Purge(x)	((Flags3(x) & DR_PURGE) != 0)
/* Who did this dark thing? and why? it broke stuff... */
/*#define Dark(x)         (((Flags(x) & DARK) != 0) && (Wizard(x) || !Alive(x)))*/
#define Altq(x)		((Flags3(x) & ALTQUOTA) != 0)
#define Dark(x)		( (!DePriv(x, NOTHING, DP_DARK, POWER8, POWER_LEVEL_NA) && ((Flags(x) & DARK) != 0)) || \
                          ((Flags2(x) & RECOVER) != 0) || ((Flags(x) & GOING) != 0) )
#define Jump_ok(x)      (((Flags(x) & JUMP_OK) != 0) && Has_contents(x))
#define Sticky(x)       ((Flags(x) & STICKY) != 0)
#define Destroy_ok(x)   ((Flags(x) & DESTROY_OK) != 0)
#define Haven(x)        ((Flags(x) & HAVEN) != 0)
#define Player_haven(x) ((Flags(Owner(x)) & HAVEN) != 0)
#define Quiet(x)        ((Flags(x) & QUIET) != 0)
#define Halted(x)       ((Flags(x) & HALT) != 0)
#define Trace(x)        ((Flags(x) & TRACE) != 0)
#define Going(x)        ((Flags(x) & GOING) != 0)
#define Monitor(x)      ((Flags(x) & MONITOR) != 0)
#define Myopic(x)       ((Flags(x) & MYOPIC) != 0)
#define Puppet(x)       ((Flags(x) & PUPPET) != 0)
#define Chown_ok(x)     ((Flags(x) & CHOWN_OK) != 0)
#define Enter_ok(x)     (((Flags(x) & ENTER_OK) != 0) && \
				Has_location(x) && Has_contents(x))
#define Visual(x)       ((Flags(x) & VISUAL) != 0)
#define TogReason(x)	((Toggles(x) & TOG_MONITOR_DISREASON) != 0)
#ifndef STANDALONE
#define Immortal(x)     (evlevchk(x,5) ? mudstate.evalresult : \
		         ((Flags(x) & IMMORTAL) || God(x) || \
			 ((Flags(Owner(x)) & IMMORTAL) && Inherits(x))))
#else
#define Immortal(x)	((Flags(x) & IMMORTAL) || God(x) || \
			 ((Flags(Owner(x)) & IMMORTAL) && Inherits(x)))
#endif
#define OCloak(x)	(((Flags(Owner(x)) & IMMORTAL) || \
			 (Flags(Owner(x)) & WIZARD)) && Hidden(x) && \
			 Unfindable(x))
#ifndef STANDALONE
#define Admin(x)        (evlevchk(x,3) ? mudstate.evalresult : \
			((Flags2(x) & ADMIN) || \
			 Wizard(x) || \
			 ((Flags2(Owner(x)) & ADMIN) && Inherits(x))))
#define Guildmaster(x)  (evlevchk(x,1) ? mudstate.evalresult : \
			((Flags2(x) & GUILDMASTER) || \
			 Builder(x) || Admin(x) || Wizard(x) || \
			 ((Flags2(Owner(x)) & GUILDMASTER) && Inherits(x))))
#else
#define Admin(x)	((Flags2(x) & ADMIN) || \
			 Wizard(x) || \
			 ((Flags2(Owner(x)) & ADMIN) && Inherits(x)))
#define Guildmaster(x)	((Flags2(x) & GUILDMASTER) || \
			 Builder(x) || Admin(x) || Wizard(x) || \
			 ((Flags2(Owner(x)) & GUILDMASTER) && Inherits(x)))
#endif
#define Privilaged(x)      (Builder(x) || \
				Guildmaster(x) )
#define Opaque(x)       ((Flags(x) & OPAQUE) != 0)
#define Verbose(x)      ((Flags(x) & VERBOSE) != 0)
#define Inherits(x)     (((Flags(x) & INHERIT) != 0) || \
			 ((Flags(Owner(x)) & INHERIT) != 0) || \
			 ((x) == Owner(x)))
#define Nospoof(x)      ((Flags(x) & NOSPOOF) != 0)
#define Safe(x,p)       (OwnsOthers(x) || \
			 (Flags(x) & SAFE) || \
			 (mudconf.safe_unowned && (Owner(x) != Owner(p))))
#define Control_ok(x)   ((Flags(x) & CONTROL_OK) != 0)
#define Audible(x)      ((Flags(x) & HEARTHRU) != 0)
#define Terse(x)        ((Flags(x) & TERSE) != 0)
#define Key(x)          ((Flags2(x) & KEY) != 0)
#define Abode(x)        (((Flags2(x) & ABODE) != 0) && Home_ok(x))
#define Floating(x)     ((Flags2(x) & FLOATING) != 0)
#define Findable(x)     ((Flags2(x) & UNFINDABLE) == 0)
#define Unfindable(x)	(((Flags2(x) & UNFINDABLE) != 0) || ((Flags2(x) & RECOVER) != 0) || \
                         ((Flags(x) & GOING) != 0))
#define Hideout(x)      ((Flags2(x) & UNFINDABLE) != 0)
#define Parent_ok(x)    ((Flags2(x) & PARENT_OK) != 0)
#define Light(x)        ((Flags2(x) & LIGHT) != 0)
#define Suspect(x)      ((Flags2(Owner(x)) & SUSPECT) != 0)
#ifndef STANDALONE
#define Builder(x)      (evlevchk(x,2) ? mudstate.evalresult : \
			((Flags2(x) & BUILDER) || \
			 Admin(x) || \
			 ((Flags2(Owner(x)) & BUILDER) && Inherits(x))))
#else
#define Builder(x)	((Flags2(x) & BUILDER) || \
			 Admin(x) || \
			 ((Flags2(Owner(x)) & BUILDER) && Inherits(x)))
#endif
#define Connected(x)    (((Flags2(x) & CONNECTED) != 0) && \
			 (Typeof(x) == TYPE_PLAYER))
#define Slave(x)        ((Flags2(Owner(x)) & SLAVE) != 0)
#define Fubar(x)	((Flags2(Owner(x)) & FUBAR) != 0)
/*
#define Hidden(x)       (((Flags(x) & DARK) != 0) || ((Flags2(x) & RECOVER) != 0) || \
                         ((Flags(x) & GOING) != 0))
*/
#define Hidden(x)	( (!DePriv(x, NOTHING, DP_DARK, POWER8, POWER_LEVEL_NA) && ((Flags(x) & DARK) != 0)) || \
                          ((Flags2(x) & RECOVER) != 0) || ((Flags(x) & GOING) != 0) )
#ifndef STANDALONE
#define Cloak(x)	(Unfindable(x) && Hidden(x) && (Immortal(x) || \
			 Immortal(Owner(x)) || ((Wizard(x) || \
			 Wizard(Owner(x))) && \
			 !DePriv(x,NOTHING,DP_CLOAK,POWER6,POWER_LEVEL_NA))))
#else
#define Cloak(x)	(Unfindable(x) && Hidden(x) && Wizard(x))
#endif
#define SCloak(x)	(((Flags2(x) & SCLOAK) != 0) || ((Flags2(x) & RECOVER) != 0))
#define NCloak(x)	((Flags2(x) & CLOAK) != 0)
#define Snoop(x)	((Flags2(x) & SNOOP) != 0)
#define No_yell(x)	((Flags2(x) & NO_YELL) != 0)
#define No_tel(x)	((Flags2(x) & NO_TEL) != 0)
#define FreeFlag(x)	((Flags2(x) & FREE) != 0)
#define Indestructable(x)	((Flags2(x) & INDESTRUCTABLE) != 0)
#define ShowAnsi(x)	((Flags2(x) & ANSI) != 0)
#define ShowAnsiColor(x)	((Flags2(x) & ANSICOLOR) != 0)
#define ShowAnsiXterm(x)	((Flags4(x) & XTERMCOLOR) != 0)
#define NoFlash(x)	((Flags2(x) & NOFLASH) != 0)
#define NoUnderline(x)	((Flags4(x) & NOUNDERLINE) != 0)
#define NoName(x)	((Flags4(x) & NONAME) != 0)
#define ZoneParent(x)	((Flags4(x) & ZONEPARENT) != 0)
#define NoSpam(x)	((Flags4(x) & SPAMMONITOR) != 0)
#define Blind(x)        ((Flags4(x) & BLIND) != 0)

#define H_Startup(x)    ((Flags(x) & HAS_STARTUP) != 0)
#define H_Fwdlist(x)    ((Flags2(x) & HAS_FWDLIST) != 0)
#define H_Listen(x)     ((Flags2(x) & HAS_LISTEN) != 0)
#define H_Protect(x)	((Flags4(x) & HAS_PROTECT) != 0)
#define H_Attrpipe(x)	((Flags4(x) & HAS_ATTRPIPE) != 0)
#define H_ObjectTag(x)    ((Flags4(x) & HAS_OBJECTTAG) != 0)

#define s_Halted(x)     s_Flags((x), Flags(x) | HALT)
#define s_Going(x)      s_Flags((x), Flags(x) | GOING)
#define s_Recover(x)	s_Flags2((x), Flags2(x) | RECOVER)
#define s_Connected(x)  s_Flags2((x), Flags2(x) | CONNECTED)
#define c_Connected(x)  s_Flags2((x), Flags2(x) & ~CONNECTED)

#define Parentable(p,x) (Controls(p,x) || \
			 (Parent_ok(x) && could_doit(p,x,A_LPARENT,1,0)))

#define MyopicExam(p,x) (((Flags(x) & VISUAL) != 0) || \
			 (!Myopic(p) && Examinable(p,x)))
#define Affects(p,x)    (Good_obj(x) && \
			 (!(God(x) && !God(p))) && \
			 (Wizard(p) || \
			  (Owner(p) == Owner(x))))
#define Mark(x)         (mudstate.markbits->chunk[(x)>>3] |= \
			 mudconf.markdata[(x)&7])
#define Unmark(x)       (mudstate.markbits->chunk[(x)>>3] &= \
			 ~mudconf.markdata[(x)&7])
#define Marked(x)       (mudstate.markbits->chunk[(x)>>3] & \
			 mudconf.markdata[(x)&7])
#define Mark_all(i)     for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
				mudstate.markbits->chunk[i]=0xff
#define Unmark_all(i)   for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
				mudstate.markbits->chunk[i]=0x0
#define Link_exit(p,x)  ((Typeof(x) == TYPE_EXIT) && \
			 (!NoEx(x)) && \
			 ((Location(x) == NOTHING) || Controls(p,x)))
#define Linkable(p,x)   (Good_obj(x) && \
			 (Has_contents(x)) && \
			 (((Flags(x) & LINK_OK) != 0) || \
			  Controls(p,x)))
#define ControlsforattrImmortal(p,x,a,f) \
			 (Immortal(p) && !(((a)->flags & (AF_GOD)) || (f & (AF_GOD))))
#define ControlsforattrWizard(p,x,a,f) \
			  (Wizard(p) && !(Flags(Owner(x)) & IMMORTAL) && \
			   !(((a)->flags & (AF_GOD|AF_IMMORTAL)) || \
                             (f & (AF_GOD|AF_IMMORTAL))))
#define ControlsforattrAdmin(p,x,a,f) \
			  (Admin(p) && !(Flags(Owner(x)) & WIZARD) && \
			   !(Flags(Owner(x)) & IMMORTAL) && \
			   !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD))))
#define ControlsforattrBuilder(p,x,a,f) \
			  (Builder(p) && !(Flags(Owner(x)) & IMMORTAL) && \
			   !(Flags(Owner(x)) & WIZARD) && \
			   !(Flags2(Owner(x)) & ADMIN) && \
			   !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN))))
#define ControlsforattrGuildmaster(p,x,a,f) \
			  (Guildmaster(p) && !(Flags(Owner(x)) & IMMORTAL) && \
			   !(Flags(Owner(x)) & WIZARD) && \
			   !(Flags2(Owner(x)) & ADMIN) && \
			   !(Flags2(Owner(x)) & BUILDER) && \
			   !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN|AF_BUILDER)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN|AF_BUILDER))))
#ifndef STANDALONE
#define ControlsforattrOwner(p,x,a,f) \
			  ((((Owner(p) == Owner(x)) && \
                             (evlevchk(p,bittype(p)) ? mudstate.evalresult : 1) && \
			     (Inherits(p) || !Inherits(x))) || \
                             could_doit(p,x,A_LTWINK,0,0)) && \
			   ((Immortal(p) && !(((a)->flags & (AF_GOD)) || \
                                              (f & (AF_GOD)))) || \
			    (Wizard(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL)) || \
                               (f & (AF_GOD|AF_IMMORTAL)))) || \
			    (Admin(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                             AF_WIZARD)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD)))) || \
			    (Builder(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                             AF_WIZARD|AF_ADMIN)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN)))) || \
			    (Guildmaster(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                              AF_WIZARD|AF_ADMIN| \
                                              AF_BUILDER)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN|AF_BUILDER)))) || \
			    (!Guildmaster(p) && !Builder(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD| \
                                              AF_ADMIN|AF_BUILDER| \
                                              AF_GUILDMASTER)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN| \
                                     AF_BUILDER|AF_GUILDMASTER))))))
#else
#define ControlsforattrOwner(p,x,a,f) \
			  ((((Owner(p) == Owner(x)) && \
			     (Inherits(p) || !Inherits(x)))) && \
			   ((Immortal(p) && !(((a)->flags & (AF_GOD)) || \
                                              (f & (AF_GOD)))) || \
			    (Wizard(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL)) || \
                               (f & (AF_GOD|AF_IMMORTAL)))) || \
			    (Admin(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                             AF_WIZARD)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD)))) || \
			    (Builder(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                             AF_WIZARD|AF_ADMIN)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN)))) || \
			    (Guildmaster(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                             AF_WIZARD|AF_ADMIN| \
                                             AF_BUILDER)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD| \
                                     AF_ADMIN|AF_BUILDER)))) || \
			    (!Guildmaster(p) && !Builder(p) && \
                             !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD| \
                                             AF_ADMIN|AF_BUILDER| \
                                             AF_GUILDMASTER)) || \
                               (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD| \
                                     AF_ADMIN|AF_BUILDER|AF_GUILDMASTER))))))
#endif
#ifndef STANDALONE
#define ControlsforattrZoneWizard(p,x,a,f) \
                        (ZoneWizard(p,x) && \
                         (!(Flags(Owner(x)) & IMMORTAL) && \
                          !(Flags(Owner(x)) & WIZARD) && \
                          !(Flags(Owner(x)) & ADMIN) && \
                          !(Flags(Owner(x)) & BUILDER)) && \
			 ((Immortal(p) && !(((a)->flags & (AF_GOD)) || \
                                            (f & (AF_GOD)))) || \
			  (Wizard(p) && \
                           !(((a)->flags & (AF_GOD|AF_IMMORTAL)) || \
                             (f & (AF_GOD|AF_IMMORTAL)))) || \
			  (Admin(p) && \
                           !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                           AF_WIZARD)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD)))) || \
			  (Builder(p) && \
                           !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                           AF_WIZARD|AF_ADMIN)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN)))) || \
			  (Guildmaster(p) && \
                           !(((a)->flags & (AF_GOD|AF_IMMORTAL| \
                                           AF_WIZARD|AF_ADMIN| \
                                           AF_BUILDER)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN| \
                                   AF_BUILDER)))) || \
			  (!Guildmaster(p) && !Builder(p) && \
                           !(((a)->flags & (AF_GOD|AF_IMMORTAL|AF_WIZARD| \
                                           AF_ADMIN|AF_BUILDER| \
                                           AF_GUILDMASTER)) || \
                             (f & (AF_GOD|AF_IMMORTAL|AF_WIZARD|AF_ADMIN| \
                                   AF_BUILDER|AF_GUILDMASTER))))))
#else
#define ControlsforattrZoneWizard(p,x,a,f) (0)
#endif

#define See_attr_explicit(p,x,a,o,f) \
			(!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && \
			  ((Owner(p) == (o)) && \
				!(((a)->flags & (AF_DARK|AF_MDARK)) || \
                                  (f & (AF_DARK|AF_MDARK)))))
#ifdef ATTR_HACK
#define Seeattrtmp(p,a) ((mudconf.hackattr_see == 0) && !Wizard(p) && ((a)->name[0] == '_'))
#else
#define Seeattrtmp(p,a) (0)
#endif
#define Set_attr(p,x,a,f) \
                        (((God(p) && !((a)->flags & (AF_INTERNAL|AF_IS_LOCK))) || \
                         (!God(x) && \
			  Controlsforattr(p,x,a,f) && \
                          ( !((f & AF_LOCK) && !Wizard(p)) && \
                            !((a)->flags & AF_INTERNAL) \
			  ) \
			 )) && \
                         ( (God(p) || !(((a)->flags & AF_GOD) || (f & AF_GOD))) && \
                           (Immortal(p) || !(((a)->flags & AF_IMMORTAL) || (f & AF_IMMORTAL))) \
                         ) \
			)
#define Write_attr(p,x,a,f) \
                        ((God(p) && !((a)->flags & AF_INTERNAL)) || \
                         (!God(x) && Controlsforattr(p,x,a,f) && \
			  ( \
                           (!(f & AF_LOCK) && \
                            !(((a)->flags & \
                             (AF_INTERNAL|AF_WIZARD|AF_GOD)) || \
                              (f & (AF_INTERNAL|AF_WIZARD|AF_GOD)))) || \
                           (Wizard(p) && \
                            !((a)->flags & (AF_INTERNAL|AF_GOD))))))
#define Has_power(p,x)  (check_access((p),powers_nametab[x].flag, 0, 0))

#define WizMod(x) (Immortal(x) || (Wizard(x) && !mudconf.imm_nomod))

#endif
