#include "hscopyright.h"

#include <stdlib.h>
#include <string.h>

#include "hscmds.h"
#include "hsuniverse.h"
#include "hspace.h"
#include "hsdb.h"
#include "hsinterface.h"
#include "hsutils.h"
#include "hsconf.h"
#include "hsterritory.h"
#include "externs.h"
#include "hsswitches.h"

// To add a new HSpace command, add it's prototype here, then
// add it to the hsSpaceCommandArray below.
HSPACE_COMMAND_PROTO(hscNewWeapon)
    HSPACE_COMMAND_PROTO(hscCloneObject)
    HSPACE_COMMAND_PROTO(hscCheckSpace)
    HSPACE_COMMAND_PROTO(hscStartSpace)
    HSPACE_COMMAND_PROTO(hscHaltSpace)
    HSPACE_COMMAND_PROTO(hscActivateShip)
    HSPACE_COMMAND_PROTO(hscMothballObject)
    HSPACE_COMMAND_PROTO(hscNewUniverse)
    HSPACE_COMMAND_PROTO(hscDelUniverse)
    HSPACE_COMMAND_PROTO(hscSetAttribute)
    HSPACE_COMMAND_PROTO(hscAddLandingLoc)
    HSPACE_COMMAND_PROTO(hscAddSroom)
    HSPACE_COMMAND_PROTO(hscDelSroom)
    HSPACE_COMMAND_PROTO(hscAddHatch)
    HSPACE_COMMAND_PROTO(hscAddConsole)
    HSPACE_COMMAND_PROTO(hscDelConsole)
    HSPACE_COMMAND_PROTO(hscDelClass)
    HSPACE_COMMAND_PROTO(hscAddObject)
    HSPACE_COMMAND_PROTO(hscRepairShip)
    HSPACE_COMMAND_PROTO(hscSetSystemAttr)
    HSPACE_COMMAND_PROTO(hscAddWeapon)
    HSPACE_COMMAND_PROTO(hscSetMissile)
    HSPACE_COMMAND_PROTO(hscNewClass)
    HSPACE_COMMAND_PROTO(hscDumpClassInfo)
    HSPACE_COMMAND_PROTO(hscAddSysClass)
    HSPACE_COMMAND_PROTO(hscSysInfoClass)
    HSPACE_COMMAND_PROTO(hscSysInfo)
    HSPACE_COMMAND_PROTO(hscSetSystemAttrClass)
    HSPACE_COMMAND_PROTO(hscSetAttrClass)
    HSPACE_COMMAND_PROTO(hscListDatabase)
    HSPACE_COMMAND_PROTO(hscDelWeapon)
    HSPACE_COMMAND_PROTO(hscAddTerritory)
    HSPACE_COMMAND_PROTO(hscDelTerritory)
    HSPACE_COMMAND_PROTO(hscSetAttrWeapon)
    HSPACE_COMMAND_PROTO(hscAddSys)
    HSPACE_COMMAND_PROTO(hscDelSys)
    HSPACE_COMMAND_PROTO(hscDelSysClass)
    HSPACE_COMMAND_PROTO(hscDumpWeapon)
    HSPACE_COMMAND_PROTO(hscAddMessage)
    HSPACE_COMMAND_PROTO(hscDelMessage)
// For external calling from C code
extern "C" {
    HSPACE_COMMAND_PROTO(hscManConsole)	// Not an @space command
    HSPACE_COMMAND_PROTO(hscUnmanConsole)	// Not either.
    HSPACE_COMMAND_PROTO(hscDisembark)	// Nope, not one.
    HSPACE_COMMAND_PROTO(hscBoardShip)	// Nope.
}
// The hsCommandArray holds all externally callable @space commands.
    HSPACE_COMMAND hsSpaceCommandArray[] =
{
    HSC_ACTIVATE, hscActivateShip, HCP_WIZARD,
    HSC_ADDHATCH, hscAddHatch, HCP_WIZARD,
    HSC_ADDCONSOLE, hscAddConsole, HCP_WIZARD,
    HSC_ADDLANDINGLOC, hscAddLandingLoc, HCP_WIZARD,
    HSC_ADDMESSAGE, hscAddMessage, HCP_WIZARD,
    HSC_ADDOBJECT, hscAddObject, HCP_WIZARD,
    HSC_ADDSROOM, hscAddSroom, HCP_WIZARD,
    HSC_ADDSYS, hscAddSys, HCP_WIZARD,
    HSC_ADDSYSCLASS, hscAddSysClass, HCP_WIZARD,
    HSC_ADDTERRITORY, hscAddTerritory, HCP_WIZARD,
    HSC_ADDWEAPON, hscAddWeapon, HCP_WIZARD,
    HSC_CLONE, hscCloneObject, HCP_WIZARD,
    HSC_DELCLASS, hscDelClass, HCP_WIZARD,
    HSC_DELCONSOLE, hscDelConsole, HCP_WIZARD,
    HSC_DELMESSAGE, hscDelMessage, HCP_WIZARD,
    HSC_DELSROOM, hscDelSroom, HCP_WIZARD,
    HSC_DELTERRITORY, hscDelTerritory, HCP_WIZARD,
    HSC_DELUNIVERSE, hscDelUniverse, HCP_WIZARD,
    HSC_DELWEAPON, hscDelWeapon, HCP_WIZARD,
    HSC_DELSYS, hscDelSys, HCP_WIZARD,
    HSC_DELSYSCLASS, hscDelSysClass, HCP_WIZARD,
    HSC_DUMPCLASS, hscDumpClassInfo, HCP_WIZARD,
    HSC_DUMPWEAPON, hscDumpWeapon, HCP_WIZARD,
    HSC_HALT, hscHaltSpace, HCP_WIZARD,
    HSC_LIST, hscListDatabase, HCP_WIZARD,
    HSC_MOTHBALL, hscMothballObject, HCP_WIZARD,
    HSC_NEWCLASS, hscNewClass, HCP_WIZARD,
    HSC_NEWUNIVERSE, hscNewUniverse, HCP_WIZARD,
    HSC_NEWWEAPON, hscNewWeapon, HCP_WIZARD,
    HSC_REPAIR, hscRepairShip, HCP_WIZARD,
    HSC_SET, hscSetAttribute, HCP_WIZARD,
    HSC_SETCLASS, hscSetAttrClass, HCP_WIZARD,
    HSC_SETMISSILE, hscSetMissile, HCP_WIZARD,
    HSC_SETWEAPON, hscSetAttrWeapon, HCP_WIZARD,
    HSC_SYSINFO, hscSysInfo, HCP_WIZARD,
    HSC_SYSINFOCLASS, hscSysInfoClass, HCP_WIZARD,
    HSC_SYSSET, hscSetSystemAttr, HCP_WIZARD,
    HSC_SYSSETCLASS, hscSetSystemAttrClass, HCP_WIZARD,
    HSC_START, hscStartSpace, HCP_WIZARD,
    HSC_NONE, hscCheckSpace, HCP_ANY,
    0, NULL, 0
};

// Finds a command with a given switch in the supplied command array.
HSPACE_COMMAND *hsFindCommand(int switches, HSPACE_COMMAND * cmdlist)
{
    HSPACE_COMMAND *ptr;

    ptr = cmdlist;
    while(ptr->key != 0) {
	if (ptr->key == switches)
	    return ptr;
        ptr++;
    }

    return NULL;
}

// Called from the game to give a status of HSpace
HSPACE_COMMAND_HDR(hscCheckSpace)
{
    UINT uActiveShips;
    UINT uShips;
    UINT uCelestials;
    CHSUniverse *cUniv;
    int i;

    // If not a wizard, then just print out the HSpace version
    // and activity status.
    notify(player, HSPACE_VERSION);
    notify(player, unsafe_tprintf("   The system is currently %s.",
				  HSpace.Active()? "ACTIVE" : "INACTIVE"));

    if (!Wizard(player)) {
	return;
    }
    // Cycle time
    notify(player, unsafe_tprintf("   Time per cycle: %.1fms",
				  HSpace.CycleTime()));

    // Tally up active ships in universes.
    uActiveShips = 0;
    uShips = 0;
    uCelestials = 0;
    for (i = 0; i < HS_MAX_UNIVERSES; i++) {
	cUniv = uaUniverses.GetUniverse(i);
	if (cUniv) {
	    uActiveShips += cUniv->NumActiveObjects(HST_SHIP);
	    uShips += cUniv->NumObjects(HST_SHIP);
	    uCelestials += cUniv->NumObjects(HST_PLANET);
	}
    }

    // DB information
    notify(player, "HSpace Database Report:");
    notify(player,
	   unsafe_tprintf("   Weapons    : %4d", dbHSDB.NumWeapons()));
    notify(player,
	   unsafe_tprintf("   Classes    : %4d", caClasses.NumClasses()));
    notify(player,
	   unsafe_tprintf("   Universes  : %4d", dbHSDB.NumUniverses()));
    notify(player,
	   unsafe_tprintf("   Ships      : %4d (%d)", uShips,
			  uActiveShips));
    notify(player, unsafe_tprintf("   Celestials : %4d", uCelestials));
    notify(player,
	   unsafe_tprintf("   Territories: %4d",
			  taTerritories.NumTerritories()));
}

// Start HSpace
HSPACE_COMMAND_HDR(hscStartSpace)
{
    if (HSpace.Active())
	notify(player, "HSpace: System already active.");
    else {
	HSpace.SetCycle(TRUE);
	notify(player, "HSpace: System started.");
    }
}

// Stop HSpace
HSPACE_COMMAND_HDR(hscHaltSpace)
{
    if (!HSpace.Active())
	notify(player, "HSpace: System already inactive.");
    else {
	HSpace.SetCycle(FALSE);
	notify(player, "HSpace: System halted.");
    }
}

// Activates a new ship
HSPACE_COMMAND_HDR(hscActivateShip)
{
    CHSShip *newShip;
    int iClass;
    dbref dbConsole;
    CHSUniverse *unDest;
    int idx;

    // Find the console
    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;
    if (Typeof(dbConsole) != TYPE_THING) {
	notify(player, unsafe_tprintf("%s(#%d) is not of type THING.",
				      Name(dbConsole), dbConsole));
	return;
    }
    // See if it's already a ship.
    if (dbHSDB.FindShip(dbConsole)) {
	notify(player, "That object is already an HSpace ship.");
	return;
    }
    // Check the class
    if (!arg_right || !*arg_right)
	iClass = -1;
    else
	iClass = atoi(arg_right);
    if (!caClasses.GoodClass(iClass)) {
	notify(player, "Invalid ship class specified.");
	return;
    }
    // Allocate the new ship
    newShip = new CHSShip;
    newShip->SetDbref(dbConsole);

    // Set the class info
    if (!newShip->SetClassInfo(iClass)) {
	notify(player, "Error setting class info.  Aborting.");
	delete newShip;
	return;
    }
    // Add the ship to the first available universe
    unDest = NULL;
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (unDest = uaUniverses.GetUniverse(idx))
	    break;
    }
    if (!unDest) {
	notify(player, "No valid universe for this ship exists.");
	delete newShip;
	return;
    }
    if (!unDest->AddObject(newShip)) {
	notify(player, "Ship could not be added to default universe.");
	delete newShip;
	return;
    }
    // Set the universe id for the ship
    newShip->SetUID(unDest->GetID());

    // Set the flag
    hsInterface.SetFlag(dbConsole, THING_HSPACE_OBJECT);

    // Fully repair the ship
    newShip->TotalRepair();
    notify(player, unsafe_tprintf("%s (#%d) - activated with class %d.",
				  Name(dbConsole), dbConsole, iClass));
}

// Deactivates any HSpace object.
HSPACE_COMMAND_HDR(hscMothballObject)
{
    CHSObject *cObj;
    dbref dbObj;

    // Find the object
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;
    if (Typeof(dbObj) != TYPE_THING) {
	notify(player, unsafe_tprintf("%s(#%d) is not of type THING.",
				      Name(dbObj), dbObj));
	return;
    }
    // See if it's a space object.
    if (!(cObj = dbHSDB.FindObject(dbObj))) {
	notify(player,
	       "That is either not a space object, or it has no HSDB_TYPE attribute.");
	return;
    }
    // Find the universe it's in.
    CHSUniverse *uSource;
    uSource = uaUniverses.FindUniverse(cObj->GetUID());

    // Remove it from the universe.
    if (uSource)
	uSource->RemoveObject(cObj);

    // Delete the object. This is the end of it all.
    delete cObj;

    notify(player,
	   unsafe_tprintf("HSpace object(#%d) mothballed.", dbObj));
}

// Allows the creation of a new universe.
HSPACE_COMMAND_HDR(hscNewUniverse)
{
    dbHSDB.CreateNewUniverse(player, arg_left);
}

// Allows the destruction of an existing universe.
HSPACE_COMMAND_HDR(hscDelUniverse)
{
    dbHSDB.DeleteUniverse(player, arg_left);
}

// Allows the addition of a new landing location to a planet or
// ship.
HSPACE_COMMAND_HDR(hscAddLandingLoc)
{
    dbref dbConsole;
    dbref dbRoom;
    CHSObject *cObj;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    dbRoom = hsInterface.NoisyMatchRoom(player, arg_right);
    if (dbRoom == NOTHING)
	return;

    if (Typeof(dbRoom) != TYPE_ROOM) {
	notify(player, "You may only add rooms as landing locations.");
	return;
    }
    // Find the planet or ship
    cObj = dbHSDB.FindObject(dbConsole);
    if (!cObj) {
	notify(player,
	       unsafe_tprintf("That thing(#%d) is not an HSpace object.",
			      dbConsole));
	return;
    }
    // Determine type of object, and try to add the landing loc
    switch (cObj->GetType()) {
    case HST_SHIP:
	CHSShip * cShip;
	cShip = (CHSShip *) cObj;
	if (!cShip->AddLandingLoc(dbRoom))
	    notify(player,
		   "Failed to add landing loc.  Check log for details.");
	else
	    notify(player, "Landing location added to ship.");
	break;

    case HST_PLANET:
	CHSPlanet * cPlanet = (CHSPlanet *) cObj;
	if (!cPlanet->AddLandingLoc(dbRoom))
	    notify(player,
		   "Failed to add landing loc.  Check log for details.");
	else
	    notify(player, "Landing location added to planet.");
	break;
    }
}

// Allows an attribute to be modified on any type of HSpace object.
HSPACE_COMMAND_HDR(hscSetAttribute)
{
    dbref dbConsole;
    char left_switch[64];
    char right_switch[64];
    char *ptr, *dptr;

    // We have to pull out the left and right parts of
    // the @space/set <console>/<attrib> format.
    dptr = left_switch;
    for (ptr = arg_left; *ptr && *ptr != '/'; ptr++) {
	// Check for buffer overflow
	if ((dptr - left_switch) > 62) {
	    notify(player, "Invalid object/attribute pair supplied.");
	    return;
	}

	*dptr++ = *ptr;
    }
    *dptr = '\0';

    // Right side.
    if (!*ptr) {
	notify(player, "Invalid object/attribute pair supplied.");
	return;
    }
    ptr++;

    dptr = right_switch;
    while (*ptr) {
	*dptr++ = *ptr++;
    }
    *dptr = '\0';

    dbConsole = hsInterface.NoisyMatchThing(player, left_switch);
    if (dbConsole == NOTHING)
	return;

    if (!*right_switch) {
	notify(player, "Invalid or no attribute name supplied.");
	return;
    }
    // Ok, find the HSpace object, and set the attribute.

    // Check flags first.
    if (hsInterface.HasFlag(dbConsole, TYPE_ROOM, ROOM_HSPACE_LANDINGLOC)) {
	CHSLandingLoc *llLoc;

	llLoc = dbHSDB.FindLandingLoc(dbConsole);
	if (!llLoc) {
	    notify(player,
		   "That thing appears to be a landing location, but it's not.");
	    return;
	}
	if (!llLoc->SetAttributeValue(right_switch, arg_right))
	    notify(player, "Attribute - failed.");
	else if (!*arg_right)
	    notify(player, "Attribute - cleared.");
	else
	    notify(player, "Attribute - set.");
    } else if (hsInterface.HasFlag(dbConsole, TYPE_THING,
				   THING_HSPACE_OBJECT)) {
	CHSObject *cObj;

	cObj = dbHSDB.FindObject(dbConsole);
	if (!cObj) {
	    notify(player, "That thing is not a true HSpace object.");
	    return;
	}
	if (!cObj->SetAttributeValue(right_switch, arg_right))
	    notify(player, "Attribute - failed.");
	else if (!*arg_right)
	    notify(player, "Attribute - cleared.");
	else
	    notify(player, "Attribute - set.");
    } else if (hsInterface.HasFlag(dbConsole, TYPE_THING,
				   THING_HSPACE_CONSOLE)) {
	CHSObject *cObj;
	CHSConsole *cConsole;

	cObj = dbHSDB.FindObjectByConsole(dbConsole);
	cConsole = dbHSDB.FindConsole(dbConsole);
	if (!cConsole) {
	    notify(player,
		   "That console does not belong to any HSpace object.");
	    return;
	}

	if (!cObj) {
	    notify(player, "That thing is not a true HSpace object.");
	    return;
	}
	// Try to set the attribute on the console first.
	// If that fails, set it on the object.
	if (!cConsole->SetAttributeValue(right_switch, arg_right)) {
	    if (!cObj->SetAttributeValue(right_switch, arg_right))
		notify(player, "Attribute - failed.");
	    else if (!*arg_right)
		notify(player,
		       unsafe_tprintf("%s Attribute - cleared.",
				      cObj->GetName()));
	    else
		notify(player,
		       unsafe_tprintf("%s Attribute - set.",
				      cObj->GetName()));
	} else
	    notify(player, "Console Attribute - set.");
    } else if (hsInterface.HasFlag(dbConsole, TYPE_ROOM,
				   ROOM_HSPACE_UNIVERSE)) {
	// Set a universe attribute
	CHSUniverse *uDest;
	uDest = uaUniverses.FindUniverse(dbConsole);

	if (!uDest) {
	    notify(player,
		   "That room has a universe flag, but it's not a valid universe.");
	    return;
	}
	if (!uDest->SetAttributeValue(right_switch, arg_right))
	    notify(player, "Attribute - failed.");
	else
	    notify(player, "Attribute - set.");
    } else if (hsInterface.HasFlag(dbConsole, TYPE_THING,
				   THING_HSPACE_TERRITORY)) {
	// Find the territory.
	CHSTerritory *territory;

	territory = taTerritories.FindTerritory(dbConsole);
	if (!territory) {
	    notify(player,
		   "That object has a territory flag, but it's not a valid territory.");
	    return;
	}

	if (!territory->SetAttributeValue(right_switch, arg_right))
	    notify(player, "Attribute - failed.");
	else
	    notify(player, "Attribute - set.");
    } else
	notify(player,
	       "That thing does not appear to be an HSpace object.");
}

// Adds a room to a ship object
HSPACE_COMMAND_HDR(hscAddSroom)
{
    dbref dbConsole;
    dbref dbRoom;
    CHSShip *cShip;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    dbRoom = hsInterface.NoisyMatchRoom(player, arg_right);
    if (dbRoom == NOTHING)
	return;
    if (Typeof(dbRoom) != TYPE_ROOM) {
	notify(player, "You may only register rooms with this command.");
	return;
    }

    cShip = dbHSDB.FindShip(dbConsole);
    if (!cShip) {
	notify(player, "That is not a HSpace ship.");
	return;
    }

    if (cShip->AddSroom(dbRoom))
	notify(player, "Room - added.");
    else
	notify(player, "Room - failed.");
}

// Deletes an sroom from a ship object
HSPACE_COMMAND_HDR(hscDelSroom)
{
    dbref dbConsole;
    dbref dbRoom;
    CHSShip *cShip;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    if (!arg_right || !*arg_right) {
	notify(player, "No room number specified.");
	return;
    }
    dbRoom = strtodbref(arg_right);

    cShip = dbHSDB.FindShip(dbConsole);
    if (!cShip) {
	notify(player, "That is not a HSpace ship.");
	return;
    }

    if (cShip->DeleteSroom(dbRoom))
	notify(player, "Room - deleted.");
    else
	notify(player, "Room - not found.");
}

// Called to allow a player to man a console.
HSPACE_COMMAND_HDR(hscManConsole)
{
    dbref dbConsole;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    if (!hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_CONSOLE)) {
	notify(player, "That thing is not an HSpace console.");
	return;
    }

    if (hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_C_LOCKED)) {
	notify(player, "That console is locked.");
	return;
    }
    // Can puppets man the console?
    if (Typeof(player) != TYPE_PLAYER && HSCONF.forbid_puppets) {
	notify(player, "Only players may man this console.");
	return;
    }

    if (hsInterface.GetLock(dbConsole, LOCK_USE) == player) {
	notify(player, "You are already manning that console.");
	return;
    }

    if (hsInterface.ConsoleUser(dbConsole) != dbConsole
	&& hsInterface.AtrGet(hsInterface.ConsoleUser(dbConsole),
			      "MCONSOLE")) {
	hsInterface.AtrDel(hsInterface.ConsoleUser(dbConsole), "MCONSOLE",
			   GOD);
    }
#ifdef ONLINE_REG
    // Unregistered players can't use the system.
    if (hsInterface.HasFlag(player, TYPE_PLAYER, PLAYER_UNREG)) {
	notify(player,
	       "Unregistered players are not allowed to use the space system.");
	return;
    }
#endif
    // Check if the player isn't already manning a console.
    if (hsInterface.AtrGet(player, "MCONSOLE")) {
	dbref dbOldConsole;

	dbOldConsole = strtodbref(hsInterface.m_buffer);

	hsInterface.SetLock(dbOldConsole, dbOldConsole, LOCK_USE);

	// Delete attribute from player.
	if (hsInterface.AtrGet(player, "MCONSOLE"))
	    hsInterface.AtrDel(player, "MCONSOLE", GOD);

	notify(player,
	       unsafe_tprintf("You unman the %s.", Name(dbOldConsole)));
	notify_except(Location(dbOldConsole), player, player,
		      unsafe_tprintf("%s unmans the %s.", Name(player),
				     Name(dbOldConsole)), 0);
    }
    // Set the lock.
    hsInterface.SetLock(dbConsole, player, LOCK_USE);

    // Set attribute on player.
    hsInterface.AtrAdd(player, "MCONSOLE",
		       unsafe_tprintf("#%d", dbConsole), GOD, AF_MDARK
		       || AF_WIZARD);

    // Give some messages
    notify(player, unsafe_tprintf("You man the %s.", Name(dbConsole)));

    notify_except(Location(dbConsole), player, player,
		  unsafe_tprintf("%s mans the %s.", Name(player),
				 Name(dbConsole)), 0);
}

// Called to allow a player to unman a console.
HSPACE_COMMAND_HDR(hscUnmanConsole)
{
    dbref dbConsole;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    if (!hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_CONSOLE)) {
	notify(player, "You are not manning that object.");
	return;
    }

    if (hsInterface.GetLock(dbConsole, LOCK_USE) != player) {
	notify(player, "You are not manning that console.");
	return;
    }
    // Set the lock, give some messages.
    hsInterface.SetLock(dbConsole, dbConsole, LOCK_USE);

    // Delete attribute from player.
    if (hsInterface.AtrGet(player, "MCONSOLE"))
	hsInterface.AtrDel(player, "MCONSOLE", GOD);

    notify(player, unsafe_tprintf("You unman the %s.", Name(dbConsole)));
    notify_except(Location(dbConsole), player, player,
		  unsafe_tprintf("%s unmans the %s.", Name(player),
				 Name(dbConsole)), 0);
}

// Called to add a given object as a new console for the
// space object.
HSPACE_COMMAND_HDR(hscAddConsole)
{
    dbref dbShip;
    dbref dbConsole;
    CHSObject *cObj;

    dbShip = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbShip == NOTHING)
	return;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_right);
    if (dbConsole == NOTHING)
	return;

    cObj = dbHSDB.FindObject(dbShip);
    if (!cObj) {
	notify(player, "That thing is not an HSpace object.");
	return;
    }

    switch (cObj->GetType()) {
    case HST_SHIP:
	CHSShip * ptr;
	ptr = (CHSShip *) cObj;
	if (!ptr->AddConsole(dbConsole))
	    notify(player, "Console - failed.");
	else
	    notify(player, "Console - added.");
	break;

    default:
	notify(player,
	       "That HSpace object does not support that operation.");
	return;
    }
}

// Called to delete a given console object from a space object
HSPACE_COMMAND_HDR(hscDelConsole)
{
    dbref dbObj;
    dbref dbConsole;
    CHSObject *cObj;

    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    if (!arg_right || !*arg_right) {
	notify(player, "No console number specified.");
	return;
    }
    dbConsole = strtodbref(arg_right);

    cObj = dbHSDB.FindObject(dbObj);
    if (!cObj) {
	notify(player, "That is not a HSpace object.");
	return;
    }

    switch (cObj->GetType()) {
    case HST_SHIP:
	CHSShip * ptr;
	ptr = (CHSShip *) cObj;
	if (!ptr->RemoveConsole(dbConsole))
	    notify(player, "Console - not found.");
	else
	    notify(player, "Console - deleted.");
	break;

    default:
	notify(player,
	       "That HSpace object does not support that operation.");
	break;
    }
}

// Called to delete a given class from the game
HSPACE_COMMAND_HDR(hscDelClass)
{
    UINT uClass;
    CHSShip *cShip;
    CHSUniverse *uSource;
    int idx, jdx;

    if (!arg_left || !*arg_left) {
	notify(player, "Must specify a class number to delete.");
	return;
    }

    uClass = atoi(arg_left);

    // Check to see if any ships remain with that class.
    // If so, don't allow it to be deleted.
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	uSource = uaUniverses.GetUniverse(idx);
	if (!uSource)
	    continue;

	// Search ships in the universe
	for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
	    cShip = (CHSShip *) uSource->GetUnivObject(jdx, HST_SHIP);
	    if (!cShip)
		continue;

	    if (cShip->ClassNum() == uClass) {
		notify(player, "You may not delete a class still in use.");
		return;
	    }
	}
    }

    // If we're here, then no ships of the class were found.
    // Delete it!
    if (!caClasses.RemoveClass(uClass))
	notify(player, "Class - failed.");
    else
	notify(player, "Class - deleted.");
}

// Called to add a new space object to the game.
HSPACE_COMMAND_HDR(hscAddObject)
{
    CHSObject *cObj;
    dbref dbObj;
    CHSUniverse *unDest;
    char strName[32];
    int idx;

    // Find the game object representing the new celestial
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;
    if (Typeof(dbObj) != TYPE_THING) {
	notify(player, unsafe_tprintf("%s(#%d) is not of type THING.",
				      Name(dbObj), dbObj));
	return;
    }
    // See if it's already an object.
    if (dbHSDB.FindObject(dbObj)) {
	notify(player, "That object is already an HSpace object.");
	return;
    }
    // Check the type to turn it into
    if (!arg_right || !*arg_right) {
	notify(player, "You must specify a type of object.");
	return;
    }

    cObj = NULL;

    // Figure out the type of object, allocate the
    // object, etc.
    switch (atoi(arg_right)) {
    case HST_SHIP:
	notify(player, "You must use @space/activate for that type.");
	return;

    case HST_PLANET:
	strcpy(strName, "planet");
	cObj = new CHSPlanet;
	break;

    case HST_NEBULA:
	strcpy(strName, "nebula");
	cObj = new CHSNebula;
	break;

    case HST_ASTEROID:
	strcpy(strName, "asteroid belt");
	cObj = new CHSAsteroid;
	break;

    case HST_BLACKHOLE:
	strcpy(strName, "black hole");
	cObj = new CHSBlackHole;
	break;

    case HST_WORMHOLE:
	strcpy(strName, "wormhole");
	cObj = new CHSWormHole;
	break;

    case HST_NOTYPE:		// Generic object
	cObj = new CHSObject;
	strcpy(strName, "space object");
	break;

    default:
	notify(player, "Invalid type of object specified.");
	return;
    }


    // Add the object to the first available universe
    unDest = NULL;
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (unDest = uaUniverses.GetUniverse(idx))
	    break;
    }
    if (!unDest) {
	notify(player, "No valid universe for this object exists.");
	delete cObj;
	return;
    }
    if (!unDest->AddObject(cObj)) {
	notify(player, "Object could not be added to default universe.");
	delete cObj;
	return;
    }
    // Set the UID of the default universe for this object.
    cObj->SetUID(unDest->GetID());

    cObj->SetDbref(dbObj);

    // Set the flag
    hsInterface.SetFlag(dbObj, THING_HSPACE_OBJECT);

    notify(player,
	   unsafe_tprintf("%s (#%d) - is now a new %s in universe 0.",
			  Name(dbObj), dbObj, strName));
}

// Called to completely repair a ship.
HSPACE_COMMAND_HDR(hscRepairShip)
{
    dbref dbConsole;
    CHSShip *cShip;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    cShip = dbHSDB.FindShip(dbConsole);
    if (!cShip) {
	notify(player, "That is not a HSpace ship.");
	return;
    }
    // Tell the ship to repair!
    cShip->TotalRepair();
    notify(player, "Ship - repaired.");
}

// Allows an attribute for a particular engineering
// system to be changed.
HSPACE_COMMAND_HDR(hscSetSystemAttr)
{
    CHSObject *cObj;
    char *sptr, *dptr;
    char tbuf[256];
    char sysname[64];
    char attrname[64];
    dbref dbObj;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/sysset obj:system/attr=value
    if (!arg_left || !arg_left) {
	notify(player, "You must specify an object and system name.");
	return;
    }
    // Pull out the object of interest
    dptr = tbuf;
    for (sptr = arg_left; *sptr; sptr++) {
	if (*sptr == ':') {
	    *dptr = '\0';
	    break;
	} else
	    *dptr++ = *sptr;
    }
    *dptr = '\0';
    dbObj = hsInterface.NoisyMatchThing(player, tbuf);
    if (dbObj == NOTHING)
	return;

    // Pull out the system name
    dptr = sysname;
    if (!*sptr) {
	notify(player, "Invalid system specified.");
	return;
    }
    sptr++;
    while (*sptr) {
	if ((*sptr == '/') || ((dptr - sysname) > 63))	// Don't allow overflow
	{
	    break;
	} else
	    *dptr++ = *sptr++;
    }
    *dptr = '\0';

    // Now pull out the attribute name
    if (!*sptr || (*sptr != '/')) {
	notify(player, "Invalid command format given.");
	return;
    }
    sptr++;
    dptr = attrname;
    while (*sptr) {
	if ((dptr - attrname) > 63)
	    break;
	else
	    *dptr++ = *sptr++;
    }
    *dptr = '\0';

    // If we're setting the attribute based on a console
    // object, try to find the CHSObject for that console.
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);
    if (!cObj) {
	notify(player,
	       "That thing does not appear to be an HSpace object.");
	return;
    }
    // Currently only support ships
    if (cObj->GetType() != HST_SHIP) {
	notify(player,
	       "That HSpace object does not currently support that operation.");
	return;
    }
    // Try to set the attribute
    CHSShip *cShip;
    cShip = (CHSShip *) cObj;
    if (!cShip->SetSystemAttribute(sysname, attrname, arg_right))
	notify(player, "SystemAttribute - failed.");
    else
	notify(player, "SystemAttribute - set.");
}

// Allows a player to disembark from a ship.
HSPACE_COMMAND_HDR(hscDisembark)
{
    dbref dbRoom;
    dbref dbShip;
    CHSShip *cShip;
    int id;

    // Grab the location of the player.
    dbRoom = Location(player);

    // See if the room has a SHIP attribute on it.
    if (!hsInterface.AtrGet(dbRoom, "SHIP")) {
	notify(player, "You cannot disembark from here.");
	return;
    }
    // Convert the dbref string to a ship dbref
    dbShip = strtodbref(hsInterface.m_buffer);

    // Find the ship.
    cShip = dbHSDB.FindShip(dbShip);
    if (!cShip) {
	notify(player,
	       "This room is setup as a disembarking location, but its SHIP attribute does not reference an actual ship.");
	return;
    }
    // The player can optionally supply and ID for disembarking
    // through a specific board link.
    if (arg_left && *arg_left)
	id = atoi(arg_left);
    else
	id = NOTHING;
    cShip->DisembarkPlayer(player, id);
}

// Called to allow a player board a ship.
HSPACE_COMMAND_HDR(hscBoardShip)
{
    dbref dbShipObj;
    dbref dbBay;
    CHSShip *cShip;
    char *ptr;

    dbShipObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbShipObj == NOTHING)
	return;

    // Check to be sure it's a ship object
    if (!hsInterface.HasFlag(dbShipObj, TYPE_THING, THING_HSPACE_OBJECT)) {
	notify(player, "That thing is not a space vessel.");
	return;
    }
    // Find the CHSShip object based on the ship dbref
    cShip = dbHSDB.FindShip(dbShipObj);
    if (!cShip) {
	notify(player,
	       "That thing seems to be a space vessel, but I couldn't find the ship it belongs to.");
	return;
    }
    // Check to see if the ship object has a boarding loc
    if (!hsInterface.AtrGet(dbShipObj, "BAY")) {
	notify(player,
	       "That thing has no BAY attribute to indicate where to put you.");
	return;
    }
    dbBay = strtodbref(hsInterface.m_buffer);

    // Check boarding code needs
    ptr = cShip->GetBoardingCode();
    if (ptr) {
	if (!arg_right) {
	    notify(player,
		   "You must specify a boarding code for this vessel.");
	    return;
	}

	if (strcmp(arg_right, ptr)) {
	    notify(player, "Invalid boarding code -- permission denied.");
	    return;
	}
    }
    // At this point all is ok.

    // Tell the player about boarding.
    notify(player, unsafe_tprintf("You board the %s.", Name(dbShipObj)));

    // Tell the players inside that the player is boarding.
    hsInterface.NotifyContents(dbBay,
			       unsafe_tprintf
			       ("%s boards the ship from the outside.",
				Name(player)));

    dbref dbPrevLoc = Location(player);

    // Move the player
    moveto(player, dbBay);

    // Tell the players in the previous location that
    // the player boarded.
    hsInterface.NotifyContents(dbPrevLoc,
			       unsafe_tprintf("%s boards the %s.",
					      Name(player),
					      Name(dbShipObj)));

    // Set the player's HSPACE_LOCATION attr.
    hsInterface.AtrAdd(player, "HSPACE_LOCATION",
		       unsafe_tprintf("#%d", dbShipObj), GOD,
		       AF_MDARK | AF_WIZARD);
}

// Called to allow an administrator to add a weapon to a console.
HSPACE_COMMAND_HDR(hscAddWeapon)
{
    CHSConsole *cConsole;
    dbref dbConsole;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    // Check to be sure it's a ship object
    if (!hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_CONSOLE)) {
	notify(player, "That thing is not a HSpace console.");
	return;
    }
    // Find the CHSConsole object based on the console dbref
    cConsole = dbHSDB.FindConsole(dbConsole);
    if (!cConsole) {
	notify(player,
	       "That thing has a console flag, but it's not a console.");
	return;
    }
    // Proper command usage
    if (!arg_right || !*arg_right) {
	notify(player, "You must specify the ID of a weapon type to add.");
	return;
    }

    cConsole->AddWeapon(player, atoi(arg_right));
}

// Called to allow an administrator to delete a weapon 
// from a console.
HSPACE_COMMAND_HDR(hscDelWeapon)
{
    CHSConsole *cConsole;
    dbref dbConsole;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    // Check to be sure it's a console object
    if (!hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_CONSOLE)) {
	notify(player, "That thing is not a HSpace console.");
	return;
    }
    // Find the CHSConsole object based on the console dbref
    cConsole = dbHSDB.FindConsole(dbConsole);
    if (!cConsole) {
	notify(player,
	       "That thing has a console flag, but it's not a console.");
	return;
    }
    // Proper command usage
    if (!arg_right || !*arg_right) {
	notify(player, "You must specify the ID of a weapon type to add.");
	return;
    }

    cConsole->DeleteWeapon(player, atoi(arg_right));
}

// Called to allow an administrator set the
// number of a specific type of munition on a ship.
HSPACE_COMMAND_HDR(hscSetMissile)
{
    CHSShip *cShip;
    dbref dbConsole;
    char strObj[64];
    char strType[8];
    char *sptr, *dptr;

    // Command usage check.
    if (!arg_left || !*arg_left || !arg_right || !*arg_right) {
	notify(player, "Invalid command usage.");
	return;
    }
    // Pull out the object
    dptr = strObj;
    for (sptr = arg_left; *sptr; sptr++) {
	if (*sptr == '/' || ((dptr - strObj) > 63))
	    break;
	else
	    *dptr++ = *sptr;
    }
    *dptr = '\0';

    // Pull out the weapon id
    if (!*sptr) {
	notify(player, "Must specify a weapon type.");
	return;
    }
    sptr++;
    dptr = strType;
    while (*sptr) {
	if ((dptr - strType) > 7)
	    break;
	else
	    *dptr++ = *sptr++;
    }
    *dptr = '\0';

    dbConsole = hsInterface.NoisyMatchThing(player, strObj);
    if (dbConsole == NOTHING)
	return;

    // Is the target object a console?  If so, find
    // the ship through the console.
    if (hsInterface.HasFlag(dbConsole, TYPE_THING, THING_HSPACE_CONSOLE)) {
	CHSObject *cObj;
	cObj = dbHSDB.FindObjectByConsole(dbConsole);
	if (cObj && cObj->GetType() != HST_SHIP) {
	    notify(player, "You can only do that for ships.");
	    return;
	}
	cShip = (CHSShip *) cObj;
    } else
	cShip = dbHSDB.FindShip(dbConsole);

    if (!cShip) {
	notify(player, "That is not an HSpace ship.");
	return;
    }

    cShip->SetNumMissiles(player, atoi(strType), atoi(arg_right));
}

// Adds a new class with the specified name to the game
HSPACE_COMMAND_HDR(hscNewClass)
{
    HSSHIPCLASS *pClass;

    // Must specify a name
    if (!arg_left || !*arg_left) {
	notify(player, "Must specify a name for the class.");
	return;
    }
    // Name limits
    if (strlen(arg_left) > (SHIP_CLASS_NAME_LEN - 1)) {
	notify(player,
	       unsafe_tprintf("Name length may not exceed %d characters.",
			      SHIP_CLASS_NAME_LEN - 1));
	return;
    }
    // Grab a new class.
    pClass = caClasses.NewClass();
    if (!pClass) {
	notify(player,
	       "Failed to allocate a new slot for this class.  Probably too many classes already.");
	return;
    }
    // Zero out the class
    memset(pClass, 0, sizeof(HSSHIPCLASS));

    // Set size to default of 1
    pClass->m_size = 1;

    // Set the name
    strcpy(pClass->m_name, arg_left);

    notify(player, unsafe_tprintf("Ship class \"%s\" - added.", arg_left));
}

// Prints out information about a given class
HSPACE_COMMAND_HDR(hscDumpClassInfo)
{
    HSSHIPCLASS *pClass;
    int iClass;

    // Find the class based on the number
    iClass = atoi(arg_left);
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Print out the info
    notify(player,
	   unsafe_tprintf("Ship Class: %d   %s", iClass, pClass->m_name));
    notify(player,
	   "------------------------------------------------------------");
    notify(player,
	   unsafe_tprintf("Ship Size : %-3d         Cargo Size: %d",
			  pClass->m_size, pClass->m_cargo_size));
    notify(player,
	   unsafe_tprintf("Min. Crew : %-3d         Max Hull  : %d",
			  pClass->m_minmanned, pClass->m_maxhull));
    notify(player,
	   unsafe_tprintf("Can Drop  : %-3s",
			  pClass->m_can_drop ? "YES" : "NO"));

    // print out systems info
    notify(player, "\n- Engineering Systems -");

    CHSEngSystem *cSys, *cNext;
    cSys = pClass->m_systems.GetHead();
    while (cSys) {
	cNext = cSys->GetNext();

	// Print out two per line or just one?
	if (cNext)
	    notify(player,
		   unsafe_tprintf("*%-30s*%s",
				  cSys->GetName(), cNext->GetName()));
	else
	    notify(player, unsafe_tprintf("*%s", cSys->GetName()));

	if (!cNext)
	    break;
	else
	    cSys = cNext->GetNext();
    }
}

HSPACE_COMMAND_HDR(hscAddSysClass)
{
    int iClass;
    HSSHIPCLASS *pClass;

    // Find the class
    iClass = atoi(arg_left);
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Find the system based on the name
    HSS_TYPE type;
    if (!arg_right || !*arg_right) {
	notify(player, "Must specify a system name to add.");
	return;
    }

    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE) {
	notify(player, "Invalid system name specified.");
	return;
    }
    // Try to find the system already on the class
    CHSEngSystem *cSys;

    if (type != HSS_FICTIONAL) {
	cSys = pClass->m_systems.GetSystem(type);
	if (cSys) {
	    notify(player, "That system already exists for that class.");
	    return;
	}
    }
    /* else {
       notify(player, "Classes currently do not support fictional systems.");
       return; } */
    // Add the system
    cSys = pClass->m_systems.NewSystem(type);
    if (!cSys) {
	notify(player, "Failed to add the system to the specified class.");
	return;
    }

    pClass->m_systems.AddSystem(cSys);
    notify(player,
	   unsafe_tprintf("%s system added to class %d.",
			  cSys->GetName(), iClass));
}

// Prints systems information for a given system on a given class.
HSPACE_COMMAND_HDR(hscSysInfoClass)
{
    int iClass;
    HSSHIPCLASS *pClass;

    iClass = atoi(arg_left);
    if (!arg_right || !*arg_right) {
	notify(player, "Must specify a system name.");
	return;
    }
    // Find the class
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Find the system type based on the name
    HSS_TYPE type;
    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE) {
	notify(player, "Invalid system name specified.");
	return;
    }

    CHSEngSystem *cSys;
    // Grab the system from the class
    cSys = pClass->m_systems.GetSystem(type);
    if (!cSys) {
	notify(player, "That system does not exist for that class.");
	return;
    }
    // Print the info
    notify(player, "----------------------------------------");
    cSys->GiveSystemInfo(player);
    notify(player, "----------------------------------------");
}

// Allows an attribute for a particular engineering
// system to be changed on a given ship class.
HSPACE_COMMAND_HDR(hscSetSystemAttrClass)
{
    HSSHIPCLASS *pClass;
    char *sptr, *dptr;
    char tbuf[256];
    char sysname[64];
    char attrname[64];
    int iClass;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/syssetclass class#:system/attr=value
    if (!arg_left || !arg_left) {
	notify(player,
	       "You must specify an class number and system name.");
	return;
    }
    // Pull out the class of interest
    dptr = tbuf;
    for (sptr = arg_left; *sptr; sptr++) {
	if (*sptr == ':') {
	    *dptr = '\0';
	    break;
	} else
	    *dptr++ = *sptr;
    }
    *dptr = '\0';
    iClass = atoi(tbuf);
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Pull out the system name
    dptr = sysname;
    if (!*sptr) {
	notify(player, "Invalid system specified.");
	return;
    }
    sptr++;
    while (*sptr) {
	if ((*sptr == '/') || ((dptr - sysname) > 63))	// Don't allow overflow
	{
	    break;
	} else
	    *dptr++ = *sptr++;
    }
    *dptr = '\0';

    // Now pull out the attribute name
    if (!*sptr || (*sptr != '/')) {
	notify(player, "Invalid command format given.");
	return;
    }
    sptr++;
    dptr = attrname;
    while (*sptr) {
	if ((dptr - attrname) > 63)
	    break;
	else
	    *dptr++ = *sptr++;
    }
    *dptr = '\0';

    // Determine what type of system is being queried.
    HSS_TYPE type;
    CHSEngSystem *cSys;
    type = hsGetEngSystemType(sysname);
    if (type == HSS_NOTYPE) {
	cSys = pClass->m_systems.GetSystemByName(sysname);
	if (!cSys) {
	    notify(player, "That system does not exist on that class.");
	    return;
	}
    } else {
	cSys = pClass->m_systems.GetSystem(type);

	if (!cSys) {
	    notify(player, "That system does not exist on that class.");
	    return;
	}
    }

    // Try to set the attribute
    if (!cSys->SetAttributeValue(attrname, arg_right))
	notify(player, "SystemAttribute - failed.");
    else
	notify(player, "SystemAttribute - set.");
}

// Sets an attribute for a specified class.
HSPACE_COMMAND_HDR(hscSetAttrClass)
{
    int iClass;
    HSSHIPCLASS *pClass;
    char *sptr, *dptr;
    char tbuf[64];

    // Command format is:
    //
    // @space/setclass #/attr=value
    //
    if (!arg_left || !*arg_left) {
	notify(player,
	       "You must specify a class number and attribute name.");
	return;
    }

    dptr = tbuf;
    for (sptr = arg_left; *sptr; sptr++) {
	if ((dptr - tbuf) > 63)
	    break;

	if (*sptr == '/')
	    break;

	*dptr++ = *sptr;
    }
    *dptr = '\0';

    iClass = atoi(tbuf);

    // Find the class
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Pull out the attr name
    char attrname[64];
    if (!*sptr) {
	notify(player, "You must specify an attribute name.");
	return;
    }
    sptr++;
    dptr = attrname;
    while (*sptr) {
	if ((dptr - attrname) > 63)
	    break;

	*dptr++ = *sptr++;
    }
    *dptr = '\0';

    // Match the attribute name
    int len;
    int iVal;
    len = strlen(attrname);

    if (!strncasecmp(attrname, "NAME", len)) {
	if (strlen(arg_right) > (SHIP_CLASS_NAME_LEN - 1)) {
	    notify(player,
		   unsafe_tprintf
		   ("Class name length may not exceed %d characters.",
		    SHIP_CLASS_NAME_LEN - 1));
	    return;
	}

	strcpy(pClass->m_name, arg_right);
    } else if (!strncasecmp(attrname, "SIZE", len)) {
	iVal = atoi(arg_right);
	if (iVal < 1) {
	    notify(player, "Sizes must be greater than 0.");
	    return;
	}

	pClass->m_size = iVal;
    } else if (!strncasecmp(attrname, "MAXHULL", len)) {
	pClass->m_maxhull = atoi(arg_right);
    } else if (!strncasecmp(attrname, "CAN DROP", len)) {
	pClass->m_can_drop = atoi(arg_right);
    } else if (!strncasecmp(attrname, "SPACEDOCK", len)) {
	pClass->m_spacedock = atoi(arg_right);
    } else if (!strncasecmp(attrname, "CARGO", len)) {
	pClass->m_cargo_size = atoi(arg_right);
    } else if (!strncasecmp(attrname, "MINMANNED", len)) {
	pClass->m_minmanned = atoi(arg_right);
    } else {
	notify(player, "Invalid attribute specified.");
	return;
    }

    notify(player, "Class attribute - set.");
}

// Allows an administrator to list information contained
// within a given database (e.g. weapons).
HSPACE_COMMAND_HDR(hscListDatabase)
{
    // Command usage?
    if (!arg_left || !*arg_left) {
	notify(player, "You must specify a given database to list.");
	return;
    }
    // Match the database name
    int len;
    char *ptr, *dptr;
    char strName[32];
    dptr = strName;
    for (ptr = arg_left; *ptr; ptr++) {
	if ((dptr - strName) > 31)
	    break;

	if (*ptr == '/')
	    break;

	*dptr++ = *ptr;
    }
    *dptr = '\0';


    len = strlen(strName);
    if (!strncasecmp(strName, "objects", len)) {
	// Command usage for this is:
	//
	// @space/list objects/uid=type
	HS_TYPE type;

	if (!arg_right || !*arg_right)
	    type = HST_NOTYPE;
	else
	    type = (HS_TYPE) atoi(arg_right);

	// Pull out the universe id
	char *dptr;
	for (dptr = arg_left; *dptr; dptr++) {
	    if (*dptr == '/')
		break;
	}
	if (*dptr != '/') {
	    notify(player, "You must specify a valid universe ID.");
	    return;
	}

	dptr++;
	int uid;
	uid = atoi(dptr);

	// If uid is < 0, list all universes
	if (uid < 0) {
	    // Print the header
	    notify(player,
		   "[Dbref#] Name                    X        Y        Z        UID   Active");

	    CHSUniverse *uSource;
	    int idx;
	    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
		uSource = uaUniverses.GetUniverse(idx);
		if (!uSource)
		    continue;

		int jdx;
		CHSObject *cObj;
		for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
		    cObj = uSource->GetUnivObject(jdx);
		    if (!cObj)
			continue;

		    if (type != HST_NOTYPE && cObj->GetType() != type)
			continue;

		    // Print object info
		    notify(player,
			   unsafe_tprintf
			   ("[%6d] %-24s%-9.0f%-9.0f%-9.0f%-5d  %s",
			    cObj->GetDbref(), cObj->GetName(),
			    cObj->GetX(), cObj->GetY(), cObj->GetZ(),
			    cObj->GetUID(),
			    cObj->IsActive()? "YES" : "NO"));
		}
	    }
	} else {
	    // List just one universe

	    // Print the header
	    notify(player,
		   "[Dbref#] Name                    X        Y        Z        UID   Active");

	    CHSUniverse *uSource;

	    uSource = uaUniverses.FindUniverse(uid);
	    if (!uSource) {
		notify(player, "Invalid universe ID specified.");
		return;
	    }

	    int jdx;
	    CHSObject *cObj;
	    for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
		cObj = uSource->GetUnivObject(jdx);
		if (!cObj)
		    continue;

		if (type != HST_NOTYPE && cObj->GetType() != type)
		    continue;

		// Print object info
		notify(player,
		       unsafe_tprintf
		       ("[%6d] %-24s%-9.0f%-9.0f%-9.0f%-5d  %s",
			cObj->GetDbref(), cObj->GetName(), cObj->GetX(),
			cObj->GetY(), cObj->GetZ(), cObj->GetUID(),
			cObj->IsActive()? "YES" : "NO"));
	    }
	}

    } else if (!strncasecmp(strName, "weapons", len)) {
	waWeapons.PrintInfo(player);
    } else if (!strncasecmp(strName, "universes", len)) {
	uaUniverses.PrintInfo(player);
    } else if (!strncasecmp(strName, "classes", len)) {
	caClasses.PrintInfo(player);
    } else if (!strncasecmp(strName, "territories", len)) {
	taTerritories.PrintInfo(player);
    } else if (!strncasecmp(strName, "destroyed", len)) {
	// Print the header
	notify(player,
	       "[Dbref#] Name                    X        Y        Z        UID");

	CHSUniverse *uSource;
	int idx;
	for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	    uSource = uaUniverses.GetUniverse(idx);
	    if (!uSource)
		continue;

	    int jdx;
	    CHSObject *cObj;
	    for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
		cObj = uSource->GetUnivObject(jdx);
		if (!cObj)
		    continue;

		if (cObj->GetType() != HST_SHIP)
		    continue;

		CHSShip *cShip;
		cShip = (CHSShip *) cObj;

		if (!cShip->IsDestroyed())
		    continue;

		// Print object info
		notify(player,
		       unsafe_tprintf("[%6d] %-24s%-9.0f%-9.0f%-9.0f%d",
				      cObj->GetDbref(),
				      cObj->GetName(),
				      cObj->GetX(),
				      cObj->GetY(),
				      cObj->GetZ(), cObj->GetUID()));
	    }
	}
    } else
	notify(player,
	       "Must specify one of: objects, weapons, universes, classes, or destroyed.");
}

// Prints systems information for a given system on a given object.
HSPACE_COMMAND_HDR(hscSysInfo)
{
    CHSObject *cObj;
    dbref dbObj;

    // Find the object
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;
    if (Typeof(dbObj) != TYPE_THING) {
	notify(player, unsafe_tprintf("%s(#%d) is not of type THING.",
				      Name(dbObj), dbObj));
	return;
    }
    // See if it's a console.  If so, find the ship based on the console.
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE)) {
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    } else {
	cObj = dbHSDB.FindObject(dbObj);
    }

    // See if it's a space object.
    if (!cObj) {
	notify(player,
	       "That is either not a space object, or it has no HSDB_TYPE attribute.");
	return;
    }
    // Find the system type based on the name
    HSS_TYPE type;
    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE) {
	notify(player, "Invalid system name specified.");
	return;
    }

    CHSEngSystem *cSys;
    // Grab the system from the object
    cSys = cObj->GetEngSystem(type);
    if (!cSys) {
	notify(player, "That system does not exist for that object.");
	return;
    }
    // Print the info
    notify(player, "----------------------------------------");
    cSys->GiveSystemInfo(player);
    notify(player, "----------------------------------------");
}

// Deactivates any HSpace object.
HSPACE_COMMAND_HDR(hscCloneObject)
{
    CHSObject *cObj;
    dbref dbObj;

    // Find the object
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    // See if it's a space object.
    if (!(cObj = dbHSDB.FindObject(dbObj))) {
	notify(player,
	       "That is either not a space object, or it has no HSDB_TYPE attribute.");
	return;
    }
    // Only clone ships right now.
    if (cObj->GetType() != HST_SHIP) {
	notify(player, "You may only clone ships at this time.");
	return;
    }

    CHSShip *cShip;
    cShip = (CHSShip *) cObj;

    // Clone it!
    dbref dbShipObj = cShip->Clone();

    if (dbShipObj == NOTHING)
	notify(player, "Clone ship - failed.");
    else
	notify(player,
	       unsafe_tprintf("Clone ship - cloned #%d", dbShipObj));
}

HSPACE_COMMAND_HDR(hscAddTerritory)
{
    dbref dbObj;

    // Find the object
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    // Check to see if it's already a territory.
    CHSTerritory *territory;
    territory = taTerritories.FindTerritory(dbObj);
    if (territory) {
	notify(player, "That object is already a space territory.");
	return;
    }
    // Proper command usage.
    if (!arg_right || !*arg_right) {
	notify(player, "Must specify type of territory to add.");
	return;
    }
    // Grab a new territory
    territory =
	taTerritories.NewTerritory(dbObj, (TERRTYPE) atoi(arg_right));

    if (!territory)
	notify(player, "Territory - failed.");
    else
	notify(player,
	       unsafe_tprintf("%s (#%d) added as a new territory.",
			      Name(dbObj), dbObj));
}

HSPACE_COMMAND_HDR(hscDelTerritory)
{
    dbref dbObj;

    dbObj = strtodbref(arg_left);

    // Check to see if it's a territory.
    CHSTerritory *territory;
    territory = taTerritories.FindTerritory(dbObj);
    if (!territory) {
	notify(player, "No such territory with that dbref.");
	return;
    }

    if (!taTerritories.RemoveTerritory(dbObj))
	notify(player, "Delete territory - failed.");
    else
	notify(player, "Delete territory - deleted.");
}

// Allows an admin to add a new weapon to the weapondb
HSPACE_COMMAND_HDR(hscNewWeapon)
{
    CHSDBWeapon *cWeap;

    // Command usage.
    if (!arg_left || !*arg_left) {
	notify(player, "Must specify the type of weapon to create.");
	return;
    }

    if (!arg_right || !*arg_right) {
	notify(player, "Must specify the name of the new weapon.");
	return;
    }
    // Try to get a new weapon.
    cWeap = waWeapons.NewWeapon(atoi(arg_left));
    if (!cWeap) {
	notify(player, "Failed to create new weapon by specified type.");
	return;
    }

    notify(player, "Weapon - created.");

    // Set the name of the weapon.
    if (!cWeap->SetAttributeValue("NAME", arg_right))
	notify(player, "WARNING: Failed to set weapon name as specified.");
}

// Sets an attribute for a specified weapon.
HSPACE_COMMAND_HDR(hscSetAttrWeapon)
{
    int iWeapon;
    char *ptr;
    char name[64];

    // Command format is:
    //
    // @space/setweapon #/attr=value
    //
    if (!arg_left || !*arg_left) {
	notify(player,
	       "You must specify a weapon number and attribute name.");
	return;
    }
    // Pull out the weapon id and attribute name.
    ptr = strchr(arg_left, '/');
    if (!ptr) {
	notify(player, "Invalid weapon/attribute combination.");
	return;
    }

    *ptr = '\0';
    iWeapon = atoi(arg_left);
    ptr++;
    strncpy(name, ptr, 62);
    name[63] = '\0';

    // See if the weapon exists.
    CHSDBWeapon *cWeap;
    cWeap = waWeapons.GetWeapon(iWeapon, HSW_NOTYPE);
    if (!cWeap) {
	notify(player, "Invalid weapon ID specified.");
	return;
    }
    // Tell the weapon to set the attribute value.
    if (!cWeap->SetAttributeValue(name, arg_right))
	notify(player, "Attribute - failed.");
    else
	notify(player, "Attribute - set.");
}

HSPACE_COMMAND_HDR(hscAddSys)
{
    dbref dbObj;
    CHSShip *pShip;
    CHSObject *cObj;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/sysset obj:system/attr=value
    if (!arg_left || !arg_right || !*arg_right) {
	notify(player, "You must specify an object and system name.");
	return;
    }
    // Pull out the object of interest

    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    // Find the system based on the name
    HSS_TYPE type;

    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE) {
	notify(player, "Invalid system name specified.");
	return;
    }

    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);
    if (!cObj) {
	notify(player,
	       "That thing does not appear to be an HSpace object.");
	return;
    }
    // Currently only support ships
    if (cObj->GetType() != HST_SHIP) {
	notify(player,
	       "That HSpace object does not currently support that operation.");
	return;
    }

    pShip = (CHSShip *) cObj;

    // Try to find the system already on the class
    CHSEngSystem *cSys;

    if (type != HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystem(type);
	if (cSys) {
	    notify(player, "That system already exists for that ship.");
	    return;
	}
    }
    // Add the system
    cSys = pShip->m_systems.NewSystem(type);
    if (!cSys) {
	notify(player, "Failed to add the system to the specified ship.");
	return;
    }

    /* Lack of owner caused core dump - TM3 */

    cSys->SetOwner(cObj);
    pShip->m_systems.AddSystem(cSys);
    notify(player,
	   unsafe_tprintf("%s system added to the %s.",
			  cSys->GetName(), pShip->GetName()));
}

HSPACE_COMMAND_HDR(hscDelSys)
{
    dbref dbObj;
    CHSShip *pShip;
    CHSObject *cObj;
    CHSEngSystem *cSys;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/sysset obj:system/attr=value
    if (!arg_left || !arg_right || !*arg_right) {
	notify(player, "You must specify an object and system name.");
	return;
    }
    // Pull out the object of interest

    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);


    if (!cObj) {
	notify(player,
	       "That thing does not appear to be an HSpace object.");
	return;
    }
    // Currently only support ships
    if (cObj->GetType() != HST_SHIP) {
	notify(player,
	       "That HSpace object does not currently support that operation.");
	return;
    }

    pShip = (CHSShip *) cObj;

    // Find the system based on the name
    HSS_TYPE type;

    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystemByName(arg_right);
	if (!cSys) {
	    notify(player, "Invalid system name specified.");
	    return;
	} else {
	    type = HSS_FICTIONAL;
	}
    }
    // See if the system is at the ship.

    if (type != HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystem(type);
	if (!cSys) {
	    notify(player, "That system does not exists for that ship.");
	    return;
	}
    }


    char tmp[64];
    sprintf(tmp, "%s", cSys->GetName());

    // Delete the system
    if (pShip->m_systems.DelSystem(cSys))
	notify(player,
	       unsafe_tprintf("%s system deleted from the %s.",
			      tmp, pShip->GetName()));
    else
	notify(player, "Failed to delete system.");
}

HSPACE_COMMAND_HDR(hscDelSysClass)
{
    int iClass;
    HSSHIPCLASS *pClass;

    // Find the class
    iClass = atoi(arg_left);
    pClass = caClasses.GetClass(iClass);
    if (!pClass) {
	notify(player, "Ship class non-existent.");
	return;
    }
    // Find the system based on the name
    if (!arg_right || !*arg_right) {
	notify(player, "Must specify a system name to delete.");
	return;
    }
    // Try to find out if the system on the class.
    CHSEngSystem *cSys;
    // Find the system based on the name
    HSS_TYPE type;

    type = hsGetEngSystemType(arg_right);
    if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	cSys = pClass->m_systems.GetSystemByName(arg_right);
	if (!cSys) {
	    notify(player, "Invalid system name specified.");
	    return;
	} else {
	    type = HSS_FICTIONAL;
	}
    }

    if (type != HSS_FICTIONAL) {
	cSys = pClass->m_systems.GetSystem(type);
	if (!cSys) {
	    notify(player, "That system does not exists for that class.");
	    return;
	}
    }

    char tmp[64];
    sprintf(tmp, "%s", cSys->GetName());

    if (pClass->m_systems.DelSystem(cSys))
	notify(player,
	       unsafe_tprintf("%s system deleted from class %d.",
			      tmp, iClass));
    else
	notify(player, "Failed to delete system.");
}


/* Special thanks to Chronus for supplying me with this
	piece of code to use in the new patch, thanks Chronus! */
HSPACE_COMMAND_HDR(hscDumpWeapon)
{
    int iWeapon;
    CHSDBWeapon *pWeapon;

    iWeapon = atoi(arg_left);

    if (!waWeapons.GoodWeapon(iWeapon)) {
	notify(player, "That weapon does not exist!");
	return;
    }

    pWeapon = waWeapons.GetWeapon(iWeapon, HSW_NOTYPE);

    notify(player, unsafe_tprintf("Weapon: %d    %s", iWeapon,
				  pWeapon->m_name));
    notify(player,
	   "------------------------------------------------------------");

    if (pWeapon->m_type == 0)
	notify(player,
	       unsafe_tprintf("Type: Laser          Regen Time: %s",
			      pWeapon->GetAttributeValue("REGEN TIME")));
    if (pWeapon->m_type == 1)
	notify(player,
	       unsafe_tprintf("Type: Missile        Reload Time: %s",
			      pWeapon->GetAttributeValue("RELOAD TIME")));

    char tbuf[256];

    if (pWeapon->m_type == 0) {
	CHSDBLaser *pLaser;

	pLaser = (CHSDBLaser *) pWeapon;
	sprintf(tbuf, "Strength: %-3d        Range: %d",
		pLaser->m_strength, pLaser->m_range);
	notify(player, tbuf);
	sprintf(tbuf, "Accuracy: %-3d        Power Use: %d",
		pLaser->m_accuracy, pLaser->m_powerusage);
	notify(player, tbuf);
	notify(player, unsafe_tprintf("Nohull: %d", pLaser->m_nohull));
    }
    if (pWeapon->m_type == 1) {
	CHSDBMissile *pMissile;

	pMissile = (CHSDBMissile *) pWeapon;

	sprintf(tbuf, "Strength: %-3d        Range: %d",
		pMissile->m_strength, pMissile->m_range);
	notify(player, tbuf);
	sprintf(tbuf, "Turnrate: %-3d        Speed: %d",
		pMissile->m_turnrate, pMissile->m_speed);
	notify(player, tbuf);
    }
}

// Adds a room to a ship object
HSPACE_COMMAND_HDR(hscAddHatch)
{
    dbref dbConsole;
    dbref dbExit;
    CHSShip *cShip;

    dbConsole = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbConsole == NOTHING)
	return;

    dbExit = hsInterface.NoisyMatchExit(player, arg_right);
    if (dbExit == NOTHING)
	return;
    if (Typeof(dbExit) != TYPE_EXIT) {
	notify(player, "You may only register exits with this command.");
	return;
    }

    cShip = dbHSDB.FindShip(dbConsole);
    if (!cShip) {
	notify(player, "That is not a HSpace ship.");
	return;
    }

    if (cShip->AddHatch(dbExit))
	notify(player, "Hatch - added.");
    else
	notify(player, "Hatch - failed.");
}

HSPACE_COMMAND_HDR(hscAddMessage)
{
    CHSConsole *cConsole;
    dbref dbObj;
    int idx;
    BOOL Success = FALSE;

    // Find the game object representing the console.
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    // See if it's a console.
    cConsole = dbHSDB.FindConsole(dbObj);
    if (!cConsole) {
	notify(player, "That is not a valid console.");
	return;
    }
    // Check the type to turn it into
    if (!arg_right || !*arg_right) {
	notify(player, "You must specify a type of message.");
	return;
    }
    // Figure out the message type.
    switch (atoi(arg_right)) {
    case MSG_GENERAL:
	notify(player, "All consoles hear general messages.");
	return;

    case MSG_SENSOR:
	Success = cConsole->AddMessage(MSG_SENSOR);
	break;

    case MSG_ENGINEERING:
	Success = cConsole->AddMessage(MSG_ENGINEERING);
	break;

    case MSG_COMBAT:
	Success = cConsole->AddMessage(MSG_COMBAT);
	break;

    case MSG_COMMUNICATION:
	Success = cConsole->AddMessage(MSG_COMMUNICATION);
	break;

    default:
	notify(player, "Invalid type of message specified.");
	return;
    }

    if (Success)
	notify(player,
	       unsafe_tprintf("%s - Message type %d added.", Name(dbObj),
			      atoi(arg_right)));
    else
	notify(player,
	       unsafe_tprintf("%s - Failed to add message type.",
			      Name(dbObj)));
}

HSPACE_COMMAND_HDR(hscDelMessage)
{
    CHSConsole *cConsole;
    dbref dbObj;
    int idx;
    BOOL Success = FALSE;

    // Find the game object representing the console.
    dbObj = hsInterface.NoisyMatchThing(player, arg_left);
    if (dbObj == NOTHING)
	return;

    // See if it's a console.
    cConsole = dbHSDB.FindConsole(dbObj);
    if (!cConsole) {
	notify(player, "That is not a valid console.");
	return;
    }
    // Check the type to turn it into
    if (!arg_right || !*arg_right) {
	notify(player, "You must specify a type of message.");
	return;
    }
    // Figure out the message type.
    switch (atoi(arg_right)) {
    case MSG_GENERAL:
	notify(player, "All consoles hear general messages.");
	return;

    case MSG_SENSOR:
	Success = cConsole->DelMessage(MSG_SENSOR);
	break;

    case MSG_ENGINEERING:
	Success = cConsole->DelMessage(MSG_ENGINEERING);
	break;

    case MSG_COMBAT:
	Success = cConsole->DelMessage(MSG_COMBAT);
	break;

    case MSG_COMMUNICATION:
	Success = cConsole->DelMessage(MSG_COMMUNICATION);
	break;

    default:
	notify(player, "Invalid type of message specified.");
	return;
    }

    if (Success)
	notify(player,
	       unsafe_tprintf("%s - Message type %d deleted.", Name(dbObj),
			      atoi(arg_right)));
    else
	notify(player,
	       unsafe_tprintf("%s - Failed to delete message type.",
			      Name(dbObj)));
}
