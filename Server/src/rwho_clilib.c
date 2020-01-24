/*
	Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
	
	Mod history:
	
	ANSIfied function headers, 9/91, glenn@ces.cwru.edu
	STREAMS/TLIified, 7/93, vasoll@a.cs.okstate.edu
*/

/*
code to interface client MUDs with rwho server

this is a standalone library.
*/

#ifdef SOLARIS
/* Solaris has broken header declarations */
void bcopy(const void *, void *, int);
void bzero(void *, int);
#endif


#include	"autoconf.h"

#include	<sys/file.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<netdb.h>

#ifdef TLI
#include <sys/stream.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <netinet/netinet.h>
#else
#include <arpa/inet.h>
#endif

#include "mudconf.h"

#ifndef	NO_HUGE_RESOLVER_CODE
#ifndef EXTENDED_SOCKET_DCLS
extern	struct	hostent	*	FDECL(gethostbyname, (const char *));
#endif
#endif

static	int	dgramfd = -1;
static	char	*password;
static	char	*localnam;
static	char	*lcomment;
static	struct	sockaddr_in	addr;

#ifdef TLI
static struct t_unitdata *pbuf;
#endif


/* enable RWHO and send the server a "we are up" message */
int rwhocli_setup(char *server, int dgramport, char *serverpw, 
		char *myname, char *comment)
{
#ifndef	NO_HUGE_RESOLVER_CODE
	struct	hostent		*hp;
#endif
#ifndef TLI
	char			pbuf[512];
#endif
	char			*p;

	if(dgramfd != -1)
		return(1);

	password = malloc(strlen(serverpw) + 1);
	localnam = malloc(strlen(myname) + 1);
	lcomment = malloc(strlen(comment) + 1);
	if(password == (char *)0 || localnam == (char *)0 || lcomment == (char *)0)
		return(1);
	strcpy(password,serverpw);
	strcpy(localnam,myname);
	strcpy(lcomment,comment);

#ifdef TLI
#endif
	p = server;
	while(*p != '\0' && (*p == '.' || isdigit((int)*p)))
		p++;

	if(*p != '\0') {
#ifndef	NO_HUGE_RESOLVER_CODE
		if((hp = gethostbyname(server)) == (struct hostent *)0)
			return(1);
		(void)bcopy(hp->h_addr,(char *)&addr.sin_addr,hp->h_length);
#else
		return(1);
#endif
	} else {
		/* unsigned long	f; */
		in_addr_t f;

		if((f = inet_addr(server)) == -1L)
			return(1);
		(void)bcopy((char *)&f,(char *)&addr.sin_addr,sizeof(f));
	}

	addr.sin_port = htons(dgramport);
	addr.sin_family = AF_INET;

#ifdef TLI
	if ((dgramfd = t_open (TLI_UDP, O_RDWR, NULL)) < 0)
		return (1);
	if (t_bind (dgramfd, NULL, NULL) < 0) {
		t_close (dgramfd);
		dgramfd = -1;
		return (1);
	}
	pbuf = (struct t_unitdata *)t_alloc (dgramfd, T_UNITDATA, T_ALL);
	if (pbuf == NULL) {
		t_close (dgramfd);
		dgramfd = -1;
		return (1);
	}

	bcopy(&addr, pbuf->addr.buf, sizeof(struct sockaddr_in));
	pbuf->addr.len = sizeof (struct sockaddr_in);

	sprintf(pbuf->udata.buf,"U\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.25s",
		localnam,password,localnam,mudstate.now,comment);
	pbuf->udata.len = strlen(pbuf->udata.buf);
	t_sndudata (dgramfd, pbuf);
#else
	if((dgramfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		return(1);

	sprintf(pbuf,"U\t%.20s\t%.20s\t%.20s\t%.10ld\t0\t%.25s",
		localnam,password,localnam,mudstate.now,comment);
	sendto(dgramfd,pbuf,strlen(pbuf),0,(void *)&addr,sizeof(addr));
#endif
	return(0);
}

/* disable RWHO */
int NDECL(rwhocli_shutdown)
{
#ifndef TLI
	char	pbuf[512];
#endif

	if(dgramfd != -1) {
#ifdef TLI
		sprintf(pbuf->udata.buf,"D\t%.20s\t%.20s\t%.20s",
			localnam, password,localnam);
		pbuf->udata.len = strlen(pbuf->udata.buf);
		t_sndudata (dgramfd, pbuf);
		t_close(dgramfd);
#else
		sprintf(pbuf,"D\t%.20s\t%.20s\t%.20s",
			localnam, password, localnam);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(void *)&addr,sizeof(addr));
		close(dgramfd);
#endif
		dgramfd = -1;
		free(password);
		free(localnam);
	}
	return(0);
}

/* send an update ping that we're alive */
int NDECL(rwhocli_pingalive)
{
#ifndef TLI
	char	pbuf[512];
#endif

	if(dgramfd != -1) {
#ifdef TLI
		sprintf(pbuf->udata.buf,"M\t%.20s\t%.20s\t%.20s\t%.10ld\t0\t%.25s",
			localnam, password, localnam, mudstate.now, lcomment);
		pbuf->udata.len = strlen(pbuf->udata.buf);
		t_sndudata (dgramfd, pbuf);
#else
		sprintf(pbuf,"M\t%.20s\t%.20s\t%.20s\t%.10ld\t0\t%.25s",
			localnam, password, localnam,mudstate.now, lcomment);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(void *)&addr,sizeof(addr));
#endif
	}
	return(0);
}

/* send a "so-and-so-logged in" message */
int rwhocli_userlogin(char *uid, char *name, time_t tim)
{
#ifndef TLI
	char	pbuf[512];
#endif

	if(dgramfd != -1) {
#ifdef TLI
		sprintf(pbuf->udata.buf,"A\t%.20s\t%.20s\t%.20s\t%.20s\t%.10ld\t0\t%.20s",
			localnam, password, localnam, uid, tim, name);
		pbuf->udata.len = strlen(pbuf->udata.buf);
		t_sndudata (dgramfd, pbuf);
#else
		sprintf(pbuf,"A\t%.20s\t%.20s\t%.20s\t%.20s\t%.10ld\t0\t%.20s",
			localnam, password, localnam, uid, tim, name);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(void *)&addr,sizeof(addr));
#endif
	}
	return(0);
}

/* send a "so-and-so-logged out" message */
int rwhocli_userlogout(char *uid)
{
#ifndef TLI
	char	pbuf[512];
#endif

	if(dgramfd != -1) {
#ifdef TLI
		sprintf(pbuf->udata.buf,"Z\t%.20s\t%.20s\t%.20s\t%.20s",
			localnam, password, localnam, uid);
		pbuf->udata.len = strlen(pbuf->udata.buf);
		t_sndudata (dgramfd, pbuf);
#else
		sprintf(pbuf,"Z\t%.20s\t%.20s\t%.20s\t%.20s",
			localnam, password, localnam, uid);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(void *)&addr,sizeof(addr));
#endif
	}
	return(0);
}
