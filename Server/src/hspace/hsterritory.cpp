#include "hscopyright.h"
#include "hsterritory.h"
#include "hsutils.h"
#include "hsinterface.h"
#include <string.h>
#include <stdlib.h>

CHSTerritoryArray taTerritories;	// Global instance


//
// CHSTerritoryArray
//
CHSTerritoryArray::CHSTerritoryArray(void)
{
    // Initialize territories
    int idx;
    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	m_territories[idx] = NULL;
    }
}

// Attempts to allocate a new territory in the array of the
// specified type.
CHSTerritory *CHSTerritoryArray::NewTerritory(int objnum, TERRTYPE type)
{
    // Do we have any free slots?
    int idx;
    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (!m_territories[idx])
	    break;
    }
    if (idx == HS_MAX_TERRITORIES)
	return NULL;

    // Determine type of territory
    switch (type) {
    case T_RADIAL:
	m_territories[idx] = new CHSRadialTerritory;
	break;

    case T_CUBIC:
	m_territories[idx] = new CHSCubicTerritory;
	break;

    default:			// What is this territory type?
	return NULL;
    }

    // Set object num
    m_territories[idx]->SetDbref(objnum);

    // Set the territory flag
    if (objnum != NOTHING)
	hsInterface.SetFlag(m_territories[idx]->GetDbref(),
			    THING_HSPACE_TERRITORY);

    return (CHSTerritory *) m_territories[idx];
}

// Saves territories to the specified file path.
void CHSTerritoryArray::SaveToFile(const char *lpstrPath)
{
    int idx;
    FILE *fp;

    // Try to open the specified file for writing.
    fp = fopen(lpstrPath, "w");
    if (!fp)
	return;			// bummer

    // Run through our territories, telling them to write
    // to the file.
    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx]) {
	    fprintf(fp, "TERRITORY=%d\n", m_territories[idx]->GetType());	// Our indicator

	    m_territories[idx]->SaveToFile(fp);
	}
    }

    fclose(fp);
}

// Returns the number of territories loaded.
UINT CHSTerritoryArray::NumTerritories(void)
{
    int idx;
    int cnt = 0;

    // Count em.
    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx])
	    cnt++;
    }
    return cnt;
}

// Prints information about the loaded territories to the
// specified player.
void CHSTerritoryArray::PrintInfo(int player)
{
    int idx;

    notify(player,
	   "[Dbref] Name               Type    UID Specifications");

    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx]) {
	    switch (m_territories[idx]->GetType()) {
	    case T_RADIAL:
		CHSRadialTerritory * cRadial;
		cRadial = (CHSRadialTerritory *) m_territories[idx];
		notify(player,
		       unsafe_tprintf
		       ("[%5d] %-18s RADIAL  %-3d Center: %d,%d,%d  Radius: %d",
			m_territories[idx]->GetDbref(),
			(char *)Name(m_territories[idx]->GetDbref()),
			m_territories[idx]->GetUID(), cRadial->GetX(),
			cRadial->GetY(), cRadial->GetZ(),
			cRadial->GetRadius()));
		break;

	    case T_CUBIC:
		CHSCubicTerritory * cCubic;
		cCubic = (CHSCubicTerritory *) m_territories[idx];
		notify(player,
		       (const char *)unsafe_tprintf
		       ((const char *)"[%5d] %-18s CUBIC   %-3d Min: %d,%d,%d  Max: %d,%d,%d",
			m_territories[idx]->GetDbref(),
			Name(m_territories[idx]->GetDbref()),
			m_territories[idx]->GetUID(), cCubic->GetMinX(),
			cCubic->GetMinY(), cCubic->GetMinZ(),
			cCubic->GetMaxX(), cCubic->GetMaxY(),
			cCubic->GetMaxZ()));
		break;
	    }
	}
    }
}

// Loads territories from the specified file.
BOOL CHSTerritoryArray::LoadFromFile(const char *lpstrPath)
{
    FILE *fp;

    // Try to open the specified file.
    fp = fopen(lpstrPath, "r");
    if (!fp)
	return FALSE;		// Drat

    // Read in the territories file, looking for TERRITORY
    // keywords.
    char key[128];
    char value[128];
    char tbuf[256];
    char *ptr;
    CHSTerritory *cTerritory = NULL;
    while (fgets(tbuf, 128, fp)) {
	// Strip returns
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';

	// Pull out the key and value.
	extract(tbuf, key, 0, 1, '=');
	extract(tbuf, value, 1, 1, '=');

	// Is it a new territory?
	if (!strcasecmp(key, "TERRITORY")) {
	    // Grab a new territory.
	    cTerritory = NewTerritory(NOTHING, (TERRTYPE) atoi(value));
	    if (!cTerritory) {
                hs_log
                    ((char *)"ERROR: Error encountered while loading territories.");
		break;
	    }
	} else {
	    // Assumed to be a territory key=value pair.
	    // Pass it to the current territory.
	    if (cTerritory) {
		if (!cTerritory->SetAttributeValue(key, value)) {
		    sprintf(tbuf,
			    "WARNING: Failed to set attribute \"%s\" on territory.",
			    key);
		    hs_log(tbuf);
		}
	    }
	}
    }
    fclose(fp);
    return TRUE;
}

// Returns the territory that the given object falls within,
// or NULL if none matched.
CHSTerritory *CHSTerritoryArray::InTerritory(CHSObject * cObj)
{
    // Run through our territories, asking them for a match.
    double x, y, z;
    int uid;

    x = cObj->GetX();
    y = cObj->GetY();
    z = cObj->GetZ();
    uid = cObj->GetUID();
    for (int idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx]) {
	    if (m_territories[idx]->PtInTerritory(uid, x, y, z))
		return m_territories[idx];
	}
    }
    return NULL;		// No match
}

// Returns a CHSTerritory given the object number representing
// the territory.
CHSTerritory *CHSTerritoryArray::FindTerritory(int objnum)
{
    for (int idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx] && m_territories[idx]->GetDbref() == objnum)
	    return m_territories[idx];
    }
    // No match
    return NULL;
}

// Removes the territory with the specified object number
BOOL CHSTerritoryArray::RemoveTerritory(int objnum)
{
    int idx;
    for (idx = 0; idx < HS_MAX_TERRITORIES; idx++) {
	if (m_territories[idx] && m_territories[idx]->GetDbref() == objnum) {
	    // Remove the flag if the object is good.
	    if (hsInterface.ValidObject(m_territories[idx]->GetDbref()))
		hsInterface.UnsetFlag(m_territories[idx]->GetDbref(),
				      THING_HSPACE_TERRITORY);

	    delete m_territories[idx];
	    m_territories[idx] = NULL;
	    return TRUE;
	}
    }
    return FALSE;		// Not found
}

//
// CHSTerritory
//
CHSTerritory::CHSTerritory(void)
{
    m_objnum = -1;
    m_uid = -1;
    m_type = T_CUBIC;
}

// Returns TRUE if the given point is in this territory.
// Base class returns FALSE always.
BOOL CHSTerritory::PtInTerritory(int uid, double x, double y, double z)
{
    return FALSE;
}

// Sets the object dbref representing the territory
void CHSTerritory::SetDbref(int objnum)
{
    m_objnum = objnum;
}

// Returns the dbref of the object representing the territory
int CHSTerritory::GetDbref(void)
{
    return m_objnum;
}

// Returns the territory type for the territory.
TERRTYPE CHSTerritory::GetType(void)
{
    return m_type;
}

// Returns the UID of the territory.
int CHSTerritory::GetUID(void)
{
    return m_uid;
}

// Attempts to set the given attribute name to the specified
// value.  If successful, TRUE is returned, otherwise FALSE.
BOOL CHSTerritory::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name
    if (!strcasecmp(strName, "UID")) {
	iVal = atoi(strValue);
	m_uid = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "OBJNUM")) {
	iVal = atoi(strValue);
	m_objnum = iVal;

	// Set the territory flag
	hsInterface.SetFlag(m_objnum, THING_HSPACE_TERRITORY);
	return TRUE;
    }
    return FALSE;
}

// Writes attributes out for the territory to the given file pointer.
void CHSTerritory::SaveToFile(FILE * fp)
{
    fprintf(fp, "UID=%d\n", m_uid);
    fprintf(fp, "OBJNUM=%d\n", m_objnum);
}

//
// CHSRadialTerritory
//
CHSRadialTerritory::CHSRadialTerritory(void)
{
    m_radius = 0;
    m_cx = m_cy = m_cz = 0;

    m_type = T_RADIAL;
}

// Returns the central X value for the territory.
int CHSRadialTerritory::GetX(void)
{
    return m_cx;
}

// Returns the central Y value for the territory.
int CHSRadialTerritory::GetY(void)
{
    return m_cy;
}

// Returns the central Z value for the territory.
int CHSRadialTerritory::GetZ(void)
{
    return m_cz;
}

// Returns the radius of the territory
int CHSRadialTerritory::GetRadius(void)
{
    return m_radius;
}

// Saves all radial territory attrs to the file stream.
void CHSRadialTerritory::SaveToFile(FILE * fp)
{
    // Write base attrs first
    CHSTerritory::SaveToFile(fp);

    // Now ours
    fprintf(fp, "CX=%d\n", m_cx);
    fprintf(fp, "CY=%d\n", m_cy);
    fprintf(fp, "CZ=%d\n", m_cz);
    fprintf(fp, "RADIUS=%d\n", m_radius);
}

// Attempts to set the given attribute name to the specified
// value.  If successful, TRUE is returned, otherwise FALSE.
BOOL CHSRadialTerritory::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name
    if (!strcasecmp(strName, "CX")) {
	iVal = atoi(strValue);
	m_cx = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "CY")) {
	iVal = atoi(strValue);
	m_cy = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "CZ")) {
	iVal = atoi(strValue);
	m_cz = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "RADIUS")) {
	iVal = atoi(strValue);
	m_radius = iVal;
	return TRUE;
    } else
	return CHSTerritory::SetAttributeValue(strName, strValue);
}

// Returns TRUE if the given point is in this territory.
BOOL CHSRadialTerritory::PtInTerritory(int uid,
				       double x, double y, double z)
{
    // In our universe?
    if (uid != m_uid)
	return FALSE;

    // TRUE will be returned if the distance to the point is
    // less than our radius.
    double objDist;

    // Calculate squared distance from our center to point.
    objDist = ((m_cx - x) * (m_cx - x) +
	       (m_cy - y) * (m_cy - y) + (m_cz - z) * (m_cz - z));

    // Square radius
    double rad = (m_radius * m_radius);

    // Compare
    if (objDist <= rad)
	return TRUE;
    else
	return FALSE;
}

//
// CHSCubicTerritory
//
CHSCubicTerritory::CHSCubicTerritory(void)
{
    m_maxx = m_maxy = m_maxz = m_minx = m_miny = m_minz = 0;

    m_type = T_CUBIC;
}

// Saves all cubic territory attrs to the file stream.
void CHSCubicTerritory::SaveToFile(FILE * fp)
{
    // Write base attrs first
    CHSTerritory::SaveToFile(fp);

    // Now ours
    fprintf(fp, "MINX=%d\n", m_minx);
    fprintf(fp, "MINY=%d\n", m_miny);
    fprintf(fp, "MINZ=%d\n", m_minz);
    fprintf(fp, "MAXX=%d\n", m_maxx);
    fprintf(fp, "MAXY=%d\n", m_maxy);
    fprintf(fp, "MAXZ=%d\n", m_maxz);
}

// Attempts to set the given attribute name to the specified
// value.  If successful, TRUE is returned, otherwise FALSE.
BOOL CHSCubicTerritory::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name
    if (!strcasecmp(strName, "MINX")) {
	iVal = atoi(strValue);
	m_minx = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MINY")) {
	iVal = atoi(strValue);
	m_miny = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MINZ")) {
	iVal = atoi(strValue);
	m_minz = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MAXX")) {
	iVal = atoi(strValue);
	m_maxx = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MAXY")) {
	iVal = atoi(strValue);
	m_maxy = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MAXZ")) {
	iVal = atoi(strValue);
	m_maxz = iVal;
	return TRUE;
    } else
	return CHSTerritory::SetAttributeValue(strName, strValue);
}

// Returns TRUE if the given point is in this territory.
BOOL CHSCubicTerritory::PtInTerritory(int uid,
				      double x, double y, double z)
{
    // In our universe?
    if (uid != m_uid)
	return FALSE;

    // TRUE will be returned if the point is between the min
    // and max coordinates of us.
    if ((x <= m_maxx) && (x >= m_minx) &&
	(y <= m_maxy) && (y >= m_miny) && (z <= m_maxz) && (z >= m_minz))
	return TRUE;

    return FALSE;
}

// Returns the specified minimum value for the territory.
int CHSCubicTerritory::GetMinX(void)
{
    return m_minx;
}

// Returns the specified minimum value for the territory.
int CHSCubicTerritory::GetMinY(void)
{
    return m_miny;
}

// Returns the specified minimum value for the territory.
int CHSCubicTerritory::GetMinZ(void)
{
    return m_minz;
}

// Returns the specified maximum value for the territory.
int CHSCubicTerritory::GetMaxX(void)
{
    return m_maxx;
}

// Returns the specified maximum value for the territory.
int CHSCubicTerritory::GetMaxY(void)
{
    return m_maxy;
}

// Returns the specified maximum value for the territory.
int CHSCubicTerritory::GetMaxZ(void)
{
    return m_maxz;
}
