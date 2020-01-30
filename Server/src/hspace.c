#include "autoconf.h"
#include "config.h"
#include "externs.h"
#include "local.h"
#include "command.h"
#include "functions.h"
#include "hspace/hsswitches.h"

FUNCTION(hs_addsys);
FUNCTION(hs_clone);
FUNCTION(hs_comm_msg);
FUNCTION(hs_delsys);
FUNCTION(hs_set_missile);
FUNCTION(hs_get_missile);
FUNCTION(hs_sysset);
FUNCTION(hs_sys_attr);
FUNCTION(hs_weapon_attr);
FUNCTION(hs_add_weapon);
FUNCTION(hs_get_attr);
FUNCTION(hs_space_msg);
FUNCTION(hs_eng_systems);
FUNCTION(hs_set_attr);
FUNCTION(hs_srep);
FUNCTION(hs_xyang);
FUNCTION(hs_zang);

dbref hsClonedObject;

FILE *spacelog_fp;
void local_hs_init(void);
void hsCommand(char *, int, char *, char *, int);
extern int valid_obj(dbref);
extern void hscManConsole(int, char *);
extern void hscUnmanConsole(int, char *);
extern void hscDisembark(int, char *);
extern void hscBoardShip(int, char *, char *);
extern void hsInit(int);

NAMETAB space_sw[] = {
    {"ACTIVATE", 		2, CA_PUBLIC, 	0, HSC_ACTIVATE},
    {"ADDHATCH",		4, CA_WIZARD,	0, HSC_ADDHATCH},
    {"ADDCONSOLE",		4, CA_WIZARD,	0, HSC_ADDCONSOLE},
    {"ADDLANDINGLOC",		4, CA_WIZARD,	0, HSC_ADDLANDINGLOC},
    {"ADDMESSAGE",		4, CA_WIZARD,	0, HSC_ADDMESSAGE},
    {"ADDOBJECT",		4, CA_WIZARD,	0, HSC_ADDOBJECT},
    {"ADDSROOM",		5, CA_WIZARD,	0, HSC_ADDSROOM},
    {"ADDSYS",			5, CA_WIZARD,	0, HSC_ADDSYS},
    {"ADDSYSCLASS",		7, CA_WIZARD,	0, HSC_ADDSYSCLASS},
    {"ADDTERRITORY",		4, CA_WIZARD,   0, HSC_ADDTERRITORY},
    {"ADDWEAPON",		4, CA_WIZARD,	0, HSC_ADDWEAPON},
    {"CLONE",			2, CA_WIZARD,   0, HSC_CLONE},
    {"DELCLASS",		5, CA_WIZARD,	0, HSC_DELCLASS},
    {"DELCONSOLE",		5, CA_WIZARD,	0, HSC_DELCONSOLE},
    {"DELMESSAGE",		4, CA_WIZARD,   0, HSC_DELMESSAGE},
    {"DELSROOM",		4, CA_WIZARD,   0, HSC_DELSROOM},
    {"DELTERRITORY",		4, CA_WIZARD,   0, HSC_DELTERRITORY},
    {"DELUNIVERSE",		4, CA_WIZARD,	0, HSC_DELUNIVERSE},
    {"DELWEAPON",		4, CA_WIZARD,   0, HSC_DELWEAPON},
    {"DELSYS",			4, CA_WIZARD,   0, HSC_DELSYS},
    {"DELSYSCLASS",		7, CA_WIZARD,   0, HSC_DELSYSCLASS},
    {"DUMPCLASS",		5, CA_WIZARD,   0, HSC_DUMPCLASS},
    {"DUMPWEAPON",		5, CA_WIZARD,   0, HSC_DUMPWEAPON},
    {"HALT",                	2, CA_WIZARD,   0, HSC_HALT},
    {"LIST",			2, CA_WIZARD,   0, HSC_LIST},
    {"MOTHBALL",		2, CA_WIZARD,   0, HSC_MOTHBALL},
    {"NEWCLASS",		4, CA_WIZARD,   0, HSC_NEWCLASS},
    {"NEWUNIVERSE",		4, CA_WIZARD,   0, HSC_NEWUNIVERSE},
    {"NEWWEAPON",		4, CA_WIZARD,   0, HSC_NEWWEAPON},
    {"REPAIR",			2, CA_WIZARD,   0, HSC_REPAIR},
    {"SET",			3, CA_WIZARD,   0, HSC_SET},
    {"SETCLASS",		4, CA_WIZARD,   0, HSC_SETCLASS},
    {"SETMISSILE",		4, CA_WIZARD,   0, HSC_SETMISSILE},
    {"SETWEAPON",           	4, CA_WIZARD,   0, HSC_SETWEAPON},
    {"SYSINFO",			4, CA_WIZARD,   0, HSC_SYSINFO},
    {"SYSINFOCLASS",		8, CA_WIZARD,	0 ,HSC_SYSINFOCLASS},
    {"SYSSET",			4, CA_WIZARD,   0, HSC_SYSSET},
    {"SYSSETCLASS",		7, CA_WIZARD,   0, HSC_SYSSETCLASS},
    {"START",               	3, CA_WIZARD,   0, HSC_START},
    {NULL, 0, 0, 0}
};

NAMETAB console_sw[] = {
    {"AUTOLOAD",		5, CA_PUBLIC, 	0, HSCONS_AUTOLOAD},
    {"AUTOROTATE",		5, CA_PUBLIC,   0, HSCONS_AUTOROTATE},
    {"CONFIGURE",		3, CA_PUBLIC,	0, HSCONS_CONFIGURE},
    {"FIRE",			1, CA_PUBLIC,	0, HSCONS_FIRE},
    {"GREP",			2, CA_PUBLIC,   0, HSCONS_GREP},
    {"HEAD",        		2, CA_PUBLIC,   0, HSCONS_HEAD},
    {"LOAD",			2, CA_PUBLIC,   0, HSCONS_LOAD},
    {"LOCK",			2, CA_PUBLIC,   0, HSCONS_LOCK},
    {"POWER",               	3, CA_PUBLIC,   0, HSCONS_POWER},
    {"QREP",			2, CA_PUBLIC,   0, HSCONS_QREP},
    {"TARGET",              	1, CA_PUBLIC,   0, HSCONS_TARGET},
    {"UNLOAD",              	5, CA_PUBLIC,   0, HSCONS_UNLOAD},
    {"UNLOCK",              	5, CA_PUBLIC,   0, HSCONS_UNLOCK},
    {0, 0, 0, 0}
};

NAMETAB eng_sw[] = {
    {"SELFDESTRUCT",	    5, CA_PUBLIC,   0, HSENG_SELFDESTRUCT},
    {"SETSYSPOWER",	    4, CA_PUBLIC,   0, HSENG_SETSYSPOWER},
    {"SHIPSTATS",           4, CA_PUBLIC,   0, HSENG_SHIPSTATS},
    {"SYSTEMREPORT",        7, CA_PUBLIC,   0, HSENG_SYSTEMREPORT},
    {"SYSTEMPRIORITY",      7, CA_PUBLIC,   0, HSENG_SYSTEMPRIORITY},
    {"CREWREP",             3, CA_PUBLIC,   0, HSENG_CREWREP},
    {"ASSIGNCREW",          3, CA_PUBLIC,   0, HSENG_ASSIGNCREW},
    {0, 0, 0, 0}
};

NAMETAB nav_sw[] = {
    {"AFTERBURN", 	    3, CA_PUBLIC,   0, NSNAV_AFTERBURN},
    {"BOARDLINK",           6, CA_PUBLIC,   0, HSNAV_BOARDLINK},
    {"BOARDUNLINK",         7, CA_PUBLIC,   0, HSNAV_BOARDUNLINK},
    {"CLOAK",               2, CA_PUBLIC,   0, HSNAV_CLOAK},
    {"GATE",                2, CA_PUBLIC,   0, HSNAV_GATE},
    {"HSTAT",               2, CA_PUBLIC,   0, HSNAV_HSTAT},
    {"JUMP",                2, CA_PUBLIC,   0, HSNAV_JUMP},
    {"LAND",                2, CA_PUBLIC,   0, HSNAV_LAND},
    {"MAPRANGE",            2, CA_PUBLIC,   0, HSNAV_MAPRANGE},
    {"SCAN",                2, CA_PUBLIC,   0, HSNAV_SCAN},
    {"SENSORREPORT",        3, CA_PUBLIC,   0, HSNAV_SENSORREPORT},
    {"SETHEADING",          4, CA_PUBLIC,   0, HSNAV_SETHEADING},
    {"SETROLL",             4, CA_PUBLIC,   0, HSNAV_SETROLL},
    {"SETVELOCITY",         4, CA_PUBLIC,   0, HSNAV_SETVELOCITY},
    {"STATUS",              2, CA_PUBLIC,   0, HSNAV_STATUS},
    {"SVIEW",               2, CA_PUBLIC,   0, HSNAV_SVIEW},
    {"TAXI",                2, CA_PUBLIC,   0, HSNAV_TAXI},
    {"TRACTORMODE",         8, CA_PUBLIC,   0, HSNAV_TRACTORMODE},
    {"TRACTORLOCK",         8, CA_PUBLIC,   0, HSNAV_TRACTORLOCK},
    {"TRACTORDOCK",         8, CA_PUBLIC,   0, HSNAV_TRACTORDOCK},
    {"UNDOCK",              2, CA_PUBLIC,   0, HSNAV_UNDOCK},
    {"VIEW",                2, CA_PUBLIC,   0, HSNAV_VIEW},
    {0, 0, 0, 0}
};

extern void local_hs_init(void);
extern void hsDumpDatabases(void);
extern void hsCycle(void);
extern char *hsFunction(char *, dbref, char **);

void do_hs_space(dbref player, dbref cause, int key, char *arg_left, char *arg_right)
{
    if(key == 0)
        key = HSC_NONE;
    
    hsCommand("@space", key, arg_left, arg_right, player);
}

void do_hs_nav(dbref player, dbref cause, int key, char *arg_left, char *arg_right)
{
    hsCommand("@nav", key, arg_left, arg_right, player);
}

void do_hs_eng(dbref player, dbref cause, int key, char *arg_left, char *arg_right)
{
    hsCommand("@eng", key, arg_left, arg_right, player);
}

void do_hs_console(dbref player, dbref cause, int key, char *arg_left, char *arg_right)
{
    hsCommand("@console", key, arg_left, arg_right, player);
}

void do_hs_man(dbref player, dbref cause, int key, char *arg_left)
{
    hscManConsole(player, arg_left);
}

void do_hs_unman(dbref player, dbref cause, int key, char *arg_left)
{
    hscUnmanConsole(player, arg_left);
}

void do_hs_disembark(dbref player, dbref cause, int key, char *arg_left)
{
    hscDisembark(player, arg_left);
}

void do_hs_board(dbref player, dbref cause, int key, char *arg_left, char *arg_right)
{
    hscBoardShip(player, arg_left, arg_right);
}

void local_hs_init(void) {

    static CMDENT hs_cmd_table[] = {
        {(char *) "board", NULL, CA_PUBLIC, 0, 0, CS_ONE_ARG, 0, do_hs_board},
        {(char *) "disembark", NULL, CA_PUBLIC, 0, 0, CS_ONE_ARG, 0, do_hs_disembark},
        {(char *) "unman", NULL, CA_PUBLIC, 0, 0, CS_ONE_ARG, 0, do_hs_unman},
        {(char *) "man", NULL, CA_PUBLIC, 0, 0, CS_ONE_ARG, 0, do_hs_man},
        {(char *) "@console", console_sw, CA_PUBLIC, 0, 0, CS_TWO_ARG, 0, do_hs_console},
        {(char *) "@eng", eng_sw, CA_PUBLIC, 0, 0, CS_TWO_ARG, 0, do_hs_eng},
        {(char *) "@nav", nav_sw, CA_PUBLIC, 0, 0, CS_TWO_ARG, 0, do_hs_nav},
        {(char *) "@space", space_sw, CA_NO_SLAVE | CA_NO_GUEST | CA_NO_WANDER, 0, 0, CS_TWO_ARG, 0, do_hs_space},
        {(char *) NULL, NULL, 0, 0, 0, 0, 0, NULL}

    };

    static FUN hs_fun_table[] = {
	    {"HS_ADDSYS",   	hs_addsys,      2,  0,          CA_PUBLIC},
	    {"HS_CLONE",    	hs_clone,       1,  0,          CA_PUBLIC},
	    {"HS_COMM_MSG", 	hs_comm_msg,    8,  0,          CA_PUBLIC},
	    {"HS_DELSYS",   	hs_delsys,      2,  0,          CA_PUBLIC},
	    {"HS_SET_MISSILE",  hs_set_missile, 3,  0,          CA_PUBLIC},
	    {"HS_GET_MISSILE",  hs_get_missile, 2,  0,          CA_PUBLIC},
	    {"HS_SYSSET",   	hs_sysset,      2,  0,          CA_PUBLIC},
	    {"HS_SYS_ATTR", 	hs_sys_attr,    3,  0,          CA_PUBLIC},
	    {"HS_WEAPON_ATTR",  hs_weapon_attr, 2,  0,          CA_PUBLIC},
	    {"HS_ADD_WEAPON",   hs_add_weapon,  2,  0,          CA_PUBLIC},
	    {"HS_GET_ATTR", 	hs_get_attr,    2,  0,          CA_PUBLIC},
	    {"HS_SPACEMSG", 	hs_space_msg,   7,  0,          CA_PUBLIC},
	    {"HS_ENG_SYS",  	hs_eng_systems, 1,  0,          CA_PUBLIC},
	    {"HS_SET_ATTR", 	hs_set_attr,    2,  0,          CA_PUBLIC},
	    {"HS_SREP",     	hs_srep,        2,  0,          CA_PUBLIC},
	    {"XYANG",      	hs_xyang,       4,  0,          CA_PUBLIC},
	    {"ZANG",        	hs_zang,        6,  0,          CA_PUBLIC},
            {NULL, NULL, 0, 0, 0, 0}
    };

    CMDENT *cmdp;
    FUN *fp;
    char *buff, *cp, *dp;

    /* Add the commands to the command table */
    for (cmdp = hs_cmd_table; cmdp->cmdname; cmdp++) {
        cmdp->cmdtype = CMD_BUILTIN_e;
        hashadd(cmdp->cmdname, (int *) cmdp, &mudstate.command_htab);
    }

    /* Register the functions */
    buff = alloc_sbuf("init_hs_functab");
    for (fp = hs_fun_table ; fp->name ; fp++) {
       cp = (char *) fp->name;
        dp = buff;
        while (*cp) {
            *dp = ToLower(*cp);
            cp++;
            dp++;
        }
        *dp = '\0';
        hashadd2(buff, (int *) fp, &mudstate.func_htab, 1);
    }
    free_sbuf(buff);

    hsInit(mudstate.reboot_flag);
}

FUNCTION(hs_clone)
{
    safe_str(hsFunction("CLONE", player, fargs), buff, bufcx);
}

FUNCTION(hs_set_missile)
{
    safe_str(hsFunction("SETMISSILE", player, fargs), buff, bufcx);
}

FUNCTION(hs_sysset)
{
    safe_str(hsFunction("SYSSET", player, fargs), buff, bufcx);
}

FUNCTION(hs_addsys)
{
    safe_str(hsFunction("ADDSYS", player, fargs), buff, bufcx);
}

FUNCTION(hs_delsys)
{
    safe_str(hsFunction("DELSYS", player, fargs), buff, bufcx);
}

FUNCTION(hs_sys_attr)
{
    safe_str(hsFunction("SYSATTR", player, fargs), buff, bufcx);
}

FUNCTION(hs_get_missile)
{
    safe_str(hsFunction("GETMISSILE", player, fargs), buff, bufcx);
}

FUNCTION(hs_weapon_attr)
{
    safe_str(hsFunction("WEAPONATTR", player, fargs), buff, bufcx);
}

FUNCTION(hs_add_weapon)
{
    safe_str(hsFunction("ADDWEAPON", player, fargs), buff, bufcx);
}
FUNCTION(hs_get_attr)
{
    safe_str(hsFunction("GETATTR", player, fargs), buff, bufcx);
}
FUNCTION(hs_xyang)
{
    safe_str(hsFunction("XYANG", player, fargs), buff, bufcx);
}

FUNCTION(hs_zang)
{
    safe_str(hsFunction("ZANG", player, fargs), buff, bufcx);
}

FUNCTION(hs_space_msg)
{
    safe_str(hsFunction("SPACEMSG", player, fargs), buff, bufcx);
}

FUNCTION(hs_eng_systems)
{
    safe_str(hsFunction("GETENGSYSTEMS", player, fargs), buff, bufcx);
}

FUNCTION(hs_set_attr)
{
    safe_str(hsFunction("SETATTR", player, fargs), buff, bufcx);
}

FUNCTION(hs_srep)
{
    safe_str(hsFunction("SENSORCONTACTS", player, fargs), buff, bufcx);
}

FUNCTION(hs_comm_msg)
{
    safe_str(hsFunction("COMMMSG", player, fargs), buff, bufcx );
}

int valid_obj(dbref objnum) {
    return Good_chk(objnum);
}
