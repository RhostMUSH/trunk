#include "hscopyright.h"
#include "hsobjects.h"
#include "hscelestial.h"
#include "hsinterface.h"
#include <stdlib.h>
#include "hsutils.h"
#include "hsconf.h"
#include "hsengines.h"
#include "hsdb.h"
#include "hspace.h"
#include "ansi.h"
#include "hsuniverse.h"
#include "hsterritory.h"
#include "hsutils.h"

#include <string.h>

extern double d2cos_table[];
extern double d2sin_table[];

// Allows a player to set a desired velocity for the ship.
void CHSShip::SetVelocity(dbref player, int iVel)
{
    CHSSysEngines *cEngines;
    int iMaxVelocity;
    double dCurrentSpeed;
    char tbuf[256];

    // Check to see if we're dropping or lifting off
    if (m_drop_status) {
	hsStdError(player, "Unable to set speed during surface exchange.");
	return;
    }

    if (m_dock_status) {
	hsStdError(player, "Unable to set speed while docking.");
	return;
    }
    // Find the engines
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    if (!cEngines) {
	hsStdError(player, "This vessel has no engines.");
	return;
    }

    if (!cEngines->GetCurrentPower()) {
	hsStdError(player, HSCONF.engines_offline);
	return;
    }

    if (cEngines->GetDamageLevel() == DMG_INOPERABLE) {
	hsStdError(player, "The engines have been destroyed!");
	return;
    }

    if (m_docked || m_dropped) {
	hsStdError(player, HSCONF.ship_is_docked);
	return;
    }

    if (m_dock_status) {
	hsStdError(player, HSCONF.ship_is_docking);
	return;
    }
    // Is the ship afterburning?
    if (cEngines->GetAfterburning()) {
	hsStdError(player,
		   "Cannot set speed while afterburners are engaged.");
	return;
    }
    iMaxVelocity = cEngines->GetMaxVelocity();
    if ((iVel > iMaxVelocity) || (iVel < (-1 * (iMaxVelocity / 2.0)))) {
	sprintf(tbuf,
		"Specified velocity must range from %d to %d.",
		(int) (-1 * (iMaxVelocity / 2.0)), iMaxVelocity);
	hsStdError(player, tbuf);
	return;
    }

    if (cEngines->SetDesiredSpeed(iVel)) {
	sprintf(tbuf, "Desired velocity now set to %d hph.", iVel);
	hsStdError(player, tbuf);
    } else {
	hsStdError(player, "Failed to set desired velocity.");
	return;
    }
    dCurrentSpeed = cEngines->GetCurrentSpeed();

    /*
     * Give some effects messages 
     */
    if ((iVel > 0) && (dCurrentSpeed < 0))
	NotifySrooms(HSCONF.engine_forward);
    else if ((iVel < 0) && (dCurrentSpeed > 0))
	NotifySrooms(HSCONF.engine_reverse);
    else if (iVel > dCurrentSpeed)
	NotifySrooms(HSCONF.speed_increase);
    else
	NotifySrooms(HSCONF.speed_decrease);
}

// Sets the heading of the ship to a desired XY and Z angle.
// Either variable can be ignored, resulting in no change 
// for the angle represented by that variable.
void CHSShip::SetHeading(dbref player, int iXYAngle, int iZAngle)
{
    CHSSysThrusters *cThrust;

    if (m_hyperspace) {
	hsStdError(player, HSCONF.ship_is_jumping);
	return;
    }

    if (m_drop_status) {
	hsStdError(player,
		   "Unable to change course during surface exchange.");
	return;
    }

    if (m_dock_status) {
	hsStdError(player, "Unable to change course while docking.");
	return;
    }
    // Find the thrusters system
    cThrust = (CHSSysThrusters *) m_systems.GetSystem(HSS_THRUSTERS);
    if (!cThrust) {
	hsStdError(player, "This vessel has no maneuvering thrusters.");
	return;
    }

    if (!cThrust->GetCurrentPower()) {
	hsStdError(player, "Thrusters are not currently online.");
	return;
    }

    if (cThrust->GetDamageLevel() == DMG_INOPERABLE) {
	hsStdError(player,
		   "The maneuvering thrusters have been destroyed!");
	return;
    }

    if (iXYAngle == NOTHING)
	iXYAngle = m_desired_xyheading;

    if (iZAngle == NOTHING)
	iZAngle = m_desired_zheading;

    if ((iXYAngle < 0) || (iXYAngle > 359)) {
	notify(player, "Valid XY headings range from 0 - 359.");
	return;
    }

    if ((iZAngle < -90) || (iZAngle > 90)) {
	notify(player, "Valid Z headings range from -90 - 90.");
	return;
    }

    m_desired_xyheading = iXYAngle;
    m_desired_zheading = iZAngle;

    char tbuf[256];

    sprintf(tbuf,
	    "Course heading changed to %d mark %d.", iXYAngle, iZAngle);
    hsStdError(player, tbuf);

    //  increase_training(player, tship, .01);

}

void CHSShip::SetRoll(dbref player, int iRoll)
{
    CHSSysThrusters *cThrust;

    if (m_hyperspace) {
	hsStdError(player, HSCONF.ship_is_jumping);
	return;
    }

    if (m_drop_status) {
	hsStdError(player,
		   "Unable to change course during surface exchange.");
	return;
    }

    if (m_dock_status) {
	hsStdError(player, "Unable to change course while docking.");
	return;
    }
    // Find the thrusters system
    cThrust = (CHSSysThrusters *) m_systems.GetSystem(HSS_THRUSTERS);
    if (!cThrust) {
	hsStdError(player, "This vessel has no maneuvering thrusters.");
	return;
    }

    if (cThrust->GetDamageLevel() == DMG_INOPERABLE) {
	hsStdError(player,
		   "The maneuvering thrusters have been destroyed!");
	return;
    }

    if (!cThrust->GetCurrentPower()) {
	hsStdError(player, "Thrusters are not currently online.");
	return;
    }

    if (iRoll == NOTHING)
	iRoll = m_desired_roll;

    if (iRoll < 0 || iRoll > 359) {
	hsStdError(player, "Valid rolls reach from 0 to 359 degrees.");
	return;
    }

    m_desired_roll = iRoll;
    hsStdError(player,
	       unsafe_tprintf("Ship roll now set to %i degrees.", iRoll));

}


// Cyclicly changes the heading of the ship.
void CHSShip::ChangeHeading(void)
{
    CHSSysThrusters *cThrust;
    int iTurnRate;
    int iZChange;
    int iXYChange;
    int iDiff;
    BOOL bChanged;

    cThrust = (CHSSysThrusters *) m_systems.GetSystem(HSS_THRUSTERS);
    if (!cThrust || !cThrust->GetCurrentPower())
	return;

    iTurnRate = cThrust->GetRate();

    bChanged = FALSE;

    iXYChange = iZChange = 0;
    // Check the zheading first because this will affect the
    // trig for the XY plane.
    if (m_current_zheading != m_desired_zheading) {
	bChanged = TRUE;
	if (m_desired_zheading > m_current_zheading) {
	    if ((m_desired_zheading - m_current_zheading) > iTurnRate) {
		m_current_zheading += iTurnRate;
		iZChange = iTurnRate;
	    } else {
		iZChange = m_desired_zheading - m_current_zheading;
		m_current_zheading = m_desired_zheading;
	    }
	} else {
	    if ((m_current_zheading - m_desired_zheading) > iTurnRate) {
		m_current_zheading -= iTurnRate;
		iZChange = -iTurnRate;
	    } else {
		iZChange = m_desired_zheading - m_current_zheading;
		m_current_zheading = m_desired_zheading;
	    }
	}
    }
    // Now handle any changes in the XY plane.
    if (m_desired_xyheading != m_current_xyheading) {
	bChanged = TRUE;
	if (abs(m_current_xyheading - m_desired_xyheading) < 180) {
	    if (abs(m_current_xyheading - m_desired_xyheading) < iTurnRate) {
		iXYChange = m_desired_xyheading - m_current_xyheading;
		m_current_xyheading = m_desired_xyheading;
	    } else if (m_current_xyheading > m_desired_xyheading) {
		iXYChange = -iTurnRate;
		m_current_xyheading -= iTurnRate;
	    } else {
		iXYChange = iTurnRate;
		m_current_xyheading += iTurnRate;
	    }
	} else if (((360 - m_desired_xyheading) + m_current_xyheading) <
		   180) {
	    iDiff = (360 - m_desired_xyheading) + m_current_xyheading;
	    if (iDiff < iTurnRate) {
		iXYChange = -iDiff;
		m_current_xyheading = m_desired_xyheading;
	    } else {
		iXYChange = -iTurnRate;
		m_current_xyheading -= iTurnRate;
	    }
	} else if (((360 - m_current_xyheading) + m_desired_xyheading) <
		   180) {
	    iDiff = (360 - m_current_xyheading) + m_desired_xyheading;
	    if (iDiff < iTurnRate) {
		iXYChange = iDiff;
		m_current_xyheading = m_desired_xyheading;
	    } else {
		iXYChange = iTurnRate;
		m_current_xyheading += iTurnRate;
	    }
	} else			// This should never be true, but just in case.
	{
	    iXYChange = iTurnRate;
	    m_current_xyheading += iTurnRate;
	}

	// Check to make sure angles are 0-359
	if (m_current_xyheading > 359)
	    m_current_xyheading -= 360;
	else if (m_current_xyheading < 0)
	    m_current_xyheading += 360;
    }
    // Now handle any changes in the XY plane.
    if (m_desired_roll != m_current_roll) {
	if (abs(m_current_roll - m_desired_roll) < 180) {
	    if (abs(m_current_roll - m_desired_roll) < iTurnRate) {
		iXYChange = m_desired_roll - m_current_roll;
		m_current_roll = m_desired_roll;
	    } else if (m_current_roll > m_desired_roll) {
		iXYChange = -iTurnRate;
		m_current_roll -= iTurnRate;
	    } else {
		iXYChange = iTurnRate;
		m_current_roll += iTurnRate;
	    }
	} else if (((360 - m_desired_roll) + m_current_roll) < 180) {
	    iDiff = (360 - m_desired_roll) + m_current_roll;
	    if (iDiff < iTurnRate) {
		iXYChange = -iDiff;
		m_current_roll = m_desired_roll;
	    } else {
		iXYChange = -iTurnRate;
		m_current_roll -= iTurnRate;
	    }
	} else if (((360 - m_current_roll) + m_desired_roll) < 180) {
	    iDiff = (360 - m_current_roll) + m_desired_roll;
	    if (iDiff < iTurnRate) {
		iXYChange = iDiff;
		m_current_roll = m_desired_roll;
	    } else {
		iXYChange = iTurnRate;
		m_current_roll += iTurnRate;
	    }
	} else			// This should never be true, but just in case.
	{
	    iXYChange = iTurnRate;
	    m_current_roll += iTurnRate;
	}

	// Check to make sure angles are 0-359
	if (m_current_roll > 359)
	    m_current_roll -= 360;
	else if (m_current_roll < 0)
	    m_current_roll += 360;
    }

    // If nothing was changed, do nothing, else update stuff.
    if (bChanged) {
	// Run through any consoles, changing their headings
	int idx;
	for (idx = 0; idx < MAX_SHIP_CONSOLES; idx++) {
	    if (m_console_array[idx])
		m_console_array[idx]->AdjustHeading(iXYChange, iZChange);
	}

	// Recompute heading vector.  Necessary for any heading change.
	RecomputeVectors();
    }
}

// Returns the current speed of the ship.
int CHSShip::GetSpeed(void)
{
    CHSSysEngines *cEngines;

    // Find the engines
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    if (!cEngines)
	return 0;
    else {
	int speed;
	speed = (int) cEngines->GetCurrentSpeed();
	return speed;
    }
}

// Returns the XY heading of the ship.
UINT CHSShip::GetXYHeading(void)
{
    return m_current_xyheading;
}

// Returns the XY heading of the ship.
UINT CHSShip::GetRoll(void)
{
    return m_current_roll;
}

// Returns the current Z heading of the ship
int CHSShip::GetZHeading(void)
{
    return m_current_zheading;
}

// Moves the ship according to engine settings, heading vectors,
// all that good stuff.
void CHSShip::Travel(void)
{
    double speed;
    double dCurrentSpeed;
    CHSSysEngines *cEngines;

    // Find the engines
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    if (!cEngines)
	dCurrentSpeed = 0;
    else
	dCurrentSpeed = cEngines->GetCurrentSpeed();
    /*
     * Speed is measured in Hetramere per hour 
     */
    if (dCurrentSpeed) {
	// Bring speed down to the unit per second level 
	// The .0002778 is actually 1/3600.0 precomputed
	// to save time.
	speed = (dCurrentSpeed) * (.0002778 * HSCONF.cyc_interval);

	/*
	 * If the ship is jumping, everything is accelerated 
	 */
	if (m_hyperspace)
	    speed *= HSCONF.jump_speed_multiplier;

	// Add to the current position .. heading vector * speed.
	m_x += m_motion_vector.i() * speed;
	m_y += m_motion_vector.j() * speed;
	m_z += m_motion_vector.k() * speed;

	// See if we've moved into a new territory.
	CHSTerritory *newterritory;
	newterritory = taTerritories.InTerritory(this);

	dbref oldterr, newterr;

	// Check to see if we've moved in or out of a territory
	if (!m_territory)
	    oldterr = NOTHING;
	else
	    oldterr = m_territory->GetDbref();

	if (newterritory)
	    newterr = newterritory->GetDbref();
	else
	    newterr = NOTHING;

	if (oldterr != newterr) {
	    // We've crossed borders.

	    // If entering a new territory, only give that
	    // message.  Else, give a leave message for the
	    // old one.
	    char *s;
	    if (newterr && hsInterface.ValidObject(newterr)) {
		if (hsInterface.AtrGet(newterr, "ENTER")) {
		    s = hsInterface.EvalExpression(hsInterface.m_buffer,
						   newterr,
						   m_objnum, m_objnum);
		    NotifyConsoles(s, MSG_GENERAL);
		}
		did_it(m_objnum, newterr, A_ENTER, NULL, A_OENTER,
		       NULL, A_AENTER, (char **) NULL, 0);
	    } else if (hsInterface.ValidObject(oldterr)) {
		if (hsInterface.AtrGet(oldterr, "LEAVE")) {
		    s = hsInterface.EvalExpression(hsInterface.m_buffer,
						   oldterr,
						   m_objnum, m_objnum);
		    NotifyConsoles(s, MSG_GENERAL);
		}
	    }
	    // Always trigger ALEAVE for territory left.
	    if (oldterr != NOTHING && hsInterface.ValidObject(oldterr))
		did_it(m_objnum, newterr, A_LEAVE, NULL, A_OLEAVE,
		       NULL, A_ALEAVE, (char **) NULL, 0);

	}
	// Set our new territory
	m_territory = newterritory;
    }
}

// Handles recomputing the current heading vectors.  Handy function.
void CHSShip::RecomputeVectors(void)
{
    int zhead;
    int xyhead;

    zhead = m_current_zheading;
    if (zhead < 0)
	zhead += 360;

    xyhead = m_current_xyheading;
    CHSVector tvec(d2sin_table[xyhead] * d2cos_table[zhead],
		   d2cos_table[xyhead] * d2cos_table[zhead],
		   d2sin_table[zhead]);

    m_motion_vector = tvec;
}

// Gives the big, navigation status report for the vessel.
void CHSShip::GiveNavigationStatus(dbref player)
{
    CHSSysComputer *cComputer;
    CHSSysSensors *cSensors;
    CHSSysShield *cShield;
    CHSSysEngines *cEngines;

    // Lines that will get printed in the map portion
    char mLine1[128];
    char mLine2[128];
    char mLine3[128];
    char mLine4[128];
    char mLine5[128];
    char mLine6[128];
    char mLine7[128];
    char mLine8[128];

    char charcolors[64][64];	// Used to store object chars and colors

    char tbuf[256];

    int insX, insY;		// Where to insert into the map
    int idx;
    int tdx;
    char *ptr;

    // Find a variety of systems we'll need.
    cComputer = (CHSSysComputer *) m_systems.GetSystem(HSS_COMPUTER);
    if (!cComputer) {
	hsStdError(player, "This vessel has no internal computer.");
	return;
    }

    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);

    charcolors[0][0] = '\0';

    // We have to spend the majority of this function
    // just shoving characters into the map, so that's what we'll
    // start with.
    memset(mLine1, '\0', 128);
    memset(mLine2, '\0', 128);
    for (idx = 0; idx < 32; idx++)
	mLine1[idx] = ' ';
    for (idx = 0; idx < 34; idx++)
	mLine2[idx] = ' ';
    for (idx = 0; idx < 35; idx++) {
	mLine3[idx] = ' ';
	mLine4[idx] = ' ';
	mLine5[idx] = ' ';
	mLine6[idx] = ' ';
	mLine7[idx] = ' ';
    }
    mLine3[idx] = '\0';
    mLine4[idx] = '\0';
    mLine5[idx] = '\0';
    mLine6[idx] = '\0';
    mLine7[idx] = '\0';
    for (idx = 0; idx < 34; idx++)
	mLine8[idx] = ' ';
    mLine8[idx] = '\0';

    // Ok, now we need to run through sensor contacts and
    // insert them into the map.  Each object has a certain
    // character and color, and we need to map it's real world
    // coordinates into the map.  The map range specifies the
    // scale of the map, so if the object maps to a location
    // outside of the scale, it's excluded.
    if (cSensors) {
	SENSOR_CONTACT *cContact;
	CHSObject *cObj;
	double dX, dY;
	char filler;

	for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	    cContact = cSensors->GetContact(idx);
	    if (!cContact)
		continue;

	    cObj = cContact->m_obj;

	    // Grab it's coordinates
	    dX = cObj->GetX();
	    dY = cObj->GetY();

	    // Check to see if it's within our map range
	    if ((m_x - dX) > m_map_range ||
		(m_x - dX) < -m_map_range ||
		(m_y - dY) > m_map_range || (m_y - dY) < -m_map_range)
		continue;

	    filler = cObj->GetObjectCharacter();

	    // See if we need to store color and character
	    // info for this object type.
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == filler)
		    break;
	    }

	    if (!charcolors[tdx][0]) {
		// Store it
		ptr = charcolors[tdx];

		charcolors[tdx][0] = filler;

		charcolors[tdx][1] = '\0';
		strcat(charcolors[tdx], cObj->GetObjectColor());
		charcolors[tdx + 1][0] = '\0';
	    }
	    // Determine where to put the object in the ASCII map.
	    insX = (int) ((((dX - m_x) / (m_map_range * 2)) * 34) + 17);
	    insY = (int) ((((m_y - dY) / (m_map_range * 2)) * 8) + 4);

	    switch (insY) {
	    case 0:
		if (insX > 2 && insX < 32)
		    mLine1[insX] = filler;
		break;
	    case 1:
		if (insX > 0 && insX < 34)
		    mLine2[insX] = filler;
		break;
	    case 2:
		mLine3[insX] = filler;
		break;
	    case 3:
		mLine4[insX] = filler;
		break;
	    case 4:
		mLine5[insX] = filler;
		break;
	    case 5:
		mLine6[insX] = filler;
		break;
	    case 6:
		mLine7[insX] = filler;
		break;
	    default:
		if (insX > 2 && insX < 34)
		    mLine8[insX] = filler;
		break;
	    }
	}
    }

    // Now do HUD stuff

    // XY HUD first
    int x1, x2, x3;
    x2 = (int) ((m_current_xyheading / 10.0) * 10);
    x1 = x2 - 10;
    x3 = x2 + 10;
    if (x1 < 0)
	x1 += 360;
    else if (x1 > 359)
	x1 -= 360;
    if (x2 < 0)
	x2 += 360;
    else if (x2 > 359)
	x2 -= 360;
    if (x3 < 0)
	x3 += 360;
    else if (x3 > 359)
	x3 -= 360;

    // Now Z HUD stuff
    int z1, z2, z3;		// This are the little ticks on the heading bar
    z2 = (int) ((m_current_zheading / 10.0) * 10);
    z1 = z2 + 10;
    z3 = z2 - 10;

    // Now print the display.
    char tbuf2[256];
    sprintf(tbuf,
	    "%s%s.---------------------------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf2, "%s (%s)", GetName(), m_ident ? m_ident : "--");
    sprintf(tbuf, "%s%s|%s %-29s %s%s|%s %40s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf2,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    m_classinfo->m_name, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s >---%sNavigation Status Report%s---%s+%s------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_WHITE, ANSI_BLUE,
	    ANSI_WHITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    char tnum1[32];
    char tnum2[32];
    char tnum3[32];
    if (x1 < 10)
	sprintf(tnum1, " %d ", x1);
    else
	sprintf(tnum1, "%-3d", x1);
    if (x2 < 10)
	sprintf(tnum2, " %d ", x2);
    else
	sprintf(tnum2, "%3d", x2);
    if (x3 < 10)
	sprintf(tnum3, " %d ", x3);
    else
	sprintf(tnum3, "%3d", x3);
    sprintf(tbuf,
	    "%s%s|%s                   %s        %s        %s     %s%s| %sX:%s %10.0f           %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tnum1, tnum2, tnum3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_x, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s|%s %4d%s%s__             |____%s%s.%s_____|_____%s%s.%s____|      %s| %sY:%s %10.0f           %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    z1,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_y, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    if (m_current_xyheading < 10)
	sprintf(tnum1, " %d ", m_current_xyheading);
    else
	sprintf(tnum1, "%3d", m_current_xyheading);
    sprintf(tbuf,
	    "%s%s|       %s|%s                    > %s <              %s%s| %sZ:%s %10.0f           %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    tnum1,
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_z, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s|%s      %s-%s|          %s___________________________    |%s %s+%s- Course -%s%s+            %s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Now we have to print out the map lines.  Phew.
    char tbuf3[256];

    // Line 1
    *tbuf2 = '\0';
    for (idx = 3; mLine1[idx]; idx++) {
	if (mLine1[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine1[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tbuf3, "%s%c%s", ptr, mLine1[idx], ANSI_NORMAL);
	    else
		sprintf(tbuf3,
			"%s%c%s", ANSI_HILITE, mLine1[idx], ANSI_NORMAL);
	} else
	    sprintf(tbuf3, " ");
	strcat(tbuf2, tbuf3);
    }
    sprintf(tbuf,
	    "%s%s|%s %4d%s%s-->%s %-3d    %s%s/%s%s%s%s\\  | %sC:%s %3d/%-3d  %s%sD:%s %3d/%-3d  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    z2,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    m_current_zheading,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf2,
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_current_xyheading, m_current_zheading,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    m_desired_xyheading, m_desired_zheading,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);


    // Line 2
    *tbuf3 = '\0';
    for (idx = 1; mLine2[idx]; idx++) {
	if (mLine2[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine2[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine2[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine2[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }
    if (cEngines && cEngines->CanBurn()) {
	sprintf(tbuf2, "%.0f/%s%.0f%s (%.0f)",
		cEngines ? cEngines->GetCurrentSpeed() : 0.0,
		cEngines->GetAfterburning()? "*" : "",
		cEngines ? cEngines->GetDesiredSpeed() : 0.0,
		cEngines->GetAfterburning()? "*" : "",
		cEngines ? cEngines->GetMaxVelocity() : 0.0);
    } else
	sprintf(tbuf2, "%.0f/%.0f (%.0f)",
		cEngines ? cEngines->GetCurrentSpeed() : 0.0,
		cEngines ? cEngines->GetDesiredSpeed() : 0.0,
		cEngines ? cEngines->GetMaxVelocity() : 0.0);
    sprintf(tbuf,
	    "%s%s|%s      %s-%s|%s      %s%s/%s%s%s%s\\| %sV:%s %-21s%s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf3, ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    tbuf2, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Line 3
    *tbuf3 = '\0';
    for (idx = 0; mLine3[idx]; idx++) {
	if (mLine3[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine3[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine3[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine3[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }
    sprintf(tbuf,
	    "%s%s|     %s__|     %s|%s%s%s%s|                         |%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL,
	    tbuf3, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Line 4
    *tbuf3 = '\0';
    for (idx = 0; mLine4[idx]; idx++) {
	if (mLine4[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine4[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine4[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine4[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_FORE_SHIELD);
    if (!cShield)
	strcpy(tnum1, "  * ");
    else
	sprintf(tnum1, "%.0f%%", cShield->GetShieldPerc());
    sprintf(tbuf,
	    "%s%s|%s %4d        %s%s|%s%s%s%s| %sShields%s  %4s           %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    z3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    tnum1, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Line 5
    *tbuf3 = '\0';
    for (idx = 0; mLine5[idx]; idx++) {
	if (idx == 17)
	    sprintf(tnum1, "%s+%s", ANSI_HILITE, ANSI_NORMAL);
	else if (mLine5[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine5[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine5[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine5[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }
    sprintf(tbuf,
	    "%s%s|             |%s%s%s%s|%s            |            %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, tbuf3, ANSI_HILITE,
	    ANSI_BLUE, ANSI_NORMAL, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Line 6
    *tbuf3 = '\0';
    for (idx = 0; mLine6[idx]; idx++) {
	if (mLine6[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine6[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine6[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine6[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_PORT_SHIELD);
    if (cShield)
	sprintf(tnum1, "%.0f%%", cShield->GetShieldPerc());
    else
	strcpy(tnum1, "   *");

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_STARBOARD_SHIELD);
    if (cShield)
	sprintf(tnum2, "%.0f%%", cShield->GetShieldPerc());
    else
	strcpy(tnum2, "*   ");

    double perc;
    perc = GetMaxHullPoints();
    perc = 100 * (GetHullPoints() / perc);
    sprintf(tbuf,
	    "%s%s| %sHP:%s %3.0f%%    %s%s|%s%s%s%s|%s      %4s -%s+%s- %-4s      %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    perc,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tnum1, ANSI_HILITE, ANSI_NORMAL,
	    tnum2, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Line 7
    *tbuf3 = '\0';
    for (idx = 0; mLine7[idx]; idx++) {
	if (mLine7[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine7[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine7[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine7[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }

    sprintf(tbuf,
	    "%s%s| %sMR:%s %-8d%s%s|%s%s%s%s|%s            |            %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL,
	    m_map_range,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tbuf3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    // Finally, line 8.
    *tbuf3 = '\0';
    for (idx = 1; mLine8[idx]; idx++) {
	if (mLine8[idx] != ' ') {
	    // Find the character type and color
	    ptr = NULL;
	    for (tdx = 0; charcolors[tdx][0]; tdx++) {
		if (charcolors[tdx][0] == mLine8[idx]) {
		    ptr = charcolors[tdx];
		    ptr++;
		    break;
		}
	    }
	    if (ptr)
		sprintf(tnum1, "%s%c%s", ptr, mLine8[idx], ANSI_NORMAL);
	    else
		sprintf(tnum1,
			"%s%c%s", ANSI_HILITE, mLine8[idx], ANSI_NORMAL);
	} else
	    sprintf(tnum1, " ");
	strcat(tbuf3, tnum1);
    }

    cShield = (CHSSysShield *) m_systems.GetSystem(HSS_AFT_SHIELD);
    if (!cShield)
	strcpy(tnum1, "  * ");
    else
	sprintf(tnum1, "%.0f%%", cShield->GetShieldPerc());
    sprintf(tbuf,
	    "%s%s| %s             %s\\%s%s%s%s/%s           %4s           %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN,
	    ANSI_BLUE, ANSI_NORMAL,
	    tbuf3,
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL,
	    tnum1, ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

    sprintf(tbuf,
	    "%s%s`---------------\\\\.___________________________.//--------------------------`%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);

    notify(player, tbuf);
}

// Sets the map range of the ship to a given distance.
// This distance is like a scale.  It specifies, basically,
// a clipping region such that objects further than this 
// distance are not included in the map.  At the same time,
// it maps objects within the clipping range to the map
// on the navigation display.
BOOL CHSShip::SetMapRange(dbref player, int range)
{
    // Have to have a real range
    if (range <= 0)
	return FALSE;

    // We can handle up to 8 digit ranges
    if (range > 99999999)
	return FALSE;

    m_map_range = range;

    return TRUE;
}

// Attempts to land the ship in another ship or on a 
// celestial surface.
void CHSShip::LandVessel(dbref player, int id, int loc, char *lpstrCode)
{
    SENSOR_CONTACT *cContact;
    CHSSysSensors *cSensors;
    CHSSysEngines *cEngines;
    CHSObject *cObj;
    char tbuf[256];
    HS_TYPE tType;
    CHSLandingLoc *cLocation;

    // Perform some situational error checking
    if (m_drop_status) {
	hsStdError(player, "You may not do that during surface exchange.");
	return;
    }

    if (m_dock_status) {
	hsStdError(player,
		   "Ship is currently in docking/undocking procedures.");
	return;
    }
    // Find the sensors, so we can grab the contact
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
    if (!cSensors) {
	hsStdError(player,
		   "This vessel has no sensors.  Contact not found.");
	return;
    }
    // Find the sensor contact based on the given number.
    cContact = cSensors->GetContactByID(id);
    if (!cContact) {
	sprintf(tbuf, "%s%s[%s%4d%s%s]%s - No such contact on sensors.",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		id, ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	notify(player, tbuf);
	return;
    }
    // Are our engines online?
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    if (!cEngines) {
	hsStdError(player, "This vessel has no engines.  Unable to land.");
	return;
    }

    if (cEngines->GetCurrentPower() == 0) {
	hsStdError(player, "Engines are currently offline.");
	return;
    }
    // Contact found, so let's see if it's an object
    // we can land on.  We support ships and planets.
    cObj = cContact->m_obj;
    tType = cObj->GetType();
    if (tType != HST_SHIP && tType != HST_PLANET) {
	sprintf(tbuf,
		"%s%s[%s%4d%s%s]%s - Cannot land on that type of object.",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		id, ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	notify(player, tbuf);
	return;
    }
    // Do some ship to ship error checking.
    if (cObj->GetType() == HST_SHIP) {
	CHSShip *cShip;
	cShip = (CHSShip *) cObj;

	// Are we too big?
	if (GetSize() > (unsigned int)HSCONF.max_dock_size && !cShip->IsSpacedock()) {
	    hsStdError(player,
		       "Our vessel is too large to dock in another vessel.");
	    return;
	}
	// Are they too small?
	if (GetSize() >= cObj->GetSize()) {
	    hsStdError(player,
		       "That vessel is too small to accomodate us.");
	    return;
	}
	// Is it within range?
	double dDist;

	dDist = Dist3D(m_x, m_y, m_z,
		       cObj->GetX(), cObj->GetY(), cObj->GetZ());
	if (dDist > HSCONF.max_dock_dist) {
	    sprintf(tbuf,
		    "Location must be within %d %s to commence docking.",
		    HSCONF.max_dock_dist, HSCONF.unit_name);
	    hsStdError(player, tbuf);
	    return;
	}
	// Must be at a full stop to dock.
	if (cEngines && cEngines->GetCurrentSpeed()) {
	    hsStdError(player,
		       "Must be at a full stop to commence docking.");
	    return;
	}
	// Find the landing location
	// The landing locations are specified by the player as
	// 1 .. n, but we store them in array notation 0..n-1.
	// Thus, subtract 1 from location when retrieving.
	cLocation = cShip->GetLandingLoc(loc - 1);
    } else			// Must be a planet
    {
	// Do we have drop rockets?
	if (!CanDrop()) {
	    hsStdError(player,
		       "This vessel does not have drop capability.");
	    return;
	}
	// Is it within range?
	double dDist;

	dDist = Dist3D(m_x, m_y, m_z,
		       cObj->GetX(), cObj->GetY(), cObj->GetZ());
	if (dDist > HSCONF.max_drop_dist) {
	    sprintf(tbuf,
		    "Location must be within %d %s to commence landing.",
		    HSCONF.max_drop_dist, HSCONF.unit_name);
	    hsStdError(player, tbuf);
	    return;
	}
	// Are we too fast to land?
	if (cEngines && cEngines->GetCurrentSpeed() >
	    HSCONF.max_land_speed) {
	    sprintf(tbuf,
		    "Must be traveling at less than %d %s to commence landing.",
		    HSCONF.max_land_speed, HSCONF.unit_name);
	    hsStdError(player, tbuf);
	    return;
	}
	// Find the landing location
	CHSPlanet *cPlanet;
	cPlanet = (CHSPlanet *) cObj;

	cLocation = cPlanet->GetLandingLoc(loc - 1);
    }

    // Location exists?
    if (!cLocation) {
	hsStdError(player, "That landing location does not exist.");
	return;
    }
    // See if the landing site is active.
    if (!cLocation->IsActive()) {
	if (tType == HST_PLANET) {
	    hsStdError(player,
		       "That landing site is currently not open for landing.");
	    return;
	} else {
	    hsStdError(player, "The bay doors to that bay are closed.");
	    return;
	}
    }
    // See if we have code clearance?
    if (!cLocation->CodeClearance(lpstrCode)) {
	hsStdError(player, "Invalid landing code -- permission denied.");
	return;
    }
    // See if the landing site can accomodate us.
    if (!cLocation->CanAccomodate(this)) {
	hsStdError(player, "Landing site cannot accomodate this vessel.");
	return;
    }
    // Set desired and current speed to 0 to stop the ship.
    cEngines->SetDesiredSpeed(0);
    cEngines->SetAttributeValue("CURRENT SPEED", "0");

    // Ok.  Everything checks out, so begin landing procedures.
    InitLanding(player, cContact, cLocation);
}

// Initializes the landing procedures, be it for a planet or ship.
void CHSShip::InitLanding(dbref player,
			  SENSOR_CONTACT * cContact,
			  CHSLandingLoc * cLocation)
{
    HS_TYPE tType;
    CHSObject *cObj;
    char tbuf[256];
    CHSUniverse *uDest;

    // Grab a few variables from the contact
    cObj = cContact->m_obj;
    tType = cObj->GetType();

    // Get our universe
    uDest = uaUniverses.FindUniverse(m_uid);

    // Now, what type of object is it?  We need to set some
    // variables.
    m_drop_status = 0;
    m_dock_status = 0;
    switch (tType) {
    case HST_SHIP:
	CHSShip * cShip;
	cShip = (CHSShip *) cObj;

	// Give some messages
	sprintf(tbuf,
		"%s[%s%s%d%s%s]%s - Docking request accepted .. beginning docking procedures.",
		cObj->GetObjectColor(), ANSI_NORMAL,
		ANSI_HILITE, cContact->m_id, ANSI_NORMAL,
		cObj->GetObjectColor(), ANSI_NORMAL);
	NotifyConsoles(tbuf, MSG_GENERAL);

	sprintf(tbuf,
		"The %s is beginning docking procedures at this location.",
		GetName());
	cShip->HandleMessage(tbuf, MSG_SENSOR, (long *) this);

	sprintf(tbuf,
		"In the distance, the %s begins docking procedures with the %s.",
		GetName(), cObj->GetName());

	if (uDest)
	    uDest->SendContactMessage(tbuf, IDENTIFIED, this);

	// Set the dock status to 8, which is the number
	// of seconds it takes to dock.  Undocking is a 
	// negative number, so sign matters!
	m_dock_status = 8;
	m_docked = FALSE;
	m_landing_target = cLocation;
	break;

    case HST_PLANET:
	// Give some messages
	sprintf(tbuf,
		"%s-%s Beginning descent to the surface of %s ...",
		cObj->GetObjectColor(), ANSI_NORMAL, cObj->GetName());
	NotifyConsoles(tbuf, MSG_GENERAL);
	NotifySrooms(HSCONF.begin_descent);

	sprintf(tbuf,
		"In the distance, the %s begins its descent toward the surface of %s.",
		GetName(), cObj->GetName());

	if (uDest)
	    uDest->SendContactMessage(tbuf, IDENTIFIED, this);

	// Set the drop status to the positive number
	// of seconds it takes to drop.  This is specified
	// in the config file.
	m_drop_status = HSCONF.seconds_to_drop;
	m_dropped = FALSE;
	m_landing_target = cLocation;
	break;

    default:			// Who knows!
	notify(player,
	       "Help!  What type of HSpace object are you landing on?");
	return;
    }
}

// Handles landing the ship, be it to a planet or another ship.
void CHSShip::HandleLanding(void)
{
    CHSSysShield *cShield;
    CHSUniverse *uSource;
    char tbuf[256];

    // Determine which procedure we're in.
    if (m_dock_status > 0) {
	// We're landing in a ship.
	m_dock_status--;

	switch (m_dock_status) {
	case 6:		// Bay doors open
	    NotifyConsoles
		("The enormous bay doors slowly begin to slide open in front of the ship.\n",
		 MSG_GENERAL);
	    m_landing_target->
		HandleMessage
		("The enormous bay doors slowly begin to slide open ...",
		 MSG_GENERAL);
	    break;

	case 3:		// Shields are dropped
	    BOOL bShields;
	    BOOL bShieldsUp;

	    bShields = FALSE;
	    bShieldsUp = FALSE;

	    // If no shields present, go to docking
	    cShield = (CHSSysShield *)
		m_systems.GetSystem(HSS_FORE_SHIELD);
	    if (cShield) {
		if (cShield->GetCurrentPower() > 0)
		    bShieldsUp = TRUE;
		cShield->SetCurrentPower(0);
		bShields = TRUE;
	    }
	    cShield = (CHSSysShield *)
		m_systems.GetSystem(HSS_AFT_SHIELD);
	    if (cShield) {
		if (cShield->GetCurrentPower() > 0)
		    bShieldsUp = TRUE;
		cShield->SetCurrentPower(0);
		bShields = TRUE;
	    }
	    cShield = (CHSSysShield *)
		m_systems.GetSystem(HSS_PORT_SHIELD);
	    if (cShield) {
		if (cShield->GetCurrentPower() > 0)
		    bShieldsUp = TRUE;
		cShield->SetCurrentPower(0);
		bShields = TRUE;
	    }
	    cShield = (CHSSysShield *)
		m_systems.GetSystem(HSS_STARBOARD_SHIELD);
	    if (cShield) {
		if (cShield->GetCurrentPower() > 0)
		    bShieldsUp = TRUE;
		cShield->SetCurrentPower(0);
		bShields = TRUE;
	    }
	    if (!bShields) {
		m_dock_status = 1;	// Skip quickly to docking
	    } else {
		sprintf(tbuf,
			"%s%s-%s Shield check ...",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
		NotifyConsoles(tbuf, MSG_GENERAL);
		if (bShieldsUp) {
		    sprintf(tbuf,
			    "  %s%s-%s lowering shields ...",
			    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
		    NotifyConsoles(tbuf, MSG_GENERAL);
		} else {
		    sprintf(tbuf,
			    "  %s%s-%s shields down.",
			    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
		    NotifyConsoles(tbuf, MSG_GENERAL);
		}
	    }
	    break;

	case 0:		// Ship docks
	    sprintf(tbuf,
		    "Through the bay doors, the %s comes in and docks.",
		    GetName());
	    m_landing_target->HandleMessage(tbuf, MSG_GENERAL);
	    sprintf(tbuf,
		    "The %s pushes forward as it glides in and docks.",
		    GetName());
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    m_docked = TRUE;
	    MoveShipObject(m_landing_target->objnum);

	    // Set our location
	    CHSObject *cOwner;
	    cOwner = m_landing_target->GetOwnerObject();
	    if (cOwner)
		m_objlocation = cOwner->GetDbref();
	    else
		m_objlocation = NOTHING;

	    // Remove us from active space
	    uSource = uaUniverses.FindUniverse(m_uid);
	    if (uSource)
		uSource->RemoveActiveObject(this);

	    // Deduct the capacity from the landing loc
	    m_landing_target->DeductCapacity(GetSize());
	    break;
	}
    } else if (m_drop_status > 0) {
	int iHalfMarker;

	// We're dropping to a planet.
	m_drop_status--;

	iHalfMarker = (int) (HSCONF.seconds_to_drop / 2.0);

	// At the half way point, give another message
	if (m_drop_status == iHalfMarker) {
	    CHSSysSensors *cSensors;

	    sprintf(tbuf,
		    "%s%s-%s Surface contact in %d seconds ...",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, iHalfMarker);
	    NotifyConsoles(tbuf, MSG_GENERAL);

	    sprintf(tbuf,
		    "In the sky above, a dropship comes into view as it descends toward the surface.");
	    m_landing_target->HandleMessage(tbuf, MSG_GENERAL);

	    // Clear the sensors if needed
	    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
	    if (cSensors)
		cSensors->ClearContacts();

	    // Remove us from the active universe.  We're in
	    // the atmosphere now.
	    uSource = uaUniverses.FindUniverse(m_uid);
	    if (uSource)
		uSource->RemoveActiveObject(this);

	    // Indicate that we're no longer in space.  We're
	    // somewhere between heaven and hell!
	    m_in_space = FALSE;
	} else if (m_drop_status == 0) {
	    NotifySrooms(HSCONF.landing_msg);
	    sprintf(tbuf,
		    "You take a step back as the %s fires its drop rockets and lands before you.",
		    GetName());
	    m_landing_target->HandleMessage(tbuf, MSG_GENERAL);
	    MoveShipObject(m_landing_target->objnum);
	    m_dropped = TRUE;

	    // Set our location
	    CHSObject *cOwner;
	    cOwner = m_landing_target->GetOwnerObject();
	    if (cOwner)
		m_objlocation = cOwner->GetDbref();
	    else
		m_objlocation = NOTHING;

	    m_landing_target->DeductCapacity(GetSize());
	}
    }
}

// Allows a player to undock, or lift off, a ship that is
// docked or dropped.
void CHSShip::UndockVessel(dbref player)
{
    CHSSysEngines *cEngines;
    CHSLandingLoc *cLocation;
    CHSSysComputer *cComputer;
    CHSObject *cObj;
    dbref dbRoom;
    char tbuf[256];

    // Determine our status.
    if (!m_dropped && !m_docked) {
	// We're not docked or dropped.
	hsStdError(player,
		   "This vessel is not currently docked or landed.");
	return;
    }
    // Make sure we're not docking or dropping
    if (m_dock_status) {
	if (m_dock_status < 0)
	    hsStdError(player, HSCONF.ship_is_undocking);
	else
	    hsStdError(player, HSCONF.ship_is_docking);
	return;
    }
    if (m_drop_status) {
	if (m_drop_status > 0)
	    hsStdError(player,
		       "Vessel is currently descending to the surface.");
	else
	    hsStdError(player,
		       "Vessel is already lifting off from the surface.");
	return;
    }
    // Find the current landing location based on the
    // location of the ship object.
    if (m_objnum == NOTHING) {
	notify(player,
	       "Help!  I can't find the ship object for your vessel.");
	return;
    }

    dbRoom = Location_hspace(m_objnum);
    cLocation = dbHSDB.FindLandingLoc(dbRoom);
    if (!cLocation) {
	hsStdError(player, "You may not undock from this location.");
	return;
    }
    // Determine the owner of the landing location.
    cObj = cLocation->GetOwnerObject();
    if (!cObj) {
	notify(player,
	       "Help!  I can't figure out which planet/ship this landing location belongs to.");
	return;
    }
    // Make sure engines and computer are online.
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    cComputer = (CHSSysComputer *) m_systems.GetSystem(HSS_COMPUTER);

    if (!cEngines || !cComputer) {
	hsStdError(player,
		   "This vessel is either missing the engines or computer needed to take off.");
	return;
    }

    if (!cEngines->GetCurrentPower() || !cComputer->GetCurrentPower()) {
	hsStdError(player,
		   "Engines and computer systems must first be online.");
	return;
    }
    // We've got the location.
    // We've got the CHSObject that owns the location.
    // Figure out the type of object and undock/lift off.

    // See if the location we're docking on is a vessel and if so if bay is active.
    if (!cLocation->IsActive()) {
	if (cObj->GetType() == HST_SHIP) {
	    hsStdError(player, "The bay doors are closed.");
	    return;
	}
    }

    if (cObj->GetType() == HST_SHIP) {
	CHSShip *cShip;

	cShip = (CHSShip *) cObj;

	sprintf(tbuf, "The %s is undocking from this location.",
		GetName());
	cShip->HandleMessage(tbuf, MSG_SENSOR, (long *) this);


	// Takes 15 seconds to undock, so set that variable.
	m_dock_status = -15;
	sprintf(tbuf,
		"%s%s-%s Undocking .. systems check initiating ...",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	NotifyConsoles(tbuf, MSG_GENERAL);
    } else if (cObj->GetType() == HST_PLANET) {
	// Set the lift off time plus 5 seconds systems check
	m_drop_status = -(HSCONF.seconds_to_drop) - 5;
	sprintf(tbuf,
		"%s%s-%s Commencing lift off procedures ...",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	NotifyConsoles(tbuf, MSG_GENERAL);
	sprintf(tbuf,
		"Smoke begins to trickle from the lift rockets of the %s ...",
		GetName());
	hsInterface.NotifyContents(dbRoom, tbuf);
    } else {
	notify(player,
	       "What the ..?  This landing location isn't on a planet or ship.");
	return;
    }

    // We'll use the landing target variable to indicate
    // the location we're coming from.
    m_landing_target = cLocation;

}

// Handles the undocking/lift off continuation procedures such
// as systems checking, putting the ship into space, etc.
void CHSShip::HandleUndocking(void)
{
    char tbuf[256];
    double dVal;

    // Determine our status
    if (m_dock_status < 0) {
	m_dock_status++;

	switch (m_dock_status) {
	case -13:		// Hull check
	    dVal = GetMaxHullPoints();
	    dVal = 100 * (GetHullPoints() / dVal);
	    sprintf(tbuf,
		    "  %s%s-%s Hull at %.0f%% integrity ...",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, dVal);
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    break;

	case -10:		// Reactor check
	    CHSReactor * cReactor;
	    cReactor = (CHSReactor *) m_systems.GetSystem(HSS_REACTOR);
	    if (!cReactor)
		sprintf(tbuf,
			"  %s%s-%s Reactor not present .. ?",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    else {
		dVal = cReactor->GetMaximumOutput(FALSE);
		dVal = 100 * cReactor->GetOutput() / dVal;
		sprintf(tbuf,
			"  %s%s-%s Reactor online at %.0f%% power ...",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, dVal);
	    }
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    break;

	case -7:		// Life support check
	    CHSSysLifeSupport * cLife;

	    cLife =
		(CHSSysLifeSupport *) m_systems.
		GetSystem(HSS_LIFE_SUPPORT);
	    if (!cLife || !cLife->GetCurrentPower())
		sprintf(tbuf,
			"  %s%s%s*%s %s%sWARNING%s %s%s%s*%s Life support systems are not online.",
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL);
	    else
		sprintf(tbuf,
			"  %s%s-%s Life support systems -- online.",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    break;

	case -4:		// Engine status report
	    CHSSysEngines * cEngines;
	    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
	    if (!cEngines)
		sprintf(tbuf,
			"  %s%s-%s Engines not present .. ?",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    else {
		dVal = cEngines->GetOptimalPower(FALSE);
		dVal = 100 * cEngines->GetCurrentPower() / dVal;
		sprintf(tbuf,
			"  %s%s-%s Engines online at %.0f%% power ...",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, dVal);
	    }
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    break;

	case -2:		// Bay doors open
	    sprintf(tbuf,
		    "\nThe bay doors begin to slide open as the %s prepares for departure ...\n",
		    GetName());
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    sprintf(tbuf,
		    "The bay doors begin to slide open as the %s prerares for departure ...",
		    GetName());
	    hsInterface.NotifyContents(m_landing_target->objnum, tbuf);
	    break;

	case 0:
	    sprintf(tbuf,
		    "You feel a sudden lift as the %s glides forth from the docking bay.",
		    GetName());
	    NotifySrooms(tbuf);
	    sprintf(tbuf,
		    "The %s fires its engines as it departs through the docking bay doors.",
		    GetName());
	    hsInterface.NotifyContents(m_landing_target->objnum, tbuf);

	    // Find the object we're undocking from.
	    CHSObject *cUndockingFrom;
	    cUndockingFrom =
		dbHSDB.FindObjectByLandingLoc(m_landing_target->objnum);
	    CHSUniverse *uDest;
	    if (cUndockingFrom) {
		uDest = uaUniverses.FindUniverse(cUndockingFrom->GetUID());
		m_x = cUndockingFrom->GetX();
		m_y = cUndockingFrom->GetY();
		m_z = cUndockingFrom->GetZ();
		sprintf(tbuf,
			"In the distance, the %s undocks from the %s.",
			GetName(), cUndockingFrom->GetName());
		if (uDest)
		    uDest->SendContactMessage(tbuf, IDENTIFIED,
					      cUndockingFrom);
	    } else {
		NotifyConsoles
		    ("Unable to find source ship.  Putting you where you docked at.",
		     MSG_GENERAL);
		uDest = uaUniverses.FindUniverse(m_uid);
	    }
	    // Add us to the univeres's active list.
	    if (!uDest) {
		NotifyConsoles("Help!  I can't find your universe!",
			       MSG_GENERAL);
		return;
	    }

	    uDest->AddActiveObject(this);


	    // We're in space now
	    m_in_space = TRUE;
	    m_docked = FALSE;

	    // Move the ship object to the
	    // universe room.
	    MoveShipObject(uDest->GetID());
	    m_uid = uDest->GetID();

	    // Clear our location
	    m_objlocation = NOTHING;

	    // Add to the landing loc capacity
	    int iSize;
	    iSize = GetSize();
	    m_landing_target->DeductCapacity(-iSize);
	    break;
	}
    } else if (m_drop_status < 0) {
	m_drop_status++;

	int iLifeCheck;
	int iTestLifters;
	int iHalfWay;

	iHalfWay = (int) (-HSCONF.seconds_to_drop / 2.0);
	iLifeCheck = -HSCONF.seconds_to_drop - 4;
	iTestLifters = -HSCONF.seconds_to_drop - 2;
	if (m_drop_status == iLifeCheck) {
	    CHSSysLifeSupport *cLife;

	    // Check life support.
	    cLife =
		(CHSSysLifeSupport *) m_systems.
		GetSystem(HSS_LIFE_SUPPORT);
	    if (!cLife) {
		sprintf(tbuf,
			"  %s%s%s*%s %s%sWARNING%s %s%s%s*%s Life support systems non-existant!",
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL);
		NotifyConsoles(tbuf, MSG_GENERAL);
	    } else if (!cLife->GetCurrentPower()) {
		sprintf(tbuf,
			"  %s%s%s*%s %s%sWARNING%s %s%s%s*%s Life support systems are not online.",
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_YELLOW, ANSI_NORMAL,
			ANSI_HILITE, ANSI_BLINK, ANSI_YELLOW, ANSI_NORMAL);
		NotifyConsoles(tbuf, MSG_GENERAL);
	    } else {
		sprintf(tbuf,
			"  %s%s-%s Life support systems check - OK.",
			ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
		NotifyConsoles(tbuf, MSG_GENERAL);
	    }
	} else if (m_drop_status == iTestLifters) {
	    // Test lift rockets.
	    sprintf(tbuf,
		    "  %s%s-%s Testing lift rockets ...",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    NotifyConsoles(tbuf, MSG_GENERAL);

	    sprintf(tbuf,
		    "Flames spurt intermittently from the lift rockets of the %s ...",
		    GetName());
	    hsInterface.NotifyContents(m_landing_target->objnum, tbuf);
	} else if (m_drop_status == -HSCONF.seconds_to_drop) {
	    // LIFT OFF!
	    sprintf(tbuf,
		    "%s%s-%s Lift off procedures complete .. %d seconds to orbit.",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
		    HSCONF.seconds_to_drop);
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    NotifySrooms(HSCONF.lift_off);

	    sprintf(tbuf,
		    "The wind suddenly picks up as the %s fires its lift rockets and begins its climb upward.",
		    GetName());
	    hsInterface.NotifyContents(m_landing_target->objnum, tbuf);

	    // We're not dropped right now, but we're not in
	    // space.
	    m_dropped = FALSE;
	    m_in_space = FALSE;

	    // Clear our location
	    m_objlocation = NOTHING;

	    // Move the ship object to the
	    // universe room.
	    MoveShipObject(m_uid);
	} else if (m_drop_status == iHalfWay) {
	    sprintf(tbuf,
		    "%s%s-%s Orbit in %d seconds ...",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, -iHalfWay);
	    NotifyConsoles(tbuf, MSG_GENERAL);
	    sprintf(tbuf,
		    "The %s disappears in the sky above as it continues its climb into orbit.",
		    GetName());
	    hsInterface.NotifyContents(m_landing_target->objnum, tbuf);
	} else if (!m_drop_status) {
	    int iSize;

	    sprintf(tbuf,
		    "%s%s-%s Ship is now in orbit above the celestial surface.",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    NotifyConsoles(tbuf, MSG_GENERAL);

	    // Add us to the univeres's active list.
	    CHSUniverse *uDest;
	    uDest = uaUniverses.FindUniverse(m_uid);
	    if (!uDest) {
		NotifyConsoles("Help!  I can't find your universe!",
			       MSG_GENERAL);
		return;
	    }
	    uDest->AddActiveObject(this);
	    sprintf(tbuf,
		    "In the distance, the %s undocks from %s.",
		    GetName(),
		    m_landing_target->GetOwnerObject()->GetName());

	    if (uDest)
		uDest->SendContactMessage(tbuf, IDENTIFIED,
					  m_landing_target->
					  GetOwnerObject());

	    // We're in space now.
	    m_in_space = TRUE;


	    // Add to the landing loc capacity
	    iSize = GetSize();
	    m_landing_target->DeductCapacity(-iSize);
	}
    }
}

// Returns the current location where the ship is docked/dropped,
// or NOTHING if not applicable.
dbref CHSShip::GetDockedLocation(void)
{
    if (m_objnum == NOTHING)
	return NOTHING;

    if (m_dropped) {
	return Location_hspace(m_objnum);
    } else if (m_docked) {
	return Location_hspace(m_objnum);
    } else
	return NOTHING;
}

// Attempts to engage or disengage the afterburners.
// Set bStat to TRUE to engage, FALSE to disengage.
void CHSShip::EngageAfterburn(dbref player, BOOL bStat)
{
    CHSSysEngines *cEngines;

    // Look for the engines
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    if (!cEngines) {
	hsStdError(player, "This vessel has no engines.");
	return;
    }
    // Can't afterburn while not in space
    if (!m_in_space && bStat) {
	hsStdError(player,
		   "Unable to engage afterburners when not actively in space.");
	return;
    }
    // Can't afterburn when docking/dropping
    if (bStat && (m_drop_status || m_dock_status)) {
	hsStdError(player, "Unable to engage afterburners at this time.");
	return;
    }
    // Can the engines burn?
    if (!cEngines->CanBurn()) {
	hsStdError(player, "Our engines are not capable of afterburning.");
	return;
    }
    // Are engines online?
    if (!cEngines->GetCurrentPower()) {
	hsStdError(player, "Engines are not currently online.");
	return;
    }
    // Are the engines burning already?
    if (bStat && cEngines->GetAfterburning()) {
	hsStdError(player, "Afterburners already engaged.");
	return;
    } else if (!bStat && !cEngines->GetAfterburning()) {
	hsStdError(player, "The afterburners are not currently engaged.");
	return;
    }
    // Find our universe to give some messages
    CHSUniverse *uDest;
    char tbuf[256];

    uDest = uaUniverses.FindUniverse(m_uid);

    // What change do we want to make?
    if (bStat) {
	// Engage.
	if (!cEngines->SetAfterburn(TRUE))
	    hsStdError(player, "Failed to engage afterburners.");
	else {
	    sprintf(tbuf,
		    "%s%s-%s Afterburners engaged.",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    NotifyConsoles(tbuf, MSG_ENGINEERING);

	    NotifySrooms(HSCONF.afterburn_engage);

	    // Give some effects messages
	    if (uDest) {
		sprintf(tbuf,
			"Flames roar from the engines of the %s as it engages its afterburners.",
			GetName());
		uDest->SendContactMessage(tbuf, IDENTIFIED, this);
	    }
	}
    } else {
	// Disengage
	if (!cEngines->SetAfterburn(FALSE))
	    hsStdError(player, "Failed to disengage afterburners.");
	else {
	    sprintf(tbuf,
		    "%s%s-%s Afterburners disengaged.",
		    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	    NotifyConsoles(tbuf, MSG_ENGINEERING);

	    NotifySrooms(HSCONF.afterburn_disengage);

	    // Give some effects messages
	    if (uDest) {
		sprintf(tbuf,
			"The roaring flames from the engines of the %s cease as it disengages afterburners.",
			GetName());
		uDest->SendContactMessage(tbuf, IDENTIFIED, this);
	    }
	}
    }
}

// Attempts to engage or disengage the jump drive.
// Set bStat to TRUE to engage, FALSE to disengage.
void CHSShip::EngageJumpDrive(dbref player, BOOL bStat)
{
    CHSJumpDrive *cJumpers;
    CHSSysEngines *cEngines;
    char tbuf[128];

    // Look for the jumpers
    cJumpers = (CHSJumpDrive *) m_systems.GetSystem(HSS_JUMP_DRIVE);
    if (!cJumpers) {
	hsStdError(player, "This vessel has no jump drive.");
	return;
    }
    // Can't jump while not in space
    if (!m_in_space && bStat) {
	hsStdError(player,
		   "Unable to engage jump drive when not actively in space.");
	return;
    }
    // Can't jump when docking/dropping
    if (bStat && (m_drop_status || m_dock_status)) {
	hsStdError(player, "Unable to engage jump drive at this time.");
	return;
    }
    // Are jumpers online?
    if (!cJumpers->GetCurrentPower()) {
	hsStdError(player, "Jump drive is not currently online.");
	return;
    }
    // Are the jumpers engaged already?
    if (bStat && cJumpers->GetEngaged()) {
	hsStdError(player, "Jump drive already engaged.");
	return;
    } else if (!bStat && !cJumpers->GetEngaged()) {
	hsStdError(player, "The jump drive is not currently engaged.");
	return;
    }
    // What change do we want to make?
    if (bStat) {
	// Make sure we're at a good speed.
	cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
	if (!cEngines) {
	    hsStdError(player, "This vessel doesn't even have engines.");
	    return;
	}

	if (cEngines->GetCurrentSpeed() < cJumpers->GetMinJumpSpeed()) {
	    sprintf(tbuf,
		    "Minimum sublight speed of %d %s/hr required to engage jump drive.",
		    cJumpers->GetMinJumpSpeed(), HSCONF.unit_name);
	    hsStdError(player, tbuf);
	    return;
	}
	// Engage.
	if (!cJumpers->SetEngaged(TRUE))
	    hsStdError(player, "Failed to engage jump drive.");
	else {
	    // Put us in hyperspace
	    EnterHyperspace();
	}
    } else {
	// Disengage
	if (!cJumpers->SetEngaged(FALSE))
	    hsStdError(player, "Failed to disengage jump drive");
	else {
	    ExitHyperspace();
	}
    }
}

// Handles putting the ship into hyperspace, including effects
// messages.
void CHSShip::EnterHyperspace(void)
{
    char tbuf[128];
    CHSUniverse *uSource;

    if (m_hyperspace)
	return;

    m_hyperspace = TRUE;

    // Effects messages
    sprintf(tbuf,
	    "%s%s-%s Jump drive engaged.",
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
    NotifyConsoles(tbuf, MSG_ENGINEERING);
    NotifySrooms(HSCONF.ship_jumps);

    // Notify ships with us on sensors that
    // we've jumped.
    uSource = uaUniverses.FindUniverse(m_uid);
    if (uSource) {
	sprintf(tbuf,
		"You see a flash of blue light as the %s engages its jump drives.",
		GetName());
	uSource->SendContactMessage(tbuf, IDENTIFIED, this);
    }
}

// Handles taking the ship out of hyperspace, including effects
// messages.
void CHSShip::ExitHyperspace(void)
{
    CHSUniverse *uSource;
    char tbuf[128];

    if (!m_hyperspace)
	return;

    m_hyperspace = FALSE;

    sprintf(tbuf,
	    "%s%s-%s Jump drive disengaged.",
	    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
    NotifyConsoles(tbuf, MSG_ENGINEERING);
    NotifySrooms(HSCONF.end_jump);

    // Notify ships with us on sensors that
    // we've come out of hyperspace.
    uSource = uaUniverses.FindUniverse(m_uid);
    if (uSource) {
	sprintf(tbuf,
		"You see a flash of blue light as a vessel comes out of hyperspace.");
	uSource->SendMessage(tbuf, 1000, this);
    }
}

// Returns TRUE or FALSE for whether the ship is in hyperspace
// or not.
BOOL CHSShip::InHyperspace(void)
{
    return m_hyperspace;
}

// Allows a player to break a boarding link with a ship
// in the specified boarding link slot.
void CHSShip::DoBreakBoardLink(dbref player, int slot)
{
    // Decrement slot for array notation.
    slot--;

    CHSHatch *lHatch;

    lHatch = GetHatch(slot);
    // Link exists?
    if (!lHatch) {
	hsStdError(player, "No hatch with that notation found.");
	return;
    }
    int port;
    port = lHatch->m_targetHatch;

    CHSShip *cShip;

    cShip = (CHSShip *) dbHSDB.FindObject(lHatch->m_targetObj);

    if (!cShip) {
	hsStdError(player, "No vessel docked on that port.");
	return;
    }

    CHSHatch *cHatch;
    cHatch = cShip->GetHatch(port);
    if (!cHatch) {
	hsStdError(player, "No vessel docked on that port.");
	return;
    }

    if (lHatch->m_clamped == 1) {
	hsStdError(player, "Local hatch is clamped can't break link.");
	return;
    }

    if (cHatch->m_clamped == 1) {
	hsStdError(player, "Remote hatch is clamped can't break link.");
	return;
    }

    cHatch->m_targetObj = NOTHING;
    cHatch->m_targetHatch = NOTHING;
    lHatch->m_targetObj = NOTHING;
    lHatch->m_targetHatch = NOTHING;
    hsInterface.UnlinkExits(cHatch->objnum, lHatch->objnum);
    cHatch->
	HandleMessage(unsafe_tprintf
		      ("%s is disconnected.", Name(cHatch->objnum)),
		      MSG_GENERAL);
    lHatch->
	HandleMessage(unsafe_tprintf
		      ("%s is disconnected.", Name(lHatch->objnum)),
		      MSG_GENERAL);

    char tbuf[256];
    sprintf(tbuf, "The %s disengages docking couplings.", GetName());
    cShip->NotifyConsoles(tbuf, MSG_GENERAL);

    hsStdError(player, "Docking couplings disengaged.");
}

// Attempts to establish a boarding connection with
// another vessel.
void CHSShip::DoBoardLink(dbref player, int id, int lhatch, int dhatch)
{
    CHSJumpDrive *cJumpers;
    CHSSysEngines *cEngines;
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;
    CHSObject *cObj;
    CHSShip *cShip;
    char tbuf[128];


    // Look for the jumpers
    cJumpers = (CHSJumpDrive *) m_systems.GetSystem(HSS_JUMP_DRIVE);
    cEngines = (CHSSysEngines *) m_systems.GetSystem(HSS_ENGINES);
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);

    lhatch--;
    dhatch--;

    // Are they on sensors?
    cContact = NULL;
    if (cSensors) {
	cContact = cSensors->GetContactByID(id);
    }
    if (!cContact) {
	hsStdError(player, "No such contact id on sensors.");
	return;
    }
    cObj = cContact->m_obj;

    // Other object must be capable of board linking.
    if (cObj->GetType() != HST_SHIP) {
	hsStdError(player,
		   "You may not establish a boarding link with that.");
	return;
    }
    cShip = (CHSShip *) cObj;

    // Are the jumpers engaged?
    if (cJumpers && cJumpers->GetEngaged()) {
	hsStdError(player,
		   "Cannot make a boarding link while in hyperspace.");
	return;
    }
    // Are the afterburners engaged?
    if (cEngines && cEngines->GetAfterburning()) {
	hsStdError(player,
		   "Cannot make a boarding link while afterburning.");
	return;
    }

    // See if the distance is ok
    if (Dist3D(GetX(), GetY(), GetZ(),
	       cObj->GetX(), cObj->GetY(), cObj->GetZ()) >
	HSCONF.max_board_dist) {
	sprintf(tbuf,
		"Vessel must be within %d %s to establish boarding link.",
		HSCONF.max_board_dist, HSCONF.unit_name);
	hsStdError(player, tbuf);
	return;
    }

    CHSHatch *cHatch;
    cHatch = cShip->GetHatch(dhatch);
    if (!cHatch) {
	hsStdError(player, "No such hatch on target vessel.");
	return;
    }
    CHSHatch *lHatch;
    lHatch = GetHatch(lhatch);
    if (!lHatch) {
	hsStdError(player, "No such hatch on this vessel.");
	return;
    }
    if (cHatch->m_targetObj != NOTHING) {
	hsStdError(player, "Target hatch already taken.");
	return;
    }
    if (lHatch->m_targetObj != NOTHING) {
	hsStdError(player, "Local hatch already taken.");
	return;
    }
    if (lHatch->m_clamped == 1) {
	hsStdError(player,
		   "Local hatch is clamped can't estabilish link.");
	return;
    }

    if (cHatch->m_clamped == 1) {
	hsStdError(player,
		   "Remote hatch is clamped can't estabilish link.");
	return;
    }


    cHatch->m_targetObj = GetDbref();
    lHatch->m_targetObj = cShip->GetDbref();
    cHatch->m_targetHatch = lhatch;
    lHatch->m_targetHatch = dhatch;
    hsInterface.LinkExits(lHatch->objnum, cHatch->objnum);
    sprintf(tbuf, "%s connects with %s's %s.",
	    Name(lHatch->objnum), cShip->GetName(), Name(cHatch->objnum));
    lHatch->HandleMessage(tbuf, MSG_GENERAL);
    sprintf(tbuf, "%s connects with %s's %s.",
	    Name(cHatch->objnum), GetName(), Name(lHatch->objnum));
    cHatch->HandleMessage(tbuf, MSG_GENERAL);
    sprintf(tbuf,
	    "A loud clang is heard as docking couplings are engaged.");
    NotifySrooms(tbuf);
    cShip->NotifySrooms(tbuf);

    sprintf(tbuf, "The %s has engaged docking couplings on our hatch %d.",
	    GetName(), lhatch);
    cShip->NotifyConsoles(tbuf, MSG_GENERAL);

    hsStdError(player, "Hatches connected.");
}

// Scans a target ID on sensors and gives the player a 
// scan report.
void CHSShip::ScanObjectID(dbref player, int id)
{
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;

    // Do we have sensors?
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
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
    // Scan the object
    if (cContact->status == IDENTIFIED)
	cContact->m_obj->GiveScanReport(this, player, TRUE);
    else
	cContact->m_obj->GiveScanReport(this, player, FALSE);
}

// Displays the description of a target vessel.
void CHSShip::ViewObjectID(dbref player, int id)
{
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;

    // Do we have sensors?
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
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
    // Show the object
    if (cContact->status == IDENTIFIED)
	if (hsInterface.AtrGet(cContact->m_obj->GetDbref(), "DESCRIBE"))
	    notify(player,
		   unsafe_tprintf("%s",
				  hsInterface.EvalExpression(hsInterface.
							     m_buffer,
							     cContact->
							     m_obj->
							     GetDbref(),
							     m_objnum,
							     m_objnum)));
	else
	    notify(player, "No description set on that object.");
    else {
	hsStdError(player, "Contact outside visual range.");
	return;
    }
}

void CHSShip::GateObjectID(dbref player, int id)
{
    CHSSysSensors *cSensors;
    SENSOR_CONTACT *cContact;

    // Do we have sensors?
    cSensors = (CHSSysSensors *) m_systems.GetSystem(HSS_SENSORS);
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
	(GetX(), GetY(), GetZ(), cObj->GetX(), cObj->GetY(),
	 cObj->GetZ()) > 2) {
	hsStdError(player, "You must be within 5 hm to gate an object.");
	return;
    }

    if (cObj->GetType() != HST_WORMHOLE) {
	hsStdError(player, "You cannot gate that type of object.");
	return;
    }

    if (cObj->GetType() == HST_WORMHOLE) {
	CHSWormHole *cWorm;

	cWorm = (CHSWormHole *) cObj;

	if (cWorm->GetSize() / 2 < GetSize()) {
	    hsStdError(player,
		       "This ship is too large to gate the wormhole.");
	    return;
	}

	cWorm->GateShip(this);
    } else {
	hsStdError(player, "Error, cannot gate target.");
    }

}

// Attempts to engage or disengage the cloaking device.
// Set bStat to TRUE to engage, FALSE to disengage.
void CHSShip::EngageCloak(dbref player, BOOL bStat)
{
    CHSSysCloak *cCloak;
    char tbuf[128];

    // Look for the cloaking device.
    cCloak = (CHSSysCloak *) m_systems.GetSystem(HSS_CLOAK);
    if (!cCloak) {
	hsStdError(player, "This vessel has no cloaking device.");
	return;
    }
    // Can't jump while not in space
    if (!m_in_space && bStat) {
	hsStdError(player,
		   "Unable to engage cloaking device when not actively in space.");
	return;
    }
    // Can't jump when docking/dropping
    if (bStat && (m_drop_status || m_dock_status)) {
	hsStdError(player,
		   "Unable to engage cloaking device at this time.");
	return;
    }
    // Are the jumpers engaged already?
    if (bStat && (cCloak->GetEngaged() || cCloak->IsEngaging())) {
	hsStdError(player, "Cloaking Device already engaged.");
	return;
    } else if (!bStat && !cCloak->GetEngaged()) {
	hsStdError(player,
		   "The cloaking device is not currently engaged.");
	return;
    }
    // What change do we want to make?
    if (bStat) {
	CHSUniverse *uSource;
	cCloak->SetEngaged(TRUE);

	sprintf(tbuf,
		"%s%s-%s Cloaking Device engaged.",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	NotifyConsoles(tbuf, MSG_ENGINEERING);
	// Notify ships with us on sensors that
	// we've decloaked.
	uSource = uaUniverses.FindUniverse(m_uid);
	if (uSource) {
	    sprintf(tbuf,
		    "A vessel slowly shifts out of view as it engages its cloaking device.");
	    uSource->SendMessage(tbuf, 1000, this);
	}
    } else {
	CHSUniverse *uSource;
	cCloak->SetEngaged(FALSE);
	sprintf(tbuf,
		"%s%s-%s Cloaking Device disengaged.",
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL);
	NotifyConsoles(tbuf, MSG_ENGINEERING);
	// Notify ships with us on sensors that
	// we've decloaked.
	uSource = uaUniverses.FindUniverse(m_uid);
	if (uSource) {
	    sprintf(tbuf,
		    "A vessel slowly shifts into view as it disengages its cloaking device.");
	    uSource->SendMessage(tbuf, 1000, this);
	}
    }
}

void CHSShip::GiveHatchRep(dbref player)
{
    int idx, hatches;

    hatches = 0;
    CHSHatch *cHatch;
    for (idx = 0; idx < MAX_HATCHES; idx++) {
	cHatch = m_hatches[idx];

	if (cHatch)
	    hatches++;
    }

    if (hatches < 1) {
	hsStdError(player, "This vessel has no hatches.");
	return;
    }


    char tbuf[512];
    // Give the header info
    sprintf(tbuf,
	    "%s%s.----------------------------------------------------------.%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s Hatch Status Report      %30s  %s%s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL, GetName(),
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s >--------------------------------------------------------<%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);
    sprintf(tbuf,
	    "%s%s|%s-%s#%s%s-       - %sShip Linked %s%s-                 - %sRemote Port %s%s- %s|%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE,
	    ANSI_GREEN, ANSI_NORMAL, ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
	    ANSI_HILITE, ANSI_GREEN, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);


    char tbuf2[256], tbuf3[256];


    for (idx = 1; idx < hatches + 1; idx++) {
	cHatch = m_hatches[idx - 1];

	CHSShip *cShip;
	cShip = dbHSDB.FindShip(cHatch->m_targetObj);
	if (cShip) {
	    sprintf(tbuf2, "%s", cShip->GetName());
	    sprintf(tbuf3, "%i", cHatch->m_targetHatch + 1);
	} else {
	    sprintf(tbuf2, "Unconnected");
	    sprintf(tbuf3, "Unconnected");
	}

	sprintf(tbuf, "%s%s|%s[%s%i%s%s]%s  %-36s  %11s    %s%s|%s",
		ANSI_HILITE, ANSI_BLUE, ANSI_GREEN, ANSI_NORMAL, idx,
		ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, tbuf2, tbuf3,
		ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
	notify(player, tbuf);
    }

    sprintf(tbuf,
	    "%s%s`----------------------------------------------------------'%s",
	    ANSI_HILITE, ANSI_BLUE, ANSI_NORMAL);
    notify(player, tbuf);

}

