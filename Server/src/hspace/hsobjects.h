#ifndef __HSOBJECTS_INCLUDED__
#define __HSOBJECTS_INCLUDED__

#include "hstypes.h"
#include "hsclass.h"
#include "hsweapon.h"
#include "hsfuel.h"

// A class that represents a vector.  You can do lots of
// things with this class.
class CHSVector
{
public:
	CHSVector(void);
	CHSVector(double, double, double);

	double i(void);
	double j(void);
	double k(void);
	void operator=(CHSVector &);
	BOOL operator==(CHSVector &);
	CHSVector &operator+(CHSVector &);
	void operator+=(CHSVector &);
	
	double DotProduct(CHSVector &);

protected:
	double ci, cj, ck; // The coefficients for the vector
};

// Possible object types.  If you're adding any new types of objects
// to space, you have to add a listing here.
enum HS_TYPE
{
	HST_NOTYPE = 0,
	HST_SHIP,
	HST_MISSILE,
	HST_PLANET,
	HST_WORMHOLE,
	HST_BLACKHOLE,
	HST_NEBULA,
	HST_ASTEROID
};

class CHSConsole;

// Base HSpace object class
class CHSObject
{
public:
	CHSObject(void);
	virtual ~CHSObject(void);

	// Access methods
	double GetX(void);
	double GetY(void);
	double GetZ(void);
	UINT   GetUID(void);
	dbref  GetDbref(void);
	char   *GetName(void);
	void   SetX(double);
	void   SetY(double);
	void   SetZ(double);
	void   SetUID(UINT);
	void   SetName(char *);
	void   SetDbref(dbref);
	BOOL   IsVisible(void);

	CHSVector &GetMotionVector(void);

	// Operator to compare objects.
	BOOL operator==(CHSObject &);
	BOOL operator==(CHSObject *);

	virtual int  GetSpeed(void);
	virtual void MoveTowards(double, double, double, float); // Move the object towards certain coordinates at a certain speed.
	virtual void MoveInDirection(int, int, float); // Move the object a certain amount of hms in a certain direction.
	virtual void HandleMessage(char *, int, long *data = NULL);
	virtual void DoCycle(void);
	virtual void ClearObjectAttrs(void);
	virtual void WriteToFile(FILE *);  // Override this.
	virtual void HandleLock(CHSObject *, BOOL);
	virtual void HandleDamage(CHSObject *, CHSWeapon *, int, CHSConsole *, int);
	virtual void ExplodeMe(void);
	virtual void GiveScanReport(CHSObject *, dbref, BOOL);
	virtual void EndLoadObject(void);

	virtual char *GetObjectColor(void);  // oooh Color. 
	virtual char GetObjectCharacter(void);
	virtual char *GetAttributeValue(char *);

	virtual BOOL SetAttributeValue(char *, char *);
	virtual BOOL IsActive(void);
	virtual BOOL LoadFromFile(FILE *); // Can be overridden if you want

	virtual UINT GetEngSystemTypes(int *);
	virtual UINT GetSize(void);

	virtual CHSSystemArray *GetEngSystemArray(void);
	virtual CHSConsole *FindConsole(dbref);
	virtual CHSEngSystem *GetEngSystem(int);

	HS_TYPE GetType(void);

	// Linked list access functions (if needed)
	void   SetNext(CHSObject *);
	void   SetPrev(CHSObject *);
	CHSObject *GetNext(void);
	CHSObject *GetPrev(void);

protected:
	// Methods

	// You MUST implement this function in your derived
	// object if you have custom information that's loaded
	// from the object db.  Otherwise, you can just initialize
	// your variables in the constructor and let the CHSObject
	// handle any keyed information in the database.
	virtual BOOL HandleKey(int, char *, FILE *);

	dbref  m_objnum; // Unique object identifier
	double m_x;	// Coordinate locations
	double m_y;
	double m_z;
	UINT   m_uid; // Universe membership
	BOOL   m_visible;
	int    m_size;
	int    m_current_speed;
	char   *m_name;

	enum HS_TYPE m_type;

	CHSVector m_motion_vector;

	// These can be used to implement a linked list,
	// but they may not be used.
	CHSObject *m_prev;
	CHSObject *m_next;
};

#define HS_MAX_MESSAGE_TYPES	5
// A generic console object
class CHSConsole
{
public:
	CHSConsole(void);
	~CHSConsole(void);

	void WriteObjectAttrs(void);
	void ClearObjectAttrs(void);
	void SetOwner(dbref);
	void HandleMessage(char *, int);
	void AdjustHeading(int, int);
	void SetOwnerObj(CHSObject *);
	void SetMissileBay(CHSMissileBay *);
	void DoCycle(void);
	void SetComputer(CHSSysComputer *);

	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);
	UINT GetMaximumPower(void);
	BOOL LoadFromObject(dbref);
	BOOL IsOnline(void);
	BOOL OnFrq(double); // On a given frequency or not.
	BOOL GetMessage(int);
	BOOL AddMessage(int);
	BOOL DelMessage(int);

	int  GetXYHeading(void);
	int  GetZHeading(void);

	dbref GetOwner(void);

	CHSObject *GetObjectLock(void);
	CHSVector &GetHeadingVector(void);
	CHSWeaponArray *GetWeaponArray(void);

	// These commands interact with players in the game.
	BOOL AddWeapon(dbref, int);
	void LockTargetID(dbref, int);
	void UnlockWeapons(dbref);
	void GiveGunneryReport(dbref);
	void DeleteWeapon(dbref, int);
	void ConfigureWeapon(dbref, int, int);
	void LoadWeapon(dbref, int);
	void ChangeHeading(dbref, int, int);
	void FireWeapon(dbref, int);
	void PowerUp(dbref);
	void PowerDown(dbref);
	void GiveTargetReport(dbref);
	void UnloadWeapon(dbref, int);
	void SetAutoload(dbref, BOOL);
	void SetAutoRotate(dbref, BOOL);
	void SetSystemTarget(dbref, int);

	// Attributes
	dbref m_objnum;

protected:
	// Member functions
	void LoadWeapons(char *);
	BOOL IsInCone(CHSObject *);

	// Attributes
	dbref m_owner;
	BOOL m_online;
	CHSSysComputer *m_computer; // Source of information
	int m_xyheading;
	int m_xyoffset; // XY offset from front of ship
	int m_zoffset;  // Z offset from front of ship
	int m_zheading;
	int m_arc; // Firing arc
	int m_targetting; // Which system is being targetted
	int m_msgtypes[HS_MAX_MESSAGE_TYPES];

	BOOL m_can_rotate; // Console can rotate like a turret
	BOOL m_autoload;
	BOOL m_autorotate;

	CHSObject *m_target_lock;

	CHSWeaponArray *m_weapon_array;
	CHSObject *m_ownerObj;
	CHSMissileBay *m_missile_bay;
};

// This generic landing location class can be used for landing
// pads on planets or as landing bays in a ship.
#define HS_LANDING_CODE_LEN  8
#define HS_MAX_LANDING_LOCS	15
class CHSLandingLoc
{
public:
	// Methods
	CHSLandingLoc(void);
	~CHSLandingLoc(void);

	void ClearObjectAttrs(void);
	void WriteObjectAttrs(void);
	void WriteToFile(FILE *);
	void HandleMessage(char *, int);
	void DeductCapacity(int);
	void SetActive(BOOL);
	void SetOwnerObject(CHSObject *);
	BOOL HandleKey(int, char *, FILE *);
	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);
	BOOL CodeRequired(void);
	BOOL LoadFromObject(dbref);
	BOOL IsActive(void);
	BOOL CodeClearance(char *);
	BOOL CanAccomodate(CHSObject *);

	CHSObject *GetOwnerObject(void);


	// Attributes
	dbref objnum;
protected:
	CHSObject *m_ownerObj;

	int   m_capacity;
	int   m_max_capacity; // -1 for unlimited
	char  strCode[9];
	BOOL  bActive;
};

class CHSHatch
{
public:
	// Methods
	CHSHatch(void);
	~CHSHatch(void);

	void ClearObjectAttrs(void);
	void WriteObjectAttrs(void);
	void HandleMessage(char *, int);
	void SetOwnerObject(CHSObject *);
	char *GetAttributeValue(char *);
	BOOL SetAttributeValue(char *, char *);
	BOOL LoadFromObject(dbref);

	CHSObject *GetOwnerObject(void);


	// Attributes
	dbref objnum;
	dbref m_targetObj;
	int m_targetHatch;
	int m_clamped;
protected:
	CHSObject *m_ownerObj;
};

#define MAX_SHIP_ROOMS		250
#define MAX_SHIP_CONSOLES	 20
#define MAX_HATCHES		 10
class CHSTerritory; // From hsterritory.h
// The generic ship object, which contains information about its
// class, weapons, etc.
class CHSShip : public CHSObject
{
public:
	CHSShip(void);
	~CHSShip(void);

	float CloakingEffect(void); // 0 - 1.00
	float TachyonEffect(void); // 0 - 1.00

	UINT ClassNum(void);
	UINT GetXYHeading(void);
	UINT GetRoll(void);
	UINT GetMaxHullPoints(void);
	UINT GetHullPoints(void);
	UINT GetSize(void); // Overrides the base class
	UINT GetEngSystemTypes(int *);

	int GetZHeading(void);
	
	int   GetSpeed(void);
	int	GetMannedConsoles(void);
	int	GetMinManned(void);
	dbref GetDockedLocation(void);
	dbref Clone(void);

	BOOL LoadFromObject(dbref);
	BOOL IsActive(void);
	BOOL SetClassInfo(UINT);
	BOOL AddConsole(dbref);
	BOOL RemoveConsole(dbref);
	BOOL SetAttributeValue(char *, char *);
	BOOL SetSystemAttribute(char *, char *, char *);
	BOOL AddSroom(dbref);
	BOOL AddHatch(dbref);
	BOOL DeleteSroom(dbref);
	BOOL CanDrop(void);
	BOOL IsSpacedock(void);
	BOOL AddLandingLoc(dbref);
	BOOL InHyperspace(void);
	BOOL IsDestroyed(void);
	BOOL MakeBoardLink(CHSShip *, BOOL);
	BOOL Landed(void);
	BOOL IsObjectLocked(CHSObject *);

	void KillShipCrew(const char *);
	void EndLoadObject(void);
	void MoveShipObject(dbref);
	void TotalRepair(void); // Repairs everything
	void DoCycle(void);
	void ClearObjectAttrs(void);
	void WriteObjectAttrs(void);
	void WriteToFile(FILE *);
	void SetHeadingVector(int, int, int);
	char *GetObjectColor(void);
	char *GetBoardingCode(void);
	char *GetAttributeValue(char *);
	char GetObjectCharacter(void);
	void HandleMessage(char *, int, long *data = NULL);
	void HandleLock(CHSObject *, BOOL);
	void HandleDamage(CHSObject *, CHSWeapon *, int, CHSConsole *, int);
	void ExplodeMe(void);
	void GiveScanReport(CHSObject *, dbref, BOOL);
	void GiveCrewRep(dbref);
	void GiveHatchRep(dbref);
	void AssignCrew(dbref, int, char *);
	void ConfirmHatches(void);

	CHSConsole *GetConsole(UINT);
	CHSConsole *FindConsole(dbref); // Locates a console object
	CHSLandingLoc *GetLandingLoc(int);
	CHSLandingLoc *FindLandingLoc(dbref);
	CHSHatch *GetHatch(int);
	CHSHatch *FindHatch(dbref);
	CHSEngSystem  *GetEngSystem(int);
	CHSSystemArray *GetEngSystemArray(void);
	SENSOR_CONTACT *GetSensorContact(CHSObject *);
	SENSOR_CONTACT *GetSensorContact(int);

	// Methods that players interact with
	void GiveEngSysReport(dbref);
	void AddHatch(dbref, dbref);
	void DeleteHatch(dbref, dbref);
	void SetSystemPower(dbref, char *, int, BOOL);
	void ChangeSystemPriority(dbref, char *, int);
	void SetVelocity(dbref, int);
	void SetHeading(dbref, int xy = -1, int z = -1);
	void SetRoll(dbref, int roll = -1);
	void GiveSensorReport(dbref, HS_TYPE tType = HST_NOTYPE);
	void GiveVesselStats(dbref);
	void GiveNavigationStatus(dbref);
	void LandVessel(dbref, int, int, char *);
	void UndockVessel(dbref);
	void NotifyConsoles(char *, int); // Sends a message to all consoles
	void NotifySrooms(char *); // Send a message to all ship rooms
	void EngageAfterburn(dbref, BOOL);
	void EngageJumpDrive(dbref, BOOL);
	void EngageCloak(dbref, BOOL);
	void DoBoardLink(dbref, int, int, int);
	void DoBreakBoardLink(dbref, int);
	void DisembarkPlayer(dbref, int);
	void ScanObjectID(dbref, int);
	void ViewObjectID(dbref, int);
	void GateObjectID(dbref, int);
	void InitSelfDestruct(dbref, int, char *);
	int  m_hull_points;
	BOOL SetNumMissiles(dbref, int, int);

	// Engineering systems
	CHSSystemArray m_systems;

	BOOL SetMapRange(dbref, int);

	CHSMissileBay *GetMissileBay(void);

	dbref m_objlocation; // Dbref where it's docked or dropped

	BOOL m_docked;

protected:

	// Methods
	CHSSysShield *DetermineHitShield(int, int);
	dbref CloneRoom(dbref, int *, CHSShip *); // Clones a specific room recursively

	BOOL HandleKey(int, char *, FILE *);
	BOOL LoadSystem(FILE *);
	void EnterHyperspace(void);
	void ExitHyperspace(void);
	void Travel(void);
	void SensorSweep(void);
	void HandleSensors(void);
	void HandleSystems(void);
	void HandleLifeSupport(void);
	void ChangeHeading(void);
	void RecomputeVectors(void);
	void InitLanding(dbref, SENSOR_CONTACT *, CHSLandingLoc *);
	void HandleLanding(void);
	void HandleUndocking(void);
	void ResurrectMe(void);
	void HandleCommMsg(const char *, long *);
	void SelfDestruct(void);

	CHSLandingLoc *NewLandingLoc(void);
	CHSHatch *NewHatch(void);

	// Attributes
	char *m_ident;
	char *m_boarding_code;
	BOOL *m_can_drop;
	BOOL *m_spacedock;
	int  *m_cargo_size;
	CHSTerritory *m_territory; // Territory we're in.

	CHSLandingLoc *m_landing_target;  // Valid only during landing

	int  m_current_xyheading;
	int  m_desired_xyheading;
	int  m_current_zheading;
	int  m_desired_zheading;
	int  m_desired_roll;
	int  m_current_roll;
	int  m_drop_status;
	int  m_dock_status;
	int  *m_minmanned;
	int  m_class;
	int  m_map_range;
	int  m_current_speed;
	int  m_self_destruct_timer;
	int  m_age;

	BOOL bGhosted; // Life support failed?
	BOOL bReactorOnline;
	BOOL m_destroyed;
	BOOL m_dropped;
	BOOL m_in_space;
	BOOL m_hyperspace; // In warp/hyperspace or not.

	CHSObject *m_target; // Used for locking onto a target.

	// Registered ship rooms
	dbref m_ship_rooms[MAX_SHIP_ROOMS];

	// Registered consoles.  NOTE: Ships initially
	// start with NO consoles.  The object representing
	// the ship may or may NOT be a console.
	CHSConsole *m_console_array[MAX_SHIP_CONSOLES];

	// Landing bays
	CHSLandingLoc *m_landing_locs[HS_MAX_LANDING_LOCS];

	// Boarding links with other ships
	CHSHatch *m_hatches[MAX_HATCHES];

	// Ship class info
	HSSHIPCLASS *m_classinfo;

	// Missile bay for the ship
	CHSMissileBay m_missile_bay;
};

#endif // __HSOBJECTS_INCLUDED__
