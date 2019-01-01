#ifndef __HSMISSILE_INCLUDED__
#define __HSMISSILE_INCLUDED__

#include "hsweapon.h"
#include "hsobjects.h"

// A missile that gets fired from a missile tube
class CHSMissile : public CHSObject, public CHSWeapon
{
public:
	CHSMissile(void);

	void DoCycle(void);
	void SetTargetObject(CHSObject *);
	void SetSourceObject(CHSObject *);
	void SetSourceConsole(CHSConsole *);
	void HandleDamage(CHSObject *, CHSWeapon *, int, CHSConsole *, int);
	char *GetObjectColor(void);
	char GetObjectCharacter(void);
	void SetHeading(int, int);  // Sets the current heading of the missile
	
protected:
	void CalculateTimeLeft(void);
	void HitTarget(void);
	void Expire(void);
	void ChangeHeading(void);
	void MoveTowardTarget(void);

	// Attributes
	CHSObject *m_source_object;
	CHSObject *m_target;
	CHSConsole *m_source_console;

	BOOL m_delete_me;  // Internal use.
	BOOL m_target_hit; // Flag to mark if target is hit

	int m_timeleft;
	int m_xyheading;
	int m_zheading;
};


// CHSMTube is a strange name for a weapons class, but that's what it
// really is .. a missile tube, not a missile.  Missiles are stored
// in the ship's ammunition storage, and each missile tube just loads
// and fires them.
class CHSMTube : public CHSWeapon
{
public:
	CHSMTube(void);

	// Methods
	UINT GetReloadTime(void);
	UINT GetRange(void);
	UINT GetStrength(void);
	UINT GetTurnRate(void);
	UINT GetSpeed(void);

	int  Reloading(void);
	int  GetStatusInt(void);
	char *GetName(void);
	char *GetStatus(void);
	char *GetAttrInfo(void);
	void SetMissileBay(CHSMissileBay *);
	void DoCycle(void);
	void SetStatus(int);
	void AttackObject(CHSObject *, CHSObject *, CHSConsole *, int);

	BOOL RequiresLock(void);
	BOOL IsReady(void);
	BOOL CanAttackObject(CHSObject *cObj);
	BOOL Reload(void);
	BOOL Configurable(void);
	BOOL Configure(int);
	BOOL Loadable(void);
	BOOL Unload(void);
protected:

	// Attributes
	UINT m_time_to_reload;
	BOOL m_loaded;
	BOOL m_reloading;

	CHSMissileBay *m_missile_bay; // Source of missiles
};


#endif // __HSMISSILE_INCLUDED__
