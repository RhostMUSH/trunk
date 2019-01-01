#include "hscopyright.h"
#include "hsobjects.h"
#include "hsutils.h"
#include "hsinterface.h"
#include "hspace.h"
#include "ansi.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Extern trig stuff
extern double d2cos_table[];
extern double d2sin_table[];

//
// CHSConsole Stuff
//
CHSConsole::CHSConsole(void)
{
    int idx;

    m_weapon_array = NULL;
    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++)
	m_msgtypes[idx] = NOTHING;

    m_xyheading = 0;
    m_zheading = 0;
    m_xyoffset = 0;		// Front of ship by default
    m_zoffset = 0;
    m_arc = 30;			// Default of 30 degree arc

    m_objnum = NOTHING;
    m_ownerObj = NULL;
    m_missile_bay = NULL;
    m_target_lock = NULL;
    m_can_rotate = FALSE;
    m_online = FALSE;
    m_autoload = FALSE;
    m_computer = NULL;
    m_targetting = HSS_NOTYPE;
}

CHSConsole::~CHSConsole(void)
{
    // Clear attributes and flags
    ClearObjectAttrs();

    hsInterface.UnsetFlag(m_objnum, THING_HSPACE_CONSOLE);
}

// Indicates if the console is powered up.  If there is
// no computer, the console requires no power.
BOOL CHSConsole::IsOnline(void)
{
    if (!m_computer)
	return TRUE;

    return m_online;
}

// Sets the computer source for the console.  If no
// source is set, the console requires no computer.
void CHSConsole::SetComputer(CHSSysComputer * cComputer)
{
    m_computer = cComputer;
}

// Saves information in the form of attributes on the
// object for this console.
void CHSConsole::WriteObjectAttrs(void)
{
    int idx;
    char tbuf[256];
    char tbuf2[32];

    if (m_objnum == NOTHING)
	return;

    hsInterface.AtrAdd(m_objnum, "HSDB_XYHEADING",
		       unsafe_tprintf("%d", m_xyheading), GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_ZHEADING",
		       unsafe_tprintf("%d", m_zheading), GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_XYOFFSET",
		       unsafe_tprintf("%d", m_xyoffset), GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_ZOFFSET",
		       unsafe_tprintf("%d", m_zoffset), GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_CAN_ROTATE",
		       unsafe_tprintf("%d", m_can_rotate), GOD);

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	if (m_msgtypes[idx] == NOTHING)
	    continue;
	sprintf(tbuf, "HSDB_MSGTYPE_%d", idx);
	hsInterface.AtrAdd(m_objnum, tbuf,
			   unsafe_tprintf("%d", m_msgtypes[idx]), GOD,
			   AF_MDARK || AF_WIZARD);

    }

    // Write weapons info
    if (m_weapon_array) {
	CHSWeapon *cWeap;
	char strWeaponID[32];

	*tbuf = '\0';
	for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	    if (cWeap = m_weapon_array->GetWeapon(idx)) {
		if (!*tbuf)
		    sprintf(tbuf2, "%d", cWeap->m_class);
		else
		    sprintf(tbuf2, " %d", cWeap->m_class);

		strcat(tbuf, tbuf2);

		// Save the weapon status
		sprintf(tbuf2, "%d", cWeap->GetStatusInt());
		sprintf(strWeaponID, "WEAPON_STAT_%d", idx);
		hsInterface.AtrAdd(m_objnum, strWeaponID, tbuf2, GOD);
	    }
	}
	hsInterface.AtrAdd(m_objnum, "HSDB_WEAPONS", tbuf, GOD);
    } else
	hsInterface.AtrAdd(m_objnum, "HSDB_WEAPONS", NULL, GOD);
}

// Takes a string of space delimited weapon IDs and loads
// them into the weapons array.
void CHSConsole::LoadWeapons(char *strWeapons)
{
    char tbuf[32];
    int idx;
    int iVal;

    // Do we have a weapons array?
    if (!m_weapon_array)
	m_weapon_array = new CHSWeaponArray;

    // Extract the IDs from the list, and load
    // them into the array.
    idx = 0;
    extract(strWeapons, tbuf, 0, 1, ' ');
    while (*tbuf) {
	iVal = atoi(tbuf);

	m_weapon_array->NewWeapon(iVal);

	idx++;
	extract(strWeapons, tbuf, idx, 1, ' ');
    }
}

// Clears the attributes from the object
void CHSConsole::ClearObjectAttrs(void)
{
    if (m_objnum == NOTHING)
	return;

    // Set the attributes on the console object.
    hsInterface.AtrAdd(m_objnum, "HSDB_XYHEADING", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_ZHEADING", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_XYOFFSET", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_ZOFFSET", NULL, GOD);

    int idx;
    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	char tbuf[32];

	sprintf(tbuf, "HSDB_MSGTYPE_%d", idx);
	hsInterface.AtrAdd(m_objnum, tbuf, NULL, GOD);
    }

}

// Gets the owner of the console .. the dbref of the owner
dbref CHSConsole::GetOwner(void)
{
    return m_owner;
}

// Sets the owner of the console to a specific object in the game
void CHSConsole::SetOwner(dbref obj)
{
    m_owner = obj;

    // Set my owner attribute
    if (m_objnum != NOTHING)
	hsInterface.AtrAdd(m_objnum, "HSDB_OWNER",
			   unsafe_tprintf("#%d", obj), GOD);
}

// Loads information from a game object into the 
// console object in memory.
BOOL CHSConsole::LoadFromObject(dbref objnum)
{
    // Set our object number
    m_objnum = objnum;

    // Load attributes
    if (hsInterface.AtrGet(objnum, "HSDB_XYHEADING"))
	m_xyheading = atoi(hsInterface.m_buffer);
    if (hsInterface.AtrGet(objnum, "HSDB_ZHEADING"))
	m_zheading = atoi(hsInterface.m_buffer);
    if (hsInterface.AtrGet(objnum, "HSDB_XYOFFSET"))
	m_xyoffset = atoi(hsInterface.m_buffer);
    if (hsInterface.AtrGet(objnum, "HSDB_ZOFFSET"))
	m_zoffset = atoi(hsInterface.m_buffer);
    if (hsInterface.AtrGet(objnum, "HSDB_CAN_ROTATE"))
	m_can_rotate = atoi(hsInterface.m_buffer);

    // Any weapons to load?
    if (hsInterface.AtrGet(objnum, "HSDB_WEAPONS"))
	LoadWeapons(hsInterface.m_buffer);

    // Load weapon stats
    int idx;

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	char tbuf[32];

	sprintf(tbuf, "HSDB_MSGTYPE_%d", idx);

	if (hsInterface.AtrGet(m_objnum, tbuf))
	    AddMessage(atoi(hsInterface.m_buffer));
    }
    if (m_weapon_array) {
	char strWeaponID[32];
	CHSWeapon *cWeap;

	for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	    cWeap = m_weapon_array->GetWeapon(idx);
	    if (cWeap) {
		sprintf(strWeaponID, "WEAPON_STAT_%d", idx);
		if (hsInterface.AtrGet(m_objnum, strWeaponID))
		    cWeap->SetStatus(atoi(hsInterface.m_buffer));
	    }
	}
    }
    // Set the console flag
    hsInterface.SetFlag(objnum, THING_HSPACE_CONSOLE);

    ClearObjectAttrs();
    return TRUE;

}

// Call this function to have the console handle a message,
// which is usually given to the user of the console in some
// form.
void CHSConsole::HandleMessage(char *strMsg, int type)
{
    dbref dbUser;

    dbUser = hsInterface.ConsoleUser(m_objnum);
    if (dbUser == NOTHING)
	return;

    // Is the console online?
    if (!IsOnline())
	return;

    if (GetMessage(type))
	notify(dbUser, strMsg);
}

// Returns the power needs for the console, which includes
// a base amount plus any weapon needs.
UINT CHSConsole::GetMaximumPower(void)
{
    UINT uPower;

    uPower = 2;			// Console requires at least 2 MW of power

    // Figure in weapon needs
    if (m_weapon_array) {
	uPower += m_weapon_array->GetTotalPower();
    }

    return uPower;
}

// Can be used to tell the console to adjust its heading, for
// example, when the ship it belongs to turns.
void CHSConsole::AdjustHeading(int iXY, int iZ)
{
    m_xyheading += iXY;
    m_zheading += iZ;

    if (m_xyheading > 359)
	m_xyheading -= 360;
    else if (m_xyheading < 0)
	m_xyheading += 360;
}

// Returns a character string containing the value of
// the requested console attribute.
char *CHSConsole::GetAttributeValue(char *strName)
{
    static char rval[64];
    char tmp[32];
    int idx;

    *rval = '\0';
    if (!strcasecmp(strName, "CXYHEADING")) {
	sprintf(rval, "%d", m_xyheading);
    } else if (!strcasecmp(strName, "CZHEADING")) {
	sprintf(rval, "%d", m_zheading);
    } else if (!strcasecmp(strName, "USER")) {
	sprintf(rval, "#%d", hsInterface.ConsoleUser(m_objnum));
    } else if (!strcasecmp(strName, "WEAPONS")) {
	CHSWeapon *cWeap;

	// If we have a weapons array, run through
	// the weapons, pulling out their types.

	if (m_weapon_array) {
	    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
		cWeap = m_weapon_array->GetWeapon(idx);
		if (!cWeap)
		    continue;

		// Add the class of the weapon to the list
		if (!*rval)
		    sprintf(tmp, "%d", cWeap->m_class);
		else
		    sprintf(tmp, " %d", cWeap->m_class);

		strcat(rval, tmp);
	    }
	}
    } else if (!strcasecmp(strName, "MESSAGES")) {
	for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	    if (m_msgtypes[idx] == NOTHING)
		continue;

	    // Add the class of the weapon to the list
	    if (!*rval)
		sprintf(tmp, "%d", m_msgtypes[idx]);
	    else
		sprintf(tmp, " %d", m_msgtypes[idx]);

	    strcat(rval, tmp);
	}
    } else if (!strcasecmp(strName, "XYOFFSET")) {
	sprintf(rval, "%d", m_xyoffset);
    } else if (!strcasecmp(strName, "ZOFFSET")) {
	sprintf(rval, "%d", m_zoffset);
    } else if (!strcasecmp(strName, "CAN ROTATE")) {
	sprintf(rval, "%d", m_can_rotate ? 1 : 0);
    } else if (!strcasecmp(strName, "LOCK")) {
	// Are we locked?
	if (!m_target_lock)
	    return "#-1";

	CHSSysSensors *cSensors;

	// Get the sensors from our owner
	cSensors = (CHSSysSensors *) m_ownerObj->GetEngSystem(HSS_SENSORS);
	if (!cSensors)
	    return "";

	SENSOR_CONTACT *cContact;
	cContact = cSensors->GetContact(m_target_lock);
	if (!cContact)
	    return "";

	sprintf(rval, "%d", cContact->m_id);
    } else if (!strcasecmp(strName, "ISPOWERED")) {
	sprintf(rval, "%d", IsOnline());
    } else
	return NULL;

    return rval;
}

// Attempts to set an attribute with a value for the
// console.
BOOL CHSConsole::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Find the attribute name, set the value.
    if (!strcasecmp(strName, "XYHEADING")) {
	iVal = atoi(strValue);
	if (iVal < 0 || iVal > 359)
	    return FALSE;
	m_xyheading = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "ZHEADING")) {
	iVal = atoi(strValue);
	if (iVal > 90 || iVal < -90)
	    return FALSE;
	m_zheading = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "XYOFFSET")) {
	iVal = atoi(strValue);
	if (iVal < 0 || iVal > 359)
	    return FALSE;
	m_xyoffset = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "CAN ROTATE")) {
	m_can_rotate = atoi(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "ZOFFSET")) {
	iVal = atoi(strValue);
	if (iVal < -90 || iVal > 90)
	    return FALSE;
	m_zoffset = iVal;
	return TRUE;
    }
    return FALSE;
}

// Sets the CHSObject pointer for the object that this
// console belongs to.
void CHSConsole::SetOwnerObj(CHSObject * cObj)
{
    m_ownerObj = cObj;
}

// Tries to lock the console onto a target ID.  This is only
// possible if the console belongs to a certain object type
// which has sensors.
void CHSConsole::LockTargetID(dbref player, int ID)
{
    UINT uMaxRange;
    double sX, sY, sZ;		// Our owner's coords.
    double tX, tY, tZ;		// Target's coords
    double dDistance;

    // Do we have an owner object?
    if (!m_ownerObj) {
	hsStdError(player,
		   "This console doesn't belong to an HSpace object.");
	return;
    }
    // Does the object type support sensors?
    if (m_ownerObj->GetType() != HST_SHIP) {
	hsStdError(player,
		   "Only consoles on space vessels support that command.");
	return;
    }
    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Try to find the sensor contact for the vessel.
    CHSShip *cShip;
    SENSOR_CONTACT *cContact;

    cShip = (CHSShip *) m_ownerObj;
    cContact = cShip->GetSensorContact(ID);
    if (!cContact) {
	hsStdError(player, "No such contact ID on sensors.");
	return;
    }
    // Grab the CHSObject pointer from the sensor contact
    CHSObject *cTarget;
    cTarget = cContact->m_obj;

    // Find max weapon range
    uMaxRange = m_weapon_array->GetMaxRange();

    // Calculate distance to target
    sX = cShip->GetX();
    sY = cShip->GetY();
    sZ = cShip->GetZ();
    tX = cTarget->GetX();
    tY = cTarget->GetY();
    tZ = cTarget->GetZ();
    dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ);

    // Distance greater than max weapon range?
    if (dDistance > uMaxRange) {
	hsStdError(player, "Target is out of our max weapons range.");
	return;
    }
    // Already locked onto that target?
    if (m_target_lock) {
	if (m_target_lock->GetDbref() == cTarget->GetDbref()) {
	    hsStdError(player,
		       "Weapons are already locked onto that target.");
	    return;
	}
    }
    // Lock onto the object
    CHSObject *cPrevLock;
    cPrevLock = m_target_lock;
    m_target_lock = cTarget;

    // Tell the old target we've unlocked?
    if (cPrevLock)
	cPrevLock->HandleLock(m_ownerObj, FALSE);

    // Tell the new target we've locked.
    cTarget->HandleLock(m_ownerObj, TRUE);

    hsStdError(player, "Weapons now locked on specified target contact.");
}

// Returns the weapons array object for the console
CHSWeaponArray *CHSConsole::GetWeaponArray(void)
{
    return m_weapon_array;
}

// Gives the gunnery readout report for the console.
void CHSConsole::GiveGunneryReport(dbref player)
{
    int idx;
    CHSWeapon *cWeap;
    char tbuf[256];

    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Print out the header.
    sprintf(tbuf,
	    "%s%s.-----------------------------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Console Weaponry Report            %39s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    m_ownerObj->GetName(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >---------------------------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Print weapon info header
    sprintf(tbuf,
	    "%s%s| %s[%s%sID%s] Name                         Status         Weapon Attributes          %s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Run down the weapons in the array, calling on them for
    // information.
    char lpstrName[64];
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	cWeap = m_weapon_array->GetWeapon(idx);
	if (!cWeap)
	    continue;

	// Copy in the weapon name, and truncate it
	strcpy(lpstrName, cWeap->GetName());
	lpstrName[28] = '\0';

	// Print weapon info.  It's up to the weapon to
	// give us most of the info.  The ID of the weapon
	// is array notation + 1.
	sprintf(tbuf,
		"%s%s|%s [%2d] %-29s%-15s%-27s%s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		idx + 1, lpstrName, cWeap->GetStatus(),
		cWeap->GetAttrInfo(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }

    // Give autoload status
    sprintf(tbuf,
	    "%s%s| %75s |%s", ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sAutoloading:%s %-3s    %s%sAutorotate:%s %-3s %40s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_autoload ? "ON" : "OFF",
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    m_autorotate ? "ON" : "OFF", " ",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // If there's a missile bay, give info for that
    if (m_missile_bay) {
	HSMISSILESTORE *mStore;
	int nStores = 0;
	BOOL bHeaderPrinted = FALSE;
	CHSDBWeapon *cDBWeap;


	// Print out storage info
	for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	    mStore = m_missile_bay->GetStorage(idx);
	    if (!mStore)
		continue;

	    // Has the header been printed?
	    if (!bHeaderPrinted) {
		bHeaderPrinted = TRUE;
		// Print missile bay header info
		sprintf(tbuf,
			"%s%s >---------------------------------------------------------------------------<%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
		notify(player, tbuf);
		sprintf(tbuf,
			"%s%s|%s                       %s+%s- Munitions Storage -%s%s+                               %s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
		notify(player, tbuf);
		sprintf(tbuf,
			"%s%s|%77s|%s",
			ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL);
		notify(player, tbuf);
		sprintf(tbuf,
			"%s%s| %s[%s%sID%s] Munitions Type                    Max      Remaining%19s%s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
			ANSI_HILITE, ANSI_GREEN, " ", ANSI_BLUE,
			ANSI_NORMAL);
		notify(player, tbuf);

	    }
	    // Give the information
	    cDBWeap = waWeapons.GetWeapon(mStore->m_class, HSW_NOTYPE);
	    sprintf(tbuf, "%s%s|%s [%2d] %-34s%2d          %2d%23s%s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		    idx + 1, cDBWeap->m_name, mStore->m_maximum,
		    mStore->m_remaining, " ",
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	    notify(player, tbuf);
	}
    }
    // Finish off the report.
    sprintf(tbuf,
	    "%s%s`-----------------------------------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}

// Allows a player (admin) to delete the specified weapon slot
// from the console.  The gunnery report lists weapons from 1 .. n,
// so we expect the same here.  However, weapon slots are from 
// 0 .. n, so that adjustment is made.
void CHSConsole::DeleteWeapon(dbref player, int slot)
{
    // Do we have a weapons array?
    if (!m_weapon_array) {
	notify(player, "This console is not equipped with weaponry.");
	return;
    }
    // Decrement slot by 1.
    slot--;
    if (slot < 0)
	slot = 0;

    // Check to see if the weapon exists.
    if (!m_weapon_array->GetWeapon(slot)) {
	notify(player, "That weapon slot does not exist on that console.");
	return;
    }

    if (!m_weapon_array->DeleteWeapon(slot)) {
	notify(player, "Failed to delete the weapon in that slot.");
	return;
    }

    notify(player, "Weapon deleted.");

    // Check to see if there is a weapon in slot 0.  If not,
    // then there are no weapons left, and we can delete the
    // weapons array.
    if (!m_weapon_array->GetWeapon(0)) {
	delete m_weapon_array;
	m_weapon_array = NULL;
    }
}

// Allows a player (admin) to add a weapon of a certain type
// to the console.  All error checking is performed.
BOOL CHSConsole::AddWeapon(dbref player, int type)
{
    // Valid weapon type?
    if (!waWeapons.GoodWeapon(type)) {
	notify(player, "Invalid weapon type specified.");
	return FALSE;
    }
    // Do we have a weapons array?
    if (!m_weapon_array) {
	m_weapon_array = new CHSWeaponArray;

	// Tell the console array of our missile bay
	m_weapon_array->SetMissileBay(m_missile_bay);
    }

    if (!m_weapon_array->NewWeapon(type))
	notify(player, "Failed to add specified weapon type to console.");
    else
	notify(player,
	       unsafe_tprintf("Weapon type %d added to console.", type));

    return TRUE;
}

// Tells the console where the missile bay is for its weapons
// to grab missiles from.
void CHSConsole::SetMissileBay(CHSMissileBay * mBay)
{
    m_missile_bay = mBay;

    if (m_weapon_array)
	m_weapon_array->SetMissileBay(mBay);
}

// Configures a weapon so that it takes on the attributes
// of a specified weapon, usually a missile.
void CHSConsole::ConfigureWeapon(dbref player, int weapon, int type)
{
    CHSWeapon *cWeap;

    // Weapon and type are specified from 1 .. n, but weapons
    // are stored as 0 .. n-1, so decrement.
    type--;
    weapon--;

    // Do we have any weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Does the weapon exist?
    cWeap = m_weapon_array->GetWeapon(weapon);
    if (!cWeap) {
	hsStdError(player, "Invalid weapon ID specified.");
	return;
    }
    // Does the weapon support configuration?
    if (!cWeap->Configurable()) {
	hsStdError(player, "That weapon type cannot be configured.");
	return;
    }
    // Do we have the type in storage?
    if (!m_missile_bay) {
	hsStdError(player, "No munitions storage available.");
	return;
    }
    // Grab the storage slot.
    HSMISSILESTORE *mStore;
    mStore = m_missile_bay->GetStorage(type);
    if (!mStore) {
	hsStdError(player,
		   unsafe_tprintf("Invalid munitions ID (%d) specified.",
				  type + 1));
	return;
    }
    // Try to configure it.
    if (!cWeap->Configure(mStore->m_class)) {
	hsStdError(player,
		   "Failed to configure weapon to specified munitions type.");
	return;
    }
    // All went ok.
    hsStdError(player, "Weapon configured to specified munitions type.");
}

// Allows a player to load a specific weapon.
void CHSConsole::LoadWeapon(dbref player, int weapon)
{
    CHSWeapon *cWeap;

    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Find the weapon.  Decrement weapon variable to go
    // to array notation.
    weapon--;
    cWeap = m_weapon_array->GetWeapon(weapon);
    if (!cWeap) {
	hsStdError(player, "Invalid weapon ID specified.");
	return;
    }
    // Is the weapon loadable?
    if (!cWeap->Loadable()) {
	hsStdError(player, "That weapon cannot be loaded.");
	return;
    }
    // Is it already reloading
    if (cWeap->Reloading()) {
	hsStdError(player, "That weapon is already currently loading.");
	return;
    }
    // Try to load it.
    if (!cWeap->Reload()) {
	hsStdError(player, "Unable to reload that weapon at this time.");
	return;
    }
    // Went ok.
    hsStdError(player, "Reloading weapon ...");
}

// Handles cyclic stuff for consoles
void CHSConsole::DoCycle(void)
{
    dbref dbUser;
    dbUser = hsInterface.ConsoleUser(m_objnum);

    // Verify that the computer has enough power to
    // power us.
    int need;
    need = GetMaximumPower();

    // Check target lock.
    if (m_target_lock && m_target_lock != NULL) {
	BOOL bUnlock = FALSE;

	// Target still active?
	if (!m_target_lock->IsActive())
	    bUnlock = TRUE;

	// Find max weapon range
	UINT uMaxRange;
	if (m_weapon_array)
	    uMaxRange = m_weapon_array->GetMaxRange();
	else
	    uMaxRange = 0;

	// Calculate distance to target
	double sX, sY, sZ;
	double tX, tY, tZ;
	sX = m_ownerObj->GetX();
	sY = m_ownerObj->GetY();
	sZ = m_ownerObj->GetZ();
	tX = m_target_lock->GetX();
	tY = m_target_lock->GetY();
	tZ = m_target_lock->GetZ();

	double dDistance;
	dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ);

	// Distance greater than max weapon range?
	if (dDistance > uMaxRange)
	    bUnlock = TRUE;


	// Is our owner still active?
	if (!m_ownerObj->IsActive())
	    bUnlock = TRUE;

	// Unlock weapons?
	if (bUnlock) {
	    m_target_lock->HandleLock(m_ownerObj, FALSE);
	    m_target_lock = NULL;
	    if (dbUser != NOTHING)
		hsStdError(dbUser, "Target lock no longer capable.");
	}
    }

    if (!IsOnline())
	return;

    // If we have a weapons array, tell it to cycle.
    if (m_weapon_array) {
	m_weapon_array->DoCycle();

	// Indicate any weapons that just became ready;
	int idx;
	CHSWeapon *cWeap;
	char tbuf[64];

	// Find the console's user.
	if (dbUser != NOTHING) {
	    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
		if (cWeap = m_weapon_array->GetWeapon(idx)) {
		    if (cWeap->GetStatusChange() == STAT_READY) {
			if (cWeap->Loadable())
			    sprintf(tbuf,
				    "%s%s[%s%s%d%s]%s - Weapon loaded.",
				    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				    ANSI_HILITE, idx + 1, ANSI_GREEN,
				    ANSI_NORMAL);
			else
			    sprintf(tbuf,
				    "%s%s[%s%s%d%s]%s - Weapon ready.",
				    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				    ANSI_HILITE, idx + 1, ANSI_GREEN,
				    ANSI_NORMAL);
			notify(dbUser, tbuf);
		    }
		}
	    }
	}
    }
}

// Unlocks the weapons (if locked) from the locked
// target.
void CHSConsole::UnlockWeapons(dbref player)
{
    // Do we have an owner object?
    if (!m_ownerObj) {
	hsStdError(player,
		   "This console doesn't belong to an HSpace object.");
	return;
    }
    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Are we currently locked?
    if (!m_target_lock) {
	hsStdError(player,
		   "Weapons are not currently locked onto a target.");
	return;
    }
    // Tell the target we've unlocked
    m_target_lock->HandleLock(m_ownerObj, FALSE);

    m_target_lock = NULL;

    // Tell the player
    hsStdError(player, "Weapons now unlocked from previous target.");
}

// Changes the heading of the console (in this case, a turret)
// to a given angle.  Error checking is performed to ensure
// that it doesn't turn into the ship it's on.
void CHSConsole::ChangeHeading(dbref player, int iXY, int iZ)
{
    int iXYAngle;
    int iZAngle;
    double is, js, ks;		// Ship vector
    double ic, jc, kc;		// Turret vector;

    // Do we have an owner object?
    if (!m_ownerObj) {
	hsStdError(player,
		   "This console does not belong to any HSpace object.");
	return;
    }
    // Can the console rotate like a turret?
    if (!m_can_rotate) {
	hsStdError(player,
		   "This console does not have rotate capability.");
	return;
    }
    // Do we belong to a ship?
    if (m_ownerObj->GetType() != HST_SHIP) {
	hsStdError(player,
		   "This command only applies to consoles located on ships.");
	return;
    }
    CHSShip *cShip;
    cShip = (CHSShip *) m_ownerObj;

    // Do some error checking.
    if (iXY < 0)
	iXY = m_xyheading;
    else if (iXY > 359)
	iXY = 359;

    if (iZ < -90)
	iZ = m_zheading;
    else if (iZ > 90)
	iZ = 90;

    // The following code computes a normal vector for
    // the side of the ship we're located on.  We'll
    // then compute the normal vector for where the
    // console wants to turn to.  If the dot product
    // of the two vectors is negative, the console is
    // trying to turn into the ship.
    iXYAngle = cShip->GetXYHeading() + m_xyoffset;
    iZAngle = cShip->GetZHeading() + m_zoffset;

    // Make sure angles fall within specifications
    if (iXYAngle > 359)
	iXYAngle -= 360;
    else if (iXYAngle < 0)
	iXYAngle += 360;

    if (iZAngle < 0)
	iZAngle += 360;

    // Compute normal vector for the side of the ship
    // we're on.
    is = d2sin_table[iXYAngle] * d2cos_table[iZAngle];
    js = d2cos_table[iXYAngle] * d2cos_table[iZAngle];
    ks = d2sin_table[iZAngle];


    // Find normal vector for where we want to head.
    if (iZ < 0)
	iZ += 360;
    ic = d2sin_table[iXY] * d2cos_table[iZ];
    jc = d2cos_table[iXY] * d2cos_table[iZ];
    kc = d2sin_table[iZ];

    // Compute the dot product between the two vectors
    double dp;
    dp = (is * ic) + (js * jc) + (ks * kc);

    if (dp < 0) {
	hsStdError(player,
		   "This console cannot currently rotate to that angle.");
	return;
    }
    // Set the heading
    m_xyheading = iXY;

    if (iZ > 90)
	iZ -= 360;
    m_zheading = iZ;

    char tbuf[128];
    sprintf(tbuf, "Console heading changed to %d mark %d.", iXY, iZ);
    hsStdError(player, tbuf);
}

// Allows a player to fire a specified weapon number.
void CHSConsole::FireWeapon(dbref player, int weapon)
{
    // Weapons are specified from 1 .. n, but they're
    // stored in the array as 0 .. n-1, so decrement.
    weapon--;

    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Is it a valid weapon?
    CHSWeapon *cWeap;
    cWeap = m_weapon_array->GetWeapon(weapon);
    if (!cWeap) {
	hsStdError(player, "Invalid weapon ID specified.");
	return;
    }
    // Does the weapon require a target lock?
    if (cWeap->RequiresLock() && !m_target_lock) {
	hsStdError(player,
		   "You must be locked onto a target to fire that weapon.");
	return;
    }
    // Can the weapon attack that type of object?
    if (!cWeap->CanAttackObject(m_target_lock)) {
	hsStdError(player,
		   "You cannot attack that target with that weapon.");
	return;
    }
    // Is the weapon ready?
    if (!cWeap->IsReady()) {
	hsStdError(player, "That weapon is not ready for firing.");
	return;
    }
    // Do we try to autorotate to the target?
    if (m_target_lock && m_autorotate) {
	// Calculate the bearing to the target.
	int iXY, iZ;
	double sX, sY, sZ;	// Our coords
	double tX, tY, tZ;	// Target coords

	sX = m_ownerObj->GetX();
	sY = m_ownerObj->GetY();
	sZ = m_ownerObj->GetZ();

	tX = m_target_lock->GetX();
	tY = m_target_lock->GetY();
	tZ = m_target_lock->GetZ();

	iXY = XYAngle(sX, sY, tX, tY);
	iZ = ZAngle(sX, sY, sZ, tX, tY, tZ);

	ChangeHeading(player, iXY, iZ);
    }
    // Is the target in the cone?
    if (m_target_lock && !IsInCone(m_target_lock)) {
	hsStdError(player, "Target is not currently within firing cone.");
	return;
    }
    // Tell the weapon to fire.
    cWeap->AttackObject(m_ownerObj, m_target_lock, this, m_targetting);

    // Is the weapon loadable, and should we reload?
    if (m_autoload && cWeap->Loadable())
	cWeap->Reload();
}

// Indicates whether a given CHSObject is within the firing
// cone of the console.  To do this, we calculate the vector
// to the target, use the dot product to get the difference
// between that vector and the console heading vector, and
// then convert the dot product to an angle.  We use this
// angle to see if it's larger than the cone.
BOOL CHSConsole::IsInCone(CHSObject * cTarget)
{
    double ic, jc, kc;		// Console heading vector
    double it, jt, kt;		// Target vector
    int iXY, iZ;
    double tX, tY, tZ;		// Target coords.
    double sX, sY, sZ;		// Source (our) coords

    if (!cTarget)
	return FALSE;

    if (!m_ownerObj)
	return FALSE;

    sX = m_ownerObj->GetX();
    sY = m_ownerObj->GetY();
    sZ = m_ownerObj->GetZ();

    tX = cTarget->GetX();
    tY = cTarget->GetY();
    tZ = cTarget->GetZ();

    // Calculate the vector for where the console is
    // pointing.
    iZ = m_zheading;
    if (iZ < 0)
	iZ += 360;
    ic = d2sin_table[m_xyheading] * d2cos_table[iZ];
    jc = d2cos_table[m_xyheading] * d2cos_table[iZ];
    kc = d2sin_table[iZ];

    // Now calculate the vector to the target.
    iXY = XYAngle(sX, sY, tX, tY);
    if (iXY > 359)
	iXY -= 360;
    else if (iXY < 0)
	iXY += 360;
    iZ = ZAngle(sX, sY, sZ, tX, tY, tZ);
    if (iZ < 0)
	iZ += 360;

    it = d2sin_table[iXY] * d2cos_table[iZ];
    jt = d2cos_table[iXY] * d2cos_table[iZ];
    kt = d2sin_table[iZ];

    // Calculate the dot product of the two vectors.
    double dp;
    dp = (ic * it) + (jc * jt) + (kc * kt);
    if (dp > 1)
	dp = 1;
    else if (dp < -1)
	dp = -1;

    // Determine the angular difference between the vectors
    // based on the dot product.
    double diff;
    diff = acos(dp) * RADTODEG;

    // If the absolute difference between the vectors is
    // greater than 1/2 the firing arc, it's out of the cone.
    if (diff < 0)
	diff *= -1;
    if (diff > (m_arc * .5))
	return FALSE;
    return TRUE;
}

// Returns the current XYHeading of the console
int CHSConsole::GetXYHeading(void)
{
    return m_xyheading;
}

// Returns the ZHeading of the console
int CHSConsole::GetZHeading(void)
{
    return m_zheading;
}

// Allows a player to power up the console
void CHSConsole::PowerUp(dbref player)
{
    int maxpower;

    // Are we already online?
    if (IsOnline()) {
	hsStdError(player, "This console is already online.");
	return;
    }
    // Does the computer have enough power?
    if (m_computer) {
	maxpower = GetMaximumPower();
	if (!m_computer->DrainPower(maxpower)) {
	    hsStdError(player,
		       "Insufficient computer power to power console.");
	    return;
	}
    }
    // Bring it online
    m_online = TRUE;

    hsStdError(player, unsafe_tprintf("%s now online.", Name(m_objnum)));
}

// Powers down the console, releasing power to the computer
void CHSConsole::PowerDown(dbref player)
{
    int maxpower;

    if (!IsOnline()) {
	hsStdError(player, "This console is not currently online.");
	return;
    }
    // Do we have a computer?
    if (m_computer) {
	maxpower = GetMaximumPower();
	m_computer->ReleasePower(maxpower);
	m_online = FALSE;
	hsStdError(player,
		   unsafe_tprintf("%s powered down.", Name(m_objnum)));
    } else
	hsStdError(player, "That's not necessary with this console.");
}

// Gives a quick, target report for when the weapons are
// locked onto a target.
void CHSConsole::GiveTargetReport(dbref player)
{
    SENSOR_CONTACT *cContact;

    // Are we locked onto a target?
    if (!m_target_lock) {
	hsStdError(player,
		   "Weapons are not currently locked onto a target.");
	return;
    }
    // Find the contact in the owner's sensor array
    CHSShip *cShip;
    cShip = (CHSShip *) m_ownerObj;
    cContact = cShip->GetSensorContact(m_target_lock);

    char tbuf[256];		// For printing info

    double sX, sY, sZ;		// Owner's coords
    double tX, tY, tZ;		// Target's coords

    sX = m_ownerObj->GetX();
    sY = m_ownerObj->GetY();
    sZ = m_ownerObj->GetZ();

    tX = m_target_lock->GetX();
    tY = m_target_lock->GetY();
    tZ = m_target_lock->GetZ();

    // Calculate distance to target
    double dDistance;
    dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ);

    // Print the header info
    sprintf(tbuf,
	    "%s%s.----------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s|%s Target Info Report                              ID: %-4d %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    cContact ? cContact->m_id : -1,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s >--------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Is the target in the firing cone?
    if (IsInCone(m_target_lock)) {
	sprintf(tbuf,
		"%s%s|%s                       %s%s* IN CONE *                        %s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		ANSI_HILITE, ANSI_RED, ANSI_BLUE, ANSI_NORMAL);
    } else
	sprintf(tbuf,
		"%s%s| %56s |%s",
		ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s|                  ______________________                  |%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Before printing the rest of the display, calculate
    // which vertical line and horizontal slot the indicators
    // are going into.
    double dx, dy, dz;

    // Determine vector from us to target
    dx = (int) ((tX - sX) / dDistance);
    dy = (int) ((tY - sY) / dDistance);
    dz = (int) ((tZ - sZ) / dDistance);
    CHSVector tVec(dx, dy, dz);

    // Determine our heading vector for the console
    int iZAngle = m_zheading;
    if (iZAngle < 0)
	iZAngle += 360;

    dx = d2sin_table[m_xyheading] * d2cos_table[iZAngle];
    dy = d2cos_table[m_xyheading] * d2cos_table[iZAngle];
    dz = d2sin_table[iZAngle];
    CHSVector hVec(dx, dy, dz);

    // Calculate dot product between both vectors
    double dp;
    dp = hVec.DotProduct(tVec);

    // Determine target bearing
    int iXYAngle;
    iZAngle = ZAngle(sX, sY, sZ, tX, tY, tZ);
    iXYAngle = XYAngle(sX, sY, tX, tY);

    // If the dot product is positive, the target is
    // on our visual screen.
    int iVertLine = -1;
    int iHorzSlot = -1;
    if (dp > 0) {

	// Determine which vertical line the target falls into
	iVertLine = ((iZAngle - m_zheading) - 90) * .05 * -1;

	// Determine which horizontal slot it goes into
	iHorzSlot = ((iXYAngle - m_xyheading) + 90) * .166667;
    }

    char tbuf2[32];		// Used for printing out the target

    strcpy(tbuf2, "                        ");
    if (iVertLine == 0)
	tbuf2[iHorzSlot - 3] = 'X';
    sprintf(tbuf,
	    "%s%s|                /%s%s%s\\                |%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
	    tbuf2, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    strcpy(tbuf2, "                            ");
    if (iVertLine == 1)
	tbuf2[iHorzSlot - 1] = 'X';
    sprintf(tbuf,
	    "%s%s|              /%s%s%s\\              |%s",
	    ANSI_HILITE, ANSI_BLUE,
	    ANSI_RED, tbuf2, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    char lbuf[32];		// Used for printing out a line in pieces
    char rbuf[32];

    if (iVertLine == 2) {
	strcpy(tbuf2, "               -              ");
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7) {
	    // Just print everything in red
	    tbuf2[iHorzSlot] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sYou:        %s|%s%s%s| %sTarget:     %s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN,
		    ANSI_BLUE, ANSI_RED,
		    tbuf2, ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
	} else {
	    // Target red, crosshairs white.  We have
	    // to split the line up into three pieces
	    strcpy(lbuf, "           ");
	    strcpy(rbuf, "          ");
	    if (iHorzSlot < 11)
		lbuf[iHorzSlot] = 'X';
	    else
		rbuf[iHorzSlot - 20] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sYou:        %s|%s%s%s    -    %s%s%s| %sTarget:     %s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN,
		    ANSI_BLUE, ANSI_RED,
		    lbuf,
		    ANSI_WHITE, ANSI_RED,
		    rbuf, ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
	}
    } else {
	// Print all in red or all in white
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7)
	    sprintf(tbuf,
		    "%s%s| %sYou:        %s|%s               -              %s| %sTarget:     %s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN,
		    ANSI_BLUE, ANSI_RED,
		    ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
	else
	    sprintf(tbuf,
		    "%s%s| %sYou:        %s|%s               -              %s| %sTarget:     %s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN,
		    ANSI_BLUE, ANSI_WHITE,
		    ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
    }
    notify(player, tbuf);

    // Figure out the heading of the target.  This code
    // is fairly slow, but we have no choice.
    CHSVector thVec = m_target_lock->GetMotionVector();
    int tXYHeading;
    int tZHeading;

    tXYHeading = atan(thVec.i() / thVec.j()) * RADTODEG;
    if (tXYHeading < 0)
	tXYHeading += 360;
    tZHeading = asin(thVec.k()) * RADTODEG;

    if (iVertLine == 3) {
	strcpy(tbuf2, "              ---             ");
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7) {
	    // Just print everything in red
	    tbuf2[iHorzSlot] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sH:%s %3dm%-3d  %s%s|%s%s%s| %sH:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_xyheading, m_zheading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    tbuf2,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    tXYHeading, tZHeading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	} else {
	    // Target red, crosshairs white.  We have
	    // to split the line up into three pieces
	    strcpy(lbuf, "           ");
	    strcpy(rbuf, "          ");
	    if (iHorzSlot < 11)
		lbuf[iHorzSlot] = 'X';
	    else
		rbuf[iHorzSlot - 20] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sH:%s %3dm%-3d  %s%s|%s%s%s   ---   %s%s%s| %sH:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_xyheading, m_zheading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    lbuf,
		    ANSI_WHITE, ANSI_RED,
		    rbuf, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    tXYHeading, tZHeading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	}
    } else {
	// Print all in red or all in white
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7)
	    sprintf(tbuf,
		    "%s%s| %sH:%s %3dm%-3d  %s%s|%s              ---             %s| %sH:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_xyheading, m_zheading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    tXYHeading, tZHeading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	else
	    sprintf(tbuf,
		    "%s%s| %sH:%s %3dm%-3d  %s%s|%s              ---             %s| %sH:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_xyheading, m_zheading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    tXYHeading, tZHeading,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    }
    notify(player, tbuf);

    if (iVertLine == 4) {
	strcpy(tbuf2, "           ( ( + ) )          ");
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7) {
	    // Just print everything in red
	    tbuf2[iHorzSlot] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sV:%s %-7d  %s%s|%s%s%s| %sV:%s %-7d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_ownerObj->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    tbuf2,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_target_lock->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	} else {
	    // Target red, crosshairs white.  We have
	    // to split the line up into three pieces
	    strcpy(lbuf, "           ");
	    strcpy(rbuf, "          ");
	    if (iHorzSlot < 11)
		lbuf[iHorzSlot] = 'X';
	    else
		rbuf[iHorzSlot - 20] = 'X';
	    sprintf(tbuf,
		    "%s%s| %sV:%s %-7d  %s%s|%s%s%s( ( + ) )%s%s%s| %sV:%s %-7d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_ownerObj->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    lbuf,
		    ANSI_WHITE, ANSI_RED,
		    rbuf, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_target_lock->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	}
    } else {
	// Print all in red or all in white
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7)
	    sprintf(tbuf,
		    "%s%s| %sV:%s %-7d  %s%s|%s           ( ( + ) )          %s| %sV:%s %-7d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_ownerObj->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_target_lock->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	else
	    sprintf(tbuf,
		    "%s%s| %sV:%s %-7d  %s%s|%s           ( ( + ) )          %s| %sV:%s %-7d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_ownerObj->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    m_target_lock->GetSpeed(),
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    }
    notify(player, tbuf);

    if (iVertLine == 5) {
	strcpy(tbuf2, "              ___             ");
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7) {
	    // Just print everything in red
	    tbuf2[iHorzSlot] = 'X';
	    sprintf(tbuf,
		    "%s%s|             |%s%s%s| %sR:%s %-5.0f    %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE,
		    ANSI_RED,
		    tbuf2,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    dDistance, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	} else {
	    // Target red, crosshairs white.  We have
	    // to split the line up into three pieces
	    strcpy(lbuf, "           ");
	    strcpy(rbuf, "          ");
	    if (iHorzSlot < 11)
		lbuf[iHorzSlot] = 'X';
	    else
		rbuf[iHorzSlot - 20] = 'X';
	    sprintf(tbuf,
		    "%s%s|             |%s%s%s   ___   %s%s%s| %sR:%s %-5.0f    %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    lbuf,
		    ANSI_WHITE, ANSI_RED,
		    rbuf, ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    dDistance, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	}
    } else {
	// Print all in red or all in white
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7)
	    sprintf(tbuf,
		    "%s%s|             |%s              ___             %s| %sR:%s %-5.0f    %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    dDistance, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	else
	    sprintf(tbuf,
		    "%s%s|             |%s              ___             %s| %sR:%s %-5.0f    %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    dDistance, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    }
    notify(player, tbuf);

    if (iVertLine == 6) {
	strcpy(tbuf2, "               _              ");
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7) {
	    // Just print everything in red
	    tbuf2[iHorzSlot] = 'X';
	    sprintf(tbuf,
		    "%s%s|             |%s%s%s| %sB:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE,
		    ANSI_RED,
		    tbuf2,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    iXYAngle, iZAngle,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	} else {
	    // Target red, crosshairs white.  We have
	    // to split the line up into three pieces
	    strcpy(lbuf, "           ");
	    strcpy(rbuf, "          ");
	    if (iHorzSlot < 11)
		lbuf[iHorzSlot] = 'X';
	    else
		rbuf[iHorzSlot - 20] = 'X';
	    sprintf(tbuf,
		    "%s%s|             |%s%s%s    _    %s%s%s| %sB:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    lbuf,
		    ANSI_WHITE, ANSI_RED,
		    rbuf,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    iXYAngle, iZAngle,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	}
    } else {
	// Print all in red or all in white
	if (iHorzSlot > 10 && iHorzSlot < 20 &&
	    iVertLine > 1 && iVertLine < 7)
	    sprintf(tbuf,
		    "%s%s|             |%s               _              %s| %sB:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_RED,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    iXYAngle, iZAngle,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	else
	    sprintf(tbuf,
		    "%s%s|             |%s               _              %s| %sB:%s %3dm%-3d  %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE,
		    ANSI_BLUE, ANSI_WHITE, ANSI_NORMAL,
		    iXYAngle, iZAngle,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    }
    notify(player, tbuf);

    strcpy(tbuf2, "                              ");
    if (iVertLine == 7)
	tbuf2[iHorzSlot] = 'X';
    sprintf(tbuf,
	    "%s%s|             |%s%s%s|             |%s",
	    ANSI_HILITE, ANSI_BLUE,
	    ANSI_RED, tbuf2, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    strcpy(tbuf2, "                            ");
    if (iVertLine == 8)
	tbuf2[iHorzSlot - 1] = 'X';
    sprintf(tbuf,
	    "%s%s|              \\%s%s%s/              |%s",
	    ANSI_HILITE, ANSI_BLUE,
	    ANSI_RED, tbuf2, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s`---------------\\\\.______________________.//---------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}

// Indicates whether the console is on a given frequency.
BOOL CHSConsole::OnFrq(double frq)
{
    // Check for the COMM_FRQS attr.
    if (!hsInterface.AtrGet(m_objnum, "COMM_FRQS"))
	return FALSE;

    // Separate the string into blocks of spaces
    char *ptr;
    char *bptr;			// Points to the first char in the frq string
    double cfrq;
    ptr = strchr(hsInterface.m_buffer, ' ');
    bptr = hsInterface.m_buffer;
    while (ptr) {
	*ptr = '\0';
	cfrq = atof(bptr);

	// Do we have a match, Johnny?
	if (cfrq == frq)
	    return TRUE;

	// Find the next frq in the string.
	bptr = ptr + 1;
	ptr = strchr(bptr, ' ');
    }
    // Last frq in the string.
    cfrq = atof(bptr);
    if (cfrq == frq)
	return TRUE;

    // No match!
    return FALSE;
}

// Attempts to unload a given weapon, if that weapon is
// a loadable type of weapon.
void CHSConsole::UnloadWeapon(dbref player, int weapon)
{
    // Do we have any weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry");
	return;
    }
    // Find the weapon.
    weapon--;
    CHSWeapon *cWeap;
    cWeap = m_weapon_array->GetWeapon(weapon);
    if (!cWeap) {
	hsStdError(player, "Invalid weapon ID specified.");
	return;
    }
    // Is the weapon loadable and, therefore, unloadable?
    if (!cWeap->Loadable()) {
	hsStdError(player, "That weapon cannot be unloaded.");
	return;
    }

    if (!cWeap->Unload())
	hsStdError(player, "Failed to unload specified weapon.");
    else
	hsStdError(player, "Weapon unloaded.");
}

// Sets the autoloading status for the console.  This is a
// level higher than letting the weapon handle it.  The console
// tries to reload any loadable weapons when they're fired.
void CHSConsole::SetAutoload(dbref player, BOOL bStat)
{
    if (!bStat) {
	m_autoload = FALSE;
	hsStdError(player, "Autoloading deactivated.");
    } else {
	m_autoload = TRUE;
	hsStdError(player, "Autoloading activated.");
    }
}

// Sets the autorotate status for the console.  If activated,
// the turret automatically tries to rotate to the target
// when the fire command is issued.
void CHSConsole::SetAutoRotate(dbref player, BOOL bStat)
{
    if (!bStat) {
	m_autorotate = FALSE;
	hsStdError(player, "Autorotate deactivated.");
    } else {
	m_autorotate = TRUE;
	hsStdError(player, "Autorotate activated.");
    }
}

// Sets the system that the player wishes to target when
// firing a targetable weapon.
void CHSConsole::SetSystemTarget(dbref player, int type)
{
    char *ptr;

    // Do we have weapons?
    if (!m_weapon_array) {
	hsStdError(player, "This console is not equipped with weaponry.");
	return;
    }
    // Are weapons locked?
    if (!m_target_lock) {
	hsStdError(player,
		   "Weapons are not currently locked on a target.");
	return;
    }

    if (type == HSS_NOTYPE) {
	m_targetting = HSS_NOTYPE;
	hsStdError(player, "System targetting disabled.");
    } else {
	// Find the system name.
	ptr = hsGetEngSystemName(type);

	m_targetting = type;

	if (!ptr)
	    hsStdError(player,
		       "Targetting unknown system on enemy target.");
	else
	    hsStdError(player,
		       unsafe_tprintf
		       ("Weapons now targeting %s system on enemy target.",
			ptr));
    }
}

// Returns a CHSVector object based on the current heading.
CHSVector & CHSConsole::GetHeadingVector(void)
{
    double i, j, k;
    int zang;

    zang = m_zheading;
    if (zang < 0)
	zang += 360;

    // Calculate coefficients
    i = d2sin_table[m_xyheading] * d2cos_table[zang];
    j = d2cos_table[m_xyheading] * d2cos_table[zang];
    k = d2sin_table[zang];

    static CHSVector tvec(i, j, k);
    return tvec;
}

// Returns the current target lock.
CHSObject *CHSConsole::GetObjectLock(void)
{
    return m_target_lock;
}

BOOL CHSConsole::GetMessage(int msgType)
{
    int idx;
    BOOL FoundType = FALSE;

    if (msgType == MSG_GENERAL)
	return TRUE;

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	if (m_msgtypes[idx] == NOTHING)
	    continue;
	else
	    FoundType = TRUE;

	if (m_msgtypes[idx] == msgType)
	    return TRUE;
    }

    if (FoundType)
	return FALSE;
    else
	return TRUE;
}

BOOL CHSConsole::AddMessage(int msgType)
{
    int idx;

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	if (m_msgtypes[idx] == NOTHING)
	    continue;

	if (m_msgtypes[idx] == msgType)
	    return FALSE;
    }

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	if (m_msgtypes[idx] == NOTHING)
	    break;
    }

    if (idx == HS_MAX_MESSAGE_TYPES)
	return FALSE;

    m_msgtypes[idx] = msgType;
    return TRUE;
}

BOOL CHSConsole::DelMessage(int msgType)
{
    int idx;

    for (idx = 0; idx < HS_MAX_MESSAGE_TYPES; idx++) {
	if (m_msgtypes[idx] == NOTHING)
	    continue;

	if (m_msgtypes[idx] == msgType) {
	    m_msgtypes[idx] = NOTHING;
	    return TRUE;
	}

    }

    return FALSE;
}
