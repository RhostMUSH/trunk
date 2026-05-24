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
#include <arpa/inet.h>


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


#include "telnet_io.h"
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

static int sock, sock2, sock6, sock6_api;
int ndescriptors = 0;
int ndesc_slots = 0;
DESC desc_slots[MAX_DESCRIPTORS];
DESC_COLD desc_cold[MAX_DESCRIPTORS];
struct desc_hot_arrays desc_hot;
DESC *desc_in_use = NULL;

DESC *_alloc_desc(const char *tag) {
    for (int i = 0; i < MAX_DESCRIPTORS; i++) {
        if (desc_hot.descriptor[i] < 0) {
            DESC *d = &desc_slots[i];
            d->slot_index = i;
            d->cold = &desc_cold[i];
            memset(d->cold, 0, sizeof(DESC_COLD));
            return d;
        }
    }
    return NULL;
}

void _free_desc(DESC *d) {
    if (d) {
        D_DESCRIPTOR(d) = -1;
        d->cold = NULL;
    }
}

DESC *FDECL(initializesock, (int, const char *, int, unsigned short, char *, int, int));
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
   struct sockaddr_in *p_sock4;
   struct sockaddr_in6 *p_sock6;
   void *v_sock;
   struct addrinfo hints, *res, *p_res;
   char ipstr[INET6_ADDRSTRLEN + 1], *tbuff, *tbuff2, *tpr2, *strtok, *mystrtok_r;
   char *tok1, *tok2, *tok3, *tok4, *tokptr;
   int i_validip;

   if ( !*(mudconf.tor_localhost) ) {
      strcpy(mudstate.tor_localcache, (char *)"1.0.0.127");
      return;
   }

   memset(&hints, 0, sizeof(hints));
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_family = AF_UNSPEC;

   tbuff = alloc_lbuf("populate_tor_seed");
   tpr2 = tbuff2 = alloc_lbuf("populate_tor_seed2");
   memset(tbuff2, '\0', LBUF_SIZE);
   strcpy(tbuff, mudconf.tor_localhost);

   strtok = strtok_r(tbuff, " \t", &mystrtok_r);

   i_validip = 0;
   while ( strtok ) {
      if ((getaddrinfo(strtok, NULL, &hints, &res)) == 0) {
         for( p_res = res; p_res != NULL; p_res = p_res->ai_next) {
            if (p_res->ai_family == AF_INET) {
               p_sock4 = (struct sockaddr_in *)p_res->ai_addr;
               v_sock = &(p_sock4->sin_addr);
               inet_ntop(AF_INET, v_sock, ipstr, sizeof(ipstr)-1);
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
            } else if (p_res->ai_family == AF_INET6) {
               struct in6_addr addr6;
               p_sock6 = (struct sockaddr_in6 *)p_res->ai_addr;
               addr6 = p_sock6->sin6_addr;
               if ( i_validip )
                  safe_chr(' ', tbuff2, &tpr2);
               int i;
               for (i = 15; i >= 0; i--) {
                  safe_str(unsafe_tprintf("%x.%x.", (addr6.s6_addr[i] >> 4) & 0xF, addr6.s6_addr[i] & 0xF), tbuff2, &tpr2);
               }
               *(tpr2 - 1) = '\0';
               tpr2--;
               i_validip = 1;
            }
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
 * check_tor_str: IPv6-aware Tor exitlist DNS checking.
 * Generates reverse nibble PTR for IPv6, standard reverse for IPv4.
 * Checks for `127.0.0.2` (IPv4) or `::2` (IPv6) response.
 */
int check_tor_str(const char *addr_str, int addr_family, int i_port) {
   char *s_reverselocal, *s_reverseremote, *s_tordns, *tortok, *tortokptr,
        *s_tmp, *s_tok1, *s_tok2, *s_tok3, *s_tok4, *s_tokptr, *s_tmp2,
        ipstr[INET6_ADDRSTRLEN + 1];
   int i_found;
   struct addrinfo hints, *res, *p_res;
   struct sockaddr_in *p_sock2;
   void *v_sock;

   if ( !*(mudstate.tor_localcache) )
      populate_tor_seed();

   s_tmp           = alloc_lbuf("check_tor_local");
   s_reverselocal  = alloc_sbuf("check_tor_1");
   s_reverseremote = alloc_mbuf("check_tor_2");
   s_tordns        = alloc_lbuf("check_tor_3");

   memset(s_tmp, '\0', LBUF_SIZE);
   strcpy(s_tmp, mudstate.tor_localcache);
   tortok = strtok_r(s_tmp, " \t", &tortokptr);
   i_found = 0;
   while (tortok) {
      sprintf(s_reverselocal, "%s", tortok);
      if (addr_family == AF_INET6) {
         struct in6_addr addr6;
         if (inet_pton(AF_INET6, addr_str, &addr6) != 1) {
            tortok = strtok_r(NULL, " \t", &tortokptr);
            continue;
         }
         char revbuf[INET6_ADDRSTRLEN * 4 + 64];
         int pos = 0;
         int i;
         for (i = 15; i >= 0; i--) {
            pos += sprintf(revbuf + pos, "%x.%x.", (addr6.s6_addr[i] >> 4) & 0xF, addr6.s6_addr[i] & 0xF);
         }
         sprintf(s_reverseremote, "%s", revbuf);
         sprintf(s_tordns, "%s%d.%s.ip6-port.exitlist.torproject.org", s_reverseremote, i_port, s_reverselocal);
      } else {
         s_tmp2 = (char *)addr_str;
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
         if ( !s_tok4 ) {
            tortok = strtok_r(NULL, " \t", &tortokptr);
            continue;
         }
         sprintf(s_reverseremote, "%s.%s.%s.%s", s_tok4, s_tok3, s_tok2, s_tok1);
         sprintf(s_tordns, "%s.%d.%s.ip-port.exitlist.torproject.org", s_reverseremote, i_port, s_reverselocal);
      }
      memset(&hints, 0, sizeof(hints));
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_family = (addr_family == AF_INET6) ? AF_INET6 : AF_INET;
      if ((getaddrinfo(s_tordns, NULL, &hints, &res)) == 0) {
         memset(ipstr, '\0', sizeof(ipstr));
         for( p_res = res; p_res != NULL; p_res = p_res->ai_next) {
            if (addr_family == AF_INET6) {
               struct sockaddr_in6 *p_sock6 = (struct sockaddr_in6 *)p_res->ai_addr;
               v_sock = &(p_sock6->sin6_addr);
               inet_ntop(p_res->ai_family, v_sock, ipstr, sizeof(ipstr)-1);
               if ( strcmp(ipstr, "::2") == 0 || strcmp(ipstr, "127.0.0.2") == 0 ) {
                  i_found = 1;
                  break;
               }
            } else {
               p_sock2 = (struct sockaddr_in *)p_res->ai_addr;
               v_sock = &(p_sock2->sin_addr);
               inet_ntop(p_res->ai_family, v_sock, ipstr, sizeof(ipstr)-1);
               if ( strcmp(ipstr, "127.0.0.2") == 0 ) {
                  i_found = 1;
                  break;
               }
            }
         }
         freeaddrinfo(res);
         if ( i_found )
            break;
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
    int is_ipv6 = (strchr(address, ':') != NULL);

#ifdef TLI
    struct t_bind *bindreq, *bindret;
    struct sockaddr_in *serverreq, *serverret;

#else
    struct sockaddr_in server;
    struct sockaddr_in6 server6;

#endif
    DPUSH; /* #1 */

#ifdef TLI
    s = t_open(TLI_TCP, (O_RDWR | O_NDELAY), NULL);
#else
    s = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
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
    if (is_ipv6) {
        opt = 1;
        if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, sizeof(opt)) < 0) {
            log_perror("NET", "FAIL", NULL, "setsockopt(IPV6_V6ONLY)");
        }
        memset(&server6, 0, sizeof(server6));
        server6.sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, address, &server6.sin6_addr) != 1)
            server6.sin6_addr = in6addr_any;
        server6.sin6_port = htons(port);
    } else {
        server.sin_family = AF_INET;
        if(inet_addr((const char*)address) != -1)
            server.sin_addr.s_addr = inet_addr((const char*)address);
        else
            server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);
    }

    i_loop = 0;
    if (bind(s, (struct sockaddr *) (is_ipv6 ? (void *)&server6 : (void *)&server),
             is_ipv6 ? sizeof(server6) : sizeof(server))) {
        while ( i_loop <= 5 ) {
           i_loop++;
           if ( mudconf.api_port == port ) {
	      log_perror("NET", "FAIL", unsafe_tprintf("[%d of 5] failed to bind to API port <%d>", i_loop, port), "bind");
           } else {
	      log_perror("NET", "FAIL", unsafe_tprintf("[%d of 5] failed to bind to port <%d>", i_loop, port), "bind");
           }
           if ( bind(s, (struct sockaddr *) (is_ipv6 ? (void *)&server6 : (void *)&server),
                     is_ipv6 ? sizeof(server6) : sizeof(server)) == 0 ) {
              break;
           } else {
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
shovechars(int port, char *address, char *address_v6, int ip_family)
{
    fd_set input_set, output_set;
    struct timeval last_slice, current_time, next_slice, timeout;
    int found, check, new_connection_error_count = 0;
    DESC *d, *newd;
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
    mudstate_hot.debug_cmd = (char *) "< shovechars >";
    sock = sock2 = sock6 = sock6_api = -1;
    maxd = 0;
    if (ip_family & 1) {
       sock = make_socket(port, address);
       maxd = sock + 1;
       if ( apiport != -1 ) {
          sock2 = make_socket(apiport, address);
          if ( sock2 >= maxd )
             maxd = sock2 + 1;
       }
    }
    if (ip_family & 2) {
       sock6 = make_socket(port, address_v6);
       if ( sock6 >= maxd )
          maxd = sock6 + 1;
       if ( apiport != -1 ) {
          sock6_api = make_socket(apiport, address_v6);
          if ( sock6_api >= maxd )
             maxd = sock6_api + 1;
       }
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
      if( D_DESCRIPTOR(d) >= maxd ) {
        maxd = D_DESCRIPTOR(d) + 1;
      }
      if ((D_FLAGS(d) & DS_HAS_DOOR) && (d->cold->door_desc >= maxd)) {
	maxd = d->cold->door_desc + 1;
      }
      /* If we rebooted after the d->cold->longaddr addition, makes sure
         all current d->addrs get copied over --Amb */
      if( d->cold->longaddrcheck != 242242242)
      {
        memset(d->cold->longaddr, '\0', sizeof(d->cold->longaddr));
        strncpy(d->cold->longaddr,d->cold->addr,sizeof(d->cold->longaddr));
        d->cold->longaddrcheck = 242242242;
      }
      if( D_FLAGS(d) & DS_CONNECTED ) {
        if(!silent) {
          queue_string(d, "Game: New server image successfully loaded.\r\n");
        } else if ( D_PLAYER(d) == found ) {
          queue_string(d, "Game: Finished Rebooting Silently.\r\n"); 
        }
        strncpy(all, Name(D_PLAYER(d)), 5);
        *(all + 5) = '\0';
        if ( strlen(mudconf.guest_namelist) > 0 ) {
           memset(tsitebuff, 0, sizeof(tsitebuff));
           strncpy(tsitebuff, mudconf.guest_namelist, 1000);
           ptsitebuff = strtok_r(tsitebuff, " \t", &tstrtokr);
           sitecntr = 1;
           while ( (ptsitebuff != NULL) && (sitecntr < 32) ) {
              if ( lookup_player(NOTHING, ptsitebuff, 0) == D_PLAYER(d) ) {
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
            temp1 = atoi((Name(D_PLAYER(d)) + 5));
            temp2 = 0x00000001;
            temp2 <<= (temp1 - 1);
            mudstate.guest_status |= temp2;
            mudstate.guest_num++;
        }
      }
      if ( Good_obj(D_PLAYER(d)) && InProgram(D_PLAYER(d)) ) {
         if ( (mudconf.login_to_prog && !(ProgCon(D_PLAYER(d)))) ||
              (!mudconf.login_to_prog && ProgCon(D_PLAYER(d))) ) {
            queue_string(d, "You are still in a @program.\r\n");
            progatr = atr_get(D_PLAYER(d), A_PROGPROMPTBUF, &aowner2, &aflags2);
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
            s_Flags4(D_PLAYER(d), (Flags4(D_PLAYER(d)) & (~INPROGRAM)));
            queue_string(d, "\377\371");
            mudstate.shell_program = 0;
            atr_clr(D_PLAYER(d), A_PROGBUFFER);
            atr_clr(D_PLAYER(d), A_PROGPROMPTBUF);
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

    while (mudstate_hot.shutdown_flag == 0) {

        /* Beginning of a new cycle - free unsafe_tprint buffers */
        freeTrackedBuffers();

	get_tod(&current_time);
	last_slice = update_quotas(last_slice, current_time);

	process_commands();
#ifdef ENABLE_DOORS
	door_processInternalDoors();
#endif
	local_tick();
	if (mudstate_hot.shutdown_flag || mudstate_hot.reboot_flag)
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

	if (ndescriptors < avail_descriptors && ndesc_slots < MAX_DESCRIPTORS) {
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
            if ( sock6 >= 0 ) {
               FD_SET(sock6, &input_set);
            }
            if ( sock6_api >= 0 ) {
               FD_SET(sock6_api, &input_set);
            }
#endif
	}

        active_auths = 0;

	/* Mark sockets that we want to test for change in status */
	DESC_ITER_ALL(d) {
#ifdef TLI
	    if (!D_INPUT_HEAD(d) || D_OUTPUT_HEAD(d)) {
		fds[D_DESCRIPTOR(d)].fd = D_DESCRIPTOR(d);
		fds[D_DESCRIPTOR(d)].events = 0;
		if (!D_INPUT_HEAD(d))
		    fds[D_DESCRIPTOR(d)].events |= POLLIN;
		if (D_OUTPUT_HEAD(d))
		    fds[D_DESCRIPTOR(d)].events |= POLLOUT;
	    }
#else
	    if (!D_INPUT_HEAD(d) && (D_FLAGS(d) & DS_AUTH_IN_PROGRESS) == 0)
		FD_SET(D_DESCRIPTOR(d), &input_set);
	    if (D_OUTPUT_HEAD(d))
		FD_SET(D_DESCRIPTOR(d), &output_set);
            /* If API we always want to test input and output */
            if ( D_FLAGS(d) & DS_API ) {
		FD_SET(D_DESCRIPTOR(d), &input_set);
		FD_SET(D_DESCRIPTOR(d), &output_set);
            }
	    if (D_FLAGS(d) & DS_AUTH_IN_PROGRESS) {
                if ( D_FLAGS(d) & DS_API ) {
                   D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
                } else {
                   active_auths++;
		   if (d->cold->authdescriptor >= maxd)
		       maxd = d->cold->authdescriptor + 1;
		   if (D_FLAGS(d) & (DS_NEED_AUTH_WRITE|DS_AUTH_CONNECTING))
		       FD_SET(d->cold->authdescriptor, &output_set);
		   FD_SET(d->cold->authdescriptor, &input_set);
               }
	    }
	    if (D_FLAGS(d) & DS_HAS_DOOR) {
	      if (d->cold->door_desc >= maxd)
		maxd = d->cold->door_desc + 1;
	      if (!d->cold->door_input_head)
		FD_SET(d->cold->door_desc, &input_set);
	      if (d->cold->door_output_head)
		FD_SET(d->cold->door_desc, &output_set);
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
	  DESC_SAFEITER_ALL(d) {
	    if( (D_FLAGS(d) & DS_AUTH_IN_PROGRESS) &&
	        (time(NULL) - d->cold->connected_at >= 3) ) {
		D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
		logbuff = alloc_mbuf("shovechars.LOG.authtimeout");
                sprintf(logbuff, "%s %s", d->cold->addr, (d->cold->addr_family == AF_INET6) ? "/128" : "255.255.255.255");
                cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NOAUTH | H_AUTOSITE), 0, 1, "noauth_site");
	      	shutdown(d->cold->authdescriptor, 2);
		close(d->cold->authdescriptor);
              	strcpy(d->cold->userid,"");
		STARTLOG(LOG_NET, "NET", "FAIL")
  		  sprintf(logbuff,
	   	  	"[%d/%s] Auth request timed out",
		    	D_DESCRIPTOR(d),
			d->cold->longaddr);
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
		if (D_DESCRIPTOR(newd) >= maxd)
		    maxd = D_DESCRIPTOR(newd) + 1;
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
		   if (D_DESCRIPTOR(newd) >= maxd)
		       maxd = D_DESCRIPTOR(newd) + 1;
	       }
	   }
         }
        if ( sock6 >= 0 ) {
	   check = CheckInput(sock6);
	   if (check) {
	       newd = new_connection(sock6, 0);
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
		   if (D_DESCRIPTOR(newd) >= maxd)
		       maxd = D_DESCRIPTOR(newd) + 1;
	       }
	   }
        }
        if ( sock6_api >= 0 ) {
	   check = CheckInput(sock6_api);
	   if (check) {
	       newd = new_connection(sock6_api, 1);
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
		   if (D_DESCRIPTOR(newd) >= maxd)
		       maxd = D_DESCRIPTOR(newd) + 1;
	       }
	   }
        }

	/* Check for activity on user sockets */
	DESC_SAFEITER_ALL(d) {
	    /* Check for Auth */

	    if (D_FLAGS(d) & DS_AUTH_IN_PROGRESS) {
		if ((D_FLAGS(d) & DS_AUTH_CONNECTING) &&
                    CheckOutput(d->cold->authdescriptor)) {
                    check_auth_connect(d);
                }
		else if ((D_FLAGS(d) & DS_NEED_AUTH_WRITE) &&
		    CheckOutput(d->cold->authdescriptor)) {
		    write_auth(d);
                }
                else if( CheckInput(d->cold->authdescriptor) ) {
		      check_auth(d);
                }
	    }

	    if (D_FLAGS(d) & DS_HAS_DOOR) {
	      if (CheckInput(d->cold->door_desc)) {
		D_LAST_TIME(d) = mudstate_hot.now;
		if (!process_door_input(d)) {
		  closeDoorWithId(d, d->cold->door_num);
		}
	      }
	    }
	    /* Lensy:
	     *  Additional check needed because the above call might close the conny
	     */
	    if (D_FLAGS(d) & DS_HAS_DOOR) {
	      if (CheckOutput(d->cold->door_desc)) {
		if (!process_door_output(d)) {
		  closeDoorWithId(d, d->cold->door_num);
		}
	      }
	    }
	    /* Process input from sockets with pending input */

	    check = CheckInput(D_DESCRIPTOR(d));
	    if (check) {

		/* Undo autodark */

                i_oldlasttime = D_LAST_TIME(d);
                flagkeep = D_FLAGS(d);
                if ( Good_obj(D_PLAYER(d)) && !TogHideIdle(D_PLAYER(d)) ) {
                   D_LAST_TIME(d) = mudstate_hot.now;
                   if (D_FLAGS(d) & DS_AUTODARK) {
                       D_FLAGS(d) &= ~DS_AUTODARK;
                       s_Flags(D_PLAYER(d),
                               Flags(D_PLAYER(d)) & ~DARK);
                   }
                   if (D_FLAGS(d) & DS_AUTOUNF) {
                       D_FLAGS(d) &= ~DS_AUTOUNF;
                       s_Flags2(D_PLAYER(d),
                               Flags2(D_PLAYER(d)) & ~UNFINDABLE);
                   }
                } else if ( D_LAST_TIME(d) == 0 ) {
                   D_LAST_TIME(d) = mudstate_hot.now;
                }

		/* Process received data */

                i_oldlastcnt = D_INPUT_TOT(d);
				
		if (!process_input(d)) {
		    shutdownsock(d, R_SOCKDIED);
		    continue;
		}

                /* Idle stamp checking for command typed */
                if ( mudconf.idle_stamp && (D_FLAGS(d) & DS_CONNECTED) && D_INPUT_HEAD(d) && D_INPUT_HEAD(d)->cmd[0] ) {
                   ulCRC32 = 0;
                   i_len = strlen(D_INPUT_HEAD(d)->cmd);
                   ulCRC32 = CRC32_ProcessBuffer(ulCRC32, D_INPUT_HEAD(d)->cmd, i_len);
                   anum = mkattr("_IDLESTAMP");
                   if ( anum > 0 ) {
                      ap = atr_num(anum);
                      if (ap) {
                         attr_wizhidden("_IDLESTAMP");
                         progatr = atr_get(D_PLAYER(d), ap->number, &aowner2, &aflags2);
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
                                              D_LAST_TIME(d) = i_oldlasttime;
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
                            atr_add_raw(D_PLAYER(d), ap->number, b_progatr);
                            free_sbuf(s_progatr);
                            free_lbuf(b_progatr);
                         }
                         free_lbuf(progatr);
                      }
                   }
                }

                if ( (D_FLAGS(d) & DS_CONNECTED) && D_INPUT_HEAD(d) && D_INPUT_HEAD(d)->cmd[0] ) {
                   memcpy(s_cutter, D_INPUT_HEAD(d)->cmd, 5);
                   memcpy(s_cutter2, D_INPUT_HEAD(d)->cmd, 7);
                   s_cutter[5] = '\0';
                   s_cutter2[7] = '\0';
                   if ( Good_obj(D_PLAYER(d)) && 
                       (((Wizard(D_PLAYER(d)) || HasPriv(D_PLAYER(d), NOTHING, POWER_WIZ_IDLE, POWER5, NOTHING)) && (stricmp(s_cutter, "idle ") == 0)) || 
                        ((stricmp(s_cutter, "@@") == 0) && mudconf.null_is_idle) ||
                        ((stricmp(s_cutter, "th") == 0) && mudconf.think_is_idle) ||
                        ((stricmp(s_cutter, "thi") == 0) && mudconf.think_is_idle) ||
                        ((stricmp(s_cutter, "thin") == 0) && mudconf.think_is_idle) ||
                        ((stricmp(s_cutter, "think") == 0) && mudconf.think_is_idle) ||
                        (stricmp(s_cutter, "idle") == 0) ||
                        (stricmp(s_cutter2, "idle @@") == 0)) ) {
                      cmdp = (CMDENT *) ohtab_find("idle", &mudstate_hot.command_htab);
                      if ( cmdp && check_access(D_PLAYER(d), cmdp->perms, cmdp->perms2, 0)) {
                         if ( !(CmdCheck(D_PLAYER(d)) && cmdtest(D_PLAYER(d), "idle")) ) {
                            D_LAST_TIME(d) = i_oldlasttime;
                            D_FLAGS(d) = D_FLAGS(d) | flagkeep;
                            if ( D_FLAGS(d) & DS_AUTOUNF ) 
                               s_Flags2(D_PLAYER(d), Flags2(D_PLAYER(d)) | UNFINDABLE);
                            if ( D_FLAGS(d) & DS_AUTODARK ) 
                               s_Flags(D_PLAYER(d), Flags(D_PLAYER(d)) | DARK);
                         }
                      }
                   }
                }

                /* Ignore Null Input */
                if ( (D_INPUT_TOT(d) <= (i_oldlastcnt + 2)) && D_INPUT_HEAD(d) && D_INPUT_HEAD(d)->cmd[0] &&
                     ((*(D_INPUT_HEAD(d)->cmd) == '\r') || (*(D_INPUT_HEAD(d)->cmd) == '\n')) ) {
                   D_LAST_TIME(d) = i_oldlasttime;
                }
                /* Ignore Potato's broken NOP code */
                if ( (D_INPUT_TOT(d) == (i_oldlastcnt + 3)) && !D_INPUT_HEAD(d) ) {
                   D_LAST_TIME(d) = i_oldlasttime;
                }
	    }

	    /* Process output for sockets with pending output */

	    check = CheckOutput(D_DESCRIPTOR(d));
	    if (check) {
		if (!process_output(d)) {
		    shutdownsock(d, R_SOCKDIED);
		}
	    }

            /* Force API disconnect if timeon greater than timeout value */
            if ( (D_FLAGS(d) & DS_API) ) {
		if ( (d->cold->connected_at + d->cold->timeout) < time(NULL) ) {
			shutdownsock(d, R_API);
		}
            }

#ifdef ENABLE_WEBSOCKETS
            /* WebSocket cleanup: close + ping/pong keepalive */
            if (D_FLAGS(d) & DS_WEBSOCKETS) {
                if (d->cold->ws_closing) {
                    shutdownsock(d, R_WEBSOCKETS);
                    continue;
                }
                time_t now = time(NULL);
                if (!d->cold->ws_last_pong)
                    d->cold->ws_last_pong = now;
                else if (now - d->cold->ws_last_pong > WS_PING_TIMEOUT)
                    shutdownsock(d, R_TIMEOUT);
                else if (now - d->cold->ws_last_pong > WS_PING_INTERVAL)
                    websocket_send_ping(d);
            }
#endif
	}
	door_checkInternalDoorDescriptors(&input_set, &output_set);
	
    }
    DPOPCONDITIONAL; /* #2 */
}

const char *
addrout(const char *ip_str, int addr_family, int i_key)
{
    static char hostname_buf[NI_MAXHOST];
    char *logbuff;

    DPUSH; /* #3 */

    if (mudconf.use_hostname) {
        int gai_ret;
        struct sockaddr_storage sa_storage;
        struct sockaddr *sa;
        socklen_t sa_len;

        memset(&sa_storage, 0, sizeof(sa_storage));
        if (addr_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&sa_storage;
            if (inet_pton(AF_INET6, ip_str, &sin6->sin6_addr) != 1) {
               RETURN(ip_str);
            }
            sin6->sin6_family = AF_INET6;
            sa = (struct sockaddr *)sin6;
            sa_len = sizeof(struct sockaddr_in6);
        } else {
            struct sockaddr_in *sin4 = (struct sockaddr_in *)&sa_storage;
            if (inet_pton(AF_INET, ip_str, &sin4->sin_addr) != 1) {
               RETURN(ip_str);
            }
            sin4->sin_family = AF_INET;
            sa = (struct sockaddr *)sin4;
            sa_len = sizeof(struct sockaddr_in);
        }

        if ( (i_key && mudconf.api_nodns) ||
             site_check_str(ip_str, addr_family, mudstate.special_list, 1, 0, H_NODNS) & H_NODNS ||
             blacklist_check_str(ip_str, addr_family, 1) ) {
           RETURN(ip_str);
        }

        gai_ret = getnameinfo(sa, sa_len, hostname_buf, sizeof(hostname_buf), NULL, 0, 0);
        if (gai_ret == 0) {
            if ( !mudconf.nospam_connect ) {
               STARTLOG(LOG_NET, "NET", "DNS")
                 logbuff = alloc_lbuf("bsd.addrout");
                 sprintf(logbuff, "%s = %s", ip_str, hostname_buf);
                 log_text(logbuff);
                 free_lbuf(logbuff);
               ENDLOG
            }
            RETURN(hostname_buf);
        } else {
    	    logbuff = alloc_lbuf("bsd.addrout");
            snprintf(logbuff, LBUF_SIZE, "%s %s", ip_str, (addr_family == AF_INET6) ? "/128" : "255.255.255.255");
            cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NODNS | H_AUTOSITE), 0, 1, "nodns_site");
            free_lbuf(logbuff);
            RETURN(ip_str);
        }
    }
    RETURN(ip_str);
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
    DESC *d, *dchk;
    struct sockaddr_storage addr_storage;
    char ipbuf[INET6_ADDRSTRLEN];
    int addr_family = AF_INET;
    struct in_addr ipv4_addr;
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
    socklen_t addr_len;

#else
    int ret;
    struct sockaddr_in addr;

#endif

    DPUSH; /* #5 */

    cur_port = i_proxychk = 0;
    cmdsave = mudstate_hot.debug_cmd;
    mudstate_hot.debug_cmd = (char *) "< new_connection >";
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

    sighold(SIGALRM);
    ret = ioctl(newsock, I_PUSH, "tirdwr");
    sigrelse(SIGALRM);

    if (ret < 0) {
	log_tli_error("STRM", "ioctl I_PUSH", errno, t_errno);
	t_close(newsock);
	RETURN(0); /* #5 */
    }
    bcopy(nc_call->addr.buf, &addr, sizeof(struct sockaddr_in));
    addr_family = AF_INET;
    ipv4_addr = addr.sin_addr;
    cur_port = ntohs(addr.sin_port);
    inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf));

#else
    addr_len = sizeof(addr_storage);

    newsock = accept(sock, (struct sockaddr *) &addr_storage, &addr_len);
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

    if (addr_storage.ss_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&addr_storage;
        cur_port = ntohs(sin->sin_port);
        addr_family = AF_INET;
        ipv4_addr = sin->sin_addr;
        inet_ntop(AF_INET, &sin->sin_addr, ipbuf, sizeof(ipbuf));
    } else {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&addr_storage;
        cur_port = ntohs(sin6->sin6_port);
        addr_family = AF_INET6;
        memset(&ipv4_addr, 0, sizeof(ipv4_addr));
        inet_ntop(AF_INET6, &sin6->sin6_addr, ipbuf, sizeof(ipbuf));
    }
#endif /* TLI */

    memset(tchbuff, 0, sizeof(tchbuff));

     if ( mudconf.tor_paranoid ) {
        i_chktor = check_tor_str(ipbuf, addr_family, mudconf.port);
     }

    tsite_buff = alloc_lbuf("check_max_sitecons");
    maxsitecon = 0;
    maxtsitecon = 0;

#ifndef __MACH__
#ifndef CYGWIN
#ifndef BROKEN_PROXY
    if ( addr_family == AF_INET && mudconf.proxy_checker > 0 ) {
       i_mtulen = sizeof(i_mtu);
       i_msslen = sizeof(i_mss);
       getsockopt(newsock, SOL_IP, IP_MTU, &i_mtu, (unsigned int *)&i_mtulen);
       getsockopt(newsock, IPPROTO_TCP, TCP_MAXSEG,  &i_mss, (unsigned int*)&i_msslen);
   
       if ( (i_mtu <= 1500) && ((i_mss + 80) < i_mtu) ) {
          addroutbuf = (char *) addrout(ipbuf, addr_family, (i_addflags & MF_API));
          strcpy(tsite_buff, addroutbuf);
          strcpy(tchbuff, mudconf.passproxy_host);
          if ( !( (site_check(ipv4_addr, mudstate.suspect_list, 1, 0, H_PASSPROXY) == H_PASSPROXY) || 
                  (mudconf.passproxy_host[0] && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar)) ) ) {
             STARTLOG(LOG_NET | LOG_SECURITY, "NET", "PROXY");
                buff = alloc_lbuf("new_connection.LOG.badsite");
                sprintf(buff, "[%d/%s] Possible Proxy [MTU %d/MSS %d].  (Remote port %d)",
                        newsock, ipbuf, i_mtu, i_mss, cur_port);
                log_text(buff);
                free_lbuf(buff);
             ENDLOG
             if ( (mudconf.proxy_checker & 2) && (mudconf.proxy_checker & 4) ) {
                i_proxychk = H_NOGUEST | H_REGISTRATION;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [GUEST/REGISTER DISABLED]", NULL,
                                        ipbuf, newsock, 0, cur_port, NULL);
             } else if ( mudconf.proxy_checker & 2 ) {
                i_proxychk = H_NOGUEST;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [GUEST DISABLED]", NULL,
                                        ipbuf, newsock, 0, cur_port, NULL);
             } else if ( mudconf.proxy_checker & 4 ) {
                i_proxychk = H_REGISTRATION;
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY [REGISTER DISABLED]", NULL,
                                        ipbuf, newsock, 0, cur_port, NULL);
             } else {
                broadcast_monitor(NOTHING, MF_CONN | i_addflags, "POSSIBLE PROXY", NULL,
                                        ipbuf, newsock, 0, cur_port, NULL);
             }
          } 
       }
    }
#endif
#endif
#endif

    if ( blacklist_check_str(ipbuf, addr_family, 0) || i_chktor ) {
       STARTLOG(LOG_NET | LOG_SECURITY, "NET", (i_chktor ? "TOR" : "BLACK"));
          buff = alloc_lbuf("new_connection.LOG.badsite");
          sprintf(buff, "[%d/%s] Connection refused - %s.  (Remote port %d)",
                  newsock, ipbuf, (i_chktor ? "TOR" : "Blacklisted"), cur_port);
          log_text(buff);
          if ( i_chktor ) {
             broadcast_monitor(NOTHING, MF_CONN | i_addflags, "SITE IN TOR", NULL,
                               ipbuf, newsock, 0, cur_port, NULL);
          } else {
             broadcast_monitor(NOTHING, MF_CONN | i_addflags, "SITE IN BLACKLIST", NULL,
                               ipbuf, newsock, 0, cur_port, NULL);
          }
          free_lbuf(buff);
       ENDLOG
        fcache_rawdump(newsock, FC_CONN_SITE, ipbuf, (char *)NULL);
       shutdown(newsock, 2);
       close(newsock);
       errno = 0;
       free_lbuf(tsite_buff);
       RETURN(0);
    }

     if ( i_addflags & MF_API ) {
        mudstate.api_lastsite_cnt++;
        if ( ((mudstate.last_apicon_attempt + 60) < mudstate_hot.now) || (mudstate.last_apicon_attempt == 0) ) {
           mudstate.last_apicon_attempt = mudstate_hot.now;
        }
        strcpy(tchbuff, mudconf.passapi_host);
        addroutbuf = (char *) addrout(ipbuf, addr_family, (i_addflags & MF_API));
        if ( ((mudstate.api_lastsite_cnt >= mudconf.max_lastsite_api) &&
              (strcmp(mudstate.api_lastsite_ip, ipbuf) == 0) &&
              (mudstate.last_apicon_attempt >= (mudstate_hot.now - 60))) &&
            !((site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_PASSAPI) == H_PASSAPI) ||
                (mudconf.passapi_host[0] && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar))) ) {
            sprintf(tchbuff, "%s %s", ipbuf, (addr_family == AF_INET6) ? "/128" : "255.255.255.255");
            if ( !(site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_FORBIDAPI) == H_FORBIDAPI) ) {
              cf_site((int *)&mudstate.access_list, tchbuff, H_FORBIDAPI|H_AUTOSITE, 0, 1, "forbidapi_site");
              broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("API SITE SET AUTO-FORBID[%d attempts/%ds time]", 
                                mudstate.api_lastsite_cnt, (mudstate_hot.now - mudstate.last_apicon_attempt)),
                                NULL, ipbuf, newsock, 0, cur_port, NULL);
              STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOF")
                 buff = alloc_lbuf("new_connection.LOG.autoforbidapi");
                 sprintf(buff, "[%d/%s] marked for autoAPIforbid.  (Remote port %d)",
                  newsock, ipbuf, cur_port);
                 log_text(buff);
                 free_lbuf(buff);
              ENDLOG
           }
           mudstate.api_lastsite_cnt = 0;
           memset(tchbuff, 0, sizeof(tchbuff));
        }
        strncpy(mudstate.api_lastsite_ip, ipbuf, sizeof(mudstate.api_lastsite_ip) - 1);
        mudstate.api_lastsite_ip[sizeof(mudstate.api_lastsite_ip) - 1] = '\0';
     }
     if ( !(i_addflags & MF_API) && (mudconf.lastsite_paranoia > 0) ) { 
       if ( !(site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_PERMIT) == H_PERMIT) ) {
         if ( ((mudconf.lastsite_paranoia > 1) && 
               !(site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN)) ||
              ((mudconf.lastsite_paranoia == 1) &&
               !(site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_REGISTRATION) == H_REGISTRATION)) ) {
           if ( (mudstate.cmp_lastsite_cnt >= mudconf.max_lastsite_cnt) &&
                (strcmp(mudstate.lastsite_ip, ipbuf) == 0) &&
                 (mudstate.last_con_attempt >= (mudstate_hot.now - mudconf.min_con_attempt)) ) {
               sprintf(tchbuff, "%s %s", ipbuf, (addr_family == AF_INET6) ? "/128" : "255.255.255.255");
              if (mudconf.lastsite_paranoia == 1) {
                 cf_site((int *)&mudstate.access_list, tchbuff,
                         H_REGISTRATION|H_AUTOSITE, 0, 1, "register_site");
    	        broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("SITE SET AUTO-REGISTER[%d attempts/%ds time]", 
                                   mudstate.cmp_lastsite_cnt, 
                                   (mudstate_hot.now - mudstate.last_con_attempt)),
                                   NULL, ipbuf, newsock, 0, cur_port, NULL);
  	        STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOR")
  	        buff = alloc_lbuf("new_connection.LOG.autoregister");
  	        sprintf(buff, "[%d/%s] marked for autoregistration.  (Remote port %d)",
  		        newsock, ipbuf, cur_port);
  	        log_text(buff);
  	        free_lbuf(buff);
  	        ENDLOG
              } else {
                 cf_site((int *)&mudstate.access_list, tchbuff,
                         H_FORBIDDEN|H_AUTOSITE, 0, 1, "forbid_site");
    	        broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("SITE SET AUTO-FORBID[%d attempts/%ds time]", 
                                   mudstate.cmp_lastsite_cnt, 
                                   (mudstate_hot.now - mudstate.last_con_attempt)),
                                   NULL, ipbuf, newsock, 0, cur_port, NULL);
  	        STARTLOG(LOG_NET | LOG_SECURITY, "NET", "AUTOF")
  	        buff = alloc_lbuf("new_connection.LOG.autoregister");
  	        sprintf(buff, "[%d/%s] marked for autoforbidding.  (Remote port %d)",
  		        newsock, ipbuf, cur_port);
  	        log_text(buff);
  	        free_lbuf(buff);
  	        ENDLOG
               }
               mudstate.cmp_lastsite_cnt = 0;
           } else if ( (mudconf.lastsite_paranoia > 0) &&
                       ((strcmp(mudstate.lastsite_ip, ipbuf) != 0) || 
                        (mudstate.last_con_attempt < (mudstate_hot.now - mudconf.min_con_attempt))) ) {
              strncpy(mudstate.lastsite_ip, ipbuf, sizeof(mudstate.lastsite_ip) - 1);
              mudstate.lastsite_ip[sizeof(mudstate.lastsite_ip) - 1] = '\0';
              mudstate.cmp_lastsite_cnt = 0;
           }
           if ( ((mudstate.last_con_attempt + mudconf.min_con_attempt) < mudstate_hot.now) ||
                 (mudstate.last_con_attempt == 0) ) {
              mudstate.last_con_attempt = mudstate_hot.now;
              mudstate.cmp_lastsite_cnt = 0;
           }
           mudstate.cmp_lastsite_cnt++;
           memset(tchbuff, 0, sizeof(tchbuff));
         }
       }
     }
    addroutbuf = (char *) addrout(ipbuf, addr_family, (i_addflags & MF_API));
    strcpy(tsite_buff, addroutbuf);
    if ( tsite_buff ) {
       DESC_SAFEITER_ALL(dchk) {
          if ( strcmp(tsite_buff, dchk->cold->addr) == 0 )
             maxsitecon++;
          maxtsitecon++;
       }
    }

    strcpy(tchbuff, mudconf.permit_host);
    strcpy(tsite_buff, addroutbuf);
    if ( !(site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_PERMIT) == H_PERMIT) &&
        !(mudconf.permit_host[0] && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar)) ) {
      strcpy(tchbuff, mudconf.forbid_host);
      if ((site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN) ||
          (maxsitecon >= mudconf.max_sitecons) || (maxtsitecon >= (mudstate.max_logins_allowed+7)) ||
           (mudconf.forbid_host[0] && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar))) {
    
          i_chksite = site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 1, H_FORBIDDEN);
         i_forbid = 1;
         if ( i_chksite != -1 ) {  
            if ( maxsitecon < i_chksite ) {
               i_forbid = 0;
            }
         }
      }
    }
    if ( !i_forbid && (i_addflags & MF_API) ) {
       strcpy(tchbuff, mudconf.forbidapi_host);
       strcpy(tsite_buff, addroutbuf);
        if ( (site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 0, H_FORBIDAPI) == H_FORBIDAPI) ||
            (maxsitecon >= mudconf.max_sitecons) || (maxtsitecon >= (mudstate.max_logins_allowed+7)) ||
             (mudconf.forbidapi_host[0] && lookup(addroutbuf, tchbuff, maxsitecon, &i_retvar)) ) {
    
          i_chksite = site_check_str(ipbuf, addr_family, mudstate.access_list, 1, 1, H_FORBIDAPI);
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
         if ( !mudconf.nospam_connect ) {
	   STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
	       buff = alloc_lbuf("new_connection.LOG.badsite");
	   sprintf(buff, "[%d/%s] Connection refused.  (Remote port %d)",
		   newsock, ipbuf, cur_port);
	   log_text(buff);
	   free_lbuf(buff);
	   ENDLOG
         } else if ( mudconf.nospam_connect == 1 ) {
            if ( !(*mudstate.nospam_lastsite) || (strcmp(mudstate.nospam_lastsite, ipbuf) != 0) ) {
               if ( mudstate_hot.nospam_counter > 0 ) {
                  STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
                     buff = alloc_lbuf("new_connection.LOG.badsite");
                     sprintf(buff, "[%s] Connection refused [total %d times].",
                             mudstate.nospam_lastsite, mudstate_hot.nospam_counter);
                     log_text(buff);
                     free_lbuf(buff);
                  ENDLOG
               }
               memset(mudstate.nospam_lastsite, '\0', sizeof(mudstate.nospam_lastsite));
               sprintf(mudstate.nospam_lastsite, "%.*s", (int)sizeof(mudstate.nospam_lastsite) - 1, ipbuf);
               mudstate_hot.nospam_counter = 1;
            } else {
               mudstate_hot.nospam_counter++;
            }
         }
         if ( maxsitecon >= mudconf.max_sitecons ) {
   	   broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("MAX OPEN PORTS[%d]", maxsitecon), NULL, 
                              ipbuf, newsock, 0, cur_port, NULL);
         } else if ( maxtsitecon >= (mudstate.max_logins_allowed+7) ) {
   	   broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("MAX OPEN DESCRIPTORS[%d]", maxtsitecon), NULL, 
                              ipbuf, newsock, 0, cur_port, NULL);
         } else {
            if ( i_chksite == -1 )
               i_chksite = i_retvar;
            if ( i_chksite != -1 ) {
   	      broadcast_monitor(NOTHING, MF_CONN | i_addflags, unsafe_tprintf("PORT REJECT[%d max]", i_chksite), NULL, 
                                 ipbuf, newsock, 0, cur_port, NULL);
            } else {
   	      broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT REJECT", NULL, 
                                 ipbuf, newsock, 0, cur_port, NULL);
            }
         }
          if ( i_addflags & MF_API ) {
             fcache_rawdump(newsock, FC_CONN_SITE, ipbuf, (char *)"SITE_FORBIDAPI");
          } else {
             if ( i_chksite != -1 ) {
   	      fcache_rawdump(newsock, FC_CONN_SITE, ipbuf, (char *)"SITE_FORBID");
             } else {
   	      fcache_rawdump(newsock, FC_CONN_SITE, ipbuf, (char *)NULL);
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
               if ( mudstate_hot.nospam_counter > 0 ) {
                  STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE")
                     sprintf(buff, "[%s] Connection refused [total %d times].",
                             mudstate.nospam_lastsite, mudstate_hot.nospam_counter);
                     log_text(buff);
                  ENDLOG
               }
               memset(mudstate.nospam_lastsite, '\0', sizeof(mudstate.nospam_lastsite));
               mudstate_hot.nospam_counter = 0;
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
	d = initializesock(newsock, ipbuf, addr_family, cur_port, buff, i_proxychk, key);
         broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT CONNECT", NULL, buff, newsock, 0, cur_port, NULL);
	free_lbuf(buff);
	mudstate_hot.debug_cmd = cmdsave;
    }
    mudstate_hot.debug_cmd = cmdsave;

    if ( key && d ) {
       D_FLAGS(d) |= DS_API;
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
    DESC *dchk;

    DPUSH; /* #6 */
    i_addflags = 0;
    if ( D_FLAGS(d) & DS_API ) {
       i_addflags = MF_API;
    }

    if ( ((reason == R_LOGOUT) || (reason == R_SU)) &&
          (site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, H_FORBIDDEN) == H_FORBIDDEN)) {
	reason = R_QUIT;
        if( D_FLAGS(d) & DS_API ) {
           reason = R_API; 
        }
    }

    if (D_FLAGS(d) & DS_CONNECTED) {

        handle_conninfo_write(d, D_PLAYER(d), CONN_ALL); /* Write A_CONNINFO */
	strncpy(all, Name(D_PLAYER(d)), 5);
	*(all + 5) = '\0';
        if ( strlen(mudconf.guest_namelist) > 0 ) {
           memset(tsitebuff, 0, sizeof(tsitebuff));
           strncpy(tsitebuff, mudconf.guest_namelist, 1000);
           ptsitebuff = strtok_r(tsitebuff, " \t", &tstrtokr);
           sitecntr = 1;
           while ( (ptsitebuff != NULL) && (sitecntr < 32) ) {
              if ( lookup_player(NOTHING, ptsitebuff, 0) == D_PLAYER(d) ) {
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
	    temp1 = atoi((Name(D_PLAYER(d)) + 5));
	    temp2 = 0x00000001;
	    temp2 <<= (temp1 - 1);
	    mudstate.guest_status &= ~temp2;
	    mudstate.guest_num--;
	}
	if (mudconf.maildelete)
	    mail_md1(D_PLAYER(d), D_PLAYER(d), 1, -1, 0);
	strcpy(all, "all");
	mail_mark(D_PLAYER(d), M_READM, all, NOTHING, 1);
	atr_clr(D_PLAYER(d), A_MPSET);
	atr_clr(D_PLAYER(d), A_MCURR);

	/* Do the disconnect stuff if we aren't doing a LOGOUT
	 * (which keeps the connection open so the player can connect
	 * to a different character).
	 */

	if ( (reason != R_LOGOUT) && (reason != R_SU) ) {
	    fcache_dump(d, FC_QUIT, (char *)NULL);
	    STARTLOG(LOG_NET | LOG_LOGIN, "NET", "DISC")
		buff = alloc_lbuf("shutdownsock.LOG.disconn");
	    sprintf(buff, "[%d/%s] Logout by ",
		    D_DESCRIPTOR(d), d->cold->longaddr);
	    log_text(buff);
	    log_name(D_PLAYER(d));
	    sprintf(buff, " <Reason: %s>",
		    disc_reasons[reason]);
	    log_text(buff);
	    free_lbuf(buff);
	    ENDLOG
	} else {
	    STARTLOG(LOG_NET | LOG_LOGIN, "NET", "LOGO")
		buff = alloc_lbuf("shutdownsock.LOG.logout");
	    sprintf(buff, "[%d/%s] Logout by ",
		    D_DESCRIPTOR(d), d->cold->longaddr);
	    log_text(buff);
	    log_name(D_PLAYER(d));
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
	    now = mudstate_hot.now - d->cold->connected_at;
	buff = alloc_lbuf("shutdownsock.LOG.accnt");
	buff2 = decode_flags(GOD, GOD, Flags(D_PLAYER(d)),
			     Flags2(D_PLAYER(d)), Flags3(D_PLAYER(d)),
			     Flags4(D_PLAYER(d)));
	sprintf(buff, "%d %s %d %ld %d %d [%s] <%s> %s",
		D_PLAYER(d), buff2, d->cold->command_count, now,
		Location(D_PLAYER(d)), Pennies(D_PLAYER(d)),
		d->cold->longaddr, disc_reasons[reason],
		Name(D_PLAYER(d)));
	log_text(buff);
	free_lbuf(buff);
	free_mbuf(buff2);
	ENDLOG
	announce_disconnect(D_PLAYER(d), d, disc_messages[reason]);
    } else {
	if ( (reason == R_LOGOUT) || (reason == R_SU) ) {
	    reason = R_QUIT;
        }
        if ( D_FLAGS(d) & DS_API ) {
	    reason = R_API;
        }
        if ( D_FLAGS(d) & DS_WEBSOCKETS ) {
	    reason = R_WEBSOCKETS;
        }
	STARTLOG(LOG_SECURITY | LOG_NET, "NET", "DISC")
	    buff = alloc_lbuf("shutdownsock.LOG.neverconn");
	sprintf(buff,
		"[%d/%s] Connection closed, never connected. <Reason: %s>",
		D_DESCRIPTOR(d), d->cold->longaddr, disc_reasons[reason]);
	log_text(buff);
	free_lbuf(buff);
	ENDLOG
    }
    /* Send WebSocket close frame before shutting down */
#ifdef ENABLE_WEBSOCKETS
    if ((D_FLAGS(d) & DS_WEBSOCKETS) && !d->cold->ws_closing) {
	websocket_send_close(d, WS_CLOSE_GOING);
    }
#endif
    process_output(d);
    process_door_output(d);
    clearstrings(d);
    i_sitecnt = i_guestcnt = 0;
    if (D_FLAGS(d) & DS_HAS_DOOR) closeDoorWithId(d, d->cold->door_num);
    if ( (reason == R_LOGOUT) || (reason == R_SU) ) {
        addroutbuf = (char *) addrout(d->cold->addr, d->cold->addr_family, (D_FLAGS(d) & DS_API));
        t_addroutbuf = alloc_lbuf("check_max_sitecons");
        strcpy(t_addroutbuf, addroutbuf);
        if ( t_addroutbuf ) {
           DESC_SAFEITER_ALL(dchk) {
              if ( strcmp(t_addroutbuf, dchk->cold->addr) == 0 ) {
                 if ( Good_chk(D_PLAYER(dchk)) && Guest(D_PLAYER(dchk)) ) {
                    i_guestcnt++;
                 }
                 i_sitecnt++;
              }
           }
        }
        free_lbuf(t_addroutbuf);
	D_FLAGS(d) &= ~DS_CONNECTED;
	d->cold->connected_at = mudstate_hot.now;
	d->cold->retries_left = mudconf.retry_limit;
	d->cold->regtries_left = mudconf.regtry_limit;
	d->cold->command_count = 0;
	d->cold->timeout = mudconf.idle_timeout;
	D_PLAYER(d) = 0;
	d->cold->doing[0] = '\0';
	D_QUOTA(d) = mudconf.cmd_quota_max;
	D_LAST_TIME(d) = 0;
         D_HOST_INFO(d) = (site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 0, 0) & ~0x4) |
 	 			  site_check_str(d->cold->addr, d->cold->addr_family,
 			  mudstate.suspect_list, 0, 0, 0);
            i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_FORBIDDEN);
            if ( (i_sitemax != -1) && (i_sitecnt < (i_sitemax + 1)) )
               D_HOST_INFO(d) &= ~H_FORBIDDEN;
            i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_REGISTRATION);
            if ( (i_sitemax != -1) && (i_sitecnt < (i_sitemax + 1)) )
               D_HOST_INFO(d) &= ~H_REGISTRATION;
            i_sitemax = site_check_str(d->cold->addr, d->cold->addr_family, mudstate.access_list, 1, 1, H_NOGUEST);
            if ( (i_sitemax != -1) && (i_guestcnt < (i_sitemax + 1)) )
               D_HOST_INFO(d) &= ~H_NOGUEST;

        memset(tchbuff, 0, sizeof(tchbuff));
        strcpy(tchbuff, mudconf.forbid_host);
        if (mudconf.forbid_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_FORBIDDEN;
        strcpy(tchbuff, mudconf.register_host);
        if (mudconf.register_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_REGISTRATION;
         if ( blacklist_check_str(d->cold->addr, d->cold->addr_family, 2) ) {
            D_HOST_INFO(d) = D_HOST_INFO(d) | H_REGISTRATION;
         }
         strcpy(tchbuff, mudconf.autoreg_host);
          if (mudconf.autoreg_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
            D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOAUTOREG;
         strcpy(tchbuff, mudconf.noguest_host);
          if (mudconf.noguest_host[0] && lookup(d->cold->longaddr, tchbuff, i_guestcnt, &i_retvar))
            D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOGUEST;
         if ( blacklist_check_str(d->cold->addr, d->cold->addr_family, 3) ) {
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOGUEST;
        }
        strcpy(tchbuff, mudconf.suspect_host);
        if (mudconf.suspect_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_SUSPECT;
        strcpy(tchbuff, mudconf.passproxy_host);
        if (mudconf.passproxy_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_PASSPROXY;
        strcpy(tchbuff, mudconf.hardconn_host);
        if (mudconf.hardconn_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_HARDCONN;
        strcpy(tchbuff, mudconf.permit_host);
        if (mudconf.permit_host[0] && lookup(d->cold->longaddr, tchbuff, i_sitecnt, &i_retvar))
           D_HOST_INFO(d) = D_HOST_INFO(d) | H_PERMIT;
	D_INPUT_TOT(d) = D_INPUT_SIZE(d);
	D_OUTPUT_TOT(d) = 0;
         
        mudstate.total_bytesin += D_INPUT_SIZE(d);
        if ( (mudstate.reset_daily_bytes + 86400) < mudstate_hot.now ) {
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
           mudstate.daily_bytesin = D_INPUT_SIZE(d);
           mudstate.reset_daily_bytes = time(NULL);
        } else {
           mudstate.daily_bytesin += D_INPUT_SIZE(d);
        }
 
	/* free up the snooplist before freeing the desc -Thorin */
          
	while (d->cold->snooplist) {
	    temp = d->cold->snooplist->next;
	    if (d->cold->snooplist->sfile) {
		fprintf(d->cold->snooplist->sfile, "Snoop stopped due to logout: (%ld) %s", mudstate_hot.now, ctime(&mudstate_hot.now));
		fclose(d->cold->snooplist->sfile);
	    }
	    free(d->cold->snooplist);
	    d->cold->snooplist = temp;
	}
	d->cold->snooplist = NULL;
	d->cold->logged = 0;
        if ( !mudstate_hot.no_announce ) {
	   welcome_user(d);
        }
    } else {
	broadcast_monitor(NOTHING, MF_CONN | i_addflags, "PORT DISCONNECT", d->cold->userid, d->cold->longaddr, D_DESCRIPTOR(d), 0, 0, (char *)disc_reasons[reason]);
	shutdown(D_DESCRIPTOR(d), 2);
	close(D_DESCRIPTOR(d));
	freeqs(d,0); /* 0 is for all */
	/* free up the snooplist before freeing the desc -Thorin */
	while (d->cold->snooplist) {
	    temp = d->cold->snooplist->next;
	    if (d->cold->snooplist->sfile) {
		fprintf(d->cold->snooplist->sfile, "Snoop stopped due to shutdown of descriptor: (%ld) %s", mudstate_hot.now, ctime(&mudstate_hot.now));
		fclose(d->cold->snooplist->sfile);
	    }
	    free(d->cold->snooplist);
	    d->cold->snooplist = temp;
	}
	if (D_FLAGS(d) & DS_AUTH_IN_PROGRESS) {
	    shutdown(d->cold->authdescriptor, 2);
	    close(d->cold->authdescriptor);
	}
	{
	    int i = (int)(d - desc_slots);
	    int last = ndesc_slots - 1;
	    telnet_free_desc(d);
	    free_desc(d);
	    if (i != last && i < last) {
		#define SWAP_F(f) desc_hot.f[i] = desc_hot.f[last]
		SWAP_F(descriptor); SWAP_F(flags); SWAP_F(quota);
		SWAP_F(host_info); SWAP_F(player);
		SWAP_F(input_head); SWAP_F(input_tail);
		SWAP_F(output_head); SWAP_F(output_tail);
		SWAP_F(input_tot); SWAP_F(input_size);
		SWAP_F(output_tot); SWAP_F(output_size);
		SWAP_F(last_time);
		#undef SWAP_F
		DESC_COLD tmp_cold = desc_cold[i];
		desc_cold[i] = desc_cold[last];
		desc_cold[last] = tmp_cold;
		desc_slots[i].cold = &desc_cold[i];
		desc_slots[last].cold = &desc_cold[last];
		desc_slots[i].slot_index = i;
		desc_slots[last].slot_index = last;
		desc_hot.descriptor[last] = -1;
	    }
	}
	ndescriptors--;
	ndesc_slots--;
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
    if ( d->cold->addr_family == AF_INET6 ) {
       D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
       VOIDRETURN;
    }
    if ( D_FLAGS(d) & DS_API ) {
      D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    if( !mudconf.authenticate ) {
      D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    /* check the noauth_site list */
    if( d->cold->addr_family == AF_INET && site_check(d->cold->address.sin_addr, mudstate.special_list, 1, 0, H_NOAUTH) & H_NOAUTH ) {
      STARTLOG(LOG_NET, "NET", "AUTH")
        logbuff = alloc_lbuf("start_auth.LOG.site_check");
        sprintf(logbuff,
        "[%d/%s] Site configured as NOAUTH, not checking", D_DESCRIPTOR(d), d->cold->longaddr);
        log_text(logbuff);
        free_lbuf(logbuff);
      ENDLOG
      D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
      VOIDRETURN; /* #8 */
    }

    bzero((char *) &sin, sizeof(sin));

    if ((d->cold->authdescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	log_perror("NET", "FAIL", "creating auth sock", "socket");
	D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
	VOIDRETURN; /* #8 */
    }

    /* now make the socket non-blocking */
    make_nonblocking(d->cold->authdescriptor);

    sin.sin_port = htons(113);	/* auth/ident port */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = d->cold->address.sin_addr.s_addr;

    /* 1/2/97 - Thorin - Solaris has a problem with blocking on
       connect even when the socket is non-blocking, so instead we
       will only give it a maximum of three seconds to connect by
       doing fun stuff with preserving the mush timers and all. */

    STARTLOG(LOG_NET, "NET", "AUTH")
      logbuff = alloc_lbuf("start_auth.LOG.connecting");
      sprintf(logbuff,
      "[%d/%s] Starting AUTH connection", D_DESCRIPTOR(d), d->cold->longaddr);
      log_text(logbuff);
      free_lbuf(logbuff);
    ENDLOG

    alarm_msec(3);

    if( connect(d->cold->authdescriptor, (struct sockaddr *) &sin, sizeof(sin)) < 0){
        if( errno != EINPROGRESS ) {
          mudstate_hot.alarm_triggered = 2;
  	  D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
          logbuff = alloc_lbuf("start_auth.LOG.timeout");
          if( errno != EINTR ) {
	    log_perror("NET", "FAIL", "connecting AUTH sock", "connect");
          } else {
            STARTLOG(LOG_NET, "NET", "AUTH")
              sprintf(logbuff,
              "[%d/%s] AUTH connect alarm timed out", D_DESCRIPTOR(d), d->cold->longaddr);
              log_text(logbuff);
            ENDLOG
          }
           sprintf(logbuff, "%s %s", d->cold->addr, (d->cold->addr_family == AF_INET6) ? "/128" : "255.255.255.255");
          cf_site((int *)&mudstate.special_list, logbuff,
                    (H_NOAUTH | H_AUTOSITE), 0, 1, "noauth_site");
          free_lbuf(logbuff);
	  shutdown(d->cold->authdescriptor, 2);
	  close(d->cold->authdescriptor);
	  VOIDRETURN; /* #8 */
        }
    }

    D_FLAGS(d) |= DS_AUTH_CONNECTING;
    DPOP; /* #8 */
}

void
check_auth_connect(DESC * d)
{
    char* logbuff;
    int sockerr = 0;
    int sockerrlen = sizeof(sockerr);

    D_FLAGS(d) &= ~DS_AUTH_CONNECTING;

    if( getsockopt(d->cold->authdescriptor, SOL_SOCKET, SO_ERROR, &sockerr, (unsigned int *) &sockerrlen) ) {
        log_perror("NET", "FAIL", "checking AUTH connect", "getsockopt");
        D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
/* do we really need this next line? */
        close(d->cold->authdescriptor);
    }
    else {
        if( sockerr ) {
            STARTLOG(LOG_NET, "NET", "AUTH")
              logbuff = alloc_lbuf("check_auth_connect.LOG.timeout");
              sprintf(logbuff,
              "[%d/%s] AUTH connect: %s", D_DESCRIPTOR(d), d->cold->longaddr, strerror(sockerr));
              log_text(logbuff);
              free_lbuf(logbuff);
            ENDLOG
            D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
/* do we really need this next line? */
            close(d->cold->authdescriptor);
        }
        else {
            D_FLAGS(d) |= DS_NEED_AUTH_WRITE;
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

    sprintf(buff, "%d, %d\r\n", d->cold->remote_port, mudconf.port);

    if (write(d->cold->authdescriptor, buff, strlen(buff)) < 0) {
	free_mbuf(buff);
	if (errno == EINTR) {

	    VOIDRETURN; /* #9 */
	}
	if (errno != EINPROGRESS) {
            STARTLOG(LOG_NET, "NET", "AUTH")
      	      logbuff = alloc_lbuf("check_auth.LOG.writeerr");
    	      sprintf(logbuff,
	              "[%d/%s] AUTH write: %s", D_DESCRIPTOR(d), d->cold->longaddr,
	              strerror(errno));
	      log_text(logbuff);
	      free_lbuf(logbuff);
	    ENDLOG
	    D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->cold->authdescriptor, 2);
	    close(d->cold->authdescriptor);
	}
    }
    else {
        free_mbuf(buff);
    }
    D_FLAGS(d) &= ~DS_NEED_AUTH_WRITE;
    shutdown(d->cold->authdescriptor, 1);	/* close write */
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

    if ((numread = read(d->cold->authdescriptor, buff, MBUF_SIZE - 1)) < 0) {
        STARTLOG(LOG_NET, "NET", "AUTH")
  	  logbuff = alloc_lbuf("check_auth.LOG.readerr");
	  sprintf(logbuff,
	          "[%d/%s] AUTH read: %s", D_DESCRIPTOR(d), d->cold->longaddr,
	          strerror(errno));
	  log_text(logbuff);
	  free_lbuf(logbuff);
	ENDLOG
	D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
	shutdown(d->cold->authdescriptor, 2);
	close(d->cold->authdescriptor);
        free_mbuf(buff);
	VOIDRETURN; /* #10 */
    }
    buff[numread] = '\0';

    useridptr = d->cold->userid + strlen(d->cold->userid);
    safe_mb_str(buff, d->cold->userid, &useridptr);
    free_mbuf(buff);

    if (index(d->cold->userid, '\r') || index(d->cold->userid, '\n')) {
	/* we got it all */
	/* extract the user id if it's there */

        resp3 = alloc_mbuf("check_auth_resp3");
        buff2 = alloc_mbuf("check_auth_buff2");
        /* d->cold->userid is guaranteed to be within MBUF size,
           therefore a direct sscanf of any field within will be
           <= MBUF size */
	if (sscanf(d->cold->userid, "%d , %d : USERID : %s : %s", &resp1, &resp2, resp3, buff2) == 4) {
	    strncpy(d->cold->userid, strip_nonprint(buff2), MBUF_SIZE - 1);
	    D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->cold->authdescriptor, 2);
	    close(d->cold->authdescriptor);
            if ( strcmp(buff2, d->cold->userid) != 0 ) {
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
		    "[%d/%s] User identified: %s", D_DESCRIPTOR(d), d->cold->longaddr,
		    d->cold->userid);
	        log_text(logbuff);
	        free_lbuf(logbuff);
	    ENDLOG
	} else {		/* protocol error */
	    D_FLAGS(d) &= ~DS_AUTH_IN_PROGRESS;
	    shutdown(d->cold->authdescriptor, 2);
	    close(d->cold->authdescriptor);
	    STARTLOG(LOG_NET, "NET", "AUTH")
		logbuff = alloc_lbuf("check_auth.LOG.prot_err");
	        sprintf(logbuff,
		  "[%d/%s] Auth protocol error: %s", D_DESCRIPTOR(d), d->cold->longaddr,
		    d->cold->userid);
	        log_text(logbuff);
	        free_lbuf(logbuff);
	    ENDLOG
            strcpy(d->cold->userid,"");
	}
        free_mbuf(resp3);
        free_mbuf(buff2);
    } 
    DPOP; /* #10 */
}


DESC *
initializesock(int s, const char *ip_str, int addr_family, unsigned short remote_port, char *addr_str, int i_keyflag, int keyval)
{
    DESC *d, *dchk;
    char tchbuff[LBUF_SIZE];
    char *addroutbuf, *t_addroutbuf;
    int i_nope, i_sitecnt, i_guestcnt, i_sitemax, i_retvar = -1;
    struct in_addr ipv4_addr;
    int is_ipv4 = (addr_family == AF_INET);

    DPUSH; /* #11 */

    i_nope = 0;
    d = alloc_desc("init_sock");
    if (!d) {
        close(s);
        RETURN(NULL);
    }
    ndescriptors++;
    ndesc_slots++;
    D_DESCRIPTOR(d) = s;
    D_FLAGS(d) = 0;
    if ( !keyval ) {
       D_FLAGS(d) = DS_AUTH_IN_PROGRESS | DS_NEED_AUTH_WRITE;
    }
    d->cold->connected_at = mudstate_hot.now;
    d->cold->retries_left = mudconf.retry_limit;
    d->cold->regtries_left = mudconf.regtry_limit;
    d->cold->command_count = 0;
    d->cold->timeout = mudconf.idle_timeout;
    d->cold->addr_family = addr_family;
    d->cold->remote_port = remote_port;
    if (is_ipv4 && inet_pton(AF_INET, ip_str, &ipv4_addr) == 1) {
       d->cold->address.sin_family = AF_INET;
       d->cold->address.sin_addr = ipv4_addr;
       d->cold->address.sin_port = htons(remote_port);
    } else {
       memset(&d->cold->address, 0, sizeof(d->cold->address));
    }
    addroutbuf = (char *) addrout(ip_str, addr_family, keyval);
    t_addroutbuf = alloc_lbuf("check_max_sitecons");
    strcpy(t_addroutbuf, addroutbuf);
    i_sitecnt = i_guestcnt = 0;
    if ( t_addroutbuf ) {
       DESC_SAFEITER_ALL(dchk) {
          if ( strcmp(t_addroutbuf, dchk->cold->addr) == 0 ) {
             if ( Good_chk(D_PLAYER(dchk)) && Guest(D_PLAYER(dchk)) ) {
                i_guestcnt++;
             }
             i_sitecnt++;
          }
       }
    }
    free_lbuf(t_addroutbuf);
     if ( is_ipv4 ) {
        D_HOST_INFO(d) = (site_check(ipv4_addr, mudstate.access_list, 1, 0, 0) & ~0x4) |
                       site_check(ipv4_addr, mudstate.suspect_list, 0, 0, 0);
        i_sitemax = site_check(ipv4_addr, mudstate.access_list, 1, 1, H_FORBIDDEN);
        if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_FORBIDDEN;
        i_sitemax = site_check(ipv4_addr, mudstate.access_list, 1, 1, H_REGISTRATION);
        if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_REGISTRATION;
        i_sitemax = site_check(ipv4_addr, mudstate.access_list, 1, 1, H_NOGUEST);
        if ( (i_sitemax != -1) && (i_guestcnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_NOGUEST;
     } else {
        D_HOST_INFO(d) = (site_check_str(ip_str, addr_family, mudstate.access_list, 1, 0, 0) & ~0x4) |
                       site_check_str(ip_str, addr_family, mudstate.suspect_list, 0, 0, 0);
        i_sitemax = site_check_str(ip_str, addr_family, mudstate.access_list, 1, 1, H_FORBIDDEN);
        if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_FORBIDDEN;
        i_sitemax = site_check_str(ip_str, addr_family, mudstate.access_list, 1, 1, H_REGISTRATION);
        if ( (i_sitemax != -1) && (i_sitecnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_REGISTRATION;
        i_sitemax = site_check_str(ip_str, addr_family, mudstate.access_list, 1, 1, H_NOGUEST);
        if ( (i_sitemax != -1) && (i_guestcnt < i_sitemax) )
           D_HOST_INFO(d) &= ~H_NOGUEST;
     }
    memset(tchbuff, 0, sizeof(tchbuff));
    strcpy(tchbuff, mudconf.forbid_host);
    if (mudconf.forbid_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_FORBIDDEN;
    strcpy(tchbuff, mudconf.register_host);
    if (mudconf.register_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_REGISTRATION;
     if ( blacklist_check_str(ip_str, addr_family, 2) ) {
        D_HOST_INFO(d) = D_HOST_INFO(d) | H_REGISTRATION;
     }
     strcpy(tchbuff, mudconf.autoreg_host);
     if (mudconf.autoreg_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar))
        D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOAUTOREG;
     strcpy(tchbuff, mudconf.noguest_host);
     if (mudconf.noguest_host[0] && lookup(addr_str, tchbuff, i_guestcnt, &i_retvar))
        D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOGUEST;
     if ( blacklist_check_str(ip_str, addr_family, 3) ) {
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_NOGUEST;
    }
    strcpy(tchbuff, mudconf.suspect_host);
    if (mudconf.suspect_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_SUSPECT;
    strcpy(tchbuff, mudconf.passproxy_host);
    if (mudconf.passproxy_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_PASSPROXY;
    strcpy(tchbuff, mudconf.hardconn_host);
    if (mudconf.hardconn_host[0] && lookup(addr_str, tchbuff, i_guestcnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_HARDCONN;
    strcpy(tchbuff, mudconf.permit_host);
    if (mudconf.permit_host[0] && lookup(addr_str, tchbuff, i_guestcnt, &i_retvar))
       D_HOST_INFO(d) = D_HOST_INFO(d) | H_PERMIT;
    D_HOST_INFO(d) = D_HOST_INFO(d) | i_keyflag;
    D_PLAYER(d) = 0;
    d->cold->addr[0] = '\0';
    d->cold->longaddr[0] = '\0';
    d->cold->longaddrcheck = 242242242;
    d->cold->doing[0] = '\0';
    make_nonblocking(s);
    d->cold->output_prefix = NULL;
    d->cold->output_suffix = NULL;
    D_OUTPUT_SIZE(d) = 0;
    D_OUTPUT_TOT(d) = 0;
    d->cold->output_lost = 0;
    D_OUTPUT_HEAD(d) = NULL;
    D_OUTPUT_TAIL(d) = NULL;
    d->cold->door_output_head = NULL;
    d->cold->door_output_tail = NULL;
    d->cold->door_input_head = NULL;
    d->cold->door_input_tail = NULL;
    d->cold->door_desc = -1;
    d->cold->door_num = 0;
    d->cold->door_output_size = 0;
    d->cold->door_int1 = 0;
    d->cold->door_int2 = 0;
    d->cold->door_int3 = 0;
    d->cold->door_lbuf = NULL;
    d->cold->door_mbuf = NULL;
    d->cold->door_raw = NULL;
    D_INPUT_HEAD(d) = NULL;
    D_INPUT_TAIL(d) = NULL;
    D_INPUT_SIZE(d) = 0;
    D_INPUT_TOT(d) = 0;
    d->cold->input_lost = 0;
    d->cold->raw_input = NULL;
    d->cold->raw_input_at = NULL;
    D_QUOTA(d) = mudconf.cmd_quota_max;
    D_LAST_TIME(d) = 0;
    d->cold->snooplist = NULL;
    d->cold->logged = 0;
    d->cold->checksum[0] = '\0';
    d->cold->ws_frame_len = 0;
    d->cold->account_owner = NOTHING;
    memset(d->cold->account_rawpass, '\0', sizeof(d->cold->account_rawpass));

    *d->cold->userid = '\0';
    memset(d->cold->addr, 0, sizeof(d->cold->addr));
    strncpy(d->cold->addr, ip_str, sizeof(d->cold->addr) - 1);
    memset(d->cold->longaddr, '\0', sizeof(d->cold->longaddr));
    strncpy(d->cold->longaddr, addr_str, sizeof(d->cold->longaddr) - 1);
    if ( !keyval && mudconf.sconnect_reip && mudconf.ssl_welcome )  {
       memset(tchbuff, 0, sizeof(tchbuff));
       strcpy(tchbuff, mudconf.sconnect_host);
        if (mudconf.sconnect_host[0] && lookup(addr_str, tchbuff, i_sitecnt, &i_retvar)) {
          i_nope = 1;
       }
    }
    /* telnet_init_desc is deferred to process_input() to avoid sending
     * IAC DO NAWS to WebSocket connections before the WS upgrade. */
    if ( !i_nope && !keyval ) {
       welcome_user(d);
       start_auth(d);
    } else if ( i_nope && !keyval ) {
       start_auth(d);
    } else if ( keyval ) {
        d->cold->timeout = 1;
        D_FLAGS(d) |= DS_API;
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

    cmdsave = mudstate_hot.debug_cmd;
    mudstate_hot.debug_cmd = (char *) "< process_output >";

    tb = D_OUTPUT_HEAD(d);
    retry_cnt = retry_success = 0;
    while (tb != NULL) {
	while (tb->hdr.nchars > 0) {

	    cnt = WRITE(D_DESCRIPTOR(d), tb->hdr.start,
			tb->hdr.nchars);
	    if (cnt < 0) {
		mudstate_hot.debug_cmd = cmdsave;
                if ( (errno == EWOULDBLOCK) || (errno == EAGAIN) ) {
                   while ( retry_cnt < 10 ) {
	              cnt = WRITE(D_DESCRIPTOR(d), tb->hdr.start, tb->hdr.nchars);
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
	              cnt = WRITE(D_DESCRIPTOR(d), tb->hdr.start, tb->hdr.nchars);
                   }
                   if ( cnt < 0 ) {
                      if ( (mudconf.log_network_errors > 0) && (mudstate.last_network_owner != D_PLAYER(d)) ) {
                         STARTLOG(LOG_ALWAYS, "WIZ", "ERR")
                            memset(s_savebuff, '\0', sizeof(s_savebuff));
                            log_text((char *) "WARNING: Failed socket write to descriptor past 10 times ");
                            sprintf(s_savebuff, "[port %d/player #%d/error %d] ", D_DESCRIPTOR(d), D_PLAYER(d),
                                    errno);
                            log_text(s_savebuff);
                            log_text((char *)strerror(errno));
                         ENDLOG
                         mudstate.last_network_owner = D_PLAYER(d);
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
	    D_OUTPUT_SIZE(d) -= cnt;
	    tb->hdr.nchars -= cnt;
	    tb->hdr.start += cnt;
	}
	save = tb;
	tb = tb->hdr.nxt;
	free_lbuf(save);
	D_OUTPUT_HEAD(d) = tb;
	if (tb == NULL)
	    D_OUTPUT_TAIL(d) = NULL;
    }
    mudstate_hot.debug_cmd = cmdsave;
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
    cmdsave = mudstate_hot.debug_cmd;
    mudstate_hot.debug_cmd = (char *) "< process_input >";

    memset(qfind, '\0', sizeof(qfind));
    memset(tmpbuf, '\0', sizeof(tmpbuf));
    got = in = READ(D_DESCRIPTOR(d), buf, sizeof buf);
    if (got <= 0) {
	mudstate_hot.debug_cmd = cmdsave;
	RETURN(0); /* #16 */
    }
#ifdef ENABLE_WEBSOCKETS
///// NEW WEBSOCK #ifdef ENABLE_WEBSOCKETS
    if (D_FLAGS(d) & DS_WEBSOCKETS) {
        /* Process using WebSockets framing. */
        got2 = got;
        got = in = process_websocket_frame(d, buf, got2);
        if (d->cold->ws_closing) {
            mudstate_hot.debug_cmd = cmdsave;
            RETURN(1);
        }
    }
///// END NEW WEBSOCK #endif
#endif
    /* First-time init for non-websocket, non-API connections.
     * Deferred from initializesock() so we don't send DO NAWS to WS clients. */
    if (!d->cold->telnet && !(D_FLAGS(d) & (DS_WEBSOCKETS | DS_API))) {
        telnet_init_desc(d);
    }
    /* Strip telnet negotiation if initialized */
    if (d->cold->telnet) {
        telnet_preprocess_input(d, buf, &got);
    }
    if (!d->cold->raw_input) {
	d->cold->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
	d->cold->raw_input_at = d->cold->raw_input->cmd;
    }
    p = d->cold->raw_input_at;
    pend = d->cold->raw_input->cmd + LBUF_SIZE - sizeof(CBLKHDR) - 1;
    lost = 0;
    in_get = 1;
    if ( D_FLAGS(d) & DS_API ) {
       in_get = 0;
    }
//fprintf(stderr, "Test: %s\nVal: %d", buf, in_get);
    for (q = buf, qend = buf + got; q < qend; q++) {
	if ( (*q == '\n') && (!(D_FLAGS(d) & DS_API) || (!in_get && ((q+10) > qend) && (D_FLAGS(d) & DS_API))) ) {
  	      *p = '\0';
		if (p > d->cold->raw_input->cmd) {
			save_command(d, d->cold->raw_input);
			d->cold->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
			p = d->cold->raw_input_at = d->cold->raw_input->cmd;
			pend = d->cold->raw_input->cmd + LBUF_SIZE -
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
	    if (p > d->cold->raw_input->cmd)
		p--;
	    if (p < d->cold->raw_input_at)
		(d->cold->raw_input_at)--;
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
    if (p > d->cold->raw_input->cmd) {
	d->cold->raw_input_at = p;
    } else {
	free_lbuf(d->cold->raw_input);
	d->cold->raw_input = NULL;
	d->cold->raw_input_at = NULL;
    }
    if ( got > 0 ) {
       D_INPUT_TOT(d) += got;
       mudstate.total_bytesin += got;
       if ( (mudstate.reset_daily_bytes + 86400) < mudstate_hot.now ) {
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
       D_INPUT_SIZE(d) += in;
    if ( lost > 0 )
       d->cold->input_lost += lost;
    
    mudstate_hot.debug_cmd = cmdsave;
    RETURN(1); /* #16 */
}

void 
close_sockets(int emergency, char *message)
{
    DESC *d;

    DPUSH; /* #17 */
    do_rwho(NOTHING, NOTHING, RWHO_STOP);
    DESC_SAFEITER_ALL(d) {
	if (emergency) {

	    WRITE(D_DESCRIPTOR(d), message, strlen(message));
	    if (shutdown(D_DESCRIPTOR(d), 2) < 0)
		log_perror("NET", "FAIL", NULL, "shutdown");
	    close(D_DESCRIPTOR(d));
	    if (D_FLAGS(d) & DS_HAS_DOOR)
	      close(d->cold->door_desc);
	} else {
	    queue_string(d, message);
	    queue_write(d, "\r\n", 2);
	    if (!(mudstate_hot.reboot_flag))
	      shutdownsock(d, R_GOING_DOWN);
	    else
	      shutdownsock(d, R_REBOOT);
	}
    }
    if ( sock >= 0 ) close(sock);
    if ( sock2 >= 0 ) close(sock2);
    if ( sock6 >= 0 ) close(sock6);
    if ( sock6_api >= 0 ) close(sock6_api);
    DPOP; /* #17 */
}
  
void close_main_socket( void )
{
    if ( sock >= 0 ) close(sock);
    if ( sock2 >= 0 ) close(sock2);
    if ( sock6 >= 0 ) close(sock6);
    if ( sock6_api >= 0 ) close(sock6_api);
}

const char *get_listening_info(void)
{
    if (sock >= 0 && sock6 >= 0)
        return "IPv4 + IPv6 (dual-stack)";
    else if (sock >= 0)
        return "IPv4 only";
    else if (sock6 >= 0)
        return "IPv6 only";
    else
        return "None";
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

#ifdef HAVE_STRUCT_SIGCONTEXT
static RETSIGTYPE sighandler(int sig, int code, struct sigcontext *scp);
#else
static RETSIGTYPE sighandler(int sig);
#endif

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
        if ( mudstate_hot.reboot_flag || (signal_depth < 1) )
           return;

	signal_depth = signal_depth - 1; /* Reduce 'signal unset' depth. We only want
									 									* to call set_signals() if it is 0.
									 									*/
	if(signal_depth == 0) 
		set_signals();
        /* If signals were ignored, the alarm_msec() will have been as well */
        mudstate_hot.alarm_triggered = 0;
        alarm_msec (next_timer());
}

static void 
check_panicking(int sig)
{
    int i;

    DPUSH; /* #21 */

    /* If we are panicking, turn off signal catching and resignal */

    if (mudstate_hot.panicking) {
	for (i = 0; i < NSIG; i++)
	    signal(i, SIG_DFL);
	kill(getpid(), sig);
    }
    mudstate_hot.panicking = 1;
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
        /* This will allow dispatch() to update the queue */
	mudstate_hot.alarm_triggered = 1;
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
                                         mudstate_hot.global_regs, mudstate_hot.global_regsname);
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
          mudstate_hot.reboot_flag = 1;
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
              mudstate_hot.shutdown_flag = 1;
           }
        ENDLOG
        if ( !mudstate_hot.shutdown_flag ) {
           mudstate.forceusr2 = 0;
           do_shutdown(NOTHING, NOTHING, 0, buff);
           mudstate_hot.shutdown_flag = 1;
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
    mudstate_hot.panicking = 0;
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
