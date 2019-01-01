#include "hscopyright.h"

// This has to come before the other includes, 
// or Windows really screws up
#ifdef WIN32
#include <winsock.h>
#undef OPAQUE			/* Clashes with flags.h */
#endif

#ifndef WIN32
#include <strings.h>
#endif

#include "hspace.h"
#include "hsuniverse.h"
#include "hsinterface.h"
#include "hscmds.h"
#include "hsfuncs.h"
#include "hsconf.h"
#include "hsdb.h"
#include "hsutils.h"

// The one and only, HSpace instance!
CHSpace HSpace;

CHSpace::CHSpace(void)
{
    m_cycling = FALSE;
    m_cycle_ms = 0;
    m_attrs_need_cleaning = FALSE;
}

// Initializes HSpace.  The reboot variable indicates whether the
// server is in reboot mode, in which the proceses will be swapped
// instead of starting from cold boot.
void CHSpace::InitSpace(int reboot)
{
    // Load the config file first.
    if (!HSCONF.LoadConfigFile(HSPACE_CONFIG_FILE)) {
        notify(3, "DIDN'T LOAD CONFIG FILE");
	return;
    }
    
    if (!dbHSDB.LoadDatabases())
	return;

    // Auto cycle?
    if (HSCONF.autostart)
	m_cycling = TRUE;
}

double CHSpace::CycleTime(void)
{
    return m_cycle_ms;
}

// Called once per second externally to do all of the stuff that
// HSpace needs to do each cycle.
void CHSpace::DoCycle(void)
{
#ifndef WIN32
    struct timeval tv;
#endif
    double firstms, secondms;
    CHSObject *cObj;
    UINT idx, jdx;
    CHSUniverse *uSrc;

    if (!m_cycling)
	return;


    // Grab the current milliseconds
#ifdef WIN32
    firstms = timeGetTime();
#else
    gettimeofday(&tv, (struct timezone *) NULL);
    firstms = (tv.tv_sec * 1000000) + tv.tv_usec;
#endif

    // If attributes need cleaned up, do that.
    if (m_attrs_need_cleaning) {
	dbHSDB.CleanupDBAttrs();
	m_attrs_need_cleaning = FALSE;
    }
    // Do cyclic stuff for space objects.
    for (idx = 0; idx < HS_MAX_UNIVERSES; idx++) {
	uSrc = uaUniverses.GetUniverse(idx);
	if (!uSrc)
	    continue;

	// Grab all of the objects in the universe, and tell them
	// to cycle.

	for (jdx = 0; jdx < HS_MAX_OBJECTS; jdx++) {
	    cObj = uSrc->GetUnivObject(jdx);
	    if (!cObj)
		continue;

	    cObj->DoCycle();
	}

    }				// Cycle stuff done for CHSObjects

    // Grab the current milliseconds, and subtract from the first
#ifdef WIN32
    secondms = timeGetTime();
    m_cycle_ms = secondms - firstms;
#else
    gettimeofday(&tv, (struct timezone *) NULL);
    secondms = (tv.tv_sec * 1000000) + tv.tv_usec;
    m_cycle_ms = (secondms - firstms) * .001;
#endif

}

void CHSpace::DumpDatabases(void)
{
    dbHSDB.DumpDatabases();
    m_attrs_need_cleaning = TRUE;
}

BOOL CHSpace::LoadConfigFile(char *lpstrPath)
{

    return TRUE;
}

char *CHSpace::DoFunction(char *func, dbref executor, char **args)
{
    HSFUNC *hFunc;

    hFunc = hsFindFunction(func);

    if (!hFunc)
	return "#-1 HSpace function not found.";
    else
	return hFunc->func(executor, args);
}

void CHSpace::DoCommand(char *cmd, int switches, char *arg_left,
			char *arg_right, dbref player)
{
    HSPACE_COMMAND *hCmd;

    if (!strcasecmp(cmd, "@space")) {
	hCmd = hsFindCommand(switches, hsSpaceCommandArray);
    } else if (!strcasecmp(cmd, "@nav")) {
	hCmd = hsFindCommand(switches, hsNavCommandArray);
    } else if (!strcasecmp(cmd, "@console")) {
	hCmd = hsFindCommand(switches, hsConCommandArray);
    } else if (!strcasecmp(cmd, "@eng")) {
	hCmd = hsFindCommand(switches, hsEngCommandArray);
    } else {
	notify(player, "Invalid HSpace command.");
	return;
    }

    if (!hCmd) {
	notify(player, "Invalid HSpace switch supplied with command.");
	return;
    }
    // Check the permissions of the command
    if (hCmd->perms & HCP_GOD) {
	if (!God(player)) {
	    notify(player, "HSpace command restricted to God.");
	    return;
	}
    }

    if (hCmd->perms & HCP_WIZARD) {
	if (!Wizard(player)) {
	    notify(player, "HSpace command restricted to wizards.");
	    return;
	}
    }
    // Log the command first
    if (HSCONF.log_commands) {
	char tbuf[1024];
	sprintf(tbuf, "CMD: (#%d) %s/%s %s=%s",
		player, cmd, switches, arg_left, arg_right);
	hs_log(tbuf);
    }
    // Perms look ok, call the function.
    hCmd->func(player, arg_left, arg_right);
}

BOOL CHSpace::Active(void)
{
    return m_cycling;
}

// Turns the space cycle on/off
void CHSpace::SetCycle(BOOL bStart)
{
    m_cycling = bStart;
}
