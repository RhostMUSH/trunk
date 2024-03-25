/* attrs.h - Attribute definitions */

#ifndef _M_ATTRS_H
#define _M_ATTRS_H

#include "copyright.h"

/* Attribute flags */
#define	AF_ODARK	0x00000001	/* players other than owner can't see it */
#define	AF_DARK		0x00000002	/* No one can see it */
#define	AF_WIZARD	0x00000004	/* wizard and above modifiable */
#define	AF_MDARK	0x00000008	/* Only wizards can see it. Dark to mortals */
#define	AF_INTERNAL	0x00000010	/* Don't show even to #1 */
#define	AF_NOCMD	0x00000020	/* Don't create a @ command for it */
#define	AF_LOCK		0x00000040	/* Attribute is locked */
#define	AF_DELETED	0x00000080	/* Attribute should be ignored */
#define	AF_NOPROG	0x00000100	/* Don't process $-commands from this attr */
#define	AF_GOD		0x00000200	/* Only #1 can change it */
#define	AF_ADMIN	0x00000400	/* counc and above modifiable */
#define	AF_BUILDER	0x00000800	/* arch and above modifiable */
#define AF_IS_LOCK  	0x00001000
#define AF_GUILDMASTER	0x00002000	/* guildmaster can modify it */
#define AF_IMMORTAL 	0x00004000	/* immortal modifiable */
#define	AF_PRIVATE	0x00008000	/* Not inherited by children */
#define AF_NONBLOCKING	0x00010000	/* command doesn't keep parent and
					   globals from being checked if matched
                                           NOT USED -- Used now for @dbclean marking */
#define AF_VISUAL	0x00020000
#define AF_NOANSI	0x00040000
#define AF_PINVIS	0x00080000
#define AF_NORETURN     0x00100000	/* Attribute can't have /r/n */
#define AF_NOCLONE      0x00200000	/* Attribute isn't @cloned */
#define AF_NOPARSE      0x00400000	/* Attribute $command isn't parsed */
#define AF_SAFE         0x00800000	/* Attribute is SAFE and can't be modified */
#define AF_USELOCK	0x01000000	/* Attribute has a USELOCK defined */
#define AF_SINGLETHREAD 0x02000000	/* Set the attribute single-threaded for $commands */
#define AF_DEFAULT	0x04000000	/* TM3 Default Compatibility */
#define AF_ATRLOCK	0x08000000	/* Attribute has to pass global attribute lock */
#define AF_LOGGED	0x10000000	/* The attribute is logged for sets/removes */
/* AF_REGEXP is         0x20000000 */
#define AF_UNSAFE	0x40000000	/* Attribute is 'unsafe' and is u()'d at obj lvl */

typedef struct afstruct {
  const char *flagname;
  int flagmatch;
  int flagvalue;
  char flaglett;
} AFLAGENT;

#define	A_OSUCC		1	/* Others success message */
#define	A_OFAIL		2	/* Others fail message */
#define	A_FAIL		3	/* Invoker fail message */
#define	A_SUCC		4	/* Invoker success message */
#define	A_PASS		5	/* Password (only meaningful for players) */
#define	A_DESC		6	/* Description */
#define	A_SEX		7	/* Sex */
#define	A_ODROP		8	/* Others drop message */
#define	A_DROP		9	/* Invoker drop message */
#define	A_OKILL		10	/* Others kill message */
#define	A_KILL		11	/* Invoker kill message */
#define	A_ASUCC		12	/* Success action list */
#define	A_AFAIL		13	/* Failure action list */
#define	A_ADROP		14	/* Drop action list */
#define	A_AKILL		15	/* Kill action list */
#define	A_AUSE		16	/* Use action list */
#define	A_CHARGES	17	/* Number of charges remaining */
#define	A_RUNOUT	18	/* Actions done when no more charges */
#define	A_STARTUP	19	/* Actions run when game started up */
#define	A_ACLONE	20	/* Actions run when obj is cloned */
#define	A_APAY		21	/* Actions run when given COST pennies */
#define	A_OPAY		22	/* Others pay message */
#define	A_PAY		23	/* Invoker pay message */
#define	A_COST		24	/* Number of pennies needed to invoke xPAY */
#define	A_MONEY		25	/* Value or Wealth (internal) */
#define	A_LISTEN	26	/* (Wildcarded) string to listen for */
#define	A_AAHEAR	27	/* Actions to do when anyone says LISTEN str */
#define	A_AMHEAR	28	/* Actions to do when I say LISTEN str */
#define	A_AHEAR		29	/* Actions to do when others say LISTEN str */
#define	A_LAST		30	/* Date/time of last login (players only) */
#define	A_QUEUEMAX	31	/* Max. # of entries obj has in the queue */
#define	A_IDESC		32	/* Inside description (ENTER to get inside) */
#define	A_ENTER		33	/* Invoker enter message */
#define	A_OXENTER	34	/* Others enter message in dest */
#define	A_AENTER	35	/* Enter action list */
#define	A_ADESC		36	/* Describe action list */
#define	A_ODESC		37	/* Others describe message */
#define	A_RQUOTA	38	/* Relative object quota */
#define	A_ACONNECT	39	/* Actions run when player connects */
#define	A_ADISCONNECT	40	/* Actions run when player disconnects */
#define	A_ALLOWANCE	41	/* Daily allowance, if diff from default */
#define	A_LOCK		42	/* Object lock */
#define	A_NAME		43	/* Object name */
#define	A_COMMENT	44	/* Wizard-accessible comments */
#define	A_USE		45	/* Invoker use message */
#define	A_OUSE		46	/* Others use message */
#define	A_SEMAPHORE	47	/* Semaphore control info */
#define	A_TIMEOUT	48	/* Per-user disconnect timeout */
#define	A_QUOTA		49	/* Absolute quota (to speed up @quota) */
#define	A_LEAVE		50	/* Invoker leave message */
#define	A_OLEAVE	51	/* Others leave message in src */
#define	A_ALEAVE	52	/* Leave action list */
#define	A_OENTER	53	/* Others enter message in src */
#define	A_OXLEAVE	54	/* Others leave message in dest */
#define	A_MOVE		55	/* Invoker move message */
#define	A_OMOVE		56	/* Others move message */
#define	A_AMOVE		57	/* Move action list */
#define	A_ALIAS		58	/* Alias for player names */
#define	A_LENTER	59	/* ENTER lock */
#define	A_LLEAVE	60	/* LEAVE lock */
#define	A_LPAGE		61	/* PAGE lock */
#define	A_LUSE		62	/* USE lock */
#define	A_LGIVE		63	/* Give lock (who may give me away?) */
#define	A_EALIAS	64	/* Alternate names for ENTER */
#define	A_LALIAS	65	/* Alternate names for LEAVE */
#define	A_EFAIL		66	/* Invoker entry fail message */
#define	A_OEFAIL	67	/* Others entry fail message */
#define	A_AEFAIL	68	/* Entry fail action list */
#define	A_LFAIL		69	/* Invoker leave fail message */
#define	A_OLFAIL	70	/* Others leave fail message */
#define	A_ALFAIL	71	/* Leave fail action list */
#define	A_REJECT	72	/* Rejected page return message */
#define	A_AWAY		73	/* Not_connected page return message */
#define	A_IDLE		74	/* Success page return message */
#define	A_UFAIL		75	/* Invoker use fail message */
#define	A_OUFAIL	76	/* Others use fail message */
#define	A_AUFAIL	77	/* Use fail action list */
#define	A_PFAIL		78	/* Invoker page fail message */			/* NOT USED */
#define	A_TPORT		79	/* Invoker teleport message */
#define	A_OTPORT	80	/* Others teleport message in src */
#define	A_OXTPORT	81	/* Others teleport message in dst */
#define	A_ATPORT	82	/* Teleport action list */
#define	A_PRIVS		83	/* Individual permissions  - Totems */
#define	A_LOGINDATA	84	/* Recent login information */
#define	A_LTPORT	85	/* Teleport lock (can others @tel to me?) */
#define	A_LDROP		86	/* Drop lock (can I be dropped or @tel'ed) */
#define	A_LRECEIVE	87	/* Receive lock (who may give me things?) */
#define	A_LASTSITE	88	/* Last site logged in from, in clear text */
#define	A_INPREFIX	89	/* Prefix on incoming messages into objects */
#define	A_PREFIX	90	/* Prefix used by exits/objects when audible */
#define	A_INFILTER	91	/* Filter to zap incoming text into objects */
#define	A_FILTER	92	/* Filter to zap text forwarded by audible. */
#define	A_LLINK		93	/* Who may link to here */
#define	A_LTELOUT	94	/* Who may teleport out from here */
#define	A_FORWARDLIST	95	/* Recipients of AUDIBLE output */
#define	A_LCONTROL	96	/* Who controls me if CONTROL_OK set */
#define	A_LUSER		97	/* Spare lock not referenced by server */
#define	A_LPARENT	98	/* Who may @parent to me if PARENT_OK set */
#define A_LAMBDA	99	/* Attribute to store #lambda information to */
#define	A_VA		100	/* VA attribute (VB-VZ follow) */

#define A_CHANNEL       126     /* +com channels */
#define A_GUILD         127     /* player's guild */
#define A_FLAGLEVEL    128     /* Custom bitlevels thresholdsfor flags */
#define A_ZA		129	/* ZA attribute (ZB-ZZ follow) [Thorin]*/

#define A_BCCMAIL       155     /* Blind Carbon Copy Mail */
#define A_EMAIL         156     /* email address */ 				/* NOT USED */
#define A_LMAIL         157     /* Mail lock */
#define A_LSHARE	158	/* Mail share lock */

#define	A_GFAIL		159	/* Give fail message */
#define	A_OGFAIL	160	/* Others give fail message */
#define	A_AGFAIL	161	/* Give fail action */
#define	A_RFAIL		162	/* Receive fail message */
#define	A_ORFAIL	163	/* Others receive fail message */
#define	A_ARFAIL	164	/* Receive fail action */
#define	A_DFAIL		165	/* Drop fail message */
#define	A_ODFAIL	166	/* Others drop fail message */
#define	A_ADFAIL	167	/* Drop fail action */
#define	A_TFAIL		168	/* Teleport (to) fail message */
#define	A_OTFAIL	169	/* Others teleport (to) fail message */
#define	A_ATFAIL	170	/* Teleport fail action */
#define	A_TOFAIL	171	/* Teleport (from) fail message */
#define	A_OTOFAIL	172	/* Others teleport (from) fail message */
#define	A_AOTFAIL	173	/* Teleport (from) fail action */
#define A_ATOFAIL       174     /* Teleport (from) fail action */
#define A_MPASS		175
#define A_MPSET		176
#define A_LASTPAGE	177
#define A_RETPAGE	178
#define A_RECTIME 	179
#define A_MCURR		180
#define A_MQUOTA	181
#define A_LQUOTA	182
#define A_TQUOTA	183
#define A_MTIME		184
#define A_MSAVEMAX	185
#define A_MSAVECUR	186
#define A_IDENT		187
#define A_LZONEWIZ	188
#define A_LZONETO	189
#define A_LTWINK	190
#define A_SITEGOOD	191
#define A_SITEBAD	192
#define A_MAILSIG	193
#define A_ADESC2	194
#define A_PAYLIM	195
#define A_DESC2		196
#define A_RACE          197     /* player's race */
#define A_CMDCHECK	198
#define A_LSPEECH	199
#define A_SFAIL		200
#define A_ASFAIL	201
#define A_AUTOREG	202
#define A_LDARK         203	/* Darklock - pass to see if you see dark things */
#define A_STOUCH	204	/* Senses - Touch */
#define A_SATOUCH	205	/* Senses - Touch Action */
#define A_SOTOUCH	206	/* Senses - Touch Other */
#define A_SLISTEN	207	/* Senses - Listen */
#define A_SALISTEN	208	/* Senses - Listen Action */
#define A_SOLISTEN	209	/* Senses - Listen Other */
#define A_STASTE	210	/* Senses - Taste */
#define A_SATASTE	211	/* Senses - Taste Action */
#define A_SOTASTE	212	/* Senses - Taste Other */
#define A_SSMELL	213	/* Senses - Smell */
#define A_SASMELL	214	/* Senses - Smell Action */
#define A_SOSMELL	215	/* Senses - Smell Other */
#define A_LDROPTO	216	/* Drop-To lock - Check location if permission to drop */
#define A_LOPEN		217	/* Open Lock - Check for permission to open exits at location */
#define A_LCHOWN    	218	/* Lock object from being @chowned if set CHOWN_OK */
#define A_CAPTION       219     /* Caption to names */
#define A_ANSINAME      220     /* Ansi colors to names of anything */
#define A_TOTCMDS       221     /* Total commands player has done */
#define A_LSTCMDS       222     /* Commands since last on */
#define A_RECEIVELIM    223     /* Receive limit on money a player receives (max upper/lower) */
#define A_LCON_FMT	224     /* @conformat specifications */
#define A_LEXIT_FMT	225	/* @exitformat specifications */
#define A_LDEXIT_FMT	226	/* @darkexitformat specifications */
#define A_MODIFY_TIME	227	/* Modify time of objects when 'modified' */
#define A_CREATED_TIME	228	/* Created time of things when 'created' */
#define A_ALTNAME       229     /* @altname for alternate names when looking */
#define A_LALTNAME      230     /* @lock/altname for passing locks for above */
#define A_INVTYPE       231     /* Inventory type description */
#define A_TOTCHARIN     232     /* Total characters in  since last session */
#define A_TOTCHAROUT    233     /* Total characters out since last session */
#define A_LGIVETO       234     /* Lock if you can give things at location */
#define A_LGETFROM      235     /* Lock if you can pick things up at location */
#define A_SAYSTRING     236     /* Define string used for say/" instead of default 'says' */
#define A_LASTCREATE    237     /* Storage of last items created (dbref#'s) */
#define A_SAVESENDMAIL  238     /* Stores the player list of messages sent using brandy interface */
#define A_PROGBUFFER    239     /* Stores the @program information a player inputs */
#define A_PROGPROMPT    240     /* Stores the @program prompt value (40 char max) */
#define A_PROGPROMPTBUF 241     /* Program Prompt Buffer */
#define A_TEMPBUFFER    242     /* Temporary buffer to be used for various things */
#define A_DESTVATTRMAX  243	/* Store maximum @destroy and VATTR limit per player */

#define A_RLEVEL	244	/* Store reality level (17 chars)  - should always be defined */

#define A_NAME_FMT	245	/* Name Format */
#define A_LASTIP        246     /* IP address of last connection */
#define A_SPAMPROTECT	247	/* Spam protection attribute */
#define A_EXITTO	248	/* Variable Exits */
#define A_PROTECTNAME	249	/* Protect Name and/or other foo */
#define A_TITLE		250	/* Title of player */

#define A_OBJECTTAG     251 /* Internal ____OBJECTTAG Attribute */

#define	A_VLIST		252
#define	A_LIST		253
#define	A_STRUCT	254
#define	A_TEMP		255

#define	A_USER_START	256	/* Start of user-named attributes */
#define A_USER_MAXIMUM	2000000000 /* Absolute ceiling on maximum user attribs in database */
#define A_INLINE_START	2100000000 /* Start of highend user named attributes */
#define A_INLINE_END    2140000000 /* End of highend user named attributes -- DO NOT EXCEED MAXINT (2^32)*/
#define	ATR_BUF_CHUNK	100	/* Min size to allocate for attribute buffer */
#define	ATR_BUF_INCR	6	/* Max size of one attribute */

/* Define all attributes > 21000000000 here 
 * 
 * Note: You can go out of order, however, it's strongly suggested that
 *       you do not.  It does no harm, but every entry skipped does take
 *       up extra memory.  While small, it can add up.
 *       
 * You have 40 million built-in attributes you may assign.  Have fun.
 * That being between 2100000000 - 2140000000.
 */

/* Examples
 * #define A_HIGHATTRTEST	2100000000
 * #define A_HIGHATTRTEST2	2100000001
 * #define A_HIGHATTRTEST3	2100000002
 * #define A_HIGHATTRTEST4	2100000003
 */
#define A_CONNINFO	2100000000
#define A_CONNRECORD	2100000001

#endif
