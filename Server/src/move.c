/* move.c -- Routines for moving about */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "attrs.h"
#include "externs.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */

/* ---------------------------------------------------------------------------
 * process_leave_loc: Generate messages and actions resulting from leaving
 * a place.
 */

static void 
process_leave_loc(dbref thing, dbref dest, dbref cause,
		  int canhear, int hush)
{
    dbref loc;
    int quiet, pattr, oattr, aattr, blind_chk;
    char *tpr_buff, *tprp_buff;

    loc = Location(thing);
    if (!Good_obj(loc) || (loc == dest))
	return;

    if (dest == HOME)
	dest = Home(thing);

    /* Run the LEAVE attributes in the current room if we meet any of
     * following criteria:
     * - The current room has wizard privs.
     * - Neither the current room nor the moving object are dark.
     * - The moving object can hear and does not hav wizard privs.
     * EXCEPT if we were called with the HUSH_LEAVE key.
     */

     blind_chk = hush;
     if ( hush & HUSH_BLIND )
        hush = (hush & ~(HUSH_BLIND|HUSH_QUIET));

    quiet = (!(Wizard(loc) ||
	       (!Dark(thing) && !Dark(loc)) ||
	       (canhear && !(Wizard(thing) && Dark(thing))))) ||
	(hush & HUSH_LEAVE) || (hush & HUSH_QUIET);
#ifdef REALITY_LEVELS
    quiet = quiet || !IsReal(loc,thing);
#endif /* REALITY_LEVELS */
    oattr = quiet ? 0 : A_OLEAVE;
    aattr = quiet ? 0 : A_ALEAVE;
    pattr = (!mudconf.terse_movemsg && Terse(thing)) ? 0 : A_LEAVE;
    if ( pattr ) {
       pattr = (!mudconf.terse_movemsg && (Good_chk(dest) && Terse(dest) && isRoom(dest))) ? 0 : A_LEAVE;
    }
    did_it(thing, loc, pattr, NULL, oattr, NULL, aattr,
	   (char **) NULL, 0);

    /* Do OXENTER for receiving room */

    if ((dest != NOTHING) && !quiet)
	did_it(thing, dest, 0, NULL, A_OXENTER, NULL, 0,
	       (char **) NULL, 0);

    /* Display the 'has left' message if we meet any of the following
     * criteria:
     * - Neither the current room nor the moving object are dark.
     * - The object can hear and is not a dark wizard.
     */

    hush = blind_chk;
    if ((hush & HUSH_LEAVE) && 
			!DePriv(cause, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA))
      quiet = 1;
    else
      quiet = 0;

    if ( Good_obj(loc) && ((Blind(loc) && !mudconf.always_blind) || (!Blind(loc) && mudconf.always_blind)) )
        hush = (hush | HUSH_BLIND | HUSH_QUIET);
    if ( Good_obj(loc) && ((Blind(loc) && mudconf.always_blind)) && (hush & HUSH_BLIND) )
        hush = (hush & ~(HUSH_BLIND | HUSH_QUIET));

    if ( (!Dark(thing) && !Dark(loc) && !(hush & HUSH_QUIET) && !NCloak(thing)) ||
	(canhear && (((!(hush & HUSH_QUIET) && (!(Wizard(thing) && Dark(thing)) && !Cloak(thing) && !NCloak(thing)))) ||
	       DePriv(thing, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA))) ) {
            tprp_buff = tpr_buff = alloc_lbuf("process_leave_loc");
#ifdef REALITY_LEVELS
            notify_except2_rlevel(loc, thing, thing, cause,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s has left.", Name(thing)));
#else
            notify_except2(loc, thing, thing, cause,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s has left.", Name(thing)));
#endif /* REALITY_LEVELS */ 
            free_lbuf(tpr_buff);
    } else if (canhear) { /* used to be connected as well */
        tprp_buff = tpr_buff = alloc_lbuf("process_leave_loc");
	if (!Wizard(thing)) {
	    if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			    safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Cloaked)", Name(thing)));
	    }
	    else {
              if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Blind)", Name(thing)));
              else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Quiet)", Name(thing)));
	    } 
	} else if (!Immortal(thing)) {
	    if (Cloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			 safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Wizcloaked)", Name(thing)));
	    } else if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			    safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Cloaked)", Name(thing)));
	    } else {
               if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Blind)", Name(thing)));
               else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Dark)", Name(thing)));
	    }
	} else {
	    if (Cloak(thing)) {
		if (SCloak(thing)) {
		    notify_except3(loc, thing, thing, cause, 1,
		       safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Supercloaked)", Name(thing)));
		} else {
		    notify_except3(loc, thing, thing, cause, 0,
			 safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Wizcloaked)", Name(thing)));
		}
	    } else if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			    safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Cloaked)", Name(thing)));
	    } else {
               if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Blind)", Name(thing)));
               else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Dark)", Name(thing)));
	    }
	}
        free_lbuf(tpr_buff);
    } else {
       tprp_buff = tpr_buff = alloc_lbuf("process_leave_loc");
       if ( Immortal(thing) && SCloak(thing) && Cloak(thing) ) {
           if (isPlayer(thing)) 
              notify_except3(loc, thing, thing, cause, 1,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Disconnected Player)", Name(thing)));
           else
              notify_except3(loc, thing, thing, cause, 1,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Non-Listening)", Name(thing)));
       } else {
           if (isPlayer(thing)) 
              notify_except3(loc, thing, thing, cause, 0,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Disconnected Player)", Name(thing)));
           else
              notify_except3(loc, thing, thing, cause, 0,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has left. (Non-Listening)", Name(thing)));
       }
       free_lbuf(tpr_buff);
    }
}

/* ---------------------------------------------------------------------------
 * process_enter_loc: Generate messages and actions resulting from entering
 * a place.
 */

static void 
process_enter_loc(dbref thing, dbref src, dbref cause,
		  int canhear, int hush)
{
    dbref loc;
    int quiet, pattr, oattr, aattr, blind_chk;
    char *tpr_buff, *tprp_buff;

    loc = Location(thing);
    if (!Good_obj(loc) || (loc == src))
	return;

    /* Run the ENTER attributes in the current room if we meet any of
     * following criteria:
     * - The current room has wizard privs.
     * - Neither the current room nor the moving object are dark.
     * - The moving object can hear and does not hav wizard privs.
     * EXCEPT if we were called with the HUSH_ENTER key.
     */

    blind_chk = hush; 
     if ( hush & HUSH_BLIND )
        hush = (hush & ~(HUSH_BLIND|HUSH_QUIET));

    quiet = (!(Wizard(loc) ||
	       (!Dark(thing) && !Dark(loc)) ||
	       (canhear && !(Wizard(thing) && Dark(thing))))) ||
	(hush & HUSH_ENTER) || (hush & HUSH_QUIET);
#ifdef REALITY_LEVELS
    quiet = quiet || !IsReal(loc, thing);
#endif /* REALITY_LEVELS */
    oattr = quiet ? 0 : A_OENTER;
    aattr = quiet ? 0 : A_AENTER;
    pattr = (!mudconf.terse_movemsg && Terse(thing)) ? 0 : A_ENTER;
    if ( pattr ) {
       pattr = (!mudconf.terse_movemsg && (Good_chk(src) && Terse(src) && isRoom(src))) ? 0 : A_ENTER;
    }
    did_it(thing, loc, pattr, NULL, oattr, NULL, aattr,
	   (char **) NULL, 0);

    /* Do OXLEAVE for sending room */

    if ((src != NOTHING) && !quiet)
	did_it(thing, src, 0, NULL, A_OXLEAVE, NULL, 0,
	       (char **) NULL, 0);

    hush = blind_chk;

    if ( Good_obj(loc) && ((Blind(loc) && !mudconf.always_blind) || (!Blind(loc) && mudconf.always_blind)) )
        hush = (hush | HUSH_BLIND | HUSH_QUIET);
    if ( Good_obj(loc) && ((Blind(loc) && mudconf.always_blind)) && (hush & HUSH_BLIND) )
        hush = (hush & ~(HUSH_BLIND | HUSH_QUIET));

    /* Display the 'has arrived' message if we meet all of the following
     * criteria:
     * - The moving object can hear.
     * - The object is not a dark wizard.
     */
    if ( canhear && ((!(hush & HUSH_QUIET) && (!(Dark(thing) && Wizard(thing)) && !Cloak(thing) && !NCloak(thing))) ||
		DePriv(thing, NOTHING, DP_CLOAK, POWER6, POWER_LEVEL_NA)) ) {
            tprp_buff = tpr_buff = alloc_lbuf("process_enter_loc");
#ifdef REALITY_LEVELS
            notify_except2_rlevel(loc, thing, thing, cause,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived.", Name(thing)));
#else
            notify_except2(loc, thing, thing, cause,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived.", Name(thing)));
#endif /* REALITY_LEVELS */
            free_lbuf(tpr_buff);
    } else if (canhear) { /* used to be connected as well */
        tprp_buff = tpr_buff = alloc_lbuf("process_enter_loc");
	if (!Wizard(thing)) {
	    if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			 safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Cloaked)", Name(thing)));
	    }
	    else {
               if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Blind)", Name(thing)));
               else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Quiet)", Name(thing)));
	    } 
	} else if (!Immortal(thing)) {
	    if (Cloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
		      safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Wizcloaked)", Name(thing)));
	    } else if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			 safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Cloaked)", Name(thing)));
	    }
	    else {
              if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Blind)", Name(thing)));
              else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Dark)", Name(thing)));
	    }
	} else {
	    if (Cloak(thing)) {
		if (SCloak(thing)) {
		    notify_except3(loc, thing, thing, cause, 1,
		    safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Supercloaked)", Name(thing)));
		} else {
		    notify_except3(loc, thing, thing, cause, 0,
		      safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Wizcloaked)", Name(thing)));
		}
	    } else if (NCloak(thing)) {
		notify_except3(loc, thing, thing, cause, 0,
			 safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Cloaked)", Name(thing)));
	    }
	    else {
              if ( hush & HUSH_BLIND )
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Blind)", Name(thing)));
              else
	         notify_except3(loc, thing, thing, cause, 0,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Dark)", Name(thing)));
	    }
	}
        free_lbuf(tpr_buff);
    } else {
       tprp_buff = tpr_buff = alloc_lbuf("process_enter_loc");
       if ( Immortal(thing) && SCloak(thing) && Cloak(thing) ) {
          if (isPlayer(thing))
              notify_except3(loc, thing, thing, cause, 1,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Disconnected Player)", Name(thing)));
          else
              notify_except3(loc, thing, thing, cause, 1,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Non-Listening)", Name(thing)));
       } else {
          if (isPlayer(thing))
              notify_except3(loc, thing, thing, cause, 0,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Disconnected Player)", Name(thing)));
          else
              notify_except3(loc, thing, thing, cause, 0,
                             safe_tprintf(tpr_buff, &tprp_buff, "%s has arrived. (Non-Listening)", Name(thing)));
       }
       free_lbuf(tpr_buff);
    }
}

/* ---------------------------------------------------------------------------
 * move_object: Physically move an object from one place to another.
 * Does not generate any messages or actions.
 */

void 
move_object(dbref thing, dbref dest)
{
    dbref src;
    char *tpr_buff, *tprp_buff;

    /* Remove from the source location */

    src = Location(thing);
    if (Good_obj(src))
	s_Contents(src, remove_first(Contents(src), thing));

    /* Special check for HOME */

    if (dest == HOME)
	dest = Home(thing);

    /* Add to destination location */

    if (dest != NOTHING)
	s_Contents(dest, insert_first(Contents(dest), thing));
    else
	s_Next(thing, NOTHING);
    s_Location(thing, dest);

    /* Look around and do the penny check */

    look_in(thing, dest, (LK_SHOWEXIT | LK_OBEYTERSE));
    if (isPlayer(thing) &&
	(mudconf.payfind > 0) &&
	(Pennies(thing) < mudconf.paylimit) &&
	(!Controls(thing, dest)) &&
	((random() % mudconf.payfind) == 0)) {
	giveto(thing, 1, NOTHING);
        tprp_buff = tpr_buff = alloc_lbuf("move_object");
	notify(thing, safe_tprintf(tpr_buff, &tprp_buff, "You found a %s!", mudconf.one_coin));
        free_lbuf(tpr_buff);
    }
}

/* ---------------------------------------------------------------------------
 * send_dropto, process_sticky_dropto, process_dropped_dropto,
 * process_sacrifice_dropto: Check for and process droptos.
 */

/* send_dropto: Send an object through the dropto of a room */

static void 
send_dropto(dbref thing, dbref player)
{
    if (!Sticky(thing) && Good_obj(Location(thing)))
	move_via_generic(thing, Dropto(Location(thing)), player, 0);
    else
	move_via_generic(thing, HOME, player, 0);
    divest_object(thing);

}

/* process_sticky_dropto: Call when an object leaves the room to see if
 * we should empty the room
 */

static void 
process_sticky_dropto(dbref loc, dbref player)
{
    dbref dropto, thing, next;

    /* Do nothing if checking anything but a sticky room */

    if (!Good_obj(loc) || !Has_dropto(loc) || !Sticky(loc))
	return;

    /* Make sure dropto loc is valid */

    dropto = Dropto(loc);
    if ((dropto == NOTHING) || (dropto == loc))
	return;

    /* Make sure no players hanging out */

    DOLIST(thing, Contents(loc)) {
	if (Dropper(thing))
	    return;
    }

    /* Send everything through the dropto */

    s_Contents(loc, reverse_list(Contents(loc)));
    SAFE_DOLIST(thing, next, Contents(loc)) {
	send_dropto(thing, player);
    }
}

/* process_dropped_dropto: Check what to do when someone drops an object. */

static void 
process_dropped_dropto(dbref thing, dbref player)
{
    dbref loc;

    /* If STICKY, send home */

    if (Sticky(thing)) {
	move_via_generic(thing, HOME, player, 0);
	divest_object(thing);
	return;
    }
    /* Process the dropto if location is a room and is not STICKY */

    loc = Location(thing);
    if (Good_obj(loc) && Has_dropto(loc) && (Dropto(loc) != NOTHING) && !Sticky(loc))
	send_dropto(thing, player);
}

/* ---------------------------------------------------------------------------
 * move_via_generic: Generic move routine, generates standard messages and
 * actions.
 */

void 
move_via_generic(dbref thing, dbref dest, dbref cause, int hush)
{
    dbref src;
    int canhear;

    if (dest == HOME)
	dest = Home(thing);
/*  if ((dest != NOTHING) && (!Good_obj(dest) || Going(dest) || Recover(dest))) {
        dest = new_home(thing);
    } */
    src = Location(thing);
    canhear = Hearer(thing);
    process_leave_loc(thing, dest, cause, canhear, hush);
    move_object(thing, dest);
    did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE,
	   (char **) NULL, 0);
    process_enter_loc(thing, src, cause, canhear, hush);
}

/* ---------------------------------------------------------------------------
 * move_via_exit: Exit move routine, generic + exit messages + dropto check.
 */

void 
move_via_exit(dbref thing, dbref dest, dbref cause, dbref exit, int hush)
{
    dbref src;
    int canhear, darkwiz, quiet, pattr, oattr, aattr;

    if (dest == HOME)
	dest = Home(thing);
    if ((dest != NOTHING) && (!Good_obj(dest) || Going(dest) || Recover(dest))) {
      return;
    }
    src = Location(thing);
    canhear = Hearer(thing);

    /* Dark wizards don't trigger OSUCC/ASUCC */

    darkwiz = (Wizard(thing) && Dark(thing));
    quiet = darkwiz || (hush & HUSH_EXIT);

    oattr = quiet ? 0 : A_OSUCC;
    aattr = quiet ? 0 : A_ASUCC;
    pattr = (!mudconf.terse_movemsg && Terse(thing)) ? 0 : A_SUCC;
    if ( pattr ) {
       pattr = (!mudconf.terse_movemsg && (Good_chk(dest) && Terse(dest) && isRoom(dest))) ? 0 : A_SUCC;
    }
    if ( Good_obj(exit) && ((Blind(exit) && !mudconf.always_blind) || (!Blind(exit) && mudconf.always_blind)) )
       hush = (hush | HUSH_QUIET | HUSH_BLIND);
    did_it(thing, exit, pattr, NULL, oattr, NULL, aattr,
	   (char **) NULL, 0);
    process_leave_loc(thing, dest, cause, canhear, hush);
    move_object(thing, dest);

    /* Dark wizards don't trigger ODROP/ADROP */

    oattr = quiet ? 0 : A_ODROP;
    aattr = quiet ? 0 : A_ADROP;
    pattr = (!mudconf.terse_movemsg && Terse(thing)) ? 0 : A_DROP;
    if ( pattr ) {
       pattr = (!mudconf.terse_movemsg && (Good_chk(dest) && Terse(dest) && isRoom(dest))) ? 0 : A_DROP;
    }
    did_it(thing, exit, pattr, NULL, oattr, NULL, aattr,
	   (char **) NULL, 0);

    did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE,
	   (char **) NULL, 0);
    process_enter_loc(thing, src, cause, canhear, hush);
    process_sticky_dropto(src, thing);
}

/* ---------------------------------------------------------------------------
 * move_via_teleport: Teleport move routine, generic + teleport messages +
 * divestiture + dropto check.
 */

int 
move_via_teleport(dbref thing, dbref dest, dbref cause, int hush, int quiet)
{
    dbref src, curr, exit1, exit2;
    int canhear, count;
    char *failmsg;

    if (isExit(thing))
      src = Exits(thing);
    else
      src = Location(thing);
    if ((dest != HOME) && Good_obj(src)) {
	curr = src;
	for (count = mudconf.ntfy_nest_lim; count > 0; count--) {
	    if (!could_doit(cause, curr, A_LTELOUT,1,0) && (!Controls(cause, curr) ||
	      DePriv(cause, NOTHING, DP_OVERRIDE, POWER7, POWER_LEVEL_NA) ||
              NoOverride(cause) || !(mudconf.wiz_override) ||
		   DePriv(cause, Owner(curr), DP_LOCKS, POWER6, NOTHING))) {
		if ((thing == cause) || (cause == NOTHING))
		    failmsg = (char *)
			"You can't teleport out!";
		else
		    failmsg = (char *)
			"You can't teleport that out!";
		if (thing != cause)
		  notify(cause,failmsg);
		if (!isExit(thing) && !quiet)
		  did_it(thing, src,
		       A_TOFAIL, failmsg, A_OTOFAIL, NULL,
		       A_ATOFAIL, (char **) NULL, 0);
		return 0;
	    }
	    if (isRoom(curr))
		break;
	    curr = Location(curr);
	    if (!Good_obj(curr))
	      break;
	}
    }
    if ((dest == HOME) && !isExit(thing))
	dest = Home(thing);
    if ((dest != NOTHING) && (!Good_obj(dest) || Going(dest) || Recover(dest))) {
      return 0;
    }
    if (!isExit(thing)) {
      canhear = Hearer(thing);
      if (!quiet)
        did_it(thing, thing, 0, NULL, A_OXTPORT, NULL, 0, (char **) NULL, 0);
      if (quiet)
        hush = HUSH_QUIET;
      process_leave_loc(thing, dest, NOTHING, canhear, hush);
      move_object(thing, dest);
      if (!quiet) {
        did_it(thing, thing, A_TPORT, NULL, A_OTPORT, NULL, A_ATPORT,
	   (char **) NULL, 0);
        did_it(thing, thing, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE,
	   (char **) NULL, 0);
      }
      if (quiet)
        hush = HUSH_QUIET;
      process_enter_loc(thing, src, NOTHING, canhear, hush);
      divest_object(thing);
      process_sticky_dropto(src, thing);
    }
    else {
      exit1 = NOTHING;
      exit2 = Exits(src);
      while ((exit2 != NOTHING) && (exit2 != thing)) {
	exit1 = exit2;
	exit2 = Next(exit2);
      }
      if (exit2 == NOTHING) {
	notify(cause, "Internal mush error.");
	STARTLOG(LOG_WIZARD | LOG_BUGS | LOG_PROBLEMS, "DB", "ERROR")
	  log_text("Exit not found in source list");
	  log_number(thing);
	ENDLOG
	return 0;
      }
      if (exit1 == NOTHING)
	s_Exits(src, Next(exit2));
      else
	s_Next(exit1, Next(exit2));
      s_Exits(exit2, dest);
      s_Next(exit2, Exits(dest));
      s_Exits(dest, exit2);
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * move_exit: Try to move a player through an exit.
 */

void 
move_exit(dbref player, dbref exit, int divest, const char *failmsg,
	  int hush)
{
    dbref loc, aowner;
    int oattr, aattr, x, aflags;
    char *retbuff, *atext, *savereg[MAX_GLOBAL_REGS], *pt;

    oattr = aattr = 0;
    loc = Location(exit);
    if (loc == HOME)
	loc = Home(player);
    if ( VariableExit(exit) && !mudstate.chkcpu_toggle ) {
       atext = atr_pget(exit, A_EXITTO, &aowner, &aflags);
       if ( *atext ) {
          mudstate.chkcpu_stopper = time(NULL);
          mudstate.chkcpu_toggle = 0;
          for (x = 0; x < MAX_GLOBAL_REGS; x++) {
             savereg[x] = alloc_lbuf("ulocal_reg_moveexit");
             pt = savereg[x];
             safe_str(mudstate.global_regs[x],savereg[x],&pt);
          }
          retbuff = exec(exit, player, player, EV_FIGNORE|EV_EVAL|EV_TOP, atext, (char **) NULL, 0);
          if ( !*retbuff ) {
             loc = NOTHING;
          } else if ( *retbuff && (stricmp(retbuff, "home") == 0) ) {
             loc = Home(player);
          } else {
             if ( (strlen(retbuff) > 1) && (*retbuff == NUMBER_TOKEN) ) {
                loc = parse_dbref(retbuff+1);
                if ( !Good_obj(loc) || Recover(loc) || Going(loc) || !Linkable(Owner(exit), loc) ) {
                   loc = NOTHING;
                }
             } else {
                loc = NOTHING;
             }
          }
          free_lbuf(retbuff);
          for (x = 0; x < MAX_GLOBAL_REGS; x++) {
             pt = mudstate.global_regs[x];
             safe_str(savereg[x],mudstate.global_regs[x],&pt);
             free_lbuf(savereg[x]);
          }
       } else {
          loc = NOTHING;
       }
       free_lbuf(atext);
    }
    if ( !Good_obj(loc) || (loc == NOTHING) ) {
        if ( loc == NOTHING )
        {
          if ((Wizard(player) && Dark(player)) || (hush & HUSH_EXIT)) {
              oattr = 0;
              aattr = 0;
          } else {
              oattr = A_OFAIL;
              aattr = A_AFAIL;
          }
	   did_it(player, exit, A_FAIL, failmsg, oattr, NULL, aattr,
	         (char **) NULL, 0);
        }
        else
           notify(player,"You can't go that way.");
        return;
    }
#ifdef REALITY_LEVELS
    /* Allow player through only if the player is also real for the exit */
    if ( Good_obj(loc) && !IsReal(exit, player))
    {
        notify(player,"You can't go that way.");
        return;
    }
#endif
    if (Good_obj(loc) && could_doit(player, exit, A_LOCK,1,0)) {
	switch (Typeof(loc)) {
	case TYPE_ROOM:
	    move_via_exit(player, loc, NOTHING, exit, hush);
	    if (divest)
		divest_object(player);
	    break;
	case TYPE_PLAYER:
	case TYPE_THING:
	    if (Going(loc)) {
		notify(player, "You can't go that way.");
		return;
	    }
	    move_via_exit(player, loc, NOTHING, exit, hush);
	    divest_object(player);
	    break;
	case TYPE_EXIT:
	    notify(player, "You can't go that way.");
	    return;
	}
    } else {
	if ((Wizard(player) && Dark(player)) || (hush & HUSH_EXIT)) {
	    oattr = 0;
	    aattr = 0;
	} else {
	    oattr = A_OFAIL;
	    aattr = A_AFAIL;
	}
	did_it(player, exit, A_FAIL, failmsg, oattr, NULL, aattr,
	       (char **) NULL, 0);
    }
}

/* ---------------------------------------------------------------------------
 * do_move: Move from one place to another via exits or 'home'.
 */

void 
do_move(dbref player, dbref cause, int key, char *direction)
{
    dbref exit, loc;
    int i, quiet;
    char *tpr_buff, *tprp_buff;

    if ((!Fubar(player) && !(Flags3(player) & NOMOVE)) || (Wizard(cause))) {
	if (!string_compare(direction, "home") &&
            !(Flags2(player) & NO_TEL) &&
            !(cmdtest(player, "home")) &&
            check_access(player, mudconf.restrict_home, mudconf.restrict_home2, 0)) {	/* go home w/o stuff */
	    loc = Location(player);
            if (!Good_obj(loc) || Going(loc) || Recover(loc)) {
               notify(player, "Bad location.");
               return;
            }
	    if (Good_obj(loc) && !Recover(loc) && !Going(loc) &&
		!Dark(player) && !Dark(loc)) {

		/* tell all */

              tprp_buff = tpr_buff = alloc_lbuf("do_move");
#ifdef REALITY_LEVELS
              notify_except_rlevel(loc, player, player,
                                   safe_tprintf(tpr_buff, &tprp_buff, "%s goes home.", Name(player)), 0);
#else
              notify_except(loc, player, player,
                            safe_tprintf(tpr_buff, &tprp_buff, "%s goes home.", Name(player)), 0);
#endif
              free_lbuf(tpr_buff);
	    }
	    /* give the player the messages */

	    for (i = 0; i < 3; i++)
		notify(player, "There's no place like home...");
	    move_via_generic(player, HOME, NOTHING, 0);
	    divest_object(player);
	    process_sticky_dropto(loc, player);
	    return;
	}
	/* find the exit */

	init_match_check_keys(player, direction, TYPE_EXIT);
/* Modify to check parent exits with 'go' instead of normal */
/*	match_exit();
	exit = match_result(); */
        match_exit_with_parents();
        exit = last_match_result();
	switch (exit) {
	case NOTHING:		/* try to force the object */
	    notify(player, "You can't go that way.");
	    break;
	case AMBIGUOUS:
	    notify(player, "I don't know which way you mean!");
	    break;
	default:
	    quiet = 0;
	    if ((key & MOVE_QUIET) && Controls(player, exit))
		quiet = HUSH_EXIT;
	    move_exit(player, exit, 0, "You can't go that way.", quiet);
	}
    } else
	notify_quiet(cause, "Permission Denied.");
}

/* ---------------------------------------------------------------------------
 * do_get: Get an object.
 */

void 
do_get(dbref player, dbref cause, int key, char *what)
{
    dbref thing, playerloc, thingloc;
    char *failmsg;
    int oattr, aattr, quiet;

    playerloc = Location(player);
    if (!Good_obj(playerloc))
	return;

    /* You can only pick up things in rooms and ENTER_OK objects/players */

    if (!isRoom(playerloc) && !Enter_ok(playerloc) &&
	!controls(player, playerloc)) {
	notify(player, "Permission denied.");
	return;
    }
    /* Look for the thing locally */

    init_match_check_keys(player, what, TYPE_THING);
    match_neighbor();
    match_exit();
    if (Privilaged(player) || HasPriv(player, NOTHING, POWER_LONG_FINGERS, POWER3, NOTHING))
	match_absolute();	/* the wizard has long fingers */
    thing = match_result();

    /* Look for the thing in other people's inventories */

    if (!Good_obj(thing))
	thing = match_status(player,
			   match_possessed(player, player, what, thing, 1));
    if (!Good_obj(thing))
	return;

    if ( Going(thing) || Recover(thing) || (Wizard(thing) && Cloak(thing) && !Wizard(player)) ||
        (Immortal(thing) && SCloak(thing) && !Immortal(player))) {
      notify(player, "I don't see that here.");
      return;
    }

    if (((Flags3(thing) & NOMOVE) || (Flags2(thing) & FUBAR)) && !Wizard(player)) {
	notify(player, "Permission denied.");
	return;
    }

    /* If we found it, get it */

    quiet = 0;
    switch (Typeof(thing)) {
    case TYPE_PLAYER:
    case TYPE_THING:
	/* You can't take what you already have */

	thingloc = Location(thing);
        if ( (playerloc == NOTHING) || (thingloc == NOTHING) ) {
	    notify(player, "Permission denied.");
	    break;
        }
	if (thingloc == player) {
	    notify(player, "You already have that!");
	    break;
	}
	if ((key & GET_QUIET) && Controls(player, thing))
	    quiet = 1;

	if (thing == player) {
	    notify(player, "You cannot get yourself!");
	} else if (!could_doit(player, playerloc, A_LGETFROM, 1, 0)) {
		notify(player, "You can not get anything at your location.");
        } else if (!could_doit(player, thingloc, A_LGETFROM, 1, 0)) {
		notify(player, "You can not get anything from that location.");
	} else if (could_doit(player, thing, A_LOCK, 1, 0)) {
	    if (thingloc != Location(player)) {
		notify(thingloc,
		       unsafe_tprintf("%s was taken from you.",
			       Name(thing)));
	    }
	    move_via_generic(thing, player, player, 0);
	    notify(thing, "Taken.");
	    oattr = quiet ? 0 : A_OSUCC;
	    aattr = quiet ? 0 : A_ASUCC;
	    did_it(player, thing, A_SUCC, "Taken.", oattr, NULL,
		   aattr, (char **) NULL, 0);
	} else {
	    oattr = quiet ? 0 : A_OFAIL;
	    aattr = quiet ? 0 : A_AFAIL;
	    if (thingloc != Location(player))
		failmsg = (char *) "You can't take that from there.";
	    else
		failmsg = (char *) "You can't pick that up.";
	    did_it(player, thing,
		   A_FAIL, failmsg,
		   oattr, NULL, aattr, (char **) NULL, 0);
	}
	break;
    case TYPE_EXIT:
	/* You can't take what you already have */

	thingloc = Exits(thing);
	if (thingloc == player) {
	    notify(player, "You already have that!");
	    break;
	}
	/* You must control either the exit or the location */

	playerloc = Location(player);
        if ( (playerloc == NOTHING) || (thingloc == NOTHING) ) {
	    notify(player, "Permission denied.");
	    break;
        }
	if (!Controls(player, thing) && !Controls(player, playerloc)) {
	    notify(player, "Permission denied.");
	    break;
	} else if (!could_doit(player, playerloc, A_LGETFROM, 1, 0)) {
	    notify(player, "You can not get anything at your location.");
	    break;
        } else if (!could_doit(player, thingloc, A_LGETFROM, 1, 0)) {
	    notify(player, "You can not get anything from that location.");
	    break;
        }
	/* Do it */

	s_Exits(thingloc, remove_first(Exits(thingloc), thing));
	s_Exits(player, insert_first(Exits(player), thing));
	s_Exits(thing, player);
	if (!Quiet(player))
	    notify(player, "Exit taken.");
	break;
    default:
	notify(player, "You can't take that!");
	break;
    }
}

/* ---------------------------------------------------------------------------
 * do_drop: Drop an object.
 */

void 
do_drop(dbref player, dbref cause, int key, char *name)
{
    dbref loc, exitloc, thing;
    char *buf;
    int quiet, oattr, aattr;

    loc = Location(player);
    if (!Good_obj(loc))
	return;

    init_match(player, name, TYPE_THING);
    match_possession();
    match_carried_exit();

    thing = match_result();
    switch (thing) {
    case NOTHING:
	notify(player, "You don't have that!");
	return;
    case AMBIGUOUS:
	notify(player, "I don't know which you mean!");
	return;
    }
/*  if (Cloak(thing) && !Controls(player,thing)) */
    if ((SCloak(thing) && Cloak(thing) && !Immortal(player)) || (Cloak(thing) && !Wizard(player))) {
	thing = NOTHING;
	notify(player, "You don't have that!");
	return;
    }

    if ((Flags3(thing) & NOMOVE) && !Wizard(player)) {
	notify(player, "Permission denied.");
	return;
    }

    switch (Typeof(thing)) {
    case TYPE_THING:
    case TYPE_PLAYER:

	/* You have to be carrying it */

	if (((Location(thing) != player) && !Wizard(player)) ||
	    (!could_doit(player, thing, A_LDROP, 1, 0)) || 
            (!could_doit(player, Location(player), A_LDROPTO, 1, 0))) {
            if ( !could_doit(player, thing, A_LDROP, 1, 0))
	       did_it(player, thing, A_DFAIL, "You can't drop that.",
		      A_ODFAIL, NULL, A_ADFAIL, (char **) NULL, 0);
            else
               notify(player, "You can't drop that here.");
	    return;
	}
	/* Move it */

	move_via_generic(thing, Location(player), player, 0);
	notify(thing, "Dropped.");
	quiet = 0;
	if ((key & DROP_QUIET) && Controls(player, thing))
	    quiet = 1;
	buf = alloc_lbuf("do_drop.did_it");
	sprintf(buf, "dropped %.3900s.", Name(thing));
	oattr = quiet ? 0 : A_ODROP;
	aattr = quiet ? 0 : A_ADROP;
	did_it(player, thing, A_DROP, "Dropped.", oattr, buf,
	       aattr, (char **) NULL, 0);
	free_lbuf(buf);

	/* Process droptos */

	process_dropped_dropto(thing, player);

	break;
    case TYPE_EXIT:

	/* You have to be carrying it */

	if ((Exits(thing) != player) && !Wizard(player)) {
	    notify(player, "You can't drop that.");
	    return;
	}
	if (!Controls(player, loc)) {
	    notify(player, "Permission denied.");
	    return;
	}
	/* Do it */

	exitloc = Exits(thing);
	s_Exits(exitloc, remove_first(Exits(exitloc), thing));
	s_Exits(loc, insert_first(Exits(loc), thing));
	s_Exits(thing, loc);

	if (!Quiet(player))
	    notify(player, "Exit dropped.");
	break;
    default:
	notify(player, "You can't drop that.");
    }

}

/* ---------------------------------------------------------------------------
 * do_enter, do_leave: The enter and leave commands.
 */

void 
do_enter_internal(dbref player, dbref thing, int quiet)
{
    dbref loc;
    int oattr, aattr;

    if (!Enter_ok(thing) && !controls(player, thing)) {
	oattr = quiet ? 0 : A_OEFAIL;
	aattr = quiet ? 0 : A_AEFAIL;
	did_it(player, thing, A_EFAIL, "Permission denied.",
	       oattr, NULL, aattr, (char **) NULL, 0);
    } else if (player == thing) {
	notify(player, "You can't enter yourself!");
    } else if (could_doit(player, thing, A_LENTER, 1, 0)) {
	loc = Location(player);
	oattr = quiet ? HUSH_ENTER : 0;
	move_via_generic(player, thing, NOTHING, oattr);
	divest_object(player);
	process_sticky_dropto(loc, player);
    } else {
	oattr = quiet ? 0 : A_OEFAIL;
	aattr = quiet ? 0 : A_AEFAIL;
	did_it(player, thing, A_EFAIL, "You can't enter that.",
	       oattr, NULL, aattr, (char **) NULL, 0);
    }
}

void 
do_enter(dbref player, dbref cause, int key, char *what)
{
    dbref thing;
    int quiet;

    init_match(player, what, TYPE_THING);
    match_neighbor();
    if (Wizard(player))
	match_absolute();	/* the wizard has long fingers */

    if ((thing = noisy_match_result()) == NOTHING)
	return;

    if (Flags3(player) & NOMOVE) {
	notify(player,"Permission denied.");
	return;
    }
    switch (Typeof(thing)) {
    case TYPE_PLAYER:
    case TYPE_THING:
	quiet = 0;
	if ((key & MOVE_QUIET) && Controls(player, thing))
	    quiet = 1;
	do_enter_internal(player, thing, quiet);
	break;
    default:
	notify(player, "Permission denied.");
    }
    return;
}

void 
do_leave(dbref player, dbref cause, int key)
{
    dbref loc, thing;
    int quiet, oattr, aattr;

    loc = Location(player);

    if (!Good_obj(loc) || isRoom(loc) || Going(loc) || Recover(loc)) {
	notify(player, "You can't leave.");
	return;
    }
    if (Flags3(player) & NOMOVE) {
	notify(player, "Permission denied.");
	return;
    }
    quiet = 0;
    if ((key & MOVE_QUIET) && Controls(player, loc))
	quiet = HUSH_LEAVE;
    if (could_doit(player, loc, A_LLEAVE, 1, 0)) {
       thing = Location(loc);
       if ( Typeof(thing) == TYPE_ROOM ) {
          if (could_doit(player, thing, A_LENTER, 1, 0)) 
	     move_via_generic(player, Location(loc), NOTHING, quiet);
          else
             notify(player, "You can't enter the room that way.");
       }
       else
	  move_via_generic(player, Location(loc), NOTHING, quiet);
    } else {
	oattr = quiet ? 0 : A_OLFAIL;
	aattr = quiet ? 0 : A_ALFAIL;
	did_it(player, loc, A_LFAIL, "You can't leave.",
	       oattr, NULL, aattr, (char **) NULL, 0);
    }
}
