#include "hscopyright.h"
#include "hsserver.h"
#include "hsutils.h"
#include "hsclass.h"
#include "hsweapon.h"
#include "hsuniverse.h"
#include "hseng.h"

#ifdef WIN32
#include <io.h>
#endif

#ifndef WIN32
#include <strings.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

CHSServer HSServer;		// One instance of this .. right here!

CHSServer::CHSServer(void)
{
    m_sock = -1;
    m_bOpenStat = FALSE;

    for (int idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	m_opensocks[idx] = -1;
	m_sockdata[idx] = NULL;
    }
}


BOOL CHSServer::StartServer(int port, int rebooting)
{
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!rebooting) {
	sin.sin_family = AF_INET;
	sin.sin_port = htons((u_short) port);

	int opt;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,
		       (char *) &opt, sizeof(opt)) < 0) {
	    hs_log("SETSOCKOPT FAILED");
	    return FALSE;
	}

	if (m_sock < 0) {
	    hs_log("SOCK FAILED");
	    return FALSE;
	}

	/* Bind our structure to the socket */
	if (bind(m_sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	    hs_log("BIND FAILED");
	    return FALSE;
	}

	if (listen(m_sock, 1) < 0) {
	    hs_log("LISTEN FAILED");
	    return FALSE;
	}

    }
    maxd = m_sock + 1;

    FD_ZERO(&input_set);
    FD_ZERO(&output_set);

    m_bOpenStat = TRUE;

    return TRUE;
}

void CHSServer::DoCycle()
{
    struct timeval timeout;
    int found;
    int idx;

    if (!m_bOpenStat)
	return;

    FD_ZERO(&input_set);
    FD_SET(m_sock, &input_set);

    // Include our active descriptors in the list
    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	if (m_opensocks[idx] != -1)
	    FD_SET(m_opensocks[idx], &input_set);
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    /* See if anyone has requested a connection to us or has input ready */
    found = select(maxd, &input_set, &output_set, (fd_set *) 0, &timeout);

    if (found && FD_ISSET(m_sock, &input_set)) {
	/* New Connection */
#ifdef WIN32
	int size;
#else
	socklen_t size;
#endif
	int newdes;

	notify(73, "HERE");
	size = sizeof(struct sockaddr);
	newdes = accept(m_sock, (struct sockaddr *) &sin, &size);
	if (newdes > 0) {
	    char *host;
	    char tbuf[64];
	    host = (char *) inet_ntoa(sin.sin_addr);
	    strcpy(tbuf, host);
	    // Add it to our list of sockets
	    AddNewSocket(newdes);

//                      addr = sin.sin_addr;
//                      /* Set the host name or IP address for this connection */
//                      hnt = gethostbyaddr((char *) &addr, sizeof(struct in_addr), AF_INET);
//                      if (hnt)
//                        strcpy(str, hnt->h_name);
//                      else {
//                        a = ntohl(addr.s_addr);
//                        sprintf(str, "%d.%d.%d.%d", (a >> 24) & 0xff,
//                              (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff);
//                      }
	    maxd = newdes + 1;
	}
    }
    // Process any input coming in.
    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	if (m_opensocks[idx] != -1) {
	    if (FD_ISSET(m_opensocks[idx], &input_set)) {
		if (!ProcessInput(m_opensocks[idx])) {
		    shutdown(m_opensocks[idx], 2);
		    close(m_opensocks[idx]);
		    RemoveSocket(m_opensocks[idx]);
		}
	    }
	}
    }
}

int CHSServer::DesToArraySlot(int des)
{
    int idx;

    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	if (m_opensocks[idx] == des)
	    return idx;
    }

    return -1;
}

void CHSServer::AddNewSocket(int des)
{
    int idx;

#ifdef WIN32
    unsigned long arg = 1;
    ioctlsocket(des, FIONBIO, &arg);
#else
    fcntl(des, F_SETFL, O_NDELAY);
#endif

    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	// If the slot is = -1, then fill it
	if (m_opensocks[idx] == -1) {
	    m_opensocks[idx] = des;
	    return;
	}
    }

    // If we got here, all open descriptors are full.  OUCH.
}

void CHSServer::RemoveSocket(int des)
{
    int idx;

    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	if (m_opensocks[idx] == des) {
	    m_opensocks[idx] = -1;
	    return;
	}
    }
}

BOOL CHSServer::ProcessInput(int des)
{
    int nread;
    char tbuf[512];

#ifdef WIN32
    nread = recv(des, tbuf, 511, 0);
#else
    nread = read(des, tbuf, 511);
#endif

#ifdef WIN32
    if (nread == 0 || nread == SOCKET_ERROR) {
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	    return FALSE;
    }
#else
    if (nread < 0 && errno != EWOULDBLOCK)
	return FALSE;
#endif

    tbuf[nread] = '\0';

    char cmdbuf[2048];		// Used to store the full command.

    if (nread > 0) {
	// If there is already information in the sockdata
	// array, pull that out.
	int aidx;
	aidx = DesToArraySlot(des);
	if (aidx != -1 && m_sockdata[aidx]) {
	    // Data already stored in the sockdata array,
	    // which means we need to add the new data to
	    // it.
	    sprintf(cmdbuf, "%s%s", m_sockdata[aidx], tbuf);

	    delete[]m_sockdata[aidx];
	    m_sockdata[aidx] = NULL;
	} else
	    strcpy(cmdbuf, tbuf);

	// Run through the string, pulling out commands
	// terminated by a newline.  Whatever is left, we
	// store in the sockdata array, awaiting more input
	// to come.
	char *storeptr;
	char *ptr;
	char *dptr;
	char cmd[2048];
	storeptr = cmdbuf;
	dptr = cmd;

	for (ptr = cmdbuf; *ptr; ptr++) {
	    if (*ptr == '\n') {
		*dptr = '\0';
		if (*cmd && *cmd != '\n')
		    ProcessCommand(cmd, des);
		dptr = cmd;

		storeptr = ptr + 1;
	    } else
		*dptr++ = *ptr;
	}
	if (*storeptr) {
	    m_sockdata[aidx] = new char[strlen(storeptr) + 1];
	    strcpy(m_sockdata[aidx], storeptr);
	}
    }
    return TRUE;
}

// Processes a single command from the client
void CHSServer::ProcessCommand(char *strCmd, int des)
{
    if (!strCmd || !*strCmd)
	return;

    // Look at the first digit of the command,
    // and call the appropriate handler.
    int dig1;
    dig1 = strCmd[0] - '0';

    char *ptr;
    ptr = strCmd + 1;
    switch (dig1) {
    case SC1_OBJECT:
	ProcessObjectCommand(ptr, des);
	break;

    case SC1_CLASS:
	ProcessClassCommand(ptr, des);
	break;

    case SC1_WEAPON:
	ProcessWeaponCommand(ptr, des);
	break;

    default:
	hs_log(strCmd);
	// Do nothing;
	break;
    }
}

// Specifically processes an object command.
void CHSServer::ProcessObjectCommand(char *strCmd, int des)
{
    int cmd;

    cmd = SvrCmd(strCmd, 3);
    switch (cmd) {
    case SC3_RETRIEVE:
	char *ptr;
	ptr = strCmd + 3;
	RetrieveObjectInfo(ptr, des);
	break;
    }
}

void CHSServer::RetrieveObjectInfo(char *strCmd, int des)
{
    int cmd;
    cmd = SvrCmd(strCmd, 3);

    switch (cmd) {
    case SC3_NUMBER_OF:
	char retcmd[32];
	CHSUniverse *cUniv;
	int nObjs;
	int i;

	// Number of what type?
	char *ptr;
	ptr = strCmd + 3;
	HS_TYPE type;
	type = (HS_TYPE) (*ptr - '0');

	nObjs = 0;
	for (i = 0; i < HS_MAX_UNIVERSES; i++) {
	    cUniv = uaUniverses.GetUniverse(i);
	    if (cUniv) {
		nObjs += cUniv->NumObjects(type);
	    }
	}

	sprintf(retcmd, "%d%d%d%d%d",
		SC1_OBJECT, SC3_INFO, SC3_NUMBER_OF, type, nObjs);

	SendCommand(retcmd, des);
	break;
    }
}

// Specifically processes a class command
void CHSServer::ProcessClassCommand(char *strCmd, int des)
{
    int cmd;
    char *ptr;
    int idx;
    HSSHIPCLASS *pClass;
    char retcmd[256];

    cmd = SvrCmd(strCmd, 3);
    switch (cmd) {
    case SC3_RETRIEVE:
	ptr = strCmd + 3;
	RetrieveClassInfo(ptr, des);
	break;

    case SC3_MODIFY:
	ptr = strCmd + 3;
	ModifyClassInfo(ptr, des);
	break;

    case SC3_DELETE:
	ptr = strCmd + 3;

	// Find the class
	for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	    pClass = caClasses.GetClass(idx);
	    if (pClass && !strcmp(pClass->m_name, ptr))
		break;
	}
	if (!pClass) {
	    // Class not found
	    sprintf(retcmd,
		    "%dRequested class to delete - not found.",
		    SC1_MESSAGE);
	    SendCommand(retcmd, des);
	    return;
	}

	if (!caClasses.RemoveClass(idx)) {
	    sprintf(retcmd,
		    "%dFailed to delete specified class.", SC1_MESSAGE);
	    SendCommand(retcmd, des);
	    return;
	}
	// Tell all clients to delete the class
	sprintf(retcmd, "%d%d%s", SC1_CLASS, SC3_DELETE, ptr);
	SendAllCommand(retcmd);
	break;

    case SC3_ADD:
	ptr = strCmd + 3;	// Name of class
	pClass = caClasses.NewClass();
	if (!pClass) {
	    sprintf(retcmd, "%dFailed to add new class.", SC1_MESSAGE);
	    SendCommand(retcmd, des);
	} else {
	    // Zero out the class
	    memset(pClass, 0, sizeof(HSSHIPCLASS));

	    // Set the name
	    strcpy(pClass->m_name, ptr);
	    // Send the info to the clients
	    sprintf(retcmd,
		    "%d%d%d%s,SIZE=%d,NUM CREWS=%d,CAN DROP=%s,CARGO SIZE=%d,MAX HULL=%d",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS,
		    pClass->m_name,
		    pClass->m_size,
		    pClass->m_ncrews,
		    pClass->m_can_drop ? "YES" : "NO",
		    pClass->m_cargo_size, pClass->m_maxhull);
	    SendAllCommand(retcmd);
	}
	break;
    }
}

void CHSServer::ModifyClassInfo(char *strCmd, int des)
{
    int cmd;
    char tbuf[512];
    int idx;
    char *ptr;
    char classname[64];
    HSSHIPCLASS *sclass;
    char attr[32];
    char value[64];
    int iVal;

    // What to modify?
    cmd = SvrCmd(strCmd, 3);

    switch (cmd) {
    case SC3_DETAILS:
	ptr = strCmd + 3;

	// Pull out the class name, and find the class
	extract(ptr, classname, 0, 1, ',');

	// Find the class
	for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	    sclass = caClasses.GetClass(idx);
	    if (sclass && !strcmp(sclass->m_name, classname))
		break;
	}
	if (!sclass)
	    return;		// Class not found;

	// Pull out the attribute and value pair;
	extract(ptr, tbuf, 1, 1, ',');
	extract(tbuf, attr, 0, 1, '=');
	extract(tbuf, value, 1, 1, '=');

	// Match the attribute name
	*tbuf = '\0';
	if (!strcasecmp(attr, "SIZE")) {
	    iVal = atoi(value);
	    sclass->m_size = iVal;

	    sprintf(tbuf,
		    "%d%d%d%s,SIZE=%d",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS, classname, iVal);
	} else if (!strcasecmp(attr, "NUM CREWS")) {
	    iVal = atoi(value);
	    sclass->m_ncrews = iVal;

	    sprintf(tbuf,
		    "%d%d%d%s,NUM CREWS=%d",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS, classname, iVal);
	} else if (!strcasecmp(attr, "CAN DROP")) {
	    iVal = atoi(value);
	    sclass->m_can_drop = iVal;

	    sprintf(tbuf,
		    "%d%d%d%s,CAN DROP=%s",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS,
		    classname, iVal ? "YES" : "NO");
	} else if (!strcasecmp(attr, "CARGO SIZE")) {
	    iVal = atoi(value);
	    sclass->m_cargo_size = iVal;

	    sprintf(tbuf,
		    "%d%d%d%s,CARGO SIZE=%d",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS, classname, iVal);
	} else if (!strcasecmp(attr, "MAX HULL")) {
	    iVal = atoi(value);
	    sclass->m_maxhull = iVal;

	    sprintf(tbuf,
		    "%d%d%d%s,MAX HULL=%d",
		    SC1_CLASS, SC3_INFO, SC3_DETAILS, classname, iVal);
	}
	if (*tbuf)
	    SendAllCommand(tbuf);

	break;

    case SC3_ENG_SYSTEM:
	ptr = strCmd + 3;

	// Get system type
	HSS_TYPE type;
	type = (HSS_TYPE) SvrCmd(ptr, 3);

	// Get class name
	ptr += 3;
	extract(ptr, classname, 0, 1, ',');

	// Find the class
	for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	    sclass = caClasses.GetClass(idx);
	    if (sclass && !strcmp(sclass->m_name, classname))
		break;
	}
	if (!sclass)
	    return;		// Class not found;

	// Find the system on the class
	CHSEngSystem *cSys;
	cSys = sclass->m_systems.GetSystem(type);
	if (!cSys)
	    return;

	// Get attr name and value.
	extract(ptr, tbuf, 1, 1, ',');
	extract(tbuf, attr, 0, 1, '=');
	extract(tbuf, value, 1, 1, '=');
	cSys->SetAttributeValue(attr, value);

	// Send out information to all clients
	sprintf(tbuf,
		"%d%d%d%d%d%d%s,%s",
		SC1_CLASS, SC3_INFO, SC3_ENG_SYSTEM,
		type / 100,
		(type % 100) / 10,
		type % 10, classname, cSys->GetAttrValueString());
	SendAllCommand(tbuf);
	break;
    }
}

// Specifically processes a weapon command
void CHSServer::ProcessWeaponCommand(char *strCmd, int des)
{
    int cmd;

    cmd = SvrCmd(strCmd, 3);
    switch (cmd) {
    case SC3_RETRIEVE:
	char *ptr;
	ptr = strCmd + 3;
	RetrieveWeaponInfo(ptr, des);
	break;
    }
}

void CHSServer::SendAllCommand(char *strCmd)
{
    int idx;

    for (idx = 0; idx < HS_MAX_OPEN_SOCKS; idx++) {
	if (m_opensocks[idx] != -1)
	    SendCommand(strCmd, m_opensocks[idx]);
    }
}

void CHSServer::SendCommand(char *strCmd, int des)
{
    send(des, strCmd, strlen(strCmd), 0);
    send(des, "\n", 1, 0);
}

void CHSServer::RetrieveClassInfo(char *strCmd, int des)
{
    int cmd;
    char retcmd[512];
    int idx;
    HSSHIPCLASS *sclass;

    cmd = SvrCmd(strCmd, 3);

    switch (cmd) {
    case SC3_NUMBER_OF:
	sprintf(retcmd, "%d%d%d%d",
		SC1_CLASS, SC3_INFO, SC3_NUMBER_OF,
		caClasses.NumClasses());
	SendCommand(retcmd, des);
	break;

    case SC3_ALL:
	for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	    sclass = caClasses.GetClass(idx);
	    if (sclass) {
		sprintf(retcmd,
			"%d%d%d%s,SIZE=%d,NUM CREWS=%d,CAN DROP=%s,CARGO SIZE=%d,MAX HULL=%d",
			SC1_CLASS, SC3_INFO, SC3_DETAILS,
			sclass->m_name,
			sclass->m_size,
			sclass->m_ncrews,
			sclass->m_can_drop ? "YES" : "NO",
			sclass->m_cargo_size, sclass->m_maxhull);
		SendCommand(retcmd, des);
	    }
	}
	break;

    case SC3_ENG_SYSTEM:
	HSS_TYPE sys;
	CHSEngSystem *cSys;

	// Which eng system?
	char *ptr;
	ptr = strCmd + 3;
	sys = (HSS_TYPE) SvrCmd(ptr, 3);

	// Which class name?
	ptr += 3;		// Class name is remainder of string

	for (idx = 0; idx < HS_MAX_CLASSES; idx++) {
	    sclass = caClasses.GetClass(idx);
	    if (sclass && !strcmp(sclass->m_name, ptr))
		break;
	}
	if (!sclass)
	    return;		// Class not found;

	switch (sys) {
	case SC3_ALL:		// All system info
	    cSys = sclass->m_systems.GetHead();
	    int type;
	    while (cSys) {
		type = cSys->GetType();
		sprintf(retcmd,
			"%d%d%d%d%d%d%s,%s",
			SC1_CLASS, SC3_INFO, SC3_ENG_SYSTEM,
			type / 100,
			(type % 100) / 10,
			type % 10,
			sclass->m_name, cSys->GetAttrValueString());
		SendCommand(retcmd, des);

		cSys = cSys->GetNext();
	    }
	    break;

	default:
	    // Grab the specific system and return info
	    cSys = sclass->m_systems.GetSystem(sys);
	    if (cSys) {
		sprintf(retcmd,
			"%d%d%d00%d%s,%s",
			SC1_CLASS, SC3_INFO, SC3_ENG_SYSTEM,
			cSys->GetType(),
			sclass->m_name, cSys->GetAttrValueString());
		SendCommand(retcmd, des);
	    }
	    break;
	}
	break;
    }
}

void CHSServer::RetrieveWeaponInfo(char *strCmd, int des)
{
    int cmd;
    cmd = SvrCmd(strCmd, 3);

    switch (cmd) {
    case SC3_NUMBER_OF:
	char retcmd[32];
	sprintf(retcmd, "%d%d%d%d",
		SC1_WEAPON, SC3_INFO, SC3_NUMBER_OF,
		waWeapons.NumWeapons());
	SendCommand(retcmd, des);
	break;
    }
}

int CHSServer::SvrCmd(char *str, int nchars)
{
    int val;
    char *ptr;
    int multiplier;

    val = 0;

    // Error check
    if (strlen(str) < nchars)
	nchars = strlen(str);

    ptr = str + nchars - 1;
    multiplier = 1;
    while (nchars > 0) {
	val += ((*ptr - '0') * multiplier);
	multiplier *= 10;
	ptr--;
	nchars--;
    }

    return val;
}
