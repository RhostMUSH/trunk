#include "hscopyright.h"
#include "hstypes.h"
#include "hsutils.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include "ansi.h"
#include "hsinterface.h"
//#include "command.h"
#include <string.h>
#include "hsconf.h"

extern "C" {
    FILE *spacelog_fp;
} int hs_num_keys = -1;

// Key/Value pairs we expect to find in the various
// databases for HSpace.
//
// IF YOU ADD ANYTHING HERE, you need to add it in alphabetical
// order.  The HSFindKey() function uses a binary search algorithm.
struct HSKEYVALUEPAIR hsdbkeypairs[] = {
    "AGE", HSK_AGE,
    "BASESTABILITY", HSK_BASESTABILITY,
    "BOARDING_CODE", HSK_BOARDING_CODE,
    "CARGOSIZE", HSK_CARGOSIZE,
    "CLASSID", HSK_CLASSID,
    "CONSOLE", HSK_CONSOLE,
    "DBVERSION", HSK_DBVERSION,
    "DENSITY", HSK_DENSITY,
    "DESTROYED", HSK_DESTROYED,
    "DESTUID", HSK_DESTUID,
    "DESTX", HSK_DESTX,
    "DESTY", HSK_DESTY,
    "DESTZ", HSK_DESTZ,
    "DOCKED", HSK_DOCKED,
    "DROP CAPABLE", HSK_DROP_CAPABLE,
    "DROPPED", HSK_DROPPED,
    "FLUCTUATION", HSK_FLUCTUATION,
    "HATCH", HSK_HATCH,
    "HULL_POINTS", HSK_HULL_POINTS,
    "IDENT", HSK_IDENT,
    "LANDINGLOC", HSK_LANDINGLOC,
    "MAXFUEL", HSK_MAXFUEL,
    "MAXHULL", HSK_MAXHULL,
    "MINMANNED", HSK_MINMANNED,
    "MISSILEBAY", HSK_MISSILEBAY,
    "NAME", HSK_NAME,
    "OBJECTEND", HSK_OBJECTEND,
    "OBJECTTYPE", HSK_OBJECTTYPE,
    "OBJLOCATION", HSK_OBJLOCATION,
    "OBJNUM", HSK_OBJNUM,
    "PRIORITY", HSK_PRIORITY,
    "ROLL", HSK_ROLL,
    "ROOM", HSK_ROOM,
    "SHIELDAFF", HSK_SHIELDAFF,
    "SIZE", HSK_SIZE,
    "SPACEDOCK", HSK_SPACEDOCK,
    "STABILITY", HSK_STABILITY,
    "SYSTEMDEF", HSK_SYSTEMDEF,
    "SYSTEMEND", HSK_SYSTEMEND,
    "SYSTYPE", HSK_SYSTYPE,
    "UID", HSK_UID,
    "VISIBLE", HSK_VISIBLE,
    "X", HSK_X,
    "XYHEADING", HSK_XYHEADING,
    "Y", HSK_Y,
    "Z", HSK_Z,
    "ZHEADING", HSK_ZHEADING,
    NULL
};

int load_int(FILE * fp)
{
    char word[512];

    if (!fgets(word, 512, fp))
	return SPACE_ERR;
    else
	return atoi(word);
}

float load_float(FILE * fp)
{
    char word[512];

    if (!fgets(word, 512, fp))
	return SPACE_ERR;
    else
	return (float) atof(word);
}


double load_double(FILE * fp)
{
    char word[512];

    if (!fgets(word, 512, fp))
	return SPACE_ERR;
    else
	return atof(word);
}

const char *getstr(FILE * fp)
{
    static char word[4096];
    char *ptr;

    fgets(word, 4096, fp);

    // Strip newlines
    if ((ptr = strchr(word, '\n')) != NULL)
	*ptr = '\0';
    if ((ptr = strchr(word, '\r')) != NULL)
	*ptr = '\0';

    return word;
}

extern "C" {
    void process_command(dbref, dbref, int, char*, char**, int, int);
}

void hs_log(char *msg)
{
    char command[LBUF_SIZE];
    
    STARTLOG(LOG_ALWAYS, "WIZ", "SPACE");
    log_text(msg);
    ENDLOG

    // FIXME
    
    notify(3, msg);
    if (Good_chk(HSCONF.space_wiz)) {
	snprintf(command, LBUF_SIZE-1, "@cemit space=%s", msg ? msg :  "WTF?");

	process_command(HSCONF.space_wiz, HSCONF.space_wiz, 0,
			command, (char **)NULL, 0, 0);
    }

    return;    

}



/*
 * Pulls a delimited number of words out of a string at 
 * a given location and for a given number of delimiters.
 */
void extract(char *string, char *into, int start, int num, char delim)
{
    UINT uDelimsEncountered;
    char *dptr;
    char *sptr;

    uDelimsEncountered = 0;
    dptr = into;
    *dptr = '\0';
    for (sptr = string; *sptr; sptr++) {
	if (*sptr == delim) {
	    uDelimsEncountered++;
	    if ((uDelimsEncountered - start) == (UINT)num) {
		*dptr = '\0';
		return;
	    } else if (uDelimsEncountered > (UINT)start)
		*dptr++ = delim;
	} else {
	    if (uDelimsEncountered >= (UINT)start)
		*dptr++ = *sptr;
	}
    }
    *dptr = '\0';
}

// Takes a string and strips off the leading # sign, turning
// the rest into an integer(dbref)
int strtodbref(char *string)
{
    char *ptr;

    ptr = string;

    if (*ptr == '#')
	ptr++;

    return atoi(ptr);
}

// Looks for a key based on the key string.  Since the key
// strings are sorted, we can use some intelligent searching here.
HS_DBKEY HSFindKey(char *strKeyName)
{
    int top, middle, bottom;
    int ival;

    // If hs_num_keys is -1, then we have to initialize it.
    if (hs_num_keys < 0) {
	for (ival = 0; hsdbkeypairs[ival].name; ival++) {
	};
	hs_num_keys = ival;
    }

    bottom = 0;
    top = hs_num_keys;

    // Look for the key name
    while (bottom < top) {
	// Split the array in two
	middle = ((top - bottom) / 2) + bottom;

	// Check to see if the middle is the match
	ival = strcasecmp(hsdbkeypairs[middle].name, strKeyName);
	if (!ival)
	    return (hsdbkeypairs[middle].key);

	// Not a match, so check to see where we go next.
	// Down or up in the array.
	if (ival > 0) {
	    // Lower portion
	    // Check to see if it's redundant
	    if (top == middle)
		break;

	    top = middle;
	} else {
	    // Upper portion
	    // Check to see if it's redundant
	    if (bottom == middle)
		break;

	    bottom = middle;
	}
    }
    return HSK_NOKEY;
}

// Returns the 3D distance between two sets of coordinates
double Dist3D(double x1, double y1, double z1,
	      double x2, double y2, double z2)
{
    double dist;

    dist = ((x2 - x1) * (x2 - x1)) +
	((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1));

    return sqrt(dist);
}

// Returns the Z angle between the given coordinates.  Six points
// are needed for this operation.
int ZAngle(double fx, double fy, double fz,
	   double sx, double sy, double sz)
{
    char tbuf[8];		// Used to round the number

    if (fz == sz)		// On the same XY plane
	return 0;
    else {
	// Objects are on different XY planes.

	// Are they at the same 2D location?  If so, that's easy!
	if (sy == fy && sx == fx) {
	    if (sz > fz)
		return 90;
	    else
		return -90;
	} else {

	    // They're not at the same 2D location.
	    sprintf(tbuf, "%.0f",
		    RADTODEG *
		    atan((sz - fz) / sqrt(((sx - fx) * (sx - fx)) +
					  ((sy - fy) * (sy - fy)))));
	    return atoi(tbuf);
	}
    }
}

// Returns the XY angle between the given coordinates.  Four points
// are needed for this operation.
int XYAngle(double fx, double fy, double sx, double sy)
{
    char tbuf[8];
    int ang;

    if (sx - fx == 0) {
	if (sy - fy == 0)
	    return 0;
	else if (sy > fy)
	    return 0;
	else
	    return 180;
    } else if (sy - fy == 0) {
	if (sx > fx)
	    return 90;
	else
	    return 270;
    } else if (sx < fx) {
	sprintf(tbuf, "%.0f",
		270 - RADTODEG * atan((sy - fy) / (sx - fx)));
	ang = atoi(tbuf);
	if (ang < 0)
	    ang += 360;
	else if (ang > 359)
	    ang -= 360;
	return ang;
    } else {
	sprintf(tbuf, "%.0f", 90 - RADTODEG * atan((sy - fy) / (sx - fx)));
	ang = atoi(tbuf);
	if (ang < 0)
	    ang += 360;
	else if (ang > 359)
	    ang -= 360;
	return ang;
    }
}

// Gives a standard format error message to a player.
void hsStdError(int player, char *strMsg)
{
    char *strNewMsg;

    strNewMsg = new char[strlen(strMsg) + 32];
    sprintf(strNewMsg,
	    "%s%s-%s %s", ANSI_HILITE, ANSI_GREEN, ANSI_NORMAL, strMsg);
    notify(player, strNewMsg);
    delete[]strNewMsg;
}

extern "C" {
    void process_command(int, int, int, char*, char**, int, int);
}

void hs_format_log(dbref player, char *format, ...)
{
        char buff[LBUF_SIZE];
        char command[LBUF_SIZE];
        
        va_list ap;

        va_start(ap, format);

        vsnprintf(buff, LBUF_SIZE-1, format, ap);
        va_end(ap);
        
        snprintf(command, LBUF_SIZE-1, "@cemit space=%s", buff);
        process_command((dbref)HSCONF.space_wiz, (dbref)HSCONF.space_wiz, 0,
                    (char *)command, (char **)NULL, 0, 0);
                    
}
