/* bsd.c */

#ifdef SOLARIS
/* Solaris has broken header declarations */
char *index(const char *, int);
void bzero(void *, int);
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

/* For MTU/MSS additions */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


/*hack*/
/* #define NSIG 32 */
#ifndef NSIG
#define NSIG 32
#endif

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "file_c.h"
#include "externs.h"
#include "interface.h"
#include "flags.h"
#include "alloc.h"
#include "command.h"
#include "attrs.h"
#include "mail.h"
#include "rhost_ansi.h"
#include "rhost_utf8.h"
#include "local.h"
#include "door.h"
#ifdef ENABLE_WEBSOCKETS
///// NEW WEBSOCK
#include "websock2.h"
///// END NEW WEBSOCK
#endif


#include "debug.h"
#define FILENUM BSD_C

#include <math.h>

#ifdef TLI
#include <sys/stream.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <stropts.h>
#include <poll.h>
#define shutdown(x,y)		0
extern int t_errno;
extern char *t_errlist[];
#endif

#if defined BSD_LIKE || defined TLI || defined SOLARIS
#ifndef MIN
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#endif
#endif

extern void NDECL(dispatch);
void NDECL(pcache_sync);

static int sock, sock2;
int ndescriptors = 0;

DESC *descriptor_list = NULL;
DESC *desc_in_use = NULL;

DESC *FDECL(initializesock, (int, struct sockaddr_in *, char *, int, int));
DESC *FDECL(new_connection, (int, int));
int FDECL(process_output, (DESC *));
int FDECL(process_input, (DESC *));
extern void FDECL(broadcast_monitor, (dbref, int, char *, char *, char *, int, int, int, char *));
extern int FDECL(lookup, (char *, char *, int, int *));
extern CF_HAND(cf_site);
extern double NDECL(next_timer);

extern int FDECL(alarm_msec, (double));
extern int NDECL(alarm_stop);
extern unsigned int CRC32_ProcessBuffer(unsigned int, const void *, unsigned int);

int signal_depth;

void populate_tor_seed(void)
{
   struct sockaddr_in *p_sock2;
   void *v_sock;
   struct addrinfo hints, *res, *p_res;
   char ipstr[INET_ADDRSTRLEN + 1], *tbuff, *tbuff2, *tpr2, *strtok, *mystrtok_r;
   char *tok1, *tok2, *tok3, *tok4, *tokptr;
   int i_validip;

   if ( !*(mudconf.tor_localhost) ) {
      strcpy(mudstate.tor_localcache, (char *)"1.0.0.127");
      return;
   }

   memset(&hints, 0, sizeof(hints));
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_family = AF_INET;

   tbuff = alloc_lbuf("populate_tor_seed");
   tpr2 = tbuff2 = alloc_lbuf("populate_tor_seed2");
   memset(tbuff2, '\0', LBUF_SIZE);
   strcpy(tbuff, mudconf.tor_localhost);

   strtok = strtok_r(tbuff, " \t", &mystrtok_r);

   i_validip = 0;
   while ( strtok ) {
      if ((getaddrinfo(strtok, NULL, &hints, &res)) == 0) {
         for( p_res = res; p_res != NULL; p_res = p_res->ai_next) {
            p_sock2 = (struct sockaddr_in *)p_res->ai_addr;
            v_sock = &(p_sock2->sin_addr);
   
            inet_ntop(p_res->ai_family, v_sock, ipstr, sizeof(ipstr)-1);
            tok1 = tok2 = tok3 = tok4 = NULL;
            tok1 = strtok_r(ipstr, ".", &tokptr);
            if ( tok1 ) {
               tok2 = strtok_r(NULL, ".", &tokptr);
               if ( tok2 ) {
                  tok3 = strtok_r(NULL, ".", &tokptr);
                  if ( tok3 ) {
                     tok4 = strtok_r(NULL, ".", &tokptr);
                  }
               }
            }
            if ( !tok4 ) {
               continue;
            }
            if ( i_validip )
               safe_chr(' ', tbuff2, &tpr2);
            safe_str(tok4, tbuff2, &tpr2);
            safe_chr('.', tbuff2, &tpr2);
            safe_str(tok3, tbuff2, &tpr2);
            safe_chr('.', tbuff2, &tpr2);
            safe_str(tok2, tbuff2, &tpr2);
            safe_chr('.', tbuff2, &tpr2);
            safe_str(tok1, tbuff2, &tpr2);
            i_validip = 1;
         }
         freeaddrinfo(res);
      }
      strtok = strtok_r(NULL, " \t", &mystrtok_r);
   }
   tpr2 = tbuff2 + 999;
   while ( tpr2 && *tpr2 && !isspace(*tpr2) )
      tpr2--;
   *tpr2 = '\0';
   if ( !*tbuff2 ) {
      strcpy(mudstate.tor_localcache, (char *)"1.0.0.127");
   } else {
      strncpy(mudstate.tor_localcache, tbuff2, 999);
   }
   free_lbuf(tbuff);
   free_lbuf(tbuff2);
}

/*********************************************************************************************
 * 1. Need a mudstate.tor_ipaddr 1000 char buffer to house the tor-translated addresses
 * 2. Need to write a mudconf.tor_localhost to mudstate.tor_ipaddr cache routine
 * 3. Need to loop for each entry in tor_ipaddr for a tor lookup *bleh*
 * 4. This function needs re-arrangement to minimalize as much as we can on overhead
 */
int check_tor(struct in_addr a_remote, int i_port) {
   char *s_reverselocal, *s_reverseremote, *s_tordns, *tortok, *tortokptr,
        *s_tmp, *s_tok1, *s_tok2, *s_tok3, *s_tok4, *s_tokptr, *s_tmp2,
        ipstr[INET_ADDRSTRLEN + 1];
   int i_found;
   struct addrinfo hints, *res, *p_res;
   struct sockaddr_in *p_sock2;
   void *v_sock;

   if ( !*(mudstate.tor_localcache) )
      populate_tor_seed();


   s_tmp           = alloc_lbuf("check_tor_local");
   s_reverselocal  = alloc_sbuf("check_tor_1");
   s_reverseremote = alloc_sbuf("check_tor_2");
   s_tordns        = alloc_lbuf("check_tor_3");

   memset(s_tmp, '\0', LBUF_SIZE);
   strcpy(s_tmp, mudstate.tor_localcache);
   tortok = strtok_r(s_tmp, " \t", &tortokptr);
   i_found = 0;
   while (tortok) {
      sprintf(s_reverselocal, "%s", tortok);
      s_tmp2 = inet_ntoa(a_remote);
      s_tok1 = s_tok2 = s_tok3 = s_tok4 = NULL; 
      s_tok1 = strtok_r(s_tmp2, ".", &s_tokptr);
      if ( s_tok1 ) {
         s_tok2 = strtok_r(NULL, ".", &s_tokptr);
         if ( s_tok2 ) {
            s_tok3 = strtok_r(NULL, ".", &s_tokptr);
            if ( s_tok3 ) {
               s_tok4 = strtok_r(NULL, ".", &s_tokptr);
            }
         }
      }
      if ( s_tok4 ) {
         sprintf(s_reverseremote, "%s.%s.%s.%s", s_tok4, s_tok3, s_tok2, s_tok1);
         sprintf(s_tordns, "%s.%d.%s.ip-port.exitlist.torproject.org", s_reverseremote, i_port, s_reverselocal);
         memset(&hints, 0, sizeof(hints));
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_family = AF_INET;
         if ((getaddrinfo(s_tordns, NULL, &hints, &res)) == 0) {
            memset(ipstr, '\0', sizeof(ipstr));
            for( p_res = res; p_res != NULL; p_res = p_res->ai_next) {
               p_sock2 = (struct sockaddr_in *)p_res->ai_addr;
               v_sock = &(p_sock2->sin_addr);
            
               inet_ntop(p_res->ai_family, v_sock, ipstr, sizeof(ipstr)-1);
               if ( strcmp(ipstr, "127.0.0.2") == 0 ) {
                  i_found = 1;
                  break;
               }
            }
            freeaddrinfo(res);
            if ( i_found )
               break;
         }
      }
      tortok = strtok_r(NULL, " \t", &tortokptr);
   }
   free_lbuf(s_tmp);
   free_sbuf(s_reverselocal);
   free_sbuf(s_reverseremote);
   free_lbuf(s_tordns);

   return i_found;
}

int make_socket(int port, char* address)
{
    int s, opt, i_loop;
    FILE *f_fptr;

#ifdef TLI
    struct t_bind *bindreq, *bindret;
    struct sockaddr_in *serverreq, *serverret;

#else
    struct sockaddr_in server;

#endif
    DPUSH; /* #1 */

#ifdef TLI
    s = t_open(TLI_TCP, (O_RDWR | O_NDELAY), NULL);
#else
    s = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (s < 0) {
	log_perror("NET", "FAIL", NULL, "creating master socket");
        DPOP; /* #1 */
	exit(3);
    }
#ifdef TLI
    bindreq = (struct t_bind *) t_alloc(s, T_BIND, T_ALL);
    if (!bindreq) {
	log_perror("NET", "FAIL", NULL, "t_alloc(req)");
	close(s);
        DPOP; /* #1 */
	exit(4);
    }
    bindret = (struct t_bind *) t_alloc(s, T_BIND, T_ALL);
    if (!bindret) {
	log_perror("NET", "FAIL", NULL, "t_alloc(ret)");
	close(s);
        DPOP; /* #1 */
	exit(4);
    }
    serverreq = (struct sockaddr_in *) bindreq->addr.buf;
    serverreq->sin_family = AF_INET;
    if(inet_addr((const char*)address) != -1)
        serverreq->sin_addr.s_addr = inet_addr((const char*)address);
    else
        serverreq->sin_addr.s_addr = INADDR_ANY;
    serverreq->sin_port = htons(port);
    bindreq->addr.len = sizeof(struct sockaddr_in);
    bindret->addr.maxlen = sizeof(struct sockaddr_in);

    bindreq->qlen = 5;

    if (t_bind(s, bindreq, bindret) < 0) {
	log_perror("NET", "FAIL", NULL, "t_bind");
	close(s);
        DPOP; /* #1 */
	exit(5);
    }
    serverret = (struct sockaddr_in *) bindret->addr.buf;
    if (serverreq->sin_port != serverret->sin_port) {
	log_perror("NET", "FAIL", NULL, "t_bind(portinuse)");
	close(s);
        DPOP; /* #1 */
	exit(6);
    }
#else
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &opt, sizeof(opt)) < 0) {
	log_perror("NET", "FAIL", NULL, "setsockopt");
    }
    server.sin_family = AF_INET;
    if(inet_addr((const char*)address) != -1)
        server.sin_addr.s_addr = inet_addr((const char*)address);
    else
        server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    i_loop = 0;
    if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
        while ( i_loop <= 5 ) {
           i_loop++;
           if ( mudconf.api_port == port ) {
	      log_perror("NET", "FAIL", unsafe_tprintf("[%d of 5] failed to bind to API port <%d>", i_loop, port), "bind");
           } else {
	      log_perror("NET", "FAIL", unsafe_tprintf("[%d of 5] failed to bind to port <%d>", i_loop, port), "bind");
           }
           if ( bind(s, (struct sockaddr *) &server, sizeof(server)) == 0 ) {
              break;
           } else {
              /* Sleep for 10 seconds and try to rebind */
              nanosleep((struct timespec[]){{10, 0}}, NULL);
           }
        }
        if ( i_loop >= 5 ) {
           if ( mudconf.api_port == port ) {
	      log_perror("NET", "FAIL", unsafe_tprintf("[max] Aborting -- failed to bind to API port <%d>", port), "bind");
           } else {
	      log_perror("NET", "FAIL", unsafe_tprintf("[max] Aborting -- failed to bind to port <%d>", port), "bind");
           }
	   close(s);
           DPOP; /* #1 */
	   exit(4);
        }
    }
    listen(s, 5);
#endif
    STARTLOG(LOG_STARTUP, "INI", "BSD")
      if ( mudconf.api_port == port ) {
         log_text(unsafe_tprintf("RhostMUSH API is now listening on port <%d>", port));
      } else {
         log_text(unsafe_tprintf("RhostMUSH is now listening on port <%d>", port));
      }
    ENDLOG
    if ( (f_fptr = fopen("netrhost.pid", "w")) != NULL ) {
       fprintf(f_fptr, "%d\n", (int)getpid());
       fclose(f_fptr);
    }
    RETURN(s); /* #1 */
}

#ifndef HAVE_GETTIMEOFDAY
#define get_tod(x)	{ (x)->tv_sec = time(NULL); (x)->tv_usec = 0; }
#else
#define get_tod(x)	gettimeofday(x, (struct timezone *)0)
#endif

int maxd;



void 
shovechars(int port,char* address)
{
    fd_set input_set, output_set;
    struct timeval last_slice, current_time, next_slice, timeout;
    int found, check, new_connection_error_count = 0;
    DESC *d, *dnext, *newd;
    CMDENT *cmdp = NULL;
    int avail_descriptors, maxfds, active_auths, aflags2, temp1, temp2;
    int sitecntr, i_oldlasttime, i_oldlastcnt, flagkeep, i_len, i_cntr;
    dbref aowner2;
    char *logbuff, *progatr, all[10], tsitebuff[1001], *ptsitebuff, s_cutter[6], 
         s_cutter2[8], *progatr_str, *progatr_strptr, *s_progatr, *b_progatr, 
         *t_progatr, *b_progatrptr, *tstrtokr;
#ifdef ZENTY_ANSI
    char *s_buff, *s_buffptr, *s_buff2, *s_buff2ptr, *s_buff3, *s_buff3ptr;
#endif
    FILE *f;
    int silent, i_progatr, anum, apiport;
    unsigned int ulCRC32;
    ATTR *ap;

#ifdef TLI
    struct pollfd *fds;
    int i;

#define CheckInput(x)	((fds[x].revents & POLLIN) == POLLIN)
#define CheckOutput(x)	((fds[x].revents & POLLOUT) == POLLOUT)
#else
#define CheckInput(x)	FD_ISSET(x, &input_set)
#define CheckOutput(x)	FD_ISSET(x, &output_set)
#endif

    DPUSH; /* #2 */
    apiport = mudconf.api_port;
    mudstate.debug_cmd = (char *) "< shovechars >";
    sock = make_socket(port, address);
    maxd = sock + 1;
    if ( apiport != -1 ) {
       sock2 = make_socket(apiport, address);
       if ( sock2 > sock ) 
          maxd = sock2 + 1;
    }
    get_tod(&last_slice);
    flagkeep = i_oldlasttime = i_oldlastcnt = 0;
    f = fopen("reboot.silent","r");
    silent=0;
    found = -1;
    if(f != NULL) {
      silent=1;
      memset(tsitebuff, '\0', sizeof(tsitebuff));
      fgets(tsitebuff, sizeof(tsitebuff)-1, f);
      fclose(f);
      if ( *tsitebuff )
         found = atoi(tsitebuff);
      remove("reboot.silent");
    }

    /* we may be rebooting, so recalc maxd */
    DESC_ITER_ALL(d) {
      if( d->descriptor >= maxd ) {
        maxd = d->descriptor + 1;
      }
      if ((d->flags & DS_HAS_DOOR) && (d->door_desc >= maxd)) {
	maxd = d->door_desc + 1;
      }
      /* If we rebooted after the d->longaddr addition, makes sure
         all current d->addrs get copied over --Amb */
      if( (d->longaddr[0] != '\0') || (d->longaddr[255] != '\0'))
      {
        memset(d->longaddr, '\0', sizeof(d->longaddr));
        strncpy(d->longaddr,d->addr,sizeof(d->longaddr));
      }
      if( d->flags & DS_CONNECTED ) {
        if(!silent) {
          queue_string(d, "Game: New server image successfully loaded.\r\n");
        } else if ( d->player == found ) {
          queue_string(d, "Game: Finished Rebooting Silently.\r\n"); 
        }
        strncpy(all, Name(d->player), 5);
        *(all + 5) = '\0';
        if ( strlen(mudconf.guest_namelist) > 0 ) {
           memset(tsitebuff, 0, sizeof(tsitebuff));
           strncpy(tsitebuff, mudconf.guest_namelist, 1000);
           ptsitebuff = strtok_r(tsitebuff, " \t", &tstrtokr);
           sitecntr = 1;
           while ( (ptsitebuff != NULL) && (sitecntr < 32) ) {
              if ( lookup_player(NOTHING, ptsitebuff, 0) == d->player ) {
                 temp1 = sitecntr;
                 temp2 = 0x00000001;
                 temp2 <<= (temp1 - 1);
                 mudstate.guest_status |= temp2;
                 mudstate.guest_num++;
              }
              ptsitebuff = strtok_r(NULL, " \t", &tstrtokr);
              sitecntr++;
           }
        } else if (!stricmp(all, "guest")) {
            temp1 = atoi((Name(d->player) + 5));
            temp2 = 0x00000001;
            temp2 <<= (temp1 - 1);
            mudstate.guest_status |= temp2;
            mudstate.guest_num++;
        }
      }
      if ( Good_obj(d->player) && InProgram(d->player) ) {
         if ( (mudconf.login_to_prog && !(ProgCon(d->player))) ||
              (!mudconf.login_to_prog && ProgCon(d->player)) ) {
            queue_string(d, "You are still in a @program.\r\n");
            progatr = atr_get(d->player, A_PROGPROMPTBUF, &aowner2, &aflags2);
            if ( progatr && *progatr ) {
               if ( strcmp(progatr, "NULL") != 0 ) {
#ifdef ZENTY_ANSI
                  s_buffptr = s_buff = alloc_lbuf("parse_ansi_prompt");
                  s_buff2ptr = s_buff2 = alloc_lbuf("parse_ansi_prompt2");
                  s_buff3ptr = s_buff3 = alloc_lbuf("parse_ansi_prompt3");
                  parse_ansi((char *) progatr, s_buff, &s_buffptr, s_buff2, &s_buff2ptr, s_buff3, &s_buff3ptr);
                  queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, s_buff, ANSI_NORMAL));
                  free_lbuf(s_buff);
                  free_lbuf(s_buff2);
                  free_lbuf(s_buff3);
#else
                  queue_string(d, unsafe_tprintf("%s%s%s \377\371", ANSI_HILITE, progatr, ANSI_NORMAL));
#endif
               }
            } else {
               queue_string(d, unsafe_tprintf("%s>%s \377\371", ANSI_HILITE, ANSI_NORMAL));
            }
            free_lbuf(progatr);
         } else {
            queue_string(d, "Your @program was aborted from the @reboot.\r\n");
            s_Flags4(d->player, (Flags4(d->player) & (~INPROGRAM)));
            queue_string(d, "\377\371");
            mudstate.shell_program = 0;
            atr_clr(d->player, A_PROGBUFFER);
            atr_clr(d->player, A_PROGPROMPTBUF);
         }
      }
    }

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf(_SC_OPEN_MAX);
#endif

#ifdef TLI
    fds = (struct pollfd *) malloc(sizeof(struct pollfd) * maxfds);

#endif
    avail_descriptors = maxfds - 36;

    STARTLOG(LOG_STARTUP, "INI", "BSD")
	log_text((char *) "Max Descriptors: ");
    log_text((char *) unsafe_tprintf("%d", avail_descriptors));
    mudstate.max_logins_allowed = avail_descriptors - 10;
    ENDLOG

    while (mudstate.shutdown_flag == 0) {

        /* Beginning of a new cycle - free unsafe_tprint buffers */
        freeTrackedBuffers();

	get_tod(&current_time);
	last_slice = update_quotas(last_slice, current_time);

	process_commands();
#ifdef ENABLE_DOORS
	door_processInternalDoors();
#endif
	local_tick();
	if (mudstate.shutdown_flag || mudstate.reboot_flag)
	    break;

	/* test for events */

	dispatch();

	/* any queued robot commands waiting? */

	double next = roundf(que_next() * (double)mudconf.mtimer) / (double)mudconf.mtimer;
	timeout.tv_sec = floor(next);
	timeout.tv_usec = floor(1000000 * fmod(next,(double)mudconf.mtimer / 10.0));
	next_slice = msec_add(last_slice, mudconf.timeslice);
	timeval_sub(next_slice, current_time);

#ifdef TLI
	for (i = 0; i < maxfds; i++)
	    fds[i].fd = -1;
#else
	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
#endif

	/* Listen for new connections if there are free descriptors */

	if (ndescriptors < avail_descriptors) {
#ifdef TLI
	    fds[sock].fd = sock;
	    fds[sock].events = POLLIN;
            if ( apiport != -1 ) {
	       fds[sock2].fd = sock2;
	       fds[sock2].events = POLLIN;
            }
#else
	    FD_SET(sock, &input_set);
            if ( apiport != -1 ) {
	       FD_SET(sock2, &input_set);
            }
#endif
	}

        active_auths = 0;

	/* Mark sockets that we want to test for change in status */
	DESC_ITER_ALL(d) {
#ifdef TLI
	    if (!d->input_head || d->output_head) {
		fds[d->descriptor].fd = d->descriptor;
		fds[d->descriptor].events = 0;
		if (!d->input_head)
		    fds[d->descriptor].events |= POLLIN;
		if (d->output_head)
		    fds[d->descriptor].events |= POLLOUT;
	    }
#else
	    if (!d->input_head && (d->flags & DS_AUTH_IN_PROGRESS) == 0)
		FD_SET(d->descriptor, &input_set);
	    if (d->output_head)
		FD_SET(d->descriptor, &output_set);
	    if (d->flags & DS_AUTH_IN_PROGRESS) {
                if ( d->flags & DS_API ) {
                   d->flags &= ~DS_AUTH_IN_PROGRESS;
                } else {
                   active_auths++;
		   if (d->authdescriptor >= maxd)
		       maxd = d->authdescriptor + 1;
		   if (d->flags & (DS_NEED_AUTH_WRITE|DS_AUTH_CONNECTING))
		       FD_SET(d->authdescriptor, &output_set);
		   FD_SET(d->authdescriptor, &input_set);
               }
	    }
	    if (d->flags & DS_HAS_DOOR) {
	      if (d->door_desc >= maxd)
		maxd = d->door_desc + 1;
	      if (!d->door_input_head)
		FD_SET(d->door_desc, &input_set);
	      if (d->door_output_head)
		FD_SET(d->door_desc, &output_set);
	    }
#endif
	}
	door_registerInternalDoorDescriptors(&input_set, &output_set, &maxd);


        /* Enable AUTH timeout processing */
        if( active_auths > 0 ) {
	  timeout.tv_sec = MIN(timeout.tv_sec,1);
        }

	/* Wait for something to happen */
#ifdef TLI
	found = poll(fds, maxfds, timeout.tv_sec);
#else
	found = select(maxd, &input_set, &output_set, (fd_set *) NULL,
		       &timeout);
#endif
	if (found < 0) {
	    if (errno != EINTR) {
		(mudstate.scheck)++;
		switch(errno) {
		  case EBADF:
			fprintf(stderr,"BAD FILE DESCRIPTOR\n");
			break;
		  case EINVAL:
			fprintf(stderr,"N is NEGATIVE\n");
			break;
		  case ENOMEM:
			fprintf(stderr,"NO MEMORY\n");
		}
		log_perror("NET", "FAIL",
			   "checking for activity, clearing descriptor queues", "select");
		DESC_ITER_ALL(d) {
		  freeqs(d,0);
		}
		if (mudstate.scheck > 2) {
		  log_perror("NET","FAIL","Repeated select failures.  Shutting down.","select");
		  break;
                }
	    }
	}

        /* NOTE: Some firewalls discard AUTH connection attempts
        ** instead of refusing the connection.  This can cause
        ** the connection to hang with no activity on the port.
        ** That is why we must check for timeouts here before
        ** we detect if there is socket activity. -Thorin 04/2002
        */
        if( active_auths > 0 ) {
	  DESC_SAFEITER_ALL(d, dnext) {
	    if( (d->flags & DS_AUTH_IN_PROGRESS) &&
	        (time(NULL) - d->connected_at >= 3) ) {
		d->flags &= ~DS_AUTH_IN_PROGRESS;
	  	logbuff = alloc_mbuf("shovechars.LOG.authtimeout");
                sprintf(logbuff, "%s 255.255.255.255", inet_ntoa(d->address.sin_addr));
                cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NOAUTH | H_AUTOSITE), 0, 1, "noauth_site");
	      	shutdown(d->authdescriptor, 2);
		close(d->authdescriptor);
              	strcpy(d->userid,"");
		STARTLOG(LOG_NET, "NET", "FAIL")
  		  sprintf(logbuff,
	   	  	"[%d/%s] Auth request timed out",
		    	d->descriptor,
			d->longaddr);
		  log_text(logbuff);
	 	ENDLOG
		free_mbuf(logbuff);
	    }
          }
        }

	if (found < 0) {
	    continue;
        }
	mudstate.scheck = 0;

	/* if !found then time for robot commands */

	if (!found) {
	    do_top(mudconf.queue_chunk);
	    continue;
	} else {
	    do_top(mudconf.active_q_chunk);
	}

	/* Check for new connection requests */

	check = CheckInput(sock);
	if (check) {
	    newd = new_connection(sock, 0);
	    if (!newd) {
#ifdef TLI
		check = (errno && (errno != ENFILE));
#else
		check = (errno && (errno != EINTR) &&
			 (errno != EMFILE) &&
			 (errno != ENFILE));
#endif
		if (check) {
                    if( ++new_connection_error_count == 10 ) {
                      log_perror("NET", "FAIL", NULL,
                                 "new_connection/stop");
                    }
                    else if( new_connection_error_count < 10 ) {
		      log_perror("NET", "FAIL", NULL,
		   	         "new_connection");
                    }
		}
	    } else {
		if (newd->descriptor >= maxd)
		    maxd = newd->descriptor + 1;
	    }
	}
        if ( apiport != -1 ) {
	   check = CheckInput(sock2);
	   if (check) {
	       newd = new_connection(sock2, 1);
	       if (!newd) {
#ifdef TLI
		   check = (errno && (errno != ENFILE));
#else
		   check = (errno && (errno != EINTR) &&
			    (errno != EMFILE) &&
			    (errno != ENFILE));
#endif
		   if (check) {
                       if( ++new_connection_error_count == 10 ) {
                         log_perror("NET", "FAIL", NULL,
                                    "new_connection/stop");
                       }
                       else if( new_connection_error_count < 10 ) {
		         log_perror("NET", "FAIL", NULL,
		   	            "new_connection");
                       }
		   }
	       } else {
		   if (newd->descriptor >= maxd)
		       maxd = newd->descriptor + 1;
	       }
	   }
        }

	/* Check for activity on user sockets */
	DESC_SAFEITER_ALL(d, dnext) {
	    /* Check for Auth */

	    if (d->flags & DS_AUTH_IN_PROGRESS) {
		if ((d->flags & DS_AUTH_CONNECTING) &&
                    CheckOutput(d->authdescriptor)) {
                    check_auth_connect(d);
                }
		else if ((d->flags & DS_NEED_AUTH_WRITE) &&
		    CheckOutput(d->authdescriptor)) {
		    write_auth(d);
                }
                else if( CheckInput(d->authdescriptor) ) {
		      check_auth(d);
                }
	    }

	    if (d->flags & DS_HAS_DOOR) {
	      if (CheckInput(d->door_desc)) {
		d->last_time = mudstate.now;
		if (!process_door_input(d)) {
		  closeDoorWithId(d, d->door_num);
		}
	      }
	    }
	    /* Lensy:
	     *  Additional check needed because the above call might close the conny
	     */
	    if (d->flags & DS_HAS_DOOR) {
	      if (CheckOutput(d->door_desc)) {
		if (!process_door_output(d)) {
		  closeDoorWithId(d, d->door_num);
		}
	      }
	    }
	    /* Process input from sockets with pending input */

	    check = CheckInput(d->descriptor);
	    if (check) {

		/* Undo autodark */

                i_oldlasttime = d->last_time;
                flagkeep = d->flags;
                if ( Good_obj(d->player) && !TogHideIdle(d->player) ) {
                   d->last_time = mudstate.now;
                   if (d->flags & DS_AUTODARK) {
                       d->flags &= ~DS_AUTODARK;
                       s_Flags(d->player,
                               Flags(d->player) & ~DARK);
                   }
                   if (d->flags & DS_AUTOUNF) {
                       d->flags &= ~DS_AUTOUNF;
                       s_Flags2(d->player,
                               Flags2(d->player) & ~UNFINDABLE);
                   }
                } else if ( d->last_time == 0 ) {
                   d->last_time = mudstate.now;
                }

		/* Process received data */

                i_oldlastcnt = d->input_tot;
				
		if (!process_input(d)) {
		    shutdownsock(d, R_SOCKDIED);
		    continue;
		}

                /* Idle stamp checking for command typed */
                if ( mudconf.idle_stamp && (d->flags & DS_CONNECTED) && d->input_head && (char *)(d->input_head->cmd) ) {
                   ulCRC32 = 0;
                   i_len = strlen(d->input_head->cmd);
                   ulCRC32 = CRC32_ProcessBuffer(ulCRC32, d->input_head->cmd, i_len);
                   anum = mkattr("_IDLESTAMP");
                   if ( anum > 0 ) {
                      ap = atr_num(anum);
                      if (ap) {
                         attr_wizhidden("_IDLESTAMP");
                         progatr = atr_get(d->player, ap->number, &aowner2, &aflags2);
                         if ( progatr ) {
                            progatr_str = progatr;
                            i_progatr = 0;
                            while ( progatr_str && *progatr_str ) {
                               if ( *progatr_str == ' ' )
                                  i_progatr++;
                               progatr_str++;
                            }
                            s_progatr = alloc_sbuf("idle_stamp");
                            sprintf(s_progatr, "%u", ulCRC32);
                            if ( (i_progatr >= (mudconf.idle_stamp_max-1)) && (strstr(progatr, s_progatr) == NULL) ) {
                               progatr_str = strtok_r(progatr, " ", &progatr_strptr);
                               if ( progatr_str )
                                  progatr_str = strtok_r(NULL, " ", &progatr_strptr);
                               /* Let's catch up to the current value if i_progatr - 1 still > mudconf.idle_stamp_max */
                               if ( ((i_progatr - 1) >= (mudconf.idle_stamp_max-1)) && progatr_str )
                                  progatr_str = strtok_r(NULL, " ", &progatr_strptr);
                            } else {
                               progatr_str = strtok_r(progatr, " ", &progatr_strptr);
                            }
                            b_progatrptr = b_progatr = alloc_lbuf("idle_stamp");
                            i_progatr = 0;
                            anum = 0;
                            i_cntr = 0;
                            if ( progatr_str ) {
                               t_progatr = alloc_lbuf("idle_stamp_movetoend");
                               while ( progatr_str ) {
                                  if ( strstr(progatr_str, s_progatr) != NULL ) {
                                     anum = 1;
                                     if ( strchr(progatr_str, ':') != NULL ) {
                                        i_progatr = atoi(strchr(progatr_str, ':')+1); 
                                        i_progatr++;
                                        if(mudconf.idle_cmdcount > -1) {
                                           if(i_progatr > mudconf.idle_cmdcount)
                                              d->last_time = i_oldlasttime;
                                        }
                                     } else {
                                        i_progatr = 1;
                                     }
                                     if ( i_progatr > 1 ) {
                                        sprintf(t_progatr, "%u:%d", ulCRC32, i_progatr);
                                     } else {
                                        sprintf(s_progatr, "%u:%d", ulCRC32, i_progatr);
                                        if ( i_cntr > 0 )
                                           safe_chr(' ', b_progatr, &b_progatrptr);
                                        safe_str(s_progatr, b_progatr, &b_progatrptr);
                                        i_cntr++;
                                     }
                                  } else {
                                     if ( i_cntr > 0 )
                                        safe_chr(' ', b_progatr, &b_progatrptr);
                                     safe_str(progatr_str, b_progatr, &b_progatrptr);
                                     i_progatr = 1;
                                     i_cntr++;
                                  }
                                  progatr_str = strtok_r(NULL, " ", &progatr_strptr);
                               }
                               /* Move the last command issued to end of the list if existed */
                               if ( *t_progatr ) {
                                  if ( i_cntr > 0 ) {
                                     safe_chr(' ', b_progatr, &b_progatrptr);
                                  }
                                  safe_str(t_progatr, b_progatr, &b_progatrptr);
                               }
                               free_lbuf(t_progatr);
                            } 
                            if ( !anum ) {
                               if ( i_progatr )
                                  safe_chr(' ', b_progatr, &b_progatrptr);
                               sprintf(s_progatr, "%u:1", ulCRC32);
                               safe_str(s_progatr, b_progatr, &b_progatrptr);
                            }
                            atr_add_raw(d->player, ap->number, b_progatr);
                            free_sbuf(s_progatr);
                            free_lbuf(b_progatr);
                         }
                         free_lbuf(progatr);
                      }
                   }
                }

                if ( (d->flags & DS_CONNECTED) && d->input_head && (char *)(d->input_head->cmd) ) {
                   memcpy(s_cutter, d->input_head->cmd, 5);
                   memcpy(s_cutter2, d->input_head->cmd, 7);
                   s_cutter[5] = '\0';
                   s_cutter2[7] = '\0';
                   if ( Good_obj(d->player) && 
                       (((Wizard(d->player) || HasPriv(d->player, NOTHING, POWER_WIZ_IDLE, POWER5, NOTHING)) && (stricmp(s_cutter, "idle ") == 0)) || 
                        ((stricmp(s_cutter, "@@") == 0) && mudconf.null_is_idle) ||
                        (stricmp(s_cutter, "idle") == 0) ||
                        (stricmp(s_cutter2, "idle @@") == 0)) ) {
                      cmdp = (CMDENT *) hashfind("idle", &mudstate.command_htab);
                      if ( cmdp && check_access(d->player, cmdp->perms, cmdp->perms2, 0)) {
                         if ( !(CmdCheck(d->player) && cmdtest(d->player, "idle")) ) {
                            d->last_time = i_oldlasttime;
                            d->flags = d->flags | flagkeep;
                            if ( d->flags & DS_AUTOUNF ) 
                               s_Flags2(d->player, Flags2(d->player) | UNFINDABLE);
                            if ( d->flags & DS_AUTODARK ) 
                               s_Flags(d->player, Flags(d->player) | DARK);
                         }
                      }
                   }
                }

                /* Ignore Null Input */
                if ( (d->input_tot <= (i_oldlastcnt + 2)) && d->input_head && (char *)(d->input_head->cmd) &&
                     ((*(d->input_head->cmd) == '\r') || (*(d->input_head->cmd) == '\n')) ) {
                   d->last_time = i_oldlasttime;
                }
                /* Ignore Potato's broken NOP code */
                if ( (d->input_tot == (i_oldlastcnt + 3)) && !d->input_head ) {
                   d->last_time = i_oldlasttime;
                }
	    }

	    /* Process output for sockets with pending output */

	    check = CheckOutput(d->descriptor);
	    if (check) {
		if (!process_output(d)) {
		    shutdownsock(d, R_SOCKDIED);
		}
	    }
            if ( (d->flags & DS_API) ) {
		if ( (d->connected_at + 5) < time(NULL) ) {
			shutdownsock(d, R_API);
		}
            }
	}
	door_checkInternalDoorDescriptors(&input_set, &output_set);
	
    }
    DPOPCONDITIONAL; /* #2 */
}

const char *
addrout(struct in_addr a, int i_key)
{
    extern char *inet_ntoa();
    char *retval;
    char *logbuff;

    DPUSH; /* #3 */

    if (mudconf.use_hostname) {
	struct hostent *he;

        /* check the nodns_site list */

        if ( (i_key && mudconf.api_nodns) || 
             site_check(a, mudstate.special_list, 1, 0, H_NODNS) & H_NODNS ||
             blacklist_check(a, 1) ) {
          retval = inet_ntoa(a);
          RETURN(retval); /* #3 */
        }

	he = gethostbyaddr((char *) &a.s_addr, sizeof(a.s_addr), AF_INET);
	if (he) {
            retval = he->h_name;
            if ( !mudconf.nospam_connect ) {
               STARTLOG(LOG_NET, "NET", "DNS")
    	         logbuff = alloc_lbuf("bsd.addrout");
                 sprintf(logbuff, "%s = %s", inet_ntoa(a), retval);
                 log_text(logbuff);
                 free_lbuf(logbuff);
               ENDLOG
            }
	    RETURN(retval); /* #3 */
        } else {
    	    logbuff = alloc_lbuf("bsd.addrout");
            sprintf(logbuff, "%s 255.255.255.255", inet_ntoa(a));
            cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NODNS | H_AUTOSITE), 0, 1, "nodns_site");
            free_lbuf(logbuff);
            retval = inet_ntoa(a);
	    RETURN(retval); /* 3 */
        }
    } 
    retval = inet_ntoa(a);
    RETURN(retval); /* 3 */
}

#ifdef TLI
static void 
log_tli_error(const char *key, const char *routine,
	      int err, int t_err)
{
    char *buff;

    DPUSH; /* #4 */

    STARTLOG(LOG_NET, "NET", "BIND")
	buff = alloc_mbuf("log_tli_error");
    sprintf(buff, "%s failed, errno=%d(%s), t_errno=%d(%s)",
	    routine, err, sys_errlist[err],
	    t_err, t_errlist[t_err]);
    log_text(buff);
    free_mbuf(buff);
    ENDLOG
    DPOP; /* #4 */
}

#endif

#ifdef TLI
struct t_call *nc_call = (struct t_call *) NULL;

#endif

DESC *
new_connection(int sock, int key)
{
    int newsock, maxsitecon, maxtsitecon, cur_port, i_retvar = -1;
    char *buff, *buff1, *cmdsave, tchbuff[LBUF_SIZE], *tsite_buff;
    DESC *d, *dchk, *dchknext;
    struct sockaddr_in addr;
    static int spam_log = 0;
    char *logbuff, *addroutbuf;
    int myerrno = 0, i_chktor = 0, i_chksite = -1, i_forbid = 0, i_proxychk, i_addflags;
#ifndef __MACH__
#ifndef CYGWIN
#ifndef BROKEN_PROXY
    int i_mtu, i_mtulen, i_mss, i_msslen;
#endif
#endif
#endif

#ifndef TLI
    int addr_len;

#else
    int ret;

#endif

    DPUSH; /* #5 */

    cur_port = i_proxychk = 0;
    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< new_connection >";
    i_addflags = 0;
    if ( key ) {
       i_addflags = MF_API;
    }
#ifdef TLI
    if (!nc_call) {
	nc_call = (struct t_call *) t_alloc(sock, T_CALL, T_ALL);
	if (!nc_call) {
	    RETURN(0); /* #5 */
        }
    }
    if (t_listen(sock, nc_call) < 0) {
       RETURN(0); /* #5 */
    }

    sighold(SIGALRM);
    newsock = t_open(TLI_TCP, (O_RDWR | O_NDELAY), NULL);
    sigrelse(SIGALRM);

    if (newsock < 0) {
	log_tli_error("RES", "t_open", errno, t_errno);
	RETURN(0); /* #5 */
    }
    sighold(SIGALRM);
    ret = t_bind(newsock, NULL, NULL);
    sigrelse(SIGALRM);

    if (ret < 0) {
	log_tli_error("BIND", "t_bind", errno, t_errno);
	t_close(newsock);
	RETURN(0); /* #5 */
    }
    sighold(SIGALRM);
    ret = t_accept(sock, newsock, nc_call);
    sigrelse(SIGALRM);

    if (ret < 0) {
	log_tli_error("RES", "t_accept", errno, t_errno);
	t_close(newsock);
	RETURN(0); /* #5 */
    }
    /* push the read/write support module on the stream */

    sighold(SIGALRM);
    ret = ioctl(newsock, I_PUSH, "tirdwr");
    sigrelse(SIGALRM);

    if (ret < 0) {
	log_tli_error("STRM", "ioctl I_PUSH", errno, t_errno);
	t_close(newsock);
	RETURN(0); /* #5 */
    }
    bcopy(nc_call->addr.buf, &addr, sizeof(struct sockaddr_in));

#else
    addr_len = sizeof(struct sockaddr);

    newsock = accept(sock, (struct sockaddr *) &addr, (unsigned int *) &addr_len);
    if (newsock < 0) {
        myerrno = errno;
        spam_log++;
        if( spam_log < 10 ) {
          STARTLOG(LOG_NET, "NET", "FAIL")
      		logbuff = alloc_mbuf("new_connection.accept");
  	        sprintf(logbuff,
		    "new_connection: Accept failed with code %d",
		    myerrno);
	        log_text(logbuff);
	        free_mbuf(logbuff);
	  ENDLOG
        }
        else if( spam_log == 10 ) {
          spam_log++;
          STARTLOG(LOG_NET, "NET", "FAIL")
      		logbuff = alloc_mbuf("new_connection.accept2");
  	        sprintf(logbuff,
		    "new_connection: Accept failed with code %d",
		    myerrno);
	        log_text(logbuff);
  	        sprintf(logbuff,
		    "This will be the last time this is reported.");
	        log_text(logbuff);
	        free_mbuf(logbuff);
	  ENDLOG
        }
	RETURN(0); /* #5 */
    }
#endif /* TLI */

    memset(tchbuff, 0, sizeof(tchbuff));
    cur_port =  ntohs(addr.sin_port);

    /* DO TOR LOOKUP HERE IF ENABLED */
    if ( mudconf.tor_paranoid ) {
       i_chktor = check_tor(addr.sin_addr, mudconf.port);
    }

    tsite_buff = alloc_lbuf("check_max_sitecons");
    maxsitecon = 0;
    maxtsitecon = 0;

#ifndef __MACH__
#ifndef CYGWIN
#ifndef BROKEN_PROXY
    if ( mudconf.proxy_checker > 0 ) {
       /* Check MTU */
       i_mtulen = sizeof(i_mtu);
       i_msslen = sizeof(i_mss);
       getsockopt(newsock, SOL_IP, IP_MTU, &i_mtu, (unsigned int *)&i_mtulen);
       getsockopt(newsock, IPPROTO_TCP, TCP_MAXSEG,  &i_mss, (unsigned int*)&i_msslen);
   
       if ( (i_mtu <= 1500) && ((i_mss + 80) < i_mtu) ) { /* Possible Proxy -- Block */
          /* Ignore all this if user is in the bypass site list */
          addroutbuf = (char *) addrout(addr.sin_addr, (i_addflags & MF_API));
          strcpy(tsite_buff, addroutbuf);
          strcpy(tchbuff, mudconf.passproxy_host);
          if ( !( (site_check(addr.sin_addr, mudstate.suspect_list, 1, 0, H_PASSPROXY) == H_PASSPROXY) || 
                  ((char *)mudconf.passproxy_host && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar)) ) ) {
             STARTLOG(LOG_NET | LOG_SECURITY, "NET", "PROXY");
                buff = alloc_lbuf("new_connection.LOG.badsite");
                sprintf(buff, "[%d/%s] Possible Proxy [MTU %d/MSS %d].  (Remote port %d)",
                        newsock, inet_ntoa(addr.sin_addr), i_mtu, i_mss, cur_port);
                log_text(buff);
                free_lbuf(buff);
             ENDLOG
             if ( (mudconf.proxy_checker & 2) && (mudconf.proxy_checker & 4) ) {
                i_proxychk = H_NOGUEST | H_REGISTRATION;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [GUEST/REGISTER DISABLED]", NULL,
                                        inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
             } else if ( mudconf.proxy_checker & 2 ) {
                i_proxychk = H_NOGUEST;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [GUEST DISABLED]", NULL,
                                        inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
             } else if ( mudconf.proxy_checker & 4 ) {
                i_proxychk = H_REGISTRATION;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [REGISTER DISABLED]", NULL,
                                        inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
             } else {
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY", NULL,
                                        inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
             }
          } 
       }
    }
#endif
#endif
#endif

    /* DO BLACKLIST CHECK HERE */
    if ( blacklist_check(addr.sin_addr, 0) || i_chktor  ) {
       STARTLOG(LOG_NET | LOG_SECURITY, "NET", (i_chktor ? "TOR" : "BLACK"));
          buff = alloc_lbuf("new_connection.LOG.badsite");
          sprintf(buff, "[%d/%s] Connection refused - %s.  (Remote port %d)",
                  newsock, inet_ntoa(addr.sin_addr), (i_chktor ? "TOR" : "Blacklisted"), cur_port);
          log_text(buff);
          if ( i_chktor ) {
             broadcast_monitor(NOTHING, MF_CONN | i_addflags, "SITE IN TOR", NULL,
                               inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
          } else {
             broadcast_monitor(NOTHING, MF_CONN | i_addflags, "SITE IN BLACKLIST", NULL,
                               inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
          }
          free_lbuf(buff);
       ENDLOG
       fcache_rawdump(newsock, FC_CONN_SITE, addr.sin_addr, (char *)NULL);
       shutdown(newsock, 2);
       close(newsock);
       errno = 0;
       free_lbuf(tsite_buff);
       RETURN(0);
    }

    /* Ok, check all the sites that match this one.  If it does, don't allow it if > max */

    /* Only do this for API */
    if ( i_addflags & MF_API ) {
       mudstate.api_lastsite_cnt++;
       if ( ((mudstate.last_apicon_attempt + 60) < mudstate.now) || (mudstate.last_apicon_attempt == 0) ) {
          mudstate.last_apicon_attempt = mudstate.now;
       }
       strcpy(tchbuff, mudconf.passapi_host);
       addroutbuf = (char *) addrout(addr.sin_addr, (i_addflags & MF_API));
       if ( ((mudstate.api_lastsite_cnt >= mudconf.max_lastsite_api) &&
             (mudstate.api_lastsite == (int)addr.sin_addr.s_addr) &&
             (mudstate.last_apicon_attempt >= (mudstate.now - 60))) &&
           !((site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_PASSAPI) == H_PASSAPI) ||
             ((char *)mudconf.passapi_host && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar))) ) {
          sprintf(tchbuff, "%s 255.255.255.255", inet_ntoa(addr.sin_addr));
          if ( !(site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_FORBIDAPI) == H_FORBIDAPI) ) {
             cf_site((int *)&mudstate.access_list, tchbuff, H_FORBIDAPI|H_AUTOSITE, 0, 1, "forbidapi_site");
             broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("API SITE SET AUTO-FORBID[%d attempts/%ds time]", 
                               mudstate.api_lastsite_cnt, (mudstate.now - mudstate.last_apicon_attempt)),
                               NULL, inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
             STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOF")
                buff = alloc_lbuf("new_connection.LOG.autoforbidapi");
                sprintf(buff, "[%d/%s] marked for autoAPIforbid.  (Remote port %d)",
                 newsock, inet_ntoa(addr.sin_addr), cur_port);
                log_text(buff);
                free_lbuf(buff);
             ENDLOG
          }
          mudstate.api_lastsite_cnt = 0;
          memset(tchbuff, 0, sizeof(tchbuff));
       }
       mudstate.api_lastsite = (int)addr.sin_addr.s_addr;
    }
    /* Only do this for non-API */
    if ( !(i_addflags & MF_API) && (mudconf.lastsite_paranoia > 0) ) { 
     if ( ((mudconf.lastsite_paranoia > 1) && 
           !(site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN)) ||
          ((mudconf.lastsite_paranoia == 1) &&
           !(site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION)) ) {
       if ( (mudstate.cmp_lastsite_cnt >= mudconf.max_lastsite_cnt) &&
            (mudstate.cmp_lastsite == (int)addr.sin_addr.s_addr) &&
            (mudstate.last_con_attempt >= (mudstate.now - mudconf.min_con_attempt)) ) {
          sprintf(tchbuff, "%s 255.255.255.255", inet_ntoa(addr.sin_addr));
          if (mudconf.lastsite_paranoia == 1) {
             cf_site((int *)&mudstate.access_list, tchbuff,
                     H_REGISTRATION|H_AUTOSITE, 0, 1, "register_site");
  	     broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("SITE SET AUTO-REGISTER[%d attempts/%ds time]", 
                               mudstate.cmp_lastsite_cnt, 
                               (mudstate.now - mudstate.last_con_attempt)),
                               NULL, inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
	     STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOR")
	     buff = alloc_lbuf("new_connection.LOG.autoregister");
	     sprintf(buff, "[%d/%s] marked for autoregistration.  (Remote port %d)",
		     newsock, inet_ntoa(addr.sin_addr), cur_port);
	     log_text(buff);
	     free_lbuf(buff);
	     ENDLOG
          } else {
             cf_site((int *)&mudstate.access_list, tchbuff,
                     H_FORBIDDEN|H_AUTOSITE, 0, 1, "forbid_site");
  	     broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("SITE SET AUTO-FORBID[%d attempts/%ds time]", 
                               mudstate.cmp_lastsite_cnt, 
                               (mudstate.now - mudstate.last_con_attempt)),
                               NULL, inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
	     STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOF")
	     buff = alloc_lbuf("new_connection.LOG.autoregister");
	     sprintf(buff, "[%d/%s] marked for autoforbidding.  (Remote port %d)",
		     newsock, inet_ntoa(addr.sin_addr), cur_port);
	     log_text(buff);
	     free_lbuf(buff);
	     ENDLOG
           }
           mudstate.cmp_lastsite_cnt = 0;
       } else if ( (mudconf.lastsite_paranoia > 0) &&
                   ((mudstate.cmp_lastsite != (int)addr.sin_addr.s_addr) || 
                    (mudstate.last_con_attempt < (mudstate.now - mudconf.min_con_attempt))) ) {
          mudstate.cmp_lastsite = (int)addr.sin_addr.s_addr;
          mudstate.cmp_lastsite_cnt = 0;
       }
       if ( ((mudstate.last_con_attempt + mudconf.min_con_attempt) < mudstate.now) ||
             (mudstate.last_con_attempt == 0) ) {
          mudstate.last_con_attempt = mudstate.now;
          mudstate.cmp_lastsite_cnt = 0;
       }
       mudstate.cmp_lastsite_cnt++;
       memset(tchbuff, 0, sizeof(tchbuff));
     }
    }
/*  strcpy(tsite_buff, addrout(addr.sin_addr)); */
    addroutbuf = (char *) addrout(addr.sin_addr, (i_addflags & MF_API));
    strcpy(tsite_buff, addroutbuf);
    if ( tsite_buff ) {
       DESC_SAFEITER_ALL(dchk, dchknext) {
          if ( strcmp(tsite_buff, dchk->addr) == 0 )
             maxsitecon++;
          maxtsitecon++;
       }
    }

/* They're the same size... 1000 bytes */
    strcpy(tchbuff, mudconf.forbid_host);
    strcpy(tsite_buff, addroutbuf);
    if ((site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN) ||
        (maxsitecon >= mudconf.max_sitecons) || (maxtsitecon >= (mudstate.max_logins_allowed+7)) ||
        ((char *)mudconf.forbid_host && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar))) {

       i_chksite = site_check(addr.sin_addr, mudstate.access_list, 1, 1, H_FORBIDDEN);
       i_forbid = 1;
       if ( i_chksite != -1 ) {  
          if ( maxsitecon < i_chksite ) {
             i_forbid = 0;
          }
       }
    }
    if ( !i_forbid && (i_addflags & MF_API) ) {
       strcpy(tchbuff, mudconf.forbidapi_host);
       strcpy(tsite_buff, addroutbuf);
       if ((site_check(addr.sin_addr, mudstate.access_list, 1, 0, H_FORBIDAPI) == H_FORBIDAPI) ||
           (maxsitecon >= mudconf.max_sitecons) || (maxtsitecon >= (mudstate.max_logins_allowed+7)) ||
           ((char *)mudconf.forbidapi_host && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar))) {
   
          i_chksite = site_check(addr.sin_addr, mudstate.access_list, 1, 1, H_FORBIDAPI);
          i_forbid = 1;
          if ( i_chksite != -1 ) {  
             if ( maxsitecon < i_chksite ) {
                i_forbid = 0;
             }
          }
       }
    }
    free_lbuf(tsite_buff);
        
    if ( i_forbid ) {
        if ( !mudconf.nospam_connect ) { /* Ok, this removes possible spamming of logs */
	   STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
	       buff = alloc_lbuf("new_connection.LOG.badsite");
	   sprintf(buff, "[%d/%s] Connection refused.  (Remote port %d)",
		   newsock, inet_ntoa(addr.sin_addr), cur_port);
	   log_text(buff);
	   free_lbuf(buff);
	   ENDLOG
        } else if ( mudconf.nospam_connect == 1 ) {
           buff1 = inet_ntoa(addr.sin_addr);
           if ( !(*mudstate.nospam_lastsite) || (strcmp(mudstate.nospam_lastsite, buff1) != 0) ) {
              if ( mudstate.nospam_counter > 0 ) {
                 STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
                    buff = alloc_lbuf("new_connection.LOG.badsite");
                    sprintf(buff, "[%s] Connection refused [total %d times].",
                            mudstate.nospam_lastsite, mudstate.nospam_counter);
                    log_text(buff);
                    free_lbuf(buff);
                 ENDLOG
              }
              memset(mudstate.nospam_lastsite, '\0', sizeof(mudstate.nospam_lastsite));
              sprintf(mudstate.nospam_lastsite, "%.*s", (int)sizeof(mudstate.nospam_lastsite) - 1, buff1);
              mudstate.nospam_counter = 1;
           } else {
              mudstate.nospam_counter++;
           }
        }
        if ( maxsitecon >= mudconf.max_sitecons ) {
  	   broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("MAX OPEN PORTS[%d]", maxsitecon), NULL, 
                             inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
        } else if ( maxtsitecon >= (mudstate.max_logins_allowed+7) ) {
  	   broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("MAX OPEN DESCRIPTORS[%d]", maxtsitecon), NULL, 
                             inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
        } else {
           if ( i_chksite == -1 )
              i_chksite = i_retvar;
           if ( i_chksite != -1 ) {
  	      broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("PORT REJECT[%d max]", i_chksite), NULL, 
                                inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
           } else {
  	      broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT REJECT", NULL, 
                                inet_ntoa(addr.sin_addr), newsock, 0, cur_port, NULL);
           }
        }
        if ( i_addflags & MF_API ) {
           fcache_rawdump(newsock, FC_CONN_SITE, addr.sin_addr, (char *)"SITE_FORBIDAPI");
        } else {
           if ( i_chksite != -1 ) {
 	      fcache_rawdump(newsock, FC_CONN_SITE, addr.sin_addr, (char *)"SITE_FORBID");
           } else {
 	      fcache_rawdump(newsock, FC_CONN_SITE, addr.sin_addr, (char *)NULL);
           }
        }
	shutdown(newsock, 2);
	close(newsock);
	errno = 0;
	d = NULL;
    } else {
	buff = alloc_lbuf("new_connection.sitename");
        if ( mudconf.nospam_connect == 1 ) {
           if ( *mudstate.nospam_lastsite ) {
              if ( mudstate.nospam_counter > 0 ) {
                 STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
                    sprintf(buff, "[%s] Connection refused [total %d times].",
                            mudstate.nospam_lastsite, mudstate.nospam_counter);
                    log_text(buff);
                 ENDLOG
              }
              memset(mudstate.nospam_lastsite, '\0', sizeof(mudstate.nospam_lastsite));
              mudstate.nospam_counter = 0;
           }
        }
        memset(buff, 0, LBUF_SIZE);
  	strncpy(buff, strip_nonprint(addroutbuf), LBUF_SIZE - 1);
	STARTLOG(LOG_NET, "NET", "CONN")
	    buff1 = alloc_lbuf("new_connection.LOG.open");
	sprintf(buff1, "[%d/%s] Connection opened (remote port %d)",
		newsock, buff, cur_port);
	log_text(buff1);
	free_lbuf(buff1);
	ENDLOG
        if ( strcmp(buff, addroutbuf) != 0) {
	   STARTLOG(LOG_NET, "NET", "CONN")
           buff1 = alloc_lbuf("error_on_dns");
           sprintf(buff1, "Warning: Non-printable codes stripped in dns '%s'.", addroutbuf);
           log_text(buff1);
           free_lbuf(buff1);
           ENDLOG
        }
	d = initializesock(newsock, &addr, buff, i_proxychk, key);
        broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT CONNECT", NULL, buff, newsock, 0, cur_port, NULL);
	free_lbuf(buff);
	mudstate.debug_cmd = cmdsave;
    }
    mudstate.debug_cmd = cmdsave;

    if ( key && d ) {
       d->flags |= DS_API;
    }
    RETURN(d); /* #5 */
}

/* Disconnect reasons that get written to the logfile */

static const char *disc_reasons[] =
{
    "Unspecified",
    "Quit",
    "Inactivity Timeout",
    "Booted",
    "Remote Close or Net Failure",
    "Game Shutdown",
    "Login Retry Limit",
    "Logins Disabled",
    "Logout (Connection Not Dropped)",
    "Too Many Connected Players",
    "Reboot",
    "Attempted Hacking",
    "Too Many Open OS-Based Descriptors",
    "API Connection",
    "WebSockets Connection",
    "SU Account",
};

/* Disconnect reasons that get fed to A_ADISCONNECT via announce_disconnect */

static const char *disc_messages[] =
{
    "unknown",
    "quit",
    "timeout",
    "boot",
    "netdeath",
    "shutdown",
    "badlogin",
    "nologins",
    "logout",
    "toomany",
    "reboot",
    "hacking",
    "nodescriptor",
    "api",
    "websockets",
    "su"
};

void 
shutdownsock(DESC * d, int reason)
{
    char *buff, *buff2, all[10], tchbuff[LBUF_SIZE], tsitebuff[1001], *ptsitebuff;
    char *addroutbuf, *t_addroutbuf, *tstrtokr;
    time_t now;
    int temp1, temp2, sitecntr, i_sitecnt, i_guestcnt, i_sitemax, i_retvar = -1, i_addflags;
    struct SNOOPLISTNODE *temp;
    DESC *dchk, *dchknext;

    DPUSH; /* #6 */
    i_addflags = 0;
    if ( d->flags & DS_API ) {
       i_addflags = MF_API;
    }

    if ( ((reason == R_LOGOUT) || (reason == R_SU)) &&
         (site_check((d->address).sin_addr, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN)) {
	reason = R_QUIT;
        if( d->flags & DS_API ) {
           reason = R_API; 
        }
    }

    if (d->flags & DS_CONNECTED) {

	strncpy(all, Name(d->player), 5);
	*(all + 5) = '\0';
        if ( strlen(mudconf.guest_namelist) > 0 ) {
           memset(tsitebuff, 0, sizeof(tsitebuff));
           strncpy(tsitebuff, mudconf.guest_namelist, 1000);
           ptsitebuff = strtok_r(tsitebuff, " \t", &tstrtokr);
           sitecntr = 1;
           while ( (ptsitebuff != NULL) && (sitecntr < 32) ) {
              if ( lookup_player(NOTHING, ptsitebuff, 0) == d->player ) {
                 temp1 = sitecntr;
                 temp2 = 0x00000001;
                 temp2 <<= (temp1 - 1);
                 mudstate.guest_status &= ~temp2;
                 mudstate.guest_num--;
              }
              ptsitebuff = strtok_r(NULL, " \t", &tstrtokr);
              sitecntr++;
           }
        } else if (!stricmp(all, "guest")) {
	    temp1 = atoi((Name(d->player) + 5));
	    temp2 = 0x00000001;
	    temp2 <<= (temp1 - 1);
	    mudstate.guest_status &= ~temp2;
	    mudstate.guest_num--;
	}
	if (mudconf.maildelete)
	    mail_md1(d->player, d->player, 1, -1);
	strcpy(all, "all");
	mail_mark(d->player, M_READM, all, NOTHING, 1);
	atr_clr(d->player, A_MPSET);
	atr_clr(d->player, A_MCURR);

	/* Do the disconnect stuff if we aren't doing a LOGOUT
	 * (which keeps the connection open so the player can connect
	 * to a different character).
	 */

	if ( (reason != R_LOGOUT) && (reason != R_SU) ) {
	    fcache_dump(d, FC_QUIT, (char *)NULL);
	    STARTLOG(LOG_NET | LOG_LOGIN, "NET", "DISC")
		buff = alloc_lbuf("shutdownsock.LOG.disconn");
	    sprintf(buff, "[%d/%s] Logout by ",
		    d->descriptor, d->longaddr);
	    log_text(buff);
	    log_name(d->player);
	    sprintf(buff, " <Reason: %s>",
		    disc_reasons[reason]);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    STARTLOG(LOG_NET | LOG_LOGIN, "NET", "LOGO")
		buff = alloc_lbuf("shutdownsock.LOG.logout");
	    sprintf(buff, "[%d/%s] Logout by ",
		    d->descriptor, d->longaddr);
	    log_text(buff);
	    log_name(d->player);
	    sprintf(buff, " <Reason: %s>",
		    disc_reasons[reason]);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	}

	/* If requested, write an accounting record of the form:
	 * Plyr# Flags Cmds ConnTime Loc Money [Site] <DiscRsn> Name
	 */

	STARTLOG(LOG_ACCOUNTING, "DIS", "ACCT")
	    now = mudstate.now - d->connected_at;
	buff = alloc_lbuf("shutdownsock.LOG.accnt");
	buff2 = decode_flags(GOD, GOD, Flags(d->player),
			     Flags2(d->player), Flags3(d->player),
			     Flags4(d->player));
	sprintf(buff, "%d %s %d %ld %d %d [%s] <%s> %s",
		d->player, buff2, d->command_count, now,
		Location(d->player), Pennies(d->player),
		d->longaddr, disc_reasons[reason],
		Name(d->player));
	log_text(buff);
	free_lbuf(buff);
	free_mbuf(buff2);
	ENDLOG
	announce_disconnect(d->player, d, disc_messages[reason]);
    } else {
	if ( (reason == R_LOGOUT) || (reason == R_SU) ) {
	    reason = R_QUIT;
        }
        if ( d->flags & DS_API ) {
	    reason = R_API;
        }
        if ( d->flags & DS_WEBSOCKETS ) {
	    reason = R_WEBSOCKETS;
        }
	STARTLOG(LOG_SECURITY | LOG_NET, "NET", "DISC")
	    buff = alloc_lbuf("shutdownsock.LOG.neverconn");
	sprintf(buff,
		"[%d/%s] Connection closed, never connected. <Reason: %s>",
		d->descriptor, d->longaddr, disc_reasons[reason]);
	log_text(buff);
	free_lbuf(buff);
	ENDLOG
    }
    process_output(d);
    process_door_output(d);
    clearstrings(d);
    i_sitecnt = i_guestcnt = 0;
    if (d->flags & DS_HAS_DOOR) closeDoorWithId(d, d->door_num);
    if ( (reason == R_LOGOUT) || (reason == R_SU) ) {
        addroutbuf = (char *) addrout((d->address).sin_addr, (d->flags & DS_API));
        t_addroutbuf = alloc_lbuf("check_max_sitecons");
        strcpy(t_addroutbuf, addroutbuf);
        if ( t_addroutbuf ) {
           DESC_SAFEITER_ALL(dchk, dchknext) {
              if ( strcmp(t_addroutbuf, dchk->addr) == 0 ) {
                 if ( Good_chk(dchk->player) && Guest(dchk->player) ) {
                    i_guestcnt++;
                 }
                 i_sitecnt++;
              }
           }
        }
        free_lbuf(t_addroutbuf);
	d->flags &= ~DS_CONNECTED;
	d->connected_at = mudstate.now;
	d->retries_left = mudconf.retry_limit;
	d->regtries_left = mudconf.regtry_limit;
	d->command_count = 0;
	d->timeout = mudconf.idle_timeout;
	d->player = 0;
	d->doing[0] = '\0';
	d->quota = mudconf.cmd_quota_max;
	d->last_time = 0;
	d->host_info = site_check((d->address).sin_addr,
				  mudstate.access_list, 1, 0, 0) |
	    			  site_check((d->address).sin_addr,
				  mudstate.suspect_list, 0, 0, 0);
        i_sitemax = site_check((d->address).sin_addr, mudstate.access_list, 1, 1, H_FORBIDDEN);
        if ( (i_sitemax != -1) && (i_sitecnt < (i_sitemax + 1)) )
           d->host_info &= ~H_FORBIDDEN;
        i_sitemax = site_check((d->address).sin_addr, mudstate.access_list, 1, 1, H_REGISTRATION);
        if ( (i_sitemax != -1) && (i_sitecnt < (i_sitemax + 1)) )
           d->host_info &= ~H_REGISTRATION;
        i_sitemax = site_check((d->address).sin_addr, mudstate.access_list, 1, 1, H_NOGUEST);
        if ( (i_sitemax != -1) && (i_guestcnt < (i_sitemax + 1)) )
           d->host_info &= ~H_NOGUEST;
        i_sitemax = site_check((d->address).sin_addr, mudstate.access_list, 1, 1, H_HARDCONN);
        if ( i_sitemax != -1 ) 
           d->host_info &= ~H_HARDCONN;

        memset(tchbuff, 0, sizeof(tchbuff));
        strcpy(tchbuff, mudconf.forbid_host);
        if ((char *)mudconf.forbid_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_FORBIDDEN;
        strcpy(tchbuff, mudconf.register_host);
        if ((char *)mudconf.register_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_REGISTRATION;
        strcpy(tchbuff, mudconf.autoreg_host);
        if ((char *)mudconf.autoreg_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_NOAUTOREG;
        strcpy(tchbuff, mudconf.noguest_host);
        if ((char *)mudconf.noguest_host && lookup(d->longaddr, tchbuff, i_guestcnt, &i_retvar))
           d->host_info = d->host_info | H_NOGUEST;
        strcpy(tchbuff, mudconf.suspect_host);
        if ((char *)mudconf.suspect_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_SUSPECT;
        strcpy(tchbuff, mudconf.passproxy_host);
        if ((char *)mudconf.passproxy_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_PASSPROXY;
        strcpy(tchbuff, mudconf.hardconn_host);
        if ((char *)mudconf.hardconn_host && lookup(d->longaddr, tchbuff, i_sitecnt, &i_retvar))
           d->host_info = d->host_info | H_HARDCONN;
	d->input_tot = d->input_size;
	d->output_tot = 0;
         
        mudstate.total_bytesin += d->input_size;
        if ( (mudstate.reset_daily_bytes + 86400) < mudstate.now ) {
           if ( mudstate.avg_bytesin == 0 ) {
              mudstate.avg_bytesin = mudstate.daily_bytesin;
           } else {
              mudstate.avg_bytesin = (mudstate.avg_bytesin + mudstate.daily_bytesin) / 2;
           }
           if ( mudstate.avg_bytesout == 0 ) {
              mudstate.avg_bytesout = mudstate.daily_bytesout;
           } else {
              mudstate.avg_bytesout = (mudstate.avg_bytesout + mudstate.daily_bytesout) / 2;
           }
           mudstate.daily_bytesout = 0;
           mudstate.daily_bytesin = d->input_size;
           mudstate.reset_daily_bytes = time(NULL);
        } else {
           mudstate.daily_bytesin += d->input_size;
        }
 
	/* free up the snooplist before freeing the desc -Thorin */
          
	while (d->snooplist) {
	    temp = d->snooplist->next;
	    if (d->snooplist->sfile) {
		fprintf(d->snooplist->sfile, "Snoop stopped due to logout: (%ld) %s", mudstate.now, ctime(&mudstate.now));
		fclose(d->snooplist->sfile);
	    }
	    free(d->snooplist);
	    d->snooplist = temp;
	}
	d->snooplist = NULL;
	d->logged = 0;
        if ( !mudstate.no_announce ) {
	   welcome_user(d);
        }
    } else {
	broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT DISCONNECT", d->userid, d->longaddr, d->descriptor, 0, 0, (char *)disc_reasons[reason]);
	shutdown(d->descriptor, 2);
	close(d->descriptor);
	freeqs(d,0); /* 0 is for all */
	*d->prev = d->next;
	if (d->next)
	    d->next->prev = d->prev;
	/* free up the snooplist before freeing the desc -Thorin */
	while (d->snooplist) {
	    temp = d->snooplist->next;
	    if (d->snooplist->sfile) {
		fprintf(d->snooplist->sfile, "Snoop stopped due to shutdown of descriptor: (%ld) %s", mudstate.now, ctime(&mudstate.now));
		fclose(d->snooplist->sfile);
	    }
	    free(d->snooplist);
	    d->snooplist = temp;
	}
	if (d->flags & DS_AUTH_IN_PROGRESS) {
	    shutdown(d->authdescriptor, 2);
	    close(d->authdescriptor);
	}
	free_desc(d);
	ndescriptors--;
    }
    DPOP; /* #6 */
}

void 
make_nonblocking(int s)
{
#ifndef TLI
#ifdef HAVE_LINGER
    struct linger ling;

#endif /* HAVE_LINGER */
    DPUSH; /* #7 */

#ifdef FNDELAY
    if (fcntl(s, F_SETFL, FNDELAY) == -1) {
	log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
    }
#else 
    if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
	log_perror("NET", "FAIL", "make_nonblocking", "fcntl");
    }
#endif /* FNDELAY */
#ifdef HAVE_LINGER
    ling.l_onoff = 0;
    ling.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER,
		   (char *) &ling, sizeof(ling)) < 0) {
	log_perror("NET", "FAIL", "linger", "setsockopt");
    }
#endif /* HAVE_LINGER */
    DPOP; /* #7 */
#endif /* not TLI */
}

  /* start_auth begins the process of identifying the user on their
     remote machine  -Thorin 2/95 */
void 
start_auth(DESC * d)
{
    char* logbuff;
    struct sockaddr_in sin;

    DPUSH; /* #8 */
    if ( d->flags & DS_API ) {
      d->flags &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    if( !mudconf.authenticate ) {
      d->flags &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    /* check the noauth_site list */
    if( site_check(d->address.sin_addr, mudstate.special_list, 1, 0, H_NOAUTH) & H_NOAUTH ) {
      STARTLOG(LOG_NET, "NET", "AUTH")
        logbuff = alloc_lbuf("start_auth.LOG.site_check");
        sprintf(logbuff,
        "[%d/%s] Site configured as NOAUTH, not checking", d->descriptor, d->longaddr);
        log_text(logbuff);
        free_lbuf(logbuff);
      ENDLOG
      d->flags &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    bzero((char *) &sin, sizeof(sin));

    if ((d->authdescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	log_perror("NET", "FAIL", "creating auth sock", "socket");
	d->flags &= ~DS_AUTH_IN_PROGRESS;
	VOIDRETURN; /* #8 */
    }

    /* now make the socket non-blocking */
    make_nonblocking(d->authdescriptor);

    sin.sin_port = htons(113);	/* auth/ident port */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = d->address.sin_addr.s_addr;

    /* 1/2/97 - Thorin - Solaris has a problem with blocking on
       connect even when the socket is non-blocking, so instead we
       will only give it a maximum of three seconds to connect by
       doing fun stuff with preserving the mush timers and all. */

    STARTLOG(LOG_NET, "NET", "AUTH")
      logbuff = alloc_lbuf("start_auth.LOG.connecting");
      sprintf(logbuff,
      "[%d/%s] Starting AUTH connection", d->descriptor, d->longaddr);
      log_text(logbuff);
      free_lbuf(logbuff);
    ENDLOG

    alarm_msec(3);

    if( connect(d->authdescriptor, (struct sockaddr *) &sin, sizeof(sin)) < 0){
        if( errno != EINPROGRESS ) {
  	  d->flags &= ~DS_AUTH_IN_PROGRESS;
          logbuff = alloc_lbuf("start_auth.LOG.timeout");
          if( errno != EINTR ) {
	    log_perror("NET", "FAIL", "connecting AUTH sock", "connect");
          } else {
            STARTLOG(LOG_NET, "NET", "AUTH")
              sprintf(logbuff,
              "[%d/%s] AUTH connect alarm timed out", d->descriptor, d->longaddr);
              log_text(logbuff);
            ENDLOG
          }
          sprintf(logbuff, "%s 255.255.255.255", inet_ntoa(d->address.sin_addr));
          cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NOAUTH | H_AUTOSITE), 0, 1, "noauth_site");
          free_lbuf(logbuff);
	  shutdown(d->authdescriptor, 2);
	  close(d->authdescriptor);
	  VOIDRETURN; /* #8 */
        }
    }

    /* recalibrate the mush timers */

    mudstate.alarm_triggered = 0;
    alarm_msec(next_timer());

    d->flags |= DS_AUTH_CONNECTING;
    DPOP; /* #8 */
}

void
check_auth_connect(DESC * d)
{
    char* logbuff;
    int sockerr = 0;
    int sockerrlen = sizeof(sockerr);

    d->flags &= ~DS_AUTH_CONNECTING;

    if( getsockopt(d->authdescriptor, SOL_SOCKET, SO_ERROR, &sockerr, (unsigned int *) &sockerrlen) ) {
        log_perror("NET", "FAIL", "checking AUTH connect", "getsockopt");
        d->flags &= ~DS_AUTH_IN_PROGRESS;
/* do we really need this next line? */
        close(d->authdescriptor);
    }
    else {
        if( sockerr ) {
            STARTLOG(LOG_NET, "NET", "AUTH")
              logbuff = alloc_lbuf("check_auth_connect.LOG.timeout");
              sprintf(logbuff,
              "[%d/%s] AUTH connect: %s", d->descriptor, d->longaddr, strerror(sockerr));
              log_text(logbuff);
              free_lbuf(logbuff);
            ENDLOG
            d->flags &= ~DS_AUTH_IN_PROGRESS;
/* do we really need this next line? */
            close(d->authdescriptor);
        }
        else {
            d->flags |= DS_NEED_AUTH_WRITE;
        }
    }
}

void 
write_auth(DESC * d)
{
    char *buff;
    char *logbuff;

    DPUSH; /* #9 */

    buff = alloc_mbuf("auth");

    sprintf(buff, "%d, %d\r\n", ntohs(d->address.sin_port), mudconf.port);

    if (write(d->authdescriptor, buff, strlen(buff)) < 0) {
	free_mbuf(buff);
	if (errno == EINTR) {

	    VOIDRETURN; /* #9 */
	}
	if (errno != EINPROGRESS) {
            STARTLOG(LOG_NET, "NET", "AUTH")
      	      logbuff = alloc_lbuf("check_auth.LOG.writeerr");
    	      sprintf(logbuff,
	              "[%d/%s] AUTH write: %s", d->descriptor, d->longaddr,
	              strerror(errno));
	      log_text(logbuff);
	      free_lbuf(logbuff);
	    ENDLOG
	    d->flags &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->authdescriptor, 2);
	    close(d->authdescriptor);
	}
    }
    else {
        free_mbuf(buff);
    }
    d->flags &= ~DS_NEED_AUTH_WRITE;
    shutdown(d->authdescriptor, 1);	/* close write */
    VOIDRETURN; /* #9 */
}

void 
check_auth(DESC * d)
{
    char *buff;
    char *buff2;
    int numread;
    char *logbuff;
    int resp1, resp2;
    char *resp3;
    char *useridptr;

    DPUSH; /* #10 */

    buff = alloc_mbuf("check_auth_rec_buffer");

    if ((numread = read(d->authdescriptor, buff, MBUF_SIZE - 1)) < 0) {
        STARTLOG(LOG_NET, "NET", "AUTH")
  	  logbuff = alloc_lbuf("check_auth.LOG.readerr");
	  sprintf(logbuff,
	          "[%d/%s] AUTH read: %s", d->descriptor, d->longaddr,
	          strerror(errno));
	  log_text(logbuff);
	  free_lbuf(logbuff);
	ENDLOG
	d->flags &= ~DS_AUTH_IN_PROGRESS;
	shutdown(d->authdescriptor, 2);
	close(d->authdescriptor);
        free_mbuf(buff);
	VOIDRETURN; /* #10 */
    }
    buff[numread] = '\0';

    useridptr = d->userid + strlen(d->userid);
    safe_mb_str(buff, d->userid, &useridptr);
    free_mbuf(buff);

    if (index(d->userid, '\r') || index(d->userid, '\n')) {
	/* we got it all */
	/* extract the user id if it's there */

        resp3 = alloc_mbuf("check_auth_resp3");
        buff2 = alloc_mbuf("check_auth_buff2");
        /* d->userid is guaranteed to be within MBUF size,
           therefore a direct sscanf of any field within will be
           <= MBUF size */
	if (sscanf(d->userid, "%d , %d : USERID : %s : %s", &resp1, &resp2, resp3, buff2) == 4) {
	    strncpy(d->userid, strip_nonprint(buff2), MBUF_SIZE - 1);
	    d->flags &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->authdescriptor, 2);
	    close(d->authdescriptor);
            if ( strcmp(buff2, d->userid) != 0 ) {
               STARTLOG(LOG_NET, "NET", "AUTH")
		  logbuff = alloc_lbuf("check_auth.LOG.got_it");
                  sprintf(logbuff, "Warning: Non-printable codes stripped in userid '%s'.", buff2);
                  log_text(logbuff);
                  free_lbuf(logbuff);
               ENDLOG
            }
	    STARTLOG(LOG_NET, "NET", "AUTH")
		logbuff = alloc_lbuf("check_auth.LOG.got_it");
	        sprintf(logbuff,
		    "[%d/%s] User identified: %s", d->descriptor, d->longaddr,
		    d->userid);
	        log_text(logbuff);
	        free_lbuf(logbuff);
	    ENDLOG
	} else {		/* protocol error */
	    d->flags &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->authdescriptor, 2);
	    close(d->authdescriptor);
	    STARTLOG(LOG_NET, "NET", "AUTH")
		logbuff = alloc_lbuf("check_auth.LOG.prot_err");
	        sprintf(logbuff,
		  "[%d/%s] Auth protocol error: %s", d->descriptor, d->longaddr,
		    d->userid);
	        log_text(logbuff);
	        free_lbuf(logbuff);
	    ENDLOG
            strcpy(d->userid,"");
	}
        free_mbuf(resp3);
        free_mbuf(buff2);
    } 
    DPOP; /* #10 */
}


DESC *
initializesock(int s, struct sockaddr_in * a, char *addr, int i_keyflag, int keyval)
{
    DESC *d, *dchk, *dchknext;
    char tchbuff[LBUF_SIZE];
    char *addroutbuf, *t_addroutbuf;
    int i_sitecnt, i_guestcnt, i_sitemax, i_retvar = -1;

    DPUSH; /* #11 */

    ndescriptors++;
    d = alloc_desc("init_sock");
    d->descriptor = s;
    d->flags = 0;
    if ( !keyval ) {
       d->flags = DS_AUTH_IN_PROGRESS | DS_NEED_AUTH_WRITE;	/* start authenticating */
    }
    d->connected_at = mudstate.now;
    d->retries_left = mudconf.retry_limit;
    d->regtries_left = mudconf.regtry_limit;
    d->command_count = 0;
    d->timeout = mudconf.idle_timeout;
    addroutbuf = (char *) addrout((*a).sin_addr, keyval);
    t_addroutbuf = alloc_lbuf("check_max_sitecons");
    strcpy(t_addroutbuf, addroutbuf);
    i_sitecnt = i_guestcnt = 0;
    if ( t_addroutbuf ) {
       DESC_SAFEITER_ALL(dchk, dchknext) {
          if ( strcmp(t_addroutbuf, dchk->addr) == 0 ) {
             if ( Good_chk(dchk->player) && Guest(dchk->player) ) {
                i_guestcnt++;
             }
             i_sitecnt++;
          }
       }
    }
    free_lbuf(t_addroutbuf);
    d->host_info = site_check((*a).sin_addr, mudstate.access_list, 1, 0, 0) |
		   site_check((*a).sin_addr, mudstate.suspect_list, 0, 0, 0);
    i_sitemax = site_check((*a).sin_addr, mudstate.access_list, 1, 1, H_FORBIDDEN);
    if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
       d->host_info &= ~H_FORBIDDEN;
    i_sitemax = site_check((*a).sin_addr, mudstate.access_list, 1, 1, H_REGISTRATION);
    if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
       d->host_info &= ~H_REGISTRATION;
    i_sitemax = site_check((*a).sin_addr, mudstate.access_list, 1, 1, H_NOGUEST);
    if ( (i_sitemax != -1) && (i_guestcnt < i_sitemax) )
       d->host_info &= ~H_NOGUEST;
    memset(tchbuff, 0, sizeof(tchbuff));
    strcpy(tchbuff, mudconf.forbid_host);
    if ((char *)mudconf.forbid_host && lookup(addr, tchbuff, i_sitecnt, &i_retvar))
       d->host_info = d->host_info | H_FORBIDDEN;
    strcpy(tchbuff, mudconf.register_host);
    if ((char *)mudconf.register_host && lookup(addr, tchbuff, i_sitecnt, &i_retvar))
       d->host_info = d->host_info | H_REGISTRATION;
    strcpy(tchbuff, mudconf.autoreg_host);
    if ((char *)mudconf.autoreg_host && lookup(addr, tchbuff, i_sitecnt, &i_retvar))
       d->host_info = d->host_info | H_NOAUTOREG;
    strcpy(tchbuff, mudconf.noguest_host);
    if ((char *)mudconf.noguest_host && lookup(addr, tchbuff, i_guestcnt, &i_retvar))
       d->host_info = d->host_info | H_NOGUEST;
    strcpy(tchbuff, mudconf.suspect_host);
    if ((char *)mudconf.suspect_host && lookup(addr, tchbuff, i_sitecnt, &i_retvar))
       d->host_info = d->host_info | H_SUSPECT;
    strcpy(tchbuff, mudconf.passproxy_host);
    if ((char *)mudconf.passproxy_host && lookup(addr, tchbuff, i_sitecnt, &i_retvar))
       d->host_info = d->host_info | H_PASSPROXY;
    strcpy(tchbuff, mudconf.hardconn_host);
    if ((char *)mudconf.hardconn_host && lookup(addr, tchbuff, i_guestcnt, &i_retvar))
       d->host_info = d->host_info | H_HARDCONN;
    d->host_info = d->host_info | i_keyflag;
    d->player = 0;		/* be sure #0 isn't wizard.  Shouldn't be. */
    d->addr[0] = '\0';
    d->longaddr[0] = '\0';
    d->doing[0] = '\0';
    make_nonblocking(s);
    d->output_prefix = NULL;
    d->output_suffix = NULL;
    d->output_size = 0;
    d->output_tot = 0;
    d->output_lost = 0;
    d->output_head = NULL;
    d->output_tail = NULL;
    d->door_output_head = NULL;
    d->door_output_tail = NULL;
    d->door_input_head = NULL;
    d->door_input_tail = NULL;
    d->door_desc = -1;
    d->door_num = 0;
    d->door_output_size = 0;
    d->door_int1 = 0;
    d->door_int2 = 0;
    d->door_int3 = 0;
    d->door_lbuf = NULL;
    d->door_mbuf = NULL;
    d->door_raw = NULL;
    d->input_head = NULL;
    d->input_tail = NULL;
    d->input_size = 0;
    d->input_tot = 0;
    d->input_lost = 0;
    d->raw_input = NULL;
    d->raw_input_at = NULL;
    d->quota = mudconf.cmd_quota_max;
    d->last_time = 0;
    d->snooplist = NULL;
    d->logged = 0;
    d->checksum[0] = '\0';
    d->ws_frame_len = 0;
    d->account_owner = NOTHING;
    memset(d->account_rawpass, '\0', sizeof(d->account_rawpass));

    *d->userid = '\0';
    memset(d->addr, 0, sizeof(d->addr)); /* Null terminate the sucker */
    strncpy(d->addr, addr, 50);
    memset(d->longaddr, '\0', sizeof(d->longaddr)); /* Null terminate this sucker too */
    strncpy(d->longaddr, addr, 255);
    d->address = *a;		/* added 5/3/90 SCG */
    if (descriptor_list)
	descriptor_list->prev = &d->next;
    d->hashnext = NULL;
    d->next = descriptor_list;
    d->prev = &descriptor_list;
    descriptor_list = d;
    if ( !keyval ) {
       welcome_user(d);
       start_auth(d);
    } else {
       d->timeout = 1;
       d->flags |= DS_API;
    }
    RETURN(d); /* #11 */
}

int 
process_output(DESC * d)
{
    TBLOCK *tb, *save;
    int cnt, retry_cnt, retry_success;
    char *cmdsave, s_savebuff[80];

    DPUSH; /* #12 */

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< process_output >";

    tb = d->output_head;
    retry_cnt = retry_success = 0;
    while (tb != NULL) {
	while (tb->hdr.nchars > 0) {

	    cnt = WRITE(d->descriptor, tb->hdr.start,
			tb->hdr.nchars);
	    if (cnt < 0) {
		mudstate.debug_cmd = cmdsave;
                if ( (errno == EWOULDBLOCK) || (errno == EAGAIN) ) {
                   while ( retry_cnt < 10 ) {
	              cnt = WRITE(d->descriptor, tb->hdr.start, tb->hdr.nchars);
                      if ( (cnt < 0) && !((errno == EWOULDBLOCK) || (errno == EAGAIN)) ) {
                         retry_cnt = 20;
                         retry_success = 1;
                      } else if ( (cnt >= 0) ) {
                         retry_cnt = 20;
                         retry_success = 1;
                      } else if ( !((errno == EWOULDBLOCK) || (errno == EAGAIN)) ) {
                         retry_cnt = 20;
                      }
                      retry_cnt++;
                   }
                }
                if ( !retry_success ) {
                   if ( errno == 11 ) {
                   /* It's a timeout, let's wait for a few milliseconds and try again */
/*                    usleep(10); */
                      nanosleep((struct timespec[]){{0, 100000000}}, NULL);
	              cnt = WRITE(d->descriptor, tb->hdr.start, tb->hdr.nchars);
                   }
                   if ( cnt < 0 ) {
                      if ( (mudconf.log_network_errors > 0) && (mudstate.last_network_owner != d->player) ) {
                         STARTLOG(LOG_ALWAYS, "WIZ", "ERR")
                            memset(s_savebuff, '\0', sizeof(s_savebuff));
                            log_text((char *) "WARNING: Failed socket write to descriptor past 10 times ");
                            sprintf(s_savebuff, "[port %d/player #%d/error %d] ", d->descriptor, d->player,
                                    errno);
                            log_text(s_savebuff);
                            log_text((char *)strerror(errno));
                         ENDLOG
                         mudstate.last_network_owner = d->player;
                      }
                      RETURN(1);
                   }
                }
#if 0 /* original code here */
  		if (errno == EWOULDBLOCK) {
  		    RETURN(1); /* #12 */
                }
#endif
                if ( cnt < 0 )
		   RETURN(0); /* #12 */
	    }
	    d->output_size -= cnt;
	    tb->hdr.nchars -= cnt;
	    tb->hdr.start += cnt;
	}
	save = tb;
	tb = tb->hdr.nxt;
	free_lbuf(save);
	d->output_head = tb;
	if (tb == NULL)
	    d->output_tail = NULL;
    }
    mudstate.debug_cmd = cmdsave;
    RETURN(1); /* #12 */
}


/* The monster that's telnet negotiation -- for now we just eat it */
int
snarfle_the_garthok(char *input, char *output) 
{
   int i_count;
   char *s_in, *s_out;

   i_count = 0;
   s_in = input;
   s_out = output;
   if ( s_out )
      ; /* Temporary initialize for warnings */

   switch ((unsigned char)*s_in) {
      /* IAC/255 is already handled */
      case 254: /* DONT */
         if ( *(s_in+1) ) {
            i_count+=2;
         } else {
            i_count++;
         }
         break;
      case 253: /* DO */
         if ( *(s_in+1) ) {
            i_count+=2;
         } else {
            i_count++;
         }
         break;
      case 252: /* WONT */
         if ( *(s_in+1) ) {
            i_count+=2;
         } else {
            i_count++;
         }
         break;
      case 251: /* WILL */
         if ( *(s_in+1) ) {
            i_count+=2;
         } else {
            i_count++;
         }
         break;
      case 250: /* SB -- NEED TO LOOK FOR SE/240 or corrupt packet stream */
         if ( strchr(s_in, (unsigned char)240) == NULL ) {
            /* No SE found -- eat what we can and ignore it */
            i_count+=2;
         } else {
            i_count=(strchr(s_in, (unsigned char)240) - s_in);
         }
      case 241: /* NOP */
         i_count++;
         break;
      default: /* Any other char */
         i_count++;
         break;
   }
   return(i_count);
}

int 
process_input(DESC * d)
{
    static char buf[LBUF_SIZE];
    int got, in, lost, in_get;
    char *p, *pend, *q, *qend, qfind[SBUF_SIZE], *qf, *tmpptr = NULL, tmpbuf[SBUF_SIZE];
    char *cmdsave;
#ifdef ENABLE_WEBSOCKETS
///// NEW WEBSOCK #ifdef ENABLE_WEBSOCKETS
    int got2;
///// END NEW WEBSOCK #endif
#endif

    DPUSH; /* #16 */
    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< process_input >";

    memset(qfind, '\0', sizeof(qfind));
    memset(tmpbuf, '\0', sizeof(tmpbuf));
    got = in = READ(d->descriptor, buf, sizeof buf);
    if (got <= 0) {
	mudstate.debug_cmd = cmdsave;
	RETURN(0); /* #16 */
    }
#ifdef ENABLE_WEBSOCKETS
///// NEW WEBSOCK #ifdef ENABLE_WEBSOCKETS
    if (d->flags & DS_WEBSOCKETS) {
        /* Process using WebSockets framing. */
        got2 = got;
        got = in = process_websocket_frame(d, buf, got2);
    }
///// END NEW WEBSOCK #endif
#endif
    if (!d->raw_input) {
	d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
	d->raw_input_at = d->raw_input->cmd;
    }
    p = d->raw_input_at;
    pend = d->raw_input->cmd + LBUF_SIZE - sizeof(CBLKHDR) - 1;
    lost = 0;
    in_get = 1;
    if ( d->flags & DS_API ) {
       in_get = 0;
    }
//fprintf(stderr, "Test: %s\nVal: %d", buf, in_get);
    for (q = buf, qend = buf + got; q < qend; q++) {
	if ( (*q == '\n') && (!(d->flags & DS_API) || (!in_get && ((q+10) > qend) && (d->flags & DS_API))) ) {
  	      *p = '\0';
		if (p > d->raw_input->cmd) {
			save_command(d, d->raw_input);
			d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
			p = d->raw_input_at = d->raw_input->cmd;
			pend = d->raw_input->cmd + LBUF_SIZE -
			sizeof(CBLKHDR) - 1;
		} else {
			in -= 1;	/* for newline */
		}
	} else if ((*q == '\b') || (*q == 127)) {
	    if (*q == 127)
		queue_string(d, "\b \b");
	    else
		queue_string(d, " \b");
	    in -= 2;
	    if (p > d->raw_input->cmd)
		p--;
	    if (p < d->raw_input_at)
		(d->raw_input_at)--;
        /* Display char 255  -- no need for accent_extend as it's handled in eval.c */
        } else if ( (((int)(unsigned char)*q) == 255) && *(q+1) && (((int)(unsigned char)*(q+1)) == 255) ) {
            sprintf(qfind, "%c<%3d>", '%', (int)(unsigned char)*q);
            in+=5;
            got+=5;
            qf = qfind;
            while ( *qf ) {
               *p++ = *qf++;
            }
            q++;
        /* This is telnet negotiation -- we eat telnet negotiation */
        } else if ( (((int)(unsigned char)*q) == 255) && *(q+1) && (((int)(unsigned char)*(q+1)) != 255) ) {
           q++;
        /* Else let's print printables -- This is ASCII-7 [0-128] */
  	} else if (p < pend && ((*q == '\n') || (isascii((int)*q) && isprint((int)*q))) ) {
	    *p++ = *q;
        } else if ( ((p+13) < pend) && *q && *(q+1) && *(q+2) && *(q+3) && IS_4BYTE((int)(unsigned char)*q) && IS_CBYTE(*(q+1)) && IS_CBYTE(*(q+2)) && IS_CBYTE(*(q+3))) {
            sprintf(tmpbuf, "%02x%02x%02x%02x", (int)(unsigned char)*q, (int)(unsigned char)*(q+1), (int)(unsigned char)*(q+2), (int)(unsigned char)*(q+3));
            tmpptr = encode_utf8(tmpbuf);
            sprintf(qfind, "%s", tmpptr);
            free_sbuf(tmpptr);
            
            q+=3;
            in+=12;
            got+=12;
               qf = qfind;
               while ( *qf ) {
                  *p++ = *qf++;
               }
        } else if ( ((p+13) < pend) && *q && *(q+1) && *(q+2) && IS_3BYTE((int)(unsigned char)*q) && IS_CBYTE(*(q+1)) && IS_CBYTE(*(q+2))) {
            sprintf(tmpbuf, "%02x%02x%02x", (int)(unsigned char)*q, (int)(unsigned char)*(q+1), (int)(unsigned char)*(q+2));
            tmpptr = encode_utf8(tmpbuf);
            sprintf(qfind, "%s", tmpptr);
            free_sbuf(tmpptr);
            
            q+=2;
            in+=10;
            got+=10;
            qf = qfind;
            while (*qf) {
                *p++ = *qf++;
            }   
        } else if ( ((p+13) < pend) && *q && *(q+1) && IS_2BYTE((int)(unsigned char)*q) && IS_CBYTE(*(q+1))) {
            sprintf(tmpbuf, "%02x%02x", (int)(unsigned char)*q, (int)(unsigned char)*(q+1));
            tmpptr = encode_utf8(tmpbuf);
            sprintf(qfind, "%s", tmpptr);
            free_sbuf(tmpptr);
            
            q+=1;
            in+=8;
            got+=8;
            qf = qfind;
            while (*qf) {
                *p++ = *qf++;
            }
        /* Let's handle accents [129-255] -- no accent_extend here as it's handled in eval.c in parse_ansi */
        } else if ( (((int)(unsigned char)*q) > 160) && (((int)(unsigned char)*q) < 256) && ((p+10) < pend) ) {
            sprintf(qfind, "%c<%3d>", '%', (int)(unsigned char)*q);
            in+=5;
            got+=5;
            qf = qfind;
            while ( *qf ) {
               *p++ = *qf++;
            }
	} else {
	    in--;
	    if (p >= pend)
		lost++;
	}
    }
    if (p > d->raw_input->cmd) {
	d->raw_input_at = p;
    } else {
	free_lbuf(d->raw_input);
	d->raw_input = NULL;
	d->raw_input_at = NULL;
    }
    if ( got > 0 ) {
       d->input_tot += got;
       mudstate.total_bytesin += got;
       if ( (mudstate.reset_daily_bytes + 86400) < mudstate.now ) {
          if ( mudstate.avg_bytesin == 0 ) {
             mudstate.avg_bytesin = mudstate.daily_bytesin;
          } else {
             mudstate.avg_bytesin = (mudstate.avg_bytesin + mudstate.daily_bytesin) / 2;
          }
          if ( mudstate.avg_bytesout == 0 ) {
             mudstate.avg_bytesout = mudstate.daily_bytesout;
          } else {
             mudstate.avg_bytesout = (mudstate.avg_bytesout + mudstate.daily_bytesout) / 2;
          }
          mudstate.daily_bytesout = 0;
          mudstate.daily_bytesin = got;
          mudstate.reset_daily_bytes = time(NULL);
       } else {
          mudstate.daily_bytesin += got;
       }
    }
    if ( in > 0 )
       d->input_size += in;
    if ( lost > 0 )
       d->input_lost += lost;
    
    mudstate.debug_cmd = cmdsave;
    RETURN(1); /* #16 */
}

void 
close_sockets(int emergency, char *message)
{
    DESC *d, *dnext;

    DPUSH; /* #17 */
    do_rwho(NOTHING, NOTHING, RWHO_STOP);
    DESC_SAFEITER_ALL(d, dnext) {
	if (emergency) {

	    WRITE(d->descriptor, message, strlen(message));
	    if (shutdown(d->descriptor, 2) < 0)
		log_perror("NET", "FAIL", NULL, "shutdown");
	    close(d->descriptor);
	    if (d->flags & DS_HAS_DOOR)
	      close(d->door_desc);
	} else {
	    queue_string(d, message);
	    queue_write(d, "\r\n", 2);
	    if (!(mudstate.reboot_flag))
	      shutdownsock(d, R_GOING_DOWN);
	    else
	      shutdownsock(d, R_REBOOT);
	}
    }
    close(sock);
    if ( mudconf.api_port != -1 ) {
       close(sock2);
    }
    DPOP; /* #17 */
}
  
void close_main_socket( void )
{
    close(sock);
    if ( mudconf.api_port != -1 ) {
       close(sock2);
    }
}

void 
NDECL(emergency_shutdown)
{
    DPUSH; /* #18 */
    close_sockets(1, (char *) "Going down - Bye");
    DPOP; /* #18 */
}


/* ---------------------------------------------------------------------------
 * Signal handling routines.
 */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

static RETSIGTYPE sighandler();

NAMETAB sigactions_nametab[] =
{
    {(char *) "resignal", 3, 0, 0, SA_RESIG},
    {(char *) "retry", 3, 0, 0, SA_RETRY},
    {(char *) "default", 1, 0, 0, SA_DFLT},
    {NULL, 0, 0, 0, 0}};

void 
NDECL(set_signals)
{
    sigset_t sigs;

    DPUSH; /* #19 */
    sigfillset(&sigs);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);

    signal(SIGUSR1, sighandler);
    signal(SIGUSR2, sighandler);
    signal(SIGALRM, sighandler);
    signal(SIGCHLD, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGPROF, SIG_IGN);
#ifdef SIGXCPU
    signal(SIGXCPU, sighandler);
#endif

    if (mudconf.sig_action != SA_DFLT) {
	signal(SIGILL, sighandler);
	signal(SIGTRAP, sighandler);
  	signal(SIGFPE, sighandler);
  	signal(SIGSEGV, sighandler);
	signal(SIGABRT, sighandler);
#ifdef SIGFSZ
	signal(SIGXFSZ, sighandler);
#endif
#ifdef SIGEMT
	signal(SIGEMT, sighandler);
#endif
#ifdef SIGBUS
	signal(SIGBUS, sighandler);
	signal(SIGFPE, SIG_IGN);
#endif
#ifdef SIGSYS
	signal(SIGSYS, sighandler);
#endif
    }
    DPOP; /* #19 */
}

static void 
unset_signals()
{
    int i;
    DPUSH; /* #20 */

    for (i = 0; i < NSIG; i++)
	    signal(i, SIG_DFL);
    abort();
    DPOP; /* #20 */
}

/* This turns off signals during db operations. */

void ignore_signals(){
  int i;

  if ( signal_depth != 0 ) 
     return;

  signal_depth = signal_depth + 1; /* Increase our 'signal unset level' */
  for (i = 0; i < NSIG; i++) {
      if ( i != SIGUSR2 )
         signal(i, SIG_IGN);
  }
}

/* Called to reset signal handling after db operations. */

void reset_signals(){
        if ( mudstate.reboot_flag || (signal_depth < 1) )
           return;

	signal_depth = signal_depth - 1; /* Reduce 'signal unset' depth. We only want
									 									* to call set_signals() if it is 0.
									 									*/
	if(signal_depth == 0) 
		set_signals();
        /* If signals were ignored, the alarm_msec() will have been as well */
        mudstate.alarm_triggered = 0;
        alarm_msec (next_timer());
}

static void 
check_panicking(int sig)
{
    int i;

    DPUSH; /* #21 */

    /* If we are panicking, turn off signal catching and resignal */

    if (mudstate.panicking) {
	for (i = 0; i < NSIG; i++)
	    signal(i, SIG_DFL);
	kill(getpid(), sig);
    }
    mudstate.panicking = 1;
    DPOP; /* #21 */
}

void 
log_signal(const char *signame, int sig)
{
    static char s_to_i[30];
   
    DPUSH; /* #22 */
    memset(s_to_i, '\0', 30);
    STARTLOG(LOG_PROBLEMS, "SIG", "CATCH")
	log_text((char *) "Caught signal ");
    log_text((char *) signame);
    sprintf(s_to_i, " [%d]", sig);
    log_text((char *) s_to_i);
    ENDLOG
    DPOP; /* #22 */
}

#ifdef HAVE_STRUCT_SIGCONTEXT
static RETSIGTYPE 
sighandler(int sig, int code, struct sigcontext *scp)
#else
static RETSIGTYPE 
sighandler(int sig)
/* sighandler(int sig, int code) */
#endif
{
#ifdef HAVE_SYS_SIGLIST
#ifdef NEED_SYS_SIGLIST_DCL
    extern char *sys_siglist[];

#endif
#define signames sys_siglist
#else /* HAVE_SYS_SIGLIST */
#ifdef HAVE__SYS_SIGLIST
#ifdef NEED__SYS_SIGLIST_DCL
    extern char *_sys_siglist[];

#endif
#define signames _sys_siglist
#else /* HAVE__SYS_SIGLIST */
    static const char *signames[] =
    {
	"SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT",
	"SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT",
	"SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV",
	"SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM",
	"SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT",
	"SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO",
	"SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF",
	"SIGWINCH", "SIGLOST", "SIGBUS", "SIGSYS"};

#endif /* HAVE__SYS_SIGLIST */
#endif /* HAVE_SYS_SIGLIST */

    char buff[32], *s_crontmp, *s_crontmpwlk;
    char *flatfilename;
    FILE *f, *f_crontmp;
    char *ptr2;
    int aflags, i_cronchk, i_croncnt;
    ATTR *ptr;
    dbref player, aowner, i_crontmp;
    sigset_t sigs;

#ifdef HAVE_UNION_WAIT
    union wait stat;

#else
    int stat;

#endif
    DPUSH; /* #23 */
#ifdef IGNORE_SIGNALS /* Do nothing. */
#ifdef VMS
    if ( sig != SIGALRM )
       RETURN(1); /* #23 */
#endif
    if ( sig != SIGALRM )
       VOIDRETURN; /* #23 */
#endif

    i_cronchk = i_croncnt = 0;
    i_crontmp = NOTHING;

    switch (sig) {
    case SIGALRM:		/* Timer */
	mudstate.alarm_triggered = 1;
	break;
    case SIGCHLD:		/* Change in child status */
#ifndef SIGNAL_SIGCHLD_BRAINDAMAGE
	signal(SIGCHLD, sighandler);
#endif
#ifdef HAVE_WAIT3
	while (wait3(&stat, WNOHANG, NULL) > 0);
#else
	wait(&stat);
#endif
	break;
    case SIGUSR1:   /* Perform a @reboot. */
          log_signal(signames[sig], sig);
          /* User configured signal handling might happen. */
          if ( mudconf.signal_crontab ) {
             if ( (f_crontmp = fopen("cron/rhost.cron", "r")) != NULL ) {
                i_cronchk = 1;
                if ( feof(f_crontmp) ) {
                   i_cronchk = 0;
                } else {
                   s_crontmp = alloc_lbuf("do_sigusr1_cron");
                   fgets(s_crontmp, LBUF_SIZE - 1, f_crontmp);
                   if ( *s_crontmp ) {
                      if ( (*s_crontmp == '#') && isdigit(*(s_crontmp+1))) {
                         i_crontmp = atoi(s_crontmp+1);
                      } 
                      if ( i_crontmp == NOTHING ) {
                         s_crontmpwlk = s_crontmp;
                         while ( *s_crontmpwlk ) {
                            if ( (*s_crontmpwlk == '\r') || (*s_crontmpwlk == '\n') ) {
                               *s_crontmpwlk = '\0';
                               break;
                            }
                            s_crontmpwlk++;
                         }
                         i_crontmp = lookup_player(NOTHING, s_crontmp, 0);
                      }
                      if ( !Good_chk(i_crontmp) ) {
                         i_cronchk = 0;
                      } else {
                         if ( feof(f_crontmp) ) {
                            i_cronchk = 0;
                         } else {
                            fgets(s_crontmp, LBUF_SIZE - 1, f_crontmp);
                            while ( !feof(f_crontmp) ) {
                               s_crontmpwlk = s_crontmp;
                               while ( *s_crontmpwlk ) {
                                  if ( !isprint(*s_crontmpwlk) && !(*s_crontmpwlk == '\n') &&
                                       !(*s_crontmpwlk == '\r') ) {
                                     i_cronchk = 0;
                                     break;
                                  }
                                  s_crontmpwlk++;
                               }
                               if ( i_cronchk == 0 )
                                  break;
                               if ( i_croncnt < 20 ) {
                                  wait_que(i_crontmp, i_crontmp, 0, NOTHING, s_crontmp, (char **)NULL, 0,
                                         mudstate.global_regs, mudstate.global_regsname);
                               } else {
                                  STARTLOG(LOG_ALWAYS, "WIZ", "CRON")
                                     log_text((char *) "Signal USR1 received with signal_crontab: Entries after the 20th command line ignored.");
                                  ENDLOG
                                  break;
                               }
                               fgets(s_crontmp, LBUF_SIZE -1, f_crontmp);
                               i_croncnt++;
                            }
                         }
                      }
                   } else {
                      i_cronchk = 0;
                   }
                   if ( i_cronchk == 0 ) {
                      STARTLOG(LOG_ALWAYS, "WIZ", "CRON")
                         log_text((char *) "Signal USR1 received with signal_crontab yet crontab file was corrupt.");
                      ENDLOG
                   } else {
                      STARTLOG(LOG_ALWAYS, "WIZ", "CRON")
                         sprintf(s_crontmp, "Signal USR1 crontab entry executed.  %d total lines processed.", i_croncnt);
                         log_text(s_crontmp);
                      ENDLOG
                   }
                   free_lbuf(s_crontmp);
                }
                fclose(f_crontmp);
             } else {
                STARTLOG(LOG_ALWAYS, "WIZ", "CRON")
                   log_text((char *) "Signal USR1 received with signal_crontab enabled but no crontab file found.");
                ENDLOG
             }
             sigfillset(&sigs);
             sigprocmask(SIG_UNBLOCK, &sigs, NULL);
             break;
          } 
          if ( Good_chk(mudconf.signal_object) ) {
             ptr = atr_str("SIG_USR1");
             if ( ptr ) {
                ptr2 = atr_get(mudconf.signal_object, ptr->number, &aowner, &aflags); 
                if ( *ptr2 ) {
                   if ( Good_chk(Owner(mudconf.signal_object)) ) 
                      player = Owner(mudconf.signal_object);
                   else
                      player = 1; /* It's so easy being God. */
                   /* Scream and run away, #1/player is not a valid object! */
                   if ( !Good_obj(player) ) {
                      STARTLOG(LOG_ALWAYS, "WIZ", "ERR")
                         log_text((char *) "User defined signal handling aborted. 'player' is not valid!");
                      ENDLOG
                      free_lbuf(ptr2);
                      break;
                   }
                   /* Attribute gotten and has text */
                   if ( mudconf.signal_object_type ) {
                      did_it(Owner(mudconf.signal_object), mudconf.signal_object, 0, NULL, 0, NULL,
                             ptr->number, (char **) NULL, 0);
                      free_lbuf(ptr2);
                   } else {
                      did_it(Owner(mudconf.signal_object), mudconf.signal_object, ptr->number, 
                             NULL, 0, NULL, 0, (char **) NULL, 0);
                      free_lbuf(ptr2);
                   }
                   sigfillset(&sigs);
                   sigprocmask(SIG_UNBLOCK, &sigs, NULL);
                   break;
                } else {
                   free_lbuf(ptr2); /* buffer is _always_ allocated from atr_get */
                   STARTLOG(LOG_ALWAYS, "WIZ", "ERR")
                      log_text((char *) "Caught signal SIGUSR1, but no SIG_USR1 attribute to execute. Ignoring.");
                   ENDLOG
                   sigfillset(&sigs);
                   sigprocmask(SIG_UNBLOCK, &sigs, NULL);
                   break;
                }
             }
          }
          alarm_msec(0); 
          alarm_stop();
          ignore_signals();
          raw_broadcast(0, 0, "Game: Restarting due to signal SIGUSR1.");
          raw_broadcast(0, 0, "Game: Your connection will pause, but will remain connected.");
          STARTLOG(LOG_ALWAYS, "WIZ", "RBT")
            log_text((char *) "Reboot due to signal SIGUSR1.");
          ENDLOG
          mudstate.reboot_flag = 1;
          break;			
    case SIGHUP:		/* Perform a database dump */
        log_signal(signames[sig], sig);
        STARTLOG(LOG_DBSAVES, "DMP", "FLAT")
           log_text((char *)"Queueing a flatfile dump of the database.");
        ENDLOG
        s_crontmp = alloc_lbuf("do_sighup_cron");
        sprintf(s_crontmp, "%s", "@dump/flat netrhost.SIGHUP");
        wait_que(GOD, GOD, 0, NOTHING, s_crontmp , (char **)NULL, 0, (char **)NULL, (char **)NULL);
        free_lbuf(s_crontmp);
        sigfillset(&sigs);
        sigprocmask(SIG_UNBLOCK, &sigs, NULL);
	break;
    case SIGINT:		/* Log + ignore */
#ifdef SIGSYS
    case SIGSYS:
#endif
	log_signal(signames[sig], sig);
	break;
    case SIGUSR2:		/* Perform clean shutdown. */
        mudstate.forceusr2 = 1;
        log_signal(signames[sig], sig);
        raw_broadcast(0, 0, "Game: Immediately shutting down due to signal SIGUSR2!");
        sprintf(buff, "Caught signal %s", signames[sig]);
        STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
           log_text((char*) "Shutting down due to signal SIGUSR2.");
           if ( mudstate.dumpstatechk ) {
              log_text((char *)"  Waiting for dump to finish.  Passively triggering shutdown.");
              mudstate.shutdown_flag = 1;
           }
        ENDLOG
        if ( !mudstate.shutdown_flag ) {
           mudstate.forceusr2 = 0;
           do_shutdown(NOTHING, NOTHING, 0, buff);
           mudstate.shutdown_flag = 1;
        }
        break;
    case SIGTERM:		/* Attempt flatfile dump before shutdown. */
        log_signal(signames[sig], sig);
        ignore_signals();
        raw_broadcast(0, 0, "Game: Immediately shutting down due to signal SIGTERM!");
        raw_broadcast(0, 0, "Game: This might be caused by the hosting server going down.");
        STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
           log_text((char*) "Attempting flatfile dump due to signal SIGTERM.");
        ENDLOG
        /* We don't actually want to call fork_and_dump at this point, since
         * fork()ing new processes on a system that might be shutting down
         * just isn't nice. Not that this is particularly nice either. 
         */
        flatfilename = alloc_mbuf("dump/flat");
        /* mbufs are 200 bytes, buffer needs to be under that always */
        sprintf(flatfilename, "%.90s/%.90s.termflat", mudconf.data_dir, mudconf.indb);
        f = fopen(flatfilename, "w");
        free_mbuf(flatfilename);
        if (f) {
           STARTLOG(LOG_DBSAVES, "DMP", "FLAT")
              log_text((char*) "Dumping db to flatfile...");
           ENDLOG
           db_write(f, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS);
           STARTLOG(LOG_DBSAVES, "DMP", "FLAT")
              log_text((char*) "Dump complete.");
           ENDLOG
           fclose(f);
           time(&mudstate.mushflat_time);
        } else {
           STARTLOG(LOG_PROBLEMS, "DMP", "FLAT")
              log_text((char*) "Unable to open flatfile.");
           ENDLOG
        }
        STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
           log_text((char*) "SIGTERM Flatfile dump complete, exiting...");
        ENDLOG
        raw_broadcast(0, 0, "Game: Emergency dump complete, exiting...");
        DPOP;
        /* QDBM was giving some weird-ass corruption, this should hopefully fix it */
        pcache_sync();
        SYNC;
        CLOSE;
        exit(1); /* Brutal. But daddy said I had to go to bed now. */
        break; 
    case SIGQUIT:		/* Normal shutdown */
    case SIGSEGV:               /* SEGV/BUS, we have no idea on the state engine - just drop hard */
    case SIGILL:		/* Panic save + coredump */
    case SIGFPE:
#ifdef SIGXCPU
    case SIGXCPU:
#endif
	check_panicking(sig);
	log_signal(signames[sig], sig);
	sprintf(buff, "Caught signal %s - shutting down.", signames[sig]);
	do_shutdown(NOTHING, NOTHING, 0, buff);
	break;
    case SIGTRAP:
#ifdef SIGXFSZ
    case SIGXFSZ:
#endif
#ifdef SIGEMT
    case SIGEMT:
#endif
#ifdef SIGBUS
    case SIGBUS:
#endif
        /* Hopefully this saves corruption on SIGSEGV's and butt-hurt crashes */
        pcache_sync();
        SYNC;
	check_panicking(sig);
	log_signal(signames[sig], sig);
	report();
	do_shutdown(NOTHING, NOTHING, SHUTDN_PANIC, buff);

	/* Either resignal, or clear signal handling and retry the
	 * failed operation.  It should still fail, we haven't done
	 * anything to fix it, but it just _might_ succeed.  If
	 * that happens things will go downhill rapidly because we
	 * have closed all our files and cleaned up.  On the other
	 * hand, retrying the failing operation usually gets a 
	 * usable coredump, whereas aborting from within the
	 * signal handler often doesn't.
	 */

	unset_signals();
	if (mudconf.sig_action == SA_RESIG)
	    signal(sig, SIG_DFL);
	else {
	    VOIDRETURN; /* #23 */
        }
        DPOP; /* #23 */
	exit(1);

    case SIGABRT:		/* Coredump. */
	check_panicking(sig);
	log_signal(signames[sig], sig);
	report();

	unset_signals();
	if (mudconf.sig_action == SA_RESIG)
	    signal(sig, SIG_DFL);
	else {
	    VOIDRETURN; /* #23 */
        }
	DPOP; /* #23 */
	exit(1);

    }
    signal(sig, sighandler);
    mudstate.panicking = 0;
#ifdef VMS
    RETURN(1); /* #23 */
#else
    VOIDRETURN; /* #23 */
#endif
}

#ifdef LOCAL_RWHO_SERVER
void 
dump_rusers(DESC * call_by)
{
    struct sockaddr_in addr;
    struct hostent *hp;
    char *rbuf, *p, *srv;
    int fd, red;

    DPUSH; /* #24 */

    if (!(mudconf.control_flags & CF_ALLOW_RWHO)) {
	queue_string(call_by, "RWHO is not available now.\r\n");
	VOIDRETURN; /* #24 */
    }
    p = srv = mudconf.rwho_host;
    while ((*p != '\0') && ((*p == '.') || isdigit((int)*p)))
	p++;

    if (*p != '\0') {
	if ((hp = gethostbyname(srv)) == (struct hostent *) NULL) {
	    queue_string(call_by, "Error connecting to rwho.\r\n");
	    VOIDRETURN; /* #24 */
	}
	(void) bcopy(hp->h_addr, (char *) &addr.sin_addr, hp->h_length);
    } else {
	unsigned long f;

	if ((f = inet_addr(srv)) == -1L) {
	    queue_string(call_by, "Error connecting to rwho.\r\n");
	    VOIDRETURN; /* #24 */
	}
	(void) bcopy((char *) &f, (char *) &addr.sin_addr, sizeof(f));
    }
    addr.sin_port = htons(mudconf.rwho_data_port);
    addr.sin_family = AF_INET;

#ifdef TLI
    fd = tf_topen(TLI_TCP, O_RDWR);
    if (fd = <0) {
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
	VOIDRETURN; /* #24 */
    }
    if (t_bind(fd, NULL, NULL) < 0) {
	tf_close(fd);
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
	VOIDRETURN; /* #24 */
    }
    call = (struct t_call *) t_alloc(fd, T_CALL, T_ALL);
    if (call == NULL) {
	tf_close(fd);
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
	VOIDRETURN; /* #24 */
    }
    bcopy(addr, pbuf->addr.buf, sizeof(struct sockaddr_in));
    call->addr.len = sizeof(struct sockaddr_in);

#else
    fd = tf_socket(AF_INET, SOCK_STREAM);
    if (fd < 0) {
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
	VOIDRETURN; /* #24 */
    }
#endif

#ifdef TLI
    if (t_connect(fd, call, NULL) < 0) {
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
        tf_close(fd);
        VOIDRETURN; /* #24 */
    }

    if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
        queue_string(call_by, "Error in connecting to rwhod.\r\n");
        tf_close(fd);
        VOIDRETURN; /* #24 */
    }
#else
    if (connect(fd, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
	queue_string(call_by, "Error in connecting to rwhod.\r\n");
	tf_close(fd);
	VOIDRETURN; /* #24 */
    }
#endif
    rbuf = alloc_lbuf("dump_rusers");
    red = 0;
    while ((red = READ(fd, rbuf, LBUF_SIZE)) > 0) {
	rbuf[red] = '\0';
	queue_string(call_by, rbuf);
    }

    free_lbuf(rbuf);
    tf_close(fd);
    VOIDRETURN; /* #24 */
}
#endif /* LOCAL_RWHO_SERVER */
