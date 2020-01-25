#ifndef __HSTERRITORY_INCLUDED__
#define __HSTERRITORY_INCLUDED__

#include "hstypes.h"
#include "hsobjects.h"

#define HS_MAX_TERRITORIES	20

enum TERRTYPE
{
	T_RADIAL = 0,
	T_CUBIC,
};

// A base territory class, from which other types of
// territories can be derived.
class CHSTerritory
{
public:
	CHSTerritory(void);

	void SetDbref(int);
	int GetDbref(void);
	int GetUID(void);
	TERRTYPE GetType(void);
	
	// Overridables
        virtual ~CHSTerritory() = default;
	virtual void SaveToFile(FILE *);
	virtual BOOL SetAttributeValue(char *, char *);
	virtual BOOL PtInTerritory(int, double, double, double);
protected:
	int m_uid;	// Universe territory is in
	int m_objnum; // Object in the game that represents this territory
	TERRTYPE m_type;
};

// A radial territory that defines a sphere of ownership
class CHSRadialTerritory : public CHSTerritory
{
public:
	CHSRadialTerritory(void);

	BOOL SetAttributeValue(char *, char *);
	BOOL PtInTerritory(int, double, double, double);

	int GetX(void);
	int GetY(void);
	int GetZ(void);
	int GetRadius(void);

	void SaveToFile(FILE *);

protected:
	int m_cx;	// Center coords
	int m_cy;
	int m_cz;

	UINT m_radius;
};

// A cubic territory that defines a cube of ownership
class CHSCubicTerritory : public CHSTerritory
{
public:
	CHSCubicTerritory(void);

	BOOL SetAttributeValue(char *, char *);
	BOOL PtInTerritory(int, double, double, double);

	int GetMinX(void);
	int GetMinY(void);
	int GetMinZ(void);
	int GetMaxX(void);
	int GetMaxY(void);
	int GetMaxZ(void);

	void SaveToFile(FILE *);
protected:
	int m_minx;	// Bounding coords.
	int m_miny;
	int m_minz;
	int m_maxx;
	int m_maxy;
	int m_maxz;
};

// The CHSTerritoryArray stores a list of all territories in
// the game and provides that information to whomever
// requests it.
class CHSTerritoryArray
{
public:
	CHSTerritoryArray(void);

	CHSTerritory *NewTerritory(int, TERRTYPE type = T_RADIAL);
	CHSTerritory *InTerritory(CHSObject *cObj);
	CHSTerritory *FindTerritory(int);

	UINT NumTerritories(void);
	BOOL LoadFromFile(const char *);
	BOOL RemoveTerritory(int);

	void SaveToFile(const char *);
	void PrintInfo(int);
protected:
	CHSTerritory *m_territories[HS_MAX_TERRITORIES];
};

extern CHSTerritoryArray taTerritories;

#endif // __HSTERRITORY_INCLUDED__
