#include "hscopyright.h"
#include "hsfuel.h"
#include "hsinterface.h"

#include <string.h>
#include <stdlib.h>

CHSFuelSystem::CHSFuelSystem(void)
{
    m_burnable_fuel = 0;
    m_reactable_fuel = 0;
    m_max_burnable_fuel = NULL;
    m_max_reactable_fuel = NULL;

    m_type = HSS_FUEL_SYSTEM;

    // Fuel systems are not visible as other
    // systems are.  They are more of an 
    // interal use system.
    m_bVisible = FALSE;
}

// Returns a pointer to a new CHSFuelSystem that's just
// like this one.
CHSEngSystem *CHSFuelSystem::Dup(void)
{
    CHSFuelSystem *ptr;

    ptr = new CHSFuelSystem;

    ptr->m_bVisible = m_bVisible;

    return (CHSEngSystem *) ptr;
}

void CHSFuelSystem::GiveSystemInfo(int player)
{
    // Print base system info first
    CHSEngSystem::GiveSystemInfo(player);

    // Now our information
    notify(player, unsafe_tprintf("BURNABLE FUEL: %.2f", m_burnable_fuel));
    notify(player,
	   unsafe_tprintf("REACTABLE FUEL: %.2f", m_reactable_fuel));
    notify(player,
	   unsafe_tprintf("MAX REACTABLE FUEL: %d",
			  GetMaxFuel(FL_REACTABLE)));
    notify(player,
	   unsafe_tprintf("MAX BURNABLE FUEL: %d",
			  GetMaxFuel(FL_BURNABLE)));
}

BOOL CHSFuelSystem::SetAttributeValue(char *strName, char *strValue)
{
    int iVal;

    // Find the attribute name, and set the value.
    if (!strcasecmp(strName, "BURNABLE FUEL")) {
	m_burnable_fuel = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "REACTABLE FUEL")) {
	m_reactable_fuel = atof(strValue);
	return TRUE;
    } else if (!strcasecmp(strName, "MAX REACTABLE FUEL")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_max_reactable_fuel) {
		delete m_max_reactable_fuel;
		m_max_reactable_fuel = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (!m_max_reactable_fuel)
	    m_max_reactable_fuel = new int;

	*m_max_reactable_fuel = iVal;

	return TRUE;
    } else if (!strcasecmp(strName, "MAX BURNABLE FUEL")) {
	// If strValue contains a null, clear our local setting
	if (!*strValue) {
	    if (m_max_burnable_fuel) {
		delete m_max_burnable_fuel;
		m_max_burnable_fuel = NULL;
		return TRUE;
	    }

	    return FALSE;
	}
	iVal = atoi(strValue);
	if (!m_max_burnable_fuel)
	    m_max_burnable_fuel = new int;

	*m_max_burnable_fuel = iVal;

	return TRUE;
    }

    return CHSEngSystem::SetAttributeValue(strName, strValue);
}

// Returns the value for the specified attribute, or NULL if
// invalid attribute.
char *CHSFuelSystem::GetAttributeValue(char *strName, BOOL bAdjusted)
{
    static char rval[32];

    // Find the attribute name.
    if (!strcasecmp(strName, "MAX BURNABLE FUEL"))
	sprintf(rval, "%d", GetMaxFuel(FL_BURNABLE));
    else if (!strcasecmp(strName, "MAX REACTABLE FUEL"))
	sprintf(rval, "%d", GetMaxFuel(FL_REACTABLE));
    else if (!strcasecmp(strName, "BURNABLE FUEL"))
	sprintf(rval, "%.2f", m_burnable_fuel);
    else if (!strcasecmp(strName, "REACTABLE FUEL"))
	sprintf(rval, "%.2f", m_reactable_fuel);
    else
	return CHSEngSystem::GetAttributeValue(strName, bAdjusted);

    return rval;
}

// Returns the maximum fuel of a given type.  If no type
// is specified, it defaults to burnable fuel.
int CHSFuelSystem::GetMaxFuel(HS_FUELTYPE tType)
{
    // We have to determine type and then see if 
    // we have to return the value from the parent or
    // not.
    if (tType == FL_BURNABLE) {
	if (!m_max_burnable_fuel) {
	    if (!m_parent)
		return 0;
	    else {
		CHSFuelSystem *cFuel;
		cFuel = (CHSFuelSystem *) m_parent;
		return cFuel->GetMaxFuel(tType);
	    }
	} else
	    return *m_max_burnable_fuel;
    } else if (tType == FL_REACTABLE) {
	if (!m_max_reactable_fuel) {
	    if (!m_parent)
		return 0;
	    else {
		CHSFuelSystem *cFuel;
		cFuel = (CHSFuelSystem *) m_parent;
		return cFuel->GetMaxFuel(tType);
	    }
	} else
	    return *m_max_reactable_fuel;
    }
    // An unsupported fuel type
    return 0;
}

// Returns the current fuel levels of a given type.  If
// not type is specified, this defaults to burnable fuel.
float CHSFuelSystem::GetFuelRemaining(HS_FUELTYPE tType)
{
    if (tType == FL_BURNABLE)
	return m_burnable_fuel;
    else if (tType == FL_REACTABLE)
	return m_reactable_fuel;
    else
	return 0;
}

// ATTEMPTS to extract a given amount of fuel from the
// system for a certain type.  If type is not specified,
// it defaults to burnable fuel.  The return value is 
// the level of fuel actually given, which may not be
// the same as requested.
float CHSFuelSystem::ExtractFuelUnit(float amt, HS_FUELTYPE tType)
{
    float rval;

    if (amt < 0)
	return 0;

    if (tType == FL_BURNABLE) {
	if (amt > m_burnable_fuel) {
	    rval = m_burnable_fuel;
	    m_burnable_fuel = 0;
	    return rval;
	} else {
	    m_burnable_fuel -= amt;
	    return amt;
	}
    } else if (tType == FL_REACTABLE) {
	if (amt > m_reactable_fuel) {
	    rval = m_reactable_fuel;
	    m_reactable_fuel = 0;
	    return rval;
	} else {
	    m_reactable_fuel -= amt;
	    return amt;
	}
    } else
	return 0;
}

// Writes the fuel system attributes to the specified
// file stream.
void CHSFuelSystem::SaveToFile(FILE * fp)
{
    // Save base class info first
    CHSEngSystem::SaveToFile(fp);

    // Output RAW values.  Do not call any Get() functions,
    // because those will return local or parent values.  We
    // ONLY want to output local values.
    if (m_max_burnable_fuel)
	fprintf(fp, "MAX BURNABLE FUEL=%d\n", *m_max_burnable_fuel);
    if (m_max_reactable_fuel)
	fprintf(fp, "MAX REACTABLE FUEL=%d\n", *m_max_reactable_fuel);

    fprintf(fp, "REACTABLE FUEL=%.2f\n", m_reactable_fuel);
    fprintf(fp, "BURNABLE FUEL=%.2f\n", m_burnable_fuel);
}
