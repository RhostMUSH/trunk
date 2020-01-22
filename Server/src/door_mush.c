#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "copyright.h"
#include "autoconf.h"
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "interface.h"
#include "door.h"

extern int parse_dynhelp(dbref, dbref, int, char *, char *, char *, char *, int, int, char *);

int mushDoorOpen(DESC *d, int nArgs, char *args[], int id) {
  dbref player = d->player;
  int sock, i_found;
  char *t_buff, *t_bufptr, *s_addy, *s_port, *s_strtok;

  i_found = 1;
  t_bufptr = t_buff = alloc_lbuf("mush_doors");
  s_port = s_strtok = NULL;
  parse_dynhelp(player, player, 0, (char *)"doors", args[0], t_buff, t_bufptr, 1, 0, (char *)NULL); 
  if ( t_buff && *t_buff && 
       ((strstr(t_buff, (char *)"No entry for") != NULL) ||
        (strstr(t_buff, (char *)"Here are the entries which match") != NULL) ||
        (strstr(t_buff, (char *)"Sorry, that file") != NULL)) ) 
     i_found = 0;
  if ( i_found && *t_buff && args[0] && *args[0]) {
     s_addy = strtok_r(t_buff, " \t\r\n", &s_strtok);
     if ( s_addy )
        s_port = strtok_r(NULL, " \t\r\n", &s_strtok);
     if ( s_addy && s_port )
        sock = door_tcp_connect(s_addy, s_port, d, id, 1);
     else
        sock = -1;
     if (sock < 0) {
       queue_string(d, "Mush connection failed\r\n");
       free_lbuf(t_buff);
       return -1;
     } else {
       queue_string(d, "*** CONNECTED ***\r\n");
     }
  } else if ( !i_found || (i_found && args[0] && *args[0]) ) {
     if ( strstr(t_buff, (char *)"Sorry, that file") != NULL )
        queue_string(d, "There are currently no mush doors configured.\r\n");
     else
        queue_string(d, "Unrecognized mush.  Can not establish connection.\r\n");
     free_lbuf(t_buff);
     return -1;
  } else if ( i_found && (!args[0] || !*args[0]) && t_buff ) {
     queue_string(d, t_buff);
  }
  free_lbuf(t_buff);
  return 1;
}
