// This file will serve as our connection between PennMUSH and
// HSpace.
#include "hscopyright.h"
#include "hspace.h"
#include "hsinterface.h"
#include "hsutils.h"

extern "C" {
    void hsInit(int);
    void hsDumpDatabases(void);
    void hsCommand(char *, int, char *, char *, int);
    void hsCycle(void);
    char *hsFunction(char *, dbref, char **);
}

void hsInit(int reboot)
{
    hs_log("HSpace initialized...");
    HSpace.InitSpace(reboot);
}

void hsDumpDatabases(void)
{
    HSpace.DumpDatabases();
}

void
hsCommand(char *strCmd, int switches,
	  char *arg_left, char *arg_right, int player)
{
    HSpace.DoCommand(strCmd, switches,
		     arg_left, arg_right, (dbref) player);
}

char *hsFunction(char *strFunc, dbref executor, char **args)
{
    return HSpace.DoFunction(strFunc, executor, args);
}

void hsCycle(void)
{
    HSpace.DoCycle();
}
