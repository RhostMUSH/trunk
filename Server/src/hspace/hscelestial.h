#ifndef __HSCELESTIAL_INCLUDED__
#define __HSCELESTIAL_INCLUDED__

#include "hsobjects.h"

// The CHSCelestial object is derived from the base object class and
// is used to derive other types of celestial objects.
class CHSCelestial : public CHSObject
{
public:
	CHSCelestial(void);
	virtual ~CHSCelestial(void);

	// Methods
	virtual void WriteToFile(FILE *);
	virtual char *GetAttributeValue(char *);
	virtual BOOL SetAttributeValue(char *, char *);
	virtual void ClearObjectAttrs(void);
	virtual char *GetObjectColor(void);
	virtual char GetObjectCharacter(void);

	UINT GetType(void);

protected:
	virtual BOOL HandleKey(int, char *, FILE *fp = NULL);
};


// The CHSPlanet object is derived from the celestial object
// to handle functions and information specific to planets.
class CHSPlanet : public CHSCelestial
{
public:
	CHSPlanet(void);
	~CHSPlanet(void);

	BOOL AddLandingLoc(dbref);
	void ClearObjectAttrs(void);
	void WriteToFile(FILE *);
	void GiveScanReport(CHSObject *, dbref, BOOL);
	char *GetObjectColor(void);
	char GetObjectCharacter(void);
	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);

	CHSLandingLoc *FindLandingLoc(dbref);
	CHSLandingLoc *GetLandingLoc(int);

protected:
	CHSLandingLoc *NewLandingLoc(void);
	BOOL HandleKey(int, char *, FILE *);

	// Attributes
	CHSLandingLoc *m_landing_locs[HS_MAX_LANDING_LOCS];
};

class CHSNebula : public CHSCelestial
{
public:
	CHSNebula(void);
	~CHSNebula(void);

	void WriteToFile(FILE *);
	void ClearObjectAttrs(void);
	void GiveScanReport(CHSObject *, dbref, BOOL);
	char GetObjectCharacter(void);
	char *GetObjectColor(void);
	char *GetAttributeValue(char *);
	int GetDensity(void);
	float GetShieldaff(void);
	BOOL SetAttributeValue(char *, char *);


protected:
	BOOL HandleKey(int, char *, FILE *);

	// Attributes
	int	m_density;
	float m_shieldaff;
};

class CHSAsteroid : public CHSCelestial
{
public:
	CHSAsteroid(void);
	~CHSAsteroid(void);

	void DoCycle(void);
	void WriteToFile(FILE *);
	void ClearObjectAttrs(void);
	void GiveScanReport(CHSObject *, dbref, BOOL);
	char GetObjectCharacter(void);
	char *GetObjectColor(void);
	char *GetAttributeValue(char *);
	int GetDensity(void);
	BOOL SetAttributeValue(char *, char *);


protected:
	BOOL HandleKey(int, char *, FILE *);

	// Attributes
	int	m_density;
};

class CHSBlackHole : public CHSCelestial
{
public:
	CHSBlackHole(void);
	~CHSBlackHole(void);

	void DoCycle(void);
	char GetObjectCharacter(void);
	char *GetObjectColor(void);

protected:
};

class CHSWormHole : public CHSCelestial
{
public:
	CHSWormHole(void);
	~CHSWormHole(void);

	void DoCycle(void);
	void GateShip(CHSShip *);
	void GiveScanReport(CHSObject *, dbref, BOOL);
	void ClearObjectAttrs(void);
	void WriteToFile(FILE *);
	char *GetAttributeValue(char *);
	char GetObjectCharacter(void);
	char *GetObjectColor(void);
	float GetFluctuation(void);
	float GetStability(void);
	float GetBaseStability(void);
	BOOL SetAttributeValue(char *, char *);

protected:
	BOOL HandleKey(int, char *, FILE *);

	double m_destx;
	double m_desty;
	double m_destz;
	UINT m_destuid;
	float m_fluctuation;
	float m_stability;
	float m_basestability;
};
#endif // __HSCELESTIAL_INCLUDED__
