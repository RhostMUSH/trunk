#include "hscopyright.h"
#include "hsobjects.h"
#include "hsinterface.h"
#include <stdlib.h>
#include "hsutils.h"
#include "ansi.h"
#include "hsuniverse.h"
#include "hsconf.h"
#include <math.h>
#include <string.h>

extern "C" {
#include "externs.h"
} extern double d2sin_table[];
extern double d2cos_table[];

CHSObject::CHSObject(void)
{
    m_x = 0;
    m_y = 0;
    m_z = 0;

    m_objnum = NOTHING;
    m_uid = 0;
    m_visible = TRUE;
    m_size = 1;
    m_name = NULL;
    m_current_speed = 0;	// No speed

    m_type = HST_NOTYPE;

    m_next = m_prev = NULL;
}

CHSObject::~CHSObject(void)
{
    if (m_name)
	delete[]m_name;

    if (m_objnum != NOTHING) {
	// Reset object flag
	if (hsInterface.ValidObject(m_objnum))
	    hsInterface.UnsetFlag(m_objnum, THING_HSPACE_OBJECT);
    }
}

// Indicates whether the object is actively in space or not.
BOOL CHSObject::IsActive(void)
{
    // By default, all objects are active
    return TRUE;
}

// Indicates whether the object is visible or not.
BOOL CHSObject::IsVisible(void)
{
    return m_visible;
}

// A general message handler for all CHSObjects.  It does
// absosmurfly nothing.  If you implement this function in your
// derived class, the long pointer points to whatever data are
// passed in.  It's up to you to know what is being passed.
void CHSObject::HandleMessage(char *lpstrMsg, int msgType, long *data)
{
    // Do nothing.
}

// Can be overridden in derived classes to retrieve
// engineering systems.  Generic CHSObjects have no
// engineering systems.
CHSEngSystem *CHSObject::GetEngSystem(int type)
{
    return NULL;
}

// Can be overridden in derived classes to retrieve
// the engineering systems array.  Generic CHSObjects have
// no engineering systems array.
CHSSystemArray *CHSObject::GetEngSystemArray(void)
{
    return NULL;
}

// Can be overridden in derived classes to retrieve
// a list of engineering system types available on
// the object.  Generic CHSObjects have no engineering
// systems.
//
// The return value is the number of systems returned.
UINT CHSObject::GetEngSystemTypes(int *iBuff)
{
    return 0;
}

// ATTEMPTS to set a given attribute with the specified value.
// If the attribute doesn't exist or the value is invalid, it
// return FALSE.  Otherwise, TRUE.
BOOL CHSObject::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Look for the attribute
    if (!strcasecmp(strName, "X")) {
	m_x = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "Y")) {
	m_y = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "Z")) {
	m_z = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "NAME")) {
	if (!strValue || !*strValue)
	    return FALSE;

	if (m_name)
	    delete[]m_name;

	m_name = new char[strlen(strValue) + 1];
	strcpy(m_name, strValue);

	// Change the name of the MUSH object
	if (m_objnum != NOTHING) {
	    set_name(m_objnum, strValue);
	}
	return TRUE;

    } else if (!strcasecmp(strName, "UID")) {
	iVal = atoi(strValue);

	// Check to see if the universe exists.
	CHSUniverse *uSource;
	CHSUniverse *uDest;

	uDest = uaUniverses.FindUniverse(iVal);
	if (!uDest)
	    return FALSE;

	// Grab the source universe
	uSource = uaUniverses.FindUniverse(m_uid);

	// Now pull it from one, and put it in another
	uSource->RemoveObject(this);
	m_uid = iVal;
	return (uDest->AddObject(this));
    } else if (!strcasecmp(strName, "SIZE")) {
	iVal = atoi(strValue);
	if (iVal < 1)
	    return FALSE;

	m_size = iVal;
	return TRUE;
    } else
	return FALSE;
}

// Returns a character value for the requested object
// attribute.  If the attribute is not valid, the string
// contains a single null character.
char *CHSObject::GetAttributeValue(char *strName)
{
    static char rval[64];

    *rval = '\0';

    if (!strcasecmp(strName, "X")) {
	sprintf(rval, "%.2f", m_x);
    } else if (!strcasecmp(strName, "Y")) {
	sprintf(rval, "%.2f", m_y);
    } else if (!strcasecmp(strName, "Z")) {
	sprintf(rval, "%.2f", m_z);
    } else if (!strcasecmp(strName, "NAME")) {
	strcpy(rval, GetName());
    } else if (!strcasecmp(strName, "UID")) {
	sprintf(rval, "%d", m_uid);
    } else if (!strcasecmp(strName, "SIZE")) {
	sprintf(rval, "%d", GetSize());
    } else
	return NULL;

    return rval;
}

// This function does nothing at present, but it
// can be overridden in the derived classes.
void CHSObject::ClearObjectAttrs(void)
{

}

// Given a key and a value, the function tries to
// load the value.  If the key is not recognized by the
// object, it returns FALSE.
//
// If you implement the WriteToFile() function, you MUST
// have this function present to handle loading the values
// in the database.  They are in the form of key=value.
BOOL CHSObject::HandleKey(int key, char *strValue, FILE * fp)
{
    // Determine key, load value
    switch (key) {
    case HSK_X:
	m_x = atof(strValue);
	return TRUE;
    case HSK_Y:
	m_y = atof(strValue);
	return TRUE;
    case HSK_Z:
	m_z = atof(strValue);
	return TRUE;
    case HSK_NAME:
	if (m_name)
	    delete[]m_name;
	m_name = new char[strlen(strValue) + 1];
	strcpy(m_name, strValue);
	return TRUE;

    case HSK_UID:
	m_uid = atoi(strValue);
	return TRUE;
    case HSK_OBJNUM:
	m_objnum = atoi(strValue);
	return TRUE;

    case HSK_SIZE:
	m_size = atoi(strValue);
	return TRUE;

    case HSK_VISIBLE:
	m_visible = atoi(strValue);
	return TRUE;
    }
    return FALSE;
}

// You can override this function in your base class IF YOU WANT.
// However, if you implement the HandleKey(), that's enough.
// Override it if you want some extra initialization when your
// derived object is loaded from the database.
//
// This function is called by some database loading routine, and passed
// a file pointer which it uses to load variables from the database.
// The variables are in key=value form, and this function just passes
// the key and value to the HandleKey() function.  If you haven't
// overridden it, then the CHSObject handles everything that it can.
// If you have overridden the HandleKey(), then the key and values
// are passed to your function first.  You can then pass unhandled
// keys to the base object.
BOOL CHSObject::LoadFromFile(FILE * fp)
{
    char tbuf[256];
    char strKey[64];
    char strValue[64];
    char *ptr;
    HS_DBKEY key;
    CHSUniverse *uDest;

    // Load until end of file or, more likely, an OBJECTEND
    // key.  If we don't find the OBJECTEND key, the object
    // is incomplete.
    while (fgets(tbuf, 256, fp)) {
	// Strip newlines
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	// Extract key/value pairs
	extract(tbuf, strKey, 0, 1, '=');
	extract(tbuf, strValue, 1, 1, '=');


	// Determine key, and handle it.
	key = HSFindKey(strKey);
	if (key == HSK_OBJECTEND) {
	    // Exit loading
	    EndLoadObject();
	    break;
	} else if (!HandleKey(key, strValue, fp)) {
	    sprintf(tbuf,
		    "WARNING: Object key \"%s\" (%d) found but not handled.",
		    strKey, key);
	    hs_log(tbuf);
	}
    }

    // Ensure that object is complete
    if (key != HSK_OBJECTEND)
	return FALSE;

    // Check to be sure the objnum is valid.
    if (!hsInterface.ValidObject(m_objnum))
	return FALSE;

    // Set flags
    hsInterface.SetFlag(m_objnum, THING_HSPACE_OBJECT);

    // Set type
    hsInterface.AtrAdd(m_objnum, "HSDB_TYPE", unsafe_tprintf("%d",
							     m_type), GOD);

    // Add to universe
    uDest = uaUniverses.FindUniverse(m_uid);
    if (!uDest) {
	sprintf(tbuf,
		"WARNING: HSpace object #%d has an invalid UID.  Adding to default universe.",
		m_objnum);
	hs_log(tbuf);
	uDest = NULL;
	for (int jdx = 0; jdx < HS_MAX_UNIVERSES; jdx++) {
	    if ((uDest = uaUniverses.GetUniverse(jdx)) != NULL)
		break;
	}
	if (!uDest) {
	    hs_log("ERROR: Unable to find default universe.");
	    return FALSE;
	}
	if (!uDest->AddObject(this)) {
	    hs_log("ERROR: Failed to add to default universe.");
	    return FALSE;
	}
	// Set the new ID of the valid universe.
	m_uid = uDest->GetID();
    } else {
	if (!uDest->AddObject(this)) {
	    sprintf(tbuf,
		    "WARNING: HSpace object #%d was not be added to universe.",
		    m_objnum);
	    hs_log(tbuf);
	}
    }
    return TRUE;
}

// This function is called by some database saving routine.  If
// you have derived an object from CHSObject and have attributes
// that you want written to the database, you MUST override this
// in your derived class.  You must also, then, implement the
// HandleKey() function to handle loading those values.
void CHSObject::WriteToFile(FILE * fp)
{
    // Write attributes to file
    fprintf(fp, "OBJNUM=%d\n", m_objnum);
    if (m_name)
	fprintf(fp, "NAME=%s\n", m_name);
    fprintf(fp, "X=%.2f\n", m_x);
    fprintf(fp, "Y=%.2f\n", m_y);
    fprintf(fp, "Z=%.2f\n", m_z);
    fprintf(fp, "UID=%d\n", m_uid);
    fprintf(fp, "VISIBLE=%d\n", m_visible);
    fprintf(fp, "SIZE=%d\n", m_size);
}

// Returns the name of the object.
char *CHSObject::GetName(void)
{
    if (!m_name)
	return "No Name";

    return m_name;
}

// Sets the name of the object.
void CHSObject::SetName(char *strName)
{
    if (!strName)
	return;

    if (m_name)
	delete[]m_name;

    m_name = new char[strlen(strName) + 1];
    strcpy(m_name, strName);
}

// This function does nothing, but it can be overridden by 
// derived objects to handle HSpace cycles.  For example, ships
// want to travel, do sensor sweeps, etc.  The generic CHSObject
// doesn't have any cyclic stuff to do.
void CHSObject::DoCycle(void)
{
    // Do nothing
}

void CHSObject::MoveTowards(double x, double y, double z, float speed)
{
    int zhead;
    int xyhead;

    xyhead = XYAngle(m_x, m_y, x, y);
    zhead = ZAngle(m_x, m_y, m_z, x, y, z);
    if (zhead < 0)
	zhead += 360;

    CHSVector tvec(d2sin_table[xyhead] * d2cos_table[zhead],
		   d2cos_table[xyhead] * d2cos_table[zhead],
		   d2sin_table[zhead]);

    // Add to the current position .. heading vector * speed.
    m_x += tvec.i() * speed;
    m_y += tvec.j() * speed;
    m_z += tvec.k() * speed;

}

void CHSObject::MoveInDirection(int xyhead, int zhead, float speed)
{
    if (zhead < 0)
	zhead += 360;

    CHSVector tvec(d2sin_table[xyhead] * d2cos_table[zhead],
		   d2cos_table[xyhead] * d2cos_table[zhead],
		   d2sin_table[zhead]);

    // Add to the current position .. heading vector * speed.
    m_x += tvec.i() * speed;
    m_y += tvec.j() * speed;
    m_z += tvec.k() * speed;

}

// Returns TRUE if the two objects are equal.  That is to say that
// their object dbrefs are equal.
BOOL CHSObject::operator ==(CHSObject & cRVal)
{
    if (cRVal.GetDbref() == m_objnum)
	return TRUE;

    return FALSE;
}

BOOL CHSObject::operator ==(CHSObject * cRVal)
{
    if (cRVal->GetDbref() == m_objnum)
	return TRUE;

    return FALSE;
}

UINT CHSObject::GetSize(void)
{
    return m_size;
}

HS_TYPE CHSObject::GetType(void)
{
    return m_type;
}

double CHSObject::GetX(void)
{
    return m_x;
}

double CHSObject::GetY(void)
{
    return m_y;
}

double CHSObject::GetZ(void)
{
    return m_z;
}

// Returns the ANSI character string used to define the
// color of the object.  Each object should have its own color ..
// or none at all.  Basic objects have no color.  Override this
// member function in your base class to provide a color.
char *CHSObject::GetObjectColor(void)
{
    return ANSI_HILITE;
}

// Returns the character representation of the object.  This can
// be used in maps and whatever.
char CHSObject::GetObjectCharacter(void)
{
    return '.';
}

UINT CHSObject::GetUID(void)
{
    return m_uid;
}

dbref CHSObject::GetDbref(void)
{
    return m_objnum;
}

void CHSObject::SetDbref(dbref objnum)
{
    m_objnum = objnum;

    // Set the type attribute on the object
    if (hsInterface.ValidObject(objnum))
	hsInterface.AtrAdd(objnum, "HSDB_TYPE",
			   unsafe_tprintf("%d", m_type), GOD);
}

void CHSObject::SetUID(UINT uid)
{
    m_uid = uid;
}

void CHSObject::SetX(double x)
{
    m_x = x;
}

void CHSObject::SetY(double y)
{
    m_y = y;
}

void CHSObject::SetZ(double z)
{
    m_z = z;
}

// This function returns NULL because the function should
// be implemented specifically on the derived class.
CHSConsole *CHSObject::FindConsole(dbref obj)
{
    return NULL;
}

//
// Linked list functions
//
CHSObject *CHSObject::GetNext(void)
{
    return m_next;
}

CHSObject *CHSObject::GetPrev(void)
{
    return m_prev;
}

void CHSObject::SetNext(CHSObject * cObj)
{
    m_next = cObj;
}

void CHSObject::SetPrev(CHSObject * cObj)
{
    m_prev = cObj;
}

// Override this function in your derived object class to handle
// when another object locks onto it (or unlocks).
void CHSObject::HandleLock(CHSObject * cLocker, BOOL bLocking)
{
    // cLocker is the other object that is locking onto
    // us.
    //
    // bLocking is TRUE if the object is locking.  FALSE,
    // if unlocking.

    // Do nothing
}

// Override this function in your derived object class to
// handle damage from another object that is attacking.
void CHSObject::HandleDamage(CHSObject * cSource,
			     CHSWeapon * cWeapon,
			     int strength,
			     CHSConsole * cConsole, int iSysType)
{
    // Strength is the amount of damage inflicted.  It is
    // up to you to handle that damage in some way.

    // You can use cConsole to get the console that
    // fired the shot.

    // iSysType is the type of system preferred to damage.
}

// Can be used to explode the object, for whatever reason.
void CHSObject::ExplodeMe(void)
{
    CHSUniverse *uSource;

    // Find my universe
    uSource = uaUniverses.FindUniverse(m_uid);
    if (uSource) {
	// Remove me from active space.
	uSource->RemoveActiveObject(this);
    }
}

// Override this function in your derived class to handle
// the end of an object loading from the database.  When
// the loader for the object database encounters the end
// of the object definition, it'll call this routine to
// let your object handle any final changes.
void CHSObject::EndLoadObject(void)
{
    // Nothing to do for base objects
}

// This function can be called by a scanning object to
// get a scan report.  Because scan reports can be different
// for each derived type of object, you should override this
// function to give custom scanning information.  The
// information should be sent to the player variable
// supplied.  The id variable indicates whether the object
// being scanned is on the scanning objects sensors and how
// well it has been identified.
void CHSObject::GiveScanReport(CHSObject * cScanner, dbref player, BOOL id)
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

    // Finish the report
    sprintf(tbuf,
	    "%s%s`------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

}

// Returns the motion vector for the object.
CHSVector & CHSObject::GetMotionVector(void)
{
    return m_motion_vector;
}

// Returns the current velocity for the object
int CHSObject::GetSpeed(void)
{
    return m_current_speed;
}


//
// CHSVector implementation
//
CHSVector::CHSVector(void)
{
    ci = cj = ck = 0;
}

CHSVector::CHSVector(double ii, double jj, double kk)
{
    ci = ii;
    cj = jj;
    ck = kk;
}

BOOL CHSVector::operator ==(CHSVector & vec)
{
    if (vec.ci == ci && vec.cj == cj && vec.ck == ck)
	return TRUE;

    return FALSE;
}

void CHSVector::operator =(CHSVector & vec)
{
    ci = vec.ci;
    cj = vec.cj;
    ck = vec.ck;
}

CHSVector & CHSVector::operator +(CHSVector & vec)
{
    int ti, tj, tk;

    ti = ci + vec.ci;
    tj = cj + vec.cj;
    tk = ck + vec.ck;

    static CHSVector newVec(ti, tj, tk);

    return newVec;
}

double CHSVector::i(void)
{
    return ci;
}

double CHSVector::j(void)
{
    return cj;
}

double CHSVector::k(void)
{
    return ck;
}

void CHSVector::operator +=(CHSVector & vec)
{
    ci += vec.ci;
    cj += vec.cj;
    ck += vec.ck;
}

double CHSVector::DotProduct(CHSVector & vec)
{
    double rval;

    rval = (ci * vec.ci) + (cj * vec.cj) + (ck * vec.ck);
    if (rval > 1)
	rval = 1;
    else if (rval < -1)
	rval = -1;

    return rval;
}
