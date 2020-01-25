#include "hscopyright.h"
#include "hscmds.h"
#include "hsdb.h"
#include "hseng.h"
#include "hsinterface.h"
#include "hsutils.h"
#include "hsconf.h"
#include <stdlib.h>
#include "hsswitches.h"

HSPACE_COMMAND_PROTO(hscSetVelocity)
    HSPACE_COMMAND_PROTO(hscSetHeading)
    HSPACE_COMMAND_PROTO(hscSetRoll)
    HSPACE_COMMAND_PROTO(hscSensorReport)
    HSPACE_COMMAND_PROTO(hscNavStatus)
    HSPACE_COMMAND_PROTO(hscSetMapRange)
    HSPACE_COMMAND_PROTO(hscLandVessel)
    HSPACE_COMMAND_PROTO(hscUndockVessel)
    HSPACE_COMMAND_PROTO(hscDoAfterburn)
    HSPACE_COMMAND_PROTO(hscDoJump)
    HSPACE_COMMAND_PROTO(hscDoBoardLink)
    HSPACE_COMMAND_PROTO(hscBreakBoardLink)
    HSPACE_COMMAND_PROTO(hscTaxiVessel)
    HSPACE_COMMAND_PROTO(hscDoView)
    HSPACE_COMMAND_PROTO(hscScan)
    HSPACE_COMMAND_PROTO(hscDoCloak)
    HSPACE_COMMAND_PROTO(hscDoSView)
    HSPACE_COMMAND_PROTO(hscDoGate)
    HSPACE_COMMAND_PROTO(hscHatchStat)
    HSPACE_COMMAND_PROTO(hscTractorMode)
    HSPACE_COMMAND_PROTO(hscTractorLock)
    HSPACE_COMMAND_PROTO(hscTractorDock)
// The hsNavCommandArray holds all externally callable 
// @nav commands.
HSPACE_COMMAND hsNavCommandArray[] = {
    NSNAV_AFTERBURN, hscDoAfterburn, HCP_ANY,
    HSNAV_BOARDLINK, hscDoBoardLink, HCP_ANY,
    HSNAV_BOARDUNLINK, hscBreakBoardLink, HCP_ANY,
    HSNAV_JUMP, hscDoJump, HCP_ANY,
    HSNAV_LAND, hscLandVessel, HCP_ANY,
    HSNAV_SETHEADING, hscSetHeading, HCP_ANY,
    HSNAV_MAPRANGE, hscSetMapRange, HCP_ANY,
    HSNAV_SCAN, hscScan, HCP_ANY,
    HSNAV_HSTAT, hscHatchStat, HCP_ANY,
    HSNAV_SENSORREPORT, hscSensorReport, HCP_ANY,
    HSNAV_SETVELOCITY, hscSetVelocity, HCP_ANY,
    HSNAV_SETROLL, hscSetRoll, HCP_ANY,
    HSNAV_STATUS, hscNavStatus, HCP_ANY,
    HSNAV_TAXI, hscTaxiVessel, HCP_ANY,
    HSNAV_UNDOCK, hscUndockVessel, HCP_ANY,
    HSNAV_VIEW, hscDoView, HCP_ANY,
    HSNAV_CLOAK, hscDoCloak, HCP_ANY,
    HSNAV_SVIEW, hscDoSView, HCP_ANY,
    HSNAV_GATE, hscDoGate, HCP_ANY,
    HSNAV_TRACTORMODE, hscTractorMode, HCP_ANY,
    HSNAV_TRACTORLOCK, hscTractorLock, HCP_ANY,
    HSNAV_TRACTORDOCK, hscTractorDock, HCP_ANY,
    0, NULL, 0
};

// Sets the velocity of a vessel
HSPACE_COMMAND_HDR(hscSetVelocity)
{
    dbref dbUser;
    CHSShip *cShip;
    int iVel;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    iVel = atoi(arg_left);
    cShip->SetVelocity(dbUser, iVel);
}

// Sets the heading of a vessel
HSPACE_COMMAND_HDR(hscSetHeading)
{
    dbref dbUser;
    CHSShip *cShip;
    int xy, z;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left)
	xy = NOTHING;
    else
	xy = atoi(arg_left);

    if (!arg_right || !*arg_right)
	z = NOTHING;
    else
	z = atoi(arg_right);

    cShip->SetHeading(dbUser, xy, z);
}

// Sets the roll of a vessel
HSPACE_COMMAND_HDR(hscSetRoll)
{
    dbref dbUser;
    CHSShip *cShip;
    int roll;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left)
	roll = NOTHING;
    else
	roll = atoi(arg_left);

    cShip->SetHeading(dbUser, roll);
}


// Prints out a sensor report, perhaps for a specific object type.
HSPACE_COMMAND_HDR(hscSensorReport)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left)
	cShip->GiveSensorReport(dbUser);
    else
	cShip->GiveSensorReport(dbUser, (HS_TYPE) atoi(arg_left));
}

// Gives the big navigation status display
HSPACE_COMMAND_HDR(hscNavStatus)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    cShip->GiveNavigationStatus(dbUser);
}

// Allows the player to set the ship's map range
HSPACE_COMMAND_HDR(hscSetMapRange)
{
    dbref dbUser;
    CHSShip *cShip;
    char tbuf[64];
    int units;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    units = atoi(arg_left);
    if (!cShip->SetMapRange(dbUser, units))
	hsStdError(dbUser, "Failed to set map range.");
    else {
	sprintf(tbuf, "Map range now set to %d %s.",
		units, HSCONF.unit_name);
	hsStdError(dbUser, tbuf);
    }
}

// Allows a player to land the vessel on some object, at some
// location, with a code (if needed)
HSPACE_COMMAND_HDR(hscLandVessel)
{
    dbref dbUser;
    CHSShip *cShip;
    char *sptr, *dptr;
    int id, loc;
    char tbuf[64];

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a target ID/location.");
	return;
    }
    // The format of the landing command is:
    //
    // @nav/land ID/loc=code
    //
    // Parse out this info
    dptr = tbuf;
    for (sptr = arg_left; *sptr; sptr++) {
	if (*sptr == '/') {
	    *dptr = '\0';
	    id = atoi(tbuf);
	    break;
	} else
	    *dptr++ = *sptr;
    }

    // Was a location specified?
    if (!*sptr) {
	notify(player,
	       "You must specify the number of a location at that target.");
	return;
    }
    // Skip the slash
    sptr++;
    dptr = tbuf;
    while (*sptr) {
	*dptr++ = *sptr++;
    }
    *dptr = '\0';
    loc = atoi(tbuf);

    // Tell the ship to land.
    cShip->LandVessel(dbUser, id, loc, arg_right);
}

// Allows the player to undock/lift off the vessel
HSPACE_COMMAND_HDR(hscUndockVessel)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (cShip->GetMannedConsoles() < cShip->GetMinManned()) {
	hsStdError(dbUser,
		   unsafe_tprintf
		   ("Only %d out of a minimum of %d consoles are manned.",
		    cShip->GetMannedConsoles(), cShip->GetMinManned()));
	return;
    }

    cShip->UndockVessel(dbUser);
}

// Allows a player to engage/disengage the afterburners.
HSPACE_COMMAND_HDR(hscDoAfterburn)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify 0 or 1 for afterburn status.");
	return;
    }
    cShip->EngageAfterburn(dbUser, atoi(arg_left));
}

// Allows a player to engage/disengage the jump drive.
HSPACE_COMMAND_HDR(hscDoJump)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser,
	       "You must specify 0 or 1 for jump drive engage status.");
	return;
    }
    cShip->EngageJumpDrive(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscDoBoardLink)
{
    dbref dbUser;
    CHSShip *cShip;
    char left_switch[64];
    char right_switch[64];
    char *ptr, *dptr;


    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // We have to pull out the left and right parts of
    // the @space/set <console>/<attrib> format.
    dptr = left_switch;
    for (ptr = arg_left; *ptr && *ptr != '/'; ptr++) {
	// Check for buffer overflow
	if ((dptr - left_switch) > 62) {
	    notify(player, "Invalid contact/local hatch pair supplied.");
	    return;
	}

	*dptr++ = *ptr;
    }
    *dptr = '\0';

    // Right side.
    if (!*ptr) {
	notify(player, "Invalid contact/local hatch pair supplied.");
	return;
    }
    ptr++;

    dptr = right_switch;
    while (*ptr) {
	*dptr++ = *ptr++;
    }
    *dptr = '\0';

    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_right || !*arg_right) {
	notify(dbUser, "You must specify a target hatch.");
	return;
    }

    if (!left_switch || !*left_switch) {
	notify(dbUser, "You must specify a target contact.");
	return;
    }
    if (!right_switch || !*right_switch) {
	notify(dbUser, "You must specify a local hatch.");
	return;
    }
    // Tell the ship to board link.
    cShip->DoBoardLink(dbUser, atoi(left_switch), atoi(right_switch),
		       atoi(arg_right));
}

HSPACE_COMMAND_HDR(hscBreakBoardLink)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a target contact number.");
	return;
    }
    // Tell the ship to board link.
    cShip->DoBreakBoardLink(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscTaxiVessel)
{
    dbref dbUser;
    CHSShip *cShip;
    dbref dbObj;
    dbref dbExit;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a direction to taxi.");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }
    // Ship must not be in space
    if (!cShip->Landed()) {
	hsStdError(dbUser, "Must be landed or docked to taxi.");
	return;
    }
    // Grab the ship's shipobj
    dbObj = cShip->GetDbref();
    if (dbObj == NOTHING) {
	notify(dbUser, "Your ship has no ship object to taxi around.");
	return;
    }
    // Find the exit
    dbExit = hsInterface.NoisyMatchExit(dbObj, arg_left);
    if (dbExit == NOTHING || Typeof(dbExit) != TYPE_EXIT) {
	notify(dbUser, "That is not a valid direction to taxi.");
	return;
    }
    // Try to move the ship object
    if (Location(dbExit) == NOTHING) {
	notify(dbUser,
	       "That location leads to nowhere.  You don't want to go there.");
	return;
    }
    // See if the object passes the lock
    if (!hsInterface.PassesLock(dbObj, dbExit, LOCK_NORMAL)) {
	hsStdError(dbUser, "That location is blocked to our departure.");
	return;
    }
    moveto(dbObj, Location(dbExit));

    hsInterface.NotifyContents(Location(dbObj),
			       unsafe_tprintf
			       ("The %s fires is vector rockets and taxis to %s.",
				cShip->GetName(), Name(Location(dbExit))));
    moveto(dbObj, Location(dbExit));
    hsInterface.NotifyContents(Location(dbExit),
			       unsafe_tprintf
			       ("The %s taxis in, firing its vector rockets.",
				cShip->GetName()));
    hsStdError(dbUser, "You fire the vector rockets and taxi onward.");
    look_room(dbUser, Location(dbExit), 0);
}

HSPACE_COMMAND_HDR(hscDoView)
{
    dbref dbUser;
    CHSShip *cShip;
    dbref dbObj;
    dbref dbLoc;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Ship must not be in space
    if (!cShip->Landed()) {
	hsStdError(dbUser, "This vessel is not currently landed.");
	return;
    }
    // Grab the ship's shipobj
    dbObj = cShip->GetDbref();
    if (dbObj == NOTHING) {
	notify(dbUser,
	       "Your ship has no ship object.  Don't know where you're landed.");
	return;
    }
    // Find the location of the shipobj
    dbLoc = Location(dbObj);

    notify(dbUser, "Outside the vessel you see ...");
    look_room(dbUser, dbLoc, 0);
}

// Allows a player to scan an object on sensors
HSPACE_COMMAND_HDR(hscScan)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a target contact number.");
	return;
    }
    // Tell the ship to board link.
    cShip->ScanObjectID(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscDoCloak)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser,
	       "You must specify 0 or 1 for cloaking device engage status.");
	return;
    }
    cShip->EngageCloak(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscDoSView)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a target contact number.");
	return;
    }
    // Tell the ship to view the object.
    cShip->ViewObjectID(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscDoGate)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    if (!arg_left || !*arg_left) {
	notify(dbUser, "You must specify a target contact number.");
	return;
    }
    // Tell the ship to gate the object.
    cShip->GateObjectID(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscHatchStat)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    cShip->GiveHatchRep(dbUser);
}

HSPACE_COMMAND_HDR(hscTractorMode)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    CHSSysTractor *cTractor;
    cTractor = (CHSSysTractor *) cShip->m_systems.GetSystem(HSS_TRACTOR);
    if (!cTractor) {
	hsStdError(dbUser, "No tractor beam system on this vessel.");
	return;
    }

    if (atoi(arg_left) < 0 || atoi(arg_left) > 2) {
	hsStdError(dbUser, "Invalid tractor beam mode setting.");
	return;
    }

    cTractor->SetMode(atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscTractorLock)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    CHSSysTractor *cTractor;
    cTractor = (CHSSysTractor *) cShip->m_systems.GetSystem(HSS_TRACTOR);
    if (!cTractor) {
	hsStdError(dbUser, "No tractor beam system on this vessel.");
	return;
    }
    // Tell the ship to lock the object.
    cTractor->SetLock(dbUser, atoi(arg_left));
}

HSPACE_COMMAND_HDR(hscTractorDock)
{
    dbref dbUser;
    CHSShip *cShip;

    // Find the ship based on the console
    cShip = (CHSShip *) dbHSDB.FindObjectByConsole(player);
    if (!cShip || (cShip->GetType() != HST_SHIP)) {
	notify(player, "Go away!  You are not an HSpace ship!");
	return;
    }
    // Find the user of the console.
    dbUser = hsInterface.ConsoleUser(player);
    if (dbUser == NOTHING)
	return;

    // Is the ship destroyed?
    if (cShip->IsDestroyed()) {
	notify(dbUser, "This vessel has been destroyed!");
	return;
    }
    // Find the console object
    CHSConsole *cConsole;
    cConsole = cShip->FindConsole(player);
    if (cConsole && !cConsole->IsOnline()) {
	hsStdError(dbUser,
		   unsafe_tprintf("%s is currently powered down.",
				  Name(player)));
	return;
    }

    CHSSysTractor *cTractor;
    cTractor = (CHSSysTractor *) cShip->m_systems.GetSystem(HSS_TRACTOR);
    if (!cTractor) {
	hsStdError(dbUser, "No tractor beam system on this vessel.");
	return;
    }

    if (!arg_left) {
	hsStdError(dbUser, "No docking location specified.");
	return;
    }

    if (!arg_right) {
	hsStdError(dbUser, "No target contact specified.");
	return;
    }
    // Tell the ship to dock the object.
    cTractor->DockShip(dbUser, atoi(arg_right), atoi(arg_left));
}
