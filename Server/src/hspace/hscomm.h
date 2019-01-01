#ifndef __HSCOMM_INCLUDED__
#define __HSCOMM_INCLUDED__

#include "hsobjects.h"

// The structure that contains communications information
typedef struct
{
	CHSObject *cObj;	// Object that sent the message.
	dbref dbSource;		// Source dbref object
	UINT suid;			// Source uid of message
	UINT duid;			// Destination uid of message.
	double frq;
	double sX, sY, sZ;	// Source coords;
	double dMaxDist;	// Max dist to send the message.
	char *msg;	// Ptr to the message to send.
}HSCOMM;

// The class that handles all communications message relay
// in HSpace.  All messages are sent to this thing to be
// relayed to objects in the game.
class CHSCommRelay
{
public:
	CHSCommRelay(void);

	BOOL RelayMessage(HSCOMM *);
private:
	BOOL OnFrq(int, double);
	void RelayCommlinks(HSCOMM *);
};

extern CHSCommRelay cmRelay;

#endif // __HSCOMM_INCLUDED__
