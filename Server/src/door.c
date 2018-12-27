/*
 * NOTE: INPUT  means from DOOR -> MUSH
 *       OUTPUT means from MUSH -> DOOR
 */
// TODO:
//  1) Get new list code working                                  .. DONE
//  2) Get connect code working 
//       (Find door, validate status, call open func)             .. DONE
//  3) Add status switch to @door                                 .. DONE
//  4) Add wiz command, list all players through a given door     .. DONE
//  5) Add in log messages                                        .. DONE
//  6) Port mush.c and empire.c                                   .. DONE
//  7) Add the init call to the Rhost init loop                   .. DONE
//  8) Let doors be compiled out.                                 .. DONE
//  9) Allow door to be set into background :)
//      That way you can have multiple doors doing things like..
//        ICQ, or mail, or whatever!
// 10) Move bittype out                                           .. DONE
// 11) Fix wiz.c                                                  .. DONE
// 12) Tidy door calls, rename input/outout to readfrom/writeto   
// 13) Check doorparm (help.c)
// 14) **BUG** Adds first door at 1                               .. DONE
// 15) **BUG** Why need -1 on the strings in the lookup func      .. DONE
// 16) **BUG** At current players can only belong to 1 door
// 17) **BUG** When QUIT'ing from a mush, the quit msg is missing 
// 18) **BUG** Can't @Reboot and mantain DOORS ... ifdef in netcommon
// 19) **FEAT* Need the 'DOOR' connect command, plus DESC support
// 20) **FEAT* Need to disallowed door'd players from opening doors

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef SOLARIS
/* Solaris is stupid with declarations and needs this */
void bcopy(const void *, void *, int);
char *index(const char *, int);
#endif
#include "copyright.h"
#include "autoconf.h"

#ifdef VMS
#include "multinet_root:[multinet.include.sys]file.h"
#include "multinet_root:[multinet.include.sys]ioctl.h"
#include "multinet_root:[multinet.include]errno.h"
#else
#include <sys/file.h>
#include <sys/ioctl.h>
#ifdef SYSWAIT
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#endif
#include <signal.h>
#ifdef NEED_BSDH
#include <bsd/bsd.h>
#endif

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "interface.h"
#include "door.h"
#include "debug.h"
#include "utils.h"
#include "door_prototypes.h"
#include "rhost_ansi.h"

#define FILENUM DOOR_C
#define INIT_NUM_DOORS 5

#define LOGTEXT(DOM, PLYR, MSG) \
    STARTLOG(LOG_DOOR, "DRE", DOM)\
       if (PLYR > 0) { log_name(PLYR); log_text(" "); }\
       log_text((char *) MSG); \
    ENDLOG


#include "empire.h"

extern int ndescriptors;
extern int maxd;

static door_t **gaDoors = NULL;
static int gnDoors = 0;
#ifdef ENABLE_DOORS
static int maxDoors = INIT_NUM_DOORS;
//static int maxInternalDoors = INIT_NUM_DOORS / 2;
#endif



static char *gaStatusArray[] = {
  "ONLINE",
  "OFFLINE",
  "OPEN",
  "CLOSED",
  "INTERNAL"
};

static void registerDoors(void);


static void forceDoorClosed(int player, int door) {
  DESC *d;
  DPUSH; /* #1 */
  DESC_ITER_CONN(d) {
    if (d->flags & DS_HAS_DOOR && d->door_num == door) {
      closeDoorWithId(d, door);
      notify(player, unsafe_tprintf("%s has slammed the door shut.", Name(player)));
    }
  }
  VOIDRETURN; /* #1 */
}


static int findDoor(const char *doorName) {
  char *pUName;
  int i;
  DPUSH /* 2 */
  pUName = alloc_mbuf("doorName");
  if (!pUName) {
    LOGTEXT("ERR", -1, "Could not allocate findDoor mbuf");
    RETURN(-1); /* 2 */
  } else {
    strncpy(pUName, doorName, MBUF_SIZE - 1);
    pUName[MBUF_SIZE - 1] = '\0';
    for (i = 0 ; pUName[i] != '\0' ; i++) {
      pUName[i] = toupper(pUName[i]);
    }
    for (i = 0 ; i < gnDoors ; i++) {
      if (strcmp(pUName, gaDoors[i]->pName) == 0) {
	free_mbuf(pUName);  
	RETURN(i); /* 2 */
      }
    }
  }
  free_mbuf(pUName);
  RETURN(-1); /* 2 */
}

#ifdef ENABLE_DOORS
static int addDoor(const char *doorName,
		   doorInit_t   pFnInit,
		   doorOpen_t   pFnOpen,
		   doorClose_t  pFnClose,
		   doorOutput_t pFnWrite,
		   doorInput_t  pFnRead,
		   int bitLvl, int loc) {
  int door, deAlloc = 0;
  DPUSH; /* 3 */
  door = findDoor(doorName);
  if (door > 0) {
    LOGTEXT("ERR", -1, unsafe_tprintf("Name clash when trying to add door: %s/%d", 
			       doorName, door));
    RETURN(-1); /* 3 */
  }

  // Whee!! Door code doesn't recover old slots:!!! BUG

  if (gnDoors == (maxDoors - 1)) {
    // grow array
    gaDoors = realloc(gaDoors, sizeof(door_t) * (maxDoors + 5));
    if (gaDoors == NULL) {
      LOGTEXT("ERR", -1, "Could not allocate memory for resizing door array.");
      RETURN(-1); /* 3 */      
    } else {
      LOGTEXT("INF", -1, unsafe_tprintf("Door array resized from %d to %d", 
      				 maxDoors, maxDoors + 5));
      maxDoors += 5;
    }

  }
  gaDoors[gnDoors] = NULL;
  gaDoors[gnDoors] = calloc(1, sizeof(door_t));
  if (!gaDoors[gnDoors]) {
    LOGTEXT("ERR", -1, "Could not allocate door_t for door array");
    RETURN(-1); /* 3 */
  }

  gaDoors[gnDoors]->pName = alloc_mbuf("door_name");
  if (!gaDoors[gnDoors]->pName) {
    LOGTEXT("ERR", -1, "Could not allocate door_name mbuf");
    goto error;
  }
  deAlloc++;

  strcpy(gaDoors[gnDoors]->pName, doorName);
  gaDoors[gnDoors]->switchNum = 0;
  if (pFnOpen == NULL) {
    LOGTEXT("ERR", -1, unsafe_tprintf("%s has a NULL open functions", doorName));
    goto error;
  }

  if (loc == dINTERNAL_e && (!pFnInit || !pFnRead || !pFnWrite)) {
    LOGTEXT("ERR", -1, 
	    unsafe_tprintf("Internal door '%s' is misconfigured.", doorName));
    goto error;
  }

  gaDoors[gnDoors]->pFnOpenDoor  = pFnOpen;
  gaDoors[gnDoors]->pFnCloseDoor = pFnClose;
  gaDoors[gnDoors]->pFnWriteDoor = pFnWrite;
  gaDoors[gnDoors]->pFnReadDoor  = pFnRead;
  gaDoors[gnDoors]->doorStatus = OFFLINE_e;
  gaDoors[gnDoors]->nConnections = 0;

  if ( loc == dINTERNAL_e ) {
    gaDoors[gnDoors]->pDescriptor = malloc(sizeof(DESC));
    if (gaDoors[gnDoors]->pDescriptor == NULL) {
      LOGTEXT("ERR", -1, "Allocation of descriptor for internal door failed");
      goto error;
    }
    gaDoors[gnDoors]->doorStatus = INTERNAL_e;
    gaDoors[gnDoors]->pDescriptor->player= SYSTEM;
  }

  // Check LOC is a room
  if (loc >=0 &&
      (!Good_obj(loc) || !isThing(loc) || Going(loc) || Recover(loc))) {
    LOGTEXT("ERR", -1, unsafe_tprintf("%s: Location %d is not a valid object", doorName, loc)); 
    goto error;
  } else {    
    gaDoors[gnDoors]->locTrig = loc;
  }

  // set BT
  gaDoors[gnDoors]->bittype = bitLvl;

  // Call init function.
  if (pFnInit) {
    if (pFnInit() < 0) {
      LOGTEXT("ERR", -1, 
	      unsafe_tprintf("Could not initialize door '%s'\r\n", doorName));
      goto error;
    }
  }

  if (loc == dINTERNAL_e) { 
    if (pFnOpen(gaDoors[gnDoors]->pDescriptor, 0, NULL, gnDoors) < 0) {
      gaDoors[gnDoors]->doorStatus = OFFLINE_e;
      LOGTEXT("ERR", -1,
	      unsafe_tprintf("Failed to open internal door '%s'\r\n", doorName));
    }
  }

  gnDoors++;
  LOGTEXT("NEW", -1, unsafe_tprintf("Added new door: %s", doorName));
  RETURN(0); /* 3 */
 error:
  if (gaDoors[gnDoors]->pName) {
    free_mbuf(gaDoors[gnDoors]->pName);
  }
  if (gaDoors[gnDoors] != NULL) {
    free(gaDoors[gnDoors]);
  }
  RETURN(-1); /* 3 */
}
#endif

static void shut_door(DESC *d)
{
  DPUSH; /* 4 */
  if (d->flags & DS_HAS_DOOR) {
    shutdown(d->door_desc,2);
    close(d->door_desc);
    ndescriptors--;
    d->door_num = 0;
    d->door_desc = -1;
    d->door_output_size = 0;
  }
  freeqs(d,1);
  if (d->door_lbuf != NULL) {
    free_lbuf(d->door_lbuf);
    d->door_lbuf = NULL;
  }
  if (d->door_mbuf != NULL) {
    free_mbuf(d->door_mbuf);
    d->door_mbuf = NULL;
  }
  d->flags &= ~DS_HAS_DOOR;
  d->door_num = 0;
  if (d->door_raw != NULL) {
    free_lbuf(d->door_raw);
    d->door_raw = NULL;
  }
  VOIDRETURN; /* 4 */
}

/************* ORIGINAL RHOST DOOR CODE ***************/
/* from mu socket to door socket via any handling     */
void door_raw_input(DESC *d, char *input)
{
  DPUSH; /* 5 */
  if (gaDoors[d->door_num]->pFnReadDoor == NULL) {
    queue_door_string(d, input, 1); /* 3rd arg means slap a newline on */
  } else {
    if ((gaDoors[d->door_num]->pFnReadDoor)(d, input) < 0) {
      LOGTEXT("ERR", d->player, 
	      unsafe_tprintf("ran into an unexpected error writing to '%s'", gaDoors[d->door_num]->pName));
      notify(d->player, 
	     unsafe_tprintf("Worlds shimmer, reality wavers, and the door to '%s' slams before your eyes.",
		     gaDoors[d->door_num]->pName));
      closeDoorWithId(d, d->door_num);
      
    }
  }
  VOIDRETURN; /* 5 */
}

/* from mu socket to door socket via any handling */
void door_raw_output(DESC *d, char *output)
{
  DPUSH; /* 5 */
  if (gaDoors[d->door_num]->pFnWriteDoor == NULL) {
    queue_string(d, output);
  } else {
    if ((gaDoors[d->door_num]->pFnWriteDoor)(d, output) < 0) {
      LOGTEXT("ERR", d->player, 
	      unsafe_tprintf("ran into an unexpected error reading from '%s'", gaDoors[d->door_num]->pName));
      notify(d->player, 
	     unsafe_tprintf("Worlds shimmer, reality wavers, and the door to '%s' slams before your eyes.",
		     gaDoors[d->door_num]->pName));
      closeDoorWithId(d, d->door_num);
    }
  }
  VOIDRETURN; /* 5 */
}

/* Used to setup all the door stuff on a player. */
static int setup_player(DESC *d, int sock, int doorIdx) {
  int retval = sock;

  if (sock >= 0) {
      d->door_lbuf = alloc_lbuf("door_lbuf");
      if (d->door_lbuf == NULL) {
          close(sock);
          queue_string(d, "Could not allocate door buffer\r\n");
          retval = -1;
      }
      *(d->door_lbuf) = '\0';
      d->door_mbuf = alloc_mbuf("door_lbuf");
      if ( (sock >= 0) && d->door_mbuf == NULL ) {
          close(sock);
          queue_string(desc_in_use, "Could not allocate door buffer\r\n");
          free_lbuf(d->door_lbuf);
          d->door_lbuf = NULL;
          retval = -1;
      }

      if (retval >= 0) {
          *(d->door_mbuf) = '\0';
          d->door_desc = sock;
          d->flags |= DS_HAS_DOOR;   
          d->door_num = doorIdx;
      /*  process_output(d); */
      }
  }
  return sock;
}

int door_tcp_connect(char *host, char *port, DESC *d, int doorIdx)
{
  int new_port;
  struct servent *sp;
  struct hostent *hp;
  struct sockaddr_in sin;
  DPUSH; /* 6 */
  new_port = -2;
  if (host == NULL || *host == '\0' || port == NULL || *port == '\0')
    new_port = -1;
  else {
    if (isdigit((int)*host)) {
      sin.sin_addr.s_addr = inet_addr(host);
    }
    else {
      hp = gethostbyname(host);
      if (hp == NULL)
	new_port = -1;
      else 
	bcopy(hp->h_addr, (char *)&(sin.sin_addr), sizeof(sin.sin_addr));
    }
    if (new_port != -1) {
      if (isdigit((int)*port)) {
	sin.sin_port = htons(atoi(port));
      }
      else {
	sp = getservbyname(port, "tcp");
	if (sp == NULL)
	  new_port = -1;
	else
	  sin.sin_port = sp->s_port;
      }
      new_port = socket(AF_INET, SOCK_STREAM, 0);
      if (new_port < 0)
	new_port = -1;
      else {
	sin.sin_family = AF_INET;
	if (connect(new_port, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	  close(new_port);
	  new_port = -1;
	} else {
	  ndescriptors++;
	  if (new_port >= maxd)
	    maxd = new_port + 1;
	  new_port = setup_player(d, new_port, doorIdx);
	}
      }
    }
  }
  

  
  RETURN(new_port); /* 6 */
}


int process_door_output(DESC * d)
{
    TBLOCK *tb, *save;
    int cnt;

    DPUSH; /* #7 */

    tb = d->door_output_head;
    while (tb != NULL) {
	while (tb->hdr.nchars > 0) {

	    cnt = WRITE(d->door_desc, tb->hdr.start,
			tb->hdr.nchars);
	    if (cnt < 0) {
		if (errno == EWOULDBLOCK) {
		    RETURN(1); /* #7 */
                }
		RETURN(0); /* #7 */
	    }
	    d->door_output_size -= cnt;
	    tb->hdr.nchars -= cnt;
	    tb->hdr.start += cnt;
	}
	save = tb;
	tb = tb->hdr.nxt;
	free_lbuf(save);
	d->door_output_head = tb;
	if (tb == NULL)
	    d->door_output_tail = NULL;
    }
    RETURN(1); /* #7 */
}

int process_door_input(DESC * d)
{
  static char buf[LBUF_SIZE];
  char *pt, save;
  int got;

  DPUSH; /* #8 */
  got = READ(d->door_desc, buf, sizeof(buf) - 1);
  if (got <= 0) {
    RETURN(0); /* #8 */
  }
  *(buf + got) = '\0';
  if (d->door_raw != NULL) {
    pt = strchr(buf,'\n');
    if (pt == NULL) {
      if ((strlen(d->door_raw) + strlen(buf)) < LBUF_SIZE -1) {
	strcat(d->door_raw,buf);
	RETURN(1); /* #8 */
      }
      else {
	queue_string(d,"Output overflow from door.\r\n");
	RETURN(0); /* #8 */
      }
    }
    else {
      *pt = '\0';
      if ((strlen(d->door_raw) + strlen(buf)) < LBUF_SIZE - sizeof(CBLKHDR) -2) {
	strcat(d->door_raw,buf);
	strcat(d->door_raw,"\r\n");
	save_door(d,d->door_raw);
	if (*(pt+1) == '\0') {
	  free_lbuf(d->door_raw);
	  d->door_raw = NULL;
	  RETURN(1); /* #8 */
	}
	else {
	  strcpy(d->door_raw,pt+1);
	  strcpy(buf,d->door_raw);
	  free_lbuf(d->door_raw);
	  d->door_raw = NULL;
	}
      }
      else {
	queue_string(d,"Output overflow from door.\r\n");
	RETURN(0); /* #8 */
      }
    }
  }
  if ((strlen(buf) < LBUF_SIZE - sizeof(CBLKHDR) -1) && (*(buf + got - 1) == '\n')) {
    save_door(d, buf);
  } else {
    pt = buf + got -2;
    while ((*pt != '\n') && (pt > buf))
      pt--;
    if ((pt > buf) || ((pt == buf) && (*pt == '\n'))) {
      pt++;
      save = *pt;
      *pt = '\0';
      save_door(d,buf);
      *pt = save;
      d->door_raw = alloc_lbuf("door_raw");
      if (d->door_raw == NULL) {
	RETURN(0); /* #8 */
      }
      strcpy(d->door_raw,pt);
    }
    else {
      d->door_raw = alloc_lbuf("door_raw");
      if (d->door_raw == NULL) {
	RETURN(0); /* #8 */
      }
      strcpy(d->door_raw,buf);
    }
  }
  RETURN(1); /* #8 */
}

void save_door(DESC *d, char *info)
{
  CBLK *qpt;

  DPUSH; /* #9 */
  qpt = (CBLK *) alloc_lbuf("process_door_input");
  strcpy(qpt->cmd, info);
  qpt->hdr.nxt = NULL;
  if (d->door_input_tail == NULL)
    d->door_input_head = qpt;
  else
    d->door_input_tail->hdr.nxt = qpt;
  d->door_input_tail = qpt;
  DPOP; /* #9 */
}

void queue_door_string(DESC *d, const char *s, int addnl)
{
    char *new;
    int len;

    DPUSH; /* #120 */

    if (index(s, ESC_CHAR))
	new = strip_ansi(s);
    else
	new = (char *) s;
    if (new) {
	len = strlen(new);
	if (addnl) {
	  if (len >= LBUF_SIZE-1) {
	    *(new + LBUF_SIZE - 2) = '\r';
	    *(new + LBUF_SIZE - 1) = '\n';
	    *(new + LBUF_SIZE) = '\0';
	    len = LBUF_SIZE - 1;
	  } else {
	    strcat(new, "\r\n");
	    len++;
	  }
	}
	queue_door_write(d, new, len);
    }
    VOIDRETURN; /* #120 */
}

void queue_door_write(DESC * d, const char *b, int n)
{
    int left;
    char *buf;
    TBLOCK *tp;

    DPUSH; /* #118 */

    if (n <= 0) {
	VOIDRETURN; /* #118 */
    }

    if (d->door_output_size + n > mudconf.output_limit)
	process_door_output(d);

    left = mudconf.output_limit - d->door_output_size - n;
    if (left < 0) {
	tp = d->door_output_head;
	if (tp == NULL) {
	    STARTLOG(LOG_PROBLEMS, "QUE", "WRITE")
		log_text((char *) "Flushing when door_output_head is null!");
	    ENDLOG
	} else {
	    STARTLOG(LOG_NET, "NET", "WRITE")
            buf = alloc_lbuf("queue_door_write.LOG");
	    sprintf(buf,
		    "[%d/%s] Output buffer overflow, %d chars discarded by ",
		    d->door_desc, d->addr, tp->hdr.nchars);
	    log_text(buf);
	    free_lbuf(buf);
	    log_name(d->player);
	    ENDLOG
            d->door_output_size -= tp->hdr.nchars;
	    d->door_output_head = tp->hdr.nxt;
	    if (d->door_output_head == NULL)
		d->door_output_tail = NULL;
	    free_lbuf(tp);
	}
    }
    /* Allocate an output buffer if needed */

    if (d->door_output_head == NULL) {
	tp = (TBLOCK *) alloc_lbuf("queue_door_write.new");
	tp->hdr.nxt = NULL;
	tp->hdr.start = tp->data;
	tp->hdr.end = tp->data;
	tp->hdr.nchars = 0;
	d->door_output_head = tp;
	d->door_output_tail = tp;
    } else {
	tp = d->door_output_tail;
    }

    /* Now tp points to the last buffer in the chain */

    d->door_output_size += n;
    do {

	/* See if there is enough space in the buffer to hold the
	 * string.  If so, copy it and update the pointers..
	 */

        /* BUGFIX 1/4/1997 - Thorin */
	/* left = LBUF_SIZE - (tp->hdr.end - (char *) tp); */
	left = LBUF_SIZE - sizeof(TBLKHDR) - (tp->hdr.end - (char *) tp);

	if (n <= left) {
	    strncpy(tp->hdr.end, b, n);
	    tp->hdr.end += n;
	    tp->hdr.nchars += n;
	    n = 0;
	} else {

	    /* It didn't fit.  Copy what will fit and then allocate
	     * another buffer and retry.
	     */

	    if (left > 0) {
		strncpy(tp->hdr.end, b, left);
		tp->hdr.end += left;
		tp->hdr.nchars += left;
		b += left;
		n -= left;
	    }
	    tp = (TBLOCK *) alloc_lbuf("queue_door_write.extend");
	    tp->hdr.nxt = NULL;
	    tp->hdr.start = tp->data;
	    tp->hdr.end = tp->data;
	    tp->hdr.nchars = 0;
	    d->door_output_tail->hdr.nxt = tp;
	    d->door_output_tail = tp;
	}
    } while (n > 0);
    VOIDRETURN; /* #118 */
}

/*********** END ORIGINAL RHOST DOOR CODE **************/

/* Called on startup to start the door system.
 * Please note that for reasons of security, all DOORS are
 * started disconnected.
 */

void initDoorSystem(void) {
  DPUSH; /* #10 */
  LOGTEXT("INF", -1, "Initializing door system");
  gaDoors = calloc(INIT_NUM_DOORS, sizeof(door_t));
  if (gaDoors == NULL) {
    LOGTEXT("ERR", -1, "Could not allocate memory for door system");
  } else {
    gnDoors = 0;
    registerDoors();
  }
  VOIDRETURN; /* #10 */
}

void modifyDoorStatus(dbref player, char *pDoorName, char * status) {
  int d, i;
  doorStatus_t s;
  DPUSH; /* #11 */

  if (!Wizard(player)) {
    notify(player, "Permission Denied.");
  } else {

    d = findDoor(pDoorName);
    if (d < 0) {
      notify(player, "Could not find a door with that name.");
    } else {

      if (gaDoors[d]->locTrig == dINTERNAL_e) {
	notify(player, "This command has no power over internal doors.");
	VOIDRETURN; /* #11 */
      }

      for (i = 0 ; status[i] != '\0' ; i++) {
	status[i] = toupper(status[i]);
      }    
      if (strcmp(status, gaStatusArray[ONLINE_e]) == 0) {      
	s = ONLINE_e;
      } else if (strcmp(status, gaStatusArray[OFFLINE_e]) == 0) {
	s = OFFLINE_e;
      } else if (strcmp(status, gaStatusArray[CLOSED_e]) == 0) {
	s = CLOSED_e;
      } else if (strcmp(status, gaStatusArray[OPEN_e]) == 0) {
	  notify(player, "The only way to open a door is to walk through it");
	  VOIDRETURN; /* #11 */
      } else {
	notify(player, "Invalid door status. (Choose from ONLINE, OFFLINE, CLOSED)");
	VOIDRETURN; /* #11 */
      }
      
      if (gaDoors[d]->nConnections > 0 
	  && 
	  gaDoors[d]->doorStatus == OPEN_e) {
	notify(player, "Forcing door closed.. all players will be kicked.");
	forceDoorClosed(player, d);
      }

      gaDoors[d]->doorStatus = s;
      notify(player, "Door status changed.");
      listDoors(player, pDoorName, 0);
      LOGTEXT("INF", player, unsafe_tprintf("changed status of door %s to %s",
				     gaDoors[d]->pName, gaStatusArray[s]));
    }
  }
  VOIDRETURN; /* #11 */
}

void openDoor(dbref player, 
	      dbref cause, 
	      char *pDoor,
	      int nArgs, char *args[], int i_type) {
  int d, i_now;
  DESC *d_door, *d_use;
  DPUSH; /* #11 */

  d = findDoor(pDoor);
  if (d < 0) {
    if ( player != cause ) {
       notify(cause, "Bad door in open for specified target.");
    } else {
       notify(player, "Bad door in open.");
    }
  } else if (!isPlayer(player)) {
    if ( player != cause ) {
       notify(cause, "The target is not a player so can not use doors.");
    } else {
       notify(player, "Only players can open a door.");
    }
  } else if (desc_in_use && (desc_in_use->flags & DS_HAS_DOOR) ) {
    if ( player != cause ) {
       notify(cause, "Target already has a door open on the attempted connection.");
    } else {
       notify(player, "You already have a door open on the attempted connection.");
    }
  } else if (bittype(player) <  gaDoors[d]->bittype) {
    if ( player != cause ) {
       notify(cause, "Try as you might, you can't shove the target through that door.");
    } else {
       notify(player, "Try as you might, the door refuses to budge");
    }
  } else if (gaDoors[d]->doorStatus == OFFLINE_e){ 
    if ( player != cause ) {
       notify(cause, "That door is currently offline.");
    } else {
       notify(player, "That door is currently offline.");
    }
  } else if (gaDoors[d]->locTrig == dINTERNAL_e) {
    notify(player, "Internal doors cannot be opened in this manner.");
  } else if (gaDoors[d]->locTrig != PLAYER_e &&
	     ((gaDoors[d]->locTrig == ROOM_e && !isRoom(cause))
	      ||
	      (gaDoors[d]->locTrig == EXIT_e && !isExit(cause))
	      ||
	      (gaDoors[d]->locTrig >= 0 && gaDoors[d]->locTrig != cause))) {
    notify(cause, "You can't force a player to open a door."); 
  } else if ( i_type || desc_in_use == NULL) {
     if ( !isPlayer(player) ) {
        if ( player != cause ) {
           notify(cause, "Invalid target.  Door's can't be opened from a non-player.");
        } else {
           notify(player, "Only players who are connected can issue a door.");
        }
     } else if ( !controls(cause, player) ) {
        notify(cause, "Sorry, you do not have permission to force the target into a door.");
     } else {
        i_now = 0;
        d_use = NULL;
        DESC_ITER_CONN(d_door) {
           if (d_door->player == player) {
              if ( d_door->last_time > i_now ) {
                 d_use = d_door;
                 i_now = d_door->last_time;
              }
           }
        }
        if ( d_use ) {
           if ((gaDoors[d]->pFnOpenDoor)(d_use, nArgs, args, d) < 0) {
              LOGTEXT("ERR", player, unsafe_tprintf("failed to open a door to %s", gaDoors[d]->pName));
              if ( player != cause ) {
                 notify(cause, "The door fails to open for the target.");
              } else {
                 notify(player, "The door fails to open.");
              }
              shut_door(d_use);
           } else {
              gaDoors[d]->doorStatus = OPEN_e;
              (gaDoors[d]->nConnections)++;
              LOGTEXT("INF", player, unsafe_tprintf("opens a door to %s", gaDoors[d]->pName));
              if ( player != cause ) {
                 notify(cause, unsafe_tprintf("Target %s opens a door to %s", Name(player), gaDoors[d]->pName));
              }
           }
        } else {
           if ( player != cause ) {
              notify(cause, "Invalid target.  Door's can't be opened from a queue with an invalid or disconnected player.");
           } else {
              notify(player, "Sorry, we couldn't open the door for you.");
           }
        }
     }
  } else {
    if ((gaDoors[d]->pFnOpenDoor)(desc_in_use, nArgs, args, d) < 0) {
      LOGTEXT("ERR", player, unsafe_tprintf("failed to open a door to %s",
				     gaDoors[d]->pName));
      if ( player != cause ) {
         notify(cause, "The door fails to open for the target player.");
      } else {
         notify(player, "The door fails to open.");
      }
      shut_door(desc_in_use);
    } else {
      gaDoors[d]->doorStatus = OPEN_e;
      (gaDoors[d]->nConnections)++;
      LOGTEXT("INF", player, unsafe_tprintf("opens a door to %s",
				     gaDoors[d]->pName));
      if ( player != cause ) {
         notify(cause, unsafe_tprintf("Target %s opens a door to %s", Name(player), gaDoors[d]->pName));
      }
    }

  }
  VOIDRETURN; /* #11 */
}

void closeDoorWithId(DESC *desc, int d) {
  DPUSH; /* #11.5 */

  if (d < 0 || d >= gnDoors ) {
    // LOG
    notify(desc->player,
	   "@DOOR/CLOSE attempted with invalid door id. This has been logged"); 
    LOGTEXT("ERR", desc->player, unsafe_tprintf("tried to @door/close an invalid door id '%d'",
			           d));
    VOIDRETURN /* 11.5 */
  }

  if (d && gaDoors[d]->pFnCloseDoor) {
    if ((gaDoors[d]->pFnCloseDoor)(desc) < 0) {
      LOGTEXT("ERR", desc->player, 
	      unsafe_tprintf("tried to close a door to %s, but the door did not close cleanly.",
		      gaDoors[d]->pName));
    }
  }
  shut_door(desc);
  queue_string(desc, "Door connection closed.\r\n");
  (gaDoors[d]->nConnections)--;
  if (gaDoors[d]->doorStatus == INTERNAL_e) {
    gaDoors[d]->doorStatus = OFFLINE_e;
  } else if (!gaDoors[d]->nConnections) {
    gaDoors[d]->doorStatus = CLOSED_e;
  }

  LOGTEXT("INF", desc->player, unsafe_tprintf("Door '%s' closed",
  				       gaDoors[d]->pName));

  VOIDRETURN; /* #11.5*/
}

void closeDoor(DESC *desc, char *pDoor) {
  int d;
  DPUSH; /* #12 */
 
  if ( !pDoor || *pDoor == '\0') {
    notify(desc->player, "@DOOR/CLOSE requires a door name.");
  } else {
    d = findDoor(pDoor);
    if (d < 0) {
      notify(desc->player, "Invalid door name.");
    } else {
      closeDoorWithId(desc, d);    
    }
  }
  VOIDRETURN; /* #12 */
}

const char * returnDoorName(int d) {
  if (d >= 0 && d < gnDoors && gaDoors[d] != NULL && gaDoors[d]->pName != NULL) {
    return gaDoors[d]->pName;
  } else {
    return "BAD_DOOR";
  }
}

void listDoors(dbref player, char *dName, int full) {
  int i, d;
  DESC *desc;

  DPUSH; /* #13 */

  if (dName && *dName != '\0') {
    d = findDoor(dName);
    if (d >= 0 && (gaDoors[d]->locTrig != dINTERNAL_e || Wizard(player))) {
      notify(player, 
	     unsafe_tprintf("  NUM  %-10s %-10s  P  %-5s", "NAME", "STATUS", "LOC"));
      notify(player, 
	     unsafe_tprintf(" (#%2d) %-10s %-10s (%d) #%-20s",
		     d,
		     gaDoors[d]->pName,
		     gaStatusArray[gaDoors[d]->doorStatus],
		     gaDoors[d]->bittype,
		     (gaDoors[d]->locTrig > 0 
		      ? unsafe_tprintf("%d", gaDoors[d]->locTrig)
		      : (gaDoors[d]->locTrig == PLAYER_e ? "PLAYER"
			 : (gaDoors[d]->locTrig == ROOM_e ? "ROOM"
			    : (gaDoors[d]->locTrig == EXIT_e ? "EXIT"
                              : "INTERNAL"))))));
      if (full) {
	notify(player, "The following players are using this door:");
	DESC_ITER_CONN(desc) {
	  if (desc->flags & DS_HAS_DOOR && desc->door_num == d) {
	    notify(player, unsafe_tprintf("   (#%-6d) %s", desc->player, Name(desc->player)));
	  }
	}
      }
    } else {
      notify(player, unsafe_tprintf("Could not find a door called '%s'", dName));
    }
  } else {
      notify(player, 
	     unsafe_tprintf("  NUM  %-10s %-10s  P  %-5s", "NAME", "STATUS", "LOC"));
    for (i = 0 ; i < gnDoors ; i++) {
      if (bittype(player) >= gaDoors[i]->bittype
	  &&
	  (gaDoors[i]->locTrig != dINTERNAL_e || Wizard(player))) {
	notify(player, 
	       unsafe_tprintf(" (#%2d) %-10s %-10s (%d) #%-20s",
		       i,
		       gaDoors[i]->pName,
		       gaStatusArray[gaDoors[i]->doorStatus],
		       gaDoors[i]->bittype,
		       (gaDoors[i]->locTrig > 0 
			? unsafe_tprintf("%d", gaDoors[i]->locTrig)
			: (gaDoors[i]->locTrig == PLAYER_e ? "PLAYER"
			   : (gaDoors[i]->locTrig == ROOM_e ? "ROOM"
			      : (gaDoors[i]->locTrig == EXIT_e ? "EXIT"
                              : "INTERNAL"))))));
      }
    }
  }
  VOIDRETURN; /* #13 */
}

void door_registerInternalDoorDescriptors(fd_set *input_set, 
					  fd_set *output_set, 
					  int *maxd) {
  int i;
  for (i = 0 ; i < gnDoors ; i++) {
    if (gaDoors[i]->doorStatus == INTERNAL_e) {
      FD_SET(gaDoors[i]->pDescriptor->door_desc, input_set);
      FD_SET(gaDoors[i]->pDescriptor->door_desc, output_set);
      if (gaDoors[i]->pDescriptor->door_desc >= *maxd)
	(*maxd) = gaDoors[i]->pDescriptor->door_desc + 1;
    }
  }
}

void door_checkInternalDoorDescriptors(fd_set *input_set, 
				       fd_set *output_set) {
  int i;
  for (i = 0 ; i < gnDoors ; i++) {
    if (gaDoors[i]->doorStatus == INTERNAL_e) {
      if (FD_ISSET(gaDoors[i]->pDescriptor->door_desc, input_set)) {
	if (!process_door_input(gaDoors[i]->pDescriptor)) {
	  closeDoorWithId(gaDoors[i]->pDescriptor, i);
	}	
      }
    }

    /* We need this check twice, because the first call can close
     * the door. Doh!
     */
    if (gaDoors[i]->doorStatus == INTERNAL_e) {
      if (FD_ISSET(gaDoors[i]->pDescriptor->door_desc, output_set)) {
	if (!process_door_output(gaDoors[i]->pDescriptor)) {
	  closeDoorWithId(gaDoors[i]->pDescriptor, i);
	}
      }
    }
  }
}

void door_processInternalDoors(void) {
    int nprocessed, i;
    DESC *d;
    CBLK *t;
    char *cmdsave;

    DPUSH; /* #148 */

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "process_commands";

    do {
	nprocessed = 0;
	for (i = 0 ; i < gnDoors ; i++) {
	  if (gaDoors[i]->doorStatus != INTERNAL_e) {
	    continue;
	  }
	  d = gaDoors[i]->pDescriptor;
	  if (d->quota > 0 && (t = d->input_head)) {
	    d->quota--;
	    nprocessed++;
	    d->input_head = (CBLK *) t->hdr.nxt;
	    if (!d->input_head)
	      d->input_tail = NULL;
	    d->input_size -= (strlen(t->cmd) + 1);
	    if (d->flags & DS_HAS_DOOR && t->cmd[0] != '!') //input = player input
	      door_raw_input(d, t->cmd); 
	    free_lbuf(t);
	  }

	  if ((t = d->door_input_head)) {
	    nprocessed++;
	    d->door_input_head = (CBLK *) t->hdr.nxt;
	    if (!d->door_input_head)
	      d->door_input_tail = NULL;
	    door_raw_output(d, t->cmd);
	    free_lbuf(t);
	  }
	}
    } while (nprocessed > 0);
    mudstate.debug_cmd = cmdsave;
    VOIDRETURN; /* #148 */
}
/****************************************************
 ****************************************************
          REGISTER YOUR DOORS IN HERE
 ****************************************************
 ****************************************************/

static void registerDoors(void) {
  DPUSH; /* #14 */

  // addDoor("NAME", 
  //         initFunc, openFunc, closeFunc, readFunc, writeFunc, bitLevel, loc)
#ifdef EXAMPLE_DOOR_CODE
#ifdef MUSH_DOORS
  addDoor("MUSH", NULL, mushDoorOpen, NULL, NULL, NULL, 0, -1);
#endif
#ifdef POP3_DOORS
  addDoor("MAIL", NULL, mailDoorOpen, mailDoorClose, mailDoorInput, mailDoorOutput, 0, -1);
#endif
#ifdef EMPIRE_DOORS
  addDoor("EMPIRE", NULL, empire_init, NULL, empire_from_empsrv, NULL, 0, -1);
#endif
  //  addDoor("TEST", testDoorInit, testDoorOpen, testDoorClose, testDoorInput, testDoorOutput, 0, dINTERNAL_e);
#endif

  VOIDRETURN; /* #14 */
}
