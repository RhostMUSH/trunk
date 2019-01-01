#ifndef __HSENG_INCLUDED__
#define __HSENG_INCLUDED__

#include "hstypes.h"
#include <stdio.h>

class CHSObject;

// Types of systems.  If you add a new system, you'll want
// to add it's type here so that the system array can search
// for it by ID.  You'll also have to add support for it in
// the CHSSystemArray class for creating a new system of that
// type.  And, of course, you'll have to add support for it
// wherever you want it to work in the code.
enum HSS_TYPE
{
	HSS_ENGINES	= 0,
	HSS_COMPUTER,
	HSS_SENSORS,
	HSS_LIFE_SUPPORT,
	HSS_REACTOR,
	HSS_THRUSTERS,
	HSS_FORE_SHIELD,
	HSS_AFT_SHIELD,
	HSS_STARBOARD_SHIELD,
	HSS_PORT_SHIELD,
	HSS_FUEL_SYSTEM,
	HSS_JUMP_DRIVE,
	HSS_COMM,
	HSS_NOTYPE,
	HSS_CLOAK,
	HSS_TACHYON,
	HSS_FICTIONAL,
	HSS_DAMCON,
	HSS_JAMMER,
	HSS_TRACTOR
};

extern char *hsGetEngSystemName(int);
extern HSS_TYPE hsGetEngSystemType(char *);

// Used to indicate how much damage a system has
enum HS_DAMAGE
{
	DMG_NONE = 0,
	DMG_LIGHT,
	DMG_MEDIUM,
	DMG_HEAVY,
	DMG_INOPERABLE
};

// This is the percent of stress increase per cycle per percent
// overload.  So if you overload systems by 1%, then stress 
// increases .012% per cycle up to 100% stress.  If you overload
// 150%, then it increases .6% each cycle, which gives you about
// 2.8 minutes of heavy overloading before full system stress, 
// assuming cycles are 1 second intervals.  As tolerance increases,
// though, the stress increases more slow.  In fact, it's divided
// by tolerance, so the above information applies only for a tolerance
// of 1.
#define HS_STRESS_INCREASE	.012

// A specific system, though general, that will
// be contained in the system array.  Systems can be
// derived from this base type.
class CHSEngSystem
{
public:
	CHSEngSystem(void);
	~CHSEngSystem(void);

	// This function MUST be implemented in any class you
	// derive from it.  It allows the SystemArray duplicator
	// to clone systems without having to know their types.
	virtual CHSEngSystem *Dup(void);

	// If you're deriving an object from this base class,
	// you should implement this function.  That's only
	// if you want the classdb to contain default information
	// for your system on the ship classes.  If not, then
	// you need to have your object constructor initialize
	// your variables to something meaningful, cause the
	// classdb loader won't do it without this function.
	virtual void SaveToFile(FILE *);

	// This doesn't have to be overloaded, but if you're
	// providing any strange name handling for your system,
	// you can override it to return the system name.
	virtual char *GetName(void);

	// If your system has any specific variables not covered
	// by this generic CHSEngSystem, you'll need to implement
	// this in your derived class to handle setting those
	// variables, especially when the system gets loaded
	// from the DB.
	virtual BOOL SetAttributeValue(char *, char *);

	// Override this to provide information about variables
	// specific to your system.  Refer to one of the other,
	// standard systems for implementation examples.
	virtual char *GetAttributeValue(char *, BOOL);

	// If you're doing any cyclic stuff that is different
	// from the cycle in CHSEngSystem, you'll want to 
	// implement this function in your derived class.  Typically,
	// you'll call CHSEngSystem::DoCycle() in your overridden
	// function to handle stress and such, but you don't have to.
	virtual void DoCycle(void);

	// You can override this in your derived class to
	// return optimal power in a different way than normal.
	virtual int GetOptimalPower(BOOL bAdjusted = TRUE);

	// You can override this to provide custom status information
	// for your derived system.
	virtual char *GetStatus(void);

	// You can override this to handle anything specific to
	// your system when the power level is changed.  The
	// basic CHSEngSystem just does some error checking and
	// sets the power.
	virtual BOOL SetCurrentPower(int);

	// Override this in your derived class to print out
	// current system information such as max values.
	// The only parameter passed to this function is a
	// player dbref to print to.
	virtual void GiveSystemInfo(int);

	// Override this to provide a string containing
	// a delimited list of attribute names and values for
	// the system.
	virtual char *GetAttrValueString(void);

	// Override this to provide and action taken
	// when power is taken from this system.
	virtual void CutPower(int);

	// Override this to provide an action taken when the
	// system is powered up(ONLY done when the system
	// was at 0 and now receiving power.)
	virtual void PowerUp(int);

	// These functions are not overridable.
	BOOL IsVisible(void);
	void SetName(char *);
	void SetOwner(CHSObject *);
	void SetType(HSS_TYPE);
	void SetNext(CHSEngSystem *);
	void SetPrev(CHSEngSystem *);
	char *GetOrigName(void);
	void SetParentSystem(CHSEngSystem *); // Indicates a default
										  // parent system with vals.
	int GetCurrentPower(void);
	int GetTolerance(void);
	float GetStress(void);

	HSS_TYPE GetType(void);

	HS_DAMAGE GetDamageLevel(void);
	HS_DAMAGE DoDamage(void);
	HS_DAMAGE ReduceDamage(HS_DAMAGE lvl);
	HS_DAMAGE ReduceDamage(void);

	CHSEngSystem *GetPrev(void);
	CHSEngSystem *GetNext(void);

protected:
	HSS_TYPE m_type;

	BOOL m_bVisible; // Users can see it in the list and use it.

	char *m_name; // System name

	CHSObject *m_ownerObj; // Indicates the CHSObject owning the system

	// These variables are defined as pointers.  Why?  Well,
	// that's so we can override them specifically at the ship
	// level.  When a system tries to return a value to a calling
	// function, it checks to see if these variables are NULL.  If
	// so, it returns the default value from the class for that
	// ship.  Otherwise, we have no way of knowing whether a variable
	// is overridden on the ship or not.
	int  *m_optimal_power; // Optimal power to allocate
	int  *m_tolerance; // How quickly the system stresses (1 .. n)

	// Variables used locally
	float m_stress; // 0 - 100 for amount of stress incurred
	int m_current_power; // Power allocated

	HS_DAMAGE m_damage_level; // Current damage level

	CHSEngSystem *m_parent; // Parent system to retrieve default vals.
	CHSEngSystem *m_next; // Ptrs for the linked list
	CHSEngSystem *m_prev; // implementation.

	void CheckSystemPower(void); // Prevents inadvertent overloading
};


// A sensor contact, stored in the sensor system
enum CONTACT_STATUS
{
	UPDATED = 0,  // It was detected, now identified
	DETECTED,     // Only detected
	IDENTIFIED,   // Instantly detected AND identified
};

typedef struct
{
	UINT m_id; // Contact number
	CHSObject *m_obj;
	BOOL bVerified; // Used each cycle to indicate object still on sensors
	CONTACT_STATUS status;
}SENSOR_CONTACT;

#define HS_MAX_CONTACTS		64
#define HS_MAX_NEW_CONTACTS 64
// A system specifically for sensors, derived from
// the base CHSEngSystem class.  This class handles the
// list of contacts currently on sensors.
class CHSSysSensors : public CHSEngSystem
{
public:
	CHSSysSensors(void);
	~CHSSysSensors(void);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	UINT GetSensorRating(void);
	UINT NumContacts(void);
	void SaveToFile(FILE *);
	void DoCycle(void);
	void ClearContacts(void);
  	void GiveSystemInfo(int);
	char *GetAttrValueString(void);
	void LoseObject(CHSObject *);

	SENSOR_CONTACT *GetFirstNewContact(void);
	SENSOR_CONTACT *GetNextNewContact(void);
	SENSOR_CONTACT *GetFirstLostContact(void);
	SENSOR_CONTACT *GetNextLostContact(void);
	SENSOR_CONTACT *GetContact(int); // Retrieves by slot
	SENSOR_CONTACT *GetContact(CHSObject *); // Retrieves by object
	SENSOR_CONTACT *GetContactByID(int); // Retrives by ID

	CHSEngSystem *Dup(void);
protected:
	// Methods
	void NewContact(CHSObject *, CONTACT_STATUS);
	void UpdateContact(SENSOR_CONTACT *);
	void LoseContact(SENSOR_CONTACT *);
	UINT RandomSensorID(void);

	// Overridable variables at the ship level
	int	*m_sensor_rating;

	SENSOR_CONTACT *m_contact_list[HS_MAX_CONTACTS]; // Contact list

	// Stores new contacts
	SENSOR_CONTACT *m_new_contacts[HS_MAX_NEW_CONTACTS];
	SENSOR_CONTACT *m_lost_contacts[HS_MAX_NEW_CONTACTS];

	// Index to the current sensor contact slot being queried
	UINT uContactSlot;
	UINT m_nContacts;
};

// Thrusters, which control steering for the ship.
class CHSSysThrusters : public CHSEngSystem
{
public:
	CHSSysThrusters(void);
	~CHSSysThrusters(void);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	UINT GetRate(BOOL bAdjusted = TRUE);
	void SaveToFile(FILE *);
  	void GiveSystemInfo(int);
	char *GetAttrValueString(void);
	CHSEngSystem *Dup(void);
	void PowerUp(int);

protected:
	int *m_turning_rate;
};

// The computer system, which handles a lot of internal
// ship power allocation and information handling.
class CHSSysComputer : public CHSEngSystem
{
public:
	CHSSysComputer(void);

	int GetOptimalPower(BOOL bDamage = TRUE);
	int GetPowerSurplus(void);
	int GetConsoles(void);
	int GetUsedPower(void);
	int GetPoweredConsoles(void);
	void PowerUp(int);
	void CutPower(int);

	BOOL DrainPower(int);
	void ReleasePower(int);

	CHSEngSystem *Dup(void);

protected:
	int  m_power_used;
	UINT TotalShipPower(void);
};

// An array of systems that can exist for a ship.
// In fact, the systems are stored in a linked list.
class CHSSystemArray
{
public:
	CHSSystemArray(void);
	BOOL AddSystem(CHSEngSystem *);
	BOOL DelSystem(CHSEngSystem *);

	void operator =(CHSSystemArray &); // Duplicates this array
	void DoCycle(void);
	void UpdatePowerUse(void);
	void SaveToFile(FILE *);

	BOOL BumpSystem(CHSEngSystem *, int);
	UINT GetPowerUse(void);
	UINT NumSystems(void);

	CHSEngSystem *NewSystem(HSS_TYPE);
	CHSEngSystem *GetSystem(HSS_TYPE);
	CHSEngSystem *GetSystemByName(char *);
	CHSEngSystem *GetHead(void);
	CHSEngSystem *GetRandomSystem(void);

protected:
	CHSEngSystem *m_SystemHead;
	CHSEngSystem *m_SystemTail;

	UINT m_nSystems;
	UINT m_uPowerUse;
};

// The life support system, which provides life supporting
// goodies to a vessel.
class CHSSysLifeSupport : public CHSEngSystem
{
public:
	CHSSysLifeSupport(void);

	CHSEngSystem *Dup(void);

	void DoCycle(void);
	double GetAirLeft(void);
	void PowerUp(int);
	void CutPower(int);

protected:
	float m_perc_air; // Amount of air in the vessel
};

// There are currently two types of shields.  These are
// their enums
enum HS_SHIELDTYPE
{
	ST_DEFLECTOR = 0,
	ST_ABSORPTION
};

// The shield class.
class CHSSysShield : public CHSEngSystem
{
public:
	CHSSysShield(void);

	CHSEngSystem *Dup(void);

	HS_SHIELDTYPE GetShieldType(void);

	UINT DoDamage(int);
	void DoCycle(void);
	void SaveToFile(FILE *);
  	void GiveSystemInfo(int);
	char *GetAttrValueString(void);
	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	BOOL SetCurrentPower(int);

	double GetMaxStrength(void);
	double GetRegenRate(BOOL bAdjusted = TRUE);
	double GetShieldPerc(void);

protected:
	double *m_max_strength;
	double m_strength;
	double *m_regen_rate;

	HS_SHIELDTYPE m_shield_type; // Type of shield
};

class CHSFuelSystem;

// The HSpace Reactor Class.  Power me up, Scotty!
// This object is found on ships to supply power that
// can be transferred to various systems.  It is actually
// a sort of system because it's derived from CHSEngSystem,
// but we'll ultimately use it differently.  It will provide
// power, not consume it.
class CHSReactor : public CHSEngSystem
{
public:
	CHSReactor(void);
	~CHSReactor(void);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	BOOL SetOutputLevel(int);
	BOOL IsOnline(void);
	UINT GetOutput(void);
	UINT GetDesiredOutput(void);
	UINT GetMaximumOutput(BOOL bWithDamage = TRUE);
	int  GetEfficiency();
	void SaveToFile(FILE *);
	void DoCycle(void); // Overridden from base class
	void SetFuelSource(CHSFuelSystem *);
  	void GiveSystemInfo(int);
	char *GetAttrValueString(void);
	char *GetStatus(void);

	CHSEngSystem *Dup(void);
protected:
	UINT m_output; // Power output
	UINT m_desired_output; // Desired output level

	// These variables are overridable at the ship level
	int *m_maximum_output; // Maximum operating level
	int *m_efficiency; // Thousands of distance units per fuel unit

	CHSFuelSystem *m_fuel_source; // Optional fuel source
};

// The communications system simply indicates if the
// parent object can receive and transmit.
class CHSSysComm : public CHSEngSystem
{
public:
	CHSSysComm(void);
	~CHSSysComm(void);

	BOOL SetAttributeValue(char *, char *);
	char *GetAttributeValue(char *, BOOL);
	UINT GetMaxRange(BOOL bAdjusted = TRUE);
	void SaveToFile(FILE *);
  	void GiveSystemInfo(int);
	char *GetAttrValueString(void);
	CHSEngSystem *Dup(void);

protected:
	UINT *m_comm_range;
};

// Cloaking Device, Generic system with some enhancements.
class CHSSysCloak : public CHSEngSystem
{
public:
	CHSSysCloak(void);
	~CHSSysCloak(void);

	BOOL SetAttributeValue(char *, char *);
	void SaveToFile(FILE *);
	char *GetAttributeValue(char *, BOOL);
	char *GetAttrValueString(void);
	float GetEfficiency(BOOL);
	int GetEngaged(void);
	void SetEngaged(BOOL);
	CHSEngSystem *Dup(void);
	BOOL IsEngaging(void);
	void DoCycle(void);

protected:

	// Efficiency of the cloaking device (0 - 100)
	float *m_efficiency;
	double m_engaging;
	int m_engaged;
};

// Tachyon Sensors, Generic system with some enhancements.
class CHSSysTach : public CHSEngSystem
{
public:
	CHSSysTach(void);
	~CHSSysTach(void);


	BOOL SetAttributeValue(char *, char *);
	void SaveToFile(FILE *);
	char *GetAttributeValue(char *, BOOL);
	char *GetAttrValueString(void);
	float GetEfficiency(BOOL);
	CHSEngSystem *Dup(void);

protected:

	// Efficiency of the Tachyon Sensor Array (0 - 100)
	float *m_efficiency;
};

// Fictional System, Generic System with no hardcoded purpose.
class CHSFictional : public CHSEngSystem
{
public:
	CHSFictional(void);
	
	CHSEngSystem *Dup(void);

protected:

};

#define HS_MAX_DAMCREWS	15
class CHSDamCon : public CHSEngSystem
{
public:
	CHSDamCon(void);
	~CHSDamCon(void);

	BOOL SetAttributeValue(char *, char *);
	void SaveToFile(FILE *);
	void DoCycle(void);
	void AssignCrew(dbref, int, CHSEngSystem *);
  	void GiveSystemInfo(int);
	char *GetAttributeValue(char *, BOOL);
	char *GetAttrValueString(void);
	int GetNumCrews(void);
	int GetCyclesLeft(int);
	CHSEngSystem *Dup(void);
	CHSEngSystem *GetWorkingCrew(int);

protected:
 
	int *m_ncrews;
	CHSEngSystem *m_crew_working[HS_MAX_DAMCREWS];
	int m_crew_cyclesleft[HS_MAX_DAMCREWS];
};

enum HS_TMODE
{
	HSTM_TRACTOR = 0,
	HSTM_REPULSE,
	HSTM_HOLD
};

// Communication Jammers, interfere with communications within range
class CHSSysJammer : public CHSEngSystem
{
public:
	CHSSysJammer(void);
	~CHSSysJammer(void);


	BOOL SetAttributeValue(char *, char *);
	void SaveToFile(FILE *);
	char *GetAttributeValue(char *, BOOL);
	char *GetAttrValueString(void);
	double GetRange(BOOL);
	CHSEngSystem *Dup(void);

protected:

	// Range of the sensor jammer.
	double *m_range;
};

// Tractor Beams, tractor, repulse or hold ships within the beam range.
class CHSSysTractor : public CHSEngSystem
{
public:
	CHSSysTractor(void);
	~CHSSysTractor(void);

	BOOL SetAttributeValue(char *, char *);
	
	void SetMode(int);
	void SetLock(dbref, int);
	void SaveToFile(FILE *);
	void DoCycle(void);
	void DockShip(dbref, int, int);
		
	char *GetAttributeValue(char *, BOOL);
	char *GetAttrValueString(void);
	
	double GetStrength(BOOL);
	
	float GetEffect(void);

	HS_TMODE GetMode(void);
	
	CHSEngSystem *Dup(void);

	CHSObject *GetLock(void);

protected:

	// Strength of the tractor beam.
	double *m_strength;

	HS_TMODE m_mode;

	CHSObject *m_lock;
};

#endif // __HSENG_INCLUDED__

