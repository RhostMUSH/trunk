#include "hscopyright.h"
#include "hstypes.h"
#include "hsfuncs.h"
#include "hsinterface.h"
#include "hsobjects.h"
#include "hsdb.h"
#include "hsutils.h"
#include "hsuniverse.h"
#include "hspace.h"
#include "hseng.h"
#include "hscomm.h"

#ifdef TM3
#include "powers.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

HSPACE_FUNC_PROTO(hsfGetMissile)
    HSPACE_FUNC_PROTO(hsfGetAttr)
    HSPACE_FUNC_PROTO(hsfCalcXYAngle)
    HSPACE_FUNC_PROTO(hsfCalcZAngle)
    HSPACE_FUNC_PROTO(hsfSpaceMsg)
    HSPACE_FUNC_PROTO(hsfGetEngSystems)
    HSPACE_FUNC_PROTO(hsfSetAttr)
    HSPACE_FUNC_PROTO(hsfGetSensorContacts)
    HSPACE_FUNC_PROTO(hsfAddWeapon)
    HSPACE_FUNC_PROTO(hsfWeaponAttr)
    HSPACE_FUNC_PROTO(hsfSysAttr)
    HSPACE_FUNC_PROTO(hsfSysSet)
    HSPACE_FUNC_PROTO(hsfDelSys)
    HSPACE_FUNC_PROTO(hsfAddSys)
    HSPACE_FUNC_PROTO(hsfCommMsg)
    HSPACE_FUNC_PROTO(hsfSetMissile)
    HSPACE_FUNC_PROTO(hsfClone)

HSFUNC hs_func_list[] = {
    "ADDSYS", hsfAddSys,
    "ADDWEAPON", hsfAddWeapon,
    "CLONE", hsfClone,
    "COMMMSG", hsfCommMsg,
    "DELSYS", hsfDelSys,
    "GETATTR", hsfGetAttr,
    "GETENGSYSTEMS", hsfGetEngSystems,
    "GETMISSILE", hsfGetMissile,
    "SENSORCONTACTS", hsfGetSensorContacts,
    "SETATTR", hsfSetAttr,
    "SETMISSILE", hsfSetMissile,
    "SPACEMSG", hsfSpaceMsg,
    "SYSATTR", hsfSysAttr,
    "SYSSET", hsfSysSet,
    "WEAPONATTR", hsfWeaponAttr,
    "XYANG", hsfCalcXYAngle,
    "ZANG", hsfCalcZAngle,
    NULL
};

// Finds a function with a given name.
HSFUNC *hsFindFunction(char *name)
{
    HSFUNC *ptr;
    for (ptr = hs_func_list; ptr->name; ptr++) {
	// Straight comparison of names
	if (!strcasecmp(name, ptr->name))
	    return ptr;
    }

    return NULL;
}

// Gets or sets an attribute on one of the globally loaded database
// weapons.
HSPACE_FUNC_HDR(hsfWeaponAttr)
{
    int iWclass;
    char arg_left[32];		// left of colon
    char arg_right[32];		// right of colon

    // Executor must be a wizard
    if (!Wizard(executor))
	return "#-1 Permission denied.";


    // Find a colon (:) in the attrname parameter.  If one
    // exists, this is a set operation as opposed to get.
    *arg_left = *arg_right = '\0';
    char *ptr;
    ptr = strchr(args[1], ':');
    if (ptr) {
	// Split args[1] in half at the colon.  Copy the left
	// side into arg_left and the right side into .. arg_right.
	*ptr = '\0';
	strncpy(arg_left, args[1], 31);
	arg_left[31] = '\0';

	// Increment ptr and copy arg_right stuff.
	ptr++;
	strncpy(arg_right, ptr, 31);
	arg_right[31] = '\0';
    } else {
	// This is a GET operation.  Args[1] is an attrname and
	// no more.
	strncpy(arg_left, args[1], 31);
	arg_left[31] = '\0';
    }

    // Determine weapon class #
    iWclass = atoi(args[0]);

    // See if it's a valid weapon.
    if (!waWeapons.GoodWeapon(iWclass))
	return "#-1 Bad weapon identifier.";

    CHSDBWeapon *cWeap;
    cWeap = waWeapons.GetWeapon(iWclass, HSW_NOTYPE);
    if (!cWeap)
	return "#-1 Failed to retrieve specified weapon object.";

    // If this is a GET, then just ask the weapon for the
    // info.
    if (!*arg_right) {
	char *ptr;
	ptr = cWeap->GetAttributeValue(arg_left);

	if (!ptr)
	    return "#-1 Attribute not found.";
	else
	    return ptr;
    } else {
	if (!cWeap->SetAttributeValue(arg_left, arg_right))
	    return "#-1 Failed to set attribute.";
	else
	    return "Weapon attribute - Set.";
    }
}

HSPACE_FUNC_HDR(hsfAddWeapon)
{
    dbref dbObj;

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    // Executor must control the object
    if (!controls(executor, dbObj)) {
	return "#-1 Permission denied.";
    }

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }
    // If object is not a console, don't do it.
    if (!hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE)) {
	return "#-1 Target object must be a ship console.";
    }
    // Find the console object
    CHSConsole *cConsole;

    cConsole = dbHSDB.FindConsole(dbObj);
    if (!cConsole)
	return "#-1 Invalid HSpace console.";

    // Tell the console to add the weapon
    if (!cConsole->AddWeapon(executor, atoi(args[1])))
	return "#-1 Failed to add weapon.";
    else
	return "Weapon - Added.";
}

HSPACE_FUNC_HDR(hsfCalcXYAngle)
{
    double x1, y1;
    double x2, y2;

    // Grab coordinate input values
    x1 = atof(args[0]);
    y1 = atof(args[1]);
    x2 = atof(args[2]);
    y2 = atof(args[3]);

    static char rval[8];

    sprintf(rval, "%d", XYAngle(x1, y1, x2, y2));
    return rval;
}

HSPACE_FUNC_HDR(hsfCalcZAngle)
{
    double x1, y1, z1;
    double x2, y2, z2;

    // Grab coordinate input values
    x1 = atof(args[0]);
    y1 = atof(args[1]);
    z1 = atof(args[2]);
    x2 = atof(args[3]);
    y2 = atof(args[4]);
    z2 = atof(args[5]);

    static char rval[8];

    sprintf(rval, "%d", ZAngle(x1, y1, z1, x2, y2, z2));
    return rval;
}

HSPACE_FUNC_HDR(hsfGetMissile)
{
    dbref dbObj;
    static char rval[32];

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }
    // Determine type of object
    if (!hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_OBJECT))
	return "#-1 Not an HSpace object.";

    CHSObject *cObj;

    cObj = dbHSDB.FindObject(dbObj);
    if (!cObj)
	return "#-1 Invalid HSpace object.";

    if (cObj->GetType() != HST_SHIP)
	return "#-1 Not an HSpace ship.";


    CHSShip *cShip;
    cShip = (CHSShip *) cObj;

    // Grab the missile bay from the ship
    CHSMissileBay *mBay;
    mBay = cShip->GetMissileBay();

    // Ask the missile bay for the maximum and current values.
    UINT max;
    UINT left;
    int iClass;
    iClass = atoi(args[1]);
    max = mBay->GetMaxMissiles(iClass);
    left = mBay->GetMissilesLeft(iClass);
    sprintf(rval, "%d/%d", left, max);
    return rval;
}

HSPACE_FUNC_HDR(hsfGetAttr)
{
    dbref dbObj;
    char *ptr;

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }
    // Determine type of object
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_OBJECT)) {
	CHSObject *cObj;

	cObj = dbHSDB.FindObject(dbObj);
	if (!cObj)
	    return "#-1 Invalid HSpace object.";

	// Disable access to some attrs
	if (!strcasecmp(args[1], "BOARDING CODE") &&
	    !See_All(executor) && !controls(executor, dbObj)) {
	    return "#-1 Permission Denied.";
	}
	ptr = cObj->GetAttributeValue(args[1]);
    } else if (hsInterface.
	       HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE)) {
	CHSConsole *cConsole;

	cConsole = dbHSDB.FindConsole(dbObj);
	if (!cConsole)
	    return "#-1 Invalid HSpace console.";

	// Disable access to some attrs
	if (!strcasecmp(args[1], "BOARDING CODE") &&
	    !See_All(executor) && !controls(executor, dbObj)) {
	    return "#-1 Permission Denied.";
	}
	// Try to get the attribute from the console.
	ptr = cConsole->GetAttributeValue(args[1]);

	// If attribute not found, try the ship object
	// the console belongs to.
	if (!ptr) {
	    CHSObject *cObj;
	    cObj = dbHSDB.FindObjectByConsole(dbObj);
	    if (cObj)
		ptr = cObj->GetAttributeValue(args[1]);
	}
    } else if (hsInterface.
	       HasFlag(dbObj, TYPE_ROOM, ROOM_HSPACE_LANDINGLOC)) {
	CHSLandingLoc *cLoc;

	cLoc = dbHSDB.FindLandingLoc(dbObj);
	if (!cLoc)
	    return "#-1 Invalid HSpace landing location.";

	ptr = cLoc->GetAttributeValue(args[1]);
    } else if (hsInterface.HasFlag(dbObj, TYPE_EXIT, EXIT_HSPACE_HATCH)) {
	CHSHatch *cHatch;

	cHatch = dbHSDB.FindHatch(dbObj);
	if (!cHatch)
	    return "#-1 Invalid HSpace hatch.";

	ptr = cHatch->GetAttributeValue(args[1]);
    } else
	return "#-1 Not an HSpace object.";

    if (!ptr)
	return "#-1 Attribute not found.";

    return ptr;
}

HSPACE_FUNC_HDR(hsfSetAttr)
{
    dbref dbObj;
    char *ptr, *dptr;
    char strName[32];
    char strAttr[32];

    // The object/attribute pair is specified by a slash
    // in between.  Separate them.
    dptr = strName;
    for (ptr = args[0]; *ptr; ptr++) {
	if ((dptr - strName) > 31)
	    break;

	if (*ptr == '/')
	    break;

	*dptr++ = *ptr;
    }
    *dptr = '\0';

    if (!*ptr)
	return "#-1 Invalid object/attribute pair specified.";

    dptr = strAttr;
    ptr++;
    while (*ptr) {
	if ((dptr - strAttr) > 31)
	    break;

	*dptr++ = *ptr++;
    }
    *dptr = '\0';

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, strName);

    if (dbObj == NOTHING)
	return "#-1";

    // Executor must control the object
    if (!controls(executor, dbObj)) {
	return "#-1 Permission denied.";
    }

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }
    // Determine type of object
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_OBJECT)) {
	CHSObject *cObj;

	cObj = dbHSDB.FindObject(dbObj);
	if (!cObj)
	    return "#-1 Invalid HSpace object.";

	if (cObj->SetAttributeValue(strAttr, args[1]))
	    return "Attribute - set.";
	else
	    return "Attribute - failed.";
    } else if (hsInterface.
	       HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE)) {
	CHSConsole *cConsole;

	cConsole = dbHSDB.FindConsole(dbObj);
	if (!cConsole)
	    return "#-1 Invalid HSpace console.";

	// Try to get the attribute from the console.
	if (cConsole->SetAttributeValue(strAttr, args[1]))
	    return "Attribute - set.";
	else {
	    // If attribute not found, try the ship object
	    // the console belongs to.
	    CHSObject *cObj;
	    cObj = dbHSDB.FindObjectByConsole(dbObj);
	    if (cObj) {
		if (cObj->SetAttributeValue(strAttr, args[1])) {
		    static char rbuf[32];
		    sprintf(rbuf, "%s Attribute - set.", cObj->GetName());
		    return rbuf;
		} else
		    return "Attribute - failed.";
	    } else
		return "Attribute - failed.";
	}
    } else if (hsInterface.
	       HasFlag(dbObj, TYPE_ROOM, ROOM_HSPACE_LANDINGLOC)) {
	CHSLandingLoc *cLoc;

	cLoc = dbHSDB.FindLandingLoc(dbObj);
	if (!cLoc)
	    return "#-1 Invalid HSpace landing location.";

	if (cLoc->SetAttributeValue(strAttr, args[1]))
	    return "Attribute - set.";
	else
	    return "Attribute - failed.";
    } else if (hsInterface.HasFlag(dbObj, TYPE_EXIT, EXIT_HSPACE_HATCH)) {
	CHSHatch *cHatch;

	cHatch = dbHSDB.FindHatch(dbObj);
	if (!cHatch)
	    return "#-1 Invalid HSpace hatch.";

	if (cHatch->SetAttributeValue(strAttr, args[1]))
	    return "Attribute - set.";
	else
	    return "Attribute - failed.";
    } else
	return "#-1 Not an HSpace object.";
}

// Sends a message into space by 1 of 3 possible ways.
HSPACE_FUNC_HDR(hsfSpaceMsg)
{
    CHSObject *cObj;

    // Check flags on the object to see if it's an
    // actual object or maybe a console.
    if (hsInterface.HasFlag(executor, TYPE_THING, THING_HSPACE_OBJECT))
	cObj = dbHSDB.FindObject(executor);
    else if (hsInterface.HasFlag(executor, TYPE_THING,
				 THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(executor);
    else
	cObj = NULL;
    if (!Wizard(executor) && !cObj) {
	return "#-1 Permission denied.";
    }

    // Methods of sending:
    //
    // 0 = Blind message sent to all ships within a specified
    //     distance of the given coordinates.
    // 1 = Message sent to all ships who have the source object
    //     on sensors.
    // 2 = Message sent to all ships who have the source object
    //     identified on sensors.
    int how = atoi(args[0]);
    CHSUniverse *uDest;
    int i;
    CHSObject *cTarget;
    switch (how) {
    case 0:
	if (!args[1] || !isdigit(*args[1]))
	    return "#-1 Invalid universe ID specified.";

	int uid;
	uid = atoi(args[1]);
	uDest = uaUniverses.FindUniverse(uid);
	if (!uDest)
	    return "#-1 Invalid universe ID specified.";


	if (!args[2] || !isdigit(*args[2]) ||
	    !args[3] || !isdigit(*args[3]) ||
	    !args[4] || !isdigit(*args[4]))
	    return "#-1 Invalid coordinate parameters supplied.";

	double sX, sY, sZ;	// Source coords
	sX = atof(args[2]);
	sY = atof(args[3]);
	sZ = atof(args[4]);

	if (!args[5] || !isdigit(*args[5]))
	    return "#-1 Invalid distance parameter value.";

	double dMaxDist;
	dMaxDist = atof(args[5]);
	double tX, tY, tZ;	// Target object coords

	if (!args[6])
	    return "#-1 No message to send.";

	double dDistance;
	for (i = 0; i < HS_MAX_ACTIVE_OBJECTS; i++) {
	    cTarget = uDest->GetActiveUnivObject(i);
	    if (!cTarget)
		continue;

	    tX = cTarget->GetX();
	    tY = cTarget->GetY();
	    tZ = cTarget->GetZ();

	    dDistance = Dist3D(sX, sY, sZ, tX, tY, tZ);

	    if (dDistance > dMaxDist)
		continue;

	    cTarget->HandleMessage(args[6], MSG_GENERAL, NULL);
	}
	break;

    case 1:
    case 2:
	if (!cObj)
	    return "#-1 You are not a HSpace object.";

	// Grab the source universe
	uDest = uaUniverses.FindUniverse(cObj->GetUID());
	if (!uDest)
	    return "#-1 Object universe could not be found.";

	CHSSysSensors *cSensors;
	SENSOR_CONTACT *cContact;
	for (i = 0; i < HS_MAX_ACTIVE_OBJECTS; i++) {
	    cTarget = uDest->GetActiveUnivObject(i);
	    if (!cTarget)
		continue;

	    cSensors =
		(CHSSysSensors *) cTarget->GetEngSystem(HSS_SENSORS);
	    if (!cSensors)
		continue;

	    cContact = cSensors->GetContact(cObj);
	    if (!cContact)
		continue;

	    if (cContact->status == DETECTED && how == 1)
		cTarget->HandleMessage(args[1], MSG_SENSOR, (long *) cObj);
	    else if (cContact->status == IDENTIFIED && how == 2)
		cTarget->HandleMessage(args[1], MSG_SENSOR, (long *) cObj);
	}
	break;

    default:
	return "#-1 Invalid sending method specified.";
    }
    return "Message sent.";
}

// Returns a comma delimited list of engineering systems
// located on the object.
HSPACE_FUNC_HDR(hsfGetEngSystems)
{
    CHSObject *cObj;
    dbref dbObj;

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }

    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_OBJECT))
	cObj = dbHSDB.FindObject(dbObj);
    else if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = NULL;

    if (!cObj)
	return "#-1 Target object is not an HSpace object.";

    CHSShip *dObj;

    if (cObj->GetType() != HST_SHIP)
	return "#-1 Target object does not have systems.";
    else
	dObj = (CHSShip *) cObj;

    // We've got the list, now translate the types into
    // names, and put them in the return buffer.
    static char rbuf[512];
    char *ptr;
    CHSEngSystem *cSys;
    char tbuf[32];

    *rbuf = '\0';

    for (cSys = dObj->m_systems.GetHead(); cSys; cSys = cSys->GetNext()) {
	// Find the system name
	if (cSys->GetType() != HSS_FICTIONAL)
	    ptr = hsGetEngSystemName(cSys->GetType());
	else
	    ptr = cSys->GetName();

	if (!*rbuf)
	    sprintf(tbuf, "%s", ptr ? ptr : "Unknown");
	else
	    sprintf(tbuf, ",%s", ptr ? ptr : "Unknown");

	strcat(rbuf, tbuf);
    }
    return rbuf;
}

HSPACE_FUNC_HDR(hsfGetSensorContacts)
{
    dbref dbObj;
    int oType;			// Type of object to report

    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);
    if (dbObj == NOTHING)
	return "#-1 No object found.";

    CHSObject *cObj;
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);

    if (!cObj)
	return "#-1 Not a HSpace object.";

    // Grab the sensors from the object
    CHSSysSensors *cSensors;
    cSensors = (CHSSysSensors *) cObj->GetEngSystem(HSS_SENSORS);
    if (!cSensors)
	return "#-1 Object has no sensors.";

    int nContacts;
    nContacts = cSensors->NumContacts();
    if (!nContacts)
	return "";

    oType = atoi(args[1]);
    int idx;
    SENSOR_CONTACT *cContact;
    CHSObject *cTarget;
    static char rbuf[1024];
    char tbuf[128];
    *rbuf = '\0';

    double sX, sY, sZ;		// Source coords
    double tX, tY, tZ;		// Contact coords

    sX = cObj->GetX();
    sY = cObj->GetY();
    sZ = cObj->GetZ();
    for (idx = 0; idx < HS_MAX_CONTACTS; idx++) {
	// Grab the next contact
	cContact = cSensors->GetContact(idx);
	if (!cContact)
	    continue;

	// Does the type match what is requested?
	cTarget = cContact->m_obj;
	if (oType != HST_NOTYPE && (cTarget->GetType() != oType))
	    continue;

	CHSShip *cShip;
	cShip = NULL;
	if (cTarget->GetType() == HST_SHIP)
	    cShip = (CHSShip *) cTarget;


	// Grab target coords
	tX = cTarget->GetX();
	tY = cTarget->GetY();
	tZ = cTarget->GetZ();

	// Print out object info
	if (!*rbuf) {
	    if (!cShip)
		sprintf(tbuf, "%d:#%d:%d:%d:%s:%d:%d:%.4f",
			cContact->m_id,
			cTarget->GetDbref(),
			cTarget->GetType(),
			cTarget->GetSize(),
			cContact->status ==
			DETECTED ? "Unknown" : cTarget->GetName(),
			XYAngle(sX, sY, tX, tY), ZAngle(sX, sY, sZ, tX, tY,
							tZ), Dist3D(sX, sY,
								    sZ, tX,
								    tY,
								    tZ));
	    else
		sprintf(tbuf, "%d:#%d:%d:%d:%s:%d:%d:%.4f:%d:%d:%d",
			cContact->m_id,
			cTarget->GetDbref(),
			cTarget->GetType(),
			cTarget->GetSize(),
			cContact->status ==
			DETECTED ? "Unknown" : cTarget->GetName(),
			XYAngle(sX, sY, tX, tY), ZAngle(sX, sY, sZ, tX, tY,
							tZ), Dist3D(sX, sY,
								    sZ, tX,
								    tY,
								    tZ),
			cShip->GetXYHeading(), cShip->GetZHeading(),
			cShip->GetSpeed());
	} else {
	    if (!cShip)
		sprintf(tbuf, "|%d:#%d:%d:%d:%s:%d:%d:%.4f",
			cContact->m_id,
			cTarget->GetDbref(),
			cTarget->GetType(),
			cTarget->GetSize(),
			cContact->status ==
			DETECTED ? "Unknown" : cTarget->GetName(),
			XYAngle(sX, sY, tX, tY), ZAngle(sX, sY, sZ, tX, tY,
							tZ), Dist3D(sX, sY,
								    sZ, tX,
								    tY,
								    tZ));
	    else
		sprintf(tbuf, "|%d:#%d:%d:%d:%s:%d:%d:%.4f:%d:%d:%d",
			cContact->m_id,
			cTarget->GetDbref(),
			cTarget->GetType(),
			cTarget->GetSize(),
			cContact->status ==
			DETECTED ? "Unknown" : cTarget->GetName(),
			XYAngle(sX, sY, tX, tY), ZAngle(sX, sY, sZ, tX, tY,
							tZ), Dist3D(sX, sY,
								    sZ, tX,
								    tY,
								    tZ),
			cShip->GetXYHeading(), cShip->GetZHeading(),
			cShip->GetSpeed());
	}

	strcat(rbuf, tbuf);
    }

    return rbuf;
}

HSPACE_FUNC_HDR(hsfSysAttr)
{
    dbref dbObj;
    char *ptr;
    char lpstrAttr[32];

    // Args[0] should be obj/attr, so drop a null in the 
    // split.
    ptr = strchr(args[0], '/');
    if (!ptr)
	return "#-1 Invalid object/system combination.";

    *ptr = '\0';
    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    if (!nearby(executor, dbObj) && !See_All(executor)) {
	notify(executor, "I don't see that here.");
	return "#-1";
    }

    CHSObject *cObj;
    // If it's a console, search for the object based on
    // the console.
    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);

    if (!cObj)
	return "#-1 Invalid HSpace object.";


    // The rest of the string (after the /) should be
    // the attribute name.
    ptr++;
    strncpy(lpstrAttr, ptr, 31);
    lpstrAttr[31] = '\0';

    // Determine what type of system is being queried.
    HSS_TYPE type;
    CHSEngSystem *cSys;
    type = hsGetEngSystemType(lpstrAttr);
    if (type == HSS_NOTYPE && cObj->GetType() == HST_SHIP
	|| type == HSS_FICTIONAL && cObj->GetType() == HST_SHIP) {
	CHSShip *dObj;
	dObj = (CHSShip *) cObj;
	cSys = dObj->m_systems.GetSystemByName(lpstrAttr);
	if (!cSys)
	    return "#-1 Invalid system specified.";
    } else if (type != HSS_NOTYPE) {
	cSys = cObj->GetEngSystem(type);
    } else
	return "#-1 Invalid system specified.";

    // Ask the object for the system.
    if (!cSys)
	return "#-1 System not present.";

    ptr = cSys->GetAttributeValue(args[1], atoi(args[2]));
    if (!ptr)
	return "#-1 Attribute not found.";

    return ptr;
}


HSPACE_FUNC_HDR(hsfSysSet)
{
    dbref dbObj;
    char *ptr;
    char strSysName[32];

    // The object/attribute pair is specified by a slash
    // in between.  Separate them.
    ptr = strchr(args[0], '/');
    if (!ptr)
	return "#-1 Invalid object/system pair specified.";

    *ptr = '\0';

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    // Executor must control the object
    if (!controls(executor, dbObj)) {
	return "#-1 Permission denied.";
    }

    CHSObject *cObj;

    cObj = dbHSDB.FindObject(dbObj);
    if (!cObj)
	return "#-1 Invalid HSpace object.";

    // The rest of the string (after the /) should be
    // the system name.
    ptr++;
    strncpy(strSysName, ptr, 31);
    strSysName[31] = '\0';

    // Determine what type of system is being queried.
    HSS_TYPE type;
    CHSEngSystem *cSys;
    type = hsGetEngSystemType(strSysName);
    if (type == HSS_NOTYPE && cObj->GetType() == HST_SHIP
	|| type == HSS_FICTIONAL && cObj->GetType() == HST_SHIP) {
	CHSShip *dObj;
	dObj = (CHSShip *) cObj;
	cSys = dObj->m_systems.GetSystemByName(strSysName);
	if (!cSys)
	    return "#-1 Invalid system specified.";
    } else if (type != HSS_NOTYPE) {
	cSys = cObj->GetEngSystem(type);
    } else
	return "#-1 Invalid system specified.";

    if (!cSys)
	return "#-1 System not present.";

    // Now set the attribute value
    char strAttr[32];
    char strValue[32];

    // Look for a colon separating the attr:value pair.  It's
    // ok to have an empty value.
    ptr = strchr(args[1], ':');
    if (!ptr) {
	strncpy(strAttr, args[1], 31);
	strAttr[31] = '\0';
	strValue[0] = '\0';
    } else {
	*ptr = '\0';
	strncpy(strAttr, args[1], 31);
	strAttr[31] = '\0';

	ptr++;
	strncpy(strValue, ptr, 31);
	strValue[31] = '\0';
    }

    // Tell the system to set the attribute
    if (!cSys->SetAttributeValue(strAttr, strValue))
	return "Attribute - failed.";
    else
	return "Attribute - set.";
}

HSPACE_FUNC_HDR(hsfCommMsg)
{
    // See if the executor is a COMM console
    if (!hsInterface.HasFlag(executor, TYPE_THING, THING_HSPACE_COMM)
	&& !hsInterface.HasFlag(executor, TYPE_THING,
				THING_HSPACE_CONSOLE))
	return
	    "#-1 Must have HSPACE_COMM or HSPACE_CONSOLE flag to do that.";

    // Grab an HSCOMM struct and fill in the info.
    HSCOMM commdata;

    // Try to find a source HSpace obj based on the executor.
    CHSObject *cObj;
    cObj = dbHSDB.FindObjectByConsole(executor);
    if (cObj) {
	commdata.cObj = cObj;
	commdata.dbSource = cObj->GetDbref();
    } else {
	commdata.cObj = NULL;
	commdata.dbSource = executor;
    }

    // Source UID
    commdata.suid = atoi(args[0]);

    // Dest UID
    commdata.duid = atoi(args[1]);

    // Source coords
    commdata.sX = atoi(args[2]);
    commdata.sY = atoi(args[3]);
    commdata.sZ = atoi(args[4]);

    // Max range
    commdata.dMaxDist = atoi(args[5]);

    // Frequency
    commdata.frq = atof(args[6]);

    // Message
    commdata.msg = args[7];

    if (cmRelay.RelayMessage(&commdata))
	return "Sent.";
    else
	return "Failed.";
}

HSPACE_FUNC_HDR(hsfSetMissile)
{
    dbref dbObj;
    char *ptr;

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    // Executor must control the object
    if (!controls(executor, dbObj)) {
	return "#-1 Permission denied.";
    }

    CHSObject *cObj;

    cObj = dbHSDB.FindObject(dbObj);
    if (!cObj)
	return "#-1 Invalid HSpace object.";

    // The object must be a ship.
    if (cObj->GetType() != HST_SHIP)
	return "#-1 Not an HSpace ship.";

    CHSShip *cShip;
    cShip = (CHSShip *) cObj;
    if (cShip->SetNumMissiles(executor, atoi(args[1]), atoi(args[2])))
	return "Value - set.";
    else
	return "Value - failed.";
}

HSPACE_FUNC_HDR(hsfClone)
{
    dbref dbObj;

    // Find the object in question
    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);

    if (dbObj == NOTHING)
	return "#-1";

    // Executor must be wizard
    if (!Wizard(executor)) {
	return "#-1 Permission denied.";
    }

    CHSObject *cObj;

    cObj = dbHSDB.FindObject(dbObj);
    if (!cObj)
	return "#-1 Invalid HSpace object.";

    // The object must be a ship.
    if (cObj->GetType() != HST_SHIP)
	return "#-1 Not an HSpace ship.";

    CHSShip *cShip;
    cShip = (CHSShip *) cObj;

    // Clone the ship.
    dbObj = cShip->Clone();

    static char tbuf[64];
    sprintf(tbuf, "#%d", dbObj);
    return tbuf;
}

HSPACE_FUNC_HDR(hsfDelSys)
{
    dbref dbObj;
    CHSShip *pShip;
    CHSObject *cObj;
    CHSEngSystem *cSys;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/sysset obj:system/attr=value
    if (!args[0] || !args[1])
	return "You must specify an object and system name.";

    // Pull out the object of interest

    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);
    if (dbObj == NOTHING)
	return "#-1";

    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);


    if (!cObj)
	return "#-1 That thing does not appear to be an HSpace object.";


    // Currently only support ships
    if (cObj->GetType() != HST_SHIP)
	return
	    "#-1 That HSpace object does not currently support that operation.";

    pShip = (CHSShip *) cObj;

    // Find the system based on the name
    HSS_TYPE type;

    type = hsGetEngSystemType(args[1]);
    if (type == HSS_NOTYPE || type == HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystemByName(args[1]);
	if (!cSys) {
	    return "#-1 Invalid system name specified.";
	} else {
	    type = HSS_FICTIONAL;
	}
    }
    // See if the system is at the ship.

    if (type != HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystem(type);
	if (!cSys) {
	    return "#-1 That system does not exists for that ship.";
	}
    }


    char tmp[64];
    sprintf(tmp, "%s", cSys->GetName());

    // Delete the system
    if (pShip->m_systems.DelSystem(cSys))
	return unsafe_tprintf("%s system deleted from the %s.", tmp,
			      pShip->GetName());
    else
	return "Failed to delete system.";
}

HSPACE_FUNC_HDR(hsfAddSys)
{
    dbref dbObj;
    CHSShip *pShip;
    CHSObject *cObj;

    // Parse out the parts of the command.
    // Command format is:
    //
    // @space/sysset obj:system/attr=value
    if (!args[0] || !args[1])
	return "#-1 You must specify an object and system name.";

    // Pull out the object of interest

    dbObj = hsInterface.NoisyMatchThing(executor, args[0]);
    if (dbObj == NOTHING)
	return "#-1";

    // Find the system based on the name
    HSS_TYPE type;

    type = hsGetEngSystemType(args[1]);
    if (type == HSS_NOTYPE)
	return "#-1 Invalid system name specified.";

    if (hsInterface.HasFlag(dbObj, TYPE_THING, THING_HSPACE_CONSOLE))
	cObj = dbHSDB.FindObjectByConsole(dbObj);
    else
	cObj = dbHSDB.FindObject(dbObj);

    if (!cObj)
	return "#-1 That thing does not appear to be an HSpace object.";

    // Currently only support ships
    if (cObj->GetType() != HST_SHIP)
	return
	    "#-1 That HSpace object does not currently support that operation.";


    pShip = (CHSShip *) cObj;

    // Try to find the system already on the class
    CHSEngSystem *cSys;

    if (type != HSS_FICTIONAL) {
	cSys = pShip->m_systems.GetSystem(type);
	if (cSys)
	    return "#-1 That system already exists for that ship.";
    }
    // Add the system
    cSys = pShip->m_systems.NewSystem(type);
    if (!cSys)
	return "#-1 Failed to add the system to the specified ship.";


    pShip->m_systems.AddSystem(cSys);
    return unsafe_tprintf("%s system added to the %s.", cSys->GetName(),
			  pShip->GetName());
}
