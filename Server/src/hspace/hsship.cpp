#include "hscopyright.h"
#include "hsobjects.h"
#include "hsinterface.h"
#include "hsuniverse.h"
#include <stdlib.h>
#include "hsutils.h"
#include "hsconf.h"
#include "hspace.h"
#include "ansi.h"
#include "hsengines.h"
#include <string.h>
#include "hscomm.h"
#include "hsdb.h"

#include "externs.h"

CHSShip::CHSShip(void)
{
	int idx;

	// Initialize a slew of variables
	m_type  = HST_SHIP;
	m_ident = NULL;
	m_target = NULL;
	m_current_xyheading = 0;
	m_desired_xyheading = 0;
	m_current_zheading  = 0;
	m_desired_zheading  = 0;
	m_current_roll		= 0;
	m_desired_roll		= 0;
	m_hull_points		= 1;
	m_map_range			= 1000;
	m_drop_status		= 0;
	m_dock_status		= 0;
	m_age = 0;

	bReactorOnline = FALSE;
	bGhosted	= FALSE;
	m_in_space	= TRUE;
	m_docked	= FALSE;
	m_dropped	= FALSE;
	m_destroyed = FALSE;
	m_hyperspace= FALSE;
	m_class		= 0;
	m_boarding_code = NULL;
	m_territory = NULL;
	m_objlocation = NOTHING;
	m_self_destruct_timer = 0;

	// Overridables
	m_can_drop = NULL;
	m_spacedock = NULL;
	m_cargo_size = NULL;
	m_minmanned = NULL;

	memset(&m_classinfo, 0, sizeof(m_classinfo));

	SetHeadingVector(0,0,0);

	// Initialize board links
	for (idx = 0; idx < MAX_HATCHES; idx++)
		m_hatches[idx] = NULL;

	// Initialize consoles
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
		m_console_array[idx] = NULL;

	// Initialize ship rooms
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
		m_ship_rooms[idx] = NOTHING;

	// Initialize landing locations
	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
		m_landing_locs[idx] = NULL;
}

CHSShip::~CHSShip(void)
{
	int idx;

	if (m_ident)
		delete [] m_ident;

	// Delete consoles
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
			delete m_console_array[idx];
	}
}

// Determines whether the ship is actively in space or, perhaps,
// docked or dropped.
BOOL CHSShip::IsActive(void)
{
	if (!m_docked && !m_dropped && m_in_space && !m_destroyed)
		return TRUE;

	return FALSE;
}

// Sets the heading vector on the ship to new coefficients
void CHSShip::SetHeadingVector(int i, int j, int k)
{
	CHSVector vec(i, j, k);

	m_motion_vector = vec;
}

// Given a class number, this function will setup the
// appropriate pointers and system information so that
// the ship becomes of that class type.  The function
// is not just used for changing a class.  It's also
// used when the ship is loaded from the database and
// needs its class information setup.
BOOL CHSShip::SetClassInfo(UINT uClass)
{
	HSSHIPCLASS *hsClass;
	char tbuf[64];
	CHSEngSystem *cSys;
	CHSEngSystem *tSys = NULL;

	// Grab the class from the global array
	hsClass = caClasses.GetClass(uClass);
	if (!hsClass)
	{
		sprintf(tbuf, "Class info for class #%d not found.", 
			uClass);
		hs_log(tbuf);
		return FALSE;
	}

	// Set the pointer to the class.
	m_classinfo = hsClass;

	// Set the class id in case it hasn't already
	m_class = uClass;

    // ONLY duplicate system information if this is a fresh ship
	// without systems saved for itsself already!
	if(!m_systems.GetHead()) {
		m_systems = hsClass->m_systems; }


	// Run through the systems, setting the owner to us.
	for (cSys = m_systems.GetHead(); cSys; cSys = cSys->GetNext())
	{
		tSys = hsClass->m_systems.GetSystem(cSys->GetType());
		if (tSys)
			cSys->SetParentSystem(tSys);
		cSys->SetOwner(this);
	}

	// Do some fuel systems handling
	CHSFuelSystem *cFuel;
	cFuel = (CHSFuelSystem *)m_systems.GetSystem(HSS_FUEL_SYSTEM);
	if (cFuel)
	{
		CHSReactor *cReactor;
		cReactor = (CHSReactor *)m_systems.GetSystem(HSS_REACTOR);

		// Tell the reactor where to get fuel
		if (cReactor)
		{
			cReactor->SetFuelSource(cFuel);
		}

		// Tell the engines where to get fuel
		CHSSysEngines *cEngines;
		cEngines = (CHSSysEngines *)m_systems.GetSystem(HSS_ENGINES);
		if (cEngines)
		{
			cEngines->SetFuelSource(cFuel);
		}

		// Tell jump drives where to get fuel
		CHSJumpDrive *cJumpers;
		cJumpers = (CHSJumpDrive *)m_systems.GetSystem(HSS_JUMP_DRIVE);
		if (cJumpers)
		{
			cJumpers->SetFuelSource(cFuel);
		}
	}
	return TRUE;
}

BOOL CHSShip::AddConsole(dbref objnum)
{
	int idx;
	int slot;
	
	// Verify that it's a good object
	if (!hsInterface.ValidObject(objnum))
		return FALSE;

	// Find a slot
	slot = MAX_SHIP_CONSOLES;
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		// Empty slot?
		if (!m_console_array[idx] && (slot == MAX_SHIP_CONSOLES))
			slot = idx;

		// Console already in array?
		if (m_console_array[idx] && 
			(m_console_array[idx]->m_objnum == objnum))
			return FALSE;
	}

	if (slot == MAX_SHIP_CONSOLES)
	{
		char tbuf[256];
		sprintf(tbuf, "WARNING: Maximum consoles for ship #%d reached.",
			m_objnum);
		hs_log(tbuf);
		return FALSE;
	}

	// Make a new console .. set the variables
	m_console_array[slot] = new CHSConsole;

	// Tell the console to load attributes from the object.
	// Even if it's a new console and new object, that's ok.
	m_console_array[slot]->LoadFromObject(objnum);

	// Record it's owner for later use in finding the
	// actual console object.
	m_console_array[slot]->SetOwner(m_objnum);

	// Record it's CHSObject owner
	m_console_array[slot]->SetOwnerObj(this);

	// Tell the console where our missile bay is
	m_console_array[slot]->SetMissileBay(&m_missile_bay);

	// Tell the console where our computer is
	CHSSysComputer *cComputer;
	cComputer = (CHSSysComputer *)m_systems.GetSystem(HSS_COMPUTER);
	if (cComputer)
		m_console_array[slot]->SetComputer(cComputer);

	return TRUE;
}

// Call this to have the object clear its attributes AND
// all attrs in the base objects that were saved to objects
// in the game.
void CHSShip::ClearObjectAttrs(void)
{
	UINT idx;

	CHSObject::ClearObjectAttrs();

	// Clear console attributes
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
			m_console_array[idx]->ClearObjectAttrs();
	}
}

// Called upon to set an attribute value.  If the attribute
// name is not found, it's passed up to the base CHSObject 
// for handling.
BOOL CHSShip::SetAttributeValue(char *strName, char *strValue)
{
	int iVal;

	if (!strcasecmp(strName, "IDENT"))
	{
		if (m_ident)
			delete [] m_ident;

		if (strValue)
		{
			m_ident = new char[strlen(strValue) + 1];
			strcpy(m_ident, strValue);
		}
		return TRUE;
	}
	else if (!strcasecmp(strName, "ZHEADING"))
	{
		iVal = atoi(strValue);
		if (iVal < -90 || iVal > 90)
			return FALSE;

		m_current_zheading = iVal;

		return TRUE;
	}
	else if (!strcasecmp(strName, "XYHEADING"))
	{
		iVal = atoi(strValue);
		if (iVal < 0 || iVal > 359)
			return FALSE;

		m_current_xyheading = iVal;

		return TRUE;
	}
	else if (!strcasecmp(strName, "DESTROYED"))
	{
		iVal = atoi(strValue);

		if (m_destroyed && !iVal)
			ResurrectMe();

		m_destroyed = iVal;

		return TRUE;
	}
	else if (!strcasecmp(strName, "BOARDING CODE"))
	{
		if (m_boarding_code)
			delete [] m_boarding_code;

		if (strValue)
		{
			m_boarding_code = new char[strlen(strValue) + 1];
			strcpy(m_boarding_code, strValue);
		}
		return TRUE;
	}
	else if (!strcasecmp(strName, "CARGO SIZE"))
	{
		if (!strValue || !*strValue)
		{
			if (m_cargo_size)
			{
				delete m_cargo_size;
				m_cargo_size = NULL;
			}
			return TRUE;
		}


		if (!m_cargo_size)
			m_cargo_size = new int;

		*m_cargo_size = atoi(strValue);
		return TRUE;
	}
	else if (!strcasecmp(strName, "CAN DROP"))
	{
		if (!strValue || !*strValue)
		{
			if (m_can_drop)
			{
				delete m_can_drop;
				m_can_drop = NULL;
			}
			return TRUE;
		}


		if (!m_can_drop)
			m_can_drop = new BOOL;

		*m_can_drop = atoi(strValue);
		return TRUE;
	}
	else if (!strcasecmp(strName, "SPACEDOCK"))
	{
		if (!strValue || !*strValue)
		{
			if (m_spacedock)
			{
				delete m_spacedock;
				m_spacedock = NULL;
			}
			return TRUE;
		}


		if (!m_spacedock)
			m_spacedock = new BOOL;

		*m_spacedock = atoi(strValue);
		return TRUE;
	}
	else if (!strcasecmp(strName, "DROPLOC"))
	{
		CHSUniverse *uDest;

		iVal = strtodbref(strValue);

		// If iVal < 0, then we set the vessel's
		// status to not-dropped.
		if (iVal < 0)
		{
			if (!m_dropped) // Do nothing
				return TRUE;

			uDest = uaUniverses.FindUniverse(m_uid);
			if (!uDest)
				return FALSE;

			uDest->AddActiveObject(this);

			// Put the ship into space.
			MoveShipObject(uDest->GetID());

			m_dropped = FALSE;

			if (!m_docked)
			{
				m_objlocation = NOTHING;
				m_in_space = TRUE;
			}

			return TRUE;
		}
		else
		{
			if (!hsInterface.ValidObject(iVal))
				return FALSE;

			MoveShipObject(iVal);
			m_dropped = TRUE;

			uDest = uaUniverses.FindUniverse(m_uid);
			if (uDest)
				uDest->RemoveActiveObject(this);

			m_in_space = FALSE;

			// See if we can find what HSpace object
			// we're on now.
			CHSObject *cDest;
			cDest = dbHSDB.FindObjectByRoom(iVal);
			if (!cDest)
			{
				m_objlocation = NOTHING;
			}
			else
				m_objlocation = cDest->GetDbref(); 
			return TRUE;
		}
	}
	else if (!strcasecmp(strName, "HULL"))
	{
		iVal = atoi(strValue);

		m_hull_points = iVal;

		if (m_hull_points < 0 && !m_destroyed)
			ExplodeMe();
		return TRUE;
	}
	else if (!strcasecmp(strName, "MINMANNED"))
	{
		if (!strValue || !*strValue)
		{
			if (m_minmanned)
			{
				delete m_minmanned;
				m_minmanned = NULL;
			}
			return TRUE;
		}


		if (!m_minmanned)
			m_minmanned = new int;

		*m_minmanned = atoi(strValue);
		return TRUE;	}
	else if (!strcasecmp(strName, "AGE"))
	{
		if (!strValue || !*strValue)
		{
			m_age = 0;
			return TRUE;
		}
		m_age = atoi(strValue);
		return TRUE;	}
	else if (!strcasecmp(strName, "DOCKLOC"))
	{
		CHSUniverse *uDest;

		iVal = strtodbref(strValue);

		// If iVal < 0, then we set the vessel's
		// status to not-docked.
		if (iVal < 0)
		{
			if (!m_docked) // Do nothing
				return TRUE;


			uDest = uaUniverses.FindUniverse(m_uid);
			if (!uDest)
				return FALSE;

			uDest->AddActiveObject(this);

			// Put the ship into space.
			MoveShipObject(uDest->GetID());

			m_docked = FALSE;

			if (!m_dropped)
			{
				m_objlocation = NOTHING;
				m_in_space = TRUE;
			}

			return TRUE;
		}
		else
		{
			if (!hsInterface.ValidObject(iVal))
				return FALSE;

			MoveShipObject(iVal);
			m_docked = TRUE;

			uDest = uaUniverses.FindUniverse(m_uid);
			if (uDest)
				uDest->RemoveActiveObject(this);

			m_in_space = FALSE;

			// See if we can find what HSpace object
			// we're on now.
			CHSObject *cDest;
			cDest = dbHSDB.FindObjectByRoom(iVal);
			if (!cDest)
			{
				m_objlocation = NOTHING;
			}
			else
				m_objlocation = cDest->GetDbref();
 
			return TRUE;
		}
	}
	else
		return CHSObject::SetAttributeValue(strName, strValue);
}

int CHSShip::GetMinManned(void)
{
	if (m_minmanned)
		return *m_minmanned;
	else
		return m_classinfo->m_minmanned;
}

// Returns a character string containing the value of the
// requested attribute.
char *CHSShip::GetAttributeValue(char *strName)
{
	static char rval[2048];
	char tmp[32];

	int idx;

	*rval = 0;
	if (!strcasecmp(strName, "IDENT"))
	{
		if (m_ident)
			strcpy(rval, m_ident);
	}
	else if (!strcasecmp(strName, "HATCHES"))
	{
		for (idx = 0; idx < MAX_HATCHES; idx++)
		{
			if (m_hatches[idx])
			{
				if (!*rval)
					sprintf(tmp, "#%d", 
						m_hatches[idx]->objnum);
				else
					sprintf(tmp, " #%d",
						m_hatches[idx]->objnum);

				strcat(rval, tmp);	
			}
		}
	}
	else if (!strcasecmp(strName, "SROOMS"))
	{
		for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
		{
			if (m_ship_rooms[idx] != NOTHING)
			{
				if (!*rval)
					sprintf(tmp, "#%d", 
						m_ship_rooms[idx]);
				else
					sprintf(tmp, " #%d",
						m_ship_rooms[idx]);

				strcat(rval, tmp);	
			}
		}
	}
 	else if (!strcasecmp(strName, "LANDINGLOCS"))
 	{
 		for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
 		{
 			if (m_landing_locs[idx] != NULL)
 			{
 				if (!*rval)
 					sprintf(tmp, "#%d", 
 						m_landing_locs[idx]->objnum);
 				else
 					sprintf(tmp, " #%d",
 						m_landing_locs[idx]->objnum);
 
 				strcat(rval, tmp);	
 			}
 		}
 	}
	else if (!strcasecmp(strName, "CONSOLES"))
	{
		for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
		{
			if (m_console_array[idx])
			{
				if (!*rval)
					sprintf(tmp, "#%d", 
						m_console_array[idx]->m_objnum);
				else
					sprintf(tmp, " #%d",
						m_console_array[idx]->m_objnum);

				strcat(rval, tmp);	
			}
		}
	}
	else if (!strcasecmp(strName, "DESTROYED"))
	{
		sprintf(rval, "%d", m_destroyed ? 1 : 0);
	}
	else if (!strcasecmp(strName, "BOARDING CODE"))
	{
		if (m_boarding_code)
			strcpy(rval, m_boarding_code);
	}
	else if (!strcasecmp(strName, "CAN DROP"))
	{
		sprintf(rval, "%d", m_can_drop ? *m_can_drop :
			m_classinfo->m_can_drop);
	}
	else if (!strcasecmp(strName, "SPACEDOCK"))
	{
		sprintf(rval, "%d", m_spacedock ? *m_spacedock :
			m_classinfo->m_spacedock);
	}
	else if (!strcasecmp(strName, "OBJLOC"))
	{
			sprintf(rval, "#%d", m_objlocation);
	}
	else if (!strcasecmp(strName, "DROPLOC"))
	{
			sprintf(rval, "#%d", m_dropped ? 
				Location_hspace(m_objnum) : -1);
	}
	else if (!strcasecmp(strName, "MAXHULL"))
	{
		sprintf(rval, "%d", GetMaxHullPoints());
	}
	else if (!strcasecmp(strName, "HULL"))
	{
		sprintf(rval, "%d", GetHullPoints());
	}
	else if (!strcasecmp(strName, "AGE"))
	{
		sprintf(rval, "%d", m_age);
	}
	else if (!strcasecmp(strName, "DOCKLOC"))
	{
		sprintf(rval, "#%d", m_docked ? 
				Location_hspace(m_objnum) : -1);
	}
	else if (!strcasecmp(strName, "DXYHEADING"))
		sprintf(rval, "%d", m_desired_xyheading);
	else if (!strcasecmp(strName, "DZHEADING"))
		sprintf(rval, "%d", m_desired_zheading);
	else if (!strcasecmp(strName, "DROLL"))
		sprintf(rval, "%d", m_desired_roll);
	else if (!strcasecmp(strName, "XYHEADING"))
		sprintf(rval, "%d", m_current_xyheading);
	else if (!strcasecmp(strName, "ZHEADING"))
		sprintf(rval, "%d", m_current_zheading);
	else if (!strcasecmp(strName, "ROLL"))
		sprintf(rval, "%d", m_current_roll);
	else if (!strcasecmp(strName, "CARGO SIZE"))
	{
		sprintf(rval, "%d", 
			m_cargo_size ? *m_cargo_size : m_classinfo->m_cargo_size);
	}
	else if (!strcasecmp(strName, "CLASS"))
		sprintf(rval, "%d", m_class);
	else if (!strcasecmp(strName, "CLASSNAME"))
		strcpy(rval, m_classinfo->m_name);
	else if (!strcasecmp(strName, "MCONSOLES"))
		sprintf(rval, "%d", GetMannedConsoles());
	else if (!strcasecmp(strName, "MINMANNED"))
		sprintf(rval, "%d", GetMinManned());
	else
		return CHSObject::GetAttributeValue(strName);

	return rval;
}

// Return amount of manned consoles
int CHSShip::GetMannedConsoles(void)
{
	int rval, idx;

	rval = 0;
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			if(hsInterface.ConsoleUser(m_console_array[idx]->m_objnum) != m_console_array[idx]->m_objnum)
				rval += 1;
		}
	}
	return rval;
}

// Adds a registered room to a ship
BOOL CHSShip::AddSroom(dbref dbRoom)
{
	int idx;

	// Find a slot for the room
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (m_ship_rooms[idx] == NOTHING)
			break;
		if (m_ship_rooms[idx] == dbRoom)
			return FALSE;
	}

	if (idx >= MAX_SHIP_ROOMS)
		return FALSE;
	
	if (HSCONF.autozone) {
		hsInterface.SetLock(m_objnum, GOD, LOCK_ZONE);
// FIXME
		//Zone(dbRoom) = m_objnum;
	}
	m_ship_rooms[idx] = dbRoom;
	return TRUE;
}

// Adds a registered blink to a ship
BOOL CHSShip::AddHatch(dbref dbExit)
{
	CHSHatch *cHatch;
	char tbuf[256]; // Temp string storage

	// Grab a new landing loc
	cHatch = NewHatch();
	if (!cHatch)
	{
		sprintf(tbuf, 
			"WARNING: Maximum hatches reached for ship #%d.",
			m_objnum);
		hs_log(tbuf);
		return FALSE;
	}

	// Initialize some attrs
	cHatch->objnum = dbExit;
	cHatch->SetOwnerObject(this);

	// Set the flag
	hsInterface.SetFlag(dbExit, EXIT_HSPACE_HATCH);

	// Set the planet attribute
	hsInterface.AtrAdd(dbExit, "HSDB_SHIP",
		unsafe_tprintf("#%d", m_objnum), GOD, AF_MDARK || AF_WIZARD);

	return TRUE;
}

// Deletes a given console from the registered consoles
BOOL CHSShip::RemoveConsole(dbref dbConsole)
{
	int idx;

	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx] &&
			(m_console_array[idx]->m_objnum == dbConsole))
		{
			// Reset the flag
			hsInterface.UnsetFlag(dbConsole, THING_HSPACE_CONSOLE);

			// Free the object
			delete m_console_array[idx];
			m_console_array[idx] = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

UINT CHSShip::ClassNum(void)
{
	return m_class;
}

// Deletes a given room from the registered srooms
BOOL CHSShip::DeleteSroom(dbref dbRoom)
{
	int idx;
	BOOL bFound = FALSE;

	// Run through the list, deleting any rooms that match.
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (m_ship_rooms[idx] == dbRoom)
		{
			bFound = TRUE;
			m_ship_rooms[idx] = NOTHING;
		}
	}

	if (bFound)
		return TRUE;
	else
		return FALSE;
}
/*
BOOL CHSShip::DeleteBlink(dbref dbExit)
{
	int idx;
	BOOL bFound = FALSE;

	Run through the list, deleting any rooms that match.
	for (idx = 0; idx < MAX_BOARD_LINKS; idx++)
	{
		if (m_boarding_links[idx] == dbExit)
		{
			bFound = TRUE;
			m_boarding_links[idx] = NOTHING;
		}
	}

	if (bFound)
		return TRUE;
	else
		return FALSE;
} */

CHSConsole *CHSShip::FindConsole(dbref dbConsole)
{
	UINT idx;

	// Traverse our console array, looking for consoles
	// with matching dbrefs.
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx] &&
			(m_console_array[idx]->m_objnum == dbConsole))
			return m_console_array[idx];
	}

	// Not found
	return NULL;
}

// Passed keys and values by the CHSObject LoadFromFile().
// We just have to handle them.
BOOL CHSShip::HandleKey(int key, char *strValue, FILE *fp)
{
	dbref obj;
	dbref objnum;

	// Determine key and handle it
	switch(key)
	{
		case HSK_MISSILEBAY: // Load in missile bay info
			m_missile_bay.LoadFromFile(fp);
			return TRUE;

		case HSK_IDENT:
			if (m_ident)
				delete [] m_ident;
			m_ident = new char [strlen(strValue) + 1];
			strcpy(m_ident, strValue);
			return TRUE;

		case HSK_BOARDING_CODE:
			if (m_boarding_code)
				delete [] m_boarding_code;

			m_boarding_code = new char [strlen(strValue) + 1];
			strcpy(m_boarding_code, strValue);
			return TRUE;

		case HSK_LANDINGLOC:
			CHSLandingLoc *cLoc;

			cLoc = NewLandingLoc();
			if (!cLoc)
			{
				hs_log("ERROR: Couldn't add specified landing location.");
			}
			else
			{
				objnum = atoi(strValue);
				if (!hsInterface.ValidObject(objnum))
					return TRUE;

				cLoc->SetOwnerObject(this);
				cLoc->LoadFromObject(objnum);
				// Set the planet attribute
				hsInterface.AtrAdd(objnum, "HSDB_SHIP",
					unsafe_tprintf("#%d", m_objnum), GOD);
			}
			return TRUE;
		
		case HSK_HATCH:
			CHSHatch *cHatch;

			objnum = atoi(strValue);
			if (!hsInterface.ValidObject(objnum))
				return TRUE;

			cHatch = NewHatch();
			if (!cHatch)
			{
				hs_log("ERROR: Couldn't add specified hatch.");
			}
			else
			{
				cHatch->SetOwnerObject(this);
				cHatch->LoadFromObject(objnum);
				// Set the planet attribute
				hsInterface.AtrAdd(objnum, "HSDB_SHIP",
					unsafe_tprintf("#%d", m_objnum), GOD);
			}
			return TRUE;


		case HSK_CARGOSIZE:
			if (!m_cargo_size)
				m_cargo_size = new int;

			*m_cargo_size = atoi(strValue);
			return TRUE;

		case HSK_DROP_CAPABLE:
			if (!m_can_drop)
				m_can_drop = new BOOL;

			*m_can_drop = atoi(strValue);
			return TRUE;

		case HSK_HULL_POINTS:
			m_hull_points = atoi(strValue);
			return TRUE;

		case HSK_SPACEDOCK:
			if (!m_spacedock)
				m_spacedock = new BOOL;

			*m_spacedock = atoi(strValue);
			return TRUE;

		case HSK_OBJLOCATION:
			m_objlocation = atoi(strValue);
			if (!hsInterface.ValidObject(m_objlocation))
				m_objlocation = NOTHING;
			return TRUE;
			
		case HSK_CLASSID:
			m_class = atoi(strValue);
			SetClassInfo(m_class); /* We just want to load it from Object DB so we can
			can just delete systems manually */
			return TRUE;

		case HSK_DROPPED:
			m_dropped = atoi(strValue);

			if (m_dropped)
				m_in_space = FALSE;
			return TRUE;

		case HSK_DOCKED:
			m_docked = atoi(strValue);

			if (m_docked)
				m_in_space = FALSE;
			return TRUE;

		case HSK_DESTROYED:
			m_destroyed = atoi(strValue);
			return TRUE;

		case HSK_XYHEADING:
			m_current_xyheading = atoi(strValue);
			m_desired_xyheading = m_current_xyheading;

			// Recompute heading vectors
			RecomputeVectors();
			return TRUE;

		case HSK_ZHEADING:
			m_current_zheading = atoi(strValue);
			m_desired_zheading = m_current_zheading;

			// Recompute heading vectors
			RecomputeVectors();
			return TRUE;

		case HSK_ROLL:
			m_current_roll = atoi(strValue);
			m_desired_roll = m_current_roll;
			return TRUE;

		case HSK_MINMANNED:
			if (!m_minmanned)
				m_minmanned = new int;

			*m_minmanned = atoi(strValue);
			return TRUE;

		case HSK_ROOM:
			obj = atoi(strValue);
			if (!hsInterface.ValidObject(obj))
				return FALSE;

			AddSroom(obj);
			return TRUE;

		case HSK_CONSOLE:
			obj = atoi(strValue);
			if (!hsInterface.ValidObject(obj))
				return FALSE;

			AddConsole(obj);
			return TRUE;

		case HSK_SYSTEMDEF: // Handle a system!
			LoadSystem(fp);
			return TRUE;

		case HSK_AGE:
			m_age = atoi(strValue);
			return TRUE;

		default:  // Just call the base function
			return (CHSObject::HandleKey(key, strValue, fp));
	}
}

// Cleans up any final ship details after it is loaded from
// the database.
void CHSShip::EndLoadObject(void)
{
	// If we have a computer and any consoles,
	// set the computer for the consoles.
	CHSSysComputer *cComputer;
	cComputer = (CHSSysComputer *)m_systems.GetSystem(HSS_COMPUTER);
	if (cComputer)
	{
		int idx;

		for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
		{
			if (m_console_array[idx])
				m_console_array[idx]->SetComputer(cComputer);
		}
	}
}

// Overridden from the base CHSObject class, but we'll call up
// to that object as well.
void CHSShip::WriteToFile(FILE *fp)
{
	int idx;

	// Call the base class first, then output our
	// stuff.  It's best to do it that way, but not
	// really necessary.
	CHSObject::WriteToFile(fp);

	// Now our's
	if (m_ident)
		fprintf(fp, "IDENT=%s\n", m_ident);
	if (m_boarding_code)
		fprintf(fp, "BOARDING_CODE=%s\n", m_boarding_code);
	fprintf(fp, "HULL_POINTS=%d\n", m_hull_points);
	fprintf(fp, "DROPPED=%d\n", m_dropped);
	fprintf(fp, "DOCKED=%d\n", m_docked);
	fprintf(fp, "DESTROYED=%d\n", m_destroyed);
	fprintf(fp, "XYHEADING=%d\n", m_current_xyheading);
	fprintf(fp, "ZHEADING=%d\n", m_current_zheading);
	fprintf(fp, "ROLL=%d\n", m_current_roll);
	fprintf(fp, "AGE=%d\n", m_age);
 
	if (m_objlocation != NOTHING)
		fprintf(fp, "OBJLOCATION=%d\n", m_objlocation);
	
	if (m_can_drop)
		fprintf(fp, "DROP CAPABLE=%d\n", *m_can_drop);

	if (m_spacedock)
		fprintf(fp, "SPACEDOCK=%d\n", *m_spacedock);

	if (m_cargo_size)
		fprintf(fp, "CARGOSIZE=%d\n", *m_cargo_size);
	
	if (m_minmanned)
		fprintf(fp, "MINMANNED=%d\n", *m_minmanned);

	// Write out missile bay info.  This needs to come
	// before consoles.
	fprintf(fp, "MISSILEBAY\n");
	m_missile_bay.SaveToFile(fp);

	// Write out rooms
	for (idx = 0; m_ship_rooms[idx] != NOTHING; idx++)
	{
		fprintf(fp, "ROOM=%d\n", m_ship_rooms[idx]);
	}

	// Write out consoles
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			fprintf(fp, "CONSOLE=%d\n",
				m_console_array[idx]->m_objnum);
			m_console_array[idx]->WriteObjectAttrs();
		}
	}

	// Landing locs
	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
	{
		if (m_landing_locs[idx])
		{
			fprintf(fp, "LANDINGLOC=%d\n", m_landing_locs[idx]->objnum);
			m_landing_locs[idx]->WriteObjectAttrs();
		}
	}

	// Hatches
	for (idx = 0; idx < MAX_HATCHES; idx++)
	{
		if (m_hatches[idx])
		{
			fprintf(fp, "HATCH=%d\n", m_hatches[idx]->objnum);
			m_hatches[idx]->WriteObjectAttrs();
		}
	}

	// Output system information
	m_systems.SaveToFile(fp);

	fprintf(fp, "CLASSID=%d\n", m_class);
}

// When the HandleKey() encounters a SYSTEMDEF key, it
// calls this function to load the system information into
// a system that already exists on the ship.
BOOL CHSShip::LoadSystem(FILE *fp)
{
	CHSEngSystem *cSys;
	CHSEngSystem *cSysTmp;
	char strKey[64];
	char strValue[64];
	char *ptr;
	char tbuf[256];
	HS_DBKEY key;
	int iCurSlot;
	int iPriority;
	HSS_TYPE type;

	cSys = NULL;

	// We have NO WAY of knowing what kind of information each
	// system needs.  We ONLY know what types of systems exist
	// because this is defined in hseng.h.  Thus, we simply
	// figure out the type of system, and then pass the key/value
	// information to the system as if it were setting attributes.
	while(fgets(tbuf, 256, fp))
	{
		// Truncate newlines
		if ((ptr = strchr(tbuf, '\n')) != NULL)
			*ptr = '\0';
		if ((ptr = strchr(tbuf, '\r')) != NULL)
			*ptr = '\0';

		extract(tbuf, strKey, 0, 1, '=');
		extract(tbuf, strValue, 1, 1, '=');
		key = HSFindKey(strKey);

		// Check for end of database .. would be an error
		if (!strcmp(strKey, "*END*"))
		{
			hs_log(
				"ERROR: Encountered end of objectdb before loading specified system info.");
			break;
		}

		// Check the key, and do something
		switch(key)
		{
			case HSK_SYSTYPE:
				// Find the system on the ship
				type = (HSS_TYPE) atoi(strValue);
				cSys = m_systems.GetSystem(type);
				// Don't exit if system not found, just
				// warning.
				if (!cSys || type == HSS_FICTIONAL)
				{
					cSys = m_systems.NewSystem(type);
					m_systems.AddSystem(cSys);
				}
				break;

			case HSK_SYSTEMEND:
				return TRUE;

			case HSK_PRIORITY:
				// Put this system back into it's proper slot.
				// Initially it is in the priority position specified
				// by how it was loaded in the class object.  That
				// may or may not be the position it was at for the
				// specific ship.  Thus, we have to handle that.
				iPriority = atoi(strValue);

				// If no system, then do nothing
				if (!cSys)
					break;

				// Find the current priority level
				iCurSlot = 0;
				for (cSysTmp = m_systems.GetHead(); cSysTmp; cSysTmp = cSysTmp->GetNext())
				{
					if (!strcmp(cSysTmp->GetName(), cSys->GetName()))
					{
						// System match.
						break;
					}
					else
						iCurSlot++;
				}

				if (iCurSlot > iPriority)
				{
					// Move the system up in priority
					while(iCurSlot > iPriority)
					{
						if (!m_systems.BumpSystem(cSys, 1))
							break;

						iCurSlot--;
					}
				}
				else if (iCurSlot < iPriority)
				{
					// Move the system down
					while(iCurSlot < iPriority)
					{
						if (!m_systems.BumpSystem(cSys, -1))
							break;

						iCurSlot++;
					}
				}
				break;

			default:
				// Simply call upon the object to set it's
				// own attributes.  We don't know what they
				// are.
				if (cSys &&
					!cSys->SetAttributeValue(strKey, strValue))
				{
					sprintf(tbuf, 
						"WARNING: System type %d unable to handle attribute \"%s\"",
						type, strKey);
					hs_log(tbuf);
				}
				break;
		}
	}
	return FALSE;
}

// Does various system stuff each cycle.
void CHSShip::HandleSystems(void)
{
	CHSSysEngines *cEngines;
	CHSReactor *cReactor;
	CHSJumpDrive *cJumpers;
	int iPowerDeficit;
	int iSysPower;
	int iSpeed;
	BOOL bStat;
	char tbuf[256];
	CHSEngSystem *cSys;


	// Find the regular engines.
	cEngines = (CHSSysEngines *)m_systems.GetSystem(HSS_ENGINES);
	if (cEngines)
		iSpeed = (int)cEngines->GetCurrentSpeed();

	// Find jump drives to see if we get a disengage status.
	BOOL bEngaged = FALSE;
	cJumpers = (CHSJumpDrive *)m_systems.GetSystem(HSS_JUMP_DRIVE);
	if (cJumpers)
	{
		bEngaged = cJumpers->GetEngaged();
		if (bEngaged)
		{
			// Tell the jumpers our current sublight speed.
			cJumpers->SetSublightSpeed(iSpeed);
		}
	}

	// Tell the systems to do their cycle stuff
	m_systems.DoCycle();

	// Check various jump conditions.
	if (bEngaged)
	{
		if (!cJumpers->GetEngaged())
		{
			ExitHyperspace();
		}
		else if (cEngines)
		{
			if (cEngines->GetCurrentSpeed() < 
				cJumpers->GetMinJumpSpeed())
			{
				cJumpers->SetEngaged(FALSE);
				ExitHyperspace();
			}
		}
	}

	if (m_self_destruct_timer > 0)
	{
		m_self_destruct_timer--;

		if (m_self_destruct_timer < 1)
		{
			NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN PROGRESS ***%s",
				ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL));

			ExplodeMe();
			if (!hsInterface.HasFlag(m_objnum, TYPE_THING, THING_HSPACE_SIM))
				KillShipCrew("SELF DESTRUCTED");
		} else {
			if (m_self_destruct_timer == 3600) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 1 HOUR ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 1200) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 20 MINUTES ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 300) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 5 MINUTES ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 120) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 2 MINUTES ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 60) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 1 MINUTE ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 30) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 30 SECONDS ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer == 20) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN 20 SECONDS ***%s",
					ANSI_RED, ANSI_HILITE, ANSI_NORMAL));
			} else if (m_self_destruct_timer < 11) {
				NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN %d SECONDS ***%s",
					ANSI_RED, ANSI_HILITE, m_self_destruct_timer, ANSI_NORMAL));
			}
		}
	}

	cReactor = (CHSReactor *)m_systems.GetSystem(HSS_REACTOR);
	if (cReactor)
	{
		bStat = cReactor->IsOnline();
		if (!bReactorOnline && bStat)
		{
			bReactorOnline = TRUE;
			sprintf(tbuf,
				"%s%s-%s A light flashes on your console, indicating that the main reactor is now online.",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
			NotifyConsoles(tbuf, MSG_ENGINEERING);

			NotifySrooms(HSCONF.reactor_activating);
			// Give an outside effects message
			dbref loc;	

			loc = GetDockedLocation();
			if (loc != NOTHING)
			{
				char tbuf[128];
				sprintf(tbuf,
					"Lights flicker on the %s as its reactor comes online.",
					GetName());
				hsInterface.NotifyContents(loc, tbuf);
			}
		}
		else if (bReactorOnline && !bStat)
			bReactorOnline = FALSE;

		// Check power availability.  If the reactor is producing
		// less power than we're using, we have to start drawing power
		// from systems.
		iPowerDeficit = m_systems.GetPowerUse() - cReactor->GetOutput();
		if (iPowerDeficit > 0)
		{
			// Start drawing power from systems
			cSys = m_systems.GetHead();


			while(cSys && (iPowerDeficit > 0))
			{
				// Don't draw power from non-visible systems
				if (!cSys->IsVisible())
				{
					cSys = cSys->GetNext();
					continue;
				}

				iSysPower = cSys->GetCurrentPower();

				// Can we get enough power from this system?
				if (iSysPower >= iPowerDeficit)
				{
					// Draw the needed power from this system.
					iSysPower -= iPowerDeficit;
					iPowerDeficit -= cSys->GetCurrentPower();
					cSys->SetCurrentPower(iSysPower);

					sprintf(tbuf,
						"%s%s-%s A warning light flashes, indicating that the %s system has lost power.",
						ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
						cSys->GetName());
					NotifyConsoles(tbuf, MSG_ENGINEERING);
				}
				else if (iSysPower > 0)
				{
					// Draw ALL power from this system.
					iPowerDeficit -= iSysPower;
					cSys->SetCurrentPower(0);

					sprintf(tbuf,
						"%s%s-%s A warning light flashes, indicating that the %s system has lost power.",
						ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
						cSys->GetName());
					NotifyConsoles(tbuf, MSG_ENGINEERING);
				}
				cSys = cSys->GetNext();
			}
		}
		if(bReactorOnline)
			m_age++;
	}

	// Now do stuff for each system.

	// Do sensor stuff (this isn't finding contacts.
	// That's done in the DoCycle() of systems.).
	HandleSensors();

	// Life support
	HandleLifeSupport();
}

// This is THE CYCLE for ships.  It handles everything from
// traveling to sensor sweeping.
void CHSShip::DoCycle(void)
{
	int idx;

	// Don't do anything if we're destroyed
	if (m_destroyed)
		return;

	
	// Consoles
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			m_console_array[idx]->DoCycle();
		}
	}

	dbref loc;
	loc = GetDockedLocation();
	if (loc)
	{
		CHSLandingLoc *cLocation;
		cLocation = dbHSDB.FindLandingLoc(loc);
		if(cLocation) 
		{
			CHSObject *cDocked;

			cDocked = cLocation->GetOwnerObject();

			if(cDocked)
			{
				if(cDocked->GetType() == HST_SHIP)
				{
					CHSShip *cShip;

					cShip = (CHSShip *)cLocation->GetOwnerObject();
					if (cShip)
					{				
						SetX(cShip->GetX());
						SetY(cShip->GetY());
						SetZ(cShip->GetZ());
					}
				}
			}
		}
	}

	// System stuff first.
	HandleSystems();

	// See if we're landing or taking off
	if (m_drop_status > 0 || m_dock_status > 0)
	{
		HandleLanding();
	}
	else if (m_drop_status < 0 || m_dock_status < 0)
	{
		HandleUndocking();
	}

	// Change the heading of the ship.
	ChangeHeading();

	// Move the ship to new coordinates
	Travel();

	// Check Hatches validity
	ConfirmHatches();

}


// Call this function to completely repair everything on the ship.
void CHSShip::TotalRepair(void)
{
	CHSEngSystem *cSys;

	// Not destroyed
	m_destroyed = FALSE;

	// Repair all systems
	for (cSys = m_systems.GetHead(); cSys; cSys = cSys->GetNext())
	{
		cSys->ReduceDamage(DMG_NONE);
		cSys->SetAttributeValue("STRESS", "0");
	}

	// Repair hull
	m_hull_points = GetMaxHullPoints();

	// Put the ship into space if it's not docked or dropped
	if (!m_docked && !m_dropped)
	{
		ResurrectMe();
	}
}

// Returns the current cloaking efficiency for the ship.
// If the ship cannot cloak or isn't cloaked, this is just 0.
// This value ranges from 0 - 1.00, where 1.00 is 100% visible.
float CHSShip::CloakingEffect(void)
{
	CHSSysCloak *cCloak;
	float rval;

	// Look for the cloaking device.
	cCloak = (CHSSysCloak *)m_systems.GetSystem(HSS_CLOAK);
	if (!cCloak)
	{
		return 1;
	}
	
	if (cCloak->GetEngaged())
	{
		rval = cCloak->GetEfficiency(TRUE);
		rval = (100 - rval) / 100;
		return rval;
	} else {
		return 1;
	}
}

float CHSShip::TachyonEffect(void)
{
	CHSSysTach *cTach;
	float rval;


	// Look for the Tachyon Sensor Array.
	cTach = (CHSSysTach *)m_systems.GetSystem(HSS_TACHYON);
	if (!cTach)
	{
		return 0;
	}

	rval = cTach->GetEfficiency(TRUE);
	rval = rval / 100;
	return rval;

}

// Used to query sensors, give messages to the ship consoles, etc.
void CHSShip::HandleSensors(void)
{
	CHSSysSensors *cSensors;
	SENSOR_CONTACT *cContact;
	char tbuf[512];
	char name[64];

	cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
	if (!cSensors)
		return;

	// Set the sensors to the first new contact.
	cContact = cSensors->GetFirstNewContact();

	// This is ok to do because the Next and First functions
	// return the same first contact.
	while((cContact = cSensors->GetNextNewContact()) != NULL)
	{
		// Special name handling for ships.
		if (cContact->m_obj->GetType() == HST_SHIP)
			sprintf(name, "the %s", cContact->m_obj->GetName());
		else
			strcpy(name, cContact->m_obj->GetName());

		if (cContact->status == DETECTED)
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unidentified contact has appeared on sensors.",
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, ANSI_HILITE,
			cContact->m_id, ANSI_NORMAL, 
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL);
		else if (cContact->status == UPDATED)
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Contact identified as %s.",
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, ANSI_HILITE,
			cContact->m_id, ANSI_NORMAL,
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, name);
		else
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - New contact identified as %s.",
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, ANSI_HILITE,
			cContact->m_id, ANSI_NORMAL, 
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, name);
		NotifyConsoles(tbuf, MSG_SENSOR);
	}

	// Now lost contacts
	// Set the sensors to the first lost contact.
	cContact = cSensors->GetFirstLostContact();

	// This is ok to do because the Next and First functions
	// return the same first contact.
	while((cContact = cSensors->GetNextLostContact()) != NULL)
	{
		// Special name handling for ships.
		if (cContact->m_obj->GetType() == HST_SHIP)
			sprintf(name, "The %s", cContact->m_obj->GetName());
		else
		{
			strncpy(name, cContact->m_obj->GetName(), 63);
			name[63] = '\0';
		}
		if (cContact->status == DETECTED)
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Unidentified contact has been lost from sensors.",
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, ANSI_HILITE,
			cContact->m_id, ANSI_NORMAL,
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL);
		else
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - %s has been lost from sensors.",
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, ANSI_HILITE,
			cContact->m_id, ANSI_NORMAL,
			cContact->m_obj->GetObjectColor(), ANSI_NORMAL, name);

		NotifyConsoles(tbuf, MSG_SENSOR);
	}
}

// Sends a message to all consoles on a ship
void CHSShip::NotifyConsoles(char *strMsg, int msgLevel)
{
	int idx;

	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			m_console_array[idx]->HandleMessage(strMsg, msgLevel);
		}
	}
}

// Sends a message to all registered ship rooms
void CHSShip::NotifySrooms(char *strMsg)
{
	int idx;

	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (m_ship_rooms[idx] != NOTHING)
		{
			hsInterface.NotifyContents(m_ship_rooms[idx], strMsg);
		}
	}
}

// Returns the color of the ship object
char *CHSShip::GetObjectColor(void)
{
	static char tbuf[32];

	sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_RED);
	return tbuf;
}


// Returns the CHSConsole object in a given slot in the
// ship's console array.
CHSConsole *CHSShip::GetConsole(UINT slot)
{
	if ((slot < 0) || (slot >= MAX_SHIP_CONSOLES))
		return NULL;

	return m_console_array[slot];
}

// Returns TRUE if any of the ship's console are locked
// on the given target object.
BOOL CHSShip::IsObjectLocked(CHSObject *cObj)
{
	if (!cObj)
		return FALSE;

	// Run through our consoles, checking their locks.
	int idx;
	CHSObject *cTarget;
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			cTarget = m_console_array[idx]->GetObjectLock();
			if (cTarget &&
				(cTarget->GetDbref() == cObj->GetDbref()))
				return TRUE;
		}
	}
	return FALSE;
}

// Gives a report of sensor contacts to a player.  An object 
// type can be specified to only display sensor contacts with
// a certain type of object class.  If this variable is not
// set, all types are displayed.
void CHSShip::GiveSensorReport(dbref player, HS_TYPE tType)
{
	CHSSysSensors *cSensors;
	SENSOR_CONTACT *cContact;
	int idx;
	BOOL bHeaderGiven;
	char tbuf[512];
	double rangelist[64];
	char *bufflist[64]; // Used to sort output
	UINT uBuffIdx;
    static char strSizes[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	float dDistance;
	char dDistanceS[64], dType[64];
	double dX, dY, dZ;
	char dXYHeading[64], dZHeading[64], dSpeed[64];
	int iXYBearing, iZBearing;
	char sizesymbol;
	char lockchar, cloakchar;
	BOOL bLinked;

	cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
	if (!cSensors)
	{
		hsStdError(player,
			"This vessel has no sensor array.");
		return;
	}

	if (!cSensors->GetCurrentPower())
	{
		hsStdError(player,
			"Sensor array is not currently online.");
		return;
	}

	// No contacts on sensors?
	if (!cSensors->NumContacts())
	{
		if (tType == HST_NOTYPE)
			hsStdError(player,
			"No contacts currently on sensors.");
		else
			hsStdError(player,
			"No contacts of that type currently on sensors.");
		return;
	}

	bufflist[0] = NULL;
	uBuffIdx = 0;

	// All looks good.  Give the report.
	bHeaderGiven = FALSE;
	for (idx = 0; idx < HS_MAX_CONTACTS; idx++)
	{
		// Grab the contact
		cContact = cSensors->GetContact(idx);
		if (!cContact)
			continue;

		// Store a pointer to the object for ease of use.
		CHSObject *cObj;
		cObj = cContact->m_obj;

		// Be sure it's the proper type.
		if (tType != HST_NOTYPE)
		{
			if (cObj->GetType() != tType)
				continue;
		}

		// The object is of our type, now give the header.
		// We do it here because if tType is not of NOTYPE,
		// we need to query the CHSObject to know what color
		// it displays as.
		if (!bHeaderGiven)
		{
			if (tType != HST_NOTYPE)
				sprintf(tbuf, 
				"%sSensor Contacts:%s %d",
				cObj->GetObjectColor(),
				ANSI_NORMAL, cSensors->NumContacts());
			else
				sprintf(tbuf,
				"%sContacts:%s %d",
				ANSI_HILITE, ANSI_NORMAL, cSensors->NumContacts());

			notify(player, tbuf);
			sprintf(tbuf,
				"%s%s-------------------------------------------------------------------------------%s",
				ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
			notify(player, tbuf);
			sprintf(tbuf,
				"%sC   ####  Name                     Bearing Range      Heading Speed  Type      %s",
				ANSI_YELLOW, ANSI_NORMAL);
			notify(player, tbuf);
			sprintf(tbuf,
				"%s-  ------ ------------------------ --- --- ---------- --- --- ------ ----------%s",
				ANSI_BLUE, ANSI_NORMAL);
			notify(player, tbuf);

			bHeaderGiven = TRUE;
		}

		// Calculate a variety of information.
		dX = cObj->GetX();
		dY = cObj->GetY();
		dZ = cObj->GetZ();

		// Distance
		dDistance = Dist3D(m_x, m_y, m_z, dX, dY, dZ);
		sprintf(dDistanceS, "%.4f", dDistance);

				
		// Bearing
		iXYBearing = XYAngle(m_x, m_y, dX, dY);
		iZBearing  = ZAngle(m_x, m_y, m_z, dX, dY, dZ);

		// Symbol representing the size
		if (cObj->GetSize() > 27)
			sizesymbol = '+';
		else
			sizesymbol = strSizes[cObj->GetSize() - 1];

		// Is it a ship, and is it locked on us?
		bLinked = FALSE;
		if (cObj->GetType() == HST_SHIP)
		{
			CHSShip *source;

			source = (CHSShip *) cObj;
			if (source->IsObjectLocked(this))
				lockchar = '*';
			else
				lockchar = ' ';
			if (source->CloakingEffect() < 1)
				cloakchar = '~';
			else
				cloakchar = ' ';

			// Do we have a boarding link with the object?
			bLinked = FALSE;
			for (int jdx = 0; jdx < MAX_HATCHES; jdx++)
			{
				if (m_hatches[jdx] && 
					(m_hatches[jdx]->m_targetObj ==
					cObj->GetDbref()))
				{
					bLinked = TRUE;
					break;
				}
			}
		} else {
			lockchar = ' ';
			cloakchar = ' '; }
		
		switch(cObj->GetType())
		{
		case HST_SHIP:
			sprintf(dType, "Base/Ship");
			break;

		case HST_PLANET:
			sprintf(dType, "Planet");
			break;

		case HST_BLACKHOLE:
			sprintf(dType, "Black Hole");
			break;

		case HST_WORMHOLE:
			sprintf(dType, "Wormhole");
			break;

		case HST_MISSILE:
			sprintf(dType, "Missile");
			break;

		case HST_NEBULA:
			sprintf(dType, "Nebula");
			break;

		case HST_ASTEROID:
			sprintf(dType, "Asteroids");
			break;

		default:
			sprintf(dType, "Unknown");
			break;
		}

		// Based on the type of object, given one of a variety
		// of print outs.  Not all objects may be supported, in
		// which case a generic report is given.
		bufflist[uBuffIdx] = new char[512];
		rangelist[uBuffIdx] = dDistance;


/*		switch(cObj->GetType())
		{
			case HST_SHIP:
				CHSShip *cShip;
				cShip = (CHSShip *)cObj;

				sprintf(bufflist[uBuffIdx],
				"%c%s%c%s%c%s[%s%d%s]%s%s%s%c%s%-24s %s%sB:%s %-3d%s%sm%s%-3d %s%sR:%s %-8.0f %s%sH:%s %-3d%s%sm%s%-3d %s%sS:%s %-7.0d",
				sizesymbol, 
				ANSI_HILITE, lockchar, ANSI_NORMAL,
				bLinked ? 'd' : ' ',
				cObj->GetObjectColor(), ANSI_NORMAL, 
				cContact->m_id, cObj->GetObjectColor(), ANSI_NORMAL,
				ANSI_HILITE, ANSI_BLUE, cloakchar, ANSI_NORMAL,
				cContact->status == DETECTED ? "Unknown" : cObj->GetName(),
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				iXYBearing,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				iZBearing,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				dDistance,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				cShip->GetXYHeading(),
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				cShip->GetZHeading(),
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				cShip->GetSpeed());
				break;

			case HST_PLANET:
				sprintf(bufflist[uBuffIdx],
				"%c  %s[%s%d%s]%s %-24s %s%sB:%s %-3d%s%sm%s%-3d %s%sR:%s %-8.0f %s%sSize:%s %d",
				sizesymbol,
				cObj->GetObjectColor(), ANSI_NORMAL, 
				cContact->m_id, cObj->GetObjectColor(), ANSI_NORMAL, 
				cContact->status == DETECTED ? "Unknown" : cObj->GetName(),
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				iXYBearing,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				iZBearing, 
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				dDistance,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				cObj->GetSize());
				break;

			default:
				sprintf(bufflist[uBuffIdx],
				"%c  %s[%s%d%s]%s %-24s %s%sB:%s %-3d%s%sm%s%-3d %s%sR:%s %.0f",
				sizesymbol,
				cObj->GetObjectColor(), ANSI_NORMAL, 
				cContact->m_id, cObj->GetObjectColor(), ANSI_NORMAL, 
				cContact->status == DETECTED ? "Unknown" : cObj->GetName(),
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				iXYBearing,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				iZBearing, 
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, 
				dDistance,
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
				break;
		} */

			CHSShip *cShip;
			BOOL NoEngines, Jumping;
			Jumping = FALSE;
			NoEngines = FALSE;
			if (cObj->GetType() == HST_SHIP)
			{
				cShip = (CHSShip *)cObj;
				sprintf(dSpeed, "%d", cShip->GetSpeed());
				sprintf(dXYHeading, "%d", cShip->GetXYHeading());
				sprintf(dZHeading, "%d", cShip->GetZHeading());
				CHSJumpDrive *cJumpers;
				CHSSysEngines *cEngines;
				cEngines = (CHSSysEngines *)cShip->m_systems.GetSystem(HSS_ENGINES);
				if (!cEngines)
					NoEngines = TRUE;
				cJumpers = (CHSJumpDrive *)cShip->m_systems.GetSystem(HSS_JUMP_DRIVE);
				if (cJumpers)
					if (cJumpers->GetEngaged())
						Jumping = TRUE;
			} else {
				cShip = NULL;
				sprintf(dSpeed, "--");
				sprintf(dXYHeading, "--");
				sprintf(dZHeading, "--");
			}
					switch(cObj->GetType())
		{
		case HST_SHIP:
			if (NoEngines)
				sprintf(dType, "Base");
			else
				sprintf(dType, "Ship");
			break;

		case HST_PLANET:
			sprintf(dType, "Planet");
			break;

		case HST_BLACKHOLE:
			sprintf(dType, "Black Hole");
			break;

		case HST_WORMHOLE:
			sprintf(dType, "Wormhole");
			break;

		case HST_MISSILE:
			sprintf(dType, "Missile");
			break;

		case HST_NEBULA:
			sprintf(dType, "Nebula");
			break;

		case HST_ASTEROID:
			sprintf(dType, "Asteroids");
			break;

		default:
			sprintf(dType, "Unknown");
			break;
		}


			sprintf(bufflist[uBuffIdx],
			"%c%s%c%s%c%s[%s%d%s]%s%s%c%s%-24s %3d %3d %10s %3s %3s%c%-6s %-10s",
			sizesymbol, 
			ANSI_HILITE, lockchar, ANSI_NORMAL,
			bLinked ? 'd' : ' ',
			cObj->GetObjectColor(), ANSI_NORMAL, 
			cContact->m_id, cObj->GetObjectColor(),
			ANSI_HILITE, ANSI_BLUE, cloakchar, ANSI_NORMAL,
			cContact->status == DETECTED ? "Unknown" : cObj->GetName(),
			iXYBearing,
			iZBearing,
			dDistanceS,
			dXYHeading,
			dZHeading,
			Jumping ? 'J' : ' ',
			dSpeed,
			dType);
		

		uBuffIdx++;
	} 

	// Now sort the contacts according to range.  This is your
	// pretty standard, very inefficient bubble-sort.
	int i, j;
	char *ptr;
	for (i = 0; (UINT)i < uBuffIdx; i++)
	{
		for (j = i+1; (UINT)j < uBuffIdx; j++)
		{
			// Compare element ranges
			if (rangelist[i] > rangelist[j])
			{
				// Swap elements
				dDistance = rangelist[i];
				ptr = bufflist[i];

				rangelist[i] = rangelist[j];
				bufflist[i] = bufflist[j];

				bufflist[j] = ptr;
				rangelist[j] = dDistance;
			}
		}
	}

	// List sorted, now print it out to the player
	for (i = 0; (UINT)i < uBuffIdx; i++)
	{
		notify(player, bufflist[i]);

		// Free the buffer
		delete [] bufflist[i];
	}

	// Print the closing line
	sprintf(tbuf,
		"%s%s-------------------------------------------------------------------------------%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
}

// Handles life support for the vessel, which basically
// just means checking to see if there's any life support
// left .. oh, and killing players.
void CHSShip::HandleLifeSupport(void)
{
	CHSSysLifeSupport *cLife;
	char tbuf[256];

	// Find the system
	cLife = (CHSSysLifeSupport *)m_systems.GetSystem(HSS_LIFE_SUPPORT);
	if (!cLife || !IsActive())
		return;

	// Is there air left?
	if (cLife->GetAirLeft() <= 0)
	{
		if (bGhosted)
		{
			// Do nothing
			return;
		}

		// Kill people on the ship.
		bGhosted = TRUE;
		sprintf(tbuf,
			"%s%s%s*%s LIFE SUPPORT SYSTEMS FAILURE %s%s%s*%s",
			ANSI_HILITE, ANSI_BLINK, ANSI_RED, ANSI_NORMAL,
			ANSI_HILITE, ANSI_BLINK, ANSI_RED, ANSI_NORMAL);
		NotifySrooms(tbuf);
		if (!hsInterface.HasFlag(m_objnum, TYPE_THING, THING_HSPACE_SIM))
			KillShipCrew("YOU HAVE SUFFOCATED!");
	}
	else if (cLife->GetCurrentPower() == 0)
	{
		double remainder;
		int airleft;

		airleft = (int)(cLife->GetAirLeft() / 10);
		remainder = (cLife->GetAirLeft() * .1) - airleft;
		
		// Send out a warning each 10 percent drop in life support.
		// Because life support may be decremented each cycle by
		// something that doesn't always add up to 1%, we need to
		// account for some minor error.  For example, decreasing
		// 1% over 3 cycles is .3333 each cycle, for a total of
		// .9999% over 3 cycles.  That gives an error of .0001%,
		// so the remainder will never be exactly 0.
		if (remainder < .001) 
		{
			sprintf(tbuf,
				"%s%s%s*%s %s%sWARNING%s %s%s%s*%s Life support systems are failing.",
				ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL,
				ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
				ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL);
			NotifySrooms(tbuf);
		}
	}
	else if (bGhosted)
		bGhosted = FALSE;
}

// Returns the maximum hull points for the vessel
UINT CHSShip::GetMaxHullPoints(void)
{
	return (m_classinfo->m_maxhull ? m_classinfo->m_maxhull : 1);
}

// Returns the current hull points for the vessel
UINT CHSShip::GetHullPoints(void)
{
	return m_hull_points;
}

// Returns the drop capable status of the ship
BOOL CHSShip::CanDrop(void)
{
	if (m_can_drop)
		return *m_can_drop;
	else
		return m_classinfo->m_can_drop;
}

BOOL CHSShip::IsSpacedock(void)
{
	if (m_spacedock)
		return *m_spacedock;
	else
		return m_classinfo->m_spacedock;
}

// Ships are represented as a plus sign.
char CHSShip::GetObjectCharacter(void)
{
	return '+';
}

// Retrieves a landing location (a bay) given a slot number (0 - n)
CHSLandingLoc *CHSShip::GetLandingLoc(int which)
{
	if (which < 0 || which >= HS_MAX_LANDING_LOCS)
		return NULL;

	return m_landing_locs[which];
}

// Retrieves a landing location (a bay) given a slot number (0 - n)
CHSHatch *CHSShip::GetHatch(int which)
{
	if (which < 0 || which >= MAX_HATCHES)
		return NULL;

	return m_hatches[which];
}


// Moves the ship object, which represents the ship, to a location.
void CHSShip::MoveShipObject(dbref whereto)
{
	if (!hsInterface.ValidObject(whereto))
		return;

	if (m_objnum != NOTHING)
		moveto(m_objnum, whereto);
}

// Adds a landing location (a bay) to the ship.
BOOL CHSShip::AddLandingLoc(dbref room)
{
	CHSLandingLoc *cLoc;
	char tbuf[256]; // Temp string storage

	// Grab a new landing loc
	cLoc = NewLandingLoc();
	if (!cLoc)
	{
		sprintf(tbuf, 
			"WARNING: Maximum landing locations reached for ship #%d.",
			m_objnum);
		hs_log(tbuf);
		return FALSE;
	}

	// Initialize some attrs
	cLoc->SetActive(TRUE);
	cLoc->objnum = room;
	cLoc->SetOwnerObject(this);

	// Set the flag
	hsInterface.SetFlag(room, ROOM_HSPACE_LANDINGLOC);

	// Set the planet attribute
	hsInterface.AtrAdd(room, "HSDB_SHIP",
		unsafe_tprintf("#%d", m_objnum), GOD);

	return TRUE;
}

// Allocates a new landing location, if possible, in the
// ship's landing bay array.
CHSLandingLoc *CHSShip::NewLandingLoc(void)
{
	int idx;

	// Check to see if it'll fit
	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
	{
		if (m_landing_locs[idx] == NULL)
			break;
	}

	if (idx == HS_MAX_LANDING_LOCS)
	{
		return NULL;
	}
	m_landing_locs[idx] = new CHSLandingLoc;
	return m_landing_locs[idx];
}

CHSHatch *CHSShip::NewHatch(void)
{
	int idx;

	// Check to see if it'll fit
	for (idx = 0; idx < MAX_HATCHES; idx++)
	{
		if (m_hatches[idx] == NULL)
			break;
	}

	if (idx == MAX_HATCHES)
	{
		return NULL;
	}
	m_hatches[idx] = new CHSHatch;
	return m_hatches[idx];
}


// Call this function to find a landing location based on its
// room number.
CHSLandingLoc *CHSShip::FindLandingLoc(dbref objnum)
{
	int idx;

	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
	{
		if (m_landing_locs[idx] &&
			m_landing_locs[idx]->objnum == objnum)
			return m_landing_locs[idx];
	}
	return NULL;
}

CHSHatch *CHSShip::FindHatch(dbref objnum)
{
	int idx;

	for (idx = 0; idx < MAX_HATCHES; idx++)
	{
		if (m_hatches[idx] &&
			m_hatches[idx]->objnum == objnum)
			return m_hatches[idx];
	}
	return NULL;
}

// Overrides the base class GetSize() because we derive our
// size from the class information.
UINT CHSShip::GetSize(void)
{
	return m_classinfo->m_size;
}

// Takes a message and propogates it to the consoles.  If the
// cObj variable is not NULL, the ship will attempt to find
// the sensor contact for that object and prefix the message
// with a contact id.  The long pointer points to whatever
// data were passed to the function.
void CHSShip::HandleMessage(char *lpstrMsg,
							int msgType,
							long *data)
{
	SENSOR_CONTACT *cContact;
	CHSSysSensors *cSensors;
	char tbuf[512];

	if(!lpstrMsg)
		return;

	if(!data)
		return;

	// Determine message type
	switch(msgType)
	{
		case MSG_SENSOR:
		case MSG_GENERAL:
		case MSG_ENGINEERING:
		case MSG_COMBAT:
		        if (strlen(lpstrMsg) > 400)
			    lpstrMsg[400] = '\0';

			CHSObject *cObj;
                        cObj = (CHSObject *)data;

			// Find our sensors
			cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
			if (!cSensors || !cObj)
			    cContact = NULL;
			else
			    cContact = cSensors->GetContact(cObj);

			if (cContact)
			{
			    sprintf(tbuf, "%s[%s%s%4d%s%s]%s - %s",
				    cObj->GetObjectColor(), ANSI_NORMAL,
				    ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
				    cObj->GetObjectColor(), ANSI_NORMAL,
				    lpstrMsg);
			}
			else
			    strcpy(tbuf, lpstrMsg);

			NotifyConsoles(tbuf, msgType);
			break;

		case MSG_COMMUNICATION:
			HandleCommMsg(lpstrMsg, data);
			break;
	}
}

// Runs through the boarding links, confirming their
// validity.
void CHSShip::ConfirmHatches(void)
{
	int idx;
	BOOL bBreak;
	CHSShip *cShip;
	double sX, sY, sZ; // Our coords
	double tX, tY, tZ; // Other ship's coords
	double dDist;

	sX = GetX();
	sY = GetY();
	sZ = GetZ();

	// Run through boarding links, checking conditions.
	for (idx = 0; idx < MAX_HATCHES; idx++)
	{
		if (!m_hatches[idx])
			continue;
		cShip = (CHSShip *)dbHSDB.FindObject(m_hatches[idx]->m_targetObj);

		if (!cShip)
			continue;

		bBreak = FALSE; // Don't break link by default.

		tX = cShip->GetX();
		tY = cShip->GetY();
		tZ = cShip->GetZ();

		// Check to see if the other ship is destroyed
		if (cShip->IsDestroyed())
			bBreak = TRUE;

		// Check to see if we're still in space
		else if (!IsActive())
			bBreak = TRUE;

		// Are we dropping or docking?
		else if (m_drop_status || m_dock_status)
			bBreak = TRUE;

		// Check to see if the other guy is still
		// in space.
		else if (!cShip->IsActive())
			bBreak = TRUE;

		// Check distance
		else
		{
			dDist = Dist3D(sX, sY, sZ,
						   tX, tY, tZ);
			if (dDist > HSCONF.max_board_dist)
				bBreak = TRUE;
		}


		if (bBreak)
		{
			CHSHatch *lHatch;
			lHatch = m_hatches[idx];

			int port;
			port = lHatch->m_targetHatch;

			CHSHatch *cHatch;
			cHatch = cShip->GetHatch(port);
			if (cHatch)
			{
				cHatch->m_targetObj = NOTHING;
				cHatch->m_targetHatch = NOTHING;
				cHatch->HandleMessage(unsafe_tprintf("%s is disconnected.", Name(cHatch->objnum)), MSG_GENERAL);
				hsInterface.UnlinkExits(cHatch->objnum, lHatch->objnum);
			} else
				s_Location(lHatch->objnum, NOTHING);

			lHatch->m_targetObj = NOTHING;
			lHatch->m_targetHatch = NOTHING;
			lHatch->HandleMessage(unsafe_tprintf("%s is disconnected.", Name(lHatch->objnum)), MSG_GENERAL);
			
			char tbuf[256];

			sprintf(tbuf, "Docking couplings on hatch %d disengaged.", port + 1);
			cShip->NotifyConsoles(tbuf, MSG_GENERAL);

			NotifyConsoles(unsafe_tprintf("Docking couplings on hatch %d disengaged.", idx + 1), MSG_GENERAL);
		}
	}
}

// Indicates if the ship is destroyed or not.
BOOL CHSShip::IsDestroyed(void)
{
	return m_destroyed;
}

// Allows a player to disembark from the ship.  This could
// be if the ship is docked, dropped, or boardlinked.
void CHSShip::DisembarkPlayer(dbref player, int id)
{
	dbref dbLoc NOTHING;
	dbref dbDestObj;

	// If the ship is docked or dropped, find the location
	// of the ship object, and put the player there.
	if (m_docked || m_dropped)
	{
		if (m_objnum == NOTHING)
		{
			notify(player,
				"This ship has no ship object, so I don't know where to put you.");
			return;
		}
		dbLoc = Location_hspace(m_objnum);

		dbDestObj = m_objlocation;
	}
	if (dbLoc == NOTHING) {
		notify(player,
			"You cannot disembark from the vessel at this time.");
		return;
	}
	// At this point, dbLoc should be set to a
	// location.
	if (!hsInterface.ValidObject(dbLoc))
	{
		notify(player,
			"Erg.  The location of this vessel's ship object is invalid.");
		return;
	}

	// Tell the player of the disembarking
	notify(player,
		"You disembark from the vessel.");
	
	// Tell players in the other location of the disembarking.
	hsInterface.NotifyContents(dbLoc,
		unsafe_tprintf("%s disembarks from the %s.",
		Name(player), GetName()));

	dbref dbPrevLoc = Location_hspace(player);

	// Move the player
	moveto(player, dbLoc);

	// Tell players in the previous location that the
	// player disembarked.
	hsInterface.NotifyContents(dbPrevLoc,
		unsafe_tprintf("%s disembarks from the vessel.",
		Name(player)));

	// Set the location attribute on the player.
	hsInterface.AtrAdd(player, "HSPACE_LOCATION",
		unsafe_tprintf("#%d", dbDestObj), GOD, AF_MDARK | AF_WIZARD);
}

// Returns the boarding code for the vessel.
char *CHSShip::GetBoardingCode(void)
{
	return m_boarding_code;
}

// Returns TRUE or FALSE to indicate whether the ship is
// landed or docked.
BOOL CHSShip::Landed(void)
{
	if ((m_docked || m_dropped) && !m_in_space)
		return TRUE;

	return FALSE;
}

// When another object locks onto the ship, this function
// can be called to handle that situation.
void CHSShip::HandleLock(CHSObject *cLocker, BOOL bStat)
{
	SENSOR_CONTACT *cContact;
	CHSSysSensors  *cSensors;
	char tbuf[256];

	// bStat could be TRUE for locking, or FALSE
	// for unlocking.

	// Find our sensors
	cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
	if (!cSensors)
		return;

	// Find the contact on sensors
	cContact = cSensors->GetContact(cLocker);

	// If not on sensors, give a general message
	if (!cContact)
	{
		if (bStat)
			sprintf(tbuf,
			"%s%s-%s An unknown contact has locked weapons!",
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL);
		else
			sprintf(tbuf,
			"%s%s-%s An unknown contact has released weapons lock.",
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL);
	}
	else if (cContact->status == DETECTED)
	{
		if (bStat)
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Contact has locked weapons!",
			cLocker->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
			cLocker->GetObjectColor(), ANSI_NORMAL);
		else
			sprintf(tbuf,
			"%s[%s%s%d%s%s]%s - Contact has released weapons lock.",
			cLocker->GetObjectColor(), ANSI_NORMAL,
			ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
			cLocker->GetObjectColor(), ANSI_NORMAL);
	}
	else
	{
		char name[64];
		if (cLocker->GetType() == HST_SHIP)
			sprintf(name, "The %s", cLocker->GetName());
		else
			strcpy(name, cLocker->GetName());

		if (bStat)
			sprintf(tbuf,
				"%s[%s%s%d%s%s]%s - %s has locked weapons!",
				cLocker->GetObjectColor(), ANSI_NORMAL,
				ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
				cLocker->GetObjectColor(), ANSI_NORMAL,
				name);
		else
			sprintf(tbuf,
				"%s[%s%s%d%s%s]%s - %s has released weapons lock.",
				cLocker->GetObjectColor(), ANSI_NORMAL,
				ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
				cLocker->GetObjectColor(), ANSI_NORMAL,
				name);
	}

	// Notify the consoles of this message
	NotifyConsoles(tbuf, MSG_COMBAT);
}

// Handles damage from an attacker, with a weapon, and a given
// level of damage.
void CHSShip::HandleDamage(CHSObject *cSource,
						   CHSWeapon *cWeapon,
						   int strength,
						   CHSConsole *cConsole,
						   int iSysType)
{
	dbref dbAttacker;
	CHSSysShield *cShield;
	char msgUs[256];  // Message gets sent to our ship
	char msgThem[256];// Message gets sent back to attacker
	CHSLaser *cLaser;

	cLaser = NULL;

	// Get the attacking player.
	dbAttacker = hsInterface.ConsoleUser(cConsole->m_objnum);

	// Determine which shield is hit.  To do that, we
	// supply the XY and Z angles from the attacker to
	// us.
	int iXYAngle;
	int iZAngle;
	
	iXYAngle = XYAngle(cSource->GetX(), cSource->GetY(),
		GetX(), GetY());

	iZAngle = ZAngle(cSource->GetX(), cSource->GetY(), cSource->GetZ(),
		GetX(), GetY(), GetZ());

	cShield = DetermineHitShield(iXYAngle, iZAngle);

	if (cWeapon->GetType() == HSW_LASER) {
		cLaser = (CHSLaser *)cWeapon; }

	// It's possible the ship has no shields
	char strShieldType[32];
	if (cShield)
	{
		// Tell the shield to take damage, and record
		// how much damage was not handled.
		strength = cShield->DoDamage(strength);
		int shieldtype = cShield->GetShieldType();

		if (shieldtype == ST_DEFLECTOR)
			strcpy(strShieldType, "deflected");
		else
			strcpy(strShieldType, "absorbed");
	}
	if (cShield && strength <= 0)
	{
		// Shields absorbed damage

		// Customize message for type of weapon.
		switch(cWeapon->GetType())
		{
			case HSW_LASER:
				sprintf(msgUs,
					"%s have %s an incoming energy blast.",
					cShield->GetName(), strShieldType);
				sprintf(msgThem,
					"Your energy blast has been %s by the enemy's shields.",
					strShieldType);
				break;

			case HSW_MISSILE:
				sprintf(msgUs,
					"Our shields have %s the impact from an inbound missile.",
					strShieldType);
				sprintf(msgThem,
					"The impact of your missile has been %s by the enemy's shields.",
					strShieldType);
				break;
		}

		// Give messages
		HandleMessage(msgUs, MSG_COMBAT, (long *)cSource);

		if (dbAttacker != NOTHING)
			hsStdError(dbAttacker, msgThem);
	}
	else
	{
		// Some damage to the hull occurs.  If it's greater
		// than a critical hit value, then systems take
		// damage.

		BOOL HitSys;
		// Deduct from hull
		if (cLaser) {
			if (!cLaser->NoHull())
				m_hull_points -= strength; 
		} else
			m_hull_points -= strength;

		
		double CritPerc;

		if (cLaser) {
			if (cLaser->NoHull()) {
			CritPerc = .2 / cLaser->GetAccuracy();
			HitSys = getrandom(2);
			} else {
			CritPerc = .1;
			HitSys = 1; }
		} else {
			CritPerc = .1;
			HitSys = 1; }


		// Is hull < 0?
		if (m_hull_points < 0)
		{
			ExplodeMe();
			if (!hsInterface.HasFlag(m_objnum, TYPE_THING, THING_HSPACE_SIM))
				KillShipCrew("YOU HAVE BEEN BLOWN TO TINY BITS!");
// FIXME
			// Trigger akill
			did_it(dbAttacker, m_objnum, A_KILL, NULL, A_OKILL,
				NULL, A_AKILL, (char **)NULL, 0);
		}
		else
		{
			// Was the hit strong enough to do critical
			// damage?  Critical damage is defined as
			// 1/10th the hull value.
			double dCritStrength;
			dCritStrength = m_hull_points * CritPerc;

			if (strength >= dCritStrength && HitSys)
			{
				// Damage a system.
				CHSEngSystem *cSys;

				// Try to get the system of choice
				if (iSysType != HSS_NOTYPE)
				{
					cSys = m_systems.GetSystem((HSS_TYPE)iSysType);

					// If the system wasn't found, grab a
					// random one.
					if (!cSys)
						cSys = m_systems.GetRandomSystem();
				}
				else
					cSys = m_systems.GetRandomSystem();
				if (!cSys)
				{
					// Odd.  No system found?  Just give
					// a hull-like message.
					notify(1, "NO SYSTEM");
					switch(cWeapon->GetType())
					{
						case HSW_LASER:
							if (cLaser->NoHull()) {
							strcpy(msgUs,
								"An incoming energy blast has not damaged any systems.");
							strcpy(msgThem,
								"Your energy blast has not damaged any systems.");
							} else {
								strcpy(msgUs,
									"An incoming energy blast has damaged the hull.");
								strcpy(msgThem,
									"Your energy blast has landed damage to the enemy's hull."); }
							break;

						case HSW_MISSILE:
							strcpy(msgUs,
								"An inbound missile has landed damage to our hull.");
							strcpy(msgThem,
								"Your missile has landed damage to the enemy's hull.");
							break;
					}
				}
				else
				{
					// Give a message
					switch(cWeapon->GetType())
					{
						case HSW_LASER:
							sprintf(msgUs,
								"An incoming energy blast has damaged the %s.",
								cSys->GetName());
							sprintf(msgThem,
								"Your energy blast has landed damage to the enemy's %s.",
								cSys->GetName());
							break;

						case HSW_MISSILE:
							sprintf(msgUs,
								"An inbound missile has landed damage to our %s.",
								cSys->GetName());
							sprintf(msgThem,
								"Your missile has landed damage to the enemy's %s.",
								cSys->GetName());
							break;
					}

					// Damage the system
					cSys->DoDamage();
				}

				HandleMessage(msgUs, MSG_COMBAT, (long *)cSource);

				if (dbAttacker != NOTHING)
					hsStdError(dbAttacker, msgThem);
			}
			else
			{
				// Just hull damage
				switch(cWeapon->GetType())
				{
					case HSW_LASER:
						if (cLaser->NoHull()) {
							strcpy(msgUs,
								"An incoming energy blast has not damaged any systems.");
							strcpy(msgThem,
								"Your energy blast has not damaged any systems.");
						} else {
							strcpy(msgUs,
								"An incoming energy blast has damaged the hull.");
							strcpy(msgThem,
								"Your energy blast has landed damage to the enemy's hull."); }
						break;

					case HSW_MISSILE:
						strcpy(msgUs,
							"An inbound missile has landed damage to our hull.");
						strcpy(msgThem,
							"Your missile has landed damage to the enemy's hull.");
						break;
				}

				HandleMessage(msgUs, MSG_COMBAT, (long *)cSource);

				if (dbAttacker != NOTHING)
					hsStdError(dbAttacker, msgThem);
			}
		}
	}
}

// Can be used to explode the ship, for whatever reason.
void CHSShip::ExplodeMe(void)
{
	CHSUniverse *uSource;
	char tbuf[128];

	// Find my universe
	uSource = uaUniverses.FindUniverse(m_uid);
	if (uSource)
	{
		// Give out a message
		sprintf(tbuf,
			"You gaze in awe as the %s explodes before your eyes.",
			GetName());

		uSource->SendContactMessage(tbuf, IDENTIFIED,
			this);

		// Remove me from active space.
		uSource->RemoveActiveObject(this);

		m_in_space = FALSE;
	}

	// I'm destroyed
	m_destroyed = TRUE;

	// Clear any weapons locks consoles may have
	int idx;
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx] &&
			m_console_array[idx]->GetObjectLock())
			m_console_array[idx]->UnlockWeapons(NOTHING);
	}
}

// Can be used to resurrect the ship, which just means
// it's no longer destroyed and is now in active space
void CHSShip::ResurrectMe(void)
{
	CHSUniverse *uDest;

	m_destroyed = FALSE;

	// Put us into our universe, or a default one if
	// needed.
	uDest = uaUniverses.FindUniverse(m_uid);
	if (!uDest)
	{
		// Find the first available universe
		uDest = NULL;
		for (int idx = 0; idx < HS_MAX_UNIVERSES; idx++)
		{
			if ((uDest = uaUniverses.GetUniverse(idx)))
				break;
		}

		if (!uDest)
			return;

		// Since we had to find this universe, that also
		// means this ship is not in that universe, so add
		// it.
		uDest->AddObject(this);
	}

	// A universe exists, so add it.
	uDest->AddActiveObject(this);

	m_in_space = TRUE;

	// Be sure to verify the UID
	m_uid = uDest->GetID();
}

// Called by another object to give a scan report.  We
// decide what information to include in the scan.
void CHSShip::GiveScanReport(CHSObject *cScanner,
							 dbref player,
							 BOOL id)
{
	char tbuf[256];
	CHSSysEngines *cEngines;

	// Find our engines for speed info
	cEngines = (CHSSysEngines *)m_systems.GetSystem(HSS_ENGINES);

	// Give the header info
	sprintf(tbuf,
		"%s%s.-----------------------------------------------------.%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	char tbuf2[256];
	if(id)
		sprintf(tbuf2, "%s(%s)", GetName(), m_ident ? m_ident : "--");
	else
		sprintf(tbuf2, "Unknown");
	sprintf(tbuf,
		"%s%s|%s Vessel Scan Report %31s  %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, 
		tbuf2, 
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	sprintf(tbuf,
		"%s%s >---------------------------------------------------<%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	// Give the vessel class
	sprintf(tbuf,
		"%s%s|%s %-40s            %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		id ? m_classinfo->m_name : "Vessel Class Unknown",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	// Give coordinate and heading information.
	sprintf(tbuf,
		"%s%s| %sX:%s %9.0f%23s%s%sSize:%s %-3d        %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
		GetX(), " ",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		GetSize(),
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	sprintf(tbuf,
		"%s%s| %sY:%s %9.0f%23s%s%sHeading:%s %3d/%-3d %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
		GetY(), " ",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		m_current_xyheading, m_current_zheading,
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	sprintf(tbuf,
		"%s%s| %sZ:%s %9.0f%23s%s%sVelocity:%s %-6.0f %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
		GetZ(), " ",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		cEngines ? cEngines->GetCurrentSpeed() : 0.0,
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	// If we're identified, give systems information
	if (id)
	{
		CHSEngSystem *cSys;
		char strDamage[64];
		CHSSysShield *cFore;
		CHSSysShield *cAft;
		CHSSysShield *cStar;
		CHSSysShield *cPort;

		sprintf(tbuf,
			"%s%s >---------------------------------------------------<%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
		notify(player, tbuf);

		// Find the shields
		cFore = (CHSSysShield *)
			m_systems.GetSystem(HSS_FORE_SHIELD);
		cAft = (CHSSysShield *)
			m_systems.GetSystem(HSS_AFT_SHIELD);
		cStar = (CHSSysShield *)
			m_systems.GetSystem(HSS_STARBOARD_SHIELD);
		cPort = (CHSSysShield *)
			m_systems.GetSystem(HSS_PORT_SHIELD);

		// Give hull, shield info
		sprintf(tbuf, 
			"%s%s| %sHull Status               Shield Status             %s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_BLUE,
			ANSI_NORMAL);
		notify(player, tbuf);
		
		char strFore[8]; // Shield status strings
		char strAft[8];
		char strStar[8];
		char strPort[8];

		if (cFore)
		{
			sprintf(strFore, "%.0f%%",
				cFore->GetShieldPerc());
		}
		else
			strcpy(strFore, "N/A");

		if (cAft)
		{
			sprintf(strAft, "%.0f%%",
				cAft->GetShieldPerc());
		}
		else
			strcpy(strAft, "N/A");

		if (cPort)
		{
			sprintf(strPort, "%.0f%%",
				cPort->GetShieldPerc());
		}
		else
			strcpy(strPort, "N/A");

		if (cStar)
		{
			sprintf(strStar, "%.0f%%",
				cStar->GetShieldPerc());
		}
		else
			strcpy(strStar, "N/A");

		int hullperc;
		hullperc = (int)(100 * GetHullPoints() / 
			(float)GetMaxHullPoints());
		sprintf(tbuf,
			"%s%s|%s    %3d%%       %s%sF:%s %-4s  %s%sA:%s %-4s  %s%sP:%s %-4s  %s%sS:%s %-4s    %s%s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
			hullperc,
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
			strFore, 
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
			strAft, 
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, strPort, 
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
			strStar,
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
		notify(player, tbuf);


		// print systems header
		sprintf(tbuf,
			"%s%s|%53s|%s",
			ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL);
		notify(player, tbuf);

		sprintf(tbuf,
			"%s%s| %sSystem Name          Status       Damage    %8s%s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, " ", ANSI_BLUE,
			ANSI_NORMAL);
		notify(player, tbuf);

		// Run down the list of systems, giving info for
		// visible ones (or reactor).
		for (cSys = m_systems.GetHead(); cSys; cSys = cSys->GetNext())
		{
			if (cSys->IsVisible() ||
				cSys->GetType() == HSS_REACTOR)
			{
				// Setup damage indicator
				switch(cSys->GetDamageLevel())
				{
					case DMG_LIGHT:
						sprintf(strDamage,
							"%s%s  LIGHT   %s",
							ANSI_HILITE, ANSI_BGREEN, ANSI_NORMAL);
						break;

					case DMG_MEDIUM:
						sprintf(strDamage,
							"%s%s  MEDIUM  %s",
							ANSI_HILITE, ANSI_BYELLOW, ANSI_NORMAL);
						break;

					case DMG_HEAVY:
						sprintf(strDamage,
							"%s%s  HEAVY   %s",
							ANSI_HILITE, ANSI_BRED, ANSI_NORMAL);
						break;

					case DMG_INOPERABLE:
						sprintf(strDamage,
							"%s%sINOPERABLE%s",
							ANSI_HILITE, ANSI_BBLACK, ANSI_NORMAL);
						break;

					default:
						strcpy(strDamage, "None   ");
				}
				sprintf(tbuf,
					"%s%s|%s %-20s %-12s %-10s        %s%s|%s",
					ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
					cSys->GetName(),
					cSys->GetStatus(),
					strDamage,
					ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
				notify(player, tbuf);
			}
		}
	
		int nLocs;
		int idx;
		nLocs = 0;
		for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
		{
			if (m_landing_locs[idx])
				nLocs++;
		}

		// Do we have any landing locs to report?
		if (nLocs > 0)
		{
			// Print em baby.
			notify(player,
				unsafe_tprintf("%s%s|%53s|%s",
				ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL)); 
			sprintf(tbuf,
				"%s%s| %sLanding Locations:%s %-2d%31s%s%s|%s",
				ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
				nLocs, " ",
				ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
			notify(player, tbuf);	

			sprintf(tbuf,
				"%s%s| %s[%s##%s] Name                          Doors   Code     %s|%s",
				ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_WHITE, ANSI_GREEN,
				ANSI_BLUE, ANSI_NORMAL);
			notify(player, tbuf);	

			char strPadName[32];
			for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
			{
				if (m_landing_locs[idx])
				{
					strncpy(strPadName, Name(m_landing_locs[idx]->objnum),
						32);
					strPadName[23] = '\0';
					sprintf(tbuf,
						"%s%s|%s  %2d  %-29s %-6s  %3s      %s%s|%s",
						ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
						idx + 1,
						strPadName,
						m_landing_locs[idx]->IsActive() ? "Open" : "Closed",
						m_landing_locs[idx]->CodeRequired() ? "Yes" : "No",
						ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
					notify(player, tbuf);
				}
			}
		}

	}



	// Finish the report
	sprintf(tbuf,
		"%s%s`-----------------------------------------------------'%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	// Do we notify consoles that we're being scanned?
	if (HSCONF.notify_shipscan)
	{
		CHSSysSensors *cSensors;

		cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
		if (cSensors && cSensors->GetCurrentPower() > 0)
			HandleMessage("We are being scanned by another contact.",
				MSG_SENSOR, (long *)cScanner);
	}


}

// Sets the number of a specific type of missile
// in the missile storage bay.
BOOL CHSShip::SetNumMissiles(dbref player, int type, int max)
{
	// Valid type of weapon?
	CHSDBWeapon *cWeap;
	if (!(cWeap = waWeapons.GetWeapon(type, HSW_MISSILE)))
	{
		notify(player,
			"Invalid weapon type specified.");
		return FALSE;
	}

	if (!m_missile_bay.SetRemaining(type, max))
	{
		notify(player,
		"Failed to set storage value for that weapon type.");
		return FALSE;
	}
	else
		notify(player,
		"Missile storage value - set.");

	return TRUE;
}

// Retrieves the missile bay for the ship, or NULL if none.
CHSMissileBay *CHSShip::GetMissileBay(void)
{
	return &m_missile_bay;
}

// Runs through the registered rooms on the ship, moving the
// contents to the afterworld.  The ship is dead.  The msg
// is what is shown to the player/object when it is killed.
void CHSShip::KillShipCrew(const char *msg)
{
	dbref contents[50];
	int idx;
	int jdx;
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (m_ship_rooms[idx] != NOTHING)
		{
			hsInterface.GetContents(m_ship_rooms[idx],
				contents, NOTYPE);

			for (jdx = 0; contents[jdx] != -1; jdx++)
			{
				if (Typeof(contents[jdx]) != TYPE_PLAYER)
					continue;

				//flag_broadcast(0, PLAYER_HSPACE_ADMIN, "HSPACE: %s(#%i) has died.", Name(contents[jdx]),contents[jdx]);
				hs_log(unsafe_tprintf(">> %s(#%i) has died.", Name(contents[jdx]), contents[jdx]));

				notify(contents[jdx], msg);
				moveto(contents[jdx], HSCONF.afterworld);
			}
		}
	}
}

// Clones the ship, including all registered rooms and objects.
// It's handy!
dbref CHSShip::Clone(void)
{
	int rooms_visited[MAX_SHIP_ROOMS]; // Rooms already cloned.

	// Clone the ship object.
	dbref dbShipObj = hsInterface.CloneThing(m_objnum);

	// Allocate the new ship
	CHSShip *newShip;
	newShip = new CHSShip;
	newShip->SetDbref(dbShipObj);

	if (HSCONF.autozone)
		hsInterface.SetLock(dbShipObj, GOD, LOCK_ZONE);
	
	// Set the class info
	if (!newShip->SetClassInfo(m_class))
	{
		delete newShip;
		return NOTHING;
	}

	// Add the ship to the first available universe
	CHSUniverse *unDest;
	unDest = NULL;
	unDest = uaUniverses.FindUniverse(m_uid);
	unDest->AddObject(newShip);

	// Set the UID of the ship.
	newShip->SetUID(unDest->GetID());

	// Repair the ship
	newShip->TotalRepair();

	moveto(dbShipObj, Location_hspace(m_objnum));

	// Set droploc or dock loc
	if (m_dropped)
	{
		newShip->m_dropped = TRUE;
		newShip->m_in_space = FALSE;
	}
	if (m_docked)
	{
		newShip->m_in_space = FALSE;
		newShip->m_docked = TRUE;
	}

	// Start with the first registered room, and recursively
	// clone through the exits until all rooms are visited.
	rooms_visited[0] = NOTHING;
	CloneRoom(m_ship_rooms[0], rooms_visited, newShip);

	return dbShipObj;
}

// Recursive function to clone a specific room.  The return
// value is the dbref of the cloned room.
dbref CHSShip::CloneRoom(dbref room, int rooms_visited[],
						 CHSShip *newShip)
{
	// Check to see if the room is already visited.
	int idx;
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (rooms_visited[idx] == room)
		{
			// This room has been visited.  We're done here!
			return NOTHING;
		}
	}

	// Room is new, so clone it, find exits, and clone their
	// rooms.
// FIXME
	dbref dbNewRoom = hsInterface.CloneThing(room);
	// Add the new room to the ship
	newShip->AddSroom(dbNewRoom);

// FIXME
//    if (Zone(room) != m_objnum)
//		Zone(dbNewRoom) = Zone(room);

	// Get the dbref of the shipobj for the new ship.
	dbref dbShipObj = newShip->GetDbref();

	// Now see if this room has a SHIP attr on it.  If so,
	// set it to the new ship.
	char tbuf[32];
	if (hsInterface.AtrGet(dbNewRoom, "SHIP"))
	{
		sprintf(tbuf, "#%d", dbShipObj);
		hsInterface.AtrAdd(dbNewRoom, "SHIP", tbuf, GOD);
	}

	// See if this model room is designated as our bay of
	// our shipobj.  If so, then set the bay of the new
	// shipobj.
	if (hsInterface.AtrGet(m_objnum, "BAY"))
	{
		dbref dbOurBay = strtodbref(hsInterface.m_buffer);
		if (room == dbOurBay)
		{
			// Set the bay attr on the new ship obj to this
			// new room.
			sprintf(tbuf, "#%d", dbNewRoom);
			hsInterface.AtrAdd(dbShipObj, "BAY", tbuf, GOD);
		}
	}

	// Room cloned
	for (idx = 0; idx < MAX_SHIP_ROOMS; idx++)
	{
		if (rooms_visited[idx] == NOTHING)
		{
			rooms_visited[idx] = room;
			rooms_visited[idx + 1] = NOTHING;
			break;
		}
	}

	// Clone any consoles that might be in the room.  This
	// is not a very robust clone.  Only the console object
	// gets cloned and added to the ship.  It does not
	// clone the total information (weapons) of the console.
	// That needs to be added still.
	dbref thing;
	dbref dbNewConsole;
	DOLIST(thing, Contents(room))
	{
		if (hsInterface.HasFlag(thing, TYPE_THING, 
			THING_HSPACE_CONSOLE))
		{
			// Potential console found.  Look for it in our
			// console list.
			for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
			{
				if (m_console_array[idx] &&
					m_console_array[idx]->m_objnum == thing)
				{
					m_console_array[idx]->WriteObjectAttrs();
					// Definite console.  Clone it and add
					// it to the new ship.
					dbNewConsole = hsInterface.CloneThing(thing);
					newShip->AddConsole(dbNewConsole);
					moveto(dbNewConsole, dbNewRoom);

					m_console_array[idx]->ClearObjectAttrs();
				}
			}
		}
	}

	if (hsInterface.HasFlag(room, TYPE_ROOM, ROOM_HSPACE_LANDINGLOC))
		newShip->AddLandingLoc(dbNewRoom);

	// Now run through the exits on the model room, and 
	// clone them for the new room.
	dbref exit_m;
	dbref loc;
	dbref rRoom;
	dbref dbNewExit;
	for (exit_m = Exits(room); exit_m && exit_m != NOTHING;
		exit_m = Next(exit_m))
	{
			// Grab the location of the exit .. the next room
			// to clone.  Clone that room, and open an exit 
			// to it.
			rRoom = NOTHING;

			loc = Location_hspace(exit_m);
			if (!hsInterface.HasFlag(exit_m, TYPE_EXIT, EXIT_HSPACE_HATCH))
			{
				rRoom = CloneRoom(loc, rooms_visited, newShip);
			}

			CHSHatch *cHatch = NULL;
			if (hsInterface.HasFlag(exit_m, TYPE_EXIT, EXIT_HSPACE_HATCH))
			{
				cHatch = dbHSDB.FindHatch(exit_m);
				cHatch->WriteObjectAttrs();
			}

			if (rRoom != NOTHING || cHatch)
			{
				dbNewExit = hsInterface.CloneThing(exit_m);
				
				// Remove the cloned exit from the current
				// room 

				s_Exits(room,
					remove_first(Exits(room),
					dbNewExit));
				
				// Add the cloned exit to the cloned room
				
				s_Exits(dbNewRoom, 
					insert_first(Exits(dbNewRoom),
					dbNewExit));

				// Set the home of the exit to the new room
				
				s_Exits(dbNewExit, dbNewRoom);
				
				// The new exit doesn't lead anywhere
				
				s_Location(dbNewExit, NOTHING);
				
				// Link it in
			}
	
			if (cHatch)
			{
				newShip->AddHatch(dbNewExit);
				cHatch->ClearObjectAttrs();
			}

			if (rRoom != NOTHING)
			{
// FIXME
				//link_exit(Owner(dbNewExit), dbNewExit,
				//	rRoom);				
				s_Location(dbNewExit, rRoom);
				// Now!  Open a return exit from that room,
				// to us.
				dbref exit_opposite;

				// Search the location for a return exit to
				// this model room.
				for (exit_opposite = Exits(loc); 
					exit_opposite && exit_opposite != NOTHING;
					exit_opposite = Next(exit_opposite))
				{
					if (Location_hspace(exit_opposite) == room)
					{
					dbNewExit = hsInterface.CloneThing(exit_opposite);
					s_Exits(room,
						remove_first(Exits(room),
						dbNewExit));
					s_Exits(rRoom, 
					    insert_first(Exits(rRoom),
						dbNewExit));
					s_Exits(dbNewExit, rRoom);
					s_Location(dbNewExit, NOTHING);
					//link_exit(Owner(dbNewExit), dbNewExit,
					//	dbNewRoom);				
					s_Location(dbNewExit, dbNewRoom);
					}
				}
			}
	}


	return dbNewRoom;
}

// Handles any communications messages that were sent to the
// ship.  The long pointer points to a HSCOMM structure with
// the information needed.
void CHSShip::HandleCommMsg(const char *msg, long *data)
{
	HSCOMM *hsComm;
	char tbuf[1024];
	char strName[64];
	int idx;
	CHSSysSensors *cSensors;

	hsComm = (HSCOMM *)data;

	// Do we have a comm array, and is it on?
	CHSSysComm *cComm;
	cComm = (CHSSysComm *)m_systems.GetSystem(HSS_COMM);
	if (!cComm || !cComm->GetCurrentPower())
		return;

	// Setup the message of how it will be printed to people
	// who get it.
	hsComm->msg[900] = '\0'; // Just in case it's a long one.
	
	// Find the source object on sensors, if applicable.
	if (!hsComm->cObj)
		strcpy(strName, "Unknown");
	else
	{
		// Are we the source object?
		if (hsComm->cObj->GetDbref() == m_objnum)
			return;

		// Find our sensors.
		cSensors = (CHSSysSensors *)m_systems.GetSystem(HSS_SENSORS);
		if (!cSensors)
			strcpy(strName, "Unknown");
		else
		{
			SENSOR_CONTACT *cContact;
			cContact = cSensors->GetContact(hsComm->cObj);
			if (!cContact)
				strcpy(strName, "Unknown");
			else
			{
				// Determine if contact is id'd or not.
				if (cContact->status == IDENTIFIED)
					strcpy(strName, hsComm->cObj->GetName());
				else
					sprintf(strName, "%d", cContact->m_id);
			}
		}
	}
	sprintf(tbuf, "%s[COMM Frq:%s %.2f  %sSource:%s %s%s]%s\n%s\n%s*EOT*%s",
		ANSI_CYAN, ANSI_NORMAL, 
		hsComm->frq, 
		ANSI_CYAN, ANSI_NORMAL,
		strName, 
		ANSI_CYAN, ANSI_NORMAL,
		hsComm->msg,
		ANSI_CYAN, ANSI_NORMAL);

	// Run through our consoles, looking for comm consoles on this
	// frequency.
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++)
	{
		if (m_console_array[idx])
		{
			// Is it a COMM console, and is it on this frq?
			if (hsInterface.HasFlag(m_console_array[idx]->m_objnum,
				TYPE_THING, THING_HSPACE_COMM) &&
				m_console_array[idx]->OnFrq(hsComm->frq))
			{
				// Give the user the message.
				m_console_array[idx]->HandleMessage(tbuf, 
					MSG_COMMUNICATION);
			}
		}
	}
}

void CHSShip::InitSelfDestruct(dbref player, int Timer, char *BoardCode)
{
	if (GetBoardingCode())
	{
		if(strcmp(BoardCode, m_boarding_code)) {
			notify(player, unsafe_tprintf("%s%s-%s Invalid access code.",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL)); 
			return;	
		}
	}

	if(Timer < 1)
	{
		if(m_self_destruct_timer > 0)
		{
			NotifySrooms(unsafe_tprintf("%s%sSelf destruct aborted.%s",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL));
			NotifyConsoles(unsafe_tprintf("%s%s-%s Self destruct aborted.",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL), MSG_GENERAL);
			m_self_destruct_timer = 0;
		} else {
			notify(player, unsafe_tprintf("%s%s-%s Self destruct not in progress.",
				ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL));
		}
	} else {
		NotifySrooms(unsafe_tprintf("%s%s*** SELF DESTRUCT IN %d SECONDS ***%s",
			ANSI_HILITE, ANSI_RED, Timer, ANSI_NORMAL));
		NotifyConsoles(unsafe_tprintf("%s%s-%s Self destruct initiated.",
			ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL), MSG_GENERAL);
		m_self_destruct_timer = Timer;
	}
}
//
// CHSLandingLoc stuff
//
CHSHatch::CHSHatch(void)
{
	objnum = -1;
	m_ownerObj = NULL;
	m_targetObj = NOTHING;
	m_targetHatch = 0;
	m_clamped = 0;
}

CHSHatch::~CHSHatch(void)
{
	// Clear attributes and flags
	hsInterface.UnsetFlag(objnum, EXIT_HSPACE_HATCH);

	ClearObjectAttrs();
}

BOOL CHSHatch::LoadFromObject(dbref obj)
{
	objnum = obj;

	hsInterface.SetFlag(objnum, EXIT_HSPACE_HATCH);
	
	if (hsInterface.AtrGet(objnum, "HSDB_TARGETOBJ"))
		m_targetObj = strtodbref(hsInterface.m_buffer);

	if (hsInterface.AtrGet(objnum, "HSDB_TARGETHATCH"))
		m_targetHatch = atoi(hsInterface.m_buffer);

	if (hsInterface.AtrGet(objnum, "HSDB_CLAMPED"))
		m_clamped = atoi(hsInterface.m_buffer);

	ClearObjectAttrs();

	return TRUE;
}

// Who knows, might be used in the future.
void CHSHatch::WriteObjectAttrs(void)
{
	hsInterface.AtrAdd(objnum, "HSDB_TARGETOBJ", unsafe_tprintf("#%d", m_targetObj), GOD, AF_MDARK || AF_WIZARD);
	hsInterface.AtrAdd(objnum, "HSDB_TARGETHATCH", unsafe_tprintf("%d", m_targetHatch), GOD, AF_MDARK || AF_WIZARD);
	hsInterface.AtrAdd(objnum, "HSDB_CLAMPED", unsafe_tprintf("%d", m_clamped), GOD, AF_MDARK || AF_WIZARD);
}

// Future?
void CHSHatch::ClearObjectAttrs(void)
{
	hsInterface.AtrAdd(objnum, "HSDB_TARGETOBJ", NULL, GOD);
	hsInterface.AtrAdd(objnum, "HSDB_TARGETHATCH", NULL, GOD);
	hsInterface.AtrAdd(objnum, "HSDB_CLAMPED", NULL, GOD);
}

// All not used yet.
char *CHSHatch::GetAttributeValue(char *strName)
{
	static char rval[32];

	*rval = '\0';
	if (!strcasecmp(strName, "TARGETOBJ"))
		sprintf(rval, "#%d", m_targetObj);
	else if (!strcasecmp(strName, "TARGETHATCH"))
		sprintf(rval, "%d", m_targetHatch);
	else if (!strcasecmp(strName, "CLAMPED"))
		sprintf(rval, "%d", m_clamped);
	else
		return NULL;

	return rval;
}

// Unused currently, might be used in the future.
BOOL CHSHatch::SetAttributeValue(char *strName, char *strValue)
{
	// Match the name .. set the value
	if (!strcasecmp(strName, "TARGETHATCH"))
	{
		m_targetHatch = atoi(strValue);
		return TRUE;
	}
	if (!strcasecmp(strName, "CLAMPED"))
	{
		if (atoi(strValue) == 1 || atoi(strValue) == 0) {
			m_clamped = atoi(strValue);
			return TRUE;
		} else
			return FALSE;
	}
	else
		return FALSE;
}

// Handles a message, which for now is just giving it to the room the
// hatch is situated in.
void CHSHatch::HandleMessage(char *lpstrMsg, int type)
{
	hsInterface.NotifyContents(hsInterface.GetHome(objnum), lpstrMsg);
}

// Sets the CHSObject that owns the landing location.
void CHSHatch::SetOwnerObject(CHSObject *cObj)
{
	m_ownerObj = cObj;
}

// Returns the CHSObject that controls the landing location.
CHSObject *CHSHatch::GetOwnerObject(void)
{
	return m_ownerObj;
}
