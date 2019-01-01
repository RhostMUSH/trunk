#include "hscopyright.h"
#include "hscmds.h"
#include "hsdb.h"
#include "hseng.h"
#include "hsinterface.h"
#include "hsutils.h"
#include <stdlib.h>
#include "hsswitches.h"

HSPACE_COMMAND_PROTO(hscSetSystemPower)
    HSPACE_COMMAND_PROTO(hscSystemReport)
    HSPACE_COMMAND_PROTO(hscSetSystemPriority)
    HSPACE_COMMAND_PROTO(hscShipStats)
    HSPACE_COMMAND_PROTO(hscCrewRep)
    HSPACE_COMMAND_PROTO(hscAssignCrew)
    HSPACE_COMMAND_PROTO(hscSelfDestruct)
// The hsCommandArray holds all externally callable @eng commands.
HSPACE_COMMAND hsEngCommandArray[] = {
    HSENG_SELFDESTRUCT, hscSelfDestruct, HCP_ANY,
    HSENG_SETSYSPOWER, hscSetSystemPower, HCP_ANY,
    HSENG_SHIPSTATS, hscShipStats, HCP_ANY,
    HSENG_SYSTEMREPORT, hscSystemReport, HCP_ANY,
    HSENG_SYSTEMPRIORITY, hscSetSystemPriority, HCP_ANY,
    HSENG_CREWREP, hscCrewRep, HCP_ANY,
    HSENG_ASSIGNCREW, hscAssignCrew, HCP_ANY,
    NULL, NULL, 0
};

HSPACE_COMMAND_HDR(hscSetSystemPower)
{
    dbref dbUser;
    CHSShip *cShip;
    int iLvl;

    // arg_left is the system name
    // arg_right is the level to set

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

    if (!arg_left || !*arg_left || !arg_right || !*arg_right) {
	hsStdError(dbUser,
		   "You must specify both a system name and power level.");
	return;
    }

    iLvl = atoi(arg_right);
    if (arg_right[strlen(arg_right) - 1] == '%')
	cShip->SetSystemPower(dbUser, arg_left, iLvl, TRUE);
    else
	cShip->SetSystemPower(dbUser, arg_left, iLvl, FALSE);
}

HSPACE_COMMAND_HDR(hscSystemReport)
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

    cShip->GiveEngSysReport(dbUser);
}

HSPACE_COMMAND_HDR(hscSetSystemPriority)
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

    if (!arg_left || !*arg_left || !arg_right || !*arg_right) {
	hsStdError(dbUser,
		   "You must specify both a system name and priority change.");
	return;
    }

    cShip->ChangeSystemPriority(dbUser, arg_left, atoi(arg_right));
}

HSPACE_COMMAND_HDR(hscShipStats)
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

    cShip->GiveVesselStats(dbUser);
}

HSPACE_COMMAND_HDR(hscCrewRep)
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

    cShip->GiveCrewRep(dbUser);
}

HSPACE_COMMAND_HDR(hscAssignCrew)
{
    dbref dbUser;
    CHSShip *cShip;
    int iCrew;

    // arg_left is the system name
    // arg_right is the level to set

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

    if (!arg_left || !*arg_left || !arg_right || !*arg_right) {
	hsStdError(dbUser,
		   "You must specify both a crew number and a system.");
	return;
    }

    iCrew = atoi(arg_left);
    cShip->AssignCrew(dbUser, iCrew, arg_right);
}

HSPACE_COMMAND_HDR(hscSelfDestruct)
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

    if (!arg_left || !*arg_left) {
	hsStdError(dbUser, "You must specify the timer value.");
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

    cShip->InitSelfDestruct(dbUser, atoi(arg_left), arg_right);
}
