/* wiz.c -- Wizard-only commands */

#ifdef SOLARIS
/* Borked declarations in header files */
char *strtok_r(char *, const char *, char **);
#endif

#include <dirent.h>
#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "file_c.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "command.h"
#include "htab.h"
#include "alloc.h"
#include "attrs.h"
#include "door.h"
#include "rhost_ansi.h"

extern void remote_write_obj(FILE *, dbref, int, int);
extern int remote_read_obj(FILE *, dbref, int, int, int*, int);
extern int remote_read_sanitize(FILE *, dbref, int, int, int);
extern dbref FDECL(match_thing, (dbref, char *));
extern void fun_parenstr(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern ATRCACHE *atrcache_head;

static const char *
time_format_1(time_t dt)
{
    register struct tm *delta;
    static char buf[64];


    if (dt < 0)
        dt = 0;

    delta = gmtime(&dt);
    if ((int)dt >= 31556926 ) {
        sprintf(buf, "%dy", (int)dt / 31556926);
    } else if ((int)dt >= 2629743 ) {
        sprintf(buf, "%dM", (int)dt / 2629743);
    } else if ((int)dt >= 604800) {
        sprintf(buf, "%dw", (int)dt / 604800);
    } else if (delta->tm_yday > 0) {
        sprintf(buf, "%dd", delta->tm_yday);
    } else if (delta->tm_hour > 0) {
        sprintf(buf, "%dh", delta->tm_hour);
    } else if (delta->tm_min > 0) {
        sprintf(buf, "%dm", delta->tm_min);
    } else {
        sprintf(buf, "%ds", delta->tm_sec);
    }
    return(buf);
}

static const char *
wiz_time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];


    if (dt < 0)
        dt = 0;

    delta = gmtime(&dt);
    if ((int)dt >= 31556926 ) {
        sprintf(buf, "%dy", (int)dt / 31556926);
    } else if ((int)dt >= 2629743 ) {
        sprintf(buf, "%dM", (int)dt / 2629743);
    } else if ((int)dt >= 604800) {
        sprintf(buf, "%dw", (int)dt / 604800);
    } else if (delta->tm_yday > 0) {
        sprintf(buf, "%dd", delta->tm_yday);
    } else if (delta->tm_hour > 0) {
        sprintf(buf, "%dh", delta->tm_hour);
    } else if (delta->tm_min > 0) {
        sprintf(buf, "%dm", delta->tm_min);
    } else {
        sprintf(buf, "%ds", delta->tm_sec);
    }
    return(buf);
}

void do_teleport(dbref player, dbref cause, int key, char *slist, 
		 char *dlist[], int nargs)
{
   dbref victim, destination, loc;
   char *to, *arg1, *arg2, separgs[3], *tstrtokr, *cpulbuf;
   int con, dcount, quiet, side_effect=0, tel_bool_chk, i_abort;
   time_t i_time, i_timechk;

   /* get victim */
   if ( mudstate.remotep != NOTHING) {
      notify(player, "You can't teleport.");
      return;
   }

   if ( !mudstate.argtwo_fix && nargs && !*dlist[0] ) {
      notify(player, "No valid destination to teleport to.");
      return;
   }

   tel_bool_chk = 0;
   if (key & TEL_QUIET) {
      key &= ~TEL_QUIET;
      quiet = 1;
   } else {
      quiet =0;
   }

   if ( key & SIDEEFFECT ) {
      key &= ~SIDEEFFECT;
      side_effect=1;
   }
   if (nargs < 1) {
      notify(player,"No destination given.");
      return;
   }
   memset(separgs, '\0', sizeof(separgs));

   if ((key != TEL_LIST) && (nargs > 1)) {
      notify(player,"Extra destinations ignored."); 
   }

   dcount = 0;
   con = 1;
   if (key == TEL_LIST) {
      if ( strchr(slist, ',') != NULL )
         *separgs = ',';
      else
         *separgs = ' ';
      arg1 = strtok_r(slist, separgs, &tstrtokr);
      if (!arg1) {
         notify(player,"No match.");
      }
   } else {
      arg1 = slist;
   }

   i_time = time(NULL);
   i_abort = 0;
   while (con && arg1 && ((dcount < nargs) || (nargs == 1))) {
      i_timechk = time(NULL);
      /* We allow twice the time check on this */
      if ( (key & TEL_LIST) && (i_timechk > (i_time + (mudconf.cputimechk * 2))) ) {
         notify(player, "@tel/list exceeded the timeout limit");
         i_abort = 1;
      }
      if ( (key & TEL_LIST) && (mudstate.chkcpu_toggle || i_abort) ) {
         broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (@tel/list)",
                           (char *)"(@tel list processing)", NULL, player, 0, 0, NULL);
         STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
            log_name_and_loc(player);
            cpulbuf = alloc_lbuf("log_@tel_cpuslam");
            sprintf(cpulbuf, " CPU/@tel-list overload: #%d (Loc: %d) - UseLock Eval", player, Location(player));
            log_text(cpulbuf);
            free_lbuf(cpulbuf);
         ENDLOG
         break;
      }

      if (key != TEL_LIST) {
         con = 0;
      }
      arg2 = dlist[dcount];
      if (nargs != 1) {
         dcount++;
      }
      if (key == TEL_JOIN) {
         loc = lookup_player(player,arg1,0);
         if (loc == NOTHING) {
            notify(player,"Player not found.");
            return;
         }
         if ((!HasPriv(player,loc,POWER_JOIN_PLAYER,POWER3,NOTHING)) && !Controls(player,loc)) {
            notify(player,"Permission denied.");
            return;
         }
         sprintf(arg1,"#%d",Location(loc));
         victim = player;
         to = arg1;
      } else if (key == TEL_GRAB) {
         victim = lookup_player(player,arg1,0);
         if (victim == NOTHING) {
            notify(player,"Player not found.");
            return;
         }
         sprintf(arg1,"#%d",Location(player));
         to = arg1;
      } else {
         if (*arg2 == '\0') {
            victim = player;
            to = arg1;
         } else {
            init_match(player, arg1, NOTYPE);
            match_everything(0);
            victim = noisy_match_result();

            if (victim == NOTHING) {
               if (key != TEL_LIST) {
                  continue;
               } else {
                  arg1 = strtok_r(NULL, separgs, &tstrtokr);
                  continue;
               }
            }
            to = arg2;
         }
      }

      /* Validate type of victim */
      if (!Has_location(victim) && !isExit(victim)) {
         notify_quiet(player, "You can't teleport that.");
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }

      /* Fail if we don't control the victim or the victim's location */
      if ((!Controls(player, victim) && !HasPriv(player,victim,POWER_TEL_ANYTHING,POWER4,POWER_CHECK_OWNER) &&
           !((key == TEL_GRAB) && HasPriv(player,victim,POWER_GRAB_PLAYER,POWER3,NOTHING)) &&
           !Controls(player, Location(victim)) && !TelOK(victim)) || Fubar(player) ||
          (Fubar(victim) && !Wizard(player)) || DePriv(player,victim,DP_TEL_ANYTHING,POWER7,NOTHING)) {

         notify_quiet(player, "Permission denied.");
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }
      if ((Immortal(victim) && SCloak(victim) && (!Immortal(player) || !Controls(player,victim))) ||
          (Wizard(victim) && Cloak(victim) && !Wizard(player))) {

         notify_quiet(player,"Permission denied.");
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }
      if (Wizard(victim) && !Controls(player,victim) && !TelOK(victim)) {
         notify_quiet(player,"Permission denied.");
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }
      if ( (mudstate.remotep != NOTHING) || (No_tel(victim) && !Wizard(player)) ) {
         if ( victim == player ) {
            notify_quiet(player,"You aren't allowed to @tel.");
         } else {
            notify_quiet(player,"That can't be @tel'd.");
         }
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }

      /* Check for teleporting home */
      if (!string_compare(to, "home") && !isExit(victim)) {
         (void)move_via_teleport(victim, HOME, player, 0, quiet);
         if ( !((side_effect) || Quiet(player)) ) {
            notify_quiet(player,"Teleported.");
         }
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }
 
      if ((Privilaged(victim) && !Controls(player,victim) && !TelOK(victim) &&
          !((key == TEL_GRAB) && HasPriv(player,victim,POWER_GRAB_PLAYER,POWER3,NOTHING)) &&
          !(HasPriv(player,victim,POWER_TEL_ANYTHING,POWER4,POWER_CHECK_OWNER))) ||
          DePriv(player,victim,DP_TEL_ANYTHING,POWER7,NOTHING)) {
 
         notify_quiet(player,"Permission denied.");
         if (key != TEL_LIST) {
            continue;
         } else {
            arg1 = strtok_r(NULL, separgs, &tstrtokr);
            continue;
         }
      }
 
      /* Find out where to send the victim */
      init_match(player, to, NOTYPE);
      match_everything(0);
      destination = match_result();
 
      switch (destination) {
         case NOTHING:
            notify_quiet(player, "No match.");
            if (key != TEL_LIST) {
               continue;
            } else {
               arg1 = strtok_r(NULL, separgs, &tstrtokr);
               continue;
            }
         case AMBIGUOUS:
            notify_quiet(player, "I don't know which destination you mean!");
            if (key != TEL_LIST) {
               continue;
            } else {
               arg1 = strtok_r(NULL, separgs, &tstrtokr);
               continue;
            }
         default:
            if (Recover(destination) && !Immortal(player)) {
               notify_quiet(player, "No match.");
               if (key != TEL_LIST) {
                  continue;
               } else {
                  arg1 = strtok_r(NULL, separgs, &tstrtokr);
                  continue;
               }
            }
            if (victim == destination) {
               notify_quiet(player, "Bad destination.");
               if (key != TEL_LIST) {
                  continue;
               } else {
                  arg1 = strtok_r(NULL, separgs, &tstrtokr);
                  continue;
               }
            }
      }

      /* If fascist teleport is on, you must control the victim's ultimate
       * location (after LEAVEing any objects) or it must be JUMP_OK.
       */

      if (mudconf.fascist_tport) {
         loc = where_room(victim);
         if ( !Good_obj(loc) || !isRoom(loc) ||
              (!Controls(player, loc) && !Jump_ok(loc))) {
            notify_quiet(player, "Permission denied.");
            if (key != TEL_LIST) {
               continue;
            } else {
               arg1 = strtok_r(NULL, separgs, &tstrtokr);
               continue;
            }
         }
      }
			
      /* You must control the destination, or it must be a JUMP_OK
       * room where you pass its TELEPORT lock.
       */
      if (Has_contents(destination)) {	
         if ( mudconf.empower_fulltel ) {
            tel_bool_chk = (!Good_obj(victim) || !Good_obj(destination) ||
                            Cloak(victim) || Cloak(destination) || (victim == 1) || (destination == 1));
         } else {
            tel_bool_chk = (victim != player);
         }

         if ( ((!Controls(player, destination) && 
              !HasPriv(player,destination,POWER_TEL_ANYWHERE,POWER4,POWER_CHECK_OWNER) &&
                (!HasPriv(player,NOTHING,POWER_FULLTEL_ANYWHERE,POWER5,POWER_LEVEL_NA) ||
                 (HasPriv(player,NOTHING,POWER_FULLTEL_ANYWHERE,POWER5,POWER_LEVEL_NA) &&
                  (God(destination) || Cloak(destination) || Going(destination) || 
                   Recover(destination) || !isPlayer(victim) || tel_bool_chk)))) ||
              DePriv(player,Owner(destination),DP_TEL_ANYWHERE,POWER7,NOTHING) ||
              DePriv(player,Owner(destination),DP_LOCKS,POWER6,NOTHING)) &&
              (!Jump_ok(destination) || !could_doit(player, destination, A_LTPORT,1,0))) {

           /* Nope, report failure */
           if (player != victim) {
              notify_quiet(player, "Permission denied.");
           }
           if ( !isExit(victim) && !((Owner(victim) != Owner(cause)) && TelOK(victim)) ) {
              did_it(victim, destination, A_TFAIL, "You can't teleport there!", A_OTFAIL, 0, A_ATFAIL, (char **)NULL, 0);
           }
           if (key != TEL_LIST) {
              continue;
           } else {
              arg1 = strtok_r(NULL, separgs, &tstrtokr);
              continue;
           }
        }

        /* We're OK, do the teleport */
        if (move_via_teleport(victim, destination, player, 0, quiet)) {
           if (player != victim) {
              if (! (Quiet(player) || (side_effect)) ) {
                 notify_quiet(player, "Teleported.");
              }
           }
        }
     } else if (isExit(destination) && !isExit(victim)) {
        if (Exits(destination) == Location(victim)) {
           move_exit(victim, destination, 0, "You can't go that way.", 0);
        } else {
           notify_quiet(player, "I can't find that exit.");
        }
     }

     if (key == TEL_LIST) {
        arg1 = strtok_r(NULL, separgs, &tstrtokr);
     }
  } /* While */
}

/* ---------------------------------------------------------------------------
 * do_force_prefixed: Interlude to do_force for the # command
 */

void do_force_prefixed(dbref player, dbref cause, int key, 
                       char *command, char *args[], int nargs) {
   char *cp;

   cp = parse_to(&command, ' ', 0);

   if (!command) {
       return;
   }

   while (*command && isspace((int)*command)) {
      command++;
   }

   if (*command) {
      if ( (*cp == '#') && *(cp+1) == '#' ) {
         do_force(player, cause, key | FORCE_INLINE, cp+1, command, args, nargs);
      } else {
         do_force(player, cause, key, cp, command, args, nargs);
      }
   }
}

void
do_atrcache_fetch(dbref player, char *s_slot, char *buff, char **bufcx, char **cargs, int ncargs)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff, *retbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (cp->visible || Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         /* Past interval, re-cache the data from executor */
         if ( ((cp->i_interval != 0) && (mudstate.now >= (cp->i_interval + cp->i_lastrun))) ||
              ((cp->i_interval == 0) && (cp->commandtrig == 1)) ) {
         /* Force lock on controller -- if owner is invalid or lock fails, don't re-cache */
            if ( Good_chk(cp->owner) && (Immortal(player) || !cp->lock || (cp->lock && Controls(player, cp->owner))) ) {
               s_tbuff = alloc_lbuf("atrcache_fetch");
               strcpy(s_tbuff, cp->s_cachebuild);
               retbuff = exec(cp->owner, cp->owner, cp->owner, EV_EVAL | EV_FCHECK, s_tbuff, cargs, ncargs, (char **)NULL, 0);
               strcpy(cp->s_cache, retbuff);
               free_lbuf(retbuff);
               free_lbuf(s_tbuff);
               cp->i_lastrun = mudstate.now;
               cp->commandtrig = 0;
            }
         } 
         safe_str(cp->s_cache, buff, bufcx);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_recache(dbref player, char *s_slot, char *buff, char **bufcx, int key, char **cargs, int ncargs)
{
   int i_slot, i_found, i_cnt, i_noshow;
   char *s_tbuff, *retbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   i_noshow = 0;
   if ( key & 2 ) {
      key &= ~2;
      i_noshow = 1;
   }
   if ( !s_slot || !*s_slot ) {
      if ( !i_noshow ) 
         safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         if ( !i_noshow ) 
            safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           ((cp->visible && !cp->lock) || Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         /* Past interval, re-cache the data from executor */
         s_tbuff = alloc_lbuf("atrcache_recache");
         if ( key || 
              ((cp->i_interval != 0) && (mudstate.now >= (cp->i_interval + cp->i_lastrun))) ||
              ((cp->i_interval == 0) && (cp->commandtrig == 1)) ) {
         /* Force lock on controller -- if owner is invalid or lock fails, don't re-cache */
            if ( Good_chk(cp->owner) && (Immortal(player) || !cp->lock || (cp->lock && Controls(player, cp->owner))) ) {
               strcpy(s_tbuff, cp->s_cachebuild);
               retbuff = exec(cp->owner, cp->owner, cp->owner, EV_EVAL | EV_FCHECK, s_tbuff, cargs, ncargs, (char **)NULL, 0);
               strcpy(cp->s_cache, retbuff);
               free_lbuf(retbuff);
               cp->i_lastrun = mudstate.now;
               cp->commandtrig = 0;
            }
         } 
         if ( !i_noshow ) {
            sprintf(s_tbuff, "%ld", cp->i_lastrun);
            safe_str(s_tbuff, buff, bufcx);
         }
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found && !i_noshow ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_interval(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_interval");
         sprintf(s_tbuff, "%ld", cp->i_interval);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_visible(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_visible");
         sprintf(s_tbuff, "%d", cp->visible);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_lock(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_lock");
         sprintf(s_tbuff, "%d", cp->lock);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_lastrun(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_lastrun");
         sprintf(s_tbuff, "%ld", cp->i_lastrun);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_owner(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_owner");
         sprintf(s_tbuff, "#%d", cp->owner);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_execval(dbref player, char *s_slot, char *buff, char **bufcx)
{
   int i_slot, i_found, i_cnt;
   char *s_tbuff;
   ATRCACHE *cp;

   if ( !buff ) {
      return;
   }
   
   if ( !s_slot || !*s_slot ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
      return;
   }

   i_slot = -1;
   if ( isdigit(*s_slot) ) {
      i_slot = atoi(s_slot);
      if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
         safe_str("#-1 INVALID CACHE", buff, bufcx);
         return;
      }
   }

   i_cnt = i_found = 0;
   for ( cp = atrcache_head; cp; cp = cp->next ) {
      if ( !cp->enabled ) {
         i_cnt++;
         continue;
      }
      if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
            ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
           (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
         i_found = 1;
         s_tbuff = alloc_lbuf("atrcache_execval");
         sprintf(s_tbuff, "%s", cp->s_cachebuild);
         safe_str(s_tbuff, buff, bufcx);
         free_lbuf(s_tbuff);
         break;
      }
   }
   if ( !i_found ) {
      safe_str("#-1 INVALID CACHE", buff, bufcx);
   }
}

void
do_atrcache_handler(dbref player, char *s_slot, int key, char *buff, char **bufcx, char **cargs, int ncargs)
{
   switch ( key ) {
      case 0: /* cache */
         do_atrcache_recache(player, s_slot, buff, bufcx, 0, cargs, ncargs);
         break;
      case 1: /* interval */
         do_atrcache_interval(player, s_slot, buff, bufcx);
         break;
      case 2: /* visible */
         do_atrcache_visible(player, s_slot, buff, bufcx);
         break;
      case 3: /* lock */
         do_atrcache_lock(player, s_slot, buff, bufcx);
         break;
      case 4: /* last */
         do_atrcache_lastrun(player, s_slot, buff, bufcx);
         break;
      case 5: /* owner */
         do_atrcache_owner(player, s_slot, buff, bufcx);
         break;
      case 6: /* fetch */
         do_atrcache_fetch(player, s_slot, buff, bufcx, cargs, ncargs);
         break;
      case 7: /* forced recache */
         do_atrcache_recache(player, s_slot, buff, bufcx, 1, cargs, ncargs);
         break;
      case 8: /* exec cache to build */
         do_atrcache_execval(player, s_slot, buff, bufcx);
      case 9: /* grab -- force recache then grab */
         /* passing '3' is a bitmask to snuff output */
         do_atrcache_recache(player, s_slot, buff, bufcx, 3, cargs, ncargs);
         do_atrcache_fetch(player, s_slot, buff, bufcx, cargs, ncargs);
         break;
      default: /* Error */
         safe_str("#-1 UNRECOGNIZED SWITCH", buff, bufcx);
         break;
   }
}

/* ---------------------------------------------------------------------------
 * do_atrcache: Set up an object cache
 */
void do_atrcache(dbref player, dbref cause, int key, char *s_slot,
                    char *command, char *args[], int nargs)
{
   ATRCACHE *cp;
   char *s_tbuff, *retbuff, *tbuf, *tbufptr, *tbuf2;
   char *s_wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL };
   char *s_mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };
   int i_slot, i_cnt, i_found, i_setval, i_cur, i_vis, i_control, i_enabled, i_page, i_noansi;
   struct tm *ttm2;


   
   i_noansi = 0;
   if ( key & ATRCACHE_NOANSI ) {
      i_noansi = 1;
      key &= ~ATRCACHE_NOANSI;
   }
   if ( !key ) {
      key = ATRCACHE_LIST;
   }
   switch (key) {
      case ATRCACHE_INIT: /* initialize cache by slot */
         if ( !s_slot || !*s_slot || !isdigit(*s_slot) ) {
            notify(player, "@atrcache/init: expecting valid slot (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/init: expecting valid name (empty/null).");
            return;
         }
         if ( isdigit(*command) ) {
            notify(player, "@atrcache/init: expecting valid name (can't start with a number).");
            return;
         }
         if ( strlen(command) > 30 ) {
            notify(player, "@atrcache/init: expecting valid name (can't be over 30 characters long).");
            return;
         }
         s_tbuff = command;
         while ( *s_tbuff ) {
            if ( isspace(*s_tbuff) ) {
               notify(player, "@atrcache/init: expecting valid name (whitespace not allowed).");
               return;
            }
            *s_tbuff = ToLower(*s_tbuff);
            s_tbuff++;
         }
         if ( stricmp(command, strip_all_special(command)) ) {
            notify(player, "@atrcache/init: expecting valid name (no special characters allowed).");
            return;
         }
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if (cp->name) {
               if ( !stricmp(cp->name, command) ) {
                  notify(player, "@atrcache/init: expecting valid name (name matches existing cache).");
                  return;
               }
            }
         }
         i_slot = atoi(s_slot);
         if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
            notify(player, "@atrcache/init: expecting valid slot (outside of range).");
            return;
         }
         i_cnt = 0;
         s_tbuff = alloc_lbuf("atrcache_init");
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( i_slot == i_cnt ) {
               if ( cp->enabled == 1 ) {
                  sprintf(s_tbuff, "@atrcache/init: Slot %d [%s] is already enabled and initialized.", i_slot, cp->name);
                  notify(player, s_tbuff);
                  break;
               } else {
                  cp->name = alloc_atrname("atrcache_name");
                  sprintf(cp->name, "%.30s", strip_all_special(command)); 
                  cp->s_cache = alloc_atrcache("atrcache_cache");
                  cp->s_cachebuild = alloc_atrcache("atrcache_exec");
                  cp->owner = player;
                  cp->enabled = 1;
                  cp->commandtrig = 1;
                  sprintf(s_tbuff, "@atrcache/init: Slot %d [%s] has been enabled and initialized.", i_slot, cp->name);
                  notify(player, s_tbuff);
                  break;
               }
            }
            i_cnt++;
         } 
         free_lbuf(s_tbuff);
         break;
      case ATRCACHE_NAME: /* Rename cache by name (or slot) */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/name: Expecting name or slot as target (empty/null).");
            break;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/name: Expecting value for new name (empty/null).");
            break;
         }
         if ( isdigit(*command) ) {
            notify(player, "@atrcache/name: expecting value for new name (can't start with a number).");
            return;
         }
         if ( strlen(command) > 30 ) {
            notify(player, "@atrcache/name: expecting value for new name (> 30 chars long).");
            return;
         }
         s_tbuff = command;
         while ( *s_tbuff ) {
            if ( isspace(*s_tbuff) ) {
               notify(player, "@atrcache/name: expecting value for new name (whitespace not allowed).");
               return;
            }
            *s_tbuff = ToLower(*s_tbuff);
            s_tbuff++;
         }
         if ( stricmp(command, strip_all_special(command)) ) {
            notify(player, "@atrcache/name: expecting value for new name (no special chars allowed).");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/name: expecting valid slot (outside of range).");
               break;
            }
         }

         i_cnt = i_found = 0;
         if ( i_slot != -1 ) {
            for ( cp = atrcache_head; cp; cp = cp->next ) {
               if ( cp->name && !stricmp(cp->name, command) ) {
                  i_found = 1;
                  break;
               }
            }
            if ( i_found ) {
               notify(player, "@atrcache/name: That name is already in use");
               break;
            }
         }
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner)) ) {
               if ( ((i_slot != -1) && (i_cnt == i_slot)) ||
                    ((i_slot == -1) && cp->name && !stricmp(cp->name, s_slot)) ) {
                  retbuff = s_tbuff = alloc_lbuf("atrcache_name");
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/name: Slot %d [%s] renamed to '%s'.", i_cnt, cp->name, strip_all_special(command)));
                  sprintf(cp->name, "%.30s", strip_all_special(command)); 
                  free_lbuf(s_tbuff);
                  i_found = 1;
                  break;
               }
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/name: Can not find matching name or slot to rename.");
         }
         break;
      case ATRCACHE_DELETE: /* Remove a cache by name (or slot) */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/delete: Expecting valid name or slot as target (empty/null).");
            return;
         }
         if ( command && *command ) {
            notify(player, "@atrcache/delete: Command/Target argument not expected.");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/delete: Expecting valid name or slot as target (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( ((i_slot != -1) && (i_cnt == i_slot)) ||
                 ((i_slot == -1) && !stricmp(cp->name, s_slot)) ) {
               i_found = 1;
               if ( Immortal(player) || (Good_chk(cp->owner) && ((player == cp->owner) || Controls(player, cp->owner))) ) {
                  s_tbuff = alloc_lbuf("atrcache_delete");
                  sprintf(s_tbuff, "@atrcache/delete: Cache slot %d [%s] has been de-allocated and uninitialized.", i_cnt, cp->name);
                  notify(player, s_tbuff);
                  free_lbuf(s_tbuff);
                  free_atrcache(cp->s_cachebuild);
                  free_atrcache(cp->s_cache);
                  free_atrname(cp->name);
                  cp->s_cachebuild = NULL;
                  cp->s_cache = NULL;
                  cp->name = NULL;
                  cp->owner = 1;
                  cp->enabled = 0;
                  cp->i_lastrun = 0;
                  cp->commandtrig = 0;
                  break;
               } else {
                  notify(player, "@atrcache/delete: You have no permission on that cache slot.");
                  break;
               }
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/delete: That cache slot is not found or already uninitialized.");
         }
         break;
      case ATRCACHE_FETCH: /* Fetch the value of the cache */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/fetch: Expecting name or slot (empty/null).");
            return;
         }
         if ( command && *command ) {
            notify(player, "@atrcache/fetch: Command/Target argument not expected.");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/fetch: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (cp->visible || Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               i_found = 1;
               /* Past interval, re-cache the data from executor */
               if ( ((cp->i_interval != 0) && (mudstate.now >= (cp->i_interval + cp->i_lastrun))) ||
                    ((cp->i_interval == 0) && (cp->commandtrig == 1)) ) {
                  /* Force lock on controller -- if owner is invalid or lock fails, don't re-cache */
                  /* Logic:
                   *   Immortal -- always recache 
                   *   invalid owner -- don't recache
                   *   Cache is in locked (protected) mode and enactor doesn't control cache owner -- don't recache
                   *   All other conditions, if interval is lapsed, recache before fetch
                   */
                  if ( Good_chk(cp->owner) && (Immortal(player) || !cp->lock || (cp->lock && Controls(player, cp->owner))) ) {
                     /* As exec is destructive, copy the buffer */
                     s_tbuff = alloc_lbuf("atrcache_fetch");
                     strcpy(s_tbuff, cp->s_cachebuild);
                     retbuff = exec(cp->owner, cp->owner, cp->owner, EV_EVAL | EV_FCHECK, s_tbuff, (char **)NULL, 0, (char **)NULL, 0);
                     strcpy(cp->s_cache, retbuff);
                     free_lbuf(retbuff);
                     free_lbuf(s_tbuff);
                     cp->i_lastrun = mudstate.now;
                     cp->commandtrig = 0;
                  }
               } 
               notify(player, cp->s_cache);
               break;
            }
         }
         if ( !i_found ) {
            notify(player, "#-1 NO CACHE FOUND OR NO ACCESS ALLOWED");
         }
         break;
      case ATRCACHE_IVAL: /* Change interval of checking for specific name or slot */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/interval: Expecting name or slot as target (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/interval: Expecting valid interval (empty/null).");
            return;
         }
         i_setval = atoi(command);
         if ( (i_setval < 60) && (i_setval != 0) ) {
            notify(player, "@atrcache/interval: Expecting valid interval (> 60 seconds or 0).");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/interval: Expecting name or slot as target (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               i_found = 1;
               retbuff = s_tbuff = alloc_lbuf("atrcache_interval");
               if ( i_setval == 0 ) {
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/interval: Slot %d [%s] interval changed from %d to %d (per commmand triggered).", 
                          i_cnt, cp->name, cp->i_interval, i_setval));
                  cp->commandtrig = 1;
               } else {
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/interval: Slot %d [%s] interval changed from %d to %d.", 
                          i_cnt, cp->name, cp->i_interval, i_setval));
               }
               cp->i_interval = i_setval;
               free_lbuf(s_tbuff);
               break;
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/interval: Slot not found or permission denied.");
         }
         break;
      case ATRCACHE_CACHE: /* Force a cache update on the cache */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/cache: Expecting name or slot (empty/null).");
            return;
         }
         if ( command && *command ) {
            notify(player, "@atrcache/cache: Command/Target argument not expected.");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/cache: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               /* Do not recache if bad user */
               if ( Good_chk(cp->owner) ) {
                  s_tbuff = alloc_lbuf("atrcache_cache");
                  strcpy(s_tbuff, cp->s_cachebuild);
                  retbuff = exec(cp->owner, cp->owner, cp->owner, EV_EVAL | EV_FCHECK, s_tbuff, (char **)NULL, 0, (char **)NULL, 0);
                  strcpy(cp->s_cache, retbuff);
                  free_lbuf(retbuff);
                  sprintf(s_tbuff, "@atrcache/cache: Cache on slot %d [%s] has been forcefully refreshed.", i_cnt, cp->name);
                  notify(player, s_tbuff);
                  free_lbuf(s_tbuff);
                  cp->i_lastrun = mudstate.now;
                  cp->commandtrig = 0;
                  i_found = 1;
                  break;
               }
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/cache: Slot not found, bad owner, or permission denied.");
         }
         break;
      case ATRCACHE_LIST: /* list all the caches currently in use */
      case ATRCACHE_INUSE: /* list all the caches currently in use */
         i_cnt = i_cur = i_vis = i_control = i_enabled = 0;
         s_tbuff = alloc_lbuf("atrcache_list");
         if ( Wizard(player) ) {
            sprintf(s_tbuff, "Slot %-30s %-10s %-10s %-4s %-4s %-8s", (char *)"Name", (char *)"Interval", (char *)"Fetched", 
                     (char *)"Lock", (char *)"vis", (char *)"State");
         } else {
            sprintf(s_tbuff, "Slot %-30s %-10s %-22s %-10s", (char *)"Name", (char *)"Interval", (char *)"Fetched", (char *)"Control");
         }
         notify(player, s_tbuff);
         notify(player, (char *)"------------------------------------------------------------------------------");

         /* Let's count up the values */
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( cp->enabled ) {
               i_enabled++;
               if ( Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner)) ) {
                  i_vis++;
                  i_control++;
               } else if ( cp->visible ) {
                  i_vis++;
               }
            } else {
               if ( !(key & ATRCACHE_INUSE) && Immortal(player) ) {
                  i_vis++;
                  i_control++;
               }
            }
         }
 
         /* Immortals see all */
         if ( !(key & ATRCACHE_INUSE) && Immortal(player) ) {
            i_vis = mudconf.atrcachemax;
         }

         /* Find page value on what is visible */
         i_found = 0;
         i_page = (i_vis / 20 + 1);
         if ( (i_page != 0) && ((i_vis % 20) == 0) ) {
            i_page--;
         }
         if ( s_slot && *s_slot ) {
            i_found = atoi(s_slot);
            if ( i_found <= 0 )
               i_found = 0;
            if ( i_found >= ((i_vis / 20) + 1) ) {
                i_found = i_page;
            }
         }

         /* We need i_cur to keep track of what is visible to the player */
         i_cur = i_cnt = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( i_vis == 0 ) {
               break;
            }
            if ( cp->enabled ) {
               if ( Wizard(player) ) {
                  if ( (i_found != 0) && ((i_cur < ((i_found - 1) * 20)) || (i_cur >= (i_found * 20))) ) {
                     i_cnt++;
                     i_cur++;
                     continue;
                  }
                  sprintf(s_tbuff, "%03d  %-30s %-10d %-10d %-4d %-4d %s%-8s%s", i_cnt, (cp ? cp->name : (char *)"(NULL)"), 
                          (int)cp->i_interval, (int)cp->i_lastrun, cp->lock, cp->visible, 
#ifdef ZENTY_ANSI
                          (Good_chk(cp->owner) ? SAFE_ANSI_GREEN : SAFE_ANSI_RED),
                          (Good_chk(cp->owner) ? (char *)"Enabled" : (char *)"BadUser"), ANSI_NORMAL);
#else
                          (Good_chk(cp->owner) ? ANSI_GREEN : ANSI_RED),
                          (Good_chk(cp->owner) ? (char *)"Enabled" : (char *)"BadUser"), ANSI_NORMAL);
#endif
                  notify(player, s_tbuff);
                  i_cur++;
               } else if ( cp->visible || Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner)) ) {
                  if ( (i_found != 0) && ((i_cur < ((i_found - 1) * 20)) || (i_cur >= (i_found * 20))) ) {
                     i_cnt++;
                     i_cur++;
                     continue;
                  }
                  sprintf(s_tbuff, "%03d  %-30s %-10d %-10d%-12s %-10s", i_cnt, (cp ? cp->name : (char *)"(NULL)"), 
                          (int)cp->i_interval, (int)cp->i_lastrun,
                           (cp->i_lastrun ? (char *)" " : (char *)" (UnFetchd)"), 
                           (Controls(player, cp->owner) ? (char *)"Yes" : (char *)"No"));
                  notify(player, s_tbuff);
                  i_cur++;
               }   
            } else {
               if ( !(key & ATRCACHE_INUSE) && Immortal(player) ) {
                  if ( (i_found != 0) && ((i_cur < ((i_found - 1) * 20)) || (i_cur >= (i_found * 20))) ) {
                     i_cnt++;
                     i_cur++;
                     continue;
                  }
                  sprintf(s_tbuff, "%03d  %-30s %-10d %-10s %-4d %-4d %-8s", i_cnt, (char *)"(NULL)", 
                            (int)cp->i_interval, (char *)"N/A", 0, 1, (char *)"Disabled");
                  notify(player, s_tbuff);
                  i_cur++;
               }
            }
            i_cnt++;
         }
         sprintf(s_tbuff, "-- Page: %2d/%2d --------------------- %3d Visible, %3d Controlled, %3d Total --", 
                          i_found, i_page, i_vis, i_control, mudconf.atrcachemax);
         notify(player, s_tbuff);
         free_lbuf(s_tbuff);
         break;
      case ATRCACHE_INFO: /* Get information on the specific cache */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/info: Expecting name or slot (empty/null).");
            return;
         }
         if ( command && *command ) {
            notify(player, "@atrcache/info: Command/Target not expected.");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/info: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) &&  
                  (((i_slot != -1) && (i_cnt == i_slot)) ||
                   ((i_slot == -1) && cp->name && !stricmp(cp->name, s_slot))) ) {
               i_found = 1;
               retbuff = s_tbuff = alloc_lbuf("atrcache_info");
               if ( cp->enabled ) {
                  notify(player, "------------------------------------------------------------------------------");
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "Slot: %-3d Name: %-30s   \r\nVisible: %d  Locked: %d  Enabled: True", 
                                 i_cnt, cp->name, cp->visible, cp->lock));
                  if ( Good_chk(cp->owner) ) {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Owner: #%d [%s]", cp->owner, Name(cp->owner)));
                  } else {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Owner: #%d [Invalid Owner -- Will not Re-Cache]", cp->owner));
                  }
                  if ( cp->i_interval == 0 ) {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Interval: %d (command triggered)", cp->i_interval));
                  } else {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Interval: %d", cp->i_interval));
                  }
                  if ( cp->i_lastrun == 0 ) {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Last Fetched: %d [Never Fetched]", cp->i_lastrun));
                  } else {
                     ttm2 = localtime(&cp->i_lastrun);
                     ttm2->tm_year += 1900;
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Last Fetched: %d [%s %s %2d %02d:%02d:%02d %d]", cp->i_lastrun,
                            s_wday[ttm2->tm_wday % 7], s_mon[ttm2->tm_mon % 12], ttm2->tm_mday, ttm2->tm_hour, ttm2->tm_min, ttm2->tm_sec, ttm2->tm_year));
                  }
                  notify(player, "------------------------------------------------------------------------------");
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "Cache: %s", (*(cp->s_cache) ? cp->s_cache : (char *)"(EMPTY)")));
                  notify(player, "------------------------------------------------------------------------------");
                  if ( i_noansi ) {
                     noansi_notify(player, safe_tprintf(s_tbuff, &retbuff, "Exec-: %s", (*(cp->s_cachebuild) ? cp->s_cachebuild : (char *)"(EMPTY)")));
                  } else {
                     tbuf = tbufptr = alloc_lbuf("atrcache_ansi");
                     tbuf2 = alloc_lbuf("atrcache_ansi_scratch");
                     if ( *(cp->s_cachebuild) ) {
                        strcpy(tbuf2, cp->s_cachebuild);
                     } else {
                        strcpy(tbuf2, (char *)"(EMPTY)");
                     }
                     fun_parenstr(tbuf, &tbufptr, player, player, player, &tbuf2, 1, (char **)NULL, 0);
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Exec-: %s", tbuf));
                     free_lbuf(tbuf);
                     free_lbuf(tbuf2);
                  }
                  notify(player, "------------------------------------------------------------------------------");
               } else {
                  notify(player, "------------------------------------------------------------------------------");
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "Slot: %-3d Name: %-30s   \r\nVisible: %d  Locked: %d  Enabled: False", 
                                 i_cnt, (char *)"N/A", cp->visible, cp->lock));
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "Owner: #%d [Default owner always 1]", cp->owner));
                  if ( cp->i_interval == 0 ) {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Interval: %d [Default interval -- command triggered]", cp->i_interval));
                  } else {
                     notify(player, safe_tprintf(s_tbuff, &retbuff, "Interval: %d [Default interval]", cp->i_interval));
                  }
                  notify(player, safe_tprintf(s_tbuff, &retbuff, "Last Fetched: %d [Never Fetched]", cp->i_lastrun));
                  notify(player, "------------------------------------------------------------------------------");
                  notify(player, "Cache: N/A [unitialized]");
                  notify(player, "------------------------------------------------------------------------------");
                  notify(player, "Exec-: N/A [unitialized]");
                  notify(player, "------------------------------------------------------------------------------");
               }
              free_lbuf(s_tbuff);
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "#-1 NO PERMISSION TO VIEW CACHE SLOT");
         }
         break;
      case ATRCACHE_OWNER: /* Change owner of the specific cache */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/owner: Expecting name or slot (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/owner: Expecting target owner (empty/null).");
            return;
         }
	 init_match(player,command,NOTYPE);
	 match_everything(MAT_EXIT_PARENTS);
	 i_setval = (int)noisy_match_result();
         if ( !Good_chk((dbref)i_setval) || !Controls(player, (dbref)i_setval) ) {
            notify(player, "@atrcache/owner: Expecting target owner (invalid owner/no permission).");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/owner: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               i_found = 1;
               retbuff = s_tbuff = alloc_lbuf("atrcache_owner");
               notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/owner: Slot %d [%s] owner changed from #%d [%s] to #%d [%s].", i_cnt, 
                  cp->name, cp->owner, (Good_chk(cp->owner) ? Name(cp->owner) : (char *)"#-1 Bad Owner"), 
                  i_setval, (Good_chk((dbref)i_setval) ? Name((dbref)i_setval) : (char *)"#-1 Bad Owner")));
               free_lbuf(s_tbuff);
               cp->owner = i_setval;
               break;
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/owner: Slot not found or permission denied.");
         }
         break;
      case ATRCACHE_SET: /* Set the value that will cache */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/set: Expecting name or slot (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/set: Expecting EXEC string for cache (empty/null).");
            return;
         }
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/set: Expecting nmae or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( ((i_slot != -1) && (i_cnt == i_slot)) ||
                 ((i_slot == -1) && !stricmp(cp->name, s_slot)) ) {
               i_found = 1;
               if ( Immortal(player) || (Good_chk(cp->owner) && ((player == cp->owner) || Controls(player, cp->owner))) ) {
                  s_tbuff = alloc_lbuf("atrcache_set");
                  sprintf(s_tbuff, "@atrcache/set: Cache value set to slot %d [%s].", i_cnt, cp->name);
                  notify(player, s_tbuff);
                  sprintf(cp->s_cachebuild, "%.*s", (LBUF_SIZE - 10), command);
                  /* We need to reset the timer on caching since it's new values */
                  cp->i_lastrun = 0;
                  free_lbuf(s_tbuff);
                  break;
               } else {
                  notify(player, "@atrcache/set: You have no permission on that cache slot.");
                  break;
               }
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/set: That cache slot is not enabled.");
            break;
         }
         break;
      case ATRCACHE_VIS: /* Is cache visible and executable to everyone? */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/visible: Expecting name or slot (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/visible: Expecting visual toggle of 1 or 0 (empty/null).");
            return;
         }
         i_setval = (atoi(command) ? 1 : 0);
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/visible: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               i_found = 1;
               retbuff = s_tbuff = alloc_lbuf("atrcache_visible");
               notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/visible: Slot %d [%s] visibility changed from %d to %d.", i_cnt, 
                       cp->name, cp->visible, i_setval));
               free_lbuf(s_tbuff);
               cp->visible = i_setval;
               break;
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/visible: Slot not found or permission denied.");
         }
          break;
      case ATRCACHE_LOCK: /* Is cache locked to enforce update cache only on control/owner? */
         if ( !s_slot || !*s_slot ) {
            notify(player, "@atrcache/lock: Expecting name or slot (empty/null).");
            return;
         }
         if ( !command || !*command ) {
            notify(player, "@atrcache/lock: Expecting lock toggle of 1 or 0 (empty/null).");
            return;
         }
         i_setval = (atoi(command) ? 1 : 0);
         i_slot = -1;
         if ( isdigit(*s_slot) ) {
            i_slot = atoi(s_slot);
            if ( (i_slot < 0) || (i_slot >= mudconf.atrcachemax) ) {
               notify(player, "@atrcache/lock: Expecting name or slot (outside range).");
               return;
            }
         }

         i_cnt = i_found = 0;
         for ( cp = atrcache_head; cp; cp = cp->next ) {
            if ( !cp->enabled ) {
               i_cnt++;
               continue;
            }
            if ( (((i_slot != -1) && (i_cnt == i_slot)) ||
                  ((i_slot == -1) && !stricmp(cp->name, s_slot))) &&
                  (Immortal(player) || (Good_chk(cp->owner) && Controls(player, cp->owner))) ) {
               i_found = 1;
               retbuff = s_tbuff = alloc_lbuf("atrcache_lock");
               notify(player, safe_tprintf(s_tbuff, &retbuff, "@atrcache/lock: Slot %d [%s] lock changed from %d to %d.", i_cnt, 
                        cp->name, cp->lock, i_setval));
               free_lbuf(s_tbuff);
               cp->lock = i_setval;
               break;
            }
            i_cnt++;
         }
         if ( !i_found ) {
            notify(player, "@atrcache/lock: Slot not found or permission denied.");
         }
          break;
      default: /* Shouldn't get here but do unhandled exception */
         notify(player, "@atrcache: Unhandled switch.  Get a hardcode dev involved.");
         break;
   }
}

/* ---------------------------------------------------------------------------
 * do_force: Force an object to do something.
 */

void do_force(dbref player, dbref cause, int key, char *what, 
		char *command, char *args[], int nargs)
{
dbref	victim;

	init_match(player,what,NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	victim = noisy_match_result();
	if (!Good_obj(victim) || Recover(victim) || Going(victim) || !Controls(player,victim) ||
  	    ((cause != Owner(victim)) && (HasPriv(victim,player,POWER_NOFORCE,POWER3,NOTHING) || 
  		DePriv(player,victim,DP_FORCE,POWER6,NOTHING)))) { 
	  notify(player,"Permission denied.");
	  return;
	}

        if ( Halted(victim) && ForceHalted(player) ) {
           mudstate.force_halt = 1;
        } else
           mudstate.force_halt = 0;
        /* If /inline call sudo */
        if ( key & FORCE_INLINE ) {
           /* If you want additional keys to @force/inline, use @sudo */
           do_sudo(player, cause, 0, what, command, args, nargs);
        } else {
	   /* force victim to do command */
	   wait_que(victim, player, 0, NOTHING, command, args, nargs,
		   mudstate.global_regs, mudstate.global_regsname);
        }
}

/* ---------------------------------------------------------------------------
 * do_remote: Run a command from a different location.
 */

void do_remote(dbref player, dbref cause, int key, char *loc,
    char *command, char *args[], int nargs)
{
dbref target;
char *retbuff, *s_rollback;
int i_jump, i_rollback;

   if ( mudstate.remote != -1 ) {
      notify(player, "You can not nest @remote.");
      return;
   }
   if ( !command || !*command ) {
      return;
   }
   if ( !loc || !*loc ) {
      target = NOTHING;
   } else {
      retbuff = exec(player, cause, cause, EV_EVAL | EV_FCHECK, loc, (char **)NULL, 0, (char **)NULL, 0);
      target = match_thing(player, retbuff);
      free_lbuf(retbuff);
   }

  if(!Good_obj(target) ||Recover(target) || Going(target) || !Controls(player,target)) {
    notify(player,"Permission denied.");
  }
  mudstate.remote = target;
  mudstate.remotep = player;
  s_rollback = alloc_lbuf("s_rollback_remote");
  strcpy(s_rollback, mudstate.rollback);
  i_jump = mudstate.jumpst;
  i_rollback = mudstate.rollbackcnt;
  mudstate.jumpst = mudstate.rollbackcnt = 0;
  strcpy(mudstate.rollback, command);
  process_command(player, player, 0, command, args, nargs, 0, mudstate.no_hook, mudstate.no_space_compress);
  mudstate.jumpst = i_jump;
  mudstate.rollbackcnt = i_rollback;
  strcpy(mudstate.rollback, s_rollback);
  free_lbuf(s_rollback);
  mudstate.remote = -1;
  mudstate.remotep = -1;
}

/* ---------------------------------------------------------------------------
 * do_turtle: Turn a player into an object with specified name.
 */

void do_turtle(dbref player, dbref cause, int key, char *turtle, char *newowner)
{
dbref	victim, recipient, loc, aowner, aowner2, newplayer;
char	*buf, *s_retval, *s_retplayer, *s_retvalbuff, *s_strtok, *s_strtokr, *s_chkattr, *s_buffptr, *s_mbuf, *tstrtokr;
int	count, aflags, aflags2, i, i_array[LIMIT_MAX];

	init_match(player, turtle, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
        match_player_absolute();
	if ((victim = noisy_match_result()) == NOTHING) return;

	if (!isPlayer(victim)) {
		notify_quiet(player, "Try @destroy instead.");
		return;
	}
	if (God(victim) || Admin(victim)) {
		notify_quiet(player, "You can't turtle that player.");
		return;
	}
	if (DePriv(player,victim,DP_NUKE,POWER6,NOTHING)) {
		notify_quiet(player, "You can't turtle that player.");
		return;
	}
        newplayer = NOTHING;
        if ( isPlayer(player) ) {
           newplayer = player;
        } else {
           newplayer = Owner(player);
           if ( !(Good_obj(newplayer) && isPlayer(newplayer)) )
              newplayer = NOTHING;
        }
        if ( Good_chk(newplayer) ) {
           s_chkattr = atr_get(newplayer, A_DESTVATTRMAX, &aowner2, &aflags2);
           if ( *s_chkattr ) {
              i_array[0] = i_array[2] = 0;
              i_array[4] = i_array[1] = i_array[3] = -2;
              for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &tstrtokr), i = 0;
                   s_buffptr && (i < LIMIT_MAX);
                   s_buffptr = (char *) strtok_r(NULL, " ", &tstrtokr), i++) {
                  i_array[i] = atoi(s_buffptr);
              }
              if ( i_array[3] != -1 ) {
                 if ( (i_array[2]+1) > (i_array[3] == -2 ? (Wizard(newplayer) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) : i_array[3]) ) {
                    notify_quiet(newplayer,"@destruction limit maximum reached.");
                    STARTLOG(LOG_SECURITY, "SEC", "TURTLE")
                      log_text("@destruction limit maximum reached -> Player: ");
                      log_name(newplayer);
                      log_text(" Object: ");
                      log_name(victim);
                    ENDLOG
                    broadcast_monitor(newplayer,MF_VLIMIT,"[TURTLE] DESTROY MAXIMUM",
                            NULL, NULL, victim, 0, 0, NULL);
                    free_lbuf(s_chkattr);
                    return;
                 }
              }
              s_mbuf = alloc_mbuf("vattr_check");
              sprintf(s_mbuf, "%d %d %d %d %d", i_array[0], i_array[1],
                                             i_array[2]+1, i_array[3], i_array[4]);
              atr_add_raw(newplayer, A_DESTVATTRMAX, s_mbuf);
              free_mbuf(s_mbuf);
           } else {
              atr_add_raw(newplayer, A_DESTVATTRMAX, (char *)"0 -2 1 -2 -2");
           }
           free_lbuf(s_chkattr);
        }

        s_retplayer = NULL;
        s_retval = NULL;
        if ( (key & TOAD_UNIQUE) && *newowner && (strchr(newowner, '/') != NULL) ) {
           s_retplayer = (char *)strtok_r(newowner, "/", &s_retvalbuff);
           s_retval = (char *)strip_all_special(strtok_r(NULL, "/", &s_retvalbuff));
           if ( *s_retval && (!ok_name(s_retval) || (strlen(s_retval) > 100)) ) {
              notify(player, "Specified re-name is not a valid object name.");
              return;
           }
        } else {
           s_retplayer = NULL;
           if ( *newowner && (!ok_name(newowner) || (strlen(newowner) > 100)) ) {
              notify(player, "Specified re-name is not a valid object name.");
              return;
           } else
              s_retval = strip_all_special(newowner);
        }
	if ((s_retplayer != NULL) && *s_retplayer) {
		init_match (player, s_retplayer, TYPE_PLAYER);
		match_neighbor ();
		match_absolute ();
		match_player ();
		match_player_absolute ();
		if ((recipient = noisy_match_result ()) == NOTHING)
			return;
	} else {
		recipient = player;
	}

	STARTLOG(LOG_WIZARD,"WIZ","TURTLE")
		log_name_and_loc(victim);
		log_text((char *)" was @turtled by ");
		log_name(player);
	ENDLOG

	/* Clear everything out */

	if (key & TOAD_NO_CHOWN) {
		count = -1;
	} else {
		count = chown_all(victim, recipient, 0);
		s_Owner(victim, recipient);	/* you get it */
	}
	s_Flags(victim, TYPE_THING|HALT);
	s_Flags2(victim, 0);
	s_Flags3(victim, 0);
	s_Flags4(victim, 0);
	s_Toggles(victim, 0);
	s_Toggles2(victim, 0);
	s_Toggles3(victim, 0);
	s_Toggles4(victim, 0);
	s_Pennies(victim, 1);

	/* notify people */

	loc = Location(victim);
	buf=alloc_mbuf("do_turtle");
        if ( *s_retval ) {
	   sprintf(buf, "%s has been turned into a %.100s!", Name(victim), s_retval);
        } else {
	   sprintf(buf, "%s has been turned into a soft-shell turtle!", Name(victim));
        }
	notify_except2(loc, player, victim, player, buf);
	sprintf(buf, "You turtled %s! (%d objects @chowned)", Name(victim),
		count + 1);
	notify_quiet(player, buf);

	/* Zap the name from the name hash table */

	delete_player_name(victim, Name(victim));
        if ( *s_retval )
	   sprintf(buf, "a %.100s named %s", s_retval, Name(victim));
        else
	   sprintf(buf, "a soft-shell turtle named %s", Name(victim));
	s_Name(victim, buf);
	free_mbuf(buf);

	/* Zap the alias too */

	buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
	delete_player_name(victim, buf);
	free_lbuf(buf);

        if ( H_Protect(victim) ) {
           buf = atr_pget(victim, A_PROTECTNAME, &aowner, &aflags);
           s_strtok = strtok_r(buf, "\t", &s_strtokr);
           while ( s_strtok ) {
              (void) protectname_remove(s_strtok, GOD);
              s_strtok = strtok_r(NULL, "\t", &s_strtokr);
           }
           free_lbuf(buf);
           atr_clr(victim, A_PROTECTNAME);
        }

        if ( *s_retval )
	   count = boot_off(victim,
		   unsafe_tprintf("You have been turned into a %.100s!", s_retval));
        else
	   count = boot_off(victim,
		   (char *)"You have been turned into a soft-shell turtle!");
	notify_quiet(player, unsafe_tprintf("%d connection%s closed.",
		count, (count==1 ? "" : "s")));
}

/* ---------------------------------------------------------------------------
 * do_toad: Turn a player into an object.
 */

void do_toad(dbref player, dbref cause, int key, char *toad, char *newowner)
{
dbref	victim, recipient, loc, aowner, aowner2, newplayer;
char	*buf, *s_strtok, *s_strtokr, *s_chkattr, *s_buffptr, *s_mbuf, *tstrtokr;
int	count, aflags, i, i_array[LIMIT_MAX], aflags2;

	init_match(player, toad, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
	match_player_absolute();
	if ((victim = noisy_match_result()) == NOTHING) return;

	if (!isPlayer(victim)) {
		notify_quiet(player, "Try @destroy instead.");
		return;
	}
	if (God(victim) || Admin(victim)) {
		notify_quiet(player, "You can't toad that player.");
		return;
	}
	if (DePriv(player,victim,DP_NUKE,POWER6,NOTHING)) {
		notify_quiet(player, "You can't toad that player.");
		return;
	}
	if ((newowner != NULL) && *newowner) {
		init_match (player, newowner, TYPE_PLAYER);
		match_neighbor ();
		match_absolute ();
		match_player ();
		match_player_absolute ();
		if ((recipient = noisy_match_result ()) == NOTHING)
			return;
	} else {
		recipient = player;
	}

        newplayer = NOTHING;
        if ( isPlayer(player) ) {
           newplayer = player;
        } else {
           newplayer = Owner(player);
           if ( !(Good_obj(newplayer) && isPlayer(newplayer)) )
              newplayer = NOTHING;
        }
        if ( Good_chk(newplayer) ) {
           s_chkattr = atr_get(newplayer, A_DESTVATTRMAX, &aowner2, &aflags2);
           if ( *s_chkattr ) {
              i_array[0] = i_array[2] = 0;
              i_array[4] = i_array[1] = i_array[3] = -2;
              for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &tstrtokr), i = 0;
                   s_buffptr && (i < LIMIT_MAX);
                   s_buffptr = (char *) strtok_r(NULL, " ", &tstrtokr), i++) {
                  i_array[i] = atoi(s_buffptr);
              }
              if ( i_array[3] != -1 ) {
                 if ( (i_array[2]+1) > (i_array[3] == -2 ? (Wizard(newplayer) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) : i_array[3]) ) {
                    notify_quiet(newplayer,"@destruction limit maximum reached.");
                    STARTLOG(LOG_SECURITY, "SEC", "TOAD")
                      log_text("@destruction limit maximum reached -> Player: ");
                      log_name(newplayer);
                      log_text(" Object: ");
                      log_name(victim);
                    ENDLOG
                    broadcast_monitor(newplayer,MF_VLIMIT,"[TOAD] DESTROY MAXIMUM",
                            NULL, NULL, victim, 0, 0, NULL);
                    free_lbuf(s_chkattr);
                    return;
                 }
              }
              s_mbuf = alloc_mbuf("vattr_check");
              sprintf(s_mbuf, "%d %d %d %d %d", i_array[0], i_array[1],
                                             i_array[2]+1, i_array[3], i_array[4]);
              atr_add_raw(newplayer, A_DESTVATTRMAX, s_mbuf);
              free_mbuf(s_mbuf);
           } else {
              atr_add_raw(newplayer, A_DESTVATTRMAX, (char *)"0 -2 1 -2 -2");
           }
           free_lbuf(s_chkattr);
        }

	STARTLOG(LOG_WIZARD,"WIZ","TOAD")
		log_name_and_loc(victim);
		log_text((char *)" was @toaded by ");
		log_name(player);
	ENDLOG

	/* Clear everything out */

	if (key & TOAD_NO_CHOWN) {
		count = -1;
	} else {
		count = chown_all(victim, recipient, 0);
		s_Owner(victim, recipient);	/* you get it */
	}
	s_Flags(victim, TYPE_THING|HALT);
	s_Flags2(victim, 0);
	s_Flags3(victim, 0);
	s_Flags4(victim, 0);
	s_Toggles(victim, 0);
	s_Toggles2(victim, 0);
	s_Toggles3(victim, 0);
	s_Toggles4(victim, 0);
	s_Pennies(victim, 1);

	/* notify people */

	loc = Location(victim);
	buf=alloc_mbuf("do_toad");
	sprintf(buf, "%s has been turned into a slimy toad!", Name(victim));
	notify_except2(loc, player, victim, player, buf);
	sprintf(buf, "You toaded %s! (%d objects @chowned)", Name(victim),
		count + 1);
	notify_quiet(player, buf);

	/* Zap the name from the name hash table */

	delete_player_name(victim, Name(victim));
	sprintf(buf, "a slimy toad named %s", Name(victim));
	s_Name(victim, buf);
	free_mbuf(buf);

	/* Zap the alias too */

	buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
	delete_player_name(victim, buf);
	free_lbuf(buf);

        /* Zap protect name lists */
        if ( H_Protect(victim) ) {
	   buf = atr_pget(victim, A_PROTECTNAME, &aowner, &aflags);
           s_strtok = strtok_r(buf, "\t", &s_strtokr);
           while ( s_strtok ) {
              (void) protectname_remove(s_strtok, GOD);
              s_strtok = strtok_r(NULL, "\t", &s_strtokr);
           }
           free_lbuf(buf);
           atr_clr(victim, A_PROTECTNAME);
        }

	count = boot_off(victim,
		(char *)"You have been turned into a slimy toad!");
	notify_quiet(player, unsafe_tprintf("%d connection%s closed.",
		count, (count==1 ? "" : "s")));
}

void do_newpassword(dbref player, dbref cause, int key, char *name, 
		char *password)
{
   dbref victim;
   char *buf;

   if ((victim = lookup_player(player, name, 0)) == NOTHING) {
      notify_quiet(player, "No such player.");
      return;
   }

   if (*password != '\0' && !ok_password(password, player, 0)) {
      /* Can set null passwords, but not bad passwords */
      notify_quiet(player, "Bad password");
      return;
   }

   if (God(victim) && !God(player) && !(mudconf.newpass_god && Immortal(player))) {
      notify_quiet(player, "You cannot change that player's password.");
      return;
   }

   if (Immortal(victim) && !Immortal(player)) {
      notify(player, "You cannot change an Immortal's password.");
      return;
   }

   if (Wizard(victim) && !Immortal(player)) {
      notify(player, "You cannot change the password of Royalty.");
      return;
   }

   if (Admin(victim) && (player != victim) && !Wizard(player)) {
      notify(player, "You cannot change that player's password.");
      return;
   }

   if (!Immortal(player) && DePriv(player,NOTHING,DP_PASSWORD,POWER8,POWER_LEVEL_NA)) {
      notify_quiet(player, "You cannot change that player's password.");
      return;
   }

   if (!Immortal(player) && !Immortal(victim)  && DePriv(victim,NOTHING,DP_PASSWORD,POWER8,POWER_LEVEL_NA)) {
      notify_quiet(player, "You cannot change that player's password.");
      return;
   }

   STARTLOG(LOG_WIZARD,"WIZ","PASS")
      log_name(player);
      log_text((char *)" changed the password of ");
      log_name(victim);
   ENDLOG

   /* it's ok, do it */
   if ( key & NEWPASSWORD_DES ) {
      s_Pass(victim, mush_crypt((const char *)password, 1));
   } else {
      s_Pass(victim, mush_crypt((const char *)password, 0));
   }
   buf = alloc_lbuf("do_newpassword");
   notify_quiet(player, "Password changed.");
   sprintf(buf, "Your password has been changed by %s.", Name(player));
   notify_quiet(victim, buf);
   if ( mudconf.newpass_god && God(victim)) {
      mudconf.newpass_god = 0;
      notify(player, "The ability to @newpassword #1 has been automatically disabled.");
   }
   free_lbuf(buf);
}

void do_conncheck(dbref player, dbref cause, int key)
{
  DESC *d, *dnext;
  char buff[MBUF_SIZE], buff2[MBUF_SIZE], *tpr_buff, *tprp_buff;

  if ( key & CONNCHECK_ACCT ) {
    notify(player,unsafe_tprintf("%-23s %-16s Dbref  Port    Cmds User@Site","Name", "Account Name"));
  } else if ( key & CONNCHECK_QUOTA ) {
    notify(player,unsafe_tprintf("%-23s Port  %-16s Port CmdQtas User@Site","Name", "Door Name"));
  } else {
    notify(player,unsafe_tprintf("%-23s Port  %-16s Port    Cmds User@Site","Name", "Door Name"));
  }
  DESC_SAFEITER_ALL(d, dnext) {
    // LENSY: FIX ME
    if ( key & CONNCHECK_ACCT ) {
       if ( d->account_owner > 0 ) {
          sprintf(buff2, "%-15.15s #%d", Name(d->account_owner), d->account_owner);
       } else {
         strcpy(buff2, "NONE");
       }
    } else {
       if ( (d->flags & DS_HAS_DOOR) && (d->door_num >= 0) ) {
         sprintf(buff2, "%4d  %-16s", d->door_desc, returnDoorName(d->door_num));
       } else {
         strcpy(buff2, "NONE");
       }
    }
    if (d->flags & DS_CONNECTED) {
      strcpy(buff,Name(d->player));
    } else {
      strcpy(buff,"NONE");
    }
    tprp_buff = tpr_buff = alloc_lbuf("do_dolist");
    if (*(d->userid)) {
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-23s %-22s %4d %7d %s@%s",
             buff, buff2, d->descriptor, ((key & CONNCHECK_QUOTA) ? d->quota : d->command_count), d->userid, d->longaddr));
    } else {
      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-23s %-22s %4d %7d %s",
             buff, buff2, d->descriptor, ((key & CONNCHECK_QUOTA) ? d->quota : d->command_count), d->longaddr));
    }
    free_lbuf(tpr_buff);
  }
}

void do_selfboot(dbref player, dbref cause, int key, char *name)
{
time_t	dtime, now;
int	port, count, count2;
char    *tbuf;
DESC	*d;

  dtime = 0;
  count = 0;
  port = -1;
  switch (key) {
     case SELFBOOT_LIST:   /* List ports you have open */
        tbuf = alloc_lbuf("do_selfboot");
        sprintf(tbuf, "%-10s %-10s %-10s %s\r\n%s", (char *)"Port", (char *)"Idle",
                (char *)"Conn", (char *)"Site",
                (char *)"------------------------------------------------------------------------------");
        notify(player, tbuf);
        DESC_ITER_CONN(d) {
          if (d->player == player) {
             sprintf(tbuf, "%-10d %-10s %-10s %s", d->descriptor, time_format_1(mudstate.now - d->last_time),
                     wiz_time_format_2(mudstate.now - d->connected_at), inet_ntoa(d->address.sin_addr));
             notify(player, tbuf);
          }
        }
        notify(player, (char *)"------------------------------------------------------------------------------");
        free_lbuf(tbuf);
        break;
     case SELFBOOT_PORT:   /* boot specified port */
        if ( !name || !*name ) {
           notify(player, "You must specify a port with the /port switch.");
        } else {
           port = atoi(name);
           count = count2 = 0;
           DESC_ITER_CONN(d) {
             if ( d->player == player )
                count2++;
           }
          
           DESC_ITER_CONN(d) {
             if ( (d->player == player) && (d->descriptor == port) ) {
                if ( count2 > 1 )
	           boot_by_port(d->descriptor,!God(player),player,NULL);
                count++;
             }
           }
           if ( count > 0 ) {
              if ( count2 <= 1 ) {
                 notify(player, "You can't boot your only connection.");
              } else {
                 notify(player,unsafe_tprintf("%d connection(s) closed.", count));
              }
           } else {
              notify(player, "You do not have a connection from that port.");
           }
        }
        break;
     default:              /* default behavior */
        time(&now);
        DESC_ITER_CONN(d) {
          if (d->player == player) {
            if (!count || (now - d->last_time) < dtime) {
	      if (count)
	        boot_by_port(port,!God(player),player,NULL);
	      dtime = now - d->last_time;
	      port = d->descriptor;
            } else {
	      boot_by_port(d->descriptor,!God(player),player,NULL);
            }
            count++;
          }
        }
        if (count < 2) {
          notify(player,"You only have 1 connection.");
        } else {
          notify(player,unsafe_tprintf("%d connection(s) closed.", count - 1));
        }
        break;
   }
}
      

void do_boot(dbref player, dbref cause, int key, char *name)
{
dbref	victim;
char	*buf, *bp;
int	count, lcomp;

	if (!Admin(player) && !HasPriv(player,NOTHING,POWER_BOOT,POWER3,POWER_LEVEL_NA)) {
	  notify(player,"Permission denied.");
	  return;
	}
	if (key & BOOT_PORT) {
		if (is_number(name)) {
			victim = atoi(name);
		} else {
			notify_quiet(player, "That's not a number!");
			return;
		}
	} else {
		init_match(player, name, TYPE_PLAYER);
		match_neighbor();
		match_absolute();
		match_player();
		match_player_absolute();
		if ((victim = noisy_match_result()) == NOTHING) return;

		if (God(victim)) {
			notify_quiet(player, "You cannot boot that player!");
			return;
		}
		if (Immortal(victim) && !Immortal(player)) {
			notify_quiet(player, "You cannot boot that player!");
			return;
		}
		if (DePriv(player,victim,DP_BOOT,POWER6,NOTHING)) {
			notify_quiet(player, "You cannot boot that player!");
			return;
		}

		if (Immortal(player)) lcomp = 5;
		else if (Wizard(player)) lcomp = 4;
		else if (Admin(player)) lcomp = 3;
		else lcomp = HasPriv(player,NOTHING,POWER_BOOT,POWER3,POWER_LEVEL_NA);
		if (!lcomp || HasPriv(victim,NOTHING,POWER_NO_BOOT,POWER3,lcomp)) {
		  notify_quiet(player,"You cannot boot that player!");
		  return;
		}
		if (Wizard(victim) && Admin(player)) {
			notify(player, "You cannot boot that player!");
			return;
		}

		if ((!isPlayer(victim) && !God(player)) ||
			 (player == victim)) {
			notify_quiet(player, "You can only boot off other players!");
			return;
		}

		STARTLOG(LOG_WIZARD,"WIZ","BOOT")
			log_name_and_loc(victim);
			log_text((char *)" was @booted by ");
			log_name(player);
		ENDLOG
		notify_quiet(player, unsafe_tprintf("You booted %s off!", Name(victim)));
	}
	if (key & BOOT_QUIET) {
		buf = NULL;
	} else {
		bp = buf = alloc_lbuf("do_boot.msg");
		safe_str(Name(player), buf, &bp);
		safe_str((char *)" gently shows you the door.", buf, &bp);
		*bp = '\0';
	}
	if (key & BOOT_PORT)
		count = boot_by_port(victim, !God(player), player, buf);
	else
		count = boot_off(victim, buf);

	notify_quiet(player, unsafe_tprintf("%d connection%s closed.",
		count, (count==1 ? "" : "s")));
	if (buf)
		free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * do_poor: Reduce the wealth of anyone over a specified amount.
 */

void do_poor(dbref player, dbref cause, int key, char *arg1)
{
dbref	a;
int	amt, curamt;

	STARTLOG(LOG_WIZARD,"ADM","POOR")
		log_text(unsafe_tprintf("%s attempted a poor with argument: %s\n",
			Name(player), (*arg1 ? arg1 : (char *)"(NULL)") ));
	ENDLOG
	if (!is_number(arg1) || !*arg1) {
		notify(player, "Bad/missing value in @poor command.");
		return;
	}
	amt = atoi(arg1);
	DO_WHOLE_DB(a) {
		if (isPlayer(a) && Controls(player,a)) {
			curamt = Pennies(a);
			if (amt < curamt)
				s_Pennies(a, amt);
		}
	}
	notify(player, unsafe_tprintf("Poor done with value: %d.",amt));
}

/* ---------------------------------------------------------------------------
 * do_cut: Chop off a contents or exits chain after the named item.
 */

void do_cut(dbref player, dbref cause, int key, char *thing)
{
dbref	object;

	object = match_controlled(player, thing);
	switch (object) {
	case NOTHING:
		notify_quiet(player, "No match.");
		break;
	case AMBIGUOUS:
		notify_quiet(player, "I don't know which one");
		break;
	default:
		s_Next(object, NOTHING);
		notify_quiet(player, "Cut.");
	}
}

#define MAXQUOTASET	1999

void q_count(dbref who, int cq[])
{
  dbref i;

  DO_WHOLE_DB(i) {
    if (Owner(i) != who)
      continue;
    if (Going(i) || Recover(i))
      continue;
    switch (Typeof(i)) {
	case TYPE_EXIT:		cq[3] += mudconf.exit_quota;	break;
	case TYPE_ROOM:		cq[1] += mudconf.room_quota;	break;
	case TYPE_THING:	cq[2] += mudconf.thing_quota;	break;
	case TYPE_PLAYER:	cq[4] += mudconf.player_quota;	break;
    }
  }
}

void q_fix(dbref who, int redo)
{
  int rq[5] = { 0, 0, 0, 0, 0 }, 
      i, 
      flags, 
      cq[5] = { 0, 0, 0, 0, 0 }, 
      sq[5] = { 0, 0, 0, 0, 0 };
  dbref owner;
  char *atr1;
  int total, altq;

  total = 0;
  altq = Altq(who);
  q_count(who, cq);
  if (!altq) {
    total = 0;
    for (i = 0; i < 5; i++)
      total += cq[i];
  }
  if (redo) {
    if (altq) 
      atr_add_raw(who,A_RQUOTA,"0 0 0 0 0");
    else
      atr_add_raw(who,A_RQUOTA,"0");
    atr1 = alloc_lbuf("q_fix");
  }
  else {
    atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
    if (altq)
      sscanf(atr1,"%d %d %d %d %d",&rq[0],&rq[1],&rq[2],&rq[3],&rq[4]);
    else
      rq[0] = atoi(atr1);
  }
  sq[0] = rq[0];
  if (!altq) {
    sq[0] += total;
    sprintf(atr1,"%d",sq[0]);
    atr_add_raw(who,A_QUOTA,atr1);
    free_lbuf(atr1);
    return;
  }
  for (i = 1; i < 5; i++)
    sq[i] = rq[i] + cq[i];
  sprintf(atr1,"%d %d %d %d %d",sq[0],sq[1],sq[2],sq[3],sq[4]);
  atr_add_raw(who,A_QUOTA,atr1);
  for (i = 0; i < 5; i++) {
    sq[i] = sq[i] * 2;
    if (i != 4) {
      if (sq[i] < 5) sq[i] = 5;
    }
    if (sq[i] > MAXQUOTASET) sq[i] = MAXQUOTASET;
  }
  sq[0] = 10;
  sprintf(atr1,"%d %d %d %d %d",sq[0],sq[1],sq[2],sq[3],sq[4]);
  atr_add_raw(who,A_MQUOTA,atr1);
  free_lbuf(atr1);
}

void quota_fix(dbref player, dbref who, int redo)
{
  dbref i;

  if (who != NOTHING)
    q_fix(who, redo);
  else {
    DO_WHOLE_DB(i) {
      if (Going(i) || Recover(i) || !isPlayer(i))
	continue;
      else
	q_fix(i, redo);
    }
  }
  if (!redo)
    notify(player,"Fixed.");
}

void q_redo(dbref who)
{
  if (Altq(who)) {
    atr_add_raw(who,A_LQUOTA,"0 0 0 0 0");
    atr_clr(who,A_TQUOTA);
  }
}

void quota_redo(dbref player, dbref who)
{
  dbref i;

  quota_fix(player, who, 1);
  if (who != NOTHING)
    q_redo(who);
  else {
    DO_WHOLE_DB(i) {
      if (Going(i) || Recover(i) || !isPlayer(i))
	continue;
      else
	q_redo(i);
    }
  }
  notify(player,"Fixed.");
}

void quota_dis(dbref player, dbref who, int qswitch[], int qset, char *bf, char **bfx)
{
  char *atr1, *pt1, *pt2, l1, *tpr_buff, *tprp_buff;
  dbref owner;
  int flags, count, ucheck, i;
  int q[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int d[5] = { 0, 0, 0, 0, 0 };
  int l[5] = { 0, 0, 0, 0, 0 };
  char *names[] = {"General",
		   "Room",
		   "Thing",
		   "Exit",
		   "Player"};

  pt2 = NULL;
  tprp_buff = tpr_buff = alloc_lbuf("quota_dis");
  if (!Altq(who)) {
    atr1 = atr_get(who, A_QUOTA, &owner, &flags);
    q[0] = atoi(atr1);
    free_lbuf(atr1);
    atr1 = atr_get(who, A_RQUOTA, &owner, &flags);
    q[1] = atoi(atr1);
    free_lbuf(atr1);
    if (Wizard(player)) {
      atr1 = atr_get(who, A_LQUOTA, &owner, &flags);
      if (atoi(atr1) == 1)
	l1 = '*';
      else
	l1 = ' ';
      free_lbuf(atr1);
    }
    else
      l1 = ' ';
    if ((Builder(who) || HasPriv(who,NOTHING,POWER_FREE_QUOTA,POWER3,POWER_LEVEL_NA)) &&
	(!DePriv(who,NOTHING,DP_UNL_QUOTA,POWER7,POWER_LEVEL_NA))) { 
      if (bf == NULL)
        notify(player,safe_tprintf(tpr_buff, &tprp_buff, 
               "Quota for %-16.16s  Quota: UNLIM  Used: %-5d  Remaining: UNLIM",
               Name(who),q[0]-q[1]));
      else
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "U %d U",q[0]-q[1]),bf,bfx);
    }
    else {
      if (bf == NULL)
        notify(player,safe_tprintf(tpr_buff, &tprp_buff, 
               "Quota for %-16.16s %cQuota: %-5d  Used: %-5d  Remaining: %d",
               Name(who),l1,q[0],q[0]-q[1],q[1]));
      else  {
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d",q[0],q[0]-q[1],q[1]),bf,bfx);
	if (l1 == '*')
	  safe_str(" L",bf,bfx);
      }
    }
    free_lbuf(tpr_buff);
    return;
  }
  atr1 = atr_get(who, A_QUOTA, &owner, &flags);
  sscanf(atr1,"%d %d %d %d %d",&q[0],&q[1],&q[2],&q[3],&q[4]);
  free_lbuf(atr1);
  atr1 = atr_get(who, A_RQUOTA, &owner, &flags);
  sscanf(atr1,"%d %d %d %d %d",&q[5],&q[6],&q[7],&q[8],&q[9]);
  free_lbuf(atr1);
  atr1 = atr_get(who, A_LQUOTA, &owner, &flags);
  sscanf(atr1,"%d %d %d %d %d",&l[0],&l[1],&l[2],&l[3],&l[4]);
  free_lbuf(atr1);
  if (!Wizard(player)) {
    for (flags = 0; flags < 5; flags++) {
      d[flags] = q[flags] - q[flags + 5];
      l[flags] = 0;
    }
  }
  else {
    for (flags = 0; flags < 5; flags++)
      d[flags] = q[flags] - q[flags + 5];
  }
  count = 1;
  ucheck = 0;
  pt1 = alloc_lbuf("quota_dis");
  if (Privilaged(player)) {
    if ((!Builder(who) && !HasPriv(who,NOTHING,POWER_FREE_QUOTA,POWER3,POWER_LEVEL_NA)) ||
	(DePriv(who,NOTHING,DP_UNL_QUOTA,POWER7,POWER_LEVEL_NA))) {
      atr1 = atr_get(who, A_MQUOTA, &owner, &flags);
      sscanf(atr1,"%d %d %d %d %d",&q[10],&q[11],&q[12],&q[13],&q[14]);
      free_lbuf(atr1);
      ucheck = 1;
    }
    sprintf(pt1,"Quota for %s, (Type: [Used/Set/Max])\r\n",Name(who));
    if (ucheck) {
      if (bf == NULL)
        pt2 = safe_tprintf(tpr_buff, &tprp_buff, " Total: [%d/%d/%d]",d[0]+d[1]+d[2]+d[3]+d[4],
					q[0]+q[1]+q[2]+q[3]+q[4],
					q[10]+q[11]+q[12]+q[13]+q[14]);
      else
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d",d[0]+d[1]+d[2]+d[3]+d[4],q[0]+q[1]+q[2]+q[3]+q[4],
					q[10]+q[11]+q[12]+q[13]+q[14]),bf,bfx); 
    }
    else {
      if (bf == NULL)
        pt2 = safe_tprintf(tpr_buff, &tprp_buff, " Total: [%d/U/U]",d[0]+d[1]+d[2]+d[3]+d[4]);
      else
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d U U",d[0]+d[1]+d[2]+d[3]+d[4]),bf,bfx);
    }
    if (bf == NULL) {
      for (i = strlen(pt2); i < 27; i++) {
        *(pt2 + i) = ' ';
      }
      *(pt2 + 27) = '\0';
      strcat(pt1,pt2);
    }
    for (flags = 0; flags < 5; flags++) {
      if (!qset || qswitch[flags]) {
	count++;
        if (ucheck) {
	  if (bf == NULL)
            pt2 = safe_tprintf(tpr_buff, &tprp_buff, " %s: [%d/%d/%d]",names[flags],d[flags],q[flags],q[flags+10]);
	  else  {
	    safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %d %d %d",d[flags],q[flags],q[flags+10]),bf,bfx);
	    if (l[flags])
	      safe_str(" L",bf,bfx);
	  }
        }
        else {
	  if (bf == NULL)
	    pt2 = safe_tprintf(tpr_buff, &tprp_buff, " %s: [%d/U/U]",names[flags],d[flags]);
	  else  {
	    safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %d U U",d[flags]),bf,bfx);
	    if (l[flags])
	      safe_str(" L",bf,bfx);
	  }
	}
	if (bf == NULL) {
	  if (l[flags])
	    *pt2 = '*';
	  if (count % 3) {
	    for (i = strlen(pt2); i < 27; i++) {
	      *(pt2 + i) = ' ';
	    }
	    *(pt2 + 27) = '\0';
	  }
	  strcat(pt1,pt2);
	  if (count == 3)
	    strcat(pt1,"\r\n");
	}
      }
    }
  }
  else {
    if ((!Builder(who) && !HasPriv(who,NOTHING,POWER_FREE_QUOTA,POWER3,POWER_LEVEL_NA)) ||
	(DePriv(player,NOTHING,DP_UNL_QUOTA,POWER7,POWER_LEVEL_NA))) {
      ucheck = 1;
    }
    sprintf(pt1,"Quota for %s, (Type: [Used/Set])\r\n",Name(who));
    if (ucheck) {
      if (bf == NULL)
        pt2 = safe_tprintf(tpr_buff, &tprp_buff, " Total: [%d/%d]",d[0]+d[1]+d[2]+d[3]+d[4],
					q[0]+q[1]+q[2]+q[3]+q[4]);
      else
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d",d[0]+d[1]+d[2]+d[3]+d[4],q[0]+q[1]+q[2]+q[3]+q[4]),bf,bfx);
    }
    else {
      if (bf == NULL)
        pt2 = safe_tprintf(tpr_buff, &tprp_buff, " Total: [%d/U]",d[0]+d[1]+d[2]+d[3]+d[4]);
      else
	safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d U",d[0]+d[1]+d[2]+d[3]+d[4]),bf,bfx);
    }
    if (bf == NULL) {
      for (i = strlen(pt2); i < 27; i++) {
        *(pt2 + i) = ' ';
      }
      *(pt2 + 27) = '\0';
      strcat(pt1,pt2);
    }
    for (flags = 0; flags < 5; flags++) {
      if (!qset || qswitch[flags]) {
	count++;
        if (ucheck) {
	  if (bf == NULL)
            pt2 = safe_tprintf(tpr_buff, &tprp_buff, " %s: [%d/%d]",names[flags],d[flags],q[flags]);
	  else 
	    safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %d %d",d[flags],q[flags]),bf,bfx);
        }
        else {
	  if (bf == NULL)
	    pt2 = safe_tprintf(tpr_buff, &tprp_buff, " %s: [%d/U]",names[flags],d[flags]);
	  else 
	    safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %d U",d[flags]),bf,bfx);
	}
	if (bf == NULL) {
	  if (count % 3) {
	    for (i = strlen(pt2); i < 27; i++) {
	      *(pt2 + i) = ' ';
	    }
	    *(pt2 + 27) = '\0';
	  }
	  strcat(pt1,pt2);
	  if (count == 3)
	    strcat(pt1,"\r\n");
	}
      }
    }
  }
  if (bf == NULL)
    notify(player,pt1);
  free_lbuf(tpr_buff);
  free_lbuf(pt1);
}

int quota_set(dbref player, dbref who, int key, int qswitch[], int qset, char *cargs[], int nargs, int all)
{
  char *atr1, *atr2;
  dbref owner;
  int flags, i;
  int s[5] = { 0, 0, 0, 0, 0 };
  int d[5] = { 0, 0, 0, 0, 0 };
  int m[5] = { 0, 0, 0, 0, 0 };
  int r[5] = { 0, 0, 0, 0, 0 };
  int t[5] = { 0, 0, 0, 0, 0 };

  if (!Altq(who)) { 
    if (key || qset) {
      if (!all)
        notify(player,"Permission denied.");
      return 0;
    }
    if (!Wizard(player) || mudconf.must_unlquota) {
      atr1 = atr_get(who,A_LQUOTA,&owner,&flags);
      flags = atoi(atr1);
      free_lbuf(atr1);
      if (flags == 1) {
        if ( Wizard(player) )
	   notify(player,"You must unlock their @quota first.");
        else
	   notify(player,"Permission denied.");
	return 0;
      }
    }
    s[0] = atoi(cargs[0]);
    if ((s[0] < 0) || (s[0] > MAXQUOTASET)) {
      if ( s[0] > MAXQUOTASET )
         notify(player, unsafe_tprintf("New quota must be no higher than %d", MAXQUOTASET));
      else
         notify(player, "New quota must be higher than 0.");
      return 0;
    }
    atr1 = atr_get(who,A_QUOTA,&owner,&flags);
    d[0] = atoi(atr1);
    free_lbuf(atr1);
    atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
    d[1] = atoi(atr1);
    s[1] = d[1] + s[0] - d[0];
    sprintf(atr1,"%d",s[0]);
    atr_add_raw(who,A_QUOTA,atr1);
    sprintf(atr1,"%d",s[1]);
    atr_add_raw(who,A_RQUOTA,atr1);
    free_lbuf(atr1);
    return 1;
  }
  if (qset) {
    for (i = 0; i < 5; i++) {
      if (qswitch[i]) {
	if (!is_number(cargs[0]))
	  s[i] = -1;
	else
	  s[i] = atoi(cargs[0]);
      }
      else {
	s[i] = -1;
      }
    }
  }
  else {
    for (i = 0; i < nargs; i++) {
      if (!is_number(cargs[i]))
	s[i] = -1;
      else
	s[i] = atoi(cargs[i]);
    }
    for (i = nargs; i < 5; i++) 
      s[i] = -1;
  }
  for (i = 0; i < 5; i++)
    d[i] = m[i] = r[i] = 0;
  if (key) {
    atr1 = atr_get(who,A_MQUOTA,&owner,&flags);
    sscanf(atr1,"%d %d %d %d %d",&d[0],&d[1],&d[2],&d[3],&d[4]);
    atr2 = atr_get(who,A_QUOTA,&owner,&flags);
    sscanf(atr2,"%d %d %d %d %d",&m[0],&m[1],&m[2],&m[3],&m[4]);
    free_lbuf(atr2);
    for (i = 0; i < 5; i++) {
      if (s[i] == -1)
	s[i] = d[i];
      else if (s[i] < m[i])
	s[i] = m[i];
      else if (s[i] > MAXQUOTASET)
	s[i] = MAXQUOTASET;
    }
    sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
    atr_add_raw(who,A_MQUOTA,atr1);
    free_lbuf(atr1);
  }
  else {
    atr1 = atr_get(who,A_QUOTA,&owner,&flags);
    sscanf(atr1,"%d %d %d %d %d",&d[0],&d[1],&d[2],&d[3],&d[4]);
    for (i = 0; i < 5; i++) {
      if (s[i] < 0)
	s[i] = d[i];
      else if (s[i] > MAXQUOTASET)
	s[i] = MAXQUOTASET;
    }
    atr2 = atr_get(who,A_MQUOTA,&owner,&flags);
    sscanf(atr2,"%d %d %d %d %d",&m[0],&m[1],&m[2],&m[3],&m[4]);
    for (i = 0; i < 5; i++) {
      if (s[i] > m[i]) {
	if (Wizard(player)) {
	  m[i] = s[i];
	}
	else {
	  s[i] = m[i];
	}
      }
      r[i] = s[i] - d[i];
      d[i] = 0;
    }
    free_lbuf(atr1);
    atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
    sscanf(atr1,"%d %d %d %d %d",&d[0],&d[1],&d[2],&d[3],&d[4]);
    for (i = 0; i < 5; i++) {
      t[i] = d[i]+r[i];
      if (t[i] < 0) {
	s[i] -= t[i];
	t[i] = 0;
      }
    }
    sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
    sprintf(atr2,"%d %d %d %d %d",m[0],m[1],m[2],m[3],m[4]);
    atr_add_raw(who,A_QUOTA,atr1);
    atr_add_raw(who,A_MQUOTA,atr2);
    sprintf(atr1,"%d %d %d %d %d",t[0],t[1],t[2],t[3],t[4]);
    atr_add_raw(who,A_RQUOTA,atr1);
    free_lbuf(atr1);
    free_lbuf(atr2);
  }
  return 1;
}

void quota_set_dis(dbref player, dbref who, int all, int key, int qswitch[], int qset, char *cargs[], int nargs, int altq)
{
  dbref i;

  if (!nargs) {
    if (!all) {
      if (Examinable(player,who) || (!Builder(player) && HasPriv(player,NOTHING,POWER_CHANGE_QUOTAS,POWER3,POWER_LEVEL_NA))) 
        quota_dis(player, who, qswitch, qset, NULL, NULL);
      else
	notify(player,"Permission denied.");
    }
    else {
      DO_WHOLE_DB(i) {
	if (Going(i) || Recover(i) || !isPlayer(i))
	  continue;
        if (Examinable(player,i) || (!Builder(player) && HasPriv(player,NOTHING,POWER_CHANGE_QUOTAS,POWER3,POWER_LEVEL_NA))) {
          quota_dis(player, i, qswitch, qset, NULL, NULL);
	}
      }
    }
  }
  else {
    if (!Immortal(player) && (DePriv(player,NOTHING,DP_QUOTA,POWER7,POWER_LEVEL_NA))) {
      notify(player,"Permission denied.");
      return;
    }
    if (!all) {
      if ((Privilaged(player) && 
           (Controls(player,who) || 
            !Privilaged(who))) || 
          (!Privilaged(who) && 
           (HasPriv(player,NOTHING,POWER_CHANGE_QUOTAS,
                    POWER3,POWER_LEVEL_NA)))) {
	if (quota_set(player, who, key, qswitch, qset, cargs, nargs, 0)) {
	  notify(player,"Set.");
	  STARTLOG(LOG_WIZARD,"WIZ","QUOTA")
 	    log_name(player);
	    log_text((char *)" changed the quota of ");
	    log_name(who);
	  ENDLOG
	}
      }
      else
	notify(player,"Permission denied.");       
    }
    else {
      DO_WHOLE_DB(i) {
	if (Going(i) || Recover(i) || !isPlayer(i))
	  continue;
        if (Controls(player,who) || 
            (!Builder(player) && 
             HasPriv(player,NOTHING,POWER_CHANGE_QUOTAS,
                     POWER3,POWER_LEVEL_NA))) {
	  quota_set(player, who, key, qswitch, qset, cargs, nargs, 1);
	}
      }
      STARTLOG(LOG_WIZARD,"WIZ","QUOTA")
        log_name(player);
        log_text((char *)" changed everyone's quota");
      ENDLOG
      notify(player,"Done.");
    }
  }
}

void q_lock(dbref who, int key, int qswitch[])
{
  char *atr1;
  int flags, i;
  dbref owner;
  int l[5] = { 0, 0, 0, 0, 0 };

  if (Altq(who)) {
    atr1 = atr_get(who,A_LQUOTA,&owner,&flags);
    sscanf(atr1,"%d %d %d %d %d",&l[0],&l[1],&l[2],&l[3],&l[4]);
    for (i = 0; i < 5; i++) {
      if (qswitch[i]) {
        if (key == QUOTA_LOCK)
          l[i] = 1;
        else
          l[i] = 0;
      }
    }
    sprintf(atr1,"%d %d %d %d %d",l[0],l[1],l[2],l[3],l[4]);
    atr_add_raw(who,A_LQUOTA,atr1);
    free_lbuf(atr1);
  }
  else {
    if (key == QUOTA_LOCK)
      atr_add_raw(who,A_LQUOTA,"1");
    else
      atr_clr(who,A_LQUOTA);
  }
}

void quota_lock(dbref player, dbref who, int all, int key, int qswitch[], int qset)
{
  dbref i;
  char *tbuf;

  if (!all) {
    if (!Controls(player,who)) {
      notify(player,"Permission denied.");
      return;
    }
    if (Wizard(who)) {
      notify(player,"That command is meaningless on Royalty.");
      return;
    }
    if (!qset && Altq(who)) {
      notify(player,"Permission denied.");
      return;
    }
    q_lock(who, key, qswitch);
  }
  else {
    DO_WHOLE_DB(i) {
      if (Going(i) || Recover(i) || !isPlayer(i) || !Controls(player,i) || Wizard(i))
	continue;
      else if (!qset && Altq(who)) 
	continue;
      else
	q_lock(i, key, qswitch);
    }
  }
  if (key == QUOTA_LOCK) {
    if ( TogNoisy(player) ) {
       tbuf = alloc_lbuf("quota_lock");
       sprintf(tbuf, "Locked - %s/quotalock", Name(player));
       notify(player, tbuf);
       free_lbuf(tbuf);
    } else {
       notify(player,"Locked.");
    }
  } else {
    if ( TogNoisy(player) ) {
       tbuf = alloc_lbuf("quota_lock");
       sprintf(tbuf, "Unlocked - %s/quotalock", Name(player));
       notify(player, tbuf);
       free_lbuf(tbuf);
    } else {
       notify(player,"Unlocked.");
    }
  }
}

void quota_take(dbref player, dbref who, int key, char *cargs[], int nargs)
{
  int i, flags;
  dbref owner;
  char *buff;

  if (!nargs) {
    if (key) {
      atr_clr(who,A_TQUOTA);
      notify(player,"Take list cleared.");
    }
    else {
      buff = atr_get(who,A_TQUOTA,&owner,&flags);
      if (buff)
        notify(player,unsafe_tprintf("Take order is: %s",buff));
      else
	notify(player,"Take order not set.");
      free_lbuf(buff);
    }
  }
  else {
    buff = alloc_lbuf("quota_take");
    *buff = '\0';
    for (i = 0; i < nargs; i++) {
      if (strlen(cargs[i]) != 1) {
	notify(player,"Unknown item in take list.");
      }
      else {
	switch (toupper(*cargs[i])) {
	  case 'R':  	strcat(buff,"R ");
			break;
	  case 'T':	strcat(buff,"T ");
			break;
	  case 'E':	strcat(buff,"E ");
			break;
	  case 'P':	strcat(buff,"P ");
			break;
	  default:	notify(player,"Unknown item in take list.");
	}
      }
    }
    i = strlen(buff);
    if (!i) {
      notify(player,"Take list not changed.");
    }
    else {
      *(buff + i - 1) = '\0';
      atr_add_raw(who,A_TQUOTA,buff);
      notify(player,unsafe_tprintf("Take order set to: %s",buff));
    }
    free_lbuf(buff);
  }
}

int q_check(dbref player, dbref who, int amount, char src, char dest, int call, int pay)
{
  char *atr1;
  int flags, snum, dnum;
  dbref owner;
  int s[5] = { 0, 0, 0, 0, 0 };
  int d[5] = { 0, 0, 0, 0, 0 };
  int l[5] = { 0, 0, 0, 0, 0 };
  int m[5] = { 0, 0, 0, 0, 0 };

  if (amount < 1)
    return 0;
  switch (toupper(src)) {
    case 'G': snum = 0; break;
    case 'R': snum = 1; break;
    case 'T': snum = 2; break;
    case 'E': snum = 3; break;
    case 'P': snum = 4; break;
    default: return 0;
  }
  switch (toupper(dest)) {
    case 'G': dnum = 0; break;
    case 'R': dnum = 1; break;
    case 'T': dnum = 2; break;
    case 'E': dnum = 3; break;
    case 'P': dnum = 4; break;
    default: return 0;
  }
  atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
  sscanf(atr1,"%d %d %d %d %d",&s[0],&s[1],&s[2],&s[3],&s[4]);
  free_lbuf(atr1);
  if ((s[snum] - amount) < 0)
    return 0;
  atr1 = atr_get(who,A_LQUOTA,&owner,&flags);
  sscanf(atr1,"%d %d %d %d %d",&l[0],&l[1],&l[2],&l[3],&l[4]);
  free_lbuf(atr1);
  if (l[dnum] && !Wizard(player))
    return 0;
  atr1 = atr_get(who,A_QUOTA,&owner,&flags);
  sscanf(atr1,"%d %d %d %d %d",&d[0],&d[1],&d[2],&d[3],&d[4]);
  free_lbuf(atr1);
  atr1 = atr_get(who,A_MQUOTA,&owner,&flags);
  sscanf(atr1,"%d %d %d %d %d",&m[0],&m[1],&m[2],&m[3],&m[4]);
  if ((d[dnum] + amount) > m[dnum]) {
    if (Wizard(player) && pay) {
      m[dnum] = d[dnum] + amount;
      sprintf(atr1,"%d %d %d %d %d",m[0],m[1],m[2],m[3],m[4]);
      atr_add_raw(who,A_MQUOTA,atr1);
    }
    else {
      free_lbuf(atr1);
      return 0;
    }
  }
  s[snum] -= amount;
  if (call)
    s[dnum] = 0;
  else
    s[dnum] += amount;
  d[dnum] += amount;
  d[snum] -= amount;
  if (pay) {
    sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
    atr_add_raw(who,A_RQUOTA,atr1);
    sprintf(atr1,"%d %d %d %d %d",d[0],d[1],d[2],d[3],d[4]);
    atr_add_raw(who,A_QUOTA,atr1);
  }
  free_lbuf(atr1);
  return 1;
}
  
void quota_xfer(dbref player, dbref who, char *amount, char *src, char *dest)
{
  int num, rval;
  
  if (!is_number(amount)) rval = 0;
  else {
    num = atoi(amount);
    if ((strlen(src) != 1) || (strlen(dest) != 1)) rval = 0;
    else rval = q_check(player, who, num, *src, *dest, 0, 1);
  }
  if (rval)
    notify(player,"Quota transferred.");
  else
    notify(player,"Transfer failed.");
}

void do_quota(dbref player, dbref cause, int key, char *name, char *cargs[], int nargs)
{
  dbref who;
  int qswitch[5] = { 0, 0, 0, 0, 0 };
  int i, qset, xfer, all, clear, nargs2, alt;
  char *nbuff, *pt1, *pt2;

  if ( mudconf.quotas == 0 ) {
     notify(player, "Building quotas are not in effect.");
     return;
  }
  if (nargs == 1) {
    if (!(*cargs[0])) 
      nargs2 = 0;
    else
      nargs2 = 1;
  }
  else 
    nargs2 = nargs;
  if (key & QUOTA_CLEAR) {
    clear = 1;
    key &= ~QUOTA_CLEAR;
  }
  else
    clear = 0;

  if( key & QUOTA_XFER ) {
    xfer = 1;
  }
  else {
    xfer = 0;
  }
  if (key & QUOTA_SET)
    key &= ~QUOTA_SET;
  if (key & QUOTA_GEN) qswitch[0] = 1; 
  if (key & QUOTA_THING) qswitch[2] = 1;  
  if (key & QUOTA_PLAYER) qswitch[4] = 1; 
  if (key & QUOTA_EXIT) qswitch[3] = 1;
  if (key & QUOTA_ROOM) qswitch[1] = 1;
  qset = (qswitch[0] || qswitch[1] || qswitch[2] || qswitch[3] || qswitch[4]); 
  if (key & QUOTA_ALL) {
    if (qset) {
      notify(player,"Illegal combination of switches.");
      return;
    }
    else {
      for (i = 0; i < 5; i++) *(qswitch + i) = 1;
      qset = 1;
    }
  }
  if ((qset && (nargs2 > 1)) || (nargs2 > 5)) {
    notify(player,"Invalid quota format.");
    return;
  }
  key &= (~(QUOTA_GEN | QUOTA_THING | QUOTA_PLAYER |
	    QUOTA_EXIT | QUOTA_ROOM | QUOTA_ALL));
  if ((key != QUOTA_MAX) && (key != QUOTA_LOCK) && (key != QUOTA_UNLOCK) && (key != 0) && qset) {
    notify(player,"Illegal combination of switches.");
    return;
  }
  nbuff = alloc_lbuf("do_quota");
  pt1 = nbuff;
  pt2 = name;
  while (*pt2) *pt1++ = toupper(*pt2++);
  *pt1 = '\0';
  if (strcmp(nbuff,"ALL") == 0) {
    who = NOTHING;
    all = 1;
  }
  else  {
    all = 0;
    if (*name || nargs2)
      who = lookup_player(player,name,0);
    else
      who = player;
  }
  free_lbuf(nbuff);
  if ((who == NOTHING) && !all) {
    notify(player,"I don't know that player.");
    return;
  }
  if (!all) {
    alt = Altq(who);
    if (!alt && (qset || xfer)) {
      if (Controls(player,who))
        notify(player,"Alternate quota is not in effect for that player.");
      else
        notify(player,"Permission denied.");
      return;
    }
  }
  else
    alt = 0;
  switch (key) {
    case 0:
    case QUOTA_MAX:
	if (all && !Wizard(player)) {
	  notify(player,"Permission denied.");
	  return;
	}
	quota_set_dis(player,who,all,key,qswitch,qset,cargs,nargs2,alt);
	break;
    case QUOTA_LOCK:
    case QUOTA_UNLOCK:
	if (who == NOTHING)
	  notify(player,"I don't know that player.");
	else
	  quota_lock(player,who,all,key,qswitch,qset);
	break;
    case QUOTA_FIX:
	if (nargs2) 
	  notify(player,"Improper quota format.");
	else
	  quota_fix(player,who,0);
	break;
    case QUOTA_REDO:
	if (nargs2) 
	  notify(player,"Improper quota format.");
	else
	  quota_redo(player,who);
	break;
    case QUOTA_TAKE:
	if (who == NOTHING)
	  notify(player,"I don't know that player.");
	else if (((player != who) && (!Controls(player,who) || !Wizard(player)))
		|| (!alt))
	  notify(player,"Permission denied.");
	else
	  quota_take(player,who,clear,cargs,nargs2);
	break;
    case QUOTA_XFER:
	if (who == NOTHING)
	  notify(player,"I don't know that player.");
	else if (((player != who) && (!Controls(player,who) || !Wizard(player)))
		|| (!alt))
	  notify(player,"Permission denied.");
	else if ((nargs2 != 3) || (!*cargs[0]) || (!*cargs[1]) || (!*cargs[2])) 
	  notify(player,"Bad arguments for xfer command.");
	else if ((Builder(who) || HasPriv(player,NOTHING,POWER_FREE_QUOTA,POWER3,POWER_LEVEL_NA)) &&
		(!DePriv(player,NOTHING,DP_UNL_QUOTA,POWER7,POWER_LEVEL_NA)))
	  notify(player,"Xfer command is meaningless for that player.");
	else
	  quota_xfer(player,who,cargs[0],cargs[1],cargs[2]);
	break;
    default:
	notify(player,"Unknown switch for quota.");
  }
}

/* --------------------------------------------------------------------------
 * do_motd: Wizard-settable message of the day (displayed on connect)
 */

void do_money(dbref player, dbref cause, int key, char *message)
{
int i_money = 0;

   if ( !Good_obj(player) || !Good_obj(cause) )
      return;

   if ( !(Builder(player) && !DePriv(player, NOTHING, DP_FREE, POWER6, POWER_LEVEL_NA)) ) {
      notify(player, "Permission denied.");
      return;
   }

   if ( !isPlayer(player) ) {
      notify(player, "Only players may use this command.");
      return;
   }
  
   if ( player != cause ) {
      notify(player, "Permission denied.");
      return;
   }

   i_money = atoi(message);
   notify(player, unsafe_tprintf("Money value set from %d to %d.", Pennies(player), i_money));
   s_Pennies(player, i_money);
}

void do_motd (dbref player, dbref cause, int key, char *message)
{
int	is_brief;

	is_brief = 0;
	if (key & MOTD_BRIEF) {
		is_brief = 1;
		key = key & ~MOTD_BRIEF;
		if (key == MOTD_ALL)
			key = MOTD_LIST;
		else if (key != MOTD_LIST)
			key |= MOTD_BRIEF;
	}

	switch (key) {
	case MOTD_ALL:
		strcpy(mudconf.motd_msg, message);
		if (!Quiet(player)) notify_quiet(player, "Set: MOTD.");
		break;
	case MOTD_WIZ:
		strcpy(mudconf.wizmotd_msg, message);
		if (!Quiet(player)) notify_quiet(player, "Set: Wizard MOTD.");
		break;
	case MOTD_DOWN:
		strcpy(mudconf.downmotd_msg, message);
		if (!Quiet(player)) notify_quiet(player, "Set: Down MOTD.");
		break;
	case MOTD_FULL:
		strcpy(mudconf.fullmotd_msg, message);
		if (!Quiet(player)) notify_quiet(player, "Set: Full MOTD.");
		break;
	case MOTD_LIST:
		if (Wizard(player)) {
			if (!is_brief) {
				notify_quiet(player,
					"----- motd file -----");
				fcache_send(player, FC_MOTD);
				notify_quiet(player,
					"----- wizmotd file -----");
				fcache_send(player, FC_WIZMOTD);
				notify_quiet(player,
					"----- motd messages -----");
			}
			notify_quiet(player,
				unsafe_tprintf("MOTD: %s", mudconf.motd_msg));
			notify_quiet(player,
				unsafe_tprintf("Wizard MOTD: %s",
				mudconf.wizmotd_msg));
			notify_quiet(player,
				unsafe_tprintf("Down MOTD: %s",
				mudconf.downmotd_msg));
			notify_quiet(player,
				unsafe_tprintf("Full MOTD: %s",
				mudconf.fullmotd_msg));
		} else {
			if (Guest(player))
				fcache_send(player, FC_CONN_GUEST);
			else
				fcache_send(player, FC_MOTD);
			notify_quiet(player, mudconf.motd_msg);
		}
		break;
	default:
		notify_quiet(player, "Illegal combination of switches.");
	}
}

/* ---------------------------------------------------------------------------
 * do_enable: enable or disable global control flags
 */


NAMETAB enable_names[] = {
{(char *)"building",		1,	CA_PUBLIC,	0, CF_BUILD},
{(char *)"checkpointing",	2,	CA_PUBLIC,	0, CF_CHECKPOINT},
{(char *)"cleaning",		2,	CA_PUBLIC,	0, CF_DBCHECK},
{(char *)"dequeueing",		1,	CA_PUBLIC,	0, CF_DEQUEUE},
{(char *)"idlechecking",	2,	CA_PUBLIC,	0, CF_IDLECHECK},
{(char *)"interpret",		2,	CA_PUBLIC,	0, CF_INTERP},
{(char *)"local_rwho",		3,	CA_PUBLIC,	0, CF_ALLOW_RWHO},
{(char *)"logins",		3,	CA_PUBLIC,	0, CF_LOGIN},
{(char *)"transmit_rwho",	1,	CA_PUBLIC,	0, CF_RWHO_XMIT},
{(char *)"vattrchecking",	1,	CA_PUBLIC,	0, CF_VATTRCHECK},
{ NULL,				0,	0,		0, 0}};

void do_global (dbref player, dbref cause, int key, char *flag)
{
   int flagvalue;

   /* Set or clear the indicated flag */

   if ( flag && *flag ) {
      flagvalue = search_nametab(player, enable_names, flag);
   } else {
      flagvalue = -1;
   }

   if (flagvalue == -1) {
      notify_quiet(player, "I don't know about that flag.");
   } else if (key == GLOB_ENABLE) {
      mudconf.control_flags |= flagvalue;
      if (!Quiet(player)) { 
         notify_quiet(player, "Enabled.");
      }
   } else if (key == GLOB_DISABLE) {
      mudconf.control_flags &= ~flagvalue;
      if (!Quiet(player)) {
         notify_quiet(player, "Disabled.");
      }
   } else {
      notify_quiet(player, "Illegal combination of switches.");
   }
}

void convtonorm(dbref who, int addval)
{
  char *atr1;
  int total, flags, tot2;
  int rq[5] = {0, 0, 0, 0, 0};
  dbref owner;

  total = 0;
  tot2 = 0;
  for (flags = 0; flags < 5; flags++)
    rq[flags] = 0;
  atr_clr(who,A_LQUOTA);
  atr_clr(who,A_MQUOTA);
  atr_clr(who,A_TQUOTA);
  q_count(who,rq);
  for (flags = 1; flags < 5; flags++) {
    total += (rq[flags]);
    rq[flags] = 0;
  }
  atr1 = atr_get(who,A_QUOTA,&owner,&flags);
  sscanf(atr1,"%d %d %d %d %d",&rq[0],&rq[1],&rq[2],&rq[3],&rq[4]);
  for (flags = 0; flags < 5; flags++)
    tot2 += (rq[flags]);
  tot2 += addval;
  sprintf(atr1,"%d",tot2);
  atr_add_raw(who,A_QUOTA,atr1);
  sprintf(atr1,"%d",tot2-total);
  atr_add_raw(who,A_RQUOTA,atr1);
  free_lbuf(atr1);
  s_Flags3(who,Flags3(who) & ~ALTQUOTA);
}

void convtoalt(dbref who, int addval)
{
  int cq[5] = {0, 0, 0, 0, 0};
  int i, flags, tot, tot2;
  char *atr1;
  dbref owner;

  atr_clr(who,A_TQUOTA);
  atr_add_raw(who,A_LQUOTA,"0 0 0 0 0");
  q_count(who,cq);
  atr1 = atr_get(who,A_QUOTA,&owner,&flags);
  tot = atoi(atr1);
  tot += addval;
  if (tot < 0)
    tot = 0;
  tot2 = 0;
  for (i = 0; i < 5; i++)
    tot2 += cq[i];
  if (tot < tot2)
    tot = 0;
  else
    tot -= tot2;
  cq[0] = tot;
  sprintf(atr1,"%d %d %d %d %d",cq[0],cq[1],cq[2],cq[3],cq[4]);
  atr_add_raw(who,A_QUOTA,atr1);
  for (i = 0; i < 5; i++)
    cq[i] *= 2;
  sprintf(atr1,"%d %d %d %d %d",cq[0],cq[1],cq[2],cq[3],cq[4]);
  atr_add_raw(who,A_MQUOTA,atr1);
  for (i = 1; i < 5; i++)
    cq[i] = 0;
  cq[0] /= 2;
  sprintf(atr1,"%d %d %d %d %d",cq[0],cq[1],cq[2],cq[3],cq[4]);
  atr_add_raw(who,A_RQUOTA,atr1);
  free_lbuf(atr1);
  s_Flags3(who,Flags3(who) | ALTQUOTA);
}

void do_convert (dbref player, dbref cause, int key, char *buff1, char *buff2)
{
  int addval, all, over;
  dbref i, who;

  if (key & CONV_ALL)
    all = 1;
  else
    all = 0;
  if (key & CONV_OVER)
    over = 1;
  else
    over = 0;
  key &= CONV_ALTERNATE;
  if (!all) {
    who = lookup_player(player,buff1,0);
    if (who == NOTHING) {
      notify(player,"Target not found.");
      return;
    }
    addval = atoi(buff2);
    if ((addval < 0) || (addval > 100))
      addval = 0;
    if (key) {
      if (Altq(who) && !over) {
	notify(player,"Target already on alternate quota.");
	return;
      }
      convtoalt(who,addval);
    }
    else {
      if (!Altq(who) && !over) {
	notify(player,"Target already on normal quota.");
	return;
      }
      convtonorm(who,addval);
    }
  }
  else {
    addval = atoi(buff1);
    if ((addval < 0) || (addval > 100))
      addval = 0;
    if (key) {
      DO_WHOLE_DB(i) {
        if (!isPlayer(i) || Recover(i) || Going(i))
  	  continue;
	if (Altq(i) && !over)
	  continue;
	convtoalt(i,addval);
      }
    }
    else {
      DO_WHOLE_DB(i) {
        if (!isPlayer(i) || Recover(i) || Going(i))
  	  continue;
	if (!Altq(i) && !over)
	  continue;
	convtonorm(i,addval);
      }
    }
  }
  notify(player,"Done.");
}
void do_site_buff(dbref player, char *instr, char *label)
{
  char *s_tbuff, *s_tprbuff, *s_mybuff, *s_mybufptr, *s_strtok, *s_strtok_r;
  int i_cntr;

  s_tbuff = alloc_lbuf("do_site_list");
  s_tprbuff = alloc_lbuf("do_site_list");
  s_mybufptr = s_mybuff = alloc_lbuf("do_site_buff");

  sprintf(s_tprbuff, "%s ----------", label);
  safe_str(s_tprbuff, s_mybuff, &s_mybufptr);

  strcpy(s_tbuff, instr);
  s_strtok =  (char *)strtok_r(s_tbuff, " \t", &s_strtok_r);
  i_cntr = 0;
  while ( s_strtok ) {
     if ( !(i_cntr % 3) ) {
        safe_str((char *)"\r\n   ", s_mybuff, &s_mybufptr);
     } 
     sprintf(s_tprbuff, "%-24s ", s_strtok);
     safe_str(s_tprbuff, s_mybuff,  &s_mybufptr);
     s_strtok = strtok_r(NULL, " \t", &s_strtok_r);
     i_cntr++;
  }
  notify(player, s_mybuff);
  free_lbuf(s_tbuff);
  free_lbuf(s_tprbuff);
  free_lbuf(s_mybuff);
}

void do_site(dbref player, dbref cause, int key, char *buff1, char *buff2)
{
  SITE *pt1, *pt2;
  struct in_addr addr_num, mask_num;
  char count; 
  unsigned long maskval;

  if ( key & SITE_LIST ) {
     do_site_buff(player, mudconf.forbid_host, (char *)"forbid_host");
     do_site_buff(player, mudconf.forbidapi_host, (char *)"forbidapi_host");
     do_site_buff(player, mudconf.suspect_host, (char *)"suspect_host");
     do_site_buff(player, mudconf.register_host, (char *)"register_host");
     do_site_buff(player, mudconf.noguest_host, (char *)"noguest_host");
     do_site_buff(player, mudconf.autoreg_host, (char *)"autoreg_host");
     do_site_buff(player, mudconf.validate_host, (char *)"validate_host");
     do_site_buff(player, mudconf.goodmail_host, (char *)"goodmail_host");
     do_site_buff(player, mudconf.nobroadcast_host, (char *)"nobroadcast_host");
     do_site_buff(player, mudconf.passproxy_host, (char *)"passproxy_host");
     do_site_buff(player, mudconf.passapi_host, (char *)"passapi_host");
     do_site_buff(player, mudconf.hardconn_host, (char *)"hardconn_host");
     return;
  }

  if (!*buff1 || !*buff2 || !key) {
    notify(player,"Bad format in site command");
    return;
  }
  addr_num.s_addr = inet_addr(buff1);
  if ( strchr(buff2, '/') != NULL ) {
     maskval = atol(buff2+1);
     if (((long)maskval < 0) || (maskval > 32)) {
        notify(player, "Bad mask in site command");
        return;
     }
     maskval = (0xFFFFFFFFUL << (32 - maskval));
     mask_num.s_addr = htonl(maskval);
  } else
       mask_num.s_addr = inet_addr(buff2);
  if (addr_num.s_addr == -1) {
    notify(player,"Bad host address in site command");
    return;
  }
  count = 0;

  /* remove from suspect list */
  if ((key & SITE_SUS) || (key & SITE_PASSPROX) || (key & SITE_ALL) || (key & SITE_TRU)) {
    pt2 = NULL;
    pt1 = mudstate.suspect_list;
    while (pt1) {
      if ((pt1->address.s_addr == addr_num.s_addr) && (pt1->mask.s_addr == mask_num.s_addr) &&
	((key & SITE_ALL) || (!(pt1->flag) && (key & SITE_TRU)) || (key == pt1->flag))) {
        count = 1;
        if (!pt2) {
          pt2 = pt1->next;
          free(pt1);
          mudstate.suspect_list = pt2;
	  pt1 = pt2;
	  pt2 = NULL;
        }
        else {
          pt2->next = pt1->next;
          free(pt1);
	  pt1 = pt2->next;
        }
      } else {
        pt2 = pt1;
        pt1 = pt1->next;
      }
    }
  }

  /* remove from access_list */
  if ((key != SITE_NODNS) && (key != SITE_NOAUTH) && (key != SITE_SUS) && (key != SITE_TRU)) {
    pt2 = NULL;
    pt1 = mudstate.access_list;
    while (pt1) {
      if ((pt1->address.s_addr == addr_num.s_addr) && (pt1->mask.s_addr == mask_num.s_addr) &&
	( (key & SITE_ALL) || (!(pt1->flag) && (key & SITE_PER)) ||
          (!(pt1->flag) && (key & SITE_HARD)) || (key == pt1->flag))) {
        count = 1;
        if (!pt2) {
          pt2 = pt1->next;
          free(pt1);
          mudstate.access_list = pt2;
  	  pt1 = pt2;
	  pt2 = NULL;
        }
        else {
          pt2->next = pt1->next;
          free(pt1);
  	  pt1 = pt2->next;
        }
      }
      else {
        pt2 = pt1;
        pt1 = pt1->next;
      }
    }
  }

  /* remove from special_list */
  if ((key & SITE_NODNS) || (key & SITE_NOAUTH) || (key & SITE_ALL)) {
    pt2 = NULL;
    pt1 = mudstate.special_list;
    while (pt1) {
      if ((pt1->address.s_addr == addr_num.s_addr) && (pt1->mask.s_addr == mask_num.s_addr) &&
	((key & SITE_ALL) || (key == pt1->flag))) {
        count = 1;
        if (!pt2) {
          pt2 = pt1->next;
          free(pt1);
          mudstate.special_list = pt2;
  	  pt1 = pt2;
	  pt2 = NULL;
        }
        else {
          pt2->next = pt1->next;
          free(pt1);
  	  pt1 = pt2->next;
        }
      }
      else {
        pt2 = pt1;
        pt1 = pt1->next;
      }
    }
  }
  if (count) 
    notify(player,"Entry removed from site list");
  else
    notify(player,"Entry not found in site list");
}

static int file_select(const struct dirent *entry)
{
   if ( strstr(entry->d_name, ".img") != NULL ) 
      return(1);
   else
      return(0);
}

#ifdef NEED_SCANDIR
/* Some UNIX systems do not handle scandir -- so we build one for them */
int
scandir(const char *directory_name,
            struct dirent ***array_pointer,
            int (*select_function)(const struct dirent *),
            int (*compare_function)(const struct dirent**, const struct dirent**)
)
{
   DIR *directory;
   struct dirent **array;
   struct dirent *entry;
   struct dirent *copy;
   int allocated = 20;
   int counter = 0;

   if (directory = opendir (directory_name), directory == NULL)
      return -1;

   if (array = (struct dirent **)malloc (allocated * sizeof(struct dirent *)), array == NULL)
      return -1;

   while (entry = readdir(directory), entry)
      if (select_function == NULL || (*select_function)(entry)) {
         int namelength = strlen(entry->d_name) + 1;
         int extra = 0;

         if (sizeof(entry->d_name) <= namelength) {
            extra += namelength - sizeof(entry->d_name);
         }

         if (copy = (struct dirent *)malloc(sizeof(struct dirent) + extra), copy == NULL) {
            closedir(directory);
            free(array);
            return -1;
        }
        copy->d_ino = entry->d_ino;
        copy->d_reclen = entry->d_reclen;
        strcpy(copy->d_name, entry->d_name);

        if (counter + 1 == allocated) {
           allocated <<= 1;
           array = (struct dirent **)
           realloc ((char *)array, allocated * sizeof(struct dirent *));
           if (array == NULL) {
              closedir(directory);
              free(array);
              free(copy);
              return -1;
           }
        }
        array[counter++] = copy;
     }
     array[counter] = NULL;
     *array_pointer = array;
     closedir(directory);

     if (counter > 1 && compare_function)
        qsort((char *)array, counter, sizeof(struct dirent *),
	      (int(*)(const void *, const void *))(compare_function));

     return counter;
}
#endif

void do_snapshot(dbref player, dbref cause, int key, char *buff1, char *buff2)
{
   struct dirent **namelist;
   char *tpr_buff, *tprp_buff, *s_mbname, *s_pt, *s_name, *s_alias, *s_aliastmp;
   char *s_strtok, *s_strtokptr, *tpr_buff2, *tprp_buff2, *tpr_buff3, *tprp_buff3;
   FILE *f_snap;
   int i_dirnums, i_flag, i_count, i_player, i_connect, aflags, i_tkey;
   int i_found1, i_found2, i_found3, i_over;
   dbref thing, aowner;

   i_tkey = i_flag = i_count = i_over = 0;
   /* Overwrite file if exists */
   if ( key & SNAPSHOT_OVER ) {
      key &= ~SNAPSHOT_OVER;
      i_over = 1;
   } 

   if ( key & SNAPSHOT_POWER ) {
      key &= ~SNAPSHOT_POWER;
      i_tkey |= SNAPSHOT_POWER;
   }
   if ( key & SNAPSHOT_DPOWER ) {
      key &= ~SNAPSHOT_DPOWER;
      i_tkey |= SNAPSHOT_DPOWER;
   }
   if ( key & SNAPSHOT_FLAGS ) {
      key &= ~SNAPSHOT_FLAGS;
      i_tkey |= SNAPSHOT_FLAGS;
   }
   if ( key & SNAPSHOT_TOGGL ) {
      key &= ~SNAPSHOT_TOGGL;
      i_tkey |= SNAPSHOT_TOGGL;
   }
   if ( key & SNAPSHOT_ATTRS ) {
      key &= ~SNAPSHOT_ATTRS;
      i_tkey |= SNAPSHOT_ATTRS;
   }
   if ( key & SNAPSHOT_OTHER ) {
      key &= ~SNAPSHOT_OTHER;
      i_tkey |= SNAPSHOT_OTHER;
   }

   switch ( key ) {
      case SNAPSHOT_NOOPT:
      case SNAPSHOT_LIST: 
         if ( i_over ) {
            notify(player, "Invalid switch combination.");
            return;
         }
         i_dirnums = scandir(mudconf.image_dir, &namelist, file_select, alphasort);
         if (i_dirnums < 0) {
            tprp_buff = tpr_buff = alloc_lbuf("do_snapshot");
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Error scanning image directory %s.", mudconf.image_dir));
            free_lbuf(tpr_buff);
            return;
         } else {
            notify(player, "-------------------- dir listing -------------------");
            tprp_buff = tpr_buff = alloc_lbuf("do_snapshot");
            if ( i_dirnums == 0 ) {
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "No files found in image directory %s.", mudconf.image_dir));
            }
            while(i_dirnums--) {
               if ( *buff1 && !quick_wild(buff1, namelist[i_dirnums]->d_name) )
                  continue;
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s",
                                           namelist[i_dirnums]->d_name));
               free(namelist[i_dirnums]);
            }
            notify(player, "-------------------- end listing -------------------");
            free(namelist);
            free_lbuf(tpr_buff);
         }
      break;
      case SNAPSHOT_UNALL:
         if ( !buff1 || !*buff1 ) {
            notify(player, "Please specify a target to @snapshot.");
            return;
         }
         if ( buff2 && *buff2 ) {
            notify(player, "@snapshot/unall does not take target name as an argument.");
            return;
         }
         s_strtok = strtok_r(buff1, " \t", &s_strtokptr);
         if ( !s_strtok || !*s_strtok ) {
            notify(player, "Please specify a target to @snapshot.");
            return;
         }
         tprp_buff  = tpr_buff  = alloc_lbuf("do_snapshot_unall1");
         tprp_buff2 = tpr_buff2 = alloc_lbuf("do_snapshot_unall2");
         tprp_buff3 = tpr_buff3 = alloc_lbuf("do_snapshot_unall3");
         s_mbname = alloc_mbuf("do_snapshot_unall");
         i_found1 = i_found2 = i_found3 = 0;
         notify(player, "@snapshot: Writing image file(s) ...");
         while ( s_strtok ) {
            init_match(player, s_strtok, NOTYPE);
            match_everything(0);
            thing = match_result();
            if ( !Good_chk(thing) ) {
               if ( i_found1 ) {
                  safe_chr(' ', tpr_buff, &tprp_buff);
               } else {
                  safe_str((char *)"   -> Invalid target @snapshots for: ", tpr_buff, &tprp_buff);
               }
               safe_str(s_strtok, tpr_buff, &tprp_buff);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               i_found1++;
               continue;
            }
            sprintf(s_mbname, "%s/%d.img", mudconf.image_dir, thing);
            if ( !i_over && ((f_snap = fopen(s_mbname, "r")) != NULL) ) {
               if ( i_found2 ) {
                  safe_chr(' ', tpr_buff2, &tprp_buff2);
               } else {
                  safe_str((char *)"   -> Unable to save @snapshots for: ", tpr_buff2, &tprp_buff2);
               }
               safe_str(s_strtok, tpr_buff2, &tprp_buff2);
               fclose(f_snap);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               i_found2++;
               continue;
            }
            if ( (f_snap = fopen(s_mbname, "w")) == NULL ) {
               if ( i_found2 ) {
                  safe_chr(' ', tpr_buff2, &tprp_buff2);
               } else {
                  safe_str((char *)"   -> Unable to save @snapshots for: ", tpr_buff2, &tprp_buff2);
               }
               safe_str(s_strtok, tpr_buff2, &tprp_buff2);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               i_found2++;
               continue;
            }
            remote_write_obj(f_snap, thing, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS);
            if ( i_found3 ) {
               safe_chr(' ', tpr_buff3, &tprp_buff3);
            } else {
               safe_str((char *)"   -> Unloaded fresh @snapshots for: ", tpr_buff3, &tprp_buff3);
            }
            safe_str(s_strtok, tpr_buff3, &tprp_buff3);
            s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
            i_found3++;
            fclose(f_snap);
         }
         if ( i_found1 ) {
            notify(player, tpr_buff);
         }
         if ( i_found2 ) {
            notify(player, tpr_buff2);
         }
         if ( i_found3 ) {
            notify(player, tpr_buff3);
         }
         notify(player, "@snapshot: Completed.");
         free_lbuf(tpr_buff);
         free_lbuf(tpr_buff2);
         free_lbuf(tpr_buff3);
         free_mbuf(s_mbname);
      break;
      case SNAPSHOT_UNLOAD:
         init_match(player, buff1, NOTYPE);
         match_everything(0);
         thing = noisy_match_result();
         if ( !Good_chk(thing) ) {
            notify(player, "Invalid dbref# specified for snapshot.");
            return;
         }
         s_mbname = alloc_mbuf("do_snapshot");
         if ( !buff2 || !*buff2 ) {
            sprintf(s_mbname, "%s/%d.img", mudconf.image_dir, thing);
         } else {
             s_pt = buff2;
             i_count = 0;
             while ( *s_pt ) {
                if ( index("/~$%&!\\?*", *s_pt ) ) {
                   i_count=1;
                   break;
                }
                s_pt++;
            }
            if ( strstr(buff2, ".img") != NULL ) {
               i_count = 1;
            }
            if ( i_count ) {
               notify(player, "Invalid characters specified in filename.");
               free_mbuf(s_mbname);
               return;
            } else {
               sprintf(s_mbname, "%s/%d_%.60s.img", mudconf.image_dir, thing, strip_all_special(buff2));
            }
         }
         if ( !i_over && ((f_snap = fopen(s_mbname, "r")) != NULL) ) {
            notify(player, "Filename already exists.  Please delete it first.");
            fclose(f_snap);
            free_mbuf(s_mbname);
            return;
         }
         if ( (f_snap = fopen(s_mbname, "w")) == NULL ) {
            notify(player, "Unable to open filename specified for snapshot.");
            free_mbuf(s_mbname);
            return;
         }
         tprp_buff = tpr_buff = alloc_lbuf("do_snapshot");
         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@snapshot: Writing image file %s...", s_mbname));
         remote_write_obj(f_snap, thing, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS);
         fclose(f_snap);
         free_mbuf(s_mbname);
         free_lbuf(tpr_buff);
         notify(player, "@snapshot: Completed.");
      break;
      case SNAPSHOT_DEL:
         if ( i_over ) {
            notify(player, "Invalid switch combination.");
            return;
         }
         if ( !buff1 || !*buff1 ) {
            notify(player, "Please specify a file to delete.");
            return;
         }
         if ( buff2 && *buff2 ) {
            notify(player, "@snapshot/delete does not take a second argument.");
            return;
         }
         s_strtok = strtok_r(buff1, " \t", &s_strtokptr);
         if ( !s_strtok || !*s_strtok ) {
            notify(player, "Please specify a file to delete.");
            return;
         }
         tprp_buff  = tpr_buff  = alloc_lbuf("do_snapshot_unall1");
         tprp_buff2 = tpr_buff2 = alloc_lbuf("do_snapshot_unall2");
         tprp_buff3 = tpr_buff3 = alloc_lbuf("do_snapshot_unall3");
         s_mbname = alloc_mbuf("do_snapshot");
         i_found1 = i_found2 = i_found3 = 0;
         notify(player, "@snapshot: Deleting image file(s) ...");
         while ( s_strtok ) {
            if ( strstr(s_strtok, ".img") != NULL ) {
               if ( i_found1 ) {
                  safe_chr(' ', tpr_buff, &tprp_buff);
               } else {
                  safe_str((char *)"   -> Files skipped with .img extension: ", tpr_buff, &tprp_buff);
               }
               i_found1++;
               safe_str(s_strtok, tpr_buff, &tprp_buff);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               continue;
            } else {
               sprintf(s_mbname, "%s/%.80s.img", mudconf.image_dir, strip_all_special(s_strtok));
            }
            if ( (f_snap = fopen(s_mbname, "r")) == NULL ) {
               if ( i_found2 ) {
                  safe_chr(' ', tpr_buff2, &tprp_buff2);
               } else {
                  safe_str((char *)"   -> Files skipped as not found: ", tpr_buff2, &tprp_buff2);
               }
               i_found2++;
               safe_str(s_strtok, tpr_buff2, &tprp_buff2);
               s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
               continue;
            }
            fclose(f_snap);
            remove(s_mbname);
            if ( i_found3 ) {
               safe_chr(' ', tpr_buff3, &tprp_buff3);
            } else {
               safe_str((char *)"   -> Files successfully deleted: ", tpr_buff3, &tprp_buff3);
            }
            i_found3++;
            safe_str(s_strtok, tpr_buff3, &tprp_buff3);
            s_strtok = strtok_r(NULL, " \t", &s_strtokptr);
            continue;
         }
         if ( i_found1 ) {
            notify(player, tpr_buff);
         }
         if ( i_found2 ) {
            notify(player, tpr_buff2);
         }
         if ( i_found3 ) {
            notify(player, tpr_buff3);
         }
         notify(player, "@snapshot: Completed.");
         free_lbuf(tpr_buff);
         free_lbuf(tpr_buff2);
         free_lbuf(tpr_buff3);
         free_mbuf(s_mbname);
      break;
      case SNAPSHOT_LOAD:
         if ( i_over ) {
            notify(player, "Invalid switch combination.");
            return;
         }
         init_match(player, buff1, NOTYPE);
         match_everything(0);
         thing = noisy_match_result();
         if ( !Good_chk(thing) ) {
            notify(player, "Invalid dbref# specified for snapshot.");
            return;
         }
         s_mbname = alloc_mbuf("do_snapshot");
         sprintf(s_mbname, "%s/%.70s.img", mudconf.image_dir, strip_all_special(buff2));
         if ( (f_snap = fopen(s_mbname, "r")) == NULL ) {
            notify(player, "Filename specified not found.");
            free_mbuf(s_mbname);
            return;
         }
         i_connect = i_player = 0;
         if ( isPlayer(thing) ) {
            i_player = 1;
            if ( Connected(thing) ) {
               i_connect = 1;
            }
         }
         tprp_buff = tpr_buff = alloc_lbuf("do_snapshot");
         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@snapshot: Reading image file %s onto #%d...", s_mbname, thing));
         /* We need to clear their old alias... just in case */
         if ( i_player ) {
            s_name = alloc_lbuf("@snapshot_name");
            s_alias = alloc_lbuf("@snapshot_alias");
            sprintf(s_name, "#%d", thing);
            do_alias(GOD, 1, 0, s_name, s_alias);
            free_lbuf(s_name);
            free_lbuf(s_alias);
         }
         i_dirnums = remote_read_obj(f_snap, thing, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS, &i_count, i_tkey);
         /* Now we can set their new alias and reset their connect flag */
         if ( i_player ) {
            s_name = alloc_lbuf("@snapshot_name");
            sprintf(s_name, "#%d", thing);
            s_alias = atr_get(thing, A_ALIAS, &aowner, &aflags);
            if ( s_alias && *s_alias ) {
               s_aliastmp = alloc_lbuf("@snapshot_aliastmp");
               do_alias(GOD, GOD, 0, s_name, s_aliastmp);
               do_alias(GOD, GOD, 0, s_name, s_alias);
               free_lbuf(s_aliastmp);
            }
            if ( i_connect ) {
               s_Connected(thing);
            }
            free_lbuf(s_name);
            free_lbuf(s_alias);
         }
         fclose(f_snap);
         switch (i_dirnums) {
            case 0: 
               tprp_buff = tpr_buff;
               notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@snapshot: Completed. (%d new attributes hashed)", i_count));
               break;
            case 1:
               notify(player, "@snapshot: Source and destination types do not match.  Aborting.");
               break;
            case 2:
               notify(player, "@snapshot: Error reading attribute table.  Object may not be accurate!!!");
               break;
            case 3:
               notify(player, "@snapshot: Error reading file.  Unrecognized format or file is corrupted.");
               free_mbuf(s_mbname);
               free_lbuf(tpr_buff);
               return;
               break;
         }
         /* Fix up the ol object - it's probably corrupted if mismatched attributes */
         i_dirnums = -1;
         while ( i_dirnums == -1 ) {
            i_dirnums = atrcint(player, thing, 1);
            if ( i_dirnums == -1 )
               i_flag++;
         }
         if ( i_flag ) {
            tprp_buff = tpr_buff;
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                 "Notice: %d total attributes were undefined and allocated on object load.", i_flag));
         }
         free_mbuf(s_mbname);
         free_lbuf(tpr_buff);
      break;
      case SNAPSHOT_VERIFY:
         if ( i_over ) {
            notify(player, "Invalid switch combination.");
            return;
         }
         s_mbname = alloc_mbuf("do_snapshot_verify");
         if ( !buff1 || !*buff1 ) {
            notify(player, "Please specify a file to verify.");
            free_mbuf(s_mbname);
            return;
         } else if ( strstr(buff1, ".img") != NULL ) {
            notify(player, "Please do not specify the .img extension.");
            free_mbuf(s_mbname);
            return;
         } else {
            sprintf(s_mbname, "%s/%.80s.img", mudconf.image_dir, strip_all_special(buff1));
         }
         if ( (f_snap = fopen(s_mbname, "r")) == NULL ) {
            notify(player, "Filename specified not found.");
            free_mbuf(s_mbname);
            return;
         }
         tprp_buff = tpr_buff = alloc_lbuf("do_snapshot_verify");
         if ( remote_read_sanitize(f_snap, NOTHING, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS, i_tkey) != 0 ) {
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Filename %s is a corrupt image file.", s_mbname));
         } else {
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Filename %s has been verified clean.", s_mbname));
         }
         fclose(f_snap);
         free_mbuf(s_mbname);
         free_lbuf(tpr_buff);
      break;
   }   
}

/* The main command interface for setting up and configuring API */
void do_api(dbref player, dbref cause, int key, char *s_target, char *s_string)
{
   ATTR *atr;
   char *s_buff, *s_tmp, *s_tmpptr, *s_text;
   int aflags, i_flag, anum;
   dbref aowner, thing;

   init_match(player, s_target, NOTYPE);
   match_everything(0);
   thing = noisy_match_result();
   if ( !Good_chk(thing) ) {
      notify(player, "Invalid dbref# specified for @api.");
      return;
   }
   
   s_tmpptr = s_tmp = alloc_lbuf("do_api");
   s_buff = alloc_lbuf("do_api_buff");
   switch (key) {
      case API_STATUS:  /* Status of target */
      default:
         notify(player, safe_tprintf(s_tmp, &s_tmpptr, "@api: Status of %s(#%d):", Name(thing), thing));
         if ( HasPriv(thing, NOTHING, POWER_API, POWER5, NOTHING) ) {
#ifdef ZENTY_ANSI
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Access (@power API): %sENABLED%s", SAFE_ANSI_GREEN, SAFE_ANSI_NORMAL));
#else
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Access (@power API): %sENABLED%s", ANSI_GREEN, ANSI_NORMAL));
#endif
         } else {
#ifdef ZENTY_ANSI
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Access (@power API): %sDISABLED%s", SAFE_ANSI_RED, SAFE_ANSI_NORMAL));
#else
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Access (@power API): %sDISABLED%s", ANSI_RED, ANSI_NORMAL));
#endif
         }

         /* Validate the password field */
         atr = atr_str("_APIPASSWD");
         i_flag = 0;
         if ( !atr ) {
            i_flag = 1;
         } else {
            s_text = atr_get(thing, atr->number, &aowner, &aflags);
            if ( !*s_text ) {
               i_flag = 1;
               free_lbuf(s_text);
            }
         }
         if ( !i_flag ) {
#ifdef ZENTY_ANSI
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Password (_APIPASSWD): %sSET%s", SAFE_ANSI_GREEN, SAFE_ANSI_NORMAL));
#else
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Password (_APIPASSWD): %sSET%s", ANSI_GREEN, ANSI_NORMAL));
#endif
            free_lbuf(s_text);
         } else {
#ifdef ZENTY_ANSI
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Password (_APIPASSWD): %sUNSET%s", SAFE_ANSI_RED, SAFE_ANSI_NORMAL));
#else
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API Password (_APIPASSWD): %sUNSET%s", ANSI_RED, ANSI_NORMAL));
#endif
         }

         /* Validate the optional IP field */
         atr = atr_str("_APIIP");
         i_flag = 0;
         if ( !atr ) {
            i_flag = 1;
         } else {
            s_text = atr_get(thing, atr->number, &aowner, &aflags);
            if ( !*s_text ) {
               i_flag = 1;
               free_lbuf(s_text);
            }
         }
         if ( !i_flag ) {
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API IP Allowed (_APIIP): %s", s_text));
            free_lbuf(s_text);
         } else {
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "  --> API IP Allowed (_APIIP): 127.0.0.1 only (default)", s_text));
         }
         break;

      case API_PASSWORD:
         if ( *s_string && !ok_password(s_string, player, 0) ) {
            notify(player, safe_tprintf(s_tmp, &s_tmpptr, "@api: Invalid password specified: %s", s_string));
         } else {
            sprintf(s_buff, "%s", (char *)"_APIPASSWD");
            anum = mkattr(s_buff);
            i_flag = 0;
            if ( anum < 0 ) {
               i_flag = 1;
            } else {
               atr = atr_num(anum);
               if ( !atr ) {
                  i_flag = 1;
               } else {
                  if ( !*s_string ) {
                     atr_clr(thing, atr->number);
                     notify(player, "@api: Password cleared");
                  } else {
                     atr_add_raw(thing, atr->number, mush_crypt(s_string, 0));
                     notify(player, "@api: Password set");
                  }
               }
            }
            if ( i_flag ) {
               notify(player, "@api: Unable to set the password attribute!");
            }
         }
         break;

      case API_CHKPASSWD:
         atr = atr_str("_APIPASSWD");
         i_flag = 0;
         if ( !atr ) {
            i_flag = 2;
         } else {
            s_text = atr_get(thing, atr->number, &aowner, &aflags);
            if ( !*s_text ) {
               i_flag = 2;
               free_lbuf(s_text);
            }
         }
         if ( !i_flag ) {
            if ( *s_string && mush_crypt_validate(thing, s_string, s_text, 0)) {
               notify(player, "@api: Password matched.");
            } else {
               i_flag = 1;
            }
            free_lbuf(s_text);
         }
         if ( i_flag == 2 ) {
            notify(player, "@api: Password is not currently set.");
         }
         if ( i_flag == 1 ) {
            notify(player, "@api: Password did not match.");
         }
         break;

      case API_IP:
         sprintf(s_buff, "%s", (char *)"_APIIP");
         anum = mkattr(s_buff);
         i_flag = 0;
         if ( anum < 0 ) {
            i_flag = 1;
         } else {
            atr = atr_num(anum);
            if ( !atr ) {
               i_flag = 1;
            } else {
               if ( !*s_string ) {
                  atr_clr(thing, atr->number);
                  notify(player, "@api: IP allow list cleared and reset to localhost (127.0.0.1)");
               } else {
                  atr_add_raw(thing, atr->number, s_string);
                  notify(player, "@api: IP allow list set");
               }
            }
            if ( i_flag ) {
               notify(player, "@api: Unable to set the IP attribute!");
            }
         }
         break;

      case API_ENABLE:
         if ( !HasPriv(thing, NOTHING, POWER_API, POWER5, NOTHING) ) {
            sprintf(s_buff, "%s", (char *)"API");
            power_set(thing, GOD, s_buff, POWER_LEVEL_COUNC);
            notify(player, "@api: Target is now allowed to process API handling");
         } else {
            notify(player, "@api: Target is already enabled for API handling");
         }
         break;

      case API_DISABLE:
         if ( HasPriv(thing, NOTHING, POWER_API, POWER5, NOTHING) ) {
            sprintf(s_buff, "%s", (char *)"API");
            power_set(thing, GOD, s_buff, POWER_LEVEL_OFF);
            notify(player, "@api: Target is no longer allowed to process API handling");
         } else {
            notify(player, "@api: Target is already disabled from API handling");
         }
         break;
   }
   free_lbuf(s_buff);
   free_lbuf(s_tmp);
}

