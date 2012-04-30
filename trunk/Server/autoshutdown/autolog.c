/********************************************************************************
 * Author:  Ashen-Shugar
 *
 * Autologin script for mushes.  This is used to log into the mush and issue
 * a shutdown that brings the mush down via script and/or automation.
 * The includes may have to be tweeked.
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "autolog.h"

main()
{
  struct sockaddr_in sin;
  struct hostent *hp;
  char buf[10000];
  int fd;
 
  memset((char *) &sin, '\0', sizeof(sin));
 
  sin.sin_port = htons(MUDPORT);
 
  if (isdigit (*(MUDHOST))) {
     sin.sin_addr.s_addr = inet_addr (MUDHOST);
     sin.sin_family = AF_INET;
  } else { 
     if ((hp = gethostbyname(MUDHOST)) == 0) 
        return (-1);
     memcpy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
     sin.sin_family = hp->h_addrtype;
  }
 
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
     return -1;

  if (connect(fd,(struct sockaddr *) &sin, sizeof(sin)) < 0) 
     return -1;

  /* Give time to connect */
  sleep(10); 
  memset(buf, '\0', sizeof(buf));
  sprintf(buf, "%s %s %s\n", CONNECTION, PLAYER, PASSWD); 
  write(fd, buf, LBUF_SIZE-1);
  fsync(fd);
  /* Give time to finish connection */
  sleep(5);
  if ( WALL ) {
     memset(buf, '\0', sizeof(buf));
     sprintf(buf, "@wall %s\n", WALL_MESSAGE);
     write(fd, buf, LBUF_SIZE-1);
     fsync(fd);
  }
  sleep(SHUTDOWN_DELAY);
  memset(buf, '\0', sizeof(buf));
  sprintf(buf, "@shutdown\n");
  write(fd, buf, LBUF_SIZE-1);
  fsync(fd);
  sleep(1);
  exit(0);
}
