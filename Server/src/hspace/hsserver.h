#ifndef __HSSERVER_INCLUDED__
#define __HSSERVER_INCLUDED__

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "hstypes.h"

// 1 digit server commands
#define SC1_OBJECT		1
#define SC1_CLASS		2
#define SC1_WEAPON		3
#define SC1_UNIVERSE	4
#define SC1_MESSAGE		5

// 3 digit server commands
#define SC3_RETRIEVE	100	// Retrieve specified info
#define SC3_INFO		101	// Information as follows
#define SC3_ALL			102
#define SC3_NUMBER_OF	103 // Count of requested info
#define SC3_DETAILS		104 // Information details follow
#define SC3_ENG_SYSTEM  105 // Selected eng system attrs
#define SC3_MODIFY		106 // Modify values as follows
#define SC3_ADD			107 // Add following info
#define SC3_DELETE		108 // Delete following info

#define HS_MAX_OPEN_SOCKS	32

class CHSServer
{
public:
	CHSServer(void);

	BOOL StartServer(int, int rebooting = 0);
	BOOL ShutdownServer(void);
	void DoCycle(void);

protected:
	// Attributes
	int m_sock;
#ifdef WIN32
	SOCKADDR_IN sin;
#else
	struct sockaddr_in sin;
#endif
	BOOL m_bOpenStat;
	int maxd;
	fd_set input_set, output_set;
	int m_opensocks[HS_MAX_OPEN_SOCKS];
	char *m_sockdata[HS_MAX_OPEN_SOCKS];

	// Methods
	void AddNewSocket(int);
	int  DesToArraySlot(int);
	void RemoveSocket(int);
	BOOL ProcessInput(int);
	void ProcessCommand(char *, int);
	void ProcessObjectCommand(char *, int);
	void ProcessClassCommand(char *, int);
	void RetrieveClassInfo(char *, int);
	void ProcessWeaponCommand(char *, int);
	void RetrieveWeaponInfo(char *, int);
	void RetrieveObjectInfo(char *, int);
	void ModifyClassInfo(char *, int);
	int  SvrCmd(char *, int);
	void SendCommand(char *, int);
	void SendAllCommand(char *);
};

extern CHSServer HSServer;

#endif // __HSSERVER_INCLUDED__
