#ifndef __HSDB_INCLUDED__
#define __HSDB_INCLUDED__

#define HSPACE_VERSION	"HSpace v4.2"

#include "hstypes.h"
#include "hsobjects.h"
#include "hscelestial.h"

class CHSDB
{
public:
	CHSDB(void);
	BOOL LoadDatabases(void);
	void DumpDatabases(void);
	UINT NumWeapons(void);
	UINT NumUniverses(void);
	void CreateNewUniverse(dbref, char *);
	void DeleteUniverse(dbref, char *);
	void CleanupDBAttrs(void);

	CHSObject *FindObjectByConsole(dbref);
	CHSObject *FindObject(int);
	CHSObject *FindObjectByLandingLoc(int);
	CHSObject *FindObjectByRoom(int);
	CHSLandingLoc *FindLandingLoc(int);
	CHSCelestial *FindCelestial(int);
	CHSShip *FindShip(int);
	CHSConsole *FindConsole(int);
	CHSHatch *FindHatch(int);
	UINT m_nWeapons;

protected:
	//Loading
	BOOL LoadUniverseDB(char *);
	BOOL LoadClassDB(char *);
	BOOL LoadWeaponDB(char *);
	BOOL LoadObjectDB(void);
	BOOL LoadTerritoryDB(char *);

	// Saving
	void DumpUniverseDB(char *);
	void DumpClassDB(char *);
	void DumpWeaponDB(char *);
	void DumpObjectDB(void);
	void DumpTerritoryDB(char *);

	// Misc
	CHSObject *FindRoomOwner(dbref, int *);

	// Attributes
	UINT m_nUniverses;
	UINT m_nSystems;
};

extern 	CHSDB dbHSDB;

#endif // __HSDB_INCLUDED__
