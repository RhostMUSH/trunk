#include "hscopyright.h"
#include "hsengines.h"
#include "hsobjects.h"
#include "hsinterface.h"
#include "hspace.h"
#include "ansi.h"
#include "hsconf.h"
#include <stdlib.h>

#ifndef WIN32
#include <strings.h>
#endif

CHSSysEngines::CHSSysEngines(void)
{
    m_type = HSS_ENGINES;

    m_max_velocity = NULL;
    m_acceleration = NULL;
    m_efficiency = NULL;

    m_current_speed = 0;
    m_desired_speed = 0;
    m_fuel_source = NULL;
    m_can_afterburn = NULL;
    m_afterburning = FALSE;
}

// Destructor
CHSSysEngines::~CHSSysEngines(void)
{
    // Free up our variables
    if (m_max_velocity)
	delete m_max_velocity;
    if (m_acceleration)
	delete m_acceleration;
    if (m_efficiency)
	delete m_efficiency;
    if (m_can_afterburn)
	delete m_can_afterburn;
}

void CHSSysEngines::PowerUp(int level)
{
    if (m_ownerObj->GetType() == HST_SHIP) {
	CHSShip *cShip;

	cShip = (CHSShip *) m_ownerObj;

	cShip->NotifySrooms(HSCONF.engines_activating);
    }
}

void CHSSysEngines::CutPower(int level)
{
    if (level == 0) {
	if (m_ownerObj->GetType() == HST_SHIP) {
	    CHSShip *cShip;

	    cShip = (CHSShip *) m_ownerObj;

	    cShip->NotifySrooms(HSCONF.engines_cut);
	}
    }
}



// Sets a specific attribute value for the system.  This
// also allows system default values to be overridden at the
// ship level.
BOOL CHSSysEngines::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Match the name .. set the value
    if (!strcasecmp(strName, "MAX VELOCITY")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_max_velocity) {
		delete m_max_velocity;
		m_max_velocity = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_max_velocity)
	    m_max_velocity = new int;

	*m_max_velocity = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "ACCELERATION")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_acceleration) {
		delete m_acceleration;
		m_acceleration = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (iVal < 0)
	    return FALSE;

	if (!m_acceleration)
	    m_acceleration = new int;

	*m_acceleration = iVal;
	return TRUE;
    } else if (!strcasecmp(strName, "DESIRED SPEED")) {
	m_desired_speed = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "CURRENT SPEED")) {
	m_current_speed = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "CAN AFTERBURN")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_can_afterburn) {
		delete m_can_afterburn;
		m_can_afterburn = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	if (!m_can_afterburn)
	    m_can_afterburn = new BOOL;

	*m_can_afterburn = atoi(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "AFTERBURNING")) {
	m_afterburning = atoi(strValue);
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
char *CHSSysEngines::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "MAX VELOCITY"))
	sprintf(rval, "%d", GetMaxVelocity(bAdjusted));
    else if (!strcasecmp(strName, "ACCELERATION"))
	sprintf(rval, "%d", GetAcceleration(bAdjusted));
    else if (!strcasecmp(strName, "DESIRED SPEED"))
	sprintf(rval, "%.2f", m_desired_speed);
    else if (!strcasecmp(strName, "CURRENT SPEED"))
	sprintf(rval, "%.2f", m_current_speed);
    else if (!strcasecmp(strName, "CAN AFTERBURN"))
	sprintf(rval, "%d", CanBurn());
    else if (!strcasecmp(strName, "AFTERBURNING"))
	sprintf(rval, "%d", GetAfterburning());
    else if (!strcasecmp(strName, "EFFICIENCY"))
	sprintf(rval, "%d", GetEfficiency());
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Prints out system information to the specified player
void CHSSysEngines::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    // Now our info
    notify(player,
	   unsafe_tprintf("MAX VELOCITY: %d", GetMaxVelocity(FALSE)));
    notify(player,
	   unsafe_tprintf("ACCELERATION: %d", GetAcceleration(FALSE)));
    notify(player, unsafe_tprintf("DESIRED SPEED: %.0f", m_desired_speed));
    notify(player, unsafe_tprintf("CURRENT SPEED: %.0f", m_current_speed));
    notify(player,
	   unsafe_tprintf("CAN AFTERBURN: %s", CanBurn()? "YES" : "NO"));
    notify(player,
	   unsafe_tprintf("AFTERBURNING : %s",
			  GetAfterburning()? "YES" : "NO"));
    notify(player, unsafe_tprintf("EFFICIENCY   : %d", GetEfficiency()));

}

// Returns the current speed (thrust) of the engines
double CHSSysEngines::GetCurrentSpeed(void)
{
    return m_current_speed;
}

// Indicates whether the engines are afterburning or not
BOOL CHSSysEngines::GetAfterburning(void)
{
    return m_afterburning;
}

// Indicates whether the engines are capable of afterburning
// or not.
BOOL CHSSysEngines::CanBurn(void)
{
    // Do we have a local value?
    if (!m_can_afterburn) {
	// No local value.  Do we have a parent?
	if (!m_parent)
	    return FALSE;

	// Get the parent's value.
	CHSSysEngines *ptr;
	ptr = (CHSSysEngines *) m_parent;
	return ptr->CanBurn();
    } else
	return *m_can_afterburn;
}

// Sets the burn/no-burn status of the engine afterburners.
BOOL CHSSysEngines::SetAfterburn(BOOL bStat)
{
    UINT uMax;

    if (!CanBurn())
	return FALSE;

    uMax = GetMaxVelocity();
    if (bStat) {
	m_desired_speed = uMax * HSCONF.afterburn_ratio;
	m_afterburning = TRUE;
    } else {
	m_afterburning = FALSE;
	m_desired_speed = uMax;
    }
    return TRUE;
}

// Sets the desired speed of the engines
BOOL CHSSysEngines::SetDesiredSpeed(int speed)
{
    int iMaxVelocity;

    iMaxVelocity = GetMaxVelocity();
    if ((speed > iMaxVelocity) || (speed < (-1 * (iMaxVelocity / 2.0)))) {
	return FALSE;
    }
    // If desired speed is 0, turn off afterburners
    if (speed == 0)
	SetAfterburn(FALSE);

    m_desired_speed = (double) speed;
    return TRUE;
}

// Returns the current, desired speed of the engines
double CHSSysEngines::GetDesiredSpeed(void)
{
    return m_desired_speed;
}

// Handles cyclic stuff for engines
void CHSSysEngines::DoCycle(void)
{
    int iFuel;

    // Do base class stuff first
    CHSEngSystem::DoCycle();

    // Do anything?
    if (!m_current_speed && !m_current_power)
	return;

    // Change our speed if needed
    ChangeSpeed();

    // Consume fuel.  Yummy.
    if (m_current_speed) {
	iFuel = m_current_speed;
	if (GetAfterburning())
	    iFuel *= HSCONF.afterburn_fuel_ratio;
	ConsumeFuelBySpeed((int) m_current_speed);
    }
}

// Handles slowing or speeding up the engines
void CHSSysEngines::ChangeSpeed(void)
{
    UINT uMax;
    UINT uAccel;

    // Grab maximum velocity
    uMax = GetMaxVelocity();

    // If afterburning, max is increased by some
    // ratio.
    if (m_afterburning)
	uMax *= HSCONF.afterburn_ratio;

    // Make sure the desired speed doesn't currently
    // exceed the maximum speed limits.
    if (m_desired_speed > uMax) {
	m_desired_speed = uMax;
    } else if (m_desired_speed < (uMax * -.5)) {
	m_desired_speed = uMax * -.5;
    }

    // This has to come after the checking of uMax.  Otherwise,
    // the ship could be at top speed, power is taken away from
    // engines, but the speed doesn't change because the desired
    // and current speed are already equal.
    if (m_desired_speed == m_current_speed)
	return;


    // Change the speed closer to the desired speed
    if (m_current_speed > m_desired_speed) {
	// Grab deceleration rate, which is always the
	// same as maximum acceleration.  Deceleration is
	// not affected by engine damage or power.
	uAccel = GetAcceleration(FALSE);

	m_current_speed -= uAccel;

	// Did we go too far?
	if (m_current_speed < m_desired_speed)
	    m_current_speed = m_desired_speed;
    } else if (m_current_speed < m_desired_speed) {
	// Grab the acceleration rate, which is affected
	// by power and damage.
	uAccel = GetAcceleration(TRUE);

	m_current_speed += uAccel;

	// Did we go too far?
	if (m_current_speed > m_desired_speed)
	    m_current_speed = m_desired_speed;
    }
}

// Tells the engines that they need to gobble up some fuel
// from the fuel source.  The engines know their efficiency
// and such, so they just drain the fuel from the fuel system.
// Speed here should be given in units per hour.
void CHSSysEngines::ConsumeFuelBySpeed(int speed)
{
    float fConsume;
    float fRealized;

    // Do we have a fuel system?
    if (!m_fuel_source || (m_fuel_source->GetFuelRemaining() <= 0))
	return;

    // Calculate per second travel rate
    fConsume = (float) (speed * .0002778);

    // Divide by efficiency
    fConsume = (float) (fConsume / (1000.0 * GetEfficiency()));

    fRealized = m_fuel_source->ExtractFuelUnit(fConsume);
    if (fRealized < fConsume) {
	// Out of gas.  Set desired speed to 0.
	m_desired_speed = 0;
	char tbuf[128];
	sprintf(tbuf,
		"%s%s-%s A warning light flashing, indicating engines have run out of fuel.",
		ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL);
	if (m_ownerObj)
	    m_ownerObj->HandleMessage(tbuf, MSG_ENGINEERING, NULL);
    }
}

// Sets the fuel system source for the engines.  If this
// is NULL, engines don't consume fuel .. duh.
void CHSSysEngines::SetFuelSource(CHSFuelSystem * cFuel)
{
    m_fuel_source = cFuel;
}

// Returns the efficiency of the engines, which is a 
// number specifying thousands of units of distance travel
// per unit of fuel consumption .. just like gas mileage on
// a car.  For example 1000km/gallon.  This would be returned
// here as just 1.
int CHSSysEngines::GetEfficiency(void)
{
    // Do we have a local value?
    if (!m_efficiency) {
	// Do we have a parent?
	if (m_parent) {
	    CHSSysEngines *ptr;
	    ptr = (CHSSysEngines *) m_parent;
	    return ptr->GetEfficiency();
	} else
	    return 1;		// default of 1000 (1k)
    } else
	return *m_efficiency;
}

// A derived implementation of the Dup() in the base class.
CHSEngSystem *CHSSysEngines::Dup(void)
{
    CHSSysEngines *ptr;

    ptr = new CHSSysEngines;

    // No initialization to do.
    return (CHSEngSystem *) ptr;
}

// Returns maximum velocity for the engines.  Since this
// variable is overridable at the ship level, we need to
// do some logic to determine whether to return a default
// value or not.
UINT CHSSysEngines::GetMaxVelocity(BOOL bAdjusted)
{
    double dmgperc;
    CHSSysEngines *ptr;
    UINT uVal;
    int iOptPower;

    // Use some logic here.
    if (!m_max_velocity) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysEngines *) m_parent;
	    uVal = ptr->GetMaxVelocity(FALSE);
	}
    } else
	uVal = *m_max_velocity;


    // Make overloading and damage adjustments?
    if (bAdjusted) {
	// At this point, uVal is the maximum velocity assuming
	// 100% power level.  We have to adjust for power level
	// now.  The speed inrcreases proportional to overload
	// percentage.
	float fVal;
	iOptPower = GetOptimalPower(TRUE);
	fVal = (float) m_current_power;
	fVal /= (float) iOptPower;
	uVal = (UINT) (fVal * uVal);

	// Figure in damage.  1/4 reduction per level of damage
	dmgperc = 1 - (.25 * GetDamageLevel());
	uVal = (UINT) (uVal * dmgperc);

	// Check for fuel source and 0 fuel condition
	if (m_fuel_source) {
	    if (m_fuel_source->GetFuelRemaining() <= 0)
		return 0;
	}
    }
    return uVal;
}

// Returns the acceleration rate for the engines.  If the
// bAdjusted variable is set to FALSE, no damage or power level
// adjustments are made.
UINT CHSSysEngines::GetAcceleration(BOOL bAdjusted)
{
    double dmgperc;
    CHSSysEngines *ptr;
    UINT uVal;
    int iOptPower;

    // Use some logic here.
    if (!m_acceleration) {
	// Go to the parent's setting?
	if (!m_parent) {
	    // No.  We are the default values.
	    return 0;
	} else {
	    // Yes, this system exists on a ship, so
	    // find the default values on the parent.
	    ptr = (CHSSysEngines *) m_parent;
	    uVal = ptr->GetAcceleration(FALSE);
	}
    } else
	uVal = *m_acceleration;

    // Make overloading and damage adjustments?
    if (bAdjusted) {
	// If afterburning, acceleration is increased
	// like speed.
	if (GetAfterburning()) {
	    uVal *= HSCONF.afterburn_ratio;
	}

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
void CHSSysEngines::SaveToFile(FILE * fp)
{
    // Call base class FIRST
    CHSEngSystem::SaveToFile(fp);

    // Ok to output, so we print out our information
    // specific to this system.
    if (m_max_velocity)
	fprintf(fp, "MAX VELOCITY=%d\n", *m_max_velocity);
    if (m_acceleration)
	fprintf(fp, "ACCELERATION=%d\n", *m_acceleration);
    if (m_efficiency)
	fprintf(fp, "EFFICIENCY=%d\n", *m_efficiency);
    if (m_can_afterburn)
	fprintf(fp, "CAN AFTERBURN=%d\n", *m_can_afterburn);

    fprintf(fp, "DESIRED SPEED=%.0f\n", m_desired_speed);
    fprintf(fp, "CURRENT SPEED=%.0f\n", m_current_speed);
    fprintf(fp, "AFTERBURNING=%d\n", m_afterburning);
}


//
// CHSJumpDrive stuff
//
CHSJumpDrive::CHSJumpDrive(void)
{
    m_type = HSS_JUMP_DRIVE;
    m_charge_level = 0;
    m_engaged = FALSE;
    m_min_jump_speed = NULL;
    m_efficiency = NULL;
    m_sublight_speed = 0;
    m_fuel_source = NULL;
}

CHSEngSystem *CHSJumpDrive::Dup(void)
{
    CHSJumpDrive *ptr;

    ptr = new CHSJumpDrive;

    return (CHSEngSystem *) ptr;
}

// Prints out system information to the specified player
void CHSJumpDrive::GiveSystemInfo(int player)
{
    // Print base class info
    CHSEngSystem::GiveSystemInfo(player);

    // Now our info
    notify(player, unsafe_tprintf("CHARGE: %.2f", m_charge_level));
    notify(player,
	   unsafe_tprintf("ENGAGED: %s", m_engaged ? "YES" : "NO"));
    notify(player, unsafe_tprintf("MIN SPEED: %d", GetMinJumpSpeed()));
    notify(player, unsafe_tprintf("EFFICIENCY: %d", GetEfficiency()));
}


BOOL CHSJumpDrive::SetAttributeValue(char *strName, char *strValue)
{
    if (!strcasecmp(strName, "CHARGE")) {
	m_charge_level = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "ENGAGED")) {
	m_engaged = atoi(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "MIN SPEED")) {
	if (!m_min_jump_speed)
	    m_min_jump_speed = new int;

	*m_min_jump_speed = atoi(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "EFFICIENCY")) {
	if (!m_efficiency)
	    m_efficiency = new int;

	*m_efficiency = atoi(strValue);
	return TRUE;
    } else
	return CHSEngSystem::SetAttributeValue(strName, strValue);
}

char *CHSJumpDrive::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[64];
    *rval = '\0';

    if (!strcasecmp(strName, "ENGAGED")) {
	sprintf(rval, "%d", GetEngaged());
    } else if (!strcasecmp(strName, "EFFICIENCY")) {
	sprintf(rval, "%d", GetEfficiency());
    } else if (!strcasecmp(strName, "MIN SPEED")) {
	sprintf(rval, "%d", GetMinJumpSpeed());
    } else if (!strcasecmp(strName, "CHARGE")) {
	sprintf(rval, "%.2f", m_charge_level);
    } else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

void CHSJumpDrive::SaveToFile(FILE * fp)
{
    // Save the base first
    CHSEngSystem::SaveToFile(fp);

    // Save our stuff
    fprintf(fp, "CHARGE=%.2f\n", m_charge_level);
    fprintf(fp, "ENGAGED=%d\n", m_engaged);

    if (m_min_jump_speed)
	fprintf(fp, "MIN SPEED=%d\n", *m_min_jump_speed);
    if (m_efficiency)
	fprintf(fp, "EFFICIENCY=%d\n", *m_efficiency);
}

// Returns the charge rate for the jump drives.  If bAdjusted
// is set to TRUE, damage and power level adjustements are
// made to the value before returning it.
double CHSJumpDrive::GetChargeRate(BOOL bAdjusted)
{
    double rate;

    // Base rate of 1/2 percent per second
    rate = 0.5;

    // Adjust?
    if (bAdjusted) {
	// Damage adjustment
	rate *= 1 - (.25 * GetDamageLevel());

	// Power adjustment.  Add .00001 just in case
	// optimal power is 0.
	float fPower;
	fPower = (float) (GetOptimalPower(FALSE) + .00001);
	fPower = m_current_power / fPower;

	rate *= fPower;
    }
    return rate;
}

void CHSJumpDrive::DoCycle(void)
{
    double rate;

    // Do base stuff first
    CHSEngSystem::DoCycle();

    // Charge jumpers?
    if ((m_current_power > 0) && (m_charge_level < 100)) {
	rate = GetChargeRate(TRUE);

	if (rate > 0) {
	    m_charge_level += rate;

	    // Make sure we don't overcharge
	    if (m_charge_level > 100)
		m_charge_level = 100;

	    if (m_charge_level == 100) {
		if (m_ownerObj && m_ownerObj->GetType() == HST_SHIP) {
		    CHSShip *cShip;
		    cShip = (CHSShip *) m_ownerObj;
		    if (cShip)
			cShip->
			    NotifyConsoles(unsafe_tprintf
					   ("%s%s-%s Jump Drive charged.",
					    ANSI_HILITE, ANSI_GREEN,
					    ANSI_NORMAL), MSG_ENGINEERING);
		}
	    }

	}
    }
    // If engaged, consume fuel.
    if (m_engaged) {
	ConsumeFuelBySpeed(m_sublight_speed *
			   HSCONF.jump_speed_multiplier);
    }
}

char *CHSJumpDrive::GetStatus(void)
{
    static char tbuf[32];

    if (m_charge_level == 100)
	return "Charged";
    else if (m_current_power <= 0)
	return "Offline";
    else {
	sprintf(tbuf, "%.0f/100%%", m_charge_level);
	return tbuf;
    }
}

void CHSJumpDrive::CutPower(int level)
{
    if (level == 0) {
	if (m_ownerObj->GetType() == HST_SHIP) {
	    CHSShip *cShip;

	    cShip = (CHSShip *) m_ownerObj;

	    cShip->NotifySrooms(HSCONF.jumpers_cut);
	}
	// Set charge level to 0.
	m_charge_level = 0;

	// If jumpers are engaged and power is set to 0, then
	// disengage.
	if (GetEngaged()) {
	    char tbuf[128];

	    m_engaged = FALSE;
	    sprintf(tbuf, "%s%s-%s Jump drive disengaged.",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    if (m_ownerObj)
		m_ownerObj->HandleMessage(tbuf, MSG_ENGINEERING);
	}

    }
}

// Indicates whether jump drive is engaged or not.
BOOL CHSJumpDrive::GetEngaged(void)
{
    return m_engaged;
}

// Attempts to set the engaged status
BOOL CHSJumpDrive::SetEngaged(BOOL bStat)
{
    if (bStat) {
	// Already engaged?
	if (GetEngaged())
	    return FALSE;

	// Not enough power?
	if (m_charge_level < 100)
	    return FALSE;

	// Engage em.
	m_engaged = TRUE;

	// Drain power
	m_charge_level = 0;

	return TRUE;
    } else {
	// Not engaged?
	if (!GetEngaged())
	    return FALSE;

	// Disengage
	m_engaged = FALSE;
	return TRUE;
    }
}

// Returns the current charge percentage from 0 - 100.
int CHSJumpDrive::GetChargePerc(void)
{
    return (int) (m_charge_level);
}

// Returns the minimum jump speed for the drive.
int CHSJumpDrive::GetMinJumpSpeed(void)
{
    // Do we have a local value?
    if (!m_min_jump_speed) {
	// Do we have a parent?
	if (!m_parent)
	    return 0;
	else {
	    CHSJumpDrive *ptr;
	    ptr = (CHSJumpDrive *) m_parent;
	    return ptr->GetMinJumpSpeed();
	}
    } else
	return *m_min_jump_speed;
}

// Tells the engines that they need to gobble up some fuel
// from the fuel source.  The engines know their efficiency
// and such, so they just drain the fuel from the fuel system.
// Speed here should be given in units per hour.
void CHSJumpDrive::ConsumeFuelBySpeed(int speed)
{
    float fConsume;
    float fRealized;

    // Do we have a fuel system?
    if (!m_fuel_source || (m_fuel_source->GetFuelRemaining() <= 0))
	return;

    // Calculate per second travel rate
    fConsume = (float) (speed * .0002778);

    // Divide by efficiency
    fConsume = (float) (fConsume / (1000.0 * GetEfficiency()));

    fRealized = m_fuel_source->ExtractFuelUnit(fConsume);
    if (fRealized < fConsume) {
	// Out of gas.  Disengage.
	m_engaged = FALSE;
	char tbuf[128];
	sprintf(tbuf,
		"%s%s-%s A warning light flashing, indicating jump drives have run out of fuel.",
		ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL);
	if (m_ownerObj)
	    m_ownerObj->HandleMessage(tbuf, MSG_ENGINEERING, NULL);
    }
}

// Returns the efficiency of the jump drive, which is a 
// number specifying thousands of units of distance travel
// per unit of fuel consumption .. just like gas mileage on
// a car.  For example 1000km/gallon.  This would be returned
// here as just 1.
int CHSJumpDrive::GetEfficiency(void)
{
    // Do we have a local value?
    if (!m_efficiency) {
	// Do we have a parent?
	if (m_parent) {
	    CHSJumpDrive *ptr;
	    ptr = (CHSJumpDrive *) m_parent;
	    return ptr->GetEfficiency();
	} else
	    return 1;		// default of 1000 (1k)
    } else
	return *m_efficiency;
}

// Sets the fuel system source for the engines.  If this
// is NULL, engines don't consume fuel .. duh.
void CHSJumpDrive::SetFuelSource(CHSFuelSystem * cFuel)
{
    m_fuel_source = cFuel;
}

// Tells the jump drive the current sublight speed so that
// it can calculate fuel consumption and hyperspace speed.
void CHSJumpDrive::SetSublightSpeed(int iSpeed)
{
    // Has to be positive
    if (iSpeed < 0)
	iSpeed *= -1;

    m_sublight_speed = iSpeed;
}
