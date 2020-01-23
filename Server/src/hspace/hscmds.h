#ifndef __HSCMDS_INCLUDED__
#define __HSCMDS_INCLUDED__


// Command permissions
#define HCP_WIZARD	0x1	// Only wizards can use the command
#define HCP_ANY		0x2	// Anyone can use the command
#define HCP_GOD		0x4     // Only God can use the command

// A handy prototyper
#define HSPACE_COMMAND_PROTO(x)	void x(int, char *, char *);
#define HSPACE_COMMAND_HDR(x)	void x(int player, char *arg_left, char *arg_right)

// A command entry in the array
typedef struct
{
	int key;
	void (*func)(int, char *, char *);
	int  perms;
}HSPACE_COMMAND;

extern HSPACE_COMMAND hsSpaceCommandArray[];
extern HSPACE_COMMAND hsNavCommandArray[];
extern HSPACE_COMMAND hsEngCommandArray[];
extern HSPACE_COMMAND hsConCommandArray[];
extern HSPACE_COMMAND *hsFindCommand(int, HSPACE_COMMAND *);

#endif