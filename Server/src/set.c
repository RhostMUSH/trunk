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
#include "debug.h"

extern NAMETAB indiv_attraccess_nametab[];
extern NAMETAB lock_sw[];
extern POWENT pow_table[];
extern POWENT depow_table[];
extern void depower_set(dbref, dbref, char *, int);
extern dbref    FDECL(match_thing, (dbref, char *));
extern void	FDECL(process_command, (dbref, dbref, int, char *, char *[], int, int, int));
extern int count_chars(const char *, const char c);
static void set_attr_internal (dbref, dbref, int, char *, int, dbref, int *, int);
extern double safe_atof(char *);

int
allowed_to_lock( dbref player, int aflags, int key)
{
   int i_return, i_enactor, i_thing;
   ATTR *ap;

   i_return = 1;
   ap = atr_num(key);
   if ( (aflags != 0) || (ap && (ap->flags != 0))  ) {

      /* Establish the power of the enactor */
      if ( God(player) ) {
         i_enactor = 7;
      } else if ( Immortal(player) ) {
         i_enactor = 6;
      } else if ( Wizard(player) ) {
         i_enactor = 5;
      } else if ( Admin(player) ) {
         i_enactor = 4;
      } else if ( Builder(player) ) {
         i_enactor = 3;
      } else if ( Guildmaster(player) ) {
         i_enactor = 2;
      } else {
         i_enactor = 1;
      }

      /* Establish the power of the lock */
      if ( (aflags & AF_GOD) || (ap && (ap->flags & AF_GOD)) ) {
         i_thing = 7;
      } else if ( (aflags & AF_IMMORTAL) || (ap && (ap->flags & AF_IMMORTAL)) ) {
         i_thing = 6;
      } else if ( (aflags & AF_WIZARD) || (ap && (ap->flags & AF_WIZARD)) ) {
         i_thing = 5;
      } else if ( (aflags & AF_ADMIN) || (ap && (ap->flags & AF_ADMIN)) ) {
         i_thing = 4;
      } else if ( (aflags & AF_BUILDER) || (ap && (ap->flags & AF_BUILDER)) ) {
         i_thing = 3;
      } else if ( (aflags & AF_GUILDMASTER) || (ap && (ap->flags & AF_GUILDMASTER)) ) {
         i_thing = 2;
      } else {
         i_thing = 1;
      }
      if ( i_enactor < i_thing ) {
         i_return = 0;
      }
   }
   return(i_return);
}

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
            !could_doit(player,mat,A_LTWINK,0,0)) {
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

	if ((thing = match_controlled_or_twinked(player, name)) == NOTHING)
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
                    !could_doit(player,thing,A_LTWINK,0,0)) {

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
				if ( strchr(trimalias, ';') != NULL ) {
					notify_quiet(player, "Warning: ';' detected.  If you want multiple aliases, use @protect.");
				}
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
   dbref thing, aowner;
   int atr, aflags, nomtest;
   ATTR *ap;
   struct boolexp *okey;
   time_t tt;
   char *bufr;
   NAMETAB *nt;

   if (parse_attrib(player, name, &thing, &atr)) {
      if (atr != NOTHING) {
         if (!atr_get_info(thing, atr, &aowner, &aflags)) {
            notify_quiet(player, "Attribute not present on object.");
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

         nomtest = ((NoMod(thing) && !WizMod(player)) || 
                    (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) && (Owner(player) != Owner(thing))) || 
                    (Backstage(player) && NoBackstage(thing) && !Immortal(player)));
         if (ap && !nomtest && (God(player) ||
             (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner)) ||
             (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner) &&
             (Wizard(player) || (Admin(player) && !Wizard(thing)) || (aowner == Owner(player)))))) {
            aflags |= AF_LOCK;
            atr_set_flags(thing, atr, aflags);
            if (!Quiet(player) && !Quiet(thing)) {
               if ( TogNoisy(player) ) {
                  bufr = alloc_lbuf("do_lock");
                  sprintf(bufr, "Attribute locked - %s/%s", Name(thing), ap->name);
                  notify_quiet(player, bufr);
                  free_lbuf(bufr);
               } else {
                  notify_quiet(player, "Attribute locked.");
               }
            }
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
         notify_quiet(player, "I don't see what you want to lock!");
         return;
      case AMBIGUOUS:
         notify_quiet(player, "I don't know which one you want to lock!");
         return;
      default:
         if (!controls(player, thing)) {
            notify_quiet(player, "You can't lock that!");
            return;
         }
   }

   if( (key == A_LZONETO || key == A_LZONEWIZ) && !ZoneMaster(thing) ) {
      notify_quiet(player, "That object isn't a Zone Master.");
      return;
   }

   if ( (NoMod(thing) && !WizMod(player)) || 
        (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) && (Owner(player) != Owner(thing))) || 
        (Backstage(player) && NoBackstage(thing) && !Immortal(player))) {
      notify_quiet(player, "Permission denied.");
      return;
   }

   if (!key) {
      key = A_LOCK;
   }

   /* Check flags of lock to flags of enactor */
   if ( atr_get_info(thing, key, &aowner, &aflags) ) {
      if ( !allowed_to_lock(player, aflags, key) ) {
         notify_quiet(player, "Permission denied.");
         return;
      }
   } else {
      if ( !allowed_to_lock(player, 0, key) ) {
         notify_quiet(player, "Permission denied.");
         return;
      }
   }

   if( key == A_LTWINK && Typeof(thing) == TYPE_PLAYER ) {
      notify_quiet(player, "Warning: Setting a TwinkLock on a player is generally not a good idea.");
   }
   
   okey = parse_boolexp(player, strip_returntab(keytext,3), 0);
   if (okey == TRUE_BOOLEXP) {
      notify_quiet(player, "I don't understand that key.");
   } else {
      /* everything ok, do it */
      if ( mudconf.enable_tstamps && !NoTimestamp(thing) ) {
         time(&tt);
         bufr = (char *) ctime(&tt);
         bufr[strlen(bufr) - 1] = '\0';
         atr_add_raw(thing, A_MODIFY_TIME, bufr);
      }

      atr_add_raw(thing, key, unparse_boolexp_quiet(player, okey));
      atr_set_flags(thing, key, aflags);

      if (!Quiet(player) && !Quiet(thing)) {
         if ( TogNoisy(player) ) { 
            for ( nt = lock_sw; nt && nt->name; nt++ ) {
               if ( nt->flag == key ) 
                  break;
            }
            bufr = alloc_lbuf("mbuf_lock_noisy");
            if ( nt && nt->name ) {
               sprintf(bufr, "Locked - %s/%s", Name(thing), nt->name);
            } else {
               sprintf(bufr, "Locked - %s/%s", Name(thing), (char*)"(UNKNOWN)");
            }
            notify_quiet(player, bufr);
            free_lbuf(bufr);
         } else {
            notify_quiet(player, "Locked.");
         }
      }
   }
   free_boolexp(okey);
}

/* ---------------------------------------------------------------------------
 * Remove a lock from an object of attribute.
 */

void do_unlock(dbref player, dbref cause, int key, char *name)
{
   dbref thing, aowner;
   int atr, aflags;
   char *bufr;
   ATTR *ap;
   NAMETAB *nt;

   if (parse_attrib(player, name, &thing, &atr)) {
      if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
         notify_quiet(player, "Permission denied.");
         return;
      }
      if (atr != NOTHING) {
         if (!atr_get_info(thing, atr, &aowner, &aflags)) {
            notify_quiet(player, "Attribute not present on object.");
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
		
         if ( ap && (God(player) ||
              (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner)) ||
              (!God(thing) && Set_attr(player, player, ap, 0) && Controls(player,aowner) &&
              (Wizard(player) || (aowner == Owner(player)))))) {
            aflags &= ~AF_LOCK;
            atr_set_flags(thing, atr, aflags);
            if (Owner(player != Owner(thing))) {
               if (!Quiet(player) && !Quiet(thing)) {
                  if ( TogNoisy(player) ) {
                     bufr = alloc_lbuf("do_unlock");
                     sprintf(bufr, "Attribute unlocked - %s/%s", Name(thing), ap->name);
                     notify_quiet(player, bufr);
                     free_lbuf(bufr);
                  } else {
                     notify_quiet(player, "Attribute unlocked.");
                  }
               }
            }
         } else {
            notify_quiet(player, "Permission denied.");
         }
         return;
      }
   }
    
   if (!key) {
      key = A_LOCK;
   }

   if ((thing = match_controlled(player, name)) != NOTHING) {
      if ( Good_obj(thing) && (NoMod(thing) && !WizMod(player)) ) {
         notify_quiet(player, "Permission denied.");
         return;
      }
      /* Check flags of lock to flags of enactor */
      if ( atr_get_info(thing, key, &aowner, &aflags) ) {
         if ( !allowed_to_lock(player, aflags, key) ) {
            notify_quiet(player, "Permission denied.");
            return;
         }
      } else {
         if ( !allowed_to_lock(player, 0, key) ) {
            notify_quiet(player, "Permission denied.");
            return;
         }
      }

      atr_clr(thing, key);
      if (!Quiet(player) && !Quiet(thing)) {
         if ( TogNoisy(player) ) {
            for ( nt = lock_sw; nt && nt->name; nt++ ) {
               if ( nt->flag == key ) 
                  break;
            }
            bufr = alloc_lbuf("mbuf_lock_noisy");
            if ( nt && nt->name ) {
               sprintf(bufr, "Unlocked - %s/%s", Name(thing), nt->name);
            } else {
               sprintf(bufr, "Unlocked - %s/%s", Name(thing), (char*)"(UNKNOWN)");
            }
            notify_quiet(player, bufr);
            free_lbuf(bufr);
         } else {
            notify_quiet(player, "Unlocked.");
         }
      }
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
                    !could_doit(player,exit,A_LTWINK,0,0)) {
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
	       !(Chown_ok(thing) && could_doit(player, thing, A_LCHOWN,1,0)) &&
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
      s_Flags(thing, (Flags(thing) & ~(CHOWN_OK|INHERIT|WIZARD)) | HALT);
      s_Flags2(thing, (Flags2(thing) & ~(IMMORTAL|ADMIN|BUILDER|GUILDMASTER)));
    }
    halt_que(NOTHING, thing);
      
    if (!Quiet(player))
      notify_quiet(player, "Owner changed.");
  }
}

/* ---------------------------------------------------------------------------
 * do_set: Set flags or attributes on objects, or flags on attributes.
 */

int set_trees(dbref thing, char *attr_name, dbref owner, int flags)
{
   char *s_name, *s_ptr, *s_ptr2, *s_tmp, *s_string;
   int atr, val, i_attrcnts, aflags, i_cntr;
   dbref aowner;
   ATTR *a_name;

   s_tmp = NULL;
   i_cntr = val = i_attrcnts = 0;

   if ( strchr(attr_name, *(mudconf.tree_character)) == NULL )
      return 1;

   s_ptr2 = s_ptr = attr_name;
   s_ptr2++;
   while ( s_ptr2 && s_ptr && *s_ptr && *s_ptr2) {
      if ( (*s_ptr == *s_ptr2) && (*s_ptr == *(mudconf.tree_character)) )
         return 2;
      s_ptr++;
      s_ptr2++;
   }

   i_attrcnts = atrcint(GOD, thing, 0);
   if ( ((i_attrcnts + 1) >= mudconf.vlimit) || (i_attrcnts < 0) ) 
      return 1;

   s_name = alloc_sbuf("set_trees");   
   memset(s_name, '\0', SBUF_SIZE);
   sprintf(s_name, "%.*s", SBUF_SIZE - 1, attr_name);

   s_ptr = strchr(s_name, *(mudconf.tree_character));
   s_ptr2 = s_name;
   s_string = alloc_lbuf("set_trees2");
   sprintf(s_string, "%s", (char *) "@@ - Tree Trunk");

   /* Do attribute checking */
   s_tmp = alloc_lbuf("set_trees_contents");
   while ( s_ptr ) {
      i_cntr++;
      *s_ptr = '\0';
      atr = mkattr(s_name);
      *s_ptr = *(mudconf.tree_character);
      if ( atr < 0 ) {
         val = 1;
         break;
      }
      a_name = atr_num3(atr);
      if ( a_name ) {
         set_attr_internal(owner, thing, a_name->number, s_string, SET_TREECHK, owner, &val, 1);   
         if ( val ) 
            break;
         atr_get_str(s_tmp, thing, a_name->number, &aowner, &aflags);
         if ( *s_tmp ) {
            i_attrcnts--;
         }
         if ( i_attrcnts >= mudconf.vlimit ) {
            val = 1;
         }
      } else {
         val = 1;
      }
      if ( val ) {
         break;
      }
      s_ptr++;
      s_ptr2 = s_ptr;
      if ( !s_ptr2 )
         break;
      s_ptr = strchr(s_ptr2, *(mudconf.tree_character));
   }
   if ( (i_attrcnts + 1 + i_cntr) >= mudconf.vlimit )
      val=1;

   /* Ok, trees good, let's set them */
   if ( !val ) {
      memset(s_name, '\0', SBUF_SIZE);
      sprintf(s_name, "%.*s", SBUF_SIZE - 1, attr_name);
      s_ptr = strchr(s_name, *(mudconf.tree_character));
      s_ptr2 = s_name;
      while ( s_ptr ) {
         *s_ptr = '\0';
         atr = mkattr(s_name);
         *s_ptr = *(mudconf.tree_character);
         if ( atr < 0 ) {
            val = 1;
            break;
         }
         a_name = atr_num3(atr);
         if ( a_name ) {
            atr_get_str(s_tmp, thing, a_name->number, &aowner, &aflags);
            if ( !*s_tmp ) {
               set_attr_internal(owner, thing, a_name->number, s_string, SET_QUIET | SET_BYPASS, owner, &val, 1);   
            }
         }
         s_ptr++;
         s_ptr2 = s_ptr;
         if ( !s_ptr2 )
            break;
         s_ptr = strchr(s_ptr2, *(mudconf.tree_character));
      }
   }
   free_lbuf(s_tmp);
   free_lbuf(s_string);
   free_sbuf(s_name);
   return val;
}

void ext_set_attr_internal (dbref player, dbref thing, int attrnum, 
			char *attrtext, int key, dbref cause, int *val, int i_chk)
{
   set_attr_internal(player, thing, attrnum, attrtext, key, cause, val, i_chk);
}

static void set_attr_internal (dbref player, dbref thing, int attrnum, 
			char *attrtext, int key, dbref cause, int *val, int i_chk)
{
dbref	aowner, aowner2;
int	aflags, could_hear, aflags2;
char    *buff2, *buff2ret, *tpr_buff, *tprp_buff;
ATTR	*attr;

   if ( i_chk )
      attr = atr_num4(attrnum);
   else
      attr = atr_num2(attrnum);
   if ( !attr || ((attr->flags) & AF_IS_LOCK) ) {
      if ( !(key & SET_TREECHK) )
         notify_quiet(player,"Permission denied.");
      *val = 1;
      return;
   }

   atr_pget_info(thing, attrnum, &aowner, &aflags);
   if (attr && Set_attr(player, thing, attr, aflags)) { 
      if ( (attr->check != NULL) &&
           (!(*attr->check)(0, player, thing, attrnum, attrtext))) {
         *val = 1;
         return;
      }
      if ( Good_obj(mudconf.global_attrdefault) &&
           !Recover(mudconf.global_attrdefault) &&
           !Going(mudconf.global_attrdefault) &&
           ((attr->flags & AF_ATRLOCK) || (aflags & AF_ATRLOCK)) &&
           (thing != mudconf.global_attrdefault) ) {
         buff2 = alloc_lbuf("global_attr_chk");
         atr_get_str(buff2, mudconf.global_attrdefault, attrnum, &aowner2, &aflags2);
         if ( *buff2 ) {
            buff2ret = exec(player, mudconf.global_attrdefault, mudconf.global_attrdefault,
                            EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &attrtext, 1, (char **)NULL, 0);
            if ( atoi(buff2ret) == 0 ) {
               free_lbuf(buff2);
               free_lbuf(buff2ret);
               if ( !(key & SET_TREECHK) )
                  notify_quiet(player, "Permission denied: String does not match unique attribute lock.");
               *val = 1;
               return;
            }
            free_lbuf(buff2ret);
         }
         free_lbuf(buff2);
      } 
      if (((attr->flags) & AF_NOANSI) && index(attrtext,ESC_CHAR)) {
         if ( !((attrnum == 220) && Good_obj(thing) && (thing != NOTHING) && ExtAnsi(thing)) ) {
            if ( !(key & SET_TREECHK) )
               notify_quiet(player,"Ansi codes not allowed on this attribute.");
            *val = 1;
            return;
         }
      }
      if (((attr->flags) & AF_NORETURN) && (index(attrtext,'\n') || index(attrtext, '\r'))) {
         if ( !(key & SET_TREECHK) )
            notify_quiet(player,"Return codes not allowed on this attribute.");
         *val = 1;
         return;
      }
      *val = 0;
      if ( key & SET_TREECHK ) {
         return;
      }
      could_hear = Hearer(thing);
      mudstate.vlplay = player;

      if ( !(key & SET_TREE) && ReqTrees(thing) && !(key & SET_BYPASS) && (strchr(attr->name, *(mudconf.tree_character)) != NULL) ) {
         notify_quiet(player, "Target requires TREE set method for TREE attributes.");
         return;
      }
      if ( key & SET_TREE ) {
         *val = set_trees(thing, (char *)attr->name, Owner(player), aflags);
         if ( *val != 0 ) {
            if ( !Quiet(player) && !Quiet(thing) ) {
               buff2ret = alloc_lbuf("error_msg_set_internal");
               if ( *val == 2 ) {
                  sprintf(buff2ret, "Empty/Null branches defined in target tree '%s'.", attr->name);
               } else {
                  sprintf(buff2ret, "Unable to set attribute branches for target tree '%s'.", attr->name);
               }
               notify_quiet(player, buff2ret);
               free_lbuf(buff2ret);
            }
            return;
         }
      }
      if ( attrnum == A_QUEUEMAX ) 
         atr_add(thing, attrnum, attrtext, thing, aflags);
      else
         atr_add(thing, attrnum, attrtext, Owner(player), aflags);

      if ( (attr->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
         STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
            log_name_and_loc(player);
            buff2ret = alloc_lbuf("log_attribute");
            if ( *attrtext ) {
               sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.*s'",
                       cause, attr->name, thing, (LBUF_SIZE - 100), attrtext);
            } else {
               sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to an empty string",
                       cause, attr->name, thing);
            }
            log_text(buff2ret);
#ifndef NODEBUGMONITOR
            sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
            log_text(buff2ret);
#endif
            free_lbuf(buff2ret);
         ENDLOG
      }

      handle_ears(thing, could_hear, Hearer(thing));
      if (!(key & SET_QUIET) && !Quiet(player) && !Quiet(thing)) {
         if (mudstate.vlplay != NOTHING) {
            if ( (key & SET_NOISY) || TogNoisy(player) ) {
               tprp_buff = tpr_buff = alloc_lbuf("set_attr_internal");
               if ( *attrtext )
                  notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s",Name(thing),attr->name));
               else
                  notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s/%s (cleared)",Name(thing),attr->name));
               free_lbuf(tpr_buff);
            } else
               notify_quiet(player, "Set.");
         } else {
            notify_quiet(player, "Permission denied.");
            *val = 1;
         }
      }
   } else {
      if ( !(key & SET_TREECHK ) )
         notify_quiet(player, "Permission denied.");
      *val = 1;
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


void 
do_lset(dbref player, dbref cause, int key, char *name, char *flag)
{
   dbref it, aowner;
   int nomtest, flagvalue, aflags, i_clear, i_again;
   char *s_buff, *s_buffptr;
   ATTR *attr;

   /* @lset, @lset/list, */
   if ( !name || !*name ) {
      notify(player, "I don't see that here.");
      return;
   }
   if ( (key & LSET_LIST) && flag && *flag ) {
      notify(player, "@lset/list does not take flag argument.");
      return;
   }
   if ( !(key & LSET_LIST) && !(flag && *flag) ) {
      notify(player, "@lset without /list expects a flag to set or unset.");
      return;
   }
   i_clear = 0;
   if ( *flag == NOT_TOKEN ) {
      flag++;
      i_clear = 1;
   }
   if ( !(key & LSET_LIST) && !(flag && *flag) ) {
      notify(player, "@lset without /list expects a flag to set or unset.");
      return;
   }

   s_buffptr = s_buff = alloc_lbuf("do_lset");
   if ( !get_obj_and_lock(player, name, &it, &attr, s_buff, &s_buffptr) ) {
      notify(player, s_buff);
      free_lbuf(s_buff);
      return;
   }
   free_lbuf(s_buff);

   if ( !atr_get_info(it, attr->number, &aowner, &aflags)) {
      notify(player, "Lock not on target.");
      return;
   }

   nomtest = ( (NoMod(it) && !WizMod(player)) ||
               (DePriv(player,Owner(it),DP_MODIFY,POWER7,NOTHING) && (Owner(player) != Owner(it))) ||
               (Backstage(player) && NoBackstage(it) && !Immortal(player)));

   if ( !nomtest && 
        ( God(player) || (!God(it) && Set_attr(player, player, attr, 0) && Controls(player, aowner)) ||
          (!God(it) && Set_attr(player, player, attr, 0) && Controls(player, it) &&
          (Wizard(player) || (Admin(player) && !Wizard(it)) || (aowner == Owner(player)))) ) ) {

      if ( key & LSET_LIST ) {
         s_buffptr = s_buff = alloc_lbuf("do_lset");
         if ( aflags & AF_GOD ) {
            safe_str("god ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_IMMORTAL ) {
            safe_str("immortal ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_WIZARD ) {
            safe_str("royalty ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_ADMIN ) {
            safe_str("councilor ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_BUILDER ) {
            safe_str("architect ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_GUILDMASTER ) {
            safe_str("guildmaster ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_MDARK ) {
            safe_str("hidden ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_PRIVATE ) {
            safe_str("no_inherit ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_VISUAL ) {
            safe_str("visual ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_PINVIS ) {
            safe_str("pinvisible ", s_buff, &s_buffptr);
         }
         if ( aflags & AF_NOCLONE ) {
            safe_str("noclone ", s_buff, &s_buffptr);
         }
         s_buffptr = alloc_lbuf("do_lset_format");
         if ( !*s_buff ) {
            strcpy(s_buff, (char *)"No additional flags set.");
         }
         sprintf(s_buffptr, "@lset: [%s] on %s(#%d): %s", 
                 attr->name, Name(it), it, s_buff);
         notify(player, s_buffptr);
         free_lbuf(s_buff);
         free_lbuf(s_buffptr);
      } else {
         flagvalue = search_nametab(player, indiv_attraccess_nametab, flag);
         if ( (flagvalue == -1) || !(LSET_FLAGS & flagvalue) ) {
            notify_quiet(player, "You can't set that!");
         } else {
            i_again = aflags;
            if ( i_clear ) {
               i_again = !( aflags & flagvalue );
               aflags &= ~flagvalue;
            } else {
               i_again = ( aflags & flagvalue );
               aflags |= flagvalue;
            }
            atr_set_flags(it, attr->number, aflags);
            s_buff = alloc_lbuf("do_lset");
            sprintf(s_buff, "@lset: [%s] %s%s%s on %s(#%d)", 
                    attr->name, (i_clear ? "cleared" : "set"), 
                    (i_again ? " (again) " : " "), 
                    give_name_aflags(player, cause, flagvalue), Name(it), it);
            notify(player, s_buff);
            free_lbuf(s_buff);
         }
      }
   } else {
      notify(player, "Permission denied.");
   }
}


void 
do_set(dbref player, dbref cause, int key, char *name, char *flag) 
{
dbref	thing, thing2, aowner;
char	*p, *buff, *tpr_buff, *tprp_buff, *buff2ret;
int	atr, atr2, aflags, curraflags, clear, flagvalue, could_hear, i_flagchk, val;
ATTR	*attr, *attr2;
int     ibf = -1;

	/* See if we have the <obj>/<attr> form, which is how you set attribute
	 * flags.
	 */
        i_flagchk = val = 0;
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
			curraflags = aflags;
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
				if ( (attr->flags & AF_LOGGED) || (curraflags & AF_LOGGED) ) {
					STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
					log_name_and_loc(player);
					buff2ret = alloc_lbuf("log_attribute_set");
					sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d %s attribute flag '%s'",
						cause, attr->name, thing, (clear ? "cleared" : "set"), give_name_aflags(player, cause, flagvalue));
					log_text(buff2ret);
#ifndef NODEBUGMONITOR
					sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
					log_text(buff2ret);
#endif
					free_lbuf(buff2ret);
					ENDLOG
				}
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

		set_attr_internal(player, thing, atr, p, key, cause, &val, 0);
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
int	val;

	init_match(player, name, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	thing = noisy_match_result();

        val = 0;
	if (thing == NOTHING)
		return;
	set_attr_internal(player, thing, attrnum, attrtext, key, cause, &val, 0);
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
  twk1 = could_doit(player,thing,A_LTWINK,0,0);
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
                            EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &astr, 1, (char **)NULL, 0);
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
            sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.*s'",
                               cause, out_attr->name, thing, (LBUF_SIZE - 100), astr);
            log_text(buff2ret);
#ifndef NODEBUGMONITOR
            sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
            log_text(buff2ret);
#endif
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
#ifndef NODEBUGMONITOR
          sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
          log_text(buff2ret);
#endif
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
                    !could_doit(Owner(player), *thing, A_LZONEWIZ, 0, 0)) {
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
char    *buff, *str_tmp, *stok, *tbuf;
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
           stok = strchr(tbuf, '/')+1;
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

static void find_wild_attrs (dbref player, dbref thing, char *str, int check_exclude, int hash_insert,
		int get_locks,OBLOCKMASTER *master, int i_regexp, int i_tree)
{
ATTR	*attr, *atr2;
char	*as;
dbref	aowner;
int	ca, ok, aflags, i_nowild;

	/* Walk the attribute list of the object */

        i_nowild = 0;
        if ( *str && (!strchr(str, '*') && !strchr(str, '?')) ) {
           i_nowild = 1;
           atr2 = (ATTR *) hashfind(str, &mudstate.attr_name_htab);
           if ( !atr2 )
              i_nowild = 0;
        }
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

                else if (!God(player) && ((attr->flags & AF_GOD) || (aflags & AF_GOD)) &&
                                         ((attr->flags & AF_PINVIS) || (aflags & AF_PINVIS))) 
			ok = 0;

		else if (!Wizard(player) && ((attr->flags & AF_PINVIS) || (aflags & AF_PINVIS)))
			ok = 0;

                if ( mudstate.reverse_wild == 1 ) {
		   if (ok && ((i_nowild && (strcmp(atr2->name, attr->name) == 0)) ||
                              (!i_regexp && !quick_wild(str, (char *)attr->name)) ||
                              ( i_regexp && !quick_regexp_match(str, (char *)attr->name, 0))) ) {
                      if ( !i_tree || (i_tree && (count_chars(attr->name, *(mudconf.tree_character)) <= count_chars(str, *(mudconf.tree_character)))) ) {
			   olist_add(master,ca);
			   if (hash_insert) {
				   nhashadd(ca, (int *)attr,
					   &mudstate.parent_htab);
			   }
                      }
		   }
                } else {
		   if (ok && ((i_nowild && (strcmp(atr2->name, attr->name) == 0)) ||
                              (!i_regexp && quick_wild(str, (char *)attr->name)) ||
                              ( i_regexp && quick_regexp_match(str, (char *)attr->name, 0))) ) {
                      if ( !i_tree || (i_tree && (count_chars(attr->name, *(mudconf.tree_character)) <= count_chars(str, *(mudconf.tree_character)))) ) {
			   olist_add(master,ca);
			   if (hash_insert) {
				   nhashadd(ca, (int *)attr,
					   &mudstate.parent_htab);
			   }
                      }
		   }
                }
	}
	atr_pop();
}

int parse_attrib_wild(dbref player, char *str, dbref *thing, int check_parents, int get_locks, 
                      int df_star, OBLOCKMASTER *master, int check_cluster, int i_regexp, int i_tree)
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
		            find_wild_attrs(player, parent, str, check_exclude, 0, get_locks, master, i_regexp, i_tree);
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
			find_wild_attrs(player, parent, str, check_exclude, hash_insert, get_locks,master, i_regexp, i_tree);
			check_exclude = 1;
		}
	} else {
		find_wild_attrs(player, *thing, str, 0, 0, get_locks,master, i_regexp, i_tree);
	}
	free_lbuf(buff);
	return 1;
}

/* ---------------------------------------------------------------------------
 * edit_string, do_edit: Modify attributes.
 */

void edit_string (char *src, char **dst, char **rdst, char *from, char *to, int key, int i_type, int i_compat, int i_redeye)
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
#ifdef ZENTY_ANSI
                   if ( i_redeye ) 
                      safe_str(SAFE_ANSI_RED, *rdst, &rcp);
                   else
                      safe_str(SAFE_ANSI_HILITE, *rdst, &rcp);
		   safe_str(strip_all_ansi(to), *rdst, &rcp);
                   safe_str(SAFE_ANSI_NORMAL, *rdst, &rcp);
#else
                   if ( i_redeye ) 
                      safe_str(ANSI_RED, *rdst, &rcp);
                   else
                      safe_str(ANSI_HILITE, *rdst, &rcp);
		   safe_str(strip_all_ansi(to), *rdst, &rcp);
                   safe_str(ANSI_NORMAL, *rdst, &rcp);
#endif
		   safe_str(strip_all_ansi(src), *rdst, &rcp);
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
                   safe_str(strip_all_ansi(src), *rdst, &rcp);
#ifdef ZENTY_ANSI
                   if ( i_redeye )
                      safe_str(SAFE_ANSI_RED, *rdst, &rcp);
                   else
                      safe_str(SAFE_ANSI_HILITE, *rdst, &rcp);
		   safe_str(strip_all_ansi(to), *rdst, &rcp);
                   safe_str(SAFE_ANSI_NORMAL, *rdst, &rcp);
#else
                   if ( i_redeye )
                      safe_str(ANSI_RED, *rdst, &rcp);
                   else
                      safe_str(ANSI_HILITE, *rdst, &rcp);
		   safe_str(strip_all_ansi(to), *rdst, &rcp);
                   safe_str(ANSI_NORMAL, *rdst, &rcp);
#endif
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
                if ( i_compat == 2 ) {
		   *dst = replace_string(from, to, src, i_type);
                } else {
		   *dst = replace_string_ansi(from, to, src, i_type, i_compat);
                }
                if ( key == 0 ) {
                   tprp_buff = tpr_buff = alloc_lbuf("edit_string");
#ifdef ZENTY_ANSI
                   if ( i_compat == 2 ) {
                      *rdst = replace_string(from, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s", 
                                             (i_redeye ? SAFE_ANSI_RED : SAFE_ANSI_HILITE),
                                             to, SAFE_ANSI_NORMAL), src, i_type);
                   } else {
                      *rdst = replace_string_ansi(from, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s", 
                                             (i_redeye ? SAFE_ANSI_RED : SAFE_ANSI_HILITE),
                                             strip_all_ansi(to), SAFE_ANSI_NORMAL), src, i_type, i_compat);
                   }
#else
                   if ( i_compat == 2 ) {
                      *rdst = replace_string(from, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s", 
                                             (i_redeye ? ANSI_RED : ANSI_HILITE),
                                             to, ANSI_NORMAL), src, i_type);
                   } else {
                      *rdst = replace_string(from, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s", 
                                             (i_redeye ? ANSI_RED : ANSI_HILITE),
                                             strip_all_ansi(to), ANSI_NORMAL), src, i_type);
                   }
#endif
                   free_lbuf(tpr_buff);
                }
	}
}

void do_edit(dbref player, dbref cause, int key, char *it, 
		char *args[], int nargs)
{
dbref	thing, aowner, aowner2;
int	attr, got_one, aflags, doit, aflags2, editchk, editsingle, i_compat, i_tog;
char	*from, *to, *result, *retresult, *atext, *buff2, *buff2ret, *tpr_buff, *tprp_buff;
ATTR	*ap;
OBLOCKMASTER master;

	/* Make sure we have something to do. */

	if ( (key & EDIT_STRICT) && (key & EDIT_RAW) ) {
		notify_quiet(player, "Incompatible switches to edit");
		return;
	}
	if ((nargs < 1) || !*args[0]) {
		notify_quiet(player, "Nothing to do.");
		return;
	}
	i_compat = editchk = editsingle = 0;
	if ( key & EDIT_CHECK ) {
	   editchk = 1;
	   key = key & ~EDIT_CHECK;
	}
        if ( key & EDIT_SINGLE ) {
	   editsingle = 1;
	   key = key & ~EDIT_SINGLE;
        }
	if ( key & EDIT_STRICT ) {
	   i_compat |= 1;
	   key = key & ~EDIT_STRICT;
        }
        if ( key & EDIT_RAW ) {
	   i_compat |= 2;
	   key = key & ~EDIT_RAW;
        }
        
	from = args[0];
	to = (nargs >= 2) ? args[1] : (char *)"";

	/* Look for the object and get the attribute (possibly wildcarded) */

        olist_init(&master);
	if (!it || !*it || !parse_attrib_wild(player, it, &thing, 0, 0, 0, &master, 0, 0, 0)) {
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
				edit_string(atext, &result, &retresult, from, to, 0, editsingle, i_compat, 0);
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
                                                      EV_STRIP | EV_FCHECK | EV_EVAL, buff2, &result, 1, (char **)NULL, 0);
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
                                               sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d set to '%.*s'",
                                                                  cause, ap->name, thing, (LBUF_SIZE - 100), result);
                                               log_text(buff2ret);
#ifndef NODEBUGMONITOR
                                               sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
                                               log_text(buff2ret);
#endif
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
                                                   i_tog = abs(strcmp(atext, retresult));
						   notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%s%s%s - %s: %s",
#ifdef ZENTY_ANSI
                                                                   (i_tog ? SAFE_ANSI_HILITE : SAFE_ANSI_NORMAL),
#else
                                                                   (i_tog ? ANSI_HILITE : ANSI_NORMAL),
#endif
								   (editchk ? "Check" : "Set"),
#ifdef ZENTY_ANSI
                                                                   SAFE_ANSI_NORMAL,
#else
                                                                   ANSI_NORMAL,
#endif
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

void do_wipe(dbref player, dbref cause, int key, char *it2)
{
   dbref thing, aowner, target;
   int attr, got_one, aflags, orig_revwild, i_regexp, i_owner;
   ATTR *ap;
   char *atext, *buff2ret, *tpr_buff, *tprp_buff, *it;
   OBLOCKMASTER master;

   i_owner = i_regexp = mudstate.wipe_state = 0;
   if ( key & WIPE_PRESERVE )
      mudstate.reverse_wild = 1;

   if ( key & WIPE_REGEXP )
      i_regexp = 1;

   if ( key & WIPE_OWNER ) {
      i_owner = 1;
      target = NOTHING;
      if ( (it = strchr(it2, '/')) != NULL ) {
         *it = '\0'; 
         target = lookup_player(player, it2, 0);
         *it = '/'; 
         if ( !Good_chk(target) ) {
            if ( !(key & SIDEEFFECT) )
               notify_quiet(player, "Not a valid player.");
            return;
         } else { 
            it = it+1;
         }
      } else {
         if ( !(key & SIDEEFFECT) )
            notify_quiet(player, "Need to specify user with wipe-by-user option.");
         return;
      }
   } else {
      it = it2;
   }
   
   olist_init(&master);
   orig_revwild = mudstate.reverse_wild;
   if (!it || !*it || !parse_attrib_wild(player, it, &thing, 0, 0, 1, &master, 0, i_regexp, 0)) {
      if ( !(key & SIDEEFFECT) )
         notify_quiet(player, "No match.");
      olist_cleanup(&master);
      mudstate.reverse_wild = orig_revwild;
      mudstate.wipe_state = -1;
      return;
   }
   mudstate.reverse_wild = orig_revwild;
   if ( (( Flags(thing) & SAFE ) || Indestructable(thing)) && mudconf.safe_wipe > 0 ) {
      if ( !(Controls( player, thing ) || could_doit(player,thing,A_LTWINK,0,0)) ) {
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
            if ( !i_owner || (i_owner && (aowner == target)) ) {
               atr_clr(thing, ap->number);
               if ( (ap->flags & AF_LOGGED) || (aflags & AF_LOGGED) ) {
                  STARTLOG(LOG_ALWAYS, "LOG", "ATTR");
                     log_name_and_loc(player);
                     buff2ret = alloc_lbuf("log_attribute");
                     sprintf(buff2ret, " <cause: #%d> Attribute '%s' on #%d cleared",
                             cause, ap->name, thing);
                     log_text(buff2ret);
#ifndef NODEBUGMONITOR
                     sprintf(buff2ret, " Command: %.*s", (LBUF_SIZE - 100), debugmem->last_command);
                     log_text(buff2ret);
#endif
                     free_lbuf(buff2ret);
                  ENDLOG
               }
               got_one = 1;
               mudstate.wipe_state++;
            }
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

/* @rollback <steps> [= <args>]   @rollback/label <label>|<steps> [=<args>]
   @rollback <steps>/<count> [= <args>]   @rollback/label <label>|<steps>/<count> [= <args>]
   @rollback/retry <boolean> [= <args>]   @rollback/retry/label <label>|<boolean> [= <args>]
   @rollback/wait <steps>/<count>[</wait-value>] [= <args>]   @rollback/wait/label <label>|<steps>/<count>[</wait-value>] [= <args>]  
*/
void do_rollback(dbref player, dbref cause, int key, char *in_string,
                 char *argv[], int nargs, char *cargs[], int ncargs)
{
   char *s_buff, *s_buffptr, *s_tmp, *s_eval[10], *s_store[10], *t_string, 
        *cp, *cp2, *s_waitbuff, *s_waitbuffptr, *s_tmp2, *string,
        labelorig[16], labelbak[16];
   int i_rollbackcnt, i_step, i_count, i_loop, i, i_jump, i_rollbackstate, 
       i_waitcnt, i_waitfirst, i_dolabel, i_gotostate;
   double i_wait;
   time_t  i_now;

   if ( !*(mudstate.rollback) ) {
      return;
   }

   string = in_string;
   i_dolabel = 0;
   if ( key & ROLLBACK_LABEL ) {
      if ( (string = strchr(string, '|')) != NULL ) {
         *string = '\0';
         string++;
         i_dolabel = 1;
         i_gotostate = mudstate.gotostate;
         mudstate.gotostate = 1;
         memset(mudstate.gotolabel,'\0',16);
         memset(labelorig, '\0', 16);
         memset(labelbak, '\0', 16);
         strncpy(labelbak, mudstate.gotolabel, 15);
         strncpy(mudstate.gotolabel, in_string, 15);
      } else {
         notify_quiet(player, "@rollback/label expects a label");
         return;
      }
   }

   i_rollbackstate = mudstate.rollbackstate;
   mudstate.rollbackstate = 1;
   i_jump = mudstate.jumpst;
   i_rollbackcnt = mudstate.rollbackcnt;
   t_string = alloc_lbuf("do_rollback_string");
   strcpy(t_string, string);

   s_tmp = exec(player, cause, cause, EV_EVAL | EV_FCHECK, t_string, cargs, ncargs, (char **)NULL, 0);

   if ( key & ROLLBACK_RETRY ) {
      if ( atoi(s_tmp) != 0 ) {
         i_step = i_rollbackcnt;
         i_count = mudconf.rollbackmax;
      } else {
         i_step = 0;
         i_count = 0;
      }
   } else if ( key & ROLLBACK_WAIT ) {
      /* @wait state is handled differently */
      if ( i_dolabel ) {
         mudstate.gotostate = i_gotostate;
         strncpy(mudstate.gotolabel, labelbak, 15);
      }
      i_step = atoi(s_tmp);
      i_count = 1;
      if ( (cp = strchr(s_tmp, '/')) != NULL ) {
         i_count = atoi(cp+1);
      } 

      if ( (i_step <= 0) || (i_count <= 0) ) {
         free_lbuf(t_string);
         mudstate.rollbackstate = i_rollbackstate;
         free_lbuf(s_tmp);
         return; 
      }

      if ( i_count > mudconf.rollbackmax ) {
         i_count = mudconf.rollbackmax;
      }
 
      i_wait = 0;
      if ( cp && (cp2 = strchr(cp+1, '/')) != NULL ) {
         i_wait = safe_atof(cp2+1);
      }

      if ( i_wait < 0 ) {
         free_lbuf(t_string);
         mudstate.rollbackstate = i_rollbackstate;
         free_lbuf(s_tmp);
         return; 
      }
      /* Grab the command stack and populate it here */
      s_buff = alloc_lbuf("do_rollback_wait");
      s_waitbuffptr = s_waitbuff = alloc_lbuf("do_rollback_waitbuff");
      strcpy(s_buff, mudstate.rollback);
      s_buffptr = s_buff;
      i_waitfirst = 0;
      if ( i_dolabel ) {
         safe_str("@goto ", s_waitbuff, &s_waitbuffptr);
         safe_str(in_string, s_waitbuff, &s_waitbuffptr);
         i_waitfirst = 1;
      }

      i_waitcnt = 0;
      while ( s_buffptr ) {
         if ( i_waitcnt < (mudstate.rollbackcnt - i_step - 1)) {
            cp = parse_to(&s_buffptr, ';', 0);
         } else {
            break;
         }
         i_waitcnt++;
      }

      while ( s_buffptr ) {
         if ( i_waitcnt < (mudstate.rollbackcnt - 1) ) {
            if ( i_waitfirst ) {
               safe_chr(';', s_waitbuff, &s_waitbuffptr);
            }
            cp = parse_to(&s_buffptr, ';', 0);
            safe_str(cp, s_waitbuff, &s_waitbuffptr);
            i_waitfirst = 1;
         } else {
            break;
         }
         i_waitcnt++;
      }

      if ( *s_waitbuff ) {
         for (i = 0; i < 10; i++ ) {
            s_store[i] = alloc_lbuf("do_rollback_store");
            s_eval[i] = alloc_lbuf("do_rollback_args");
            *s_eval[i] = '\0';
            if ( (i < ncargs) && cargs[i] ) {
               strcpy(s_eval[i], cargs[i]);
            }
         }
         while ( i_count > 0 ) {
            for (i = 0; i < 10; i++ ) {
               if ( (i < nargs) && (((nargs == 1) && *argv[i]) || (nargs > 1)) ) {
                  strcpy(s_store[i], argv[i]);
                  s_tmp2 = exec(player, cause, cause, EV_EVAL | EV_FCHECK, s_store[i], s_eval, 10, (char **)NULL, 0);
                  strcpy(s_eval[i], s_tmp2);
                  free_lbuf(s_tmp2);
                  if ( mudstate.chkcpu_toggle ) {
                     break;
                  }
               }
            }
            if ( mudstate.chkcpu_toggle ) {
               break;
            }
            wait_que(player, cause, i_wait, NOTHING, s_waitbuff, s_eval, 10, mudstate.global_regs, mudstate.global_regsname);
            i_count--;
            if ( i_count > 0 ) {
               strcpy(t_string, string);
               s_tmp2 = exec(player, cause, cause, EV_EVAL | EV_FCHECK, t_string, s_eval, 10, (char **)NULL, 0);
               if ( (cp = strchr(s_tmp2, '/')) != NULL ) {
                  cp2 = strchr(cp+1, '/');
               }
               if ( cp2 ) {
                  i_wait = safe_atof(cp2+1);
               }
               free_lbuf(s_tmp2);
               if ( i_wait < 0 ) {
                  break;
               }
            }
         }
         for (i = 0; i < 10; i++ ) {
            free_lbuf(s_eval[i]);
            free_lbuf(s_store[i]);
         }
      }
      free_lbuf(s_waitbuff);
      free_lbuf(s_buff);
      free_lbuf(s_tmp);
      free_lbuf(t_string);
      mudstate.rollbackstate = i_rollbackstate;
      return; /* Wait is special condition */
   } else {
      i_step = atoi(s_tmp);
      i_count = 1;
      if ( strchr(s_tmp, '/') != NULL ) {
         i_count = atoi(strchr(s_tmp, '/')+1);
      } 
  
      if ( (i_step <= 0) || (i_count <= 0) ) {
         free_lbuf(t_string);
         mudstate.rollbackstate = i_rollbackstate;
         free_lbuf(s_tmp);
         return; 
      }
      if ( i_count > mudconf.rollbackmax ) {
         i_count = mudconf.rollbackmax;
      }
   }
   free_lbuf(s_tmp);

   i_now = mudstate.now;
   for (i = 0; i < 10; i++ ) {
      s_store[i] = alloc_lbuf("do_rollback_store");
      if ( (i < nargs) && (((nargs == 1) && *argv[i]) || (nargs > 1)) ) {
         strcpy(s_store[i], argv[i]);
         s_eval[i] = exec(player, cause, cause, EV_EVAL | EV_FCHECK, s_store[i], cargs, ncargs, (char **)NULL, 0);
      } else {
         s_eval[i] = alloc_lbuf("do_rollback_args");
         *s_eval[i] = '\0';
         if ( (i < ncargs) && cargs[i] ) {
            strcpy(s_eval[i], cargs[i]);
         }
      }
   }

   s_buff = alloc_lbuf("do_rollback");

   while ( !mudstate.chkcpu_toggle && (i_count > 0) ) {
      strcpy(s_buff, mudstate.rollback);
      s_buffptr = s_buff;
      i_loop = 0;
      mudstate.rollbackcnt = i_rollbackcnt - i_step - 1;
      if ( mudstate.rollbackcnt < 0 )
         mudstate.rollbackcnt = 0; 
      mudstate.jumpst = 0;
      mudstate.gotostate = 0;
      if ( i_dolabel ) {
         mudstate.gotostate = 1;
      }
      while (s_buffptr && !mudstate.chkcpu_toggle) {
         i_loop++;
         cp = parse_to(&s_buffptr, ';', 0);
         if ( !cp )
            break;
         if ( i_loop < (i_rollbackcnt - i_step) ) 
            continue; 
         if ( i_loop >= i_rollbackcnt )
            break;
         if (cp && *cp) {
            process_command(player, cause, 0, cp, s_eval, 10, InProgram(player), mudstate.no_hook);
         }
         if ( time(NULL) > (i_now + ((mudconf.cputimechk > 5) ? 5 : mudconf.cputimechk)) ) {
            notify(player, "@rollback:  Aborted for high utilization.");
            mudstate.breakst=1;
            mudstate.chkcpu_toggle =1;
            break;
         }
      }
      mudstate.jumpst = i_jump;
      if ( mudstate.chkcpu_toggle )
         break;
      i_count--;

      if ( i_count > 0 ) {
         if ( i_dolabel) {
            strncpy(mudstate.gotolabel, in_string, 15);
            mudstate.gotostate = 1;
         }
         if ( (i_step <= 0) ) {
            free_lbuf(t_string);
            free_lbuf(s_buff);
            mudstate.rollbackcnt = i_rollbackcnt;
            mudstate.rollbackstate = i_rollbackstate;
            mudstate.jumpst = i_jump;
            for (i = 0; i < 10; i++ ) {
               free_lbuf(s_eval[i]);
               free_lbuf(s_store[i]);
            }
            if ( i_dolabel ) {
               mudstate.gotostate = i_gotostate;
               strncpy(mudstate.gotolabel, labelbak, 15);
            }
            return; 
         }

         if ( key & ROLLBACK_RETRY ) {
            strcpy(t_string, string);
            s_tmp = exec(player, cause, cause, EV_EVAL | EV_FCHECK, t_string, s_eval, 10, (char **)NULL, 0);
            if ( atoi(s_tmp) == 0 ) {
               i_step = 0;
               i_count = 0;
               free_lbuf(s_tmp);
               break;
            }
            free_lbuf(s_tmp);
         }
         if ( mudstate.chkcpu_toggle )
            break;
         for (i = 0; i < 10; i++ ) {
            if ( (i < nargs) && (((nargs == 1) && *argv[i]) || (nargs > 1)) ) {
               strcpy(s_store[i], argv[i]);
               s_tmp = exec(player, cause, cause, EV_EVAL | EV_FCHECK, s_store[i], s_eval, 10, (char **)NULL, 0);
               strcpy(s_eval[i], s_tmp);
               free_lbuf(s_tmp);
               if ( mudstate.chkcpu_toggle )
                  break;
            }
         }
         if ( mudstate.chkcpu_toggle )
            break;
      }

   }
   free_lbuf(s_buff);
   free_lbuf(t_string);
   for (i = 0; i < 10; i++ ) {
      free_lbuf(s_eval[i]);
      free_lbuf(s_store[i]);
   }
   if ( i_dolabel ) {
      mudstate.gotostate = i_gotostate;
      strncpy(mudstate.gotolabel, labelbak, 15);
   }
   mudstate.rollbackcnt = i_rollbackcnt;
   mudstate.jumpst = i_jump;
   mudstate.rollbackstate = i_rollbackstate;

}

void do_include(dbref player, dbref cause, int key, char *string,
                char *argv[], int nargs, char *cargs[], int ncargs)
{
   dbref thing, owner, target;
   int attrib, flags, i, x, i_savebreak, i_rollback, i_jump;
   time_t  i_now;
   char *buff1, *buff1ptr, *cp, *pt, *s_buff[10], *savereg[MAX_GLOBAL_REGS],
        *npt, *saveregname[MAX_GLOBAL_REGS], *s_rollback;

   if ( desc_in_use != NULL ) {
      notify_quiet(player, "You can not use @include at command line.");
      return;
   }
   if ( mudstate.includenest >= mudconf.includenest ) {
      notify_quiet(player, "Exceeded @include nest limit.");
      return;
   }
   if ( mudstate.includecnt >= mudconf.includecnt ) {
      notify_quiet(player, "Exceeded total number of @includes allowed.");
      return;
   }
   if (!parse_attrib(player, string, &thing, &attrib) || (attrib == NOTHING) || (thing == NOTHING)) {
      notify_quiet(player, "No match.");
      return;
   }
   if (!Good_chk(thing) || (!controls(player, thing) &&
       !could_doit(player,thing,A_LTWINK,0,0)) ) {
       notify_quiet(player, "Permission denied.");
       return;
   }
   target = player;
   if ( (key & INCLUDE_TARGET) && controls(player, thing) ) {
      target = thing;
   }
   mudstate.includecnt++;
   mudstate.includenest++;
   buff1ptr = buff1 = atr_pget(thing, attrib, &owner, &flags);
   if ( key & INCLUDE_COMMAND ) {
      if ( *buff1 && (((*buff1 == '$') || (*buff1 == '^')) && (strchr(buff1, ':') != NULL)) ) {
         buff1ptr = strchr(buff1, ':') + 1;
      }
   }
   if ( !*buff1 || !*buff1ptr ) {
      free_lbuf(buff1);
      return;
   }
   for (i = 0; i < 10; i++) {
      s_buff[i] = alloc_lbuf("do_include_buffers");
      if ( key & INCLUDE_OVERRIDE ) {
/*       if ( nargs > 0 ) { */
         if ( !mudstate.argtwo_fix ) {
            if ( (i < nargs) && (((nargs > 1) || ((nargs <= 1) && argv[i] && *argv[i]))) ) {
               if ( argv[i] && *argv[i] )
                  sprintf(s_buff[i], "%s", argv[i]);      
            }
         } else {
            if ( (i < ncargs) && cargs[i] && *cargs[i] )
               sprintf(s_buff[i], "%s", cargs[i]);
         }
      } else {
         if ( (i < ncargs) && cargs[i] && *cargs[i] )
            sprintf(s_buff[i], "%s", cargs[i]);
         if ( (i < nargs) && (((nargs > 1) || ((nargs <= 1) && argv[i] && *argv[i]))) ) {
            if ( !argv[i] || !*argv[i] ) {
               memset(s_buff[i], '\0', LBUF_SIZE);
            } else {
               sprintf(s_buff[i], "%s", argv[i]);      
            }
         }
      }
   }
   if ( (key & INCLUDE_LOCAL) || (key & INCLUDE_CLEAR) ) {
      for (x = 0; x < MAX_GLOBAL_REGS; x++) {
         savereg[x] = alloc_lbuf("ulocal_reg");
         saveregname[x] = alloc_sbuf("ulocal_regname");
         pt = savereg[x];
         npt = saveregname[x];
         safe_str(mudstate.global_regs[x],savereg[x],&pt);
         safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
         if ( key & INCLUDE_CLEAR ) {
            *mudstate.global_regs[x] = '\0';
            *mudstate.global_regsname[x] = '\0';
         }
      }
   }
   i_savebreak = mudstate.breakst;
   i_now = mudstate.now;
   s_rollback = alloc_lbuf("s_rollback_include");
   strcpy(s_rollback, mudstate.rollback);
   i_jump = mudstate.jumpst;
   i_rollback = mudstate.rollbackcnt;
   mudstate.rollbackcnt = mudstate.jumpst = 0;
   if ( buff1ptr ) {
      strcpy(mudstate.rollback, buff1ptr);
   }
   while (buff1ptr && !mudstate.breakst) {
      cp = parse_to(&buff1ptr, ';', 0);
      if (cp && *cp) {
         process_command(target, cause, 0, cp, s_buff, 10, InProgram(thing), mudstate.no_hook);
         if ( key & INCLUDE_NOBREAK )
            mudstate.breakst = i_savebreak;
      }
      if ( time(NULL) > (i_now + 5) ) {
         notify(player, "@include:  Aborted for high utilization.");
         mudstate.breakst=1;
         mudstate.chkcpu_toggle =1;
         break;
      }
   }
   mudstate.jumpst = i_jump;
   mudstate.rollbackcnt = i_rollback;
   strcpy(mudstate.rollback, s_rollback);
   free_lbuf(s_rollback);
   if ( (key & INCLUDE_LOCAL) || (key & INCLUDE_CLEAR) ) {
      for (x = 0; x < MAX_GLOBAL_REGS; x++) {
         pt = mudstate.global_regs[x];
         npt = mudstate.global_regsname[x];
         safe_str(savereg[x],mudstate.global_regs[x],&pt);
         safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
         free_lbuf(savereg[x]);
         free_sbuf(saveregname[x]);
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
         !could_doit(it,thing,A_LTWINK,0,0)) {
         notify_quiet(it, "Permission denied.");
         return;
     }
  } else {
     if (!parse_attrib(player, object, &thing, &attrib) || (attrib == NOTHING) || (thing == NOTHING)) {
        notify_quiet(player, "No match.");
        return;
     }
     if (!controls(player, thing) &&
         !could_doit(player,thing,A_LTWINK,0,0)) {
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

	if (!could_doit(player, thing, A_LUSE,1,1)) {
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
int	anum, i_tree;

	arg1++;					/* skip the '&' */
        i_tree = 0;
        if ( *arg1 == '`' ) {
           i_tree = SET_TREE;
           arg1++;
        }
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
	do_setattr(player, cause, anum, (key | i_tree), s, arg2);
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
