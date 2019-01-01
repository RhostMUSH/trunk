#ifndef __HSUNIVERSE_INCLUDED__
#define __HSUNIVERSE_INCLUDED__

#include "hsobjects.h"
#include "hscelestial.h"

#define HS_MAX_OBJECTS			400
#define HS_MAX_ACTIVE_OBJECTS		150	
#define HS_MAX_UNIVERSES		10

class CHSUniverse
{
public:
	CHSUniverse(void);
	~CHSUniverse(void);

	char *GetName(void);

	UINT GetID(void);
	UINT NumObjects(HS_TYPE type = HST_NOTYPE);
	UINT NumActiveObjects(HS_TYPE type = HST_NOTYPE);

	void RemoveObject(CHSObject *);
	void RemoveActiveObject(CHSObject *);
	void SendMessage(char *, int, CHSObject *);
	void SendContactMessage(char *, int, CHSObject *);
	void SetID(int);
 	void SetName(char *);
	void SaveToFile(FILE *);

	BOOL AddActiveObject(CHSObject *);
	BOOL AddObject(CHSObject *);
	BOOL IsEmpty(void);
	BOOL SetAttributeValue(char *, char *);

	// This function can't be named GetObject(), or it will
	// conflict with something stupid in Windows.
	CHSObject *GetUnivObject(UINT, HS_TYPE tType = HST_NOTYPE);
	CHSObject *GetActiveUnivObject(UINT);

	CHSObject *FindObject(dbref, HS_TYPE type = HST_NOTYPE);

protected:
	// Member Functions
	void RemoveShip(CHSShip *);
	void RemoveCelestial(CHSCelestial *);

	BOOL AddCelestial(CHSCelestial *);
	BOOL AddShip(CHSShip *);

	CHSCelestial *FindCelestial(dbref);
	CHSCelestial *GetCelestial(UINT);
	CHSShip		 *FindShip(dbref);
	CHSShip		 *GetShip(UINT);

	// Attributes
	dbref	m_objnum;
	char	*m_name;

	UINT	m_nCelestials;
	UINT	m_nShips;
	UINT    m_nObjects;

	// Storage arrays
	CHSObject **m_objlist;
	CHSObject **m_active_objlist;
	CHSShip **m_shiplist;
	CHSCelestial **m_celestials;
};

class CHSUniverseArray
{
public:
	CHSUniverseArray(void);
	~CHSUniverseArray(void);

	// Public Methods
	CHSUniverse *NewUniverse(void);
	CHSUniverse *GetUniverse(UINT); // Gets a universe in a slot
	CHSUniverse *FindUniverse(dbref); // Finds a universe by ID

	UINT NumUniverses(void);
	void SaveToFile(char *);
	void PrintInfo(int);
	BOOL AddUniverse(CHSUniverse *);
	BOOL DeleteUniverse(UINT);
	BOOL LoadFromFile(char *);

protected:
	CHSUniverse **m_universes;
	UINT m_num_universes;

};

extern CHSUniverseArray uaUniverses;

#endif // __UNIVERSE_INCLUDED__
