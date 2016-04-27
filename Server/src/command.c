/* command.c -perms2; command parser and support routines */

#ifdef SOLARIS
/* Solaris has borked declarations for header files */
char *index(const char *, int);
#endif

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "copyright.h"
#include "autoconf.h"


#include "config.h"
#include "db.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "functions.h"
#include "externs.h"
#include "match.h"
#include "flags.h"
#include "alloc.h"
#include "vattr.h"
/* MMMail- include mail flags */
#include "mail.h"
#include "rhost_ansi.h"
#include "door.h"

#include "debug.h"
#define FILENUM COMMAND_C

extern char * attrib_show(char *, int);
extern void display_perms(dbref, int, int, char *);
extern void del_perms(dbref, char *, char *, char **, int);
extern void add_perms(dbref, char *, char *, char **, int);
extern void mod_perms(dbref, char *, char *, char **, int);
extern void FDECL(list_cf_access, (dbref, char *, int));
extern void FDECL(list_siteinfo, (dbref));
extern int news_system_active;
extern POWENT pow_table[];
extern POWENT depow_table[];
extern int lookup(char *, char *, int, int *);
extern CF_HAND(cf_site);
extern dbref       FDECL(match_thing, (dbref, char *));
extern void cf_log_syntax(dbref, char *, const char *, char *);
extern dbref FDECL(match_thing_quiet, (dbref, char *));
extern ATRP *atrp_head;

extern double FDECL(time_ng, (double*));
extern void FDECL(populate_tor_seed, (void));

#ifdef CACHE_OBJS
#define CACHING "object"
#else
#define CACHING "attribute"
#endif

/* IMPORTANT: Make sure you understand the difference
 *            between the two commands.
 */
CMDENT * lookup_command(char *cmdname);
CMDENT * lookup_orig_command(char *cmdname);
/* ---------------------------------------------------------------------------
 * Switch tables for the various commands.
 */

#define SW_MULTIPLE 0x80000000  /* This sw may be spec'd w/others */
#define SW_GOT_UNIQUE   0x40000000  /* Already have a unique option */
                    /* (typically via a switch alias) */

/* Switch nametab setup.  Structure syntax is as follows
 *
 *    command name
 *    chars required for unique lookup of command name
 *    permissions
 *    2nd word of permissions
 *    switch bitwise
 */

NAMETAB admin_sw[] =
{
    {(char *) "load", 2, CA_IMMORTAL, 0, ADMIN_LOAD},
    {(char *) "save", 1, CA_IMMORTAL, 0, ADMIN_SAVE},
    {(char *) "execute", 1, CA_IMMORTAL, 0, ADMIN_EXECUTE},
    {(char *) "list", 2, CA_IMMORTAL, 0, ADMIN_LIST},
    {NULL, 0, 0, 0, 0}};

NAMETAB break_sw[] =
{
    {(char *) "queued", 1, CA_PUBLIC, 0, 0}, /* Default value -- no need to label it */
    {(char *) "inline", 1, CA_PUBLIC, 0, BREAK_INLINE},
    {NULL, 0, 0, 0, 0}};

NAMETAB evaltab_sw[] =
{
    {(char *) "immortal", 1, CA_IMMORTAL, 0, 5},
    {(char *) "royalty", 1, CA_WIZARD, 0, 4},
    {(char *) "wizard", 1, CA_WIZARD, 0, 4},
    {(char *) "councilor", 2, CA_ADMIN, 0, 3},
    {(char *) "architect", 1, CA_BUILDER, 0, 2},
    {(char *) "guildmaster", 1, CA_GUILDMASTER, 0, 1},
    {(char *) "citizen", 2, CA_PUBLIC, 0, 0},
    {NULL, 0, 0, 0, 0}};

NAMETAB type_sw[] =
{
    {(char *) "thing", 1, CA_PUBLIC, 0, TYPE_THING},
    {(char *) "player", 1, CA_PUBLIC, 0, TYPE_PLAYER},
    {(char *) "exit", 1, CA_PUBLIC, 0, TYPE_EXIT},
    {(char *) "room", 1, CA_PUBLIC, 0, TYPE_ROOM},
    {(char *) "other", 1, CA_PUBLIC, 0, TYPE_MASK},
    {NULL, 0, 0, 0, 0}};

NAMETAB hook_sw[] =
{
    {(char *) "before", 3, CA_IMMORTAL, 0, HOOK_BEFORE},
    {(char *) "after", 3, CA_IMMORTAL, 0, HOOK_AFTER},
    {(char *) "permit", 3, CA_IMMORTAL, 0, HOOK_PERMIT},
    {(char *) "ignore", 3, CA_IMMORTAL, 0, HOOK_IGNORE},
    {(char *) "clear", 3, CA_IMMORTAL, 0, HOOK_CLEAR|SW_MULTIPLE},
    {(char *) "list", 3, CA_IMMORTAL, 0, HOOK_LIST},
    {(char *) "igswitch", 3, CA_IMMORTAL, 0, HOOK_IGSWITCH},
    {(char *) "extend", 3, CA_IMMORTAL, 0, HOOK_IGSWITCH},
    {(char *) "fail", 3, CA_IMMORTAL, 0, HOOK_FAIL},
    {NULL, 0, 0, 0, 0}};

NAMETAB aflags_sw[] =
{
    {(char *) "full", 2, CA_WIZARD, 0, AFLAGS_FULL},
    {(char *) "perms", 2, CA_WIZARD, 0, AFLAGS_PERM},
    {(char *) "list", 2, CA_WIZARD, 0, AFLAGS_PERM},
    {(char *) "search", 2, CA_WIZARD, 0, AFLAGS_SEARCH|SW_MULTIPLE},
    {(char *) "add", 2, CA_WIZARD, 0, AFLAGS_ADD},
    {(char *) "del", 2, CA_WIZARD, 0, AFLAGS_DEL},
    {(char *) "mod", 2, CA_WIZARD, 0, AFLAGS_MOD},
    {NULL, 0, 0, 0, 0}};

NAMETAB areg_sw[] =
{
    {(char *) "load", 2, CA_IMMORTAL, 0, AREG_LOAD},
    {(char *) "unload", 1, CA_IMMORTAL, 0, AREG_UNLOAD},
    {(char *) "list", 3, CA_IMMORTAL, 0, AREG_LIST},
    {(char *) "add", 2, CA_IMMORTAL, 0, AREG_ADD},
    {(char *) "fadd", 1, CA_IMMORTAL, 0, AREG_ADD_FORCE},
    {(char *) "del", 1, CA_IMMORTAL, 0, AREG_DEL_PLAY},
    {(char *) "edel", 1, CA_IMMORTAL, 0, AREG_DEL_EMAIL},
    {(char *) "limit", 3, CA_IMMORTAL, 0, AREG_LIMIT},
    {(char *) "wipe", 1, CA_IMMORTAL, 0, AREG_WIPE},
    {(char *) "aedel", 2, CA_IMMORTAL, 0, AREG_DEL_AEMAIL},
    {(char *) "clean", 1, CA_IMMORTAL, 0, AREG_CLEAN},
    {NULL, 0, 0, 0, 0}};

NAMETAB attrib_sw[] =
{
    {(char *) "access", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, ATTRIB_ACCESS},
    {(char *) "delete", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, ATTRIB_DELETE},
    {(char *) "rename", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, ATTRIB_RENAME},
    {NULL, 0, 0, 0, 0}};

NAMETAB blacklist_sw[] =
{
    {(char *) "list", 2, CA_IMMORTAL, 0, BLACKLIST_LIST},
    {(char *) "clear", 1, CA_IMMORTAL, 0, BLACKLIST_CLEAR},
    {(char *) "load", 2, CA_IMMORTAL, 0, BLACKLIST_LOAD},
    {NULL, 0, 0, 0, 0}};

NAMETAB boot_sw[] =
{
    {(char *) "port", 1, CA_WIZARD, 0, BOOT_PORT | SW_MULTIPLE},
    {(char *) "quiet", 1, CA_WIZARD, 0, BOOT_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB chown_sw[] = {
  {(char *) "preserve", 1, CA_WIZARD, 0, CHOWN_PRESERVE},
  {NULL, 0, 0, 0, 0}
};

NAMETAB chownall_sw[] = {
  {(char *) "preserve", 2, CA_WIZARD, 0, CHOWN_PRESERVE},
  {(char *) "room", 1, CA_WIZARD, 0, CHOWN_ROOM | SW_MULTIPLE},
  {(char *) "exit", 1, CA_WIZARD, 0, CHOWN_EXIT | SW_MULTIPLE},
  {(char *) "player", 2, CA_WIZARD, 0, CHOWN_PLAYER | SW_MULTIPLE},
  {(char *) "thing", 1, CA_WIZARD, 0, CHOWN_THING | SW_MULTIPLE},
  {NULL, 0, 0, 0, 0}
};

NAMETAB cluster_sw[] = {
  {(char *) "new", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_NEW},
  {(char *) "add", 2, CA_WIZARD, CA_CLUSTER, CLUSTER_ADD},
  {(char *) "delete", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_DEL},
  {(char *) "clear", 2, CA_WIZARD, CA_CLUSTER, CLUSTER_CLEAR},
  {(char *) "list", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_LIST},
  {(char *) "threshold", 2, CA_WIZARD, CA_CLUSTER, CLUSTER_THRESHOLD},
  {(char *) "action", 2, CA_WIZARD, CA_CLUSTER, CLUSTER_ACTION},
  {(char *) "edit", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_EDIT},
  {(char *) "repair", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_REPAIR},
  {(char *) "cut", 2, CA_WIZARD, CA_CLUSTER, CLUSTER_CUT},
  {(char *) "set", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_SET},
  {(char *) "wipe", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_WIPE},
  {(char *) "grep", 1, CA_WIZARD, CA_CLUSTER, CLUSTER_GREP},
  {(char *) "reaction", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_REACTION},
  {(char *) "trigger", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_TRIGGER},
  {(char *) "function", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_FUNC | SW_MULTIPLE},
  {(char *) "regexp", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_REGEXP | SW_MULTIPLE},
  {(char *) "preserve", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_PRESERVE | SW_MULTIPLE},
  {(char *) "owner", 3, CA_WIZARD, CA_CLUSTER, CLUSTER_OWNER | SW_MULTIPLE},
  {NULL, 0, 0, 0, 0}
};

NAMETAB cpattr_sw[] =
{
    {(char *) "clear", 1, CA_PUBLIC, 0, CPATTR_CLEAR},
    {(char *) "verbose", 1, CA_PUBLIC, 0, CPATTR_VERB | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB clone_sw[] =
{
    {(char *) "cost", 1, CA_PUBLIC, 0, CLONE_SET_COST},
    {(char *) "inherit", 3, CA_PUBLIC, 0, CLONE_INHERIT | SW_MULTIPLE},
    {(char *) "inventory", 3, CA_PUBLIC, 0, CLONE_INVENTORY},
    {(char *) "location", 1, CA_PUBLIC, 0, CLONE_LOCATION},
    {(char *) "parent", 2, CA_PUBLIC, 0, CLONE_PARENT | SW_MULTIPLE},
    {(char *) "preserve", 2, CA_WIZARD, 0, CLONE_PRESERVE | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB convert_sw[] =
{
    {(char *) "alternate", 3, CA_IMMORTAL, 0, CONV_ALTERNATE},
    {(char *) "all", 3, CA_IMMORTAL, 0, CONV_ALL | SW_MULTIPLE},
    {(char *) "override", 1, CA_IMMORTAL, 0, CONV_OVER | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB decomp_sw[] =
{
    {(char *) "all", 2, CA_PUBLIC, 0, DECOMP_ALL},
    {(char *) "flags", 1, CA_PUBLIC, 0, DECOMP_FLAGS},
    {(char *) "attribs", 2, CA_PUBLIC, 0, DECOMP_ATTRS},
    {(char *) "tree", 2, CA_PUBLIC, 0, DECOMP_TREE},
    {(char *) "regexp", 1, CA_PUBLIC, 0, DECOMP_REGEXP},
    {(char *) "tf", 2, CA_PUBLIC, 0, DECOMP_TF | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};
   
NAMETAB dbclean_sw[] =
{
    {(char *) "check", 1, CA_IMMORTAL, 0, DBCLEAN_CHECK},
    {NULL, 0, 0, 0, 0}};

NAMETAB dbck_sw[] =
{
    {(char *) "full", 1, CA_IMMORTAL, 0, DBCK_FULL},
    {NULL, 0, 0, 0, 0}};

/* This left null on purpose for '/instant' */
NAMETAB destroy_sw[] =
{
    {(char *) "override", 8, CA_PUBLIC, 0, DEST_OVERRIDE},
    {(char *) "purge", 1, CA_PUBLIC, 0, DEST_PURGE | SW_MULTIPLE},
    {(char *) "instant", 1, CA_PUBLIC, 0, 0},
    {NULL, 0, 0, 0, 0}};

NAMETAB dig_sw[] =
{
    {(char *) "teleport", 1, CA_PUBLIC, 0, DIG_TELEPORT},
    {NULL, 0, 0, 0, 0}};

NAMETAB doing_sw[] =
{
    {(char *) "header", 1, CA_WIZARD, 0, DOING_HEADER},
    {(char *) "message", 1, CA_PUBLIC, 0, DOING_MESSAGE},
    {(char *) "poll", 1, CA_PUBLIC, 0, DOING_POLL},
    {(char *) "unique", 1, CA_PUBLIC, 0, DOING_UNIQUE},
    {NULL, 0, 0, 0, 0}};

NAMETAB door_sw[] =
{
    {(char *) "list", 1, CA_PUBLIC, 0, DOOR_SW_LIST},
    {(char *) "open", 1, CA_PUBLIC, 0, DOOR_SW_OPEN},
    {(char *) "push", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, DOOR_SW_PUSH},
    {(char *) "kick", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, DOOR_SW_KICK},
    {(char *) "close", 1, CA_PUBLIC, 0, DOOR_SW_CLOSE},
    {(char *) "status", 1, CA_GOD | CA_IMMORTAL, 0, DOOR_SW_STATUS},
    {(char *) "full", 1, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0, DOOR_SW_FULL | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB dolist_sw[] =
{
    {(char *) "delimit", 1, CA_PUBLIC, 0, DOLIST_DELIMIT},
    {(char *) "space", 1, CA_PUBLIC, 0, DOLIST_SPACE},
    {(char *) "notify", 3, CA_PUBLIC, 0, DOLIST_NOTIFY | SW_MULTIPLE},
    {(char *) "pid", 1, CA_PUBLIC, 0, DOLIST_PID},
    {(char *) "nobreak", 3, CA_PUBLIC, 0, DOLIST_NOBREAK | SW_MULTIPLE},
    {(char *) "clearreg", 1, CA_PUBLIC, 0, DOLIST_CLEARREG | SW_MULTIPLE},
    {(char *) "inline", 1, CA_PUBLIC, 0, DOLIST_INLINE | SW_MULTIPLE},
    {(char *) "localize", 1, CA_PUBLIC, 0, DOLIST_LOCALIZE | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB drop_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, DROP_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB dump_sw[] =
{
    {(char *) "structure", 1, CA_WIZARD, 0, DUMP_STRUCT | SW_MULTIPLE},
    {(char *) "text", 1, CA_WIZARD, 0, DUMP_TEXT | SW_MULTIPLE},
    {(char *) "flat", 1, CA_WIZARD, 0, DUMP_FLAT | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB dynhelp_sw[] =
{
    {(char *) "parse", 1, CA_WIZARD, 0, DYN_PARSE},
    {(char *) "search", 1, CA_WIZARD, 0, DYN_SEARCH},
    {(char *) "nolabel", 1, CA_WIZARD, 0, DYN_NOLABEL | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB edit_sw[] =
{
    {(char *) "check", 1, CA_PUBLIC, 0, EDIT_CHECK | SW_MULTIPLE},
    {(char *) "single", 2, CA_PUBLIC, 0, EDIT_SINGLE},
    {(char *) "strict", 2, CA_PUBLIC, 0, EDIT_STRICT | SW_MULTIPLE},
    {(char *) "raw", 1, CA_PUBLIC, 0, EDIT_RAW | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB emit_sw[] =
{
    {(char *) "here", 1, CA_PUBLIC, 0, SAY_HERE | SW_MULTIPLE},
    {(char *) "room", 1, CA_PUBLIC, 0, SAY_ROOM | SW_MULTIPLE},
    {(char *) "sub", 1, CA_PUBLIC, 0, SAY_SUBSTITUTE | SW_MULTIPLE},
    {(char *) "noansi", 1, CA_PUBLIC, 0, SAY_NOANSI | SW_MULTIPLE},
    //    {(char *) "noeval", 1, CA_PUBLIC, 0, SAY_NOEVAL | SW_MULTIPLE}, 
    {NULL, 0, 0, 0, 0}};

NAMETAB enter_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, MOVE_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB examine_sw[] =
{
    {(char *) "brief", 1, CA_PUBLIC, 0, EXAM_BRIEF},
    {(char *) "debug", 1, CA_WIZARD | CA_ADMIN | CA_BUILDER, 0, EXAM_DEBUG},
    {(char *) "full", 1, CA_PUBLIC, 0, EXAM_LONG},
    {(char *) "parent", 1, CA_PUBLIC, 0, EXAM_PARENT},
    {(char *) "quick", 1, CA_PUBLIC, 0, EXAM_QUICK},
    {(char *) "tree", 1, CA_PUBLIC, 0, EXAM_TREE | SW_MULTIPLE},
    {(char *) "regexp", 1, CA_PUBLIC, 0, EXAM_REGEXP | SW_MULTIPLE},
    {(char *) "cluster", 1, CA_PUBLIC, 0, EXAM_CLUSTER},
    {NULL, 0, 0, 0, 0}};

NAMETAB extansi_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, SET_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB femit_sw[] =
{
    {(char *) "here", 1, CA_PUBLIC, 0, PEMIT_HERE | SW_MULTIPLE},
    {(char *) "room", 1, CA_PUBLIC, 0, PEMIT_ROOM | SW_MULTIPLE},
    {(char *) "noeval", 1, CA_PUBLIC, 0, PEMIT_NOEVAL | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB fixdb_sw[] =
{
/* {(char *)"add_pname",1,      CA_IMMORTAL,            0, FIXDB_ADD_PN}, */
    {(char *) "contents", 1, CA_IMMORTAL, 0, FIXDB_CON},
    {(char *) "exits", 1, CA_IMMORTAL, 0, FIXDB_EXITS},
    {(char *) "location", 1, CA_IMMORTAL, 0, FIXDB_LOC},
    {(char *) "next", 1, CA_IMMORTAL, 0, FIXDB_NEXT},
    {(char *) "owner", 1, CA_IMMORTAL, 0, FIXDB_OWNER},
    {(char *) "pennies", 1, CA_IMMORTAL, 0, FIXDB_PENNIES},
    {(char *) "rename", 1, CA_IMMORTAL, 0, FIXDB_NAME},
    {(char *) "type", 1, CA_IMMORTAL, 0, FIXDB_TYPE},
/* {(char *)"rm_pname", 1,      CA_IMMORTAL,            0, FIXDB_DEL_PN}, */
    {NULL, 0, 0, 0, 0}};

NAMETAB flag_sw[] =
{
    {(char *) "remove", 1, CA_IMMORTAL, 0, FLAGSW_REMOVE},
    {NULL, 0, 0, 0, 0}};

NAMETAB flagdef_sw[] =
{
    {(char *) "set", 2, CA_IMMORTAL, 0, FLAGDEF_SET},
    {(char *) "unset", 1, CA_IMMORTAL, 0, FLAGDEF_UNSET},
    {(char *) "see", 2, CA_IMMORTAL, 0, FLAGDEF_SEE},
    {(char *) "list", 2, CA_IMMORTAL, 0, FLAGDEF_LIST},
    {(char *) "letter", 2, CA_IMMORTAL, 0, FLAGDEF_CHAR},
    {(char *) "index", 2, CA_IMMORTAL, 0, FLAGDEF_INDEX},
    {(char *) "type", 2, CA_IMMORTAL, 0, FLAGDEF_TYPE},
    {NULL, 0, 0, 0, 0}};

NAMETAB oemit_sw[] =
{
    {(char *) "noansi", 1, CA_PUBLIC, 0, PEMIT_NOANSI | SW_MULTIPLE},
    {(char *) "multi", 1, CA_PUBLIC, 0, PEMIT_OSTR},
    {NULL, 0, 0, 0, 0}};

NAMETAB say_sw[] =
{
    {(char *) "noansi", 1, CA_PUBLIC, 0, SAY_NOANSI | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB think_sw[] =
{
    {(char *) "noansi", 1, CA_PUBLIC, 0, SAY_NOANSI | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB fsay_sw[] =
{
    {NULL, 0, 0, 0, 0}};

NAMETAB fpose_sw[] =
{
    {(char *) "default", 1, CA_PUBLIC, 0, 0},
    {(char *) "nospace", 1, CA_PUBLIC, 0, SAY_NOSPACE},
    {NULL, 0, 0, 0, 0}};

NAMETAB lfunction_sw[] =
{
    {(char *) "privileged", 3, CA_PUBLIC, 0, FN_PRIV|FN_LOCAL},
    {(char *) "protected", 3, CA_PUBLIC, 0, FN_PROTECT|SW_MULTIPLE|FN_LOCAL},
    {(char *) "list", 1, CA_PUBLIC, 0, FN_LIST|FN_LOCAL},
    {(char *) "preserve", 3, CA_PUBLIC, 0, FN_PRES|SW_MULTIPLE|FN_LOCAL},
    {(char *) "delete", 2, CA_PUBLIC, 0, FN_DEL|FN_LOCAL},
    {(char *) "display", 2, CA_PUBLIC, 0, FN_DISPLAY|FN_LOCAL},
    {(char *) "minimum", 2, CA_PUBLIC, 0, FN_MIN|FN_LOCAL},
    {(char *) "maximum", 2, CA_PUBLIC, 0, FN_MAX|FN_LOCAL},
    {(char *) "notrace", 2, CA_PUBLIC, 0, FN_NOTRACE|SW_MULTIPLE|FN_LOCAL},
    {NULL, 0, 0, 0, 0}};

NAMETAB function_sw[] =
{
    {(char *) "privileged", 3, CA_WIZARD, 0, FN_PRIV},
    {(char *) "protected", 3, CA_WIZARD, 0, FN_PROTECT|SW_MULTIPLE},
    {(char *) "list", 1, CA_WIZARD, 0, FN_LIST},
    {(char *) "preserve", 3, CA_WIZARD, 0, FN_PRES|SW_MULTIPLE},
    {(char *) "delete", 2, CA_WIZARD, 0, FN_DEL},
    {(char *) "display", 2, CA_WIZARD, 0, FN_DISPLAY},
    {(char *) "minimum", 2, CA_WIZARD, 0, FN_MIN},
    {(char *) "maximum", 2, CA_WIZARD, 0, FN_MAX},
    {(char *) "notrace", 2, CA_WIZARD, 0, FN_NOTRACE|SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB genhelp_sw[] =
{
    {(char *) "search", 1, CA_PUBLIC, 0, HELP_SEARCH},
    {NULL, 0, 0, 0, 0}};

NAMETAB get_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, GET_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB give_sw[] =
{
    {(char *) "quiet", 1, CA_WIZARD, 0, GIVE_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB goto_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, MOVE_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB grep_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, GREP_QUIET},
    {(char *) "regexp", 1, CA_PUBLIC, 0, GREP_REGEXP | SW_MULTIPLE},
    {(char *) "parent", 1, CA_PUBLIC, 0, GREP_PARENT | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB halt_sw[] =
{
    {(char *) "all", 1, CA_PUBLIC, 0, HALT_ALL},
    {(char *) "pid", 1, CA_PUBLIC, 0, HALT_PID},
    {(char *) "stop", 1, CA_PUBLIC, 0, HALT_PIDSTOP | SW_MULTIPLE},
    {(char *) "cont", 1, CA_PUBLIC, 0, HALT_PIDCONT | SW_MULTIPLE},
    {(char *) "quiet", 1, CA_PUBLIC, 0, HALT_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB hide_sw[] =
{
    {(char *) "on", 1, CA_PUBLIC, 0, HIDE_ON},
    {(char *) "off", 1, CA_PUBLIC, 0, HIDE_OFF},
    {NULL, 0, 0, 0, 0}};

NAMETAB icmd_sw[] =
{
    {(char *) "check", 2, CA_IMMORTAL, 0, ICMD_CHECK},
    {(char *) "disable", 2, CA_IMMORTAL, 0, ICMD_DISABLE},
    {(char *) "ignore", 2, CA_IMMORTAL, 0, ICMD_IGNORE},
    {(char *) "eval", 2, CA_IMMORTAL, 0, ICMD_IGNORE},
    {(char *) "on", 2, CA_IMMORTAL, 0, ICMD_ON},
    {(char *) "off", 2, CA_IMMORTAL, 0, ICMD_OFF},
    {(char *) "clear", 2, CA_IMMORTAL, 0, ICMD_CLEAR},
    {(char *) "droom", 2, CA_IMMORTAL, 0, ICMD_DROOM},
    {(char *) "iroom", 2, CA_IMMORTAL, 0, ICMD_IROOM},
    {(char *) "croom", 2, CA_IMMORTAL, 0, ICMD_CROOM},
    {(char *) "lroom", 2, CA_IMMORTAL, 0, ICMD_LROOM},
    {(char *) "lallroom", 2, CA_IMMORTAL, 0, ICMD_LALLROOM},
    {NULL, 0, 0, 0, 0}};

NAMETAB kick_sw[] =
{
    {(char *) "pid", 1, CA_WIZARD, 0, QUEUE_KICK_PID},
    {NULL, 0, 0, 0, 0}};

NAMETAB leave_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, MOVE_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB limit_sw[] =
{
    {(char *) "list", 2, CA_WIZARD, 0, LIMIT_LIST},
    {(char *) "vmod", 1, CA_WIZARD, 0, LIMIT_VADD},
    {(char *) "dmod", 1, CA_WIZARD, 0, LIMIT_DADD},
    {(char *) "reset", 1, CA_WIZARD, 0, LIMIT_RESET},
    {(char *) "lfunction", 2, CA_WIZARD, 0, LIMIT_LFUN},
    {NULL, 0, 0, 0, 0}};

NAMETAB listmotd_sw[] =
{
    {(char *) "brief", 1, CA_WIZARD, 0, MOTD_BRIEF},
    {NULL, 0, 0, 0, 0}};

NAMETAB lock_sw[] =
{
    {(char *) "controllock", 1, CA_PUBLIC, 0, A_LCONTROL},
    {(char *) "defaultlock", 1, CA_PUBLIC, 0, A_LOCK},
    {(char *) "droplock", 1, CA_PUBLIC, 0, A_LDROP},
    {(char *) "enterlock", 1, CA_PUBLIC, 0, A_LENTER},
    {(char *) "givelock", 5, CA_PUBLIC, 0, A_LGIVE},
    {(char *) "leavelock", 2, CA_PUBLIC, 0, A_LLEAVE},
    {(char *) "linklock", 2, CA_PUBLIC, 0, A_LLINK},
    {(char *) "pagelock", 3, CA_PUBLIC, 0, A_LPAGE},
    {(char *) "parentlock", 3, CA_PUBLIC, 0, A_LPARENT},
    {(char *) "receivelock", 1, CA_PUBLIC, 0, A_LRECEIVE},
    {(char *) "speechlock", 1, CA_PUBLIC, 0, A_LSPEECH},
    {(char *) "teloutlock", 2, CA_PUBLIC, 0, A_LTELOUT},
    {(char *) "tportlock", 2, CA_PUBLIC, 0, A_LTPORT},
    {(char *) "uselock", 1, CA_PUBLIC, 0, A_LUSE},
    {(char *) "userlock", 4, CA_PUBLIC, 0, A_LUSER},
    {(char *) "zonetolock", 5, CA_PUBLIC, 0, A_LZONETO},
    {(char *) "zonewizlock", 5, CA_PUBLIC, 0, A_LZONEWIZ},
    {(char *) "twinklock", 2, CA_PUBLIC, 0, A_LTWINK},
    {(char *) "darklock", 3, CA_PUBLIC, 0, A_LDARK},
    {(char *) "openlock", 3, CA_PUBLIC, 0, A_LOPEN},
    {(char *) "droptolock", 5, CA_PUBLIC, 0, A_LDROPTO},
    {(char *) "altnamelock", 5, CA_WIZARD, 0, A_LALTNAME},
    {(char *) "givetolock", 5, CA_PUBLIC, 0, A_LGIVETO},
    {(char *) "getfromlock", 3, CA_PUBLIC, 0, A_LGETFROM},
    {(char *) "chownlock", 3, CA_PUBLIC, 0, A_LCHOWN},
    {(char *) "basic", 2, CA_PUBLIC, 0, 0},
    {NULL, 0, 0, 0, 0}};

NAMETAB zone_sw[] = 
{
    {(char *) "add", 1, CA_PUBLIC, 0, ZONE_ADD},
    {(char *) "delete", 1, CA_PUBLIC, 0, ZONE_DELETE},
    {(char *) "purge", 1, CA_PUBLIC, 0, ZONE_PURGE},
    {(char *) "replace", 1, CA_PUBLIC, 0, ZONE_REPLACE},
    {(char *) "list", 1, CA_PUBLIC, 0, ZONE_LIST},
    {NULL, 0, 0, 0, 0}};

NAMETAB touch_sw[] =
{
    {NULL, 0, 0, 0, 0}};

NAMETAB taste_sw[] =
{
    {NULL, 0, 0, 0, 0}};

NAMETAB listen_sw[] =
{
    {NULL, 0, 0, 0, 0}};

NAMETAB smell_sw[] =
{
    {NULL, 0, 0, 0, 0}};

NAMETAB look_sw[] =
{
    {(char *) "outside", 1, CA_PUBLIC, 0, LOOK_OUTSIDE},
    {NULL, 0, 0, 0, 0}};

/* MMMail mail switch */
NAMETAB mail_sw[] =
{
    {(char *) "send", 3, CA_PUBLIC, 0, M_SEND},
    {(char *) "reply", 3, CA_PUBLIC, 0, M_REPLY},
    {(char *) "forward", 2, CA_PUBLIC, 0, M_FORWARD},
    {(char *) "read", 3, CA_PUBLIC, 0, M_READM},
    {(char *) "lock", 3, CA_PUBLIC, 0, M_LOCK},
    {(char *) "status", 2, CA_PUBLIC, 0, M_STATUS},
    {(char *) "delete", 1, CA_PUBLIC, 0, M_DELETE},
    {(char *) "recall", 3, CA_PUBLIC, 0, M_RECALL},
    {(char *) "mark", 3, CA_PUBLIC, 0, M_MARK},
    {(char *) "save", 2, CA_PUBLIC, 0, M_SAVE | SW_MULTIPLE},
    {(char *) "write", 2, CA_PUBLIC, 0, M_WRITE},
    {(char *) "reject", 3, CA_PUBLIC, 0, M_REJECT},
    {(char *) "number", 2, CA_PUBLIC, 0, M_NUMBER},
    {(char *) "check", 2, CA_PUBLIC, 0, M_CHECK},
    {(char *) "unmark", 3, CA_PUBLIC, 0, M_UNMARK},
    {(char *) "quick", 3, CA_PUBLIC, 0, M_QUICK},
    {(char *) "share", 2, CA_PUBLIC, 0, M_SHARE},
    {(char *) "password", 3, CA_PUBLIC, 0, M_PASS},
    {(char *) "page", 3, CA_PUBLIC, 0, M_PAGE},
    {(char *) "quota", 3, CA_PUBLIC, 0, M_QUOTA},
    {(char *) "fsend", 2, CA_IMMORTAL, 0, M_FSEND | SW_MULTIPLE},
    {(char *) "zap", 1, CA_PUBLIC, 0, M_ZAP},
    {(char *) "next", 2, CA_PUBLIC, 0, M_NEXT},
    {(char *) "alias", 3, CA_PUBLIC, 0, M_ALIAS},
    {(char *) "autofor", 2, CA_PUBLIC, 0, M_AUTOFOR},
    {(char *) "version", 1, CA_PUBLIC, 0, M_VERSION},
    {(char *) "all", 3, CA_PUBLIC, 0, M_ALL | SW_MULTIPLE},
    {(char *) "anonymous", 4, CA_PUBLIC, 0, M_ANON | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB wmail_sw[] =
{
    {(char *) "clean", 2, CA_IMMORTAL, 0, WM_CLEAN},
    {(char *) "load", 3, CA_IMMORTAL, 0, WM_LOAD},
    {(char *) "unload", 4, CA_IMMORTAL, 0, WM_UNLOAD},
    {(char *) "wipe", 2, CA_IMMORTAL | CA_WIZARD | CA_ADMIN, 0, WM_WIPE},
    {(char *) "on", 2, CA_IMMORTAL | CA_WIZARD, 0, WM_ON},
    {(char *) "off", 3, CA_IMMORTAL | CA_WIZARD, 0, WM_OFF},
    {(char *) "access", 2, CA_IMMORTAL | CA_WIZARD, 0, WM_ACCESS},
    {(char *) "override", 4, CA_GOD | CA_WIZARD, 0, WM_OVER},
    {(char *) "restart", 3, CA_WIZARD, 0, WM_RESTART},
    {(char *) "size", 2, CA_PUBLIC, 0, WM_SIZE},
    {(char *) "fix", 3, CA_IMMORTAL, 0, WM_FIX},
    {(char *) "lfix", 4, CA_IMMORTAL, 0, WM_LOADFIX},
    {(char *) "alias", 2, CA_WIZARD, 0, WM_ALIAS},
    {(char *) "remove", 3, CA_WIZARD, 0, WM_REMOVE | SW_MULTIPLE},
    {(char *) "lock", 3, CA_WIZARD, 0, WM_ALOCK | SW_MULTIPLE},
    {(char *) "time", 1, CA_IMMORTAL, 0, WM_MTIME},
    {(char *) "dtime", 1, CA_WIZARD, 0, WM_DTIME},
    {(char *) "smax", 2, CA_IMMORTAL, 0, WM_SMAX},
    {(char *) "verify", 1, CA_IMMORTAL, 0, WM_VERIFY},
    {NULL, 0, 0, 0, 0}};

NAMETAB mail2_sw[] =
{
    {(char *) "create", 2, CA_PUBLIC, 0, M2_CREATE},
    {(char *) "delete", 1, CA_PUBLIC, 0, M2_DELETE},
    {(char *) "rename", 1, CA_PUBLIC, 0, M2_RENAME},
    {(char *) "move", 1, CA_PUBLIC, 0, M2_MOVE},
    {(char *) "list", 1, CA_PUBLIC, 0, M2_LIST},
    {(char *) "current", 2, CA_PUBLIC, 0, M2_CURRENT},
    {(char *) "change", 2, CA_PUBLIC, 0, M2_CHANGE},
    {(char *) "plist", 1, CA_PUBLIC, 0, M2_PLIST},
    {(char *) "share", 1, CA_PUBLIC, 0, M2_SHARE},
    {(char *) "cshare", 2, CA_PUBLIC, 0, M2_CSHARE},
    {NULL, 0, 0, 0, 0}};


NAMETAB mark_sw[] =
{
    {(char *) "set", 1, CA_PUBLIC, 0, MARK_SET},
    {(char *) "clear", 1, CA_PUBLIC, 0, MARK_CLEAR},
    {NULL, 0, 0, 0, 0}};

NAMETAB markall_sw[] =
{
    {(char *) "set", 1, CA_PUBLIC, 0, MARK_SET},
    {(char *) "clear", 1, CA_PUBLIC, 0, MARK_CLEAR},
    {NULL, 0, 0, 0, 0}};

NAMETAB motd_sw[] =
{
    {(char *) "brief", 1, CA_WIZARD, 0, MOTD_BRIEF | SW_MULTIPLE},
    {(char *) "connect", 1, CA_WIZARD, 0, MOTD_ALL},
    {(char *) "down", 1, CA_WIZARD, 0, MOTD_DOWN},
    {(char *) "full", 1, CA_WIZARD, 0, MOTD_FULL},
    {(char *) "list", 1, CA_PUBLIC, 0, MOTD_LIST},
    {(char *) "wizard", 1, CA_WIZARD, 0, MOTD_WIZ},
    {NULL, 0, 0, 0, 0}};

NAMETAB newsdb_sw[] =
{
    {(char *) "dbinfo", 3, CA_IMMORTAL, 0, NEWSDB_DBINFO},
    {(char *) "dbck", 3, CA_IMMORTAL, 0, NEWSDB_DBCK},
    {(char *) "load", 1, CA_IMMORTAL, 0, NEWSDB_LOAD},
    {(char *) "unload", 1, CA_IMMORTAL, 0, NEWSDB_UNLOAD},
    {NULL, 0, 0, 0, 0}};

NAMETAB news_sw[] =
{
    {(char *) "adminlock", 2, CA_IMMORTAL | CA_WIZARD, 0, NEWS_ADMINLOCK},
    {(char *) "articlelife", 2, CA_IMMORTAL | CA_WIZARD, 0, NEWS_ARTICLELIFE},
    {(char *) "check", 1, CA_PUBLIC, 0, NEWS_CHECK},
    {(char *) "expire", 3, CA_IMMORTAL, 0, NEWS_EXPIRE},
    {(char *) "extend", 3, CA_PUBLIC, 0, NEWS_EXTEND},
    {(char *) "groupadd", 7, CA_IMMORTAL | CA_WIZARD, 0, NEWS_GROUPADD},
    {(char *) "groupdel", 6, CA_IMMORTAL, 0, NEWS_GROUPDEL},
    {(char *) "groupinfo", 6, CA_PUBLIC, 0, NEWS_GROUPINFO},
    {(char *) "grouplimit", 8, CA_IMMORTAL | CA_WIZARD, 0, NEWS_GROUPLIMIT},
    {(char *) "grouplist", 8, CA_PUBLIC , 0, NEWS_GROUPLIST},
    {(char *) "groupmem", 6, CA_PUBLIC, 0, NEWS_GROUPMEM},
    {(char *) "jump", 1, CA_PUBLIC, 0, NEWS_JUMP},
    {(char *) "login", 1, CA_PUBLIC, 0, NEWS_LOGIN},
    {(char *) "mailto", 1, CA_PUBLIC, 0, NEWS_MAILTO},
    {(char *) "off", 2, CA_IMMORTAL | CA_WIZARD, 0, NEWS_OFF},
    {(char *) "on", 2, CA_IMMORTAL | CA_WIZARD, 0, NEWS_ON},
    {(char *) "post", 4, CA_PUBLIC, 0, NEWS_POST},
    {(char *) "postlist", 6, CA_PUBLIC, 0, NEWS_POSTLIST},
    {(char *) "postlock", 6, CA_PUBLIC, 0, NEWS_POSTLOCK},
    {(char *) "repost", 3, CA_PUBLIC, 0, NEWS_REPOST},
    {(char *) "read", 4, CA_PUBLIC, 0, NEWS_READ},
    {(char *) "readall", 5, CA_PUBLIC, 0, NEWS_READALL},
    {(char *) "readlock", 5, CA_PUBLIC, 0, NEWS_READLOCK},
    {(char *) "status", 2, CA_PUBLIC, 0, NEWS_STATUS},
    {(char *) "subscribe", 2, CA_PUBLIC, 0, NEWS_SUBSCRIBE},
    {(char *) "unsubscribe", 1, CA_PUBLIC, 0, NEWS_UNSUBSCRIBE},
    {(char *) "userinfo", 5, CA_PUBLIC, 0, NEWS_USERINFO},
    {(char *) "userlimit", 5, CA_IMMORTAL | CA_WIZARD, 0, NEWS_USERLIMIT},
    {(char *) "usermem", 5, CA_IMMORTAL | CA_WIZARD, 0, NEWS_USERMEM},
    {(char *) "verbose", 1, CA_PUBLIC, 0, NEWS_VERBOSE | SW_MULTIPLE},
    {(char *) "yank", 1, CA_PUBLIC, 0, NEWS_YANK},
/*  {(char *) "search", 2, CA_PUBLIC, 0, HELP_SEARCH}, */
    {NULL, 0, 0, 0, 0}};

NAMETAB drain_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, NFY_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};


NAMETAB sudo_sw[] =
{
    {(char *) "globalize", 2, CA_PUBLIC, 0, SUDO_GLOBAL},
    {(char *) "clearregs", 2, CA_PUBLIC, 0, SUDO_CLEAR},
    {NULL, 0, 0, 0, 0}};

NAMETAB include_sw[] =
{
    {(char *) "command", 2, CA_PUBLIC, 0, INCLUDE_COMMAND},
    {(char *) "localize", 2, CA_PUBLIC, 0, INCLUDE_LOCAL | SW_MULTIPLE},
    {(char *) "clearregs", 2, CA_PUBLIC, 0, INCLUDE_CLEAR | SW_MULTIPLE},
    {(char *) "nobreak", 2, CA_PUBLIC, 0, INCLUDE_NOBREAK | SW_MULTIPLE},
    {(char *) "target", 2, CA_PUBLIC, 0, INCLUDE_TARGET | SW_MULTIPLE},
    {(char *) "override", 2, CA_PUBLIC, 0, INCLUDE_OVERRIDE | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB notify_sw[] =
{
    {(char *) "all", 1, CA_PUBLIC, 0, NFY_NFYALL},
    {(char *) "first", 1, CA_PUBLIC, 0, NFY_NFY},
    {(char *) "quiet", 1, CA_PUBLIC, 0, NFY_QUIET | SW_MULTIPLE},
    {(char *) "pid", 1, CA_PUBLIC, 0, NFY_PID},
    {NULL, 0, 0, 0, 0}};

NAMETAB nuke_sw[] =
{
    {(char *) "purge", 1, CA_IMMORTAL | CA_WIZARD | CA_ADMIN, 0, NUKE_PURGE},
    {NULL, 0, 0, 0, 0}};

NAMETAB open_sw[] =
{
    {(char *) "inventory", 1, CA_PUBLIC, 0, OPEN_INVENTORY},
    {(char *) "location", 1, CA_PUBLIC, 0, OPEN_LOCATION},
    {NULL, 0, 0, 0, 0}};

NAMETAB newpassword_sw[] =
{
    {(char *) "des", 1, CA_WIZARD, 0, NEWPASSWORD_DES},
    {NULL, 0, 0, 0, 0}};

NAMETAB pcreate_sw[] =
{
    {(char *) "register", 1, CA_WIZARD, 0, PCRE_REG},
    {NULL, 0, 0, 0, 0}};

NAMETAB pipe_sw[] =
{
    {(char *) "on", 1, CA_PUBLIC, 0, PIPE_ON},
    {(char *) "off", 1, CA_PUBLIC, 0, PIPE_OFF},
    {(char *) "tee", 1, CA_PUBLIC, 0, PIPE_TEE},
    {(char *) "status", 1, CA_PUBLIC, 0, PIPE_STATUS},
    {NULL, 0, 0, 0, 0}};

NAMETAB pemit_sw[] =
{
    {(char *) "contents", 1, CA_PUBLIC, 0, PEMIT_CONTENTS | SW_MULTIPLE},
    {(char *) "list", 1, CA_PUBLIC, 0, PEMIT_LIST | SW_MULTIPLE},
    {(char *) "object", 2, CA_PUBLIC, 0, 0},
    {(char *) "port", 1, CA_WIZARD, 0, PEMIT_PORT},
    {(char *) "noisy", 3, CA_PUBLIC, 0, PEMIT_NOISY | SW_MULTIPLE},
    {(char *) "zone", 1, CA_PUBLIC, 0, PEMIT_ZONE},
    {(char *) "silent", 1, CA_PUBLIC, 0, 0},
    {(char *) "nosub", 3, CA_PUBLIC, 0, PEMIT_NOSUB | SW_MULTIPLE},
    {(char *) "noansi", 3, CA_PUBLIC, 0, PEMIT_NOANSI | SW_MULTIPLE},
    {(char *) "noeval", 3, CA_PUBLIC, 0, PEMIT_NOEVAL | SW_MULTIPLE}, 
    {(char *) "oneeval", 2, CA_PUBLIC, 0, PEMIT_ONEEVAL | SW_MULTIPLE},
#ifdef REALITY_LEVELS
    {(char *) "reality", 3, CA_PUBLIC, 0, PEMIT_REALITY | SW_MULTIPLE},
    {(char *) "toreality", 3, CA_PUBLIC, 0, PEMIT_TOREALITY | SW_MULTIPLE},
#endif
    {NULL, 0, 0, 0, 0}};

NAMETAB pose_sw[] =
{
    {(char *) "default", 1, CA_PUBLIC, 0, 0},
    {(char *) "nospace", 3, CA_PUBLIC, 0, SAY_NOSPACE},
    {(char *) "noansi", 3, CA_PUBLIC, 0, SAY_NOANSI | SW_MULTIPLE},
    //    {(char *) "noeval", 1, 0, CA_PUBLIC, SAY_PNOEVAL}, 
    {NULL, 0, 0, 0, 0}};

NAMETAB protect_sw[] =
{
    {(char *) "add", 2, CA_PUBLIC, 0, PROTECT_ADD},
    {(char *) "delete", 1, CA_PUBLIC, 0, PROTECT_DEL},
    {(char *) "list", 1, CA_PUBLIC, 0, PROTECT_LIST},
    {(char *) "byplayer", 1, CA_WIZARD, 0, PROTECT_BYPLAYER},
    {(char *) "summary", 1, CA_WIZARD, 0, PROTECT_SUMMARY},
    {(char *) "alias", 3, CA_PUBLIC, 0, PROTECT_ALIAS},
    {(char *) "unalias", 1, CA_PUBLIC, 0, PROTECT_UNALIAS},
    {(char *) "all", 3, CA_WIZARD, 0, PROTECT_LISTALL | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB power_sw[] =
{
    {(char *) "guildmaster", 1, CA_PUBLIC, 0, POWER_LEVEL_GUILD},
    {(char *) "architect", 1, CA_PUBLIC, 0, POWER_LEVEL_ARCH},
    {(char *) "councilor", 2, CA_PUBLIC, 0, POWER_LEVEL_COUNC},
    {(char *) "off", 1, CA_PUBLIC, 0, POWER_LEVEL_OFF},
    {(char *) "purge", 1, CA_PUBLIC, 0, POWER_PURGE},
    {(char *) "check", 2, CA_PUBLIC, 0, POWER_CHECK},
    {NULL, 0, 0, 0, 0}};

NAMETAB depower_sw[] =
{
    {(char *) "guildmaster", 1, CA_PUBLIC, 0, POWER_LEVEL_GUILD},
    {(char *) "architect", 1, CA_PUBLIC, 0, POWER_LEVEL_ARCH},
    {(char *) "councilor", 2, CA_IMMORTAL, 0, POWER_LEVEL_COUNC},
    {(char *) "off", 1, CA_PUBLIC, 0, POWER_LEVEL_OFF},
    {(char *) "purge", 1, CA_PUBLIC, 0, POWER_PURGE},
    {(char *) "check", 2, CA_PUBLIC, 0, POWER_CHECK},
    {(char *) "remove", 1, CA_PUBLIC, 0, POWER_REMOVE},
    {NULL, 0, 0, 0, 0}};

NAMETAB rpage_sw[] =
{
    {(char *) "noansi", 1, CA_PUBLIC, 0, PAGE_NOANSI | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB page_sw[] =
{
    {(char *) "port", 1, CA_WIZARD, 0, PAGE_PORT},
    {(char *) "location", 1, CA_WIZARD, 0, PAGE_LOC},
    {(char *) "noansi", 1, CA_PUBLIC, 0, PAGE_NOANSI | SW_MULTIPLE},
    //    {(char *) "noeval", 1, CA_WIZARD, 0, PAGE_NOEVAL},
    {NULL, 0, 0, 0, 0}};

NAMETAB quitprogram_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, QUITPRG_QUIET},
    {NULL, 0, 0, 0, 0}};

NAMETAB ps_sw[] =
{
    {(char *) "all", 1, CA_PUBLIC, 0, PS_ALL | SW_MULTIPLE},
    {(char *) "brief", 1, CA_PUBLIC, 0, PS_BRIEF},
    {(char *) "long", 1, CA_PUBLIC, 0, PS_LONG},
    {(char *) "summary", 1, CA_PUBLIC, 0, PS_SUMM},
    {(char *) "freeze", 1, CA_WIZARD, 0, PS_FREEZE | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB purge_sw[] =
{
    {(char *) "all", 1, CA_IMMORTAL, 0, PURGE_ALL},
    {(char *) "time", 2, CA_IMMORTAL, 0, PURGE_TIME},
    {(char *) "type", 2, CA_IMMORTAL, 0, PURGE_TYPE},
    {(char *) "owner", 1, CA_IMMORTAL, 0, PURGE_OWNER},
    {(char *) "ttype", 2, CA_IMMORTAL, 0, PURGE_TIMETYPE},
    {(char *) "towner", 2, CA_IMMORTAL, 0, PURGE_TIMEOWNER},
    {NULL, 0, 0, 0, 0}};

NAMETAB quota_sw[] =
{
    {(char *) "set", 1, CA_PUBLIC, 0, QUOTA_SET},
    {(char *) "max", 1, CA_WIZARD, 0, QUOTA_MAX},
    {(char *) "fix", 1, CA_WIZARD, 0, QUOTA_FIX},
    {(char *) "take", 2, CA_PUBLIC, 0, QUOTA_TAKE},
    {(char *) "xfer", 1, CA_PUBLIC, 0, QUOTA_XFER},
    {(char *) "redo", 2, CA_IMMORTAL, 0, QUOTA_REDO},
    {(char *) "lock", 1, CA_WIZARD, 0, QUOTA_LOCK},
    {(char *) "unlock", 1, CA_WIZARD, 0, QUOTA_UNLOCK},
    {(char *) "clear", 1, CA_PUBLIC, 0, QUOTA_CLEAR | SW_MULTIPLE},
    {(char *) "general", 1, CA_PUBLIC, 0, QUOTA_GEN | SW_MULTIPLE},
    {(char *) "thing", 2, CA_PUBLIC, 0, QUOTA_THING | SW_MULTIPLE},
    {(char *) "player", 1, CA_PUBLIC, 0, QUOTA_PLAYER | SW_MULTIPLE},
    {(char *) "exit", 1, CA_PUBLIC, 0, QUOTA_EXIT | SW_MULTIPLE},
    {(char *) "room", 2, CA_PUBLIC, 0, QUOTA_ROOM | SW_MULTIPLE},
    {(char *) "all", 1, CA_PUBLIC, 0, QUOTA_ALL | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB reboot_sw[] =
{
    {(char *) "port", 1, CA_IMMORTAL, 0, REBOOT_PORT},
    {(char *) "silent", 1, CA_IMMORTAL, 0, REBOOT_SILENT},
    {NULL, 0, 0, 0, 0}};

NAMETAB reclist_sw[] =
{
    {(char *) "type", 1, CA_IMMORTAL, 0, REC_TYPE},
    {(char *) "owner", 1, CA_IMMORTAL, 0, REC_OWNER},
    {(char *) "count", 1, CA_IMMORTAL, 0, REC_COUNT | SW_MULTIPLE},
    {(char *) "age", 1, CA_IMMORTAL, 0, REC_AGE},
    {(char *) "destroyer", 1, CA_IMMORTAL, 0, REC_DEST},
    {(char *) "free", 1, CA_IMMORTAL, 0, REC_FREE},
    {NULL, 0, 0, 0, 0}};

NAMETAB register_sw[] =
{
    {(char *) "message", 1, CA_PUBLIC, 0, REGISTER_MSG},
    {NULL, 0, 0, 0, 0}};

NAMETAB rwho_sw[] =
{
    {(char *) "start", 3, CA_PUBLIC, 0, RWHO_START},
    {(char *) "stop", 3, CA_PUBLIC, 0, RWHO_STOP},
    {NULL, 0, 0, 0, 0}};

NAMETAB selfboot_sw[] =
{
    {(char *) "list", 1, CA_PUBLIC, 0, SELFBOOT_LIST},
    {(char *) "port", 1, CA_PUBLIC, 0, SELFBOOT_PORT},
    {NULL, 0, 0, 0, 0}};

NAMETAB search_sw[] =
{
    {(char *) "nogarbage", 1, CA_IMMORTAL, 0, SEARCH_NOGARBAGE},
    {NULL, 0, 0, 0, 0}};

NAMETAB set_sw[] =
{
    {(char *) "quiet", 1, CA_PUBLIC, 0, SET_QUIET},
    {(char *) "noisy", 1, CA_PUBLIC, 0, SET_NOISY},
    {(char *) "tree", 1, CA_PUBLIC, 0, SET_TREE},
    {NULL, 0, 0, 0, 0}};

NAMETAB skip_sw[] =
{ 
    {(char *) "ifelse", 1, CA_PUBLIC, 0, SKIP_IFELSE},
    {NULL, 0, 0, 0, 0}};

NAMETAB site_sw[] =
{
  {(char *) "register", 1, CA_IMMORTAL, 0, SITE_REG},
  {(char *) "forbidden", 1, CA_IMMORTAL, 0, SITE_FOR},
  {(char *) "suspect", 1, CA_IMMORTAL, 0, SITE_SUS},
  {(char *) "noguest", 3, CA_IMMORTAL, 0, SITE_NOG},
  {(char *) "all", 1, CA_IMMORTAL, 0, SITE_ALL},
  {(char *) "permit", 1, CA_IMMORTAL, 0, SITE_PER},
  {(char *) "trust", 1, CA_IMMORTAL, 0, SITE_TRU},
  {(char *) "noautoreg", 6, CA_IMMORTAL, 0, SITE_NOAR},
  {(char *) "noauth", 6, CA_IMMORTAL, 0, SITE_NOAUTH},
  {(char *) "nodns", 3, CA_IMMORTAL, 0, SITE_NODNS},
  {(char *) "list", 2, CA_IMMORTAL, 0, SITE_LIST},
  {NULL, 0, 0, 0, 0}};

NAMETAB snapshot_sw[] =
{
  {(char *) "load", 2, CA_IMMORTAL, 0, SNAPSHOT_LOAD},
  {(char *) "unload", 2, CA_IMMORTAL, 0, SNAPSHOT_UNLOAD},
  {(char *) "list", 2, CA_IMMORTAL, 0, SNAPSHOT_LIST},
  {(char *) "delete", 2, CA_IMMORTAL, 0, SNAPSHOT_DEL},
  {(char *) "verify", 2, CA_IMMORTAL, 0, SNAPSHOT_VERIFY},
  {NULL, 0, 0, 0, 0}};

  /* snoop code -Thorin */
NAMETAB snoop_sw[] =
{
    {(char *) "on", 2, CA_GOD | CA_IMMORTAL, 0, SNOOP_ON},
    {(char *) "off", 2, CA_GOD | CA_IMMORTAL, 0, SNOOP_OFF},
    {(char *) "status", 1, CA_GOD | CA_IMMORTAL, 0, SNOOP_STAT},
    {(char *) "log", 1, CA_GOD | CA_IMMORTAL, 0, SNOOP_LOG | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB stats_sw[] =
{
    {(char *) "all", 1, CA_PUBLIC, 0, STAT_ALL},
    {(char *) "me", 1, CA_PUBLIC, 0, STAT_ME},
    {(char *) "player", 1, CA_PUBLIC, 0, STAT_PLAYER},
    {NULL, 0, 0, 0, 0}};

NAMETAB sweep_sw[] =
{
    {(char *) "commands", 3, CA_PUBLIC, 0, SWEEP_COMMANDS | SW_MULTIPLE},
    {(char *) "connected", 3, CA_PUBLIC, 0, SWEEP_CONNECT | SW_MULTIPLE},
    {(char *) "exits", 1, CA_PUBLIC, 0, SWEEP_EXITS | SW_MULTIPLE},
    {(char *) "here", 1, CA_PUBLIC, 0, SWEEP_HERE | SW_MULTIPLE},
    {(char *) "inventory", 1, CA_PUBLIC, 0, SWEEP_ME | SW_MULTIPLE},
    {(char *) "listeners", 1, CA_PUBLIC, 0, SWEEP_LISTEN | SW_MULTIPLE},
    {(char *) "players", 1, CA_PUBLIC, 0, SWEEP_PLAYER | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB switch_sw[] =
{
    {(char *) "all", 1, CA_PUBLIC, 0, SWITCH_ANY},
    {(char *) "default", 1, CA_PUBLIC, 0, SWITCH_DEFAULT},
    {(char *) "first", 1, CA_PUBLIC, 0, SWITCH_ONE},
    {(char *) "regall", 4, CA_PUBLIC, 0, SWITCH_REGANY},
    {(char *) "regfirst", 4, CA_PUBLIC, 0, SWITCH_REGONE},
    {(char *) "case", 1, CA_PUBLIC, 0, SWITCH_CASE | SW_MULTIPLE},
    {(char *) "notify", 1, CA_PUBLIC, 0, SWITCH_NOTIFY | SW_MULTIPLE},
    {(char *) "inline", 1, CA_PUBLIC, 0, SWITCH_INLINE | SW_MULTIPLE},
    {(char *) "nobreak", 1, CA_PUBLIC, 0, SWITCH_NOBREAK | SW_MULTIPLE},
    {(char *) "clearreg", 1, CA_PUBLIC, 0, SWITCH_CLEARREG | SW_MULTIPLE},
    {(char *) "localize", 1, CA_PUBLIC, 0, SWITCH_LOCALIZE | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB tel_sw[] =
{
    {(char *) "list", 1, CA_PUBLIC, 0, TEL_LIST},
    {(char *) "quiet", 1, CA_ADMIN, 0, TEL_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB turtle_sw[] =
{
    {(char *) "no_chown", 1, CA_WIZARD | CA_ADMIN, 0, TOAD_NO_CHOWN},
    {(char *) "unique3", 1, CA_WIZARD | CA_ADMIN, 0, TOAD_UNIQUE},
    {NULL, 0, 0, 0, 0}};

NAMETAB toad_sw[] =
{
    {(char *) "no_chown", 1, CA_WIZARD | CA_ADMIN, 0, TOAD_NO_CHOWN | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB toggle_sw[] =
{
    {(char *) "check", 2, CA_PUBLIC, 0, TOGGLE_CHECK},
    {(char *) "clear", 2, CA_PUBLIC, 0, TOGGLE_CLEAR},
    {NULL, 0, 0, 0, 0}};

NAMETAB tor_sw[] =
{
    {(char *) "list", 2, CA_IMMORTAL, 0, TOR_LIST},
    {(char *) "cache", 2, CA_IMMORTAL, 0, TOR_CACHE},
    {NULL, 0, 0, 0, 0}};

NAMETAB trig_sw[] =
{
    {(char *) "command", 1, CA_PUBLIC, 0, TRIG_COMMAND},
    {(char *) "quiet", 1, CA_PUBLIC, 0, TRIG_QUIET | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB thaw_sw[] =
{
    {(char *) "delete", 1, CA_WIZARD, 0, THAW_DEL},
    {NULL, 0, 0, 0, 0}};

NAMETAB wipe_sw[] =
{
    {(char *) "preserve", 1, CA_PUBLIC, 0, WIPE_PRESERVE | SW_MULTIPLE},
    {(char *) "regexp", 1, CA_PUBLIC, 0, WIPE_REGEXP | SW_MULTIPLE},
    {(char *) "owner", 1, CA_PUBLIC, 0, WIPE_OWNER},
    {NULL, 0, 0, 0, 0}};

NAMETAB wait_sw[] =
{
    {(char *) "pid", 1, CA_PUBLIC, 0, WAIT_PID},
    {(char *) "until", 1, CA_PUBLIC, 0, WAIT_UNTIL | SW_MULTIPLE},
    {(char *) "recpid", 1, CA_PUBLIC, 0, WAIT_RECPID | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB wall_sw[] =
{
    {(char *) "emit", 1, CA_WIZARD | CA_ADMIN | CA_BUILDER, 0, SAY_WALLEMIT},
    {(char *) "no_prefix", 3, CA_WIZARD, 0, SAY_NOTAG | SW_MULTIPLE},
    {(char *) "pose", 1, CA_WIZARD | CA_ADMIN | CA_BUILDER, 0, SAY_WALLPOSE},
    {(char *) "wizard", 1, CA_WIZARD | CA_ADMIN | CA_BUILDER, 0, SAY_WIZSHOUT | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB warp_sw[] =
{
    {(char *) "check", 1, CA_WIZARD | CA_ADMIN, 0, TWARP_CLEAN | SW_MULTIPLE},
    {(char *) "dump", 1, CA_WIZARD | CA_ADMIN, 0, TWARP_DUMP | SW_MULTIPLE},
    {(char *) "idle", 1, CA_WIZARD | CA_ADMIN, 0, TWARP_IDLE | SW_MULTIPLE},
    {(char *) "queue", 1, CA_WIZARD | CA_ADMIN, 0, TWARP_QUEUE | SW_MULTIPLE},
    {(char *) "rwho", 1, CA_WIZARD | CA_ADMIN, 0, TWARP_RWHO | SW_MULTIPLE},
    {NULL, 0, 0, 0, 0}};

NAMETAB whisper_sw[] =
{
    {(char *) "port", 1, CA_WIZARD, 0, PEMIT_PORT},
    {NULL, 0, 0, 0, 0}};

NAMETAB log_sw[] =
{
    {(char *) "manual", 1, CA_WIZARD, 0, MLOG_MANUAL},
    {(char *) "stats", 1, CA_WIZARD, 0, MLOG_STATS},
    {(char *) "read", 1, CA_WIZARD, 0, MLOG_READ},
    {(char *) "file", 1, CA_WIZARD, 0, MLOG_FILE},
    {NULL, 0, 0, 0, 0}};

NAMETAB logrotate_sw[] =
{
    {(char *) "status", 1, CA_IMMORTAL, 0, LOGROTATE_STATUS},
    {NULL, 0, 0, 0, 0}};


/* ---------------------------------------------------------------------------
 * Command table: Definitions for builtin commands, used to build the command
 * hash table.
 *
 * Format:  
 *         Name                
 *         Switches        
 *         Permissions Needed
 *         2nd word of permissions
 *         Key (if any)    
 *         Calling Seq (command flags)
 *         Hook Mask
 *         Handler (command itself)
 */

CMDENT command_table[] =
{
    {(char *) "@@", NULL, 0, 0,
     0, CS_NO_ARGS, 0, do_comment},
    {(char *) "@admin", admin_sw, CA_GOD | CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_admin},
    {(char *) "@aflags", aflags_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_CMDARG | CS_INTERP, 0, do_aflags},
    {(char *) "@alias", NULL, CA_NO_GUEST | CA_NO_SLAVE, 0,
     0, CS_TWO_ARG, 0, do_alias},
    {(char *) "@apply_marked", NULL, CA_WIZARD | CA_GBL_INTERP, 0,
     0, CS_ONE_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
     0, do_apply_marked},
    {(char *) "@areg", areg_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_areg},
    {(char *) "@assert", break_sw, CA_PUBLIC, CA_NO_CODE,
     0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, 0, do_assert},
    {(char *) "@attribute", attrib_sw, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_attribute},
    {(char *) "@blacklist", blacklist_sw, CA_LOCATION | CA_IMMORTAL, 0, 0, CS_ONE_ARG | CS_INTERP, 0, do_blacklist},
    {(char *) "@boot", boot_sw, CA_NO_GUEST | CA_NO_SLAVE | CA_NO_WANDER, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_boot},
    {(char *) "@bfree", NULL, CA_GOD | CA_IMMORTAL, 0,
     0, CS_NO_ARGS, 0, do_buff_free},
    {(char *) "@break", break_sw, CA_PUBLIC, CA_NO_CODE,
     0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, 0, do_break},
    {(char *) "@chown", chown_sw,
     CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD | CA_NO_WANDER, 0,
     CHOWN_ONE, CS_TWO_ARG | CS_INTERP, 0, do_chown},
    {(char *) "@chownall", chownall_sw, CA_WIZARD | CA_ADMIN | CA_GBL_BUILD, 0,
     CHOWN_ALL, CS_TWO_ARG | CS_INTERP, 0, do_chownall},
    {(char *) "@clone", clone_sw,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST | CA_NO_WANDER, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_clone},
    {(char *) "@cluster", cluster_sw, CA_WIZARD, CA_CLUSTER, 0, 
     CS_ONE_ARG | CS_CMDARG, 0, do_cluster},
    {(char *) "@conncheck", NULL, CA_GOD | CA_IMMORTAL, 0, 0,
     CS_NO_ARGS, 0, do_conncheck},
    {(char *) "@convert", convert_sw, CA_IMMORTAL, 0, 0,
     CS_INTERP | CS_TWO_ARG, 0, do_convert},
    {(char *) "@cpattr", cpattr_sw, CA_NO_GUEST | CA_NO_SLAVE, CA_NO_CODE,
     0, CS_TWO_ARG | CS_INTERP | CS_ARGV, 0, do_cpattr},
    {(char *) "@create", NULL,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_CONTENTS | CA_NO_GUEST | CA_NO_WANDER, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_create},
    {(char *) "@cut", NULL, CA_WIZARD | CA_LOCATION, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_cut},
    {(char *) "@dbck", dbck_sw, CA_WIZARD | CA_ADMIN, 0,
     0, CS_NO_ARGS, 0, do_dbck},
    {(char *) "@dbclean", dbclean_sw, CA_GOD | CA_IMMORTAL, 0, 0,
     CS_NO_ARGS, 0, do_dbclean},
    {(char *) "@decompile", decomp_sw, 0, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_decomp},
    {(char *) "@depower", depower_sw, CA_PUBLIC, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_depower},
    {(char *) "@destroy", destroy_sw,
     CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD | CA_NO_WANDER, 0,
     DEST_ONE, CS_ONE_ARG | CS_INTERP, 0, do_destroy},
/*{(char *)"@destroyall",       NULL,           CA_WIZARD|CA_GBL_BUILD,
   DEST_ALL,    CS_ONE_ARG,                     0, do_destroy}, */
    {(char *) "@dig", dig_sw,
     CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD | CA_NO_WANDER, 0,
     0, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_dig},
    {(char *) "@disable", NULL, CA_WIZARD, 0,
     GLOB_DISABLE, CS_ONE_ARG, 0, do_global},
    {(char *) "@doing", doing_sw, CA_PLAYER, 0,
     0, CS_ONE_ARG, 0, do_doing},
    {(char *) "@dolist", dolist_sw, CA_GBL_INTERP, CA_NO_CODE,
     0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
     0, do_dolist},
#ifdef ENABLE_DOORS
    {(char *) "@door", door_sw, CA_NO_SLAVE | CA_NO_GUEST, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_door},
#endif
    {(char *) "@drain", drain_sw,
     CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST | CA_NO_WANDER, CA_NO_CODE,
     NFY_DRAIN, CS_TWO_ARG, 0, do_notify},
    {(char *) "@dump", dump_sw, CA_WIZARD | CA_GBL_INTERP, 0,
     0, CS_ONE_ARG, 0, do_dump},
    {(char *) "@dynhelp", dynhelp_sw, CA_WIZARD, 0,
     0, CS_TWO_ARG, 0, do_dynhelp},
    {(char *) "@edit", edit_sw, CA_NO_SLAVE | CA_NO_GUEST, 0,
     0, CS_TWO_ARG | CS_ARGV | CS_STRIP_AROUND,
     0, do_edit},
    {(char *) "@emit", emit_sw,
     CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE, 0,
     SAY_EMIT, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "@enable", NULL, CA_WIZARD, 0,
     GLOB_ENABLE, CS_ONE_ARG, 0, do_global},
    {(char *) "@entrances", NULL, CA_NO_GUEST, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_entrances},
    {(char *) "@eval", NULL, 0, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_eval},
    {(char *) "@extansi", extansi_sw, CA_NO_SLAVE,  0,
     0, CS_TWO_ARG, 0, do_extansi},
    {(char *) "@femit", femit_sw,
     CA_LOCATION | CA_NO_GUEST | CA_NO_SLAVE, CA_NO_CODE,
     PEMIT_FEMIT, CS_TWO_ARG | CS_INTERP, 0, do_pemit},
    {(char *) "@find", NULL, 0, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_find},
    {(char *) "@fixdb", fixdb_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_fixdb},
/*{(char *)"@fnd",              NULL,           0,
   0,           CS_ONE_ARG|CS_UNPARSE,          0, do_fnd}, */
    {(char *) "@flag", flag_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_flagstuff},
    {(char *) "@flagdef", flagdef_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_flagdef},
    {(char *) "@force", NULL,
     CA_NO_SLAVE | CA_GBL_INTERP | CA_NO_GUEST, CA_NO_CODE,
     FRC_COMMAND, CS_TWO_ARG | CS_INTERP | CS_CMDARG, 0, do_force},
    {(char *) "@fpose", fpose_sw, CA_LOCATION | CA_NO_SLAVE, CA_NO_CODE,
     PEMIT_FPOSE, CS_TWO_ARG | CS_INTERP, 0, do_pemit},
    {(char *) "@freeze", NULL, CA_WIZARD, 0,
     0, CS_ONE_ARG, 0, do_freeze},
    {(char *) "@fsay", fsay_sw, CA_LOCATION | CA_NO_SLAVE, CA_NO_CODE,
     PEMIT_FSAY, CS_TWO_ARG | CS_INTERP, 0, do_pemit},
    {(char *) "@function", function_sw, CA_WIZARD, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_function},
    {(char *) "@grep", grep_sw, CA_NO_GUEST | CA_NO_SLAVE, CA_NO_CODE,
     0, CS_TWO_ARG | CS_INTERP | CS_ARGV, 0, do_grep},
    {(char *) "@halt", halt_sw, CA_NO_SLAVE, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_halt},
    {(char *) "@hide", hide_sw, CA_NO_GUEST | CA_NO_SLAVE, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_hide},
    {(char *) "@hook", hook_sw, CA_IMMORTAL, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_hook},
    {(char *) "@icmd", icmd_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP | CS_ARGV, 0, do_icmd},
    {(char *) "@include", include_sw, CA_GBL_INTERP, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV | CS_CMDARG | CS_STRIP_AROUND,
     0, do_include},
    {(char *) "@kick", kick_sw, CA_WIZARD, 0,
     QUEUE_KICK, CS_ONE_ARG | CS_INTERP, 0, do_queue},
    {(char *) "@last", NULL, CA_NO_GUEST, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_last},
    {(char *) "@lfunction", lfunction_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_NO_WANDER, CA_NO_CODE,
     FN_LOCAL, CS_TWO_ARG | CS_INTERP, 0, do_function},
    {(char *) "@limit", limit_sw, CA_WIZARD,  0,
     0, CS_TWO_ARG | CS_INTERP | CS_ARGV, 0, do_limit},
    {(char *) "@link", NULL,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_link},
    {(char *) "@list", NULL, 0, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_list},
    {(char *) "@list_file", NULL, CA_WIZARD | CA_ADMIN | CA_BUILDER, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_list_file},
    {(char *) "@listmotd", listmotd_sw, 0, 0,
     MOTD_LIST, CS_ONE_ARG, 0, do_motd},
/* Removed CA_GBL_BUILD from @lock : ASH 08/23/98 */
    {(char *) "@lock", lock_sw, CA_NO_SLAVE, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_lock},
    {(char *) "@mark", mark_sw, CA_WIZARD, 0,
     SRCH_MARK, CS_ONE_ARG | CS_NOINTERP, 0, do_search},
    {(char *) "@mark_all", markall_sw, CA_WIZARD, 0,
     MARK_SET, CS_NO_ARGS, 0, do_markall},
    {(char *) "@money", NULL, CA_NO_GUEST, 0,
     0, CS_ONE_ARG, 0, do_money},
    {(char *) "@motd", motd_sw, CA_WIZARD, 0,
     0, CS_ONE_ARG, 0, do_motd},
    {(char *) "@mudwho", NULL, CA_NO_SLAVE, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_mudwho},
/* Removed CA_GBL_BUILD from @mvattr : ASH 08/23/98 */
    {(char *) "@mvattr", NULL,
     CA_NO_SLAVE | CA_NO_GUEST, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV, 0, do_mvattr},
/* Removed CA_GBL_BUILD from @name : ASH 08/23/98 */
    {(char *) "@name", NULL,
     CA_NO_SLAVE | CA_NO_GUEST, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_name},
    {(char *) "@newpassword", newpassword_sw, CA_WIZARD | CA_ADMIN | CA_IMMORTAL, 0,
     PASS_ANY, CS_TWO_ARG, 0, do_newpassword},
    {(char *) "@notify", notify_sw,
     CA_GBL_INTERP | CA_NO_SLAVE | CA_NO_GUEST, CA_NO_CODE,
     0, CS_TWO_ARG, 0, do_notify},
    {(char *) "@nuke", nuke_sw, CA_ADMIN | CA_WIZARD | CA_IMMORTAL, 0,
     DEST_ONE, CS_ONE_ARG | CS_INTERP, 0, do_nuke},
    {(char *) "@oemit", oemit_sw,
     CA_NO_GUEST | CA_NO_SLAVE, CA_NO_CODE,
     PEMIT_OEMIT, CS_TWO_ARG | CS_INTERP, 0, do_pemit},
    {(char *) "@open", open_sw,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST | CA_NO_WANDER, 0,
     0, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_open},
    {(char *) "@parent", NULL,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST | CA_NO_WANDER, CA_NO_CODE,
     0, CS_TWO_ARG, 0, do_parent},
    {(char *) "@password", NULL, CA_NO_GUEST, 0,
     PASS_MINE, CS_TWO_ARG, 0, do_password},
    {(char *) "@pcreate", pcreate_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_NO_WANDER | CA_GBL_BUILD, 0,
     PCRE_PLAYER, CS_TWO_ARG, 0, do_pcreate},
    {(char *) "@pemit", pemit_sw, CA_NO_GUEST | CA_NO_SLAVE, CA_NO_CODE,
     PEMIT_PEMIT, CS_TWO_ARG | CS_NOINTERP | CS_CMDARG, 0, do_pemit},
    {(char *) "@pipe", pipe_sw, CA_NO_GUEST | CA_NO_SLAVE | CA_NO_WANDER, CA_NO_CODE, 
     0, CS_ONE_ARG | CS_INTERP, 0, do_pipe},
    {(char *) "@poor", NULL, CA_WIZARD | CA_ADMIN, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_poor},
    {(char *) "@power", power_sw, CA_PUBLIC, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_power},
    {(char *) "@protect", protect_sw, CA_NO_GUEST | CA_NO_SLAVE | CA_NO_WANDER | CA_PLAYER, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_protect},
    {(char *) "@program", NULL, CA_PUBLIC, CA_NO_CODE,
     0, CS_TWO_ARG | CS_INTERP, 0, do_program},
    {(char *) "@progreset", NULL, CA_LOCATION, 0, 0, CS_ONE_ARG | CS_INTERP, CA_NO_CODE, do_progreset},
    {(char *) "@ps", ps_sw, 0, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_ps},
    {(char *) "@purge", purge_sw, CA_IMMORTAL, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_purge},
    {(char *) "@quitprogram", quitprogram_sw, CA_PUBLIC,  0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_quitprogram},
    {(char *) "@quota", quota_sw, 0, 0,
     0, CS_TWO_ARG | CS_ARGV, 0, do_quota},
    {(char *) "@readcache", NULL, CA_WIZARD, 0,
     0, CS_NO_ARGS, 0, do_readcache},
    {(char *) "@reboot", reboot_sw, CA_PUBLIC, 0,
     0, CS_NO_ARGS, 0, do_reboot},
    {(char *) "@reclist", reclist_sw, CA_IMMORTAL, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_reclist},
    {(char *) "@recover", NULL, CA_IMMORTAL, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_recover},
    {(char *) "@register", register_sw, CA_PUBLIC, 0,
     0, CS_TWO_ARG, 0, do_register},
    {(char *) "@remote", NULL,
     CA_GBL_INTERP | CA_GOD | CA_IMMORTAL | CA_WIZARD, 0,
     0, CS_TWO_ARG | CS_INTERP | CS_CMDARG, 0, do_remote},
    {(char *) "@robot", NULL,
     CA_NO_SLAVE | CA_GBL_BUILD | CA_NO_GUEST | CA_PLAYER | CA_NO_WANDER, CA_NO_CODE,
     PCRE_ROBOT, CS_TWO_ARG, 0, do_pcreate},
    {(char *) "@rwho", rwho_sw, CA_GOD, 0,
     0, CS_NO_ARGS, 0, do_rwho},
#ifdef REALITY_LEVELS
    {(char *) "@rxlevel", NULL, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0,
    0, CS_TWO_ARG | CS_INTERP, 0, do_rxlevel},
#endif /* REALITY_LEVELS */
    {(char *) "@search", search_sw, 0, 0,
     SRCH_SEARCH, CS_ONE_ARG | CS_NOINTERP, 0, do_search},
    {(char *) "@selfboot", selfboot_sw, CA_NO_SLAVE | CA_PLAYER | CA_NO_GUEST, 0,
     0, CS_ONE_ARG, 0, do_selfboot},
/* Removed CA_GBL_BUILD from @set : ASH 08/23/98 */
#ifndef NO_ENH
    {(char *) "@set", set_sw,
     CA_NO_SLAVE | CA_NO_GUEST, 0,
     0, CS_TWO_ARG, 0, do_set},
#else
    {(char *) "@set", NULL, CA_NO_SLAVE | CA_NO_GUEST, 0, 0, CS_TWO_ARG, 0, do_set},
#endif
    {(char *) "@shutdown", NULL, CA_PUBLIC, 0, 0, CS_ONE_ARG, 0, do_shutdown},
    {(char *) "@site", site_sw, CA_IMMORTAL, 0, 0, CS_TWO_ARG, 0, do_site},
    {(char *) "@skip", skip_sw, CA_NO_SLAVE | CA_NO_GUEST, CA_NO_CODE, 0, CS_TWO_ARG | CS_ARGV | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND, 0, do_skip},
    {(char *) "@snapshot", snapshot_sw, CA_IMMORTAL, 0, 0, CS_TWO_ARG | CS_INTERP, 0, do_snapshot},
#ifndef NO_SNOOP
    {(char *) "@snoop", snoop_sw, CA_PUBLIC | CA_IGNORE_ROYAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_snoop},
#endif
    {(char *) "@stats", stats_sw, 0, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_stats},
    {(char *) "@toggledef", flagdef_sw, CA_IMMORTAL, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_toggledef},
    {(char *) "@tor", tor_sw, CA_IMMORTAL, 0,
     0, CS_NO_ARGS, 0, do_tor},
    {(char *) "@sudo", sudo_sw, CA_NO_SLAVE | CA_NO_GUEST, CA_NO_CODE, 0, CS_NOINTERP | CS_TWO_ARG | CS_CMDARG | CS_STRIP_AROUND, 0, do_sudo},
    {(char *) "@sweep", sweep_sw, 0, 0,
     0, CS_ONE_ARG, 0, do_sweep},
    {(char *) "@switch", switch_sw, CA_GBL_INTERP, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
     0, do_switch},
    {(char *) "@teleport", tel_sw, CA_NO_GUEST, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_teleport},
    {(char *) "@thaw", thaw_sw, CA_WIZARD, 0,
     0, CS_ONE_ARG, 0, do_thaw},
    {(char *) "@timewarp", warp_sw, CA_WIZARD | CA_ADMIN, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_timewarp},
    {(char *) "@toad", toad_sw, CA_WIZARD | CA_ADMIN, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_toad},
    {(char *) "@toggle", toggle_sw,
     CA_NO_SLAVE | CA_NO_GUEST, 0,
     0, CS_TWO_ARG, 0, do_toggle},
#ifndef NO_ENH
    {(char *) "@trigger", trig_sw, CA_GBL_INTERP | CA_NO_SLAVE, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV, 0, do_trigger},
#else
    {(char *) "@trigger", NULL, CA_GBL_INTERP | CA_NO_SLAVE, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV, 0, do_trigger},
#endif
    {(char *) "@turtle", turtle_sw, CA_WIZARD | CA_ADMIN, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_turtle},
#ifdef REALITY_LEVELS
    {(char *) "@txlevel", NULL, CA_GOD | CA_IMMORTAL | CA_WIZARD, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_txlevel},
#endif /* REALITY_LEVELS */ 
    {(char *) "@unlink", NULL, CA_NO_SLAVE | CA_GBL_BUILD, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_unlink},
    {(char *) "@unlock", lock_sw, CA_NO_SLAVE | CA_GBL_BUILD, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_unlock},
    {(char *) "@verb", NULL, CA_GBL_INTERP | CA_NO_SLAVE, CA_NO_CODE,
     0, CS_TWO_ARG | CS_ARGV | CS_INTERP | CS_STRIP_AROUND,
     0, do_verb},
    {(char *) "@wait", wait_sw, CA_GBL_INTERP, CA_NO_CODE,
     0, CS_TWO_ARG | CS_CMDARG | CS_NOINTERP | CS_STRIP_AROUND,
     0, do_wait},
    {(char *) "@wall", wall_sw, CA_NO_GUEST, 0,
     SAY_SHOUT, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "@whereall", NULL, CA_PUBLIC, CA_NO_CODE,
     0, CS_NO_ARGS | CS_INTERP, 0, do_whereall},
    {(char *) "@whereis", NULL, CA_NO_GUEST, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_whereis},
/* Removed CA_GBL_BUILD from @wipe: ASH 08/23/98 */
    {(char *) "@wipe", wipe_sw,
     CA_NO_SLAVE | CA_NO_GUEST | CA_NO_WANDER, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP, 0, do_wipe},
    {(char *) "@zone", zone_sw,
     CA_NO_SLAVE | CA_NO_GUEST | CA_GBL_BUILD | CA_NO_WANDER, CA_NO_CODE,
     0, CS_TWO_ARG | CS_INTERP, 0, do_zone},
    {(char *) "drop", drop_sw,
     CA_NO_SLAVE | CA_CONTENTS | CA_LOCATION | CA_NO_GUEST, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_drop},
    {(char *) "enter", enter_sw, CA_LOCATION, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_enter},
    {(char *) "examine", examine_sw, 0, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_examine},
    {(char *) "get", get_sw, CA_LOCATION | CA_NO_GUEST, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_get},
/*  {(char *) "give", give_sw, CA_LOCATION | CA_NO_GUEST, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_give}, */
    {(char *) "give", give_sw,  CA_NO_GUEST, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_give},
    {(char *) "grab", NULL, CA_NO_GUEST, 0,
     TEL_GRAB, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_teleport},
    {(char *) "goto", goto_sw, CA_LOCATION, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_move},
    {(char *) "help", genhelp_sw, 0, 0,
     HELP_HELP, CS_ONE_ARG, 0, do_help},
#ifdef PLUSHELP
    {(char *) "+help", genhelp_sw, 0, 0,
     HELP_PLUSHELP, CS_ONE_ARG, 0, do_help},
#endif
    {(char *) "idle", NULL, CA_NO_GUEST | CA_NO_SLAVE | CA_PLAYER, 0, 0, CS_ONE_ARG|CS_CMDARG, 0, do_idle},
    {(char *) "inventory", NULL, 0, 0,
     LOOK_INVENTORY, CS_NO_ARGS, 0, do_inventory},
    {(char *) "join", NULL, CA_NO_GUEST, 0,
     TEL_JOIN, CS_TWO_ARG | CS_ARGV | CS_INTERP, 0, do_teleport},
    {(char *) "kill", NULL, CA_NO_GUEST | CA_NO_SLAVE, 0,
     KILL_KILL, CS_TWO_ARG | CS_INTERP, 0, do_kill},
    {(char *) "lpage", rpage_sw, CA_NO_SLAVE, 0,
     PAGE_LAST, CS_ONE_ARG | CS_INTERP, 0, do_page_one},
    {(char *) "leave", leave_sw, CA_LOCATION, 0,
     0, CS_NO_ARGS | CS_INTERP, 0, do_leave},
    /* Added LISTEN sense - 090898 Ash - listen_sw added for later need */
    {(char *) "listen", NULL, CA_LOCATION, 0,
     LOOK_LOOK, CS_ONE_ARG | CS_INTERP, 0, do_listen},
    {(char *) "look", look_sw, CA_LOCATION, 0,
     LOOK_LOOK, CS_ONE_ARG | CS_INTERP, 0, do_look},
    {(char *) "mrpage", rpage_sw, CA_NO_SLAVE, 0,
     PAGE_RETMULTI, CS_ONE_ARG | CS_INTERP, 0, do_page_one},
    {(char *) "news", news_sw, CA_NO_SLAVE, 0,
     0, CS_TWO_ARG, 0, do_news},
    {(char *) "newsdb", newsdb_sw, CA_NO_SLAVE | CA_NO_GUEST, 0,
     0, CS_TWO_ARG, 0, do_newsdb},
    {(char *) "page", page_sw, CA_NO_SLAVE, 0,
     0, CS_TWO_ARG | CS_INTERP, 0, do_page},
    {(char *) "pose", pose_sw, CA_LOCATION | CA_NO_SLAVE, 0,
     SAY_POSE, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "rpage", rpage_sw, CA_NO_SLAVE, 0,
     PAGE_RET, CS_ONE_ARG | CS_INTERP, 0, do_page_one},
    {(char *) "say", say_sw, CA_LOCATION | CA_NO_SLAVE, 0,
     SAY_SAY, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "score", NULL, 0, 0,
     LOOK_SCORE, CS_NO_ARGS, 0, do_score},
    {(char *) "slay", NULL, CA_WIZARD | CA_ADMIN | CA_BUILDER | CA_NO_SLAVE, 0,
     KILL_SLAY, CS_TWO_ARG | CS_INTERP, 0, do_kill},
    /* Added SMELL sense - 090898 Ash smell_sw created for future need */
    {(char *) "smell", NULL, CA_LOCATION, 0,
     LOOK_LOOK, CS_ONE_ARG | CS_INTERP, 0, do_smell},
    /* Added TASTE sense - 090898 Ash taste_sw created for future need */
    {(char *) "taste", NULL, CA_LOCATION, 0,
     LOOK_LOOK, CS_ONE_ARG | CS_INTERP, 0, do_taste},
    /* Added THINK for compatibility -- 120198 Ash */
    {(char *) "think", think_sw, CA_NO_SLAVE, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_think},
    /* Added TOUCH sense - 090898 Ash - touch_sw created for future need */
    {(char *) "touch", NULL, CA_LOCATION, 0,
     LOOK_LOOK, CS_ONE_ARG | CS_INTERP, 0, do_touch},
    {(char *) "train", NULL, CA_LOCATION, 0, 0, CS_ONE_ARG | CS_CMDARG, 0, do_train},
    {(char *) "use", NULL, CA_NO_SLAVE | CA_GBL_INTERP, 0,
     0, CS_ONE_ARG | CS_INTERP, 0, do_use},
    {(char *) "version", NULL, 0, 0,
     0, CS_NO_ARGS, 0, do_version},
    {(char *) "wielded", NULL, 0, 0,
     LOOK_INVENTORY, CS_NO_ARGS, 0, do_wielded},
    {(char *) "whisper", whisper_sw, CA_LOCATION | CA_NO_SLAVE, 0,
     PEMIT_WHISPER, CS_TWO_ARG | CS_INTERP, 0, do_pemit},
    {(char *) "wizhelp", genhelp_sw, CA_WIZARD | CA_ADMIN | CA_BUILDER | CA_GUILDMASTER, 0,
     HELP_WIZHELP, CS_ONE_ARG, 0, do_help},
    {(char *) "worn", NULL, 0, 0,
     LOOK_INVENTORY, CS_NO_ARGS, 0, do_worn},
    {(char *) "E", NULL,
     CA_NO_GUEST | CA_LOCATION | CF_DARK | CA_NO_SLAVE, 0,
     SAY_PREFIX, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "F", NULL,
     CA_NO_SLAVE | CA_GBL_INTERP | CF_DARK, CA_NO_CODE,
     0, CS_ONE_ARG | CS_INTERP | CS_CMDARG, 0, do_force_prefixed},
    {(char *) "P", NULL,
     CA_LOCATION | CF_DARK | CA_NO_SLAVE, 0,
     SAY_PREFIX, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "S", NULL,
     CA_LOCATION | CF_DARK | CA_NO_SLAVE, 0,
     SAY_PREFIX, CS_ONE_ARG | CS_INTERP, 0, do_say},
    {(char *) "V", set_sw,
     CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0,
     0, CS_TWO_ARG , 0, do_setvattr},
    {(char *) "C", set_sw,
     CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0,
     0, CS_TWO_ARG , 0, do_setvattr_cluster},
    {(char *) "M", NULL, CA_NO_GUEST | CA_NO_SLAVE | CF_DARK, 0,
     M_WRITE | M_ALL, CS_ONE_ARG | CS_INTERP, 0, do_single_mail},
    {(char *) "N", NULL, CA_LOCATION | CF_DARK, 0,
     0, CS_ONE_ARG | CS_CMDARG, 0, do_noparsecmd},
#ifndef SOFTCOM
    {(char *) "+channel", NULL, CA_NO_GUEST, 0,
     0, CS_ONE_ARG, 0, do_channel},
    {(char *) "+com", NULL, CA_NO_GUEST, 0,
     0, CS_TWO_ARG, 0, do_com},
#endif
    /* MMMail- stuff for hard copy mailer */
    {(char *) "mail", mail_sw, CA_NO_GUEST | CA_NO_SLAVE, 0, 0, CS_TWO_ARG | CS_INTERP, 0, do_mail},
    {(char *) "wmail", wmail_sw, CA_ADMIN | CA_NO_SLAVE, 0, 0, CS_TWO_ARG | CS_INTERP, 0, do_wmail},
    {(char *) "folder", mail2_sw, CA_NO_GUEST | CA_NO_SLAVE, 0, 0, CS_TWO_ARG | CS_INTERP, 0, do_mail2},
    {(char *) "+players", NULL, CA_WIZARD | CA_ADMIN, 0, 0, CS_NO_ARGS | CS_INTERP, 0, do_search_for_players},
    {(char *) "+uptime", NULL, 0, 0, 0, CS_NO_ARGS | CS_INTERP, 0, do_uptime},
    {(char *) "@log", log_sw, CA_WIZARD, 0, 0, CS_TWO_ARG | CS_INTERP, 0, do_log},
    {(char *) "@logrotate", logrotate_sw, CA_LOCATION | CA_IMMORTAL, 0, 0, CS_NO_ARGS | CS_INTERP, 0, do_logrotate},
    {(char *) NULL, NULL, 0, 0, 0, 0, 0, NULL}
};

CMDENT *prefix_cmds[256];

CMDENT *goto_cmdp;

dbref TopLocation(dbref num) {
   int i_recur;
   dbref cur_loc;

   cur_loc = NOTHING;

   if (!Good_obj(num))
      return NOTHING;

   cur_loc = Location(num);

   if ( !Good_obj(cur_loc) )
      return NOTHING;

   if ( isRoom(cur_loc) )
      return cur_loc;

   for (i_recur = mudconf.ntfy_nest_lim; i_recur > 0; i_recur--) {
      cur_loc = Location(cur_loc);
      if ( !Good_obj(cur_loc) )
         return NOTHING;
      if (isRoom(cur_loc))
         break;
   }
   if (!Good_obj(cur_loc) || !isRoom(cur_loc))
      return NOTHING;
   
   return cur_loc;
}

char *
time_format_2(time_t dt2, char *buf2)
{
    struct tm *delta;
    time_t dt;
    char buf[64];

    dt = dt2;
    if (dt < 0)
        dt = 0;

    delta = gmtime(&dt);
    memset(buf2, '\0', 64);
    if ((int)dt >= 31556926 ) {
        sprintf(buf, "%dy", (int)dt / 31556926);
        strcat(buf2, buf);
    }
    if ((int)dt >= 2629743 ) {
        sprintf(buf, "%dM", (int)dt / 2629743);
        strcat(buf2, buf);
    }
    if ((int)dt >= 604800) {
        sprintf(buf, "%dw", (int)dt / 604800);
        strcat(buf2, buf);
    }
    if (delta->tm_yday > 0) {
        sprintf(buf, "%dd", delta->tm_yday);
        strcat(buf2, buf);
    }
    if (delta->tm_hour > 0) {
        sprintf(buf, "%dh", delta->tm_hour);
        strcat(buf2, buf);
    }
    if (delta->tm_min > 0) {
        sprintf(buf, "%dm", delta->tm_min);
        strcat(buf2, buf);
    }
    sprintf(buf, "%ds", delta->tm_sec);
    strcat(buf2, buf);
    return(buf2);
}

int chkforcommand(char *command) {
   int retval = 0;
   if (lookup_orig_command(command)) {
     return 1;
   } else {
     return 0;
   }
   return(retval);
}

static char *
newtrim_space_sep(char *str, char sep)
{
    char *p;

    if (sep != ' ')
        return str;
    while (*str && (*str == ' '))
        str++;
    for (p = str; *p; p++);
    for (p--; *p == ' ' && p > str; p--);
    p++;
    *p = '\0';
    return str;
}

static char *
newsplit_token(char **sp, char sep)
{
    char *str, *save;

    save = str = *sp;
    if (!str) {
        *sp = NULL;
        return NULL;
    }
    while (*str && (*str != sep))
        str++;
    if (*str) {
        *str++ = '\0';
        if (sep == ' ') {
            while (*str == sep)
                str++;
        }
    } else {
        str = NULL;
    }
    *sp = str;
    return save;
}

static int
newlist2arr(char *arr[], int maxlen, char *list, char sep)
{
    char *p;
    int i;

    list = newtrim_space_sep(list, sep);
    p = newsplit_token(&list, sep);
    for (i = 0; p && i < maxlen; i++, p = newsplit_token(&list, sep)) {
        arr[i] = p;
    }
    return i;
}

void 
NDECL(init_cmdtab)
{
    CMDENT *cp;
    ATTR *ap;
    char *p, *q;
    int anum, sbuf_cntr;
    char *cbuff;

    DPUSH; /* #25 */

    hashinit(&mudstate.command_htab, 511);
    hashinit(&mudstate.cmd_alias_htab, 511);

    /* Load attribute-setting commands */

    cbuff = alloc_sbuf("init_cmdtab");
    for (ap = attr; ap->name; ap++) {
    if ((ap->flags & AF_NOCMD) == 0) {
        p = cbuff;
        *p++ = '@';
            sbuf_cntr = 0;
        for (q = (char *) ap->name; *q && (sbuf_cntr < (SBUF_SIZE-1)); p++, q++) {
        *p = ToLower((int)*q);
                sbuf_cntr++;
            }
        *p = '\0';
        cp = (CMDENT *) malloc(sizeof(CMDENT));
        cp->cmdname = (char *) strsave(cbuff);
        cp->perms = CA_NO_GUEST | CA_NO_SLAVE;
            cp->perms2 = 0;
        cp->switches = set_sw;
        if (ap->flags & AF_GUILDMASTER) {
        cp->perms |= CA_GUILDMASTER;
        }
        if (ap->flags & AF_BUILDER) {
        cp->perms |= CA_BUILDER;
        }
        if (ap->flags & AF_ADMIN) {
        cp->perms |= CA_ADMIN;
        }
        if (ap->flags & (AF_WIZARD | AF_MDARK)) {
        cp->perms |= CA_WIZARD;
        }
        if (ap->flags & (AF_IMMORTAL)) {
        cp->perms |= CA_IMMORTAL;
        }
        cp->extra = ap->number;
        cp->callseq = CS_TWO_ARG | CS_SEP;
        cp->handler = do_setattr;
            cp->hookmask = 0;
        cp->cmdtype = CMD_ATTR_e;
        hashadd(cp->cmdname, (int *) cp, &mudstate.command_htab);
    }
    }
    free_sbuf(cbuff);

    /* Load the builtin commands */

    for (cp = command_table; cp->cmdname; cp++) {
      cp->cmdtype = CMD_BUILTIN_e;
      hashadd(cp->cmdname, (int *) cp, &mudstate.command_htab);
    }

    /* Load the command prefix table.  Note - these commands can never
     * be typed in by a user because commands are lowercased before
     * the hash table is checked. The names are abbreviated to minimise
     * name checking time. */

    for (anum = 0; anum < 256; anum++)
    prefix_cmds[anum] = NULL;
    prefix_cmds['"'] = (CMDENT *) hashfind((char *) "S",
                       &mudstate.command_htab);
    prefix_cmds[':'] = (CMDENT *) hashfind((char *) "P",
                       &mudstate.command_htab);
    prefix_cmds[';'] = (CMDENT *) hashfind((char *) "P",
                       &mudstate.command_htab);
    prefix_cmds['\\'] = (CMDENT *) hashfind((char *) "E",
                        &mudstate.command_htab);
    prefix_cmds['#'] = (CMDENT *) hashfind((char *) "F",
                       &mudstate.command_htab);
    prefix_cmds['&'] = (CMDENT *) hashfind((char *) "V",
                       &mudstate.command_htab);
    prefix_cmds['>'] = (CMDENT *) hashfind((char *) "C",
                       &mudstate.command_htab);
    prefix_cmds['-'] = (CMDENT *) hashfind((char *) "M",
                       &mudstate.command_htab);
    prefix_cmds[']'] = (CMDENT *) hashfind((char *) "N",
                                           &mudstate.command_htab);
    goto_cmdp = (CMDENT *) hashfind("goto", &mudstate.command_htab);
    DPOP; /* #25 */
}

/* ---------------------------------------------------------------------------
 * check_access: Check if player has access to function.  
 */

int igcheck(dbref player, int mask)
{
  dbref loc, tloc;
  ZLISTNODE *z_ptr;

  if (mask & CA_IGNORE)
    return 1;
  else if ((mask & CA_IGNORE_IM) && ((mask & CA_IGNORE_ZONE) == 0) && !God(player))
    return 1;
  else if ((mask & CA_IGNORE_ROYAL) && ((mask & CA_IGNORE_ZONE) == 0) && !Immortal(player))
    return 1;
  else if ((mask & CA_IGNORE_COUNC) && ((mask & CA_IGNORE_ZONE) == 0) && !Wizard(player))
    return 1;
  else if ((mask & CA_IGNORE_ARCH) && ((mask & CA_IGNORE_ZONE) == 0) && !Admin(player))
    return 1;
  else if ((mask & CA_IGNORE_GM) && ((mask & CA_IGNORE_ZONE) == 0) && !Builder(player))
    return 1;
  else if ((mask & CA_IGNORE_MORTAL) && ((mask & CA_IGNORE_ZONE) == 0) && !Privilaged(player))
    return 1;
  else if (mask & CA_IGNORE_ZONE) {
        if ((mask & CA_IGNORE_IM) && God(player))
           return 0;
        else if ((mask & CA_IGNORE_ROYAL) && Immortal(player))
           return 0;
        else if ((mask & CA_IGNORE_COUNC) && Wizard(player))
           return 0;
        else if ((mask & CA_IGNORE_ARCH) && Admin(player))
           return 0;
        else if ((mask & CA_IGNORE_GM) && Builder(player))
           return 0;
        else if ((mask & CA_IGNORE_MORTAL) && Privilaged(player))
           return 0;
    loc = Location(player);
        tloc = TopLocation(player);
        if ( Good_obj(tloc) && !(Good_obj(loc) && IgnoreZone(loc)) )
           loc = tloc;
        if ( Good_obj(loc) ) {
           if ( IgnoreZone(loc) ) {
              return 1;
           }
           if ( ZoneMaster(loc) ) {
              return 0;
           }
           for ( z_ptr = db[loc].zonelist; z_ptr; z_ptr = z_ptr->next ) {
              if ( Good_obj(z_ptr->object) && !Recover(z_ptr->object) &&
                   (isRoom(z_ptr->object) || isThing(z_ptr->object)) &&
                   IgnoreZone(z_ptr->object) ) {
                 return 1;
              }
           }
        }
  }
  return 0;
}

int 
zigcheck(dbref player, int mask)
{
   dbref loc, tloc;
   ZLISTNODE *z_ptr;

   if (mask & CA_DISABLE_ZONE) {
      loc = Location(player);
      tloc = TopLocation(player);
      if ( Good_obj(tloc) && !(Good_obj(loc) && IgnoreZone(loc)) )
         loc = tloc;
      if ( Good_obj(loc) ) {
         if ( IgnoreZone(loc) ) {
            return 1;
         }
         if ( ZoneMaster(loc) ) {
            return 0;
         }
         for ( z_ptr = db[loc].zonelist; z_ptr; z_ptr = z_ptr->next ) {
            if ( Good_obj(z_ptr->object) && !Recover(z_ptr->object) &&
                (isRoom(z_ptr->object) || isThing(z_ptr->object)) &&
                 IgnoreZone(z_ptr->object) ) {
                return 1;
            }
         }
      }
   }
   return 0;
}

int 
check_access(dbref player, int mask, int mask2, int ccheck)
{
    int succ, fail;

    DPUSH; /* #26 */

    mudstate.func_ignore = 0;
    if ((mask & CA_DISABLED) || ((mask & CA_IGNORE_MASK) && igcheck(player, mask)) || ccheck) {
    if (mask & CA_IGNORE_MASK)
      mudstate.func_ignore = 1;
        DPOP; /* #26 */
    return 0;
    }
    if (God(player) || mudstate.initializing) {
        DPOP; /* #26 */
    return 1;
    }

    succ = fail = 0;
    if (mask & CA_GOD)
    fail++;
    if (mask & CA_WIZARD) {
    if (Wizard(player))
        succ++;
    else
        fail++;
    }
    if (mask & CA_ADMIN) {
    if (Admin(player))
        succ++;
    else
        fail++;
    }
    if (mask2 & CA_CLUSTER) {
        if ( Cluster(player) )
            succ++;
    else
        fail++;
    }
    if ((succ == 0) && (mask & CA_IMMORTAL)) {
    if (Immortal(player))
        succ++;
    else
        fail++;
    }
    if ((succ == 0) && (mask & CA_BUILDER)) {
    if (Builder(player))
        succ++;
    else
        fail++;
    }
    if ((succ == 0) && (mask & CA_GUILDMASTER)) {
    if (Guildmaster(player))
        succ++;
    else
        fail++;
    }
    if ((succ == 0) && (mask & CA_ROBOT)) {
    if (Robot(player))
        succ++;
    else
        fail++;
    }
    if (succ > 0)
    fail = 0;
    if (fail > 0) {
        DPOP; /* #26 */
    return 0;
    } 

    /* Check for forbidden flags. */

    if (!Wizard(player) &&
    (((mask & CA_NO_HAVEN) && Player_haven(player)) ||
     ((mask & CA_NO_ROBOT) && Robot(player)) ||
     ((mask & CA_NO_SLAVE) && Slave(player)) ||
     ((mask & CA_NO_SUSPECT) && Suspect(player)) ||
     ((mask & CA_NO_WANDER) && Wanderer(player)) ||
     ((mask & CA_NO_GUEST) && Guest(player)) ||
     ((mask2 & CA_NO_CODE) && NoCode(player) && !mudstate.nocodeoverride))) {
        DPOP; /* #26 */
    return 0;
    }
    DPOP; /* #26 */
    return 1;
}
/*****************************************************************************
 * Process the various hook calls - Idea taken from TinyMUSH3, code original *
 * Hooks processed:  before, after, ignore, permit
 *****************************************************************************/
int
process_hook(dbref player, dbref thing, char *s_uselock, ATTR *hk_ap2, int save_flg)
{
   dbref thing2;
   int attrib2, aflags_set, i_cpuslam, x, retval;
   char *atext, *cputext, *cpulbuf, *savereg[MAX_GLOBAL_REGS], *pt, *result, *cpuslam;

   i_cpuslam = 0;
   retval = 1;
   if ( hk_ap2 ) {
      atext = atr_get(thing, hk_ap2->number, &thing2, &attrib2);
      aflags_set = hk_ap2->number;
      if ( atext && !(attrib2 & AF_NOPROG) ) {
         if ( save_flg ) {
            for (x = 0; x < MAX_GLOBAL_REGS; x++) {
               savereg[x] = alloc_lbuf("ulocal_reg");
               pt = savereg[x];
               safe_str(mudstate.global_regs[x],savereg[x],&pt);
            }
         }
         cputext = alloc_lbuf("store_text_process_hook");
         strcpy(cputext, atext);
#ifndef OVERRIDE_HOOKCHK
         if ( (strstr(s_uselock, (char *)"_@password") != NULL) ||
              (strstr(s_uselock, (char *)"_@newpassword") != NULL) ) {
            mudstate.password_nochk = 1;
         }
#endif
         result = exec(thing, player, player, EV_FCHECK | EV_EVAL, atext,
                       (char **)NULL, 0, (char **)NULL, 0);
         mudstate.password_nochk = 0;
         if ( save_flg ) {
            for (x = 0; x < MAX_GLOBAL_REGS; x++) {
               pt = mudstate.global_regs[x];
               safe_str(savereg[x],mudstate.global_regs[x],&pt);
               free_lbuf(savereg[x]);
            }
         }
         if ( !i_cpuslam && mudstate.chkcpu_toggle ) {
            i_cpuslam = 1;
            cpuslam = alloc_lbuf("uselock_cpuslam");
            memset(cpuslam, 0, LBUF_SIZE);
            sprintf(cpuslam, "(HOOK:%.32s):%.3900s", s_uselock, cputext);
            broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (HOOK)",
                              (char *)cpuslam, NULL, thing, 0, 0, NULL);
            cpulbuf = alloc_lbuf("log_uselock_cpuslam");
            STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
            log_name_and_loc(player);
            sprintf(cpulbuf, " CPU/ULCK overload: (#%d) %.3940s", thing, cpuslam);
                    log_text(cpulbuf);
            free_lbuf(cpulbuf);
            ENDLOG
            free_lbuf(cpuslam);
            atr_set_flags(thing, aflags_set, (attrib2 | AF_NOPROG) ); 
         } else {
            if ( *result )
               retval = (atoi(result) > 0 ? 1 : 0);
         }
         free_lbuf(cputext);
         free_lbuf(result);
      }
      if ( atext )
         free_lbuf(atext);
   }
   return retval;
}

/*****************************************************************************
 * process_error_control - error control handler
 ****************************************************************************/

void 
process_error_control(dbref player, CMDENT *cmdp)
{
    char *s_uselock, *dx_tmp;
    int chk_stop, chk_tog;
    ATTR *hk_ap2;

    if ( (cmdp && (cmdp->hookmask & HOOK_FAIL)) && 
         Good_obj(mudconf.hook_obj) &&
         !Recover(mudconf.hook_obj) && 
         !Going(mudconf.hook_obj) ) {

       s_uselock = alloc_sbuf("command_hook_error");
       memset(s_uselock, 0, SBUF_SIZE);

       if ( strcmp(cmdp->cmdname, "S") == 0 )
          strcpy(s_uselock, "AF_say");
       else if ( strcmp(cmdp->cmdname, "P") == 0 )
          strcpy(s_uselock, "AF_pose");
       else if ( strcmp(cmdp->cmdname, "E") == 0 )
          strcpy(s_uselock, "AF_@emit");
       else if ( strcmp(cmdp->cmdname, "F") == 0 )
          strcpy(s_uselock, "AF_@force");
       else if ( strcmp(cmdp->cmdname, "V") == 0 )
          strcpy(s_uselock, "AF_@set");
       else if ( strcmp(cmdp->cmdname, "M") == 0 )
          strcpy(s_uselock, "AF_mail");
       else if ( strcmp(cmdp->cmdname, "N") == 0 )
          strcpy(s_uselock, "AF_N");
       else
          sprintf(s_uselock, "AF_%.28s", cmdp->cmdname);
       dx_tmp = s_uselock;
       while (*dx_tmp) {
          if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_'
               && *dx_tmp != '@' && *dx_tmp != '-'
               &&  *dx_tmp != '+')
             *dx_tmp = 'X';
          dx_tmp++;
       }
       hk_ap2 = atr_str(s_uselock);
       chk_stop = mudstate.chkcpu_stopper;
       chk_tog  = mudstate.chkcpu_toggle;
       mudstate.chkcpu_stopper = time(NULL);
       mudstate.chkcpu_toggle = 0;
       mudstate.chkcpu_locktog = 0;
       process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
       mudstate.chkcpu_toggle = chk_tog;
       mudstate.chkcpu_stopper = chk_stop;
       free_sbuf(s_uselock);
    } else {
       notify(player, "Permission denied.");
    }
}
/* ---------------------------------------------------------------------------
 * process_cmdent: Perform indicated command with passed args.
 */

void 
process_cmdent(CMDENT * cmdp, char *switchp, dbref player,
           dbref cause, int interactive, char *arg,
           char *unp_command, char *cargs[], int ncargs, int ccheck)
{
    char *buf1, *buf2, tchar, *s_uselock, *dx_tmp;
    char *args[MAX_ARG], *tpr_buff, *tprp_buff;
    int nargs, i, fail, interp, key, xkey, chk_tog, argtwo_save;
    ATTR *hk_ap2;
    time_t chk_stop;

    DPUSH; /* #27 */


    /* This should never happen, but with memory corruption it's possible 
     * Thus, we put a check in to validate both the command and the player
     */
    if (!Good_obj(player) || !Good_obj(cause) || !cmdp ) {
        DPOP; /* #27 */
    return;
    }

    /* Perform object type checks. */
#define Protect(x) (cmdp->perms & x)

    fail = 0;

    if (Protect(CA_LOCATION) && !Has_location(player))
    fail++;
    if (Protect(CA_CONTENTS) && !Has_contents(player))
    fail++;
    if (Protect(CA_PLAYER) && (Typeof(player) != TYPE_PLAYER))
    fail++;
    if (fail > 0) {
    notify(player, "Command incompatible with invoker type.");
        DPOP; /* #27 */
    return;
    }
    /* Check if we have permission to execute the command */
    if (!check_access(player, cmdp->perms, cmdp->perms2, ccheck)) {
        process_error_control(player, cmdp);
        DPOP; /* #27 */
    return;
    }
    /* Check global flags */

    if (Protect(CA_GBL_BUILD) && !(mudconf.control_flags & CF_BUILD) && !Builder(player)) {
    notify(player, "Sorry, building is not allowed now.");
        DPOP; /* #27 */
    return;
    }
    if (Protect(CA_GBL_INTERP) && !(mudconf.control_flags & CF_INTERP)) {
    notify(player,
           "Sorry, queueing and triggering are not allowed now.");
        DPOP; /* #27 */
    return;
    }
    key = cmdp->extra & ~SW_MULTIPLE;
    if (key & SW_GOT_UNIQUE) {
    i = 1;
    key = key & ~SW_GOT_UNIQUE;
    } else {
    i = 0;
    }

    /* Check command switches.  Note that there may be more than one,
     * and that we OR all of them together along with the extra value from
     * the command table to produce the key value in the handler call.
     */

    xkey = 0;
    if (switchp && cmdp->switches) {
    do {
        buf1 = (char *) index(switchp, '/');
        if (buf1)
        *buf1++ = '\0';
        xkey = search_nametab(player, cmdp->switches, switchp);
        if (xkey == -1) {
                tprp_buff = tpr_buff = alloc_lbuf("process_cmdent");
                if ( strcmp(switchp, "syntax") == 0 ) {
                   switch (cmdp->callseq & CS_NARG_MASK) {
                      case CS_NO_ARGS:
                         sprintf(tpr_buff, "Syntax: %s[</switch(s)>]             (no arguments)", cmdp->cmdname);
                         break;
                      case CS_ONE_ARG:
                         sprintf(tpr_buff, "Syntax: %s[</switch(s)>] <argument>", cmdp->cmdname);
                         break;
                      case CS_TWO_ARG:
                         sprintf(tpr_buff, "Syntax: %s[</switch(s)>] <argument> = <argument>", cmdp->cmdname);
                         break;
                       default:
                          sprintf(tpr_buff, "Syntax: %s -- I'm not sure what the syntax is.", cmdp->cmdname);
                   }
                   notify(player, tpr_buff);
                   display_nametab(player, cmdp->switches, (char *)"Available Switches:", 0);
                } else {
           notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                  "Unrecognized switch '%s' for command '%s'.",
                      switchp, cmdp->cmdname));
                   display_nametab(player, cmdp->switches, (char *)"Available Switches:", 0);
                }
                free_lbuf(tpr_buff);
                DPOP; /* #27 */
        return;
        } else if (!(xkey & SW_MULTIPLE)) {
        if (i == 1) {
            notify(player,
               "Illegal combination of switches.");
                    DPOP; /* #27 */
            return;
        }
        i = 1;
        } else {
        xkey &= ~SW_MULTIPLE;
        }
        if (!(cmdp->callseq & CS_SEP)) 
          key |= xkey;
        switchp = buf1;
    } while (buf1);
    } else if (switchp && ((cmdp->callseq & CS_PASS_SWITCHES) == 0)) {
        tprp_buff = tpr_buff = alloc_lbuf("process_cmdent");
        if ( strcmp(switchp, "syntax") == 0 ) {
           switch (cmdp->callseq & CS_NARG_MASK) {
              case CS_NO_ARGS:
                 sprintf(tpr_buff, "Syntax: %s                          (no arguments or switches)", cmdp->cmdname);
                 break;
              case CS_ONE_ARG:
                 sprintf(tpr_buff, "Syntax: %s <argument>               (no switches)", cmdp->cmdname);
                 break;
              case CS_TWO_ARG:
                 sprintf(tpr_buff, "Syntax: %s <argument> = <argument>  (no switches)", cmdp->cmdname);
                 break;
              default:
                 sprintf(tpr_buff, "Syntax: %s -- I'm not sure what the syntax is.", cmdp->cmdname);
           }
           notify(player, tpr_buff);
           notify(player, (char *)"Available Switches: N/A (no switches exist)");
        } else {
       notify(player,
              safe_tprintf(tpr_buff, &tprp_buff, "Command %s does not take switches.",
                  cmdp->cmdname));
        }
        free_lbuf(tpr_buff);
        DPOP; /* #27 */
    return;
    }
    /* Let's do the hook stuff */
    if ( (cmdp->hookmask & HOOK_BEFORE) && Good_obj(mudconf.hook_obj) && 
         !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
       s_uselock = alloc_sbuf("command_hook_process");
       memset(s_uselock, 0, SBUF_SIZE);
       if ( strcmp(cmdp->cmdname, "S") == 0 )
          strcpy(s_uselock, "B_say");
       else if ( strcmp(cmdp->cmdname, "P") == 0 )
          strcpy(s_uselock, "B_pose");
       else if ( strcmp(cmdp->cmdname, "E") == 0 )
          strcpy(s_uselock, "B_@emit");
       else if ( strcmp(cmdp->cmdname, "F") == 0 )
          strcpy(s_uselock, "B_@force");
       else if ( strcmp(cmdp->cmdname, "V") == 0 )
          strcpy(s_uselock, "B_@set");
       else if ( strcmp(cmdp->cmdname, "M") == 0 )
          strcpy(s_uselock, "B_mail");
       else if ( strcmp(cmdp->cmdname, "N") == 0 )
          strcpy(s_uselock, "B_N");
       else
          sprintf(s_uselock, "B_%.29s", cmdp->cmdname);
       dx_tmp = s_uselock;
       while (*dx_tmp) {
          if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_' 
           && *dx_tmp != '@' && *dx_tmp != '-' 
           &&  *dx_tmp != '+')
             *dx_tmp = 'X';
          dx_tmp++;
       }
       hk_ap2 = atr_str(s_uselock);
       chk_stop = mudstate.chkcpu_stopper;
       chk_tog  = mudstate.chkcpu_toggle;
       mudstate.chkcpu_stopper = time(NULL);
       mudstate.chkcpu_toggle = 0;
       mudstate.chkcpu_locktog = 0;
       process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
       mudstate.chkcpu_toggle = chk_tog;
       mudstate.chkcpu_stopper = chk_stop;
       free_sbuf(s_uselock);
    }

    /* We are allowed to run the command.  Now, call the handler using the
     * appropriate calling sequence and arguments.
     */

    if ((cmdp->callseq & CS_INTERP) ||
    !(interactive || (cmdp->callseq & CS_NOINTERP)))
    interp = EV_EVAL | EV_STRIP;
    else if (cmdp->callseq & CS_STRIP)
    interp = EV_STRIP;
    else if (cmdp->callseq & CS_STRIP_AROUND)
    interp = EV_STRIP_AROUND;
    else
    interp = 0;

   if ( mudstate.trainmode )
        interp = interp & ~EV_EVAL & ~EV_STRIP;

    switch (cmdp->callseq & CS_NARG_MASK) {
    case CS_NO_ARGS:        /* <cmd>   (no args) */
    (*(cmdp->handler)) (player, cause, key);
    break;
    case CS_ONE_ARG:        /* <cmd> <arg> */

    /* If an unparsed command, just give it to the handler */

    if (cmdp->callseq & CS_UNPARSE) {
        (*(cmdp->handler)) (player, unp_command);
        break;
    }
    /* Interpret if necessary */

    if (interp & EV_EVAL)
        buf1 = exec(player, cause, cause, interp | EV_FCHECK | EV_TOP,
            arg, cargs, ncargs, (char **)NULL, 0);
    else
        buf1 = parse_to(&arg, '\0', interp | EV_TOP);

    /* Call the correct handler */

    if (cmdp->callseq & CS_CMDARG) {
        (*(cmdp->handler)) (player, cause, key, buf1,
                cargs, ncargs);
    } 
    else if(cmdp->callseq & CS_PASS_SWITCHES) {
      (*(cmdp->handler)) (player, cause, switchp, buf1);
    }
    else {
      (*(cmdp->handler)) (player, cause, key, buf1);
    }

    /* Free the buffer if one was allocated */

    if (interp & EV_EVAL)
        free_lbuf(buf1);
    break;
    case CS_TWO_ARG:        /* <cmd> <arg1> = <arg2> */

    /* Interpret ARG1 */

        argtwo_save = mudstate.argtwo_fix;
        if ( arg && *arg && (strchr(arg, '=') == NULL) ) {
           mudstate.argtwo_fix = 1;
        } else {
           mudstate.argtwo_fix = 0;
        }

    buf2 = parse_to(&arg, '=', EV_STRIP_TS);

    /* Handle when no '=' was specified */

    if (!arg || (arg && !*arg)) {
        arg = &tchar;
        *arg = '\0';
    }
        if ( mudstate.trainmode ) {
           buf1 = alloc_lbuf("temp_buffer_for_parser");
           if ( buf2 && *buf2 )
              sprintf(buf1, "%s", buf2);
        } else {
       buf1 = exec(player, cause, cause, EV_STRIP | EV_FCHECK | EV_EVAL | EV_TOP,
               buf2, cargs, ncargs, (char **)NULL, 0);
    }

    if (cmdp->callseq & CS_ARGV) {

        /* Arg2 is ARGV style.  Go get the args */

        parse_arglist(player, cause, cause, arg, '\0',
              interp | EV_STRIP_LS | EV_STRIP_TS,
              args, MAX_ARG, cargs, ncargs, 0, (char **)NULL, 0);
        for (nargs = 0; (nargs < MAX_ARG) && args[nargs]; nargs++);

        /* Call the correct command handler */

        if (cmdp->callseq & CS_CMDARG) {
        (*(cmdp->handler)) (player, cause, key,
                    buf1, args, nargs, cargs, ncargs);
        } else {
        (*(cmdp->handler)) (player, cause, key,
                    buf1, args, nargs);
        }

        /* Free the argument buffers */

        for (i = 0; ((i < nargs) && (i < MAX_ARG)); i++)
        if (args[i])
            free_lbuf(args[i]);
    } else {

        /* Arg2 is normal style.  Interpret if needed */

        if (interp & EV_EVAL) {
        buf2 = exec(player, cause, cause,
                interp | EV_FCHECK | EV_TOP,
                arg, cargs, ncargs, (char **)NULL, 0);
        } else {
        buf2 = parse_to(&arg, '\0',
                interp | EV_STRIP_LS | EV_STRIP_TS | EV_TOP);
        }

        /* Call the correct command handler */

        if (cmdp->callseq & CS_CMDARG) {
        (*(cmdp->handler)) (player, cause, key,
                    buf1, buf2, cargs, ncargs);
        } else {
          if (cmdp->callseq & CS_SEP)
        (*(cmdp->handler)) (player, cause, key, xkey,
                    buf1, buf2);
          else if(cmdp->callseq & CS_PASS_SWITCHES)
        (*(cmdp->handler)) (player, cause, switchp, buf1, buf2);
          else 
        (*(cmdp->handler)) (player, cause, key,
                    buf1, buf2);
        }

        /* Free the buffer, if needed */

        if (interp & EV_EVAL)
        free_lbuf(buf2);
    }
        mudstate.argtwo_fix = argtwo_save;

    /* Free the buffer obtained by evaluating Arg1 */

    free_lbuf(buf1);
    break;
    }

    /* Let's do the other hook stuff */
    if ( (cmdp->hookmask & HOOK_AFTER) && Good_obj(mudconf.hook_obj) &&
         !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
       s_uselock = alloc_sbuf("command_hook_process");
       memset(s_uselock, 0, SBUF_SIZE);
       if ( strcmp(cmdp->cmdname, "S") == 0 )
          strcpy(s_uselock, "A_say");
       else if ( strcmp(cmdp->cmdname, "P") == 0 )
          strcpy(s_uselock, "A_pose");
       else if ( strcmp(cmdp->cmdname, "E") == 0 )
          strcpy(s_uselock, "A_@emit");
       else if ( strcmp(cmdp->cmdname, "F") == 0 )
          strcpy(s_uselock, "A_@force");
       else if ( strcmp(cmdp->cmdname, "V") == 0 )
          strcpy(s_uselock, "A_@set");
       else if ( strcmp(cmdp->cmdname, "M") == 0 )
          strcpy(s_uselock, "A_mail");
       else if ( strcmp(cmdp->cmdname, "N") == 0 )
          strcpy(s_uselock, "A_N");
       else
          sprintf(s_uselock, "A_%.29s", cmdp->cmdname);
       dx_tmp = s_uselock;
       while (*dx_tmp) {
          if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_'
           && *dx_tmp != '@' && *dx_tmp != '-' 
           &&  *dx_tmp != '+')
             *dx_tmp = 'X';
          dx_tmp++;
       }
       hk_ap2 = atr_str(s_uselock);
       chk_stop = mudstate.chkcpu_stopper;
       chk_tog  = mudstate.chkcpu_toggle;
       mudstate.chkcpu_stopper = time(NULL);
       mudstate.chkcpu_toggle = 0;
       mudstate.chkcpu_locktog = 0;
       process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
       mudstate.chkcpu_toggle = chk_tog;
       mudstate.chkcpu_stopper = chk_stop;
       free_sbuf(s_uselock);
    }

    DPOP; /* #27 */
    return;
}

void reportcputime(dbref player, struct itimerval *itimer)
{
  unsigned long hundstart;
  unsigned long hundend;
  unsigned long hundinterval;
  char *tpr_buff, *tprp_buff;

  DPUSH; /* #28 */

  if( CpuTime(player) ) {
    hundstart = 1000 * 100;
    hundend = itimer->it_value.tv_sec * 100 + 
              itimer->it_value.tv_usec / 10000;
    if ( hundend > hundstart )
       hundinterval = 0;
    else
       hundinterval = hundstart - hundend; 
    tprp_buff = tpr_buff = alloc_lbuf("reportcputime");
    notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "[CPU: %2ld.%02ld  EVALS: %4d  FUNCS: %4d  ATRFETCH: %4d]",
                                      hundinterval / 100,
                                      hundinterval % 100,
                                      mudstate.evalcount,
                                      mudstate.funccount,
                                      mudstate.attribfetchcount));
    free_lbuf(tpr_buff);
  }
  DPOP; /* #28 */
}

extern int rtpool();

int zonecmdtest(dbref player, char *cmd)
{
   dbref loc, tloc;
   int i_ret;
   ZLISTNODE *ptr;

   if ( !Good_obj(player) || Immortal(player) ) {
      return 0;
   }
   loc = Location(player);
   tloc = TopLocation(player);
   if ( Good_obj(tloc) && !(Good_obj(loc) && IgnoreZone(loc)) )
      loc = tloc;

   i_ret = 0;
   if ( Good_obj(loc) ) {
      if ( IgnoreZone(loc) ) {
         i_ret = cmdtest(loc, cmd); 
      }
      if ( i_ret == 0 ) {
         for ( ptr = db[loc].zonelist; ptr; ptr = ptr->next ) {
            if ( Good_obj(ptr->object) && !Recover(ptr->object) &&
                 (isRoom(ptr->object) || isThing(ptr->object)) &&
                 IgnoreZone(ptr->object) ) {
               i_ret = cmdtest(ptr->object, cmd);
               if ( i_ret > 0 ) {
                  break;
               }
            }
         }
      }
   }
   return i_ret;
}

int cmdtest(dbref player, char *cmd)
{
  char *buff1, *buff2, *mbuf, *pt1, *pt2;
  dbref aowner, aowner2;
  int aflags, aflags2, rval;
  ATTR *atr, *atr2;

  rval = 0;
  buff1 = atr_get(player, A_CMDCHECK, &aowner, &aflags);
  buff2 = NULL;
  pt1 = buff1;
  while (pt1 && *pt1) {
    pt2 = strchr(pt1, ':');
    if (!pt2 || (pt2 == pt1))
      break;
    if (!(strncmp(pt2+1,cmd,strlen(cmd)))) {
      if (*(pt2-1) == '1') {
    rval = 1;
      } else if ( *(pt2-1) == '3') {
         if ( !mudstate.insideicmds && Good_chk(mudconf.icmd_obj) ) {
            mbuf = alloc_mbuf("cmdtest_eval");
            sprintf(mbuf, "#%d_%.*s", player, MBUF_SIZE-20, cmd);
            atr = atr_str(mbuf);
            sprintf(mbuf, "_%.*s", MBUF_SIZE-20, cmd);
            atr2 = atr_str2(mbuf);
            free_mbuf(mbuf);
            if ( !atr && !atr2 ) {
               rval = 0;
            } else {
               if ( atr2 ) {
                  buff2 = atr_get(mudconf.icmd_obj, atr2->number, &aowner2, &aflags2);
                  if ( !*buff2 && atr ) {
                     free_lbuf(buff2);
                     buff2 = atr_get(mudconf.icmd_obj, atr->number, &aowner2, &aflags2);
                  }
               } else if ( atr ) {
                  buff2 = atr_get(mudconf.icmd_obj, atr->number, &aowner2, &aflags2);
               }
               if ( *buff2 ) {
                  mudstate.insideicmds = 1;
                  mbuf = exec(mudconf.icmd_obj, player, player, EV_EVAL | EV_FCHECK, buff2, 
                              (char **)NULL, 0, (char **)NULL, 0);
                  mudstate.insideicmds = 0;
                  if ( *mbuf ) {
                     if ( atoi(mbuf) == 2 )
                        rval = 2;
                     else if ( atoi(mbuf) == 1 )
                        rval = 1;
                     else
                        rval = 0;
                  } else {
                     rval = 0;
                  }
                  free_lbuf(mbuf);
               } else {
                  rval = 0;
               }
               free_lbuf(buff2);
            }
         } else {
            rval = 0;
         }
      } else {
    rval = 2;
      }
      break;
    }
    pt1 = strchr(pt2+1,' ');
    if (pt1 && *pt1)
      while (isspace((int)*pt1))
    pt1++;
  }
  free_lbuf(buff1);
  return rval;
}


/************************************************************
 *
 * Functions: lookup_command
 *            lookup_orig_command
 *
 * Purpose  : These functions will attempt to find the best
 *            match for the command name passed in as the
 *            argument.
 *            First they look for an internal command that
 *            matches, and if that fails they check for a
 *            matching alias.
 * 
 * Notes    : lookup_orig_command returns a pointer to the 
 *            command pointed to by the alias.
 *            lookup_command returns a COPY of the command.
 *
 *            Not thread safe, but nor's most of Rhost :P
 *
 * Lensy.
 */
CMDENT * lookup_command(char *cmdname) {
  static CMDENT cmd;
  CMDENT *cmdp, *retval;
  ALIASENT *aliasp;

  retval = NULL;

  /* Check for a builtin command (or an alias of a builtin command) */
  cmdp = (CMDENT *) hashfind(cmdname, &mudstate.command_htab);
  
  if (cmdp) {
    retval = cmdp;
  } else {
    aliasp = (ALIASENT *) hashfind(cmdname, &mudstate.cmd_alias_htab);
    if (aliasp) {
      cmd = *(aliasp->cmdp);
      cmd.extra |= aliasp->extra;
      cmd.perms |= aliasp->perms;
      cmd.perms2 |= aliasp->perms2;
      retval = &cmd;
    }
  }
  return retval;
}

CMDENT *lookup_orig_command(char *cmdname) {
  CMDENT *cmdp, *retval;
  ALIASENT *aliasp;

  retval = NULL;

  /* Check for a builtin command (or an alias of a builtin command) */
  cmdp = (CMDENT *) hashfind(cmdname, &mudstate.command_htab);
  
  if (cmdp) {
    retval = cmdp;
  } else {
    aliasp = (ALIASENT *) hashfind(cmdname, &mudstate.cmd_alias_htab);
    if (aliasp) {
      retval = aliasp->cmdp;
    }
  }
  return retval;
}

/* ---------------------------------------------------------------------------
 * process_command: Execute a command.
 */
void 
process_command(dbref player, dbref cause, int interactive,
        char *command, char *args[], int nargs, int shellprg)
{
    char *p, *q, *arg, *lcbuf, *slashp, *cmdsave, *msave, check2[2], lst_cmd[LBUF_SIZE], *dx_tmp;
    int succ, aflags, i, cval, sflag, cval2, chklogflg, aflags2, narg_prog, i_trace, i_retvar = -1, i_targetchk = 0;
    int boot_plr, do_ignore_exit, hk_retval, aflagsX, spamtimeX, spamcntX, xkey, chk_tog, i_fndexit, i_targetlist, i_apflags;
    char *arr_prog[LBUF_SIZE/2], *progatr, *cpulbuf, *lcbuf_temp, *s_uselock, *swichk_ptr, swichk_buff[80], *swichk_tok;
    char *lcbuf_temp_ptr, *log_indiv, *log_indiv_ptr, *cut_str_log, *cut_str_logptr, *tchbuff, *spamX, *spamXptr;
    char *tpr_buff, *tprp_buff, *s_aptext, *s_aptextptr, *s_strtokr, *tbuff;
    dbref pcexit, aowner, aowner2, aownerX, spamowner, passtarget, targetlist[LBUF_SIZE], i_apowner;
    CMDENT *cmdp;
    ZLISTNODE *zonelistnodeptr;
    dbref realloc;
    struct itimerval itimer;
    DESC *d;
    ATTR *hk_ap2, *ap_log;
    time_t chk_stop;
#ifdef ENH_LOGROOM
    char *s_logroom;
    int i_loc;
    s_logroom = NULL;
#endif

    DPUSH; /* #29 */

    if (!command)
    abort();

    cval = cval2 = 0;
    mudstate.func_bypass = 0;
    mudstate.shifted = 0;
    mudstate.notrace = 0;
    mudstate.start_of_cmds = 0;
    succ = i_fndexit = 0;
    cache_reset(0);
    memset(lst_cmd, 0, sizeof(lst_cmd));
    memset(mudstate.last_command, 0, sizeof(mudstate.last_command));
#ifndef NODEBUGMONITOR
    memset(debugmem->last_command, 0, sizeof(debugmem->last_command));
#endif
    *(check2 + 1) = '\0';

    /* Robustify player */

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< process_command >";
    mudstate.last_cmd_timestamp = mudstate.now;
    mudstate.heavy_cpu_recurse = 0;
    mudstate.heavy_cpu_tmark1 = time(NULL);
    mudstate.stack_val = 0;
    mudstate.stack_toggle = 0;
    mudstate.sidefx_currcalls = 0;
    mudstate.curr_percentsubs = 0;
    mudstate.tog_percentsubs = 0;
    mudstate.sidefx_toggle = 0;
    mudstate.log_maximum = 0;
    mudstate.chkcpu_stopper = time(NULL);

    /* profiling */
    mudstate.evalcount = 0;
    mudstate.funccount = 0;
    mudstate.attribfetchcount = 0;
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_usec = 0;
    itimer.it_value.tv_sec = 1000;
    setitimer(ITIMER_PROF, &itimer, NULL);

    if (!Good_obj(player) || !Good_obj(cause)) {
    STARTLOG(LOG_BUGS, "CMD", "PLYR")
        lcbuf = alloc_mbuf("process_command.LOG.badplayer");
        if ( !Good_obj(player) )
       sprintf(lcbuf, "Bad player in process_command: %d",
           player);
        else
       sprintf(lcbuf, "Bad player(cause) in process_command: %d",
           cause);
    log_text(lcbuf);
    free_mbuf(lcbuf);
    ENDLOG
        mudstate.debug_cmd = cmdsave;
        DPOP; /* #29 */
    return;
    }
    /* Make sure player isn't going or halted */

  if (!( (player == cause) && (mudstate.force_halt == player))) {
    if (Going(player) || Recover(player) ||
    (Halted(player) &&
     !((Typeof(player) == TYPE_PLAYER) && interactive) && !ForceHalted(cause))) {
        tprp_buff = tpr_buff = alloc_lbuf("process_command");
    notify(Owner(player),
           safe_tprintf(tpr_buff, &tprp_buff, "Attempt to execute command by halted object #%d",
               player));
        free_lbuf(tpr_buff);
    mudstate.debug_cmd = cmdsave;
        DPOP; /* #29 */
    return;
    }
    }

    /* Spam protection time */
    if ( NoSpam(player) || (Good_obj(Owner(player)) && NoSpam(Owner(player))) ) {
       if ( Good_obj(Owner(player)) )
          spamowner = Owner(player);
       else
          spamowner = player;
       spamX = atr_get(spamowner, A_SPAMPROTECT, &aownerX, &aflagsX);
       if ( spamX && *spamX ) {
          spamXptr = strtok(spamX, " ");
          if ( spamXptr && *spamXptr ) {
             spamtimeX = atoi(spamXptr);
             spamXptr = strtok(NULL, " ");
             if ( spamXptr && *spamXptr )
                spamcntX = atoi(spamXptr);
             else
                spamcntX = 1;
          } else {
             spamtimeX = time(NULL);
             spamcntX = 1;
          }
          if ( (time(NULL) < (spamtimeX + 60)) && (spamcntX >= mudconf.spam_limit) ) {
             if ( player != spamowner ) {
                     tprp_buff = tpr_buff = alloc_lbuf("process_command");
             notify(spamowner, safe_tprintf(tpr_buff, &tprp_buff, "%s",mudconf.spam_objmsg));
                     free_lbuf(tpr_buff);
             }
             tprp_buff = tpr_buff = alloc_lbuf("process_command");
         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s",mudconf.spam_msg));
             free_lbuf(tpr_buff);
         
         mudstate.debug_cmd = cmdsave;
             STARTLOG(LOG_ALWAYS, "WIZ", "SPAM");
             log_name_and_loc(player);
             cpulbuf = alloc_lbuf("cpulbuf");
             sprintf(cpulbuf, " SEC/SPAM overload: (#%d) %.3940s", spamowner, command);
             log_text(cpulbuf);
             free_lbuf(cpulbuf);
             ENDLOG
             if ( spamcntX == mudconf.spam_limit ) {
                broadcast_monitor(player, MF_CPU, "POSSIBLE SPAM ATTACK",
                                 (char *)command, NULL, cause, 0, 0, NULL);
                spamcntX++;
                lcbuf = alloc_lbuf("spam_command_protect");
                sprintf(lcbuf, "%d %d", spamtimeX, spamcntX);
                atr_add_raw(spamowner, A_SPAMPROTECT, lcbuf);
                free_lbuf(lcbuf);
             }
             free_lbuf(spamX);
             DPOP; /* #29 */
             return;
          } else if ( time(NULL) < (spamtimeX + 60) ) {
             spamcntX++;
          } else {
             spamtimeX = time(NULL);
             spamcntX = 1;
          }
          lcbuf = alloc_lbuf("spam_command_protect");
          sprintf(lcbuf, "%d %d", spamtimeX, spamcntX);
          atr_add_raw(spamowner, A_SPAMPROTECT, lcbuf);
          free_lbuf(lcbuf);
       } else {
          lcbuf = alloc_lbuf("spam_command_protect");
          sprintf(lcbuf, "%d %d", (int)time(NULL), 1);
          atr_add_raw(spamowner, A_SPAMPROTECT, lcbuf);
          free_lbuf(lcbuf);
       }
       free_lbuf(spamX);
    }

    chklogflg = 0;
    if ( Good_obj(player) && Suspect(player) ) {
       STARTLOG(LOG_SUSPECT, "CMD", "SUS")
          log_name_and_loc(player);
          lcbuf = alloc_lbuf("process_command.LOG.allcmds");
    /* Fix buffering so log can't be overflowed and blowup the lbuf */
          if ( strlen(command) > 3900 )
             sprintf(lcbuf, " entered: '%.3900s [overflow cut]', Lbufs: %d", command, rtpool());
          else
             sprintf(lcbuf, " entered: '%s', Lbufs: %d", command, rtpool());
          log_text(lcbuf);
          free_lbuf(lcbuf);
          chklogflg=1;
       ENDLOG
    }

#ifdef ENH_LOGROOM
    if ( Good_obj(player) )
       i_loc = Location(player);
    else
       i_loc = NOTHING;
    if ( Good_obj(i_loc) && !Recover(i_loc) && !Going(i_loc) && LogRoomEnh(i_loc) ) {
       s_logroom = alloc_mbuf("log_room");
       memset(s_logroom, '\0', MBUF_SIZE);
       sprintf(s_logroom, "%.128s/room_%d", mudconf.roomlog_path, i_loc);
       do_log(player, cause, (MLOG_FILE|MLOG_ROOM), s_logroom, (char *)command);
       free_mbuf(s_logroom);
    }
#endif

    if (!chklogflg && Good_obj(player) && God(player)) {
       STARTLOG(LOG_GHOD, "CMD", "GHOD")
       log_name_and_loc(player);
       lcbuf = alloc_lbuf("process_command.LOG.allcmds");
       if ( strlen(command) > 3900 )
          sprintf(lcbuf, " entered: '%.3900s [overflow cut]', Lbufs: %d", command, rtpool());
       else
          sprintf(lcbuf, " entered: '%s', Lbufs: %d", command, rtpool());
       log_text(lcbuf);
       free_lbuf(lcbuf);
       chklogflg=1;
       ENDLOG
    }
    if (!chklogflg && Good_obj(player) && Guest(player)) {
       STARTLOG(LOG_GUEST, "CMD", "GST")
       log_name_and_loc(player);
       lcbuf = alloc_lbuf("process_command.LOG.allcmds");
    /* Fix buffering so log can't be overflowed and blowup the lbuf */
       if ( strlen(command) > 3900 )
          sprintf(lcbuf, " entered: '%.3900s [overflow cut]', Lbufs: %d", command, rtpool());
       else
          sprintf(lcbuf, " entered: '%s', Lbufs: %d", command, rtpool());
       log_text(lcbuf);
       free_lbuf(lcbuf);
       chklogflg=1;
       ENDLOG
    }

    if (!chklogflg && Good_obj(player) && Wanderer(player)) {
       STARTLOG(LOG_WANDERER, "CMD", "WDR")
       log_name_and_loc(player);
       lcbuf = alloc_lbuf("process_command.LOG.allcmds");
    /* Fix buffering so log can't be overflowed and blowup the lbuf */
       if ( strlen(command) > 3900 )
          sprintf(lcbuf, " entered: '%.3900s [overflow cut]', Lbufs: %d", command, rtpool());
       else
      sprintf(lcbuf, " entered: '%s', Lbufs: %d", command, rtpool());
       log_text(lcbuf);
       free_lbuf(lcbuf);
       chklogflg=1;
       ENDLOG
    }

    if (!chklogflg) {
       STARTLOG(LOG_ALLCOMMANDS, "CMD", "ALL")
       log_name_and_loc(player);
       lcbuf = alloc_lbuf("process_command.LOG.allcmds");
    /* Fix buffering so log can't be overflowed and blowup the lbuf */
       if ( strlen(command) > 3900 )
          sprintf(lcbuf, " entered: '%.3900s [overflow cut]', Lbufs: %d", command, rtpool());
       else
          sprintf(lcbuf, " entered: '%s', Lbufs: %d", command, rtpool());
       log_text(lcbuf);
       free_lbuf(lcbuf);
       chklogflg=1;
       ENDLOG
    }

    if ( !chklogflg && (mudconf.log_command_list[0] != '\0') ) {
       log_indiv_ptr = log_indiv = alloc_lbuf("command_cut");
       lcbuf = alloc_lbuf("process_command.LOG.individual");
       strcpy(lcbuf, mudconf.log_command_list);
#ifdef ENHANCED_LOGGING
       safe_str(command, log_indiv, &log_indiv_ptr);
#else
       cut_str_log = alloc_lbuf("process_command.LOG.enhanced");
       strcpy(cut_str_log, command);
       cut_str_logptr = strtok(cut_str_log, " \t");
       safe_str(cut_str_logptr, log_indiv, &log_indiv_ptr);
       free_lbuf(cut_str_log);
#endif
       if ( lookup(log_indiv, lcbuf, -2, &i_retvar) ) {
          STARTLOG(LOG_ALWAYS, "CMD", "INDIVIDUAL")
             log_name_and_loc(player);
             if ( player != cause )
                sprintf(lcbuf, " <home: #%d> (cause: %s(#%d) at %s(#%d) <home: #%d>) entered: '%.3500s', Lbufs: %d", 
                               Home(player), Name(cause), cause,
                               (Good_obj(Location(cause)) ? Name(Location(cause)) : "<Invalid>"),
                               Location(cause), Home(cause),
                               command, rtpool());
             else
                sprintf(lcbuf, " <home: #%d> entered: '%.3900s', Lbufs: %d", 
                               Home(player), command, rtpool());
             log_text(lcbuf);
             if ( Good_obj(cause) && (cause != Owner(cause)) ) {
                sprintf(lcbuf, " (Cause Owner: #%d)", Owner(cause));
                log_text(lcbuf);
             }
             if ( Good_obj(player) && (player != Owner(player)) ) {
                sprintf(lcbuf, " (Player Owner: #%d)", Owner(player));
                log_text(lcbuf);
             }
          ENDLOG
          chklogflg=1;
       }
       free_lbuf(lcbuf);
       free_lbuf(log_indiv);
    }

    /* Reset recursion limits */

    mudstate.trace_nest_lev = 0;
    mudstate.func_nest_lev  = 0;
    mudstate.ufunc_nest_lev = 0;
    mudstate.func_invk_ctr  = 0;
    mudstate.ntfy_nest_lev  = 0;
    mudstate.lock_nest_lev  = 0;

    if (Verbose(player)) {
        tprp_buff = tpr_buff = alloc_lbuf("process_command");
    notify(Owner(player), safe_tprintf(tpr_buff, &tprp_buff, "%s] %s", Name(player), command));
        if ( Bouncer(player) ) {
            ap_log = atr_str("BOUNCEFORWARD");
            if ( ap_log ) {
               s_aptext = atr_get(player, ap_log->number, &i_apowner, &i_apflags);
               if ( s_aptext && *s_aptext ) {
                  s_aptextptr = strtok_r(s_aptext, " ", &s_strtokr);
                  tbuff = alloc_lbuf("bounce_on_command");
                  i_targetlist = 0;
                  for (i = 0; i < LBUF_SIZE; i++) 
                     targetlist[i] = -2000000;
                  while ( s_aptextptr ) {
                     passtarget = match_thing_quiet(player, s_aptextptr);
                     i_targetchk = 0;
                     for (i = 0; i < LBUF_SIZE; i++) {
                        if ( (targetlist[i] == -2000000) || (targetlist[i] == passtarget) ) {
                           if ( targetlist[i] == -2000000)
                              i_targetchk = 1;
                           break;
                        }
                     }
                     if ( i_targetchk && Good_chk(passtarget) && isPlayer(passtarget) && (passtarget != Owner(player)) ) {
                        if ( !No_Ansi_Ex(passtarget) )
                           sprintf(tbuff, "%sBounce [#%d]>%s %s] %.3950s", ANSI_HILITE, player, ANSI_NORMAL, Name(player), command);
                        else
                           sprintf(tbuff, "Bounce [#%d]> %s] %.3950s", player, Name(player), command);
                        notify_quiet(passtarget, tbuff);
                     }
                     s_aptextptr = strtok_r(NULL, " ", &s_strtokr);
                     targetlist[i_targetlist] = passtarget;
                     i_targetlist++;
                  }
                  free_lbuf(tbuff);
               }
               free_lbuf(s_aptext);
            }
        }

        free_lbuf(tpr_buff);
    }

    /* Eat leading whitespace, and space-compress if configured */

    while (*command && isspace((int)*command))
    command++;
    strncpy(lst_cmd, command, LBUF_SIZE - 1);
    strncpy(mudstate.last_command, command, LBUF_SIZE - 1);
#ifndef NODEBUGMONITOR
    strncpy(debugmem->last_command, command, LBUF_SIZE - 1);
    debugmem->last_player = player;
#endif
    mudstate.debug_cmd = command;
    mudstate.curr_cmd  = lst_cmd;

    if (mudconf.space_compress) {
    p = q = command;
    while (*p) {
        while (*p && !isspace((int)*p))
        *q++ = *p++;    /* scan over word */
        while (*p && isspace((int)*p))
        p++;        /* smash spaces */
        if (*p)
        *q++ = ' '; /* separate words */
    }
    *q = '\0';      /* terminate string */
    }
    /* Reset the cache so that unreferenced attributes may be flushed */

    cache_reset(0);

    /* Should always check if in program before it checks any other command */
    if (InProgram(player) && !(shellprg || mudstate.shell_program)) {
       if ((command[0] == '|') && !((NoShProg(player) && !mudconf.noshell_prog) || 
           (!Immortal(player) && mudconf.noshell_prog && !NoShProg(player))) ) {
          command = command+1;
          mudstate.shell_program = 1;
       } else {
          if ( command[0] == '|' && ((NoShProg(player) && !mudconf.noshell_prog) || 
               (mudconf.noshell_prog && !NoShProg(player))) ) {
             notify(player, "You are not allowed to execute commands from within the @program.");
             DESC_ITER_CONN(d) {
                if ( d->player == player ) {
                   progatr = atr_get(d->player, A_PROGPROMPTBUF, &aowner, &aflags);
                   if ( progatr && *progatr ) {
                      if ( strcmp(progatr, "NULL") != 0 ) {
                         tprp_buff = tpr_buff = alloc_lbuf("process_command");
                         queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
                         free_lbuf(tpr_buff);
                      }
                   } else {
                     tprp_buff = tpr_buff = alloc_lbuf("process_command");
                     queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
                     free_lbuf(tpr_buff);
                   }
                   free_lbuf(progatr);
                   break;
                }
             }
          }
          lcbuf = atr_get(player, A_PROGBUFFER, &aowner2, &aflags2);
          memset(arr_prog, 0, sizeof(arr_prog));
#ifdef PROG_LIKEMUX
          narg_prog = newlist2arr(arr_prog, LBUF_SIZE/2, command, '\0');
#else
          narg_prog = newlist2arr(arr_prog, LBUF_SIZE/2, command, ',');
#endif
          do_trigger(player, cause, (TRIG_QUIET | TRIG_PROGRAM), lcbuf, arr_prog, narg_prog);
          free_lbuf(lcbuf);
          atr_clr(player, A_PROGBUFFER);
          s_Flags4(player, (Flags4(player) & (~INPROGRAM)));
          DESC_ITER_CONN(d) {
             if ( d->player == player ) {
                queue_string(d, "\377\371");
             }
          }
      mudstate.debug_cmd = cmdsave;
          getitimer(ITIMER_PROF, &itimer);
          reportcputime(player, &itimer);
          itimer.it_value.tv_sec = 0;
          itimer.it_value.tv_usec = 0;
          setitimer(ITIMER_PROF, &itimer, NULL);
          mudstate.shell_program = 0;
          DPOP; /* #29 */
          return;
       }
    }
    /* Now comes the fun stuff.  First check for single-letter leadins.
     * We check these before checking HOME because they are among the most
     * frequently executed commands, and they can never be the HOME
     * command. */

    i = command[0] & 0xff;
    if (CmdCheck(player) && command[0]) {
      *check2 = i;
      cval = cmdtest(player,check2);
    } else if (CmdCheck(Owner(player)) && command[0]) {
      *check2 = i;
      cval = cmdtest(Owner(player),check2);
    } else
      cval = 0;
    if ( (cval == 0) && command[0]) {
      *check2 = i;
       cval = zonecmdtest(player, check2);
    }
    if ( prefix_cmds[i] )
       cval2 =  zigcheck(player, prefix_cmds[i]->perms);
    else
       cval2 = 0;

    if (command[0] == '\\')
       mudstate.start_of_cmds=1;

    if ( !cval && !cval2 && (prefix_cmds[i] != NULL) && (prefix_cmds[i]->hookmask & (HOOK_IGNORE|HOOK_PERMIT)) && 
         Good_obj(mudconf.hook_obj) && !Recover(mudconf.hook_obj) && 
         !Going(mudconf.hook_obj) ) {
         s_uselock = alloc_sbuf("command_hook");
         memset(s_uselock, 0, SBUF_SIZE);
         if ( prefix_cmds[i]->hookmask & HOOK_IGNORE ) {
            switch(command[0]) {
               case '"' :  sprintf(s_uselock, "I_%s", "say");
                           break;
               case ':' :  
               case ';' :  sprintf(s_uselock, "I_%s", "pose");
                           break;
               case '\\':  sprintf(s_uselock, "I_%s", "@emit");
                           break;
               case '#' :  sprintf(s_uselock, "I_%s", "@force");
                           break;
               case '&' :  sprintf(s_uselock, "I_%s", "@set");
                           break;
               case '>' :  sprintf(s_uselock, "I_%s", "@set");
                           break;
               case '-' :  sprintf(s_uselock, "I_%s", "mail");
                           break;
               default  :  sprintf(s_uselock, "I_%s", "error");
                           break;
            }
         } else {
            switch(command[0]) {
               case '"' :  sprintf(s_uselock, "P_%s", "say");
                           break;
               case ':' :  
               case ';' :  sprintf(s_uselock, "P_%s", "pose");
                           break;
               case '\\':  sprintf(s_uselock, "P_%s", "@emit");
                           break;
               case '#' :  sprintf(s_uselock, "P_%s", "@force");
                           break;
               case '&' :  sprintf(s_uselock, "P_%s", "@set");
                           break;
               case '>' :  sprintf(s_uselock, "I_%s", "@set");
                           break;
               case '-' :  sprintf(s_uselock, "P_%s", "mail");
                           break;
               default  :  sprintf(s_uselock, "P_%s", "error");
                           break;
            }
         }
         dx_tmp = s_uselock;
         while (*dx_tmp) {
            if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_' 
         && *dx_tmp != '@' && *dx_tmp != '-'
         && *dx_tmp != '+') 
               *dx_tmp = 'X';
            dx_tmp++;
         }
     hk_ap2 = atr_str(s_uselock);
         mudstate.chkcpu_stopper = time(NULL);
         mudstate.chkcpu_toggle = 0;
         mudstate.chkcpu_locktog = 0;
         hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 1);
         mudstate.chkcpu_toggle = 0;
         mudstate.chkcpu_locktog = 0;
         free_sbuf(s_uselock);
         if ( !hk_retval && (prefix_cmds[i]->hookmask & HOOK_IGNORE) )
            cval = 2;
         else
            cval = !hk_retval;
    }
    if (((prefix_cmds[i] != NULL) && !igcheck(player,prefix_cmds[i]->perms)) && command[0] && (cval != 2)) { 
    if (cval || cval2) {
          process_error_control(player, prefix_cmds[i]);
      DPOP; /* #29 */
      return;
    }
    mudstate.write_status = 1;
    if (command[0] == '-') {
      msave = alloc_lbuf("MWRITE_SAVE");
      strcpy(msave,command);
    }
    else
      msave = NULL;
        mudstate.chkcpu_stopper = time(NULL);
        mudstate.chkcpu_toggle = 0;
        mudstate.chkcpu_locktog = 0;
    process_cmdent(prefix_cmds[i], NULL, player, cause,
               interactive, command, command, args, nargs, 0);
        if ( mudstate.chkcpu_toggle ) {
           if ( mudstate.curr_cpu_user != player ) {
              mudstate.curr_cpu_user = player;
              mudstate.curr_cpu_cycle = 0;
           }
           mudstate.curr_cpu_cycle++;
           notify(player, "Overload on CPU detected.  Process aborted.");
           if ( mudstate.chkcpu_locktog )
              broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (LOCK-EVAL)",
                                 (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           else
              broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS",
                                 (char *)lst_cmd, NULL, cause, 0, 0, NULL);
       cpulbuf = alloc_lbuf("process_commands.LOG.badcpu");
           STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
       log_name_and_loc(player);
           sprintf(cpulbuf, " CPU overload%s %.3900s", (mudstate.chkcpu_locktog ? " (lock-eval):" : ":"), lst_cmd);
           log_text(cpulbuf);
           free_lbuf(cpulbuf);
           ENDLOG
           if ( mudstate.curr_cpu_cycle >= mudconf.max_cpu_cycles ) {
          cpulbuf = alloc_lbuf("process_commands.LOG.badcpu_toomany");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = NOTHING;
              if (mudconf.cpu_secure_lvl == 1 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted #%d%s", 
                         mudconf.max_cpu_cycles, (boot_plr == -1 ? player : boot_plr),
                         (mudstate.chkcpu_locktog ? " and SILENTEFFECTED (lock-eval)]" : "]"));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL1 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if (mudconf.cpu_secure_lvl == 2 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted%s FUBARED #%d]", 
                         mudconf.max_cpu_cycles, 
                         (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, and" : " and"), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]", 
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL2 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if (mudconf.cpu_secure_lvl == 3 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.  Goodbye.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    s_Flags3(boot_plr, (Flags3(boot_plr) | NOCONNECT));
                    if ( isPlayer(boot_plr) )
                       boot_off(boot_plr, NULL);
                    sprintf(cpulbuf, 
                           "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, and @booted #%d]",
                            mudconf.max_cpu_cycles,
                            (mudstate.chkcpu_locktog ? ", SILENTEFFECTED," : ","), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]", 
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL3 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if ( (mudconf.cpu_secure_lvl == 4) || (mudconf.cpu_secure_lvl == 5) ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.  Goodbye.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    s_Flags3(boot_plr, (Flags3(boot_plr) | NOCONNECT));
                    if ( isPlayer(boot_plr) ) {
                       tchbuff = alloc_mbuf("cpu_regsite");
                       DESC_ITER_CONN(d) {  
                          if ( d->player == boot_plr ) {
                            if ( !(site_check(d->address.sin_addr, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION) ) { 
                               sprintf(tchbuff, "%s 255.255.255.255", inet_ntoa(d->address.sin_addr));
                               if ( mudconf.cpu_secure_lvl == 4 )
                                  cf_site((int *)&mudstate.access_list, tchbuff,
                                          (H_REGISTRATION | H_AUTOSITE), 0, 1, "register_site");
                               else
                                  cf_site((int *)&mudstate.access_list, tchbuff,
                                          (H_FORBIDDEN | H_AUTOSITE), 0, 1, "forbid_site");
                            }
                          }
                       }
                       boot_off(boot_plr, NULL);
                       free_mbuf(tchbuff);
                    }
                    if ( mudconf.cpu_secure_lvl == 4 )
                       sprintf(cpulbuf, 
                              "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, @booted & RegSited #%d]",
                               mudconf.max_cpu_cycles, 
                               (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, " : ", "), boot_plr);
                    else
                       sprintf(cpulbuf, 
                              "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, @booted & ForbidSited #%d]",
                               mudconf.max_cpu_cycles, 
                               (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, " : ", "), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]", 
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL4+ SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else {
                 /* We do nothing extra */
              }
              free_lbuf(cpulbuf);
              mudstate.last_cpu_user = mudstate.curr_cpu_user;
              mudstate.curr_cpu_user = NOTHING;
              mudstate.curr_cpu_cycle = 0;
           }
    }
        mudstate.chkcpu_toggle = 0;
        mudstate.chkcpu_locktog = 0;
    if ( mudstate.stack_toggle ) {
           cpulbuf = alloc_lbuf("stack_limit");
           notify(player, "Stack limit exceeded.  Process aborted.");
           sprintf(cpulbuf, "STACK LIMIT EXCEEDED [%d of 3]: (#%d) %.3900s", mudstate.stack_cntr, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "STACK CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "STACK");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           free_lbuf(cpulbuf);
           if ( time_ng(NULL) > mudstate.cntr_reset ) {
              mudstate.stack_cntr = 0;
              mudstate.cntr_reset = (time_ng(NULL) + 60);
           }
           mudstate.stack_cntr++;
           if ( mudstate.stack_cntr >= 3 ) {
              broadcast_monitor(player, MF_CPUEXT, "MULTIPLE STACK OVERFLOWS", (char *) "[3/@halted]", NULL, cause, 0, 0, NULL);
              notify(player, "Stack violation with recursive calls.  You've been halted.");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = player;
              tprp_buff = tpr_buff = alloc_lbuf("process_command");
              do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
              free_lbuf(tpr_buff);
              s_Flags(player, (Flags(player) | HALT));
              s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
              mudstate.breakst = 1;
              mudstate.stack_cntr = 0;
           }
    } else {
           mudstate.stack_cntr = 0;
        }
        if ( mudstate.sidefx_toggle ) {
           cpulbuf = alloc_lbuf("stack_limit");
           notify(player, "SideFX limit exceeded.  Process aborted.");
           sprintf(cpulbuf, "SideFX LIMIT EXCEEDED [%d]: (#%d) %.3900s", mudconf.sidefx_maxcalls, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "SIDEFX CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "SIDEFX");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           mudstate.breakst = 1;
           free_lbuf(cpulbuf);
           mudstate.sidefx_toggle = 0;
        } 
        if ( mudstate.tog_percentsubs ) {
           cpulbuf = alloc_lbuf("percent_sub_limit");
           notify(player, "Percent Substitutions exceeded.  Process aborted.");
           sprintf(cpulbuf, "Percent Substitutions LIMIT EXCEEDED [%d]: (#%d) %.3900s", 
                   mudconf.max_percentsubs, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "PERCENT-SUB CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "P-SUB");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           mudstate.breakst = 1;
           free_lbuf(cpulbuf);
           if ( time_ng(NULL) > mudstate.cntr_reset ) {
              mudstate.cntr_percentsubs = 0;
              mudstate.cntr_reset = (time_ng(NULL) + 60);
           }
           mudstate.cntr_percentsubs++;
           if ( mudstate.cntr_percentsubs >= 3 ) {
              broadcast_monitor(player, MF_CPUEXT, "MULTIPLE PERCENT-SUB OVERFLOWS", (char *) "[3/@halted]", NULL, cause, 0, 0, NULL);
              notify(player, "Percent-sub violation with recursive calls.  You've been halted.");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = player;
              tprp_buff = tpr_buff = alloc_lbuf("process_command");
              do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
              free_lbuf(tpr_buff);
              s_Flags(player, (Flags(player) | HALT));
              s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
              mudstate.tog_percentsubs = 0;
           }
        } else {
           mudstate.tog_percentsubs = 0;
        }
    if (mudstate.write_status) {
      if (msave)
        free_lbuf(msave);
      mudstate.debug_cmd = cmdsave;
          getitimer(ITIMER_PROF, &itimer);
          reportcputime(player, &itimer);
          itimer.it_value.tv_sec = 0;
          itimer.it_value.tv_usec = 0;
          setitimer(ITIMER_PROF, &itimer, NULL);
          DPOP; /* #29 */
      return;
    }
        if ( msave ) {
       strcpy(command,msave);
       free_lbuf(msave);
        }
    }

    cval = cval2 = 0;
    /* Check for the HOME command */

    if (CmdCheck(player)) {
      cval = cmdtest(player, "home");
    } else if (CmdCheck(Owner(player))) {
      cval = cmdtest(Owner(player), "home");
    } else
      cval = 0;
    if ( cval == 0 )
       cval = zonecmdtest(player, "home");

    if (string_compare(command, "home") == 0 )
      cval2 =  check_access(player, mudconf.restrict_home, mudconf.restrict_home2, 0);
    else
      cval2 = 1;

    if ((string_compare(command, "home") == 0) && !Fubar(player) && !cval && cval2 &&
        (mudstate.remotep == NOTHING) &&
	!No_tel(player)) {
	do_move(player, cause, 0, "home");
	mudstate.debug_cmd = cmdsave;
        getitimer(ITIMER_PROF, &itimer);
        reportcputime(player, &itimer);
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = 0;
        setitimer(ITIMER_PROF, &itimer, NULL);
        DPOP; /* #29 */
    return;
    } else if ((string_compare(command, "home") == 0) && ( Fubar(player) || (cval == 1) ||
                                                           ((cval2 == 0) && (cval != 2)) || 
                                                           ((No_tel(player) || (mudstate.remotep != NOTHING)) && cval != 2) ) && 
                                                         !mudstate.func_ignore) {
    notify_quiet(player, "Permission denied.");
    mudstate.debug_cmd = cmdsave;
        DPOP; /* #29 */
    return;
    }
    /* Only check for exits if we may use the goto command */

    /* Check for @icmd for 'goto' at this point */

    cval = cval2 = 0;
    do_ignore_exit = 0;
    if (check_access(player, goto_cmdp->perms, goto_cmdp->perms2, 0)) {
       /* Test against the 'goto' command */
       if (CmdCheck(player)) {
           cval = cmdtest(player, "goto");
       } else if (CmdCheck(Owner(player))) {
           cval = cmdtest(Owner(player), "goto");
       } else
           cval = 0;
       /* We want to keep this 'player' as it checks location */
       if ( cval == 0 )
          cval = zonecmdtest(player, "goto");
       cval2 = zigcheck(player, goto_cmdp->perms);
       if ( !((cval2 == 2) || (cval == 2)) ) {
    /* Check for an exit name */
          if ( cval != 2 ) {
         mudstate.exitcheck = 1;
         if ((!Fubar(player)) || (Wizard(cause))) {
            init_match_check_keys(player, command, TYPE_EXIT);
            match_exit_with_parents();
            pcexit = last_match_result();
            if (pcexit != NOTHING) {
                   i_fndexit = 1;
                   if ( !cval && !cval2 && (goto_cmdp != NULL) && 
                        (goto_cmdp->hookmask & (HOOK_IGNORE|HOOK_PERMIT)) &&
                        Good_obj(mudconf.hook_obj) && !Recover(mudconf.hook_obj) &&
                        !Going(mudconf.hook_obj) ) {
                      s_uselock = alloc_sbuf("command_hook");
                      memset(s_uselock, 0, SBUF_SIZE);
                      if ( goto_cmdp->hookmask & HOOK_IGNORE )
                         sprintf(s_uselock, "I_%s", "goto");
                      else
                         sprintf(s_uselock, "P_%s", "goto");
                      dx_tmp = s_uselock;
                      while (*dx_tmp) {
                         if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_' 
                      && *dx_tmp != '@' && *dx_tmp != '-' 
                      && *dx_tmp != '+')
                            *dx_tmp = 'X';
                         dx_tmp++;
                      }
                      hk_ap2 = atr_str(s_uselock);
                      mudstate.chkcpu_stopper = time(NULL);
                      mudstate.chkcpu_toggle = 0;
                      mudstate.chkcpu_locktog = 0;
                      hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 1);
                      mudstate.chkcpu_toggle = 0;
                      mudstate.chkcpu_locktog = 0;
                      free_sbuf(s_uselock);
                      if ( !hk_retval && (goto_cmdp->hookmask & HOOK_IGNORE) ) {
                         cval = 2;
                         do_ignore_exit=1;
                      } else
                         cval = !hk_retval;
                   } 
                   if ( mudstate.remotep != NOTHING ) {
		      notify(player, "Permission denied.");
		   } else if ( ((Flags3(player) & NOMOVE) || cval || cval2) && !do_ignore_exit ) {
		      notify(player, "Permission denied.");
                   } else if ( !do_ignore_exit && !cval ) {
              if ( (goto_cmdp->hookmask & HOOK_BEFORE) && Good_obj(mudconf.hook_obj) &&
               !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
                 s_uselock = alloc_sbuf("command_hook_process");
                 memset(s_uselock, 0, SBUF_SIZE);
                 strcpy(s_uselock, "B_goto");
                 hk_ap2 = atr_str(s_uselock);
                 chk_stop = mudstate.chkcpu_stopper;
                 chk_tog  = mudstate.chkcpu_toggle;
                 mudstate.chkcpu_stopper = time(NULL);
                 mudstate.chkcpu_toggle = 0;
                         mudstate.chkcpu_locktog = 0;
                 hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
                 mudstate.chkcpu_toggle = chk_tog;
                 mudstate.chkcpu_stopper = chk_stop;
                 free_sbuf(s_uselock);
              }
              move_exit(player, pcexit, 0, "You can't go that way.", 0);
              if ( (goto_cmdp->hookmask & HOOK_AFTER) && Good_obj(mudconf.hook_obj) &&
                !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
                 s_uselock = alloc_sbuf("command_hook_process");
                 memset(s_uselock, 0, SBUF_SIZE);
                 strcpy(s_uselock, "A_goto");
                 hk_ap2 = atr_str(s_uselock);
                 chk_stop = mudstate.chkcpu_stopper;
                 chk_tog  = mudstate.chkcpu_toggle;
                 mudstate.chkcpu_stopper = time(NULL);
                 mudstate.chkcpu_toggle = 0;
                         mudstate.chkcpu_locktog = 0;
                 hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
                 mudstate.chkcpu_toggle = chk_tog;
                 mudstate.chkcpu_stopper = chk_stop;
                 free_sbuf(s_uselock);
              }
           }
                   if ( !do_ignore_exit ) {
              mudstate.debug_cmd = cmdsave;
              mudstate.exitcheck = 0;
                      getitimer(ITIMER_PROF, &itimer);
                      reportcputime(player, &itimer);
                      itimer.it_value.tv_sec = 0;
                      itimer.it_value.tv_usec = 0;
                      setitimer(ITIMER_PROF, &itimer, NULL);
                      DPOP; /* #29 */
              return;
                   }
            }
            /* Check for an exit in the master room */
            init_match_check_keys(player, command, TYPE_EXIT);
            match_master_exit();
            pcexit = last_match_result();
            if (pcexit != NOTHING) {
                   i_fndexit = 1;
                   if ( !cval && !cval2 && (goto_cmdp != NULL) && 
                        (goto_cmdp->hookmask & (HOOK_IGNORE|HOOK_PERMIT)) &&
                        Good_obj(mudconf.hook_obj) && !Recover(mudconf.hook_obj) &&
                        !Going(mudconf.hook_obj) ) {
                      s_uselock = alloc_sbuf("command_hook");
                      memset(s_uselock, 0, SBUF_SIZE);
                      if ( goto_cmdp->hookmask & HOOK_IGNORE )
                         sprintf(s_uselock, "I_%s", "goto");
                      else
                         sprintf(s_uselock, "P_%s", "goto");
                      dx_tmp = s_uselock;
                      while (*dx_tmp) {
                         if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_' 
                      && *dx_tmp != '@' && *dx_tmp != '-' 
                      && *dx_tmp != '+')
                            *dx_tmp = 'X';
                         dx_tmp++;
                      }
                      hk_ap2 = atr_str(s_uselock);
                      mudstate.chkcpu_stopper = time(NULL);
                      mudstate.chkcpu_toggle = 0;
                      hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 1);
                      mudstate.chkcpu_toggle = 0;
                      mudstate.chkcpu_locktog = 0;
                      free_sbuf(s_uselock);
                      if ( !hk_retval && (goto_cmdp->hookmask & HOOK_IGNORE) ) {
                         cval = 2;
                         do_ignore_exit=1;
                      } else
                         cval = !hk_retval;
                  }
                  if (!((mudstate.remotep != NOTHING) || (Flags3(player) & NOMOVE)) && !cval && !cval2) {
		    if ( (goto_cmdp->hookmask & HOOK_BEFORE) && Good_obj(mudconf.hook_obj) &&
			 !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
		      s_uselock = alloc_sbuf("command_hook_process");
		      memset(s_uselock, 0, SBUF_SIZE);
		      strcpy(s_uselock, "B_goto");
		      hk_ap2 = atr_str(s_uselock);
		      chk_stop = mudstate.chkcpu_stopper;
		      chk_tog  = mudstate.chkcpu_toggle;
		      mudstate.chkcpu_stopper = time(NULL);
		      mudstate.chkcpu_toggle = 0;
                      mudstate.chkcpu_locktog = 0;
              hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
              mudstate.chkcpu_toggle = chk_tog;
              mudstate.chkcpu_stopper = chk_stop;
              free_sbuf(s_uselock);
            }
            move_exit(player, pcexit, 1, NULL, 0);
            if ( (goto_cmdp->hookmask & HOOK_AFTER) && Good_obj(mudconf.hook_obj) &&
             !Recover(mudconf.hook_obj) && !Going(mudconf.hook_obj) ) {
              s_uselock = alloc_sbuf("command_hook_process");
              memset(s_uselock, 0, SBUF_SIZE);
              strcpy(s_uselock, "A_goto");
              hk_ap2 = atr_str(s_uselock);
              chk_stop = mudstate.chkcpu_stopper;
              chk_tog  = mudstate.chkcpu_toggle;
              mudstate.chkcpu_stopper = time(NULL);
              mudstate.chkcpu_toggle = 0;
                      mudstate.chkcpu_locktog = 0;
              hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 0);
              mudstate.chkcpu_toggle = chk_tog;
              mudstate.chkcpu_stopper = chk_stop;
              free_sbuf(s_uselock);
            }
          } else if ( !do_ignore_exit ) {
              notify(player, "Permission denied.");
                  }
                  if ( !do_ignore_exit ) {
             mudstate.debug_cmd = cmdsave;
             mudstate.exitcheck = 0;
                     getitimer(ITIMER_PROF, &itimer);
                     reportcputime(player, &itimer);
                     itimer.it_value.tv_sec = 0;
                     itimer.it_value.tv_usec = 0;
                     setitimer(ITIMER_PROF, &itimer, NULL);
                     DPOP; /* #29 */
             return;
                  }
            }
         } else {
            notify_quiet(cause, "Permission denied.");
            mudstate.debug_cmd = cmdsave;
            mudstate.exitcheck = 0;
                DPOP; /* #29 */
            return;
         }
         mudstate.exitcheck = 0;
          } else {
             init_match_check_keys(player, command, TYPE_EXIT);
             match_exit_with_parents();
             pcexit = last_match_result();
             if ( pcexit != NOTHING ) {
                i_fndexit = 1;
                do_ignore_exit = 1;
             }
          } /* Inner ignore check */
       } else {
          init_match_check_keys(player, command, TYPE_EXIT);
          match_exit_with_parents();
          pcexit = last_match_result();
          if ( pcexit != NOTHING ) {
             i_fndexit = 1;
             do_ignore_exit = 1;
          }
       } /* Outer ignore check */
    } else {
       if ((goto_cmdp->perms & CA_IGNORE_MASK) && 
            igcheck(player, goto_cmdp->perms)) {
          init_match_check_keys(player, command, TYPE_EXIT);
          match_exit_with_parents();
          pcexit = last_match_result();
          if ( pcexit != NOTHING ) {
             i_fndexit = 1;
             do_ignore_exit = 1;
          }
       }
    }

    /* If exit match, clear it */
    if ( !i_fndexit || (i_fndexit && ((cval == 2) || (cval2 == 2))) )
       cval = cval2 = 0;

    /* Set up a lowercase command and an arg pointer for the hashed
     * command check.  Since some types of argument processing destroy
     * the arguments, make a copy so that we keep the original command
     * line intact.  Store the edible copy in lcbuf after the lowercased 
     * command. */
    /* Removed copy of the rest of the command, since it's ok do allow
     * it to be trashed.  -dcm */

    lcbuf = alloc_lbuf("process_commands.LCbuf");
    for (p = command, q = lcbuf; *p && !isspace((int)*p); p++, q++)
    *q = ToLower((int)*p);  /* Make lowercase command */
    *q++ = '\0';        /* Terminate command */
    while (*p && isspace((int)*p))
    p++;            /* Skip spaces before arg */
    arg = p;            /* Remember where arg starts */

    /* Strip off any command switches and save them */

    slashp = (char *) index(lcbuf, '/');
    if (slashp)
    *slashp++ = '\0';

    /* Build a new command of 'goto exit' here */

    if ( mudconf.expand_goto && do_ignore_exit ) {
       lcbuf_temp_ptr = lcbuf_temp = alloc_lbuf("process_commands.exit_buff");
       safe_str("goto ", lcbuf_temp, &lcbuf_temp_ptr);
       safe_str(lcbuf, lcbuf_temp, &lcbuf_temp_ptr);
       do_ignore_exit = 2;
    }

    /* This only happens if expand_goto and do_ignore_exit is true */
    /* Begin if of IGNORE_EXIT */
    /* If we know it's an exit match, we bypass all the below */
    if ( do_ignore_exit != 2 ) {

    /* Check for a builtin command (or an alias of a builtin command) */
    //cmdp = (CMDENT *) hashfind(lcbuf, &mudstate.command_htab);
    cmdp = lookup_command(lcbuf);

    cval = cval2 = 0;
    /* If command is checked to ignore NONMATCHING switches, fall through */
    if ( cmdp && (cmdp->hookmask & HOOK_IGSWITCH) ) {
       if ( slashp && cmdp->switches ) {
          xkey = search_nametab(player, cmdp->switches, slashp);
          if ( xkey & SW_MULTIPLE ) {
             sprintf(swichk_buff, "%.79s", slashp);
             swichk_ptr = strtok_r(swichk_buff, "/", &swichk_tok);
             while ( swichk_ptr && *swichk_ptr ) {
                xkey = search_nametab(player, cmdp->switches, swichk_ptr);
                if ( xkey == -1 )
                   break;
                swichk_ptr = strtok_r(NULL, "/", &swichk_tok);
             }
          }
          if (xkey == -1)
             cval = 2;
       } else if ( slashp && !(cmdp->switches) ) {
          cval = 2;
       }
    }

    /* cval values: 0 normal, 1 disable, 2 ignore */
    if ( (cval != 2) ) {
       if (cmdp && CmdCheck(player)) {
          cval = cmdtest(player, cmdp->cmdname);
       } else if (cmdp && CmdCheck(Owner(player))) {
          cval = cmdtest(Owner(player), cmdp->cmdname);
       } else
         cval = 0;
       /* We want to keep this 'player' as it checks location */
       if ( cmdp && (cval == 0) )
          cval = zonecmdtest(player, cmdp->cmdname);
       if ( cmdp )
          cval2 = zigcheck(player, cmdp->perms);
       else
          cval2 = 0;
    }
    if ( !cval && !cval2 && (cmdp != NULL) && (cmdp->hookmask & (HOOK_IGNORE|HOOK_PERMIT)) &&
         Good_obj(mudconf.hook_obj) && !Recover(mudconf.hook_obj) &&
         !Going(mudconf.hook_obj) ) {
         s_uselock = alloc_sbuf("command_hook");
         memset(s_uselock, 0, SBUF_SIZE);
         if ( cmdp->hookmask & HOOK_IGNORE )
            sprintf(s_uselock, "I_%.29s", cmdp->cmdname);
         else
            sprintf(s_uselock, "P_%.29s", cmdp->cmdname);
         dx_tmp = s_uselock;
         while (*dx_tmp) {
            if ( !isalnum((int)*dx_tmp) && *dx_tmp != '_'
         && *dx_tmp != '@' && *dx_tmp != '-' 
         && *dx_tmp != '+')
               *dx_tmp = 'X';
            dx_tmp++;
         }
         hk_ap2 = atr_str(s_uselock);
         mudstate.chkcpu_stopper = time(NULL);
         mudstate.chkcpu_toggle = 0;
         mudstate.chkcpu_locktog = 0;
         hk_retval = process_hook(player, mudconf.hook_obj, s_uselock, hk_ap2, 1);
         mudstate.chkcpu_toggle = 0;
         mudstate.chkcpu_locktog = 0;
         free_sbuf(s_uselock);
         if ( !hk_retval && (cmdp->hookmask & HOOK_IGNORE) )
            cval = 2;
         else
            cval = !hk_retval;
    }
    if ( (cmdp != NULL) && !((cmdp->perms & CA_IGNORE_MASK) && 
        igcheck(player,cmdp->perms)) && (cval != 2) && (cval2 != 2) ) {
        mudstate.chkcpu_stopper = time(NULL);
        mudstate.chkcpu_toggle = 0;
        mudstate.chkcpu_locktog = 0;
        if ( cval2 > 0 )
           cmdp->perms |= CA_DISABLED;
    process_cmdent(cmdp, slashp, player, cause, interactive, arg,
               command, args, nargs, cval);
        if ( mudstate.chkcpu_toggle ) {
           if ( mudstate.curr_cpu_user != player ) {
              mudstate.curr_cpu_user = player;
              mudstate.curr_cpu_cycle = 0;
           }
           mudstate.curr_cpu_cycle++;
           notify(player, "Overload on CPU detected.  Process aborted.");
           if ( mudstate.chkcpu_locktog )
              broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (LOCK-EVAL)",
                                 (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           else
              broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS",
                                 (char *)lst_cmd, NULL, cause, 0, 0, NULL);
       cpulbuf = alloc_lbuf("process_commands.LOG.badcpu");
           STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
       log_name_and_loc(player);
           sprintf(cpulbuf, " CPU overload%s %.3900s", (mudstate.chkcpu_locktog ? " (lock-eval):" : ":"), lst_cmd);
           log_text(cpulbuf);
           free_lbuf(cpulbuf);
           ENDLOG
           if ( mudstate.curr_cpu_cycle >= mudconf.max_cpu_cycles ) {
          cpulbuf = alloc_lbuf("process_commands.LOG.badcpu_toomany");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = NOTHING;
              if (mudconf.cpu_secure_lvl == 1 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted #%d%s", 
                         mudconf.max_cpu_cycles, (boot_plr == -1 ? player : boot_plr),
                         (mudstate.chkcpu_locktog ? " and SILENTEFFECTED]" : "]") );
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL1 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if (mudconf.cpu_secure_lvl == 2 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted%s FUBARED #%d]", 
                         mudconf.max_cpu_cycles, 
                         (mudstate.chkcpu_locktog ? ", SILENTEFFECTED and" : " and"), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]", 
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL2 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if (mudconf.cpu_secure_lvl == 3 ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.  Goodbye.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    s_Flags3(boot_plr, (Flags3(boot_plr) | NOCONNECT));
                    if ( isPlayer(boot_plr) )
                       boot_off(boot_plr, NULL);
                    sprintf(cpulbuf, 
                           "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, and @booted #%d]",
                            mudconf.max_cpu_cycles, 
                            (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, " : ", "), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]", 
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL3 SECURITY",
                              (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else if ( (mudconf.cpu_secure_lvl == 4) || (mudconf.cpu_secure_lvl == 5) ) {
                 notify(player, "Excessive CPU slamming detected.  HALTED.");
                 if ( Good_obj(boot_plr) ) {
                    notify(boot_plr, "You (or your object) were detected consistantly CPU-SLAMMING the mush.  " \
                                     "Proper steps have been taken.  Goodbye.");
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
                    free_lbuf(tpr_buff);
                    s_Flags2(boot_plr, (Flags2(boot_plr) | FUBAR));
                    s_Flags3(boot_plr, (Flags3(boot_plr) | NOCONNECT));
                    if ( isPlayer(boot_plr) ) {
                       tchbuff = alloc_mbuf("cpu_regsite");
                       DESC_ITER_CONN(d) {
                          if ( d->player == boot_plr ) {
                            if ( !(site_check(d->address.sin_addr, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION) ) {
                               sprintf(tchbuff, "%s 255.255.255.255", inet_ntoa(d->address.sin_addr));
                               if ( mudconf.cpu_secure_lvl == 4 )
                                  cf_site((int *)&mudstate.access_list, tchbuff,
                                          (H_REGISTRATION | H_AUTOSITE), 0, 1, "register_site");
                               else
                                  cf_site((int *)&mudstate.access_list, tchbuff,
                                          (H_FORBIDDEN | H_AUTOSITE), 0, 1, "forbid_site");
                            }
                          }
                       }
                       boot_off(boot_plr, NULL);
                       free_mbuf(tchbuff);
                    }
                    if ( mudconf.cpu_secure_lvl == 4 )
                       sprintf(cpulbuf, 
                              "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, @booted & RegSited #%d]",
                               mudconf.max_cpu_cycles, 
                               (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, " : ", "), boot_plr);
                    else
                       sprintf(cpulbuf, 
                              "MULTICPU RUNAWAY [%d times/@halted%s FUBARED, NOCONNECTED, @booted & ForbidSited #%d]",
                               mudconf.max_cpu_cycles,
                               (mudstate.chkcpu_locktog ? ", SILENTEFFECTED, " : ", "), boot_plr);
                 } else {
                    tprp_buff = tpr_buff = alloc_lbuf("process_command");
                    do_halt(player, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", player));
                    free_lbuf(tpr_buff);
                    sprintf(cpulbuf, "MULTICPU RUNAWAY [%d times/@halted only]",
                         mudconf.max_cpu_cycles);
                 }
                 s_Flags(player, (Flags(player) | HALT));
                 s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
                 if ( mudstate.chkcpu_locktog )
                    s_Toggles2(player, (Toggles2(player) | TOG_SILENTEFFECTS));
                 broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - LEVEL4+ SECURITY",
                                          (char *)cpulbuf, NULL, cause, 0, 0, NULL);
                 STARTLOG(LOG_ALWAYS, "WIZ", "CPUMX");
                 log_name_and_loc(player);
                 log_text(" ");
                 log_text(cpulbuf);
                 ENDLOG
              } else {
                 /* We do nothing extra */
              }
              free_lbuf(cpulbuf);
              mudstate.last_cpu_user = mudstate.curr_cpu_user;
              mudstate.curr_cpu_user = NOTHING;
              mudstate.curr_cpu_cycle = 0;
           }
    }
        mudstate.chkcpu_toggle = 0;
        mudstate.chkcpu_locktog = 0;
    if ( mudstate.stack_toggle ) {
           cpulbuf = alloc_lbuf("stack_limit");
           notify(player, "Stack limit exceeded.  Process aborted.");
           sprintf(cpulbuf, "STACK LIMIT EXCEEDED [%d of 3]: (#%d) %.3900s", mudstate.stack_cntr, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "STACK CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "STACK");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           free_lbuf(cpulbuf);
           if ( time_ng(NULL) > mudstate.cntr_reset ) {
              mudstate.stack_cntr = 0;
              mudstate.cntr_reset = (time_ng(NULL) + 60);
           }
           mudstate.stack_cntr++;
           if ( mudstate.stack_cntr >= 3 ) {
              broadcast_monitor(player, MF_CPUEXT, "MULTIPLE STACK OVERFLOWS", (char *) "[3/@halted]", NULL, cause, 0, 0, NULL);
              notify(player, "Stack violation with recursive calls.  You've been halted.");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = player;
              tprp_buff = tpr_buff = alloc_lbuf("process_command");
              do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
              free_lbuf(tpr_buff);
              s_Flags(player, (Flags(player) | HALT));
              s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
              mudstate.breakst = 1;
              mudstate.stack_cntr = 0;
           }
    } else {
           mudstate.stack_cntr = 0;
        }
        if ( mudstate.sidefx_toggle ) {
           cpulbuf = alloc_lbuf("stack_limit");
           notify(player, "SideFX limit exceeded.  Process aborted.");
           sprintf(cpulbuf, "SideFX LIMIT EXCEEDED [%d]: (#%d) %.3900s", mudconf.sidefx_maxcalls, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "SIDEFX CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "SIDEFX");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           free_lbuf(cpulbuf);
           mudstate.sidefx_toggle = 0;
        }
        if ( mudstate.tog_percentsubs ) {
           cpulbuf = alloc_lbuf("percent_sub_limit");
           notify(player, "Percent Substitutions exceeded.  Process aborted.");
           sprintf(cpulbuf, "Percent Substitutions LIMIT EXCEEDED [%d]: (#%d) %.3900s", 
                   mudconf.max_percentsubs, cause, (char *)lst_cmd);
           broadcast_monitor(player, MF_CPU, "PERCENT-SUB CEILING REACHED",
                                          (char *)lst_cmd, NULL, cause, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "P-SUB");
           log_name_and_loc(player);
           log_text(" ");
           log_text(cpulbuf);
           ENDLOG
           mudstate.breakst = 1;
           free_lbuf(cpulbuf);
           if ( time_ng(NULL) > mudstate.cntr_reset ) {
              mudstate.cntr_percentsubs = 0;
              mudstate.cntr_reset = (time_ng(NULL) + 60);
           }
           mudstate.cntr_percentsubs++;
           if ( mudstate.cntr_percentsubs >= 3 ) {
              broadcast_monitor(player, MF_CPUEXT, "MULTIPLE PERCENT-SUB OVERFLOWS", (char *) "[3/@halted]", NULL, cause, 0, 0, NULL);
              notify(player, "Percent-sub violation with recursive calls.  You've been halted.");
              notify(cause, "Percent-sub violation with recursive calls.  You've been halted.");
              if ( Good_obj(Owner(player)) )
                 boot_plr = Owner(player);
              else
                 boot_plr = player;
              tprp_buff = tpr_buff = alloc_lbuf("process_command");
              do_halt(boot_plr, cause, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", boot_plr));
              free_lbuf(tpr_buff);
              s_Flags(player, (Flags(player) | HALT));
              s_Toggles(player, (Toggles(player) & ~TOG_FORCEHALTED));
              mudstate.tog_percentsubs = 0;
           }
        } else {
           mudstate.tog_percentsubs = 0;
        }
        free_lbuf(lcbuf);

        getitimer(ITIMER_PROF, &itimer);
        reportcputime(player, &itimer);
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = 0;
        setitimer(ITIMER_PROF, &itimer, NULL);
    mudstate.debug_cmd = cmdsave;
        DPOP; /* #29 */
    return;
    }

    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    setitimer(ITIMER_PROF, &itimer, NULL);

    /* Check for enter and leave aliases, user-defined commands on the
     * player, other objects where the player is, on objects in the
     * player's inventory, and on the room that holds the player.  We
     * evaluate the command line here to allow chains of $-commands to
     * work. */

    cval = cval2 = 0;
    free_lbuf(lcbuf);
    lcbuf = exec(player, cause, cause, EV_EVAL | EV_FCHECK | EV_STRIP | EV_TOP, command,
         args, nargs, (char **)NULL, 0);
    succ = 0;

    /* Idea for enter/leave aliases from R'nice@TinyTIM */

    if (Has_location(player) && Good_obj(Location(player))) {

    /* Check for a leave alias */

	p = atr_pget(Location(player), A_LALIAS, &aowner, &aflags);
	if (p && *p) {
	    if (matches_exit_from_list(lcbuf, p)) {
		free_lbuf(lcbuf);
		free_lbuf(p);
		if (Flags3(player) & NOMOVE)
		  notify(player, "Permission denied.");
                else if (mudstate.remotep != NOTHING)
		  notify(player, "Permission denied.");
		else
		  do_leave(player, player, 0);
                getitimer(ITIMER_PROF, &itimer);
                reportcputime(player, &itimer);
                itimer.it_value.tv_sec = 0;
                itimer.it_value.tv_usec = 0;
                setitimer(ITIMER_PROF, &itimer, NULL);
                DPOP; /* #29 */
		return;
	    }
	}
	free_lbuf(p);

	/* Check for enter aliases */

	DOLIST(pcexit, Contents(Location(player))) {
	    p = atr_pget(pcexit, A_EALIAS, &aowner, &aflags);
	    if (p && *p) {
		if (matches_exit_from_list(lcbuf, p)) {
		    free_lbuf(lcbuf);
		    free_lbuf(p);
		    if (Flags3(player) & NOMOVE)
		      notify(player, "Permission denied.");
                    else if (mudstate.remotep != NOTHING)
		      notify(player, "Permission denied.");
		    else
		      do_enter_internal(player, pcexit, 0);
                    getitimer(ITIMER_PROF, &itimer);
                    reportcputime(player, &itimer);
                    itimer.it_value.tv_sec = 0;
                    itimer.it_value.tv_usec = 0;
                    setitimer(ITIMER_PROF, &itimer, NULL);
                    DPOP; /* #29 */
            return;
        }
        }
        free_lbuf(p);
    }
    }

    /* (Else) end of IGNORE_EXIT - This needs to be here */
    } else {
       strcpy(lcbuf, lcbuf_temp);
       free_lbuf(lcbuf_temp);
    } /* IGNORE_EXIT */
    
    if (!DePriv(player, NOTHING, DP_COMMAND, POWER6, POWER_LEVEL_NA)) {
    /* Check for $-command matches on me */

    sflag = 0;
    if (mudconf.match_mine) {
        if (((Typeof(player) != TYPE_PLAYER) ||
         (mudconf.match_mine_pl && mudconf.match_pl)) &&
        ((sflag = atr_match(player, player, AMATCH_CMD, lcbuf, 1, 0)) > 0)) {
        succ++;
        }
    }
    /* Check for $-command matches on nearby things and on my room */

    if (Has_location(player) && Good_obj(Location(player)) && (sflag < 2)) {
        sflag = list_check(Contents(Location(player)), player,
                   AMATCH_CMD, lcbuf, 1, 1, 0);
        succ += sflag;
        if (sflag < 2) {
          if ((sflag = atr_match(Location(player), player,
              AMATCH_CMD, lcbuf, 1, 1)) > 0) {
        succ++;
          }
        }
    }
    /* Check for $-command matches in my inventory */

    if (Has_contents(player) && (sflag < 2))
        succ += list_check(Contents(player), player,
                   AMATCH_CMD, lcbuf, 1, 1, 0);

        /* check for zone master commands */
        if( !succ && (sflag < 2) ) {
          realloc = absloc(player);
          if( Good_obj(realloc) && db[realloc].zonelist && 
              !ZoneMaster(realloc)) {
              for( zonelistnodeptr = db[realloc].zonelist;
                   zonelistnodeptr; 
                   zonelistnodeptr = zonelistnodeptr->next ) {
            if ((sflag = atr_match(zonelistnodeptr->object, player,
              AMATCH_CMD, lcbuf, 1, 1)) > 0) {
          succ++;
          if (sflag > 1)
            break;
                }
        /* check for contents of zone masters */
        if ( !succ && Good_obj(zonelistnodeptr->object) && 
             ZoneContents(zonelistnodeptr->object) ) {
             succ += list_check(Contents(zonelistnodeptr->object),
                        player, AMATCH_CMD, lcbuf, 0, 0, 0);
            }
          }
          }
        }
 
        /* Check for object/player $command matching for zonechecks if enabled */
        if ( !succ && mudconf.zones_like_parents && (sflag < 2) ) {
           /* Check player */
           if (mudconf.match_mine) {
          if ( ZoneCmdChk(player) && ((Typeof(player) != TYPE_PLAYER) ||
                   (mudconf.match_mine_pl && mudconf.match_pl)) ) {
                 if( db[player].zonelist && !ZoneMaster(player)) {
                    for( zonelistnodeptr = db[player].zonelist;
                         zonelistnodeptr; 
                         zonelistnodeptr = zonelistnodeptr->next ) {
                   if ((sflag = atr_match(zonelistnodeptr->object, player,
                AMATCH_CMD, lcbuf, 1, 1)) > 0) {
                  succ++;
                  if (sflag > 1)
                     break;
                       }
               /* check for contents of zone masters */
               if ( !succ && Good_obj(zonelistnodeptr->object) && 
                    ZoneContents(zonelistnodeptr->object) ) {
                    succ += list_check(Contents(zonelistnodeptr->object),
                               player, AMATCH_CMD, lcbuf, 0, 0, 0);
                   }
                 }
                 }
              }
           }
           /* Check Location */
       if ( !succ && Has_location(player) && Good_obj(Location(player)) && (sflag < 2) ) {
          sflag = list_check(Contents(Location(player)), player, AMATCH_CMD, lcbuf, 1, 1, 1);
          succ += sflag;
       }
           /* Check Inventory */
       if ( !succ && Has_contents(player) && (sflag < 2) ) {
           succ += list_check(Contents(player), player, AMATCH_CMD, lcbuf, 1, 1, 1);
           }
	}

	/* If we didn't find anything, try in the master room */

	if (!DePriv(player, NOTHING, DP_MASTER, POWER6, POWER_LEVEL_NA)) {
	    if (!succ && (sflag < 2) ) {
		if (Good_obj(mudconf.master_room) &&
		    Has_contents(mudconf.master_room)) {
		    succ += list_check(Contents(mudconf.master_room),
				       player, AMATCH_CMD, lcbuf, 0, 0, 0);
		    if (atr_match(mudconf.master_room,
				  player, AMATCH_CMD, lcbuf, 0, 0) > 0) {
			succ++;
		    }
		}
	    }
	 }
    }
    free_lbuf(lcbuf);

    /* If we still didn't find anything, tell how to get help. */

    if (!succ && (sflag < 2) ) {
        if ( Good_obj(mudconf.global_error_obj) && !Recover(mudconf.global_error_obj) &&
             !Going(mudconf.global_error_obj) ) {
            if ( *lst_cmd ) {
               memset(arr_prog, 0, sizeof(arr_prog));
               if ( InProgram(player) ) {
                  narg_prog = newlist2arr(arr_prog, LBUF_SIZE/2, lst_cmd+1, '\0');
               } else {
                  narg_prog = newlist2arr(arr_prog, LBUF_SIZE/2, lst_cmd, '\0');
               }
               lcbuf = atr_get(mudconf.global_error_obj, A_VA, &aowner2, &aflags2);
               mudstate.nocodeoverride = 1;
               i_trace = 0;
               i_trace = mudstate.notrace;
               mudstate.notrace = 1;
               lcbuf_temp = exec(mudconf.global_error_obj, cause, cause, 
                                 EV_EVAL | EV_FCHECK | EV_STRIP | EV_TOP, lcbuf,
                                 arr_prog, narg_prog, (char **)NULL, 0);
               mudstate.notrace = i_trace;
               mudstate.nocodeoverride = 0;
               notify(player, lcbuf_temp);
               free_lbuf(lcbuf);
               free_lbuf(lcbuf_temp);
           }
        } else {
       notify(player, errmsg(player));
       STARTLOG(LOG_BADCOMMANDS, "CMD", "BAD")
           log_name_and_loc(player);
       lcbuf = alloc_lbuf("process_commands.LOG.badcmd");
           /* Fix buffering so log can't be overflowed and blowup the lbuf */
           if ( strlen(command) > 3900 )
              sprintf(lcbuf, " entered: '%.3900s [overflow cut]'", command);
           else
          sprintf(lcbuf, " entered: '%s'", command);
       log_text(lcbuf);
       free_lbuf(lcbuf);
       ENDLOG
        }
    }
    getitimer(ITIMER_PROF, &itimer);
    reportcputime(player, &itimer);
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = 0;
    setitimer(ITIMER_PROF, &itimer, NULL);
    mudstate.debug_cmd = cmdsave;
    DPOP; /* #29 */
    return;
}

/* ---------------------------------------------------------------------------
 * powers_nametab: Extended powers table.
 * IMPORTANT - Keep this table in sync with the POW_xxx #defines in externs.h
 */

NAMETAB powers_nametab[] =
{
    {(char *) "change_quotas", 3, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "chown_anywhere", 6, CA_WIZARD, 0, CA_GOD},
    {(char *) "chown_mine", 7, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "chown_others", 7, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "chown_players", 7, CA_WIZARD, 0, CA_GOD},
    {(char *) "control_global", 2, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "expanded_who", 3, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "examine_global", 3, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "find_unfindable", 2, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "free_money", 6, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "free_quota", 6, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "grab_player", 1, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "ignore_rst_flags", 1, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "join_player", 1, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "long_fingers", 1, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "no_boot", 4, CA_WIZARD, 0, CA_GOD},
    {(char *) "no_toad", 4, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "rename_myself", 1, CA_WIZARD, 0, CA_NO_GUEST | CA_NO_SLAVE},
    {(char *) "see_admin_flags", 6, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "see_all_queue", 6, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "see_hidden", 5, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {(char *) "see_inviz_attrs", 5, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "see_maint_flags", 5, CA_WIZARD, 0, CA_GOD},
    {(char *) "set_admin_attrs", 11, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "set_admin_flags", 11, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "set_maint_attrs", 11, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "set_maint_flags", 11, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "stat_any", 3, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "steal_money", 3, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "tel_anywhere", 6, CA_WIZARD, 0, CA_WIZARD},
    {(char *) "tel_unrestricted", 6, CA_WIZARD, 0, CA_GOD},
    {(char *) "unkillable", 1, CA_WIZARD, 0, CA_WIZARD | CA_IMMORTAL},
    {NULL, 0, 0, 0, 0}};


/* ---------------------------------------------------------------------------
 * list_allpower: List internal commands.
 */
static void
list_allpower(dbref player, int key)
{
  POWENT *tp;
  char lev1[16], lev2[16], *tpr_buff, *tprp_buff;

  DPUSH; /* #30 */
  if ( (key && !Wizard(player)) || (!key && 
       (!Wizard(player) || DePriv(player,NOTHING,DP_POWER,POWER7,POWER_LEVEL_NA))) ) {
    notify(player,errmsg(player));
  } else {
     memset(lev1, 0, sizeof(lev1));
     memset(lev2, 0, sizeof(lev2));
     if ( key )
       tp = depow_table;
     else
       tp = pow_table;
     while ((tp->powername) && ((tp+1)->powername)) {
       switch (tp->powerlev) {
           case POWER_LEVEL_OFF:   strcpy(lev1,"Off"); break;
           case POWER_LEVEL_GUILD: strcpy(lev1,"Guildmaster"); break;
           case POWER_LEVEL_ARCH:  strcpy(lev1,"Architect"); break;
           case POWER_LEVEL_COUNC: strcpy(lev1,"Councilor"); break;
           case POWER_REMOVE:      strcpy(lev1,"Disabled"); break;
           default:        strcpy(lev1,"N/A");
       }
       switch ((tp+1)->powerlev) {
           case POWER_LEVEL_OFF:   strcpy(lev2,"Off"); break;
           case POWER_LEVEL_GUILD: strcpy(lev2,"Guildmaster"); break;
           case POWER_LEVEL_ARCH:  strcpy(lev2,"Architect"); break;
           case POWER_LEVEL_COUNC: strcpy(lev2,"Councilor"); break;
           case POWER_REMOVE:      strcpy(lev2,"Disabled"); break;
           default:        strcpy(lev2,"N/A");
       }
       tprp_buff = tpr_buff = alloc_lbuf("list_allpower");
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s         %-20s %-10s",
                      tp->powername,lev1,(tp+1)->powername,lev2));
       free_lbuf(tpr_buff);
       tp += 2;
     }
     if (tp->powername) {
       switch (tp->powerlev) {
           case POWER_LEVEL_OFF:   strcpy(lev1,"Off"); break;
           case POWER_LEVEL_GUILD: strcpy(lev1,"Guildmaster"); break;
           case POWER_LEVEL_ARCH:  strcpy(lev1,"Architect"); break;
           case POWER_LEVEL_COUNC: strcpy(lev1,"Councilor"); break;
           case POWER_REMOVE:      strcpy(lev1,"Disabled"); break;
           default:        strcpy(lev1,"N/A");
       }
       tprp_buff = tpr_buff = alloc_lbuf("list_allpower");
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s",tp->powername,lev1));
       free_lbuf(tpr_buff);
     }
  }
  DPOP; /* #30 */
}
/* ---------------------------------------------------------------------------
 * list_cmdtable: List internal commands.
 */

/* This will blow up if all the commands are greater than an LBUF! */
/* Right now it's a small fraction of an LBUF (1000 chars max) */
static int s_comp(const void *s1, const void *s2)
{
  return strcmp(*(char **) s1, *(char **) s2);
}

static void list_cmdtable(dbref player) {
  CMDENT *cmdp;
 const char *ptrs[LBUF_SIZE / 2];
  char *buff;  
  char *bp;
  int nptrs = 0, i;

  DPUSH; /* #31 */

  buff = alloc_lbuf("list_cmdtable");
  bp = buff;
  *bp = '\0';

  safe_str("Commands: ", buff, &bp);
  for (cmdp = (CMDENT *) hash_firstentry(&mudstate.command_htab);
       cmdp;
       cmdp = (CMDENT *) hash_nextentry(&mudstate.command_htab)) {

    if ((cmdp->cmdtype & CMD_BUILTIN_e || cmdp->cmdtype & CMD_LOCAL_e) 
    && check_access(player, cmdp->perms, cmdp->perms2, 0)) {
      if ((!strcmp(cmdp->cmdname, "@snoop") || !strcmp(cmdp->cmdname, "@depower")) && !Immortal(player))
    continue;
      if (!(cmdp->perms & CF_DARK)) {   
    ptrs[nptrs] = cmdp->cmdname;
    nptrs++;
      }
    }
  }

  qsort(ptrs, nptrs, sizeof(CMDENT *), s_comp);

  safe_str((char *)ptrs[0], buff, &bp);
  for (i = 1; i < nptrs; i++) {
    safe_chr(' ', buff, &bp);
    safe_str((char *)ptrs[i], buff, &bp);
  }
  *bp = '\0';
  notify(player, buff);
  free_lbuf(buff);
  VOIDRETURN;  /* #31 */
}

/* ---------------------------------------------------------------------------
 * list_attrtable: List available attributes.
 */
/* This will blow up if all the commands are greater than an LBUF! */
/* Right now it's a small fraction of an LBUF (1000 chars max) */

static void 
list_attrtable(dbref player)
{
    ATTR *ap;
    char *buf, *bp, *cp;

    DPUSH; /* #32 */

    buf = alloc_lbuf("list_attrtable");
    bp = buf;
    for (cp = (char *) "Attributes:"; *cp; cp++)
    *bp++ = *cp;
    for (ap = attr; ap->name; ap++) {
    if (See_attr(player, player, ap, player, 0, 0)) {
        *bp++ = ' ';
        for (cp = (char *) (ap->name); *cp; cp++)
        *bp++ = *cp;
    }
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
    DPOP; /* #32 */
}

/* ---------------------------------------------------------------------------
 * list_cmdaccess: List access commands.
 */

NAMETAB access_nametab[] =
{
    {(char *) "god", 2, CA_GOD, 0, CA_GOD},
    {(char *) "immortal", 3, CA_IMMORTAL, 0, CA_IMMORTAL},
    {(char *) "royalty", 3, CA_IMMORTAL, 0, CA_WIZARD},
    {(char *) "wizard", 3, CA_IMMORTAL, 0, CA_WIZARD},
    {(char *) "councilor", 4, CA_WIZARD, 0, CA_ADMIN},
    {(char *) "architect", 4, CA_WIZARD, 0, CA_BUILDER},
    {(char *) "guildmaster", 5, CA_WIZARD, 0, CA_GUILDMASTER},
    {(char *) "robot", 2, CA_WIZARD, 0, CA_ROBOT},
    {(char *) "no_haven", 4, CA_PUBLIC, 0, CA_NO_HAVEN},
    {(char *) "no_robot", 4, CA_WIZARD, 0, CA_NO_ROBOT},
    {(char *) "no_slave", 5, CA_PUBLIC, 0, CA_NO_SLAVE},
    {(char *) "no_suspect", 5, CA_WIZARD, 0, CA_NO_SUSPECT},
    {(char *) "no_guest", 5, CA_WIZARD, 0, CA_NO_GUEST},
    {(char *) "no_wanderer", 5, CA_WIZARD, 0, CA_NO_WANDER},
    {(char *) "global_build", 8, CA_PUBLIC, 0, CA_GBL_BUILD},
    {(char *) "global_interp", 8, CA_PUBLIC, 0, CA_GBL_INTERP},
    {(char *) "disabled", 4, CA_IMMORTAL, 0, CA_DISABLED},
    {(char *) "need_location", 6, CA_PUBLIC, 0, CA_LOCATION},
    {(char *) "need_contents", 9, CA_PUBLIC, 0, CA_CONTENTS},
    {(char *) "need_player", 6, CA_PUBLIC, 0, CA_PLAYER},
    {(char *) "dark", 4, CA_IMMORTAL, 0, CF_DARK},
    {(char *) "ignore", 6, CA_IMMORTAL, 0, CA_IGNORE},
    {(char *) "ignore_mortal", 8, CA_IMMORTAL, 0, CA_IGNORE_MORTAL},
    {(char *) "ignore_gm", 8, CA_IMMORTAL, 0, CA_IGNORE_GM},
    {(char *) "ignore_arch", 8, CA_IMMORTAL, 0, CA_IGNORE_ARCH},
    {(char *) "ignore_counc", 8, CA_IMMORTAL, 0, CA_IGNORE_COUNC},
    {(char *) "ignore_royal", 8, CA_IMMORTAL, 0, CA_IGNORE_ROYAL},
    {(char *) "ignore_immortal", 8, CA_GOD, 0, CA_IGNORE_IM},
    {(char *) "ignore_zone", 8, CA_IMMORTAL, 0, CA_IGNORE_ZONE},
    {(char *) "disable_zone", 8, CA_IMMORTAL, 0, CA_DISABLE_ZONE},
    {(char *) "logflag", 3, CA_IMMORTAL, 0, CA_LOGFLAG},
    {NULL, 0, 0, 0, 0}};

/* Second permission level of access for commands/functions/other */
NAMETAB access_nametab2[] =
{
    {(char *) "no_code", 5, CA_WIZARD, 0, CA_NO_CODE},
    {(char *) "no_eval", 5, CA_WIZARD, 0, CA_NO_EVAL},
    {(char *) "eval", 5, CA_WIZARD, 0, CA_EVAL},
    {NULL, 0, 0, 0, 0}};

static void 
list_cmdaccess(dbref player, char *s_mask, int key)
{
    char *buff, *p, *q;
    CMDENT *cmdp;
    ATTR *ap;

    DPUSH; /* #34 */

    if ( key ) {
       notify(player, "--- Command Access (wildmatched) ---");
    } else
       notify(player, "--- Command Access ---");
    buff = alloc_sbuf("list_cmdaccess");
    for (cmdp = command_table; cmdp->cmdname; cmdp++) {
    if (check_access(player, cmdp->perms, cmdp->perms2, 0)) {
        if (!(cmdp->perms & CF_DARK)) {
                if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)cmdp->cmdname)) ) {
           sprintf(buff, "%.30s:", cmdp->cmdname);
              listset_nametab(player, access_nametab, access_nametab2,
                   cmdp->perms, cmdp->perms2, buff, 1);
                }
        }
    }
    }
    for (ap = attr; ap->name; ap++) {
    p = buff;
    *p++ = '@';
    for (q = (char *) ap->name; *q; p++, q++)
        *p = ToLower((int)*q);
    if (ap->flags & AF_NOCMD)
        continue;
    *p = '\0';
    cmdp = lookup_orig_command(buff);
    if (cmdp == NULL)
        continue;
    if (!check_access(player, cmdp->perms, cmdp->perms2, 0))
        continue;
    if (!(cmdp->perms & CF_DARK)) {
            if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)cmdp->cmdname)) ) {
           sprintf(buff, "%.30s:", cmdp->cmdname);
           listset_nametab(player, access_nametab, access_nametab2,
                   cmdp->perms, cmdp->perms2, buff, 1);
            }
    }
    }
    free_sbuf(buff);
    DPOP; /* #34 */
}

/* ---------------------------------------------------------------------------
 * list_cmdswitches: List switches for commands.
 */

static void 
list_cmdswitches(dbref player, char *s_mask, int key)
{
    char *buff;
    CMDENT *cmdp;

    DPUSH; /* #35 */

    if ( key ) {
       notify(player, "--- Command Switches (wildmatched) ---");
    } else
       notify(player, "--- Command Switches ---");
    buff = alloc_sbuf("list_cmdswitches");
    for (cmdp = command_table; cmdp->cmdname; cmdp++) {
    if (cmdp->switches) {
        if (check_access(player, cmdp->perms, cmdp->perms2, 0)) {
        if (!(cmdp->perms & CF_DARK)) {
                    if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)cmdp->cmdname)) ) {
               sprintf(buff, "%.31s:", cmdp->cmdname);
               display_nametab(player, cmdp->switches,
                       buff, 0);
                    }
        }
        }
    }
    }
    free_sbuf(buff);
    DPOP; /* #35 */
}

/* ---------------------------------------------------------------------------
 * list_attraccess: List access to attributes.
 */

NAMETAB attraccess_nametab[] =
{
    {(char *) "dark", 2, CA_GOD, 0, AF_DARK},
    {(char *) "deleted", 2, CA_IMMORTAL, 0, AF_DELETED},
    {(char *) "god", 2, CA_IMMORTAL, 0, AF_GOD},
    {(char *) "hidden", 1, CA_WIZARD, 0, AF_MDARK},
    {(char *) "ignore", 2, CA_WIZARD, 0, AF_NOCMD},
    {(char *) "internal", 2, CA_IMMORTAL, 0, AF_INTERNAL},
    {(char *) "is_lock", 4, CA_PUBLIC, 0, AF_IS_LOCK},
    {(char *) "locked", 1, CA_PUBLIC, 0, AF_LOCK},
    {(char *) "no_command", 4, CA_PUBLIC, 0, AF_NOPROG},
    {(char *) "no_inherit", 4, CA_PUBLIC, 0, AF_PRIVATE},
    {(char *) "private", 2, CA_PUBLIC, 0, AF_ODARK},
    {(char *) "councilor", 4, CA_ADMIN, 0, AF_ADMIN},
    {(char *) "architect", 4, CA_BUILDER, 0, AF_BUILDER},
    {(char *) "royalty", 1, CA_WIZARD, 0, AF_WIZARD},
    {(char *) "wizard", 1, CA_WIZARD, 0, AF_WIZARD},
    {(char *) "immortal", 3, CA_IMMORTAL, 0, AF_IMMORTAL},
    {(char *) "guildmaster", 2, CA_GUILDMASTER, 0, AF_GUILDMASTER},
    {(char *) "visual", 1, CA_PUBLIC, 0, AF_VISUAL},
    {(char *) "no_ansi", 4, CA_WIZARD, 0, AF_NOANSI},
    {(char *) "nonblocking", 3, CA_GOD, 0, AF_NONBLOCKING},
    {(char *) "pinvisible", 2, CA_WIZARD, 0, AF_PINVIS},
    {(char *) "no_clone", 4, CA_PUBLIC, 0, AF_NOCLONE},
#ifdef ATTR_HACK
    {(char *) "uselock", 3, CA_PUBLIC, 0, AF_USELOCK},
#endif
    {(char *) "singlethread", 3, CA_PUBLIC, 0, AF_SINGLETHREAD},
    {(char *) "default", 3, CA_PUBLIC, 0, AF_DEFAULT},
    {(char *) "atrlock", 3, CA_WIZARD, 0, AF_ATRLOCK},
    {(char *) "logged", 3, CA_WIZARD, 0, AF_LOGGED},
    {(char *) "regexp", 3, CA_PUBLIC, 0, AF_REGEXP},
    {(char *) "unsafe", 3, CA_WIZARD, 0, AF_UNSAFE},
    {NULL, 0, 0, 0, 0}};

NAMETAB indiv_attraccess_nametab[] =
{
    {(char *) "no_command", 4, CA_PUBLIC, 0, AF_NOPROG},
    {(char *) "no_inherit", 4, CA_PUBLIC, 0, AF_PRIVATE},
    {(char *) "visual", 1, CA_PUBLIC, 0, AF_VISUAL},
    {(char *) "pinvisible", 2, CA_WIZARD, 0, AF_PINVIS},
    {(char *) "no_clone", 4, CA_PUBLIC, 0, AF_NOCLONE},
    {(char *) "no_parse", 4, CA_PUBLIC, 0, AF_NOPARSE},
    {(char *) "safe", 2, CA_PUBLIC, 0, AF_SAFE},
    {(char *) "hidden", 2, CA_WIZARD, 0, AF_MDARK},
    {(char *) "guildmaster", 2, CA_GUILDMASTER, 0, AF_GUILDMASTER},
    {(char *) "architect", 4, CA_BUILDER, 0, AF_BUILDER},
    {(char *) "councilor", 4, CA_ADMIN, 0, AF_ADMIN},
    {(char *) "royalty", 3, CA_WIZARD, 0, AF_WIZARD},
    {(char *) "wizard", 3, CA_WIZARD, 0, AF_WIZARD},
    {(char *) "immortal", 3, CA_IMMORTAL, 0, AF_IMMORTAL},
    {(char *) "god", 2, CA_GOD, 0, AF_GOD}, 
    {(char *) "dark", 2, CA_GOD, 0, AF_DARK},
#ifdef ATTR_HACK
    {(char *) "uselock", 3, CA_PUBLIC, 0, AF_USELOCK},
#endif
    {(char *) "singlethread", 3, CA_PUBLIC, 0, AF_SINGLETHREAD},
    {(char *) "default", 3, CA_PUBLIC, 0, AF_DEFAULT},
    {(char *) "atrlock", 3, CA_WIZARD, 0, AF_ATRLOCK},
    {(char *) "logged", 3, CA_WIZARD, 0, AF_LOGGED},
    {(char *) "regexp", 3, CA_PUBLIC, 0, AF_REGEXP},
    {(char *) "unsafe", 3, CA_WIZARD, 0, AF_UNSAFE},
    {NULL, 0, 0, 0, 0}};

static void 
list_attraccess(dbref player, char *s_mask, int key)
{
    char *buff;
    ATTR *ap;

    DPUSH; /* #36 */

   if ( key ) {
       notify(player, "--- Attribute Permissions (wildmatched) ---");
    } else
       notify(player, "--- Attribute Permissions ---");

    buff = alloc_lbuf("list_attraccess");
    for (ap = attr; ap->name; ap++) {
    if (Read_attr(player, player, ap, player, 0, 0)) {
            if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)ap->name)) ) {
           sprintf(buff, "%s:", ap->name);
           listset_nametab(player, attraccess_nametab, 0,
                   ap->flags, 0, buff, 1);
        }
    }
    }
    free_lbuf(buff);

    DPOP; /* #36 */
}

/* ---------------------------------------------------------------------------
 * cf_access: Change command or switch permissions.
 */

extern CF_HDCL(cf_modify_bits);
extern void FDECL(cf_log_notfound, (dbref, char *, const char *, char *));

CF_HAND(cf_access)
{
    CMDENT *cmdp;
    char *ap;
    int set_switch;
    int retval;

    DPUSH; /* #37 */

    for (ap = str; *ap && !isspace((int)*ap) && (*ap != '/'); ap++);
    if (*ap == '/') {
    set_switch = 1;
    *ap++ = '\0';
    } else {
    set_switch = 0;
    if (*ap)
        *ap++ = '\0';
    while (*ap && isspace((int)*ap))
        ap++;
    }

    cmdp = lookup_orig_command(str);
    if (cmdp != NULL) {
    if (set_switch) {
            retval = cf_ntab_access((int *) cmdp->switches, ap,
                  extra, extra2, player, cmd);
            DPOP; /* #37 */
            return retval;
        }
    else {
        retval = cf_modify_multibits((int *)&(cmdp->perms), (int *)&(cmdp->perms2), ap,
                       extra, extra2, player, cmd);
            DPOP; /* #37 */
            return retval;
        }
    } else {
        if ( !stricmp(str, "home") ) {
           retval = cf_modify_multibits(&(mudconf.restrict_home), &(mudconf.restrict_home2), ap,
                                      extra, extra2, player, cmd);
           DPOP; /* #37 */
           return retval;
        } else {
       cf_log_notfound(player, cmd, "Command", str);
           DPOP; /* #37 */
       return -1;
        }
    }
    DPOP; /* #37 */
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_acmd_access: Chante command permissions for all attr-setting cmds.
 */

CF_HAND(cf_acmd_access)
{
    CMDENT *cmdp;
    ATTR *ap;
    char *buff, *p, *q;
    int failure, save;

    DPUSH; /* #38 */

    buff = alloc_sbuf("cf_acmd_access");
    for (ap = attr; ap->name; ap++) {
    p = buff;
    *p++ = '@';
    for (q = (char *) ap->name; *q; p++, q++)
        *p = ToLower((int)*q);
    *p = '\0';
    cmdp = lookup_orig_command(buff);
    if (cmdp != NULL) {
        save = cmdp->perms;
        failure = cf_modify_multibits((int *)&(cmdp->perms), (int *)&(cmdp->perms2), str,
                        extra, extra2, player, cmd);
        if (failure != 0) {
        cmdp->perms = save;
        free_sbuf(buff);
                DPOP; /* #38 */
        return -1;
        }
    }
    }
    free_sbuf(buff);
    DPOP; /* #38 */
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_attr_access: Change access on an attribute.
 */

CF_HAND(cf_attr_access)
{
    ATTR *ap;
    char *sp;
    int retval;
    char *lbuff;

    DPUSH; /* #39 */

    for (sp = str; *sp && !isspace((int)*sp); sp++);
    if (*sp)
    *sp++ = '\0';
    while (*sp && isspace((int)*sp))
    sp++;

    ap = atr_str(str);
    if (ap != NULL) {
        if ( *sp != '\0' )
       retval = cf_modify_bits(&ap->flags, sp, extra, extra2, player, cmd);
        else
           retval = -3;
    if (!mudstate.initializing && !Quiet(player)) {
        lbuff = alloc_lbuf("cf_attr_access");
        sprintf(lbuff, "Access for %s:",
            ap->name);
        listset_nametab(player, attraccess_nametab, 0,
                ap->flags, 0, lbuff, 1);
        free_lbuf(lbuff); 
            DPOP; /* #39 */
        return retval;
    } 
    } else {
    cf_log_notfound(player, cmd, "Attribute", str);
        DPOP; /* #39 */
    return -1;
    } 
    DPOP; /* #39 */
    return 0;
}

/* ---------------------------------------------------------------------------
 * cf_cmd_alias: Add a command alias.
 *
 * Lensy Note: If you delete an aliased command you MUST delete it from the
 *   alias table. Not doing so will almost certainly result in core dumps.
 *   This is because the alias struct contains a pointer to the real command
 *   and will have no way of knowing that the memory it points to is no longer
 *   valid.
 */

CF_HAND(cf_cmd_alias)
{
    char *alias, *orig, *ap, *p, *tpr_buff, *tprp_buff;

    CMDENT *cmdp, *cp;
    ALIASENT *aliasp, *alias2p;

    NAMETAB *nt = NULL;
    int retval;
    int bReplaceAlias = 0, bHasSwitches = 0;
    int key = 0, perms = 0, perms2 = 0;
    DPUSH; /* #40 */

    alias = strtok(str, " \t=,");
    orig = strtok(NULL, " \t=,");
    

    retval = 0;
    if (!alias) {       /* we didnt' get any arguments to @alias.  Very Bad. */
       if ( !mudstate.initializing )
          notify(player, "Error - you need to pass in at least the alias");
       DPOP; /* #40 */
       return -1;
    }

    /* If only one argument, then we want to return the alias info */
    if (!orig){
      cp = lookup_command(alias);
      if (!cp) { 
        if ( !mudstate.initializing ) {
           tprp_buff = tpr_buff = alloc_lbuf("cf_cmd_alias");
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Alias '%s' not found.", alias));
           free_lbuf(tpr_buff);
        }
    retval = -1;      
      } else if (strcmp(alias, cp->cmdname) == 0) {
        if ( !mudstate.initializing ) {
           tprp_buff = tpr_buff = alloc_lbuf("cf_cmd_alias");
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "'%s' is a built-in command.", alias));
           free_lbuf(tpr_buff);
        }
    retval = -3;
      } else {
    /* We know it's not an internal command at this point */
    aliasp = (ALIASENT *) hashfind(alias, &mudstate.cmd_alias_htab);
    if (!aliasp) {
      /* Hmm, this really shouldn't be happening.. */
          if ( !mudstate.initializing ) {
             tprp_buff = tpr_buff = alloc_lbuf("cf_cmd_alias");
         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Alias '%s' not found.", alias));
             free_lbuf(tpr_buff);
          }
      retval = -1;
    } else {
      if (aliasp->extra) {
        char *switches, *bp;
        bp = switches = alloc_lbuf("cf_cmd_alias switches");
        for (nt = (NAMETAB *)cp->switches; nt->name; nt++) { 
              if ( ((nt->flag & ~0xF0000000) == 0) && ((aliasp->extra & ~0xF0000000) == 0) ) {
                if ( nt->flag & aliasp->extra ) {
          safe_chr('/', switches, &bp);
          safe_str(nt->name, switches, &bp);
                }
              } else {
                if ( nt->flag & (aliasp->extra & ~0xF0000000) ) {
          safe_chr('/', switches, &bp);
          safe_str(nt->name, switches, &bp);
        }
              }
        }
            if ( !mudstate.initializing ) {
               tprp_buff = tpr_buff = alloc_lbuf("cf_cmd_alias");
           notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Alias '%s' points to '%s%s'.", 
                                           alias, cp->cmdname, (switches && *switches) ? switches : "/???"));
               free_lbuf(tpr_buff);
            }
        retval = -3;
        free_lbuf(switches);
      } else {
            if ( !mudstate.initializing ) {
               tprp_buff = tpr_buff = alloc_lbuf("cf_cmd_alias");
           notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Alias '%s' points to '%s'.", alias, cp->cmdname));         
               free_lbuf(tpr_buff);
            }
        retval = -3;
      }
    }
      }
      DPOP; /* #40 */
      return retval;
    }


    if (!strcmp(orig, "!")) {
      alias2p = (ALIASENT *) hashfind(alias, &mudstate.cmd_alias_htab);
      if (!alias2p) {
    cf_log_syntax(player, cmd, "Error: Cannot delete non-existant alias '%s'.", alias);       
        DPOP; /* #40 */
    return -1;
      } else {
    cf_log_syntax(player, cmd, "Warning: Deleting alias '%s'.", alias);
    hashdelete(alias, &mudstate.cmd_alias_htab);
      }
      DPOP; /* #40 */
      return 0;
    }

    for (ap = orig; *ap && (*ap != '/'); ap++);

    /* We need to filter out the switches here so
     * the command lookup will work
     */
    if (*ap == '/') {
      bHasSwitches++;
      *ap++ = '\0';      
    }

    /* 1. Lookup the original command */
    cmdp = (CMDENT *) hashfind(orig, &mudstate.command_htab);
    if (cmdp == NULL) {
      cf_log_notfound(player, cmd, "Command", orig);
      DPOP; /* #40 */
      return -1;
    }   

    /* 2. Make sure there's no conflict on the alias */
    if (hashfind(alias, &mudstate.command_htab)) {
      /* Can't override a command with an alias */
      cf_log_syntax(player, cmd, "Alias '%s' conflicts with internal command", alias);
      DPOP; /* #40 */
      return -1;
    }

    if ((alias2p = (ALIASENT *) hashfind(alias, &mudstate.cmd_alias_htab))) {
      bReplaceAlias = 1;
      cf_log_syntax(player, cmd, "Warning: Redefining alias '%s'.", alias);
    }

    /* 3. We're going to make sure the switches are valid
     * and then just store the string for later usage.
     */
    if (bHasSwitches && *ap != '\0') {
      int breakLoop = 0;
      p = ap;
      while (!breakLoop) {
    ap++;
    if (*ap == '/' || *ap == '\0') {
      if (*ap == '\0') {
        breakLoop = 1;
      } else {
        *ap = '\0';
      }
      nt = find_nametab_ent(player, (NAMETAB *) cmdp->switches, p);
      if (!nt) {
        cf_log_notfound(player, cmd, "Switch", p);
            DPOP; /* #40 */
        return -1;
      } else {
        key |= nt->flag;
        perms |= nt->perm;
            perms2 |= nt->perm2;
      }
      p = ap+1;     
    }
      }
      key = (!(nt->flag & SW_MULTIPLE)) ? key | SW_GOT_UNIQUE : key & ~SW_MULTIPLE;
      
    }


    aliasp = malloc(sizeof(ALIASENT));
    if (!aliasp) {
      fprintf(stderr, "(cf_cmd_alias) Out of memory!\n");
      DPOP; /* #40 */
      exit(-1);
    }
    aliasp->aliasname = strsave(alias);
    aliasp->extra = key;
    aliasp->perms = perms;
    aliasp->perms2 = perms2;
    aliasp->cmdp = cmdp;
    if (bReplaceAlias && alias2p) {
      hashrepl(alias, (int *) aliasp, &mudstate.cmd_alias_htab);
    } else {
      hashadd(alias, (int *) aliasp, &mudstate.cmd_alias_htab);
    }
    
    DPOP; /* #40 */
    return retval;
}


static void 
list_df_toggles(dbref player)
{
    char *buff, *s_buff;
    DPUSH; // #41-a

    notify(player, "Default Toggle Listing -- All Types.");
    buff = alloc_lbuf("list_df_toggles");
    s_buff = toggle_description(player, player, 0, 0, (int *)&mudconf.player_toggles);
    sprintf(buff, "------------------------------------------------------------------------------\nPlayer Toggles:");
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = toggle_description(player, player, 0, 0, (int *)&mudconf.room_toggles);
    sprintf(buff, "------------------------------------------------------------------------------\nRoom Toggles:");
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = toggle_description(player, player, 0, 0, (int *)&mudconf.exit_toggles);
    sprintf(buff, "------------------------------------------------------------------------------\nExit Toggles:");
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = toggle_description(player, player, 0, 0, (int *)&mudconf.thing_toggles);
    sprintf(buff, "------------------------------------------------------------------------------\nThing Toggles:");
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = toggle_description(player, player, 0, 0, (int *)&mudconf.robot_toggles);
    sprintf(buff, "------------------------------------------------------------------------------\nRobot Toggles:");
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);

    free_lbuf(buff);

    DPOP; // #41-a
}

static void 
list_df_flags(dbref player)
{
    char *playerb, *roomb, *thingb, *exitb, *robotb, *buff, *s_buff;
    DPUSH; // #41

    playerb = decode_flags(player, player, 
               (mudconf.player_flags.word1 | TYPE_PLAYER),
               mudconf.player_flags.word2,
               mudconf.player_flags.word3,
               mudconf.player_flags.word4
    );
    roomb = decode_flags(player, player, 
             (mudconf.room_flags.word1 | TYPE_ROOM),
             mudconf.room_flags.word2,
             mudconf.room_flags.word3,
             mudconf.room_flags.word4
    );
    exitb = decode_flags(player, player, 
             (mudconf.exit_flags.word1 | TYPE_EXIT),
             mudconf.exit_flags.word2,
             mudconf.exit_flags.word3,
             mudconf.exit_flags.word4
    );
    thingb = decode_flags(player, player, 
              (mudconf.thing_flags.word1 | TYPE_THING),
              mudconf.thing_flags.word2,
              mudconf.thing_flags.word3,
              mudconf.thing_flags.word4
    );
    robotb = decode_flags(player, player,
              (mudconf.robot_flags.word1 | TYPE_PLAYER),
              mudconf.robot_flags.word2,
              mudconf.robot_flags.word3,
              mudconf.robot_flags.word4
    );
    notify(player, "Default Flag Listing -- All Types.  (note: THING type letter is a space)");
    buff = alloc_lbuf("list_df_flags");
    s_buff = flag_description(player, player, 0, (int *)&mudconf.player_flags, TYPE_PLAYER);
    sprintf(buff, "------------------------------------------------------------------------------\nPlayer Flags: %s", playerb);
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = flag_description(player, player, 0, (int *)&mudconf.room_flags, TYPE_ROOM);
    sprintf(buff, "------------------------------------------------------------------------------\nRoom Flags: %s", roomb);
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = flag_description(player, player, 0, (int *)&mudconf.exit_flags, TYPE_EXIT);
    sprintf(buff, "------------------------------------------------------------------------------\nExit Flags: %s", exitb);
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = flag_description(player, player, 0, (int *)&mudconf.thing_flags, TYPE_THING);
    sprintf(buff, "------------------------------------------------------------------------------\nThing Flags: %s", thingb);
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);
    s_buff = flag_description(player, player, 0, (int *)&mudconf.robot_flags, TYPE_PLAYER);
    sprintf(buff, "------------------------------------------------------------------------------\nRobot Flags: %s", robotb);
    notify(player, buff);
    notify(player, s_buff);
    free_lbuf(s_buff);

    free_lbuf(buff);
    free_mbuf(playerb);
    free_mbuf(roomb);
    free_mbuf(exitb);
    free_mbuf(thingb);
    free_mbuf(robotb);

    DPOP; // #41
}

/* ---------------------------------------------------------------------------
 * list_costs: List the costs of things.
 */

#define coin_name(s)    (((s)==1) ? mudconf.one_coin : mudconf.many_coins)

static void 
list_costs(dbref player)
{
    char *buff;

    DPUSH; /* #42 */

    buff = alloc_mbuf("list_costs");
    *buff = '\0';
    if (mudconf.quotas)
    sprintf(buff, " and %d quota", mudconf.room_quota);
    notify(player,
       unsafe_tprintf("Digging a room costs %d %s%s.",
           mudconf.digcost, coin_name(mudconf.digcost), buff));
    if (mudconf.quotas)
    sprintf(buff, " and %d quota", mudconf.exit_quota);
    notify(player,
       unsafe_tprintf("Opening a new exit costs %d %s%s.",
           mudconf.opencost, coin_name(mudconf.opencost), buff));
    notify(player,
       unsafe_tprintf("Linking an exit, home, or dropto costs %d %s.",
           mudconf.linkcost, coin_name(mudconf.linkcost)));
    if (mudconf.quotas)
    sprintf(buff, " and %d quota", mudconf.thing_quota);
    if (mudconf.createmin == mudconf.createmax)
    notify(player,
           unsafe_tprintf("Creating a new thing costs %d %d %s%s.",
               mudconf.createmin,
               coin_name(mudconf.createmin), buff));
    else
    notify(player,
           unsafe_tprintf("Creating a new thing costs between %d and %d %s%s.",
               mudconf.createmin, mudconf.createmax,
               mudconf.many_coins, buff));
    if (mudconf.quotas)
    sprintf(buff, " and %d quota", mudconf.player_quota);
    notify(player,
       unsafe_tprintf("Creating a robot costs %d %s%s.",
           mudconf.robotcost, coin_name(mudconf.robotcost),
           buff));
    if (mudconf.killmin == mudconf.killmax) {
    notify(player,
           unsafe_tprintf("Killing costs %d %s, with a %d%% chance of success.",
               mudconf.killmin, coin_name(mudconf.digcost),
               (mudconf.killmin * 100) /
               mudconf.killguarantee));
    } else {
    notify(player,
           unsafe_tprintf("Killing costs between %d and %d %s.",
               mudconf.killmin, mudconf.killmax,
               mudconf.many_coins));
    notify(player,
           unsafe_tprintf("You must spend %d %s to guarantee success.",
               mudconf.killguarantee,
               coin_name(mudconf.killguarantee)));
    }
    notify(player,
       unsafe_tprintf("Computationally expensive commands and functions (ie: @entrances, @find, @search, @stats (with an argument or switch), search(), and stats()) cost %d %s.",
           mudconf.searchcost, coin_name(mudconf.searchcost)));
    if (mudconf.machinecost > 0)
    notify(player,
           unsafe_tprintf("Each command run from the queue costs 1/%d %s.",
               mudconf.machinecost, mudconf.one_coin));
    if (mudconf.waitcost > 0) {
    notify(player,
           unsafe_tprintf("A %d %s deposit is charged for putting a command on the queue.",
               mudconf.waitcost, mudconf.one_coin));
    notify(player, "The deposit is refunded when the command is run or canceled.");
    }
    if (mudconf.sacfactor == 0)
    sprintf(buff, "%d", mudconf.sacadjust);
    else if (mudconf.sacfactor == 1) {
    if (mudconf.sacadjust < 0)
        sprintf(buff, "<create cost> - %d", -mudconf.sacadjust);
    else if (mudconf.sacadjust > 0)
        sprintf(buff, "<create cost> + %d", mudconf.sacadjust);
    else
        sprintf(buff, "<create cost>");
    } else {
    if (mudconf.sacadjust < 0)
        sprintf(buff, "(<create cost> / %d) - %d",
            mudconf.sacfactor, -mudconf.sacadjust);
    else if (mudconf.sacadjust > 0)
        sprintf(buff, "(<create cost> / %d) + %d",
            mudconf.sacfactor, mudconf.sacadjust);
    else
        sprintf(buff, "<create cost> / %d", mudconf.sacfactor);
    }
    notify(player, unsafe_tprintf("The value of an object is %s.", buff));
    if (mudconf.clone_copy_cost)
    notify(player, "The default value of cloned objects is the value of the original object.");
    else
    notify(player,
           unsafe_tprintf("The default value of cloned objects is %d %s.",
               mudconf.createmin,
               coin_name(mudconf.createmin)));
    notify(player, unsafe_tprintf("The cost for a @wall is %d",mudconf.wall_cost));
    notify(player, unsafe_tprintf("The cost for a page is %d",mudconf.pagecost));
#ifndef SOFTCOM
    notify(player, unsafe_tprintf("The cost for a com message is %d",mudconf.comcost));
#endif

    free_mbuf(buff);

    DPOP; /* #42 */
}

/* ---------------------------------------------------------------------------
 * list_options: List more game options from mudconf.
 */

static const char *switchd[] =
{"/first", "/all"};
static const char *examd[] =
{"/brief", "/full"};
static const char *ed[] =
{"Disabled", "Enabled"};
static const char *ud[] =
{"Down", "Up"};

static void
list_options_mail(dbref player)
{
   DPUSH; /* #43 */
   notify(player, unsafe_tprintf("%-28.28s %c %s",
                  "mail_default",
                  (mudconf.mail_default ? 'Y' : 'N'),
                  "Does 'mail' default to 'mail/status'?"));
   notify(player, unsafe_tprintf("%-28.28s %c %s",
                  "mail_tolist",
                  (mudconf.mail_tolist ? 'Y' : 'N'),
                  "Does 'mail' include 'To:' automatically?"));
   notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sDefault: 99   Value: %d",
                  "mailbox_size",
                  "Current default size of mailbox.", " ",
                  mudconf.mailbox_size));
   notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sDefault: 20   Value: %d",
                  "mailmax_fold",
                  "Current maximum mail folders allowed.", " ",
                  mudconf.mailmax_fold));
   notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sDefault: 100  Value: %d",
                  "mailmax_send",
                  "Current maximum players allowed in send.", " ",
                  mudconf.mailmax_send));
   notify(player, unsafe_tprintf("%-28.28s %c %s",
                  "maildelete",
                  (mudconf.maildelete ? 'Y' : 'N'),
                  "Is mail set up for auto-deleting?"));
   notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sDefault: 21   Value: %d",
                  "mail_autodeltime",
                  "Time (in days) that mail is autodeleted. (-1 unlimited)", " ",
                  mudconf.mail_autodeltime));
   notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sAnonymous Mail Name: %s",
                  "mail_anonymous",
                  "Name used for Anonymous (/anon) mail.", " ",
                  mudconf.mail_anonymous));
   notify(player, unsafe_tprintf("%-28.28s %c %s",
          "bcc_hidden",
          (mudconf.bcc_hidden ? 'Y' : 'N'),
                  "Does +bcc blind target from seeing 'To' list?"));

   if (Wizard(player)) {
      if ( Good_obj(mudconf.mail_def_object) ) {
         notify(player, unsafe_tprintf("%-28.28s   %s", "mail_def_object", 
                        "Valid dynamic mail object defined."));
         notify(player, unsafe_tprintf("%31sGlobal mail object is %s(#%d).", " ", 
                        Name(mudconf.mail_def_object), mudconf.mail_def_object));
      } else {
         notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sValue: -1", 
                        "mail_def_object",
                        "No dynamic mail object defined.", " "));
      }
      notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sEmail Program: %s",
                     "mailprog",
                     "Mail program used with autoregistration.", " ",
                     mudconf.mailprog));
      notify(player, unsafe_tprintf("%-28.28s %c %s",
                     "mailsub",
                     (mudconf.mailsub ? 'Y' : 'N'),
                     "Does the mail program use subjects?"));
      notify(player, unsafe_tprintf("%-28.28s   %s\r\n%31sFile included: %s",
                     "mailinclude_file",
                     "File included with autoregistration.", " ",
                     mudconf.mailinclude_file));
      notify(player, unsafe_tprintf("%-28.28s %c %s",
                     "mail_verbosity",
                     (mudconf.mail_verbosity ? 'Y' : 'N'),
                     "Mail show subj. on send and to discon'd players?"));
      notify(player, unsafe_tprintf("%-28.28s %c %s",
                     "mail_hidden",
                     (mudconf.mail_hidden ? 'Y' : 'N'),
                     "Anonymous mail show player mail sent to?"));
  }
  DPOP; /* #43 */
}

static void
list_options_boolean_parse(dbref player, int p_val, char *s_val)
{
   DPUSH; /* #44 */
   if (p_val < 1)
      p_val = 1;
   list_options_boolean(player, p_val, s_val);
   DPOP; /* #44 */
}

static void
list_options_values_parse(dbref player, int p_val, char *s_val)
{
   DPUSH; /* #45 */
   if (p_val < 1)
      p_val = 1;
   list_options_values(player, p_val, s_val);
   DPOP; /* #45 */
}

static void
list_options_mysql(dbref player)
{
#ifdef MYSQL_VERSION
   char *tbuf;

   tbuf = alloc_lbuf("list_options_mysql");
   sprintf(tbuf, "Host: %s\r\nUser: %s\r\nPassword: %s\r\nDatabase: %s\r\nSocket: %s\r\nPort: %d", 
                  mudconf.mysql_host,
                  mudconf.mysql_user,
                  (Immortal(player) ? mudconf.mysql_pass : (char *)"****"),
                  mudconf.mysql_base,
                  ( (stricmp(mudconf.mysql_socket, "NULL") == 0) ? "(NULL) [default socket]" : mudconf.mysql_socket),
                  mudconf.mysql_port);
   notify(player, tbuf);
   free_lbuf(tbuf);
#else
   notify(player, "MySQL is not currently enabled.");
#endif
}

static void
list_options_convtime(dbref player)
{
   FILE *fp;
   char *s_line;

   if ( (fp = fopen((char *)"../game/convtime.template", "r")) == NULL ) {
      notify(player, "There is no convtime.template file in your game directory.");
   } else {
      s_line = alloc_lbuf("list_options_convtime");
      notify(player, "Supported Date Formats [~/game/convtime.template] ----------------------------");
      while ( !feof(fp) ) {
         fgets(s_line, (LBUF_SIZE-2), fp);
         if ( !feof(fp) && (*s_line != '#') ) {
            if ( (strlen(s_line) > 1) && (s_line[strlen(s_line)-2] == '\r') )
               s_line[strlen(s_line)-2] = '\0';
            else if ( (strlen(s_line) > 0) && (s_line[strlen(s_line)-1] == '\n') )
               s_line[strlen(s_line)-1] = '\0';
            notify(player, s_line);
         }
      }
      fclose(fp);
      notify(player, "------------------------------------------------------------------------------");
      notify(player, "Key: %a/%A - Weekday Name (Www), %b/%B - Month Name (Mmm), %D - %m/%d/%y");
      notify(player, "     %m    - month (01-12),      %d    - day (01-31),      %Y - year (yyyy)");
      notify(player, "     %H    - hour (00-23),       %M    - minute (00-59),   %S - second (00-59)");
      notify(player, "------------------------------------------------------------------------------");
      free_lbuf(s_line);
   }
}

static void
list_options_system(dbref player)
{
   char buf2[64], buf3[64], buf4[64], dbdumptime[25], dbchktime[25], playerchktime[125],
        newstime[25], mailtime[25], aregtime[25], mushtime[25];
   time_t c_count, d_count, i_count;
#ifdef MYSQL_VERSION
   char *tbuf;
#endif

   memset(playerchktime, '\0', 125);
/* "------------------------------------------------------------------" */
   notify(player, "--- System Player Config Parameters ------------------------------------------");
#ifdef TINY_SUB
   sprintf(playerchktime, 
           "%cx ------------------------------------------------------------- %s",
           '%', "ANSI");
#else
   sprintf(playerchktime, 
           "%cx ------------------------------------------------------------- %s",
           '%', "LASTCMD");
#endif
   notify(player, playerchktime);
#ifdef C_SUB
   sprintf(playerchktime, 
           "%cc ------------------------------------------------------------- %s",
           '%', "ANSI");
#else
   sprintf(playerchktime, 
           "%cc ------------------------------------------------------------- %s",
           '%', "LASTCMD");
#endif
   notify(player, playerchktime);
#ifdef M_SUB
   sprintf(playerchktime, 
           "%cm ------------------------------------------------------------- %s",
           '%', "ANSI");
#else
   sprintf(playerchktime, 
           "%cm ------------------------------------------------------------- %s",
           '%', "LASTCMD");
#endif
   notify(player, playerchktime);

   memset(playerchktime, '\0', 125);
#ifdef TINY_U
    notify(player, "u()/zfun() ----------------------------------------------------- PENN/MUX/TM3");
#else
    notify(player, "u()/zfun() ----------------------------------------------------- RHOST NATIVE");
#endif
#ifdef MUX_INCDEC
    notify(player, "inc()/dec() ---------------------------------------------------- NUMERIC");
    notify(player, "xinc()/xdec() -------------------------------------------------- REGISTERS");
#else
    notify(player, "inc()/dec() ---------------------------------------------------- REGISTERS");
    notify(player, "xinc()/xdec() -------------------------------------------------- NUMERIC");
#endif
#ifdef USE_SIDEEFFECT
    notify(player, "Sideeffects [SIDEFX required] ---------------------------------- ENABLED");
#else
    notify(player, "Sideeffects [SIDEFX required] ---------------------------------- DISABLED");
#endif
    if ( mudconf.enhanced_convtime ) {
       notify(player, "Enhanced convtime() formats --[ see @list options convtime ]---- ENABLED");
    } else {
       notify(player, "Enhanced convtime() formats ------------------------------------ DISABLED");
    }
#ifdef ENABLE_COMMAND_FLAG
    notify(player, "The COMMAND flag ----------------------------------------------- ENABLED");
#else
    notify(player, "The COMMAND flag ----------------------------------------------- DISABLED");
#endif
#ifdef EXPANDED_QREGS
    notify(player, "A-Z setq registers --------------------------------------------- ENABLED");
#else
    notify(player, "A-Z setq registers --------------------------------------------- DISABLED");
#endif
#ifdef ATTR_HACK
    if ( mudconf.hackattr_see == 0 ) {
       notify(player, "Attributes starting with _ and ~ ------------------------------- WIZARD ONLY");
    } else {
       notify(player, "Attributes starting with _ and ~ ------------------------------- ENABLED");
    }
#else
    notify(player, "Attributes starting with _ and ~ ------------------------------- DISABLED");
#endif
#ifdef BANGS
    notify(player, "Bang notation [!/!!, !$/!!$, !^/!!^] --------------------------- ENABLED");
#else
    notify(player, "Bang notation [!/!!, !$/!!$, !^/!!^] --------------------------- DISABLED");
#endif
#ifdef QDBM
    notify(player, "Database Engine ------------------------------------------------ QDBM");
#else
#ifdef BIT64
    notify(player, "Database Engine ------------------------------------------------ GDBM [64Bit]");
#else
    notify(player, "Database Engine ------------------------------------------------ GDBM [32Bit]");
#endif
#endif
#ifdef SQLITE
    notify(player, "SQLite --------------------------------------------------------- ENABLED");
#else
    notify(player, "SQLite --------------------------------------------------------- DISABLED");
#endif
#ifdef MYSQL_VERSION
    tbuf = alloc_mbuf("list_option_system");
    sprintf(tbuf, "[%.40s, SQL: %.40s]--------------------------------------", MYSQL_VER, MYSQL_VERSION);
    notify(player, unsafe_tprintf("MySQL/MariaDB ------%-42.42s-- ENABLED", tbuf));
    free_mbuf(tbuf);
#else
    notify(player, "MySQL/MariaDB -------------------------------------------------- DISABLED");
#endif
    if ( mudconf.ansi_default ) {
       notify(player, "ANSI handler for functions() ----------------------------------- ENABLED");
       notify(player, "     Functions Affected: TR(), BEFORE(), AFTER(), MID(), DELETE()");
    } else {
       notify(player, "ANSI handler for functions() ----------------------------------- DISABLED");
    }
    notify(player, unsafe_tprintf(
                       "Floating point precision --------------------------------------- %d DECIMALS",
                       mudconf.float_precision));
    if ( mudconf.accent_extend ) {
       notify(player, "ASCII 250-255 encoding ----------------------------------------- ENABLED");
    } else {
       notify(player, "ASCII 250-255 encoding ----------------------------------------- DISABLED");
    }
#ifdef CRYPT_GLIB2
    notify(player, "Player password encryption method ------------------------------ SHA512");
#else
    notify(player, "Player password encryption method ------------------------------ DES");
#endif
    notify(player, unsafe_tprintf("Current TREE character is defined as --------------------------- %s", mudconf.tree_character));
    if (mudconf.parent_control)
       notify(player, "Lock Control for all @lock processing -------------------------- PARENTS");
    else
       notify(player, "Lock Control for all @lock processing -------------------------- LOCAL");
    if ( mudconf.name_with_desc )
       notify(player, "Name shown before descs when looking at non-owned things ------- ENABLED");
    else
       notify(player, "Name shown before descs when looking at non-owned things ------- DISABLED");
    if ( mudconf.penn_setq )
       notify(player, "Setq/Setr use PennMUSH compatiability mode --------------------- ENABLED");
    else
       notify(player, "Setq/Setr use PennMUSH compatiability mode --------------------- DISABLED");
    if ( mudconf.format_compatibility )
       notify(player, "Attribute formatting compatibility (&<name>FORMAT) ------------- FORMAT AFTER");
    else
       notify(player, "Attribute formatting non-compatibility (&FORMAT<name>) --------- FORMAT FIRST");
    if ( mudconf.delim_null )
       notify(player, "@@ recognized as null output seperator ------------------------- ENABLED");
    else
       notify(player, "@@ recognized as null output seperator ------------------------- DISABLED");

    notify(player, "\r\n--- Buffer Sizes and Limits --------------------------------------------------");
    notify(player, unsafe_tprintf("The current BUFFER sizes in use are: SBUF: %d, MBUF: %d, LBUF: %d", 
                              SBUF_SIZE, MBUF_SIZE, LBUF_SIZE));
    notify(player, unsafe_tprintf("Maximum attribs per object [VLIMIT] is: %d", mudconf.vlimit));
    notify(player, unsafe_tprintf("Maximum attribs per page of lattr output is: %d", ((LBUF_SIZE - (LBUF_SIZE/20)) / SBUF_SIZE)));

    if ( Guildmaster(player) ) {
       c_count = (time_t)floor(mudstate.check_counter);
       d_count = (time_t)floor(mudstate.dump_counter);
       i_count = (time_t)floor(mudstate.idle_counter);
       memset(dbdumptime, '\0', 25);
       memset(dbchktime, '\0', 25);
       strncpy(dbchktime,(char *) ctime(&c_count), 24);
       strncpy(dbdumptime,(char *) ctime(&d_count), 24);
       strncpy(playerchktime,(char *) ctime(&i_count), 24);
       notify(player, "\r\n--- System Timers and Triggers -----------------------------------------------");
       notify(player, unsafe_tprintf("--> Next DB Check: %s [%s to trigger]\r\n--> Next DB Dump: %s [%s to trigger]\r\n--> Next Idle User Check: %s [%s to trigger]\r\n",
                      dbchktime, time_format_2(c_count - mudstate.now, buf2),
                      dbdumptime, time_format_2(d_count - mudstate.now, buf3),
                      playerchktime, time_format_2(i_count - mudstate.now, buf4)));

       memset(newstime, 0, sizeof(newstime));
       memset(mailtime, 0, sizeof(mailtime));
       memset(aregtime, 0, sizeof(aregtime));
       memset(mushtime, 0, sizeof(mushtime));
       strncpy(newstime,(char *) ctime(&mudstate.newsflat_time), 24);
       strncpy(mailtime,(char *) ctime(&mudstate.mailflat_time), 24);
       strncpy(aregtime,(char *) ctime(&mudstate.aregflat_time), 24);
       strncpy(mushtime,(char *) ctime(&mudstate.mushflat_time), 24);
       notify(player, "--- System DB Saves and DB Dumps ---------------------------------------------");
       notify(player, unsafe_tprintf("--> Mush: %s\r\n--> Mail: %s\r\n--> News: %s\r\n--> Areg: %s",
                      (mudstate.start_time == mudstate.mushflat_time  ? "[No Previous Dump]      " :
                            (char *) mushtime ),
                      (mudstate.mail_state == 2 ? "[Mail system disabled]  " :
                            (mudstate.start_time == mudstate.mailflat_time  ? "[No Previous Dump]      " : (char *) mailtime )),
                      (news_system_active == 0 ? "[News system disabled]  " :
                            (mudstate.start_time == mudstate.newsflat_time ? "[No Previous Dump]      " : (char *) newstime )),
                      (mudstate.start_time == mudstate.aregflat_time ? "[No Previous Dump]      " :
                            (char *) aregtime )));
    }
    notify(player, "------------------------------------------------------------------------------");
}

static void
list_options_config(dbref player)
{
    char *buff, *strbuff, *strptr, *tbuff1ptr, *tbuff1, *tbuff2ptr, *tbuff2, *buff3;
    int b1, b2, b3, b4, b5;
    double now;

    DPUSH; /* #46 */
    b1 = b2 = b3 = b4 = b5 = 0;
    now = time_ng(NULL);
    buff = alloc_mbuf("list_options");
    memset(buff, 0, MBUF_SIZE);
#ifdef HAS_OPENSSL
    notify(player, "OpenSSL supported digests have been enabled.");
#else
    notify(player, "OpenSSL was not found.  SHA digest supported only.");
#endif
#ifdef ATTR_HACK
    notify(player, "You may use ~ATTRS for uselocks and the USELOCK attribute flag.");
#else
    notify(player, "You can not use ~ATTRS or the USELOCK attribute flag.");
#endif
    if ( mudconf.zone_parents )
       notify(player, "Zone children inherent attributes from their zone master(s).");
#ifdef EXPANDED_QREGS
    notify(player, "SETQ/SETR/R can handle a-z registers in addition to 0-9.");
#else
    notify(player, "SETQ/SETR/R can only handle 0-9 registers.");
#endif
    if ( mudconf.look_moreflags )
       notify(player, "Look/etc is expanded to show global attribute flags inside []'s.");
    notify(player, unsafe_tprintf("Parent Defaults: Room: #%d  Exit: #%d  Player: #%d  Thing: #%d",
           (Good_obj(mudconf.room_parent) && !Recover(mudconf.room_parent)) ? 
               mudconf.room_parent : -1,
           (Good_obj(mudconf.exit_parent) && !Recover(mudconf.exit_parent)) ? 
               mudconf.exit_parent : -1,
           (Good_obj(mudconf.player_parent) && !Recover(mudconf.player_parent)) ?
               mudconf.player_parent : -1,
           (Good_obj(mudconf.thing_parent) && !Recover(mudconf.thing_parent)) ?
               mudconf.thing_parent : -1) );
    notify(player, unsafe_tprintf("Attribute Defaults: Room: #%d  Exit: #%d  Player: #%d  Thing: #%d",
           (Good_obj(mudconf.room_defobj) && !Recover(mudconf.room_defobj)) ? 
               mudconf.room_defobj : -1,
           (Good_obj(mudconf.exit_defobj) && !Recover(mudconf.exit_defobj)) ? 
               mudconf.exit_defobj : -1,
           (Good_obj(mudconf.player_defobj) && !Recover(mudconf.player_defobj)) ?
               mudconf.player_defobj : -1,
           (Good_obj(mudconf.thing_defobj) && !Recover(mudconf.thing_defobj)) ?
               mudconf.thing_defobj : -1) );
    if ( Good_obj(mudconf.global_parent_obj) &&
         !Recover(mudconf.global_parent_obj) &&
         !Going(mudconf.global_parent_obj) ) {
       b1 = 1;
    }
    if ( Good_obj(mudconf.global_parent_exit) &&
         !Recover(mudconf.global_parent_exit) &&
         !Going(mudconf.global_parent_exit) ) {
       b2 = 1;
    }
    if ( Good_obj(mudconf.global_parent_room) &&
         !Recover(mudconf.global_parent_room) &&
         !Going(mudconf.global_parent_room) ) {
       b3 = 1;
    }
    if ( Good_obj(mudconf.global_parent_player) &&
         !Recover(mudconf.global_parent_player) &&
         !Going(mudconf.global_parent_player) ) {
       b4 = 1;
    }
    if ( Good_obj(mudconf.global_parent_thing) &&
         !Recover(mudconf.global_parent_thing) &&
         !Going(mudconf.global_parent_thing) ) {
       b5 = 1;
    }
    notify(player, "Global parent object system statistics:");
    notify(player, unsafe_tprintf("     Global Generic Definition: %s",
                   (b1 ? "Yes" : "No")));
    notify(player, unsafe_tprintf("     Room: %s    Exit: %s    Thing: %s    Player: %s",
                   (b3 ? "Yes" : "No"), (b2 ? "Yes" : "No"),
                   (b5 ? "Yes" : "No"), (b4 ? "Yes" : "No")));
    b1 = b2 = b3 = b4 = b5 = 0;
    if ( Good_obj(mudconf.global_clone_obj) &&
         !Recover(mudconf.global_clone_obj) &&
         !Going(mudconf.global_clone_obj) ) {
       b1 = 1;
    }
    if ( Good_obj(mudconf.global_clone_exit) &&
         !Recover(mudconf.global_clone_exit) &&
         !Going(mudconf.global_clone_exit) ) {
       b2 = 1;
    }
    if ( Good_obj(mudconf.global_clone_room) &&
         !Recover(mudconf.global_clone_room) &&
         !Going(mudconf.global_clone_room) ) {
       b3 = 1;
    }
    if ( Good_obj(mudconf.global_clone_player) &&
         !Recover(mudconf.global_clone_player) &&
         !Going(mudconf.global_clone_player) ) {
       b4 = 1;
    }
    if ( Good_obj(mudconf.global_clone_thing) &&
         !Recover(mudconf.global_clone_thing) &&
         !Going(mudconf.global_clone_thing) ) {
       b5 = 1;
    }
    notify(player, "Global clone object system statistics:");
    notify(player, unsafe_tprintf("     Global Generic Definition: %s",
                   (b1 ? "Yes" : "No")));
    notify(player, unsafe_tprintf("     Room: %s    Exit: %s    Thing: %s    Player: %s",
                   (b3 ? "Yes" : "No"), (b2 ? "Yes" : "No"),
                   (b5 ? "Yes" : "No"), (b4 ? "Yes" : "No")));
    if (mudconf.allow_ansinames == 0 )
        notify(player, "Ansi colors have been disabled for names on all data types.");
    else {
       if (mudconf.allow_ansinames & ANSI_PLAYER )
           strcat(buff, "PLAYERS ");
       if (mudconf.allow_ansinames & ANSI_THING )
           strcat(buff, "THINGS ");
       if (mudconf.allow_ansinames & ANSI_ROOM )
           strcat(buff, "ROOMS ");
       if (mudconf.allow_ansinames & ANSI_EXIT )
           strcat(buff, "EXITS");
       notify(player, unsafe_tprintf("Ansi colors in names allowed: %s", buff));
    }
    free_mbuf(buff);
#ifdef BANGS
    notify(player, "Bang (! and !!) notation has been enabled at compiletime.");
#else
    notify(player, "Bang (! and !!) notation is currently not enabled.");
#endif
    if ( mudconf.zones_like_parents ) {
       notify(player, "Zones will behave like @parents with content/inventory/self");
       if ( Wizard(player) )
          notify(player, "WARNING:  This can be computationally expensive.");
    }
#ifdef USE_SIDEEFFECT
    if ( Wizard(player) )
       notify(player, "SECURITY WARNING:  SIDE EFFECTS ENABLED");
    notify(player, "Side effects are enabled at compiletime.");
    if ( mudconf.sideeffects > 0 ) {
       strbuff = alloc_lbuf("sideeffect");
       strptr = strbuff;
       sideEffectMaskToString(mudconf.sideeffects, strbuff, &strptr);
       notify(player, unsafe_tprintf("     Enabled: %s", strbuff));
       free_lbuf(strbuff);
    } else {
       notify(player, "     Enabled: (none)");
    }
    if ( mudconf.restrict_sidefx > 0 ) {
       if ( mudconf.restrict_sidefx > 7 ) {
          notify(player, "     Restricted: Unuseable.");
       } else if ( mudconf.restrict_sidefx > 6 ) {
          notify(player, "     Restricted: Ghod only.");
       } else if ( mudconf.restrict_sidefx > 5 ) {
          notify(player, "     Restricted: Immortal and higher only.");
       } else if ( mudconf.restrict_sidefx > 4 ) {
          notify(player, "     Restricted: Royalty and higher only.");
       } else if ( mudconf.restrict_sidefx > 3 ) {
          notify(player, "     Restricted: Councilor and higher only.");
       } else if ( mudconf.restrict_sidefx > 2 ) {
          notify(player, "     Restricted: Architect and higher only.");
       } else if ( mudconf.restrict_sidefx > 1 ) {
          notify(player, "     Restricted: Guildmaster and higher only.");
       } else {
          notify(player, "     Restricted: Non-Wanderer/Non-Guest only.");
       }
    }
#else
    notify(player, "Side effects have been disabled.");
#endif
    if ( (mudconf.global_ansimask &~0xFFF00000) != 0x000FFFFF ) {
       tbuff1ptr = tbuff1 = alloc_lbuf("list_options_ansi");
       tbuff2ptr = tbuff2 = alloc_lbuf("list_options_ansi");
       if ( !(mudconf.global_ansimask & MASK_BLACK) )
          safe_str("BLACK(x) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BLACK(x) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_RED) )
          safe_str("RED(r) ", tbuff1, &tbuff1ptr);
       else
          safe_str("RED(r) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_GREEN) )
          safe_str("GREEN(g) ", tbuff1, &tbuff1ptr);
       else
          safe_str("GREEN(g) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_YELLOW) )
          safe_str("YELLOW(y) ", tbuff1, &tbuff1ptr);
       else
          safe_str("YELLOW(y) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BLUE) )
          safe_str("BLUE(b) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BLUE(b) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_MAGENTA) )
          safe_str("MAGENTA(m) ", tbuff1, &tbuff1ptr);
       else
          safe_str("MAGENTA(m) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_CYAN) )
          safe_str("CYAN(c) ", tbuff1, &tbuff1ptr);
       else
          safe_str("CYAN(c) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_WHITE) )
          safe_str("WHITE(w) ", tbuff1, &tbuff1ptr);
       else
          safe_str("WHITE(w) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BBLACK) )
          safe_str("BKBLACK(X) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKBLACK(X) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BRED) )
          safe_str("BKRED(R) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKRED(R) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BGREEN) )
          safe_str("BKGREEN(G) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKGREEN(G) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BYELLOW) )
          safe_str("BKYELLOW(Y) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKYELLOW(Y) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BBLUE) )
          safe_str("BKBLUE(B) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKBLUE(B) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BMAGENTA) )
          safe_str("BKMAGENTA(M) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKMAGENTA(M) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BCYAN) )
          safe_str("BKCYAN(C) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKCYAN(C) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BWHITE) )
          safe_str("BKWHITE(W) ", tbuff1, &tbuff1ptr);
       else
          safe_str("INVERSE(i) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BLINK) )
          safe_str("FLASH(f) ", tbuff1, &tbuff1ptr);
       else
          safe_str("FLASH(f) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_UNDERSCORE) )
          safe_str("UNDERSCORE(u) ", tbuff1, &tbuff1ptr);
       else
          safe_str("UNDERSCORE(u) ", tbuff2, &tbuff2ptr);
       notify(player, "Global ansi code restrictions are in effect:");
       notify(player, unsafe_tprintf("     Restricted: %s", tbuff1));
       free_lbuf(tbuff1);
       notify(player, unsafe_tprintf("     Allowed: %s", tbuff2));
       free_lbuf(tbuff2);
    } else {
       notify(player, "There are no global restrictions on ansi codes.");
    }
    if ( Wizard(player) ) {
       if ( mudconf.secure_functions == 0 )
          notify(player, "Function security is not active.");
       else {
          strbuff = alloc_lbuf("sideeffect");
          strptr = strbuff;
          if ( mudconf.secure_functions & 1)
             safe_str("FOREACH ", strbuff, &strptr);
          if ( mudconf.secure_functions & 2)
             safe_str("WHILE ", strbuff, &strptr);
          if ( mudconf.secure_functions & 4)
             safe_str("FOLD ", strbuff, &strptr);
          if ( mudconf.secure_functions & 8)
             safe_str("FILTER ", strbuff, &strptr);
          if ( mudconf.secure_functions & 16)
             safe_str("MAP ", strbuff, &strptr);
          if ( mudconf.secure_functions & 32)
             safe_str("STEP ", strbuff, &strptr);
          if ( mudconf.secure_functions & 64)
             safe_str("MIX ", strbuff, &strptr);
          notify(player, unsafe_tprintf("Function security: %s", strbuff));
          free_lbuf(strbuff);
       }
       if ( Good_obj(mudconf.global_attrdefault) && 
            !Going(mudconf.global_attrdefault) &&
            !Recover(mudconf.global_attrdefault) ) {
          notify(player, unsafe_tprintf("Global Attribute Lock Object: #%d", 
                                  mudconf.global_attrdefault));
       } else {
          notify(player, "Global Attribute Lock Object: #-1 (N/A)");
       }
       buff = alloc_mbuf("options_config");
       if (mudconf.online_reg > 0 || mudconf.offline_reg > 0 ) {
          sprintf(buff, "Autoregistration:  As Guest...%s  Connect Screen...%s"\
                        "  Limit-Per-Connect: %d",
                        mudconf.online_reg > 0 ? "YES" : "NO",
                        mudconf.offline_reg > 0 ? "YES" : "NO", 
                        mudconf.regtry_limit);
          notify(player,buff);
          notify(player, unsafe_tprintf("                   Current mail used: %s.", 
                         mudconf.mailprog));       
          if ( mudconf.mailsub > 0 )
             notify(player, "                   Subjects in mail are allowed.");
          else
             notify(player, "                   Subjects in mail are not allowed.");
          notify(player, unsafe_tprintf("                   Mail file included: %s.", mudconf.mailinclude_file));
       }
       else
          notify(player, "Autoregistration is currently disabled.");
       sprintf(buff, "Intervals: Dump...%d  Clean...%d  Idlecheck...%d  Rwho...%d",
               mudconf.dump_interval, mudconf.check_interval,
               mudconf.idle_interval, mudconf.rwho_interval);
       notify(player, buff);
       sprintf(buff, "Timers: Dump...%.1f  Clean...%.1f  Idlecheck...%.1f  Rwho...%.1f",
               mudstate.dump_counter - now, mudstate.check_counter - now,
               mudstate.idle_counter - now, mudstate.rwho_counter - now);
       notify(player, buff);
       sprintf(buff, "CPU Watchdogs:  CPU...%d%%  Elapsed Time...%d seconds",
             //(mudconf.cpuintervalchk < 10 ? 10 : (mudconf.cpuintervalchk > 100 ? 100 : mudconf.cpuintervalchk)),
             (mudconf.cpuintervalchk > 100 ? 100 : mudconf.cpuintervalchk),
             //(mudconf.cputimechk < 10 ? 10 : (mudconf.cputimechk > 3600 ? 3600 : mudconf.cputimechk)) );
             (mudconf.cputimechk > 3600 ? 3600 : mudconf.cputimechk) );

       notify(player, buff);
       sprintf(buff, "                Security...Level %d  Max Sequential Slams...%d",
             (((mudconf.cpu_secure_lvl < 0) || (mudconf.cpu_secure_lvl > 5)) ? 0 : mudconf.cpu_secure_lvl),
             mudconf.max_cpu_cycles);
       notify(player, buff);
       sprintf(buff, "                Stack Ceiling: %d", mudconf.stack_limit);

       notify(player, buff);
       sprintf(buff, "                Spam Ceiling:  %d", mudconf.spam_limit);
       notify(player, buff);
       sprintf(buff, "                WildMatch Ceiling:  %d", mudconf.wildmatch_lim);
       notify(player, buff);
       sprintf(buff, "                SideFX Ceiling:  %d", mudconf.sidefx_maxcalls);
       notify(player, buff);
       sprintf(buff, "                Percent-Sub Ceiling:  %d", mudconf.max_percentsubs);
       notify(player, buff);
       sprintf(buff, "                Heavy Function CPU Recursor Limit...%d",
                  mudconf.heavy_cpu_max);
       notify(player,  buff);
       sprintf(buff, "                @aahear/@ahear/@amhear/@listen Limit...%d/%ds",
               mudconf.ahear_maxcount, mudconf.ahear_maxtime);
       notify(player,  buff);
       if ( mudstate.curr_cpu_user != NOTHING ) {
          sprintf(buff, "                Current CPU Abuser...#%d  Total CPU Slams...%d",
                  mudstate.curr_cpu_user, mudstate.curr_cpu_cycle);
       } else {
          sprintf(buff, "                Current CPU Abuser...(none)");
       }
       notify(player, buff);
       if ( mudstate.last_cpu_user != NOTHING ) {
          sprintf(buff, "                Last Punished CPU Abuser...#%d",
                  mudstate.last_cpu_user);
          notify(player, buff);
       }
       if ( mudconf.idle_timeout == -1 )
          sprintf(buff, "Timeouts: Idle...Inf  Connect...%d  Tries...%d",
                  mudconf.conn_timeout, mudconf.retry_limit);
       else
          sprintf(buff, "Timeouts: Idle...%d  Connect...%d  Tries...%d",
                  mudconf.idle_timeout, mudconf.conn_timeout,
                  mudconf.retry_limit);
       notify(player, buff);
       sprintf(buff, "Scheduling: Timeslice...%d  Max_Quota...%d  Increment...%d",
               mudconf.timeslice, mudconf.cmd_quota_max,
               mudconf.cmd_quota_incr);
       notify(player, buff);
       sprintf(buff, "Compression: Strings...%s  Spaces...%s  Savefiles...%s",
               ed[mudconf.intern_comp], ed[mudconf.space_compress],
               ed[mudconf.compress_db]);
       notify(player, buff);
       sprintf(buff, "New characters: Room...#%d  Home...#%d  DefaultHome...#%d  Quota...%d  Guild...%-11.11s",
               mudconf.start_room, mudconf.start_home, mudconf.default_home,
               mudconf.start_quota, strip_ansi(mudconf.guild_default));
       notify(player, buff);
       sprintf(buff, "Rwho: Host...%s  Port...%d  Connection...%s  Transmit...%s",
               mudconf.rwho_host, mudconf.rwho_info_port,
               ud[mudstate.rwho_on], ed[mudconf.rwho_transmit]);
       notify(player, buff);
       sprintf(buff, "Misc: IdleQueueChunk...%d  ActiveQueueChunk...%d  Master_room...#%d",
               mudconf.queue_chunk,
               mudconf.active_q_chunk, 
               mudconf.master_room);
       notify(player, buff);
       if(mudconf.pagecost <= 0)
           sprintf(buff, "Costs: Wall...%d  Page...Free  Com...%d", mudconf.wall_cost, mudconf.comcost);
       else
           sprintf(buff, "Costs: Wall...%d  Page...1/%d length (round up)  Com...%d", mudconf.wall_cost, (mudconf.pagecost), mudconf.comcost);
       
       notify(player, buff);
       sprintf(buff, "Configure Options: FailConnect Spam Protection...%s   'R' in WHO...%s   " \
                     "\r\n                   Maximum Concurrent Site-Connections...%d",
                     (mudconf.nospam_connect ? "Yes" : "No"), (mudconf.noregist_onwho ? "No" : "Yes"),
                     mudconf.max_sitecons);
       notify(player, buff);
       sprintf(buff, "                   Maximum Lines for normal .txt files...%d",
                     mudconf.nonindxtxt_maxlines);
       notify(player, buff);
/* MAX_ARGS is defined in externs.h - default 30 */
#ifdef MAX_ARGS
#define NFARGS  MAX_ARGS
#else
#define NFARGS 30
#endif
       sprintf(buff, "Limits: Trace(top-down)...%d  Retry...%d  Pay...%d\r\n" \
                     "        Function_nest...%d Function_invoke...%d  Lock_nest...%d",
           mudconf.trace_limit, mudconf.retry_limit, mudconf.paylimit, 
           (mudconf.func_nest_lim > 300 ? 300 : mudconf.func_nest_lim),
           mudconf.func_invk_lim, mudconf.lock_nest_lim);
       notify(player, buff);
       buff3 = alloc_sbuf("list_config_temp");
       sprintf(buff3, "%d", mudconf.max_pcreate_lim);
       sprintf(buff, "        Parent_nest...%d  User_attrs...%d  Function_Args...%d\r\n" \
                     "        Max sitecons...%d/%ds   Max site pcreates...%s/%ds",
           mudconf.parent_nest_lim, mudconf.vlimit, NFARGS,
           mudconf.max_lastsite_cnt, mudconf.min_con_attempt,
           (mudconf.max_pcreate_lim == -1 ? "Inf" : buff3), mudconf.max_pcreate_time);
       notify(player, buff);
       free_sbuf(buff3);
       sprintf(buff, "        Sitecon paranoia level...%d   PCreate paranoia level...%d\r\n" \
                     "        Max VAttrs...%d  Max @destroy...%d",
           mudconf.lastsite_paranoia, mudconf.pcreate_paranoia,
           mudconf.max_vattr_limit, mudconf.max_dest_limit);
       notify(player, buff);
       sprintf(buff, "        Max Wiz VAttrs...%d  Max Wiz @destroy...%d  Wiz Check...%s",
           mudconf.wizmax_vattr_limit, mudconf.wizmax_dest_limit, 
           (mudconf.vattr_limit_checkwiz ? "enabled" : "disabled") );
       notify(player, buff);
       sprintf(buff, "        Player Max Queue...%d  Wizard Max Queue...%d",
                      mudconf.queuemax, mudconf.wizqueuemax);
       notify(player, buff);
       sprintf(buff, "        Max Players Allowed: %d (%s)",
               (mudconf.max_players > 0 ? mudconf.max_players : mudstate.max_logins_allowed),
               (mudconf.max_players > 0 ? "User Defined" : "Max Free OS Descriptors"));
       notify(player, buff);
       free_mbuf(buff);
    }
    DPOP; /* #46 */
}

static void 
list_options(dbref player)
{
    char *buff, *strbuff, *strptr, newstime[30], mailtime[30], mushtime[30], aregtime[30];
    char *tbuff1, *tbuff1ptr, *tbuff2, *tbuff2ptr, *buff3;
    double now;
    int itog_val, b1, b2, b3, b4, b5;

    DPUSH; /* #47 */

    itog_val = 0;
    b1 = b2 = b3 = b4 = b5 = 0;
    now = time_ng(NULL);
    if (mudconf.quotas)
    notify(player, "Building quotas are enforced.");
    else
        notify(player, "Quotas are disabled for building.");
    if (mudconf.name_spaces)
    notify(player, "Player names may contain spaces.");
    else
    notify(player, "Player names may not contain spaces.");
    if (mudconf.paranoid_exit_linking)
        notify(player, "Exits must be controlled to be linked.");
    if (!mudconf.robot_speak)
    notify(player, "Robots are not allowed to speak in public areas.");
    if (mudconf.player_listen)
    notify(player, "The @Listen/@Ahear attribute set works on player objects.");
    if (mudconf.ex_flags)
    notify(player, "The 'examine' command lists the flag names for the object's flags.");
    if (!mudconf.quiet_look)
    notify(player, "The 'look' command shows visible attributes in addition to the description.");
    if (mudconf.see_own_dark)
    notify(player, "The 'look' command lists DARK objects owned by you.");
    if (!mudconf.dark_sleepers)
    notify(player, "The 'look' command shows disconnected players.");
    if (mudconf.terse_look)
    notify(player, "The 'look' command obeys the TERSE flag.");
    if (mudconf.trace_topdown) {
    notify(player, "Trace output is presented top-down (whole expression first, then sub-exprs).");
    notify(player, unsafe_tprintf("Only %d lines of trace output are displayed.",
                   mudconf.trace_limit));
    } else {
    notify(player, "Trace output is presented bottom-up (subexpressions first).");
    }
    if (mudconf.switch_substitutions)
        notify(player, "#$ can be used as a substitution for @switch, switch(), and switchall().");
    if (!mudconf.quiet_whisper)
    notify(player, "The 'whisper' command lets others in the room with you know you whispered.");
    if (mudconf.pemit_players)
    notify(player, "The '@pemit' command may be used to emit to faraway players.");
    if (!mudconf.terse_contents)
    notify(player, "The TERSE flag suppresses listing the contents of a location.");
    if (!mudconf.terse_exits)
    notify(player, "The TERSE flag suppresses listing obvious exits in a location.");
    if (!mudconf.terse_movemsg)
    notify(player, "The TERSE flag suppresses enter/leave/succ/drop messages generated by moving.");
    if (mudconf.pub_flags)
    notify(player, "The 'flags()' function will return the flags of any object.");
    if (mudconf.read_rem_desc)
    notify(player, "The 'get()' function will return the description of faraway objects,");
    if (mudconf.read_rem_name)
    notify(player, "The 'name()' function will return the name of faraway objects.");
    notify(player,
       unsafe_tprintf("The default switch for the '@switch' command is %s.",
           switchd[mudconf.switch_df_all]));
    notify(player,
       unsafe_tprintf("The default switch for the 'examine' command is %s.",
           examd[mudconf.exam_public]));
    if (mudconf.sweep_dark)
    notify(player, "Players may @sweep dark locations.");
    if (mudconf.fascist_tport)
    notify(player, "You may only @teleport out of locations that are JUMP_OK or that you control.");

    if (mudconf.parent_control)
    notify(player, "All locks may be inherited from parents.");

    notify(player,
    unsafe_tprintf("Players may have at most %d commands in the queue at one time.",
        mudconf.queuemax));
    if (mudconf.match_mine) {
    if (mudconf.match_mine_pl && mudconf.match_pl)
        notify(player, "All objects search themselves for $-commands.");
    else if (!mudconf.match_pl)
        notify(player, "Objects other than players are searched for $-commands.");
    else if (mudconf.match_mine_pl)
        notify(player, "Objects other than players search themselves for $-commands.");
    }
    if (mudconf.player_dark) {
       if (mudconf.secure_dark)
          notify(player,"Players can set the dark flag on listening things but not themselves.");
       else
      notify(player,"Players can set the dark flag on listening things and themselves.");
    }
    else
    notify(player,"Players can't set the dark flag on listening things or themselves.");

    if (mudconf.who_unfindable)
    notify(player,"Unfindable players aren't on the WHO list");
    else
    notify(player,"Unfindable players are on the WHO list");
    if (mudconf.player_listen)
    notify(player,"Listening on players is enabled.");
    else
    notify(player,"Listening on players is disabled.");
    if (mudconf.match_mine) {
    notify(player,"Commands on players are enabled.");
        if ( mudconf.penn_playercmds ) {
           notify(player, "Commands on players only work for themselves or their inventory.");
        }
    } else
    notify(player,"Commands on players are disabled.");
    if (mudconf.penn_switches)
        notify(player, "switch() and switchall() function like PENN's.");
    if (!(mudconf.control_flags & CF_BUILD)) {
        notify(player,"Building has been disabled for anyone under Architect.");
    }
    if (mudconf.enable_tstamps > 0)
       notify(player, "Time stamps and modify stamps will show up on all object types.");
    if (mudconf.fmt_exits) {
       notify(player, "@exitformat can be used for exit list formatting.");
       if Wizard(player) {
          notify(player, "@darkexitformat can be used for dark exit list formatting.");
       }
    } else {
      notify(player, "@exitformat is disabled. (Enable with format_exits)");
    }
    if (mudconf.fmt_contents)    
      notify(player, "@conformat can be used for content list formatting.");
    else
      notify(player, "@conformat is disabled. (Enable with format_contents)");
    if (mudconf.fmt_names)
      notify(player, "@nameformat can be used for name formatting.");
    else
      notify(player, "@nameformat is disabled. (Enable with format_name)");

    buff = alloc_mbuf("list_options");
    memset(buff, 0, MBUF_SIZE);
    if (mudconf.allow_ansinames == 0 )
        notify(player, "Ansi colors have been disabled for names on all data types.");
    else {
       if (mudconf.allow_ansinames & ANSI_PLAYER )
           strcat(buff, "PLAYERS ");
       if (mudconf.allow_ansinames & ANSI_THING )
           strcat(buff, "THINGS ");
       if (mudconf.allow_ansinames & ANSI_ROOM )
           strcat(buff, "ROOMS ");
       if (mudconf.allow_ansinames & ANSI_EXIT )
           strcat(buff, "EXITS");
       notify(player, unsafe_tprintf("Ansi colors in names allowed: %s", buff));
    }
    free_mbuf(buff);
#ifdef BANGS
    notify(player, "Bang (! and !!) notation has been enabled at compiletime.");
#else
    notify(player, "Bang (! and !!) notation is currently not enabled.");
#endif

    if ( mudconf.safe_wipe > 0 )
        notify(player, "@wipe will not work on objects set SAFE or INDESTRUCTABLE.");
    if ( mudconf.secure_jumpok > 7 )
    notify(player, "Only immortals may set the JUMP_OK flag.");
    else if ( mudconf.secure_jumpok > 6 )
    notify(player, "Royalty may set the JUMP_OK flag on rooms only.");
    else if ( mudconf.secure_jumpok > 5 )
    notify(player, "Only royalty and higher may set the JUMP_OK flag.");
    else if ( mudconf.secure_jumpok > 4 )
    notify(player, "Councilors may set the JUMP_OK flag on rooms only.");
    else if ( mudconf.secure_jumpok > 3 )
    notify(player, "Only councilors and higher may set the JUMP_OK flag.");
    else if ( mudconf.secure_jumpok > 2 )
    notify(player, "Architects may set the JUMP_OK flag on rooms only.");
    else if ( mudconf.secure_jumpok > 1 )
    notify(player, "Mortals can not set the JUMP_OK flag at all.");
    else if ( mudconf.secure_jumpok > 0 )
    notify(player, "Mortals can only set the JUMP_OK flag on type ROOM.");

#ifdef TINY_U
    notify(player, "u()/zfun() functionality changed at compile-time for TinyMUSH compatibility.");
#endif
#ifdef OLD_SETQ
    notify(player, "setq()/setq_old() functionality changed at compile-time for TinyMUSH compatibility.");
#endif
    if ( mudconf.switch_search != 0 )
       notify(player, "switch and switchng (nogarbage) functionality has been switched.");
#ifndef NOEXTSUBS
#ifdef TINY_SUB
    notify(player, "%c and %x substitution have been switched for TinyMUSH compatibility.");
#endif
#else
    notify(player, "%c and %x substitutions have been disabled.");
#endif
#ifdef USECRYPT
    notify(player, "Cryptological functionalty has been enabled.");
#endif
#ifdef SOFTCOM
    notify(player, "The hard-coded comsystem has been disabled.");
#else
    notify(player, "The hard-coded comsystem is currently being used.");
#endif
#ifdef MUX_INCDEC
    notify(player, "Mux formatted Inc() and Dec() support has been enabled.");
#endif
    if ( mudconf.lnum_compat == 1 )
       notify(player, "LNum() and LNum2() follow TinyMUX conventions.");
    if ( mudconf.lcon_checks_dark == 1 )
       notify(player, "lcon()/xcon() obeys dark/unfindable flags.");
#ifdef ENABLE_COMMAND_FLAG
    notify(player, "$commands require the COMMANDS flag to work.");
#endif
     
#ifdef USE_SIDEEFFECT
    if ( Wizard(player) )
       notify(player, "SECURITY WARNING:  SIDE EFFECTS ENABLED");
    notify(player, "Side effects are enabled at compiletime.");
    if ( mudconf.sideeffects > 0 ) {
       char *p = strbuff = alloc_lbuf("sideeffect");
       sideEffectMaskToString(mudconf.sideeffects, strbuff, &p);
       notify(player, unsafe_tprintf("     Enabled: %s", strbuff));
       free_lbuf(strbuff);
    } else {
       notify(player, "     Enabled: (none)");
    }
    if ( mudconf.restrict_sidefx > 0 ) {
       if ( mudconf.restrict_sidefx > 7 ) {
          notify(player, "     Restricted: Unuseable.");
       } else if ( mudconf.restrict_sidefx > 6 ) {
          notify(player, "     Restricted: Ghod only.");
       } else if ( mudconf.restrict_sidefx > 5 ) {
          notify(player, "     Restricted: Immortal and higher only.");
       } else if ( mudconf.restrict_sidefx > 4 ) {
          notify(player, "     Restricted: Royalty and higher only.");
       } else if ( mudconf.restrict_sidefx > 3 ) {
          notify(player, "     Restricted: Councilor and higher only.");
       } else if ( mudconf.restrict_sidefx > 2 ) {
          notify(player, "     Restricted: Architect and higher only.");
       } else if ( mudconf.restrict_sidefx > 1 ) {
          notify(player, "     Restricted: Guildmaster and higher only.");
       } else {
          notify(player, "     Restricted: Non-Wanderer/Non-Guest only.");
       }
    }
#else
    notify(player, "Side effects have been disabled.");
#endif
    if ( mudconf.alt_inventories ) {
       notify(player, "Alternate inventories are enabled.  Use 'wielded' and 'worn' for them.");
       if ( mudconf.altover_inv )
          notify(player, "Items belonging to alternate inventories will not show up in 'inventory'");
       if ( mudconf.showother_altinv )
          notify(player, "Equipment worn/wielded is shown when someone LOOKS");
    } else {
      notify(player, "Alternate inventories are disabled.");
    }
    if ( mudconf.safer_passwords )
       notify(player, "Password setting/resetting require tougher/harder passwords.");
    if ( mudconf.mail_tolist )
       notify(player, "Mail automatically adds the 'To:' list in sent mail.");
    else
       notify(player, "Mail requires a '@' before player lists to show 'To:' in mail.");
    if ( mudconf.mail_default )
       notify(player, "mail mimics mail/status");
    else
       notify(player, "mail mimics mail/quick");
    notify(player, unsafe_tprintf("Parent Defaults: Room: #%d  Exit: #%d  Player: #%d  Thing: #%d",
           (Good_obj(mudconf.room_parent) && !Recover(mudconf.room_parent)) ? 
               mudconf.room_parent : -1,
           (Good_obj(mudconf.exit_parent) && !Recover(mudconf.exit_parent)) ? 
               mudconf.exit_parent : -1,
           (Good_obj(mudconf.player_parent) && !Recover(mudconf.player_parent)) ?
               mudconf.player_parent : -1,
           (Good_obj(mudconf.thing_parent) && !Recover(mudconf.thing_parent)) ?
               mudconf.thing_parent : -1) );
    notify(player, unsafe_tprintf("Attribute Defaults: Room: #%d  Exit: #%d  Player: #%d  Thing: #%d",
           (Good_obj(mudconf.room_defobj) && !Recover(mudconf.room_defobj)) ? 
               mudconf.room_defobj : -1,
           (Good_obj(mudconf.exit_defobj) && !Recover(mudconf.exit_defobj)) ? 
               mudconf.exit_defobj : -1,
           (Good_obj(mudconf.player_defobj) && !Recover(mudconf.player_defobj)) ?
               mudconf.player_defobj : -1,
           (Good_obj(mudconf.thing_defobj) && !Recover(mudconf.thing_defobj)) ?
               mudconf.thing_defobj : -1) );
    b1 = b2 = b3 = b4 = b5 = 0;
    if ( Good_obj(mudconf.global_parent_obj) &&
         !Recover(mudconf.global_parent_obj) &&
         !Going(mudconf.global_parent_obj) ) {
       b1 = 1;
    }
    if ( Good_obj(mudconf.global_parent_exit) &&
         !Recover(mudconf.global_parent_exit) &&
         !Going(mudconf.global_parent_exit) ) {
       b2 = 1;
    }
    if ( Good_obj(mudconf.global_parent_room) &&
         !Recover(mudconf.global_parent_room) &&
         !Going(mudconf.global_parent_room) ) {
       b3 = 1;
    }
    if ( Good_obj(mudconf.global_parent_player) &&
         !Recover(mudconf.global_parent_player) &&
         !Going(mudconf.global_parent_player) ) {
       b4 = 1;
    }
    if ( Good_obj(mudconf.global_parent_thing) &&
         !Recover(mudconf.global_parent_thing) &&
         !Going(mudconf.global_parent_thing) ) {
       b5 = 1;
    }
    notify(player, "Global parent object system statistics:");
    notify(player, unsafe_tprintf("     Global Generic Definition: %s",
                   (b1 ? "Yes" : "No")));
    notify(player, unsafe_tprintf("     Room: %s    Exit: %s    Thing: %s    Player: %s",
                   (b3 ? "Yes" : "No"), (b2 ? "Yes" : "No"),
                   (b5 ? "Yes" : "No"), (b4 ? "Yes" : "No")));
    b1 = b2 = b3 = b4 = b5 = 0;
    if ( Good_obj(mudconf.global_clone_obj) &&
         !Recover(mudconf.global_clone_obj) &&
         !Going(mudconf.global_clone_obj) ) {
       b1 = 1;
    }
    if ( Good_obj(mudconf.global_clone_exit) &&
         !Recover(mudconf.global_clone_exit) &&
         !Going(mudconf.global_clone_exit) ) {
       b2 = 1;
    }
    if ( Good_obj(mudconf.global_clone_room) &&
         !Recover(mudconf.global_clone_room) &&
         !Going(mudconf.global_clone_room) ) {
       b3 = 1;
    }
    if ( Good_obj(mudconf.global_clone_player) &&
         !Recover(mudconf.global_clone_player) &&
         !Going(mudconf.global_clone_player) ) {
       b4 = 1;
    }
    if ( Good_obj(mudconf.global_clone_thing) &&
         !Recover(mudconf.global_clone_thing) &&
         !Going(mudconf.global_clone_thing) ) {
       b5 = 1;
    }
    notify(player, "Global clone object system statistics:");
    notify(player, unsafe_tprintf("     Global Generic Definition: %s",
                   (b1 ? "Yes" : "No")));
    notify(player, unsafe_tprintf("     Room: %s    Exit: %s    Thing: %s    Player: %s",
                   (b3 ? "Yes" : "No"), (b2 ? "Yes" : "No"),
                   (b5 ? "Yes" : "No"), (b4 ? "Yes" : "No")));
#ifdef PLUSHELP
    notify(player, "The hardcoded +help system has been enabled.");
#else
    notify(player, "The hardcoded +help system is disabled.");
#endif
    notify(player, "@program parameters currently in effect:");
#ifdef PROG_LIKEMUX
    itog_val = 1;
#endif
    notify(player, unsafe_tprintf("     Shell (|) from @program: %s\r\n     MUX-like return values:  %s",
                   (mudconf.noshell_prog ? "No" : "Yes"), (itog_val ? "Yes" : "No")));
    notify(player, unsafe_tprintf("     Login to existing prog?: %s", 
                   (mudconf.login_to_prog ? "Yes" : "No")));
    if ( (mudconf.global_ansimask &~0xFFF00000) != 0x000FFFFF ) {
       tbuff1ptr = tbuff1 = alloc_lbuf("list_options_ansi");
       tbuff2ptr = tbuff2 = alloc_lbuf("list_options_ansi");
       if ( !(mudconf.global_ansimask & MASK_BLACK) )
          safe_str("BLACK(x) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BLACK(x) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_RED) )
          safe_str("RED(r) ", tbuff1, &tbuff1ptr);
       else
          safe_str("RED(r) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_GREEN) )
          safe_str("GREEN(g) ", tbuff1, &tbuff1ptr);
       else
          safe_str("GREEN(g) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_YELLOW) )
          safe_str("YELLOW(y) ", tbuff1, &tbuff1ptr);
       else
          safe_str("YELLOW(y) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BLUE) )
          safe_str("BLUE(b) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BLUE(b) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_MAGENTA) )
          safe_str("MAGENTA(m) ", tbuff1, &tbuff1ptr);
       else
          safe_str("MAGENTA(m) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_CYAN) )
          safe_str("CYAN(c) ", tbuff1, &tbuff1ptr);
       else
          safe_str("CYAN(c) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_WHITE) )
          safe_str("WHITE(w) ", tbuff1, &tbuff1ptr);
       else
          safe_str("WHITE(w) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BBLACK) )
          safe_str("BKBLACK(X) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKBLACK(X) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BRED) )
          safe_str("BKRED(R) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKRED(R) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BGREEN) )
          safe_str("BKGREEN(G) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKGREEN(G) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BYELLOW) )
          safe_str("BKYELLOW(Y) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKYELLOW(Y) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BBLUE) )
          safe_str("BKBLUE(B) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKBLUE(B) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BMAGENTA) )
          safe_str("BKMAGENTA(M) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKMAGENTA(M) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BCYAN) )
          safe_str("BKCYAN(C) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKCYAN(C) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BWHITE) )
          safe_str("BKWHITE(W) ", tbuff1, &tbuff1ptr);
       else
          safe_str("BKWHITE(W) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_HILITE) )
          safe_str("HILITE(h) ", tbuff1, &tbuff1ptr);
       else
          safe_str("HILITE(h) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_INVERSE) )
          safe_str("INVERSE(i) ", tbuff1, &tbuff1ptr);
       else
          safe_str("INVERSE(i) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_BLINK) )
          safe_str("FLASH(f) ", tbuff1, &tbuff1ptr);
       else
          safe_str("FLASH(f) ", tbuff2, &tbuff2ptr);
       if ( !(mudconf.global_ansimask & MASK_UNDERSCORE) )
          safe_str("UNDERSCORE(u) ", tbuff1, &tbuff1ptr);
       else
          safe_str("UNDERSCORE(u) ", tbuff2, &tbuff2ptr);
       notify(player, "Global ansi code restrictions are in effect:");
       notify(player, unsafe_tprintf("     Restricted: %s", tbuff1));
       free_lbuf(tbuff1);
       notify(player, unsafe_tprintf("     Allowed: %s", tbuff2));
       free_lbuf(tbuff2);
    } else {
       notify(player, "There are no global restrictions on ansi codes.");
    }
    if ( mudconf.partial_conn )
       notify(player, "@aconnects triggered when log in while connected. (default)");
    else
       notify(player, "@aconnects NOT triggered when log in while connected.");
    if ( mudconf.partial_deconn )
       notify(player, "@adisconnects triggered when log off while still connected.");
    else
       notify(player, "@adisconnects NOT triggered when log off while still connected. (default)");
    if ( mudconf.formats_are_local )
       notify(player, "@nameformat, @exitformat, and @conformat are localized for setq's.");
    else
       notify(player, "@nameformat, @exitformat, and @conformat can clobber setq's.");
#ifdef REALITY_LEVELS
       notify(player, unsafe_tprintf("Reality levels have been enabled.  There are %d total levels.",
                      mudconf.no_levels));
       if (mudconf.wiz_always_real)
          notify(player, "Wizards are always in the REAL reality.");
       if (mudconf.reality_locks) {
          buff = alloc_mbuf("list_options");
          sprintf(buff,  "Reality Locks are enabled (type: %d).  Use @lock/user to define.", 
                  mudconf.reality_locktype);
          notify(player, buff);
          free_mbuf(buff);
       }
#else
       notify(player, "Reality levels are not being used.");
#endif
#ifdef HAS_OPENSSL
    notify(player, "OpenSSL supported digests have been enabled.");
#else
    notify(player, "OpenSSL was not found.  SHA digest supported only.");
#endif
#ifdef ATTR_HACK
    notify(player, "You may use ~ATTRS for uselocks and the USELOCK attribute flag.");
    if ( mudconf.hackattr_see ) {
       notify(player, "_ attributes are wizard only settable and viewable");     
    }
#else
       notify(player, "You can not use ~ATTRS or the USELOCK attribute flag.");
#endif
    if ( mudconf.zone_parents )
       notify(player, "Zone children inherent attributes from their zone master(s).");
#ifdef EXPANDED_QREGS
       notify(player, "SETQ/SETR/R can handle a-z registers in addition to 0-9.");
#else
       notify(player, "SETQ/SETR/R can only handle 0-9 registers.");
#endif
    if ( mudconf.look_moreflags )
       notify(player, "Look/etc is expanded to show global attribute flags inside []'s.");
    if (!Wizard(player)){
        DPOP; /* #47 */
    return;
    }
    if ( mudconf.examine_restrictive > 0 ) {
       switch(mudconf.examine_restrictive) {
          case 1: notify(player, "Examine restricts control for Guests/Wanderers.");
             break;
          case 2: notify(player, "Examine restricts control for mortals.");
             break;
          case 3: notify(player, "Examine restricts control for Guildmaster and lower.");
             break;
          case 4: notify(player, "Examine restricts control for Architect and lower.");
             break;
          case 5: notify(player, "Examine restricts control for Councilor and lower.");
             break;
          default: notify(player, "Examine restricts control for Wizard and lower.");
             break;
       }
    }
    buff = alloc_mbuf("list_options");
    if ( mudconf.oattr_enable_altname > 0 ) {
      notify(player, unsafe_tprintf("OSUCC/OFAIL/ODROP use optional altname of '%s'",
                 mudconf.oattr_uses_altname));
    } else {
      notify(player, "Optional altnames on OSUCC/OFAIL/ODROP is disabled.");
    }

    notify(player,
    unsafe_tprintf("Arch and higher may have at most %d commands in the queue at one time.",
        mudconf.wizqueuemax));
    if ( mudconf.secure_functions == 0 )
       notify(player, "Function security is not active.");
    else {
       strbuff = alloc_lbuf("sideeffect");
       strptr = strbuff;
       if ( mudconf.secure_functions & 1)
          safe_str("FOREACH ", strbuff, &strptr);
       if ( mudconf.secure_functions & 2)
          safe_str("WHILE ", strbuff, &strptr);
       if ( mudconf.secure_functions & 4)
          safe_str("FOLD ", strbuff, &strptr);
       if ( mudconf.secure_functions & 8)
          safe_str("FILTER ", strbuff, &strptr);
       if ( mudconf.secure_functions & 16)
          safe_str("MAP ", strbuff, &strptr);
       if ( mudconf.secure_functions & 32)
          safe_str("STEP ", strbuff, &strptr);
       if ( mudconf.secure_functions & 64)
          safe_str("MIX ", strbuff, &strptr);
       notify(player, unsafe_tprintf("Function security: %s", strbuff));
       free_lbuf(strbuff);
    }
    if ( Good_obj(mudconf.global_attrdefault) && 
         !Going(mudconf.global_attrdefault) &&
         !Recover(mudconf.global_attrdefault) ) {
       notify(player, unsafe_tprintf("Global Attribute Lock Object: #%d", 
                               mudconf.global_attrdefault));
    } else {
       notify(player, "Global Attribute Lock Object: #-1 (N/A)");
    }
    if ( mudconf.restrict_home > 0 ) {
       notify(player, unsafe_tprintf("The HOME command is restricted with mask '%d'.", 
              mudconf.restrict_home));
    }
    if ( mudconf.expand_goto ) {
       notify(player, "The GOTO command is expanded automatically on exit movement.");
    }
    if ( !mudconf.notonerr_return ) {
       notify(player, "Functions that return '#-1' are considered urinary '0' valued.");
    }
    if ( mudconf.imm_nomod > 0 ) {
       notify(player, "The NOMODIFY flag is accessable by IMMORTAL only.");
    }
    if ( mudconf.always_blind > 0 ) {
       notify(player, "The BLIND flag is 'always on' rooms and exits for exit-movement.");
       notify(player, "The BLIND flag is required to _SHOW_ arrival/leave messages.");
    } else {
       notify(player, "The BLIND flag is required to _HIDE_ arrival/leave messages.");
    }
#ifdef ATTR_HACK
    notify(player, "Attributes are allowed to be set USELOCK.");
    notify(player, "Attributes are allowed to start with a '_' character.");
    notify(player, unsafe_tprintf("     '_' attributes are settable by %s.", 
                          (mudconf.hackattr_nowiz ? "anyone" : "WIZARDS only")));
    notify(player, unsafe_tprintf("     '_' attributes are seeable by %s.", 
                          (mudconf.hackattr_see ? "anyone" : "WIZARDS only")));
#else
    notify(player, "Attributes can not be set USELOCK.");
    notify(player, "Attributes may not start with a '_' character.");
#endif
    b1 = b2 = b3 = b4 = b5 = -1;
    if ( Good_obj(mudconf.global_parent_obj) && 
         !Recover(mudconf.global_parent_obj) &&
         !Going(mudconf.global_parent_obj) ) {
       b1 = mudconf.global_parent_obj;
    }
    if ( Good_obj(mudconf.global_parent_exit) &&
         !Recover(mudconf.global_parent_exit) &&
         !Going(mudconf.global_parent_exit) ) {
       b2 = mudconf.global_parent_exit;
    }
    if ( Good_obj(mudconf.global_parent_room) &&
         !Recover(mudconf.global_parent_room) &&
         !Going(mudconf.global_parent_room) ) {
       b3 = mudconf.global_parent_room;
    }
    if ( Good_obj(mudconf.global_parent_player) &&
         !Recover(mudconf.global_parent_player) &&
         !Going(mudconf.global_parent_player) ) {
       b4 = mudconf.global_parent_player;
    }
    if ( Good_obj(mudconf.global_parent_thing) &&
         !Recover(mudconf.global_parent_thing) &&
         !Going(mudconf.global_parent_thing) ) {
       b5 = mudconf.global_parent_thing;
    }
    notify(player, "Global parent object system statistics (Wizard):");
    notify(player, unsafe_tprintf("     Global: #%d     Room: #%d    Exit: #%d    Thing: #%d    Player: #%d",
                   b1, b3, b2, b5, b4));
    b1 = b2 = b3 = b4 = b5 = -1;
    if ( Good_obj(mudconf.global_clone_obj) && 
         !Recover(mudconf.global_clone_obj) &&
         !Going(mudconf.global_clone_obj) ) {
       b1 = mudconf.global_clone_obj;
    }
    if ( Good_obj(mudconf.global_clone_exit) &&
         !Recover(mudconf.global_clone_exit) &&
         !Going(mudconf.global_clone_exit) ) {
       b2 = mudconf.global_clone_exit;
    }
    if ( Good_obj(mudconf.global_clone_room) &&
         !Recover(mudconf.global_clone_room) &&
         !Going(mudconf.global_clone_room) ) {
       b3 = mudconf.global_clone_room;
    }
    if ( Good_obj(mudconf.global_clone_player) &&
         !Recover(mudconf.global_clone_player) &&
         !Going(mudconf.global_clone_player) ) {
       b4 = mudconf.global_clone_player;
    }
    if ( Good_obj(mudconf.global_clone_thing) &&
         !Recover(mudconf.global_clone_thing) &&
         !Going(mudconf.global_clone_thing) ) {
       b5 = mudconf.global_clone_thing;
    }
    notify(player, "Global clone object system statistics (Wizard):");
    notify(player, unsafe_tprintf("     Global: #%d     Room: #%d    Exit: #%d    Thing: #%d    Player: #%d",
                   b1, b3, b2, b5, b4));
    if ( strlen(mudconf.guest_namelist) > 2 ) {
       notify(player, "The dynamic guest system (guest_namelist) is in effect.");
    }
    if ( mudconf.hide_nospoof ) {
       notify(player, "The NOSPOOF requires control to see it.");
    }
    notify(player,
       unsafe_tprintf("%d commands are run from the queue when there is no net activity.",
           mudconf.queue_chunk));
    notify(player,
    unsafe_tprintf("%d commands are run from the queue when there is net activity.",
        mudconf.active_q_chunk));
    if (mudconf.idle_wiz_cloak)
    notify(player, "Wizards idle for longer than the default timeout are automatically WIZCLOAKED.");
    else if (mudconf.idle_wiz_dark)
    notify(player, "Wizards idle for longer than the default timeout are automatically set DARK.");
    if (mudconf.idle_message)
        notify(player, "Wizards are notified if they idle out and in what method they have idled.");
    if (mudconf.safe_unowned)
    notify(player, "Objects not owned by you are automatically considered SAFE.");
    if (mudconf.paranoid_alloc)
    notify(player, "The buffer pools are checked for consistency on each allocate or free.");
    notify(player,
       unsafe_tprintf("The %s cache is %d entries wide by %d entries deep.",
           CACHING, mudconf.cache_width, mudconf.cache_depth));
    if (mudconf.cache_names)
    notify(player, "A separate object name cache is used.");
    if (mudconf.cache_steal_dirty)
    notify(player, "Old modified cache entries may be reused when allocating a new entry.");
    if (mudconf.cache_trim)
    notify(player, "The cache depth is periodically trimmed back to its initial value.");
    if (mudconf.fork_dump) {
    notify(player, "Database dumps are performed by a fork()ed process.");
    if (mudconf.fork_vfork)
        notify(player, "The 'vfork()' call is used to perform the fork.");
    }
    if (mudconf.max_players >= 0) {
        if ( mudconf.max_players > mudstate.max_logins_allowed )
           mudconf.max_players = mudstate.max_logins_allowed;
    notify(player,
           unsafe_tprintf("There may be at most %d players logged in at once.",
               mudconf.max_players));
    }
/*  if (mudconf.quotas)
    sprintf(buff, " and %d quota", mudconf.start_quota);
    else
    *buff = '\0'; */
    notify(player,
       unsafe_tprintf("New players are given %d %s to start with.",
           mudconf.paystart, mudconf.many_coins));
    notify(player,
       unsafe_tprintf("Players are given %d %s each day they connect.",
           mudconf.paycheck, mudconf.many_coins));
    notify(player,
       unsafe_tprintf("Earning money is difficult if you have more than %d %s.",
           mudconf.paylimit, mudconf.many_coins));
    if (mudconf.payfind > 0)
    notify(player,
           unsafe_tprintf("Players have a 1 in %d chance of finding a %s each time they move.",
               mudconf.payfind, mudconf.one_coin));
    notify(player,
       unsafe_tprintf("The head of the object freelist is #%d.",
           mudstate.freelist));

    switch (mudconf.sig_action) {
    case SA_DFLT:
    notify(player, "Fatal signals are not caught.");
    break;
    case SA_RESIG:
    notify(player, "Fatal signals cause a panic dump and are resignalled from the signal handler.");
    break;
    case SA_RETRY:
    notify(player, "Fatal signals cause a panic dump and control returns to the point of signal.");
    break;
    }
    /* Global Room aconnect / adisconnect */
    if (mudconf.global_aconn > 0 )
        notify(player, "The master room recognizes @aconnects on objects inside it.");
    else
        notify(player, "@aconnects on objects inside the master room are ignored.");
    if (mudconf.global_adconn > 0 )
        notify(player, "The master room recognizes @adisconnects on objects inside it.");
    else
        notify(player, "@adisconnects on objects inside the master room are ignored.");
    /* Normal room: aconnect / adisconnect */
    if (mudconf.room_aconn > 0 )
        notify(player, "@aconnects will be triggered on rooms.");
    else
        notify(player, "@aconnects will not be triggered on rooms.");
    if (mudconf.room_adconn > 0 )
        notify(player, "@adisconnects will be triggered on rooms.");
    else
        notify(player, "@adisconnects will not be triggered on rooms.");

    notify(player, unsafe_tprintf("The total number of guests currently are %d.", mudconf.max_num_guests));

    if (mudconf.online_reg > 0 || mudconf.offline_reg > 0 ) {
       sprintf(buff, "Autoregistration:  As Guest...%s  Connect Screen...%s  Limit-Per-Connect: %d", 
               mudconf.online_reg > 0 ? "YES" : "NO",
               mudconf.offline_reg > 0 ? "YES" : "NO", mudconf.regtry_limit);
       notify(player,buff);
       notify(player, 
          unsafe_tprintf("                   Current mail used: %s.", mudconf.mailprog));
       if ( mudconf.mailsub > 0 )
          notify(player, "                   Subjects in mail are allowed.");
       else
          notify(player, "                   Subjects in mail are not allowed.");
       notify(player, unsafe_tprintf("                   Mail file included: %s.", mudconf.mailinclude_file));
    }
    else
       notify(player, "Autoregistration is currently disabled.");

    if (mudconf.wiz_override > 0 && mudconf.wiz_uselock > 0 )
       notify(player, "Wizards and Immortals override all locks.");
    else if ( mudconf.wiz_override == 0 )
       notify(player, "Wizards and Immortals must pass locks for access.");
    else
       notify(player, "Wizards and Immortals must pass uselocks (only) for access.");

    if (mudconf.allow_whodark > 0 && mudconf.player_dark == 0 && mudconf.who_unfindable == 0 )
       notify(player, "Players who are set DARK will not show up in a WHO listing.");

    if (mudconf.no_startup > 0 )
       notify(player, "STARTUP attributes were not triggered for this reboot cycle.");

    if (mudconf.newpass_god > 0 && Immortal(player))
       notify(player, "Immortals may @newpassword #1 ONCE this reboot cycle.");

    notify(player, unsafe_tprintf("Manual @log file currently is: %s.txt", mudconf.manlog_file));
    if ( mudconf.must_unlquota > 0 )
       notify(player, "Wizards must @quota/unlock a player before more @quota can be given.");
    sprintf(buff, "Intervals: Dump...%d  Clean...%d  Idlecheck...%d  Rwho...%d",
        mudconf.dump_interval, mudconf.check_interval,
        mudconf.idle_interval, mudconf.rwho_interval);
    notify(player, buff);
    sprintf(buff, "Timers: Dump...%.1f  Clean...%.1f  Idlecheck...%.1f  Rwho...%.1f",
        mudstate.dump_counter - now, mudstate.check_counter - now,
        mudstate.idle_counter - now, mudstate.rwho_counter - now);
    notify(player, buff);
    memset(buff, 0, MBUF_SIZE);
    sprintf(buff, "CPU Watchdogs:  CPU...%d%%  Elapsed Time...%d seconds", 
           //(mudconf.cpuintervalchk < 10 ? 10 : (mudconf.cpuintervalchk > 100 ? 100 : mudconf.cpuintervalchk)),
           //(mudconf.cputimechk < 10 ? 10 : (mudconf.cputimechk > 3600 ? 3600 : mudconf.cputimechk)) );
           (mudconf.cpuintervalchk > 100 ? 100 : mudconf.cpuintervalchk),
           (mudconf.cputimechk > 3600 ? 3600 : mudconf.cputimechk) );
    notify(player, buff);
    sprintf(buff, "                Security...Level %d  Max Sequential Slams...%d",
          (((mudconf.cpu_secure_lvl < 0) || (mudconf.cpu_secure_lvl > 5)) ? 0 : mudconf.cpu_secure_lvl),
          mudconf.max_cpu_cycles);
    notify(player, buff);
    sprintf(buff, "                Stack Ceiling: %d", mudconf.stack_limit);
    notify(player, buff);
    sprintf(buff, "                Spam Ceiling:  %d", mudconf.spam_limit);
    notify(player, buff);
    sprintf(buff, "                WildMatch Ceiling:  %d", mudconf.wildmatch_lim);
    notify(player, buff);
    sprintf(buff, "                SideFX Ceiling:  %d", mudconf.sidefx_maxcalls);
    notify(player, buff);
    sprintf(buff, "                Percent-Sub Ceiling:  %d", mudconf.max_percentsubs);
    notify(player, buff);
    sprintf(buff, "                Heavy Function CPU Recursor Limit...%d",
               mudconf.heavy_cpu_max);
    notify(player,  buff);
    sprintf(buff, "                @aahear/@ahear/@amhear/@listen Limit...%d/%ds",
               mudconf.ahear_maxcount, mudconf.ahear_maxtime);
    notify(player,  buff);
    if ( mudstate.curr_cpu_user != NOTHING ) {
       sprintf(buff, "                Current CPU Abuser...#%d  Total CPU Slams...%d",
               mudstate.curr_cpu_user, mudstate.curr_cpu_cycle);
    } else {
       sprintf(buff, "                Current CPU Abuser...(none)");
    }
    notify(player, buff);
    if ( mudstate.last_cpu_user != NOTHING ) {
       sprintf(buff, "                Last Punished CPU Abuser...#%d",
               mudstate.last_cpu_user);
       notify(player, buff);
    }
    if ( mudconf.idle_timeout == -1 )
       sprintf(buff, "Timeouts: Idle...Inf  Connect...%d  Tries...%d",
           mudconf.conn_timeout, mudconf.retry_limit);
    else
       sprintf(buff, "Timeouts: Idle...%d  Connect...%d  Tries...%d",
           mudconf.idle_timeout, mudconf.conn_timeout,
           mudconf.retry_limit);
    notify(player, buff);

    sprintf(buff, "Scheduling: Timeslice...%d  Max_Quota...%d  Increment...%d",
        mudconf.timeslice, mudconf.cmd_quota_max,
        mudconf.cmd_quota_incr);
    notify(player, buff);

    sprintf(buff, "Compression: Strings...%s  Spaces...%s  Savefiles...%s",
        ed[mudconf.intern_comp], ed[mudconf.space_compress],
        ed[mudconf.compress_db]);
    notify(player, buff);

    sprintf(buff, "New characters: Room...#%d  Home...#%d  DefaultHome...#%d  Quota...%d  Guild...%-11.11s",
        mudconf.start_room, mudconf.start_home, mudconf.default_home,
        mudconf.start_quota, strip_ansi(mudconf.guild_default));
    notify(player, buff);

    sprintf(buff, "Rwho: Host...%s  Port...%d  Connection...%s  Transmit...%s",
        mudconf.rwho_host, mudconf.rwho_info_port,
        ud[mudstate.rwho_on], ed[mudconf.rwho_transmit]);
    notify(player, buff);

    sprintf(buff, "Misc: IdleQueueChunk...%d  ActiveQueueChunk...%d  Master_room...#%d",
        mudconf.queue_chunk,
        mudconf.active_q_chunk, 
        mudconf.master_room);
    notify(player, buff);

       if(mudconf.pagecost <= 0)
           sprintf(buff, "Costs: Wall...%d  Page...Free  Com...%d", mudconf.wall_cost, mudconf.comcost);
       else
           sprintf(buff, "Costs: Wall...%d  Page...1/%d length (round up)  Com...%d", mudconf.wall_cost, (mudconf.pagecost), mudconf.comcost);
    notify(player, buff);
    sprintf(buff, "Configure Options: FailConnect Spam Protection...%s   'R' in WHO...%s   " \
                  "\r\n                   Maximum Concurrent Site-Connections...%d",
                  (mudconf.nospam_connect ? "Yes" : "No"), (mudconf.noregist_onwho ? "No" : "Yes"),
                  mudconf.max_sitecons);
    notify(player, buff);
    sprintf(buff, "                   Maximum Lines for normal .txt files...%d", 
                  mudconf.nonindxtxt_maxlines);
    notify(player, buff);

    memset(newstime, 0, sizeof(newstime));
    memset(mailtime, 0, sizeof(mailtime));
    memset(aregtime, 0, sizeof(aregtime));
    memset(mushtime, 0, sizeof(mushtime));
    strncpy(newstime,(char *) ctime(&mudstate.newsflat_time), 24);
    strncpy(mailtime,(char *) ctime(&mudstate.mailflat_time), 24);
    strncpy(aregtime,(char *) ctime(&mudstate.aregflat_time), 24);
    strncpy(mushtime,(char *) ctime(&mudstate.mushflat_time), 24);

    sprintf(buff, 
            "Dumptimes: Mush...%s   Mail...%s\r\n           News...%s   Areg...%s",
            (mudstate.start_time == mudstate.mushflat_time  ? "[No Previous Dump]      " : 
               (char *) mushtime ),
            (mudstate.mail_state == 2 ? "[Mail system disabled]  " : 
               (mudstate.start_time == mudstate.mailflat_time  ? "[No Previous Dump]      " : 
                  (char *) mailtime )),
            (news_system_active == 0 ? "[News system disabled]  " :
               (mudstate.start_time == mudstate.newsflat_time ? "[No Previous Dump]      " : 
                  (char *) newstime )),
            (mudstate.start_time == mudstate.aregflat_time ? "[No Previous Dump]      " : 
               (char *) aregtime ));
    notify(player, buff);

/* MAX_ARGS is defined in externs.h - default 30 */
#ifdef MAX_ARGS
#define NFARGS  MAX_ARGS
#else
#define NFARGS 30
#endif
    sprintf(buff, "Limits: Trace(top-down)...%d  Retry...%d  Pay...%d\r\n" \
                  "        Function_nest...%d Function_invoke...%d  Lock_nest...%d",
       mudconf.trace_limit, mudconf.retry_limit, mudconf.paylimit, 
           (mudconf.func_nest_lim > 300 ? 300 : mudconf.func_nest_lim),
       mudconf.func_invk_lim, mudconf.lock_nest_lim);
    notify(player, buff);
    buff3 = alloc_sbuf("list_config_temp");
    sprintf(buff3, "%d", mudconf.max_pcreate_lim);
    sprintf(buff, "        Parent_nest...%d  User_attrs...%d  Function_Args...%d\r\n" \
                  "        Max sitecons...%d/%ds   Max site pcreates...%s/%ds",
           mudconf.parent_nest_lim, mudconf.vlimit, NFARGS,
           mudconf.max_lastsite_cnt, mudconf.min_con_attempt,
           (mudconf.max_pcreate_lim == -1 ? "Inf" : buff3), mudconf.max_pcreate_time);
    notify(player, buff);
    free_sbuf(buff3);
    sprintf(buff, "        Sitecon paranoia level...%d   PCreate paranoia level...%d\r\n" \
                  "        Max VAttrs...%d  Max @destroy...%d",
           mudconf.lastsite_paranoia, mudconf.pcreate_paranoia,
           mudconf.max_vattr_limit, mudconf.max_dest_limit);
    notify(player, buff);
    sprintf(buff, "        Max Wiz VAttrs...%d  Max Wiz @destroy...%d  Wiz Check...%s",
           mudconf.wizmax_vattr_limit, mudconf.wizmax_dest_limit, 
           (mudconf.vattr_limit_checkwiz ? "enabled" : "disabled") );
    sprintf(buff, "        Max Players Allowed: %d (%s)", 
           (mudconf.max_players > 0 ? mudconf.max_players : mudstate.max_logins_allowed),
           (mudconf.max_players > 0 ? "User Defined" : "Max Free OS Descriptors"));
    notify(player, buff);

    free_mbuf(buff);
    DPOP; /* #47 */
}

/* ---------------------------------------------------------------------------
 * list_vattrs: List user-defined attributes
 */

static void 
list_vattrs(dbref player, char *s_mask, int wild_mtch)
{
    VATTR *va;
    int na, wna, i_searchtype, i_flags;
    char *buff, *buffptr;
    NAMETAB  *ntptr;

    DPUSH; /* #48 */

    wna = i_searchtype = i_flags = 0;
    buff = alloc_lbuf("list_vattrs");
    if ( wild_mtch ) {
       switch(*s_mask) {
          case '|' : i_searchtype = 2;
                     break;
          case '&' : i_searchtype = 3;
                     break;
          default  : i_searchtype = 1;
                     break;
       }
       if ( i_searchtype > 1 ) {
          buffptr = s_mask;
          while ( *buffptr ) {
             *buffptr = ToLower(*buffptr);
             buffptr++;
          }
          ntptr = attraccess_nametab;
          while ( ntptr->name ) {
             if ( God(player) || check_access(player, ntptr->perm, ntptr->perm2, 0) ) {
                if ( *(s_mask+1) && strstr(s_mask+1, ntptr->name) ) {
                   i_flags |= ntptr->flag;
                }
             }
             ntptr++;
          }
       }
       notify(player, "--- User-Defined Attributes (wildmatched) ---");
    } else {
       notify(player, "--- User-Defined Attributes ---");
    }
    for (va = vattr_first(), na = 0; va; va = vattr_next(va), na++) {
    if (!(va->flags & AF_DELETED)) {
            if ( wild_mtch ) {
                if ((i_searchtype == 1) && s_mask && *s_mask && !quick_wild(s_mask, (char *)va->name))
                   continue;
                else if ((i_searchtype == 2) && !(va->flags & i_flags) ) 
                   continue;
                else if ((i_searchtype == 3) && i_flags && (va->flags != (va->flags | i_flags)) )
                   continue;
            } 
            wna++;
        sprintf(buff, "%s(%d):", va->name, va->number);
        listset_nametab(player, attraccess_nametab, 0, va->flags, 0,
                buff, 1);
    }
    }

    if ( wild_mtch ) {
       notify(player, unsafe_tprintf("%d attributes matched, %d attributes total, next=%d",
                  wna, na, mudstate.attr_next));
    } else {
       notify(player, unsafe_tprintf("%d attributes, next=%d",
                  na, mudstate.attr_next));
    }
    free_lbuf(buff);

    DPOP; /* #48 */
}

/* ---------------------------------------------------------------------------
 * list_hashstats: List information from hash tables
 */

static void 
list_hashstat(dbref player, const char *tab_name, HASHTAB * htab)
{
    char *buff;

    DPUSH; /* #49 */

    buff = hashinfo(tab_name, htab);
    notify(player, buff);
    free_mbuf(buff);
    DPOP; /* #49 */
}

static void 
list_nhashstat(dbref player, const char *tab_name, NHSHTAB * htab)
{
    char *buff;

    DPUSH; /* #50 */

    buff = nhashinfo(tab_name, htab);
    notify(player, buff);
    free_mbuf(buff);

    DPOP; /* #50 */
}

static void 
list_hashstats(dbref player)
{
    DPUSH; /* #51 */
    notify(player, "Hash Stats      Size Entries Deleted   Empty Lookups    Hits  Checks Longest");
    list_hashstat(player, "Commands", &mudstate.command_htab);
    list_hashstat(player, "Alias", &mudstate.cmd_alias_htab);
    list_hashstat(player, "Logged-out Cmds", &mudstate.logout_cmd_htab);
    list_hashstat(player, "Functions", &mudstate.func_htab);
    list_hashstat(player, "User-Functions", &mudstate.ufunc_htab);
    list_hashstat(player, "Local-Functions", &mudstate.ulfunc_htab);
    list_hashstat(player, "Flags", &mudstate.flags_htab);
    list_hashstat(player, "Attr names", &mudstate.attr_name_htab);
    list_nhashstat(player, "Attr numbers", &mudstate.attr_num_htab);
    list_hashstat(player, "Player Names", &mudstate.player_htab);
    list_nhashstat(player, "Net Descriptors", &mudstate.desc_htab);
    list_nhashstat(player, "Forwardlists", &mudstate.fwdlist_htab);
    list_nhashstat(player, "Overlaid $-cmds", &mudstate.parent_htab);
    list_hashstat(player, "News topics", &mudstate.news_htab);
    list_hashstat(player, "Help topics", &mudstate.help_htab);
    list_hashstat(player, "Wizhelp topics", &mudstate.wizhelp_htab);
    list_hashstat(player, "Error topics", &mudstate.error_htab);
#ifdef PLUSHELP
    list_hashstat(player, "PlusHelp topics", &mudstate.plushelp_htab);
#endif
#ifdef ZENTY_ANSI
    list_hashstat(player, "256 ANSI Colors", &mudstate.ansi_htab);
#endif
    list_vhashstats(player);
    DPOP; /* #51 */
}

/* These are from 'udb_cache.c'. */
extern time_t cs_ltime;
extern int cs_writes;       /* total writes */
extern int cs_reads;        /* total reads */
extern int cs_dbreads;      /* total read-throughs */
extern int cs_dbwrites;     /* total write-throughs */
extern int cs_dels;     /* total deletes */
extern int cs_checks;       /* total checks */
extern int cs_rhits;        /* total reads filled from cache */
extern int cs_ahits;        /* total reads filled active cache */
extern int cs_whits;        /* total writes to dirty cache */
extern int cs_fails;        /* attempts to grab nonexistent */
extern int cs_resets;       /* total cache resets */
extern int cs_syncs;        /* total cache syncs */
extern int cs_objects;      /* total cache size */

/* ---------------------------------------------------------------------------
 * list_db_stats: Get useful info from the DB layer about hash stats, etc.
 */

static void 
list_db_stats(dbref player)
{
    DPUSH; /* #52 */
    notify(player,
       unsafe_tprintf("DB Cache Stats   Writes       Reads  (over %d seconds)",
           time(0) - cs_ltime));
    notify(player, unsafe_tprintf("Calls      %12d%12d", cs_writes, cs_reads));
    notify(player, unsafe_tprintf("Cache Hits %12d%12d  (%d in active cache)",
               cs_whits, cs_rhits, cs_ahits));
    notify(player, unsafe_tprintf("I/O        %12d%12d",
               cs_dbwrites, cs_dbreads));
    notify(player, unsafe_tprintf("\r\nDeletes    %12d", cs_dels));
    notify(player, unsafe_tprintf("Checks     %12d", cs_checks));
    notify(player, unsafe_tprintf("Resets     %12d", cs_resets));
    notify(player, unsafe_tprintf("Syncs      %12d", cs_syncs));
    notify(player, unsafe_tprintf("Cache Size %12d", cs_objects));
    DPOP; /* 52 */
}


static void
list_functionperms(dbref player, char *s_mask, int key)
{
   FUN *fp = NULL;
   UFUN *ufp = NULL,
       *ulfp = NULL;
   char *s_buf;
   int chk_tog = 0;

   if ( Good_obj(player) && Immortal(player) )
      chk_tog = 1;

   if ( key ) {
       notify(player, "--- Function Permissions (wildmatched) ---");
   } else {
       notify(player, "--- Function Permissions ---");
   }
   s_buf = alloc_sbuf("list_functionperms");
   for (fp = (FUN *) hash_firstentry2(&mudstate.func_htab, 1); fp;
        fp = (FUN *) hash_nextentry(&mudstate.func_htab)) {
      if ( chk_tog || check_access(player, fp->perms, fp->perms2, 0)) {
         if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)fp->name)) ) {
            sprintf(s_buf, "%-.31s:", fp->name);
            listset_nametab(player, access_nametab, access_nametab2, fp->perms, fp->perms2, s_buf, 1);
         }
      }
   }
   if ( key ) {
       notify(player, "--- User-Function Permissions (wildmatched) ---");
   } else {
       notify(player, "--- User-Function Permissions ---");
   }
   for (ufp = (UFUN *) hash_firstentry2(&mudstate.ufunc_htab, 1); ufp;
        ufp = (UFUN *) hash_nextentry(&mudstate.ufunc_htab)) {
      if ( chk_tog || check_access(player, ufp->perms, ufp->perms2, 0)) {
         if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)ufp->name)) ) {
            sprintf(s_buf, "%-.31s:", ufp->name);
            listset_nametab(player, access_nametab, access_nametab2, ufp->perms, ufp->perms2, s_buf, 1);
         }
      }
   }
   if ( key ) {
       notify(player, "--- LOCAL User-Function Permissions (wildmatched) ---");
   } else {
       notify(player, "--- LOCAL User-Function Permissions ---");
   }
   for (ulfp = (UFUN *) hash_firstentry2(&mudstate.ulfunc_htab, 1); ulfp;
        ulfp = (UFUN *) hash_nextentry(&mudstate.ulfunc_htab)) {
      if ( chk_tog || check_access(player, ulfp->perms, ulfp->perms2, 0)) {
         if ( !key || (key && s_mask && *s_mask && quick_wild(s_mask, (char *)ulfp->name)) ) {
            sprintf(s_buf, "%-.31s:[#%d]", ulfp->name, ulfp->owner);
            listset_nametab(player, access_nametab, access_nametab2, ulfp->perms, ulfp->perms2, s_buf, 1);
         }
      }
   }
   free_sbuf(s_buf);
}

static void
list_logcommands(dbref player)
{
   char tbuff[1000], *strtok_tbuff;
   char tbuff1[2][40], tbuff2[80];
   int i_cntr;

   DPUSH; /* #53 */
   /* These both are 1000 */
   strcpy(tbuff, mudconf.log_command_list);
   i_cntr = 0; 
   notify(player, "----------------------------------------"\
                  "----------------------------------------");
   memset(tbuff1, 0, sizeof(tbuff1));
   strtok_tbuff = strtok(tbuff, " \t");
   while ( strtok_tbuff != NULL ) {
      if ( (i_cntr != 0) && ((i_cntr % 2) == 0) ) {
         sprintf(tbuff2, "%-.39s %-.39s", tbuff1[0], tbuff1[1]);
         notify(player, tbuff2);
      }
      sprintf(tbuff1[i_cntr % 2], "%-38.38s ", strtok_tbuff);
      strtok_tbuff = strtok(NULL, " \t");
      i_cntr++;
   }
   if ( i_cntr == 0 )
      notify(player, "                                 (NONE DEFINED)");
   else {
      if ( (i_cntr % 2) == 0 ) {
         sprintf(tbuff2, "%-.39s %-.39s", tbuff1[0], tbuff1[1]);
         notify(player, tbuff2);
      } else if ( (i_cntr % 2) != 0 )
         notify(player, tbuff1[0]);
   }
   notify(player, "----------------------------------------"\
                  "----------------------------------------");
   notify(player, unsafe_tprintf("Total Commands Logged: %d", i_cntr));
   DPOP; /* #53 */
}

static void
list_guestparse(dbref player)
{
   char *instrptr, instr[1001], chrbuff[15], guestbuff[10], dodbref[40], tonchr;
   char *tpr_buff, *tprp_buff;
   int cntr; 
   dbref validplr;
  
   memset(instr, 0, sizeof(instr));
   strncpy(instr, mudconf.guest_namelist, 1000);
   tprp_buff = tpr_buff = alloc_lbuf("list_guestparse");
   if ( Guildmaster(player) ) { 
      notify(player, unsafe_tprintf("--------------- Total Current Guests Allowed: %d", 
             mudconf.max_num_guests));
      notify(player, unsafe_tprintf("%-15.15s %-20.20s %-s", "Guest Number", "Guest Name", 
                     "Valid     Guest/Player           On"));
      notify(player, unsafe_tprintf("%-15.15s %-20.20s %-s", "---------------", "--------------------", 
                     "----------------------------------------"));
      cntr = 1;
      instrptr = strtok(instr, " \t");
      while ( instrptr != NULL && (cntr < 32 )) {
         sprintf(chrbuff, "Guest #%d", cntr);
         validplr = lookup_player(player, instrptr, 0);
         if ( validplr != NOTHING && Good_obj(validplr) ) {
            if ( Connected(validplr) )
               tonchr = '*';
            else
               tonchr = ' ';
            if (!check_pass(validplr, "guest", 0)) {
               sprintf(dodbref, "%2s/%-7s DBRef #%-10d     %c", "No", "passwd", validplr, tonchr);
            } else if ( !Guest(validplr) ) {
               sprintf(dodbref, "%2s/%-7s DBRef #%-10d     %c", "No", "!guest", validplr, tonchr);
            } else {
               sprintf(dodbref, "%-10s DBRef #%-10d     %c", "Yes", validplr, tonchr);
            }
            tprp_buff = tpr_buff;
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s %-s", chrbuff, Name(validplr), dodbref));
         } else {
            tprp_buff = tpr_buff;
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s %-s", chrbuff, instrptr, "No"));
         }
         cntr++;
         instrptr = strtok(NULL, " \t");
      }
      if ( cntr <= mudconf.max_num_guests ) {
         while ( (cntr <= mudconf.max_num_guests) && (cntr < 32) ) {
            sprintf(chrbuff, "Guest #%d", cntr);
            sprintf(guestbuff, "Guest%d", cntr);
            validplr = lookup_player(player, guestbuff, 0);
            if ( validplr != NOTHING && Good_obj(validplr) ) {
               if ( Connected(validplr) )
                  tonchr = '*';
               else
                  tonchr = ' ';
               if (!check_pass(validplr, "guest", 0)) {
                  sprintf(dodbref, "%2s/%-7s DBRef #%-10d     %c", "No", "passwd", validplr, tonchr);
               } else if ( !Guest(validplr) ) {
                  sprintf(dodbref, "%2s/%-7s DBRef #%-10d     %c", "No", "!guest", validplr, tonchr);
               } else {
                  sprintf(dodbref, "%-10s DBRef #%-10d     %c", "Yes", validplr, tonchr);
               }
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s %-s", chrbuff, Name(validplr), dodbref));
            } else {
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s %-s", chrbuff, guestbuff, "No"));
            }
            cntr++;
         }
      }
      notify(player, unsafe_tprintf("%-15.15s %-20.20s %-s", "---------------", "--------------------", 
                     "----------------------------------------"));
   } else {
      notify(player, unsafe_tprintf("%-15.15s %-20.20s", "Guest Number", "Guest Name"));
      notify(player, unsafe_tprintf("%-15.15s %-20.20s", "---------------", "--------------------"));
      cntr = 1;
      instrptr = strtok(instr, " \t");
      while ( instrptr != NULL && (cntr < 32 )) {
         sprintf(chrbuff, "Guest #%d", cntr);
         validplr = lookup_player(player, instrptr, 0);
         if ( validplr != NOTHING && Good_obj(validplr) ) {
            tprp_buff = tpr_buff;
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s", chrbuff, Name(validplr)));
         } else {
            tprp_buff = tpr_buff;
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s", chrbuff, instrptr));
         }
         cntr++;
         instrptr = strtok(NULL, " \t");
      }
      if ( cntr <= mudconf.max_num_guests ) {
         while ( (cntr <= mudconf.max_num_guests) && (cntr < 32) ) {
            sprintf(chrbuff, "Guest #%d", cntr);
            sprintf(guestbuff, "Guest%d", cntr);
            validplr = lookup_player(player, guestbuff, 0);
            if ( validplr != NOTHING && Good_obj(validplr) ) {
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s", chrbuff, Name(validplr)));
            } else {
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-15.15s %-20.20s", chrbuff, guestbuff));
            }
            cntr++;
         }
      }
      notify(player, unsafe_tprintf("%-15.15s %-20.20s", "---------------", "--------------------"));
   }
   free_lbuf(tpr_buff);
   if ( instrptr != NULL && Wizard(player)) {
      notify(player, "WARNING: Too many guests defined in param guest_namelist.");
   } 
   if ( (mudconf.max_num_guests > 31) && Wizard(player)) {
      notify(player, "WARNING: Too many guests defined in param num_guests.");
   } 
}

/* ---------------------------------------------------------------------------
 * list_process: List local resource usage stats of the mush process.
 * Adapted from code by Claudius@PythonMUCK,
 *     posted to the net by Howard/Dark_Lord.
 */

static void 
list_process(dbref player)
{
    int pid, psize;

#ifdef HAVE_GETRUSAGE
    struct rusage usage;

    DPUSH; /* #54 */

    getrusage(RUSAGE_SELF, &usage);
    /* Calculate memory use from the aggregate totals */

    DPOP; /* #54 */
#endif

    DPUSH; /* #55 */

    pid = getpid();
#if 0  /* this was causing problems on solaris - Thorin */
    psize = getpagesize();
#else
    psize = 0;
#endif

    /* Go display everything */

    notify(player,
       unsafe_tprintf("Process ID:  %10d        %10d bytes per page",
           pid, psize));
#ifdef HAVE_GETRUSAGE
    notify(player,
       unsafe_tprintf("Time used:   %10d user   %10d sys",
           usage.ru_utime.tv_sec, usage.ru_stime.tv_sec));
/*      notify(player,
   unsafe_tprintf("Resident mem:%10d shared %10d private%10d stack",
   ixrss, idrss, isrss));
 */
    notify(player,
       unsafe_tprintf("Integral mem:%10d shared %10d private%10d stack",
           usage.ru_ixrss, usage.ru_idrss, usage.ru_isrss));
    notify(player,
       unsafe_tprintf("Max res mem: %10d pages  %10d bytes",
           usage.ru_maxrss, (usage.ru_maxrss * psize)));
    notify(player,
       unsafe_tprintf("Page faults: %10d hard   %10d soft   %10d swapouts",
           usage.ru_majflt, usage.ru_minflt, usage.ru_nswap));
    notify(player,
       unsafe_tprintf("Disk I/O:    %10d reads  %10d writes",
           usage.ru_inblock, usage.ru_oublock));
    notify(player,
       unsafe_tprintf("Network I/O: %10d in     %10d out",
           usage.ru_msgrcv, usage.ru_msgsnd));
    notify(player,
       unsafe_tprintf("Context swi: %10d vol    %10d forced %10d sigs",
           usage.ru_nvcsw, usage.ru_nivcsw, usage.ru_nsignals));
#endif
    DPOP; /* #55 */
}
#ifdef REALITY_LEVELS
/* ---------------------------------------------------------------------------
 * list_rlevels: List defined reality levels
 */
static void
list_rlevels(dbref player, int i_key)
{
    int i, cmp_x, cmp_y, cmp_z;
    char *tpr_buff, *tprp_buff;
     
    DPUSH; /* #56 */
    cmp_x = sizeof( mudconf.reality_level );
    cmp_y = sizeof( mudconf.reality_level[0] );
    if ( cmp_y == 0 )
       cmp_z = 0;
    else
       cmp_z = cmp_x / cmp_y;
    notify(player, unsafe_tprintf("Reality levels: (%d configured)", mudconf.no_levels));
    notify(player, "---------------------------------------"\
                   "---------------------------------------");
/*  if ( cmp_z != mudconf.no_levels ) { */
    if ( 0 ) {
       notify(player, unsafe_tprintf("Error in reality levels: %d found/%d total.",
                      cmp_z, mudconf.no_levels));
    } else {
       tprp_buff = tpr_buff = alloc_lbuf("list_rlevels");
       if ( i_key ) {
          for(i = 0; (i < mudconf.no_levels) && (i < cmp_z); ++i) {
              tprp_buff = tpr_buff;
              notify(player, safe_tprintf(tpr_buff, &tprp_buff, "    Level: %-20s Value: %10u   Desc: %s",
                  mudconf.reality_level[i].name, mudconf.reality_level[i].value,
                  mudconf.reality_level[i].attr));
          }
       } else {
          for(i = 0; (i < mudconf.no_levels) && (i < cmp_z); ++i) {
              tprp_buff = tpr_buff;
              notify(player, safe_tprintf(tpr_buff, &tprp_buff, "    Level: %-20s Value: 0x%08x   Desc: %s",
                  mudconf.reality_level[i].name, mudconf.reality_level[i].value,
                  mudconf.reality_level[i].attr));
          }
       }
       free_lbuf(tpr_buff);
    }
    if ( mudconf.reality_locks ) {
       notify(player, unsafe_tprintf("\r\n    Enhancement: @lock/user works as a Reality Lock (type %d).", 
                      mudconf.reality_locktype));
    }
    if ( !mudconf.reality_locks && mudconf.reality_compare) {
       notify(player, "  ");
    }
    switch (mudconf.reality_compare) {
       case 0: /* Do nothing */
               break;
       case 1: notify(player, "    Enhancement: Descs linearly in reverse, all shown.");
               break;
       case 2: notify(player, "    Enhancement: Descs linearly in order, first existing desc shown.");
               break;
       case 3: notify(player, "    Enhancement: Descs linearly in reverse, first existing desc shown.");
               break;
       case 4: notify(player, "    Enhancement: Descs linearly in order, first reality shown.");
               break;
       case 5: notify(player, "    Enhancement: Descs linearly in reverse, first reality shown.");
               break;
    }
    notify(player, "---------------------------------------"\
                   "---------------------------------------");
    DPOP; /* #56 */
}
#endif /* REALITY_LEVELS */

/* ---------------------------------------------------------------------------
 * do_list: List information stored in internal structures.
 */

#define LIST_ATTRIBUTES 1
#define LIST_COMMANDS   2
#define LIST_COSTS  3
#define LIST_FLAGS  4
#define LIST_FUNCTIONS  5
#define LIST_GLOBALS    6
#define LIST_ALLOCATOR  7
#define LIST_LOGGING    8
#define LIST_DF_FLAGS   9
#define LIST_PERMS  10
#define LIST_ATTRPERMS  11
#define LIST_OPTIONS    12
#define LIST_HASHSTATS  13
#define LIST_BUFTRACE   14
#define LIST_CONF_PERMS 15
#define LIST_SITEINFO   16
#define LIST_POWERS 17
#define LIST_SWITCHES   18
#define LIST_VATTRS 19
#define LIST_DB_STATS   20  /* GAC 4/6/92 */
#define LIST_PROCESS    21
#define LIST_BADNAMES   22
#define LIST_TOGGLES    23
#define LIST_MAILGBL    24
#define LIST_GUESTS     25  /* ASH 9/17/00 */
#ifdef REALITY_LEVELS
#define LIST_RLEVELS    26
#endif /* REALITY_LEVELS */
#define LIST_LOGCOMMANDS 27	
#define LIST_DEPOWERS	28
#define LIST_STACKS	29
#define LIST_FUNPERMS	30
#define	LIST_DF_TOGGLES	31
#define LIST_BUFTRACEADV 32

NAMETAB list_names[] =
{
    {(char *) "allocations", 2, CA_WIZARD, 0, LIST_ALLOCATOR},
    {(char *) "attr_permissions", 5, CA_WIZARD, 0, LIST_ATTRPERMS},
    {(char *) "attributes", 2, CA_PUBLIC, 0, LIST_ATTRIBUTES},
    {(char *) "bad_names", 2, CA_WIZARD, 0, LIST_BADNAMES},
    {(char *) "buffers", 2, CA_WIZARD, 0, LIST_BUFTRACE},
    {(char *) "advbuffers", 3, CA_IMMORTAL, 0, LIST_BUFTRACEADV},
    {(char *) "commands", 3, CA_PUBLIC, 0, LIST_COMMANDS},
    {(char *) "config_permissions", 3, CA_IMMORTAL, 0, LIST_CONF_PERMS},
    {(char *) "costs", 3, CA_PUBLIC, 0, LIST_COSTS},
    {(char *) "db_stats", 2, CA_WIZARD, 0, LIST_DB_STATS},
    {(char *) "default_flags", 1, CA_PUBLIC, 0, LIST_DF_FLAGS},
    {(char *) "default_toggles", 1, CA_PUBLIC, 0, LIST_DF_TOGGLES},
    {(char *) "flags", 2, CA_PUBLIC, 0, LIST_FLAGS},
    {(char *) "functions", 4, CA_PUBLIC, 0, LIST_FUNCTIONS},
    {(char *) "globals", 2, CA_WIZARD, 0, LIST_GLOBALS},
    {(char *) "hashstats", 1, CA_WIZARD, 0, LIST_HASHSTATS},
    {(char *) "logging", 1, CA_IMMORTAL, 0, LIST_LOGGING},
    {(char *) "options", 1, CA_PUBLIC, 0, LIST_OPTIONS},
    {(char *) "permissions", 2, CA_WIZARD, 0, LIST_PERMS},
    {(char *) "process", 2, CA_WIZARD, 0, LIST_PROCESS},
    {(char *) "site_information", 2, CA_WIZARD, 0, LIST_SITEINFO},
    {(char *) "switches", 2, CA_PUBLIC, 0, LIST_SWITCHES},
    {(char *) "user_attributes", 1, CA_WIZARD, 0, LIST_VATTRS},
    {(char *) "toggles", 1, CA_PUBLIC, 0, LIST_TOGGLES},
    {(char *) "mail_globals", 1, CA_PUBLIC, 0, LIST_MAILGBL},
    {(char *) "guest_names", 2, CA_PUBLIC, 0, LIST_GUESTS},
#ifdef REALITY_LEVELS
    {(char *) "rlevels", 1, CA_PUBLIC, 0, LIST_RLEVELS},
#endif /* REALITY_LEVELS */
    {(char *) "cmdslogged", 3, CA_IMMORTAL, 0, LIST_LOGCOMMANDS},
    {(char *) "powers", 3, CA_WIZARD, 0, LIST_POWERS},
    {(char *) "depowers", 3, CA_IMMORTAL, 0, LIST_DEPOWERS},
    {(char *) "stacks", 3, CA_IMMORTAL, 0, LIST_STACKS},
    {(char *) "funperms", 4, CA_IMMORTAL, 0, LIST_FUNPERMS},
    {NULL, 0, 0, 0, 0}};

extern NAMETAB enable_names[];
extern NAMETAB logoptions_nametab[];
extern NAMETAB logdata_nametab[];

void 
do_list(dbref player, dbref cause, int extra, char *arg)
{
    int flagvalue, p_val, i;
    char *s_ptr, *s_ptr2, *tpr_buff, *tprp_buff, *save_buff;

    DPUSH; /* #57 */

    s_ptr = s_ptr2 = (char *)NULL;
    s_ptr = arg;
    while ( *s_ptr && *s_ptr != ' ' )
       s_ptr++; 
    while ( *s_ptr && *s_ptr == ' ' )
       s_ptr++; 
    save_buff = alloc_lbuf("do_list_save");
    sprintf(save_buff, "%.3900s", s_ptr);
    s_ptr = strtok(arg, " ");
    if ( s_ptr )
       s_ptr2 = strtok(NULL, " ");
    flagvalue = search_nametab(player, list_names, s_ptr);
    switch (flagvalue) {
    case LIST_ALLOCATOR:
    list_bufstats(player);
        notify(player, unsafe_tprintf("\r\nTotal Lbufs used in Q-Regs: %d", (MAX_GLOBAL_REGS * 2)));
        notify(player, unsafe_tprintf("Total Sbufs used in Q-Regs: %d", MAX_GLOBAL_REGS));
#ifndef NODEBUGMONITOR
        notify(player, unsafe_tprintf("Highest debugmon stack depth was: %d", debugmem->stackval));
        notify(player, unsafe_tprintf("Current debugmon stack depth is: %d", debugmem->stacktop));
#else
    notify(player, "Debug Monitor is disabled.");
#endif
    break;
    case LIST_BUFTRACE:
	list_buftrace(player, 0);
	break;
    case LIST_BUFTRACEADV:
	list_buftrace(player, 1);
	break;
    case LIST_ATTRIBUTES:
    list_attrtable(player);
    break;
    case LIST_COMMANDS:
    list_cmdtable(player);
    break;
    case LIST_SWITCHES:
    list_cmdswitches(player, s_ptr2, ((s_ptr2 && *s_ptr2) ? 1 : 0));
    break;
    case LIST_COSTS:
    list_costs(player);
    break;
    case LIST_OPTIONS:
        if ( s_ptr2 && *s_ptr2 ) {
           if ( stricmp(s_ptr2, "mail") == 0 )
              list_options_mail(player);
           else if ( (stricmp(s_ptr2, "config") == 0) )
              list_options_config(player);
           else if ( (stricmp(s_ptr2, "system") == 0) )
              list_options_system(player);
           else if ( (stricmp(s_ptr2, "convtime") == 0) ) {
              if ( !(mudconf.enhanced_convtime) )
                 notify(player, "Extended convtime() is not available.");
              else
                 list_options_convtime(player);
           } else if ( stricmp(s_ptr2, "mysql") == 0 ) {
              if ( Wizard(player) )
                 list_options_mysql(player);
              else
                 notify(player, "MySQL has been enabled.");
           } else if ( stricmp(s_ptr2, "boolean") == 0 ) {
              s_ptr = strtok(NULL, " ");
              if ( s_ptr && is_integer(s_ptr) ) {
                 p_val = atoi(s_ptr);
                 s_ptr = (char *)NULL;
              } else
                 p_val = 0;
              list_options_boolean_parse(player, p_val, s_ptr);
           } else if ( stricmp(s_ptr2, "values") == 0 ) {
              s_ptr = strtok(NULL, " ");
              if ( s_ptr && is_integer(s_ptr) ) {
                 p_val = atoi(s_ptr);
                 s_ptr = (char *)NULL;
              } else
                 p_val = 0;
              list_options_values_parse(player, p_val, s_ptr);
           } else
              notify_quiet(player, "Unknown sub-option for OPTIONS.  Use one of:"\
                                   " mail, values, boolean, config, system, mysql, convtime");
        } else {
       list_options(player);
        }
    break;
    case LIST_HASHSTATS:
    list_hashstats(player);
    break;
    case LIST_SITEINFO:
    list_siteinfo(player);
    break;
    case LIST_FLAGS:
    display_flagtab(player);
    break;
    case LIST_TOGGLES:
    display_toggletab(player);
    break;
    case LIST_FUNCTIONS:
        if ( s_ptr2 && *s_ptr2 && 
             !( (stricmp(s_ptr2, "built-in") == 0) ||
                (stricmp(s_ptr2, "user") == 0) ||
                (stricmp(s_ptr2, "local") == 0) ) ) {
           notify_quiet(player, "Unknown sub-option for FUNCTIONS.  Use one of: "\
                                "built-in, user, local");
        } else {
	   list_functable(player, s_ptr2);
        }
	break;
    case LIST_GLOBALS:
    interp_nametab(player, enable_names, mudconf.control_flags,
               (char *) "Global parameters:", (char *) "enabled",
               (char *) "disabled");
    break;
    case LIST_DF_TOGGLES:
    list_df_toggles(player);
    break;
    case LIST_DF_FLAGS:
    list_df_flags(player);
    break;
    case LIST_PERMS:
    list_cmdaccess(player, s_ptr2, ((s_ptr2 && *s_ptr2) ? 1 : 0));
    break;
    case LIST_CONF_PERMS:
        list_cf_access(player, s_ptr2, ((s_ptr2 && *s_ptr2) ? 1 : 0));
    break;
    case LIST_POWERS:
        list_allpower(player, 0);
    break;
    case LIST_DEPOWERS:
        list_allpower(player, 1);
        break;
    case LIST_ATTRPERMS:
    list_attraccess(player, s_ptr2, ((s_ptr2 && *s_ptr2) ? 1 : 0));
    break;
    case LIST_VATTRS:
    list_vattrs(player, save_buff, ((s_ptr2 && *s_ptr2) ? 1 : 0));
    break;
    case LIST_LOGGING:
    interp_nametab(player, logoptions_nametab, mudconf.log_options,
               (char *) "Events Logged:", (char *) "enabled",
               (char *) "disabled");
    interp_nametab(player, logdata_nametab, mudconf.log_info,
               (char *) "Information Logged:", (char *) "yes",
               (char *) "no");
    break;
    case LIST_DB_STATS:
    list_db_stats(player);
    break;
    case LIST_PROCESS:
    list_process(player);
    break;
    case LIST_BADNAMES:
    badname_list(player, "Disallowed names:");
    break;
    case LIST_MAILGBL:
    mail_gblout(player);
    break;
    case LIST_GUESTS:
        list_guestparse(player);
        break;
#ifdef REALITY_LEVELS
    case LIST_RLEVELS:
        if ( s_ptr2 && *s_ptr2 && (strncmp(s_ptr2, "dec", 3) == 0) )
           list_rlevels(player, 1);
        else
           list_rlevels(player, 0);
        break;
#endif /* REALITY_LEVELS */
    case LIST_STACKS:
#ifndef NODEBUGMONITOR
        notify(player, unsafe_tprintf("Debug stack depth = %d [highest %d/%d]", debugmem->stacktop, debugmem->stackval, STACKMAX));
        tprp_buff = tpr_buff = alloc_lbuf("do_list");
        for( i = 0; i < debugmem->stacktop; i++ ) {
           tprp_buff = tpr_buff;
           notify(player, safe_tprintf(tpr_buff, &tprp_buff, "stackframe %d = file %s, line %d", i, 
                  aDebugFilenameArray[debugmem->callstack[i].filenum],
                  debugmem->callstack[i].linenum));
        }
        free_lbuf(tpr_buff);
        notify(player, unsafe_tprintf("Last db fetch was for #%d", debugmem->lastdbfetch));
#else
        notify(player, "Debug stack is currently disabled.");
#endif
        break;
    case LIST_LOGCOMMANDS:
        list_logcommands(player);
        break;
    case LIST_FUNPERMS:
        list_functionperms(player, s_ptr2, ((s_ptr2 && *s_ptr2) ? 1 : 0));
        break;
    default: 
    display_nametab(player, list_names,
            (char *) "Unknown option.  Use one of:", 1);
    }
    free_lbuf(save_buff);
    DPOP; /* #57 */
}

int flagcheck(char *fname, char *rbuff)
{
  ATTR *ap;
  NAMETAB *nt;
  char *bp;
  int atrnum;

  *rbuff = '\0';
  ap = atr_str(fname);
  if (ap) {
    atrnum = ap->number;
    bp = rbuff;
    for (nt = attraccess_nametab; nt->name; nt++) {
      if (nt->flag & ap->flags) {
    if (bp != rbuff)
      safe_chr(' ', rbuff, &bp);
    safe_str(nt->name, rbuff, &bp);
      }
    }
    safe_chr('\0', rbuff, &bp);
  }
  else {
    strcpy(rbuff,"1");
    atrnum = -1;
  }
  return atrnum;
}

void do_aflags(dbref player, dbref cause, int key, char *fname, char *args, char *cargs[], int ncargs) 
{
  char *buff, *s_buff, *t_buff, *s_chkattr, *s_format;
  int atrnum, aflags, atrcnt, i_page, i_key;
  dbref i, aowner;

  i_page = i_key = 0;
  if ( key & AFLAGS_PERM ) {
     if ( fname && *fname ) {
        i_page = atoi(fname);
        if ( i_page <= 0 ) 
           i_page = 0;
     }
     if ( key & AFLAGS_SEARCH ) {
        i_key = 1;
        i_page = 0;
     }
     display_perms(player, i_page, i_key, fname);
     return;
  }
  if ( key & AFLAGS_SEARCH ) {
     notify(player, "Invalid switch combination.");
     return;
  }

  if ( key & AFLAGS_ADD ) {
     add_perms(player, fname, args, cargs, ncargs);
     return;
  }

  if ( key & AFLAGS_MOD ) {
     mod_perms(player, fname, args, cargs, ncargs);
     return;
  }

  if ( key & AFLAGS_DEL ) {
     del_perms(player, fname, args, cargs, ncargs);
     return;
  }

  buff = alloc_lbuf("do_flags");
  atrnum = flagcheck(fname, buff);
  s_chkattr = NULL;
  atrcnt = 0;
  s_buff = attrib_show(fname, 0);
  t_buff = attrib_show(fname, 1);
  s_format = alloc_sbuf("do_aflags");
  if (*buff != '1') {
     if ( (fname[0] == '_') && (!mudconf.hackattr_see || !mudconf.hackattr_nowiz) ) {
        sprintf(s_format, " [%s%s%s]",(!mudconf.hackattr_see ? "hidden" : ""),
                                      ((!mudconf.hackattr_see && !mudconf.hackattr_nowiz) ? " " : ""),
                                      (!mudconf.hackattr_nowiz ? "wizard" : "") );
     }
     if ( key & AFLAGS_FULL ) {
        s_chkattr = alloc_lbuf("attribute_delete");
        DO_WHOLE_DB(i) {
           atr_get_str(s_chkattr, i, atrnum, &aowner, &aflags);
           if ( s_chkattr && *s_chkattr ) {
              atrcnt++;
           }
        }
        free_lbuf(s_chkattr);
        if ( !mudconf.hackattr_see || !mudconf.hackattr_nowiz ) {
           notify(player,unsafe_tprintf("(Attribute %d, Total Used: %d) Flags are: %s%s %s%s", 
                                        atrnum, atrcnt, buff, s_format, s_buff, t_buff));
        } else {
           notify(player,unsafe_tprintf("(Attribute %d, Total Used: %d) Flags are: %s %s%s", 
                                        atrnum, atrcnt, buff, s_buff, t_buff));
        }
     } else {
        if ( !mudconf.hackattr_see || !mudconf.hackattr_nowiz ) {
           notify(player,unsafe_tprintf("(Attribute %d) Flags are: %s%s %s%s", 
                                        atrnum, buff, s_format, s_buff, t_buff));
        } else {
           notify(player,unsafe_tprintf("(Attribute %d) Flags are: %s %s%s", 
                                        atrnum, buff, s_buff, t_buff));
        }
     }
  } else {
     if ( !*fname )
        notify(player,"@aflags requires an argument.");
     else
        notify(player,unsafe_tprintf("Bad attribute name '%s'", fname));
  }
  free_lbuf(buff);
  free_lbuf(s_buff);
  free_lbuf(t_buff);
  free_sbuf(s_format);
}

void do_limit(dbref player, dbref cause, int key, char *name,
                char *args[], int nargs)
{
   char *s_chkattr, *s_buffptr, *s_newmbuff;
   int aflags, i_array[LIMIT_MAX], i, i_newval, i_d_default, i_v_default, i_wizcheck;
   dbref aowner, target;

   target = lookup_player(player, name, 0);
   if ((target == NOTHING) || Recover(target)) {
      notify(player, "Bad player.");
      return;
   }
   i_wizcheck = (Wizard(target) && !mudconf.vattr_limit_checkwiz);
   i_d_default = i_v_default = 0;
   if ( !key || (key & LIMIT_LIST) ) {
      s_chkattr = atr_get(target, A_DESTVATTRMAX, &aowner, &aflags);
      if ( *s_chkattr ) {
         i_array[0] = i_array[2] = 0;
         i_array[4] = i_array[1] = i_array[3] = -2;
         for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
              s_buffptr && (i < LIMIT_MAX);
              s_buffptr = (char *) strtok(NULL, " "), i++) {
             i_array[i] = atoi(s_buffptr);
         }
         if ( i_array[3] == -2 ) {
            i_array[3] = (Wizard(target) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit);
            i_d_default = 1;
         }
         if ( i_array[1] == -2 ) {
            i_array[1] = (Wizard(target) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit);
            i_v_default = 1;
         }
         if ( i_array[4] == -2 ) {
            i_array[4] = mudconf.lfunction_max;
         }
         notify(player, unsafe_tprintf("Limitations for player %s(#%d):", Name(target), target));
         if ( i_wizcheck || (i_array[3] == -1) ) {
            notify(player, "   @destroy-=>   Current: N/A        Maximum: Unlimited");
         } else {
            notify(player, unsafe_tprintf("   @destroy-=>   Current: %-10d Maximum: %d %s",
                           i_array[2], i_array[3], 
                           (i_d_default ? "(Default)" : " ")));
         }
         if ( i_wizcheck || (i_array[1] == -1) ) {
            notify(player, "   VLimit---=>   Current: N/A        Maximum: Unlimited");
         } else {
            notify(player, unsafe_tprintf("   VLimit---=>   Current: %-10d Maximum: %d %s",
                           i_array[0], i_array[1],
                           (i_v_default ? "(Default)" : " ")));
         }
         notify(player, unsafe_tprintf("   @Lfunction>   Maximum: %d%s", i_array[4], (i_array[4] == mudconf.lfunction_max ? " (Default)" : " ")));
         notify(player, unsafe_tprintf("\r\nCurrent NonWizard Global Maximums:  @destroy: %-11d VLimit: %d",
                           mudconf.max_dest_limit, mudconf.max_vattr_limit));
         notify(player, unsafe_tprintf("Current Wizard Global Maximums:     @destroy: %-11d VLimit: %d",
                           mudconf.wizmax_dest_limit, mudconf.wizmax_vattr_limit));
         notify(player, unsafe_tprintf("Current Global @lfunction Maximum:  %d", 
                           mudconf.lfunction_max));
      } else {
         notify(player, unsafe_tprintf("Limitations for player %s(#%d):", Name(target), target));
         if ( i_wizcheck ) {
            notify(player, "   @destroy-=>   Current: N/A        Maximum: Unlimited");
            notify(player, "   VLimit---=>   Current: N/A        Maximum: Unlimited");
         } else {
            notify(player, unsafe_tprintf("   @destroy-=>   Current: 0          Maximum: %d (Default)",
                           (Wizard(target) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit)));
            notify(player, unsafe_tprintf("   VLimit---=>   Current: 0          Maximum: %d (Default)",
                           (Wizard(target) ? mudconf.wizmax_vattr_limit : mudconf.max_vattr_limit)));
         }
         notify(player, unsafe_tprintf("   @Lfunction>   Maximum: %d (Default)", mudconf.lfunction_max));
         notify(player, unsafe_tprintf("\r\nCurrent NonWizard Global Maximums:  @destroy: %-11d VLimit: %d",
                           mudconf.max_dest_limit, mudconf.max_vattr_limit));
         notify(player, unsafe_tprintf("Current Wizard Global Maximums:     @destroy: %-11d VLimit: %d",
                           mudconf.wizmax_dest_limit, mudconf.wizmax_vattr_limit));
         notify(player, unsafe_tprintf("Current Global @lfunction Maximum:  %d", 
                           mudconf.lfunction_max));
      }
      free_lbuf(s_chkattr);
      return;
   }
   if ( (key & LIMIT_VADD) || (key & LIMIT_DADD) || (key & LIMIT_LFUN) ) {
      if ( nargs != 1 ) {
         notify(player, "@limit: Incorrect number of arguments.  Only one argument allowed.");
         return;
      }
      if ( i_wizcheck ) {
         notify(player, "@limit: You can not change limiters for wizards with current configuration.");
         return;
      }
      s_chkattr = atr_get(target, A_DESTVATTRMAX, &aowner, &aflags);
      i_array[0] = i_array[2] = 0;
      i_array[4] = i_array[1] = i_array[3] = -2;
      if ( *s_chkattr ) {
         for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
            s_buffptr && (i < LIMIT_MAX);
            s_buffptr = (char *) strtok(NULL, " "), i++) {
            i_array[i] = atoi(s_buffptr);
         }
      }
      free_lbuf(s_chkattr);
      i_newval = atoi(args[0]);
      if ( (i_newval < 0) && (i_newval != -1) && (i_newval != -2) ) {
         notify(player, "@limit: Value must be -2, -1, 0 or greater.");
         return;
      }
      if ( (key & LIMIT_LFUN) ) {
         if ( i_newval == -1 ) {
            notify(player, "@limit: Warning -- No unlimited for @lfunction.  Set to global default.");
            i_newval = -2;
         }
      }
      if ( key & LIMIT_VADD ) {
         s_newmbuff = alloc_mbuf("@limit.vadd");
         sprintf(s_newmbuff, "%d %d %d %d %d", i_array[0], i_newval, i_array[2], i_array[3], i_array[4]);
         atr_add_raw(target, A_DESTVATTRMAX, s_newmbuff);
         free_mbuf(s_newmbuff);
         notify(player, unsafe_tprintf("@limit: New VLimit maximum for %s(#%d) set from %d%s to %d%s", 
                        Name(target), target, i_array[1], 
                        ((i_array[1] == -1) ? " (Unlimited)" : ((i_array[1] == -2) ? " (Default)" : "")), 
                        i_newval,
                        ((i_newval == -1) ? " (Unlimited)" : ((i_newval == -2) ? " (Default)" : "")) ));
         return;
      } else if ( key & LIMIT_LFUN ) {
         s_newmbuff = alloc_mbuf("@limit.lfun");
         sprintf(s_newmbuff, "%d %d %d %d %d", i_array[0], i_array[1], i_array[2], i_array[3], i_newval);
         atr_add_raw(target, A_DESTVATTRMAX, s_newmbuff);
         free_mbuf(s_newmbuff);
         notify(player, unsafe_tprintf("@limit: New @lfunction maximum for %s(#%d) set from %d%s to %d%s", 
                        Name(target), target, i_array[4], 
                        ((i_array[4] == -1) ? " (Unlimited)" : ((i_array[4] == -2) ? " (Default)" : "")), 
                        i_newval,
                        ((i_newval == -1) ? " (Unlimited)" : ((i_newval == -2) ? " (Default)" : "")) ));
         return;
      } else {
         s_newmbuff = alloc_mbuf("@limit.dadd");
         sprintf(s_newmbuff, "%d %d %d %d %d", i_array[0], i_array[1], i_array[2], i_newval, i_array[4]);
         atr_add_raw(target, A_DESTVATTRMAX, s_newmbuff);
         free_mbuf(s_newmbuff);
         notify(player, unsafe_tprintf("@limit: New @destroy maximum for %s(#%d) set from %d%s to %d%s", 
                        Name(target), target, i_array[3], 
                        ((i_array[3] == -1) ? " (Unlimited)" : ((i_array[3] == -2) ? " (Default)" : "")), 
                        i_newval,
                        ((i_newval == -1) ? " (Unlimited)" : ((i_newval == -2) ? " (Default)" : "")) ));
         return;
      }
   }
   if ( key & LIMIT_RESET ) {
      if ( i_wizcheck ) {
         notify(player, "@limit: You can not change limiters for wizards with current configuration.");
         return;
      }
      s_chkattr = atr_get(target, A_DESTVATTRMAX, &aowner, &aflags);
      i_array[0] = i_array[2] = 0;
      i_array[4] = i_array[1] = i_array[3] = -2;
      if ( *s_chkattr ) {
         for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
            s_buffptr && (i < LIMIT_MAX);
            s_buffptr = (char *) strtok(NULL, " "), i++) {
            i_array[i] = atoi(s_buffptr);
         }
      }
      free_lbuf(s_chkattr);
      s_newmbuff = alloc_mbuf("@limit.reset");
      sprintf(s_newmbuff, "%d %d %d %d %d", i_array[0], -2, i_array[2], -2, -2);
      atr_add_raw(target, A_DESTVATTRMAX, s_newmbuff);
      free_mbuf(s_newmbuff);
      notify(player, unsafe_tprintf("@limit: Maximum values for %s(#%d) reset to global defaults.",
                     Name(target), target));
      notify(player, unsafe_tprintf("        @destroy: %d    VLimit: %d   LFunction: %d", 
                     mudconf.max_dest_limit, mudconf.max_vattr_limit, mudconf.lfunction_max));
      return;
   }
   notify(player, "@limit: unrecognized error occurred.  Please report this as a bug report.");
   return;
}

void do_icmd(dbref player, dbref cause, int key, char *name,
        char *args[], int nargs)
{
  CMDENT *cmdp;
  NAMETAB *logcmdp;
  char *buff1, *pt1, *pt2, *pt3, *atrpt, pre[2], *pt4, *pt5, *tpr_buff, *tprp_buff;
  int x, set, aflags, y, home, loc_set, found;
  dbref target, aowner;
  ZLISTNODE *z_ptr;

  target = 0;
  loc_set = -1;
  if ((key == ICMD_IROOM) || (key == ICMD_DROOM) || (key == ICMD_CROOM) ||
      (key == ICMD_LROOM) || (key == ICMD_EROOM) || (key == ICMD_LALLROOM)) {
    if ( key != ICMD_LALLROOM )
       target = match_thing(player, name);
    else
       target = NOTHING;
    if (!(key == ICMD_LALLROOM) && ((target == NOTHING) || Recover(target) ||
        !(isRoom(target) || isThing(target)))) {
       notify(player, "Bad Location.");
       return;
    }
    if (key == ICMD_CROOM) {
       atr_clr(target,A_CMDCHECK);
       notify(player,"Icmd: Location - All cleared");
       notify(player,"Icmd: Done");
       return;
    } else if ( key == ICMD_LROOM) {
       atrpt = atr_get(target,A_CMDCHECK,&aowner,&aflags);
       if (*atrpt) {
         notify(player,"Location CmdCheck attribute is:");
         notify(player,atrpt);
       } else
         notify(player, "Location CmdCheck attribute is empty");
       free_lbuf(atrpt);
       notify(player,"Icmd: Done");
       return;
    } else if ( key == ICMD_LALLROOM) {
       target = Location(player);
       if ( !Good_obj(target) || Going(target) || Recover(target) || isPlayer(target) ) {
          notify(player, "Bad Location.");
          return;
       }
       notify(player, "Scanning all locations and zones from your current location:");
       found = 0;
       atrpt = atr_get(target,A_CMDCHECK,&aowner,&aflags);
       if (*atrpt) {
         notify(player,unsafe_tprintf("%c     --- At %s(#%d) : %s",
                       (IgnoreZone(target) ? '*' : ' '), Name(target), target, (char *)"Location"));
         notify(player,atrpt);
         found = 1;
       }
       free_lbuf(atrpt);
       if ( !ZoneMaster(target) ) {
          tprp_buff = tpr_buff = alloc_lbuf("do_icmd");
          for ( z_ptr = db[target].zonelist; z_ptr; z_ptr = z_ptr->next ) {
             if ( Good_obj(z_ptr->object) && !Recover(z_ptr->object) &&
                 (isRoom(z_ptr->object) || isThing(z_ptr->object)) &&
                 IgnoreZone(z_ptr->object) ) {
                atrpt = atr_get(z_ptr->object,A_CMDCHECK,&aowner,&aflags);
                if (*atrpt) {
                   tprp_buff = tpr_buff;
                   notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%c     --- At %s(#%d) : %s",
                                         (IgnoreZone(z_ptr->object) ? '*' : ' '),
                                         Name(z_ptr->object), z_ptr->object, (char *)"Zone"));
                   notify(player,atrpt);
                   found = 1;
                }
                free_lbuf(atrpt);
             }
          }
          free_lbuf(tpr_buff);
       }
       if ( !found )
          notify(player, "Icmd: Location - No icmd's found at current location");
       notify(player, "Icmd: Done");
       return;
    } else if (key == ICMD_EROOM) {
       loc_set = 2;
    } else if (key == ICMD_IROOM) {
       loc_set = 1;
    } else if (key == ICMD_DROOM) {
       loc_set = 0;
    }
  }

  if (loc_set == -1 ) {
     target = lookup_player(player, name, 0);
     if ((target == NOTHING) || Immortal(target) || Recover(target)) {
       notify(player, "Bad player.");
       return;
     }
     if ((key == ICMD_OFF) || (key == ICMD_CLEAR)) {
       s_Flags3(target,Flags3(target) & ~CMDCHECK);
       if (key == ICMD_CLEAR) 
          atr_clr(target,A_CMDCHECK);
       notify(player,"Icmd: All cleared");
       notify(player,"Icmd: Done");
       return;
     }
     else if (key == ICMD_ON) {
       s_Flags3(target,Flags3(target) | CMDCHECK);
       notify(player,"Icmd: Done");
       return;
     }
     else if (key == ICMD_CHECK) {
       if (CmdCheck(target))
         notify(player,"CmdCheck is active");
       else
         notify(player,"CmdCheck is not active");
       atrpt = atr_get(target,A_CMDCHECK,&aowner,&aflags);
       if (*atrpt) {
         notify(player,"CmdCheck attribute is:");
         notify(player,atrpt);
       }
       else
         notify(player, "CmdCheck attribute is empty");
       free_lbuf(atrpt);
       notify(player,"Icmd: Done");
       return;
     }
     if ( key == ICMD_EVAL ) {
        key = 2;
     }
  } else {
     key = loc_set;
  }
  buff1 = alloc_lbuf("do_icmd");
  tprp_buff = tpr_buff = alloc_lbuf("do_icmd");
  for (x = 0; x < nargs; x++) {
    pt1 = args[x];
    pt2 = buff1;
    while (*pt1)
      *pt2++ = tolower(*pt1++);
    *pt2 = '\0';
    if ((*buff1 == '!') && (*(buff1+1) != '\0')) {
      pt4 = args[x] + 1;
      pt1 = buff1+1;
      set = 0;
    }
    else {
      pt4 = args[x];
      set = 1;
      pt1 = buff1;
    }
    if (*pt1) {
      home = 0;
      *pre = '\0';
      *(pre+1) = '\0';
      logcmdp = (NAMETAB *) hashfind(pt4, &mudstate.logout_cmd_htab);
      if (!logcmdp) {
        cmdp = (CMDENT *) lookup_orig_command(pt1);
    if (!cmdp) {
      if (!string_compare(pt1,"home"))
        home = 1;
      else if (prefix_cmds[(int) *pt1] && !*(pt1+1))
        *pre = *pt1;
    }
      }
      else
    cmdp = NULL;
      if (cmdp || logcmdp || home || *pre) {
    atrpt = atr_get(target,A_CMDCHECK,&aowner,&aflags);
    if (cmdp)
      aflags = strlen(cmdp->cmdname);
    else if (logcmdp)
      aflags = strlen(logcmdp->name);
    else if (home)
      aflags = 4;
    else
      aflags = 1;
    pt5 = atrpt;
    while (pt1) {
      if (cmdp)
        pt1 = strstr(pt5,cmdp->cmdname);
      else if (logcmdp)
        pt1 = strstr(pt5,logcmdp->name);
      else if (home)
        pt1 = strstr(pt5,"home");
      else if (*pre == ':') {
        pt1 = strstr(pt5,"::");
        if (pt1)
          pt1++;
      }
      else
        pt1 = strstr(pt5,pre);
      if (pt1 && (pt1 > atrpt) && (*(pt1-1) == ':') &&
        (isspace((int)*(pt1 + aflags)) || !*(pt1 + aflags)))
        break;
      else if (pt1) {
        if (*pt1)
          pt5 = pt1+1;
        else {
          pt1 = NULL;
          break;
        }
      }
    }
    if (set) {
      if (!pt1) {
        if (*atrpt && (strlen(atrpt) < LBUF_SIZE-2))
          strcat(atrpt," ");
        if (cmdp) {
              tprp_buff = tpr_buff;
          pt3 = safe_tprintf(tpr_buff, &tprp_buff, "%d:%s",key+1,cmdp->cmdname);
        } else if (logcmdp) {
              tprp_buff = tpr_buff;
          pt3 = safe_tprintf(tpr_buff, &tprp_buff, "%d:%s",key+1,logcmdp->name);
        } else if (home) {
              tprp_buff = tpr_buff;
          pt3 = safe_tprintf(tpr_buff, &tprp_buff, "%d:home",key+1);
        } else {
              tprp_buff = tpr_buff;
          pt3 = safe_tprintf(tpr_buff, &tprp_buff, "%d:%c",key+1,*pre);
            }
        if ((strlen(atrpt) + strlen(pt3)) < LBUF_SIZE -1) {
          strcat(atrpt,pt3);
          atr_add_raw(target,A_CMDCHECK,atrpt);
              if ( loc_set == -1 ) {
             s_Flags3(target,Flags3(target) | CMDCHECK);
             notify(player,"Icmd: Set");
              } else
             notify(player,"Icmd: Location - Set");
        }
      }
      else {
            if ( loc_set == -1 )
           notify(player,"Icmd: Command already present");
            else
           notify(player,"Icmd: Location - Command already present");
          }
    }
    else {
      if (pt1) {
        pt2 = pt1-1;
        while ((pt2 > atrpt) && !isspace((int)*pt2))
          pt2--;
        y = pt2-atrpt+1;
        strncpy(buff1,atrpt,y);
        if (y == 1)
          *atrpt = '\0';
        *(atrpt + y) = '\0';
        pt2 = pt1+aflags;
        if (*pt2) {
          while (*pt2 && (isspace((int)*pt2)))
            pt2++;
          if (*pt2)
            strcat(atrpt,pt2);
        }
        if ((y > 1) && !*pt2) {
          pt2 = atrpt+y;
          while ((pt2 > atrpt) && isspace((int)*pt2))
            pt2--;
          *(pt2+1) = '\0';
        }
        if ((y == 1) && !*pt2)  {
          atr_clr(target,A_CMDCHECK);
              if ( loc_set == -1 ) {
             s_Flags3(target,Flags3(target) & ~CMDCHECK);
             notify(player,"Icmd: Cleared");
              } else
             notify(player,"Icmd: Location - Cleared");
        }
        else {
          atr_add_raw(target, A_CMDCHECK, atrpt);
              if ( loc_set == -1 )
             notify(player,"Icmd: Cleared");
              else
             notify(player,"Icmd: Location - Cleared");
        }
      }
      else {
            if ( loc_set == -1 ) 
           notify(player,"Icmd: Command not present");
            else
           notify(player,"Icmd: Location - Command not present");
          }
    }
    free_lbuf(atrpt);
      }
      else {
        if ( loc_set == -1 ) 
       notify(player,"Icmd: Bad command");
        else
       notify(player,"Icmd: Location - Bad command");
      }
    }
  }
  free_lbuf(tpr_buff);
  free_lbuf(buff1);
  notify(player,"Icmd: Done");
}

void do_door(dbref player, dbref cause, int key, char *dname, char *args[], int nargs)
{
#ifdef ENABLE_DOORS
  int bFull = 0, i_now;
  char *s_strtok, *s_strtokr;
  dbref target;
  DESC *d_door, *d_use;

  DPUSH; /* #58 */

  if (key & DOOR_SW_FULL) {
    bFull = 1;
    key &= ~DOOR_SW_FULL;
  }

  switch (key) {
    case 0:
    case DOOR_SW_LIST:
      if (((nargs > 0) && (*args[0] != '\0')) || (*dname != '\0'))
    notify(player,"Doors: Extra arguments ignored.");
      notify(player,"Doors available:");
      listDoors(player, dname, bFull);
      notify(player,"Done.");
      break;
    case DOOR_SW_PUSH:
      if ( !dname || !*dname ) {
         notify(player, "That is not a valid player.");
      } else {
         s_strtok = strtok_r(dname, "|", &s_strtokr);
         if ( !s_strtok || !*s_strtok ) {
            target = NOTHING;
         } else {
            target = lookup_player(player, s_strtok, 0);
         }
         if ( !Good_chk(target) || !isPlayer(target) ) {
            notify(player, "That is not a valid player.");
         } else if ( !controls(player, target) ) {
            notify(player, "You do not control that target.");
         } else if ( !Connected(target) ) {
            notify(player, "That target is not currently connected.");
         } else {
            s_strtok = strtok_r(NULL, "|", &s_strtokr);
            if ( !s_strtok ) {
               notify(player, "Invalid door specified.");
            } else {
               notify(player, unsafe_tprintf("You push %s through the door.", Name(target)));
               openDoor(target, cause, s_strtok, nargs, args, 1);
            }
         }
      }
      break;
    case DOOR_SW_OPEN:
      openDoor(player, cause, dname, nargs, args, 0);
      break;
    case DOOR_SW_STATUS:
      modifyDoorStatus(player, dname, nargs > 0 ? args[0] : NULL);
      break;
    case DOOR_SW_CLOSE:
      closeDoor(desc_in_use, dname);      
      break;
    case DOOR_SW_KICK:
      if ( !dname || !*dname ) {
         notify(player, "That is not a valid player.");
      } else {
         s_strtok = strtok_r(dname, "|", &s_strtokr);
         if ( !s_strtok || !*s_strtok ) {
            target = NOTHING;
         } else {
            target = lookup_player(player, s_strtok, 0);
         }
         if ( !Good_chk(target) || !isPlayer(target) ) {
            notify(player, "That is not a valid player.");
         } else if ( !controls(player, target) ) {
            notify(player, "You do not control that target.");
         } else if ( !Connected(target) ) {
            notify(player, "That target is not currently connected.");
         } else {
            d_use = NULL;
            i_now = 0;
            DESC_ITER_CONN(d_door) {
               if (d_door->player == target) {
                  if ( d_door->last_time > i_now ) {
                     d_use = d_door;
                     i_now = d_door->last_time;
                  }
               }
            }
            if ( !d_use ) {
               notify(player, "That target is not currently connected.");
            } else {
               s_strtok = strtok_r(NULL, "|", &s_strtokr);
               if ( !s_strtok ) {
                  notify(player, "Invalid door specified.");
               } else {
                  notify(player, unsafe_tprintf("You kicked %s from the door.", Name(target)));
                  closeDoor(d_use, s_strtok);      
               }
            }
         }
     }
  }
#endif
  DPOP; /* #58 */
}

void do_assert(dbref player, dbref cause, int key, char *arg1, char *arg2, char *cargs[], int ncargs) {
  char *arg1_eval = NULL, *cp;
  int i_evaled = 0;

  if ( mudconf.break_compatibility ) {
     arg1_eval = exec(player, cause, cause, EV_EVAL | EV_FCHECK, arg1, (char **)NULL, 0, (char **)NULL, 0);
     i_evaled = 1;
  } else
     arg1_eval = arg1;
  if (is_number(arg1_eval) && (atoi(arg1_eval) == 0)) {
    if ( arg2 && *arg2 ) {
       if ( key == BREAK_INLINE) {
          while (arg2) {
             cp = parse_to(&arg2, ';', 0);
             if (cp && *cp) {
                process_command(player, cause, 0, cp, cargs, ncargs, 0);
             }
          }
          /* process_command(player, cause, 1, arg2, cargs, ncargs, 0); */
       } else {
          wait_que(player, cause, 0, NOTHING, arg2, cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
       }
    }
    mudstate.breakst = 1;
  }
  if ( i_evaled )
     free_lbuf(arg1_eval);
}

void do_break(dbref player, dbref cause, int key, char *arg1, char *arg2, char *cargs[], int ncargs) {
  char *arg1_eval = NULL, *cp;
  int i_evaled = 0;

  if ( mudconf.break_compatibility ) {
     arg1_eval = exec(player, cause, cause, EV_EVAL | EV_FCHECK, arg1, (char **)NULL, 0, (char **)NULL, 0);
     i_evaled = 1;
  } else
     arg1_eval = arg1;
  if (is_number(arg1_eval) && (atoi(arg1_eval) != 0)) {
    if ( arg2 && *arg2 ) {
       if ( key == BREAK_INLINE) {
          while (arg2) {
             cp = parse_to(&arg2, ';', 0);
             if (cp && *cp) {
                process_command(player, cause, 0, cp, cargs, ncargs, 0);
             }
          }
/*        process_command(player, cause, 1, arg2, cargs, ncargs, 0); */
       } else {
          wait_que(player, cause, 0, NOTHING, arg2, cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
       }
    }
    mudstate.breakst = 1;
  }
  if ( i_evaled )
     free_lbuf(arg1_eval);
}


void do_hide(dbref player, dbref cause, int key, char *arg1)
{
   if (! (Wizard(player) || HasPriv(player, NOTHING, POWER_NOWHO, POWER5, NOTHING)) ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( key == HIDE_OFF ) {
      if ( !NoWho(player) ) {
         notify(player, "You are already un-@hidden from the WHO list.");
         return;
      }
      s_Flags3(player, Flags3(player) & ~NOWHO);
      notify(player, "Your name is now visible on the WHO list.");
   } else {
      if ( NoWho(player) ) {
         notify(player, "You are already @hidden from the WHO list.");
         return;
      }
      s_Flags3(player, Flags3(player) | NOWHO);
      notify(player, "Your name is now hidden from the WHO list.");
   }
}

void do_log(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
   char *lcbuf, in_str[40], read_str[LBUF_SIZE], *p_in, *p_out, out_str[LBUF_SIZE];
   char max_file[160], *chk_valid_str, *tpr_buff, *tprp_buff;
   FILE *f_ptr;
   int t_lines=0, t_pages=0, t_chars=0, page_num=0, chk_prnt=0, 
       i_slashcnt=0, overflow=0;
   dbref t_player;
   struct tm *tp;
   time_t tt;

   if ( key & MLOG_ROOM ) {
      key = key & ~MLOG_ROOM;
      t_player = 1;     /* Set 'player' as #1 */
   } else {
      t_player = player;
   }
   if ( !*arg1 && !(key & (MLOG_STATS|MLOG_READ))) {
      notify(t_player, "No text was specified with @log.");
      return;
   }
   if ( (key & (MLOG_FILE)) && (!*arg1 || (strlen(arg1) > 127)) ) {
      notify(t_player, "No manual-insertion file specified with @log.");
      return;
   }
   if ( (key & (MLOG_FILE)) && !*arg2 ) {
      notify(t_player, "No text was specified with @log.");
      return;
   }
   if ( key & (MLOG_MANUAL|MLOG_STATS|MLOG_READ) ) {
      if ( strstr(mudconf.manlog_file, "help") != NULL || 
           strstr(mudconf.manlog_file, "wizhelp") != NULL || 
           strstr(mudconf.manlog_file, "news") != NULL || 
           strstr(mudconf.manlog_file, "badsite") != NULL || 
           strstr(mudconf.manlog_file, "connect") != NULL || 
           strstr(mudconf.manlog_file, "create_reg") != NULL || 
           strstr(mudconf.manlog_file, "doorconf") != NULL || 
           strstr(mudconf.manlog_file, "down") != NULL || 
           strstr(mudconf.manlog_file, "error") != NULL || 
           strstr(mudconf.manlog_file, "full") != NULL || 
           strstr(mudconf.manlog_file, "guest") != NULL || 
           strstr(mudconf.manlog_file, "motd") != NULL || 
           strstr(mudconf.manlog_file, "wizmotd") != NULL || 
           strstr(mudconf.manlog_file, "newuser") != NULL || 
           strstr(mudconf.manlog_file, "noguest") != NULL || 
           strstr(mudconf.manlog_file, "quit") != NULL || 
           strstr(mudconf.manlog_file, "register") != NULL || 
           strstr(mudconf.manlog_file, "areghost") != NULL || 
           strstr(mudconf.manlog_file, "autoreg") != NULL || 
           strchr(mudconf.manlog_file, '/') != NULL ||
           strchr(mudconf.manlog_file, '\\') != NULL ||
           strchr(mudconf.manlog_file, '$') != NULL ||
           strchr(mudconf.manlog_file, '~') != NULL ||
           strlen(mudconf.manlog_file) < 1 ||
           strstr(mudconf.manlog_file, "snoop") != NULL ) {
         notify(t_player, "Unable to open manual @log file.  @log aborted.");
         return;
      }
      switch(key) {
         case MLOG_MANUAL:
            memset(in_str, 0, sizeof(in_str));
            sprintf(in_str, "%.30s.txt", mudconf.manlog_file);
            if ( (f_ptr = fopen(in_str, "a")) == NULL) {
               notify(t_player, "Unable to open manual @log file.  @log aborted.");
               return;
            } else {
               if ( strlen(arg1) > 3900)
                  notify(t_player, unsafe_tprintf("%d characters were cut off from your @log entry.",
                         strlen(arg1)-3900) );
               if ( index(arg1, ESC_CHAR ) )
                  notify(t_player, "Ansi was detected in @log and was stripped.");
               lcbuf = alloc_lbuf("process_command.LOG.manual");
               time(&tt);
               tp = localtime((time_t *) (&tt));
               sprintf(lcbuf, "%02d%d%d%d%d.%d%d%d%d%d%d : [%.30s(#%d)] : %.3900s",
                       (tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
                       (((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
                       (tp->tm_mday % 10),
                       (tp->tm_hour / 10), (tp->tm_hour % 10),
                       (tp->tm_min / 10), (tp->tm_min % 10),
                       (tp->tm_sec / 10), (tp->tm_sec % 10),
                       Name(player), player, strip_returntab(strip_ansi(arg1),3));
               fprintf( f_ptr, "%s\n", lcbuf );
               fclose(f_ptr);
               sprintf(lcbuf, "[%s(#%d)] @logged to %s", Name(player), player, mudconf.manlog_file);
               STARTLOG(LOG_ALWAYS, "WIZ", "NOTE");
               log_text(lcbuf);
               free_lbuf(lcbuf);
               notify(t_player, "@log successfully written to manual log file.");
               ENDLOG
            }
            break;
         case MLOG_READ:
            memset(in_str, 0, sizeof(in_str));
            sprintf(in_str, "%.30s.txt", mudconf.manlog_file);
            if ( (f_ptr = fopen(in_str, "r")) == NULL) {
               notify(t_player, "Unable to open manual @log file.  @log aborted.");
               return;
            } else {
               while ( !feof(f_ptr) ) {
                  fgets( read_str, LBUF_SIZE - 1, f_ptr);
                  if (!feof(f_ptr)) {
                     t_chars += strlen(read_str);
                     t_lines++;
                  }
                  if (t_lines > 100000 ) {
                     notify(t_player, "Manual log file too large.  Aborting.");
                     fclose(f_ptr);
                     return;
                  }
               }
               if ( t_lines > 0 ) {
                  t_pages = (t_lines/10)+1;
                  if ((t_lines % 10) == 0)
                     t_pages--;
                  page_num = atoi(arg1);
                  if (page_num < 1)
                     page_num = 1;
                  if (page_num > t_pages)
                     page_num = t_pages;
               }
               rewind(f_ptr);
               t_lines=0;
               tprp_buff = tpr_buff = alloc_lbuf("do_log");
               while ( !feof(f_ptr) ) {
                  fgets( read_str, LBUF_SIZE - 1, f_ptr);
                  while ( strchr(read_str, '\n') == 0 && !feof(f_ptr)) {
                     fgets( read_str, LBUF_SIZE - 1, f_ptr);
                     overflow=1;
                  }
                  if (!feof(f_ptr)) {
                     if ( ((t_lines/10)+1) == page_num ) {
                        p_in=read_str;
                        p_out=out_str;
                        while (*p_in && p_in) {
                           if (isprint((int)*p_in)) {
                              *p_out++ = *p_in++;
                           } else {
                               p_in++;
                           }
                        }
                        *p_out='\0';
                        tprp_buff = tpr_buff;
                        if (overflow)
                           notify(t_player, safe_tprintf(tpr_buff, &tprp_buff, "Line #%d [overflow cut]...\r\n%s", t_lines + 1, out_str));
                        else
                           notify(t_player, safe_tprintf(tpr_buff, &tprp_buff, "Line #%d...\r\n%s", t_lines + 1, out_str));
                        chk_prnt++;
                     }
                     t_chars += strlen(read_str);
                     t_lines++;
                  }
                  overflow=0;
               }
               free_lbuf(tpr_buff);
               if (chk_prnt == 0) {
                  if (t_lines == 0)
                     notify(t_player, "Log file is empty.");
                  else
                     notify(t_player, "There was no valid text for that page.");
               } else {
                  notify(t_player, unsafe_tprintf("-------- Page #%d ( %d total page%s) --------", 
                         page_num, t_pages, t_pages < 2 ? " ": "s " ));
               }
               fclose(f_ptr);
            }
            break;
         case MLOG_STATS:
            memset(in_str, 0, sizeof(in_str));
            sprintf(in_str, "%.30s.txt", mudconf.manlog_file);
            if ( (f_ptr = fopen(in_str, "r")) == NULL) {
               notify(t_player, "Unable to open manual @log file.  @log aborted.");
               return;
            } else {
               while ( !feof(f_ptr) ) {
                  fgets( read_str, LBUF_SIZE - 1, f_ptr);
                  if (!feof(f_ptr)) {
                     t_chars += strlen(read_str);
                     t_lines++;
                  }
                  if (t_lines > 100000 ) {
                     notify(t_player, "Manual log file too large.  Aborting.");
                     fclose(f_ptr);
                     return;
                  }
               }
               if ( t_lines == 0 ) {
                  notify(t_player, "Log file is empty.");
               } else {
                  t_pages = (t_lines/10)+1;
                  if ((t_lines % 10) == 0)
                     t_pages--;
                  notify(t_player, "Log report:");
                  notify(t_player, 
                      unsafe_tprintf("Total Lines...%d  Total Chars...%d  Total Pages...%d  Avg...%d chars/line",
                              t_lines, t_chars, t_pages, t_chars/t_lines) );
               }
               fclose(f_ptr);
            }
            break;
      }
   } else if ( key & MLOG_FILE ) {
      memset(max_file, 0, sizeof(max_file));
      chk_valid_str = arg1;
      while (*chk_valid_str) {
         if ( !isalnum((int)*chk_valid_str) && (*chk_valid_str != '_') && 
              (*chk_valid_str != '/') ) 
            break;
         if ( *chk_valid_str == '/' )
            i_slashcnt++;
         if ( i_slashcnt > 5 )
            break;
         chk_valid_str++;
      }
      if ( *chk_valid_str ) {
         if ( i_slashcnt > 5 )
            notify(t_player, "Directory depth is too deep.  5 maximum.  @log aborted.");
         else
            notify(t_player, "Invalid character in filename.  @log aborted.");
         return;
      }
      sprintf(max_file, "./%.128s_manual.log", arg1);
      if ( (f_ptr = fopen(max_file, "a")) == NULL) {
         notify(t_player, "Unable to open manual @log file.  @log aborted.");
         return;
      } else {
         if ( strlen(arg2) > 3900)
            notify(t_player, unsafe_tprintf("%d characters were cut off from your @log entry.",
                   strlen(arg2)-3900) );
/* Let's allow ansi, tabs, and carrage returns to these log files 
 *
 *       if ( index(arg2, ESC_CHAR ) )
 *          notify(t_player, "Ansi was detected in @log and was stripped.");
 */
         lcbuf = alloc_lbuf("process_command.LOG.file_manual");
         /* Cut off at 3900 to save the poor little lbuf */
         time(&tt);
         tp = localtime((time_t *) (&tt));
         sprintf(lcbuf, "%02d%d%d%d%d.%d%d%d%d%d%d : [%.30s(#%d)] : %.3900s %s",
                 (tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
                 (((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
                 (tp->tm_mday % 10),
                 (tp->tm_hour / 10), (tp->tm_hour % 10),
                 (tp->tm_min / 10), (tp->tm_min % 10),
                 (tp->tm_sec / 10), (tp->tm_sec % 10),
                 Name(player), player, arg2, 
                 ((index(arg2, ESC_CHAR) && (strlen(arg2) > 3900)) ? ANSI_NORMAL : (char *)" "));
         fprintf( f_ptr, "%s\n", lcbuf );
         sprintf(lcbuf, "[%s(#%d)] @logged to %s", Name(player), player, max_file);
         STARTLOG(LOG_ALWAYS, "WIZ", "NOTE")
            log_text(lcbuf);
         ENDLOG
         free_lbuf(lcbuf);
         notify(t_player, "@log successfully written to specified file.");
      }
      if ( f_ptr )
        fclose(f_ptr);
   } else {
      if ( strlen(arg1) > 3900)
         notify(t_player, unsafe_tprintf("%d characters were cut off from your @log entry.",
                strlen(arg1)-3900) );
      if ( index(arg1, ESC_CHAR ) )
         notify(t_player, "Ansi was detected in @log and was stripped.");
      STARTLOG(LOG_ALWAYS, "WIZ", "MNLOG")
      lcbuf = alloc_lbuf("process_command.LOG.manual");
      /* Cut off at 3900 to save the poor little lbuf */
      sprintf(lcbuf, "[%.30s(#%d)] : %.3900s",
              Name(player), player, strip_returntab(strip_ansi(arg1),3));
      log_text(lcbuf);
      free_lbuf(lcbuf);
      notify(t_player, "@log successfully written to mush log file.");
      ENDLOG
   }
}

void do_program(dbref player, dbref cause, int key, char *name, char *command)
{
   dbref thing, it, aowner;
   int aflags, atr;
   char *buf, *attrib, *tmplbuf, *tmplbufptr, *progatr, strprompt[LBUF_SIZE], *tpr_buff, *tprp_buff;
   DESC *d;
#ifdef ZENTY_ANSI
   char *s_buff, *s_buff2, *s_buff3, *s_buffptr, *s_buff2ptr, *s_buff3ptr;
#endif

   if (!*name || !name) {
      notify(player, "No valid player specified.");
      return;
   }
   thing = match_thing(player, name);
   if ( thing == NOTHING || !Good_obj(thing) || Recover(thing) || Going(thing) || !isPlayer(thing) ) {
      notify(player, "Invalid player specified.");
      return;
   }
   /* Ok, let's have some MUX compatibility here */
   if ( (!(Prog(player) || Prog(Owner(player))) && (player != thing)) || 
        ((Cloak(thing) && SCloak(thing) && Immortal(thing) && !Immortal(player)) ||
         (Cloak(thing) && !Wizard(player))) ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !Connected(thing) ) {
      notify(player, "That player is not connected.");
      return;
   }
   if ( InProgram(thing) ) {
      if ( thing == player )
         notify(player, "You are already in a program.");
      else
         notify(player, "That player is already in a program.");
      return;
   }
   if ( !*command || !command ) {
      notify(player, "No such attribute.");
      return;
   }
   buf = command;
   if ( strchr(buf, ':') != NULL ) {
      attrib = parse_to(&buf, ':', 1);
      notify(thing, buf);
   } else
      attrib = buf;

   if (!parse_attrib(player, attrib, &it, &atr) || (atr == NOTHING)) {
      notify_quiet(player, "No match.");
      return;
   }
   if (!controls(player, it) &&
       !could_doit(player,it,A_LTWINK,0,0)) {
      notify_quiet(player, "Permission denied.");
      return;
   }

   tmplbufptr = tmplbuf = alloc_lbuf("do_program");
   safe_str(unsafe_tprintf("%d:%s", player, attrib), tmplbuf, &tmplbufptr);
   atr_add_raw(thing, A_PROGBUFFER, tmplbuf);    
   free_lbuf(tmplbuf);
   /* Use telnet protocol's GOAHEAD command to show prompt.
    * 
    */
   tprp_buff = tpr_buff = alloc_lbuf("do_program");
   DESC_ITER_CONN(d) {
      if ( d->player == thing ) {
        progatr = atr_get(it, A_PROGPROMPT, &aowner, &aflags);
        memset(strprompt, 0, sizeof(strprompt));
        strncpy(strprompt, progatr, sizeof(strprompt)-1);
        free_lbuf(progatr);
        if ( *strprompt ) {
           if ( strcmp(strprompt, "NULL") != 0 ) {
              tprp_buff = tpr_buff;

#ifdef ZENTY_ANSI
              s_buffptr = s_buff = alloc_lbuf("parse_ansi_prompt");
              s_buff2ptr = s_buff2 = alloc_lbuf("parse_ansi_prompt2");
              s_buff3ptr = s_buff3 = alloc_lbuf("parse_ansi_prompt3");
              parse_ansi((char *) strprompt, s_buff, &s_buffptr, s_buff2, &s_buff2ptr, s_buff3, &s_buff3ptr);
              queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s \377\371", ANSI_HILITE, s_buff, ANSI_NORMAL));
              free_lbuf(s_buff);
              free_lbuf(s_buff2);
#else
              queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s \377\371", ANSI_HILITE, strprompt, ANSI_NORMAL));
#endif
           }
           atr_add_raw(thing, A_PROGPROMPTBUF, strprompt);
        } else {
           tprp_buff = tpr_buff;
           queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
        }
        break;
      }
   }
   free_lbuf(tpr_buff);
   mudstate.shell_program = 1;
   s_Flags4(thing, (Flags4(thing) | INPROGRAM));
}

void do_quitprogram(dbref player, dbref cause, int key, char *name)
{
   dbref thing;
   DESC *d;

   if ( *name )
      thing = match_thing(player, name);
   else
      thing = player;

   if ( !Good_obj(thing) || Recover(thing) || Going(thing) || !isPlayer(thing) ) {
      notify(player, "Invalid player specified.");
      return;
   }
   if ( (!(Prog(player) || Prog(Owner(player))) && (player != thing)) ||
        ((Cloak(thing) && SCloak(thing) && Immortal(thing) && !Immortal(player)) ||
         (Cloak(thing) && !Wizard(player))) ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !Connected(thing) ) {
      notify(player, "That player is not connected.");
      return;
   }
   if ( !InProgram(thing) ) {
      if ( thing == player )
         notify(player, "You are not in a program.");
      else
         notify(player, "That player is not in a program.");
      return;
   }
   s_Flags4(thing, (Flags4(thing) & (~INPROGRAM)));
   DESC_ITER_CONN(d) {
      if ( d->player == thing ) {
         queue_string(d, "\377\371");
      }
   }
   mudstate.shell_program = 0;
   if ( !(key & QUITPRG_QUIET) )
      notify(player, "@program cleared.");
   if ( thing == player ) {
     if ( !(key & QUITPRG_QUIET) )
        notify(thing, "You have aborted your program.");
   } else {
     if ( !(key & QUITPRG_QUIET) )
        notify(thing, unsafe_tprintf("Your @program has been terminated by %s.", Name(player)));
   }
   atr_clr(thing, A_PROGBUFFER);
   atr_clr(thing, A_PROGPROMPTBUF);
}

void do_idle(dbref player, dbref cause, int key, char *string, char *args[], int nargs)
{
   struct itimerval itimer;

   if ( mudstate.train_cntr > 0 ) {
      notify(player, "Too many idle commands nested.");
      return;
   }

   if ( Wizard(player) && string && *string ) {
      if ( (*string == '@') && (*(string+1) == '@') ) {
         notify_quiet(player, string+2);
      } else {
         mudstate.train_cntr++;
         getitimer(ITIMER_PROF, &itimer);
         process_command(player, cause, 1, string, args, nargs, 0);
         setitimer(ITIMER_PROF, &itimer, NULL); 
         mudstate.train_cntr--;
      }
   } else if ( !Wizard(player) && string && *string ) {
      if ( (*string == '@') && (*(string+1) == '@') ) {
         notify_quiet(player, string+2);
      } else {
         notify(player, "idle for you does not allow command execution other than '@@' for messages");
      }
   }
}

void do_train(dbref player, dbref cause, int key, char *string, char *args[], int nargs)
{
   dbref loc, obj;
   char *newstring, *newstringptr, *pupbuff, *pupbuffptr, *tpr_buff, *tprp_buff;
   struct itimerval itimer;

   if ( mudstate.train_cntr > 2 ) {
      notify(player, "Too many train commands nested.");
      return;
   }
   loc = Location(player);
   if ( loc == NOTHING || loc == AMBIGUOUS || !Good_obj(loc) ) {
      notify(player, "Bad location.");
      return;
   }
   if ( !*string || !string ) {
      notify(player, "Train requires an argument.");
      return;
   }
   newstringptr = newstring = alloc_lbuf("do_train.buffer");
   safe_str(unsafe_tprintf("%s types -=> %s", Name(player), string), newstring, &newstringptr);
   tprp_buff = tpr_buff = alloc_lbuf("train");
   DOLIST(obj, Contents(loc)) {
      if ( Puppet(obj) && (Location(obj) != Location(Owner(obj))) ) {
         pupbuffptr = pupbuff = alloc_lbuf("do_train_puppet.buffer");
         tprp_buff = tpr_buff;
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s> %s", Name(obj), newstring), pupbuff, &pupbuffptr);
         raw_notify(Owner(obj), pupbuff, 0, 1);
         free_lbuf(pupbuff);
      } else {
         raw_notify(obj, newstring, 0, 1);
      }
   }
   free_lbuf(tpr_buff);

   free_lbuf(newstring);
   mudstate.train_cntr++;
   getitimer(ITIMER_PROF, &itimer);
   process_command(player, cause, 1, string, args, nargs, 0);
   setitimer(ITIMER_PROF, &itimer, NULL); 
   mudstate.train_cntr--;
}
void do_skip(dbref player, dbref cause, int key, char *s_boolian, char *args[], int nargs, char *cargs[], int ncargs)
{
   char *retbuff, *cp, *mys, *s_buildptr, *s_build;
   time_t i_now;
   int old_trainmode, i_breakst, i_joiner;

   if ( !nargs || !args[0] || !*args[0] )
      return;
   i_breakst = mudstate.breakst;
   char c_dummy[]="0";
   if ( !s_boolian || !*s_boolian ) {
      retbuff = c_dummy;
   }
   else {
      retbuff = exec(player, cause, cause, EV_EVAL | EV_FCHECK, s_boolian, (char **)NULL, 0, (char **)NULL, 0);
   }
   old_trainmode=mudstate.trainmode;
   int i_evalResult=0;
   if(mudconf.ifelse_compat)
      i_evalResult=tboolchk(retbuff);
   else
      i_evalResult=atoi(retbuff);
   if ( *retbuff && (((i_evalResult == 0) && !(key & SKIP_IFELSE)) ||
                     ((i_evalResult != 0) &&  (key & SKIP_IFELSE))) ) {
    /* I have no idea why this is here, but I left it in incase I need 
      if ( desc_in_use == NULL ) {
         mudstate.trainmode = 1;
      }
     */
      if ( !(key & SKIP_IFELSE) && (nargs > 1) ) {
         s_buildptr = mys = s_build = alloc_lbuf("do_skip_joiner");
         safe_str(args[0], mys, &s_buildptr);
         for (i_joiner = 1; i_joiner < nargs; i_joiner++) {
            safe_chr(',', mys, &s_buildptr);
            safe_str(args[i_joiner], mys, &s_buildptr);
         }
         i_now = mudstate.now;
         while ( mys ) {
            cp = parse_to(&mys, ';', 0);
            if (cp && *cp && !mudstate.breakst) {
               process_command(player, cause, 0, cp, cargs, ncargs, 0);
               if ( time(NULL) > (i_now + 5) ) {
                   notify(player, "@skip:  Aborted for high utilization.");
                   mudstate.breakst=1;
                   break;
               }
            }
         }
         free_lbuf(s_build);
      } else {
         mys = args[0];
         i_now = mudstate.now;
         while (mys) {
            cp = parse_to(&mys, ';', 0);
            if (cp && *cp && !mudstate.breakst) {
               process_command(player, cause, 0, cp, cargs, ncargs, 0);
               if ( time(NULL) > (i_now + 5) ) {
                   notify(player, "@skip:  Aborted for high utilization.");
                   mudstate.breakst=1;
                   break;
               }
            }
         }
      }
      mudstate.trainmode = old_trainmode;
   } else if ( *retbuff && (i_evalResult == 0) && (key & SKIP_IFELSE) && (nargs > 1) && args[1] && *args[1] ) {
      mys = args[1];
      i_now = mudstate.now;
      while (mys) {
         cp = parse_to(&mys, ';', 0);
         if (cp && *cp && !mudstate.breakst) {
            process_command(player, cause, 0, cp, cargs, ncargs, 0);
            if ( time(NULL) > (i_now + 5) ) {
                notify(player, "@skip:  Aborted for high utilization.");
                mudstate.breakst=1;
                break;
            }
         }
      }
   }
   if ( !s_boolian || !*s_boolian ) {
      retbuff = NULL;
   }
   else {
      free_lbuf(retbuff);
   }
   if ( desc_in_use != NULL ) {
      mudstate.breakst = i_breakst;
   }
}

void do_sudo(dbref player, dbref cause, int key, char *s_player, char *s_command, char *args[], int nargs)
{
   dbref target;
   char *retbuff, *cp, *pt, *savereg[MAX_GLOBAL_REGS];
   int old_trainmode, x, i_breakst, forcehalted_state;

   //if ( mudstate.sudo_cntr >= 1 ) {
   //   notify(player, "You can't nest @sudo.");
   //   return;
   //}
   if ( !s_command || !*s_command ) {
      return;
   }

   if ( !s_player || !*s_player ) {
      target = NOTHING;
   } else {
      retbuff = exec(player, cause, cause, EV_EVAL | EV_FCHECK, s_player, (char **)NULL, 0, (char **)NULL, 0);
      target = match_thing(player, retbuff);
      free_lbuf(retbuff);
   }
   if ( !Good_chk(target) || !Controls(player, target) ) {
      notify(player, "Permission denied.");
      return;
   }

   forcehalted_state = mudstate.force_halt;
   if ( Halted(target) && ForceHalted(player) ) {
      mudstate.force_halt = target;
   } else
      mudstate.force_halt = 0;

   mudstate.sudo_cntr++;
   old_trainmode=mudstate.trainmode;

   if ( !(key & SUDO_GLOBAL) || (key & INCLUDE_CLEAR) ) {
      for (x = 0; x < MAX_GLOBAL_REGS; x++) {
         savereg[x] = alloc_lbuf("ulocal_reg");
         pt = savereg[x];
         safe_str(mudstate.global_regs[x],savereg[x],&pt);
         if ( key & SUDO_CLEAR ) {
            *mudstate.global_regs[x] = '\0';
         }
      }
   }

   i_breakst = mudstate.breakst;
   while (s_command) {
      cp = parse_to(&s_command, ';', 0);
      if (cp && *cp) {
         process_command(target, target, 0, cp, args, nargs, 0);
      }
   }
   if ( desc_in_use != NULL ) {
      mudstate.breakst = i_breakst;
   }

   if ( !(key & SUDO_GLOBAL) || (key & SUDO_CLEAR) ) {
      for (x = 0; x < MAX_GLOBAL_REGS; x++) {
         pt = mudstate.global_regs[x];
         safe_str(savereg[x],mudstate.global_regs[x],&pt);
         free_lbuf(savereg[x]);
      }
   }

   /* process_command(target, target, 1, s_command, args, nargs, 0); */
   mudstate.trainmode = old_trainmode;
   mudstate.sudo_cntr--;
   mudstate.force_halt = forcehalted_state;
}

void do_noparsecmd(dbref player, dbref cause, int key, char *string, char *args[], int nargs)
{
   dbref loc;
   struct itimerval itimer;

   if ( mudstate.train_cntr > 2 ) {
      notify(player, "Too many ] commands nested.");
      return;
   }
   loc = Location(player);
   if ( loc == NOTHING || loc == AMBIGUOUS || !Good_obj(loc) ) {
      notify(player, "Bad location.");
      return;
   }
   if ( !*string || !string || !*(string+1) || !(string+1) ) {
      notify(player, "] requires an argument.");
      return;
   }
   mudstate.train_cntr++;
   mudstate.trainmode = 1;
   getitimer(ITIMER_PROF, &itimer);
   process_command(player, cause, 1, string+1, args, nargs, 0);
   setitimer(ITIMER_PROF, &itimer, NULL); 
   mudstate.trainmode = 0;
   mudstate.train_cntr--;
}

void 
do_pipe(dbref player, dbref cause, int key, char *name)
{
   ATTR *atr, *atr2;
   dbref aowner;
   int aflags, anum, i_type;
   char *s_atext, *s_tmpbuff;

   switch (key) {
      case PIPE_ON: /* Enable piping to attribute */
      case PIPE_TEE: /* Enable piping to attribute */
           if ( H_Attrpipe(player) ) {
              raw_notify(player, (char *)"@pipe: you already are piping to an attribute.", 0, 1);
           } else {
              if ( !*name ) {
                 notify_quiet(player, (char *)"@pipe: a valid attribute must be specified to pipe to.");
              } else {
                 anum = mkattr(name);
                 if ( anum < 0 ) {
                    notify_quiet(player, (char *)"@pipe: a valid attribute must be specified to pipe to.");
                 } else { 
                    atr2 = atr_str3(name);
                    if ( !atr2 ) {
                       notify_quiet(player, (char *)"@pipe: a valid attribute must be specified to pipe to.");
                    } else {
                       anum = mkattr("___ATTRPIPE");
                       if ( anum > 0 ) {
                          atr = atr_str("___ATTRPIPE");
                          if ( !atr ) {
                             notify_quiet(player, (char *)"@pipe: the piping attribute could not be written to.");
                          } else {
                             s_atext = atr_get(player, atr2->number, &aowner, &aflags);
                             free_lbuf(s_atext);
                             if ( !Controlsforattr(player, player, atr2, aflags)) {
                                notify_quiet(player, (char *)"@pipe: you have no permission to pipe to that attribute.");
                             } else {
                                notify_quiet(player, (char *)"@pipe: piping to attribute has been enabled.");
                                s_tmpbuff = alloc_lbuf("do_pipe_tee");
                                sprintf(s_tmpbuff, "%s %d", name, ((key & PIPE_TEE) ? 1 : 0));
                                atr_add_raw(player, atr->number, s_tmpbuff);
                                free_lbuf(s_tmpbuff);
                                s_Flags4(player, Flags4(player) | HAS_ATTRPIPE);
                             }
                          }
                       } else {
                          notify_quiet(player, (char *)"@pipe: the piping attribute could not be written to.");
                       }
                    }
                 }
              }
           }
           break;
      case PIPE_OFF: /* Disable piping to attribute */
           if ( H_Attrpipe(player) ) {
              s_Flags4(player, Flags4(player) & ~HAS_ATTRPIPE);
              atr = atr_str("___ATTRPIPE");
              if ( atr ) {
                 atr_clr(player, atr->number);
              }
              notify_quiet(player, (char *)"@pipe: piping to attribute has been disabled.");
           } else {
              notify_quiet(player, (char *)"@pipe: piping to attribute is not currently enabled.");
           }
           break;
      case PIPE_STATUS: /* Status of piping */
           i_type = 0;
           if ( H_Attrpipe(player) ) {
              anum = mkattr("___ATTRPIPE");
              if ( anum > 0 ) {
                 atr = atr_str("___ATTRPIPE");
                 if ( !atr ) {
                    s_Flags4(player, Flags4(player) & ~HAS_ATTRPIPE);
                    notify_quiet(player, (char *)"@pipe: piping could not continue so was automatically stopped.");
                 } else {
                    s_atext = atr_get(player, atr->number, &aowner, &aflags);
                    s_tmpbuff = strchr(s_atext, ' ');
                    if ( s_tmpbuff ) {
                       *s_tmpbuff = '\0';
                       i_type = atoi(s_tmpbuff+1);
                    }
                    s_tmpbuff = alloc_lbuf("pipe_status");
                    sprintf(s_tmpbuff, "@pipe: currently in %s mode writing to attribute '%s'.", 
                           (i_type ? "TEE" : "PIPE (ON)"), s_atext);
                    raw_notify(player, s_tmpbuff, 0, 1);
                    free_lbuf(s_tmpbuff);
                    free_lbuf(s_atext); 
                 }
              } else {
                 s_Flags4(player, Flags4(player) & ~HAS_ATTRPIPE);
                 notify_quiet(player, (char *)"@pipe: piping could not continue so was automatically stopped.");
              }
           } else {
              notify_quiet(player, (char *)"@pipe: piping to attribute is not currently enabled.");
           }
           break;
      default: /* A switch must be used */
           raw_notify(player, (char *)"@pipe: a valid switch must be used with @pipe.", 0, 1);
           break;        
   }

}

void do_extansi(dbref player, dbref cause, int key, char *name, char *instr)
{
   dbref thing;
   char *retbuff, *namebuff, *namebuffptr, *s;

   thing = match_thing(player, name);
   if ( thing == NOTHING || !Good_obj(thing) || Recover(thing) || Going(thing) ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !(Controls(player, thing) || could_doit(player, thing, A_LTWINK, 0, 0)) ||
        ((Cloak(thing) && SCloak(thing) && Immortal(thing) && !Immortal(player)) ||
         (Cloak(thing) && !Wizard(player))) ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !ExtAnsi(thing) ) {
      notify(player, "Target is not toggled EXTANSI.");
      return;
   }
   if ( !*instr || !instr ) {
      if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing)) {
          notify(player, "Ansi string cleared.");
      }
      atr_clr(thing, A_ANSINAME);
   } else {
     retbuff = exec(player, cause, cause, EV_EVAL | EV_FCHECK, instr, (char **)NULL, 0, (char **)NULL, 0);
     namebuffptr = namebuff = alloc_lbuf("do_extansi.exitname");
      if ( isExit(thing) ) {
         for (s = Name(thing); *s && (*s != ';'); s++)
            safe_chr(*s, namebuff, &namebuffptr);
      } else {
        safe_str(Name(thing), namebuff, &namebuffptr);
      }
#ifdef ZENTY_ANSI
#define extansi_strip(x) strip_all_special(x)
#else
#define extansi_strip(x) strip_all_ansi(x)
#endif

      if ( strcmp(extansi_strip(retbuff), namebuff) != 0 ) {
         notify(player, unsafe_tprintf("String '%s' does not match the name of the target, '%s'.",
                extansi_strip(retbuff),
                Name(thing)));
         free_lbuf(retbuff);
         free_lbuf(namebuff);
         return;
#undef extansi_strip
      } else {
         if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing)) {
                notify(player, unsafe_tprintf("Ansi string entered for %s of '%s'.", namebuff, 
                strip_returntab(retbuff,3)));
         }
         atr_add_raw(thing, A_ANSINAME, 
                     unsafe_tprintf("%.3900s%s", strip_returntab(retbuff,3), ANSI_NORMAL));
      }
      free_lbuf(retbuff);
      free_lbuf(namebuff);
   }
}

void show_hook(char *bf, char *bfptr, int key)
{
   if ( key & HOOK_BEFORE )
      safe_str("before ", bf, &bfptr);
   if ( key & HOOK_AFTER )
      safe_str("after ", bf, &bfptr);
   if ( key & HOOK_PERMIT )
      safe_str("permit ", bf, &bfptr);
   if ( key & HOOK_IGNORE )
      safe_str("ignore ", bf, &bfptr);
   if ( key & HOOK_IGSWITCH )
      safe_str("igswitch ", bf, &bfptr);
   if ( key & HOOK_FAIL )
      safe_str("fail ", bf, &bfptr);
   return;
}

void do_protect(dbref player, dbref cause, int key, char *name)
{
   int i_first, i_listall;
   dbref i_return, target, p_lookup;
   char *s_protect_buff, *s_protect_ptr;
   PROTECTNAME *bp;

   if ( !key )
      key=PROTECT_LIST;

   i_listall = 0;
   if ( key & PROTECT_LISTALL ) {
      key &= ~PROTECT_LISTALL;
      i_listall = 4;
      if ( !(key & PROTECT_LIST) ) {
         notify(player, "Illegal combination of switches.");
         return;
      }
   }

   if ( (key & PROTECT_DEL) && (!*name || !ok_player_name(name)) ) {
      notify(player, "@protect with /del or /add requires valid name");
      return;
   }
   s_protect_ptr = s_protect_buff=alloc_lbuf("do_protect");
   switch (key ) {
      case PROTECT_ADD:
         i_return = (dbref) protectname_add(Name(player), player);
         if ( i_return == 1 ) {
            notify(player, unsafe_tprintf("You have reached the maximum of %d protected names.", mudconf.max_name_protect));
         } else if ( i_return == 2 ) {
            notify(player, "Your current name is already protected.");
         } else {
            notify(player, "Your current name has been added to your protect list.");
            for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
               if ( bp->i_name == player ) {
                  if ( i_first )
                     safe_chr('\t', s_protect_buff, &s_protect_ptr);
                  i_first=1;
                  safe_str(bp->name, s_protect_buff, &s_protect_ptr);
               }
            }
            atr_add(player, A_PROTECTNAME, s_protect_buff, player, 0);
         }
         break;
      case PROTECT_DEL:
         i_return = protectname_remove(name, player);
         if ( i_return == -1 ) {
            notify(player, unsafe_tprintf("Delete what?  The name '%s' is not in the protected list.", name));
         } else {
            notify(player, unsafe_tprintf("You have successfully deleted '%s' from your protect list.", name));
            for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
               if ( bp->i_name == i_return ) {
                  if ( i_first )
                     safe_chr('\t', s_protect_buff, &s_protect_ptr);
                  i_first=1;
                  safe_str(bp->name, s_protect_buff, &s_protect_ptr);
               }
            }
            if ( Good_chk(i_return) )
               atr_add(i_return, A_PROTECTNAME, s_protect_buff, i_return, 0);
         }
         break;
      case PROTECT_ALIAS:
         i_return = protectname_alias(name, player);
         if ( i_return == -1 ) {
            notify(player, unsafe_tprintf("Alias what?  The name '%s' is not in the protected list.", name));
         } else if ( i_return == -2 ) {
            notify(player, unsafe_tprintf("The name '%s' is already marked as an alias.", name));
         } else if ( i_return == -3 ) {
            notify(player, unsafe_tprintf("The name '%s' is already set in your @alias attribute.", name));
         } else if ( i_return == -5 ) {
            notify(player, unsafe_tprintf("The name '%s' is their current active name.  Can not alias.", name));
         } else if ( i_return == -4 ) {
            p_lookup = (pmath2)hashfind(name, &mudstate.player_htab);
            if ( p_lookup == player ) {
               notify(player, unsafe_tprintf("Somehow, the name '%s' has been aliased but is not marked as such.  Fixing.", name, p_lookup));
               delete_player_name(player, name);
               i_return = protectname_alias(name, player);
               for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
                  if ( bp->i_name == i_return ) {
                     if ( i_first )
                        safe_chr('\t', s_protect_buff, &s_protect_ptr);
                     i_first=1;
                     safe_str(bp->name, s_protect_buff, &s_protect_ptr);
                     if ( bp->i_key )
                        safe_str(":1", s_protect_buff, &s_protect_ptr);
                  }
               }
               if ( Good_chk(i_return) )
                  atr_add(i_return, A_PROTECTNAME, s_protect_buff, i_return, 0);
            } else {
               notify(player, unsafe_tprintf("Somehow, the name '%s' has been aliased by someone else [#%d].", name, p_lookup));
            }
         } else {
            notify(player, unsafe_tprintf("You have successfully activated '%s' as an alias.", name));
            for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
               if ( bp->i_name == i_return ) {
                  if ( i_first )
                     safe_chr('\t', s_protect_buff, &s_protect_ptr);
                  i_first=1;
                  safe_str(bp->name, s_protect_buff, &s_protect_ptr);
                  if ( bp->i_key )
                     safe_str(":1", s_protect_buff, &s_protect_ptr);
               }
            }
            if ( Good_chk(i_return) )
               atr_add(i_return, A_PROTECTNAME, s_protect_buff, i_return, 0);
         }
         break;
      case PROTECT_UNALIAS:
         i_return = protectname_unalias(name, player);
         if ( i_return == -1 ) {
            notify(player, unsafe_tprintf("Un-Alias what?  The name '%s' is not in the protected list.", name));
         } else if ( i_return == -2 ) {
            notify(player, unsafe_tprintf("The name '%s' is already unmarked from being an alias.", name));
         } else if ( i_return == -3 ) {
            notify(player, unsafe_tprintf("The name '%s' is their current active name.  Can not unalias.", name));
         } else if ( i_return == -4 ) {
            notify(player, unsafe_tprintf("The name '%s' is their current @alias attribute.  Can not unalias.", name));
         } else if ( i_return == -5 ) {
            notify(player, unsafe_tprintf("The name '%s' is already unmarked from being an alias, but was still hashed as an alias.  Fixed.", name));
         } else {
            notify(player, unsafe_tprintf("You have successfully de-activated '%s' as an alias.", name));
            for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
               if ( bp->i_name == i_return ) {
                  if ( i_first )
                     safe_chr('\t', s_protect_buff, &s_protect_ptr);
                  i_first=1;
                  safe_str(bp->name, s_protect_buff, &s_protect_ptr);
                  if ( bp->i_key )
                     safe_str(":1", s_protect_buff, &s_protect_ptr);
               }
            }
            if ( Good_chk(i_return) )
               atr_add(i_return, A_PROTECTNAME, s_protect_buff, i_return, 0);
         }
         break;
      case PROTECT_LIST:
         protectname_list(player, i_listall, NOTHING);
         break;
      case PROTECT_BYPLAYER:
         if ( *name )
            target = lookup_player(player, name, 0);
         else
            target = NOTHING;
         protectname_list(player, 2, target);
         break;
      case PROTECT_SUMMARY:
         protectname_list(player, 1, NOTHING);
         break;
   }
   free_lbuf(s_protect_buff);
}

void do_hook(dbref player, dbref cause, int key, char *name) 
{
   int negate, found, sub_chk, sub_cntr, no_attr, aflags, no_attr2;
   dbref aobj;
   char *s_ptr, *s_ptrbuff, *cbuff, *p, *q, *tpr_buff, *tprp_buff, *sub_buff, 
        *sub_ptr, *sub_ptrbuff, *sub_buff2, *ret_buff, ret_char;
   char *sub_str[]={"n", "#", "!", "@", "l", "s", "o", "p", "a", "r", "t", "c", "x", "f", 
                    "k", "w", "m", ":", NULL};
   char *sub_atr[]={"SUB_N", "SUB_NUM", "SUB_BANG", "SUB_AT", "SUB_L", "SUB_S", "SUB_O", 
                    "SUB_P", "SUB_A", "SUB_R", "SUB_T", "SUB_C", "SUB_X", "SUB_F", "SUB_K", 
                    "SUB_W", "SUB_M", "SUB_COLON", NULL};
   CMDENT *cmdp;
   ATTR *ap, *ap2;
  
   cmdp = (CMDENT *)NULL;

   /* mudconf here for full hash table or just strict command table */

   if ( (key && !(key & HOOK_LIST)) || ((!key || (key & HOOK_LIST)) && *name) ) {
      cmdp = lookup_orig_command(name);
      if ( !cmdp ) { 
         notify(player, "@hook: non-existant command-name given."); 
         return;
      }
   }
   if ( (key & HOOK_CLEAR) && (key & HOOK_LIST) ) {
      notify(player, "@hook: incompatible switches.");
      return;
   }

   if ( key & HOOK_CLEAR ) {
      negate = 1;
      key = key & ~HOOK_CLEAR;
      key = key & ~SW_MULTIPLE;
   } else
      negate = 0;

   if ( key & (HOOK_BEFORE|HOOK_AFTER|HOOK_PERMIT|HOOK_IGNORE|HOOK_IGSWITCH|HOOK_AFAIL|HOOK_FAIL) ) {
      if (negate) {
         cmdp->hookmask = cmdp->hookmask & ~key;
      } else {
         cmdp->hookmask = cmdp->hookmask | key;
      }
      if ( cmdp->hookmask ) {
         s_ptr = s_ptrbuff = alloc_lbuf("@hook");
         show_hook(s_ptrbuff, s_ptr, cmdp->hookmask);
         notify(player, unsafe_tprintf("@hook: new mask for '%s' -> %s", cmdp->cmdname, s_ptrbuff));
         free_lbuf(s_ptrbuff);
      } else {
         notify(player, unsafe_tprintf("@hook: new mask for '%s' is empty.", cmdp->cmdname));
      }
   }
   if ( (key & HOOK_LIST) || !key ) {
      if ( cmdp ) {
         if ( cmdp->hookmask ) {
            s_ptr = s_ptrbuff = alloc_lbuf("@hook");
            show_hook(s_ptrbuff, s_ptr, cmdp->hookmask);
            notify(player, unsafe_tprintf("@hook: mask for hashed-command '%s' -> %s", cmdp->cmdname, s_ptrbuff));
            free_lbuf(s_ptrbuff);
         } else {
            notify(player, unsafe_tprintf("@hook: mask for hashed-command '%s' is empty.", cmdp->cmdname));
         }
      } else {
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         notify(player, unsafe_tprintf("%-32s | %s", "Built-in Command", "Hook Mask Values"));
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         found = 0;
         s_ptr = s_ptrbuff = alloc_lbuf("@hook");
         tprp_buff = tpr_buff = alloc_lbuf("do_hook");
         for (cmdp = (CMDENT *) hash_firstentry(&mudstate.command_htab);
          cmdp;
          cmdp = (CMDENT *) hash_nextentry(&mudstate.command_htab)) {

            if (! (cmdp->cmdtype & CMD_BUILTIN_e || cmdp->cmdtype & CMD_LOCAL_e)) {
           continue;
        }
        memset(s_ptrbuff, 0, LBUF_SIZE);
            s_ptr = s_ptrbuff;
            if ( cmdp->hookmask ) {
               found = 1;
               show_hook(s_ptrbuff, s_ptr, cmdp->hookmask);
               tprp_buff = tpr_buff;
               if ( strcmp(cmdp->cmdname, "S") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "S %-30.30s | %s", "('\"' hook on 'say')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "P") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "P %-30.30s | %s", "(':' or ';' hook on 'pose')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "E") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "E %-30.30s | %s", "('\\\\' hook on '@emit')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "F") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "F %-30.30s | %s", "('#' hook on '@force')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "V") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "V %-30.30s | %s", "('&' hook on '@set')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "C") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "C %-30.30s | %s", "('>' hook on '@set')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "M") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "M %-30.30s | %s", "('-' hook on 'mail')", s_ptrbuff));
               else if ( strcmp(cmdp->cmdname, "N") == 0 ) 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "N %-30.30s | %s", "('-' hook on 'no parsing')", s_ptrbuff));
               else 
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "%-32.32s | %s", cmdp->cmdname, s_ptrbuff));
            }
         }
         if ( !found )
            notify(player, unsafe_tprintf("%26s -- No @hooks defined --", " "));
         found = 0;
         /* We need to search the attribute table as well */
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         notify(player, unsafe_tprintf("%-32s | %s", "Built-in Attribute", "Hook Mask Values"));
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         cbuff = alloc_sbuf("cbuff_hook");
         for (ap = attr; ap->name; ap++) {
            memset(s_ptrbuff, 0, LBUF_SIZE);
            s_ptr = s_ptrbuff;
            p = cbuff;
            *p++ = '@';
            for (q = (char *) ap->name; *q; p++, q++)
                *p = ToLower((int)*q);
            *p = '\0';
            cmdp = lookup_orig_command(cbuff);
            if ( cmdp && cmdp->hookmask ) {
               found = 1;
               show_hook(s_ptrbuff, s_ptr, cmdp->hookmask);
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-32.32s | %s", cmdp->cmdname, s_ptrbuff));
            }
         }
         free_sbuf(cbuff);
         if ( !found )
            notify(player, unsafe_tprintf("%26s -- No @hooks defined --", " "));
         free_lbuf(s_ptrbuff);
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         notify(player, unsafe_tprintf("%-32s | %s", "Percent-Substitutions", "Override Status"));
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         found = 0;
         sub_cntr = 0;
         for ( sub_chk = 1; sub_atr[sub_cntr] != NULL ; sub_chk *= 2 ) {
            if ( mudconf.sub_override & sub_chk ) {
               found = 1;
               no_attr = 0;
               if ( !Good_obj(mudconf.hook_obj) )
                  no_attr = 1;
               if ( !no_attr ) {
                  ap2 = atr_str(sub_atr[sub_cntr]);
                  if ( !ap2 )
                     no_attr = 1;
               }
               if ( !no_attr) {
                  sub_buff = atr_pget(mudconf.hook_obj, ap2->number, &aobj, &aflags);
                  if ( !sub_buff || !*sub_buff)
                     no_attr = 1;
                  if ( sub_buff )
                     free_lbuf(sub_buff);
               }
               notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                              "<%%>-%-28s | %s", sub_str[sub_cntr], (no_attr ? "DISABLED" : "ENABLED") ));
            }
            sub_cntr++;
         }
         if ( *mudconf.sub_include && Good_obj(mudconf.hook_obj) ) {
            notify(player, unsafe_tprintf("%.32s-+-%s",
                   "------ Manual Definitions ------",
                   "--------------------------------------------"));
            sub_ptrbuff = alloc_sbuf("sub_include");
            for ( sub_ptr = mudconf.sub_include; *sub_ptr; sub_ptr++ ) {
               no_attr = no_attr2 = 0;
               ret_char = ' ';
               sprintf(sub_ptrbuff, "SUB_%c", *sub_ptr);
               if ( !ok_attr_name(sub_ptrbuff) ) 
                  sprintf(sub_ptrbuff, "SUB_%03d", (int)*sub_ptr);
               ap2 = atr_str(sub_ptrbuff);
               if ( !ap2 )
                  no_attr = no_attr2 = 1;
               if ( !no_attr ) {
                  found = 1;
                  sub_buff = atr_pget(mudconf.hook_obj, ap2->number, &aobj, &aflags);
                  if ( !sub_buff || !*sub_buff)
                     no_attr = no_attr2 = 1;
                  else {
                     sprintf(sub_ptrbuff, "CHR_%c", *sub_ptr);
                     if ( !ok_attr_name(sub_ptrbuff) ) 
                        sprintf(sub_ptrbuff, "CHR_%03d", (int)*sub_ptr);
                     ap2 = atr_str(sub_ptrbuff);
                     if ( !ap2 ) {
                        no_attr2 = 1;
                     } else {
                        sub_buff2 = atr_pget(mudconf.hook_obj, ap2->number, &aobj, &aflags);
                        if ( !sub_buff2 || !*sub_buff2 ) {
                           no_attr2 = 1;
                        } else {
                           ret_buff = exec(mudconf.hook_obj, player, player, EV_FCHECK | EV_EVAL, sub_buff2,
                                           (char **)NULL, 0, (char **)NULL, 0);
                           if ( *ret_buff )
                              ret_char = *ret_buff;
                           else
                              ret_char = ' ';
                           free_lbuf(ret_buff);
                        }
                        if ( sub_buff2 )
                           free_lbuf(sub_buff2);
                     }
                  }
                  if ( sub_buff )
                     free_lbuf(sub_buff);
               }
               notify( player, safe_tprintf(tpr_buff, &tprp_buff,
                               "%c%c%30s | %-12s %s%c%s", '%', *sub_ptr, " ", 
                               (no_attr ? "DISABLED" : "ENABLED"),
                               (no_attr2 ? " " : 
                                    (isdigit(ret_char) > 0 ? "[NUMERICAL OFFSET: '": "[SPECIAL CHAR: '" )),
                               (no_attr2 ? ' ' : ret_char),
                               (no_attr2 ? " " : "']")) );
            }
            free_sbuf(sub_ptrbuff);
         }
         if ( !found )
            notify(player, unsafe_tprintf("%20s -- No percent substitions defined --", " "));
         notify(player, unsafe_tprintf("%.32s-+-%s",
                "--------------------------------",
                "--------------------------------------------"));
         free_lbuf(tpr_buff);
         notify(player, unsafe_tprintf("The hook object is currently: #%d (%s)", mudconf.hook_obj,
                ((Good_obj(mudconf.hook_obj) && 
                  !Going(mudconf.hook_obj) && 
                  !Recover(mudconf.hook_obj)) ? "VALID" : "INVALID")));
      }
   }
}

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

void do_cluster(dbref player, dbref cause, int key, char *name, char *args[], int nargs)
{
   dbref thing, thing2, thing3, aowner;
   char *s_inbufptr, *s_instr, *s_tmpptr, *tpr_buff, *tprp_buff, *s_text,
        *s_strtok, *s_strtokptr, *s_return, *s_tmpstr, *xargs[MAX_ARG], *s_foo, *s_tmp;
   int anum, anum2, anum3, aflags, i_isequal, i_corrupted, i_temp, i_temp2, i_lowball, 
        i_highball, i_first, nxargs, i, i_clusterfunc, anum4, i_sideeffect,
        i_nomatch, i_nowipe, i_wipecnt, i_totobjs, i_regexp, i_preserve, i_owner;
   ATTR *attr;
   time_t starttme, endtme;
   double timechk;

   i_temp2 = i_temp = i_corrupted = i_isequal = i_lowball = i_highball = 0;
   i_nomatch = i_nowipe = i_wipecnt = i_totobjs = 0;
   anum = anum2 = anum3 = anum4 = NOTHING;
   s_strtokptr = NULL;

   s_foo = NULL;
   if ( !name || !*name ) {
      notify(player, "@cluster requires an argument.");
      return;
   }
   if ( !key ) {
      notify(player, "@cluster requires a switch.");
      return;
   }
   
   s_instr = alloc_lbuf("do_cluster");
   strcpy(s_instr, name);
   s_tmp = exec(player, cause, cause, EV_FCHECK | EV_EVAL, s_instr,
                  (char **)NULL, 0, (char **)NULL, 0);
   free_lbuf(s_instr);
   if ( !s_tmp || !*s_tmp ) {
      notify(player, "@cluster requires an argument.");
      free_lbuf(s_tmp);
      return;
   }

   i_regexp = 0;
   if ( key & CLUSTER_REGEXP ) {
      i_regexp = 1;
      key &= ~CLUSTER_REGEXP;
   }
   i_preserve = 0;
   if ( key & CLUSTER_PRESERVE ) {
      i_preserve = 1;
      key &= ~CLUSTER_PRESERVE;
   }
   i_owner = 0;
   if ( (key & CLUSTER_OWNER) && (key & CLUSTER_WIPE) ) {
      i_owner = 1;
      key &= ~CLUSTER_OWNER;
      if ( strchr(s_tmp, '/') != NULL ) {
         s_return = alloc_lbuf("cluster_return");
         strcpy(s_return, strchr(s_tmp, '/') + 1);
      } else {
         notify(player, "@cluster/wipe/owner requires a valid player target.");
         free_lbuf(s_tmp);
         return;
      }
   } else {
      s_return = alloc_lbuf("cluster_return");
      strcpy(s_return, s_tmp);
      free_lbuf(s_tmp);
      s_tmp = NULL;
   }
   if ( strchr(s_return, '=') ==  NULL ) {
      if ( strchr(s_return, '/') != NULL ) {
         s_inbufptr = s_instr = alloc_lbuf("do_cluster");
         s_tmpptr = s_return;
         while ( *s_tmpptr && (*s_tmpptr != '/') ) {
            safe_chr(*s_tmpptr, s_instr, &s_inbufptr);
            s_tmpptr++;
         }
         thing = match_thing(player, s_instr);
         free_lbuf(s_instr);
         s_tmpptr = s_return;
      } else {
         thing = match_thing(player, s_return);
         s_tmpptr = s_return;
      }
      i_isequal = 0;
   } else {
      i_isequal = 1;
      s_inbufptr = s_instr = alloc_lbuf("do_cluster");
      s_tmpptr = s_return;
      while ( *s_tmpptr && (*s_tmpptr != '=') ) {
         safe_chr(*s_tmpptr, s_instr, &s_inbufptr);
         s_tmpptr++;
      }
      if ( strchr(s_instr, '/') != 0 ) {
         s_inbufptr = s_instr;
         while ( *s_inbufptr && (*s_inbufptr != '/' ) )
            s_inbufptr++;
         *s_inbufptr = '\0';
      }
      thing = match_thing(player, s_instr);
      free_lbuf(s_instr);
      s_tmpptr++;
   }

   if ( !Good_chk(thing) || !isThing(thing) ) {
      notify(player, "Cluster object must be a valid object.");
      free_lbuf(s_return);
      if ( s_tmp )
         free_lbuf(s_tmp);
      return;
   }

   if ( !(Cluster(player) || Controls(player, thing)) ) {
      notify(player, "You have no control over that cluster.");
      free_lbuf(s_return);
      if ( s_tmp )
         free_lbuf(s_tmp);
      return;
   }

   tprp_buff = tpr_buff = alloc_lbuf("do_cluster_tprbuff");
   i_clusterfunc = i_sideeffect = 0;
   if ( key & CLUSTER_FUNC ) {
      i_clusterfunc = 1;
      key = key & ~CLUSTER_FUNC;
   }
   if ( key & SIDEEFFECT ) {
      i_sideeffect = 1;
      key = key & ~SIDEEFFECT;
   }
   switch (key) {
      case CLUSTER_NEW:  
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/new.  Use: @cluster/new <dbref>");
         } else if ( Cluster(thing) ) {
            notify(player, "That object is already a cluster.");
         } else {
            anum = mkattr("_CLUSTER");
            if ( anum > 0 ) {
               attr = atr_num(anum);
               if ( !attr ) {
                  notify(player, "Can not make cluster attribute!  Aborting!");
               } else {
                  atr_add_raw(thing,attr->number,safe_tprintf(tpr_buff, &tprp_buff, "#%d", thing));
                  tprp_buff = tpr_buff;
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                         "Cluster for object #%d has been created.", thing));
                  s_Toggles(thing, Toggles(thing) | TOG_CLUSTER);
                  atr_set_flags(thing, attr->number, AF_INTERNAL|AF_GOD);
               }
            } else {
               notify(player, "Can not make cluster attribute!  Aborting!");
            }
         }
         break;
/*
      case CLUSTER_CLONE:
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/clone.  Use: @cluster/clone <dbref#>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            anum = mkattr("_CLUSTER");
            if ( anum > 0 ) {
               attr = atr_num(anum);
               if ( !attr ) {
                  notify(player, "Can not make cluster attribute!  Aborting!");
               } else {
                  s_text = atr_get(thing, attr->number, &aowner, &aflags);
                  if ( !*s_text ) {
                     notify(player, "The cluster listing got corrupted!  Aborting!");
                  }
                  free_lbuf(s_text);
// Check if player has quota to clone
// Check if player has permission to clone
// Keep track of every dbref# that is being cloned.  int[LBUF/2] works well for this.
// Clone the object, keep track of it, and when done, set _CLUSTER attrib and CLUSTER toggle
               }
            } else {
               notify(player, "Can not make cluster attribute!  Aborting!");
            }
         }
         break;
*/
      case CLUSTER_ADD:  
         if ( !i_isequal ) {
            if ( i_sideeffect ) 
               notify(player, "Invalid syntax for cluster_add().  Use: cluster_add(dbref,dbref)");
            else
               notify(player, "Invalid syntax for @cluster/add.  Use: @cluster/add <dbref>=<dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            anum = mkattr("_CLUSTER");
            if ( anum > 0 ) {
               attr = atr_num(anum);
               if ( !attr ) {
                  notify(player, "Can not make cluster attribute!  Aborting!");
               } else {
                  s_text = atr_get(thing, attr->number, &aowner, &aflags);
                  if ( !*s_text ) {
                     notify(player, "The cluster listing got corrupted!  Aborting!");
                  } else if ( strlen(s_text) > (LBUF_SIZE-20) ) {
                     notify(player, "The cluster is too large to add another object.");
                  } else {
                     thing2 = match_thing(player, s_tmpptr);
                     if ( !Good_chk(thing2) || !isThing(thing2) ) {
                        notify(player, "That object isn't a valid object for a cluster.");
                     } else if ( Cluster(thing2) ) {
                        notify(player, 
                               "That object is already in a cluster.  Use @cluster/del to delete it first.");
                     } else {
                        s_inbufptr = s_instr = alloc_lbuf("do_cluster_add");
                        safe_str(s_text, s_instr, &s_inbufptr);
                        s_strtok = strtok_r(s_instr, " ", &s_strtokptr);
                        i_corrupted = 0;
                        while ( s_strtok ) {
                           thing3 = match_thing(player, s_strtok);
                           if ( !Good_chk(thing3) || !isThing(thing3) || !Cluster(thing3) ) {
                              notify(player, "The cluster listing got corrupted!  Aborting!");
                              i_corrupted = 1;
                              break;
                           }
                           tprp_buff = tpr_buff;
                           atr_add_raw(thing3, attr->number, safe_tprintf(tpr_buff, &tprp_buff, 
                                       "%s #%d", s_text, thing2));
                           atr_set_flags(thing3, attr->number, AF_INTERNAL|AF_GOD);
                           s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                        }
                        free_lbuf(s_instr);
                        if ( i_corrupted )
                           break;
                        tprp_buff = tpr_buff;
                        atr_add_raw(thing2, attr->number, safe_tprintf(tpr_buff, &tprp_buff, 
                                    "%s #%d", s_text, thing2));
                        tprp_buff = tpr_buff;
                        notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                               "Cluster object #%d has been added to %s.", thing2, s_text));
                        s_Toggles(thing2, Toggles(thing2) | TOG_CLUSTER);
                        atr_set_flags(thing2, attr->number, AF_INTERNAL|AF_GOD);
                        s_Flags(thing2, Flags(thing));
                        s_Flags2(thing2, Flags2(thing));
                        s_Flags3(thing2, Flags3(thing));
                        s_Flags4(thing2, Flags4(thing));
                        s_Toggles(thing2, Toggles(thing));
                        s_Toggles2(thing2, Toggles2(thing));
                        s_Toggles3(thing2, Toggles3(thing));
                        s_Toggles4(thing2, Toggles4(thing));
                        s_Toggles5(thing2, Toggles5(thing));
                        s_Toggles6(thing2, Toggles6(thing));
                        s_Toggles7(thing2, Toggles7(thing));
                        s_Toggles8(thing2, Toggles8(thing));
                        attr = atr_str("_CLUSTER_THRESH");
                        if ( attr ) {
                           s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                           if ( *s_instr ) {
                              atr_add_raw(thing2, attr->number, s_instr);
                              atr_set_flags(thing2, attr->number, AF_INTERNAL|AF_GOD);
                           }
                           free_lbuf(s_instr);
                        }
                        attr = atr_str("_CLUSTER_ACTION");
                        if ( attr ) {
                           s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                           if ( *s_instr ) {
                              atr_add_raw(thing2, attr->number, s_instr);
                              atr_set_flags(thing2, attr->number, AF_INTERNAL|AF_GOD);
                           }
                           free_lbuf(s_instr);
                        }
                        attr = atr_str("_CLUSTER_ACTION_FUNC");
                        if ( attr ) {
                           s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                           if ( *s_instr ) {
                              atr_add_raw(thing2, attr->number, s_instr);
                              atr_set_flags(thing2, attr->number, AF_INTERNAL|AF_GOD);
                           }
                           free_lbuf(s_instr);
                        }
                     }
                  }
                  free_lbuf(s_text);
               }
            } else {
               notify(player, "Can not make cluster attribute!  Aborting!");
            }
         }
         break;
      case CLUSTER_DEL:  
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/del.  Use: @cluster/del <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            anum = mkattr("_CLUSTER");
            if ( anum > 0 ) {
               attr = atr_num(anum);
               if ( !attr ) {
                  notify(player, "Can not make cluster attribute!  Aborting!");
               } else {
                  s_text = atr_get(thing, attr->number, &aowner, &aflags);
                  if ( !*s_text ) {
                     notify(player, "The cluster listing got corrupted!  Aborting!");
                  } else {
                     s_inbufptr = s_instr = alloc_lbuf("do_cluster_del");
                     s_tmpstr = alloc_lbuf("do_cluster_del2");
                     strcpy(s_tmpstr, s_text);
                     s_strtok = strtok_r(s_tmpstr, " ", &s_strtokptr);
                     i_corrupted = i_first = 0;
                     while ( s_strtok ) {
                        thing3 = match_thing(player, s_strtok);
                        if ( thing3 == thing ) {
                           s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                           continue;
                        }
                        if ( !Good_chk(thing3) || !isThing(thing3) || !Cluster(thing3) ) {
                           notify(player, "The cluster listing got corrupted!  Aborting!");
                           i_corrupted = 1;
                           break;
                        }
                        if ( i_first )
                           safe_chr(' ', s_instr, &s_inbufptr);
                        i_first = 1;
                        safe_str(s_strtok, s_instr, &s_inbufptr);
                        s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                     }
                     if ( i_corrupted ) {
                        free_lbuf(s_instr);
                        free_lbuf(s_tmpstr);
                        break;
                     }
                     strcpy(s_tmpstr, s_text);
                     s_strtok = strtok_r(s_tmpstr, " ", &s_strtokptr);
                     while ( s_strtok ) {
                        thing3 = match_thing(player, s_strtok);
                        atr_add_raw(thing3, attr->number, s_instr);
                        atr_set_flags(thing3, attr->number, AF_INTERNAL|AF_GOD);
                        s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                     }
                     tprp_buff = tpr_buff;
                     if ( *s_instr ) {
                        notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                               "Cluster object #%d has been deleted from %s.", thing, s_instr));
                     } else {
                        notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                               "Cluster object #%d has been deleted.  Cluster removed.", thing, s_instr));
                     }
                     s_Toggles(thing, Toggles(thing) & ~TOG_CLUSTER);
                     attr = atr_str("_CLUSTER");
                     if ( attr ) {
                        atr_clr(thing, attr->number);
                     }
                     attr = atr_str("_CLUSTER_THRESH");
                     if ( attr ) {
                        atr_clr(thing, attr->number);
                     }
                     attr = atr_str("_CLUSTER_ACTION");
                     if ( attr ) {
                        atr_clr(thing, attr->number);
                     }
                     attr = atr_str("_CLUSTER_ACTION_FUNC");
                     if ( attr ) {
                        atr_clr(thing, attr->number);
                     }
                     free_lbuf(s_instr);
                     free_lbuf(s_tmpstr);
                  }
                  free_lbuf(s_text);
               }
            } else {
               notify(player, "Can not make cluster attribute!  Aborting!");
            }
         }
         break;
      case CLUSTER_LIST: 
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/list.  Use: @cluster/list <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                   "Showing cluster statistics for cluster with member #%d.", thing));
            i_isequal = 1;
            attr = atr_str("_CLUSTER_THRESH");
            if ( attr ) {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( *s_text )
                  i_isequal++;
               free_lbuf(s_text);
            }
            attr = atr_str("_CLUSTER_ACTION");
            if ( attr ) {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( *s_text )
                  i_isequal++;
               free_lbuf(s_text);
            }
            attr = atr_str("_CLUSTER_ACTION_FUNC");
            if ( attr ) {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( *s_text )
                  i_isequal++;
               free_lbuf(s_text);
            }
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  i_temp = countwordsnew(s_text);
                  if ( i_temp <= 0 ) {
                     i_temp = 1;
                  }
                  tprp_buff = tpr_buff;
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                         "%d total members: %s", i_temp, s_text));
                  s_inbufptr = s_instr = alloc_lbuf("do_cluster_list");
                  safe_str(s_text, s_instr, &s_inbufptr);
                  s_strtok = strtok_r(s_instr, " ", &s_strtokptr);
                  i_corrupted = i_temp2 = i_highball = 0;
                  i_lowball = 2000000000;
                  while ( s_strtok ) {
                     thing3 = match_thing(player, s_strtok);
                     if ( !Good_chk(thing3) || !isThing(thing3) || !Cluster(thing3) ) {
                        notify(player, "The cluster listing is corrupt!");
                        i_corrupted = 1;
                        break;
                     }
                     i_temp2 += (db[thing3].nvattr - i_isequal);
                     if ( (db[thing3].nvattr - i_isequal) < i_lowball ) {
                        i_lowball = (db[thing3].nvattr - i_isequal);
                     }
                     if ( (db[thing3].nvattr - i_isequal) > i_highball ) {
                        i_highball = (db[thing3].nvattr - i_isequal);
                     }
                     s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                  }
                  if ( i_temp2 < 0 ) 
                     i_temp2 = 0;
                  if ( i_lowball < 0 )
                     i_lowball = 0;
                  if ( i_highball < 0 )
                     i_highball = 0;
                  free_lbuf(s_instr);
                  if ( !i_corrupted ) {
                     tprp_buff = tpr_buff;
                     notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                    "%d total attribut%s  [Highest: %d,  Lowest: %d,  Average: %d].",
                                    i_temp2, ((i_temp2 == 1) ? "e" : "es"), 
                                    i_highball, i_lowball, (i_temp2 / i_temp)) );
                     attr = atr_str("_CLUSTER_THRESH");
                     if ( attr ) {
                        s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                        if ( *s_instr ) {
                           tprp_buff = tpr_buff;
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                          "Threshold set to: %s", s_instr));
                        } else {
                           notify(player, "Threshold has not been set.");
                        }
                        free_lbuf(s_instr);
                     } else {
                        notify(player, "Threshold has not been set.");
                     }
                     attr = atr_str("_CLUSTER_ACTION");
                     if ( attr ) {
                        s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                        if ( *s_instr ) {
                           tprp_buff = tpr_buff;
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                          "Threshold Action: %.3970s", s_instr));
                           tprp_buff = tpr_buff;
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                          "Action retrigger limit: %ds", mudconf.cluster_cap));
                        } else {
                           notify(player, "Threshold action has not been set.");
                        }
                        free_lbuf(s_instr);
                     } else {
                        notify(player, "Threshold action has not been set.");
                     }
                     attr = atr_str("_CLUSTER_ACTION_FUNC");
                     if ( attr ) {
                        s_instr = atr_get(thing, attr->number, &aowner, &aflags);
                        if ( *s_instr ) {
                           tprp_buff = tpr_buff;
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                          "Threshold Action Function: %.3970s", s_instr));
                           tprp_buff = tpr_buff;
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                          "Action Function retrigger limit: %ds", mudconf.clusterfunc_cap));
                           notify(player, "Note: The threshold function has priority over normal action.");
                        } else {
                           notify(player, "Threshold action function has not been set.");
                        }
                        free_lbuf(s_instr);
                     } else {
                        notify(player, "Threshold action function has not been set.");
                     }
                  }
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_CLEAR: 
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/clear.  Use: @cluster/clear <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "Cluster storage attribute was not creatable.  Unable to repair critical error!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                  i_lowball = 0;
                  while ( s_strtok ) {
                     i_lowball++;
                     thing3 = match_thing(player, s_strtok);
                     s_Toggles(thing3, Toggles(thing3) & ~TOG_CLUSTER);
                     attr = atr_str("_CLUSTER");
                     if ( attr ) {
                        atr_clr(thing3, attr->number);
                     }
                     attr = atr_str("_CLUSTER_THRESH");
                     if ( attr ) {
                        atr_clr(thing3, attr->number);
                     }
                     attr = atr_str("_CLUSTER_ACTION");
                     if ( attr ) {
                        atr_clr(thing3, attr->number);
                     }
                     attr = atr_str("_CLUSTER_ACTION_FUNC");
                     if ( attr ) {
                        atr_clr(thing3, attr->number);
                     }
                     s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                  }
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                 "Cluster removed with member #%d.  A total of %d objects de-clustered.", thing, i_lowball));
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_GREP:
         if ( !i_isequal ) {
            notify(player, "Invalid syntax for @cluster/grep.  Use: @cluster/grep <dbref>=<attribute(s)>,<string>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               parse_arglist(player, cause, cause, s_tmpptr, '\0',
                             EV_STRIP_LS | EV_STRIP_TS,
                             xargs, MAX_ARG, (char **)NULL, 0, 0, (char **)NULL, 0);
               for (nxargs = 0; (nxargs < MAX_ARG) && xargs[nxargs]; nxargs++);
               if ( (nxargs > 1) && xargs[0] && *xargs[0] ) {
                  s_text = atr_get(thing, attr->number, &aowner, &aflags);
                  if ( !*s_text ) {
                     notify(player, "The cluster list is corrupt!");
                  } else {
                     notify(player, "Cluster grep: (multiple)");
                     s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                     while ( s_strtok && !mudstate.chkcpu_toggle ) {
                        thing3 = match_thing(player, s_strtok);
                        if ( Good_chk(thing3) && Cluster(thing3) ) {
                           if ( i_regexp ) {
                              s_tmpptr = grep_internal_regexp(player, thing3, xargs[1], xargs[0], 0, 0);
                           } else {
                              s_tmpptr = grep_internal(player, thing3, xargs[1], xargs[0], 0);
                           }
                           if ( *s_tmpptr ) {
                              notify(player, safe_tprintf(tpr_buff, &tprp_buff, "====[Grep]---> %s", s_strtok));
                              notify(player, s_tmpptr);
                           }
                           free_lbuf(s_tmpptr);
                        }
                        s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                     }
                     notify(player, "Cluster grep completed.");
                  }
                  free_lbuf(s_text);
               } else {
                  notify(player, "Invalid syntax for @cluster/grep.  Use: @cluster/grep <dbref>=<attribute(s)>,<string>");
               }
               for (i = 0; ((i < nxargs) && (i < MAX_ARG)); i++) {
                  if (xargs[i]) {
                     free_lbuf(xargs[i]);
                  }
               }
            }
         }
         break;
      case CLUSTER_REACTION: 
         if ( !i_isequal ) {
            notify(player, "Invalid syntax for @cluster/reaction.  Use: @cluster/reaction <dbref>=<old string>,<new string>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            if ( i_clusterfunc )
               anum2 = mkattr("_CLUSTER_ACTION_FUNC");
            else
               anum2 = mkattr("_CLUSTER_ACTION");
            if ( anum2 < 0 ) {
               notify(player, "Unable to create action attribute for cluster!");
               break;
            }
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  parse_arglist(player, cause, cause, s_tmpptr, '\0',
                                EV_STRIP_LS | EV_STRIP_TS,
                                xargs, MAX_ARG, (char **)NULL, 0, 0, (char **)NULL, 0);
                  for (nxargs = 0; (nxargs < MAX_ARG) && xargs[nxargs]; nxargs++);
                  if ( (nxargs > 1) && xargs[0] && *xargs[0] ) {
                     s_instr = atr_get(thing, anum2, &aowner, &aflags);
                     if ( !*s_instr ) {
                        notify(player, "No action set for cluster.  Use @cluster/action instead.");
                     } else {
                        edit_string(s_instr, &s_inbufptr, &s_tmpstr, xargs[0], xargs[1], 0, 0, 0, 0);
                        s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                        while ( s_strtok ) {
                           thing3 = match_thing(player, s_strtok);
                           if ( Good_chk(thing3) && Cluster(thing3) ) {
                              atr_add_raw(thing3, anum2, s_inbufptr);
                              atr_set_flags(thing3, anum2, AF_INTERNAL|AF_GOD);
                           }
                           s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                        }
                        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Cluster ActionList: %s", s_tmpstr));
                        free_lbuf(s_tmpstr);
                        free_lbuf(s_inbufptr);
                     }
                     free_lbuf(s_instr);
                  } else {
                     notify(player, "Invalid syntax for @cluster/reaction.  Use: @cluster/reaction <dbref>=<old string>,<new string>");
                  }
                  for (i = 0; ((i < nxargs) && (i < MAX_ARG)); i++) {
                     if (xargs[i]) {
                        free_lbuf(xargs[i]);
                     }
                  }
               }  
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_ACTION: 
         if ( !i_isequal ) {
            notify(player, "Invalid syntax for @cluster/action.  Use: @cluster/action <dbref>=<string>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            if ( i_clusterfunc )
               anum2 = mkattr("_CLUSTER_ACTION_FUNC");
            else
               anum2 = mkattr("_CLUSTER_ACTION");
            if ( anum2 < 0 ) {
               notify(player, "Unable to create action attribute for cluster!");
               break;
            }
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  s_instr = name;

                  while ( *s_instr && (*s_instr != '=') )
                     s_instr++;
                  if ( *s_instr)
                     s_instr++;

                  s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                  while ( s_strtok ) {
                     thing3 = match_thing(player, s_strtok);
                     if ( Good_chk(thing3) && isThing(thing3) && Cluster(thing3) ) {
                        if ( *s_instr ) {
                           atr_add_raw(thing3, anum2, s_instr);
                           atr_set_flags(thing3, anum2, AF_INTERNAL|AF_GOD);
                        } else {
                           atr_clr(thing3, anum2);
                        }
                     }
                     s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                  }
                  tprp_buff = tpr_buff;
                  if ( *s_instr ) {
                     notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                    "Cluster action for cluster #%d set to: %s", thing, s_instr));
                  } else {
                     notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                    "Cluster action for cluster #%d cleared.", thing));
                  }
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_TRIGGER:
         if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               anum = attr->number;
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  s_inbufptr = s_instr = alloc_lbuf("cluster_set");
                  s_strtok = s_return;
                  while ( *s_strtok && (*s_strtok != '=') ) {
                     safe_chr(*s_strtok, s_instr, &s_inbufptr);
                     s_strtok++;
                  }
                  if ( *s_strtok )
                     s_strtok++;
                  if ( strchr(s_instr, '/') != NULL ) {
                     anum3 = NOTHING;
                     s_strtokptr = s_instr;
                     while ( *s_strtokptr && (*s_strtokptr != '/') ) {
                        s_strtokptr++;
                     }
                     if ( *s_strtokptr ) 
                        s_strtokptr++;
                     attr = atr_str(s_strtokptr);
                     if ( attr ) {
                        s_tmpstr = find_cluster(thing, player, attr->number);
                        if ( *s_strtok ) {
                       parse_arglist(player, cause, cause, s_tmpptr, '\0',
                             EV_STRIP_LS | EV_STRIP_TS,
                             xargs, MAX_ARG, (char **)NULL, 0, 0, (char **)NULL, 0);
                       for (nxargs = 0; (nxargs < MAX_ARG) && xargs[nxargs]; nxargs++)
                              ; /* We just want to increment the counter to count it */
                           s_strtok = alloc_lbuf("cluster_trigger_build");
                           sprintf(s_strtok, "%.32s/%.3900s", s_tmpstr, s_strtokptr);
                           do_trigger(player, cause, (TRIG_QUIET), s_strtok, xargs, nxargs);
                           free_lbuf(s_strtok);
                       for (i = 0; ((i < nxargs) && (i < MAX_ARG)); i++) {
                       if (xargs[i]) {
                           free_lbuf(xargs[i]);
                               }
                           }
                        } else {
                           s_strtok = alloc_lbuf("cluster_trigger_build");
                           sprintf(s_strtok, "%.32s/%.3900s", s_tmpstr, s_strtokptr);
                           do_trigger(player, cause, (TRIG_QUIET), s_strtok, (char **)NULL, 0);
                           free_lbuf(s_strtok);
                        }
                        free_lbuf(s_tmpstr);
                     } else {
                        notify(player, "No match.");
                     }
                  } else {
                     notify(player, "No match.");
                  }
                  free_lbuf(s_instr);
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_EDIT: 
         if ( !i_isequal ) {
            notify(player, "Invalid syntax for @cluster/edit.  Use: @cluster/edit <dbref>[/<attr(s)>]=<old>,<new>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               anum = attr->number;
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  if ( !*s_tmpptr ) {
                     notify(player, "Nothing to edit for the cluster.");
                  } else {
                     s_inbufptr = s_instr = alloc_lbuf("cluster_set");
                     s_strtok = s_return;
                     while ( *s_strtok && (*s_strtok != '=') ) {
                        safe_chr(*s_strtok, s_instr, &s_inbufptr);
                        s_strtok++;
                     }
                     if ( *s_strtok )
                        s_strtok++;
                     if ( strchr(s_instr, '/') != NULL ) {
                        anum3 = NOTHING;
                        s_strtokptr = s_instr;
                        while ( *s_strtokptr && (*s_strtokptr != '/') ) {
                           s_strtokptr++;
                        }
                        if ( *s_strtokptr ) 
                           s_strtokptr++;
                        if ( *s_strtokptr ) {
                           if ( (strchr(s_strtokptr, '*') != NULL) ||
                                (strchr(s_strtokptr, '?') != NULL) ) {
                          parse_arglist(player, cause, cause, s_tmpptr, '\0',
                                EV_STRIP_LS | EV_STRIP_TS,
                                xargs, MAX_ARG, (char **)NULL, 0, 0, (char **)NULL, 0);
                          for (nxargs = 0; (nxargs < MAX_ARG) && xargs[nxargs]; nxargs++)
                                 ; /* We just want to count the variable */
                              if ( (nxargs > 1) && xargs[0] && *xargs[0] ) {
                                 s_strtok = alloc_lbuf("cluster_set_build");
                                 s_tmpstr = strtok_r(s_text, " ", &s_inbufptr);
                                 notify(player, "Cluster editing: (multiple)");
                                 while ( s_tmpstr && !mudstate.chkcpu_toggle ) {
                                    sprintf(s_strtok, "%.32s/%.3900s", s_tmpstr, s_strtokptr);
                                    tprp_buff = tpr_buff;
                                    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "====[Edit]---> %s", s_tmpstr));
                                    do_edit(player, cause, 0, s_strtok, xargs, nxargs);
                                    s_tmpstr = strtok_r(NULL, " ", &s_inbufptr);
                                 }
                                 notify(player, "Cluster editing complete.");
                                 free_lbuf(s_strtok);
                              } else {
                                 notify(player, "Nothing to edit for the cluster.");
                              }
                          for (i = 0; ((i < nxargs) && (i < MAX_ARG)); i++) {
                          if (xargs[i]) {
                              free_lbuf(xargs[i]);
                                  }
                              }
                           } else {
                              attr = atr_str(s_strtokptr);
                              if ( attr )
                                 anum3 = attr->number;
                              if ( anum3 == NOTHING ) {
                                 notify(player, "No matching attributes in cluster.");
                              } else {
                             parse_arglist(player, cause, cause, s_tmpptr, '\0',
                                   EV_STRIP_LS | EV_STRIP_TS,
                                   xargs, MAX_ARG, (char **)NULL, 0, 0, (char **)NULL, 0);
                             for (nxargs = 0; (nxargs < MAX_ARG) && xargs[nxargs]; nxargs++)
                                    ; /* We just want to count the variable */
                                 if ( (nxargs > 1) && xargs[0] && *xargs[0] ) {
                                    s_tmpstr = find_cluster(thing, player, anum3);
                                    s_strtok = alloc_lbuf("cluster_set_build");
                                    sprintf(s_strtok, "%.32s/%.3900s", s_tmpstr, s_strtokptr);
                                    tprp_buff = tpr_buff;
                                    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Cluster editing: (single %s)", s_tmpstr));
                                    do_edit(player, cause, 0, s_strtok, xargs, nxargs);
                                    notify(player, "Cluster editing complete.");
                                    free_lbuf(s_strtok);
                                    free_lbuf(s_tmpstr);
                                 } else {
                                    notify(player, "Nothing to edit for the cluster.");
                                 }
                             for (i = 0; ((i < nxargs) && (i < MAX_ARG)); i++) {
                             if (xargs[i]) {
                                 free_lbuf(xargs[i]);
                                     }
                                 }
                              }
                           }
                        } else {
                           notify(player, "No matching attributes in cluster.");
                        }
                     } else {
                        notify(player, "Invalid cluster specified for edit.");
                     }
                     free_lbuf(s_instr);
                  }
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_THRESHOLD: 
         if ( !i_isequal) {
            notify(player, "Invalid syntax for @cluster/threshold.  Use: @cluster/threshold <dbref>=<number>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            anum2 = mkattr("_CLUSTER_THRESH");
            if ( anum2 < 0 ) {
               notify(player, "Unable to create threshold attribute for cluster!");
               break;
            }
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "The cluster list is corrupt!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( !*s_text ) {
                  notify(player, "The cluster list is corrupt!");
               } else {
                  anum3 = atoi(s_tmpptr);

                  s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                  while ( s_strtok ) {
                     thing3 = match_thing(player, s_strtok);
                     if ( Good_chk(thing3) && isThing(thing3) && Cluster(thing3) ) {
                        if ( anum3 > 0 ) {
                           tprp_buff = tpr_buff;
                           atr_add_raw(thing3, anum2, 
                                       safe_tprintf(tpr_buff, &tprp_buff, "%d", anum3));
                           atr_set_flags(thing3, anum2, AF_INTERNAL|AF_GOD);
                        } else {
                           atr_clr(thing3, anum2);
                        }
                     }
                     s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                  }
                  tprp_buff = tpr_buff;
                  if ( anum3 > 0 ) {
                     notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                    "Cluster threshold for cluster #%d set to: %d", thing, anum3));
                  } else {
                     notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                    "Cluster threshold for cluster #%d has been cleared.", thing));
                  }
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_CUT:
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/cut.  Use: @cluster/cut <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( attr )
               atr_clr(thing, attr->number);
            attr = atr_str("_CLUSTER_THRESH");
            if ( attr )
               atr_clr(thing, attr->number);
            attr = atr_str("_CLUSTER_ACTION");
            if ( attr )
               atr_clr(thing, attr->number);
            attr = atr_str("_CLUSTER_ACTION_FUNC");
            if ( attr )
               atr_clr(thing, attr->number);
            s_Toggles(thing, Toggles(thing) & ~TOG_CLUSTER);
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Dbref #%d has been forcefully declustered.", thing));
            notify(player, "It is strongly recommended that you @cluster/repair the remaining items in the cluster.");
         }
         break;
      case CLUSTER_REPAIR: 
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/repair.  Use: @cluster/repair <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "Cluster storage attribute was not creatable.  Unable to repair critical error!");
            } else {
               anum = attr->number;
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( s_text && !*s_text ) {
                  notify(player, "Cluster list corrupt for specified object.  Choose another object in cluster.");
                  free_lbuf(s_text);
                  break;
               }
               s_tmpstr = alloc_lbuf("cluster_repair");
               s_inbufptr = s_instr = alloc_lbuf("cluster_repair");
               strcpy(s_tmpstr, s_text);
               s_strtok = strtok_r(s_tmpstr, " ", &s_strtokptr);
               i_highball = i_lowball = i_corrupted = 0;
               notify(player, "Step #1: Walking object cluster membership.");
               while ( s_strtok ) {
                  thing3 = match_thing(player, s_strtok);
                  if ( Good_chk(thing3) ) {
                     if ( Cluster(thing3) ) {
                        if ( isThing(thing3) ) {
                           if ( i_highball ) {
                              safe_chr(' ', s_instr, &s_inbufptr);
                           }
                           safe_str(s_strtok, s_instr, &s_inbufptr);
                           i_highball++;
                        } else {
                           s_Toggles(thing3, Toggles(thing3) & ~TOG_CLUSTER);
                           attr = atr_str("_CLUSTER");
                           if ( attr ) {
                              atr_clr(thing3, attr->number);
                           }
                           attr = atr_str("_CLUSTER_THRESH");
                           if ( attr ) {
                              atr_clr(thing3, attr->number);
                           }
                           attr = atr_str("_CLUSTER_ACTION");
                           if ( attr ) {
                              atr_clr(thing3, attr->number);
                           }
                           attr = atr_str("_CLUSTER_ACTION_FUNC");
                           if ( attr ) {
                              atr_clr(thing3, attr->number);
                           }
                           i_corrupted++;
                        }
                     } else {
                        i_corrupted++;
                     }
                  } else {
                     i_corrupted++;
                  }
                  s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                  i_lowball++;
               }

               strcpy(s_tmpstr, s_text);
               s_strtok = strtok_r(s_tmpstr, " ", &s_strtokptr);
               i_lowball = 0;
               notify(player, "Step #2: Sanitizing membership lists.");
               while ( s_strtok ) {
                  thing3 = match_thing(player, s_strtok);
                  s_inbufptr = atr_get(thing3, anum, &aowner, &aflags);
                  if ( strcmp(s_inbufptr, s_instr) != 0 ) {
                     i_lowball++;
                     atr_add_raw(thing3, anum, s_instr);
                     atr_set_flags(thing3, anum, AF_INTERNAL|AF_GOD);
                  }
                  free_lbuf(s_inbufptr);
                  s_strtok = strtok_r(NULL, " ", &s_strtokptr);
               }
               free_lbuf(s_tmpstr);
               free_lbuf(s_instr);

               anum2 = anum3 = NOTHING;
               attr = atr_str("_CLUSTER_THRESH");
               if ( attr ) {
                  anum2 = attr->number;
                  s_inbufptr = atr_get(thing, anum2, &aowner, &aflags);
               } else {
                  s_inbufptr = alloc_lbuf("cluster_thresh_repair");
               }
               attr = atr_str("_CLUSTER_ACTION");
               if ( attr ) {
                  anum3 = attr->number;
                  s_instr = atr_get(thing, anum3, &aowner, &aflags);
               } else {
                  s_instr = alloc_lbuf("cluster_tresh_action");
               }
               attr = atr_str("_CLUSTER_ACTION_FUNC");
               if ( attr ) {
                  anum4 = attr->number;
                  s_foo = atr_get(thing, anum4, &aowner, &aflags);
               } else {
                  s_foo = alloc_lbuf("cluster_tresh_action_func");
               }
               s_strtok = strtok_r(s_text, " ", &s_strtokptr);
               i_isequal = 0;
               notify(player, "Step #3: Sanitizing thresholds and action lists.");
               while ( s_strtok ) {
                  thing3 = match_thing(player, s_strtok);
                  if ( *s_inbufptr ) {
                     s_tmpstr = atr_get(thing3, anum2, &aowner, &aflags);
                     if ( strcmp(s_tmpstr, s_inbufptr) != 0 ) {
                        atr_add_raw(thing3, anum2, s_inbufptr);
                        atr_set_flags(thing3, anum2, AF_INTERNAL|AF_GOD);
                        i_isequal++;
                     }
                     free_lbuf(s_tmpstr);
                  }
                  if ( *s_instr ) {
                     s_tmpstr = atr_get(thing3, anum3, &aowner, &aflags);
                     if ( strcmp(s_tmpstr, s_instr) != 0 ) {
                        atr_add_raw(thing3, anum3, s_instr);
                        atr_set_flags(thing3, anum3, AF_INTERNAL|AF_GOD);
                        i_isequal++;
                     }
                     free_lbuf(s_tmpstr);
                  }
                  if ( *s_foo ) {
                     s_tmpstr = atr_get(thing3, anum4, &aowner, &aflags);
                     if ( strcmp(s_tmpstr, s_foo) != 0 ) {
                        atr_add_raw(thing3, anum4, s_foo);
                        atr_set_flags(thing3, anum4, AF_INTERNAL|AF_GOD);
                        i_isequal++;
                     }
                     free_lbuf(s_tmpstr);
                  }
                  s_strtok = strtok_r(NULL, " ", &s_strtokptr);
               }
               free_lbuf(s_instr);
               free_lbuf(s_inbufptr);
               free_lbuf(s_text);
               free_lbuf(s_foo);
               if ( i_lowball || i_corrupted || i_isequal) {
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "Cluster list has been repaired.  %d broken chains repaired, %d thresholds repaired, %d invalid cluster items yanked.", 
                                 i_lowball, i_isequal, i_corrupted));
               } else {
                  notify(player, "Cluster list is error free.  Nothing to repair.");
               }
            }
         }
         break;
      case CLUSTER_SET: 
         if ( !i_isequal ) {
            notify(player, "Invalid syntax for @cluster/set.  Use: @cluster/set <dbref[/attrib]>=<string>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "Cluster storage attribute was not creatable.  Unable to repair critical error!");
            } else {
               anum2 = attr->number;
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( s_text && !*s_text ) {
                  notify(player, "Cluster list corrupt for specified object.  @cluster/repair to repair.");
                  free_lbuf(s_text);
                  break;
               }
               s_inbufptr = s_instr = alloc_lbuf("cluster_set");
               s_strtok = s_return;
               while ( *s_strtok && (*s_strtok != '=') ) {
                  safe_chr(*s_strtok, s_instr, &s_inbufptr);
                  s_strtok++;
               }
               if ( *s_strtok )
                  s_strtok++;
               if ( strchr(s_instr, '/') != NULL ) {
                  anum = NOTHING;
                  s_strtokptr = s_instr;
                  while ( *s_strtokptr && (*s_strtokptr != '/') ) {
                     s_strtokptr++;
                  }
                  if ( *s_strtokptr ) 
                     s_strtokptr++;
                  if ( *s_strtokptr ) {
                     attr = atr_str(s_strtokptr);
                     if ( attr )
                        anum = attr->number;
                  }
                  s_tmpstr = find_cluster(thing, player, anum);
                  thing2 = match_thing(player, s_tmpstr);
                  s_strtok = alloc_lbuf("cluster_set_build");
                  sprintf(s_strtok, "%.32s/%.3900s", s_tmpstr, s_strtokptr);
                  if ( Good_chk(thing2) && Cluster(thing2) && Cluster(player) && (Owner(thing2) == Owner(player)) )
                     do_set(thing2, thing2, SET_NOISY, s_strtok, s_tmpptr);
                  else
                     do_set(player, cause, SET_NOISY, s_strtok, s_tmpptr);
                  free_lbuf(s_strtok);
                  free_lbuf(s_instr);
                  free_lbuf(s_tmpstr);
               } else {
                  free_lbuf(s_instr);
                  if ( strchr(s_tmpptr, ':') != NULL ) {
                     s_inbufptr = s_instr = alloc_lbuf("cluster_set");
                     s_strtok = s_tmpptr;
                     while ( *s_strtok && (*s_strtok != ':') ) {
                        safe_chr(*s_strtok, s_instr, &s_inbufptr);
                        s_strtok++;
                     }
                     s_strtok++;
                     attr = atr_str(s_instr);
                     free_lbuf(s_instr);
                     if ( attr ) {
                        s_tmpstr = find_cluster(thing, player, attr->number);
                        thing2 = match_thing(player, s_tmpstr);
                        if ( Good_chk(thing2) && Cluster(thing2) && Cluster(player) && (Owner(thing2) == Owner(player)) )
                           do_set(thing2, thing2, SET_NOISY, s_tmpstr, s_tmpptr);
                        else
                           do_set(player, cause, SET_NOISY, s_tmpstr, s_tmpptr);
                        free_lbuf(s_tmpstr);
                     } else {
                        s_tmpstr = find_cluster(thing, player, NOTHING);
                        thing2 = match_thing(player, s_tmpstr);
                        if ( Good_chk(thing2) && Cluster(thing2) && Cluster(player) && (Owner(thing2) == Owner(player)) )
                           do_set(thing2, thing2, SET_NOISY, s_tmpstr, s_tmpptr);
                        else
                           do_set(player, cause, SET_NOISY, s_tmpstr, s_tmpptr);
                        free_lbuf(s_tmpstr);
                     }
                     trigger_cluster_action(thing, player);
                  } else {
                     if ( *s_tmpptr ) {
                        s_instr = alloc_lbuf("cluster_set_flags1");
                        s_inbufptr = alloc_lbuf("cluster_set_flags2");
                        s_tmpstr = alloc_lbuf("cluster_set_flags3");
                        strcpy(s_tmpstr, s_text);
                        s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                        while ( s_strtok ) {
                           thing3 = match_thing(player, s_strtok);
                           strcpy(s_instr, s_strtok);
                           strcpy(s_inbufptr, s_tmpptr);
                           if ( Good_chk(thing3) && Cluster(thing3) && Cluster(player) && (Owner(thing3) == Owner(player)) )
                              do_set(thing3, thing3, SET_QUIET, s_instr, s_inbufptr);
                           else
                              do_set(player, cause, SET_QUIET, s_instr, s_inbufptr);
                           s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                        }
                        if ( !Quiet(player) ) {
                           if ( TogNoisy(player) ) {
                              tprp_buff = tpr_buff;
                              notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Cluster: Flags (%s) set on cluster members %s.", s_tmpptr, s_tmpstr));
                           } else {
                              notify(player, "Cluster: flags Set.");
                           }
                        }
                        free_lbuf(s_tmpstr);
                        free_lbuf(s_instr);
                        free_lbuf(s_inbufptr);
                     } else {
                        notify(player, "You must specify a flag or flags to set.");
                     }
                  }
               }
               free_lbuf(s_text);
            }
         }
         break;
      case CLUSTER_WIPE: 
         if ( i_isequal ) {
            notify(player, "Invalid syntax for @cluster/wipe.  Use: @cluster/wipe <dbref>");
         } else if ( !Cluster(thing) ) {
            notify(player, "That object isn't a cluster.  Use @cluster/new to make one.");
         } else {
            attr = atr_str("_CLUSTER");
            if ( !attr ) {
               notify(player, "Cluster storage attribute was not creatable.  Unable to repair critical error!");
            } else {
               s_text = atr_get(thing, attr->number, &aowner, &aflags);
               if ( s_text && !*s_text ) {
                  notify(player, "Cluster list corrupt for specified object.  @cluster/repair to repair.");
                  free_lbuf(s_text);
                  break;
               }
               s_strtok = strtok_r(s_text, " ", &s_strtokptr);
               i_lowball = 0;
               s_instr = alloc_lbuf("cluster_wipe");
               endtme = time(NULL);
               starttme = mudstate.chkcpu_stopper;
               if ( mudconf.cputimechk < 10 )
                  timechk = 10;
               else if ( mudconf.cputimechk > 3600 )
                  timechk = 3600;
               else
                  timechk = mudconf.cputimechk;
               i_totobjs = 0;
               if ( i_regexp ) 
                  i_regexp = WIPE_REGEXP;
               if ( i_preserve )
                  i_preserve = WIPE_PRESERVE;
               if ( i_owner )
                  i_owner = WIPE_OWNER;
               while ( s_strtok ) {
                  endtme = time(NULL);
                  if ( mudstate.chkcpu_toggle || ((endtme - starttme) > timechk) ) {
                      mudstate.chkcpu_toggle = 1;
                  }
                  if ( mudstate.chkcpu_toggle ) {
                     notify_quiet(player, "CPU: cluster wipe exceeded cpu limit.");
                     break;
                  }
                  i_lowball++;
                  i_totobjs++;
                  thing3 = match_thing(player, s_strtok);
                  if ( Good_chk(thing3) && Cluster(thing3) ) {
                     if ( ((!i_owner && *s_return) || (i_owner && *s_tmp)) && 
                          ((!i_owner && (strchr(s_return, '/') != NULL)) ||
                            (i_owner && (strchr(s_tmp, '/') != NULL))) ) {
                        if ( i_owner ) {
                           s_foo = strchr(s_tmp, '/');            
                           *s_foo = '\0';
                           if ( strchr(s_return, '/') )
                              sprintf(s_instr, "%s/%s%s", s_tmp, s_strtok, strchr(s_return, '/'));
                           else
                              sprintf(s_instr, "%s/%s", s_tmp, s_strtok);
                           *s_foo = '/';
                        } else
                           sprintf(s_instr, "%s%s", s_strtok, strchr(s_return, '/'));
                        do_wipe(thing3, thing3, (SIDEEFFECT|i_regexp|i_preserve|i_owner), s_instr);
                     } else
                        do_wipe(thing3, thing3, (SIDEEFFECT|i_regexp|i_preserve|i_owner), s_strtok);
                  }
                  switch (mudstate.wipe_state) {
                     case -1: i_nomatch++;
                              break;
                     case -2: i_nowipe++;
                              break;
                     default: i_wipecnt+=mudstate.wipe_state;
                              break;
                  }
                  s_strtok = strtok_r(NULL, " ", &s_strtokptr);
               }
               free_lbuf(s_instr);
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                              "Cluster wipe: %d total objects, %d no-match, %d safe, %d attributes wiped.", 
                              i_totobjs, i_nomatch, i_nowipe, i_wipecnt));
            }
            free_lbuf(s_text);
         }
         break;
      default:
         notify(player, "Unrecognized switch specified to @cluster.");
         break;
   }
   free_lbuf(tpr_buff);
   free_lbuf(s_return);
   if ( s_tmp )
      free_lbuf(s_tmp);
}

int
internal_logstatus( void )
{
   struct stat f_sb;

   if ( stat(mudconf.logdb_name, &f_sb) == -1 )
      return(-1);
   else
      return((int) f_sb.st_size);
}

void
do_progreset(dbref player, dbref cause, int key, char *name)
{
   dbref target;
   DESC *d;
   char *buff = NULL, *tpr_buff, *tprp_buff;
   int i_buff = 0;
#ifdef ZENTY_ANSI
   char *s_buff, *s_buff2, *s_buff3, *s_buffptr, *s_buff2ptr, *s_buff3ptr;
#endif

   if ( !name && !*name ) {
      target = player;
   } else {
      if ( (buff = strchr(name, '=')) != NULL ) {
         i_buff = 1;
         *buff = '\0';
         target = lookup_player(player, name, 0);
         *buff = '=';
         buff++;
         if ( !*buff ) {
            i_buff = 0;
         } else  if ( strlen(strip_all_ansi(buff)) > 80 ) {
            notify(player, "Custom prompt exceeds 80 characters.  Not setting.");
            return;
         }
      } else {
         target = lookup_player(player, name, 0);
      }
   }
   if ( !Good_chk(target) || !Controls(player, target) ) {
      notify(player, "No matching target found.");
      return;
   }
   if ( i_buff && !InProgram(target) ) {
      notify(player, "Can not assign a prompt to someone not in a program.");
      return;
   }
   if ( !InProgram(target) ) {
      DESC_ITER_CONN(d) {
         if ( d->player == target ) {
            queue_string(d, "\377\371");
            atr_clr(d->player, A_PROGPROMPTBUF);
         }
      }
      notify_quiet(player, "Program prompt reset.");
   } else {
      if ( i_buff ) {
         tprp_buff = tpr_buff = alloc_lbuf("do_progreset");
         DESC_ITER_CONN(d) {
            if ( d->player == target ) {
               tprp_buff = tpr_buff;
               atr_add_raw(target, A_PROGPROMPTBUF, buff);
#ifdef ZENTY_ANSI
              s_buffptr = s_buff = alloc_lbuf("parse_ansi_prompt");
              s_buff2ptr = s_buff2 = alloc_lbuf("parse_ansi_prompt2");
              s_buff3ptr = s_buff3 = alloc_lbuf("parse_ansi_prompt3");
              parse_ansi((char *) buff, s_buff, &s_buffptr, s_buff2, &s_buff2ptr, s_buff3, &s_buff3ptr);
              queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s \377\371", ANSI_HILITE, s_buff, ANSI_NORMAL));
              free_lbuf(s_buff);
              free_lbuf(s_buff2);
#else
              queue_string(d, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s \377\371", ANSI_HILITE, buff, ANSI_NORMAL));
#endif
            }
         }
         notify_quiet(player, "Program prompt customized.");
         free_lbuf(tpr_buff);
      } else {
         notify_quiet(player, "Player is in a program, can not reset.");
      }
   }
}

void
do_blacklist(dbref player, dbref cause, int key, char *name) 
{
   char *s_buff, *s_buffptr, *tmpbuff;
   int i_loop_chk, i_page, i_page_val, i_invalid;
   struct in_addr in_tempaddr, in_tempaddr2;
   FILE *f_in;
   BLACKLIST *b_lst_ptr, *b_lst_ptr2;
  
   if ( !key ) {
      s_buffptr = s_buff = alloc_lbuf("do_blacklistLBUF");
      notify(player, safe_tprintf(s_buff, &s_buffptr, "@blacklist: There are currently %d entries in the blacklist.",
                     mudstate.blacklist_cnt));
      free_lbuf(s_buff);
      return;
   }
 
   switch (key) {
      case BLACKLIST_LIST:
         i_page = i_page_val = 0;
         if ( !mudstate.bl_list ) {
            notify(player, "@blacklist: List is currently empty.");
            break;
         }
         if ( name && *name )
            i_page_val = atoi(name);
         if ( (i_page_val < 0) || (i_page_val > ((mudstate.blacklist_cnt / 80) + 1)) ) {
            notify(player, "@blacklist: Value specified must be a valid page value.");
            break;
         }
         s_buffptr = s_buff = alloc_lbuf("do_blacklistLBUF");
         tmpbuff = alloc_lbuf("do_blacklistLBUF2");
         memset(tmpbuff, '\0', LBUF_SIZE);
         i_loop_chk=0;
         b_lst_ptr = mudstate.bl_list;
         notify(player, "==============================================================================");
         if ( i_page_val > 0 ) {
            sprintf(tmpbuff, "= (Paged List)                    Black List                  %3d/%3d        =", i_page_val, (mudstate.blacklist_cnt/80)+1);
         } else {
            sprintf(tmpbuff, "= (Full List)                     Black List                                 =");
         }
         notify(player, tmpbuff);
         notify(player, "==============================================================================");
         while ( b_lst_ptr ) {
            i_loop_chk++;
            if ( (i_loop_chk % 80) == 1 )
               i_page++;
            if ( !((i_page_val == 0) || (i_page_val == i_page)) ) {
               b_lst_ptr = b_lst_ptr->next;
               continue;
            }
            if ( (i_loop_chk % 4) != 0 ) {
               if ( !*s_buff ) {
                  sprintf(s_buff, "   %-18s", (char *)inet_ntoa(b_lst_ptr->site_addr));
               } else {
                  sprintf(tmpbuff, "%s %-18s", s_buff, (char *)inet_ntoa(b_lst_ptr->site_addr));
                  memcpy(s_buff, tmpbuff, LBUF_SIZE - 1);
               }
            } else {
               sprintf(tmpbuff, "%s %-18s", s_buff, (char *)inet_ntoa(b_lst_ptr->site_addr));
               memcpy(s_buff, tmpbuff, LBUF_SIZE - 1);
               notify(player, s_buff);
               memset(s_buff, '\0', LBUF_SIZE);
            }
            b_lst_ptr = b_lst_ptr->next;
         } 
         free_lbuf(tmpbuff);
         if ( (i_loop_chk % 4) != 0 ) {
            notify(player, s_buff);
         }
         notify(player, "==============================================================================");
         free_lbuf(s_buff);
         break;
      case BLACKLIST_LOAD:
         if ( mudstate.blacklist_cnt > 0 ) {
            notify(player, "@blacklist: Data is already loaded.  @blacklist/clear first.");
            break;
         }
         if ( (f_in = fopen("blacklist.txt", "r")) == NULL ) {
            notify(player, "@blacklist: Error opening blacklist.txt file for reading.");
            break;
         }
         s_buff = alloc_sbuf("do_blacklist");
         i_loop_chk = i_invalid = 0;
         mudstate.bl_list = b_lst_ptr2 = NULL;
         inet_aton((char *)"255.255.255.255", &in_tempaddr2);
         while ( !feof(f_in) ) {
            fgets(s_buff, SBUF_SIZE-2, f_in);
            if ( feof(f_in) ) 
               break;
            if ( i_loop_chk > 100000 ) {
               notify(player, "@blacklist: WARNING - blacklist.txt exceeds 100,000 entries. Rest ignored.");
               break;
            }
            i_loop_chk++;
            if ( !inet_aton(s_buff, &in_tempaddr) ) {
               i_invalid++;
               continue;
            }
            b_lst_ptr = (BLACKLIST *) malloc(sizeof(BLACKLIST)+1);
            sprintf(b_lst_ptr->s_site, "%.19s", strip_returntab(s_buff,2));
            b_lst_ptr->site_addr = in_tempaddr;
            b_lst_ptr->mask_addr = in_tempaddr2;
            b_lst_ptr->next = NULL;
            if ( mudstate.bl_list == NULL) {
               mudstate.bl_list = b_lst_ptr;
            } else {
               b_lst_ptr2 = mudstate.bl_list;
               while ( b_lst_ptr2->next != NULL )
                  b_lst_ptr2=b_lst_ptr2->next;
               b_lst_ptr2->next = b_lst_ptr;
            }
            mudstate.blacklist_cnt++;
         }
         free_sbuf(s_buff);
         s_buffptr = s_buff = alloc_lbuf("do_blacklistLBUF");
         notify(player, safe_tprintf(s_buff, &s_buffptr, "Blacklist loaded.  %d entries total.  %d read in, %d ignored.", 
                        i_loop_chk, mudstate.blacklist_cnt, i_invalid));
         free_lbuf(s_buff);
         fclose(f_in);
         break;
      case BLACKLIST_CLEAR:
         i_loop_chk=0;
         b_lst_ptr = mudstate.bl_list;
         while ( b_lst_ptr ) {
            b_lst_ptr2 = b_lst_ptr->next;
            free(b_lst_ptr);
            b_lst_ptr = b_lst_ptr2;
            i_loop_chk++;
         }
         mudstate.bl_list = NULL;
         s_buffptr = s_buff = alloc_lbuf("do_blacklistLBUF");
         notify(player, safe_tprintf(s_buff, &s_buffptr, 
                        "@blacklist: List has been cleared.  %d entries total removed.", mudstate.blacklist_cnt));
         free_lbuf(s_buff);
         mudstate.blacklist_cnt=0;
         break;
   }
}

void
do_tor(dbref player, dbref cause, int key) {
   switch (key) {
      case TOR_CACHE: /* Rebuild the hostname cache */
           notify_quiet(player, "Tor: Rebuilding hostname cache.");
           populate_tor_seed();
           notify_quiet(player, "Tor: Rebuilding complete.");
           break;
      case TOR_LIST: /* List the tor */
           default:  /* Default case is list */
           notify_quiet(player, unsafe_tprintf("Tor: Protection is currently %s.  The current settings are:", 
                                (mudconf.tor_paranoid ? (char *)"ENABLED" : (char *)"DISABLED") ));
           notify_quiet(player, unsafe_tprintf("TOR host DNS: %s", 
                                (*(mudconf.tor_localhost) ? mudconf.tor_localhost : (char *)"(Empty)")));
           notify_quiet(player, unsafe_tprintf("TOR host CACHE: %s", 
                                (*(mudstate.tor_localcache) ? mudstate.tor_localcache : (char *)"(Empty)")));
           break;
   }
}

void
do_logrotate(dbref player, dbref cause, int key) {
   char *s_tprbuff, *s_tprpbuff, *c_stval;
   double d_val, div_val;
   struct stat f_sb;
   struct tm *tp;
   time_t now;

   switch (key ) {
      case LOGROTATE_STATUS:
           c_stval = (char *)" b";
           div_val = 1;
           s_tprpbuff = s_tprbuff = alloc_lbuf("do_logrotate");
           if ( stat(mudconf.logdb_name, &f_sb) != -1 ) {
              d_val = (double) f_sb.st_size;
              if ( d_val > 1024 ) {
                 c_stval = (char *)" Kb";
                 div_val = 1024;
              }
              if ( d_val > 1024000 ) {
                 c_stval = (char *)" Mb";
                 div_val = 1024000;
              }
              if ( d_val > 1024000000 ) {
                 c_stval = (char *)" Gb";
                 div_val = 1024000000;
              }
              notify(player, safe_tprintf(s_tprbuff, &s_tprpbuff, "Existing log file %s is taking up %0.3f%sytes",
                                          mudconf.logdb_name, (d_val/ div_val), c_stval ));
           } else {
              notify(player, safe_tprintf(s_tprbuff, &s_tprpbuff, "Unable to get block/size statistics of logfile %s",
                                          mudconf.logdb_name));
           }
           free_lbuf(s_tprbuff);
           break;
      default:
           time((time_t *) (&now));
           tp = localtime((time_t *) (&now));
           notify(player, "@logrotate: Rotating old log file...");
           sprintf(mudstate.buffer, "./oldlogs/%.200s.%02d%02d%02d%02d%02d%02d",
                   mudconf.logdb_name,
                   (tp->tm_year % 100), tp->tm_mon + 1,
                   tp->tm_mday, tp->tm_hour,
                   tp->tm_min, tp->tm_sec);

           if ( mudstate.f_logfile_name )
              fclose(mudstate.f_logfile_name);

           /* Backup the old filename */
           (void) rename(mudconf.logdb_name, mudstate.buffer);
           s_tprbuff = alloc_mbuf("do_logrotate_mbuf");
           sprintf(s_tprbuff, "%02d%d%d%d%d.%d%d%d%d%d%d ",
                   (tp->tm_year % 100), (((tp->tm_mon) + 1) / 10),
                   (((tp->tm_mon) + 1) % 10), (tp->tm_mday / 10),
                   (tp->tm_mday % 10),
                   (tp->tm_hour / 10), (tp->tm_hour % 10),
                   (tp->tm_min / 10), (tp->tm_min % 10),
                   (tp->tm_sec / 10), (tp->tm_sec % 10));

           fprintf(stderr, "%s%s: Game file %s rotated to %s\n", s_tprbuff, mudconf.mud_name, mudconf.logdb_name, mudstate.buffer);

           notify(player, "            Opening new log file...");
           if ( (mudstate.f_logfile_name = fopen(mudconf.logdb_name, "a")) != NULL ) {
              fprintf(stderr, "%s%s: Game log file opened as %s.\n",
                      s_tprbuff, mudconf.mud_name, mudconf.logdb_name);
           } else {
              mudstate.f_logfile_name = NULL;
              fprintf(stderr, "%s%s: Game log could not be opened.  Will write log files to stderr (this file)\n",
                      s_tprbuff,  mudconf.mud_name);
           }
           free_mbuf(s_tprbuff);
           notify(player, "@logrotate: Completed.");
           break;
   }
}
