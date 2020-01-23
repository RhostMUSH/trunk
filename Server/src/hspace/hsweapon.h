#ifndef __HSWEAPON_INCLUDED__
#define __HSWEAPON_INCLUDED__

#include <stdio.h>
#include "hstypes.h"


//
// To implement a new type of weapon:
//
// Derive a weapon from the generic CHSDBWeapon.  These are stored
// in the global CHSWeaponDBArray and contain the information loaded
// from the weapon database.
//
// Modify the CHSWeaponDBArray to implement a Create.. function
// to handle the creation of your weapon.  Use CreateLaser() or
// CreateMissile() as examples.
//
// Derive a console weapon class from CHSWeapon, which is the
// weapon object that gets added to a console.  This is different
// from CHSDBWeapon because it has a more diverse interface, whereas
// CHSDBWeapon simply contains database information for the weapon.
// Administrators can add multiple CHSWeapon's to consoles.
//
// Overload the necessary functions to make the weapon work as
// you want.
#define HS_MAX_WEAPONS		50
#define HS_WEAPON_NAME_LEN	32

#define HSW_NOTYPE		999
#define HSW_LASER		0
#define HSW_MISSILE		1

enum WEAPSTAT_CHANGE
{
	STAT_NOCHANGE = 0,
	STAT_READY
};

class CHSConsole;

// A generic weapon in the weapons database.  From this, we'll
// derive many other types of weapons.
class CHSDBWeapon
{
public:
	CHSDBWeapon(void);

	// Methods
	virtual char *GetAttributeValue(char *);
	virtual BOOL SetAttributeValue(char *, char *);

	UINT m_type;
	char m_name[HS_WEAPON_NAME_LEN];
};

// A derived weapon type: LASER
class CHSDBLaser : public CHSDBWeapon
{
public:
	CHSDBLaser(void);

	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);

	UINT	m_regen_time;
	UINT	m_range;
	UINT	m_strength;
	UINT	m_accuracy;
	UINT	m_powerusage;
	BOOL	m_nohull;
};

class CHSDBMissile : public CHSDBWeapon
{
public:
	CHSDBMissile(void);

	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);

	UINT	m_reload_time;
	UINT	m_range;
	UINT	m_strength;
	UINT	m_turnrate;
	UINT	m_speed;
};

// A missile storage slot in the missile bay
typedef struct
{
	UINT m_maximum;
	UINT m_class;
	UINT m_remaining;
}HSMISSILESTORE;

class CHSWeaponDBArray	// Not to be confused with CHSWeaponArray
{
public:
	CHSWeaponDBArray(void);

	void SaveToFile(char *);
	void WriteLaser(CHSDBLaser *, FILE *);
	void WriteMissile(CHSDBMissile *, FILE *);
	void PrintInfo(int);

	BOOL LoadFromFile(char *);
	BOOL GoodWeapon(int);	// Weapon exists or not

	UINT NumWeapons(void);
	UINT GetType(int);

	CHSDBWeapon *GetWeapon(int, UINT);
	CHSDBWeapon *NewWeapon(UINT);

protected:
	UINT m_num_weapons;
	CHSDBWeapon *m_weapons[HS_MAX_WEAPONS];

	CHSDBWeapon *CreateWeapon(char *);
	CHSDBWeapon *CreateLaser(char *, int *);
	CHSDBWeapon *CreateMissile(char *, int *);
};

class CHSObject;

// Fills a slot in the CHSWeaponArray.  Other weapons
// are derived from this.
class CHSWeapon
{
public:
	CHSWeapon(void);

	// Overridable functions.
	virtual UINT GetPowerUsage(void);
	virtual UINT GetRange(void);
	virtual char *GetName(void);
	virtual char *GetAttrInfo(void);
	virtual char *GetStatus(void);
	virtual void SetParent(CHSDBWeapon *);
	virtual void DoCycle(void);
	virtual void SetStatus(int);
	virtual void AttackObject(CHSObject *, CHSObject *, CHSConsole *, int);

	virtual BOOL RequiresLock(void);
	virtual BOOL Configurable(void);
	virtual BOOL Configure(int);
	virtual BOOL Loadable(void);
	virtual BOOL Reload(void);
	virtual BOOL Unload(void);
	virtual BOOL CanAttackObject(CHSObject *);
	virtual BOOL IsReady(void);

	virtual int  Reloading(void);
	virtual int  GetStatusInt(void);
	virtual int  GetStatusChange(void);

	int GetType(void);

	// Attributes
	UINT m_class;

protected:
	WEAPSTAT_CHANGE m_change;
	CHSDBWeapon *m_parent; // Parent to retrieve values from.
};

class CHSLaser : public CHSWeapon
{
public:
	CHSLaser(void);

	// Methods
	UINT	GetRegenTime(void);
	UINT	GetRange(void);
	UINT	GetStrength(void);
	UINT	GetAccuracy(void);
	UINT	GetPowerUsage(void);
	BOOL	NoHull(void);
	BOOL    CanAttackObject(CHSObject *cObj);
	BOOL    IsReady(void);
	BOOL	RequiresLock(void);
	void    AttackObject(CHSObject *, CHSObject *, CHSConsole *, int);
	void	Regenerate(void);
	void	DoCycle(void);
	void	SetStatus(int);
	char   *GetStatus(void);
	char   *GetAttrInfo(void);
	int     GetStatusInt(void);

protected:

	// Attributes
	UINT m_time_to_regen;
	BOOL m_regenerating;
};

#define HS_MAX_MISSILES 15 // Maximum number of missile types in a bay
// Can be used on an object to store/retrieve missles.
class CHSMissileBay
{
public:
	CHSMissileBay(void);
	~CHSMissileBay(void);

	BOOL GetMissile(int, int amt=1); // Pulls a new missile from storage
	BOOL SetRemaining(int, int); // Sets # of missiles remaining

	UINT GetMaxMissiles(int);
	UINT GetMissilesLeft(int);
	
	HSMISSILESTORE *GetStorage(int);

	void SaveToFile(FILE *);
	void LoadFromFile(FILE *);
protected:
	HSMISSILESTORE *m_storage[HS_MAX_MISSILES]; // Missle storage
};

// A weapons array that can exist on any console
class CHSWeaponArray
{
public:
	CHSWeaponArray(void);

	BOOL NewWeapon(int);	// Calls for a new weapon
	BOOL DeleteWeapon(int); // Deletes a weapon in a given slot

	void ClearArray(void);
	void SetMissileBay(CHSMissileBay *);
	void DoCycle(void);
	UINT GetTotalPower(void); // Total power needed by all weapons
	UINT GetMaxRange(void);
	
	CHSWeapon *GetWeapon(int); // Returns the weapon in a given slot

protected:
	void NewLaser(int);		// Creates a new laser in the array
	void NewMTube(int);		// Creates a new missile in the array
	CHSWeapon *m_weapons[HS_MAX_WEAPONS];	// The weapons array
	UINT m_nWeapons;
	CHSMissileBay *m_missile_bay;
};


extern CHSWeaponDBArray waWeapons;

#endif // __HSWEAPON_INCLUDED__
