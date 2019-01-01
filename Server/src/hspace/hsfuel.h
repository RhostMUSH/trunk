#ifndef __HSFUEL_INCLUDED__
#define __HSFUEL_INCLUDED__

#include "hstypes.h"
#include "hseng.h"

// Types of fuel.
enum HS_FUELTYPE
{
	FL_BURNABLE = 0,
	FL_REACTABLE,
};

// This is derived an engineering system,
// even though it is quite different.
//
// The CHSFuelSystem handles fuel storage, transfer,
// etc.
class CHSFuelSystem : public CHSEngSystem
{
public:
	CHSFuelSystem(void);

	int GetMaxFuel(HS_FUELTYPE type=FL_BURNABLE);
	float ExtractFuelUnit(float, HS_FUELTYPE type=FL_BURNABLE);
	float GetFuelRemaining(HS_FUELTYPE type=FL_BURNABLE);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	void SaveToFile(FILE *);
	void GiveSystemInfo(int);

	CHSEngSystem *Dup(void);

protected:
	float m_burnable_fuel;
	float m_reactable_fuel;

	// Overridables
	int *m_max_burnable_fuel;
	int *m_max_reactable_fuel;
};

#endif // __HSFUEL_INCLUDED__
