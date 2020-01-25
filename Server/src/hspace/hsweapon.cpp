#include "hsuniverse.h"
#include "hscopyright.h"
#include "hsweapon.h"
#include "hsobjects.h"
#include "hsinterface.h"
#include "hspace.h"
#include "hsdb.h"
#include "ansi.h"

#include "config.h"

#include <math.h>

#ifdef I_STDLIB
#include <stdlib.h>
#endif

#ifdef I_STRING
#include <string.h>
#else
#include <strings.h>
#endif

#include "hsutils.h"
#include "hsmissile.h"

CHSWeaponDBArray waWeapons;	// One instance of this .. right here.


CHSDBWeapon::CHSDBWeapon(void)
{

}

// Returns the request attribute value, or NULL if the attribute
// name is not valid.
char *CHSDBWeapon::GetAttributeValue(char *strName)
{
    static char rval[64];

    *rval = '\0';
    if (!strcasecmp(strName, "NAME"))
	return m_name;
    else if (!strcasecmp(strName, "TYPE")) {
	sprintf(rval, "%d", m_type);
    } else
	return NULL;

    return rval;
}

// Attempts to set the value of the given attribute.  If successful, TRUE
// is returned, else FALSE.
BOOL CHSDBWeapon::SetAttributeValue(char *strName, char *strValue)
{
    // Try to match the name
    if (!strcasecmp(strName, "NAME")) {
	strncpy(m_name, strValue, HS_WEAPON_NAME_LEN);
	m_name[HS_WEAPON_NAME_LEN - 1] = '\0';
	return TRUE;
    }

    return FALSE;
}

CHSDBLaser::CHSDBLaser(void)
{
    m_regen_time = 1;
    m_range = 1;
    m_strength = 1;
    m_accuracy = 1;
    m_powerusage = 1;
    m_nohull = 0;
}

// Returns the request attribute value, or NULL if the attribute
// name is not valid.
char *CHSDBLaser::GetAttributeValue(char *strName)
{
    static char rval[64];

    *rval = '\0';
    if (!strcasecmp(strName, "REGEN TIME")) {
	sprintf(rval, "%d", m_regen_time);
    } else if (!strcasecmp(strName, "RANGE")) {
	sprintf(rval, "%d", m_range);
    } else if (!strcasecmp(strName, "STRENGTH")) {
	sprintf(rval, "%d", m_strength);
    } else if (!strcasecmp(strName, "ACCURACY")) {
	sprintf(rval, "%d", m_accuracy);
    } else if (!strcasecmp(strName, "POWER")) {
	sprintf(rval, "%d", m_powerusage);
    } else if (!strcasecmp(strName, "NOHULL")) {
	sprintf(rval, "%d", m_nohull);
    } else
	return CHSDBWeapon::GetAttributeValue(strName);

    return rval;
}

// Attempts to set the value of the given attribute.  If successful, TRUE
// is returned, else FALSE.
BOOL CHSDBLaser::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Try to match the name
    if (!strcasecmp(strName, "REGEN TIME")) {
	iVal = atoi(strValue);
	if (iVal < 0)
	    iVal = 0;

	m_regen_time = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "RANGE")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_range = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "STRENGTH")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_strength = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "ACCURACY")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_accuracy = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "POWER")) {
	iVal = atoi(strValue);
	if (iVal < 0)
	    iVal = 0;

	m_powerusage = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "NOHULL")) {
	iVal = atoi(strValue);
	if (iVal == 0 || iVal == 1) {
	    m_nohull = iVal;
	    return TRUE;
	}
	return FALSE;
    }

    return CHSDBWeapon::SetAttributeValue(strName, strValue);
}

CHSDBMissile::CHSDBMissile(void)
{
    m_reload_time = 1;
    m_range = 1;
    m_strength = 1;
    m_turnrate = 0;
    m_speed = 1;
}

// Returns the request attribute value, or NULL if the attribute
// name is not valid.
char *CHSDBMissile::GetAttributeValue(char *strName)
{
    static char rval[64];

    *rval = '\0';
    if (!strcasecmp(strName, "RELOAD TIME")) {
	sprintf(rval, "%d", m_reload_time);
    } else if (!strcasecmp(strName, "RANGE")) {
	sprintf(rval, "%d", m_range);
    } else if (!strcasecmp(strName, "STRENGTH")) {
	sprintf(rval, "%d", m_strength);
    } else if (!strcasecmp(strName, "TURN RATE")) {
	sprintf(rval, "%d", m_turnrate);
    } else if (!strcasecmp(strName, "SPEED")) {
	sprintf(rval, "%d", m_speed);
    } else
	return CHSDBWeapon::GetAttributeValue(strName);

    return rval;
}

// Attempts to set the value of the given attribute.  If successful, TRUE
// is returned, else FALSE.
BOOL CHSDBMissile::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Try to match the name
    if (!strcasecmp(strName, "RELOAD TIME")) {
	iVal = atoi(strValue);
	if (iVal < 0)
	    iVal = 0;

	m_reload_time = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "RANGE")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_range = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "STRENGTH")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_strength = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "TURN RATE")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_turnrate = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "SPEED")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    iVal = 1;

	m_speed = iVal;
	return TRUE;
    }

    return CHSDBWeapon::SetAttributeValue(strName, strValue);
}

CHSWeaponDBArray::CHSWeaponDBArray(void)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_WEAPONS; idx++)
	m_weapons[idx] = NULL;

    m_num_weapons = 0;
}

// Returns the HSW type of the weapon in a given slot
UINT CHSWeaponDBArray::GetType(int slot)
{
    if (!GoodWeapon(slot))
	return HSW_NOTYPE;

    return m_weapons[slot]->m_type;
}

// Prints info to a given player for all of the weapons
// in the database.
void CHSWeaponDBArray::PrintInfo(int player)
{
    int idx;
    char strType[32];

    notify(player, "[ID] Weapon Name                     Type");
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (!m_weapons[idx])
	    continue;

	switch (m_weapons[idx]->m_type) {
	case HSW_LASER:
	    strcpy(strType, "Laser");
	    break;

	case HSW_MISSILE:
	    strcpy(strType, "Missile");
	    break;

	default:
	    strcpy(strType, "Unknown");
	    break;
	}
	notify(player,
	       unsafe_tprintf("[%2d] %-32s%s",
			      idx, m_weapons[idx]->m_name, strType));
    }
}

// Gets the CSHDBWeapon structure from the WeaponDBArray,
// but first verifies that it is actually of the appropriate type.
CHSDBWeapon *CHSWeaponDBArray::GetWeapon(int slot, UINT type)
{
    if (!GoodWeapon(slot))
	return NULL;

    if ((type != HSW_NOTYPE) && (m_weapons[slot]->m_type != type))
	return NULL;

    return (m_weapons[slot]);
}

// Load the weapons database file, creating all of the weapons from it.
BOOL CHSWeaponDBArray::LoadFromFile(char *lpstrPath)
{
    FILE *fp;
    char tbuf[LBUF_SIZE];
    UINT idx;
    char *ptr;

    sprintf(tbuf, "LOADING: %s", lpstrPath);
    hs_log(tbuf);

    fp = fopen(lpstrPath, "r");
    if (!fp) {
	sprintf(tbuf, "ERROR: Couldn't open %s for reading.", lpstrPath);
	hs_log(tbuf);
	return FALSE;
    }
    // Ok, start loading lines from the file.
    idx = 0;
    while (fgets(tbuf, LBUF_SIZE - 1, fp)) {
	// Chop the newline
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	// Check for the end of the file.
	if (!strcasecmp(tbuf, "*END*"))
	    break;

	// Pass the string to the CreateWeapon() function to create
	// the weapon for us.
	if (!(m_weapons[idx] = CreateWeapon(tbuf))) {
	    sprintf(tbuf,
		    "ERROR: Invalid weapon specification at line %d.\n",
		    idx);
	    hs_log(tbuf);
	}
	idx++;
    }
    fclose(fp);

    m_weapons[idx] = NULL;
    m_num_weapons = idx;

    sprintf(tbuf, "LOADING: %s - %d weapons loaded (done)",
	    lpstrPath, m_num_weapons);
    hs_log(tbuf);

    return TRUE;
}

void CHSWeaponDBArray::SaveToFile(char *lpstrPath)
{
    UINT idx;
    FILE *fp;
    char tbuf[512];

    if (m_num_weapons <= 0)
	return;

    fp = fopen(lpstrPath, "w");
    if (!fp) {
	sprintf(tbuf, "ERROR: Unable to write weapons to %s.", lpstrPath);
	hs_log(tbuf);
	return;
    }

    for (idx = 0; idx < m_num_weapons; idx++) {
	if (m_weapons[idx]->m_type == HSW_LASER) {
	    WriteLaser((CHSDBLaser *) m_weapons[idx], fp);
	} else if (m_weapons[idx]->m_type == HSW_MISSILE) {
	    WriteMissile((CHSDBMissile *) m_weapons[idx], fp);
	}
    }
    fprintf(fp, "*END*\n");
    fclose(fp);
}

void CHSWeaponDBArray::WriteLaser(CHSDBLaser * weapon, FILE * fp)
{
    fprintf(fp, "0\"%s\" %d %d %d %d %d %d\n",
	    weapon->m_name, weapon->m_strength, weapon->m_regen_time,
	    weapon->m_range, weapon->m_accuracy, weapon->m_powerusage,
	    weapon->m_nohull);
}

void CHSWeaponDBArray::WriteMissile(CHSDBMissile * weapon, FILE * fp)
{
    fprintf(fp, "1\"%s\" %d %d %d %d %d\n",
	    weapon->m_name, weapon->m_strength, weapon->m_reload_time,
	    weapon->m_range, weapon->m_turnrate, weapon->m_speed);
}

// Creates a new weapon, based on the specified type.  If the
// type is invalid, or the weapon couldn't be created, NULL is
// returned.
CHSDBWeapon *CHSWeaponDBArray::NewWeapon(UINT type)
{
    // Do we have any space left in the array?
    int idx;
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (!m_weapons[idx])
	    break;
    }

    if (idx == HS_MAX_WEAPONS) {
	hs_log("WARNING: Maximum number of weapons reached.");
	return NULL;
    }
    // Determine type, allocate the new weapon.
    switch (type) {
    case HSW_LASER:
	m_weapons[idx] = new CHSDBLaser;
	break;

    case HSW_MISSILE:
	m_weapons[idx] = new CHSDBMissile;
	break;

	return NULL;
    }

    dbHSDB.m_nWeapons++;
    m_num_weapons++;
    m_weapons[idx]->m_type = type;
    return m_weapons[idx];
}

//
// CreateWeapon() simply takes a line from the weapons database and pulls out
// the information for the different weapons.  It then determines the type
// of weapon that should be created from the loaded string and calls the
// specific function for that type of weapon, passing to it the values from
// the database.
//
CHSDBWeapon *CHSWeaponDBArray::CreateWeapon(char *items)
{
    int iValArray[30];		// Gets loaded with field values
    char tbuf[32];		// Temporary string storage
    char strName[256];		// Gets loaded with the weapon name.
    UINT idx;
    UINT ok;
    UINT j;
    char *ptr;
    UINT type;

    ptr = items;

    // Grab the type of weapon, which is the first char.
    type = *ptr - '0';
    ptr++;

    // Next, we should have the name surrounded by quotes.
    if (*ptr != '"') {
	return NULL;
    }

    ptr++;

    idx = 0;
    while (*ptr && ((ptr - items) < HS_WEAPON_NAME_LEN) && (*ptr != '"')) {
	strName[idx++] = *ptr++;
    }
    strName[idx] = '\0';	// Got the weapon name!

    if (!*ptr) {
	return NULL;
    }
    // Increment past the quotes.
    if (*ptr == '"')
	ptr++;
    else {
	while (*ptr != '"')
	    ptr++;
	ptr++;
    }
    // We're finished with the name here.


    // Skip a white space
    ptr++;

    /*
     * Now start pulling out numbers and putting them in the array
     */
    ok = 1;
    j = 0;
    while (ok) {
	// Extract the value
	extract(ptr, tbuf, j, 1, ' ');

	// Check to see if we've reached the end of the string.
	if (!*tbuf)
	    break;

	// Shove the val into the array
	iValArray[j] = atoi(tbuf);
	j++;
    }
    iValArray[j] = -1;

    // Determine the type of weapon, and call the appropriate handler
    // for creating that weapon type.  Pass the name and the values extracted
    // from the database string.
    if (type == HSW_LASER) {
	return CreateLaser(strName, iValArray);
    } else if (type == HSW_MISSILE) {
	return CreateMissile(strName, iValArray);
    } else
	return NULL;
}

UINT CHSWeaponDBArray::NumWeapons(void)
{
    return m_num_weapons;
}

// Specifically creates a standard laser, loaded from the weapons
// database.  This handler really just has to allocate the memory,
// copy the name into the new weapon, and copy all of the values.
CHSDBWeapon *CHSWeaponDBArray::CreateLaser(char *strName, int *iVals)
{
    CHSDBLaser *weapon;
    int idx;

    // Check the number of values passed.  We should have 5
    // or six, depending on ;
    for (idx = 0; iVals[idx] != -1 && idx != 6; idx++) {
    };

    // Five or six, depends on how new this HSpace Weapon DB
    // is.
    if (idx != 5 && idx != 6)
	return NULL;

    // Allocate space
    weapon = new CHSDBLaser;

    // Now just initialize and set the appropriate variables
    weapon->m_type = HSW_LASER;
    strcpy(weapon->m_name, strName);
    weapon->m_strength = iVals[0];
    weapon->m_regen_time = iVals[1];
    weapon->m_range = iVals[2];
    weapon->m_accuracy = iVals[3];
    weapon->m_powerusage = iVals[4];
    // Not neseccarily exists, for compatibility with earlier versions
    // of hspace.
    if (iVals[5] && iVals[5] != -1)
	weapon->m_nohull = iVals[5];
    else
	weapon->m_nohull = 0;

    return (CHSDBWeapon *) weapon;
}

// Called upon to create a missile that was loaded from the weapons
// database.
CHSDBWeapon *CHSWeaponDBArray::CreateMissile(char *strName, int *iVals)
{
    UINT idx;
    CHSDBMissile *weapon;

    // Check for the number of values passed in.  Should have 5.
    for (idx = 0; iVals[idx] != -1; idx++) {
    };

    if (idx != 5)
	return NULL;

    // Allocate space
    weapon = new CHSDBMissile;

    // Now just initialize and set the appropriate variables
    weapon->m_type = HSW_MISSILE;
    strcpy(weapon->m_name, strName);
    weapon->m_strength = iVals[0];
    weapon->m_reload_time = iVals[1];
    weapon->m_range = iVals[2];
    weapon->m_turnrate = iVals[3];
    weapon->m_speed = iVals[4];

    return (CHSDBWeapon *) weapon;
}

BOOL CHSWeaponDBArray::GoodWeapon(int type)
{
    // Simply check to see if the weapon type is within bounds
    if (type >= 0 && (UINT)type < m_num_weapons)
	return TRUE;

    return FALSE;
}

//
// CHSWeaponArray stuff
//
//
CHSWeaponArray::CHSWeaponArray(void)
{
    m_nWeapons = 0;
    m_missile_bay = NULL;

    // Initialize the array
    int idx;
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++)
	m_weapons[idx] = NULL;

}

// Attempts to remove the weapon in the specified slot.
BOOL CHSWeaponArray::DeleteWeapon(int slot)
{
    int idx;

    if ((slot < 0) || (slot >= HS_MAX_WEAPONS))
	return FALSE;

    if ((m_weapons[slot] = NULL))
	return TRUE;

    // Delete the weapon in the slot and move all other
    // weapons above it down one slot.
    delete m_weapons[slot];
    for (idx = slot; idx < (HS_MAX_WEAPONS - 1); idx++)
	m_weapons[idx] = m_weapons[idx + 1];

    m_weapons[idx] = NULL;

    m_nWeapons--;

    return TRUE;
}

// Returns the weapon in a given slot.
CHSWeapon *CHSWeaponArray::GetWeapon(int slot)
{
    if ((slot < 0) || (slot >= HS_MAX_WEAPONS))
	return NULL;

    return m_weapons[slot];
}

// Takes a missile bay object and sets that bay as the
// source for all missile weapons in the array.
void CHSWeaponArray::SetMissileBay(CHSMissileBay * cBay)
{
    int idx;
    CHSMTube *mTube;

    // Run through the array, looking for missiles.
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (!m_weapons[idx])
	    continue;

	if (m_weapons[idx]->GetType() == HSW_MISSILE) {
	    mTube = (CHSMTube *) m_weapons[idx];
	    mTube->SetMissileBay(cBay);
	}
    }
    m_missile_bay = cBay;
}

// Returns the total power usage for all weapons in the array.
UINT CHSWeaponArray::GetTotalPower(void)
{
    UINT uPower;
    int idx;

    uPower = 0;
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (m_weapons[idx])
	    uPower += m_weapons[idx]->GetPowerUsage();
    }
    return uPower;
}

void CHSWeaponArray::ClearArray(void)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (m_weapons[idx])
	    delete m_weapons[idx];
    }
}

// The NewWeapon() function makes it easy to add a new weapon to
// the console's weapons array.  Simply pass in the class of weapon,
// which must be one of the weapons loaded into the WeaponDBArray object,
// and the new weapon appears in the console's weapon array.  Presto magic!
BOOL CHSWeaponArray::NewWeapon(int wclass)
{
    // Check to see if it's a valid weapon in the global database
    if (!waWeapons.GoodWeapon(wclass))
	return FALSE;


    // Just check the type and call the appropriate handler
    switch (waWeapons.GetType(wclass)) {
    case HSW_LASER:
	NewLaser(wclass);
	m_nWeapons++;
	break;

    case HSW_MISSILE:
	NewMTube(wclass);

	CHSMTube *mtube;
	mtube = (CHSMTube *) m_weapons[m_nWeapons];
	mtube->SetMissileBay(m_missile_bay);
	m_nWeapons++;
	break;
    }
    return TRUE;
}

// Adds a new laser to the weapons array on a console.  This function
// gets called by NewWeapon, which is the generic handler.
void CHSWeaponArray::NewLaser(int wclass)
{
    CHSLaser *newLas;

    // Allocate a new object
    newLas = new CHSLaser;

    // Add it to the array
    m_weapons[m_nWeapons] = newLas;

    // Initialize some variables
    newLas->m_class = wclass;

    // Set the parent
    m_weapons[m_nWeapons]->SetParent(waWeapons.
				     GetWeapon(wclass, HSW_LASER));
}

// Adds a new missile tube to the weapons array on a console.  This function
// gets called by NewWeapon, which is the generic handler.
void CHSWeaponArray::NewMTube(int wclass)
{
    m_weapons[m_nWeapons] = new CHSMTube;
    m_weapons[m_nWeapons]->m_class = wclass;

    // Set the parent, which is the missile type
    // this tube is configured for.
    m_weapons[m_nWeapons]->SetParent(waWeapons.
				     GetWeapon(wclass, HSW_MISSILE));
}

// Runs through the list of weapons, searching out the maximum
// weapon range.
UINT CHSWeaponArray::GetMaxRange(void)
{
    UINT range;
    UINT tRange;		// Temp holder
    int idx;

    range = 0;
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (m_weapons[idx]) {
	    tRange = m_weapons[idx]->GetRange();
	    if (tRange > range)
		range = tRange;
	}
    }
    return range;
}

// Handles cyclic stuff for the weapons in the array
void CHSWeaponArray::DoCycle(void)
{
    // Run through our weapons, telling them to cycle.
    int idx;
    for (idx = 0; idx < HS_MAX_WEAPONS; idx++) {
	if (m_weapons[idx])
	    m_weapons[idx]->DoCycle();
    }
}

//
// CHSWeapon stuff
//
CHSWeapon::CHSWeapon(void)
{
    m_parent = NULL;
    m_change = STAT_NOCHANGE;
}

UINT CHSWeapon::GetPowerUsage(void)
{
    // Generic weapons require no power
    return 0;
}

// Indicates whether the object can attack the specified
// type of CHSObject.  Override this in your derived weapon
// class to have your weapon respond to different types of
// objects.
BOOL CHSWeapon::CanAttackObject(CHSObject * cObj)
{
    // Generic weapons can't attack anything.
    return FALSE;
}

// Returns the recent status change of the weapon.
int CHSWeapon::GetStatusChange(void)
{
    return m_change;
}

// Sets the status of the weapon by an integer identifier.
// It's up to the weapon to determine what it should do with
// that integer.
void CHSWeapon::SetStatus(int stat)
{
    // Generic weapons don't handle this.
}

// The generic weapon has no cyclic stuff
void CHSWeapon::DoCycle(void)
{
}

UINT CHSWeapon::GetRange(void)
{
    // Generic weapons have no range
    return 0;
}

BOOL CHSWeapon::IsReady(void)
{
    return TRUE;
}

// Returns the type of the weapon, which is taken from
// the parent.
int CHSWeapon::GetType(void)
{
    if (!m_parent)
	return HSW_NOTYPE;

    return m_parent->m_type;
}

// Sets the parent of the weapon to a CHSDBWeapon object
void CHSWeapon::SetParent(CHSDBWeapon * parent)
{
    m_parent = parent;
}

// Override this function in your derived weapon class
// to return a custom name.
char *CHSWeapon::GetName(void)
{
    if (!m_parent)
	return "No Name";

    // Return the name from the parent
    return m_parent->m_name;
}

// Indicates whether the weapon needs a target to be
// fired.
BOOL CHSWeapon::RequiresLock(void)
{
    return FALSE;
}

// Override this function in your derived weapon class
// to return custom attribute information about the weapon,
// such as strength, regeneration time, etc.
char *CHSWeapon::GetAttrInfo(void)
{
    // Generic weapons have no attributes
    return "No information available.";
}

// Override this function in your derived weapon class
// to return custom status info such as time left to 
// recharge, etc.
char *CHSWeapon::GetStatus(void)
{
    // Generic weapons have unknown status
    return "Unknown";
}

// Returns an integer that is meaningful only to the weapon.
// The status is saved during shutdown and passed back to
// the weapon on reboot for handling.
int CHSWeapon::GetStatusInt(void)
{
    return 0;
}

// Indicates whether the weapon is configurable or not.
BOOL CHSWeapon::Configurable(void)
{
    // Generic weapons are not configurable
    return FALSE;
}

BOOL CHSWeapon::Configure(int type)
{

    // Generic weapons cannot be configured.
    return FALSE;
}

BOOL CHSWeapon::Loadable(void)
{
    // Generic weapons are not loadable
    return FALSE;
}

BOOL CHSWeapon::Reload(void)
{
    // Generic weapons are not loadable
    return FALSE;
}

BOOL CHSWeapon::Unload(void)
{
    // Generic weapons can't unload
    return FALSE;
}

// Returns the number of seconds until reload, or 0 if 
// not reloading.
int CHSWeapon::Reloading(void)
{
    // Generic weapons are never reloading
    return 0;
}

// Use this routine to instruct the weapon to attack a given
// object.  Supply the dbref of the console the weapon should
// send messages back to as well.
void CHSWeapon::AttackObject(CHSObject * cSource,
			     CHSObject * cTarget,
			     CHSConsole * cConsole, int iSysType)
{
    dbref dbUser;

    dbUser = hsInterface.ConsoleUser(cConsole->m_objnum);
    if (dbUser != NOTHING)
	hsStdError(dbUser,
		   "I don't know how to attack.  I'm just a plain old weapon.");
}

//
// CHSLaser stuff
//
CHSLaser::CHSLaser(void)
{
    m_regenerating = FALSE;
}

// Cyclic stuff for lasers
void CHSLaser::DoCycle(void)
{
    m_change = STAT_NOCHANGE;

    if (m_regenerating)
	Regenerate();
}

// Attacks a target object.
void CHSLaser::AttackObject(CHSObject * cSource,
			    CHSObject * cTarget,
			    CHSConsole * cConsole, int iSysType)
{
    dbref dbUser;
    int iAttackRoll;
    int iDefendRoll;
    int i;
    CHSObject *cCTarget;
    double sX, sY, sZ;		// Source object coords
    double tX, tY, tZ;		// Target object coords;

    // Grab the user of the console.
    dbUser = hsInterface.ConsoleUser(cConsole->m_objnum);

    // Can we attack that object?
    if (cSource->GetType() == HST_SHIP) {
	CHSSysCloak *cCloak;
	CHSShip *ptr;

	ptr = (CHSShip *) cSource;

	// Look for the cloaking device.
	cCloak = (CHSSysCloak *) ptr->GetEngSystem(HSS_CLOAK);
	if (cCloak)
	    if (cCloak->GetEngaged()) {
		if (dbUser != NOTHING)
		    hsStdError(dbUser, "You cannot fire while cloaked.");
		return;
	    }
    }

    if (!CanAttackObject(cTarget)) {
	if (dbUser != NOTHING)
	    hsStdError(dbUser,
		       "You cannot attack that target with that weapon.");
    }
    // Calculate distance to object
    sX = cSource->GetX();
    sY = cSource->GetY();
    sZ = cSource->GetZ();
    tX = cTarget->GetX();
    tY = cTarget->GetY();
    tZ = cTarget->GetZ();

    double dDistance;
    dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ) + .00001;

    // Size of a target ship matters relative to distance.
    // The closer a target gets to the ship, the larger
    // it effectively is.  That is to say it takes up more
    // of the view angle.  When the target is right next
    // to the ship, in front of the gun, it is essentially
    // the broad side of a barn, which everyone can hit.
    // Thus, to handle this we'll calculate the size of
    // the target and the viewing angle it takes up.

    double dSize;		// Size of the side of the target
    double dAngle;		// Amount of viewing angle taken up by size

    dSize = cTarget->GetSize();
    dSize = (.7 * dSize) * (.7 * dSize);

    // Halve the size, and divide by distance.  This
    // gives us the tangent of the angle taken up by
    // the target.
    dSize = (dSize * .5) / dDistance;

    // Take the inverse tangent to get angle.
    dAngle = atan(dSize);

    // Double the angle because we used half of the size
    // to get the angle of a right triangle.
    dAngle *= 2;

    // We now have the viewing angle consumed by the 
    // target.  There's a maximum possible value of 180,
    // so divide by that to determine how much of the viewing
    // angle is taken up by the target.
    dSize = dAngle * .005555;

    // Subtract from 1 to get maximum values of 1 when the
    // angle is small.
    dSize = 1 - dSize;

    // Now multiply by 6 to get relative difficulty of hitting
    // target.
    iDefendRoll = (int) (6 * dSize) + getrandom(6);
    iAttackRoll = GetAccuracy() + getrandom(6);

    // Simulate difficulty when a target is moving.
    // If the target is moving toward or away from the
    // attacker, it's not very difficult.  Thus, we
    // calculate the change in angle for the target
    // during one cycle.  The maximum change is 180
    // degrees.
    CHSVector tVec;
    CHSVector aVec;
    tVec = cTarget->GetMotionVector();
    aVec = cSource->GetMotionVector();

    // Calculate vector to target now.
    double dx, dy, dz;
    dx = tX - sX;
    dy = tY - sY;
    dz = tZ - sZ;

    // Make a unit vector
    dx /= dDistance;
    dy /= dDistance;
    dz /= dDistance;

    CHSVector nowVec(dx, dy, dz);

    // Now calculate coordinate for source and target
    // in one cycle.
    double sX2, sY2, sZ2;
    double tX2, tY2, tZ2;
    double aSpeed, tSpeed;

    // Grab both object speeds, and bring them down
    // to per-second levels.
    aSpeed = cSource->GetSpeed() * .0002778;
    tSpeed = cTarget->GetSpeed() * .0002778;

    // Calculate coordinates for next cycle.
    sX2 = sX + (aVec.i() * aSpeed);
    sY2 = sY + (aVec.j() * aSpeed);
    sZ2 = sZ + (aVec.k() * aSpeed);
    tX2 = tX + (tVec.i() * tSpeed);
    tY2 = tY + (tVec.j() * tSpeed);
    tZ2 = tZ + (tVec.k() * tSpeed);

    // Calculate vector to target after next cycle
    dx = tX2 - sX2;
    dy = tY2 - sY2;
    dz = tZ2 - sZ2;

    // Divide by distance to make a unit vector
    double dDistance2;
    dDistance2 = Dist3D(sX2, sY2, sZ2, tX2, tY2, tZ2);
    dx /= dDistance2;
    dy /= dDistance2;
    dz /= dDistance2;

    CHSVector nextVec(dx, dy, dz);

    // Calculate the dot product between the previous
    // and the next cycle vectors.
    double dp;
    dp = nowVec.DotProduct(nextVec);

    // Calculate the angle change.  This is in radians.
    dAngle = acos(dp);

    // Now divide angle change by 2pi to get change in angle
    // from 0 to 1, where 1 is a huge change in angle and,
    // therefore, high difficulty.
    dAngle *= .15915;

    // Add up to 6 points of defense for "evasion" by angle
    // change.
    iDefendRoll += (int) (6 * dAngle);


    // If distance is farther than our range, the shot always
    // misses.

    double range;
    range = GetRange();

    CHSUniverse *uDest;
    char tbuf[256];
    char fstat1[128];
    char fstat2[128];

    if (dDistance >= range || iDefendRoll > iAttackRoll) {
	sprintf(fstat1, "%s%smisses%s", ANSI_HILITE, ANSI_GREEN,
		ANSI_NORMAL);
	sprintf(fstat2, "%s%smissed%s", ANSI_HILITE, ANSI_GREEN,
		ANSI_NORMAL);
    } else {
	sprintf(fstat1, "%s%shits%s", ANSI_HILITE, ANSI_RED, ANSI_NORMAL);
	sprintf(fstat2, "%s%shit%s", ANSI_HILITE, ANSI_RED, ANSI_NORMAL);
    }

    uDest = uaUniverses.FindUniverse(cSource->GetUID());

    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContactS;
    SENSOR_CONTACT *cContactD;
    for (i = 0; i < HS_MAX_ACTIVE_OBJECTS; i++) {
	cCTarget = uDest->GetActiveUnivObject(i);
	if (!cCTarget)
	    continue;
	if (cCTarget == cSource || cCTarget == cTarget)
	    continue;

	cSensors = (CHSSysSensors *) cCTarget->GetEngSystem(HSS_SENSORS);
	if (!cSensors)
	    continue;

	cContactS = cSensors->GetContact(cSource);
	cContactD = cSensors->GetContact(cTarget);


	if (!cContactS && !cContactD)
	    continue;

	if (!cContactS && cContactD) {
	    if (cContactD->status == DETECTED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unknown contact is being fired upon and %s",
			cTarget->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactD->m_id, ANSI_NORMAL,
			cTarget->GetObjectColor(), ANSI_NORMAL, fstat2);
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    } else if (cContactD->status == IDENTIFIED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - The %s is being fired upon and %s",
			cTarget->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactD->m_id, ANSI_NORMAL,
			cTarget->GetObjectColor(), ANSI_NORMAL,
			cSource->GetName(), fstat2);
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    }
	    continue;
	}


	if (cContactS && !cContactD) {
	    if (cContactS->status == DETECTED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unknown contact is firing upon something",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL);
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    } else if (cContactS->status == IDENTIFIED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - The %s is firing upon something",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL,
			cSource->GetName());
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    }
	    continue;
	}

	if (cContactS && cContactD) {
	    if (cContactS->status == DETECTED
		&& cContactD->status == DETECTED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unknown contact fires and %s unknown contact %s[%s%s%d%s%s]%s",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL, fstat1,
			cTarget->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactD->m_id, ANSI_NORMAL,
			cTarget->GetObjectColor(), ANSI_NORMAL);
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    } else if (cContactS->status == IDENTIFIED
		       && cContactD->status == IDENTIFIED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - The %s fires and %s the %s",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL,
			cSource->GetName(), fstat1, cTarget->GetName());
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    } else if (cContactS->status == IDENTIFIED
		       && cContactD->status == DETECTED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - The %s fires and %s unknown contact %s[%s%s%d%s%s]%s",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL,
			cSource->GetName(), fstat1,
			cTarget->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactD->m_id, ANSI_NORMAL,
			cTarget->GetObjectColor(), ANSI_NORMAL);
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    } else if (cContactS->status == DETECTED
		       && cContactD->status == IDENTIFIED) {
		sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unknown contact fires and %s the %s",
			cSource->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContactS->m_id, ANSI_NORMAL,
			cSource->GetObjectColor(), ANSI_NORMAL, fstat1,
			cTarget->GetName());
		cCTarget->HandleMessage(tbuf, MSG_SENSOR,
					(long *) cCTarget);
	    }
        }
    }





    if (dDistance >= range) {
	if (dbUser != NOTHING)
	    hsStdError(dbUser,
		       "Your shot dissipates short of its target.");

	strcpy(tbuf, "An incoming energy shot has missed us.");
	cTarget->HandleMessage(tbuf, MSG_COMBAT, (long *) cSource);
    } else if (iAttackRoll > iDefendRoll) {
	// The weapon hits!
	// Determine strength based on base weapon
	// strength and range to target.
	int strength;

	strength = GetStrength();
	if (dDistance > (range * .333)) {
	    strength = (int) (strength *
			      (.333 +
			       (1 - (dDistance / (range + .0001)))));
	}
	// If iSysType is not HSS_NOTYPE, then do a roll
	// against the accuracy of the weapon to see if
	// the system gets hit.
	if (iSysType != HSS_NOTYPE) {
	    UINT ARoll, SRoll;

	    ARoll = getrandom(GetAccuracy());
	    SRoll = getrandom(10);

	    if (SRoll > ARoll)
		iSysType = HSS_NOTYPE;	// Didn't succeed
	}
	// Tell the target to take damage
	cTarget->HandleDamage(cSource, this, strength, cConsole, iSysType);

    } else {
	// The weapon misses. :(
	if (dbUser != NOTHING)
	    hsStdError(dbUser,
		       "Your shot skims past your target and out into space.");

	strcpy(tbuf, "An incoming energy shot has missed us.");
	cTarget->HandleMessage(tbuf, MSG_COMBAT, (long *) cSource);
    }

    Regenerate();
}

// Lasers require a target lock
BOOL CHSLaser::RequiresLock(void)
{
    return TRUE;
}

BOOL CHSLaser::NoHull(void)
{
    CHSDBLaser *dbLas;

    if (!m_parent)
	return 0;

    // Return the range
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_nohull;
}

// Handles the status integer sent to the weapon from
// some calling functions.
void CHSLaser::SetStatus(int stat)
{
    // Any integer indicates a reloading
    // status.
    if (stat > 0) {
	m_regenerating = TRUE;
	m_time_to_regen = stat;
    }
}

// Returns attribute information in a string.
char *CHSLaser::GetAttrInfo(void)
{
    static char rval[128];

    sprintf(rval,
	    "R: %-5d S: %-3d Rt: %d",
	    GetRange(), GetStrength(), GetRegenTime());
    return rval;
}

BOOL CHSLaser::IsReady(void)
{
    if (!m_regenerating)
	return TRUE;

    return FALSE;
}

void CHSLaser::Regenerate(void)
{
    // Check to see if we're regenerating.
    // If not, then begin it.
    if (!m_regenerating) {
	m_regenerating = TRUE;
	m_time_to_regen = GetRegenTime();
    } else {
	m_time_to_regen--;

	if (m_time_to_regen <= 0) {
	    m_change = STAT_READY;
	    m_regenerating = FALSE;
	}
    }
}

// Indicates whether the laser can attack the given type
// of space object.
BOOL CHSLaser::CanAttackObject(CHSObject * cObj)
{
    // Only attack ships and missiles
    if (cObj->GetType() == HST_SHIP 
        || (cObj->GetType() == HST_MISSILE && !NoHull())) {
	return TRUE;
    }
    return FALSE;
}

// Returns an integer representing the status of the laser.
// This integer is meaningful only to the laser code.
int CHSLaser::GetStatusInt(void)
{
    // Regenerating?
    if (m_regenerating)
	return m_time_to_regen;

    return 0;			// Not regenerating
}

// Returns the status of the laser
char *CHSLaser::GetStatus(void)
{
    static char tbuf[32];


    if (m_regenerating) {
	sprintf(tbuf, "(%d) Charging", m_time_to_regen);
	return tbuf;
    } else
	return "Ready";
}

UINT CHSLaser::GetRegenTime(void)
{
    CHSDBLaser *dbLas;

    if (!m_parent)
	return 0;

    // Return the regen time
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_regen_time;
}

UINT CHSLaser::GetRange(void)
{
    CHSDBLaser *dbLas;

    if (!m_parent)
	return 0;

    // Return the range
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_range;
}

UINT CHSLaser::GetStrength(void)
{
    CHSDBLaser *dbLas;

    if (!m_parent)
	return 0;

    // Return the strength
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_strength;
}

UINT CHSLaser::GetPowerUsage(void)
{
    CHSDBLaser *dbLas;

    // Return the power usage
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_powerusage;
}

UINT CHSLaser::GetAccuracy(void)
{
    CHSDBLaser *dbLas;

    if (!m_parent)
	return 0;

    // Return the accuracy
    dbLas = (CHSDBLaser *) m_parent;
    return dbLas->m_accuracy;
}
