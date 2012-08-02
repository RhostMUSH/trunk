/* set.c -- commands which set parameters */

#ifdef SOLARIS
/* Broken declarations in Solaris - Suckith */
char *index(const char *, int);
#endif
#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "match.h"
#include "interface.h"
#include "externs.h"
#include "flags.h"
#include "alloc.h"
#include "rhost_ansi.h"

extern NAMETAB indiv_attraccess_nametab[];
extern POWENT pow_table[];
extern POWENT depow_table[];
extern void depower_set(dbref, dbref, char *, int);
extern dbref    FDECL(match_thing, (dbref, char *));
extern void	FDECL(process_command, (dbref, dbref, int, char *, char *[], int, int));

dbref match_controlled(dbref player, const char *name)
{
dbref	mat;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	mat = noisy_match_result();
	if (Good_obj(mat) && !Controls(player, mat)) {
		notify_quiet(player, "Permission denied.");
		return NOTHING;
	} else {
		return (mat);
	}
}

dbref match_controlled_or_twinked(dbref player, const char *name)
{
dbref	mat;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	mat = noisy_match_result();
	if (Good_obj(mat) && !Controls(player, mat) &&
            !could_doit(player,mat,A_LTWINK,0)) {
		notify_quiet(player, "Permission denied.");
		return NOTHING;
	} else {
		return (mat);
	}
}

dbref match_affected(dbref player, const char *name)
{
dbref	mat;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	mat = noisy_match_result();
	if ((mat != NOTHING) && 
	    ((Cloak(mat) && SCloak(mat) && !Immortal(player)) ||
	    (Cloak(mat) && !Wizard(player)))) {
		notify_quiet(player, "I don't see that here.");
		return NOTHING;
	}
	if (mat != NOTHING && !Affects(player, mat)) {
		notify_quiet(player, "Permission denied.");
		return NOTHING;
	} else {
		return (mat);
	}
}

dbref match_examinable(dbref player, const char *name)
{
dbref	mat;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	mat = noisy_match_result();
	if (mat != NOTHING && !Examinable(player, mat)) {
		notify_quiet(player, "Permission denied.");
		return NOTHING;
	} else {
		return (mat);
	}
}

void do_name(dbref player, dbref cause, int key, const char *name, 
		const char *newname)
{
dbref	thing;
int	i_chk1, i_chk2;
char	*buff;

	if ((thing = match_controlled(player, name)) == NOTHING)
		return;

	/* check for bad name */
	if (*newname == '\0') {
		notify_quiet(player, "Give it what new name?");
		return;
	}

	if ((NoMod(thing) && !WizMod(player)) || (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) &&
		(Owner(player) != Owner(thing))) || (Backstage(player) && NoBackstage(thing) && !Immortal(player))) {
	  notify(player, "Permission denied.");
	  return;
	}

        i_chk1 = i_chk2 = 0;
	/* check for renaming a player */
	if (isPlayer(thing)) {
		buff = trim_spaces((char *)strip_all_special(newname));
                i_chk1 = protectname_check(Name(thing), player, 0);
                i_chk2 = protectname_check(buff, player, 0);
		if (!ok_player_name(buff) || !i_chk2 ||
			   !badname_check(buff, player)) {
			notify_quiet(player, "You can't use that name.");
			free_lbuf(buff);
			return;
		} else if (string_compare(buff, Name(thing)) &&
			   (lookup_player(NOTHING, buff, 0) != NOTHING)) {

			/* string_compare allows changing foo to Foo, etc. */

                        if ( i_chk2 != 2 ) {
			   notify_quiet(player, "That name is already in use.");
			   free_lbuf(buff);
			   return;
                        }
		}

		/* everything ok, notify */
		STARTLOG(LOG_SECURITY,"SEC","CNAME")
			log_name(thing),
			log_text((char *)" renamed to ");
			log_text(buff);
		ENDLOG
		if (Suspect(thing)) {
			raw_broadcast(0, WIZARD,
				"[Suspect] %s renamed to %s",Name(thing),buff);
		}

                if ( i_chk1 != 2 ) {
		   delete_player_name(thing, Name(thing));
                }
		s_Name(thing, buff);
                if ( i_chk2 != 2 ) {
		   add_player_name(thing, Name(thing));
                }
		if (!Quiet(player) && !Quiet(thing) && !(key & SIDEEFFECT))
			notify_quiet(player, "Name set.");
		free_lbuf(buff);
		return;
	} else {
		if (!ok_name(newname)) {
			notify_quiet(player, "That is not a reasonable name.");
			return;
		}

		/* everything ok, change the name */
		s_Name(thing, strip_all_special(newname));
		if (!Quiet(player) && !Quiet(thing) && !(key & SIDEEFFECT))
			notify_quiet(player, "Name set.");
	}
}

/* ---------------------------------------------------------------------------
 * do_alias: Make an alias for a player or object.
 */

void do_alias (dbref player, dbref cause, int key, char *name, char *alias)
{
dbref	thing, aowner;
int	aflags;
ATTR	*ap;
char	*oldalias, *trimalias;

	if ((thing = match_controlled_or_twinked(player, name)) == NOTHING)
		return;

	/* check for renaming a player */

	ap = atr_num(A_ALIAS);
	if (isPlayer(thing)) {

		/* Fetch the old alias */

		oldalias = atr_pget(thing, A_ALIAS, &aowner, &aflags);
		trimalias = trim_spaces(alias);

                if ( NoMod(thing) && !WizMod(player) ) {
			notify_quiet(player, "Permission denied.");
                } else if (!Controls(player, thing) &&
                    !could_doit(player,thing,A_LTWINK,0)) {

			/* Make sure we have rights to do it.  We can't do
			 * the normal Set_attr check because ALIAS is only
			 * writable by GOD and we want to keep people from
			 * doing &ALIAS and bypassing the player name checks.
			 */

			notify_quiet(player, "Permission denied.");
		} else if (!*trimalias) {

			/* New alias is null, just clear it */

			delete_player_name(thing, oldalias);
			atr_clr(thing, A_ALIAS);
			if (!Quiet(player))
				notify_quiet(player, "Alias removed.");
		} else if (lookup_player(NOTHING, trimalias, 0) != NOTHING) {

			/* Make sure new alias isn't already in use */

			notify_quiet(player, "That name is already in use.");
                } else if ( !protectname_check(trimalias, player, 0) ) {
			notify_quiet(player, "That name is already in use.");
		} else {

			/* Remove the old name and add the new name */

			delete_player_name(thing, oldalias);
			atr_add(thing, A_ALIAS, trimalias, Owner(player),
				aflags);
			if (add_player_name(thing, trimalias)) {
				if (!Quiet(player))
					notify_quiet(player, "Alias set.");
			} else {
				notify_quiet(player,
					"That name is already in use or is illegal, alias cleared.");
				atr_clr(thing, A_ALIAS);
			}
		}
		free_lbuf(trimalias);
		free_lbuf(oldalias);
	} else {
		atr_pget_info(thing, A_ALIAS, &aowner, &aflags);

		/* Make sure we have rights to do it */

		if (!Set_attr(player, thing, ap, aflags)) {
			notify_quiet(player, "Permission denied.");
		} else {
			atr_add(thing, A_ALIAS, alias, Owner(player), aflags);
			if (!Quiet(player))
				notify_quiet(player, "Set.");
		}
	}
}

/* ---------------------------------------------------------------------------
 * do_lock: Set a lock on an object or attribute.
 */

void do_lock(dbref player, dbref cause, int key, char *name, char *keytext)
{
dbref	thing, aowner;
int	atr, aflags, nomtest;
ATTR	*ap;
struct boolexp *okey;
time_t  tt;
char    *bufr;

	if (parse_attrib(player, name, &thing, &atr)) {
		if (atr != NOTHING) {
			if (!atr_get_info(thing, atr, &aowner, &aflags)) {
				notify_quiet(player,
					"Attribute not present on object.");
				return;
			}

			ap = atr_num(atr);

			/* You may lock an attribute if:
			 * you could write the attribute if it were stored on
			 * yourself
			 *    --and--
			 * you own the attribute or are a wizard as long as
			 * you are not #1 and are trying to do something to #1.
			 */

			nomtest = ((NoMod(thing) && !WizMod(player)) || (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) && (Owner(player) != Owner(thing))) || (Backstage(player) && NoBackstage(thing) && !Immortal(player)));
			if (ap && !nomtest && (God(player) ||
			     (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner)) ||
			     (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner) &&
					(Wizard(player) || (Admin(player) && !Wizard(thing)) ||
			       (aowner == Owner(player)))))) {
				aflags |= AF_LOCK;
				atr_set_flags(thing, atr, aflags);
				if (!Quiet(player) && !Quiet(thing))
					notify_quiet(player,
						"Attribute locked.");
			} else {
				notify_quiet(player, "Permission denied.");
			}
			return;
		}
	}
    
	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	thing = match_result();

	switch (thing) {
	case NOTHING:
		notify_quiet(player,
			"I don't see what you want to lock!");
		return;
	case AMBIGUOUS:
		notify_quiet(player,
			"I don't know which one you want to lock!");
		return;
	default:
		if (!controls(player, thing)) {
			notify_quiet(player, "You can't lock that!");
			return;
		}
	}

        if( (key == A_LZONETO || 
             key == A_LZONEWIZ) &&
            !ZoneMaster(thing) ) {
          notify_quiet(player, "That object isn't a Zone Master.");
          return;
        }

	if  ((NoMod(thing) && !WizMod(player)) || (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) && (Owner(player) != Owner(thing))) || (Backstage(player) && NoBackstage(thing) && !Immortal(player))) {
	  notify_quiet(player, "Permission denied.");
	  return;
	}

        if( key == A_LTWINK &&
            Typeof(thing) == TYPE_PLAYER ) {
          notify_quiet(player, "Warning: Setting a TwinkLock on a player is generally not a good idea.");
        }
      
	okey = parse_boolexp(player, strip_returntab(keytext,3), 0);
	if (okey == TRUE_BOOLEXP) {
		notify_quiet(player, "I don't understand that key.");
	} else {

		/* everything ok, do it */

		if (!key)
			key = A_LOCK;
                if ( mudconf.enable_tstamps && !NoTimestamp(thing) ) {
                   time(&tt);
                   bufr = (char *) ctime(&tt);
                   bufr[strlen(bufr) - 1] = '\0';
                   atr_add_raw(thing, A_MODIFY_TIME, bufr);
                }

		atr_add_raw(thing, key, unparse_boolexp_quiet(player, okey));
		if (!Quiet(player) && !Quiet(thing))
			notify_quiet(player, "Locked.");
	}
	free_boolexp(okey);
}

/* ---------------------------------------------------------------------------
 * Remove a lock from an object of attribute.
 */

void do_unlock(dbref player, dbref cause, int key, char *name)
{
dbref	thing, aowner;
int	atr, aflags;
ATTR	*ap;

	if (parse_attrib(player, name, &thing, &atr)) {
                if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
		   notify_quiet(player, "Permission denied.");
                   return;
                }
		if (atr != NOTHING) {
			if (!atr_get_info(thing, atr, &aowner, &aflags)) {
				notify_quiet(player,
					"Attribute not present on object.");
				return;
			}
			ap = atr_num(atr);

			/* You may unlock an attribute if:
			 * you could write the attribute if it were stored on
			 * yourself
			 *   --and--
			 * you own the attribute or are a wizard as long as
			 * you are not #1 and are trying to do something to #1.
			 */
		
			if (ap && (God(player) ||
			     (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner)) ||
			     (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner) &&
			      (Wizard(player) ||
					 (aowner == Owner(player)))))) {
				aflags &= ~AF_LOCK;
				atr_set_flags(thing, atr, aflags);
				if (Owner(player != Owner(thing)))
				if (!Quiet(player) && !Quiet(thing))
					notify_quiet(player,
						"Attribute unlocked.");
			} else {
				notify_quiet(player, "Permission denied.");
			}
			return;
		}
	}
    
	if (!key)
		key = A_LOCK;
	if ((thing = match_controlled(player, name)) != NOTHING) {
                if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
		   notify_quiet(player, "Permission denied.");
                   return;
                }
		atr_clr(thing, key);
		if (!Quiet(player) && !Quiet(thing))
			notify_quiet(player, "Unlocked.");
	}
}

/* ---------------------------------------------------------------------------
 * do_unlink: Unlink an exit from its destination or remove a dropto.
 */

void do_unlink(dbref player, dbref cause, int key, char *name)
{
dbref	exit;

	init_match(player, name, TYPE_EXIT);
        match_everything(0);
	exit = match_result();

	switch (exit) {
	case NOTHING:
		notify_quiet(player, "Unlink what?");
		break;
	case AMBIGUOUS:
		notify_quiet(player, "I don't know which one you mean!");
		break;
	default:
                if ( NoMod(exit) && !WizMod(player) ) {
			notify_quiet(player, "Permission denied.");
                } else if (!controls(player, exit) &&
                    !could_doit(player,exit,A_LTWINK,0)) {
			notify_quiet(player, "Permission denied.");
		} else {
			switch (Typeof(exit)) {
			case TYPE_EXIT:
				s_Location(exit, NOTHING);
				if (!Quiet(player))
					notify_quiet(player, "Unlinked.");
				break;
			case TYPE_ROOM:
				s_Dropto(exit, NOTHING);
				if (!Quiet(player))
					notify_quiet(player,
						"Dropto removed.");
				break;
                        case TYPE_PLAYER:
                                if ( Good_obj(Home(exit)) && (Owner(Home(exit)) == player) &&
                                     Good_obj(player) && Good_obj(Location(player)) &&
                                     (Home(exit) == Location(player)) && Good_obj(mudconf.default_home) &&
                                     (Typeof(mudconf.default_home) == TYPE_ROOM) ) {
                                   s_Home(exit, mudconf.default_home);
                                   notify_quiet(player, "Player's home link has been reset.");
                                } else {
                                 notify_quiet(player, "You can't unlink that!");
                                }
                              break;
			default:
				notify_quiet(player, "You can't unlink that!");
				break;
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * do_chown: Change ownership of an object or attribute.
 */

void do_chown(dbref player, dbref cause, int key, char *name, char *newown)
{
  dbref	thing, owner, aowner;
  int	atr, aflags, do_it, cost, quota;
  ATTR	*ap; 
  int     bLeaveFlags = 0;

  bLeaveFlags = (key & CHOWN_PRESERVE) && Wizard(player);
  
  if (parse_attrib(player, name, &thing, &atr)) {
    if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
      notify_quiet(player, "Permission Denied.");
      return;
    }
    if (atr != NOTHING) {
      if (!*newown) {
	owner = Owner(thing);
      } else if (!(string_compare(newown, "me"))) {
	owner = Owner(player);
      } else {
	owner = lookup_player(player, newown, 1);
      }

      /* You may chown an attr to yourself if you own the
       * object and the attr is not locked.
       * You may chown an attr to the owner of the object if
       * you own the attribute. (not anymore/zones -thorin)
       * To do anything else you must be a wizard.
       * Only #1 can chown attributes on #1.
       */
      
      if (!atr_get_info(thing, atr, &aowner, &aflags)) {
	notify_quiet(player,
		     "Attribute not present on object.");
	return;
      }
      do_it = 0;
      if (owner == NOTHING) {
	notify_quiet(player, "I couldn't find that player.");
      } else if (God(thing) && !God(player)) {
	notify_quiet(player, "Permission denied.");
      } else if ((Wizard(player) || Admin(player) || Builder(player)) && 
		 Controls(player,Owner(thing)) && Controls(player,owner)) {
	do_it = 1;
      } else if (owner == Owner(player)) {
	
	/* chown to me: only if I own the obj and !locked */
	
	if (!Controls(player, thing) || (aflags & AF_LOCK)) {
	  notify_quiet(player, "Permission denied.");
	} else {
	  do_it = 1;
	}
	
#if 0    /* Can't do this anymore with zones -Thorin */
      } else if (owner == Owner(thing)) {
	
	/* chown to obj owner: only if I own attr
	 * and !locked */
	
	if ((Owner(player) != aowner) || (aflags & AF_LOCK)) {
	  notify_quiet(player, "Permission denied.");
	} else {
	  do_it = 1;
	}
#endif 
      } else {
	notify_quiet(player, "Permission denied.");
      }
      
      if (!do_it) return;
      
      ap = atr_num(atr);
      if (!ap || !Set_attr(player, player, ap, aflags)) {
	notify_quiet(player, "Permission denied.");
	return;
      }
      
      atr_set_owner(thing, atr, owner);
      if (!Quiet(player))
	notify_quiet(player, "Attribute owner changed.");
      return;
    }
  }
  
  init_match(player, name, TYPE_THING);
  match_possession();
  match_here();
  match_exit();
  match_me();
  if(Privilaged(player) || HasPriv(player,NOTHING,POWER_CHOWN_ANYWHERE,POWER3,POWER_LEVEL_NA)) {
    match_player();
    match_absolute();
  }
  
  thing = match_result();
  switch (thing) {
  case NOTHING:
    notify_quiet(player, "You don't have that!");
    return;
  case AMBIGUOUS:
    notify_quiet(player, "I don't know which you mean!");
    return;
  }
  if (Recover(thing) && !God(player)) {
    thing = NOTHING;
    notify_quiet(player, "You don't have that!");
    return;
  }
  
  if (!*newown || !(string_compare(newown, "me"))) {
    owner = Owner(player);
  } else {
    owner = lookup_player(player, newown, 1);
  }
  
  cost = 1;
  quota = 1;
  switch (Typeof(thing)) {
  case TYPE_ROOM:
    cost = mudconf.digcost;
    quota = mudconf.room_quota;
    break;
  case TYPE_THING:
    cost = OBJECT_DEPOSIT(Pennies(thing));
    quota = mudconf.thing_quota;
    break;
  case TYPE_EXIT:
    cost = mudconf.opencost;
    quota = mudconf.exit_quota;
    break;
  case TYPE_PLAYER:
    cost = mudconf.robotcost;
    quota = mudconf.player_quota;
  }
  
  if (owner == NOTHING) {
    notify_quiet(player, "I couldn't find that player.");
  } else if ((isPlayer(thing) && !Immortal(player))
	     || 
	     (isPlayer(thing) && Immortal(thing) && !God(player))) {
    notify_quiet(player, "Players always own themselves.");
  } else if (((!controls(player, thing) && 
	       !(Chown_ok(thing) && could_doit(player, thing, A_LCHOWN,1)) &&
	       !(HasPriv(player,thing,POWER_CHOWN_OTHER,POWER3,NOTHING)) &&
	       !((owner==Owner(player)) && HasPriv(player,thing,POWER_CHOWN_ME,POWER3,NOTHING)) ) ||
	      (isThing(thing) && (Location(thing) != player) &&
	       !(HasPriv(player,NOTHING,POWER_CHOWN_ANYWHERE,POWER3,POWER_LEVEL_NA)) &&
	       !(Builder(player)))) ||
	     (!controls(player, owner) && !(HasPriv(player,owner,POWER_CHOWN_OTHER,POWER3,NOTHING)))) {
    notify_quiet(player, "Permission denied.");
  } else if ((DePriv(player,Owner(thing),DP_CHOWN_ME,POWER7,NOTHING) && (player == owner)) ||
	     (DePriv(player,Owner(thing),DP_CHOWN_OTHER,POWER7,NOTHING) && (player != Owner(thing))) ||
	     (DePriv(player,owner,DP_CHOWN_OTHER,POWER7,NOTHING) && (player != owner))) {
    notify_quiet(player, "Permission denied.");
  } else if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
    notify_quiet(player, "Permission Denied.");
  } else if (canpayfees(player, owner, cost, quota, Typeof(thing))) {
    giveto(Owner(thing), cost, NOTHING);
    if (mudconf.quotas)
      add_quota(Owner(thing), quota,Typeof(thing));
    if(God(player) || Immortal(player)) {
      if (thing == owner && Typeof(thing) != TYPE_PLAYER) {
	notify(player, "Warning: Non-player object is now owned by itself.");
      }
      s_Owner(thing, owner);
    } else {
      s_Owner(thing, Owner(owner));
    }
    atr_chown(thing);

    if (!bLeaveFlags) {
      s_Flags(thing, (Flags(thing) & ~(CHOWN_OK|INHERIT|WIZARD|IMMORTAL|ADMIN|BUILDER|GUILDMASTER)) | HALT);
    }
    halt_que(NOTHING, thing);
      
    if (!Quiet(player))
      notify_quiet(player, "Owner changed.");
  }
}

/* ---------------------------------------------------------------------------
 * do_set: Set flags or attributes on objects, or flags on attributes.
 */

static void set_attr_internal (dbref player, dbref thing, int attrnum, 
			char *attrtext, int key, dbref cause)
{
dbref	aowner, aowner2;
int	aflags, could_hear, aflags2;
char    *buff2, *buff2ret, *tpr_buff, *tprp_buff;
ATTR	*attr;

	attr = atr_num2(attrnum);
	if ( !attr || ((attr->flags) & AF_IS_LOCK) ) {
	  notify_quiet(player,"Permission denied.");
	  return;
	}
	atr_pget_info(thing, attrnum, &aowner, &aflags);
	if (attr && Set_attr(player, thing, attr, aflags)) { 
		if ((attr->check != NULL) &&
		    (!(*attr->check)(0, player, thing, attrnum, attrtext)))
			return;
                if ( Good_obj(mudconf.global_attrdefault) &&
                     !Recover(mudconf.global_attrdefault) &&
                     !Going(mudconf.global_attrdefault) &&
                     ((attr->flags & AF_ATRLOCK) || (aflags & AF_ATRLOCK)) &&
                     (thing != mudconf.global_attrdefault) ) {
                   buff2 = alloc_lbuf("global_attr_chk");
                   atr_get_str(buff2, mudconf.global_attrdefault, attrnum, &aowner2, &aflags2);
                   if ( *buff2 ) {
                      buff2ret = exec(player, mudconf.global_attrdefault, mudconf.global_attrdefault,
                                      EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &attrtext, 1);
                      if ( atoi(buff2ret) == 0 ) {
                         free_lbuf(buff2);
                         free_lbuf(buff2ret);
                         notify_quiet(player, "Permission denied: String does not match unique attribute lock.");
                         return;
                      }
                      free_lbuf(buff2ret);
                   }
                   free_lbuf(buff2);
                } 
		if (((attr->flags) & AF_NOANSI) && index(attrtext,ESC_CHAR)) {
                  if ( !((attrnum == 220) && Good_obj(thing) && (thing != NOTHING) && ExtAnsi(thing)) ) {
		     notify_quiet(player,"Ansi codes not allowed on this attribute.");
		     return;
                  }
		}
		if (((attr->flags) & AF_NORETURN) && (index(attrtext,'\n') || index(attrtext, '\r'))) {
		  notify_quiet(player,"Return codes not allowed on this attribute.");
		  return;
                }
		could_hear = Hearer(thing);
		mudstate.vlplay = player;
                if ( attrnum == A_QUEUEMAX ) 
		   atr_add(thing, attrnum, attrtext, thing, aflags);
                else
		   atr_add(thing, attrnum, attrtext, Owner(player), aflags);
                if ( (attr->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
                    STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
                    log_name_and_loc(player);
                    buff2ret = alloc_lbuf("log_attribute");
                    if ( *attrtext )
                       sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.3940s'",
                                          cause, attr->name, thing, attrtext);
                    else
                       sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to an empty string",
                                          cause, attr->name, thing);
                    log_text(buff2ret);
                    free_lbuf(buff2ret);
                    ENDLOG
                }

		handle_ears(thing, could_hear, Hearer(thing));
		if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing)) {
		    if (mudstate.vlplay != NOTHING) {
		      if ( (key & SET_NOISY) || TogNoisy(player) ) {
                        tprp_buff = tpr_buff = alloc_lbuf("set_attr_internal");
                        if ( *attrtext )
			   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s.",Name(thing),attr->name));
                        else
			   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (cleared).",Name(thing),attr->name));
                        free_lbuf(tpr_buff);
		      } else
			notify_quiet(player, "Set.");
		    }
		    else
			notify_quiet(player, "Permission denied.");
		}
	} else {
		notify_quiet(player, "Permission denied.");
	}
}

void do_toggle(dbref player, dbref cause, int key, char *name, char *toggle)
{
	dbref thing;
	char *pt1;

	if ((thing = match_controlled_or_twinked(player, name)) == NOTHING)
		return;
	if (key == TOGGLE_CHECK || (!key && !(*toggle))) {
	  pt1 = toggle_description(player, thing, 1, 0, (int *)NULL);
	  notify(player,pt1);
	  free_lbuf(pt1);
	} else {
          if ( NoMod(thing) && !WizMod(player) ) {
             notify_quiet(player, "Permission denied.");
          } else { 
             if (key == TOGGLE_CLEAR) {
	        pt1 = toggle_description(player, thing, 1, 1, (int *)NULL);
                if ( *pt1 ) 
	           toggle_set(thing, player, pt1, (key | SET_QUIET | SIDEEFFECT));
	        free_lbuf(pt1);
                notify_quiet(player, "All toggles cleared.");
             } else {
	        toggle_set(thing, player, toggle, key);
             }
          }
        }
}

void do_power(dbref player, dbref cause, int key, char *name, char *power)
{
dbref thing;
int i;
char buff[100];
char lev1[16], lev2[16], *pt1, *tpr_buff, *tprp_buff;
POWENT *tp;

	if (!Wizard(player) || DePriv(player,NOTHING,DP_POWER,POWER7,POWER_LEVEL_NA)) {
	  notify(player,errmsg(player));
	  return;
	}
	if (!key && !*name && !*power) {
	  notify(player,unsafe_tprintf("%-20s %-10s         %-20s %-10s","Power","Level","Power","Level"));
	  for (i = 0; i < 78; i++) buff[i] = '-';
	  buff[78] = '\0';
	  notify(player,buff);
	  tp = pow_table;
          tprp_buff = tpr_buff = alloc_lbuf("do_power");
	  while ((tp->powername) && ((tp+1)->powername)) {
	    switch (tp->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev1,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev1,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev1,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev1,"Councilor"); break;
		default:	strcpy(lev1,"N/A");
	    }
	    switch ((tp+1)->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev2,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev2,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev2,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev2,"Councilor"); break;
		default:	strcpy(lev2,"N/A");
	    }
            tprp_buff = tpr_buff;
	    notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s         %-20s %-10s",
                   tp->powername,lev1,(tp+1)->powername,lev2));
	    tp += 2;
	  }
          free_lbuf(tpr_buff);
	  if (tp->powername) {
	    switch (tp->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev1,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev1,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev1,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev1,"Councilor"); break;
		default:	strcpy(lev1,"N/A");
	    }
            tprp_buff = tpr_buff = alloc_lbuf("do_power");
	    notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s",tp->powername,lev1));
            free_lbuf(tpr_buff);
	  }
	  return;
	}
	if ((thing = match_controlled(player,name)) == NOTHING) 
		return;
	if (key == POWER_CHECK) {
	  pt1 = power_description(player,thing,1,1);
	  notify(player,pt1);
	  free_lbuf(pt1);
	}
	else 
	  power_set(thing, player, power, key);
}

void do_depower(dbref player, dbref cause, int key, char *name, char *power)
{
dbref thing;
int i;
char buff[100], *tpr_buff, *tprp_buff;
char lev1[16], lev2[16], *pt1;
POWENT *tp;

	if (!Immortal(player)) {
	  notify(player,errmsg(player));
	  return;
	}
	if (!key && !*name && !*power) {
	  notify(player,unsafe_tprintf("%-20s %-10s         %-20s %-10s","Depower","Level","Depower","Level"));
	  for (i = 0; i < 78; i++) buff[i] = '-';
	  buff[78] = '\0';
	  notify(player,buff);
	  tp = depow_table;
          tprp_buff = tpr_buff = alloc_lbuf("do_depower");
	  while ((tp->powername) && ((tp+1)->powername)) {
	    switch (tp->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev1,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev1,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev1,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev1,"Councilor"); break;
		case POWER_REMOVE:	strcpy(lev1,"Disabled"); break;
		default:	strcpy(lev1,"N/A");
	    }
	    switch ((tp+1)->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev2,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev2,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev2,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev2,"Councilor"); break;
		case POWER_REMOVE:	strcpy(lev2,"Disabled"); break;
		default:	strcpy(lev2,"N/A");
	    }
            tprp_buff = tpr_buff;
	    notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s         %-20s %-10s",
                   tp->powername,lev1,(tp+1)->powername,lev2));
	    tp += 2;
	  }
          free_lbuf(tpr_buff);
	  if (tp->powername) {
	    switch (tp->powerlev) {
		case POWER_LEVEL_OFF:	strcpy(lev1,"Off"); break;
		case POWER_LEVEL_GUILD:	strcpy(lev1,"Guildmaster"); break;
		case POWER_LEVEL_ARCH:	strcpy(lev1,"Architect"); break;
		case POWER_LEVEL_COUNC:	strcpy(lev1,"Councilor"); break;
		case POWER_REMOVE:	strcpy(lev1,"Disabled"); break;
		default:	strcpy(lev1,"N/A");
	    }
            tprp_buff = tpr_buff = alloc_lbuf("do_depower");
	    notify(player,safe_tprintf(tpr_buff, &tprp_buff, "%-20s %-10s",tp->powername,lev1));
            free_lbuf(tpr_buff);
	  }
	  return;
	}
	if ((thing = match_controlled(player,name)) == NOTHING) 
		return;
	if (key == POWER_CHECK) {
	  pt1 = depower_description(player,thing,1,1);
	  notify(player,pt1);
	  free_lbuf(pt1);
	}
	else 
	  depower_set(thing, player, power, key);
}

void do_set(dbref player, dbref cause, int key, char *name, char *flag)
{
dbref	thing, thing2, aowner;
char	*p, *buff, *tpr_buff, *tprp_buff;
int	atr, atr2, aflags, clear, flagvalue, could_hear, i_flagchk;
ATTR	*attr, *attr2;
int     ibf = -1;

	/* See if we have the <obj>/<attr> form, which is how you set attribute
	 * flags.
	 */
        i_flagchk = 0;
	if (parse_attrib (player, name, &thing, &atr)) {
		if (atr != NOTHING) {

			/* You must specify a flag name */

			if (!flag || !*flag) {
				notify_quiet(player,
					"I don't know what you want to set!");
				return;
			}

			/* Check for clearing */

			clear = 0;
			if (*flag == NOT_TOKEN) {
				flag++;
				clear = 1;
			}

			/* Make sure player specified a valid attribute flag */

			flagvalue = search_nametab (player,
				indiv_attraccess_nametab, flag);
			if (flagvalue == -1) {
				notify_quiet(player, "You can't set that!");
				return;
			}
                        if ( mudconf.secure_atruselock && (flagvalue == AF_USELOCK) && !AtrUse(player) ) {
				notify_quiet(player, "You can't set that!");
				return;
                        }

			/* Make sure the object has the attribute present */

			if (!atr_get_info(thing, atr, &aowner, &aflags)) {
				notify_quiet(player,
					"Attribute not present on object.");
				return;
			}

			/* Make sure we can write to the attribute */

			attr = atr_num(atr);
			if (!attr || !Set_attr(player, thing, attr, (aflags|0x80000000))) {
				notify_quiet(player, "Permission denied.");
				return;
			}

			/* Go do it */

                        i_flagchk = ((aflags & flagvalue) ? 1 : 0);
			if (clear)
				aflags &= ~flagvalue;
			else
				aflags |= flagvalue;
			could_hear = Hearer(thing);
			atr_set_flags(thing, atr, aflags);
                        if ( (key & SET_RSET) && (mudstate.lbuf_buffer) )
                           strcpy(mudstate.lbuf_buffer, flag);

			/* Tell the player about it. */

			handle_ears(thing, could_hear, Hearer(thing));
			if (!(key & SET_QUIET) &&
			    !Quiet(player) && !Quiet(thing)) {
                                tprp_buff = tpr_buff = alloc_lbuf("do_set");
				if (clear) {
				  if ( (key & SET_NOISY) || TogNoisy(player) ) {
                                        if ( give_name_aflags(player, cause, flagvalue) )
					   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (cleared flag%s%s).", 
                                                                Name(thing), attr->name,
                                                                (i_flagchk ? " " : " [again] "),
                                                                give_name_aflags(player, cause, flagvalue)));
                                        else
					   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (cleared %s).", 
                                                                Name(thing), attr->name,
                                                                (i_flagchk ? "flag" : "[again] flag")));
				  } else
					notify_quiet(player, "Cleared.");
				}
				else {
				  if ( (key & SET_NOISY) || TogNoisy(player) ) {
                                        if ( give_name_aflags(player, cause, flagvalue) )
					   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (set flag%s%s).", 
                                                                Name(thing), attr->name,
                                                                (i_flagchk ? " [again] " : " "),
                                                                give_name_aflags(player, cause, flagvalue)));
                                        else
					   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (set %s).", 
                                                                Name(thing), attr->name,
                                                                (i_flagchk ? "[again] flag" : "flag")));
				  } else
					notify_quiet(player, "Set.");
				}
                                free_lbuf(tpr_buff);
			}
			return;
		}
	}

	/* check for attribute set first */
	for (p = flag; *p && (*p != ':'); p++) ;

	if (*p) {
		*p++ = 0;
		if ((thing = match_controlled_or_twinked(player, name)) == NOTHING)
			return;
                /* always make SBUF_SIZE a null */
                if ( strlen(flag) >= SBUF_SIZE )
                   *(flag+SBUF_SIZE-1) = '\0';
		atr = mkattr(flag);

		if (atr <= 0) {
			notify_quiet(player, "Couldn't create attribute.");
			return;
		}
		attr = atr_num(atr);
		if (!attr) {
			notify_quiet(player, "Permission denied.");
			return;
		}
		atr_get_info(thing, atr, &aowner, &aflags);
		if (!Set_attr(player, thing, attr, aflags) ) {
			notify_quiet(player, "Permission denied.");
			return;
		}
		if ((aflags & AF_NOCMD) && !Immortal(player)) {
			notify_quiet(player, "No match.");
			return;
		}

		buff=alloc_lbuf("do_set");

		/* check for _ */
		if (*p == '_') {
			strcpy(buff, p + 1);
			if (!parse_attrib(player, p + 1, &thing2, &atr2) ||
			    (atr2 == NOTHING) || (!Immortal(player) && Cloak(thing2) && SCloak(thing2)) ||
                            (!Wizard(player) && Cloak(thing2))) {
				notify_quiet(player, "No match.");
				free_lbuf(buff);
				return;
			}

			attr2 = atr_num(atr2);
			p = buff;
		        atr_pget_str(buff, thing2, atr2, &aowner, &aflags, &ibf);

			if (!attr2 ||
			    !See_attr(player, thing2, attr2, aowner, aflags, 0)) {
				notify_quiet(player, "Permission denied.");
				free_lbuf(buff);
				return;
			}
		}

		/* Go set it */

		set_attr_internal(player, thing, atr, p, key, cause);
                if ( (key & SET_RSET) && (mudstate.lbuf_buffer) )
                   strcpy(mudstate.lbuf_buffer, p);
		free_lbuf(buff);
		return;
	}

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	thing = noisy_match_result();
	if (!Good_obj(thing)) 
	  return;

	/* Set or clear a flag...control checked in flag set */  

        if ( (key & SET_RSET) && (mudstate.lbuf_buffer) )
           strcpy(mudstate.lbuf_buffer, flag);
	flag_set(thing, player, flag, key);
}

void do_setattr(dbref player, dbref cause, int attrnum, int key,
		char *name, char *attrtext)
{
dbref	thing;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	thing = noisy_match_result();

	if (thing == NOTHING)
		return;
	set_attr_internal(player, thing, attrnum, attrtext, key, cause);
}

void do_mvattr (dbref player, dbref cause, int key, char *what, 
		char *args[], int nargs)
{
  dbref	thing, aowner, axowner, aowner2;
  ATTR	*in_attr, *out_attr;
  int	i, anum, in_anum, aflags, axflags, did_one, con1, twk1, no_wipe, aflags2, stop_set;
  char	*astr, *buff2, *buff2ret, *tpr_buff, *tprp_buff;
  
  /* Make sure we have something to do. */
  
  if (nargs < 2) {
    notify_quiet(player, "Nothing to do.");
    return;
  }
  
  /* FInd and make sure we control the target object. */
  
  thing = match_controlled_or_twinked(player, what);
  if (thing == NOTHING)
    return;
  
  con1 = Controls(player,thing);
  twk1 = could_doit(player,thing,A_LTWINK,0);
  /* Look up the source attribute.  If it either doesn't exist or isn't
   * readable, use an empty string.
   */
  
  in_anum = -1;
  astr = alloc_lbuf("do_mvattr");
  in_attr = atr_str(args[0]);
  no_wipe = 0;
  if (in_attr == NULL) {
    *astr = '\0';
  } else {
    atr_get_str(astr, thing, in_attr->number, &aowner, &aflags);
    if (!See_attr(player, thing, in_attr, aowner, aflags, 0)) {
      *astr = '\0';
    } else if (!con1 && twk1 && (in_attr->flags & (AF_IS_LOCK | AF_LOCK))) {
      *astr = '\0';
    } else if (!con1 && twk1 && (aflags & AF_LOCK)) {
      *astr = '\0';
    } else {
      in_anum = in_attr->number;
    }
  }
  
  /* Copy the attribute to each target in turn. */
  
  did_one = 0;
  buff2 = alloc_lbuf("global_attr_chk");
  tprp_buff = tpr_buff = alloc_lbuf("do_mvattr");
  for (i=1; i<nargs; i++) {
    stop_set = 0;
    if ( strlen(args[i]) >= SBUF_SIZE ) 
       *(args[i]+SBUF_SIZE-1)='\0';
    anum = mkattr(args[i]);
    if (anum <= 0) {
      tprp_buff = tpr_buff;
      notify_quiet(player,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s: That's not a good name for an attribute.",
			   args[i]));
      continue;
    }
    
    out_attr = atr_num(anum);
    tprp_buff = tpr_buff;
    if (!out_attr || (!con1 && twk1 && (out_attr->flags & (AF_IS_LOCK | AF_LOCK)))) {
      notify_quiet(player,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s: Permission denied.", args[i]));
      stop_set = 1;
    } else if (out_attr->number == in_anum) {
      no_wipe = 1;
      stop_set = 1;
    } else {
      atr_get_info(thing, out_attr->number, &axowner,
		   &axflags);
      if (!Set_attr(player, thing, out_attr, axflags)) {
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "%s: Permission denied.",
			     args[i]));
        stop_set = 1;
      } else if (((out_attr->flags) & AF_NOANSI) && index(astr,ESC_CHAR) &&
		 !(ExtAnsi(thing) && (out_attr->number == 220)) ) {
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "%s: Ansi codes not allowed on this attribute.",
			     args[i]));
        stop_set = 1;
      } else if (((out_attr->flags) & AF_NORETURN) && (index(astr,'\n') ||
						       index(astr,'\r'))) {
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "%s: Return codes not allowed on this attribute.",
			     args[i]));
        stop_set = 1;
      } else if (!con1 && twk1 && (axflags & AF_LOCK)) {
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "%s: Permission denied.",
			     args[i]));
        stop_set = 1;
      } else if ( Good_obj(mudconf.global_attrdefault) &&
                  !Recover(mudconf.global_attrdefault) &&
                  !Going(mudconf.global_attrdefault) &&
                  ((out_attr->flags & AF_ATRLOCK) || (axflags & AF_ATRLOCK)) &&
                  (thing != mudconf.global_attrdefault) ) {
         atr_get_str(buff2, mudconf.global_attrdefault, out_attr->number, &aowner2, &aflags2);
         if ( *buff2 ) {
            buff2ret = exec(player, mudconf.global_attrdefault, mudconf.global_attrdefault,
                            EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &astr, 1);
            if ( atoi(buff2ret) == 0 ) {
               notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                    "%s: Permission denied: String does not match unique attribute lock.", args[i]));
               stop_set = 1;
            }
            free_lbuf(buff2ret);
         }
      }
      if (!stop_set) {
	mudstate.vlplay = player;
	atr_add(thing, out_attr->number, astr,
		Owner(player), aflags);
        if ( (out_attr->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
            STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
            log_name_and_loc(player);
            buff2ret = alloc_lbuf("log_attribute");
            sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.3940s'",
                               cause, out_attr->name, thing, astr);
            log_text(buff2ret);
            free_lbuf(buff2ret);
            ENDLOG
        }
	if (mudstate.vlplay != NOTHING) {
	  if (!Quiet(player)) {
            tprp_buff = tpr_buff;
	    notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s: Set.", out_attr->name));
	  }
	  did_one = 1;
	} else {
	  notify_quiet(player, "Permission denied.");
	}
      }
    }
  }
  free_lbuf(tpr_buff);
  free_lbuf(buff2);
  
  /* Remove the source attribute if we can. */
  
  if ((in_anum > 0) && did_one && !no_wipe) {
    in_attr = atr_num(in_anum);
    tprp_buff = tpr_buff = alloc_lbuf("do_mvattr");
    if (in_attr && Set_attr(player, thing, in_attr, aflags)) {
      atr_clr(thing, in_attr->number);
      if ( (in_attr->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
          STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
          log_name_and_loc(player);
          buff2ret = alloc_lbuf("log_attribute");
          sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d cleared",
                             cause, attr->name, thing);
          log_text(buff2ret);
          free_lbuf(buff2ret);
          ENDLOG
      }
      if (!Quiet(player))
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "%s: Cleared.",
			     in_attr->name));
    } else {
      notify_quiet(player,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s: Could not remove old attribute.  Permission denied.",
			   in_attr->name));
    }
    free_lbuf(tpr_buff);
  }
  if (!did_one) {
    notify_quiet(player, "Nothing to do.  Nothing done.");
  }
  free_lbuf(astr);
}


/* ---------------------------------------------------------------------------
 * parse_attrib, parse_attrib_wild: parse <obj>/<attr> tokens.
 */

int parse_attrib_zone(dbref player, char *str, dbref *thing, int *atr)
{
ATTR	*attr;
char	*buff;
dbref	aowner;
int	aflags;

	if (!str)
		return 0;

	/* Break apart string into obj and attr.  Return on failure */

	buff=alloc_lbuf("parse_attrib");
	strcpy(buff, str);
	if (!parse_thing_slash(player, buff, &str, thing)) {
		free_lbuf(buff);
		return 0;
	}

	/* Get the named attribute from the object if we can */

	attr = atr_str(str);
	free_lbuf(buff);
	if (!attr) {
		*atr = NOTHING;
	} else {
		atr_pget_info(*thing, attr->number, &aowner, &aflags);
		if (!See_attr(player, *thing, attr, aowner, aflags, 1) &&
                    !could_doit(Owner(player), *thing, A_LZONEWIZ, 0)) {
			*atr = NOTHING;
		} else {
			*atr = attr->number;
		}
	}
	return 1;
}

int parse_attrib(dbref player, char *str, dbref *thing, int *atr)
{
ATTR	*attr;
char    *buff, *str_tmp, *tok, *stok, *tbuf;
dbref	aowner;
int	aflags;

	if (!str)
		return 0;

	/* Break apart string into obj and attr.  Return on failure */

	buff=alloc_lbuf("parse_attrib");
        str_tmp = alloc_lbuf("parse_attrib_other");
        strcpy(str_tmp, str);
        if ( strstr(str, "#lambda/") != NULL ) {
           tbuf = alloc_lbuf("parse_attrib_lambda");
           strcpy(tbuf, str);
           stok = (char *)strtok_r(tbuf, "/", &tok);
           if ( stok && *stok )
              stok = (char *)strtok_r(NULL, "/", &tok);
           else
              stok = (char *)"";
           memset(str_tmp, '\0', LBUF_SIZE);
           sprintf(str_tmp, "#%d/%s", player, (char *)"Lambda_internal_foo");
           atr_add_raw(player, A_LAMBDA, (char *)stok);
           free_lbuf(tbuf);
        } else {
           strcpy(str_tmp, str);
        }
        strcpy(str, str_tmp);
        strcpy(buff, str);
        free_lbuf(str_tmp);
        if (!parse_thing_slash(player, buff, &str, thing)) {
                free_lbuf(buff);
                return 0;
        }

	/* Get the named attribute from the object if we can */

	attr = atr_str(str);
	free_lbuf(buff);
	if (!attr) {
		*atr = NOTHING;
	} else {
		atr_pget_info(*thing, attr->number, &aowner, &aflags);
		if (!See_attr(player, *thing, attr, aowner, aflags, 1)) {
			*atr = NOTHING;
		} else {
			*atr = attr->number;
		}
	}
	return 1;
}

static void find_wild_attrs (dbref player, dbref thing, char *str, 
		int check_exclude, int hash_insert,
		int get_locks,OBLOCKMASTER *master)
{
ATTR	*attr;
char	*as;
dbref	aowner;
int	ca, ok, aflags;

	/* Walk the attribute list of the object */

	atr_push();
	for (ca=atr_head(thing, &as); ca; ca=atr_next(&as)) {
		attr = atr_num(ca);

		/* Discard bad attributes and ones we've seen before. */

		if (!attr)
			continue;

		if (check_exclude &&
		    ((attr->flags & AF_PRIVATE) ||
		     nhashfind(ca, &mudstate.parent_htab)))
			continue;

		/* If we aren't the top level remember this attr so we exclude
		 * it in any parents.
		 */

		atr_get_info(thing, ca, &aowner, &aflags);
		if (check_exclude && (aflags & AF_PRIVATE))
			continue;

		if (get_locks)
			ok = Read_attr(player, thing, attr, aowner, aflags, 0);
		else
			ok = See_attr(player, thing, attr, aowner, aflags, 0);

		/* Enforce locality restriction on descriptions */

		if (ok && (attr->number == A_DESC) && !mudconf.read_rem_desc &&
		    !Examinable(player, thing) && !nearby(player, thing))
			ok = 0;

		else if (!Wizard(player) && ((attr->flags & AF_PINVIS) || (aflags & AF_PINVIS)))
			ok = 0;

                if ( mudstate.reverse_wild == 1 ) {
		   if (ok && !quick_wild(str, (char *)attr->name)) {
			   olist_add(master,ca);
			   if (hash_insert) {
				   nhashadd(ca, (int *)attr,
					   &mudstate.parent_htab);
			   }
		   }
                } else {
		   if (ok && quick_wild(str, (char *)attr->name)) {
			   olist_add(master,ca);
			   if (hash_insert) {
				   nhashadd(ca, (int *)attr,
					   &mudstate.parent_htab);
			   }
		   }
                }
	}
	atr_pop();
}

int parse_attrib_wild(dbref player, char *str, dbref *thing, 
		int check_parents, int get_locks, int df_star, OBLOCKMASTER *master, int check_cluster)
{
char	*buff, *s_text, *s_strtok, *s_strtokptr;
dbref	parent, aflags;
int	check_exclude, hash_insert, lev, aowner;
ATTR	*attr;

	if (!str)
		return 0;

	buff=alloc_lbuf("parse_attrib_wild");
	strcpy(buff, str);

	/* Separate name and attr portions at the first / */

	if (!parse_thing_slash(player, buff, &str, thing)) {

		/* Not in obj/attr format, return if not defaulting to * */

		if (!df_star) {
			free_lbuf(buff);
			return 0;
		}

		/* Look for the object, return failure if not found */

		init_match(player, buff, NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		*thing = match_result();

		if (!Good_obj(*thing)) {
			free_lbuf(buff);
			return 0;
		}
		str = (char *)"*";
	}

	/* Check the object (and optionally all parents) for attributes */

        if ( check_cluster ) {
		check_exclude = 0;
		attr = atr_str("_CLUSTER");
                if ( attr ) {
                   s_text = atr_get(*thing, attr->number, &aowner, &aflags);
                   if ( *s_text ) {
                      s_strtok = strtok_r(s_text, " ", &s_strtokptr);
                      while ( s_strtok ) {
                         parent = match_thing(player, s_strtok);
                         if ( Good_chk(parent) && Cluster(parent) ) {  
		            find_wild_attrs(player, parent, str, check_exclude, 0, get_locks, master);
                         }
                         s_strtok = strtok_r(NULL, " ", &s_strtokptr);
                      }
                   }
                   free_lbuf(s_text);
                }
	} else if (check_parents) {
		check_exclude = 0;
		hash_insert = check_parents;
		nhashflush(&mudstate.parent_htab, 0);
		ITER_PARENTS(*thing, parent, lev) {
			if (!Good_obj(Parent(parent)))
				hash_insert = 0;
			find_wild_attrs(player, parent, str, check_exclude,
				hash_insert, get_locks,master);
			check_exclude = 1;
		}
	} else {
		find_wild_attrs(player, *thing, str, 0, 0, get_locks,master);
	}
	free_lbuf(buff);
	return 1;
}

/* ---------------------------------------------------------------------------
 * edit_string, do_edit: Modify attributes.
 */

void edit_string (char *src, char **dst, char **rdst, char *from, char *to, int key, int i_type)
{
char	*cp, *rcp, *tpr_buff, *tprp_buff;

	/* Do the substitution.  Idea for prefix/suffix from R'nice@TinyTIM */

	if (!strcmp(from, "^")) {
		/* Prepend 'to' to string */

		*dst = alloc_lbuf("edit_string.^");
		cp = *dst;
		safe_str(to, *dst, &cp);
		safe_str(src, *dst, &cp);
                if ( key == 0 ) {
	           *rdst = alloc_lbuf("edit_string_2.^");
                   rcp = *rdst;
                   safe_str(ANSI_HILITE, *rdst, &rcp);
		   safe_str(to, *rdst, &rcp);
                   safe_str(ANSI_NORMAL, *rdst, &rcp);
		   safe_str(src, *rdst, &rcp);
                   *rcp = '\0';
                }
		*cp = '\0';
	} else if (!strcmp(from, "$")) {
		/* Append 'to' to string */

		*dst = alloc_lbuf("edit_string.$");
		cp = *dst;
		safe_str(src, *dst, &cp);
		safe_str(to, *dst, &cp);
                if ( key == 0 ) {
	           *rdst = alloc_lbuf("edit_string_2.$");
                   rcp = *rdst;
                   safe_str(src, *rdst, &rcp);
                   safe_str(ANSI_HILITE, *rdst, &rcp);
		   safe_str(to, *rdst, &rcp);
                   safe_str(ANSI_NORMAL, *rdst, &rcp);
                   *rcp = '\0';
                }
		*cp = '\0';
	} else {
		/* replace all occurances of 'from' with 'to'.  Handle the
		 * special cases of from = \$ and \^.
		 */

		if (((from[0] == '\\') || (from[0] == '%')) &&
		    ((from[1] == '$') || (from[1] == '^')) &&
		    (from[2] == '\0'))
			from++;
		*dst = replace_string(from, to, src, i_type);
                if ( key == 0 ) {
                   tprp_buff = tpr_buff = alloc_lbuf("edit_string");
                   *rdst = replace_string(from, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s", ANSI_HILITE,
                                          to, ANSI_NORMAL), src, i_type);
                   free_lbuf(tpr_buff);
                }
	}
}

void do_edit(dbref player, dbref cause, int key, char *it, 
		char *args[], int nargs)
{
dbref	thing, aowner, aowner2;
int	attr, got_one, aflags, doit, aflags2, editchk, editsingle;
char	*from, *to, *result, *retresult, *atext, *buff2, *buff2ret, *tpr_buff, *tprp_buff;
ATTR	*ap;
OBLOCKMASTER master;

	/* Make sure we have something to do. */

	if ((nargs < 1) || !*args[0]) {
		notify_quiet(player, "Nothing to do.");
		return;
	}

	editchk = editsingle = 0;
	if ( key & EDIT_CHECK ) {
	   editchk = 1;
	   key = key & ~EDIT_CHECK;
	}
        if ( key & EDIT_SINGLE ) {
	   editsingle = 1;
	   key = key & ~EDIT_SINGLE;
        }
	from = args[0];
	to = (nargs >= 2) ? args[1] : (char *)"";

	/* Look for the object and get the attribute (possibly wildcarded) */

        olist_init(&master);
	if (!it || !*it || !parse_attrib_wild(player, it, &thing, 0, 0, 0, &master, 0)) {
	        olist_cleanup(&master);
		notify_quiet(player, "No match.");
		return;
	}

	/* Iterate through matching attributes, performing edit */

	got_one = 0;
	atext = alloc_lbuf("do_edit.atext");

        tprp_buff = tpr_buff = alloc_lbuf("do_edit");
	for (attr=olist_first(&master); attr!=NOTHING; attr=olist_next(&master)) {
		ap = atr_num(attr);
		if (ap) {

			/* Get the attr and make sure we can modify it. */

			atr_get_str(atext, thing, ap->number,
				&aowner, &aflags);
                        if ( ((ap->flags) & AF_NOANSI) && index(to,ESC_CHAR) &&
                             !(ExtAnsi(thing) && (ap->number == 220)) ) {
                                tprp_buff = tpr_buff;
				notify_quiet(player,
					safe_tprintf(tpr_buff, &tprp_buff, "%s: Ansi codes not allowed on this attribute.",
						ap->name));
                        }
                        else if ( ((ap->flags) & AF_NORETURN) && (index(to,'\n') || index(to, '\r'))) {
                                tprp_buff = tpr_buff;
				notify_quiet(player,
					safe_tprintf(tpr_buff, &tprp_buff, "%s: Return codes not allowed on this attribute.",
						ap->name));
                        }
			else if (Set_attr(player, thing, ap, aflags)) {

				/* Do the edit and save the result */

				got_one = 1;
				edit_string(atext, &result, &retresult, from, to, 0, editsingle);
				if (ap->check != NULL) {
					doit = (*ap->check)(0, player, thing,
						ap->number, result);
				} else {
					doit = 1;
				}
                                if ( Good_obj(mudconf.global_attrdefault) &&
                                     !Recover(mudconf.global_attrdefault) &&
                                     !Going(mudconf.global_attrdefault) &&
                                     ((ap->flags & AF_ATRLOCK) || (aflags & AF_ATRLOCK)) &&
                                     (thing != mudconf.global_attrdefault) ) {
                                   buff2 = alloc_lbuf("global_attr_chk");
                                   atr_get_str(buff2, mudconf.global_attrdefault, ap->number, &aowner2, &aflags2);
                                   if ( buff2 && *buff2 ) {
                                      buff2ret = exec(player, 
                                                      mudconf.global_attrdefault, mudconf.global_attrdefault,
                                                      EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &result, 1);
                                      if ( buff2ret && *buff2ret && atoi(buff2ret) == 0 ) {
                                         tprp_buff = tpr_buff;
                                         notify_quiet(player,
                                             safe_tprintf(tpr_buff, &tprp_buff, "%s: String does not match unique attribute lock.",
                                                      ap->name));
                                         doit = 0;
				         got_one = 0;
                                      }
                                      free_lbuf(buff2ret);
                                   }
                                   free_lbuf(buff2);
                                }
				if (doit) {
					mudstate.vlplay = player;
                                        if ( !editchk ) {
					   atr_add(thing, ap->number, result, Owner(player), aflags);
                                           if ( (ap->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
                                               STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
                                               log_name_and_loc(player);
                                               buff2ret = alloc_lbuf("log_attribute");
                                               sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.3940s'",
                                                                  cause, ap->name, thing, result);
                                               log_text(buff2ret);
                                               free_lbuf(buff2ret);
                                               ENDLOG
                                           }
                                        }
					if (!Quiet(player)) {
					    if (mudstate.vlplay != NOTHING) {
                                                tprp_buff = tpr_buff;
                                                if ( No_Ansi_Ex(player) ) {
						   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s - %s: %s",
								   (editchk ? "Check" : "Set"),
								   ap->name,
								   result));
                                                } else {
						   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s - %s: %s",
								   (editchk ? "Check" : "Set"),
								   ap->name,
								   retresult));
                                                }
					    } else
						notify_quiet(player, "Permission denied.");
					}
				}
				free_lbuf(result);
				free_lbuf(retresult);
			} else {

				/* No rights to change the attr */

                                tprp_buff = tpr_buff;
				notify_quiet(player,
					safe_tprintf(tpr_buff, &tprp_buff, "%s: Permission denied.",
						ap->name));
			}

		}
	}
        free_lbuf(tpr_buff);

	/* Clean up */

	free_lbuf(atext);
	olist_cleanup(&master);

	if (!got_one) {
		notify_quiet(player, "No matching attributes.");
	}
}

void do_wipe(dbref player, dbref cause, int key, char *it)
{
   dbref thing, aowner;
   int attr, got_one, aflags, orig_revwild;
   ATTR *ap;
   char *atext, *buff2ret, *tpr_buff, *tprp_buff;
   OBLOCKMASTER master;

   mudstate.wipe_state = 0;
   olist_init(&master);
   orig_revwild = mudstate.reverse_wild;
   if ( key & WIPE_PRESERVE )
      mudstate.reverse_wild = 1;
   if (!it || !*it || !parse_attrib_wild(player, it, &thing, 0, 0, 1, &master, 0)) {
      if ( !(key & SIDEEFFECT) )
         notify_quiet(player, "No match.");
      olist_cleanup(&master);
      mudstate.reverse_wild = orig_revwild;
      mudstate.wipe_state = -1;
      return;
   }
   mudstate.reverse_wild = orig_revwild;
   if ( (( Flags(thing) & SAFE ) || Indestructable(thing)) && mudconf.safe_wipe > 0 ) {
      if ( !(Controls( player, thing ) || could_doit(player,thing,A_LTWINK,0)) ) {
         if ( !(key & SIDEEFFECT) ) {
            notify_quiet(player, "No matching attributes.");
         }
         mudstate.wipe_state = 0;
      } else {
         if ( !(key & SIDEEFFECT) ) {
            tprp_buff = tpr_buff = alloc_lbuf("do_wipe");
            notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "You can not @wipe %s objects.", 
                         Indestructable(thing) ? "indestructable" : "safe" ) );
            free_lbuf(tpr_buff);
         }
         mudstate.wipe_state = -2;
      }
      olist_cleanup(&master);
      return;
   }

   /* Iterate through matching attributes, zapping the writable ones */

   got_one = 0;
   atext = alloc_lbuf("do_wipe.atext");
   for (attr=olist_first(&master); attr!=NOTHING; attr=olist_next(&master)) {
      ap = atr_num(attr);
      if (ap) {
         /* Get the attr and make sure we can modify it. */
         atr_get_str(atext, thing, ap->number, &aowner, &aflags);
         if (Set_attr(player, thing, ap, aflags) &&
             !(mudconf.safe_wipe && ((aflags & AF_SAFE) || (ap->flags & AF_SAFE))) ) {
            atr_clr(thing, ap->number);
            if ( (ap->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
               STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
                  log_name_and_loc(player);
                  buff2ret = alloc_lbuf("log_attribute");
                  sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d cleared",
                          cause, ap->name, thing);
                  log_text(buff2ret);
                  free_lbuf(buff2ret);
               ENDLOG
            }
            got_one = 1;
            mudstate.wipe_state++;
         }
      }
   }

   /* Clean up */
   free_lbuf(atext);
   olist_cleanup(&master);

   if (!got_one) {
      if ( !(key & SIDEEFFECT) ) {
         notify_quiet(player, "No matching attributes.");
      }
   } else {
      if (!Quiet(player) && !(key & SIDEEFFECT)) {
         tprp_buff = tpr_buff = alloc_lbuf("do_wipe");
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Wiped - %d attributes.", mudstate.wipe_state));
         free_lbuf(tpr_buff);
      }
   }
}

void do_include(dbref player, dbref cause, int key, char *string,
                char *argv[], int nargs, char *cargs[], int ncargs)
{
   dbref thing, owner;
   int attrib, flags, i;
   char *buff1, *buff1ptr, *cp, *s_buff[10];

   if ( desc_in_use != NULL ) {
      notify_quiet(player, "You can not use @include at command line.");
      return;
   }
   if ( mudstate.includenest >= 3 ) {
      notify_quiet(player, "Exceeded @include nest limit.");
      return;
   }
   if ( mudstate.includecnt >= 10 ) {
      notify_quiet(player, "Exceeded total number of @includes allowed.");
      return;
   }
   if (!parse_attrib(player, string, &thing, &attrib) || (attrib == NOTHING) || (thing == NOTHING)) {
      notify_quiet(player, "No match.");
      return;
   }
   if (!Good_chk(thing) || (!controls(player, thing) &&
       !could_doit(player,thing,A_LTWINK,0)) ) {
       notify_quiet(player, "Permission denied.");
       return;
   }
   mudstate.includecnt++;
   mudstate.includenest++;
   buff1ptr = buff1 = atr_pget(thing, attrib, &owner, &flags);
   if ( key & INCLUDE_COMMAND ) {
      if ( ((*buff1 == '$') || (*buff1 == '^')) && (strchr(buff1, ':') != NULL) ) {
         buff1ptr = strchr(buff1, ':') + 1;
      }
   }

   for (i = 0; i < 10; i++) {
      s_buff[i] = alloc_lbuf("do_include_buffers");
      if ( (i <= ncargs) && cargs[i] && *cargs[i] )
         memcpy(s_buff[i], cargs[i], LBUF_SIZE);
      if ( (i <= nargs) && argv[i] && *argv[i] )
         memcpy(s_buff[i], argv[i], LBUF_SIZE);
   }
   while (buff1ptr && !mudstate.breakst) {
      cp = parse_to(&buff1ptr, ';', 0);
      if (cp && *cp) {
         process_command(player, cause, 0, cp, s_buff, 10, InProgram(player));
      }
   }
   free_lbuf(buff1);
   for (i = 0; i < 10; i++) {
      free_lbuf(s_buff[i]);
   }

   mudstate.includenest--;
}


void do_trigger(dbref player, dbref cause, int key, char *object, 
		char *argv[], int nargs)
{
  dbref thing, owner, owner2, owner3, it;
  int attrib, flags, flags2, flags3, num, didtrig;
  char *buff1, *buff1ptr, *buff2, *charges, *buf, *myplayer, *tpr_buff, *tprp_buff;

  didtrig = it = 0;
  if ( key & TRIG_PROGRAM ) {
     buf = object;
     myplayer = parse_to(&buf, ':', 1);
     it = atoi(myplayer);
     if ( !Good_obj(it) || it == NOTHING ) {
        notify_quiet(player, "No match.");
        return;
     }
     object = buf;
     if (!parse_attrib(it, object, &thing, &attrib) || (attrib == NOTHING) || (thing == NOTHING)) {
        notify_quiet(player, "No match.");
        return;
     }
     if (!controls(it, thing) &&
         !could_doit(it,thing,A_LTWINK,0)) {
         notify_quiet(it, "Permission denied.");
         return;
     }
  } else {
     if (!parse_attrib(player, object, &thing, &attrib) || (attrib == NOTHING) || (thing == NOTHING)) {
        notify_quiet(player, "No match.");
        return;
     }
     if (!controls(player, thing) &&
         !could_doit(player,thing,A_LTWINK,0)) {
         notify_quiet(player, "Permission denied.");
         return;
     }
  }
  buff1ptr = buff1 = atr_pget(thing, attrib, &owner2, &flags2);
  if ( key & TRIG_COMMAND ) {
     if ( ((*buff1 == '$') || (*buff1 == '^')) && (strchr(buff1, ':') != NULL) ) {
        buff1ptr = strchr(buff1, ':') + 1;
     }
  }
  if ( !(key & TRIG_PROGRAM) ) {
     if (*buff1ptr) {
       charges = atr_pget(thing, A_CHARGES, &owner, &flags);
       if (*charges) {
         num = atoi(charges);
         if (num > 0) {
	   atr_add_raw(thing, A_CHARGES, myitoa(num-1));
         } else if (*(buff2 = atr_pget(thing, A_RUNOUT, &owner3, &flags3))) {
           free_lbuf(buff1);
           buff1 = buff2;
           buff1ptr = buff1;
         } else {
           tprp_buff = tpr_buff = alloc_lbuf("do_trigger");
	   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s has no more charges.", Name(thing)));
           free_lbuf(tpr_buff);
           free_lbuf(buff1);
           free_lbuf(buff2);
           free_lbuf(charges);
           return;
         }
       }
       free_lbuf(charges);
       wait_que(thing, player, 0, NOTHING, buff1ptr, argv, nargs, 
                mudstate.global_regs, mudstate.global_regsname);
     } else {
        notify_quiet(player, "No match.");
        didtrig = 1;
     }
  } else {
     if (*buff1ptr) {
        wait_que(it, player, 0, NOTHING, buff1ptr, argv, nargs, 
                 mudstate.global_regs, mudstate.global_regsname);
     } else {
        notify_quiet(player, "No match.");
        didtrig = 1;
     }
  }
  free_lbuf(buff1);

  /* XXX be more descriptive as to what was triggered? */
  if (!(key & TRIG_QUIET) && !Quiet(player) && !didtrig)
    notify_quiet(player, "Triggered.");

}

void do_use (dbref player, dbref cause, int key, char *object)
{
char	*df_use, *df_ouse, *temp;
dbref	thing, aowner;
int	aflags, doit;
int	ibf = -1;

	init_match(player, object, NOTYPE);
	match_neighbor();
	match_possession();
	if (Wizard(player)) {
		match_absolute();
		match_player();
	}
	match_me();
	match_here();
	thing = noisy_match_result();
	if (thing == NOTHING) return;

	/* Make sure player can use it */

	if (!could_doit(player, thing, A_LUSE,1)) {
		did_it(player, thing, A_UFAIL,
			"You can't figure out how to use that.",
			A_OUFAIL, NULL, A_AUFAIL, (char **)NULL, 0);
		return;
	}

        temp = alloc_lbuf("do_use");
	doit = 0;
	if (*atr_pget_str(temp, thing, A_USE, &aowner, &aflags, &ibf))
		doit = 1;
	else if (*atr_pget_str(temp, thing, A_OUSE, &aowner, &aflags, &ibf))
		doit = 1;
	else if (*atr_pget_str(temp, thing, A_AUSE, &aowner, &aflags, &ibf))
		doit = 1;
	free_lbuf(temp);

	if (doit) {
		df_use = alloc_lbuf("do_use.use");
		df_ouse = alloc_lbuf("do_use.ouse");
		sprintf(df_use, "You use %s", Name(thing));
		sprintf(df_ouse, "uses %s", Name(thing));
		did_it (player, thing, A_USE, df_use, A_OUSE, df_ouse, A_AUSE,
			(char **)NULL, 0);
		free_lbuf(df_use);
		free_lbuf(df_ouse);
	} else {
		notify_quiet(player, "You can't figure out how to use that.");
	}
}

/* ---------------------------------------------------------------------------
 * do_setvattr: Set a user-named (or possibly a predefined) attribute.
 */

void do_setvattr(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
char	*s;
int	anum;

	arg1++;					/* skip the '&' */
	for (s=arg1; *s&&!isspace((int)*s); s++) ;	/* take to the space */
	if (*s) *s++='\0';			/* split it */

       /* always make SBUF_SIZE a null */
        if ( strlen(arg1) >= SBUF_SIZE )
          *(arg1+SBUF_SIZE-1) = '\0';

	anum = mkattr(arg1);	/* Get or make attribute */
	if (anum <= 0) {
		notify_quiet(player,
			"That's not a good name for an attribute.");
		return;
	}
	do_setattr(player, cause, anum, key, s, arg2);
}


void do_setvattr_cluster(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
char	*s;
int	anum;
dbref	mything;

	arg1++;					/* skip the '>' */
        
	for (s=arg1; *s&&!isspace((int)*s); s++) ;	/* take to the space */
	if (*s) *s++='\0';			/* split it */

       /* always make SBUF_SIZE a null */
        if ( strlen(arg1) >= SBUF_SIZE )
          *(arg1+SBUF_SIZE-1) = '\0';

	anum = mkattr(arg1);	/* Get or make attribute */
	if (anum <= 0) {
		notify_quiet(player,
			"That's not a good name for an attribute.");
		return;
	}
        mything = match_thing(player, s);
        s = find_cluster(mything, player, anum);
        if ( strcmp(s, "#-1" ) == 0 ) {
           notify_quiet(player, "Invalid cluster specified.");
           free_lbuf(s);
           return;
        }

	do_setattr(player, cause, anum, key, s, arg2);
        trigger_cluster_action(mything, player);
        free_lbuf(s);
}
