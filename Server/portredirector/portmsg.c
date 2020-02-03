/* This taken from the ftp.pennmush.org site with permission
 * Provided as-is as per the disclaimer below */
/*
 *	portmsg - generate a message on a port, then close connection
 *
 *	Usage:  portmsg file port
 *
 *		When a telnet client connects to the specified port, the
 *		text from the file will be echoed to the user.  After a
 *		short delay the connection will close.
 *
 * Derived from ftpd by Klaas @ {RUD, LPSwat}, original ftpd copyright
 * message follows:
 *
 * Copyright (c) 1985, 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/wait.h>

wait_on_child()
{
    union wait status;

    while (wait3(&status, WNOHANG, (struct rusage *) 0) > 0)
	;
}

lostconn()
{
    exit(1);
}

main(argc, argv)
        int argc;
        char *argv[];
{
    int msgfd, fd, n;
    struct stat statBuf;
    int port;
    char *msg;
    int sockfd, newsockfd;
    int addrlen; int opt;
    struct sockaddr_in tcp_srv_addr;
    struct sockaddr_in their_addr;

    if (argc != 3) {
	fprintf(stderr, "Usage: portmsg file port\n");
	exit(1);
    }

    port = atoi(argv[2]);
    if (port == 0) {
	fprintf(stderr, "error: bad port number [%s]\n", argv[2]);
	exit(1);
    }
    if ((msgfd = open(argv[1], O_RDONLY)) < 0) {
	fprintf(stderr, "error: cannot open message file [%s]\n", argv[1]);
	exit(1);
    }
    /* read the message */
    fstat(msgfd, &statBuf);
    if (statBuf.st_size <= 0) {
	fprintf(stderr, "error: message file [%s] is empty\n", argv[1]);
	exit(1);
    }
    msg = (char *)malloc(statBuf.st_size);
    if (read(msgfd, msg, statBuf.st_size) != statBuf.st_size) {
	fprintf(stderr, "error: cannot read message file [%s]\n", argv[1]);
	exit(1);
    }

    /* become a daemon */
    switch(fork()) {
    case -1:
	fprintf(stderr, "error: can't fork\n");
	exit(1);
    case 0:
	break;
    default:
	exit(0);
    }
    if (setpgrp(0, getpid()) == -1) {
	fprintf(stderr, "error: can't change process group\n");
	exit(1);
    }
    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
	ioctl(fd, TIOCNOTTY, NULL);
	close(fd);
    }

    (void)signal(SIGCLD, (void*)wait_on_child);
    bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
    tcp_srv_addr.sin_family = AF_INET;
    tcp_srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_srv_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "can't create stream socket\n");
	exit(-1);
    }
    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		 (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(1);
    }
    if (bind(sockfd, (struct sockaddr *)&tcp_srv_addr,
	     sizeof(tcp_srv_addr)) < 0) {
	fprintf(stderr, "can't bind local address\n");
	exit(-1);
    }
    listen(sockfd, 5);

main_again:
    addrlen = sizeof (their_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &their_addr, &addrlen);
    if (newsockfd < 0) {
	if (errno == EINTR)
	    goto main_again;
	fprintf(stderr, "accept error\n");
	exit(-1);
    }

    switch(fork()) {
    case -1:
	fprintf(stderr, "server can't fork\n");
	exit(-1);
    case 0:
	dup2(newsockfd, 0);
	dup2(newsockfd, 1);
	for (n = 3; n < NOFILE; n++)
	    close(n);
	break;
    default:
	close(newsockfd);
	goto main_again;
    }

    /* daemon child arrives here */
    (void)signal(SIGPIPE, (void*)lostconn);
    (void)signal(SIGCHLD, SIG_IGN);

    fprintf(stdout, msg);
    (void)fflush(stdout);
    sleep(5);
    exit(0);
}



