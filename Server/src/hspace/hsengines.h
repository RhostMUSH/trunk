#ifndef __HSENGINES_INCLUDED__
#define __HSENGINES_INCLUDED__

#include "hsfuel.h"

// A system specifically for engines, derived from
// the base CHSEngSystem class.  The engines aren't like
// car engines.  They basically handle velocity.
class CHSSysEngines : public CHSEngSystem
{
public:
	CHSSysEngines(void);
	~CHSSysEngines(void);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	BOOL SetDesiredSpeed(int);
	BOOL CanBurn(void);
	BOOL SetAfterburn(BOOL);
	BOOL GetAfterburning(void);
	UINT GetMaxVelocity(BOOL bAdjusted = TRUE);
	UINT GetAcceleration(BOOL bAdjusted = TRUE);
	double GetDesiredSpeed(void);
	double GetCurrentSpeed(void);
	int  GetEfficiency(void);
	void SetFuelSource(CHSFuelSystem *);
	void SaveToFile(FILE *);
	void ConsumeFuelBySpeed(int);
	void DoCycle(void);
	void PowerUp(int);
	void CutPower(int);
	void GiveSystemInfo(int);

	CHSEngSystem *Dup(void);
protected:
	void ChangeSpeed(void);

	// Source of fuel
	CHSFuelSystem *m_fuel_source;

	double m_current_speed;
	double m_desired_speed;

	BOOL m_afterburning;

	// Overridable variables at the ship level
	int	*m_max_velocity;
	int *m_acceleration;
	int *m_efficiency; // Thousands of distance units per fuel unit
	BOOL *m_can_afterburn;
};

// Hyperspace jump drives.  They're basically like the
// generic CHSEngSystem with some enhancements.
class CHSJumpDrive : public CHSEngSystem
{
public:
	CHSJumpDrive(void);

	char *GetAttributeValue(char *, BOOL);
	BOOL SetAttributeValue(char *, char *);
	BOOL GetEngaged(void);
	BOOL SetEngaged(BOOL);
	int  GetMinJumpSpeed(void);
	int  GetChargePerc(void);
	int  GetEfficiency(void);
	char *GetStatus(void);
	void SaveToFile(FILE *);
	void DoCycle(void);
	void ConsumeFuelBySpeed(int);
	void SetFuelSource(CHSFuelSystem *);
	void SetSublightSpeed(int);
	void GiveSystemInfo(int);
	void CutPower(int);

	double GetChargeRate(BOOL bAdjust = TRUE);

	CHSEngSystem *Dup(void);

protected:
	// Source of fuel
	CHSFuelSystem *m_fuel_source;

	BOOL   m_engaged;
	int	*m_min_jump_speed; // Overridable at the ship level
	int	*m_efficiency; // Thousands of distance units per fuel unit
	double m_charge_level;
	int  m_sublight_speed; // Set this to indicate fuel consumption
};

#endif // __HSENGINES_INCLUDED__
