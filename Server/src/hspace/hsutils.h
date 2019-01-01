#ifndef __HSUTILS_INCLUDED__
#define __HSUTILS_INCLUDED__

#include <stdio.h>

#define SPACE_ERR	-534234	// Just some random number
#define RADTODEG 	57.296  //  180 / pi

int load_int(FILE *);
double load_double(FILE *);
float load_float(FILE *);
const char *getstr(FILE *);
void hs_log(char *);
void extract(char *, char *, int, int, char);
int strtodbref(char *);
double Dist3D(double, double, double, double, double, double);
int XYAngle(double, double, double, double);
int ZAngle(double, double, double, double, double, double);
void hsStdError(int, char *);

// Various keys found in our databases
enum HS_DBKEY
{
	HSK_NOKEY = 0,
	HSK_BOARDING_CODE,
	HSK_CARGOSIZE,
	HSK_CLASSID,
	HSK_CONSOLE,
	HSK_DBVERSION,
	HSK_DESTROYED,
	HSK_DOCKED,
	HSK_DROP_CAPABLE,
	HSK_DROPPED,
	HSK_HATCH,
	HSK_HULL_POINTS,
	HSK_IDENT,
	HSK_LANDINGLOC,
	HSK_MAXHULL,
	HSK_MAXFUEL,
	HSK_MISSILEBAY,
	HSK_NAME,
	HSK_OBJECTEND,
	HSK_OBJECTTYPE,
	HSK_OBJLOCATION,
	HSK_OBJNUM,
	HSK_PRIORITY,
	HSK_ROLL,
	HSK_ROOM,
	HSK_SIZE,
	HSK_SYSTEMDEF,
	HSK_SYSTEMEND,
	HSK_SYSTYPE,
	HSK_UID,
	HSK_VISIBLE,
	HSK_X,
	HSK_XYHEADING,
	HSK_Y,
	HSK_Z,
	HSK_ZHEADING,
	HSK_DENSITY,
	HSK_SPACEDOCK,
	HSK_MINMANNED,
	HSK_BASESTABILITY,
	HSK_STABILITY,
	HSK_FLUCTUATION,
	HSK_DESTX,
	HSK_DESTY,
	HSK_DESTZ,
	HSK_DESTUID,
	HSK_AGE,
	HSK_SHIELDAFF

};

HS_DBKEY HSFindKey(char *);

// A key/value pair structure

struct HSKEYVALUEPAIR
{
	char *name;
	HS_DBKEY key;
};


#endif

