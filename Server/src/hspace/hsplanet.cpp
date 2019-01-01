//
// hsplanet.cpp
//
// Handles all of the methods for the CHSPlanet object.
//
//
#include "hscopyright.h"
#include "hsobjects.h"
#include "hsuniverse.h"
#include <stdlib.h>
#include "hsinterface.h"
#include "hsutils.h"
#include "ansi.h"

#include <string.h>


//Constructor 
CHSPlanet::CHSPlanet(void)
{
    int idx;

    m_type = HST_PLANET;

    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++)
	m_landing_locs[idx] = NULL;
}

CHSPlanet::~CHSPlanet(void)
{
    int idx;

    // Delete landing locations
    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx])
	    delete m_landing_locs[idx];
    }
}

// Planets are green
char *CHSPlanet::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_GREEN);
    return tbuf;
}

// Planets are little o's
char CHSPlanet::GetObjectCharacter(void)
{
    return 'o';
}

// Overridden from the CHSCelestial class
BOOL CHSPlanet::HandleKey(int key, char *strValue, FILE * fp)
{
    CHSLandingLoc *cLoc;
    dbref objnum;

    // Find the key and handle it
    switch (key) {
    case HSK_SIZE:
	m_size = atoi(strValue);
	return TRUE;

    case HSK_LANDINGLOC:
	cLoc = NewLandingLoc();
	if (!cLoc) {
	    hs_log("ERROR: Couldn't add specified landing location.");
	} else {
	    objnum = atoi(strValue);
	    if (!hsInterface.ValidObject(objnum))
		return TRUE;

	    cLoc->SetOwnerObject(this);
	    cLoc->LoadFromObject(objnum);
	    // Set the planet attribute
	    hsInterface.AtrAdd(objnum, "HSDB_PLANET",
			       unsafe_tprintf("#%d", m_objnum), GOD);
	}
	return TRUE;

    default:			// Pass it up to CHSCelestial
	return (CHSCelestial::HandleKey(key, strValue, fp));
    }
}

// Overridden from the CHSCelestial class
void CHSPlanet::WriteToFile(FILE * fp)
{
    int idx;

    // Write base class info first, then our's.
    CHSCelestial::WriteToFile(fp);

    // Now our's
    // Landing locs
    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx]) {
	    fprintf(fp, "LANDINGLOC=%d\n", m_landing_locs[idx]->objnum);
	    m_landing_locs[idx]->WriteObjectAttrs();
	}
    }
}

// Call this function to find a landing location based on its
// room number.
CHSLandingLoc *CHSPlanet::FindLandingLoc(dbref objnum)
{
    int idx;

    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx]->objnum == objnum)
	    return m_landing_locs[idx];
    }
    return NULL;
}

// Clears attributes specific to the planet object
void CHSPlanet::ClearObjectAttrs()
{
    int idx;

    CHSCelestial::ClearObjectAttrs();

    // Clear landing location attrs
    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx])
	    m_landing_locs[idx]->ClearObjectAttrs();
    }
}

CHSLandingLoc *CHSPlanet::NewLandingLoc(void)
{
    int idx;

    // Check to see if it'll fit
    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx] == NULL)
	    break;
    }

    if (idx == HS_MAX_LANDING_LOCS) {
	return NULL;
    }
    m_landing_locs[idx] = new CHSLandingLoc;
    return m_landing_locs[idx];
}

BOOL CHSPlanet::AddLandingLoc(dbref room)
{
    CHSLandingLoc *cLoc;
    char tbuf[256];		// Temp string storage

    // Grab a new landing loc
    cLoc = NewLandingLoc();
    if (!cLoc) {
	sprintf(tbuf,
		"WARNING: Maximum landing locations reached for planet %s.",
		m_name);
	hs_log(tbuf);
	return FALSE;
    }
    // Initialize some attrs
    cLoc->SetActive(TRUE);
    cLoc->objnum = room;

    // Set the flag
    hsInterface.SetFlag(room, ROOM_HSPACE_LANDINGLOC);

    // Set it's owner to us.
    cLoc->SetOwnerObject(this);

    // Set the planet attribute
    hsInterface.AtrAdd(room, "HSDB_PLANET",
		       unsafe_tprintf("#%d", m_objnum), GOD);

    return TRUE;
}

// Called upon to set an attribute on the planet object.  If
// the attribute name is not found, it's passed up the object
// hierarchy to the base objects.
BOOL CHSPlanet::SetAttributeValue(char *strName, char *strValue)
{
    return (CHSCelestial::SetAttributeValue(strName, strValue));
}

// Returns a character string containing the value of the
// requested attribute.
char *CHSPlanet::GetAttributeValue(char *strName)
{
    static char rval[256];
    int idx;

    *rval = '\0';

    if (!strcasecmp(strName, "LANDINGLOCS")) {
	char tmp[16];
	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	    if (m_landing_locs[idx]) {
		if (!*rval)
		    sprintf(tmp, "#%d", m_landing_locs[idx]->objnum);
		else
		    sprintf(tmp, " #%d", m_landing_locs[idx]->objnum);

		strcat(rval, tmp);
	    }
	}
    } else
	return CHSCelestial::GetAttributeValue(strName);

    return rval;
}

// Retrieves a landing location given a slot number (0 - n)
CHSLandingLoc *CHSPlanet::GetLandingLoc(int which)
{
    if (which < 0 || which >= HS_MAX_LANDING_LOCS)
	return NULL;

    return m_landing_locs[which];
}

void CHSPlanet::GiveScanReport(CHSObject * cScanner, dbref player, BOOL id)
{
    char tbuf[256];
    int nLocs;

    // Determine how many landing locs we have
    int idx;
    nLocs = 0;
    for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	if (m_landing_locs[idx])
	    nLocs++;
    }

    // Print a header
    sprintf(tbuf,
	    "%s%s.------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Scan Report    %30s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    id ? GetName() : "Unknown",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >----------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Give object info
    sprintf(tbuf,
	    "%s%s| %sX:%s %9.0f                   %s%sSize:%s %-3d       %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetX(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetSize(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sY:%s %9.0f%35s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetY(), " ", ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sZ:%s %9.0f%35s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetZ(), " ", ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Do we have any landing locs to report?
    if (nLocs > 0) {
	// Print em baby.
	notify(player,
	       unsafe_tprintf("%s%s|%48s|%s",
			      ANSI_HILITE, ANSI_BLUE, " ", ANSI_NORMAL));
	sprintf(tbuf,
		"%s%s| %sLanding Locations:%s %-2d%26s%s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
		nLocs, " ", ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	sprintf(tbuf,
		"%s%s| %s[%s##%s] Name                     Active  Code     %s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_WHITE, ANSI_GREEN,
		ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	char strPadName[32];
	for (idx = 0; idx < HS_MAX_LANDING_LOCS; idx++) {
	    if (m_landing_locs[idx]) {
		strncpy(strPadName, Name(m_landing_locs[idx]->objnum), 32);
		strPadName[23] = '\0';
		sprintf(tbuf,
			"%s%s|%s  %2d  %-25s  %3s   %3s      %s%s|%s",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
			idx + 1,
			strPadName,
			m_landing_locs[idx]->IsActive()? "Yes" : "No",
			m_landing_locs[idx]->CodeRequired()? "Yes" : "No",
			ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
		notify(player, tbuf);
	    }
	}
    }
    // Finish the report
    sprintf(tbuf,
	    "%s%s`------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

}

//
// CHSLandingLoc stuff
//
CHSLandingLoc::CHSLandingLoc(void)
{
    objnum = -1;
    *strCode = '\0';
    bActive = TRUE;
    m_max_capacity = -1;	// Unlimited
    m_capacity = 0;
    m_ownerObj = NULL;
}

CHSLandingLoc::~CHSLandingLoc(void)
{
    // Clear attributes and flags
    hsInterface.UnsetFlag(objnum, ROOM_HSPACE_LANDINGLOC);

    ClearObjectAttrs();
}

BOOL CHSLandingLoc::LoadFromObject(dbref obj)
{
    objnum = obj;

    if (hsInterface.AtrGet(obj, "HSDB_CODE")) {
	strcpy(strCode, hsInterface.m_buffer);
    }

    if (hsInterface.AtrGet(obj, "HSDB_ACTIVE")) {
	bActive = hsInterface.m_buffer[0] == '1' ? TRUE : FALSE;
    }

    if (hsInterface.AtrGet(obj, "HSDB_MAX_CAPACITY")) {
	m_max_capacity = atoi(hsInterface.m_buffer);
    }

    if (hsInterface.AtrGet(obj, "HSDB_CAPACITY")) {
	m_capacity = atoi(hsInterface.m_buffer);
    }
    // Set some default info on the room
    hsInterface.SetFlag(obj, ROOM_HSPACE_LANDINGLOC);

    // Clear attributes
    ClearObjectAttrs();

    return TRUE;
}

void CHSLandingLoc::WriteObjectAttrs(void)
{
    hsInterface.AtrAdd(objnum, "HSDB_CODE", strCode, GOD);
    hsInterface.AtrAdd(objnum, "HSDB_ACTIVE", unsafe_tprintf("%s",
							     bActive ? "1"
							     : "0"), GOD);
    hsInterface.AtrAdd(objnum, "HSDB_MAX_CAPACITY",
		       unsafe_tprintf("%d", m_max_capacity), GOD);
    hsInterface.AtrAdd(objnum, "HSDB_CAPACITY",
		       unsafe_tprintf("%d", m_capacity), GOD);
}

void CHSLandingLoc::ClearObjectAttrs(void)
{
    hsInterface.AtrAdd(objnum, "HSDB_CODE", NULL, GOD);
    hsInterface.AtrAdd(objnum, "HSDB_ACTIVE", NULL, GOD);
    hsInterface.AtrAdd(objnum, "HSDB_CAPACITY", NULL, GOD);
    hsInterface.AtrAdd(objnum, "HSDB_MAX_CAPACITY", NULL, GOD);
}

// Returns a character string containing the value of the
// requested attribute.
char *CHSLandingLoc::GetAttributeValue(char *strName)
{
    static char rval[32];

    *rval = '\0';
    if (!strcasecmp(strName, "CODE"))
	strcpy(rval, strCode);
    else if (!strcasecmp(strName, "ACTIVE"))
	sprintf(rval, "%d", bActive ? 1 : 0);
    else if (!strcasecmp(strName, "CAPACITY"))
	sprintf(rval, "%d", m_capacity);
    else if (!strcasecmp(strName, "MAX CAPACITY"))
	sprintf(rval, "%d", m_max_capacity);
    else
	return NULL;

    return rval;
}

// Set a value for a given attribute on the landing location
BOOL CHSLandingLoc::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "CODE")) {
	if (strlen(strValue) > 8)
	    return FALSE;
	strcpy(strCode, strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "ACTIVE")) {
	iVal = atoi(strValue);
	if (iVal == 0)
	    bActive = FALSE;
	else
	    bActive = TRUE;
	return TRUE;
    } else if (!strcasecmp(strName, "CAPACITY")) {
	iVal = atoi(strValue);
	m_capacity = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MAX CAPACITY")) {
	iVal = atoi(strValue);
	m_max_capacity = iVal;
	return TRUE;
    } else
	return FALSE;
}

// Indicates whether the landing location is actively open or
// not.
BOOL CHSLandingLoc::IsActive(void)
{
    return bActive;
}

// Indicates whether the supplied code matches the currently
// set code.  If no code is set, this always returns TRUE.
BOOL CHSLandingLoc::CodeClearance(char *lpstrCode)
{
    if (!*strCode)
	return TRUE;

    if (!strcasecmp(strCode, lpstrCode))
	return TRUE;

    return FALSE;
}

// Handles a message, which for now is just giving it to the room
void CHSLandingLoc::HandleMessage(char *lpstrMsg, int type)
{
    hsInterface.NotifyContents(objnum, lpstrMsg);
}

// Indicates whether the landing location can accomodate
// the given object based on size.
BOOL CHSLandingLoc::CanAccomodate(CHSObject * cObj)
{
    // Unlimited capacity?
    if (m_max_capacity < 0)
	return TRUE;

    // Compare current capacity and object size
    if (m_capacity < cObj->GetSize())
	return FALSE;

    return TRUE;
}

void CHSLandingLoc::DeductCapacity(int change)
{
    // If unlimited, do nothing.
    if (m_max_capacity < 0)
	return;

    m_capacity -= change;
}

// Sets the active status of the landing location
void CHSLandingLoc::SetActive(BOOL bStat)
{
    bActive = bStat;
}

// Sets the CHSObject that owns the landing location.
void CHSLandingLoc::SetOwnerObject(CHSObject * cObj)
{
    m_ownerObj = cObj;
}

// Returns the CHSObject that controls the landing location.
CHSObject *CHSLandingLoc::GetOwnerObject(void)
{
    return m_ownerObj;
}

// Returns TRUE or FALSE to indicate if a code is required.
BOOL CHSLandingLoc::CodeRequired(void)
{
    if (*strCode)
	return TRUE;
    else
	return FALSE;
}
