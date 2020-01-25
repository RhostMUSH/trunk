#include "hscopyright.h"
#include <stdlib.h>
#include "hsdb.h"
#include "hsconf.h"
#include "hsweapon.h"
#include "hsinterface.h"
#include "hsutils.h"
#include "hsuniverse.h"
#include "hsterritory.h"
#include <string.h>

// The HSpace database object instance
CHSDB dbHSDB;

CHSDB::CHSDB(void)
{
    m_nWeapons = 0;
}

BOOL CHSDB::LoadDatabases(void)
{
    char tbuf[267];
    int cnt;

    // Load classes
    if (!LoadClassDB(HSCONF.classdb))
	hs_log("ERROR: No class DB found.");
    // Load weapons
    if (!LoadWeaponDB(HSCONF.weapondb))
	hs_log("ERROR: No weapon DB found.");
    m_nWeapons = waWeapons.NumWeapons();

    // Load universes
    if (!LoadUniverseDB(HSCONF.univdb))
	hs_log("ERROR: No universe DB found.");
    m_nUniverses = uaUniverses.NumUniverses();

    // Load planets
    LoadObjectDB();

    // Load Territories
    sprintf(tbuf, "LOADING: %s", HSCONF.territorydb);
    hs_log(tbuf);
    LoadTerritoryDB(HSCONF.territorydb);
    cnt = taTerritories.NumTerritories();
    sprintf(tbuf, "LOADING: %d %s loaded.", cnt,
	    cnt == 1 ? "territory" : "territories");
    hs_log(tbuf);

    return TRUE;
}

BOOL CHSDB::LoadUniverseDB(char *lpstrDBPath)
{
    return uaUniverses.LoadFromFile(lpstrDBPath);
}

void CHSDB::DumpUniverseDB(char *lpstrDBPath)
{

    uaUniverses.SaveToFile(lpstrDBPath);
}

BOOL CHSDB::LoadClassDB(char *lpstrDBPath)
{
    return caClasses.LoadFromFile(lpstrDBPath);
}

void CHSDB::DumpClassDB(char *lpstrDBPath)
{
    caClasses.SaveToFile(lpstrDBPath);
}

BOOL CHSDB::LoadWeaponDB(char *lpstrDBPath)
{
    return waWeapons.LoadFromFile(lpstrDBPath);
}

void CHSDB::DumpWeaponDB(char *lpstrDBPath)
{
    waWeapons.SaveToFile(lpstrDBPath);
}

BOOL CHSDB::LoadTerritoryDB(char *lpstrDBPath)
{
    return taTerritories.LoadFromFile(lpstrDBPath);
}

void CHSDB::DumpTerritoryDB(char *lpstrDBPath)
{
    taTerritories.SaveToFile(lpstrDBPath);
}

BOOL CHSDB::LoadObjectDB(void)
{
    FILE *fp;
    char tbuf[271];
    char strKey[64];
    char strValue[64];
    char *ptr;
    UINT type;
    HS_DBKEY key;
    UINT nShips, nCelestials, nObjs;
    CHSObject *newObj;

    sprintf(tbuf, "LOADING: %s ...", HSCONF.objectdb);
    hs_log(tbuf);

    // Load the object database, pulling out key/value pairs.
    // There should be an OBJECTTYPE attribute after each
    // new object definition.  Thus, we just allocate that
    // type of object, and tell it to load from the file.
    nShips = nCelestials = nObjs = 0;
    fp = fopen(HSCONF.objectdb, "r");
    if (!fp) {
	hs_log("ERROR: Unable to open object database for loading!");
	return FALSE;
    }

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

	case HSK_OBJECTTYPE:	// NEW OBJECT!
	    // Determine type, allocate object, and tell
	    // it to load.
	    type = atoi(strValue);
	    switch (type) {
	    case HST_SHIP:
		newObj = new CHSShip;
		if (newObj->LoadFromFile(fp))
		    nShips++;
		break;

	    case HST_PLANET:
		newObj = new CHSPlanet;
		if (newObj->LoadFromFile(fp))
		    nCelestials++;
		break;

	    case HST_NEBULA:
		newObj = new CHSNebula;
		if (newObj->LoadFromFile(fp))
		    nCelestials++;
		break;

	    case HST_ASTEROID:
		newObj = new CHSAsteroid;
		if (newObj->LoadFromFile(fp))
		    nCelestials++;
		break;

	    case HST_BLACKHOLE:
		newObj = new CHSBlackHole;
		if (newObj->LoadFromFile(fp))
		    nCelestials++;
		break;

	    case HST_WORMHOLE:
		newObj = new CHSWormHole;
		if (newObj->LoadFromFile(fp))
		    nCelestials++;
		break;

	    default:		// Generic CHSObject
		newObj = new CHSObject;
		if (newObj->LoadFromFile(fp))
		    nObjs++;
		break;
	    }
	    break;

	default:
	    sprintf(tbuf,
		    "WARNING: Key \"%s\" encountered but not handled.",
		    strKey);
	    hs_log(tbuf);
	    break;
	}
    }
    fclose(fp);
    sprintf(tbuf, "LOADING: %d generic objects loaded.", nObjs);
    hs_log(tbuf);
    sprintf(tbuf, "LOADING: %d celestial objects loaded.", nCelestials);
    hs_log(tbuf);
    sprintf(tbuf, "LOADING: %d ship objects loaded (Done).", nShips);
    hs_log(tbuf);

    return TRUE;
}

void CHSDB::DumpObjectDB(void)
{
    FILE *fp;
    CHSUniverse *cUniv;
    CHSObject *cObj;
    UINT i, j;
    char tbuf[305];

    fp = fopen(HSCONF.objectdb, "w");
    if (!fp) {
	sprintf(tbuf,
		"ERROR: Unable to open object db \"%s\" for writing.",
		HSCONF.objectdb);
	hs_log(tbuf);
	return;
    }
    // DBVERSION marker
    fprintf(fp, "DBVERSION=4.0\n");

    // Now write out all of the objects in all of the universes
    for (i = 0; i < HS_MAX_UNIVERSES; i++) {
	// Grab the universe
	cUniv = uaUniverses.GetUniverse(i);
	if (!cUniv)
	    continue;

	// Now grab all of the objects in the universe, and
	// call upon them to dump their attributes.
	for (j = 0; j < HS_MAX_OBJECTS; j++) {
	    cObj = cUniv->GetUnivObject(j);
	    if (!cObj)
		continue;

	    fprintf(fp, "OBJECTTYPE=%d\n", cObj->GetType());
	    cObj->WriteToFile(fp);
	    fprintf(fp, "OBJECTEND\n");
	}
    }
    fprintf(fp, "*END*\n");
    fclose(fp);
}

void CHSDB::DumpDatabases(void)
{
    // Save classes
    DumpClassDB(HSCONF.classdb);

    // Save weapons
    DumpWeaponDB(HSCONF.weapondb);

    // Save universes
    DumpUniverseDB(HSCONF.univdb);

    // Save planets
    DumpObjectDB();

    // Territories
    taTerritories.SaveToFile(HSCONF.territorydb);
}

// Call this member function to clean up attributes that are 
// written to objects during a database save.
void CHSDB::CleanupDBAttrs()
{
    CHSUniverse *uSource;
    UINT idx;
    UINT jdx;
    CHSObject *cObj;

    // Loop through all universes and all objects in those
    // universes, clearing attributes.
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	uSource = uaUniverses.GetUniverse(idx);
	if (!uSource)
	    continue;

	// Grab all objects, and tell them to cleanup their acts!
	for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
	    cObj = uSource->GetUnivObject(jdx);
	    if (!cObj)
		continue;

	    cObj->ClearObjectAttrs();
	}
    }
}

UINT CHSDB::NumWeapons(void)
{
    return m_nWeapons;
}

UINT CHSDB::NumUniverses(void)
{
    return m_nUniverses;
}

CHSShip *CHSDB::FindShip(int objnum)
{
    CHSUniverse *unDest;
    CHSShip *rShip;
    int idx;

    // Traverse all universes, looking for the ship
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (!(unDest = uaUniverses.GetUniverse(idx)))
	    continue;

	if ((rShip = (CHSShip *) unDest->FindObject(objnum, HST_SHIP)))
	    return rShip;
    }

    // No ship found
    return NULL;
}

CHSCelestial *CHSDB::FindCelestial(int objnum)
{
    CHSUniverse *unDest;
    CHSCelestial *rCel;
    int idx;

    // Traverse all universes, looking for the celestial
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	if (!(unDest = uaUniverses.GetUniverse(idx)))
	    continue;

	// We can use HST_PLANET here because the FindObject()
	// returns a generic celestial given any of the celestial
	// types.
	if ((rCel =
	     (CHSCelestial *) unDest->FindObject(objnum, HST_PLANET)))
	    return rCel;
    }

    // No celestial found
    return NULL;
}

// Generically looks for an object in the universes.  It is
// slightly optimized to save time by looking at the HSDB_TYPE
// attribute of the object and only looking through those types
// of objects.
CHSObject *CHSDB::FindObject(int objnum)
{
    HS_TYPE hstType;
    CHSObject *cObj;

    // Grab the TYPE attribute
    if (!hsInterface.AtrGet(objnum, "HSDB_TYPE"))
	return NULL;
    hstType = (HS_TYPE) atoi(hsInterface.m_buffer);

    // Depending on the type of attribute, call a variety of
    // functions.
    switch (hstType) {
    case HST_SHIP:
	cObj = FindShip(objnum);
	break;

    case HST_PLANET:
	cObj = FindCelestial(objnum);
	break;

    default:
	CHSUniverse * unDest;
	CHSObject *cObj;
	int idx;

	// Traverse all universes, looking for the object
	for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	    if (!(unDest = uaUniverses.GetUniverse(idx)))
		continue;

	    if ((cObj = unDest->FindObject(objnum)) != NULL)
		return cObj;
	}
	return NULL;
    }
    return cObj;
}

// Call this function to allow a player to create a new universe
// with a given room #.
void CHSDB::CreateNewUniverse(dbref player, char *strRoom)
{
    CHSUniverse *uDest;
    dbref dbRoom;		// Room to use as universe

    // arg_left is the dbref of the room to represent the
    // universe.
    dbRoom = hsInterface.NoisyMatchRoom(player, strRoom);
    if (dbRoom == NOTHING)
	return;
    if (Typeof(dbRoom) != TYPE_ROOM) {
	notify(player,
	       "You must specify a room to represent this universe.");
	return;
    }
    // Grab a new universe, but check that it was created
    uDest = uaUniverses.NewUniverse();
    if (!uDest) {
	notify(player,
	       "Unable to create new universe.  Check log for errors.");
	return;
    }
    // Set the objnum
    uDest->SetID(dbRoom);

    // Set the name
    uDest->SetName((char *) Name(dbRoom));

    // Set the flag on the room
    hsInterface.SetFlag(dbRoom, ROOM_HSPACE_UNIVERSE);

    // Increment number of universes
    m_nUniverses++;

    notify(player, unsafe_tprintf("Universe %s (#%d) created.  Behold!",
				  uDest->GetName(), uDest->GetID()));
}

// Call this function to allow a player to delete an existing
// universe with a given universe ID.
void CHSDB::DeleteUniverse(dbref player, char *strID)
{
    int uid;

    if (!strID || !*strID) {
	notify(player, "You must specify the uid of a universe.");
	return;
    }

    uid = atoi(strID);

    /* Check to see if the universe even exists */
    if (!uaUniverses.FindUniverse(uid)) {
	notify(player,
	       unsafe_tprintf("No such universe with id %d.", uid));
	return;
    }

    if (uaUniverses.DeleteUniverse(uid)) {
	m_nUniverses--;
	notify(player, "BOOM!  You have destroyed the universe!");
    } else
	notify(player, "Unable to delete that universe at this time.");
}

// Returns the CHSObject that controls the specified landing
// location. 
CHSObject *CHSDB::FindObjectByLandingLoc(int loc)
{
    int objnum;

    // Check for which type of object it's a part of.
    if (hsInterface.AtrGet(loc, "HSDB_PLANET")) {
	// Find landing location in planet.
	CHSPlanet *cPlanet;

	objnum = strtodbref(hsInterface.m_buffer);
	cPlanet = (CHSPlanet *) FindObject(objnum);
	return (CHSObject *) cPlanet;
    } else if (hsInterface.AtrGet(loc, "HSDB_SHIP")) {
	// Find landing location in a ship
	// Find landing location in planet.
	CHSShip *cShip;

	objnum = strtodbref(hsInterface.m_buffer);
	cShip = (CHSShip *) FindObject(objnum);
	return (CHSObject *) cShip;
    } else
	return NULL;		// Landing location of .. ?
}

// Call this function to find a landing location based on object
// dbref.
CHSLandingLoc *CHSDB::FindLandingLoc(int obj)
{
    int objnum;

    // Check for which type of object it's a part of.
    if (hsInterface.AtrGet(obj, "HSDB_PLANET")) {
	// Find landing location in planet.
	CHSPlanet *cPlanet;

	objnum = strtodbref(hsInterface.m_buffer);
	cPlanet = (CHSPlanet *) FindObject(objnum);
	if (!cPlanet)
	    return NULL;

	// Now pull the landing location object from the planet.
	return (cPlanet->FindLandingLoc(obj));
    } else if (hsInterface.AtrGet(obj, "HSDB_SHIP")) {
	// Find landing location in a ship
	// Find landing location in planet.
	CHSShip *cShip;

	objnum = strtodbref(hsInterface.m_buffer);
	cShip = (CHSShip *) FindObject(objnum);
	if (!cShip)
	    return NULL;

	// Now pull the landing location object from the planet.
	return (cShip->FindLandingLoc(obj));
    } else
	return NULL;		// Landing location of .. ?
}


CHSHatch *CHSDB::FindHatch(int obj)
{
    int objnum;

    if (hsInterface.AtrGet(obj, "HSDB_SHIP")) {
	CHSShip *cShip;

	objnum = strtodbref(hsInterface.m_buffer);
	cShip = (CHSShip *) FindObject(objnum);
	if (!cShip)
	    return NULL;

	// Now pull the hatch exit from the ship.
	return (cShip->FindHatch(obj));
    } else
	return NULL;		// Landing location of .. ?
}

// Call this function to find a console object based on the
// object dbref.
CHSConsole *CHSDB::FindConsole(int obj)
{
    dbref owner;
    CHSObject *cObj;

    // Make sure it's a console
    if (!hsInterface.AtrGet(obj, "HSDB_OWNER"))
	return NULL;

    // Grab the dbref of the owner
    owner = strtodbref(hsInterface.m_buffer);

    // Now we can use FindObject() to locate the object
    // that owns the console.
    cObj = FindObject(owner);
    if (!cObj)
	return NULL;

    return (cObj->FindConsole(obj));
}

// Call this function to find an HSpace object based on
// a given console.  Generally used to find ships based
// on a ship console.
CHSObject *CHSDB::FindObjectByConsole(dbref obj)
{
    dbref owner;
    CHSObject *cObj;

    // Make sure it's a console
    if (!hsInterface.AtrGet(obj, "HSDB_OWNER"))
	return NULL;

    // Grab the dbref of the owner
    owner = strtodbref(hsInterface.m_buffer);

    // Now we can use FindObject() to locate the object
    // that owns the console.
    cObj = FindObject(owner);

    return cObj;
}

#define HS_MAX_ROOM_SEARCH	256	// Max rooms to check for ownership

// Given a room #, this function will attempt to find the space
// object it belongs to.  Why?  This is useful for determining
// which planet a general room may belong to or which ship a room
// is on.
CHSObject *CHSDB::FindObjectByRoom(dbref room)
{
    int iRoomsChecked[HS_MAX_ROOM_SEARCH];

    iRoomsChecked[0] = NOTHING;
    return FindRoomOwner(room, iRoomsChecked);
}

CHSObject *CHSDB::FindRoomOwner(dbref room, int *iRoomsChecked)
{
    CHSObject *cOwner;

    // Did we check this room already?
    int idx;
    for (idx = 0; idx < HS_MAX_ROOM_SEARCH; idx++) {
	if (iRoomsChecked[idx] == room)
	    return NULL;
    }

    // Check attrs to see if this room indicates who
    // it belongs to.
    dbref dbOwner;
    if (hsInterface.AtrGet(room, "HSDB_PLANET")) {
	dbOwner = strtodbref(hsInterface.m_buffer);
	return FindObject(dbOwner);
    }

    if (hsInterface.AtrGet(room, "HSDB_SHIP")) {
	dbOwner = strtodbref(hsInterface.m_buffer);
	return FindObject(dbOwner);
    }
    // We're gonna have to search for another room that
    // As the given attribute.
    for (idx = 0; idx < HS_MAX_ROOM_SEARCH; idx++) {
	if (iRoomsChecked[idx] == NOTHING) {
	    iRoomsChecked[idx] = room;
	    break;
	}
    }
    // Did we reach our limit?
    idx++;
    if (idx == HS_MAX_ROOM_SEARCH)
	return NULL;

    // Ok to look for more rooms, so check the exits
    // and check their destination rooms.
    dbref exit_m;
    dbref dbDest;
    for (exit_m = Exits(room); exit_m && exit_m != NOTHING;
	 exit_m = Next(exit_m)) {
	// Get the destination of the exit.
	dbDest = Location(exit_m);
	if (dbDest == NOTHING)
	    continue;

	// Call ourself with the new room.
	cOwner = FindRoomOwner(dbDest, iRoomsChecked);
	if (cOwner)
	    return cOwner;
    }

    // Nothing found
    return NULL;
}
