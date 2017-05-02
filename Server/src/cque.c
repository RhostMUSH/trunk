/* cque.c -- commands and functions for manipulating the command queue */

#include "copyright.h"
#include "autoconf.h"

#include <signal.h>

#include <math.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "htab.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "command.h"
#include "alloc.h"

extern int FDECL(a_Queue, (dbref, int));
extern void FDECL(s_Queue, (dbref, int));
extern int FDECL(QueueMax, (dbref));
extern double NDECL(next_timer);
extern double FDECL(time_ng, (double*));
extern int FDECL(alarm_msec, (double));

#define MUMAXPID	32767
#ifdef MAXINT
#define MYMAXINT	MAXINT
#else
#define MYMAXINT	2147483647
#endif
static char pid_table[MUMAXPID+1];
static int last_pid;
static int kick_pid;

#define FUNC_KICK	0

static int
countwordsnew(char *str)
{
    int n;

/* Trim starting white space */
    n = 0;
    while (*str && *str == ' ')
       str++;
/* Count words */
    while ( *str ) {
       while (*str && *str != ' ')
          str++;
       n++;
       while (*str && *str == ' ')
          str++;
    }
    return n;
}

void init_pid_table()
{
  int x;

  for (x = 0; x <= MUMAXPID; x++)
    pid_table[x] = '\0';
  last_pid = MUMAXPID;
  kick_pid = -1;
}

void execute_entry(BQUE *queue)
{
  dbref player;
  int i;
  char *command, *cp;

	player = queue->player;
	if ((player >= 0) && !Going(player) && !Recover(player)) {
	    if (!FreeFlag(Owner(player)))
	      giveto(player, mudconf.waitcost, NOTHING);
	    mudstate.curr_enactor = queue->cause;
	    mudstate.curr_player = player;
	    a_Queue(Owner(player), -1);
	    queue->player = 0;
	    pid_table[queue->pid] = 0;
	    if (!Halted(player) || mudstate.force_halt) {

		/* Load scratch args */

		for (i = 0; i < MAX_GLOBAL_REGS; i++) {
		    if (queue->scr[i]) {
			strcpy(mudstate.global_regs[i],
			       queue->scr[i]);
		    } else {
			*mudstate.global_regs[i] = '\0';
		    }
                    if ( queue->scrname[i]) {
			strcpy(mudstate.global_regsname[i],
			       queue->scrname[i]);
		    } else {
			*mudstate.global_regsname[i] = '\0';
		    }
		}

		command = queue->comm;
		mudstate.breakst = 0;
		mudstate.jumpst = 0;
		mudstate.gotolabel = 0;
		mudstate.rollbackcnt = 0;
                mudstate.breakdolist = 0;
                mudstate.includecnt = 0;
                mudstate.includenest = 0;
                mudstate.force_halt =0;
                if ( command ) {
		   strncpy(mudstate.rollback, command, LBUF_SIZE);
                }
		while (command && !mudstate.breakst) {
		    cp = parse_to(&command, ';', 0);
		    if (cp && *cp) {
			desc_in_use = NULL;
			process_command(player,
					queue->cause,
					0, cp,
					queue->env,
					queue->nargs, queue->shellprg, queue->hooked_command);
		    }
		}
                mudstate.shell_program = 0;
	    }
	}
	queue->player = NOTHING;
}

/* ---------------------------------------------------------------------------
 * add_to: Adjust an object's queue or semaphore count.
 */

static int 
add_to(dbref player, int am, int attrnum)
{
    int num, aflags;
    dbref aowner;
    char buff[20];
    char *atr_gotten;

    num = atoi(atr_gotten = atr_get(player, attrnum, &aowner, &aflags));
    free_lbuf(atr_gotten);
    num += am;
    if (num)
	sprintf(buff, "%d", num);
    else
	*buff = '\0';
    atr_add_raw(player, attrnum, buff);
    return (num);
}

/* ---------------------------------------------------------------------------
 * give_que: Thread a queue block onto the high or low priority queue
 */

static void 
give_que(BQUE * tmp)
{
    tmp->next = NULL;
    tmp->waittime = 0;

    /* Thread the command into the correct queue */

    if (Typeof(tmp->cause) == TYPE_PLAYER) {
	if (mudstate.qlast != NULL) {
	    mudstate.qlast->next = tmp;
	    mudstate.qlast = tmp;
	} else
	    mudstate.qlast = mudstate.qfirst = tmp;
    } else {
	if (mudstate.qllast) {
	    mudstate.qllast->next = tmp;
	    mudstate.qllast = tmp;
	} else
	    mudstate.qllast = mudstate.qlfirst = tmp;
    }
}

/* ---------------------------------------------------------------------------
 * que_want: Do we want this queue entry?
 */

static int 
que_want(BQUE * entry, dbref ptarg, dbref otarg)
{
    if ((ptarg != NOTHING) && (ptarg != Owner(entry->player)))
	return 0;
    if ((otarg != NOTHING) && (otarg != entry->player))
	return 0;
    return 1;
}

/* ---------------------------------------------------------------------------
 * halt_que: Remove all queued commands from a certain player
 */

int 
halt_que(dbref player, dbref object)
{
    BQUE *trail, *point, *next;
    int numhalted;

    numhalted = 0;

    /* Player queue */

    for (point = mudstate.qfirst; point; point = point->next)
	if (que_want(point, player, object)) {
	    numhalted++;
	    point->player = NOTHING;
	    pid_table[point->pid] = 0;
	}
    /* Object queue */

    for (point = mudstate.qlfirst; point; point = point->next)
	if (que_want(point, player, object)) {
	    numhalted++;
	    point->player = NOTHING;
	    pid_table[point->pid] = 0;
	}
    /* Wait queue */

    for (point = mudstate.qwait, trail = NULL; point; point = next)
	if (que_want(point, player, object)) {
	    numhalted++;
	    pid_table[point->pid] = 0;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	} else
	    next = (trail = point)->next;

    /* Semaphore queue */

    for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if ( (point->pid != kick_pid) && que_want(point, player, object)) {
	    pid_table[point->pid] = 0;
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    add_to(point->sem, -1, A_SEMAPHORE);
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	} else
	    next = (trail = point)->next;
    }

    if (player == NOTHING)
	player = Owner(object);
    if (!FreeFlag(Owner(player)))
      giveto(player, (mudconf.waitcost * numhalted), NOTHING);
    if (object == NOTHING)
	s_Queue(player, 0);
    else
	a_Queue(player, -numhalted);
    return numhalted;
}

int 
ind_pid_func(dbref player, int pid, int func)
{
    BQUE *trail, *point, *next;
    int numhalted;
    int found;

    numhalted = 0;
    found = 0;

    /* Player queue */

    for (point = mudstate.qfirst; point; point = point->next) {
      if (point->pid == pid) {
	if (Immortal(player) || Controls(player,Owner(point->player))) {  
	    if (func == FUNC_KICK)
	      execute_entry(point);
	    numhalted++;
	    found = 1;
	    break;
	}
	else {
	  found = -1;
	  break;
	}
      }
    }
    /* Object queue */

    if (!found) {
      for (point = mudstate.qlfirst; point; point = point->next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player))) { 
	    if (func == FUNC_KICK)
	      execute_entry(point);
	    numhalted++;
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	}
      }
    }
    /* Wait queue */

    if (!found) {
      for (point = mudstate.qwait, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player))) { 
	    if (func == FUNC_KICK)
	      execute_entry(point);
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	} else
	    next = (trail = point)->next;
      }
    }

    /* Semaphore queue */

    if (!found) {
      for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player))) { 
	    if (func == FUNC_KICK) {
              kick_pid = point->pid;
	      execute_entry(point);
              kick_pid = -1;
            }
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    add_to(point->sem, -1, A_SEMAPHORE);
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else
	    break;
	} else
	    next = (trail = point)->next;
      }
    }
    return numhalted;
}

int 
freeze_pid(dbref player, int pid, int key)
{
    BQUE *trail, *point, *next, *freezepid, *pntr;
    int numhalted, found, a;
    dbref psave;
    
    numhalted = 0;
    found = 0;
    psave = NOTHING;
    freezepid = NULL;

    /* Wait queue */

    if (!found) {
      for (point = mudstate.qwait, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            freezepid = alloc_qentry("freeze_pid.qblock");
            if (point->text) {
               freezepid->text = (char *) malloc(point->text_len); 
               freezepid->text_len = point->text_len;
               memcpy((char *)freezepid->text, (char *)point->text, point->text_len);
            } else {
               freezepid->text = NULL;
            }
            if (point->comm) {
/*             freezepid->comm = (char *) malloc(strlen(point->comm) + 1);
               strcpy((char *)freezepid->comm, (char *)point->comm); */
               freezepid->comm = freezepid->text + (point->comm - point->text);
            } else {
               freezepid->comm = NULL;
            }
            freezepid->waittime = point->waittime;

            for (a = 0; a < NUM_ENV_VARS; a++) {
                if ( point->env[a] )
                   freezepid->env[a] = freezepid->text + (point->env[a] - point->text);
                else
                   freezepid->env[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scr[a] )
                   freezepid->scr[a] = freezepid->text + (point->scr[a] - point->text);
                else
                   freezepid->scr[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scrname[a] )
                   freezepid->scrname[a] = freezepid->text + (point->scrname[a] - point->text);
                else
                   freezepid->scrname[a] = NULL;
            }
            freezepid->player = point->player;
            freezepid->cause = point->cause;
            freezepid->sem = point->sem;
            freezepid->nargs = point->nargs;
            freezepid->pid = time(NULL);
            freezepid->shellprg = point->shellprg;
            freezepid->stop_bool = point->stop_bool;
            freezepid->stop_bool_val = point->stop_bool_val;
            freezepid->hooked_command = point->hooked_command;
            if ( Good_obj(point->player) )
               freezepid->plr_type = Typeof(point->player);
            else
               freezepid->plr_type = -1;
	    numhalted++;
	    pid_table[point->pid] = 0;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	} else
	    next = (trail = point)->next;
      }
    }

    /* Semaphore queue */

    if (!found) {
      for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if ( (point->pid == pid) && (point->pid != kick_pid) ) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            freezepid = alloc_qentry("freeze_pid.qblock");
            if (point->text) {
               freezepid->text = (char *) malloc(point->text_len);
               freezepid->text_len = point->text_len;
               memcpy((char *)freezepid->text, (char *)point->text, point->text_len);
            } else {
               freezepid->text = NULL;
            }
            if (point->comm) {
/*             freezepid->comm = (char *) malloc(strlen(point->comm) + 1);
               strcpy((char *)freezepid->comm, (char *)point->comm); */
               freezepid->comm = freezepid->text + (point->comm - point->text);
            } else {
               freezepid->comm = NULL;
            }
            freezepid->waittime = point->waittime;
            for (a = 0; a < NUM_ENV_VARS; a++) {
                if ( point->env[a] )
                   freezepid->env[a] = freezepid->text + (point->env[a] - point->text);
                else
                   freezepid->env[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scr[a] )
                   freezepid->scr[a] = freezepid->text + (point->scr[a] - point->text);
                else
                   freezepid->scr[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scrname[a] )
                   freezepid->scrname[a] = freezepid->text + (point->scrname[a] - point->text);
                else
                   freezepid->scrname[a] = NULL;
            }
            freezepid->player = point->player;
            freezepid->cause = point->cause;
            freezepid->sem = point->sem;
            freezepid->nargs = point->nargs;
            freezepid->pid = time(NULL);
            freezepid->shellprg = freezepid->shellprg;
            freezepid->stop_bool = point->stop_bool;
            freezepid->stop_bool_val = point->stop_bool_val;
            freezepid->hooked_command = point->hooked_command;
            if ( Good_obj(point->player) )
               freezepid->plr_type = Typeof(point->player);
            else
               freezepid->plr_type = -1;
	    pid_table[point->pid] = 0;
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    add_to(point->sem, -1, A_SEMAPHORE);
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else
	    break;
	} else
	    next = (trail = point)->next;
      }
    }

    if ( found == 1 ) {
       if (psave != NOTHING) {
         if (!FreeFlag(Owner(psave)))
           giveto(psave, (mudconf.waitcost * numhalted), NOTHING);
         a_Queue(psave, -numhalted);
       }
       if (freezepid->sem == NOTHING) {
           /* No semaphore, put on wait queue if wait value specified.
            *          * Otherwise put on the normal queue. */
           for (pntr = mudstate.fqwait, trail = NULL;
               pntr && pntr->waittime <= freezepid->waittime;
               pntr = pntr->next) {
               trail = pntr;
           }
           freezepid->next = pntr;
           if (trail != NULL)
               trail->next = freezepid;
           else
               mudstate.fqwait = freezepid;
       } else {
           freezepid->next = NULL;
           if (mudstate.fqsemlast != NULL)
               mudstate.fqsemlast->next = freezepid;
           else
               mudstate.fqsemfirst = freezepid;
           mudstate.fqsemlast = freezepid;
       }
    }
    return numhalted;
}

int 
thaw_pid(dbref player, int pid, int key)
{
    BQUE *trail, *point, *next, *freezepid, *pntr;
    int numhalted, a;
    int found;
    int tpid, badpid;
    dbref psave;
    
    numhalted = 0;
    found = 0;
    badpid = 0;
    psave = NOTHING;

    /* Wait queue */

    if (!found) {
      for (point = mudstate.fqwait, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            if ( (!Good_obj(point->player) || Going(point->player) || Recover(point->player)) ||
                 (!Good_obj(psave) || Going(psave) || Recover(psave)) ||
                 (Typeof(point->player) != point->plr_type) ) {
               badpid = 1;
            } else if ( !(key & THAW_DEL) ) {
               freezepid = alloc_qentry("thaw_pid.qblock");
               if (point->text) {
                  freezepid->text = (char *) malloc(point->text_len);
                  freezepid->text_len = point->text_len;
                  memcpy((char *)freezepid->text, (char *)point->text, point->text_len);
               } else {
                  freezepid->text = NULL;
               }
               if (point->comm) {
/*                freezepid->comm = (char *) malloc(strlen(point->comm) + 1);
                  strcpy((char *)freezepid->comm, (char *)point->comm); */
                  freezepid->comm = freezepid->text + (point->comm - point->text);
               } else {
                  freezepid->comm = NULL;
               }
               freezepid->waittime = point->waittime - point->pid + time_ng(NULL);
               for (a = 0; a < NUM_ENV_VARS; a++) {
                   if ( point->env[a] )
                      freezepid->env[a] = freezepid->text + (point->env[a] - point->text);
                   else
                      freezepid->env[a] = NULL;
               }
               for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                   if ( point->scr[a] )
                      freezepid->scr[a] = freezepid->text + (point->scr[a] - point->text);
                   else
                      freezepid->scr[a] = NULL;
               }
               for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                   if ( point->scrname[a] )
                      freezepid->scrname[a] = freezepid->text + (point->scrname[a] - point->text);
                   else
                      freezepid->scrname[a] = NULL;
               }
               freezepid->player = point->player;
               freezepid->cause = point->cause;
               freezepid->sem = point->sem;
               freezepid->nargs = point->nargs;
               freezepid->shellprg = point->shellprg;
               freezepid->stop_bool = point->stop_bool;
               freezepid->stop_bool_val = point->stop_bool_val;
               freezepid->hooked_command = point->hooked_command;
            }
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.fqwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	} else
	    next = (trail = point)->next;
      }
    }

    /* Semaphore queue */

    if (!found) {
      for (point = mudstate.fqsemfirst, trail = NULL; point; point = next) {
	if ( (point->pid == pid) && (point->pid != kick_pid) ) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            if ( (!Good_obj(point->player) || Going(point->player) || Recover(point->player)) ||
                 (!Good_obj(psave) || Going(psave) || Recover(psave)) ||
                 (Typeof(point->player) != point->plr_type) ) {
               badpid = 1;
            } else if ( !(key & THAW_DEL) ) {
               freezepid = alloc_qentry("thaw_pid.qblock");
               if (point->text) {
                  freezepid->text = (char *) malloc(point->text_len);
                  freezepid->text_len = point->text_len;
                  memcpy((char *)freezepid->text, (char *)point->text, point->text_len);
               } else {
                  freezepid->text = NULL;
               }
               if (point->comm) {
/*                freezepid->comm = (char *) malloc(strlen(point->comm) + 1);
                  strcpy((char *)freezepid->comm, (char *)point->comm); */
                  freezepid->comm = freezepid->text + (point->comm - point->text);
               } else {
                  freezepid->comm = NULL;
               }
               if ( point->waittime  != 0 )
                  freezepid->waittime = point->waittime - point->pid + time_ng(NULL);
               else
                  freezepid->waittime = 0;
               for (a = 0; a < NUM_ENV_VARS; a++) {
                   if ( point->env[a] )
                      freezepid->env[a] = freezepid->text + (point->env[a] - point->text);
                   else
                      freezepid->env[a] = NULL;
               }
               for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                   if ( point->scr[a] )
                      freezepid->scr[a] = freezepid->text + (point->scr[a] - point->text);
                   else
                      freezepid->scr[a] = NULL;
               }
               for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                   if ( point->scrname[a] )
                      freezepid->scrname[a] = freezepid->text + (point->scrname[a] - point->text);
                   else
                      freezepid->scrname[a] = NULL;
               }
               freezepid->player = point->player;
               freezepid->cause = point->cause;
               freezepid->sem = point->sem;
               freezepid->pid = point->pid;
               freezepid->nargs = point->nargs;
               freezepid->shellprg = point->shellprg;
               freezepid->stop_bool = point->stop_bool;
               freezepid->stop_bool_val = point->stop_bool_val;
               freezepid->hooked_command = point->hooked_command;
            }
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.fqsemfirst = next = point->next;
	    if (point == mudstate.fqsemlast)
		mudstate.fqsemlast = trail;
	    add_to(point->sem, 1, A_SEMAPHORE);
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else
	    break;
	} else
	    next = (trail = point)->next;
      }
    }

    if (last_pid < MUMAXPID) {
      for (tpid = last_pid + 1; tpid <= MUMAXPID; tpid++) {
	if (!pid_table[tpid])
	  break;
      }
      if (tpid > MUMAXPID) {
	for (tpid = 0; tpid <= last_pid; tpid++) {
	  if (!pid_table[tpid])
	    break;
	}
	if (tpid > last_pid) {
	  notify(Owner(player), "Unable to thaw PID.  Queue is full.  Frozen PID deleted.");
          tpid = -1;
	}
      }
    } else {
      for (tpid = 0; tpid <= MUMAXPID; tpid++) {
        if (!pid_table[tpid])
          break;
      }
      if (tpid > MUMAXPID) {
	  notify(Owner(player), "Unable to thaw PID.  Queue is full.  Frozen PID deleted.");
          tpid = -1;
      }
    }

    if ( found == 1 && tpid != -1 && !(key & THAW_DEL) && !badpid ) {
       if (psave != NOTHING) {
         if (!FreeFlag(Owner(psave)))
           giveto(psave, -(mudconf.waitcost * numhalted), NOTHING);
         a_Queue(psave, +numhalted);
       }
       pid_table[tpid] = 1;
       last_pid = tpid;    
       freezepid->pid = tpid;
       if (freezepid->sem == NOTHING) {
           /* No semaphore, put on wait queue if wait value specified.
            *          * Otherwise put on the normal queue. */
           for (pntr = mudstate.qwait, trail = NULL;
               pntr && pntr->waittime <= freezepid->waittime;
               pntr = pntr->next) {
               trail = pntr;
           }
           freezepid->next = pntr;
           if (trail != NULL)
               trail->next = freezepid;
           else
               mudstate.qwait = freezepid;
       } else {
           freezepid->next = NULL;
           if (mudstate.qsemlast != NULL)
               mudstate.qsemlast->next = freezepid;
           else
               mudstate.qsemfirst = freezepid;
           mudstate.qsemlast = freezepid; 
       }
    } else if ( tpid == -1 && (!(key & THAW_DEL) || badpid)) {
	    if (freezepid->text)
		free(freezepid->text);
	    free_qentry(freezepid);
    } else if ( key & THAW_DEL ) {
       numhalted = -1;
    } else if ( badpid ) {
       numhalted = -2;
    }
    return numhalted;
}

int
wait_que_pidnew(dbref player, int pid, double newwait, int key)
{
   BQUE *point;
   int found;

   found = 0;
   for (point = mudstate.qwait; point; point = point->next) {
      if (point->pid == pid) {
         if (Immortal(player) || Controls(player,Owner(point->player)) ||
             HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
            point->stop_bool_val = newwait;
            if ( key == -1 ) {
               if ( (point->waittime - newwait) < (time_ng(NULL) + 10) ) 
                  return -1;
               point->waittime -= newwait;
               notify(player, unsafe_tprintf("PID %d has been re-waited with new time of %.1f",
                              pid, (point->waittime - time_ng(NULL))));
               found = 2;
            } else if ( key == 1 ) {
               if ( (point->waittime + newwait) < (time(NULL) + 10) ) 
                  return -1;
               point->waittime += newwait;
               notify(player, unsafe_tprintf("PID %d has been re-waited with new time of %.1f",
                              pid, (point->waittime - time_ng(NULL))));
               found = 2;
            } else {
               point->waittime = time_ng(NULL) + newwait;
               found = 1;
            }
            break;
         }
      }
   }
   if (!found) {
      for (point = mudstate.qsemfirst; point; point = point->next) {
         if ( (point->pid == pid) && (point->pid != kick_pid) ) {
            if (Immortal(player) || Controls(player,Owner(point->player)) ||
                HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
               point->stop_bool_val = newwait;
               if ( key == -1 ) {
                  if ( (point->waittime - newwait) < (time_ng(NULL) + 10) ) 
                     return -1;
                  point->waittime -= newwait;
                  notify(player, unsafe_tprintf("PID %d has been re-waited with new time of %.1f",
                                 pid, (point->waittime - time_ng(NULL))));
                  found = 2;
               } else if ( key == 1 ) {
                  if ( (point->waittime + newwait) < (time_ng(NULL) + 10) ) 
                     return -1;
                  point->waittime += newwait;
                  notify(player, unsafe_tprintf("PID %d has been re-waited with new time of %.1f",
                                 pid, (point->waittime - time_ng(NULL))));
                  found = 2;
               } else {
                  point->waittime = time_ng(NULL) + newwait;
                  found = 1;
               }
               break;
            }
         }
      }
   }
   return found;
}

#if 0
int 
wait_que_pid(dbref player, int pid, int newwait)
{
    BQUE *trail, *point, *next, *rewait, *pntr;
    int numhalted, a;
    int found;
    dbref psave;
    
    rewait = NULL;
    numhalted = 0;
    found = 0;
    psave = NOTHING;

    /* Wait queue */

    if (!found) {
      for (point = mudstate.qwait, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            rewait = alloc_qentry("wait_que_pid.qblock");
            if (point->text) {
               rewait->text = (char *) malloc(point->text_len);
               rewait->text_len = point->text_len;
               memcpy((char *)rewait->text, (char *)point->text, point->text_len);
            } else {
               rewait->text = NULL;
            }
            if (point->comm) {
/*             rewait->comm = (char *) malloc(strlen(point->comm) + 1);
               strcpy((char *)rewait->comm, (char *)point->comm); */
               rewait->comm = rewait->text + (point->comm - point->text);
            } else {
               rewait->comm = NULL;
            }
            rewait->waittime = time(NULL) + newwait;
            for (a = 0; a < NUM_ENV_VARS; a++) {
                if ( point->env[a] )
                   rewait->env[a] = rewait->text + (point->env[a] - point->text);
                else
                   rewait->env[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scr[a] )
                   rewait->scr[a] = rewait->text + (point->scr[a] - point->text);
                else
                   rewait->scr[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scrname[a] )
                   rewait->scrname[a] = rewait->text + (point->scrname[a] - point->text);
                else
                   rewait->scrname[a] = NULL;
            }
            rewait->player = point->player;
            rewait->cause = point->cause;
            rewait->sem = point->sem;
            rewait->nargs = point->nargs;
            rewait->pid = point->pid;
            rewait->shellprg = point->shellprg;
            rewait->stop_bool = point->stop_bool;
            rewait->stop_bool_val = newwait;
	    numhalted++;
	    pid_table[point->pid] = 0;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	} else
	    next = (trail = point)->next;
      }
    }

    /* Semaphore queue */

    if (!found) {
      for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if ( (point->pid == pid) && (point->pid != kick_pid) ) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
            rewait = alloc_qentry("wait_que_pid.qblock");
            if (point->text) {
               rewait->text = (char *) malloc(point->text_len);
               rewait->text_len = point->text_len;
               memcpy((char *)rewait->text, (char *)point->text, point->text_len);
            } else {
               rewait->text = NULL;
            }
            if (point->comm) {
/*             rewait->comm = (char *) malloc(strlen(point->comm) + 1);
               strcpy((char *)rewait->comm, (char *)point->comm); */
               rewait->comm = rewait->text + (point->comm - point->text);
            } else {
               rewait->comm = NULL;
            }
            rewait->waittime = time(NULL) + newwait;
            for (a = 0; a < NUM_ENV_VARS; a++) {
                if ( point->env[a] )
                   rewait->env[a] = rewait->text + (point->env[a] - point->text);
                else
                   rewait->env[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scr[a] )
                   rewait->scr[a] = rewait->text + (point->scr[a] - point->text);
                else
                   rewait->scr[a] = NULL;
            }
            for (a = 0; a < MAX_GLOBAL_REGS; a++) {
                if ( point->scrname[a] )
                   rewait->scrname[a] = rewait->text + (point->scrname[a] - point->text);
                else
                   rewait->scrname[a] = NULL;
            }
            rewait->player = point->player;
            rewait->cause = point->cause;
            rewait->sem = point->sem;
            rewait->nargs = point->nargs;
            rewait->pid = point->pid;
            rewait->shellprg = point->shellprg;
            rewait->stop_bool = point->stop_bool;
            rewait->stop_bool_val = newwait;
	    pid_table[point->pid] = 0;
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else
	    break;
	} else
	    next = (trail = point)->next;
      }
    }

    if ( found == 1 ) {
       if (psave != NOTHING) {
         if (!FreeFlag(Owner(psave)))
           giveto(psave, (mudconf.waitcost * numhalted), NOTHING);
         a_Queue(psave, -numhalted);
       }
       if (rewait->sem == NOTHING) {
           /* No semaphore, put on wait queue if wait value specified.
            *          * Otherwise put on the normal queue. */
           for (pntr = mudstate.qwait, trail = NULL;
               pntr && pntr->waittime <= rewait->waittime;
               pntr = pntr->next) {
               trail = pntr;
           }
           rewait->next = pntr;
           if (trail != NULL)
               trail->next = rewait;
           else
               mudstate.qwait = rewait;
       } else {
           rewait->next = NULL;
           if (mudstate.qsemlast != NULL)
               mudstate.qsemlast->next = rewait;
           else
               mudstate.qsemfirst = rewait;
           mudstate.qsemlast = rewait;
       }
    }
    return numhalted;
}
#endif

int 
halt_que_pid(dbref player, int pid, int key)
{
    BQUE *trail, *point, *next;
    int numhalted;
    int found;
    dbref psave;

    numhalted = 0;
    found = 0;
    psave = NOTHING;

    /* Player queue */

    for (point = mudstate.qfirst; point; point = point->next) {
      if (point->pid == pid) {
	if (Immortal(player) || Controls(player,Owner(point->player)) || 
	  HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
	    numhalted++;
	    point->player = NOTHING;
	    pid_table[point->pid] = 0;
	    found = 1;
	    break;
	}
	else {
	  found = -1;
	  break;
	}
      }
    }
    /* Object queue */

    if (!found) {
      for (point = mudstate.qlfirst; point; point = point->next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
	    psave = Owner(point->player);
	    numhalted++;
	    point->player = NOTHING;
	    pid_table[point->pid] = 0;
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	}
      }
    }
    /* Wait queue */

    if (!found) {
      for (point = mudstate.qwait, trail = NULL; point; point = next) {
	if (point->pid == pid) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
            if ( key > 0 ) {
               if ( (key == 1) && (point->stop_bool == 0) ) {
                  point->stop_bool = 1;
                  if ( point->waittime != 0 )
                     point->stop_bool_val = point->waittime - mudstate.nowmsec;
                  else
                     point->stop_bool_val = 0;
                  numhalted++;
               } else if ( (key == 2 ) && (point->stop_bool == 1) ) {
                  point->stop_bool = 0;
                  numhalted++;
               } else {
                  numhalted = -1;
               }
               return numhalted;
            }
	    psave = Owner(point->player);
	    numhalted++;
	    pid_table[point->pid] = 0;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qwait = next = point->next;
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else {
	    found = -1;
	    break;
	  }
	} else
	    next = (trail = point)->next;
      }
    }

    /* Semaphore queue */

    if (!found) {
      for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if ( (point->pid == pid) && (point->pid != kick_pid) ) {
	  if (Immortal(player) || Controls(player,Owner(point->player)) ||
	  	HasPriv(player, Owner(point->player), POWER_HALT_QUEUE, POWER4, NOTHING)) {
            if ( key > 0 ) {
               if ( (key == 1) && (point->stop_bool == 0) ) {
                  point->stop_bool = 1;
                  if ( point->waittime != 0 )
                     point->stop_bool_val = point->waittime - mudstate.nowmsec;
                  else
                     point->stop_bool_val = 0;
                  numhalted++;
               } else if ( (key == 2 ) && (point->stop_bool == 1) ) {
                  point->stop_bool = 0;
                  numhalted++;
               } else {
                  numhalted = -1;
               }
               return numhalted;
            }
	    psave = Owner(point->player);
	    pid_table[point->pid] = 0;
	    numhalted++;
	    if (trail)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    add_to(point->sem, -1, A_SEMAPHORE);
	    if (point->text)
		free(point->text);
            point->text=NULL;
	    free_qentry(point);
	    found = 1;
	    break;
	  }
	  else
	    break;
	} else
	    next = (trail = point)->next;
      }
    }

    if (psave != NOTHING) {
      if (!FreeFlag(Owner(psave)))
        giveto(psave, (mudconf.waitcost * numhalted), NOTHING);
      a_Queue(psave, -numhalted);
    }
    return numhalted;
}

/* ---------------------------------------------------------------------------
 * halt_que_all: blast everything from the queues (the right way)
 * Thorin - 2/95 
 */

int 
halt_que_all(void)
{
    BQUE *point, *next;
    int numhalted;

    numhalted = 0;

    /* player queue */
    /* Don't delete player queue entries, just tag them, or else
    ** the mush will crash! - Thorin - 3/99
    */

    for( point = mudstate.qfirst; point; point = point->next ) {
	if (!FreeFlag(Owner(point->player)))
	  giveto(point->player, mudconf.waitcost, NOTHING);
	a_Queue(Owner(point->player), -1);

	numhalted++;
	point->player = NOTHING;
	pid_table[point->pid] = 0;
    }
    mudstate.qlast = NULL;

    /* object queue */
    /* Don't delete object queue entries, just tag them, or else
    ** the mush will crash! - Thorin - 3/99
    */

    for( point = mudstate.qlfirst; point; point = point->next ) {
	if (!FreeFlag(Owner(point->player)))
	  giveto(point->player, mudconf.waitcost, NOTHING);
	a_Queue(Owner(point->player), -1);

	numhalted++;
	point->player = NOTHING;
	pid_table[point->pid] = 0;
    }
    mudstate.qllast = NULL;

    /* wait queue */

    while (mudstate.qwait) {
	point = mudstate.qwait;
	if (!FreeFlag(Owner(point->player)))
	  giveto(point->player, mudconf.waitcost, NOTHING);
	a_Queue(Owner(point->player), -1);
	next = point->next;
	if (point->text)
	    free(point->text);
        point->text=NULL;
	free_qentry(point);
	mudstate.qwait = next;
	numhalted++;
    }
    /* no last pointer for the wait que */

    /* Semaphore queue */

    while (mudstate.qsemfirst) {
	point = mudstate.qsemfirst;
        if ( point->pid == kick_pid ) {
           mudstate.qsemfirst = point->next;
           continue;
        }
	if (!FreeFlag(Owner(point->player)))
	  giveto(point->player, mudconf.waitcost, NOTHING);
	a_Queue(Owner(point->player), -1);
	next = point->next;
	add_to(point->sem, -1, A_SEMAPHORE);
	if (point->text)
	    free(point->text);
        point->text=NULL;
	free_qentry(point);
	mudstate.qsemfirst = next;
	numhalted++;
    }
    mudstate.qsemlast = NULL;
    init_pid_table();

    return numhalted;
}


/* ---------------------------------------------------------------------------
 * do_halt: Command interface to halt_que.
 */

void 
do_halt(dbref player, dbref cause, int key, char *target)
{
    dbref player_targ, obj_targ;
    int numhalted, pid, i_keytype, i_quiet;

    /* Figure out what to halt */

    i_keytype = i_quiet = 0;
    if ( key & HALT_QUIET ) {
       key = key & ~HALT_QUIET;
       i_quiet = 1;
    }
    pid = -1;
    if ( key & HALT_PIDSTOP ) {
       i_keytype = 1;
       key = key & ~HALT_PIDSTOP;
    }
    if ( key & HALT_PIDCONT ) {
       if ( i_keytype ) {
          notify(player, "Illegal combination of switches for @halt.");
          return;
       }
       i_keytype = 2;
       key = key & ~HALT_PIDCONT;
    }
    if ( i_keytype && !(key & HALT_PID) ) {
       notify(player, "Illegal combination of switches for @halt.");
       return;
    }
    if (!target || !*target) {
	obj_targ = NOTHING;
	if (key & HALT_ALL) {
	    if (!Builder(player) && !HasPriv(player, NOTHING, POWER_HALT_QUEUE_ALL, POWER4, NOTHING)) {
		notify(player, "Permission denied.");
		return;
	    }
	    numhalted = halt_que_all();
	} else {
	    player_targ = Owner(player);
	    if (Typeof(player) != TYPE_PLAYER)
		obj_targ = player;
	    numhalted = halt_que(player_targ, obj_targ);
	}
    } else if (key == HALT_PID) {
      if (!is_number(target)) {
	notify(player, "PID must be a number.");
	return;
      }
      pid = atoi(target);
      if ((pid < 0) || (pid > MUMAXPID)) {
	notify(player, "PID out of range.");
	return;
      }
      numhalted = halt_que_pid(player, pid, i_keytype);
      if ( numhalted == -1 ) {
        notify(player, "That PID is already in the specified state.");
        return;
      }
      else if (!numhalted)
	notify(player,"PID not found/Permisison denied.");
      if ( numhalted && (i_keytype > 0) ) {
         if ( !(Quiet(player) || i_quiet) ) {
            if ( i_keytype == 1 )
	       notify(Owner(player), unsafe_tprintf("Queue entry %d stopped.", numhalted));
            else
	       notify(Owner(player), unsafe_tprintf("Queue entry %d resumed.", numhalted));
         }
         return;
      }
    } else {
	init_match(player, target, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	obj_targ = noisy_match_result();
	if (!Good_obj(obj_targ) || (!Controls(player, obj_targ) && !HasPriv(player, obj_targ, POWER_HALT_QUEUE, POWER4, NOTHING))) {
	    notify(player, "Permission denied.");
	    return;
	}
	if (key & HALT_ALL) {
	    notify(player, "Can't specify a target and /all");
	    return;
	}
	if (Typeof(obj_targ) == TYPE_PLAYER) {
	    player_targ = obj_targ;
	    obj_targ = NOTHING;
	} else {
	    player_targ = NOTHING;
	}
	numhalted = halt_que(player_targ, obj_targ);
    }

    if (Quiet(player) || i_quiet)
	return;
    if (numhalted == 1) {
        if ( pid != -1 ) {
	   notify(Owner(player),
	       unsafe_tprintf("1 queue entry removed [PID %d].", pid));
        } else {
	   notify(Owner(player), "1 queue entry removed.");
        }
    } else {
	notify(Owner(player),
	       unsafe_tprintf("%d queue entries removed.", numhalted));
    }
}

/* ---------------------------------------------------------------------------
 * nfy_que: Notify commands from the queue and perform or discard them.
 */

int 
nfy_que(dbref sem, int key, int count, int pid_val)
{
    BQUE *point, *trail, *next;
    int num, aflags;
    dbref aowner;
    char *str;

    str = atr_get(sem, A_SEMAPHORE, &aowner, &aflags);
    num = atoi(str);
    free_lbuf(str);
    if (num > 0) {
	num = 0;
	for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	    if ( (((pid_val == -1) && (point->sem == sem)) || 
                  ((point->sem == sem) && (point->pid == pid_val))) &&
                  (point->pid != kick_pid) ) {
		num++;
		if (trail)
		    trail->next = next = point->next;
		else
		    mudstate.qsemfirst = next = point->next;
		if (point == mudstate.qsemlast)
		    mudstate.qsemlast = trail;

		/* Either run or discard the command */

		if (key != NFY_DRAIN) {
		    give_que(point);
		} else {
		  if (point->player != NOTHING) {
		    if (!FreeFlag(Owner(point->player)))
		      giveto(point->player,
			   mudconf.waitcost, NOTHING);
		    a_Queue(Owner(point->player), -1);
		  }
		    if (point->text)
			free(point->text);
                    point->text=NULL;
		    pid_table[point->pid] = 0;
		    free_qentry(point);
		}
	    } else {
		next = (trail = point)->next;
	    }

	    /* If we've notified enough, exit */

	    if ((key == NFY_NFY) && (num >= count))
		next = NULL;
	}
    } else {
	num = 0;
    }

    /* Update the sem waiters count */

    if ( (key == NFY_NFY) && num )
       add_to(sem, -count, A_SEMAPHORE);
    else if ( key != NFY_NFY )
       atr_clr(sem, A_SEMAPHORE);

    return num;
}

/* ---------------------------------------------------------------------------
 * do_notify: Command interface to nfy_que
 */

void 
do_notify(dbref player, dbref cause, int key, char *what, char *count)
{
    dbref thing;
    int loccount, no_notify, ret_val, pid_notify;

    no_notify = ret_val = pid_notify = 0;
    if ( key & NFY_QUIET ) {
       key &= ~NFY_QUIET;
       no_notify = 1;
    }
    if ( key & NFY_PID ) {
       key &= ~NFY_PID;
       pid_notify = 1;
    }
    init_match(player, what, NOTYPE);
    match_everything(0);

    if ((thing = noisy_match_result()) < 0) {
	notify(player, "No match.");
    } else if (!controls(player, thing) && !Link_ok(thing)) {
	notify(player, "Permission denied.");
    } else {
	if (count && *count)
	    loccount = atoi(count);
	else
	    loccount = 1;
	if (loccount > 0) {
            if ( pid_notify )
	       ret_val = nfy_que(thing, key, 1, loccount);
            else
	       ret_val = nfy_que(thing, key, loccount, -1);
	    if (!(Quiet(player) || Quiet(thing))) {
		if ( (key & NFY_DRAIN) ) {
                    if ( !no_notify ) {
                       if ( ret_val ) {
		          notify_quiet(player, "Drained.");
                       } else {
		          notify_quiet(player, "Semaphore reset. Nothing drained.");
                       }
                    }
		} else {
                    if ( !no_notify ) {
                       if ( ret_val ) {
		          notify_quiet(player, "Notified.");
                       } else {
                          if ( mudconf.queue_compatible ) {
                             notify_quiet(player, "Queue decremented.");
                             add_to(thing, -1, A_SEMAPHORE);
		          } else {
                             notify_quiet(player, "Nothing notified.");
                          }
                       }
                    } else if ( !ret_val && mudconf.queue_compatible ) {
                       add_to(thing, -1, A_SEMAPHORE);
                    }
                }
            } else if ( !(key & NFY_DRAIN) && !ret_val && mudconf.queue_compatible ) {
               add_to(thing, -1, A_SEMAPHORE);
	    }

	}
    }
}

/* ---------------------------------------------------------------------------
 * setup_que: Set up a queue entry.
 */

static BQUE *
setup_que(dbref player, dbref cause, char *command,
	  char *args[], int nargs, char *sargs[], char *sargsname[])
{
    int a, tlen;
    BQUE *tmp;
    char *tptr;
    int tpid;

    /* Can we run commands at all? */

    if (Halted(player) && !mudstate.force_halt)
	return NULL;

    /* make sure player can afford to do it */

    a = mudconf.waitcost;
    if ((random() % mudconf.machinecost) == 0)
	a++;
    if (!payfor(player, a)) {
	notify(Owner(player), "Not enough money to queue command.");
	return NULL;
    }
    /* Wizards and their objs may queue up to db_top+1 cmds. Players are
     * limited to QUEUE_QUOTA. -mnp */

    a = QueueMax(Owner(player));
    if (a_Queue(Owner(player), 1) > a) {

        a_Queue(Owner(player), -1);
        notify(Owner(player),
	       "Run away objects: too many commands queued.  Halted.");
	halt_que(Owner(player), NOTHING);

	/* halt also means no command execution allowed */
	s_Halted(player);
	return NULL;
    }

    if (last_pid < MUMAXPID) {
      for (tpid = last_pid + 1; tpid <= MUMAXPID; tpid++) {
	if (!pid_table[tpid])
	  break;
      }
      if (tpid > MUMAXPID) {
	for (tpid = 0; tpid <= last_pid; tpid++) {
	  if (!pid_table[tpid])
	    break;
	}
	if (tpid > last_pid) {
	  notify(Owner(player), "Unable to queue command.");
	  halt_que_all();
	  return NULL;
	}
      }
    }
    else {
      for (tpid = 0; tpid <= MUMAXPID; tpid++) {
	if (!pid_table[tpid])
	  break;
      }
      if (tpid > MUMAXPID) {
	notify(Owner(player), "Unable to queue command.");
	halt_que_all();
	return NULL;
      }
    }

    /* We passed all the tests */

    /* Calculate the length of the save string */

    tlen = 0;
    if (command)
	tlen = strlen(command) + 1;
    if (nargs > NUM_ENV_VARS)
	nargs = NUM_ENV_VARS;
    for (a = 0; a < nargs; a++) {
	if (args[a])
	    tlen += (strlen(args[a]) + 1);
    }
    if (sargs) {
	for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	    if (sargs[a])
		tlen += (strlen(sargs[a]) + 1);
	}
    }
    if ( sargsname ) {
	for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	    if (sargsname[a])
		tlen += (strlen(sargsname[a]) + 1);
	}
    }
    /* Create the qeue entry and load the save string */

    tmp = alloc_qentry("setup_que.qblock");
    pid_table[tpid] = 1;
    last_pid = tpid;
    tmp->pid = tpid;
    tmp->comm = NULL;
    for (a = 0; a < NUM_ENV_VARS; a++) {
	tmp->env[a] = NULL;
    }
    for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	tmp->scr[a] = NULL;
    }
    for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	tmp->scrname[a] = NULL;
    }

    tptr = tmp->text = (char *) malloc(tlen);
    tmp->text_len = tlen;
    if (!tptr)
	abort();

    if (command) {
	strcpy(tptr, command);
	tmp->comm = tptr;
	tptr += (strlen(command) + 1);
    }
    for (a = 0; a < nargs; a++) {
	if (args[a]) {
	    strcpy(tptr, args[a]);
	    tmp->env[a] = tptr;
	    tptr += (strlen(args[a]) + 1);
	}
    }
    if (sargs) {
	for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	    if (sargs[a]) {
		strcpy(tptr, sargs[a]);
		tmp->scr[a] = tptr;
		tptr += (strlen(sargs[a]) + 1);
	    }
	}
    }
    if ( sargsname ) {
	for (a = 0; a < MAX_GLOBAL_REGS; a++) {
	    if (sargsname[a]) {
		strcpy(tptr, sargsname[a]);
		tmp->scrname[a] = tptr;
		tptr += (strlen(sargsname[a]) + 1);
	    }
	}
    }
    /* Load the rest of the queue block */

    tmp->player = player;
    tmp->waittime = 0;
    tmp->next = NULL;
    tmp->sem = NOTHING;
    tmp->cause = cause;
    tmp->nargs = nargs;
    tmp->stop_bool = 0;
    tmp->stop_bool_val = 0;
    tmp->hooked_command = mudstate.no_hook;
    if ( InProgram(player) )
       tmp->shellprg = 1;
    else
       tmp->shellprg = 0;
    return tmp;
}

/* ---------------------------------------------------------------------------
 * wait_que: Add commands to the wait or semaphore queues.
 */

void 
wait_que(dbref player, dbref cause, double wait, dbref sem,
	 char *command, char *args[], int nargs, char *sargs[], char *sargsname[])
{
    BQUE *tmp, *point, *trail;

    if (mudconf.control_flags & CF_INTERP)
	tmp = setup_que(player, cause, command, args, nargs, sargs, sargsname);
    else
	tmp = NULL;
    if (tmp == NULL) {
	return;
    }
    if (wait != 0)
	tmp->waittime = time_ng(NULL) + wait;
    tmp->sem = sem;
    if (sem == NOTHING) {

	/* No semaphore, put on wait queue if wait value specified.
	 * Otherwise put on the normal queue. */

	if (wait <= 0) {
	    give_que(tmp);
	} else {
	    for (point = mudstate.qwait, trail = NULL;
		 point && point->waittime <= tmp->waittime;
		 point = point->next) {
		trail = point;
	    }
	    tmp->next = point;
	    if (trail != NULL)
		trail->next = tmp;
	    else
		mudstate.qwait = tmp;
	}
    } else {
	tmp->next = NULL;
	if (mudstate.qsemlast != NULL)
	    mudstate.qsemlast->next = tmp;
	else
	    mudstate.qsemfirst = tmp;
	mudstate.qsemlast = tmp;
    }
}

/* ---------------------------------------------------------------------------
 * do_freeze: FREEZE (kill -STOP) the queue process from running
 */

void
do_freeze(dbref player, dbref cause, int key, char *what)
{
   int pid, retval;
   if (is_number(what)) {
      pid = atoi(what);
      retval = freeze_pid(player, pid, key);
      if ( retval )
         notify(player, unsafe_tprintf("PID %d has been frozen.",pid));
      else
	 notify(player,"PID not found/Permission denied.");
   } else {
      notify(player, "PID value must be numeric.");
   }
}

/* ---------------------------------------------------------------------------
 * do_thaw: UN-FREEZE (kill -CONT) the queue process from running
 */

void
do_thaw(dbref player, dbref cause, int key, char *what)
{
   int pid, retval;
   if (is_number(what)) {
      pid = atoi(what);
      retval = thaw_pid(player, pid, key);
      if ( retval == -2 )
         notify(player, "Object executor is no longer valid.  Process deleted.");
      else if ( retval == -1 )
         notify(player, unsafe_tprintf("FTIME %d has been deleted.",pid));
      else if ( retval )
         notify(player, unsafe_tprintf("FTIME %d has been thawed.",pid));
      else
	 notify(player,"FTIME not found/Permission denied.");
   } else {
      notify(player, "FTIME value must be numeric.");
   }
}

/* ---------------------------------------------------------------------------
 * do_wait: Command interface to wait_que
 */

void 
do_wait(dbref player, dbref cause, int key, char *eventorig,
	char *cmd, char *cargs[], int ncargs)
{
    dbref thing;
    int num, pid, retval, recpid, recpidval, numwords, i_shiftoffset;
    char *what, *event, *eventtok, *pt, *numval;
    double howlong, newwait;

    i_shiftoffset = 0;

    if ( (key &~ WAIT_PID &~ WAIT_UNTIL ) && (key & WAIT_UNTIL) ) {
       notify(player, "Illegal combination of switches.");
       return;
    }
    if ( (key & WAIT_PID) && (key & WAIT_RECPID) ) {
       notify(player, "Illegal combination of switches.");
       return;
    }
    recpid = numwords = recpidval = 0;
    if ( key & WAIT_RECPID ) {
       recpid = 1;
       key = (key &~ WAIT_RECPID);
       numwords = countwordsnew(eventorig);
       if ( numwords > 1 ) {
          eventtok = strchr(eventorig,' ');
          if ( eventtok ) {
             event = eventtok+1;
          } else {
             notify(player, "Illegal number of arguments with /RECPID switch.");
             return;
          }
          recpidval = atoi(eventorig);
          if ( (recpidval < 0) || (recpidval > MAX_GLOBAL_REGS) ) {
             notify(player, "Invalid register defined with /RECPID switch.");
             return;
          }
          numval = alloc_sbuf("recpid");
       } else {
          notify(player, "Illegal number of arguments with /RECPID switch.");
          return;
       }
    } else {
       event = eventorig;
    }
    if ( !(key & WAIT_PID) ) {
       /* If arg1 is all numeric, do simple (non-sem) timed wait. */

       if (is_number(event)) {
           if ( key & WAIT_UNTIL )
              howlong = atof(event) - time_ng(NULL);
           else
	      howlong = atof(event);
	   wait_que(player, cause, howlong, NOTHING, cmd,
		    cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
           if ( recpid ) {
              if (!mudstate.global_regs[recpidval])
                  mudstate.global_regs[recpidval] = alloc_lbuf("fun_setq");
              pt = mudstate.global_regs[recpidval];
              sprintf(numval, "%d", last_pid);
              safe_str(numval, mudstate.global_regs[recpidval], &pt);
              free_sbuf(numval);
           }
	   return;
       }
       /* Semaphore wait with optional timeout */
   
       what = parse_to(&event, '/', 0);
       init_match(player, what, NOTYPE);
       match_everything(0);
   
       thing = noisy_match_result();
       if (!Good_obj(thing)) {
	   notify(player, "No match.");
       } else if (!controls(player, thing) && !Link_ok(thing)) {
	   notify(player, "Permission denied.");
       } else {
   
	   /* Get timeout, default 0 */
   
	   if (event && *event) {
               if ( key & WAIT_UNTIL )
	          howlong = atof(event) - time_ng(NULL);
               else
	          howlong = atof(event);
	   } else
	       howlong = 0;
   
	   num = add_to(thing, 1, A_SEMAPHORE);
	   if (num <= 0) {
   
	       /* thing over-notified, run the command immediately */
   
	       thing = NOTHING;
	       howlong = 0;
	   }
	   wait_que(player, cause, howlong, thing, cmd,
		    cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
           if ( recpid ) {
              if (!mudstate.global_regs[recpidval])
                  mudstate.global_regs[recpidval] = alloc_lbuf("fun_setq");
              pt = mudstate.global_regs[recpidval];
              sprintf(numval, "%d", last_pid);
              safe_str(numval, mudstate.global_regs[recpidval], &pt);
              free_sbuf(numval);
           }
           
       }
    } else {
       if (is_number(event)) {
	   pid = atoi(event);
           if ( key & WAIT_UNTIL ) {
              newwait = atof(cmd) - time_ng(NULL);
           } else {
              if ( *cmd == '+' ) {
                 newwait = atof(cmd+1);
                 i_shiftoffset = 1;
              } else if ( *cmd == '-' ) {
                 newwait = atof(cmd+1);
                 i_shiftoffset = -1;
              } else {
                 newwait = atof(cmd);
                 i_shiftoffset = 0;
              }
           }
           if ( pid < 0 || pid > MUMAXPID )
	      notify(player,"PID not found/Permission denied.");
           else if ( newwait < 10 ) {
              if ( key & WAIT_UNTIL )
                 notify(player, "New wait time must be more than 10 seconds from now.");
              else
                 notify(player, "New wait time must be greater than 10 seconds.");
	   } else if ( (newwait + time_ng(NULL)) > (MYMAXINT - 1) ||
		       (newwait + time_ng(NULL)) < 0 ) {
              if ( key & WAIT_UNTIL )
                 notify(player, unsafe_tprintf("New wait time exceeds maximum value of %d.", MYMAXINT - 60));
              else
	         notify(player, unsafe_tprintf("New wait time must be less than %.1f seconds.", MYMAXINT - time_ng(NULL) - 60));
           } else {
              retval = wait_que_pidnew(player, pid, newwait, i_shiftoffset);
              if ( retval == -1 )
                 notify(player, "PID value had wait of less than 10 seconds.");
              else if ( retval ) {
                 if (i_shiftoffset == 0) 
                    notify(player, unsafe_tprintf("PID %d has been re-waited with new time of %.1f",
                                   pid, newwait));
              } else 
	         notify(player,"PID not found/Permission denied.");
           }
       } else {
	  notify(player,"PID not found/Permission denied.");
       }
    }
}

/* ---------------------------------------------------------------------------
 * que_next: Return the time in seconds until the next command should be
 * run from the queue.
 */

double 
NDECL(que_next)
{
    int min, this;
    BQUE *point;

    /* If there are commands in the player queue, we want to run them
     * immediately.
     */

    if (test_top())
	return 0;

    /* If there are commands in the object queue, we want to run them
     * after a one-second pause.
     */

    if (mudstate.qlfirst != NULL)
	return 0.1;

    /* Walk the wait and semaphore queues, looking for the smallest
     * wait value.  Return the smallest value - 1, because the command
     * gets moved to the player queue when it has 1 second to go.
     */

    min = 1000;
    for (point = mudstate.qwait; point; point = point->next) {
	this = point->waittime - mudstate.nowmsec;
	if (this <= 0.2)
	    return 0.1;
	if (this < min)
	    min = this;
    }

    for (point = mudstate.qsemfirst; point; point = point->next) {
	if (point->waittime == 0)	/* Skip if no timeout */
	    continue;
	this = point->waittime - mudstate.nowmsec;
	if (this <= 0.2)
	    return 0.1;
	if (this < min)
	    min = this;
    }
    return min - 0.1;
}

/* ---------------------------------------------------------------------------
 * do_second: Check the wait and semaphore queues for commands to remove.
 */

void 
NDECL(do_second)
{
    BQUE *trail, *point, *next;
    DESC *d, *dnext;
    char *cmdsave, *cpulbuf;
    int i_offset;
    double d_timediff;

    /* move contents of low priority queue onto end of normal one
     * this helps to keep objects from getting out of control since
     * its affects on other objects happen only after one seconds
     * this should allow @halt to be type before getting blown away
     * by scrolling text */

    if ((mudconf.control_flags & CF_DEQUEUE) == 0)
	return;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< do_second >";

    if (mudstate.qlfirst) {
	if (mudstate.qlast)
	    mudstate.qlast->next = mudstate.qlfirst;
	else
	    mudstate.qfirst = mudstate.qlfirst;
	mudstate.qlast = mudstate.qllast;
	mudstate.qllast = mudstate.qlfirst = NULL;
    }
    /* Note: the point->waittime test would be 0 except the command is
     * being put in the low priority queue to be done in one second anyways
     */

    /* Do the wait queue */

    i_offset = 0;
    d_timediff = (int)mudstate.nowmsec - (int)mudstate.lastnowmsec;
    for (point = mudstate.qwait, trail = NULL; point ; point = next) {
        if ( (d_timediff > 120) || (d_timediff < -120) ) {
           point->waittime = point->waittime + d_timediff;
           i_offset = 1;
        } 
        if ( point->stop_bool ) {
           point->waittime = mudstate.nowmsec + point->stop_bool_val;
        }
        if ( point->waittime <= mudstate.nowmsec ) {
           if (trail != NULL)
              trail->next = next = point->next;
           else
	      mudstate.qwait = next = point->next;
	   give_que(point);
        } else
           next = (trail = point)->next;
    }

    /* Check the semaphore queue for expired timed-waits */

    for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
	if (point->waittime == 0) {
	    next = (trail = point)->next;
	    continue;		/* Skip if not timed-wait */
	}
        if ( (d_timediff > 120) || (d_timediff < -120) ) {
           point->waittime = point->waittime + d_timediff;
           i_offset = 1;
        }
        if ( point->stop_bool ) {
           point->waittime = mudstate.nowmsec + point->stop_bool_val;
        }
	if (point->waittime <= mudstate.nowmsec) {
	    if (trail != NULL)
		trail->next = next = point->next;
	    else
		mudstate.qsemfirst = next = point->next;
	    if (point == mudstate.qsemlast)
		mudstate.qsemlast = trail;
	    add_to(point->sem, -1, A_SEMAPHORE);
	    point->sem = NOTHING;
	    give_que(point);
	} else {
	    next = (trail = point)->next;
        }
    }
    if ( i_offset ) {
       STARTLOG(LOG_ALWAYS, "WIZ", "CLK");
          cpulbuf = alloc_lbuf("do_second.clockskew");
          sprintf(cpulbuf, "Clock Skew Detected: Skew was %.1f seconds. Queue compensated.", 
                  d_timediff);
          log_text(cpulbuf);
          free_lbuf(cpulbuf);
       ENDLOG
       /* Ok, let's update the player time stats here */
       DESC_SAFEITER_ALL(d, dnext) {
         
          if (d->flags & DS_CONNECTED) {
             d->last_time = d->last_time + floor(d_timediff);
             d->connected_at = d->connected_at + floor(d_timediff);
          } else {
             d->connected_at = d->connected_at + floor(d_timediff);
          }
       }
       /* Let's update the internal counters with the new time */
       mudstate.dump_counter = mudstate.dump_counter + d_timediff;
       mudstate.idle_counter = mudstate.idle_counter + d_timediff;
       mudstate.check_counter = mudstate.check_counter + d_timediff;
       mudstate.mstats_counter = mudstate.mstats_counter + d_timediff;
       mudstate.rwho_counter = mudstate.rwho_counter + d_timediff;
       mudstate.cntr_reset = mudstate.cntr_reset + d_timediff;

       /* Reset Alarm Timer */
       mudstate.alarm_triggered = 0;
       alarm_msec (next_timer());
    }
    mudstate.debug_cmd = cmdsave;
    return;
}

/* ---------------------------------------------------------------------------
 * do_top: Execute the command at the top of the queue
 */

int 
do_top(int ncmds)
{
    BQUE *tmp;
    int count, i;
    char *cmdsave;

    if ((mudconf.control_flags & CF_DEQUEUE) == 0)
	return 0;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< do_top >";

    for (count = 0; count < ncmds; count++) {
	if (!test_top()) {
	    mudstate.debug_cmd = cmdsave;
	    for (i = 0; i < MAX_GLOBAL_REGS; i++) {
		*mudstate.global_regs[i] = '\0';
		*mudstate.global_regsname[i] = '\0';
   	        *mudstate.global_regs_backup[i] = '\0';
            }
	    return count;
	}
	execute_entry(mudstate.qfirst);
	tmp = mudstate.qfirst;
	mudstate.qfirst = mudstate.qfirst->next;
	if (!mudstate.qfirst)
	    mudstate.qlast = NULL;
	if (tmp->text)
	    free(tmp->text);
	free_qentry(tmp);
        mudstate.curr_cmd = (char *) "";
    }

    for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	*mudstate.global_regs[i] = '\0';
	*mudstate.global_regsname[i] = '\0';
  	*mudstate.global_regs_backup[i] = '\0';
    }
    mudstate.debug_cmd = cmdsave;
    return count;
}

/* ---------------------------------------------------------------------------
 * do_ps: tell player what commands they have pending in the queue
 */

int
fun_do_chk(BQUE *tmp, dbref player, dbref *player_targ, dbref *obj_targ) 
{
   if (tmp->player == NOTHING) {
      return 0;
   } else if ( (*player_targ == NOTHING) && (*obj_targ == NOTHING) ) {
      if ( Immortal(player) || Controls(player,Owner(tmp->player) ) || (player == tmp->player) ||
           HasPriv(player,NOTHING,POWER_SEE_QUEUE_ALL, POWER3, POWER_LEVEL_NA) ||
           HasPriv(player,Owner(tmp->player),POWER_SEE_QUEUE, POWER3, NOTHING) ) {
         return 1;
      } else {
         return 0;
      }
   } else {
      if ( Good_obj(*obj_targ) && Controls(player, *obj_targ) && Builder(player) ) {
         *player_targ = NOTHING;
      }
      if ( *player_targ == *obj_targ )
         *player_targ = NOTHING;
      return que_want(tmp, *player_targ, *obj_targ);
   }
   return 0;
}

void 
fun_do_display(BQUE *tmp, dbref player, dbref player_targ, dbref obj_targ, int key, int first, char *buff, char **bufcx, char sep)
{
   int do_sep;
   static char i_buffering[SBUF_SIZE];

   do_sep = 0;
   memset(i_buffering, '\0', SBUF_SIZE);
   if ( first )
      safe_chr(sep, buff, bufcx);
   if ( (key & 1) || !key ) {
      sprintf(i_buffering, "%d", tmp->pid);
      safe_str(i_buffering, buff, bufcx);
      do_sep = 1;
   }
   if ( key & 2 ) {
      if ( do_sep )
         safe_chr('|', buff, bufcx);
      sprintf(i_buffering, "#%d", tmp->player);
      safe_str(i_buffering, buff, bufcx);
      do_sep = 1;
   }
   if ( key & 4 ) {
      if ( do_sep )
         safe_chr('|', buff, bufcx);
      sprintf(i_buffering, "#%d", tmp->cause);
      safe_str(i_buffering, buff, bufcx);
      do_sep = 1;
   }
   if ( key & 8 ) {
      if ( do_sep )
         safe_chr('|', buff, bufcx);
      if ( tmp->waittime )
         sprintf(i_buffering, "%.1f", (tmp->waittime - mudstate.nowmsec));
      else
         sprintf(i_buffering, "%d", 0);
      safe_str(i_buffering, buff, bufcx);
      do_sep = 1;
   }
   if ( key & 16 ) {
      if ( do_sep )
         safe_chr('|', buff, bufcx);
      if ( tmp->stop_bool )
         safe_str("STOPPED", buff, bufcx);
      else
         safe_str("RUNNING", buff, bufcx);
      do_sep = 1;
   }
   if ( key & 32 ) {
      if ( do_sep )
         safe_chr('|', buff, bufcx);
      safe_str(tmp->comm, buff, bufcx);
   }
}

void
show_que_func(dbref player, char *target, int key, char s_type, char *buff, char **bufcx, char sep)
{
   BQUE *tmp;
   dbref player_targ, obj_targ;
   int first, i_pid;

   i_pid = -1;
   if ( target && *target ) {
      if ( is_integer(target) ) {
         player_targ = obj_targ = NOTHING;
         i_pid = atoi(target);
         if ( i_pid <= 0 )
            i_pid = -1;
      } else {
         player_targ = player;
         init_match(player, target, NOTYPE);
         match_everything(MAT_EXIT_PARENTS);
         obj_targ = match_result();
         if (!Good_chk(obj_targ) || (!Controls(player, obj_targ) &&
              !HasPriv(player, obj_targ, POWER_SEE_QUEUE, POWER3, NOTHING))) {
            return;
         }
      }
   } else {
      player_targ = obj_targ = NOTHING;
   }
   first = 0;
   if ( s_type == 'p' ) {
      for ( tmp = mudstate.qfirst; tmp; tmp = tmp->next ) {
         if ( !fun_do_chk(tmp, player, &player_targ, &obj_targ) ) 
            continue;
         if ( (i_pid > 0) && (i_pid != tmp->pid) )
            continue;
         fun_do_display(tmp, player, player_targ, obj_targ, key, first, buff, bufcx, sep);
         first = 1;
      }
   } 
   if ( s_type == 'o' ) {
      for ( tmp = mudstate.qlfirst; tmp; tmp = tmp->next ) {
         if ( !fun_do_chk(tmp, player, &player_targ, &obj_targ) ) 
            continue;
         if ( (i_pid > 0) && (i_pid != tmp->pid) )
            continue;
         fun_do_display(tmp, player, player_targ, obj_targ, key, first, buff, bufcx, sep);
         first = 1;
      }
   }
   if ( (s_type == 'q') || (s_type == 'w') ) {
      for ( tmp = mudstate.qwait; tmp; tmp = tmp->next ) {
         if ( !fun_do_chk(tmp, player, &player_targ, &obj_targ) ) 
            continue;
         if ( (i_pid > 0) && (i_pid != tmp->pid) )
            continue;
         fun_do_display(tmp, player, player_targ, obj_targ, key, first, buff, bufcx, sep);
         first = 1;
      }
   }
   if ( (s_type == 'q') || (s_type == 's') ) {
      for ( tmp = mudstate.qsemfirst; tmp; tmp = tmp->next ) {
         if ( !fun_do_chk(tmp, player, &player_targ, &obj_targ) ) 
            continue;
         if ( (i_pid > 0) && (i_pid != tmp->pid) )
            continue;
         fun_do_display(tmp, player, player_targ, obj_targ, key, first, buff, bufcx, sep);
         first = 1;
      }
   }
}

static void 
show_que(dbref player, int key, BQUE * queue, int *qtot,
	 int *qent, int *qdel,
	 dbref player_targ, dbref obj_targ, const char *header, int sw_type)
{
    BQUE *tmp;
    char *bp, *bufp, *tpr_buff, *tprp_buff, stop_chr;
    int i, check;

    *qtot = 0;
    *qent = 0;
    *qdel = 0;
    key = key & ~PS_FREEZE;
    tprp_buff = tpr_buff = alloc_lbuf("show_que");
    for (tmp = queue; tmp; tmp = tmp->next) {
	(*qtot)++;
	if (tmp->player == NOTHING)
	  check = 0;
	else if ((player_targ == NOTHING) && (obj_targ == NOTHING)) {
	  if (Immortal(player) || Controls(player,Owner(tmp->player)) ||  (player == tmp->player) ||
		HasPriv(player,NOTHING,POWER_SEE_QUEUE_ALL, POWER3, POWER_LEVEL_NA) ||
		HasPriv(player,Owner(tmp->player),POWER_SEE_QUEUE, POWER3, NOTHING))
	    check = 1;
	  else
	    check = 0;
	}
	else {
           if ( Good_obj(obj_targ) && Controls(player, obj_targ) && Builder(player) ) {
               player_targ = NOTHING;
           }
	   check = que_want(tmp, player_targ, obj_targ);
        }
	if (check) {
	    (*qent)++;
	    if (key == PS_SUMM)
		continue;
	    if (*qent == 1) {
                tprp_buff = tpr_buff;
		notify(player, safe_tprintf(tpr_buff, &tprp_buff, "----- %s Queue -----", header));
            }
	    bufp = unparse_object2(player, tmp->player, 0);
            tprp_buff = tpr_buff;
            if ( tmp->stop_bool )
               stop_chr = 'S';
            else
               stop_chr = ' ';
	    if ((tmp->waittime > 0) && (Good_obj(tmp->sem)))
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "(%s: %-5d) %c [#%d/%.1f]%s:%s", sw_type ? "FTIME" : "PID",
                               tmp->pid,
                               stop_chr,
			       tmp->sem,
			       sw_type ? tmp->waittime - tmp->pid : tmp->waittime - mudstate.nowmsec,
			       bufp, tmp->comm));
	    else if (tmp->waittime > 0)
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "(%s: %-5d) %c [%.1f]%s:%s", sw_type ? "FTIME" : "PID",
                               tmp->pid,
                               stop_chr,
			       sw_type ? tmp->waittime - tmp->pid : tmp->waittime - mudstate.nowmsec,
			       bufp, tmp->comm));
	    else if (Good_obj(tmp->sem))
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "(%s: %-5d) %c [#%d]%s:%s", sw_type ? "FTIME" : "PID",
                               tmp->pid, stop_chr, tmp->sem,
			       bufp, tmp->comm));
	    else
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "(%s: %-5d) %c %s:%s", sw_type ? "FTIME" : "PID",
                               tmp->pid, stop_chr, bufp, tmp->comm));
	    bp = bufp;
	    if (key == PS_LONG) {
		for (i = 0; i < (tmp->nargs); i++) {
		    if (tmp->env[i] != NULL) {
			safe_str((char *) "; Arg",
				 bufp, &bp);
			safe_chr(i + '0', bufp, &bp);
			safe_str((char *) "='",
				 bufp, &bp);
			safe_str(tmp->env[i],
				 bufp, &bp);
			safe_chr('\'', bufp, &bp);
		    }
		}
		*bp = '\0';
		bp = unparse_object2(player, tmp->cause, 0);
                tprp_buff = tpr_buff;
		notify(player, safe_tprintf(tpr_buff, &tprp_buff, "   Enactor: %s%s", bp, bufp));
		free_lbuf(bp);
	    }
	    free_lbuf(bufp);
	} else if (tmp->player == NOTHING) {
	    (*qdel)++;
	}
    }
    free_lbuf(tpr_buff);
    return;
}

void 
do_ps(dbref player, dbref cause, int key, char *target)
{
    char *bufp;
    dbref player_targ, obj_targ;
    int pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent,
        sqtot, i;

    /* Figure out what to list the queue for */

    if (!target || !*target) {
	obj_targ = NOTHING;
	if (key & PS_ALL) {
/*	    if (!Builder(player) && !HasPriv(player, NOTHING, POWER_SEE_QUEUE_ALL, POWER3, POWER_LEVEL_NA)) {
		notify(player, "Permission denied.");
		return;
	    } */	/* commented out by Seawolf due to level checking for @ps/all */
	    player_targ = NOTHING;
	} else {
	    player_targ = Owner(player);
	    if (Typeof(player) != TYPE_PLAYER)
		obj_targ = player;
	}
    } else {
	player_targ = Owner(player);
	init_match(player, target, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	obj_targ = noisy_match_result();
	if (!Good_obj(obj_targ) || (!Controls(player, obj_targ) &&
	    !HasPriv(player, obj_targ, POWER_SEE_QUEUE, POWER3, NOTHING))) {
	    notify(player, "Permisson denied.");
	    return;
	}
	if (key & PS_ALL) {
	    notify(player, "Can't specify a target and /all");
	    return;
	}
	if (Typeof(obj_targ) == TYPE_PLAYER) {
	    player_targ = obj_targ;
	    obj_targ = NOTHING;
	}
    }
    key = key & ~PS_ALL;

    switch (key) {
    case PS_BRIEF:
    case PS_SUMM:
    case PS_LONG:
    case PS_FREEZE:
    case 9:		/* freeze & summ */
    case 10:		/* freeze & long */
	break;
    default:
	notify(player, "Illegal combination of switches.");
	return;
    }

    /* Go do it */
    if ( !(key & PS_FREEZE) ) {
       show_que(player, key, mudstate.qfirst, &pqtot, &pqent, &pqdel,
	        player_targ, obj_targ, "Player", 0);
       show_que(player, key, mudstate.qlfirst, &oqtot, &oqent, &oqdel,
	        player_targ, obj_targ, "Object", 0);
       show_que(player, key, mudstate.qwait, &wqtot, &wqent, &i,
	        player_targ, obj_targ, "Wait", 0);
       show_que(player, key, mudstate.qsemfirst, &sqtot, &sqent, &i,
	        player_targ, obj_targ, "Semaphore", 0);

       /* Display stats */

       bufp = alloc_mbuf("do_ps");
       if (Wizard(player))
	   sprintf(bufp, 
                   "Totals: Player...%d/%d[%ddel]  Object...%d/%d[%ddel]  Wait...%d/%d  Semaphore...%d/%d",
		   pqent, pqtot, pqdel, oqent, oqtot, oqdel,
		   wqent, wqtot, sqent, sqtot);
       else
	   sprintf(bufp, "Totals: Player...%d/%d  Object...%d/%d  Wait...%d/%d  Semaphore...%d/%d",
		   pqent, pqtot, oqent, oqtot, wqent, wqtot, sqent, sqtot);
       notify(player, bufp);
       free_mbuf(bufp);
    } else {
       show_que(player, key, mudstate.fqwait, &wqtot, &wqent, &i,
	        player_targ, obj_targ, "Wait", 1);
       show_que(player, key, mudstate.fqsemfirst, &sqtot, &sqent, &i,
	        player_targ, obj_targ, "Semaphore", 1);
       bufp = alloc_mbuf("do_ps");
       sprintf(bufp, "Freeze Totals:  Wait...%d/%d   Semaphore...%d/%d", wqent, wqtot, sqent, sqtot);
       notify(player, bufp);
       free_mbuf(bufp);
    }
}

/* ---------------------------------------------------------------------------
 * do_queue: Queue management
 */

void 
do_queue(dbref player, dbref cause, int key, char *arg)
{
    BQUE *point;
    int i, ncmds, was_disabled;


    was_disabled = 0;
    if (key & QUEUE_KICK) {
        if (desc_in_use == NULL) {
           notify(player, "You can not queue a @kick command.");
           return;
        } 
	i = atoi(arg);
	if ((mudconf.control_flags & CF_DEQUEUE) == 0) {
	    was_disabled = 1;
	    mudconf.control_flags |= CF_DEQUEUE;
	    notify(player, "Warning: automatic dequeueing is disabled.");
	}
	if (key & QUEUE_KICK_PID) {
	  if (!ind_pid_func(player,i,FUNC_KICK)) {
	    notify(player,"PID not found/Permission denied.");
	    ncmds = 0;
	  } else
	    ncmds = 1;
	} else
	  ncmds = do_top(i);
	if (was_disabled)
	    mudconf.control_flags &= ~CF_DEQUEUE;
	if (!Quiet(player))
	    notify(player,
		   unsafe_tprintf("%d commands processed.", ncmds));
    } else if (key == QUEUE_WARP) {
	i = atoi(arg);
	if ((mudconf.control_flags & CF_DEQUEUE) == 0) {
	    was_disabled = 1;
	    mudconf.control_flags |= CF_DEQUEUE;
	    notify(player, "Warning: automatic dequeueing is disabled.");
	}
	/* Handle the wait queue */

	for (point = mudstate.qwait; point; point = point->next) {
	    point->waittime = -i;
	}

	/* Handle the semaphore queue */

	for (point = mudstate.qsemfirst; point; point = point->next) {
	    if (point->waittime > 0) {
		point->waittime -= i;
		if (point->waittime <= 0)
		    point->waittime = -1;
	    }
	}

	do_second();
	if (was_disabled)
	    mudconf.control_flags &= ~CF_DEQUEUE;
	if (Quiet(player))
	    return;
	if (i > 0)
	    notify(player,
		   unsafe_tprintf("WaitQ timer advanced %d seconds.", i));
	else if (i < 0)
	    notify(player,
		   unsafe_tprintf("WaitQ timer set back %d seconds.", i));
	else
	    notify(player,
		   "Object queue appended to player queue.");

    }
}
