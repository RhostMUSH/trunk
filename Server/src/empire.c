/* empire client door */

#include "copyright.h"
#include "autoconf.h"
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "interface.h"
#include "empire.h"
#include "door.h"
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

extern int do_command(DESC *, char *);
extern int NDECL(next_timer);

static struct fn fnlist[] = {
	{ NULL,	"user",	USER,},
	{ NULL,	"coun",	COUN,},
	{ NULL,	"quit",	QUIT,},
	{ NULL,	"pass",	PASS,},
	{ NULL,	"play",	PLAY,},
	{ NULL,	"list",	LIST,},
	{ NULL,	"cmd",	CMD,},
	{ NULL,	"ctld",	CTLD,},
	{ NULL, "kill", KILL,},
	{ NULL,	"",	0,},
};

int
expect(int s, int match, char *buf)
{
	int	size;
	char	*p;
	int	n;
	int	code;
	int	newline;
	char	*ptr;
	int	cc;

	size = 1024;
	alarm(30);
	ptr = buf;
	n = recv(s, ptr, size, MSG_PEEK);
	if (n <= 0) {
		mudstate.alarm_triggered = 0;
		alarm(next_timer());
		return 0;
	}
	size -= n;
	buf[n] = '\0';
	if ((p = strchr(ptr, '\n')) == 0) {
		do {
			cc = read(s, ptr, n);
			if (cc < 0) {
				alarm(next_timer());
				return 0;
			}
			if (cc != n) {
				alarm(next_timer());
				return 0;
			}
			ptr += n;
			if ((n = recv(s, ptr, size, MSG_PEEK)) <= 0) {
				alarm(next_timer());
				return 0;
			}
			size -= n;
			ptr[n] = '\0';
		} while ((p = index(ptr, '\n')) == 0);
		newline = 1 + p - buf;
		*p = 0;
	} else
		newline = 1 + p - ptr;
	cc = read(s, buf, newline);
	if (cc < 0) {
		alarm(next_timer());
		return 0;
	}
	if (cc != newline) {
		alarm(next_timer());
		return 0;
	}
	buf[newline] = '\0';
	mudstate.alarm_triggered = 0;
	alarm(next_timer());
	if (!isxdigit((int)*buf)) {
		return 0;
	}
	if (isdigit((int)*buf))
		code = *buf - '0';
	else {
		if (isupper((int)*buf))
			*buf = tolower(*buf);
		code = 10 + *buf - 'a';
	}
	if (code == match)
		return 1;
	return 0;
}

int sendcmd(int s, int cmd, char *arg)
{
	char	buf[80];
	int	cc, len, ret;

	ret = 1;
	sprintf(buf, "%s %s\n", fnlist[cmd].name, arg != 0 ? arg : (char *)"");
	len = strlen(buf);
	cc = write(s, buf, len);
	if (cc < 0) {
		ret = 0;
	}
	if (cc != len) {
		ret = 0;
	}
	return (ret);
}

int empire_init(DESC *d, int nargs, char *args[], int id)
{
  int sock_req;
  char buf[1024], buf2[80];
  char *pt, *pt2, *country, *password;

  // get country and password
  if ((nargs != 2) || (*args[0] == '\0') || (*args[1] == '\0')) {
    queue_string(d,"Bad arguments to empire door.");
    return -1;
  } 
  country = args[0];
  password = args[1];
  sprintf(buf2, "%s %s", "localhost", "1665");

/*pt = doorparm("empire"); */
  pt = buf2;

  if (*pt == '\0') {
    queue_string(desc_in_use, "Read of door parameters failed.\r\n");
    return -1;
  }
  pt2 = strchr(pt,' ');
  if (pt2 == NULL) {
    queue_string(desc_in_use, "Bad format in door parameters file.\r\n");
    return -1;
  }
  *pt2 = '\0';
  sock_req = door_tcp_connect(pt,pt2+1, d, id);

  if (sock_req < 0) {
    queue_string(desc_in_use, "Connection to empire server failed.\r\n");
    return -1;
  }

  if (!expect(sock_req, C_INIT, buf)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 1).\r\n");
    close(sock_req);
    return -1;
  }
  strcpy(buf,Name(desc_in_use->player));
  strcat(buf,"@");
  strcat(buf,mudconf.mud_name);
  if (!sendcmd(sock_req, USER, buf)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 2).\r\n");
    goto abort;
  }
  if (!expect(sock_req, C_CMDOK, buf)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 3).\r\n");
    goto abort;
  }
  if (!sendcmd(sock_req, COUN, country)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 4).\r\n");
    goto abort;
  }
  if (!expect(sock_req, C_CMDOK, buf)) {
    queue_string(desc_in_use, "Login to empire server failed. Posible bad country name. (stage 5).\r\n");
    goto abort;
  }
  if (!sendcmd(sock_req, PASS, password)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 6).\r\n");
    goto abort;
  }
  if (!expect(sock_req, C_CMDOK, buf)) {
    queue_string(desc_in_use, "Login to empire server failed. Posible bad password. (stage 7).\r\n");
    goto abort;
  }
  if (!sendcmd(sock_req, PLAY, (char *)0)) {
    queue_string(desc_in_use, "Login to empire server failed (stage 8).\r\n");
    goto abort;
  }
  if (!expect(sock_req, C_INIT, buf)) {
    queue_string(desc_in_use, buf);
    queue_string(desc_in_use, "\r\n");
    queue_string(desc_in_use, "Login to empire server failed. (stage 9).\r\n");
    goto abort;
  }

  queue_string(desc_in_use, "\r\n\t-=O=-\r\n");
  process_output(desc_in_use);
  return 1;

 abort:
  free_lbuf(d->door_lbuf);
  free_mbuf(d->door_mbuf);
  close(sock_req);
  return -1;
}

void empire_prompt(DESC *d)
{
  char buf[1024];

  if (d->door_int1 == C_PROMPT)
    queue_string(d,"\r\n");
  sprintf(buf,"%s%s\r\n",d->door_mbuf,d->door_lbuf);
  queue_string(d,buf);
  process_output(d);
}

void empire_output(DESC *d, int code, char *buf)
{
  switch (code) {
    case C_NOECHO:
      break;	/* maybe do something later */
    case C_FLUSH:
      process_output(d);
      break;
    case C_ABORT:
      queue_string(d,"Aborted\r\n");
      break;
    case C_CMDERR:
    case C_BADCMD:
      queue_string(d,"Error; ");
      break;
    case C_EXIT:
      queue_string(d,"Exit: ");
      break;
    case C_FLASH:
      queue_string(d,"\r\n");
      break;
    default:
      break;
  }
  queue_string(d, buf);
  if (code != C_FLUSH)
    queue_string(d,"\r\n");
  else
    process_output(d);
}

int empire_from_empsrv(DESC *d, char *text)
{
  char *pt1, *pt2, *pt3;
  int code;

  pt1 = strtok(text,"\n");
  if (pt1 != NULL) {
    do {
      pt2 = pt1;
      while (*pt2 && !isspace((int)*pt2))
	pt2++;
      *pt2++ = '\0';
      if (isalpha((int)*pt1))
	code = 10 + (*pt1 - 'a');
      else
	code = *pt1 - '0';
      switch (code) {
	case C_PROMPT:
	  if (sscanf(pt2,"%d %d", &(d->door_int2), &(d->door_int3)) != 2)
	    queue_string(d, "empire: bad server prompt.\r\n");
	  d->door_int1 = code;
	  sprintf(d->door_lbuf, "[%d:%d] Command : ", d->door_int2, d->door_int3);
	  empire_prompt(d);
	  break;
	case C_REDIR:
	  queue_string(d,"empire: redirection not supported.\r\n");
	  break;
	case C_PIPE:
	  queue_string(d,"empire: pipe not supported.\r\n");
	  break;
	case C_FLUSH:
	  d->door_int1 = code;
	  strcpy(d->door_lbuf, pt2);
	  empire_prompt(d);
	  break;
	case C_EXECUTE:
	  pt3 = alloc_lbuf("door_exec_temp");
	  if (pt3 == NULL) {
	    queue_string(d,"Door exec failed.\r\n");
	    break;
	  }
	  strcpy(pt3,pt2);
	  do_command(d,pt3);
	  free_lbuf(pt3);
	  if (!(d->flags & DS_HAS_DOOR)) {
	    queue_string(d,"Exec send EOF failed; Empire door was closed.\r\n");
	    return -1;
	  }
	  else if (WRITE(d->door_desc,"ctld\n", 5) < 5) {
	    queue_string(d,"Exec send EOF failed\r\n");
	    return -1;
	  }
	  break;
	case C_INFORM:
	  if (*pt2) {
	    pt2[strlen(pt2)-1] = '\0';
	    sprintf(d->door_mbuf, "(%s)", pt2+1);
	    empire_prompt(d);
	  }
	  else
	    *(d->door_mbuf) = '\0';
	  break;
	default:
	  empire_output(d, code, pt2);
	  break;
      }
    } while ((pt1 = strtok(NULL,"\n")) != NULL);
  }
  return 1;
}
