/******************************************************************************
 *
 * A more advanced example of @DOOR code.
 *
 *    This code is more advanced than the simple mush connection one because
 *    it shows an example of handling custom read and write calls.
 *    In addition the door itself takes (and parses) arguments.
 *
 *    The door can be used to connect to a pop3 server and will then return
 *    a message informing the user of how many messages they have sitting
 *    in their mailbox.
 *
 *    Note: This check is done asynchronously. So if it takes a while (perhaps
 *          due to a congested server or slow internet) it won't lag the mush.
 *          It will however currently prevent the player from typing normally
 *          until finished. (To be fixed later)
 *
 *
 *  @copyright Lensman 2003.                            lensman@the-wyvern.net
 *
 *
 * Left to do:
 *   1) More comments
 *   2) Remove "s from username/password/host strings
 *   3) If I get really bored, maybe I'll make it bring back other info too.
 *
 *
 */



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

#define BAD_ARG_STRING "Username and password required: @door/open mail = user:pwd@host\r\n"

typedef enum mailCmd {
  OK_e, USER_e, PASS_e, STAT_e, DONE_e
} mailCmd_t;

typedef struct mail {
  dbref player;
  int   id;
  mailCmd_t status;
  char user[30];
  char pass[30];
  //  char *buff;
  struct mail *pNext;
} mail_t;

mail_t *pHead;

void addStruct(mail_t *p) {
  if (pHead == NULL) {
    pHead = p;
    pHead->pNext = NULL;
  } else {
    p->pNext = pHead;
    pHead = p;
  }
} 

void delStruct(mail_t *p) {
  mail_t *l = NULL, *t=NULL;
  t = pHead;
  while (t != NULL) {
    if (t->player == p->player) {
      break;
    }
    l = t;
    t = t->pNext;
  }

  if (t) {
    if (l) {
      l->pNext = t->pNext;
    } else {
      pHead = t->pNext;
    }
    free(p);
  }
}

mail_t * findStruct(int player) {
  mail_t *t = pHead;
  while (t != NULL) {
    if (t->player == player) {
      return t;
    }
    t = t->pNext;
  }
  return (mail_t *) NULL;
}

static int sendcmd(int s, char *buf)
{
  int	cc, len, ret;
  
  ret = 1;
  len = strlen(buf);
  cc = write(s, buf, len);
  if (cc < 0) {
    ret = 0;
  }
  if (cc != len) {
    ret = 0;
  }
#ifdef _MAILDEBUG
  queue_string(desc_in_use, buf);
#endif
  return (ret);
}

int mailDoorOpen(DESC *d, int nArgs, char *args[], int id) {
  char *p, *user, *pass, *host;
  int inString;
  int sock = -1;

  if (nArgs != 1 || *args[0] == '\0') {
    queue_string(d, BAD_ARG_STRING);
    return 0;
  }

  p = args[0];
  inString = 0;
  pass = NULL;
  user = p;
  host = NULL;
  while (*p != '\0') {
    switch (*p) {
      case '"':
	inString = !inString;
	break;
      case ':':
	if (!inString) {
	  pass = p+1;
	  *p = '\0';
	}
	break;
      case '@':
        if (!inString) {
	  host = p+1;
	  *p = '\0';
	}
	break;
      default:
      ; // Do nothing.
    }
    p++;
  }

  if (pass == NULL || *pass == '\0' ||
      user == NULL || *user == '\0' || 
      host == NULL || *host == '\0')  {
    queue_string(d, BAD_ARG_STRING);
    goto abort;
  }

  sock = door_tcp_connect(host, "110", d, id);
  if (sock < 0) {
    queue_string(d, "Connection to mail server failed.");
  } else {
    mail_t *p = malloc(sizeof(mail_t));
    queue_string(d, "Connection to mail server suceeded.");

    if (!p) {
      queue_string(d, "Count not allocate required memory.");
      goto abort;
    }
    p->player = d->player;
    p->id = id;
    p->status = OK_e;
    strncpy(p->user, user, 30);
    p->user[29] = '\0';
    strncpy(p->pass, pass, 30);
    p->pass[29] = '\0';    
    addStruct(p);
  }

  queue_string(d, "\r\n");
  process_output(d);
  return 1;

 abort:
  if (sock > 0) {
    closeDoorWithId(d, id);
  }
  queue_string(d, "\r\n");
  process_output(d);
  return -1;
}

int mailDoorClose(DESC *d) {
  mail_t *p;
  p = findStruct(d->player);
  if (p) {
    delStruct(p);
  }
  return 1;
}

int mailDoorOutput(DESC *d, char *pText) {
  notify(d->player, "Command ignored.");
  return -1;
}

int mailDoorInput(DESC *d, char *pText) {
  mail_t *p = NULL;
  char *n, *s, *e;
  int l;

#ifdef _MAILDEBUG
  queue_string(d, pText);
  queue_string(d, "\r\n");
#endif

  p = findStruct(d->player);
  if (!p) {
    queue_string(d, "Could not find you in the mail-door system.. aborting.");
    goto abort;
  }

  if (strncmp(pText, "+OK", 3) == 0) {
    switch (p->status) {
      case OK_e:
	sendcmd(d->door_desc, unsafe_tprintf("USER %s\r\n", p->user));
	p->status = USER_e;
	break;
      case USER_e:
	sendcmd(d->door_desc, unsafe_tprintf("PASS %s\r\n", p->pass));
	p->status = PASS_e;
	break;
      case PASS_e:
	sendcmd(d->door_desc, "STAT\r\n");
	p->status = STAT_e;
	break;
      case STAT_e:
	// +OK n s
	n = s = pText + 4;
	while (*s != '\0' && *s != ' ') {
	  s++;
	}
	// Find the size
	if (*s == ' ') {
	  *s = '\0';
	  s++;
	}
	// Get rid of anything at end of message
	e = s;
	while (*e >= '0' && *e <= '9') {
	  e++;
	}
	*e = '\0';
	
	l = strlen(n);
	queue_string(d, unsafe_tprintf("You have %s mail message%s occupying %s bytes.",
				n, (l > 1) ? "s" : "", (s == '\0') ? "(unknown)" : s));
	queue_string(d, "\r\n");
	sendcmd(d->door_desc, "QUIT\r\n");
	p->status = DONE_e;
	break;
    default:
      ;
    }
  } else {
    queue_string(d, "Unexpected message returned from pop server.\r\n");
    queue_string(d, "--> ");
    queue_string(d, pText);
    goto abort;
  }
  process_output(d);
  return 1;

 abort:
  if (p) {
    closeDoorWithId(d, p->id);
  }
  queue_string(d, "\r\n");
  process_output(d);
  return -1;
}
