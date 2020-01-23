#include "hscopyright.h"
#include "hsmissile.h"
#include "hsuniverse.h"
#include "hsutils.h"
#include "hsinterface.h"
#include "hspace.h"
#include "hsconf.h"
#include "ansi.h"
#include <stdlib.h>
#include <string.h>

extern double d2cos_table[];
extern double d2sin_table[];


//
// CHSMTube stuff
//
CHSMTube::CHSMTube(void)
{
    m_loaded = FALSE;
    m_reloading = FALSE;
    m_time_to_reload = 0;

    m_missile_bay = NULL;
}

// Returns an integer indicating the status of the tube.
int CHSMTube::GetStatusInt(void)
{
    // Loaded?
    if (m_loaded)
	return 0;

    // Reloading?
    if (m_reloading)
	return m_time_to_reload;

    // Must be empty and not loading
    return -1;
}

// Missiles require a target lock to fire
BOOL CHSMTube::RequiresLock(void)
{
    return TRUE;
}

// Indicates whether the missile tube can fire upon the
// given type of CHSObject.
BOOL CHSMTube::CanAttackObject(CHSObject * cObj)
{
    // Only attack ships
    if (cObj->GetType() == HST_SHIP)
	return TRUE;

    return FALSE;
}

BOOL CHSMTube::IsReady(void)
{
    if (m_reloading)
	return FALSE;

    if (!m_loaded)
	return FALSE;

    return TRUE;
}

// Handles the status integer sent to the weapon from an
// outside function.
void CHSMTube::SetStatus(int stat)
{
    // 0 indicates loaded.
    // -1 indicates not loaded.
    // Any other number indicates loading time
    if (!stat) {
	m_loaded = TRUE;
    } else if (stat < 0) {
	m_loaded = FALSE;
    } else {
	m_loaded = FALSE;
	m_reloading = TRUE;
	m_time_to_reload = stat;
    }
}

// Cyclic stuff for missile tubes
void CHSMTube::DoCycle(void)
{
    m_change = STAT_NOCHANGE;

    if (m_reloading)
	Reload();
}

// Missile types are configurable
BOOL CHSMTube::Configurable(void)
{
    return TRUE;
}

// Returns the status of the missile tube
char *CHSMTube::GetStatus(void)
{
    static char tbuf[32];

    if (m_loaded)
	return "Loaded";

    if (m_reloading) {
	sprintf(tbuf, "(%d) Loading", m_time_to_reload);
	return tbuf;
    }

    return "Empty";
}

// Configures the missile tube to a given type of
// weapon.
BOOL CHSMTube::Configure(int type)
{
    CHSDBWeapon *cDBWeap;

    // Are we loaded?
    if (m_loaded)
	return FALSE;

    // Are we reloading?
    if (m_reloading)
	return FALSE;

    // Find the weapon in the global array.  It has to
    // be a missile.
    cDBWeap = waWeapons.GetWeapon(type, HSW_MISSILE);
    if (!cDBWeap)
	return FALSE;

    // Set our parent, which is the missile
    m_parent = cDBWeap;

    // Set the class
    m_class = type;

    return TRUE;
}

// If the missile tube has no parent, it is not configured
// for a specific type of missile, so handle this for the
// name.
char *CHSMTube::GetName(void)
{
    if (!m_parent)
	return "Not Configured";

    return m_parent->m_name;
}

BOOL CHSMTube::Unload(void)
{
    if (!m_loaded)
	return FALSE;

    // If reloading, cancel it, and give the missile back
    // to storage.
    if (m_reloading) {
	m_time_to_reload = 0;
	m_reloading = FALSE;
	m_loaded = FALSE;

	// Give the missile back
	if (m_missile_bay)
	    m_missile_bay->GetMissile(m_class, -1);
	return TRUE;
    } else {
	m_loaded = FALSE;
	if (m_missile_bay)
	    m_missile_bay->GetMissile(m_class, -1);

	return TRUE;
    }
}

void CHSMTube::AttackObject(CHSObject * cSource,
			    CHSObject * cTarget,
			    CHSConsole * cConsole, int iSysType)
{
    dbref dbUser;

    // Grab the user of the console.
    dbUser = hsInterface.ConsoleUser(cConsole->m_objnum);

    if (cSource->GetType() == HST_SHIP) {
	CHSSysCloak *cCloak;
	CHSShip *ptr;
	float rval;

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
    // Can we attack that object?
    if (!CanAttackObject(cTarget)) {
	if (dbUser != NOTHING)
	    hsStdError(dbUser,
		       "You cannot attack that target with that weapon.");
    }
    // Create a missile object, and put it in space
    CHSMissile *cMiss;
    cMiss = new CHSMissile;
    cMiss->SetUID(cSource->GetUID());

    // Add it to the universe
    CHSUniverse *uDest;
    uDest = uaUniverses.FindUniverse(cMiss->GetUID());
    if (!uDest) {
	if (dbUser != NOTHING)
	    notify(dbUser,
		   "Error finding a universe to put the missile in.");
	delete cMiss;
	return;
    }
    uDest->AddObject(cMiss);

    // Set missile coords
    cMiss->SetX(cSource->GetX());
    cMiss->SetY(cSource->GetY());
    cMiss->SetZ(cSource->GetZ());

    // Set missile type
    cMiss->SetParent((CHSDBMissile *) m_parent);

    // Set source info
    cMiss->SetSourceConsole(cConsole);
    cMiss->SetSourceObject(cSource);
    cMiss->SetTargetObject(cTarget);
    cMiss->SetName(m_parent->m_name);

    // Set missile dbref to some very high number that's
    // probably not duplicated.
    cMiss->SetDbref(getrandom(10000) + 8534);

    if (dbUser != NOTHING)
	hsStdError(dbUser, "Missile launched.");

    m_loaded = FALSE;
}

BOOL CHSMTube::Reload(void)
{
    // Check to see if we're reloading.
    // If not, then begin it.
    if (!m_reloading) {
	// Are we already loaded?
	if (m_loaded)
	    return FALSE;

	// Pull a missile from storage .. or try to.
	if (!m_missile_bay)
	    return FALSE;

	if (!m_missile_bay->GetMissile(m_class))
	    return FALSE;

	m_reloading = TRUE;
	m_time_to_reload = GetReloadTime();
    } else {
	m_time_to_reload--;

	if (m_time_to_reload <= 0) {
	    m_change = STAT_READY;
	    m_reloading = FALSE;
	    m_loaded = TRUE;
	}
    }
    return TRUE;
}

// Returns attribute information in a string.
char *CHSMTube::GetAttrInfo(void)
{
    static char rval[128];

    sprintf(rval,
	    "R: %-5d S: %-3d V : %d",
	    GetRange(), GetStrength(), GetSpeed());
    return rval;
}

BOOL CHSMTube::Loadable(void)
{
    return TRUE;
}

int CHSMTube::Reloading(void)
{
    if (!m_reloading)
	return 0;

    return m_time_to_reload;
}

UINT CHSMTube::GetReloadTime(void)
{
    CHSDBMissile *dbMis;

    if (!m_parent)
	return 0;

    // Return the reload time
    dbMis = (CHSDBMissile *) m_parent;
    return dbMis->m_reload_time;
}

UINT CHSMTube::GetRange(void)
{
    CHSDBMissile *dbMis;

    if (!m_parent)
	return 0;

    dbMis = (CHSDBMissile *) m_parent;
    return dbMis->m_range;
}

UINT CHSMTube::GetStrength(void)
{
    CHSDBMissile *dbMis;

    if (!m_parent)
	return 0;

    dbMis = (CHSDBMissile *) m_parent;
    return dbMis->m_strength;
}

UINT CHSMTube::GetSpeed(void)
{
    CHSDBMissile *dbMis;

    if (!m_parent)
	return 0;

    dbMis = (CHSDBMissile *) m_parent;
    return dbMis->m_speed;
}

UINT CHSMTube::GetTurnRate(void)
{
    CHSDBMissile *dbMis;

    if (!m_parent)
	return 0;

    dbMis = (CHSDBMissile *) m_parent;
    return dbMis->m_turnrate;
}

// Sets the missile bay source for the tube, which is where
// the tube will draw its missiles from.
void CHSMTube::SetMissileBay(CHSMissileBay * mBay)
{
    m_missile_bay = mBay;
}


//
// CHSMissileBay stuff
//
CHSMissileBay::CHSMissileBay(void)
{
    int idx;

    // Initialize storage
    for (idx = 0; idx < HS_MAX_MISSILES; idx++)
	m_storage[idx] = NULL;
}

CHSMissileBay::~CHSMissileBay(void)
{
    int idx;

    // Free up any missile storage slots
    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (m_storage[idx])
	    delete m_storage[idx];
    }
}


// Tries to set a remaining missile storage value for
// the given type of missile.  If the type isn't 
// already shown in storage, the type is added IF the
// weapon type is valid.  If the remaining value is greater
// than the maximum storage value, then the maximum is raised.
BOOL CHSMissileBay::SetRemaining(int type, int remaining)
{
    UINT idx;


    // Try to find the type in storage
    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (m_storage[idx]) {
	    if (m_storage[idx]->m_class == type) {

		// If remaining is negative, erase
		// the storage.
		if (remaining < 0) {
		    delete m_storage[idx];
		    m_storage[idx] = NULL;
		} else {
		    m_storage[idx]->m_remaining = remaining;

		    // Is number larger than max?  If so, raise
		    // max.
		    if (remaining > m_storage[idx]->m_maximum)
			m_storage[idx]->m_maximum = remaining;

		}
		// We're done, so return.
		return TRUE;
	    }
	}
    }

    // Type was not found, so let's see if the missile
    // is a valid type.
    if (!waWeapons.GetWeapon(type, HSW_MISSILE))
	return FALSE;

    // Find the first available slot for storage
    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (!m_storage[idx])
	    break;
    }
    if (idx == HS_MAX_MISSILES)
	return FALSE;

    // Allocate space
    m_storage[idx] = new HSMISSILESTORE;
    m_storage[idx]->m_class = type;
    m_storage[idx]->m_maximum = remaining;
    m_storage[idx]->m_remaining = remaining;

    // Looks good.
    return TRUE;
}

// Tries to pull a missile from storage.  If the missile
// doesn't exist or the slot is empty, FALSE is returned.
// Otherwise, a value of TRUE indicates success, and the
// storage slot is decremented.
BOOL CHSMissileBay::GetMissile(int type, int amt)
{
    UINT idx;

    // Find the storage slot
    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (m_storage[idx]) {
	    // Right type of weapon?
	    if (m_storage[idx]->m_class == type) {
		// Enough left?
		if (m_storage[idx]->m_remaining >= amt) {
		    m_storage[idx]->m_remaining -= amt;
		    return TRUE;
		} else {
		    // Not enough left
		    return FALSE;
		}
	    }
	}
    }

    // Not found
    return FALSE;
}

// Saves missile bay information to the specified file stream.
void CHSMissileBay::SaveToFile(FILE * fp)
{
    int idx;

    // Write out our missile storage infos
    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (!m_storage[idx])
	    continue;

	fprintf(fp, "MISSILETYPE=%d\n", m_storage[idx]->m_class);
	fprintf(fp, "MISSILEMAX=%d\n", m_storage[idx]->m_maximum);
	fprintf(fp, "MISSILEREMAINING=%d\n", m_storage[idx]->m_remaining);
    }

    fprintf(fp, "MISSILEBAYEND\n");
}

// Loads missile bay information from the specified file stream.
void CHSMissileBay::LoadFromFile(FILE * fp)
{
    char tbuf[128];
    char strKey[64];
    char strValue[64];
    char *ptr;
    int type;

    // Load from the file stream until end of file or, more
    // like, MISSILEBAYEND indicator.
    type = -1;
    while (fgets(tbuf, 128, fp)) {
	// Strip newline chars
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	// Extract key/value pairs
	extract(tbuf, strKey, 0, 1, '=');
	extract(tbuf, strValue, 1, 1, '=');

	// Check for key match
	if (!strcmp(strKey, "MISSILEBAYEND")) {
	    // This is the end.
	    return;
	} else if (!strcmp(strKey, "MISSILETYPE")) {
	    type = atoi(strValue);
	} else if (!strcmp(strKey, "MISSILEMAX")) {
	    // Set the maximum value for the type.
	    // No need to error check.
	    SetRemaining(type, atoi(strValue));
	} else if (!strcmp(strKey, "MISSILEREMAINING")) {
	    // Set missiles remaining for the type.
	    SetRemaining(type, atoi(strValue));
	}
    }
}

// Returns the number of missiles left for the given type of missile.
UINT CHSMissileBay::GetMissilesLeft(int iclass)
{
    int idx;

    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (m_storage[idx] && m_storage[idx]->m_class == iclass)
	    return m_storage[idx]->m_remaining;
    }

    return 0;
}

// Reteurns the maximum storage value for the given type of missile.
UINT CHSMissileBay::GetMaxMissiles(int iclass)
{
    int idx;

    for (idx = 0; idx < HS_MAX_MISSILES; idx++) {
	if (m_storage[idx] && m_storage[idx]->m_class == iclass)
	    return m_storage[idx]->m_maximum;
    }

    return 0;
}

// Retrieves the HSMISSILESTORE structure for a given
// slot.
HSMISSILESTORE *CHSMissileBay::GetStorage(int slot)
{
    if (slot < 0 || slot >= HS_MAX_MISSILES)
	return NULL;

    return m_storage[slot];
}


//
// CHSMissile implementation
//
CHSMissile::CHSMissile(void)
{
    m_target = NULL;
    m_source_object = NULL;
    m_source_console = NULL;
    m_parent = NULL;
    m_timeleft = -1;
    m_delete_me = FALSE;
    m_target_hit = FALSE;

    m_type = HST_MISSILE;

    CHSVector cvTmp(1, 0, 0);
    m_motion_vector = cvTmp;

    m_xyheading = m_zheading = 0;
}

void CHSMissile::SetSourceObject(CHSObject * cSource)
{
    m_source_object = cSource;
}

void CHSMissile::SetSourceConsole(CHSConsole * cConsole)
{
    m_source_console = cConsole;
}

void CHSMissile::SetTargetObject(CHSObject * cObj)
{
    m_target = cObj;
}

void CHSMissile::DoCycle(void)
{
    // Do we need to delete ourselves?
    if (m_delete_me) {
	delete this;
	return;
    }
    // If we have no target or no parent, remove us
    // from space.
    if (!m_target || !m_parent || !m_target->IsActive()) {
	CHSUniverse *uSource;

	uSource = uaUniverses.FindUniverse(m_uid);
	if (uSource)
	    uSource->RemoveObject(this);

	// Delete me
	m_delete_me = TRUE;
	return;
    }
    // Do we know how much time is left until we die?
    if (m_timeleft < 0) {
	CalculateTimeLeft();
    } else
	m_timeleft--;

    // Missile depleted?
    if (m_timeleft == 0) {
	// Remove us from space
	CHSUniverse *cSource;
	cSource = uaUniverses.FindUniverse(m_uid);
	if (cSource)
	    cSource->RemoveObject(this);

	m_delete_me = TRUE;
	return;
    }
    // Change the missile heading toward the target
    ChangeHeading();

    // Move us closer to the target.
    MoveTowardTarget();

    // The MoveTowardTarget() checks to see if the missile hits,
    // so we just have to check our flag.
    if (m_target_hit) {
	// BOOM!

	HitTarget();

	// Remove us from space
	CHSUniverse *cSource;
	cSource = uaUniverses.FindUniverse(m_uid);
	if (cSource)
	    cSource->RemoveObject(this);

	m_delete_me = TRUE;	// Delete next time around
    }
}

// Moves the missile along the current trajectory.
void CHSMissile::MoveTowardTarget(void)
{
    // Grab parent missile object
    CHSDBMissile *dbMiss;
    dbMiss = (CHSDBMissile *) m_parent;

    // Calculate per second speed
    double speed;

    speed = dbMiss->m_speed * (.0002778 * HSCONF.cyc_interval);

    double tX, tY, tZ;
    double dist;

    tX = m_target->GetX();
    tY = m_target->GetY();
    tZ = m_target->GetZ();
    dist = Dist3D(m_x, m_y, m_z, tX, tY, tZ);

    // Determine if speed is > distance, which might indicate
    // that the missile is within hitting range.  If this is
    // true, move the missile just the distance to the target
    // and see if the coordinates are relatively close.  It's
    // possible that the missile is close, but the target is
    // evading, in which case we just have to move the missile
    // it's regular distance.
    if (speed > dist) {
	// Move just the distance
	double n_x, n_y, n_z;
	n_x = m_x + m_motion_vector.i() * dist;
	n_y = m_y + m_motion_vector.j() * dist;
	n_z = m_z + m_motion_vector.k() * dist;

	// Is new distance within 1 unit?
	dist = Dist3D(n_x, n_y, n_z, tX, tY, tZ);
	if (dist <= 1) {
	    m_target_hit = TRUE;
	    return;
	}
    }
    // At this point we know we didn't hit, so just move
    // regularly.
    m_x += m_motion_vector.i() * speed;
    m_y += m_motion_vector.j() * speed;
    m_z += m_motion_vector.k() * speed;
}

// Changes the heading of the missile to head toward the target.
void CHSMissile::ChangeHeading(void)
{
    BOOL bChange;		// Indicates if any turning has occurred;

    // Calculate X, Y, and Z difference vector
    double tX, tY, tZ;

    tX = m_target->GetX();
    tY = m_target->GetY();
    tZ = m_target->GetZ();

    int xyang = XYAngle(m_x, m_y, tX, tY);
    int zang = ZAngle(m_x, m_y, m_z, tX, tY, tZ);

    // Get the turn rate
    CHSDBMissile *dbMiss;
    dbMiss = (CHSDBMissile *) m_parent;
    int iTurnRate = dbMiss->m_turnrate;

    bChange = FALSE;

    // Check for change in zheading
    if (zang != m_zheading) {
	if (zang > m_zheading) {
	    if ((zang - m_zheading) > iTurnRate)
		m_zheading += iTurnRate;
	    else
		m_zheading = zang;
	} else {
	    if ((m_zheading - zang) > iTurnRate)
		m_zheading -= iTurnRate;
	    else
		m_zheading = zang;
	}
	bChange = TRUE;
    }
    // Now handle any changes in the XY plane.
    int iDiff;
    if (xyang != m_xyheading) {
	if (abs(m_xyheading - xyang) < 180) {
	    if (abs(m_xyheading - xyang) < iTurnRate)
		m_xyheading = xyang;
	    else if (m_xyheading > xyang) {
		m_xyheading -= iTurnRate;

		if (m_xyheading < 0)
		    m_xyheading += 360;
	    } else {
		m_xyheading += iTurnRate;

		if (m_xyheading > 359)
		    m_xyheading -= 360;
	    }
	} else if (((360 - xyang) + m_xyheading) < 180) {
	    iDiff = (360 - xyang) + m_xyheading;
	    if (iDiff < iTurnRate) {
		m_xyheading = xyang;
	    } else {
		m_xyheading -= iTurnRate;

		if (m_xyheading < 0)
		    m_xyheading += 360;
	    }
	} else if (((360 - m_xyheading) + xyang) < 180) {
	    iDiff = (360 - m_xyheading) + xyang;
	    if (iDiff < iTurnRate) {
		m_xyheading = xyang;
	    } else {
		m_xyheading += iTurnRate;

		if (m_xyheading > 359)
		    m_xyheading -= 360;
	    }
	} else			// This should never be true, but just in case.
	{
	    m_xyheading += iTurnRate;

	    if (m_xyheading > 359)
		m_xyheading -= 360;
	}
	bChange = TRUE;
    }
    // Check to see if we need to recompute trajectory.
    if (bChange) {
	zang = m_zheading;
	if (zang < 0)
	    zang += 360;

	CHSVector tvec(d2sin_table[m_xyheading] * d2cos_table[zang],
		       d2cos_table[m_xyheading] * d2cos_table[zang],
		       d2sin_table[zang]);
	m_motion_vector = tvec;
    }
}

// Sets the current heading of the missile, which is usually
// used when the missile is launched toward a target.
void CHSMissile::SetHeading(int xy, int z)
{
    m_xyheading = xy;
    m_zheading = z;
}

// Handles hitting the target and contacting the target
// object to negotiate damage.
void CHSMissile::HitTarget(void)
{
    CHSDBMissile *dbMiss;
    dbMiss = (CHSDBMissile *) m_parent;

    // Tell the target it's damaged
    m_target->HandleDamage(m_source_object,
			   (CHSWeapon *) this,
			   dbMiss->m_strength, m_source_console,
			   HSS_NOTYPE);
}

void CHSMissile::CalculateTimeLeft(void)
{
    double speed;
    int range;

    // Time until our death is calculated by
    // speed and range.
    CHSDBMissile *dbMiss;
    dbMiss = (CHSDBMissile *) m_parent;
    speed = dbMiss->m_speed;
    range = dbMiss->m_range;

    // Bring speed down to per second level.
    speed = speed * .0002778;

    m_timeleft = (int) (range / speed);
}

char CHSMissile::GetObjectCharacter(void)
{
    return 'x';
}

char *CHSMissile::GetObjectColor(void)
{
    static char tbuf[32];

    sprintf(tbuf, "%s%s", ANSI_HILITE, ANSI_YELLOW);
    return tbuf;
}

void CHSMissile::HandleDamage(CHSObject * cSource,
			      CHSWeapon * cWeapon,
			      int strength,
			      CHSConsole * cConsole, int iSysType)
{
    int player;

    player = hsInterface.ConsoleUser(cConsole->m_objnum);

    // Any hit destroys the missile.
    hsStdError(player, "Your shot explodes the target missile!");


    // Remove the missile from space
    CHSUniverse *uSource;

    // Find my universe
    uSource = uaUniverses.FindUniverse(m_uid);
    if (uSource) {
	// Remove me from active space.
	uSource->RemoveObject(this);
    }
    // Delete me
    m_delete_me = TRUE;
}
