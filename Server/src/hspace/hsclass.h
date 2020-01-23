#ifndef __HSCLASS_INCLUDED__
#define __HSCLASS_INCLUDED__

#include "hstypes.h"
#include "hseng.h"
#include "hsfuel.h"

#define SHIP_CLASS_NAME_LEN 32

/* A ship class */
typedef struct {

  char  m_name[SHIP_CLASS_NAME_LEN];
  UINT  m_size;        /* Scale of the vessel */
  UINT  m_cargo_size;   /* Size of the cargo bay */
  UINT	m_minmanned;
  UINT  m_maxhull;     /* Maximum hull pts */
  UINT  m_maxfuel;      /* Maximum amount of fuel that can be stored */
  BOOL  m_can_drop;    /* Has drop jets or not */
  BOOL	m_spacedock;	/* Can hold large vessels */

  // The following are handled by systems
  UINT  m_acceleration;/* Acceleration rate */
  float m_turnrate;    /* Turn rate of the ship */
  float m_max_velocity;/* Maximum velocity */
  float m_maxfore;     /* Maximum foreshield pts */
  int   m_maxpower;    /* Maximum power output */
  float m_maxaft;      /* Maximum aftshield pts */
  float m_fore_regen;  /* Forward shield regeneration rate */
  float m_aft_regen;   /* Aft shield regeneration rate */
  int   m_can_jump;    /* Has jump drives or not */
  int   m_fuel_eff;    /* Fuel consumption rate in thousands */
  int   m_min_jump_speed;/* Minimum speed to enter hyperspace */
  int   m_can_cloak;   /* Cloakable or not */
  int   m_sensor_rating;/* Sensor array rating */
  char  m_fuel_xfer;    /* Has fuel transfer systems or not */

  CHSSystemArray m_systems; // Array of available systems
}HSSHIPCLASS;

#define HS_MAX_CLASSES 100

class CHSClassArray
{
public:
	CHSClassArray(void);

	BOOL LoadFromFile(char *);
	BOOL GoodClass(UINT);
	BOOL RemoveClass(UINT);

	UINT NumClasses(void);
	BOOL LoadClassPicture(int, char **);

	void SaveToFile(char *);
	void PrintInfo(int);

	CHSEngSystem *LoadSystem(FILE *, int);

	HSSHIPCLASS *GetClass(UINT);
	HSSHIPCLASS *NewClass(void);

protected:

	void SetClassDefaults(HSSHIPCLASS *);

	UINT m_num_classes;
	HSSHIPCLASS *m_classes[HS_MAX_CLASSES];
};

extern CHSClassArray caClasses;

#endif