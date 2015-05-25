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

extern void remote_write_obj(FILE *, dbref, int, int);
extern int remote_read_obj(FILE *, dbref, int, int, int*);
extern int remote_read_sanitize(FILE *, dbref, int, int);
extern dbref FDECL(match_thing, (dbref, char *));

void do_teleport(dbref player, dbref cause, int key, char *slist, 
		 char *dlist[], int nargs)
{
  dbref	victim, destination, loc;
  char	*to, *arg1, *arg2;
  int	con, dcount, quiet, side_effect=0, tel_bool_chk;

	/* get victim */

      tel_bool_chk = 0;
      if (key & TEL_QUIET) {
	key &= ~TEL_QUIET;
	quiet = 1;
      }
      else
	quiet =0;
      if ( key & SIDEEFFECT ) {
        key &= ~SIDEEFFECT;
	side_effect=1;
      }
      if (nargs < 1) {
	notify(player,"No destination given.");
	return;
      }
      if ((key != TEL_LIST) && (nargs > 1))
	notify(player,"Extra destinations ignored."); 
      dcount = 0;
      con = 1;
      if (key == TEL_LIST) {
	arg1 = strtok(slist," ");
	if (!arg1) {
	  notify(player,"No match.");
	}
      }
      else {
	arg1 = slist;
      }
      while (con && arg1 && ((dcount < nargs) || (nargs == 1))) {

      if (key != TEL_LIST) con = 0;
      arg2 = dlist[dcount];
      if (nargs != 1)
	dcount++;
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
      }
      else if (key == TEL_GRAB) {
	victim = lookup_player(player,arg1,0);
	if (victim == NOTHING) {
	  notify(player,"Player not found.");
	  return;
	}
	sprintf(arg1,"#%d",Location(player));
	to = arg1;
      }
      else {
	if (*arg2 == '\0') {
		victim = player;
		to = arg1;
	} else {
		init_match(player, arg1, NOTYPE);
		match_everything(0);
		victim = noisy_match_result();

		if (victim == NOTHING) {
		  if (key != TEL_LIST)
		    continue;
		  else {
		    arg1 = strtok(NULL," ");
		    continue;
		  }
		}
		to = arg2;
	}
      }

	/* Validate type of victim */

	if (!Has_location(victim) && !isExit(victim)) {
		notify_quiet(player, "You can't teleport that.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}

	/* Fail if we don't control the victim or the victim's location */
	if ((!Controls(player, victim) && !HasPriv(player,victim,POWER_TEL_ANYTHING,POWER4,POWER_CHECK_OWNER) &&
		!((key == TEL_GRAB) && HasPriv(player,victim,POWER_GRAB_PLAYER,POWER3,NOTHING)) &&
		!Controls(player, Location(victim)) && !TelOK(victim)) || Fubar(player) ||
		(Fubar(victim) && !Wizard(player)) || DePriv(player,victim,DP_TEL_ANYTHING,POWER7,NOTHING)) {

		notify_quiet(player, "Permission denied.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}
	if ((Immortal(victim) && SCloak(victim) && (!Immortal(player) || !Controls(player,victim))) ||
	   (Wizard(victim) && Cloak(victim) && !Wizard(player))) {
		notify_quiet(player,"Permission denied.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}
	if (Wizard(victim) && !Controls(player,victim) && !TelOK(victim)) {
		notify_quiet(player,"Permission denied.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}
	if ( (mudstate.remotep == victim) || (No_tel(victim) && !Wizard(player)) ) {
		if( victim == player )
			notify_quiet(player,"You aren't allowed to @tel.");
		else
			notify_quiet(player,"That can't be @tel'd.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}

	/* Check for teleporting home */

	if (!string_compare(to, "home") && !isExit(victim)) {
		(void)move_via_teleport(victim, HOME, player, 0, quiet);
		if ( !((side_effect) || Quiet(player)) )
		   notify_quiet(player,"Teleported.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	}

	if ((Privilaged(victim) && !Controls(player,victim) && !TelOK(victim) &&
	    !((key == TEL_GRAB) && HasPriv(player,victim,POWER_GRAB_PLAYER,POWER3,NOTHING)) &&
	    !(HasPriv(player,victim,POWER_TEL_ANYTHING,POWER4,POWER_CHECK_OWNER))) ||
	     DePriv(player,victim,DP_TEL_ANYTHING,POWER7,NOTHING)) {
		notify_quiet(player,"Permission denied.");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
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
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	case AMBIGUOUS:
		notify_quiet(player,
			"I don't know which destination you mean!");
		if (key != TEL_LIST)
		  continue;
		else {
		  arg1 = strtok(NULL," ");
		  continue;
		}
	default:
		if (Recover(destination) && !Immortal(player)) {
		  notify_quiet(player, "No match.");
		  if (key != TEL_LIST)
		    continue;
		  else {
		    arg1 = strtok(NULL," ");
		    continue;
		  }
		}
		if (victim == destination) {
			notify_quiet(player, "Bad destination.");
		  if (key != TEL_LIST)
		    continue;
		  else {
		    arg1 = strtok(NULL," ");
		    continue;
		  }
		}
	}

	/* If fascist teleport is on, you must control the victim's ultimate
	 * location (after LEAVEing any objects) or it must be JUMP_OK.
	 */

	if (mudconf.fascist_tport) {
		loc = where_room(victim);
		if (!Good_obj(loc) || !isRoom(loc) ||
		    (!Controls(player, loc) && !Jump_ok(loc))) {
			notify_quiet(player, "Permission denied.");
		  if (key != TEL_LIST)
		    continue;
		  else {
		    arg1 = strtok(NULL," ");
		    continue;
		  }
		}
	}
			
	if (Has_contents(destination)) {	

		/* You must control the destination, or it must be a JUMP_OK
		 * room where you pass its TELEPORT lock.
		 */

                if ( mudconf.empower_fulltel )
                   tel_bool_chk = (!Good_obj(victim) || !Good_obj(destination) ||
                                    Cloak(victim) || Cloak(destination) || (victim == 1) || (destination == 1));
                else
                   tel_bool_chk = (victim != player);

		if (((!Controls(player, destination) && 
		     !HasPriv(player,destination,POWER_TEL_ANYWHERE,POWER4,POWER_CHECK_OWNER) &&
                     (!HasPriv(player,NOTHING,POWER_FULLTEL_ANYWHERE,POWER5,POWER_LEVEL_NA) ||
                      (HasPriv(player,NOTHING,POWER_FULLTEL_ANYWHERE,POWER5,POWER_LEVEL_NA) &&
                       (God(destination) || Cloak(destination) || Going(destination) || 
                        Recover(destination) || !isPlayer(victim) || tel_bool_chk)))) ||
			DePriv(player,Owner(destination),DP_TEL_ANYWHERE,POWER7,NOTHING) ||
			DePriv(player,Owner(destination),DP_LOCKS,POWER6,NOTHING)) &&
		    (!Jump_ok(destination) ||
		     !could_doit(player, destination, A_LTPORT,1,0))) {

			/* Nope, report failure */

			if (player != victim)
				notify_quiet(player, "Permission denied.");
			if (!isExit(victim) && !((Owner(victim) != Owner(cause)) && TelOK(victim))  )
			  did_it(victim, destination,
				A_TFAIL, "You can't teleport there!",
				A_OTFAIL, 0, A_ATFAIL, (char **)NULL, 0);
		  if (key != TEL_LIST)
		    continue;
		  else {
		    arg1 = strtok(NULL," ");
		    continue;
		  }
		}

		/* We're OK, do the teleport */

		if (move_via_teleport(victim, destination, player, 0, quiet)) {
			if (player != victim) {
				if (! (Quiet(player) || (side_effect)) )
					notify_quiet(player, "Teleported.");
			}
		}
	} else if (isExit(destination) && !isExit(victim)) {
		if (Exits(destination) == Location(victim)) {
			move_exit(victim, destination, 0,
				"You can't go that way.", 0);
		} else {
			notify_quiet(player, "I can't find that exit.");
		}
	}
	if (key == TEL_LIST)
          arg1 = strtok(NULL," ");
      }
}

/* ---------------------------------------------------------------------------
 * do_force_prefixed: Interlude to do_force for the # command
 */

void do_force_prefixed (dbref player, dbref cause, int key, 
		char *command, char *args[], int nargs)
{
char	*cp;

	cp=parse_to(&command, ' ', 0);
	if (!command) return;
	while (*command && isspace((int)*command)) command++;
	if (*command)
		do_force(player, cause, key, cp, command, args, nargs);
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
	/* force victim to do command */
	wait_que(victim, player, 0, NOTHING, command, args, nargs,
		mudstate.global_regs, mudstate.global_regsname);
}

/* ---------------------------------------------------------------------------
 * do_remote: Run a command from a different location.
 */

void do_remote(dbref player, dbref cause, int key, char *loc,
    char *command, char *args[], int nargs)
{
dbref target;
char *retbuff;

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
      retbuff = exec(player, cause, cause, EV_EVAL | EV_FCHECK, loc, NULL, 0);
      target = match_thing(player, retbuff);
      free_lbuf(retbuff);
   }

  if(!Good_obj(target) ||Recover(target) || Going(target) || !Controls(player,target)) {
    notify(player,"Permission denied.");
  }
  mudstate.remote = target;
  mudstate.remotep = player;
  process_command(player, player, 0, command, args, nargs, 0);
  mudstate.remote = -1;
  mudstate.remotep = -1;
}

/* ---------------------------------------------------------------------------
 * do_turtle: Turn a player into an object with specified name.
 */

void do_turtle(dbref player, dbref cause, int key, char *turtle, char *newowner)
{
dbref	victim, recipient, loc, aowner, aowner2, newplayer;
char	*buf, *s_retval, *s_retplayer, *s_retvalbuff, *s_strtok, *s_strtokr, *s_chkattr, *s_buffptr, *s_mbuf;
int	count, aflags, aflags2, i, i_array[LIMIT_MAX];

	init_match(player, turtle, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
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
           s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
           if ( *s_chkattr ) {
              i_array[0] = i_array[2] = 0;
              i_array[4] = i_array[1] = i_array[3] = -2;
              for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
                   s_buffptr && (i < LIMIT_MAX);
                   s_buffptr = (char *) strtok(NULL, " "), i++) {
                  i_array[i] = atoi(s_buffptr);
              }
              if ( i_array[3] != -1 ) {
                 if ( (i_array[2]+1) > (i_array[3] == -2 ? (Wizard(player) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) : i_array[3]) ) {
                    notify_quiet(player,"@destruction limit maximum reached.");
                    STARTLOG(LOG_SECURITY, "SEC", "TURTLE")
                      log_text("@destruction limit maximum reached -> Player: ");
                      log_name(player);
                      log_text(" Object: ");
                      log_name(victim);
                    ENDLOG
                    broadcast_monitor(player,MF_VLIMIT,"[TURTLE] DESTROY MAXIMUM",
                            NULL, NULL, victim, 0, 0, NULL);
                    free_lbuf(s_chkattr);
                    return;
                 }
              }
              s_mbuf = alloc_mbuf("vattr_check");
              sprintf(s_mbuf, "%d %d %d %d %d", i_array[0], i_array[1],
                                             i_array[2]+1, i_array[3], i_array[4]);
              atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
              free_mbuf(s_mbuf);
           } else {
              atr_add_raw(player, A_DESTVATTRMAX, (char *)"0 -2 1 -2 -2");
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
		count = chown_all(victim, recipient);
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
char	*buf, *s_strtok, *s_strtokr, *s_chkattr, *s_buffptr, *s_mbuf;
int	count, aflags, i, i_array[LIMIT_MAX], aflags2;

	init_match(player, toad, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
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
           s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
           if ( *s_chkattr ) {
              i_array[0] = i_array[2] = 0;
              i_array[4] = i_array[1] = i_array[3] = -2;
              for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
                   s_buffptr && (i < LIMIT_MAX);
                   s_buffptr = (char *) strtok(NULL, " "), i++) {
                  i_array[i] = atoi(s_buffptr);
              }
              if ( i_array[3] != -1 ) {
                 if ( (i_array[2]+1) > (i_array[3] == -2 ? (Wizard(player) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) : i_array[3]) ) {
                    notify_quiet(player,"@destruction limit maximum reached.");
                    STARTLOG(LOG_SECURITY, "SEC", "TOAD")
                      log_text("@destruction limit maximum reached -> Player: ");
                      log_name(player);
                      log_text(" Object: ");
                      log_name(victim);
                    ENDLOG
                    broadcast_monitor(player,MF_VLIMIT,"[TOAD] DESTROY MAXIMUM",
                            NULL, NULL, victim, 0, 0, NULL);
                    free_lbuf(s_chkattr);
                    return;
                 }
              }
              s_mbuf = alloc_mbuf("vattr_check");
              sprintf(s_mbuf, "%d %d %d %d %d", i_array[0], i_array[1],
                                             i_array[2]+1, i_array[3], i_array[4]);
              atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
              free_mbuf(s_mbuf);
           } else {
              atr_add_raw(player, A_DESTVATTRMAX, (char *)"0 -2 1 -2 -2");
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
		count = chown_all(victim, recipient);
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
dbref	victim;
char	*buf;

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

	s_Pass(victim, mush_crypt((const char *)password));
	buf=alloc_lbuf("do_newpassword");
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

  notify(player,unsafe_tprintf("%-23s Port  %-16s Port    Cmds User@Site","Name", "Door Name"));
  DESC_SAFEITER_ALL(d, dnext) {
    // LENSY: FIX ME
    if ((d->flags & DS_HAS_DOOR) && (d->door_num >= 0)) 
      sprintf(buff2, "%4d  %-16s", d->door_desc, returnDoorName(d->door_num));
    else
      strcpy(buff2, "NONE");
    if (d->flags & DS_CONNECTED) 
      strcpy(buff,Name(d->player));
    else
      strcpy(buff,"NONE");
    tprp_buff = tpr_buff = alloc_lbuf("do_dolist");
    if (*(d->userid))
      notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-23s %-22s %4d %7d %s@%s",
             buff,buff2,d->descriptor,d->command_count,d->userid,d->addr));
    else
      notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-23s %-22s %4d %7d %s",
             buff,buff2,d->descriptor,d->command_count,d->addr));
    free_lbuf(tpr_buff);
  }
}

void do_selfboot(dbref player, dbref cause, int key)
{
time_t	dtime, now;
int	port, count;
DESC	*d;

  dtime = 0;
  count = 0;
  port = -1;
  time(&now);
  DESC_ITER_CONN(d) {
    if (d->player == player) {
      if (!count || (now - d->last_time) < dtime) {
	if (count)
	  boot_by_port(port,!God(player),player,NULL);
	dtime = now - d->last_time;
	port = d->descriptor;
      }
      else
	boot_by_port(d->descriptor,!God(player),player,NULL);
      count++;
    }
  }
  if (count < 2) {
    notify(player,"You only have 1 connection.");
  }
  else {
    notify(player,unsafe_tprintf("%d connection(s) closed.", count - 1));
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
    notify(player,"Quota transfered.");
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
{ NULL,				0,	0,		0, 0}};

void do_global (dbref player, dbref cause, int key, char *flag)
{
int	flagvalue;

	/* Set or clear the indicated flag */

	flagvalue = search_nametab(player, enable_names, flag);
	if (flagvalue == -1) {
		notify_quiet(player, "I don't know about that flag.");
	} else if (key == GLOB_ENABLE) {
		mudconf.control_flags |= flagvalue;
		if (!Quiet(player)) notify_quiet(player, "Enabled.");
	} else if (key == GLOB_DISABLE) {
		mudconf.control_flags &= ~flagvalue;
		if (!Quiet(player)) notify_quiet(player, "Disabled.");
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

void do_site(dbref player, dbref cause, int key, char *buff1, char *buff2)
{
  SITE *pt1, *pt2;
  struct in_addr addr_num, mask_num;
  char count;
  unsigned long maskval;

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
  if ((key & SITE_SUS) || (key & SITE_ALL) || (key & SITE_TRU)) {
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
      }
      else {
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
	((key & SITE_ALL) || (!(pt1->flag) && (key & SITE_PER)) || (key == pt1->flag))) {
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

#ifndef scandir
/* Some unix systems do not handle scandir -- so we build one for them */
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
   char *tpr_buff, *tprp_buff, *s_mbname, *s_pt;
   FILE *f_snap;
   int i_dirnums, i_flag, i_count;
   dbref thing;

   i_flag = i_count = 0;
   switch (key ) {
      case SNAPSHOT_NOOPT:
      case SNAPSHOT_LIST: 
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
            if ( i_count ) {
               notify(player, "Invalid characters specified in filename.");
               free_mbuf(s_mbname);
               return;
            } else {
               sprintf(s_mbname, "%s/%d_%.60s.img", mudconf.image_dir, thing, strip_all_special(buff2));
            }
         }
         if ( (f_snap = fopen(s_mbname, "r")) != NULL ) {
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
         s_mbname = alloc_mbuf("do_snapshot");
         if ( !buff1 || !*buff1 ) {
            notify(player, "Please specify a file to delete.");
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
         fclose(f_snap);
         remove(s_mbname);
         free_mbuf(s_mbname);
         notify(player, "@snapshot: Snapshot file has been deleted.");
      break;
      case SNAPSHOT_LOAD:
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
         tprp_buff = tpr_buff = alloc_lbuf("do_snapshot");
         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@snapshot: Reading image file %s onto #%d...", s_mbname, thing));
         i_dirnums = remote_read_obj(f_snap, thing, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS, &i_count);
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
         /* Fix up the ol object - it's probably corrupted if missmatched attributes */
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
         if ( remote_read_sanitize(f_snap, NOTHING, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS) != 0 ) {
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
