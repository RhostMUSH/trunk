#include "hscopyright.h"
#include <stdlib.h>

#ifndef WIN32
#include <strings.h>
#endif
extern "C" {
#include "autoconf.h"
};

#include "hseng.h"
#include "hsutils.h"
#include "hsuniverse.h"
#include "ansi.h"
#include "hsengines.h"
#include "hsconf.h"
#include "hspace.h"
#include "hsinterface.h"

struct SYSTEMTYPE {
    char *name;
    HSS_TYPE type;
};

// An array of available systems that can exist on ships.
// If you create a new system type, you need to add its name
// here.
SYSTEMTYPE hs_system_list[] = {
    "Reactor", HSS_REACTOR,
    "Life Support", HSS_LIFE_SUPPORT,
    "Internal Computer", HSS_COMPUTER,
    "Engines", HSS_ENGINES,
    "Sensor Array", HSS_SENSORS,
    "Maneuv. Thrusters", HSS_THRUSTERS,
    "Fore Shield", HSS_FORE_SHIELD,
    "Aft Shield", HSS_AFT_SHIELD,
    "Starboard Shield", HSS_STARBOARD_SHIELD,
    "Port Shield", HSS_PORT_SHIELD,
    "Fuel System", HSS_FUEL_SYSTEM,
    "Jump Drive", HSS_JUMP_DRIVE,
    "Comm. Array", HSS_COMM,
    "Cloaking Device", HSS_CLOAK,
    "Tachyon Sensor Array", HSS_TACHYON,
    "Fictional System", HSS_FICTIONAL,
    "Damage Control", HSS_DAMCON,
    "Comm. Jammer", HSS_JAMMER,
    "Tractor Beam", HSS_TRACTOR,
    NULL
};

char *hsGetEngSystemName(int type)
{
    int idx;

    for (idx = 0; hs_system_list[idx].name; idx++) {
	if (hs_system_list[idx].type == type)
	    return hs_system_list[idx].name;
    }
    return NULL;
}

HSS_TYPE hsGetEngSystemType(char *name)
{
    int idx;
    int len;

    len = strlen(name);
    for (idx = 0; hs_system_list[idx].name; idx++) {
	if (!strncasecmp(name, hs_system_list[idx].name, len))
	    return hs_system_list[idx].type;
    }
    return HSS_NOTYPE;
}

//
// CHSSystemArray stuff
//
CHSSystemArray::CHSSystemArray(void)
{
    m_SystemHead = m_SystemTail = NULL;

    m_nSystems = 0;
    m_uPowerUse = 0;
}

// Returns one of the random systems
CHSEngSystem *CHSSystemArray::GetRandomSystem(void)
{
    int iRoll;
    CHSEngSystem *ptr;
    int idx;

    // Roll the dice from 1 .. nSystems
    iRoll = getrandom(m_nSystems);

    // Resulting iRoll is 0 .. n-1.  Find the system.
    idx = 0;
    for (ptr = m_SystemHead; ptr; ptr = ptr->GetNext()) {
	if (idx == iRoll)
	    return ptr;
	idx++;
    }

    // Hrm.  No system found, so return NULL.
    return NULL;
}

BOOL CHSSystemArray::AddSystem(CHSEngSystem * cSys)
{
    // Add system to the list or start new list.

    if (!m_SystemHead) {
	m_SystemHead = cSys;
	m_SystemTail = cSys;
    } else {
	m_SystemTail->SetNext(cSys);
	cSys->SetPrev(m_SystemTail);
	m_SystemTail = cSys;
    }

    m_nSystems++;
    return TRUE;
}

BOOL CHSSystemArray::DelSystem(CHSEngSystem * cSys)
{
    // Delete the system from the list and close the ends again :).
    CHSEngSystem *pSys;
    CHSEngSystem *nSys;
    pSys = cSys->GetPrev();
    nSys = cSys->GetNext();

    if (!nSys && !pSys) {
	m_SystemHead = m_SystemTail = NULL;
	return TRUE;
    } else if (!nSys) {
	pSys->SetNext(NULL);
	m_SystemTail = pSys;
    } else if (!pSys) {
	nSys->SetPrev(NULL);
	m_SystemHead = nSys;
    } else {
	pSys->SetNext(nSys);
	nSys->SetPrev(pSys);
    }

    delete cSys;
    m_nSystems--;
    return TRUE;
}



// This operator can be used to duplicate the contents
// of this array into another array that was passed in.
void CHSSystemArray::operator =(CHSSystemArray & cCopyFrom)
{
    CHSEngSystem *ptr;
    CHSEngSystem *tmp;

    // Delete any systems we have loaded already.
    ptr = m_SystemHead;
    while (ptr) {
	tmp = ptr;
	ptr = ptr->GetNext();
	delete tmp;
    }
    m_SystemHead = m_SystemTail = NULL;

    // Copy number of systems
    m_nSystems = cCopyFrom.m_nSystems;

    // Now duplicate the systems
    for (ptr = cCopyFrom.m_SystemHead; ptr; ptr = ptr->GetNext()) {
	// Dup the system
	tmp = ptr->Dup();

	// If the system we're cloning doesn't have a parent,
	// then it's a default system, so it's now our parent.
	tmp->SetParentSystem(ptr);

	// Add it to our linked list.
	if (!m_SystemHead) {
	    m_SystemHead = m_SystemTail = tmp;
	} else {
	    tmp->SetPrev(m_SystemTail);
	    m_SystemTail->SetNext(tmp);
	    m_SystemTail = tmp;
	}
    }

    // We're finished!
}

// Handles cycling systems, powering them down, etc.
void CHSSystemArray::DoCycle(void)
{
    CHSEngSystem *cSys;

    // Tell the systems to cycle.  At the same time,
    // calculate power used.
    m_uPowerUse = 0;
    for (cSys = m_SystemHead; cSys; cSys = cSys->GetNext()) {
	cSys->DoCycle();
	m_uPowerUse += cSys->GetCurrentPower();
    }
}

// Updates the current power usage.  Call this after
// changing a power setting on a system.
void CHSSystemArray::UpdatePowerUse(void)
{
    CHSEngSystem *cSys;

    m_uPowerUse = 0;
    for (cSys = m_SystemHead; cSys; cSys = cSys->GetNext()) {
	m_uPowerUse += cSys->GetCurrentPower();
    }
}

// Returns current power usage
UINT CHSSystemArray::GetPowerUse(void)
{
    return m_uPowerUse;
}

// Returns the head of the linked list of systems
CHSEngSystem *CHSSystemArray::GetHead(void)
{
    return m_SystemHead;
}

// Returns number of systems
UINT CHSSystemArray::NumSystems(void)
{
    return m_nSystems;
}

void CHSSystemArray::SaveToFile(FILE * fp)
{
    UINT priority = 0;
    CHSEngSystem *ptr;
    for (ptr = GetHead(); ptr; ptr = ptr->GetNext()) {
	fprintf(fp, "SYSTEMDEF\n");
	fprintf(fp, "SYSTYPE=%d\n", ptr->GetType());
	ptr->SaveToFile(fp);

	// Record the system priority for rebuilding priority
	// on reload.
	if (ptr->GetType() != HSS_REACTOR)
	    fprintf(fp, "PRIORITY=%d\n", priority++);

	fprintf(fp, "SYSTEMEND\n");
    }
}

// Looks for, and returns, a system given a specific system
// type.
CHSEngSystem *CHSSystemArray::GetSystem(HSS_TYPE systype)
{
    CHSEngSystem *ptr;

    // Run down the list, looking for a match on the type
    for (ptr = m_SystemHead; ptr; ptr = ptr->GetNext()) {
	if (ptr->GetType() == systype)
	    return ptr;
    }

    // Not found.
    return NULL;
}

// Attempts to allocate a new system of the
// given type.  The new system pointer is returned.
CHSEngSystem *CHSSystemArray::NewSystem(HSS_TYPE tType)
{
    CHSEngSystem *cSys;

    // Determine type, and allocate system.
    switch (tType) {
    case HSS_ENGINES:
	cSys = new CHSSysEngines;
	break;
    case HSS_SENSORS:
	cSys = new CHSSysSensors;
	break;
    case HSS_COMPUTER:
	cSys = new CHSSysComputer;
	break;
    case HSS_THRUSTERS:
	cSys = new CHSSysThrusters;
	break;
    case HSS_COMM:
	cSys = new CHSSysComm;
	break;
    case HSS_LIFE_SUPPORT:
	cSys = new CHSSysLifeSupport;
	break;
    case HSS_REACTOR:
	cSys = new CHSReactor;
	break;
    case HSS_FUEL_SYSTEM:
	cSys = new CHSFuelSystem;
	break;
    case HSS_FORE_SHIELD:
	cSys = new CHSSysShield;
	cSys->SetType(HSS_FORE_SHIELD);
	break;
    case HSS_AFT_SHIELD:
	cSys = new CHSSysShield;
	cSys->SetType(HSS_AFT_SHIELD);
	break;
    case HSS_PORT_SHIELD:
	cSys = new CHSSysShield;
	cSys->SetType(HSS_PORT_SHIELD);
	break;
    case HSS_STARBOARD_SHIELD:
	cSys = new CHSSysShield;
	cSys->SetType(HSS_STARBOARD_SHIELD);
	break;
    case HSS_JUMP_DRIVE:
	cSys = new CHSJumpDrive;
	break;
    case HSS_CLOAK:
	cSys = new CHSSysCloak;
	break;
    case HSS_TACHYON:
	cSys = new CHSSysTach;
	break;
    case HSS_FICTIONAL:
	cSys = new CHSFictional;
	break;
    case HSS_DAMCON:
	cSys = new CHSDamCon;
	break;
    case HSS_JAMMER:
	cSys = new CHSSysJammer;
	break;
    case HSS_TRACTOR:
	cSys = new CHSSysTractor;
	break;
    default:
	cSys = NULL;
	break;
    }

    return cSys;
}

// Looks for, and returns, a system by name.  That name
// must only match the first n characters of the string
// passed in.  The first system name to at least match the
// characters in the given string is returned.
CHSEngSystem *CHSSystemArray::GetSystemByName(char *strName)
{
    CHSEngSystem *ptr;
    int len;

    len = strlen(strName);
    for (ptr = m_SystemHead; ptr; ptr = ptr->GetNext()) {
	if (!strncasecmp(strName, ptr->GetName(), len))
	    return ptr;
    }
    return NULL;
}

// Allows a given system to be bumped up or down in the
// array.  If iDir is negative, the system moves down, otherwise
// up.
BOOL CHSSystemArray::BumpSystem(CHSEngSystem * cSys, int iDir)
{
    CHSEngSystem *ptr;
    CHSEngSystem *ptr2;

    if (!m_SystemHead)
	return FALSE;

    if (iDir < 0) {
	// Move the system down

	if (!cSys->GetNext())	// System is tail?
	    return FALSE;

	ptr = cSys->GetNext();
	if (!ptr->GetNext())	// Next system is tail?
	{
	    m_SystemTail = cSys;
	    cSys->SetNext(NULL);
	} else {
	    ptr2 = ptr->GetNext();
	    ptr2->SetPrev(cSys);
	    cSys->SetNext(ptr2);
	}

	if (!cSys->GetPrev())	// System is head?
	{
	    m_SystemHead = ptr;
	    ptr->SetPrev(NULL);
	} else {
	    ptr2 = cSys->GetPrev();
	    ptr2->SetNext(ptr);
	    ptr->SetPrev(ptr2);
	}

	// Swap the two systems.
	ptr->SetNext(cSys);
	cSys->SetPrev(ptr);
    } else if (iDir > 0) {
	// Move the system up

	if (!cSys->GetPrev())	// System is head?
	    return FALSE;

	ptr = cSys->GetPrev();
	if (!ptr->GetPrev())	// Prev system is head?
	{
	    m_SystemHead = cSys;
	    cSys->SetPrev(NULL);
	} else {
	    ptr2 = ptr->GetPrev();
	    ptr2->SetNext(cSys);
	    cSys->SetPrev(ptr2);
	}

	if (!cSys->GetNext())	// System is tail?
	{
	    m_SystemTail = ptr;
	    ptr->SetNext(NULL);
	} else {
	    ptr2 = cSys->GetNext();
	    ptr2->SetPrev(ptr);
	    ptr->SetNext(ptr2);
	}

	// Swap the two systems.
	ptr->SetPrev(cSys);
	cSys->SetNext(ptr);
    }
    // System was either moved, or iDir was 0.
    return TRUE;
}

//
// Generic CHSEngSystem stuff
//
CHSEngSystem::CHSEngSystem(void)
{
    // Initialize our attributes
    m_optimal_power = NULL;
    m_tolerance = NULL;
    m_current_power = 0;
    m_stress = 0;
    m_damage_level = DMG_NONE;
    m_name = NULL;

    // This is NULL, indicating a default system with default
    // values.
    m_parent = NULL;

    m_ownerObj = NULL;		// No one owns us right now

    m_next = m_prev = NULL;

    // By default, systems are visible to the players
    m_bVisible = TRUE;
}

// Destructor
CHSEngSystem::~CHSEngSystem(void)
{
    // Free up our variables
    if (m_optimal_power)
	delete m_optimal_power;

    if (m_tolerance)
	delete m_tolerance;
}

// We can use this function to have the system force a power check
// to be the power levels are wacky.
void CHSEngSystem::CheckSystemPower(void)
{
    UINT max;

    max = GetOptimalPower();

    // Don't allow levels below 0.  Level 0 is just deactivation.
    if (m_current_power < 0) {
	m_current_power = 0;
	return;
    }
    // Only allow overloading to 150%.
    if (m_current_power > (max * 1.5)) {
	m_current_power = (int) (max * 1.5);
	return;

    }
}

void CHSEngSystem::SetOwner(CHSObject * cOwner)
{
    m_ownerObj = cOwner;
}

// Returns TRUE or FALSE for whether the system is "visible"
// for use by players, power allocation, etc.
BOOL CHSEngSystem::IsVisible(void)
{
    return m_bVisible;
}

// Returns the status of the system, which is usually Online
// or Offline.  If you overload this, don't return damage and
// stuff like that.  This is just a general system status
// string.
char *CHSEngSystem::GetStatus(void)
{
    if (m_current_power > 0)
	return "Online";
    else
	return "Offline";
}

// Call this function when you want the system to update itself
// during a cycle (usually each second).  It handles recharging,
// stress calculation, etc.
//
// If you're deriving a new system from CHSEngSystem, you _can_
// override this to handle your own cyclic stuff.
void CHSEngSystem::DoCycle(void)
{
    float fVal;
    int iOptPower;
    char tbuf[128];

    // If the system is inoperable .. do nothing.
    if (GetDamageLevel() == DMG_INOPERABLE)
	return;

    // Do Anything?
    if (!m_stress && !m_current_power)
	return;

    // Calculate stress
    iOptPower = GetOptimalPower();	// Optimal power including damage
    if ((m_current_power > iOptPower) || (m_stress > 0)) {
	// Calculate percent overload/underload
	fVal = (m_current_power - iOptPower) / (float) iOptPower;
	fVal *= 100;

	// If overloaded, tolerance hinders stress.
	// If underloaded, tolerance reduces stress faster.
	if (fVal > 0)
	    fVal *= (float) (HS_STRESS_INCREASE / (float) GetTolerance());
	else {
	    // Make sure stress is reduced at least minimally
	    // at 100% power allocation.
	    if (!fVal)
		fVal = -1;
	    fVal *= (float) (HS_STRESS_INCREASE * GetTolerance());
	}
	m_stress += fVal;
	if (m_stress > 100)
	    m_stress = 100;
	if (m_stress < 0)
	    m_stress = 0;

	// Check to see if system gets damaged. HAHAHAHA!
	int iDmgRoll;
	int iStressRoll;
	time_t tCurrentSecs;
	tCurrentSecs = time(NULL);

	// We check every 15 seconds
	if (!(tCurrentSecs % 15)) {
	    // Roll the dice for damage
	    iDmgRoll = getrandom(100);

	    // Roll for stress .. mama needs a new car!
	    iStressRoll = (int) m_stress;
	    iStressRoll = getrandom(iStressRoll);

	    // Did we damage?
	    if (iDmgRoll < iStressRoll) {
		// Ouch.  Hurt.
		HS_DAMAGE prev;
		HS_DAMAGE newlvl;

		prev = GetDamageLevel();
		newlvl = DoDamage();
		if (newlvl != prev) {
		    // Damage changed, so give some messages.
		    if (m_ownerObj && (m_ownerObj->GetType() == HST_SHIP)) {
			CHSShip *cShip;
			cShip = (CHSShip *) m_ownerObj;
			sprintf(tbuf,
				"%s%s-%s A warning light flashes, indicating that the %s system has taken damaged.",
				ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
				GetName());
			cShip->HandleMessage(tbuf, MSG_ENGINEERING, NULL);

			// Force a power check
			CheckSystemPower();
		    }
		}
	    }
	}
    }
}

// Allows the type of system to be set.
// Only do this if it's not a derived system.
void CHSEngSystem::SetType(HSS_TYPE systype)
{
    m_type = systype;
}

// Prints out system information to a player
void CHSEngSystem::GiveSystemInfo(int player)
{
    notify(player, unsafe_tprintf("Name     : %s", GetName()));
    notify(player,
	   unsafe_tprintf("Visible  : %s", m_bVisible ? "YES" : "NO"));
    notify(player, unsafe_tprintf("Damage   : %d", m_damage_level));
    notify(player, unsafe_tprintf("Tolerance: %d", GetTolerance()));
    notify(player,
	   unsafe_tprintf("Opt Power: %d", GetOptimalPower(FALSE)));
    notify(player, unsafe_tprintf("Cur Power: %d", m_current_power));
    notify(player, unsafe_tprintf("Stress   : %.1f", m_stress));
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSEngSystem::GetAttrValueString(void)
{
    static char rval[512];

    sprintf(rval,
	    "VISIBLE=%s,DAMAGE=%d,TOLERANCE=%d,OPTIMAL POWER=%d,CURRENT POWER=%d,STRESS=%.1f",
	    m_bVisible ? "YES" : "NO", m_damage_level,
	    GetTolerance(),
	    GetOptimalPower(FALSE), m_current_power, m_stress);

    return rval;
}

// Allocates a new CHSEngSystem object, copies in some
// important information from this object, and gives it
// back to the calling function.
CHSEngSystem *CHSEngSystem::Dup(void)
{
    CHSEngSystem *ptr;

    ptr = new CHSEngSystem;
    ptr->SetType(m_type);

    // For custom systems added from the game side,
    // we need to check for the name.
    if (m_name) {
	ptr->m_name = new char[strlen(m_name) + 1];
	strcpy(ptr->m_name, m_name);
    }

    ptr->m_bVisible = m_bVisible;

    // There's no initialization here that wasn't
    // already done by the constructor.  Just return
    // the variable.
    return ptr;
}

HS_DAMAGE CHSEngSystem::GetDamageLevel(void)
{
    return m_damage_level;
}

//
// Linked list stuff with CHSEngSystem
CHSEngSystem *CHSEngSystem::GetNext(void)
{
    return m_next;
}

CHSEngSystem *CHSEngSystem::GetPrev(void)
{
    return m_prev;
}

void CHSEngSystem::SetNext(CHSEngSystem * cObj)
{
    m_next = cObj;
}

void CHSEngSystem::SetPrev(CHSEngSystem * cObj)
{
    m_prev = cObj;
}

//
// End linked list stuff
//

void CHSEngSystem::SetParentSystem(CHSEngSystem * cSys)
{
    m_parent = cSys;
}

//
// Value access functions
//
int CHSEngSystem::GetCurrentPower(void)
{
    return m_current_power;
}

// Sets the name of the system.
void CHSEngSystem::SetName(char *strName)
{
    // If strValue contains a null, clear our local setting
    if (!*strName) {
	if (m_name) {
	    delete m_name;
	    m_name = NULL;
	    return;
	}

	return;
    }

    if (m_name)
	delete m_name;

    m_name = new char[strlen(strName) + 1];

    strcpy(m_name, strName);
    return;
}

// Does one level of damage to the system and returns the
// new damage level.
HS_DAMAGE CHSEngSystem::DoDamage(void)
{
    HS_DAMAGE lvl;

    // Get the current level
    lvl = GetDamageLevel();

    // If it's not inoperable, increase it
    if (lvl != DMG_INOPERABLE) {
	switch (lvl) {
	case DMG_NONE:
	    m_damage_level = DMG_LIGHT;
	    break;
	case DMG_LIGHT:
	    m_damage_level = DMG_MEDIUM;
	    break;
	case DMG_MEDIUM:
	    m_damage_level = DMG_HEAVY;
	    break;
	case DMG_HEAVY:
	    m_damage_level = DMG_INOPERABLE;
	    break;
	}
	if (GetDamageLevel() == DMG_INOPERABLE)
	    SetCurrentPower(0);
    }
    return GetDamageLevel();
}

// Reduces the damage on the system one level and returns the
// new damage level.
HS_DAMAGE CHSEngSystem::ReduceDamage(void)
{
    HS_DAMAGE lvl;

    // Get the current level.
    lvl = GetDamageLevel();

    // If damaged, reduce it
    if (lvl != DMG_NONE) {
	switch (lvl) {
	case DMG_LIGHT:
	    m_damage_level = DMG_NONE;
	    break;
	case DMG_MEDIUM:
	    m_damage_level = DMG_LIGHT;
	    break;
	case DMG_HEAVY:
	    m_damage_level = DMG_MEDIUM;
	    break;
	case DMG_INOPERABLE:
	    m_damage_level = DMG_HEAVY;
	    break;
	}
    }

    return GetDamageLevel();
}

// Reduces the damage on the system to the specified level
// and returns the previous damage level.
HS_DAMAGE CHSEngSystem::ReduceDamage(HS_DAMAGE lvl)
{
    HS_DAMAGE prev;

    // Store previous level
    prev = GetDamageLevel();

    // Set current level .. we perform no error checking.
    m_damage_level = lvl;

    return prev;
}

char *CHSEngSystem::GetName()
{
    int idx;

    if (!m_name) {
	// Do we have a parent?
	if (m_parent) {
	    return m_parent->GetName();
	} else {
	    // Search for the name in the global list
	    for (idx = 0; hs_system_list[idx].name; idx++) {
		if (hs_system_list[idx].type == m_type)
		    return hs_system_list[idx].name;
	    }
	    return "No Name";
	}
    } else if (m_name && strlen(m_name) > 0)
	return m_name;

    // System not found in the list
    return "No Name";
}

// Returns the optimal (maximum, no damage) power level for
// the system.  The bAdjusted variable can be set to TRUE to return
// the optimal power for the current damage level.  If this variable
// is false, then the optimal power not including damage is returned.
int CHSEngSystem::GetOptimalPower(BOOL bAdjusted)
{
    int lvl;

    // Use some logic here.
    if (!m_optimal_power) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    lvl = 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    lvl = m_parent->GetOptimalPower();
	}
    } else
	lvl = *m_optimal_power;

    // Calculate damage?
    if (bAdjusted) {
	lvl = (int) (lvl * (1 - (.25 * GetDamageLevel())));
    }

    return lvl;
}

int CHSEngSystem::GetTolerance(void)
{
    // Use some logic here.
    if (!m_tolerance) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    return m_parent->GetTolerance();
	}
    } else
	return *m_tolerance;
}

float CHSEngSystem::GetStress(void)
{
    return m_stress;
}

HSS_TYPE CHSEngSystem::GetType(void)
{
    return m_type;
}

//
// End value access functions
//

void CHSEngSystem::CutPower(int level)
{
    // Do nothing, overridable.
}

void CHSEngSystem::PowerUp(int level)
{
    // Do nothing, overridable.
}

// Sets the current power level for the system.
BOOL CHSEngSystem::SetCurrentPower(int level)
{
    UINT max;

    max = GetOptimalPower();

    // Don't allow levels below 0.  Level 0 is just deactivation.
    if (level < 0)
	return FALSE;

    // Only allow overloading to 150%.
    if (level > (max * 1.5))
	return FALSE;

    if (level < m_current_power)
	CutPower(level);

    if (m_current_power == 0 && level > 0)
	PowerUp(level);

    m_current_power = level;

    return TRUE;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSEngSystem::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;
    float fVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "CURRENT POWER")) {
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	m_current_power = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "NAME")) {
	SetName(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "DAMAGE")) {
	iVal = atoi(strValue);
	m_damage_level = (HS_DAMAGE) iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "OPTIMAL POWER")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_optimal_power) {
		delete m_optimal_power;
		m_optimal_power = NULL;
		return TRUE;
	    }

	    return FALSE;
	}

	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_optimal_power)
	    m_optimal_power = new int;

	*m_optimal_power = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "TOLERANCE")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_tolerance) {
		delete m_tolerance;
		m_tolerance = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_tolerance)
	    m_tolerance = new int;

	*m_tolerance = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "STRESS")) {
	fVal = (float) atof(strValue);
	if ((fVal < 0) || (fVal > 100))
	    return FALSE;

	m_stress = fVal;
	return TRUE;
    } else if (!strcasecmp(strName, "VISIBLE")) {
	m_bVisible = atoi(strValue);
	return TRUE;
    }
    return FALSE;		// Attr name not matched
}

// Returns the value of the specified attribute or NULL if
// not a valid attribute.
char *CHSEngSystem::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[64];

    // Look for the attribute
    *rval = '\0';
    if (!strcasecmp(strName, "CURRENT POWER")) {
	sprintf(rval, "%d", m_current_power);
    } else if (!strcasecmp(strName, "DAMAGE")) {
	sprintf(rval, "%d", m_damage_level);
    } else if (!strcasecmp(strName, "OPTIMAL POWER")) {
	sprintf(rval, "%d", GetOptimalPower(bAdjusted));
    } else if (!strcasecmp(strName, "TOLERANCE")) {
	sprintf(rval, "%d", GetTolerance());
    } else if (!strcasecmp(strName, "STRESS")) {
	sprintf(rval, "%.2f", m_stress);
    } else if (!strcasecmp(strName, "NAME")) {
	strcpy(rval, GetName());
    } else
	return NULL;		// No attribute found

    return rval;		// Contains something, even if null.
}

// Outputs system information to a file.  This could be
// the class database or another database of objects that
// contain systems.
void CHSEngSystem::SaveToFile(FILE * fp)
{
    // Output RAW values.  Do not call any Get() functions,
    // because those will return local or parent values.  We
    // ONLY want to output local values.
    if (m_name)
	fprintf(fp, "NAME=%s\n", m_name);
    if (m_tolerance)
	fprintf(fp, "TOLERANCE=%d\n", *m_tolerance);
    if (m_optimal_power)
	fprintf(fp, "OPTIMAL POWER=%d\n", *m_optimal_power);

    fprintf(fp, "STRESS=%.2f\n", m_stress);
    fprintf(fp, "DAMAGE=%d\n", m_damage_level);
    fprintf(fp, "CURRENT POWER=%d\n", m_current_power);
    fprintf(fp, "VISIBLE=%d\n", m_bVisible);
}

//
// CHSSysSensors stuff
//
CHSSysSensors::CHSSysSensors(void)
{
    int idx;

    m_type = HSS_SENSORS;

    m_sensor_rating = NULL;
    m_nContacts = 0;

    // Initialize contact arrays
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++)
	m_contact_list[idx] = NULL;

    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	m_new_contacts[idx] = NULL;
	m_lost_contacts[idx] = NULL;
    }
}

// Outputs system information to a file.  This could be
// the class database or another database of objects that
// contain systems.
void CHSSysSensors::SaveToFile(FILE * fp)
{
    // Call base class FIRST
    CHSEngSystem::SaveToFile(fp);

    // Ok to output, so we print out our information
    // specific to this system.
    if (m_sensor_rating)
	fprintf(fp, "SENSOR RATING=%d\n", *m_sensor_rating);
}

// Prints out system information to a player
void CHSSysSensors::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player, unsafe_tprintf("SENSOR RATING: %d", GetSensorRating()));
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysSensors::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,SENSOR RATING=%d",
	    CHSEngSystem::GetAttrValueString(), GetSensorRating());

    return rval;
}

// This just sets the index to the first lost contact slot.
// Always use GetNextLostContact() to retrieve the first and
// all subsequent contacts.
SENSOR_CONTACT *CHSSysSensors::GetFirstLostContact(void)
{
    for (uContactSlot = 0; uContactSlot < HS_MAX_NEW_CONTACTS;
	 uContactSlot++) {
	if (m_lost_contacts[uContactSlot])
	    return m_lost_contacts[uContactSlot];
    }
    return NULL;
}

SENSOR_CONTACT *CHSSysSensors::GetNextLostContact(void)
{
    while (uContactSlot < HS_MAX_NEW_CONTACTS) {
	if (m_lost_contacts[uContactSlot]) {
	    uContactSlot++;
	    return m_lost_contacts[uContactSlot - 1];
	}
	uContactSlot++;
    }
    return NULL;
}

// This just sets the index to the first new contact slot.
// Always use GetNextNewContact() to retrieve the first and
// all subsequent contacts.
SENSOR_CONTACT *CHSSysSensors::GetFirstNewContact(void)
{
    for (uContactSlot = 0; uContactSlot < HS_MAX_NEW_CONTACTS;
	 uContactSlot++) {
	if (m_new_contacts[uContactSlot])
	    return m_new_contacts[uContactSlot];
    }
    return NULL;
}

SENSOR_CONTACT *CHSSysSensors::GetNextNewContact(void)
{
    while (uContactSlot < HS_MAX_NEW_CONTACTS) {
	if (m_new_contacts[uContactSlot]) {
	    uContactSlot++;
	    return m_new_contacts[uContactSlot - 1];
	}
	uContactSlot++;
    }
    return NULL;
}

void CHSSysSensors::ClearContacts(void)
{
    int idx;

    // Clear out our current contact array.
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	m_contact_list[idx] = NULL;

	if (m_contact_list[idx]) {
	    delete m_contact_list[idx];
	    m_contact_list[idx] = NULL;
	}
    }

    // Clear out any lost contacts that were moved
    // out of the current contact array.
    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	m_lost_contacts[idx] = NULL;

	if (m_lost_contacts[idx]) {
	    delete m_lost_contacts[idx];
	    m_lost_contacts[idx] = NULL;
	}
    }

    m_nContacts = 0;
}

// This will handle sensor sweeping.
void CHSSysSensors::DoCycle(void)
{
    int idx, idx2;
    CHSUniverse *uSource;
    CHSObject *cObj, *dObj;
    SENSOR_CONTACT *cContact;
    int iDetectLevel;		// 0, 1, or 2 for how to detect it.
    double MyX, MyY, MyZ;
    double NebX, NebY, NebZ;
    double TargetX, TargetY, TargetZ;
    double dIdentDist, dDetectDist, dDistance;
    float fCloakEffect;
    float fNebulaEffect;
    double dOverload;

    // Call the base class first to handle stress
    CHSEngSystem::DoCycle();

    // No power? Don't cycle.
    if (!m_current_power) {
	// Check to be sure the user didn't turn off power to
	// save sensor contacts for a later time.
	if (m_nContacts > 0)
	    ClearContacts();

	return;
    }
    // If we have no owner, then we're not sweeping
    // for contacts.
    if (!m_ownerObj)
	return;

    // If the owner is not active, then we're not sweeping.
    if (!m_ownerObj->IsActive()) {
	if (m_nContacts > 0) {
	    ClearContacts();
	}
	return;
    }
    // If we belong to a ship, and we're in hyperspace,
    // figure out what to do.
    int iDetectCap;
    iDetectCap = 2;		// Fully sense by default
    if (m_ownerObj && m_ownerObj->GetType() == HST_SHIP) {
	CHSShip *cShip;
	cShip = (CHSShip *) m_ownerObj;

	// Figure out if we should sweep at all
	if (cShip->InHyperspace()) {
	    if (HSCONF.sense_hypervessels == 0)
		iDetectCap = 0;
	    else if (HSCONF.sense_hypervessels == 1)
		iDetectCap = 1;
	}
    }
    // Calculate overload (or underload)
    int iOptPower = GetOptimalPower(FALSE);
    dOverload = m_current_power / (float) (iOptPower + .00001);

    // Clear out our new and lost contact arrays.
    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	m_new_contacts[idx] = NULL;

	if (m_lost_contacts[idx]) {
	    delete m_lost_contacts[idx];
	    m_lost_contacts[idx] = NULL;
	}
    }

    // Flag all of our current contacts as unverified
    m_nContacts = 0;
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (m_contact_list[idx]) {
	    m_nContacts++;
	    m_contact_list[idx]->bVerified = FALSE;

	    // Change updated statuses to IDENTIFIED
	    if (m_contact_list[idx]->status == UPDATED)
		m_contact_list[idx]->status = IDENTIFIED;
	}
    }

    // Look for objects in our universe

    uSource = uaUniverses.FindUniverse(m_ownerObj->GetUID());
    if (!uSource)
	return;

    // Run through all active objects, checking distance,
    // and seeing if they're on sensors.
    MyX = m_ownerObj->GetX();
    MyY = m_ownerObj->GetY();
    MyZ = m_ownerObj->GetZ();


    for (idx = 0; iDetectCap && (idx < HS_MAX_ACTIVE_OBJECTS); idx++) {
	cObj = uSource->GetActiveUnivObject(idx);
	if (!cObj)
	    continue;

	iDetectLevel = iDetectCap;	// Sense only to our ability

	// Object visible?
	if (!cObj->IsVisible())
	    continue;

	// Is it us?
	if (cObj->GetDbref() == m_ownerObj->GetDbref())
	    continue;

	// Grab target coordinates
	TargetX = cObj->GetX();
	TargetY = cObj->GetY();
	TargetZ = cObj->GetZ();

	fNebulaEffect = 1;

	// See if there are any Nebulae near the vessel.
	for (idx2 = 0; (idx2 < HS_MAX_ACTIVE_OBJECTS); idx2++) {
	    dObj = uSource->GetActiveUnivObject(idx2);

	    if (!dObj)
		continue;

	    if (dObj->GetType() != HST_NEBULA)
		continue;

	    if (dObj == cObj)
		continue;

	    NebX = dObj->GetX();
	    NebY = dObj->GetY();
	    NebZ = dObj->GetZ();

	    dDistance =
		Dist3D(NebX, NebY, NebZ, TargetX, TargetY, TargetZ);

	    if (dDistance > dObj->GetSize() * 100)
		continue;

	    // Calculate effect of Nebula on sensor report for our ship.
	    CHSNebula *tNebula;
	    tNebula = (CHSNebula *) dObj;
	    fNebulaEffect = 1 / (tNebula->GetDensity() * 1.00);
	}


	// If it's a ship, check for cloaking.
	if (cObj->GetType() == HST_SHIP) {
	    CHSShip *ptr;
	    CHSShip *ptr2;

	    ptr = (CHSShip *) cObj;
	    ptr2 = (CHSShip *) m_ownerObj;

	    // Check if the ship is in hyperspace and
	    // how we should handle it.
	    if (ptr->InHyperspace()) {
		// Determine what to do with the
		// contact.
		if (HSCONF.sense_hypervessels == 0)
		    continue;
		else if (HSCONF.sense_hypervessels == 1)
		    iDetectLevel = 1;
	    }
	    // Grab cloaking effect, where 1 is complete
	    // visibility.
	    fCloakEffect = ptr->CloakingEffect();
	    fCloakEffect += ptr2->TachyonEffect();
	    if (fCloakEffect > 1)
		fCloakEffect = 1;
	} else {
	    // Just some object
	    fCloakEffect = 1;
	}

	// Calculate distance to object
	dDistance = Dist3D(MyX, MyY, MyZ, TargetX, TargetY, TargetZ);

	// Calculate ranges, including cloaking.
	dDetectDist =
	    cObj->GetSize() / 2.0 * fCloakEffect * fNebulaEffect *
	    (HSCONF.detectdist * 2.0) * GetSensorRating();
	dIdentDist =
	    cObj->GetSize() / 2.0 * fCloakEffect * fNebulaEffect *
	    (HSCONF.identdist * 2.0) * GetSensorRating();

	// Range is increased (or decreased) based on overloading.
	dDetectDist *= dOverload;
	dIdentDist *= dOverload;

	// Now see if the object is currently on sensors.
	cContact = GetContact(cObj);

	// Object out of range?
	if ((dDistance > dDetectDist) ||
	    (dDistance > HSCONF.max_sensor_range)) {
	    // Do nothing. It'll get lost from sensors
	    // simply because it hasn't been flagged
	    // as verified.
	} else {
	    // On sensors now?
	    if (cContact) {
		// Check to see if we need to update its status.
		if ((cContact->status == DETECTED) &&
		    (dDistance <= dIdentDist) && (iDetectLevel == 2)) {
		    UpdateContact(cContact);
		}
		// This contact is a sure bet.
		cContact->bVerified = TRUE;
	    } else {
		// New contact
		// Is it immediately in identification range?
		if ((dDistance <= dIdentDist) && (iDetectLevel == 2))
		    NewContact(cObj, IDENTIFIED);
		else
		    NewContact(cObj, DETECTED);

		m_nContacts++;
	    }
	}
    }

    // Check for any unverified contacts
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (m_contact_list[idx] && (!m_contact_list[idx]->bVerified)) {
	    LoseContact(m_contact_list[idx]);
	    m_nContacts--;
	}
    }
}

// Returns the number of contacts on sensors.
UINT CHSSysSensors::NumContacts(void)
{
    return m_nContacts;
}

// Returns a 4-digit, random sensor id for a new contact.
// This id will not match any other on sensors.
UINT CHSSysSensors::RandomSensorID(void)
{
    UINT idx;
    UINT num;
    BOOL bInUse;

    bInUse = TRUE;
    while (bInUse) {
	num = getrandom(8999) + 1000;

	bInUse = FALSE;
	// Make sure it's not in use
	for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	    if (m_contact_list[idx] && m_contact_list[idx]->m_id == num) {
		bInUse = TRUE;
		break;
	    }
	}
    }

    // It's ok.
    return num;
}

// Adds a new CHSObject to the list of contacts.
void CHSSysSensors::NewContact(CHSObject * cObj, CONTACT_STATUS stat)
{
    SENSOR_CONTACT *cNewContact;
    int idx;

    // Allocate a new contact object
    cNewContact = new SENSOR_CONTACT;

    cNewContact->m_obj = cObj;
    cNewContact->status = stat;
    cNewContact->m_id = RandomSensorID();

    // Now add it to the contact list and new contact list
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (!m_contact_list[idx]) {
	    m_contact_list[idx] = cNewContact;
	    break;
	}
    }

    // Error check
    if (idx == HS_MAX_CONTACTS) {
	hs_log
	    ("WARNING: Sensor array has reached maximum number of contacts.");
	delete cNewContact;
    }
    // New contacts list
    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	if (!m_new_contacts[idx]) {
	    m_new_contacts[idx] = cNewContact;
	    break;
	}
    }

    // The contact is definitely on sensors.
    cNewContact->bVerified = TRUE;
}

// Updates the status of an existing contact to indicate
// identified.  This is similar to obtaining a new contact.
void CHSSysSensors::UpdateContact(SENSOR_CONTACT * cContact)
{
    int idx;

    cContact->status = UPDATED;

    // Add it to the new list.  It's not really new, but
    // other functions can query it for the UPDATED status
    // to see that it's not really a new contact.
    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	if (!m_new_contacts[idx]) {
	    m_new_contacts[idx] = cContact;
	    break;
	}
    }
}

// Can be called externally to force the removal of an
// object from sensors.
void CHSSysSensors::LoseObject(CHSObject * cObj)
{
    int idx;

    // Run down the contact list, looking for this object.
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (m_contact_list[idx]) {
	    CHSObject *cTmp;
	    cTmp = m_contact_list[idx]->m_obj;
	    if (cTmp->GetDbref() == cObj->GetDbref()) {
		delete m_contact_list[idx];
		m_contact_list[idx] = NULL;
		break;
	    }
	}
    }
}

// Removes a contact from sensors and indicates that it's lost.
void CHSSysSensors::LoseContact(SENSOR_CONTACT * cContact)
{
    int idx;

    // Run down the contact list, looking for it.
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (m_contact_list[idx] &&
	    (m_contact_list[idx]->m_id == cContact->m_id)) {
	    m_contact_list[idx] = NULL;
	    break;
	}
    }

    // Now add it to the lost contact list
    for (idx = 0; idx < HS_MAX_NEW_CONTACTS; idx++) {
	if (!m_lost_contacts[idx]) {
	    m_lost_contacts[idx] = cContact;
	    break;
	}
    }
}

// Tries to find a sensor contact given a contact id
SENSOR_CONTACT *CHSSysSensors::GetContactByID(int id)
{
    int idx;

    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (!m_contact_list[idx])
	    continue;

	if (m_contact_list[idx]->m_id == id)
	    return m_contact_list[idx];
    }

    // Not found
    return NULL;
}

// Tries to find a sensor contact given an object.
SENSOR_CONTACT *CHSSysSensors::GetContact(CHSObject * cObj)
{
    int idx;

    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	if (!m_contact_list[idx])
	    continue;

	// Compare dbrefs
	if (m_contact_list[idx]->m_obj->GetDbref() == cObj->GetDbref())
	    return m_contact_list[idx];
    }
    return NULL;
}

// Returns the sensor contact in a given array slot.
SENSOR_CONTACT *CHSSysSensors::GetContact(int slot)
{
    if ((slot < 0) || (slot >= HS_MAX_CONTACTS))
	return NULL;

    return m_contact_list[slot];
}

// A derived implementation of the Dup() in the base class.
CHSEngSystem *CHSSysSensors::Dup(void)
{
    CHSSysSensors *ptr;

    ptr = new CHSSysSensors;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSSysSensors::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "SENSOR RATING")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_sensor_rating) {
		delete m_sensor_rating;
		m_sensor_rating = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_sensor_rating)
	    m_sensor_rating = new int;

	*m_sensor_rating = iVal;
	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSSysSensors::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "SENSOR RATING"))
	sprintf(rval, "%d", GetSensorRating());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

UINT CHSSysSensors::GetSensorRating(void)
{
    // Use some logic here.
    if (!m_sensor_rating) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    CHSSysSensors *ptr = (CHSSysSensors *) m_parent;

	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    return ptr->GetSensorRating();
	}
    } else
	return *m_sensor_rating;
}

CHSReactor::CHSReactor(void)
{
    m_type = HSS_REACTOR;

    m_maximum_output = NULL;
    m_desired_output = 0;
    m_output = 0;
    m_efficiency = NULL;

    // The reactor is not visible as other systems are
    m_bVisible = FALSE;

    // By default, no fuel source .. unlimited fuel
    m_fuel_source = NULL;
}

//
// CHSReactor implementation
CHSReactor::~CHSReactor(void)
{
    if (m_maximum_output)
	delete m_maximum_output;
}

void CHSReactor::SetFuelSource(CHSFuelSystem * cSource)
{
    m_fuel_source = cSource;
}

// Returns the desired output setting for the reactor
UINT CHSReactor::GetDesiredOutput(void)
{
    return m_desired_output;
}

// Returns the current output for the reactor
UINT CHSReactor::GetOutput(void)
{
    return m_output;
}

// Returns the status of the reactor
char *CHSReactor::GetStatus(void)
{
    if (IsOnline())
	return "Online";
    else
	return "Offline";
}

// Returns efficiency of reactor
int CHSReactor::GetEfficiency(void)
{
    // Do we have a local value?
    if (!m_efficiency) {
	// Do we have a parent?
	if (m_parent) {
	    CHSReactor *ptr;
	    ptr = (CHSReactor *) m_parent;
	    return ptr->GetEfficiency();
	} else
	    return HSCONF.fuel_ratio;	// default of 1000 (1k)
    } else
	return *m_efficiency;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSReactor::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "MAX OUTPUT")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_maximum_output) {
		delete m_maximum_output;
		m_maximum_output = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_maximum_output)
	    m_maximum_output = new int;

	*m_maximum_output = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "DESIRED OUTPUT")) {
	iVal = atoi(strValue);
	m_desired_output = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "CURRENT OUTPUT")) {
	iVal = atoi(strValue);
	m_output = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "EFFICIENCY")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_efficiency) {
		delete m_efficiency;
		m_efficiency = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_efficiency)
	    m_efficiency = new int;

	*m_efficiency = iVal;
	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSReactor::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "MAX OUTPUT"))
	sprintf(rval, "%d", GetMaximumOutput(bAdjusted));
    else if (!strcasecmp(strName, "DESIRED OUTPUT"))
	sprintf(rval, "%d", m_desired_output);
    else if (!strcasecmp(strName, "CURRENT OUTPUT"))
	sprintf(rval, "%d", m_output);
    else if (!strcasecmp(strName, "EFFICIENCY"))
	sprintf(rval, "%d", GetEfficiency());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Prints out system information to a player
void CHSReactor::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player,
	   unsafe_tprintf("MAX OUTPUT: %d", GetMaximumOutput(FALSE)));
    notify(player, unsafe_tprintf("DESIRED OUTPUT: %d", m_desired_output));
    notify(player, unsafe_tprintf("CURRENT OUTPUT: %d", m_output));
    notify(player, unsafe_tprintf("EFFICIENCY: %d", GetEfficiency()));
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSReactor::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,MAX OUTPUT=%d,DESIRED OUTPUT=%d,CURRENT OUTPUT=%d,EFFICIENCY=%d",
	    CHSEngSystem::GetAttrValueString(),
	    GetMaximumOutput(FALSE),
	    m_desired_output, m_output, GetEfficiency());

    return rval;
}

// A derived implementation of the Dup() in the base class.
CHSEngSystem *CHSReactor::Dup(void)
{
    CHSReactor *ptr;

    ptr = new CHSReactor;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

// Outputs system information to a file.  This could be
// the class database or another database of objects that
// contain systems.
void CHSReactor::SaveToFile(FILE * fp)
{
    // Call base class FIRST
    CHSEngSystem::SaveToFile(fp);

    // Ok to output, so we print out our information
    // specific to this system.
    if (m_maximum_output)
	fprintf(fp, "MAX OUTPUT=%d\n", *m_maximum_output);

    fprintf(fp, "DESIRED OUTPUT=%d\n", m_desired_output);
    fprintf(fp, "CURRENT OUTPUT=%d\n", m_output);

    if (m_efficiency)
	fprintf(fp, "EFFICIENCY=%d\n", *m_efficiency);
}

// Attempts to set the reactor to a given output level
BOOL CHSReactor::SetOutputLevel(int level)
{
    UINT max;

    max = GetMaximumOutput();

    // Don't allow levels below 0.  Level 0 is just deactivation.
    if (level < 0)
	return FALSE;

    // Only allow overloading to 150%.
    if (level > (max * 1.5))
	return FALSE;

    m_desired_output = level;

    // If desired output is less than level, just cut
    // the power right to that level.  Cyclic update is
    // only for powering up, not powering down.
    if (m_desired_output < m_output) {
	m_output = m_desired_output;
    }

    return TRUE;
}

// Returns the maximum output capability of the reactor,
// accounting for damage that has occurred.
UINT CHSReactor::GetMaximumOutput(BOOL bWithDamage)
{
    double dmgperc;
    UINT uVal;

    // Use some logic here.
    if (!m_maximum_output) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    CHSReactor *ptr = (CHSReactor *) m_parent;

	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    uVal = ptr->GetMaximumOutput();
	}
    } else
	uVal = *m_maximum_output;

    if (bWithDamage) {
	// Figure in damage.  1/4 reduction per level of damage
	dmgperc = 1 - (.25 * GetDamageLevel());
	uVal = (UINT) (uVal * dmgperc);
    }
    return uVal;
}

// This is overridden from the base class because reactors
// work much different than other systems.  We have to power
// up, calculate stress based on output (not usage), etc.
void CHSReactor::DoCycle(void)
{
    UINT max;
    float fVal;

    // Check to see if we need to do anything at all.
    if (!m_output && !m_desired_output && !m_stress)
	return;

    // Maybe we incurred damage, so check to
    // see if we should cut power.  Remember, we can
    // overload to 150%, so allow for that.
    max = GetMaximumOutput();
    max = (UINT) (max * 1.5);	// 150% allowance
    if (m_output > max) {
	// Cut power
	m_output = max;
    }
    // Check to see if we should increase or decrease
    // power output.
    if (m_output < m_desired_output) {
	if (m_output < max) {
	    // It's ok to increase
	    m_output++;
	}
    } else if (m_output > m_desired_output) {
	// Decrease output to desired level.
	m_output = m_desired_output;
    }
    // Calculate stress.  In this case we don't
    // allow for 150% overload.  If it's anything over
    // 100%, we stress it.
    max = GetMaximumOutput();
    if ((m_output > max) || (m_stress > 0)) {
	// Calculate percent overload/underload.  Have to
	// use some extra float variables here because of
	// some funky problems with subtracting unsigned ints.
	float fVal2;
	fVal = (float) max;
	fVal2 = (float) m_output;
	fVal2 = fVal2 - max;
	fVal = fVal2 / fVal;
	fVal *= 100;

	// If overloaded, tolerance hinders stress.
	// If underloaded, tolerance reduces stress faster.
	if (fVal > 0)
	    fVal *= (float) (HS_STRESS_INCREASE / (float) GetTolerance());
	else {
	    // Make sure stress is reduced at least minimally
	    // at 100% power allocation.
	    if (!fVal)
		fVal = -1;
	    fVal *= (float) (HS_STRESS_INCREASE * GetTolerance());
	}
	m_stress += fVal;
	if (m_stress > 100)
	    m_stress = 100;
	if (m_stress < 0)
	    m_stress = 0;
    }
    // Figure out fuel usage, and consume it.
    if (m_fuel_source) {
	float fRealized;
	float fNeeded;

	// Compute fuel usage based on the ratio of consumption
	// The .0002778 is 1/3600.0 precomputed for speed
	fNeeded = (float) GetEfficiency();
	fNeeded = m_output / fNeeded;
	fNeeded *= (float) .0002778;

	if (fNeeded > 0) {
	    // Model two fuels?
	    if (HSCONF.use_two_fuels)
		fRealized = m_fuel_source->ExtractFuelUnit(fNeeded,
							   FL_REACTABLE);
	    else
		fRealized = m_fuel_source->ExtractFuelUnit(fNeeded);

	    // If the fuel realized (expended) is less than
	    // requested, the fuel tank is "empty."  Thus, 
	    // set the desired reactor power to 0.
	    if (fRealized < fNeeded) {
		char tbuf[128];

		sprintf(tbuf,
			"%s%s-%s A warning light flashes, indicating that reactable fuel supplies are depleted.",
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL);
		if (m_ownerObj)
		    m_ownerObj->HandleMessage(tbuf, MSG_ENGINEERING, NULL);

		m_desired_output = 0;
	    }
	}
    }
}

// Indicates if the reactor is "online" or not.  This doesn't
// mean whether it's powering or not.  The state of being 
// online is whether the reactor can now begin to allocate
// power to other systems.  It is defined as 5% powered.
BOOL CHSReactor::IsOnline(void)
{
    UINT max;

    // Get max reactor output, not with damage
    max = GetMaximumOutput(FALSE);

    if (m_output >= (max * .05))
	return TRUE;
    return FALSE;
}

//
// CHSSysComputer stuff
//
CHSSysComputer::CHSSysComputer(void)
{
    m_power_used = 0;
    m_type = HSS_COMPUTER;
}

CHSEngSystem *CHSSysComputer::Dup(void)
{
    CHSSysComputer *ptr;

    ptr = new CHSSysComputer;

    // No initialization to do.
    return (CHSSysComputer *) ptr;
}

// Call this function to drain power from the computer
// to your system.  If enough power is available, TRUE
// is returned, and the power is deducted from the 
// total power available.
BOOL CHSSysComputer::DrainPower(int amt)
{
    if ((m_power_used + amt) > m_current_power)
	return false;

    m_power_used += amt;
    return TRUE;
}

int CHSSysComputer::GetUsedPower(void)
{
    return m_power_used;
}

int CHSSysComputer::GetConsoles(void)
{
    int nConsoles;
    CHSShip *cShip;
    int idx;
    CHSConsole *cCons;

    cShip = (CHSShip *) m_ownerObj;
    nConsoles = 0;
    // Query consoles
    for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++) {
	cCons = cShip->GetConsole(idx);
	if (cCons) {
	    nConsoles++;
	}
    }

    return nConsoles;
}

int CHSSysComputer::GetPoweredConsoles(void)
{
    int nPConsoles;
    CHSShip *cShip;
    int idx;
    CHSConsole *cCons;

    cShip = (CHSShip *) m_ownerObj;
    nPConsoles = 0;
    // Query consoles
    for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++) {
	cCons = cShip->GetConsole(idx);
	if (cCons) {
	    if (cCons->IsOnline()) {
		nPConsoles++;
	    }
	}
    }

    return nPConsoles;
}

void CHSSysComputer::PowerUp(int level)
{
    if (m_ownerObj->GetType() == HST_SHIP) {
	CHSShip *cShip;

	cShip = (CHSShip *) m_ownerObj;

	cShip->NotifySrooms(HSCONF.computer_activating);
	if (level > 5)
	    m_power_used = 5;
	else
	    m_power_used = level;
    }
}

void CHSSysComputer::CutPower(int level)
{
    if (level >= m_power_used)
	return;

    int idx;
    CHSConsole *cCons;
    CHSShip *cShip;

    cShip = (CHSShip *) m_ownerObj;

    if (!cShip)
	return;


    for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++) {
	cCons = cShip->GetConsole(idx);
	if (cCons) {
	    if (cCons->IsOnline()) {
		cCons->PowerDown(hsInterface.ConsoleUser(cCons->m_objnum));
	    }
	}
	if (level >= m_power_used)
	    return;
    }

    if (level < 5)
	m_power_used = level;
}

// Call this function to release power you've drained.
// If you don't, the computer gets confused about how
// much power is used.  Kind of like a memory leak!
void CHSSysComputer::ReleasePower(int amt)
{
    m_power_used -= amt;
}

// Returns the surplus (or deficit) between power allocated
// to the computer and power drained.  If this is negative,
// your system should consider releasing the power.  The
// computer won't do it.
int CHSSysComputer::GetPowerSurplus(void)
{
    return (m_current_power - m_power_used);
}

// Returns the power usage for the computer system.  This
// can vary depending on the owner object for the system.
// For example, ships have consoles, weapons, etc.
int CHSSysComputer::GetOptimalPower(BOOL bDamage)
{
    UINT uPower;

    if (!m_ownerObj)
	return 0;

    if (GetDamageLevel() == DMG_INOPERABLE)
	return 0;

    // Computer requires 5 MW base power
    uPower = 5;

    // We currently only support ship computers
    switch (m_ownerObj->GetType()) {
    case HST_SHIP:
	uPower += TotalShipPower();
	break;
    }

    return uPower;
}

// Support for ship objects with computers.  This will
// query various parts of the ship (e.g. consoles) for
// power requirements.
UINT CHSSysComputer::TotalShipPower(void)
{
    UINT uPower;
    CHSShip *cShip;
    int idx;
    CHSConsole *cCons;

    cShip = (CHSShip *) m_ownerObj;
    uPower = 0;
    // Query consoles
    for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++) {
	cCons = cShip->GetConsole(idx);
	if (cCons)
	    uPower += cCons->GetMaximumPower();
    }

    return uPower;
}

//
// CHSSysThrusters implementation
//
CHSSysThrusters::CHSSysThrusters(void)
{
    m_turning_rate = NULL;
    m_type = HSS_THRUSTERS;
}

CHSSysThrusters::~CHSSysThrusters(void)
{
    if (m_turning_rate)
	delete m_turning_rate;
}

void CHSSysThrusters::PowerUp(int level)
{
    if (m_ownerObj->GetType() == HST_SHIP) {
	CHSShip *cShip;

	cShip = (CHSShip *) m_ownerObj;

	cShip->NotifySrooms(HSCONF.thrusters_activating);
    }
}

// Prints out system information to a player
void CHSSysThrusters::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player, unsafe_tprintf("TURNING RATE: %d", GetRate(FALSE)));
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysThrusters::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,TURNING RATE=%d",
	    CHSEngSystem::GetAttrValueString(), GetRate(FALSE));

    return rval;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSSysThrusters::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "TURNING RATE")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_turning_rate) {
		delete m_turning_rate;
		m_turning_rate = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_turning_rate)
	    m_turning_rate = new int;

	*m_turning_rate = iVal;

	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSSysThrusters::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "TURNING RATE"))
	sprintf(rval, "%d", GetRate(bAdjusted));
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// A derived implementation of the Dup() in the base class.
CHSEngSystem *CHSSysThrusters::Dup(void)
{
    CHSSysThrusters *ptr;

    ptr = new CHSSysThrusters;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

// Returns the turning rate for the thrusters system.  If bAdjusted
// is set to false, then the maximum raw turning rate is returned,
// otherwise, it is adjusted for damage and power levels.
UINT CHSSysThrusters::GetRate(BOOL bAdjusted)
{
    double dmgperc;
    CHSSysThrusters *ptr;
    UINT uVal;
    int iOptPower;

    // Use some logic here.
    if (!m_turning_rate) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysThrusters *) m_parent;
	    uVal = ptr->GetRate(FALSE);
	}
    } else
	uVal = *m_turning_rate;


    // Make overloading and damage adjustments?
    if (bAdjusted) {
	float fVal;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (float) m_current_power;
	fVal /= (float) iOptPower;
	uVal = (UINT) (uVal * fVal);

	// Figure in damage.  1/4 reduction per level of damage
	dmgperc = 1 - (.25 * GetDamageLevel());
	uVal = (UINT) (uVal * dmgperc);
    }
    return uVal;
}

// Outputs system information to a file.  This could be
// the class database or another database of objects that
// contain systems.
void CHSSysThrusters::SaveToFile(FILE * fp)
{
    // Call base class FIRST
    CHSEngSystem::SaveToFile(fp);

    // Now output our local values.
    if (m_turning_rate)
	fprintf(fp, "TURNING RATE=%d\n", *m_turning_rate);
}


//
// CHSSysLifeSupport Stuff
//
CHSSysLifeSupport::CHSSysLifeSupport(void)
{
    m_type = HSS_LIFE_SUPPORT;

    // Initially 100% air left
    m_perc_air = 100;
}

CHSEngSystem *CHSSysLifeSupport::Dup(void)
{
    CHSSysLifeSupport *ptr;

    ptr = new CHSSysLifeSupport;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

void CHSSysLifeSupport::PowerUp(int level)
{
    if (!m_ownerObj)
	return;

    if (m_ownerObj->GetType() == HST_SHIP) {
	CHSShip *cShip;

	cShip = (CHSShip *) m_ownerObj;

	cShip->NotifySrooms(HSCONF.life_activating);
    }
}

void CHSSysLifeSupport::CutPower(int level)
{
    if (level == 0) {
	if (m_ownerObj->GetType() == HST_SHIP) {
	    CHSShip *cShip;

	    cShip = (CHSShip *) m_ownerObj;

	    cShip->NotifySrooms(HSCONF.life_cut);
	}
    }
}


// The DoCycle() for life support will handle increasing or
// decreasing life support percentages (e.g. air).
void CHSSysLifeSupport::DoCycle(void)
{
    double dLifePerc;

    // Do base cycle stuff
    CHSEngSystem::DoCycle();

    // Based on power availability, determine what
    // percentage of life support should be maintained.
    dLifePerc = (double) GetOptimalPower() + .00001;
    dLifePerc = (m_current_power / dLifePerc) * 100;

    // Handle life support goodies
    if (m_perc_air < dLifePerc) {
	// Increase air availability by 1 percent
	m_perc_air += 1;

	// Did we increase too much?
	if (m_perc_air > dLifePerc)
	    m_perc_air = (float) dLifePerc;
    } else if (m_perc_air > dLifePerc) {
	// Decrease air availability by 1/3 percent because
	// of people breathing.
	m_perc_air = (float) (m_perc_air - .33333);

	// Did we decrease too much?
	if (m_perc_air < dLifePerc)
	    m_perc_air = (float) dLifePerc;
    }
}

// Returns the current 0 - 100 percentage of air in the system
double CHSSysLifeSupport::GetAirLeft(void)
{
    return m_perc_air;
}

//
// CHSSysShield stuff
//
CHSSysShield::CHSSysShield(void)
{
    m_type = HSS_FORE_SHIELD;
    m_shield_type = ST_ABSORPTION;	// Might as well choose a default

    m_strength = 0;
    m_max_strength = NULL;
    m_regen_rate = NULL;
}

CHSEngSystem *CHSSysShield::Dup(void)
{
    CHSSysShield *ptr;

    ptr = new CHSSysShield;

    ptr->m_shield_type = m_shield_type;
    ptr->m_type = m_type;	// Needs to be set for shield location

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

void CHSSysShield::DoCycle(void)
{
    double rate;
    int max;

    // Do base stuff first
    CHSEngSystem::DoCycle();

    // Do anything?
    if (!m_current_power)
	return;

    // Depending on the type of shield, we have
    // to do different things.
    if (m_shield_type == ST_ABSORPTION) {
	// Regen if we have power.
	rate = GetRegenRate();
	max = (int) GetMaxStrength();
	if ((m_strength < max) && (rate > 0)) {
	    // Increase by the given point rate
	    m_strength += rate;

	    if (m_strength > max)
		m_strength = max;
	}
    }
    // Don't do anything for deflector shields
}

// Does damage to the shield (maybe), and returns the number
// of points not handled by the shield.
UINT CHSSysShield::DoDamage(int iPts)
{
    int rval;
    float strAffect = 100;
    CHSObject *dObj;
    double NebX, NebY, NebZ, dDistance;
    int idx;
    int Strength2;
    CHSUniverse *uSource;

    uSource = uaUniverses.FindUniverse(m_ownerObj->GetUID());
    if (!uSource)
	return 0;

    // See if there are any Nebulae near the vessel.
    for (idx = 0; (idx < HS_MAX_ACTIVE_OBJECTS); idx++) {
	dObj = uSource->GetActiveUnivObject(idx);

	if (!dObj)
	    continue;

	if (dObj->GetType() != HST_NEBULA)
	    continue;

	NebX = dObj->GetX();
	NebY = dObj->GetY();
	NebZ = dObj->GetZ();

	dDistance =
	    Dist3D(NebX, NebY, NebZ, m_ownerObj->GetX(),
		   m_ownerObj->GetY(), m_ownerObj->GetZ());

	if (dDistance > dObj->GetSize() * 100)
	    continue;

	// Calculate effect of Nebula on sensor report for our ship.
	CHSNebula *tNebula;
	tNebula = (CHSNebula *) dObj;
	strAffect = tNebula->GetShieldaff();
    }

    Strength2 = m_strength * (strAffect / 100);

    // What type of shield are we?
    if (m_shield_type == ST_DEFLECTOR) {
	// Subtract our shield strength from the damage
	// points.
	rval = iPts - Strength2;
	if (rval < 0)
	    rval = 0;
	return rval;
    } else if (m_shield_type == ST_ABSORPTION) {
	// Knock off points from the shields.
	Strength2 -= iPts;

	m_strength = Strength2;

	// If the strength has gone negative, that means
	// the damage points were too much for the shield
	// to handle.  Thus, return the absolute value of
	// the shield strength, and set it to zero.
	if (m_strength < 0) {
	    rval = (int) (m_strength * -1);
	    m_strength = 0;
	    return rval;
	} else
	    return 0;
    }
    // Who knows what type of shield we are.  Let's just
    // say we have none.
    return iPts;
}

// Returns the type of shield.
HS_SHIELDTYPE CHSSysShield::GetShieldType(void)
{
    return m_shield_type;
}

void CHSSysShield::SaveToFile(FILE * fp)
{
    // Call base class next
    CHSEngSystem::SaveToFile(fp);

    // Ok to output, so we print out our information
    // specific to this system.
    fprintf(fp, "SHIELD TYPE=%d\n", (int) m_shield_type);
    fprintf(fp, "STRENGTH=%.2f\n", m_strength);

    if (m_regen_rate)
	fprintf(fp, "REGEN RATE=%.4f\n", *m_regen_rate);

    if (m_max_strength)
	fprintf(fp, "MAX STRENGTH=%.2f\n", *m_max_strength);
}

// Returns the regen rate for the shield system.  If bAdjusted
// is set to false, then the maximum regen rate is returned,
// otherwise, it is adjusted for damage and power levels.
// Rates are stored as points per second.
double CHSSysShield::GetRegenRate(BOOL bAdjusted)
{
    double dmgperc;
    CHSSysShield *ptr;
    double rate;
    int iOptPower;

    // Use some logic here.
    if (!m_regen_rate) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysShield *) m_parent;
	    rate = ptr->GetRegenRate(FALSE);
	}
    } else
	rate = *m_regen_rate;


    // Make overloading and damage adjustments?
    if (bAdjusted) {
	float fVal;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (float) m_current_power;
	fVal /= (float) iOptPower;
	rate *= fVal;

	// Figure in damage.  1/4 reduction per level of damage
	dmgperc = 1 - (.25 * GetDamageLevel());
	rate = (rate * dmgperc);
    }
    return rate;
}

// Returns the current shield percentage from 0 - 100.
double CHSSysShield::GetShieldPerc(void)
{
    double perc;

    // If we don't have at least 1 MW charge, shields are
    // down.
    if (m_current_power <= 0)
	return 0;

    perc = GetMaxStrength() + .00001;
    perc = m_strength / perc;
    perc *= 100;
    return (perc);
}

// Returns the maximum strength of the shield.  There is NO
// adjustment that can be made to this.  The shield regenerators
// ALWAYS recharge to full value, just maybe more slowly if damaged.
double CHSSysShield::GetMaxStrength(void)
{
    CHSSysShield *ptr;

    // Use some logic here.
    if (!m_max_strength) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysShield *) m_parent;
	    return (ptr->GetMaxStrength());
	}
    } else
	return (*m_max_strength);
}

// Prints out system information to a player
void CHSSysShield::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player,
	   unsafe_tprintf("REGEN RATE: %.2f", GetRegenRate(FALSE)));
    notify(player, unsafe_tprintf("STRENGTH: %.0f", m_strength));
    notify(player, unsafe_tprintf("MAX STRENGTH: %.0f", GetMaxStrength()));
    notify(player, unsafe_tprintf("SHIELD TYPE: %s (%d)",
				  m_shield_type ==
				  ST_DEFLECTOR ? "Deflector" :
				  "Absorption", m_shield_type));
}


// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysShield::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,REGEN RATE=%.2f,STRENGTH=%.2f,MAX STRENGTH=%.2f,SHIELD TYPE=%d",
	    CHSEngSystem::GetAttrValueString(),
	    GetRegenRate(FALSE),
	    m_strength, GetMaxStrength(), m_shield_type);

    return rval;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSSysShield::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;
    double dVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "REGEN RATE")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_regen_rate) {
		delete m_regen_rate;
		m_regen_rate = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	dVal = atof(strValue);
	if (dVal < 0)
	    return FALSE;

	if (!m_regen_rate)
	    m_regen_rate = new double;

	*m_regen_rate = dVal;
	return TRUE;
    } else if (!strcasecmp(strName, "STRENGTH")) {
	dVal = atof(strValue);
	if (dVal < 0)
	    return FALSE;

	m_strength = dVal;
	return TRUE;
    } else if (!strcasecmp(strName, "SHIELD TYPE")) {
	iVal = atoi(strValue);
	if (iVal != 0 && iVal != 1)
	    return FALSE;

	m_shield_type = (HS_SHIELDTYPE) iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "MAX STRENGTH")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_max_strength) {
		delete m_max_strength;
		m_max_strength = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	dVal = atoi(strValue);
	if (dVal < 0)
	    return FALSE;

	if (!m_max_strength)
	    m_max_strength = new double;

	*m_max_strength = dVal;
	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSSysShield::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "REGEN RATE"))
	sprintf(rval, "%.2f", GetRegenRate(bAdjusted));
    else if (!strcasecmp(strName, "STRENGTH"))
	sprintf(rval, "%.2f", m_strength);
    else if (!strcasecmp(strName, "SHIELD TYPE"))
	sprintf(rval, "%d", m_shield_type);
    else if (!strcasecmp(strName, "MAX STRENGTH"))
	sprintf(rval, "%.2f", GetMaxStrength());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// We override the SetCurrentPower() function so that we can
// do different types of shield things.
BOOL CHSSysShield::SetCurrentPower(int level)
{
    UINT max;

    max = GetOptimalPower();

    // Don't allow levels below 0.  Level 0 is just deactivation.
    if (level < 0)
	return FALSE;

    // Only allow overloading to 150%.
    if (level > (max * 1.5))
	return FALSE;

    m_current_power = level;

    // Now, depending on the type of shield we are, set
    // the strength.
    if (m_shield_type == ST_DEFLECTOR) {
	double dPerc;

	// Grab the absolute maximum power level
	dPerc = GetOptimalPower(FALSE);

	dPerc = m_current_power / dPerc;

	// Deflector shield strength is determined by
	// maximum possible strength multiplied by current
	// power settings.  If shield generators get damaged,
	// this doesn't allow as much power to be allocated,
	// and thus results in less shield strength.
	m_strength = GetMaxStrength() * dPerc;
    }
    // Don't do anything for absorption shields.

    return TRUE;
}

//
// CHSSysComm implementation
//
CHSSysComm::CHSSysComm(void)
{
    m_comm_range = NULL;
    m_type = HSS_COMM;
}

CHSSysComm::~CHSSysComm(void)
{
    if (m_comm_range)
	delete m_comm_range;
}

// Prints out system information to a player
void CHSSysComm::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player, unsafe_tprintf("MAX RANGE: %d", GetMaxRange(FALSE)));
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysComm::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,MAX RANGE=%d",
	    CHSEngSystem::GetAttrValueString(), GetMaxRange(FALSE));

    return rval;
}

// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSSysComm::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "MAX RANGE")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_comm_range) {
		delete m_comm_range;
		m_comm_range = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_comm_range)
	    m_comm_range = new UINT;

	*m_comm_range = iVal;

	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSSysComm::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "MAX RANGE"))
	sprintf(rval, "%d", GetMaxRange(bAdjusted));
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// A derived implementation of the Dup() in the base class.
CHSEngSystem *CHSSysComm::Dup(void)
{
    CHSSysComm *ptr;

    ptr = new CHSSysComm;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

// Returns the maximum transmission range for the system.  If bAdjusted
// is set to false, then the maximum raw turning rate is returned,
// otherwise, it is adjusted for damage and power levels.
UINT CHSSysComm::GetMaxRange(BOOL bAdjusted)
{
    double dmgperc;
    CHSSysComm *ptr;
    UINT uVal;
    int iOptPower;

    // Use some logic here.
    if (!m_comm_range) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysComm *) m_parent;
	    uVal = ptr->GetMaxRange(FALSE);
	}
    } else
	uVal = *m_comm_range;


    // Make overloading and damage adjustments?  No need
    // to figure in damage, cause that's figured into 
    // optimal power and overloading.  If they overload
    // a damaged system, it stresses, so we just need to
    // check current power over undamaged optimal power.
    if (bAdjusted) {
	float fVal;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (float) m_current_power;
	fVal /= (float) iOptPower;
	uVal = (UINT) (uVal * fVal);
    }
    return uVal;
}

// Outputs system information to a file.  This could be
// the class database or another database of objects that
// contain systems.
void CHSSysComm::SaveToFile(FILE * fp)
{
    // Call base class FIRST
    CHSEngSystem::SaveToFile(fp);

    // Now output our local values.
    if (m_comm_range)
	fprintf(fp, "MAX RANGE=%d\n", *m_comm_range);
}

CHSSysCloak::CHSSysCloak(void)
{
    m_type = HSS_CLOAK;
    m_engaged = 0;
    m_efficiency = NULL;
}

CHSSysCloak::~CHSSysCloak(void)
{
    if (m_efficiency)
	delete m_efficiency;
}

CHSEngSystem *CHSSysCloak::Dup(void)
{
    CHSSysCloak *ptr;

    ptr = new CHSSysCloak;

    return (CHSEngSystem *) ptr;
}

BOOL CHSSysCloak::SetAttributeValue(char *strName, char *strValue)
{
    // Match the name .. set the value
    if (!strcasecmp(strName, "EFFICIENCY")) {
	if (atof(strValue) < 0 || 100 < atof(strValue)) {
	    return FALSE;
	} else {
	    if (!*strValue) {
		delete m_efficiency;
		m_efficiency = NULL;
		return TRUE;
	    } else {
		if (!m_efficiency)
		    m_efficiency = new float;

		*m_efficiency = atof(strValue);
		return TRUE;
	    }
	}
    } else if (!strcasecmp(strName, "ENGAGED")) {
	m_engaged = atoi(strValue);
	return TRUE;
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}


float CHSSysCloak::GetEfficiency(BOOL bAdjusted)
{
    float rval;


    // Do we have a local value?
    if (!m_efficiency) {
	// Do we have a parent?
	if (m_parent) {
	    CHSSysCloak *ptr;
	    ptr = (CHSSysCloak *) m_parent;
	    rval = ptr->GetEfficiency(FALSE);
	} else
	    return 0;		// default of 1000 (1k)
    } else
	rval = *m_efficiency;

    if (bAdjusted) {
	float fVal;
	int iOptPower;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (float) m_current_power;
	if (iOptPower)
	    fVal /= (float) iOptPower;
	else
	    fVal = 1.00;
	rval *= fVal;
	if (rval > 100)
	    rval = 99.999999;
    }

    return rval;

}

void CHSSysCloak::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    fprintf(fp, "ENGAGED=%d\n", m_engaged);
    if (m_efficiency)
	fprintf(fp, "EFFICIENCY=%.4f\n", *m_efficiency);
}

char *CHSSysCloak::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "EFFICIENCY"))
	sprintf(rval, "%.4f", GetEfficiency(bAdjusted));
    else if (!strcasecmp(strName, "ENGAGED"))
	sprintf(rval, "%d", GetEngaged());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysCloak::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,ENGAGED=%d,EFFICIENCY=%.4f",
	    CHSEngSystem::GetAttrValueString(), GetEngaged(),
	    GetEfficiency(FALSE));

    return rval;
}

int CHSSysCloak::GetEngaged(void)
{
    return m_engaged;
}

void CHSSysCloak::SetEngaged(BOOL bEngage)
{
    if (bEngage) {
	m_engaging =
	    atoi(unsafe_tprintf("%.0f", GetOptimalPower() / 10.00));
	if (!IsEngaging())
	    m_engaged = 1;
    } else {
	m_engaged = 0;
    }
}

BOOL CHSSysCloak::IsEngaging(void)
{
    if (m_engaging > 0)
	return TRUE;
    else
	return FALSE;
}

void CHSSysCloak::DoCycle(void)
{
    // Do base cycle stuff
    CHSEngSystem::DoCycle();

    if (IsEngaging()) {
	m_engaging -= 1;
	if (!IsEngaging())
	    m_engaged = 1;
    } else {
	return;
    }
}

CHSSysTach::CHSSysTach(void)
{
    m_type = HSS_TACHYON;
    m_efficiency = NULL;
}

CHSSysTach::~CHSSysTach(void)
{
    if (m_efficiency)
	delete m_efficiency;
}

CHSEngSystem *CHSSysTach::Dup(void)
{
    CHSSysTach *ptr;

    ptr = new CHSSysTach;

    return (CHSEngSystem *) ptr;
}

BOOL CHSSysTach::SetAttributeValue(char *strName, char *strValue)
{
    // Match the name .. set the value
    if (!strcasecmp(strName, "EFFICIENCY")) {
	if (atof(strValue) < 0 || 100 < atof(strValue)) {
	    return FALSE;
	} else {
	    if (!*strValue) {
		delete m_efficiency;
		m_efficiency = NULL;
		return TRUE;
	    } else {
		if (!m_efficiency)
		    m_efficiency = new float;

		*m_efficiency = atof(strValue);
		return TRUE;
	    }
	}
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}



float CHSSysTach::GetEfficiency(BOOL bAdjusted)
{
    float rval;


    // Do we have a local value?
    if (!m_efficiency) {
	// Do we have a parent?
	if (m_parent) {
	    CHSSysTach *ptr;
	    ptr = (CHSSysTach *) m_parent;
	    rval = ptr->GetEfficiency(FALSE);
	} else
	    return 0;		// default of 1000 (1k)
    } else
	rval = *m_efficiency;

    if (bAdjusted) {
	float fVal;
	int iOptPower;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (float) m_current_power;
	if (iOptPower)
	    fVal /= (float) iOptPower;
	else
	    fVal = 1;
	rval *= fVal;
	if (rval > 100)
	    rval = 99.999999;
    }



    return rval;

}

void CHSSysTach::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    if (m_efficiency)
	fprintf(fp, "EFFICIENCY=%.4f\n", *m_efficiency);
}

char *CHSSysTach::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "EFFICIENCY"))
	sprintf(rval, "%.4f", GetEfficiency(bAdjusted));
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysTach::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,EFFICIENCY=%.4f",
	    CHSEngSystem::GetAttrValueString(), GetEfficiency(FALSE));

    return rval;
}

CHSFictional::CHSFictional(void)
{
    m_name = "Fictional";
    m_type = HSS_FICTIONAL;
}

CHSEngSystem *CHSFictional::Dup(void)
{
    CHSFictional *ptr;

    ptr = new CHSFictional;

    return (CHSEngSystem *) ptr;
}

CHSDamCon::CHSDamCon(void)
{
    m_type = HSS_DAMCON;
    m_ncrews = NULL;
    int idx;

    for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++)
	m_crew_working[idx] = NULL;
}

CHSEngSystem *CHSDamCon::Dup(void)
{
    CHSDamCon *ptr;

    ptr = new CHSDamCon;

    return (CHSEngSystem *) ptr;
}

char *CHSDamCon::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,NUMCREWS=%i",
	    CHSEngSystem::GetAttrValueString(), GetNumCrews());

    return rval;
}

char *CHSDamCon::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "NUMCREWS"))
	sprintf(rval, "%i", GetNumCrews());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

void CHSDamCon::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    if (m_ncrews)
	fprintf(fp, "NUMCREWS=%i\n", *m_ncrews);
}

int CHSDamCon::GetNumCrews(void)
{
    int rval;


    // Do we have a local value?
    if (!m_ncrews) {
	// Do we have a parent?
	if (!m_parent)
	    return 0;
	else {
	    CHSDamCon *ptr;
	    ptr = (CHSDamCon *) m_parent;
	    rval = ptr->GetNumCrews();
	}
    } else
	rval = *m_ncrews;

    return rval;

}

CHSDamCon::~CHSDamCon(void)
{
    if (m_ncrews)
	delete m_ncrews;
}

BOOL CHSDamCon::SetAttributeValue(char *strName, char *strValue)
{
    // Match the name .. set the value
    if (!strcasecmp(strName, "NUMCREWS")) {
	if (atoi(strValue) < 0) {
	    return FALSE;
	} else {
	    if (!*strValue) {
		if (m_ncrews) {
		    delete m_ncrews;
		    m_ncrews = NULL;
		}
		return TRUE;
	    } else {
		if (!m_ncrews)
		    m_ncrews = new int;

		*m_ncrews = atoi(strValue);
		return TRUE;
	    }
	}
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

CHSEngSystem *CHSDamCon::GetWorkingCrew(int crew)
{
    if (m_crew_working[crew])
	return m_crew_working[crew];
    else
	return NULL;
}

int CHSDamCon::GetCyclesLeft(int crew)
{
    return m_crew_cyclesleft[crew];
}

void CHSDamCon::DoCycle(void)
{
    // First we do the basic cycle, stress and such.
    CHSEngSystem::DoCycle();

    // Now we'll just go through our damage crews.
    int pinf;
    int rnd;

    if (GetCurrentPower() != 0 && GetOptimalPower(FALSE) != 0)
	pinf =
	    100 * ((GetCurrentPower() * 1.00) /
		   (GetOptimalPower(FALSE) * 1.00));
    else
	pinf = 0;

    rnd = getrandom(99) + 1;

    if (rnd > pinf)
	return;

    int idx;

    for (idx = 1; idx <= GetNumCrews(); idx++) {
	CHSEngSystem *tSys;

	tSys = GetWorkingCrew(idx);

	if (!tSys)
	    continue;

	m_crew_cyclesleft[idx]--;

	if (GetCyclesLeft(idx) <= 0) {
	    tSys->ReduceDamage();
	    int idx2;


	    if (tSys->GetDamageLevel() == DMG_NONE) {
		m_crew_working[idx] = NULL;
		m_crew_cyclesleft[idx] = 0;
		for (idx2 = 1; idx2 <= HS_MAX_DAMCREWS; idx2++) {
		    if (m_crew_working[idx2] == tSys) {
			m_crew_cyclesleft[idx2] = 0;
			m_crew_working[idx2] = NULL;
		    }
		}

		if (m_ownerObj->GetType() == HST_SHIP) {
		    CHSShip *tShip;

		    tShip = (CHSShip *) m_ownerObj;
		    tShip->
			NotifyConsoles(unsafe_tprintf
				       ("%s%s-%s %s repairs complete.",
					ANSI_HILITE, ANSI_GREEN,
					ANSI_NORMAL, tSys->GetName()),
				       MSG_ENGINEERING);
		}
	    } else {
		int cycleswork = 500;
		int numworking = 0;

		for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
		    if (m_crew_working[idx] == tSys)
			numworking++;
		}

		if (numworking > 1)
		    cycleswork /= numworking;

		for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
		    if (m_crew_working[idx] == tSys)
			m_crew_cyclesleft[idx] = cycleswork;
		}


		m_crew_cyclesleft[idx] = cycleswork;
		if (m_ownerObj->GetType() == HST_SHIP) {
		    CHSShip *tShip;

		    tShip = (CHSShip *) m_ownerObj;
		    tShip->
			NotifyConsoles(unsafe_tprintf
				       ("%s%s-%s %s repaired continueing to repair to full.",
					ANSI_HILITE, ANSI_GREEN,
					ANSI_NORMAL, tSys->GetName()),
				       MSG_ENGINEERING);
		}
	    }

	}
    }
}

void CHSDamCon::AssignCrew(dbref player, int iCrew, CHSEngSystem * tSys)
{
    int idx;
    int numworking = 0;

    if (tSys) {
	int cycleswork = 500;
	numworking = 0;
	m_crew_working[iCrew] = tSys;

	for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
	    if (m_crew_working[idx] == tSys) {
		numworking++;
		if (m_crew_cyclesleft[idx])
		    cycleswork = m_crew_cyclesleft[idx];
	    }
	}

	if (numworking > 1) {
	    cycleswork *= numworking - 1;
	    cycleswork /= numworking;
	}

	for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
	    if (m_crew_working[idx] == tSys)
		m_crew_cyclesleft[idx] = cycleswork;
	}


	m_crew_cyclesleft[iCrew] = cycleswork;
	if (m_ownerObj->GetType() == HST_SHIP) {
	    CHSShip *tShip;

	    tShip = (CHSShip *) m_ownerObj;
	    tShip->
		NotifyConsoles(unsafe_tprintf
			       ("%s%s-%s Damage Control crew %i assigned to %s.",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				iCrew, tSys->GetName()), MSG_ENGINEERING);
	}
    } else {
	CHSEngSystem *cSys;

	if (m_crew_working[iCrew])
	    cSys = m_crew_working[iCrew];
	else
	    cSys = NULL;

	m_crew_working[iCrew] = NULL;
	m_crew_cyclesleft[iCrew] = 0;

	if (cSys) {
	    for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
		if (m_crew_working[idx] == cSys)
		    numworking++;
	    }
	}

	if (cSys) {
	    for (idx = 1; idx <= HS_MAX_DAMCREWS; idx++) {
		if (m_crew_working[idx] == cSys) {
		    m_crew_cyclesleft[idx] *= (numworking + 1);
		    m_crew_cyclesleft[idx] /= numworking;
		}
	    }
	}

	if (m_ownerObj->GetType() == HST_SHIP) {
	    CHSShip *tShip;

	    tShip = (CHSShip *) m_ownerObj;
	    tShip->
		NotifyConsoles(unsafe_tprintf
			       ("%s%s-%s Damage Control crew %i now idle.",
				ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				iCrew), MSG_ENGINEERING);
	}
    }
}

// Prints out system information to a player
void CHSDamCon::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    notify(player, unsafe_tprintf("Crews    : %d", GetNumCrews()));
}

CHSSysJammer::CHSSysJammer(void)
{
    m_type = HSS_JAMMER;
    m_range = 0;
}

CHSSysJammer::~CHSSysJammer(void)
{
    if (m_range)
	delete m_range;
}

CHSEngSystem *CHSSysJammer::Dup(void)
{
    CHSSysJammer *ptr;

    ptr = new CHSSysJammer;

    return (CHSEngSystem *) ptr;
}

BOOL CHSSysJammer::SetAttributeValue(char *strName, char *strValue)
{
    // Match the name .. set the value
    if (!strcasecmp(strName, "RANGE")) {
	if (!*strValue) {
	    delete m_range;
	    m_range = NULL;
	    return TRUE;
	} else {
	    if (!m_range)
		m_range = new double;

	    *m_range = atoi(strValue);
	    return TRUE;
	}

    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}



double CHSSysJammer::GetRange(BOOL bAdjusted)
{
    double rval;


    // Do we have a local value?
    if (!m_range) {
	// Do we have a parent?
	if (m_parent) {
	    CHSSysJammer *ptr;
	    ptr = (CHSSysJammer *) m_parent;
	    rval = ptr->GetRange(FALSE);
	} else
	    return 0;		// default of 1000 (1k)
    } else
	rval = *m_range;

    if (bAdjusted) {
	double fVal;
	int iOptPower;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (double) m_current_power;
	if (iOptPower)
	    fVal /= (double) iOptPower;
	else
	    fVal = 1;
	rval *= fVal;
    }



    return rval;

}

void CHSSysJammer::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    if (m_range)
	fprintf(fp, "RANGE=%.4f\n", *m_range);
}

char *CHSSysJammer::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "RANGE"))
	sprintf(rval, "%d", GetRange(bAdjusted));
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysJammer::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,RANGE=%d",
	    CHSEngSystem::GetAttrValueString(), GetRange(FALSE));

    return rval;
}

CHSSysTractor::CHSSysTractor(void)
{
    m_type = HSS_TRACTOR;
    m_strength = 0;
    m_lock = NULL;
    m_mode = HSTM_HOLD;
}

CHSSysTractor::~CHSSysTractor(void)
{
    if (m_strength)
	delete m_strength;
}

CHSEngSystem *CHSSysTractor::Dup(void)
{
    CHSSysTractor *ptr;

    ptr = new CHSSysTractor;

    return (CHSEngSystem *) ptr;
}

BOOL CHSSysTractor::SetAttributeValue(char *strName, char *strValue)
{
    // Match the name .. set the value
    if (!strcasecmp(strName, "STRENGTH")) {
	if (!*strValue) {
	    delete m_strength;
	    m_strength = NULL;
	    return TRUE;
	} else {
	    if (!m_strength)
		m_strength = new double;

	    *m_strength = atoi(strValue);
	    return TRUE;
	}
    } else if (!strcasecmp(strName, "MODE")) {
	if (!*strValue) {
	    return FALSE;
	} else {
	    if (atoi(strValue) > 2 || atoi(strValue) < 0)
		return FALSE;

	    m_mode = (HS_TMODE) atoi(strValue);
	    return TRUE;
	}
    }
    // Attr name not matched, so pass it to the base class
    return CHSEngSystem::SetAttributeValue(strName, strValue);
}



double CHSSysTractor::GetStrength(BOOL bAdjusted)
{
    double rval;


    // Do we have a local value?
    if (!m_strength) {
	// Do we have a parent?
	if (m_parent) {
	    CHSSysTractor *ptr;
	    ptr = (CHSSysTractor *) m_parent;
	    rval = ptr->GetStrength(FALSE);
	} else
	    return 0;		// default of 1000 (1k)
    } else
	rval = *m_strength;

    if (bAdjusted) {
	double fVal;
	int iOptPower;
	iOptPower = GetOptimalPower(FALSE);
	fVal = (double) m_current_power;
	if (iOptPower)
	    fVal /= (double) iOptPower;
	else
	    fVal = 1;
	rval *= fVal;
    }



    return rval;

}

void CHSSysTractor::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    fprintf(fp, "MODE=%d\n", m_mode);
    if (m_strength)
	fprintf(fp, "STRENGTH=%.4f\n", *m_strength);
}

char *CHSSysTractor::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "STRENGTH"))
	sprintf(rval, "%.4f", GetStrength(bAdjusted));
    else if (!strcasecmp(strName, "MODE"))
	sprintf(rval, "%d", m_mode);
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Returns a comma-delimited list of system attributes and
// values.
char *CHSSysTractor::GetAttrValueString(void)
{
    static char rval[512];

    // Base information plus our's.
    sprintf(rval,
	    "%s,MODE=%d,STRENGTH=%.4f",
	    CHSEngSystem::GetAttrValueString(), m_mode,
	    GetStrength(FALSE));

    return rval;
}

void CHSSysTractor::DoCycle(void)
{
    CHSEngSystem::DoCycle();

    CHSShip *cShip;
    CHSObject *tObj;
    tObj = m_lock;
    if (!tObj)
	return;
    cShip = (CHSShip *) m_ownerObj;
    if (!cShip)
	return;

    double cDist;

    cDist =
	Dist3D(tObj->GetX(), tObj->GetY(), tObj->GetZ(), cShip->GetX(),
	       cShip->GetY(), cShip->GetZ());

    if (cDist > (GetEffect() * 10)) {
	m_lock = NULL;
	cShip->
	    NotifyConsoles(unsafe_tprintf
			   ("%s%s-%s Tractor lock target out of range, releasing lock.",
			    ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL),
			   MSG_GENERAL);
	tObj->HandleMessage("Tractor lock has been released.", MSG_GENERAL,
			    (long *) cShip);
	return;
    }

    if (!cShip->IsActive() || cShip->IsDestroyed() || !tObj->IsActive() ||
	cShip->GetDockedLocation() != NOTHING) {
	m_lock = NULL;
	cShip->NotifyConsoles(unsafe_tprintf("%s%s-%s Tractor lock lost.",
					     ANSI_GREEN, ANSI_HILITE,
					     ANSI_NORMAL), MSG_GENERAL);
	tObj->HandleMessage("Tractor lock has been released.", MSG_GENERAL,
			    (long *) cShip);
	return;
    }


    if (m_mode == HSTM_TRACTOR) {
	if (cDist > GetEffect()) {
	    tObj->
		MoveInDirection(XYAngle
				(tObj->GetX(), tObj->GetY(), cShip->GetX(),
				 cShip->GetY()), ZAngle(tObj->GetX(),
							tObj->GetY(),
							tObj->GetZ(),
							cShip->GetX(),
							cShip->GetY(),
							cShip->GetZ()),
				GetEffect());
	} else {
	    tObj->SetX(cShip->GetX());
	    tObj->SetY(cShip->GetY());
	    tObj->SetZ(cShip->GetZ());
	}
    } else if (m_mode == HSTM_REPULSE) {
	tObj->
	    MoveInDirection(XYAngle
			    (cShip->GetX(), cShip->GetY(), tObj->GetX(),
			     tObj->GetY()), ZAngle(cShip->GetX(),
						   cShip->GetY(),
						   cShip->GetZ(),
						   tObj->GetX(),
						   tObj->GetY(),
						   tObj->GetZ()),
			    GetEffect());
    } else if (m_mode == HSTM_HOLD) {
	if (cShip->GetSpeed() == 0)
	    return;

	float mvAmt;
	if ((cShip->GetSpeed() / 3600.00) < GetEffect())
	    mvAmt = cShip->GetSpeed() / 3600.00;
	else
	    mvAmt = GetEffect();

	tObj->MoveInDirection(cShip->GetXYHeading(), cShip->GetZHeading(),
			      mvAmt);
    }
    cDist =
	Dist3D(tObj->GetX(), tObj->GetY(), tObj->GetZ(), cShip->GetX(),
	       cShip->GetY(), cShip->GetZ());

    if (cDist > (GetEffect() * 10)) {
	m_lock = NULL;
	cShip->
	    NotifyConsoles(unsafe_tprintf
			   ("%s%s-%s Tractor lock target out of range, releasing lock.",
			    ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL),
			   MSG_GENERAL);
	tObj->HandleMessage("Tractor lock has been released.", MSG_GENERAL,
			    (long *) cShip);
	return;
    }

}

CHSObject *CHSSysTractor::GetLock(void)
{
    return m_lock;
}

float CHSSysTractor::GetEffect(void)
{
    float rval;

    if (!GetLock())
	return 0.00;

    CHSObject *tObj;

    tObj = GetLock();

    rval = GetStrength(TRUE) / tObj->GetSize();

    return rval;
}

HS_TMODE CHSSysTractor::GetMode(void)
{
    return m_mode;
}

void CHSSysTractor::SetMode(int mode)
{
    CHSShip *cShip;
    cShip = (CHSShip *) m_ownerObj;

    if (cShip) {
	char tbuf[128];
	if (mode == 0)
	    sprintf(tbuf, "tractor");
	else if (mode == 1)
	    sprintf(tbuf, "repulse");
	else if (mode == 2)
	    sprintf(tbuf, "hold");
	cShip->
	    NotifyConsoles(unsafe_tprintf
			   ("%s%s-%s Tractor beam mode set to %s.",
			    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, tbuf),
			   MSG_GENERAL);
    }

    m_mode = (HS_TMODE) mode;
}

void CHSSysTractor::SetLock(dbref player, int id)
{
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;

    CHSShip *cShip;
    cShip = (CHSShip *) m_ownerObj;

    if (!cShip)
	return;

    if (id == 0) {
	if (m_lock) {
	    cShip->
		NotifyConsoles(unsafe_tprintf
			       ("%s%s-%s Tractor beam unlocked.",
				ANSI_GREEN, ANSI_HILITE, ANSI_NORMAL),
			       MSG_GENERAL);
	    m_lock = NULL;
	    return;
	} else {
	    hsStdError(player, "Tractor beam is not locked.");
	    return;
	}
    }
    // Do we have sensors?
    cSensors = (CHSSysSensors *) cShip->m_systems.GetSystem(HSS_SENSORS);
    if (!cSensors) {
	hsStdError(player, "This vessel is not equipped with sensors.");
	return;
    }
    // Are sensors online?
    if (!cSensors->GetCurrentPower()) {
	hsStdError(player, "Sensors are currently offline.");
	return;
    }
    // Find the contact.
    cContact = cSensors->GetContactByID(id);
    if (!cContact) {
	hsStdError(player, "No such contact ID on sensors.");
	return;
    }

    CHSObject *cObj;

    cObj = cContact->m_obj;

    if (Dist3D
	(cShip->GetX(), cShip->GetY(), cShip->GetZ(), cObj->GetX(),
	 cObj->GetY(),
	 cObj->GetZ()) > GetStrength(TRUE) / cObj->GetSize() * 10.00) {
	hsStdError(player, "Target outside of tractor beam range.");
	return;
    }

    if (cObj->GetType() != HST_SHIP) {
	hsStdError(player, "You cannot tractor that type of object.");
	return;
    }

    m_lock = cObj;

    cShip->HandleMessage("Tractor beam locked onto target.", MSG_GENERAL,
			 (long *) cObj);
    cObj->
	HandleMessage(unsafe_tprintf
		      ("Tractor beam lock detected from the %s.",
		       cShip->GetName()), MSG_GENERAL, (long *) cShip);
    if (cObj->GetType() == HST_SHIP) {
	CHSShip *tShip;
	tShip = (CHSShip *) cObj;
	tShip->
	    NotifySrooms
	    ("The ship shakes lightly as something locks onto the hull.");
    }
}

void CHSSysTractor::DockShip(dbref player, int id, int loc)
{
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;

    CHSShip *cShip;
    cShip = (CHSShip *) m_ownerObj;

    if (!cShip)
	return;

    // Do we have sensors?
    cSensors = (CHSSysSensors *) cShip->m_systems.GetSystem(HSS_SENSORS);
    if (!cSensors) {
	hsStdError(player, "This vessel is not equipped with sensors.");
	return;
    }
    // Are sensors online?
    if (!cSensors->GetCurrentPower()) {
	hsStdError(player, "Sensors are currently offline.");
	return;
    }
    // Find the contact.
    cContact = cSensors->GetContactByID(id);
    if (!cContact) {
	hsStdError(player, "No such contact ID on sensors.");
	return;
    }

    CHSObject *cObj;

    cObj = cContact->m_obj;

    if (cObj->GetType() != HST_SHIP) {
	hsStdError(player, "Cannot tractor dock that kind of object.");
	return;
    }

    if (Dist3D
	(cShip->GetX(), cShip->GetY(), cShip->GetZ(), cObj->GetX(),
	 cObj->GetY(), cObj->GetZ()) > 2) {
	hsStdError(player,
		   "Target outside of tractor docking range(2 hm).");
	return;
    }

    CHSShip *tShip;
    tShip = (CHSShip *) cObj;

    // Are they too big?
    if (tShip->GetSize() > HSCONF.max_dock_size && !cShip->IsSpacedock()) {
	hsStdError(player,
		   "That vessel is too large to dock in another vessel.");
	return;
    }
    // Are we too small?
    if (tShip->GetSize() >= cShip->GetSize()) {
	hsStdError(player, "We are too small to accomodate that vessel.");
	return;
    }

    CHSSysEngines *cEngines;
    cEngines = (CHSSysEngines *) tShip->m_systems.GetSystem(HSS_ENGINES);

    // Are they at full stop?
    if (cEngines && cEngines->GetCurrentSpeed()) {
	hsStdError(player,
		   "Target must be at a full stop to commence docking.");
	return;
    }

    CHSLandingLoc *cLocation;

    // Find the landing location
    // The landing locations are specified by the player as
    // 1 .. n, but we store them in array notation 0..n-1.
    // Thus, subtract 1 from location when retrieving.
    cLocation = cShip->GetLandingLoc(loc - 1);

    // Location exists?
    if (!cLocation) {
	hsStdError(player, "That landing location does not exist.");
	return;
    }
    // See if the landing site is active.
    if (!cLocation->IsActive()) {
	hsStdError(player, "The bay doors to that bay are closed.");
	return;
    }
    // See if the landing site can accomodate us.
    if (!cLocation->CanAccomodate(tShip)) {
	hsStdError(player, "Landing site cannot accomodate this vessel.");
	return;
    }
    // Set desired and current speed to 0 to stop the ship.
    cEngines->SetDesiredSpeed(0);
    cEngines->SetAttributeValue("CURRENT SPEED", "0");

    char tbuf[256];

    sprintf(tbuf,
	    "Through the bay doors, the %s is tractored in and docks.",
	    tShip->GetName());
    cLocation->HandleMessage(tbuf, MSG_GENERAL);
    sprintf(tbuf,
	    "The %s pushes forward as it is tractored in and docks.",
	    tShip->GetName());
    tShip->NotifyConsoles(tbuf, MSG_GENERAL);
    tShip->m_docked = TRUE;
    tShip->MoveShipObject(cLocation->objnum);
    cShip->
	HandleMessage(unsafe_tprintf
		      ("The %s is being tractored into docking bay %d.",
		       tShip->GetName(), loc), MSG_SENSOR, (long *) tShip);

    // Set our location
    CHSObject *tOwner;
    tOwner = cLocation->GetOwnerObject();
    if (tOwner)
	tShip->m_objlocation = tOwner->GetDbref();
    else
	tShip->m_objlocation = NOTHING;

    // Remove us from active space
    CHSUniverse *uSource;
    uSource = uaUniverses.FindUniverse(tShip->GetUID());
    if (uSource)
	uSource->RemoveActiveObject(tShip);

    // Deduct the capacity from the landing loc
    cLocation->DeductCapacity(tShip->GetSize());
}
