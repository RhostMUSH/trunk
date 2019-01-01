#include "hscopyright.h"
#include "hsobjects.h"
#include "hsinterface.h"
#include <stdlib.h>
#include "hsutils.h"
#include "hsconf.h"
#include "hspace.h"
#include "ansi.h"

#include <string.h>

extern double d2sin_table[];
extern double d2cos_table[];

// Gives the engineering system report for the ship
void CHSShip::GiveEngSysReport(dbref player)
{
    char tbuf[512];
    char strTmp[32];
    char strStress[256];
    char strDamage[256];
    char strPower[32];
    CHSEngSystem *cSys;
    CHSReactor *cReactor;
    CHSSysComputer *cComputer;
    int idx;

    // Give the header info
    sprintf(tbuf,
	    "%s%s.--------------------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Engineering Systems Report         %30s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, GetName(),
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >------------------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Print system list and information
    sprintf(tbuf,
	    "%s%s|%s %s%sSystem               Status    Power      Stress      Damage%s       %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, ANSI_HILITE,
	    ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // A variety of temporarily used variables
    float stress;
    int nslots;
    char pbuf[8];
    char *ptr;
    int len;

    // Loop through the systems, giving info
    for (cSys = m_systems.GetHead(); cSys; cSys = cSys->GetNext()) {
	if (!cSys->IsVisible())
	    continue;

	// Setup the stress indicator
	*strStress = '\0';
	stress = (float) (cSys->GetStress() * .10);

	// Use some tricks to print out the numerical stress as well.
	nslots = (int) (stress - .01);
	if (nslots < 10)
	    nslots++;
	sprintf(pbuf, "%.0f%%", cSys->GetStress());
	ptr = pbuf;
	len = strlen(pbuf);

	// Here's what we'll do.  The stress bar is going
	// to take up 11 characters, regardless of how much
	// color is included.  We want to print a color indicator
	// AND a number in the bar.  Thus, we have to determine
	// how many color slots will be shown and keep track of
	// where we want to put the number, which will always
	// appear at the end of the color.
	char c;
	for (idx = 0; idx < 11; idx++) {
	    if (*ptr && (nslots <= len))
		c = *ptr++;
	    else
		c = ' ';
	    if (idx < stress) {
		if (idx < 3) {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BGREEN,
			    c, ANSI_NORMAL);
		} else if (idx < 7) {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BYELLOW,
			    c, ANSI_NORMAL);
		} else {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BRED,
			    c, ANSI_NORMAL);
		}
	    } else {
		strTmp[0] = c;
		strTmp[1] = '\0';
	    }
	    strcat(strStress, strTmp);

	    nslots--;
	}

	// Setup damage indicator
	switch (cSys->GetDamageLevel()) {
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
	strcat(strDamage, "   ");

	// Setup power output string, which can vary in length
	sprintf(strPower, "%3d (%d)",
		cSys->GetCurrentPower(), cSys->GetOptimalPower());

	sprintf(tbuf,
		"%s%s|%s %-20s %-9s %-10s %-11s %-13s%s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, cSys->GetName(),
		cSys->GetStatus(),
		strPower, strStress, strDamage,
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }


    // Give Computer information.
    cComputer = (CHSSysComputer *) m_systems.GetSystem(HSS_COMPUTER);
    if (cComputer) {
	sprintf(tbuf,
		"%s%s >------------------------------------------------------------------<%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	sprintf(tbuf,
		"%s%s|%s                        %s+%s- Computer Status -%s%s+                       %s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, ANSI_HILITE,
		ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_BLUE,
		ANSI_NORMAL);
	notify(player, tbuf);
	char tbuf2[128];
	sprintf(tbuf2, "%d/%d", cComputer->GetUsedPower(),
		cComputer->GetCurrentPower());
	sprintf(tbuf, "%s%s|%s Power Usage: %-25s Surplus: %-18s %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, tbuf2,
		unsafe_tprintf("%d", cComputer->GetPowerSurplus()),
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	sprintf(tbuf2, "%d", cComputer->GetPoweredConsoles());
	sprintf(tbuf, "%s%s|%s Consoles   : %-25s Powered: %-18s %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, unsafe_tprintf("%d",
								    cComputer->
								    GetConsoles
								    ()),
		tbuf2, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }
    // Give reactor information
    cReactor = (CHSReactor *) m_systems.GetSystem(HSS_REACTOR);
    if (cReactor) {
	sprintf(tbuf,
		"%s%s >------------------------------------------------------------------<%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	sprintf(tbuf,
		"%s%s|%s                        %s+%s- Reactor Status -%s%s+                        %s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, ANSI_HILITE,
		ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_BLUE,
		ANSI_NORMAL);
	notify(player, tbuf);

	sprintf(strPower, "%d/%d(%d/%d)",
		cReactor->GetOutput(),
		cReactor->GetDesiredOutput(),
		cReactor->GetOutput() - m_systems.GetPowerUse(),
		cReactor->GetMaximumOutput());
	sprintf(tbuf,
		"%s%s|%s Type  : %-30s Output: %-20s%s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, cReactor->GetName(),
		strPower, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);

	// Setup the stress indicator
	*strStress = '\0';
	stress = (float) (cReactor->GetStress() * .10);

	// Use some tricks to print out the numerical stress as well.
	nslots = (int) (stress - .01);
	if (nslots < 10)
	    nslots++;
	sprintf(pbuf, "%.0f%%", cReactor->GetStress());
	ptr = pbuf;
	len = strlen(pbuf);

	char c;
	for (idx = 0; idx < 30; idx++) {
	    if (*ptr && (nslots <= len))
		c = *ptr++;
	    else
		c = ' ';
	    if (idx < stress) {
		if (idx < 3) {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BGREEN,
			    c, ANSI_NORMAL);
		} else if (idx < 7) {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BYELLOW,
			    c, ANSI_NORMAL);
		} else {
		    sprintf(strTmp, "%s%s%c%s", ANSI_HILITE, ANSI_BRED,
			    c, ANSI_NORMAL);
		}
	    } else {
		strTmp[0] = c;
		strTmp[1] = '\0';
	    }
	    strcat(strStress, strTmp);

	    nslots--;
	}

	// Setup damage indicator
	switch (cReactor->GetDamageLevel()) {
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
	    strcpy(strDamage, "None");
	}
	strcat(strDamage, "         ");
	sprintf(tbuf,
		"%s%s|%s Stress: %-30s Damage: %-19s %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		strStress, strDamage, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }

    CHSFuelSystem *cFuel;
    cFuel = (CHSFuelSystem *) m_systems.GetSystem(HSS_FUEL_SYSTEM);
    if (cFuel) {
	// Give a fuel systems report
	sprintf(tbuf,
		"%s%s >------------------------------------------------------------------<%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
	sprintf(tbuf,
		"%s%s|%s                          %s+%s- Fuel Status -%s%s+                         %s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, ANSI_HILITE,
		ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_BLUE,
		ANSI_NORMAL);
	notify(player, tbuf);

	// Setup the fuel indicator
	*strStress = '\0';
	float fLvl;
	float fPerc;
	fPerc = cFuel->GetFuelRemaining(FL_BURNABLE);
	fPerc = fPerc / cFuel->GetMaxFuel(FL_BURNABLE);
	fPerc *= 100;
	fLvl = (float) (fPerc * .10);

	// Use some tricks to print out the numerical fuel as well.
	nslots = (int) (fLvl - .01);
	if (nslots < 10)
	    nslots++;
	sprintf(pbuf, "%.0f%%", fPerc);
	ptr = pbuf;
	len = strlen(pbuf);

	char c;
	for (idx = 0; idx < 10; idx++) {
	    if (*ptr && (nslots <= len))
		c = *ptr++;
	    else
		c = ' ';
	    if (idx < fLvl) {
		if (idx < 3) {
		    sprintf(strTmp, "%s%s%s%c%s",
			    ANSI_HILITE, ANSI_WHITE,
			    ANSI_BRED, c, ANSI_NORMAL);
		} else if (idx < 7) {
		    sprintf(strTmp, "%s%s%s%c%s",
			    ANSI_HILITE, ANSI_WHITE,
			    ANSI_BYELLOW, c, ANSI_NORMAL);
		} else {
		    sprintf(strTmp, "%s%s%s%c%s",
			    ANSI_HILITE, ANSI_WHITE,
			    ANSI_BGREEN, c, ANSI_NORMAL);
		}
	    } else {
		strTmp[0] = c;
		strTmp[1] = '\0';
	    }
	    strcat(strStress, strTmp);

	    nslots--;
	}
	// Burnable fuel indicator setup.
	char strBurn[256];
	strcpy(strBurn, strStress);

	// Print out two fuel types?
	if (HSCONF.use_two_fuels) {
	    // Setup reactable fuel indicator
	    *strStress = '\0';
	    float fLvl;
	    float fPerc;
	    fPerc = cFuel->GetFuelRemaining(FL_REACTABLE);
	    fPerc = (float) (fPerc /
			     (cFuel->GetMaxFuel(FL_REACTABLE) + .001));
	    fPerc *= 100;
	    fLvl = (float) (fPerc * .10);

	    // Use some tricks to print out the numerical fuel as well.
	    nslots = (int) (fLvl - .01);
	    if (nslots < 10)
		nslots++;
	    sprintf(pbuf, "%.0f%%", fPerc);
	    ptr = pbuf;
	    len = strlen(pbuf);

	    char c;
	    for (idx = 0; idx < 10; idx++) {
		if (*ptr && (nslots <= len))
		    c = *ptr++;
		else
		    c = ' ';
		if (idx < fLvl) {
		    if (idx < 3) {
			sprintf(strTmp, "%s%s%c%s",
				ANSI_HILITE, ANSI_BRED, c, ANSI_NORMAL);
		    } else if (idx < 7) {
			sprintf(strTmp, "%s%s%c%s",
				ANSI_HILITE, ANSI_BYELLOW, c, ANSI_NORMAL);
		    } else {
			sprintf(strTmp, "%s%s%c%s",
				ANSI_HILITE, ANSI_BGREEN, c, ANSI_NORMAL);
		    }
		} else {
		    strTmp[0] = c;
		    strTmp[1] = '\0';
		}
		strcat(strStress, strTmp);

		nslots--;
	    }

	    sprintf(tbuf,
		    "%s%s|%s Reactor Fuel: %-10s                   Engine Fuel: %-10s %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		    strStress, strBurn,
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	    notify(player, tbuf);
	} else {
	    sprintf(tbuf,
		    "%s%s|%s Storage Level: %-30s                                          %s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		    strBurn, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	    notify(player, tbuf);
	}

    }
    // Finish the report
    sprintf(tbuf,
	    "%s%s`--------------------------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
}

// Allows a player to try to set the desired system power
// level.  If the bPercent variable is TRUE, then iLvl is
// specified in percentage of optimal power.
void CHSShip::SetSystemPower(dbref player,
			     char *lpstrName, int iLvl, BOOL bPercent)
{
    CHSEngSystem *cSys;
    CHSReactor *cReactor;
    int iPowerLeft;
    char tbuf[64];

    HSS_TYPE type;

    type = hsGetEngSystemType(lpstrName);
    if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	cSys = m_systems.GetSystemByName(lpstrName);
	if (!cSys) {
	    notify(player, "Invalid system name specified.");
	    return;
	} else {
	    type = HSS_FICTIONAL;
	}
    }

    if (type != HSS_FICTIONAL) {
	cSys = m_systems.GetSystem(type);
	if (!cSys) {
	    notify(player, "That system does not exists for that class.");
	    return;
	}
    }
    // Find the reactor to see how much power we have
    cReactor = (CHSReactor *) m_systems.GetSystem(HSS_REACTOR);
    if (!cReactor) {
	hsStdError(player, "This vessel has no power reactor.");
	return;
    }
    // Calculate power available.
    iPowerLeft = cReactor->GetOutput() - m_systems.GetPowerUse();

    // Special case for setting reactor output
    if (cSys->GetType() == HSS_REACTOR) {
	// Translate iLvl to actual output?
	if (bPercent) {
	    iLvl = (int) (cReactor->GetMaximumOutput() * (iLvl / 100.0));
	}

	if (!cReactor->SetOutputLevel(iLvl)) {
	    hsStdError(player, "Failed to set desired output level.");
	} else {
	    sprintf(tbuf, "Desired reactor output level now set to %d MW.",
		    iLvl);
	    hsStdError(player, tbuf);
	}
    } else if (cSys->IsVisible())	// Has to be visible
    {
	// It's a regular system, so try to set the desired
	// power level.

	// Translate iLvl to actual output?
	if (bPercent) {
	    iLvl = (int) (cSys->GetOptimalPower(TRUE) * (iLvl / 100.0));
	}
	// Power available?
	if ((iLvl - cSys->GetCurrentPower()) > iPowerLeft) {
	    hsStdError(player,
		       "Insufficient power available for that operation.");
	    return;
	}

	if (!cSys->SetCurrentPower(iLvl)) {
	    hsStdError(player, "Failed to set desired power level.");
	} else {
	    sprintf(tbuf, "%s power now set to %d MW.",
		    cSys->GetName(), iLvl);
	    hsStdError(player, tbuf);
	}
    } else {
	hsStdError(player, "Invalid system specified.");
	return;
    }

    // Update total power usage
    m_systems.UpdatePowerUse();
}

// Allows a player to move an engineering system up or down
// in the list, which will force the ship to draw power from
// certain systems first, preserving more important systems.
void CHSShip::ChangeSystemPriority(dbref player,
				   char *lpstrName, int iChange)
{
    CHSEngSystem *cSys;

    cSys = m_systems.GetSystemByName(lpstrName);
    if (!cSys || !cSys->IsVisible()) {
	hsStdError(player, "Invalid system specified.");
	return;
    }

    if (!m_systems.BumpSystem(cSys, iChange)) {
	hsStdError(player, "Failed to modify system priority.");
	return;
    }

    hsStdError(player, "System priority modified.");
}

// Prints a generic picture of the ship, giving hull and shield
// percentages.
void CHSShip::GiveVesselStats(dbref player)
{
    int iFore;
    int iAft;
    int iPort;
    int iStar;
    int iHull;
    CHSSysShield *cShield;
    char tbuf[512];
    char *picture[64];

    // Initially, all values are -1 to indicate the shield
    // doesn't exist.
    iFore = iAft = iPort = iStar = -1;

    // Look for each shield
    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_FORE_SHIELD);
    if (cShield)
	iFore = (int) cShield->GetShieldPerc();

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_AFT_SHIELD);
    if (cShield)
	iAft = (int) cShield->GetShieldPerc();

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_STARBOARD_SHIELD);
    if (cShield)
	iStar = (int) cShield->GetShieldPerc();

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_PORT_SHIELD);
    if (cShield)
	iPort = (int) cShield->GetShieldPerc();

    iHull = 100 * (GetHullPoints() / GetMaxHullPoints());

    // Give the header info
    sprintf(tbuf,
	    "%s%s.--------------------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Vessel Statistics                  %30s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, GetName(),
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >------------------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Now draw a pretty picture
    if (!caClasses.LoadClassPicture(m_class, picture)) {
	sprintf(tbuf,
		"%s%s|%s                       No picture available                         %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    } else {
	int idx;
	char *ptr, *dptr;
	char valbuf[64];
	char tbuf2[256];
	int len;

	for (idx = 0; picture[idx]; idx++) {
	    // Parse the buffer line for special chars
	    len = 0;
	    dptr = tbuf2;
	    for (ptr = picture[idx]; *ptr; ptr++) {
		// Look for tokens
		if (*ptr == '%') {
		    switch (*(ptr + 1)) {
		    case 'h':	// HULL
		    case 'H':
			if (iHull < 25)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_RED,
				    iHull, ANSI_NORMAL);
			else if (iHull < 50)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_YELLOW,
				    iHull, ANSI_NORMAL);
			else
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_GREEN,
				    iHull, ANSI_NORMAL);
			len += 4;
			break;
		    case 'a':	// Aft shield
		    case 'A':
			if (iAft < 0)
			    strcpy(valbuf, "---%");
			else if (iAft < 25)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_RED,
				    iAft, ANSI_NORMAL);
			else if (iAft < 50)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_YELLOW,
				    iAft, ANSI_NORMAL);
			else
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_GREEN,
				    iAft, ANSI_NORMAL);
			len += 4;
			break;
		    case 'f':	// Fore shield
		    case 'F':
			if (iFore < 0)
			    strcpy(valbuf, "---%");
			if (iFore < 25)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_RED,
				    iFore, ANSI_NORMAL);
			else if (iFore < 50)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_YELLOW,
				    iFore, ANSI_NORMAL);
			else
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_GREEN,
				    iFore, ANSI_NORMAL);
			len += 4;
			break;
		    case 's':	// Starboard shield
		    case 'S':
			if (iStar < 0)
			    strcpy(valbuf, "---%");
			if (iStar < 25)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_RED,
				    iStar, ANSI_NORMAL);
			else if (iStar < 50)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_YELLOW,
				    iStar, ANSI_NORMAL);
			else
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_GREEN,
				    iStar, ANSI_NORMAL);
			len += 4;
			break;
		    case 'p':	// Port shield
		    case 'P':
			if (iPort < 0)
			    strcpy(valbuf, "---%");
			if (iPort < 25)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_RED,
				    iPort, ANSI_NORMAL);
			else if (iPort < 50)
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_YELLOW,
				    iPort, ANSI_NORMAL);
			else
			    sprintf(valbuf,
				    "%s%s%3d%%%s",
				    ANSI_HILITE, ANSI_GREEN,
				    iPort, ANSI_NORMAL);
			len += 4;
			break;
		    case 'i':
		    case 'I':
			sprintf(valbuf, "%s%s%-10s%s",
				ANSI_HILITE, ANSI_RED,
				m_ident, ANSI_NORMAL);
			len += 10;
			break;
		    case 'x':
		    case 'X':
			sprintf(valbuf, "%s%s%3i%s",
				ANSI_HILITE, ANSI_RED,
				m_current_xyheading, ANSI_NORMAL);
			len += 3;
			break;
		    case 'z':
		    case 'Z':
			sprintf(valbuf, "%s%s%-3i%s",
				ANSI_HILITE, ANSI_RED,
				m_current_zheading, ANSI_NORMAL);
			len += 3;
			break;
		    case 'v':
		    case 'V':
			sprintf(valbuf, "%s%s%-6i%s",
				ANSI_HILITE, ANSI_RED,
				m_current_speed, ANSI_NORMAL);
			len += 6;
			break;
		    case 'n':
		    case 'N':
			sprintf(valbuf, "%s%s%-20s%s",
				ANSI_HILITE, ANSI_BLUE,
				m_name, ANSI_NORMAL);
			len += 20;
			break;

		    default:	// Not a token
			valbuf[0] = *ptr;
			valbuf[1] = *(ptr + 1);
			valbuf[2] = '\0';
			len += 2;
			break;
		    }
		    ptr++;

		    char *sptr;

		    for (sptr = valbuf; *sptr; sptr++)
			*dptr++ = *sptr;
		} else {
		    len++;
		    *dptr++ = *ptr;
		}
	    }
	    // Make sure we have the 68 printable characters needed
	    // for the display.
	    while (len < 68) {
		*dptr++ = ' ';
		len++;
	    }
	    *dptr = '\0';

	    sprintf(tbuf,
		    "%s%s|%s%-68s%s%s|%s",
		    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
		    tbuf2, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	    notify(player, tbuf);

	    delete[]picture[idx];
	}
    }

    // Finish the report
    sprintf(tbuf,
	    "%s%s`--------------------------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

}

// Allows an attribute to be set for a specified system
// on the ship.
BOOL CHSShip::SetSystemAttribute(char *lpstrSysName,
				 char *lpstrAttr, char *lpstrValue)
{
    CHSEngSystem *cSys;

    // Look for the system
    HSS_TYPE type;
    type = hsGetEngSystemType(lpstrSysName);
    /* if (type == HSS_NOTYPE)
       return FALSE; */

    // Ask the object for the system.
    if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	cSys = m_systems.GetSystemByName(lpstrSysName);
	if (!cSys)
	    return FALSE;
	else
	    type = HSS_FICTIONAL;
    } else {
	cSys = GetEngSystem(type);
    }

    if (!cSys)
	return FALSE;

    // System found, try to set the attr.
    return (cSys->SetAttributeValue(lpstrAttr, lpstrValue));
}

// Retrieves a SENSOR_CONTACT structure from the sensor
// array for the given CHSObject.
SENSOR_CONTACT *CHSShip::GetSensorContact(CHSObject * cObj)
{
    CHSSysSensors *cSensors;

    // Get the sensor array
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);

    if (!cSensors)
	return NULL;

    return cSensors->GetContact(cObj);
}

// Retrieves a SENSOR_CONTACT structure from the sensor
// array for the given an id.
SENSOR_CONTACT *CHSShip::GetSensorContact(int id)
{
    CHSSysSensors *cSensors;

    // Get the sensor array
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);

    if (!cSensors)
	return NULL;

    return cSensors->GetContactByID(id);
}

// Determines which shield on the ship is hit based on an
// incoming XY and Z angle and a given number of shields
// on the ship.
CHSSysShield *CHSShip::DetermineHitShield(int iXYAngle, int iZAngle)
{
    CHSSysShield *cFore;
    CHSSysShield *cAft;
    CHSSysShield *cPort;
    CHSSysShield *cStar;
    double iVec[4];
    double jVec[4];		// Vector storage
    double kVec[4];

    // Find our shields
    cFore = (CHSSysShield *) m_systems.GetSystem(HSS_FORE_SHIELD);
    cAft = (CHSSysShield *) m_systems.GetSystem(HSS_AFT_SHIELD);
    cPort = (CHSSysShield *) m_systems.GetSystem(HSS_PORT_SHIELD);
    cStar = (CHSSysShield *) m_systems.GetSystem(HSS_STARBOARD_SHIELD);

    // How many shields do we have?  It should be an even number
    int nShields = 0;
    if (cFore)
	nShields++;
    if (cAft)
	nShields++;
    if (cPort)
	nShields++;
    if (cStar)
	nShields++;

    // Now, based on the number of shields, calculate normal
    // vectors.  Right now, we're only supporting 2 or 4
    // shields.
    if (!nShields)
	return NULL;

    if (nShields == 1)
	return cFore;

    if (nShields == 3)
	nShields = 2;

    // Make incoming z angle positive
    if (iZAngle < 0)
	iZAngle += 360;

    int zang;
    int xyang;
    if (nShields == 2) {
	// Calculate positive zheading
	zang = m_current_zheading;
	if (zang < 0)
	    zang += 360;

	// Forward normal vector is the ship's heading
	iVec[0] = d2sin_table[m_current_xyheading] * d2cos_table[zang];
	jVec[0] = d2cos_table[m_current_xyheading] * d2cos_table[zang];
	kVec[0] = d2sin_table[zang];

	// Aft normal vector is ship's heading - 180
	xyang = m_current_xyheading + 180;
	if (xyang > 359)
	    xyang -= 360;

	// Flip the zheading sign
	zang = m_current_zheading * -1;
	if (zang < 0)
	    zang += 360;

	iVec[1] = d2sin_table[xyang] * d2cos_table[zang];
	jVec[1] = d2cos_table[xyang] * d2cos_table[zang];
	kVec[1] = d2sin_table[zang];

	// Calculate incoming vector.
	double i, j, k;
	i = d2sin_table[iXYAngle] * d2cos_table[iZAngle];
	j = d2cos_table[iXYAngle] * d2cos_table[iZAngle];
	k = d2sin_table[iZAngle];

	// Now see which dot product is negative.  That's
	// our shield.  We only have to calculate one.
	double dp1;

	dp1 = (iVec[0] * i) + (jVec[0] * j) + (kVec[0] * k);

	// Check to see if it's negative to save another
	// calculation.
	if (dp1 < 0)
	    return cFore;
	else
	    return cAft;

    } else {
	// Calculate two normal vectors to represent two
	// intersecting planes.
	// Add 45 degrees to XY to get the normal
	// vector for plane 1.
	xyang = m_current_xyheading + 45;

	// Reduce zheading to 1/2 for plane 1.
	zang = m_current_zheading * .5;

	// Now calculate normal vector 1 (N1)
	if (xyang > 359)
	    xyang -= 360;
	if (zang < 0)
	    zang += 360;
	iVec[0] = d2sin_table[xyang] * d2cos_table[zang];
	jVec[0] = d2cos_table[xyang] * d2cos_table[zang];
	kVec[0] = d2sin_table[zang];

	// Subtract 45 degrees for XY of normal vector 2
	xyang = m_current_xyheading - 45;
	zang = m_current_zheading * .5;
	if (xyang < 0)
	    xyang += 360;
	if (zang < 0)
	    zang += 360;

	// Calculate normal vector 2 (N2)
	iVec[1] = d2sin_table[xyang] * d2cos_table[zang];
	jVec[1] = d2cos_table[xyang] * d2cos_table[zang];
	kVec[1] = d2sin_table[zang];

	// Calculate vector of incoming angle
	double i, j, k;
	i = d2sin_table[iXYAngle] * d2cos_table[iZAngle];
	j = d2cos_table[iXYAngle] * d2cos_table[iZAngle];
	k = d2sin_table[iZAngle];

	// Calculate two dot products, which can result
	// in 4 sign (negative/positive) combinations.  These
	// combos will tell us which shield.
	double dp1, dp2;

	dp1 = (iVec[0] * i) + (jVec[0] * j) + (kVec[0] * k);
	dp2 = (iVec[1] * i) + (jVec[1] * j) + (kVec[1] * k);

	if (dp1 > 1)
	    dp1 = 1;
	else if (dp1 < -1)
	    dp1 = -1;

	if (dp2 > 1)
	    dp2 = 1;
	else if (dp2 < -1)
	    dp2 = -1;

	// Determine shield based on sign combo
	if (dp1 > 0) {
	    if (dp2 >= 0)
		return cAft;
	    else if (m_current_roll > 270 && m_current_roll < 90)
		return cPort;
	    else
		return cStar;
	} else {
	    if (dp2 <= 0)
		return cFore;
	    else if (m_current_roll > 270 && m_current_roll < 90)
		return cStar;
	    else
		return cPort;
	}
    }
}

// Returns a pointer to an engineering system on the ship.
CHSEngSystem *CHSShip::GetEngSystem(int type)
{
    return m_systems.GetSystem((HSS_TYPE) type);
}

// Returns a pointer to the ship's engineering systems array.
CHSSystemArray *CHSShip::GetEngSystemArray(void)
{
    return &m_systems;
}

// Returns a list of engineering system types on the
// ship, which are stored in the buffer supplied to
// the function.  The return value is the number of
// systems found on the ship.
UINT CHSShip::GetEngSystemTypes(int *iBuff)
{
    CHSEngSystem *cSys;
    int nSystems;

    nSystems = 0;
    for (cSys = m_systems.GetHead(); cSys; cSys = cSys->GetNext()) {
	iBuff[nSystems] = cSys->GetType();
	nSystems++;
    }
    return nSystems;
}

void CHSShip::GiveCrewRep(dbref player)
{
    CHSDamCon *cDamCon;

    cDamCon = (CHSDamCon *) m_systems.GetSystem(HSS_DAMCON);
    if (!cDamCon) {
	hsStdError(player, "This vessel has no damage control systems.");
	return;
    }
    if (cDamCon->GetNumCrews() <= 0) {
	hsStdError(player,
		   "There are no crews present at damage control.");
	return;
    }

    char tbuf[512];
    // Give the header info
    sprintf(tbuf,
	    "%s%s.--------------------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Damage Control Report              %30s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, GetName(),
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >------------------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s-%s#%s%s-       - %sSystem %s%s-       - %sTime Left %s%s-  - %sDamage %s%s-   - %sAim %s%s-      %s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE,
	    ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_GREEN,
	    ANSI_NORMAL, ANSI_HILITE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);


    int idx;
    char tbuf2[256];


    for (idx = 1; idx < cDamCon->GetNumCrews() + 1; idx++) {
	CHSEngSystem *tSys;

	tSys = cDamCon->GetWorkingCrew(idx);
	if (tSys)
	    sprintf(tbuf2, "%s", tSys->GetName());
	else
	    sprintf(tbuf2, "        Idle        ");

	char strDamage[64];
	char strDamage2[64];

	if (tSys)
	    switch (tSys->GetDamageLevel()) {
	    case DMG_LIGHT:
		sprintf(strDamage,
			"%s%s  LIGHT   %s",
			ANSI_HILITE, ANSI_BGREEN, ANSI_NORMAL);
		sprintf(strDamage2, "None   ");
		break;

	    case DMG_MEDIUM:
		sprintf(strDamage,
			"%s%s  MEDIUM  %s",
			ANSI_HILITE, ANSI_BYELLOW, ANSI_NORMAL);
		sprintf(strDamage2,
			"%s%s  LIGHT   %s",
			ANSI_HILITE, ANSI_BGREEN, ANSI_NORMAL);
		break;

	    case DMG_HEAVY:
		sprintf(strDamage,
			"%s%s  HEAVY   %s",
			ANSI_HILITE, ANSI_BRED, ANSI_NORMAL);
		sprintf(strDamage2,
			"%s%s  MEDIUM  %s",
			ANSI_HILITE, ANSI_BYELLOW, ANSI_NORMAL);
		break;

	    case DMG_INOPERABLE:
		sprintf(strDamage,
			"%s%sINOPERABLE%s",
			ANSI_HILITE, ANSI_BBLACK, ANSI_NORMAL);
		sprintf(strDamage2,
			"%s%s  HEAVY   %s",
			ANSI_HILITE, ANSI_BRED, ANSI_NORMAL);
		break;

	    default:
		sprintf(strDamage, "None   ");
		sprintf(strDamage2, "None   ");
	} else {
	    sprintf(strDamage, "   Idle   ");
	    sprintf(strDamage2, "   Idle   ");
	}

	sprintf(tbuf,
		"%s%s|%s[%s%i%s%s]%s  %-20s %-15s %10s %10s     %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL, idx,
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, tbuf2,
		cDamCon->GetCyclesLeft(idx) ? unsafe_tprintf("%i",
							     cDamCon->
							     GetCyclesLeft
							     (idx)) :
		"     Idle      ", strDamage, strDamage2, ANSI_HILITE,
		ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }

    sprintf(tbuf,
	    "%s%s`--------------------------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

}

void CHSShip::AssignCrew(dbref player, int iCrew, char *lpstrSysName)
{
    CHSDamCon *cDamCon;

    cDamCon = (CHSDamCon *) m_systems.GetSystem(HSS_DAMCON);
    if (!cDamCon) {
	hsStdError(player, "This vessel has no damage control systems.");
	return;
    }

    if (iCrew < 1 || iCrew > cDamCon->GetNumCrews()) {
	hsStdError(player, "Invalid damage control crew.");
	return;
    }
    CHSEngSystem *tSys;

    if (!strcasecmp(lpstrSysName, "IDLE"))
	tSys = NULL;
    else {

	// Look for the system
	HSS_TYPE type;
	type = hsGetEngSystemType(lpstrSysName);


	// Ask the object for the system.
	if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	    tSys = m_systems.GetSystemByName(lpstrSysName);
	    if (!tSys) {
		hsStdError(player, "Invalid system specified.");
		return;
	    } else
		type = HSS_FICTIONAL;
	} else {
	    tSys = GetEngSystem(type);
	}

	if (!tSys) {
	    hsStdError(player, "Invalid system specified.");
	    return;
	}

	if (tSys->GetDamageLevel() == DMG_NONE) {
	    hsStdError(player, "That system is not damaged.");
	    return;
	}
    }


    cDamCon->AssignCrew(player, iCrew, tSys);
}
