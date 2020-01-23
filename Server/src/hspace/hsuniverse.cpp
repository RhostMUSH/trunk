#include "hscopyright.h"
#include "hsuniverse.h"
#include "hsutils.h"
#include "hspace.h"
#include "hseng.h"
#include "hsconf.h"
#include "hsinterface.h"
#include <stdlib.h>

#include <string.h>

CHSUniverseArray uaUniverses;	// One instance of this .. right here!

CHSUniverseArray::CHSUniverseArray(void)
{
    UINT idx;

    m_universes = new CHSUniverse *[HS_MAX_UNIVERSES];

    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++)
	m_universes[idx] = NULL;
}

CHSUniverseArray::~CHSUniverseArray(void)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (m_universes[idx])
	    delete m_universes[idx];
    }
    delete[]m_universes;
}

UINT CHSUniverseArray::NumUniverses(void)
{
    return m_num_universes;
}

// Prints information about all of the universes in the array
// to a given player.
void CHSUniverseArray::PrintInfo(int player)
{
    if (m_num_universes <= 0) {
	notify(player, "No universes currently loaded.");
	return;
    }

    notify(player,
	   "[Room#] Name                            Objects    Active");

    int idx;
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (m_universes[idx]) {
	    notify(player,
		   unsafe_tprintf("[%5d] %-32s%3d/%-3d    %3d/%-3d",
				  m_universes[idx]->GetID(),
				  m_universes[idx]->GetName(),
				  m_universes[idx]->NumObjects(),
				  HS_MAX_OBJECTS,
				  m_universes[idx]->NumActiveObjects(),
				  HS_MAX_ACTIVE_OBJECTS));
	}
    }
}

CHSUniverse *CHSUniverseArray::NewUniverse(void)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (!m_universes[idx]) {
	    m_universes[idx] = new CHSUniverse;

	    m_num_universes++;

	    // Give it a default ID.
	    m_universes[idx]->SetID(idx);
	    return m_universes[idx];
	}
    }
    return NULL;
}

BOOL CHSUniverseArray::AddUniverse(CHSUniverse * un)
{
    char tbuf[256];
    UINT uUID;

    // Make sure we don't go past the array limitations
    uUID = un->GetID();
    if ((uUID < 0) || (uUID >= HS_MAX_UNIVERSES)) {
	sprintf(tbuf,
		"WARNING: Invalid universe ID detected.  Discarding %s.",
		un->GetName());
	hs_log(tbuf);
	return FALSE;
    }

    // Looks ok, but we have to verify that there's not already
    // a universe in the slot that this one wants to go into.
    // Universes HAVE to go into the slot of their UID.
    if (m_universes[un->GetID()]) {
	sprintf(tbuf,
		"WARNING: Duplicate universe ID detected.  Discarding %s.",
		un->GetName());
	hs_log(tbuf);
	return FALSE;
    }

    m_universes[uUID] = un;
    m_num_universes++;

    return TRUE;
}

// Returns a pointer to a CHSUniverse given a universe dbref.
CHSUniverse *CHSUniverseArray::FindUniverse(dbref objnum)
{
    int idx;

    // Run through the list of universes
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (m_universes[idx]) {
	    if (m_universes[idx]->GetID() == objnum)
		return m_universes[idx];
	}
    }
    return NULL;
}

// Loads universes from the universe db.  It does this by
// loading each line in the db and splitting it up into
// a key/value pair.
BOOL CHSUniverseArray::LoadFromFile(char *lpstrPath)
{
    char tbuf[256];
    char strKey[256];
    char strValue[256];
    FILE *fp;
    char *ptr;
    CHSUniverse *uDest;
    HS_DBKEY key;
    UINT nLoaded;

    sprintf(tbuf, "LOADING: %s", lpstrPath);
    hs_log(tbuf);

    fp = fopen(lpstrPath, "r");
    if (!fp) {
	hs_log("ERROR: Unable to open object database for loading!");
	return FALSE;
    }

    uDest = NULL;
    nLoaded = 0;
    // Load in the lines from the database, splitting the
    // line into key/value pairs.  Each OBJNUM key specifies
    // the beginning of a new universe definition.
    while (fgets(tbuf, 256, fp)) {
	// Truncate newline chars
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	// Check for end of DB
	if (!strcmp(tbuf, "*END*"))
	    break;

	// Extract key and value
	extract(tbuf, strKey, 0, 1, '=');
	extract(tbuf, strValue, 1, 1, '=');

	// Determine key type, then do something.
	key = HSFindKey(strKey);
	switch (key) {
	case HSK_NOKEY:
	    sprintf(tbuf,
		    "WARNING: Invalid key \"%s\" encountered.", strKey);
	    hs_log(tbuf);
	    break;

	case HSK_DBVERSION:	// This should be the first line, if present
	    break;

	case HSK_OBJNUM:	// NEW UNIVERSE!
	    uDest = NewUniverse();
	    if (!uDest) {
		sprintf(tbuf,
			"ERROR: Unable to load universe #%d.  Possibly ran out slots.",
			atoi(strValue));
		hs_log(tbuf);
	    } else {
		dbref dbRoom;
		dbRoom = atoi(strValue);
		uDest->SetID(dbRoom);
		hsInterface.SetFlag(dbRoom, ROOM_HSPACE_UNIVERSE);
		nLoaded++;
	    }
	    break;

	default:
	    // Just try to set the attribute on the
	    // universe.
	    if (uDest && !uDest->SetAttributeValue(strKey, strValue)) {
		sprintf(tbuf,
			"WARNING: Key \"%s\" encountered but not handled.",
			strKey);
		hs_log(tbuf);
	    }
	    break;
	}
    }
    fclose(fp);
    sprintf(tbuf, "LOADING: %d universes loaded.", nLoaded);
    hs_log(tbuf);

    return TRUE;
}

// Returns the universe object in the given UID slot.
CHSUniverse *CHSUniverseArray::GetUniverse(UINT uid)
{
    // Error check
    if (uid < 0 || uid >= HS_MAX_UNIVERSES)
	return NULL;

    return m_universes[uid];
}

BOOL CHSUniverseArray::DeleteUniverse(UINT uid)
{
    // Find the universe
    int idx;
    CHSUniverse *uSource;
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (!m_universes[idx])
	    continue;

	uSource = m_universes[idx];

	if (uSource->GetID() == uid)
	    break;
    }
    if (idx == HS_MAX_UNIVERSES)
	return FALSE;

    // Check to see if the universe is empty
    if (!uSource->IsEmpty())
	return FALSE;

    // Delete it
    delete uSource;
    m_universes[idx] = NULL;
    m_num_universes--;

    return TRUE;
}

// Runs through the list of universes, telling them to
// save to the specified file.
void CHSUniverseArray::SaveToFile(char *lpstrPath)
{
    FILE *fp;
    int i;
    char tbuf[256];

    // Any universes to save?
    if (m_num_universes <= 0)
	return;

    // Open the database
    fp = fopen(lpstrPath, "w");
    if (!fp) {
	sprintf(tbuf, "ERROR: Unable to write universes to %s.",
		lpstrPath);
	hs_log(tbuf);
	return;
    }
    // Print dbversion
    fprintf(fp, "DBVERSION=4.0\n");

    // Run down the list of univereses, demanding they save!
    for (i = 0; i < HS_MAX_UNIVERSES; i++) {
	if (!m_universes[i])
	    continue;

	m_universes[i]->SaveToFile(fp);
    }
    fprintf(fp, "*END*\n");
    fclose(fp);
}

CHSUniverse::CHSUniverse(void)
{
    UINT idx;

    // Allocate storage arrays.  The m_objlist and
    // m_active_objlist arrays are necessary.  Everything
    // else is used to optimize searching for objects.
    m_objlist = new CHSObject *[HS_MAX_OBJECTS];
    m_active_objlist = new CHSObject *[HS_MAX_ACTIVE_OBJECTS];
    m_shiplist = new CHSShip *[HS_MAX_OBJECTS];
    m_celestials = new CHSCelestial *[HS_MAX_OBJECTS];

    // Initialize arrays
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	m_shiplist[idx] = NULL;
	m_objlist[idx] = NULL;
	m_celestials[idx] = NULL;
    }

    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	m_active_objlist[idx] = NULL;
    }

    m_name = NULL;
    m_nCelestials = 0;
    m_nShips = 0;
    m_nObjects = 0;
    m_objnum = -1;
}

UINT CHSUniverse::GetID(void)
{
    return m_objnum;
}

void CHSUniverse::SetID(int id)
{
    m_objnum = id;
}

// Sets the name of the universe
void CHSUniverse::SetName(char *strName)
{
    if (m_name) {
	delete[]m_name;
	m_name = NULL;
    }

    if (strName) {
	m_name = new char[strlen(strName) + 1];
	strcpy(m_name, strName);
    }
}

CHSUniverse::~CHSUniverse(void)
{
    if (m_objlist)
	delete[]m_objlist;

    if (m_active_objlist)
	delete[]m_active_objlist;

    if (m_celestials)
	delete[]m_celestials;

    if (m_shiplist)
	delete[]m_shiplist;
}

char *CHSUniverse::GetName(void)
{
    return m_name;
}

// Used to add a celestial to the list of celestials.  The
// celestial first exists in the object array.  It's here 
// so that we can specifically search for just a celestial
// if we want.
BOOL CHSUniverse::AddCelestial(CHSCelestial * cObj)
{
    UINT idx;

    // Check to see if it'll fit and that it's not already there.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_celestials[idx])
	    break;
    }

    if (idx == HS_MAX_OBJECTS) {
	hs_log((char *) "WARNING: Universe at celestial capacity.");
	return FALSE;
    }

    m_celestials[idx] = cObj;

    m_nCelestials++;

    return TRUE;
}

// This function is protected because any caller should call
// the generic AddObject() function, which will then call this
// function.  This function, then, adds a ship (if possible)
// to the universe ship list.
BOOL CHSUniverse::AddShip(CHSShip * cObj)
{
    UINT idx;

    // Check to see if it'll fit and that it's not already there.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_shiplist[idx])
	    break;
    }

    if (idx == HS_MAX_OBJECTS) {
	hs_log((char *) "WARNING: Universe at ship capacity.");
	return FALSE;
    }

    m_shiplist[idx] = cObj;

    m_nShips++;

    return TRUE;
}

// Finds a given object with a dbref number
CHSObject *CHSUniverse::FindObject(dbref objnum, HS_TYPE type)
{
    int idx;

    switch (type) {
    case HST_SHIP:
	return FindShip(objnum);

    case HST_PLANET:
    case HST_WORMHOLE:
    case HST_NEBULA:
    case HST_ASTEROID:
    case HST_BLACKHOLE:
	return FindCelestial(objnum);

    default:			// Generic object
	for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	    if (m_objlist[idx] && (m_objlist[idx]->GetDbref() == objnum))
		return m_objlist[idx];
	}
	return NULL;
    }
}

// Looks for a ship with the given object dbref
CHSShip *CHSUniverse::FindShip(dbref objnum)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_shiplist[idx])
	    continue;

	if (m_shiplist[idx]->GetDbref() == objnum) {
	    return m_shiplist[idx];
	}
    }
    return NULL;
}

// Looks for a celestial with the given object dbref
CHSCelestial *CHSUniverse::FindCelestial(dbref objnum)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_celestials[idx])
	    continue;

	if (m_celestials[idx]->GetDbref() == objnum)
	    return m_celestials[idx];
    }
    return NULL;
}

// Call this function to remove an object from the
// "active" list for the universe, which stores all
// objects that aren't docked, aren't dropped, etc.
void CHSUniverse::RemoveActiveObject(CHSObject * cObj)
{
    UINT idx;


    // Simply run down the list until we find the
    // object.  Then yank it.
    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	if (m_active_objlist[idx]) {
	    // If this is the object being removed, remove it.
	    // Otherwise, force its removal from the sensors
	    // of other objects.
	    if (m_active_objlist[idx]->GetDbref() == cObj->GetDbref()) {
		m_active_objlist[idx] = NULL;
		break;
	    } else {
		CHSSysSensors *cSensors;
		cSensors = (CHSSysSensors *)
		    m_active_objlist[idx]->GetEngSystem(HSS_SENSORS);
		if (cSensors)
		    cSensors->LoseObject(cObj);
	    }
	}
    }
}

// This is a public function that should be called to
// remove ANY object from the universe.
void CHSUniverse::RemoveObject(CHSObject * cObj)
{
    int idx;

    // Determine the type of object, then call
    // the appropriate function.  If you haven't
    // implemented your own storage arrays for
    // your type of object (like a ship or planet),
    // then don't worry about this.
    switch (cObj->GetType()) {
    case HST_SHIP:
	RemoveShip((CHSShip *) cObj);
	break;

    case HST_PLANET:
	break;
    }

    // Now remove the object from the object lists
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_objlist[idx])
	    continue;

	if (m_objlist[idx]->GetDbref() == cObj->GetDbref()) {
	    m_objlist[idx] = NULL;
	    m_nObjects--;

	    // Make sure it's out of the active list
	    RemoveActiveObject(cObj);
	}
    }
}

// Adds an object to the active list of objects.
BOOL CHSUniverse::AddActiveObject(CHSObject * cObj)
{
    int idx;
    int slot;
    char tbuf[128];

    // Find a slot, and also check to see if the object
    // is already on the list.
    slot = -1;
    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	if (m_active_objlist[idx]) {
	    if (m_active_objlist[idx]->GetDbref() == cObj->GetDbref())
		return FALSE;
	} else if (slot == -1)
	    slot = idx;
    }

    if (slot == -1) {
	// No more space
	sprintf(tbuf, "WARNING: Universe #%d at maximum object capacity.",
		m_objnum);
	hs_log(tbuf);
	return FALSE;
    }

    m_active_objlist[slot] = cObj;

    return TRUE;
}

// Allows any object to be generically added to the universe.
// The function determines the type of object and then calls
// a more specific adding function.  It also adds the object
// to the linked list of objects.
BOOL CHSUniverse::AddObject(CHSObject * cObj)
{
    int idx;
    int slot;
    char tbuf[128];

    // Add it to the object list first.  If we
    // don't have anymore room, then don't call
    // any specific object functions.  Also make
    // sure the object isn't already in the list.
    // Determine the type of object, then call
    // the appropriate function.
    slot = -1;
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (!m_objlist[idx] && (slot == -1))
	    slot = idx;

	// Object already in the list?
	if (m_objlist[idx] &&
	    (m_objlist[idx]->GetDbref() == cObj->GetDbref()))
	    return FALSE;
    }

    if (slot == -1) {
	// No space left
	sprintf(tbuf, "WARNING: Universe #%d at maximum object count.",
		m_objnum);
	hs_log(tbuf);
	return FALSE;
    }
    // Add the object
    m_objlist[slot] = cObj;

    m_nObjects++;

    // Add it to the active list?
    if (cObj->IsActive())
	AddActiveObject(cObj);

    // If you have any specific adding functions for a
    // type of object, add it here.  For ships and planets,
    // we have additional arrays for quick searching, thus
    // we implement that here.
    switch (cObj->GetType()) {
    case HST_SHIP:
	return (AddShip((CHSShip *) cObj));

    case HST_PLANET:
	return (AddCelestial((CHSCelestial *) cObj));

    default:
	return TRUE;		// Generic object
	break;
    }
}

// Specifically removes a ship from the universe.
void CHSUniverse::RemoveShip(CHSShip * cShip)
{
    UINT idx;

    // Now just go down the list, find the ship, and yank it.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (m_shiplist[idx]) {
	    if (m_shiplist[idx]->GetDbref() == cShip->GetDbref()) {
		m_shiplist[idx] = NULL;
		m_nShips--;
		break;
	    }
	}
    }
}

// Specifically removes a celestial from the universe.
void CHSUniverse::RemoveCelestial(CHSCelestial * cCel)
{
    UINT idx;

    // Run down the list of celestials, find this one, yank it.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	if (m_celestials[idx]) {
	    if (m_celestials[idx]->GetDbref() == cCel->GetDbref()) {
		m_celestials[idx] = NULL;
		break;
	    }
	}
    }
}

// Returns TRUE or FALSE depending on whether the universe
// has no objects in it.
BOOL CHSUniverse::IsEmpty(void)
{
    if (m_nObjects <= 0)
	return TRUE;

    return FALSE;
}

// Return number of objects, perhaps for a specific type.
UINT CHSUniverse::NumObjects(HS_TYPE type)
{
    switch (type) {
    case HST_NOTYPE:
	return m_nObjects;

    case HST_SHIP:
	return m_nShips;

    case HST_PLANET:
    case HST_WORMHOLE:
    case HST_NEBULA:
    case HST_ASTEROID:
    case HST_BLACKHOLE:
	return m_nCelestials;

    default:
	return -1;
    }
}

// Returns the ship object at a given slot, which could be NULL.
CHSShip *CHSUniverse::GetShip(UINT slot)
{
    if ((slot < 0) || (slot >= HS_MAX_OBJECTS))
	return NULL;

    return m_shiplist[slot];
}

// Returns the celestial object at a given slot, which could be NULL.
CHSCelestial *CHSUniverse::GetCelestial(UINT slot)
{
    if ((slot < 0) || (slot >= HS_MAX_OBJECTS))
	return NULL;

    return m_celestials[slot];
}

// Returns the number of active objects, perhaps for a specific type.
UINT CHSUniverse::NumActiveObjects(HS_TYPE type)
{
    int idx;
    int num;

    num = 0;
    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	if (m_active_objlist[idx]) {
	    if (type == HST_NOTYPE)
		num++;
	    else if (m_active_objlist[idx]->GetType() == type)
		num++;
	}
    }
    return num;
}

// Returns the object at a given slot, which could be NULL.
CHSObject *CHSUniverse::GetUnivObject(UINT slot, HS_TYPE tType)
{
    // Call a subfunction?
    switch (tType) {
    case HST_SHIP:
	return GetShip(slot);

    case HST_PLANET:
    case HST_WORMHOLE:
    case HST_NEBULA:
    case HST_BLACKHOLE:
    case HST_ASTEROID:
	return GetCelestial(slot);

    default:			// Return any CHSObject
	if ((slot < 0) || (slot >= HS_MAX_OBJECTS))
	    return NULL;

	return m_objlist[slot];
    }
}

// Returns the object at a given slot in the active array.
CHSObject *CHSUniverse::GetActiveUnivObject(UINT slot)
{
    if ((slot < 0) || (slot >= HS_MAX_OBJECTS))
	return NULL;

    return m_active_objlist[slot];
}

// Sends a message to every object within a given distance
// from the supplied CHSObject.
void CHSUniverse::SendMessage(char *strMsg, int iDist, CHSObject * cSource)
{
    double sX, sY, sZ;		// Source coords
    double tX, tY, tZ;		// Target coords
    double dDistance;
    CHSObject *cObj;
    int idx;

    // If a CHSObject was supplied, get its coords.
    if (cSource) {
	sX = cSource->GetX();
	sY = cSource->GetY();
	sZ = cSource->GetZ();
    } else
	sX = sY = sZ = 0;

    // Run through all active objects, checking distance
    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	if (!m_active_objlist[idx])
	    continue;

	cObj = m_active_objlist[idx];

	// Make sure it's not the source object
	if (cObj->GetDbref() == cSource->GetDbref())
	    continue;

	// Calculate distance between coords, if needed
	if (iDist > 0) {
	    tX = cObj->GetX();
	    tY = cObj->GetY();
	    tZ = cObj->GetZ();

	    dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ);

	    if (dDistance > iDist)
		continue;
	}
	// At this point, the distance is ok.  Give the
	// message
	cObj->HandleMessage(strMsg, MSG_GENERAL, (long *) cSource);
    }
}

// Sends a message to every ship that has the supplied
// CHSObject on sensors with the given status.
void CHSUniverse::SendContactMessage(char *strMsg, int status,
				     CHSObject * cSource)
{
    CHSObject *cObj;
    CHSShip *cShip;
    SENSOR_CONTACT *cContact;
    int idx;

    // Run through all active objects, checking distance
    for (idx = 0; idx < HS_MAX_ACTIVE_OBJECTS; idx++) {
	if (!m_active_objlist[idx])
	    continue;

	cObj = m_active_objlist[idx];

	// Is it an object with sensors?
	if (cObj->GetType() != HST_SHIP)
	    continue;

	cShip = (CHSShip *) cObj;

	// Grab the sensor contact for the source object
	// in the sensor array of the target object.
	cContact = cShip->GetSensorContact(cSource);
	if (cContact && (cContact->status == status))
	    cObj->HandleMessage(strMsg, MSG_GENERAL, (long *) cSource);
    }
}

// Allows an attribute with a given name to be set to
// a specified value.
BOOL CHSUniverse::SetAttributeValue(char *strName, char *strValue)
{
    // Try to match the attribute name
    if (!strcasecmp(strName, "NAME")) {
	if (m_name)
	    delete[]m_name;

	if (strValue && *strValue) {
	    m_name = new char[strlen(strValue) + 1];
	    strcpy(m_name, strValue);
	}
	return TRUE;
    } else if (!strcasecmp(strName, "OBJNUM")) {
	m_objnum = strtodbref(strValue);
	return TRUE;
    }
    return FALSE;
}

// Tells the universe to save its information to the
// specified file stream.
void CHSUniverse::SaveToFile(FILE * fp)
{
    if (!fp)
	return;

    // Write out our attrs
    fprintf(fp, "OBJNUM=%d\n", m_objnum);
    if (m_name)
	fprintf(fp, "NAME=%s\n", m_name);
}
