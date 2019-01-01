#include "hsuniverse.h"
#include "hscopyright.h"
#include "hscelestial.h"
#include <stdlib.h>
#include "hsinterface.h"
#include "hsutils.h"
#include "ansi.h"
#include "hsobjects.h"
#include "hspace.h"

#include <string.h>


CHSCelestial::CHSCelestial(void)
{
}

CHSCelestial::~CHSCelestial(void)
{
}

// Overridden from the CHSObject class
BOOL CHSCelestial::HandleKey(int key, char *strValue, FILE * fp)
{
    // Find the key and handle it
    switch (key) {
    default:			// Pass it up to CHSObject
	return (CHSObject::HandleKey(key, strValue, fp));
    }
}

// Basic celestial objects are cyan
char *CHSCelestial::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_CYAN);
    return tbuf;
}

// Returns the character representing celestials in general
char CHSCelestial::GetObjectCharacter(void)
{
    // How about a simple star
    return '*';
}

// Overridden from the CHSObject class
void CHSCelestial::WriteToFile(FILE * fp)
{
    // Write base class info first, then our's.
    CHSObject::WriteToFile(fp);
}

void CHSCelestial::ClearObjectAttrs(void)
{
    CHSObject::ClearObjectAttrs();

    // Clear the attributes
    hsInterface.AtrAdd(m_objnum, "HSDB_X", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_Y", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_Z", NULL, GOD);
    hsInterface.AtrAdd(m_objnum, "HSDB_UID", NULL, GOD);
}

UINT CHSCelestial::GetType(void)
{
    return m_type;
}

BOOL CHSCelestial::SetAttributeValue(char *strName, char *strValue)
{
    return CHSObject::SetAttributeValue(strName, strValue);
}

char *CHSCelestial::GetAttributeValue(char *strName)
{
    return CHSObject::GetAttributeValue(strName);
}

// Constructor
CHSNebula::CHSNebula(void)
{
    m_type = HST_NEBULA;
    m_visible = TRUE;
    m_density = 1;
    m_shieldaff = 100.00;
}

// Nebulas are little ~'s
char CHSNebula::GetObjectCharacter(void)
{
    return '~';
}

CHSNebula::~CHSNebula(void)
{

}

// Nebulae are magenta
char *CHSNebula::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_MAGENTA);
    return tbuf;
}

int CHSNebula::GetDensity(void)
{
    return m_density;
}

float CHSNebula::GetShieldaff(void)
{
    return m_shieldaff;
}

// Overridden from the CHSCelestial class
void CHSNebula::WriteToFile(FILE * fp)
{
    int idx;

    // Write base class info first, then our's.
    CHSCelestial::WriteToFile(fp);
    fprintf(fp, "DENSITY=%d\n", m_density);
    fprintf(fp, "SHIELFAFF=%d\n", m_shieldaff);
}

void CHSNebula::ClearObjectAttrs()
{
    int idx;

    CHSCelestial::ClearObjectAttrs();
}

char *CHSNebula::GetAttributeValue(char *strName)
{
    static char rval[64];
    *rval = '\0';

    if (!strcasecmp(strName, "DENSITY")) {
	sprintf(rval, "%d", m_density);
    }
    if (!strcasecmp(strName, "SHIELDAFF")) {
	sprintf(rval, "%f", m_shieldaff);
    } else
	return CHSCelestial::GetAttributeValue(strName);

    return rval;
}

BOOL CHSNebula::HandleKey(int key, char *strValue, FILE * fp)
{
    // Find the key and handle it
    switch (key) {
    case HSK_DENSITY:
	m_density = atoi(strValue);
	return TRUE;
    case HSK_SHIELDAFF:
	m_shieldaff = atof(strValue);
	return TRUE;
    default:			// Pass it up to CHSCelestial
	return (CHSCelestial::HandleKey(key, strValue, fp));
    }
}

BOOL CHSNebula::SetAttributeValue(char *strName, char *strValue)
{

    if (!strcasecmp(strName, "DENSITY")) {
	m_density = atoi(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "SHIELDAFF")) {
	if (atof(strValue) < 0.00)
	    return FALSE;

	m_shieldaff = atof(strValue);
	return TRUE;
    } else {
	return (CHSCelestial::SetAttributeValue(strName, strValue));
    }
}

void CHSNebula::GiveScanReport(CHSObject * cScanner, dbref player, BOOL id)
{
    char tbuf[256];

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

    // Give Nebula info
    sprintf(tbuf,
	    "%s%s| %sX:%s %9.0f              %s%sDiameter:%s %-7d hm %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetX(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetSize() * 100, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sY:%s %9.0f               %s%sDensity:%s %3d        %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetY(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetDensity(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sZ:%s %9.0f     %s%sShield Efficiency:%s %3.2f\%    %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetZ(), " ", ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetShieldaff(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    // Finish the report
    sprintf(tbuf,
	    "%s%s`------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}


// Constructor
CHSAsteroid::CHSAsteroid(void)
{
    m_type = HST_ASTEROID;
    m_visible = TRUE;
    m_density = 1;
}

// Asteroid Belts are little :'s
char CHSAsteroid::GetObjectCharacter(void)
{
    return ':';
}

CHSAsteroid::~CHSAsteroid(void)
{

}

// Asteroid Belts are black
char *CHSAsteroid::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_BLACK);
    return tbuf;
}

int CHSAsteroid::GetDensity(void)
{
    return m_density;
}

// Overridden from the CHSCelestial class
void CHSAsteroid::WriteToFile(FILE * fp)
{
    // Write base class info first, then our's.
    CHSCelestial::WriteToFile(fp);
    fprintf(fp, "DENSITY=%d\n", m_density);
}

void CHSAsteroid::ClearObjectAttrs()
{
    CHSCelestial::ClearObjectAttrs();

    m_density = NULL;
}

char *CHSAsteroid::GetAttributeValue(char *strName)
{
    static char rval[64];
    *rval = '\0';

    if (!strcasecmp(strName, "DENSITY")) {
	sprintf(rval, "%d", m_density);
    } else
	return CHSCelestial::GetAttributeValue(strName);

    return rval;
}

BOOL CHSAsteroid::HandleKey(int key, char *strValue, FILE * fp)
{
    // Find the key and handle it
    char tmp[64];
    sprintf(tmp, "%d", key);
    notify(3, tmp);
    switch (key) {
    case HSK_DENSITY:
	m_density = atoi(strValue);
	notify(3, strValue);
	return TRUE;
    default:			// Pass it up to CHSCelestial
	return (CHSCelestial::HandleKey(key, strValue, fp));
    }
}

BOOL CHSAsteroid::SetAttributeValue(char *strName, char *strValue)
{

    if (!strcasecmp(strName, "DENSITY")) {
	m_density = atoi(strValue);
	return TRUE;
    } else {
	return (CHSCelestial::SetAttributeValue(strName, strValue));
    }
}

void CHSAsteroid::GiveScanReport(CHSObject * cScanner,
				 dbref player, BOOL id)
{
    char tbuf[256];

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

    // Give Nebula info
    sprintf(tbuf,
	    "%s%s| %sX:%s %9.0f              %s%sDiameter:%s %7d hm %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetX(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetSize() * 100, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sY:%s %9.0f               %s%sDensity:%s %3d/100m2  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetY(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetDensity(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sZ:%s %9.0f%35s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetZ(), " ", ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    // Finish the report
    sprintf(tbuf,
	    "%s%s`------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}

void CHSAsteroid::DoCycle(void)
{
    CHSObject *cObj;
    CHSUniverse *uSrc;
    CHSShip *Target;
    int idx;
    double dDistance;


    uSrc = uaUniverses.FindUniverse(GetUID());
    if (!uSrc)
	return;

    // Grab all of the objects in the universe, and see if they're in the area.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	cObj = uSrc->GetUnivObject(idx);
	if (!cObj)
	    continue;
	if (cObj->GetType() != HST_SHIP)
	    continue;

	Target = (CHSShip *) cObj;
	dDistance =
	    Dist3D(GetX(), GetY(), GetZ(), Target->GetX(), Target->GetY(),
		   Target->GetZ());

	if (dDistance > GetSize() * 100)
	    continue;

	if (GetDensity() <
	    getrandom(50 / Target->GetSize() *
		      ((Target->GetSpeed() + 1) / 1000)))
	    continue;

	int strength;

	strength =
	    getrandom(GetDensity() * Target->GetSize() *
		      ((Target->GetSpeed() + 1) / 1000));

	Target->m_hull_points -= strength;

	Target->
	    NotifySrooms
	    ("The ship shakes as asteroids impact on the hull");

	// Is hull < 0?
	if (Target->m_hull_points < 0) {
	    Target->ExplodeMe();
	    if (!hsInterface.
		HasFlag(m_objnum, TYPE_THING, THING_HSPACE_SIM))
		Target->KillShipCrew("THE SHIP EXPLODES!!");
	}

    }

}

CHSBlackHole::CHSBlackHole(void)
{
    m_type = HST_BLACKHOLE;
    m_visible = TRUE;
}

// Black holes are big O's
char CHSBlackHole::GetObjectCharacter(void)
{
    return 'O';
}

CHSBlackHole::~CHSBlackHole(void)
{

}

// Nebulae are magenta
char *CHSBlackHole::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s", ANSI_BLUE);
    return tbuf;
}

void CHSBlackHole::DoCycle(void)
{
    CHSObject *cObj;
    CHSUniverse *uSrc;
    CHSShip *Target;
    int idx;
    double dDistance;


    uSrc = uaUniverses.FindUniverse(GetUID());
    if (!uSrc)
	return;

    // Grab all of the objects in the universe, and see if they're in the area.
    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	cObj = uSrc->GetUnivObject(idx);
	if (!cObj)
	    continue;
	if (cObj->GetType() != HST_SHIP)
	    continue;

	Target = (CHSShip *) cObj;
	dDistance =
	    Dist3D(GetX(), GetY(), GetZ(), Target->GetX(), Target->GetY(),
		   Target->GetZ());

	if (dDistance > GetSize() * 100 - 0.01)
	    continue;

	int strength;

	strength = (GetSize() * 100) / dDistance * 100 - 100;

	Target->MoveTowards(m_x, m_y, m_z, strength);

	Target->m_hull_points -= strength;

	Target->
	    NotifySrooms
	    ("The hull buckles from the black hole's gravity.");

	// Is hull < 0?
	if (Target->m_hull_points < 0) {
	    Target->ExplodeMe();
	    if (!hsInterface.
		HasFlag(m_objnum, TYPE_THING, THING_HSPACE_SIM))
		Target->KillShipCrew("THE SHIP EXPLODES!!");
	}

    }

}

// Constructor
CHSWormHole::CHSWormHole(void)
{
    m_type = HST_WORMHOLE;
    m_visible = TRUE;
    m_stability = 0.0;
    m_fluctuation = 0.0;
    m_basestability = 0.0;
    m_destx = m_x;
    m_desty = m_y;
    m_destz = m_z;
    m_destuid = m_uid;
}

// Asteroid Belts are little :'s
char CHSWormHole::GetObjectCharacter(void)
{
    return '0';
}

CHSWormHole::~CHSWormHole(void)
{

}

// Asteroid Belts are black
char *CHSWormHole::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s", ANSI_RED);
    return tbuf;
}

float CHSWormHole::GetStability(void)
{
    return m_stability;
}

float CHSWormHole::GetFluctuation(void)
{
    return m_fluctuation;
}

float CHSWormHole::GetBaseStability(void)
{
    return m_basestability;
}

// Overridden from the CHSCelestial class
void CHSWormHole::WriteToFile(FILE * fp)
{
    // Write base class info first, then our's.
    CHSCelestial::WriteToFile(fp);
    fprintf(fp, "STABILITY=%f\n", m_stability);
    fprintf(fp, "FLUCTUATION=%f\n", m_fluctuation);
    fprintf(fp, "BASESTABILITY=%f\n", m_basestability);
    fprintf(fp, "DESTX=%d\n", m_destx);
    fprintf(fp, "DESTY=%d\n", m_desty);
    fprintf(fp, "DESTZ=%d\n", m_destz);
    fprintf(fp, "DESTUID=%d\n", m_destuid);
}

void CHSWormHole::ClearObjectAttrs()
{
    CHSCelestial::ClearObjectAttrs();

    m_stability = 0.00;
    m_fluctuation = 0.00;
    m_basestability = 0.00;
    m_destx = 0.00;
    m_desty = 0.00;
    m_destz = 0.00;
    m_destuid = 0.00;
}

char *CHSWormHole::GetAttributeValue(char *strName)
{
    static char rval[64];
    *rval = '\0';

    if (!strcasecmp(strName, "STABILITY")) {
	sprintf(rval, "%f", m_stability);
    } else if (!strcasecmp(strName, "FLUCTUATION")) {
	sprintf(rval, "%f", m_fluctuation);
    } else if (!strcasecmp(strName, "BASESTABILITY")) {
	sprintf(rval, "%f", m_basestability);
    } else if (!strcasecmp(strName, "DESTX")) {
	sprintf(rval, "%d", m_destx);
    } else if (!strcasecmp(strName, "DESTY")) {
	sprintf(rval, "%d", m_desty);
    } else if (!strcasecmp(strName, "DESTZ")) {
	sprintf(rval, "%d", m_destz);
    } else if (!strcasecmp(strName, "DESTUID")) {
	sprintf(rval, "%d", m_destuid);
    } else
	return CHSCelestial::GetAttributeValue(strName);

    return rval;
}

BOOL CHSWormHole::HandleKey(int key, char *strValue, FILE * fp)
{
    // Find the key and handle it
    char tmp[64];
    sprintf(tmp, "%d", key);
    switch (key) {
    case HSK_STABILITY:
	m_stability = atof(strValue);
	return TRUE;
    case HSK_FLUCTUATION:
	m_fluctuation = atof(strValue);
	return TRUE;
    case HSK_BASESTABILITY:
	m_basestability = atof(strValue);
	return TRUE;
    case HSK_DESTX:
	m_destx = atof(strValue);
	return TRUE;
    case HSK_DESTY:
	m_desty = atof(strValue);
	return TRUE;
    case HSK_DESTZ:
	m_destz = atof(strValue);
	return TRUE;
    case HSK_DESTUID:
	m_destuid = atoi(strValue);
	return TRUE;
    default:			// Pass it up to CHSCelestial
	return (CHSCelestial::HandleKey(key, strValue, fp));
    }
}

BOOL CHSWormHole::SetAttributeValue(char *strName, char *strValue)
{

    if (!strcasecmp(strName, "STABILITY")) {
	m_stability = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "FLUCTUATION")) {
	m_fluctuation = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "BASESTABILITY")) {
	m_basestability = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "DESTX")) {
	m_destx = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "DESTY")) {
	m_desty = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "DESTZ")) {
	m_destz = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "DESTUID")) {
	m_destuid = atoi(strValue);
	return TRUE;
    } else {
	return (CHSCelestial::SetAttributeValue(strName, strValue));
    }
}

void CHSWormHole::GiveScanReport(CHSObject * cScanner,
				 dbref player, BOOL id)
{
    char tbuf[256];

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

    // Give Nebula info
    sprintf(tbuf,
	    "%s%s| %sX:%s %9.0f                  %s%sSize:%s %-3d        %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetX(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    GetSize(), ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sY:%s %9.0f             %s%sStability:%s %-10s %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetY(),
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    unsafe_tprintf("%3.6f%%", GetStability()),
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s| %sZ:%s %9.0f %34s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    GetZ(), " ", ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    // Finish the report
    sprintf(tbuf,
	    "%s%s`------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}

void CHSWormHole::DoCycle(void)
{
    if (m_fluctuation <= 0)
	return;

    m_stability += (getrandom(20000) - 10000) / 1000000.00;

    if (m_stability < (m_basestability - m_fluctuation))
	m_stability = m_basestability - m_fluctuation;

    if (m_stability > (m_basestability + m_fluctuation))
	m_stability = m_basestability + m_fluctuation;

}

void CHSWormHole::GateShip(CHSShip * cShip)
{
    if (!cShip)
	return;

    BOOL succeed;

    if (getrandom(100) <= m_stability)
	succeed = TRUE;
    else
	succeed = FALSE;

    cShip->
	NotifyConsoles
	("The ship enters the wormhole and an infinite amount of colors can be seen.",
	 MSG_SENSOR);
    cShip->
	NotifySrooms("The ship shakes slightly as it enters a wormhole.");

    CHSUniverse *uDest;
    uDest = uaUniverses.FindUniverse(cShip->GetUID());
    if (uDest) {
	uDest->
	    SendContactMessage("In the distance a ship gates a wormhole.",
			       DETECTED, cShip);
	uDest->
	    SendContactMessage(unsafe_tprintf
			       ("In the distance the %s gates a wormhole.",
				cShip->GetName()), IDENTIFIED, cShip);
    }


    if (succeed) {
	if (m_destx)
	    cShip->SetX(m_destx);
	if (m_desty)
	    cShip->SetY(m_desty);
	if (m_destz)
	    cShip->SetZ(m_destz);

	CHSUniverse *uSource;

	uDest = uaUniverses.FindUniverse(m_destuid);
	if (uDest) {
	    // Grab the source universe
	    uSource = uaUniverses.FindUniverse(cShip->GetUID());

	    // Now pull it from one, and put it in another
	    uSource->RemoveObject(cShip);
	    cShip->SetUID(m_destuid);
	    uDest->AddObject(cShip);
	}

	cShip->
	    NotifyConsoles
	    ("The ship safely emerges on the other side of the wormhole.",
	     MSG_SENSOR);
    } else {
	cShip->
	    NotifyConsoles
	    ("The wormhole collapses and the structural integrity is comprimised.",
	     MSG_SENSOR);
	cShip->ExplodeMe();
	if (!hsInterface.
	    HasFlag(cShip->GetDbref(), TYPE_THING, THING_HSPACE_SIM))
	    cShip->
		KillShipCrew
		("The ship explodes as the structural integrity fails!");
    }
}
