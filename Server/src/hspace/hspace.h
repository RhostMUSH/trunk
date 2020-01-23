#ifndef __HSPACE_INCLUDED__
#define __HSPACE_INCLUDED__

#include "hstypes.h"

// Message definitions, used when notifying consoles, etc.
enum HS_MESSAGE
{
	MSG_GENERAL = 0,
	MSG_SENSOR,
	MSG_ENGINEERING,
	MSG_COMBAT,
	MSG_COMMUNICATION
};

// The CHSpace Object is the master object for all of HSpace.
// It rules all!!!
class CHSpace
{
public:
	CHSpace(void);

	// Methods
	void InitSpace(int rebooting = 0);
	void DumpDatabases(void);
	void DoCommand(char *, int, char *, char *, dbref);
	char *DoFunction(char *, dbref, char **);
	void DoCycle(void);
	void SetCycle(BOOL);

	double CycleTime(void);

	BOOL Active(void);

private:

	// Methods
	BOOL LoadConfigFile(char *);

	//Attributes
	BOOL m_cycling;
	BOOL m_attrs_need_cleaning;

	double m_cycle_ms;
};

extern CHSpace HSpace;

#endif // __HSPACE_INCLUDED__
