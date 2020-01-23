#include "hscopyright.h"
#include <stdlib.h>
#include "config.h"

#ifdef I_STRING
#include <string.h>
#else
#include <strings.h>
#endif

#include "hsclass.h"
#include "hsengines.h"
#include "hsutils.h"
#include "hsconf.h"
#include "hsinterface.h"

CHSClassArray caClasses;	// One instance of this .. right here!

CHSClassArray::CHSClassArray(void)
{
    UINT idx;

    for (idx = 0; idx < HS_MAX_CLASSES; idx++)
	m_classes[idx] = NULL;

    m_num_classes = 0;
}

BOOL CHSClassArray::LoadFromFile(char *lpstrPath)
{
    FILE *fp;
    UINT idx;
    int ok;
    char tbuf[LBUF_SIZE];
    char lbuf[256];
    char strKey[64];
    char strValue[64];
    HS_DBKEY key;
    CHSEngSystem *cSys;

    sprintf(tbuf, "LOADING: %s", lpstrPath);
    hs_log(tbuf);

    idx = 0;
    fp = fopen(lpstrPath, "r");
    if (!fp) {
	sprintf(tbuf, "ERROR: Couldn't open %s for loading.", lpstrPath);
	hs_log(tbuf);
	return FALSE;
    }
    // Load database version for possible conversion.
    strcpy(tbuf, getstr(fp));
    extract(tbuf, strKey, 0, 1, '=');
    extract(tbuf, strValue, 1, 1, '=');

    if (strcasecmp(strKey, "DBVERSION")) {
	// Pre-4.0 database
    } else {
	// 4.0 and later
	strcpy(tbuf, getstr(fp));	// Beginning ! mark
    }

    if (*tbuf == EOF) {
	ok = 0;
    } else {
	ok = 1;
	m_classes[idx] = new HSSHIPCLASS;
	SetClassDefaults(m_classes[idx]);
    }

    while (ok) {
	strcpy(tbuf, getstr(fp));
	if (*tbuf == '!') {
	    idx++;
	    m_classes[idx] = new HSSHIPCLASS;
	    SetClassDefaults(m_classes[idx]);
	    continue;
	} else if (!strcasecmp(tbuf, "*END*")) {
	    ok = 0;
	    idx++;
	    continue;
	}
	// Must be a key/value pair, so work with that.
	extract(tbuf, strKey, 0, 1, '=');
	extract(tbuf, strValue, 1, 1, '=');

	// Figure out the key type, and see what we can
	// do with it.
	key = HSFindKey(strKey);
	switch (key) {
	case HSK_NAME:
	    strcpy(m_classes[idx]->m_name, strValue);
	    break;

	case HSK_DROP_CAPABLE:
	    m_classes[idx]->m_can_drop = atoi(strValue);
	    break;

	case HSK_SPACEDOCK:
	    m_classes[idx]->m_spacedock = atoi(strValue);
	    break;

	case HSK_CARGOSIZE:
	    m_classes[idx]->m_cargo_size = atoi(strValue);
	    break;

	case HSK_MINMANNED:
	    m_classes[idx]->m_minmanned = atoi(strValue);
	    break;

	case HSK_MAXFUEL:
	    m_classes[idx]->m_maxfuel = atoi(strValue);
	    break;

	case HSK_MAXHULL:
	    m_classes[idx]->m_maxhull = atoi(strValue);
	    break;

	case HSK_SIZE:
	    m_classes[idx]->m_size = atoi(strValue);
	    break;

	case HSK_SYSTEMDEF:	// Input a list of systems
	    cSys = LoadSystem(fp, idx);
	    if (cSys) {
		m_classes[idx]->m_systems.AddSystem(cSys);
	    }
	    break;

	default:
	    sprintf(lbuf,
		    "WARNING: Invalid db key \"%s\" encountered.", strKey);
	    hs_log(lbuf);
	    break;
	}
    }
    fclose(fp);
    m_classes[idx] = NULL;
    m_num_classes = idx;

    sprintf(lbuf, "LOADING: %s - %d classes loaded (done)",
	    lpstrPath, idx);
    hs_log(lbuf);

    return TRUE;
}

// Prints information about all of the loaded classes to
// the specified player.
void CHSClassArray::PrintInfo(int player)
{
    if (m_num_classes <= 0) {
	notify((int) player, (char *) "No ship classes loaded.");
	return;
    }
    // Print the header
    notify(player, "[ID ] Name");
    int idx;
    for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	if (m_classes[idx]) {
	    notify(player,
		   unsafe_tprintf("[%3d] %s",
				  idx, m_classes[idx]->m_name));
	}
    }
}

// Attempts to allocate space for a new class in the
// array and returns a pointer to that class structure.
HSSHIPCLASS *CHSClassArray::NewClass(void)
{
    // Is the array full?
    if (m_num_classes >= (HS_MAX_CLASSES - 1))
	return NULL;

    m_classes[m_num_classes] = new HSSHIPCLASS;
    SetClassDefaults(m_classes[m_num_classes]);
    m_num_classes++;
    m_classes[m_num_classes] = NULL;

    return m_classes[m_num_classes - 1];
}

// Can be used to delete an existing class.  Give it a class
// number, and it will try to delete it.  It does NOT do any
// operations with objects of this class.  It is your responsibility
// to make sure objects of that class are appropriately handled.
BOOL CHSClassArray::RemoveClass(UINT uClass)
{
    int idx;

    // Error check
    if (!GoodClass(uClass))
	return FALSE;

    // Delete the class, then move the rest above it down
    // one slot.
    delete m_classes[uClass];
    for (idx = uClass + 1; idx < HS_MAX_CLASSES; idx++) {
	m_classes[idx - 1] = m_classes[idx];
    }
    // There's definitely a free spot at the end, so NULLify it.
    m_classes[HS_MAX_CLASSES - 1] = NULL;

    m_num_classes--;

    return TRUE;
}

// When the LoadFromFile() encounters a SYSTEMDEF key, it
// calls upon this function to load the system information
// from the file, construct a CHSEngSystem of some type, load
// it with information, and return that system.
CHSEngSystem *CHSClassArray::LoadSystem(FILE * fp, int slot)
{
    CHSEngSystem *cSys;
    char strKey[64];
    char strValue[64];
    char *ptr;
    char tbuf[256];
    HS_DBKEY key;
    HSS_TYPE type;

    cSys = NULL;

    // We start here, after LoadFromFile() already encountered
    // the SYSTEMDEF keyword.  We figure out the type of system,
    // which should be the first keyword, and then we load
    // according to that.
    // 
    // We have NO WAY of knowing what kind of information each
    // system needs.  We ONLY know what types of systems exist
    // because this is defined in hseng.h.  Thus, we simply
    // figure out the type of system, and then pass the key/value
    // information to the system as if it were setting attributes.
    while (fgets(tbuf, 256, fp)) {
	// Truncate newlines
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';

	extract(tbuf, strKey, 0, 1, '=');
	extract(tbuf, strValue, 1, 1, '=');
	key = HSFindKey(strKey);

	// Check for end of database .. would be an error
	if (!strcmp(strKey, "*END*")) {
	    hs_log
		("ERROR: Encountered end of classdb before loading specified system list.");
	    break;
	}

	switch (key) {
	case HSK_SYSTYPE:
	    type = (HSS_TYPE) atoi(strValue);
	    cSys = m_classes[slot]->m_systems.NewSystem(type);

	    if (!cSys) {
		sprintf(tbuf,
			"ERROR: Invalid system type %d encountered.",
			type);
		hs_log(tbuf);
	    }
	    break;

	case HSK_SYSTEMEND:
	    return cSys;

	default:
	    // Simply call upon the object to set it's
	    // own attributes.  We don't know what they
	    // are.
	    if (cSys) {
		cSys->SetAttributeValue(strKey, strValue);
	    }
	    break;
	}
    }

    return cSys;
}

// Saves class information to the class database.  To do this,
// we just run down the list of classes, outputting some standard
// variables, and then running down the system array for each
// class, using the Sys->SaveToFile() to output system information.
void CHSClassArray::SaveToFile(char *lpstrPath)
{
    FILE *fp;
    UINT i;
    char tbuf[512];

    // Verify we have any classes
    if (m_num_classes <= 0)
	return;

    // Open output file.
    fp = fopen(lpstrPath, "w");
    if (!fp) {
	sprintf(tbuf, "ERROR: Unable to write classes to %s.", lpstrPath);
	hs_log(tbuf);
	return;
    }
    // Write out the database version
    fprintf(fp, "DBVERSION=4.1\n");

    // Ok.  Now just run through the classes, outputting information.
    for (i = 0; i < m_num_classes; i++) {
	// Output standard variables for each class
	fprintf(fp, "!\n");
	fprintf(fp, "NAME=%s\n", m_classes[i]->m_name);
	fprintf(fp, "CARGOSIZE=%d\n", m_classes[i]->m_cargo_size);
	fprintf(fp, "MINMANNED=%d\n", m_classes[i]->m_minmanned);
	fprintf(fp, "SIZE=%d\n", m_classes[i]->m_size);
	fprintf(fp, "MAXHULL=%d\n", m_classes[i]->m_maxhull);
	fprintf(fp, "MAXFUEL=%d\n", m_classes[i]->m_maxfuel);
	fprintf(fp, "DROP CAPABLE=%d\n", m_classes[i]->m_can_drop);
	fprintf(fp, "SPACEDOCK=%d\n", m_classes[i]->m_spacedock);

	// Now output system information
	m_classes[i]->m_systems.SaveToFile(fp);
    }
    fprintf(fp, "*END*\n");
    fclose(fp);
}

BOOL CHSClassArray::GoodClass(UINT uClass)
{
    if (uClass < 0 || uClass >= m_num_classes)
	return FALSE;

    return TRUE;
}

HSSHIPCLASS *CHSClassArray::GetClass(UINT uClass)
{
    if (!GoodClass(uClass))
	return NULL;

    return m_classes[uClass];
}

UINT CHSClassArray::NumClasses(void)
{
    return m_num_classes;
}

// Attempts to load a picture from the account into the
// supplied buffer.
BOOL CHSClassArray::LoadClassPicture(int iClass, char **buff)
{
    FILE *fp;
    char tbuf[512];
    char *ptr;
    int idx;

    sprintf(tbuf, "%s/class_%d.pic", HSCONF.picture_dir, iClass);
    fp = fopen(tbuf, "r");
    if (!fp)
	return FALSE;

    // Load the file
    idx = 0;
    while (fgets(tbuf, 512, fp)) {
	if ((ptr = strchr(tbuf, '\n')) != NULL)
	    *ptr = '\0';
	if ((ptr = strchr(tbuf, '\r')) != NULL)
	    *ptr = '\0';
	buff[idx] = new char[strlen(tbuf) + 1];
	strcpy(buff[idx], tbuf);
	idx++;
    }
    buff[idx] = NULL;

    return TRUE;
}

// Sets some default values for the supplied ship class
// structure.  Since ship classes are just structs and
// not class objects, we have to initialize them a bit with
// some viable default values.
void CHSClassArray::SetClassDefaults(HSSHIPCLASS * sClass)
{
    // Don't allow sizes of 0
    sClass->m_size = 1;

    sClass->m_name[0] = '\0';
    sClass->m_cargo_size = 0;
    sClass->m_maxhull = 1;
    sClass->m_can_drop = FALSE;
    sClass->m_spacedock = FALSE;
    sClass->m_minmanned = 0;
}
