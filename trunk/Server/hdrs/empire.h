#define CLIENTPROTO	2	/* if new things are added, bump this */

#define	USER		0
#define	COUN		1
#define	QUIT		2
#define	PASS		3
#define	PLAY		4
#define	LIST		5
#define	CMD		6
#define	CTLD		7
#define WAT		8
#define KILL		9
#define GKILL		10	/* kill even if process stuck --ts */

#define	C_CMDOK		0x0
#define	C_DATA		0x1
#define	C_INIT		0x2
#define	C_EXIT		0x3
#define C_FLUSH		0x4
#define	C_NOECHO	0x5
#define C_PROMPT	0x6
#define	C_ABORT		0x7
#define C_REDIR		0x8
#define C_PIPE		0x9
#define	C_CMDERR	0xA
#define	C_BADCMD	0xB
#define C_EXECUTE	0xC
#define C_FLASH		0xD
#define C_INFORM        0xE
#define C_LAST          0xE

struct	fn {
	int	(*func)();
	char	*name;
	int	value;
};
