#include "hscopyright.h"
#include <string.h>

#include "hstypes.h"
#include "hsinterface.h"

extern "C" {
#include "flags.h"
    extern void do_zone(dbref, dbref, int, char *, char *);
} CHSInterface hsInterface;	// One instance of this.

// Adds an attribute with a value to an object.
void CHSInterface::AtrAdd(int obj, const char *atrname, char *val,
			  int owner, int flags)
{
    int atr;
    char name[SBUF_SIZE];

    strncpy(name, atrname, SBUF_SIZE-1);
    atr = mkattr((char *) name);
    if(!atr)
        return;
        
    atr_add_raw(obj, atr, val);
}

void CHSInterface::AtrDel(int obj, const char *atrname, int owner)
{
    int atr;
    char name[SBUF_SIZE];

    strncpy(name, atrname, SBUF_SIZE-1);
    atr = mkattr((char *) atrname);
    if(!atr)
        return;
        
    atr_clr(obj, atr);
}

// Clones an object and returns the dbref of the new clone.
dbref CHSInterface::CloneThing(dbref model)
{
    dbref clone;

    char name[SBUF_SIZE];
    sprintf(name, "#%d", model);
    do_clone(Owner(model), Owner(model), 0, name, 0);
    clone = hsClonedObject;
    s_Flags(clone, Flags(clone) & ~HALT);

    return clone;
}

// Gets an attribute from an object and stores the value
// in the buffer.  It returns FALSE if the attribute was
// not found.  Otherwise, TRUE.
BOOL CHSInterface::AtrGet(int obj, const char *atrname)
{
    int atr;
    char *value;
    char name[SBUF_SIZE];

    strcpy(name, atrname);
    atr = mkattr(name);
    if(!atr)
        return FALSE;
        
    value = atr_get_raw((dbref) obj, atr);

    if (!value || !*value)
	return FALSE;

    strncpy(m_buffer, value, LBUF_SIZE-1);
    return TRUE;
}

int CHSInterface::MaxObjects(void)
{
    return mudstate.db_top;
}

dbref CHSInterface::NoisyMatchThing(dbref player, char *name)
{
    dbref console;

    init_match(player, name, TYPE_THING);
    match_neighbor();
    match_absolute();
    match_me();
    match_here();
    console = noisy_match_result();

    if (console == AMBIGUOUS)
	console = NOTHING;

    return console;
}

dbref CHSInterface::NoisyMatchRoom(dbref player, char *name)
{
    dbref room;

    init_match(player, name, TYPE_ROOM);
    match_absolute();
    match_here();
    room = noisy_match_result();

    if (room == AMBIGUOUS)
	room = NOTHING;

    return room;
}

// Specifically matches an exit for a given game driver
dbref CHSInterface::NoisyMatchExit(dbref player, char *name)
{
    dbref exit_m;

    init_match(player, name, TYPE_EXIT);
    match_exit();
    match_absolute();
    exit_m = match_result();

    if (exit_m == AMBIGUOUS)
	exit_m = NOTHING;

    return exit_m;
}

void CHSInterface::SetFlag(int objnum, int flag)
{
    s_Flags4(objnum, Flags4(objnum) | flag);
}

void CHSInterface::UnsetFlag(int objnum, int flag)
{
    s_Flags4(objnum, Flags4(objnum) & ~flag);
}

// Checks to see if the specified object exists, isn't garbage,
// etc.
BOOL CHSInterface::ValidObject(dbref objnum)
{
    if (Good_chk(objnum))
	return TRUE;
    else
	return FALSE;
}

// Returns TRUE or FALSE depending on whether the object has
// the specified flag or not.
BOOL CHSInterface::HasFlag(dbref objnum, int type, int flag)
{
    if (!(Typeof(objnum) == type))
	return FALSE;
    if (Flags(objnum) & flag)
	return TRUE;
    if (Flags2(objnum) & flag)
	return TRUE;
    if (Flags3(objnum) & flag)
	return TRUE;
    if (Flags4(objnum) & flag)
	return TRUE;

    return FALSE;
}

// Can be used to set a type of lock on an object
void CHSInterface::SetLock(int objnum, int lockto, HS_LOCKTYPE lock)
{
    char tmp[SBUF_SIZE];
    BOOLEXP *key;

    sprintf(tmp, "#%d", lockto);
    key = parse_boolexp(objnum, tmp, 1);

    switch (lock) {
    case LOCK_USE:
	atr_add_raw(objnum, A_LUSE, unparse_boolexp_quiet(objnum, key));
	break;
    case LOCK_ZONE:
	atr_add_raw(objnum, A_LCONTROL,
		    unparse_boolexp_quiet(objnum, key));
    default:
        break;
    }
    free_boolexp(key);
}

dbref CHSInterface::GetLock(int objnum, HS_LOCKTYPE lock)
{
    char *value;
    BOOLEXP *key;
    int aowner, aflags;
    dbref lockobj;

    value = atr_get((dbref) objnum, A_LUSE, &aowner, &aflags);
    key = parse_boolexp((dbref) objnum, value, 1);
    free_lbuf(value);
    if (key == TRUE_BOOLEXP) {
	free_boolexp(key);
	return NOTHING;
    } else {
	lockobj = key->thing;
	free_boolexp(key);
	return lockobj;
    }
}

dbref CHSInterface::ConsoleUser(int objnum)
{
    dbref dbUser;

    dbUser = GetLock(objnum, LOCK_USE);

    if (!dbUser || !objnum)
	return NOTHING;

    if (isPlayer(dbUser)) {
	// If the user is not in the same location as the object,
	// set the lock to the object and return NOTHING.
	if (Location(dbUser) != Location(objnum)
	    || (!HasFlag(dbUser, TYPE_PLAYER, CONNECTED) && isPlayer(dbUser))) {
	    SetLock(objnum, objnum, LOCK_USE);


	    // Delete attribute from player.
	    hsInterface.AtrDel(dbUser, "MCONSOLE", GOD);
	    notify_except(Location(objnum), dbUser, dbUser,
			  unsafe_tprintf("%s unmans the %s.", Name(dbUser),
					 Name(objnum)), 0);
	    return NOTHING;
	}
    }

    return dbUser;
}

// Sends a message to the contents of an object, which is usually
// a room.
void CHSInterface::NotifyContents(int objnum, char *strMsg)
{
    notify_all((dbref) objnum, objnum, strMsg);
}

// Handles getting the contents of an object, which is often
// specific to each game driver.
void CHSInterface::GetContents(int loc, int *iArray, int type)
{
    int thing;
    int idx;

    idx = 0;
    for (thing = Contents(loc); Good_obj(thing); thing = Next(thing)) {
	if (type <= 0x05) {
	    if (Typeof(thing) == type)
		iArray[idx++] = thing;
	} else
	    iArray[idx++] = thing;
    }
    iArray[idx] = -1;
}

// Call this function to see if a given object can
// pass a type of lock on a target object.
BOOL CHSInterface::PassesLock(int obj, int target, HS_LOCKTYPE locktype)
{
    if (locktype == LOCK_ZONE)
	return could_doit(obj, target, A_LCONTROL, 0, 0);
    else
	return could_doit(obj, target, A_LUSE, 0, 0);
}

char *CHSInterface::EvalExpression(char *input, dbref executor,
				   dbref caller, dbref enactor)
{
    static char tbuf[LBUF_SIZE];

    *tbuf = '\0';

    char *bp, *str;
    bp = tbuf;
    str = input;

    bp = exec(executor, caller, enactor, EV_FCHECK | EV_EVAL | EV_TOP, str,
	      (char **) NULL, 0, (char **) NULL, 0);
    *bp = '\0';

    return tbuf;
}

void CHSInterface::CHZone(dbref room, dbref zone)
{
    char zonename[SBUF_SIZE - 1];
    char roomname[SBUF_SIZE - 1];

    snprintf(zonename, SBUF_SIZE, "#%d", zone);
    snprintf(roomname, SBUF_SIZE, "#%d", room);

    do_zone(GOD, GOD, ZONE_ADD, roomname, zonename);

    return;
}

BOOL CHSInterface::LinkExits(dbref sexit, dbref dexit)
{
    if (!sexit || !dexit)
	return FALSE;

    if (!isExit(sexit) || !isExit(dexit))
	return FALSE;

    s_Location(dexit, Home(sexit));
    s_Location(sexit, Home(dexit));

    return TRUE;
}

BOOL CHSInterface::UnlinkExits(dbref sexit, dbref dexit)
{
    if (!sexit || !dexit)
	return FALSE;

    if (!isExit(sexit) || !isExit(dexit))
	return FALSE;

    s_Location(dexit, NOTHING);
    s_Location(sexit, NOTHING);

    return TRUE;
}

dbref CHSInterface::GetHome(dbref dbObject)
{
    if (dbObject)
	return Home(dbObject);

    return NOTHING;
}

char *CHSInterface::GetName(dbref dbObject)
{
    if (dbObject)
	return Name(dbObject);

    return NULL;
}
