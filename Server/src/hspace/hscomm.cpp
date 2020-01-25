#include "hscomm.h"
#include "hsuniverse.h"
#include "hspace.h"
#include "hsconf.h"
#include "hsinterface.h"
#include "hsutils.h"
#include "hsdb.h"
#include <stdlib.h>
#include <string.h>
#include "ansi.h"

CHSCommRelay cmRelay;		// Our global relay object.

// Default constructor
CHSCommRelay::CHSCommRelay(void)
{

}

// Relays a comm message to all commlinks in the game.
void CHSCommRelay::RelayCommlinks(HSCOMM * commdata)
{
    if (!HSCONF.use_comm_objects)
	return;

    int idx;
    //int uid;
    int tX, tY, tZ;
    char *s;
    char strFrq[16];
    char strDbref[16];

    // Setup some strings to pass into the COMM_HANDLER.
    sprintf(strDbref, "#%d", commdata->dbSource);
    sprintf(strFrq, "%.2f", commdata->frq);

    for (idx = 0; idx < hsInterface.MaxObjects(); idx++) {
	if (!hsInterface.HasFlag(idx, TYPE_THING,
				 THING_HSPACE_COMM) ||
	    hsInterface.HasFlag(idx, TYPE_THING, THING_HSPACE_CONSOLE))
	    continue;

	// Check attributes
	//if (!hsInterface.AtrGet(idx, "UID"))
	//    continue;
	//uid = atoi(hsInterface.m_buffer);

	// Get coordinates
	if (!hsInterface.AtrGet(idx, "X"))
	    continue;
	s = hsInterface.EvalExpression(hsInterface.m_buffer, idx,
				       idx, idx);
	tX = atoi(s);

	if (!hsInterface.AtrGet(idx, "Y"))
	    continue;
	s = hsInterface.EvalExpression(hsInterface.m_buffer, idx,
				       idx, idx);
	tY = atoi(s);

	if (!hsInterface.AtrGet(idx, "Z"))
	    continue;
	s = hsInterface.EvalExpression(hsInterface.m_buffer, idx,
				       idx, idx);
	tZ = atoi(s);

	// Target within range?
	double dDistance;
	dDistance = Dist3D(commdata->sX, commdata->sY,
			   commdata->sZ, tX, tY, tZ);
	if (dDistance > commdata->dMaxDist)
	    continue;

	if (!OnFrq(idx, commdata->frq))
	    continue;

	// Call the object's handler function.
	if (!hsInterface.AtrGet(idx, "COMM_HANDLER"))
	    continue;

// FIXME
#ifdef OSOS
	wenv[0] = commdata->msg;
	wenv[1] = strFrq;
	wenv[2] = strDbref;
#endif
#ifdef TM3
	numwenv = 3;
#endif
	hsInterface.EvalExpression(hsInterface.m_buffer, idx, idx, idx);
    }
}

// Relays a message throughout the game givin the input data.
BOOL CHSCommRelay::RelayMessage(HSCOMM * commdata)
{
    // Find the destination universe for the message
    CHSUniverse *uDest;
    uDest = uaUniverses.FindUniverse(commdata->duid);
    if (!uDest)
	return FALSE;


    // Find all objects in the destination universe that
    // are within range, and relay the message to them.
    CHSObject *cObj;
    int idx;
    double dX, dY, dZ;		// Destination object coords
    double dDist;		// Distance between coords

    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	cObj = uDest->GetUnivObject(idx);
	if (!cObj)
	    continue;

	if (cObj->GetType() != HST_SHIP)
	    continue;

	CHSShip *cShip;

	cShip = (CHSShip *) cObj;

	CHSSysJammer *cJammer;

	cJammer = (CHSSysJammer *) cShip->GetEngSystem(HSS_JAMMER);

	if (!cJammer)
	    continue;

	dX = cObj->GetX();
	dY = cObj->GetY();
	dZ = cObj->GetZ();

	// Calculate squared distance between objects.  Don't
	// bother doing sqrt, cause it's slow as a snail!
	dDist = ((commdata->sX - dX) * (commdata->sX - dX) +
		 (commdata->sY - dY) * (commdata->sY - dY) +
		 (commdata->sZ - dZ) * (commdata->sZ - dZ));

	if (dDist < cJammer->GetRange(TRUE)) {
	    CHSShip *tShip;
	    tShip = (CHSShip *) dbHSDB.FindObject(commdata->dbSource);
	    if (tShip)
		tShip->
		    NotifyConsoles(unsafe_tprintf
				   ("%s%s-%s You are being jammed, source bearing %d mark %d.",
				    ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL,
				    XYAngle(dX, dY, commdata->sX,
					    commdata->sY), ZAngle(dX, dY,
								  dZ,
								  commdata->
								  sX,
								  commdata->
								  sY,
								  commdata->
								  sZ)),
				   MSG_GENERAL);

	    return FALSE;
	}
    }

    // Relay to commlinks
    RelayCommlinks(commdata);

    double dMaxDist;		// Squared max distance to target objs
    dMaxDist = commdata->dMaxDist * commdata->dMaxDist;

    for (idx = 0; idx < HS_MAX_OBJECTS; idx++) {
	cObj = uDest->GetUnivObject(idx);
	if (!cObj)
	    continue;

	dX = cObj->GetX();
	dY = cObj->GetY();
	dZ = cObj->GetZ();

	// Calculate squared distance between objects.  Don't
	// bother doing sqrt, cause it's slow as a snail!
	dDist = ((commdata->sX - dX) * (commdata->sX - dX) +
		 (commdata->sY - dY) * (commdata->sY - dY) +
		 (commdata->sZ - dZ) * (commdata->sZ - dZ));

	if (dDist > dMaxDist)
	    continue;
	// Give the object the message.
	cObj->HandleMessage(commdata->msg, MSG_COMMUNICATION,
			    (long *) commdata);
    }
    return TRUE;
}

// Indicates whether the commlink object is on a given frequency.
BOOL CHSCommRelay::OnFrq(int obj, double frq)
{
    // Check for the COMM_FRQS attr.
    if (!hsInterface.AtrGet(obj, "COMM_FRQS"))
	return FALSE;

    // Separate the string into blocks of spaces
    char *ptr;
    char *bptr;			// Points to the first char in the frq string
    double cfrq;
    ptr = strchr(hsInterface.m_buffer, ' ');
    bptr = hsInterface.m_buffer;
    while (ptr) {
	*ptr = '\0';
	cfrq = atof(bptr);

	// Do we have a match, Johnny?
	if (cfrq == frq)
	    return TRUE;

	// Find the next frq in the string.
	bptr = ptr + 1;
	ptr = strchr(bptr, ' ');
    }
    // Last frq in the string.
    cfrq = atof(bptr);
    if (cfrq == frq)
	return TRUE;

    // No match!
    return FALSE;
}
