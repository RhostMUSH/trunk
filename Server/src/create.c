/* create.c -- Commands that create new objects */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "match.h"
#include "command.h"
#include "alloc.h"
#include "attrs.h"
/* MMMail Addition for hardcoded mailer. */
#include "mail.h"

/* ---------------------------------------------------------------------------
 * parse_linkable_room: Get a location to link to.
 */

static dbref 
parse_linkable_room(dbref player, char *room_name)
{
    dbref room;

    init_match(player, room_name, NOTYPE);
    match_everything(MAT_NO_EXITS | MAT_NUMERIC | MAT_HOME);
    room = match_result();

    /* HOME is always linkable */

    if (room == HOME)
	return HOME;

    /* Make sure we can link to it */

    if (!Good_obj(room)) {
	notify_quiet(player, "That's not a valid object.");
	return NOTHING;
    } else if (!Has_contents(room) || !Linkable(player, room)) {
	notify_quiet(player, "You can't link to that.");
	return NOTHING;
    } else {
	return room;
    }
}

/* ---------------------------------------------------------------------------
 * open_exit, do_open: Open a new exit and optionally link it somewhere.
 */

static void 
open_exit(dbref player, dbref loc, char *direction, char *linkto, int key)
{
    dbref exit;
    char *tpr_buff, *tprp_buff;

    if (!Good_obj(loc))
	return;

    if (!direction || !*direction) {
	notify_quiet(player, "Open where?");
	return;
    } else if (!controls(player, loc) && !could_doit(player, loc, A_LOPEN, 0, 0)) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    exit = create_obj(player, TYPE_EXIT, direction, 0);
    if (exit == NOTHING)
	return;

    /* Initialize everything and link it in. */

    s_Exits(exit, loc);
    s_Next(exit, Exits(loc));
    s_Exits(loc, exit);

    /* and we're done */

    if ( !(key & SIDEEFFECT ) )
       notify_quiet(player, "Opened.");

    /* See if we should do a link */

    if (!linkto || !*linkto)
	return;

    loc = parse_linkable_room(player, linkto);
    if (loc != NOTHING) {

	/* Make sure the player passes the link lock */

	if ((loc != HOME) && !could_doit(player, loc, A_LLINK, 1, 0)) {
	    notify_quiet(player, "You can't link to there.");
	    return;
	}
	/* Link it if the player can pay for it */

	if (!payfor(player, mudconf.linkcost)) {
            tprp_buff = tpr_buff = alloc_lbuf("open_exit");
	    notify_quiet(player,
			 safe_tprintf(tpr_buff, &tprp_buff, "You don't have enough %s to link.",
				 mudconf.many_coins));
            free_lbuf(tpr_buff);
	} else {
	    s_Location(exit, loc);
	    if ( !(key & SIDEEFFECT ) )
	       notify_quiet(player, "Linked.");
	}
    }
}

char *
get_free_num(dbref player, char *buff)
{
    char *pt, *retpt;
    dbref ftemp;

    pt = buff + 1;
    retpt = buff;
    while (isdigit((int)*pt))
	pt++;
    if (*pt != '\0') {
	if ((pt == (buff + 1)) || (!isspace((int)*pt)))
	    notify(player, "No or bad dbref given.");
	else {
	    *pt = '\0';
	    do {
		pt++;
	    } while (isspace((int)*pt));
	    ftemp = atoi(buff + 1);
	    if (Good_obj(ftemp)) {
		mudstate.free_num = ftemp;
		retpt = pt;
	    } else {
		notify(player, "Bad dbref given.");
	    }
	}
    }
    return retpt;
}

void 
do_open(dbref player, dbref cause, int key, char *direction,
	char *links[], int nlinks)
{
    dbref loc, destnum;
    char *dest, *dir2;
    char spam_chr[32];

    memset(spam_chr, 0, sizeof(spam_chr));
    if ( (key & SIDEEFFECT) && !SideFX(player) ) {
       notify(player, "#-1 FUNCTION DISABLED");
       return;
    }
    if (Immortal(player) && (*direction == '#')) {
	dir2 = get_free_num(player, direction);
    } else {
	dir2 = direction;
    }

    /* Create the exit and link to the destination, if there is one */

    if (nlinks >= 1)
	dest = links[0];
    else
	dest = NULL;

    if ((key == OPEN_INVENTORY) || ((Typeof(player) == TYPE_ROOM) && (mudconf.rooms_can_open == 1)))
	loc = player;
    else {
        if ( mudstate.store_loc != NOTHING )
            loc = mudstate.store_loc;
        else
           loc = Location(player);
    }

    open_exit(player, loc, dir2, dest, key);

    /* Open the back link if we can */

    if (nlinks >= 2) {
	destnum = parse_linkable_room(player, dest);
	if (destnum != NOTHING) {
/* For some reason unsafe_tprintf() doesn't work right here */
            sprintf(spam_chr, "%d", loc);
            open_exit(player, destnum, links[1], spam_chr, key);
	}
    }
}

/* ---------------------------------------------------------------------------
 * link_exit, do_link: Set destination(exits), dropto(rooms) or
 * home(player,thing)
 */

static void 
link_exit(dbref player, dbref exit, dbref dest, int key)
{
    int cost, quot;

    /* Make sure we can link there */

    if ((dest != HOME) &&
	((!controls(player, dest) && !Link_ok(dest)) ||
	 !could_doit(player, dest, A_LLINK, 1, 0))) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    /* Exit must be unlinked or controlled by you */

    if ((Location(exit) != NOTHING) && !controls(player, exit)) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    /* If exit is unlinked and mudconf 'paranoid_exit_linking' is enabled
     * Do not allow the exit to be linked if you do not control it
     */
    if ( (mudconf.paranoid_exit_linking == 1) && !controls(player,exit) ) {
        notify_quiet(player, "Permission denied.");
	return;
    }

    /* handle costs */

    cost = mudconf.linkcost;
    quot = 0;
    if (Owner(exit) != Owner(player)) {
	cost += mudconf.opencost;
	quot += mudconf.exit_quota;
    }
    if (!canpayfees(player, player, cost, quot, TYPE_EXIT))
	return;

    /* Pay the owner for his loss */

    /* Ok, if paranoid_exit_linking is enabled the linker does NOT own exit */
    if (!(mudconf.paranoid_exit_linking)) {
       if (Owner(exit) != Owner(player)) {
	   giveto(Owner(exit), mudconf.opencost, NOTHING);
	   add_quota(Owner(exit), quot, TYPE_EXIT);
	   s_Owner(exit, Owner(player));
	   s_Flags(exit, (Flags(exit) & ~(INHERIT | WIZARD)) | HALT);
       }
    }
    /* link has been validated and paid for, do it and tell the player */

    s_Location(exit, dest);
    if (!(Quiet(player) || (key & SIDEEFFECT)) )
	notify_quiet(player, "Linked.");
}

void 
do_link(dbref player, dbref cause, int key, char *what, char *where)
{
    dbref thing, room;
    char *buff;
    int nomtest;

    if ( (key & SIDEEFFECT) && !SideFX(player) ) {
       notify(player, "#-1 FUNCTION DISABLED");
       return;
    }

    /* Find the thing to link */

    init_match(player, what, TYPE_EXIT);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
	return;

    nomtest = ((NoMod(thing) && !WizMod(player)) || (DePriv(player,Owner(thing),DP_MODIFY,POWER7,NOTHING) && (Owner(thing) != Owner(player))) || (Backstage(player) && NoBackstage(thing) && !Immortal(player)));
    /* Allow unlink if where is not specified */

    if (!where || !*where) {
      if (!nomtest)
	do_unlink(player, cause, key, what);
      else
	notify(player,"Permission denied.");
      return;
    }
    switch (Typeof(thing)) {
    case TYPE_EXIT:

	/* Set destination */

	room = parse_linkable_room(player, where);
	if (room != NOTHING) {
	  if (!nomtest)
	    link_exit(player, thing, room, key);
	  else
	    notify(player,"Permission denied.");
	}
	break;
    case TYPE_PLAYER:
    case TYPE_THING:

	/* Set home */

	if (!Controls(player, thing) || nomtest) {
	    notify_quiet(player, "Permission denied.");
	    break;
	}
	init_match(player, where, NOTYPE);
	match_everything(MAT_NO_EXITS);
	room = noisy_match_result();
	if (!Good_obj(room))
	    break;
	if (!Has_contents(room)) {
	    notify_quiet(player, "Can't link to an exit.");
	    break;
	}
	if (!can_set_home(player, thing, room) ||
	    !could_doit(player, room, A_LLINK, 1, 0)) {
	    notify_quiet(player, "Permission denied.");
	} else if (room == HOME) {
	    notify_quiet(player, "Can't set home to home.");
	} else {
	    s_Home(thing, room);
	    if (!(Quiet(player) || (key & SIDEEFFECT)) )
		notify_quiet(player, "Home set.");
	}
	break;
    case TYPE_ROOM:

	/* Set dropto */

	if (!Controls(player, thing) || nomtest) {
	    notify_quiet(player, "Permission denied.");
	    break;
	}
	room = parse_linkable_room(player, where);
	if (!Good_obj(room) && (room != HOME)) {
	    notify_quiet(player, "Permission denied.");
	    break;
	}

	if ((room != HOME) && !isRoom(room)) {
	    notify_quiet(player, "That is not a room!");
	} else if ((room != HOME) &&
		   ((!controls(player, room) && !Link_ok(room)) ||
		    !could_doit(player, room, A_LLINK, 1, 0))) {
	    notify_quiet(player, "Permission denied.");
	} else {
	    s_Dropto(thing, room);
	    if (!Quiet(player))
		notify_quiet(player, "Dropto set.");
	}
	break;
    default:
	STARTLOG(LOG_BUGS, "BUG", "OTYPE")
	    buff = alloc_mbuf("do_link.LOG.badtype");
	sprintf(buff, "Strange object type: object #%d = %d",
		thing, Typeof(thing));
	log_text(buff);
	free_mbuf(buff);
	ENDLOG
    }
}

/* ---------------------------------------------------------------------------
 * do_zone: Add a zonemaster to an object's zone list
 */

void
do_zone(dbref player, dbref cause, int key, char *tname, char *pname)
{
  dbref thing, zonemaster;

  switch( key ) {
    case ZONE_ADD: /* or default */
      if( *tname && !*pname ) {
        init_match(player, tname, NOTYPE);
        match_everything(0);
        thing = noisy_match_result();
        if( thing == NOTHING )
          return;
        if( !Examinable(player, thing) ) {
          notify_quiet(player, "You can't do that.");
          return;
        }
        viewzonelist(player, thing);
        return;
      }
       
      if( !*tname || !*pname ) {
        notify_quiet(player, "This switch expects two arguments.");
        return;
      }
     
      init_match(player, tname, NOTYPE);
      match_everything(0);
      thing = noisy_match_result();
      if( thing == NOTHING )
        return;

      /* Make sure we can do it */

      if ( (NoMod(thing) && !WizMod(player)) ||
           (Backstage(player) && NoBackstage(thing)) ) {
        notify_quiet(player, "Permission denied.");
        return;
      }
      if (!Controls(player, thing)) {
        notify_quiet(player, "Permission denied.");
        return;
      }

      if( ZoneMaster(thing) ) {
        notify_quiet(player, "You can't zone a Zone Master.");
        return;
      }
      /* Find out what the new zone is */

      init_match(player, pname, NOTYPE);
      match_everything(0);
      zonemaster = noisy_match_result();
      if (zonemaster == NOTHING)
        return;
 
      if( !ZoneMaster(zonemaster) ) {
        notify_quiet(player, "That's not a Zone Master.");
        return;
      }

      if(!Controls(player, zonemaster) && 
        !could_doit(player, zonemaster, A_LZONETO, 0, 0) &&
        !could_doit(player, zonemaster, A_LZONEWIZ, 0, 0)) {
        notify_quiet(player, "Permission denied.");
        return;
      }

      if( zlist_inlist(thing, zonemaster) ) {
        notify_quiet(player, "Object is already in that zone.");
        return;
      }

      zlist_add(thing, zonemaster);
      zlist_add(zonemaster, thing);

      notify_quiet(player, "Zone Master added to object.");
      break;
    case ZONE_DELETE:
      if( !*tname || !*pname ) {
        notify_quiet(player, "This switch expects two arguments.");
        return;
      }
      /* Find out what the zone is */

      init_match(player, pname, NOTYPE);
      match_everything(0);
      zonemaster = noisy_match_result();
      if (zonemaster == NOTHING)
        return;
     
      init_match(player, tname, NOTYPE);
      match_everything(0);
      thing = noisy_match_result();
      if( thing == NOTHING )
        return;

      if(!zlist_inlist(thing, zonemaster)) {
        notify_quiet(player, "That is not one of this object's Zone Masters.");
        return;
      }

      /* only need to control zmo or be zonewiz to delete 
         or control object */

      if(!Controls(player, thing) &&
         !Controls(player, zonemaster) && 
         !could_doit(player, zonemaster, A_LZONEWIZ, 0, 0)
         ) {
        notify_quiet(player, "Permission denied.");
        return;
      }

      if ( (NoMod(thing) && !WizMod(player)) ||
           (Backstage(player) && NoBackstage(thing)) ) {
        notify_quiet(player, "Permission denied.");
        return;
      }

      zlist_del(thing, zonemaster);
      zlist_del(zonemaster, thing);

      notify_quiet(player, "Deleted.");
      break;
    case ZONE_PURGE:
      if( !*tname || *pname) {
        notify_quiet(player, "This switch expects one argument.");
        return;
      }
      /* Find out what the zone is */

      init_match(player, tname, NOTYPE);
      match_everything(0);
      thing = noisy_match_result();
      if (thing == NOTHING)
        return;

      if( ZoneMaster(thing) ) {
        if(!Controls(player, thing) && 
          !could_doit(player, thing, A_LZONEWIZ, 0, 0)) {
          notify_quiet(player, "Permission denied.");
          return;
        }
        if ( (NoMod(thing) && !WizMod(player)) ||
             (Backstage(player) && NoBackstage(thing)) ) {
          notify_quiet(player, "Permission denied.");
          return;
        }
        zlist_destroy(thing);
        notify_quiet(player, "All objects removed from zone.");
      }
      else {
        if(!Controls(player, thing)) {
          notify_quiet(player, "Permission denied.");
          return;
        }
        if ( (NoMod(thing) && !WizMod(player)) ||
             (Backstage(player) && NoBackstage(thing)) ) {
          notify_quiet(player, "Permission denied.");
          return;
        }
        zlist_destroy(thing);

        notify_quiet(player, "Object removed from all zones.");
      }
      break;
    default:
      notify_quiet(player, "Unknown switch!");
      break;
  }
  return;
}

     
  

     


/* ---------------------------------------------------------------------------
 * do_parent: Set an object's parent field.
 */

void 
do_parent(dbref player, dbref cause, int key, char *tname, char *pname)
{
    dbref thing, parent, curr;
    int lev;

    /* get victim */

    init_match(player, tname, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
	return;

    /* Make sure we can do it */

    if ( NoMod(thing) && !WizMod(player) ) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    if (!Controls(player, thing) &&
        !could_doit(player,thing,A_LTWINK, 0, 0)) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    if ( Backstage(player) && NoBackstage(thing) ) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    /* Find out what the new parent is */

    if (*pname) {
	init_match(player, pname, Typeof(thing));
	match_everything(0);
	parent = noisy_match_result();
	if (parent == NOTHING)
	    return;

	/* Make sure we have rights to set parent */

	if (!Parentable(player, parent)) {
	    notify_quiet(player, "Permission denied.");
	    return;
	}
	/* Verify no recursive reference */

	ITER_PARENTS(parent, curr, lev) {
	    if (curr == thing) {
		notify_quiet(player,
			     "You can't have yourself as a parent!");
		return;
	    }
	}
    } else {
	parent = NOTHING;
    }

    s_Parent(thing, parent);
    if ( key & SIDEEFFECT )
       mudstate.store_lastcr = parent;
    if (!Quiet(thing) && !Quiet(player) && !(key & SIDEEFFECT)) {
	if (parent == NOTHING)
	    notify_quiet(player, "Parent cleared.");
	else
	    notify_quiet(player, "Parent set.");
    }
}

/* ---------------------------------------------------------------------------
 * do_dig: Create a new room.
 */

void 
do_dig(dbref player, dbref cause, int key, char *name,
       char *args[], int nargs)
{
    dbref room, location;
    char *buff, *name2;
    int old_lastobj;

    if ( (key & SIDEEFFECT) && !SideFX(player) ) {
       notify(player, "#-1 FUNCTION DISABLED");
       return;
    }
    if ( mudstate.store_loc != NOTHING )
       location = mudstate.store_loc;
    else
       location = Location(player);
    /* we don't need to know player's location!  hooray! */

    if (!name || !*name) {
	notify_quiet(player, "Dig what?");
	return;
    }
    if (Immortal(player) && (*name == '#')) {
	name2 = get_free_num(player, name);
    } else {
	name2 = name;
    }

    room = create_obj(player, TYPE_ROOM, name2, 0);
    if (room == NOTHING)
	return;

    old_lastobj = mudstate.store_lastcr;
    if ( !(key & SIDEEFFECT) ) 
       notify(player,
	   unsafe_tprintf("%s created with room number %d.", name2, room));

    buff = alloc_sbuf("do_dig");
    if ((nargs >= 1) && args[0] && *args[0]) {
	sprintf(buff, "%d", room);
	open_exit(player, location, args[0], buff, key);
        mudstate.store_lastx1 = mudstate.store_lastcr;
    }
    if ((nargs >= 2) && args[1] && *args[1]) {
	sprintf(buff, "%d", location);
	open_exit(player, room, args[1], buff, key);
        mudstate.store_lastx2 = mudstate.store_lastcr;
    }
    free_sbuf(buff);
    if (key == DIG_TELEPORT ) {
       if ( No_tel(player) || (mudstate.remotep == player) ) {
          notify_quiet(player, "You can't teleport to your @dug room.");
       } else {
	(void) move_via_teleport(player, room, cause, 0, 0);
       }
    }
    mudstate.store_lastcr = old_lastobj;
}

/* ---------------------------------------------------------------------------
 * do_create: Make a new object.
 */

void 
do_create(dbref player, dbref cause, int key, char *name, char *coststr)
{
    dbref thing;
    int cost;
    char *name2;

    if ( (key & SIDEEFFECT) && !SideFX(player) ) {
       notify(player, "#-1 FUNCTION DISABLED");
       return;
    }
    cost = atoi(coststr);
    if (!name || !*name) {
	notify_quiet(player, "Create what?");
	return;
    } else if (cost < 0) {
	notify_quiet(player,
		     "You can't create an object for less than nothing!");
	return;
    }
    if (Immortal(player) && (*name == '#')) {
	name2 = get_free_num(player, name);
    } else {
	name2 = name;
    }

    thing = create_obj(player, TYPE_THING, strip_all_special(name2), cost);
    if (thing == NOTHING)
	return;

    move_via_generic(thing, player, NOTHING, 0);
    s_Home(thing, new_home(player));
    if (!(Quiet(player) || (key & SIDEEFFECT))) {
	notify(player,
	       unsafe_tprintf("%s created as object #%d",
		       Name(thing), thing));
    }
}

/* ---------------------------------------------------------------------------
 * do_clone: Create a copy of an object.
 */

void 
do_clone(dbref player, dbref cause, int key, char *name, char *arg2)
{
    dbref clone, thing, new_owner, loc;
    FLAG rmv_flags;
    int cost, side_effect;
    char *tpr_buff, *tprp_buff;

    if ((key & CLONE_INVENTORY) || !Has_location(player))
	loc = player;
    else
	loc = Location(player);

    if (!Good_obj(loc))
	return;

    side_effect = 0;
    if ( key & SIDEEFFECT ) {
       side_effect = 1;
       key ^= SIDEEFFECT;
    }
    init_match(player, name, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();
    if ((thing == NOTHING) || (thing == AMBIGUOUS))
	return;

    /* Let players clone things set VISUAL.  It's easier than retyping in
     * all that data
     */

    if (!Examinable(player, thing)) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    if (isPlayer(thing)) {
	notify_quiet(player, "You cannot clone players!");
	return;
    }
    if (ZoneMaster(thing)) {
        notify_quiet(player, "You can't clone a Zone Master!");
        return;
    }
    /* You can only make a parent link to what you control */

    if (!Controls(player, thing) && !Parent_ok(thing) &&
	(key & CLONE_PARENT)) {
        tprp_buff = tpr_buff = alloc_lbuf("do_clone");
	notify_quiet(player,
		     safe_tprintf(tpr_buff, &tprp_buff, "You don't control %s, ignoring /parent.",
			     Name(thing)));
        free_lbuf(tpr_buff);
	key &= ~CLONE_PARENT;
    }
    /* Determine the cost of cloning */

    new_owner = (key & CLONE_PRESERVE) ? Owner(thing) : Owner(player);
    if (key & CLONE_SET_COST) {
	cost = atoi(arg2);
	arg2 = NULL;
    } else {
	cost = 1;
	switch (Typeof(thing)) {
	case TYPE_THING:
	    cost = OBJECT_DEPOSIT((mudconf.clone_copy_cost) ?
				  Pennies(thing) : 1);
	    break;
	case TYPE_ROOM:
	    cost = mudconf.digcost;
	    break;
	case TYPE_EXIT:
	    if (!Controls(player, loc)) {
		notify_quiet(player, "Permission denied.");
		return;
	    }
	    cost = mudconf.digcost;
	    break;
	}
    }

    /* Go make the clone object */

    clone = create_obj(new_owner, Typeof(thing), Name(thing), cost);
    if (clone == NOTHING)
	return;

    /* Wipe out any old attributes and copy in the new data */

    cost = Pennies(clone);
    al_destroy(clone);
    if (key & CLONE_PARENT)
	s_Parent(clone, thing);
    else
	atr_cpy(player, clone, thing);
    s_Pennies(clone,cost);
    if (arg2 && *arg2) {
	if (ok_name(arg2))
	    s_Name(clone, arg2);
	else {
	    s_Name(clone, Name(thing));
	    notify(player, "Bad name for given for clone object.");
	    *arg2 = '\0';
	}
    } else
	s_Name(clone, Name(thing));

    /* Clear out problem flags from the original */

    rmv_flags = WIZARD;
    if (!(key & CLONE_INHERIT) || (!Inherits(player)))
	rmv_flags |= INHERIT | IMMORTAL;
    s_Flags(clone, Flags(thing) & ~rmv_flags);

    /* Tell creator about it */

    if (!(Quiet(player) || side_effect) ) {
        tprp_buff = tpr_buff = alloc_lbuf("do_clone");
	if (arg2 && *arg2)
	    notify(player,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s cloned as %s, new copy is object #%d.",
			   Name(thing), arg2, clone));
	else
	    notify(player,
		   safe_tprintf(tpr_buff, &tprp_buff, "%s cloned, new copy is object #%d.",
			   Name(thing), clone));
        free_lbuf(tpr_buff);
    }
    /* Put the new thing in its new home.  Break any dropto or link, then
     * try to re-establish it.
     */

    switch (Typeof(thing)) {
    case TYPE_THING:
	s_Home(clone, clone_home(player, thing));
	move_via_generic(clone, loc, player, 0);
	break;
    case TYPE_ROOM:
	s_Dropto(clone, NOTHING);
	if (Dropto(thing) != NOTHING)
	    link_exit(player, clone, Dropto(thing), key);
	break;
    case TYPE_EXIT:
	s_Exits(loc, insert_first(Exits(loc), clone));
	s_Exits(clone, loc);
	s_Location(clone, NOTHING);
	if (Location(thing) != NOTHING)
	    link_exit(player, clone, Location(thing), key);
	break;
    }

    /* If same owner run ACLONE, else halt it.  Also copy parent
     * if we can
     */

    if (new_owner == Owner(thing)) {
	if (!(key & CLONE_PARENT))
	    s_Parent(clone, Parent(thing));
	did_it(player, clone, 0, NULL, 0, NULL, A_ACLONE,
	       (char **) NULL, 0);
    } else {
	if (!(key & CLONE_PARENT) &&
	    (Controls(player, thing) || Parent_ok(thing)))
	    s_Parent(clone, Parent(thing));
	s_Halted(clone);
    }
}

/* ---------------------------------------------------------------------------
 * do_pcreate: Create new players and robots.
 */

void 
do_pcreate(dbref player, dbref cause, int key, char *name, char *pass)
{
    int isrobot;
    dbref newplayer, dest, goodplayer;
    char *name2;
    time_t now, dtime;
    DESC *d, *e;

    if ( (key & PCREATE_REG) && !Wizard(player) ) {
	notify(player, "Permission denied.");
	return;
    }
    if ((!Wizard(player) && !Admin(player) &&
	 !HasPriv(player, NOTHING, POWER_PCREATE, POWER4, POWER_LEVEL_NA)) ||
	DePriv(player, NOTHING, DP_PCREATE, POWER7, POWER_LEVEL_NA)) {
	notify(player, "Permission denied.");
	return;
    }
    if (Immortal(player) && (*name == '#')) {
	name2 = get_free_num(player, name);
    } else {
	name2 = name;
    }
    if ( key & PCREATE_REG ) {
       time(&now);
       dtime = 0;
       e = NULL;
       goodplayer = NOTHING;
       if ( Good_obj(player) ) {
          if ( isPlayer(player) )
             goodplayer = player;
          else
             goodplayer = Owner(player);
       }
       if ( !Good_obj(goodplayer) || !(Connected(goodplayer) && isPlayer(goodplayer)) ) {
          if ( Good_obj(cause) && isPlayer(cause) )
             goodplayer = cause;
          else
             goodplayer = Owner(cause);
       }

       if ( !Good_obj(goodplayer) || !isPlayer(goodplayer) ) {
	    STARTLOG(LOG_PCREATES, "CRE", "AREG")
               log_text((char *) "Invalid player specified for @pcreate/reg of #");
               log_number(player);
            ENDLOG
            return;
       }
       DESC_ITER_PLAYER(goodplayer, d) {
          if (!dtime || (now - d->last_time) < dtime) {
             e = d;
             dtime = now - d->last_time;
          }
       }
       if ( !e ) {
          STARTLOG(LOG_PCREATES, "CRE", "AREG")
             log_text((char *) "Connection non-existing for player ");
             log_name(goodplayer);
             log_text((char *) ".  Issued by ");
             log_name(cause);
          ENDLOG
          notify(player, "Unable to create player.  Unable to validate enactor site information.");
          return;
       }
       switch (reg_internal(name2, pass, (char *)e, 1)) {
          case 0:
            newplayer = lookup_player(player, name2, 0);
            notify(player,unsafe_tprintf("Player '%s [#%d]' autoregistered to email '%s'.", name2, newplayer, pass));
	    STARTLOG(LOG_PCREATES, "CRE", "AREG")
	        log_name(newplayer);
	    log_text((char *) " created by ");
	    log_name(player);
            log_text((char *) " and emailed to ");
            log_text(pass);
	    ENDLOG
            break;
          case 1:
            newplayer = lookup_player(player, name2, 0);
            if ( Good_obj(newplayer) && newplayer != NOTHING )
               notify(player,"Bad character name in autoregistration.  Player already exists.");
            else
               notify(player,"Bad character name in autoregistration.  Invalid characters.");
            break;
          case 2:
            newplayer = lookup_player(player, name2, 0);
            notify(player,"Warning: Autoregistration email send failed.  Unable to send out email.");
            notify(player,unsafe_tprintf("Player '%s [#%d]' created but NOT emailed to '%s'.", name2, newplayer, pass));
	    STARTLOG(LOG_PCREATES, "CRE", "AREG")
	        log_name(newplayer);
	    log_text((char *) " created by ");
	    log_name(player);
            log_text((char *) " and FAILED to email to ");
            log_text(pass);
	    ENDLOG
            break;
          case 3:
            notify(player,"Spaces in email addresses are not allowed.");
            break;
          case 4:
            notify(player,"@areg limit on specified email reached.  Permission denied.");
            break;
          case 5:
            notify(player, "Invalid email specified.");
            break;
          case 6:
            notify(player, "That email address is not allowed.");
            break;
          case 7:
            notify(player, "Invalid character detected in email.");
            break;
       }
    } else {
       isrobot = (key == PCRE_ROBOT) ? 1 : 0;
       newplayer = create_player(name2, pass, player, isrobot);
       if (newplayer == NOTHING) {
	   notify_quiet(player, unsafe_tprintf("Failure creating '%s'", name2));
	   return;
       }
       if (isrobot) {
	   if (!Good_obj(Location(player))) {
	     if (!Good_obj(default_home()))
	       dest = mudconf.start_room;
	     else
	       dest = default_home();
	   }
	   else
	     dest = Location(player);
	   move_object(newplayer, dest);
	   notify_quiet(player,
		        unsafe_tprintf("New robot '%s' created with password '%s' [dbref #%d]",
			        name2, pass, newplayer));
	   notify_quiet(player, "Your robot has arrived.");
	   STARTLOG(LOG_PCREATES, "CRE", "ROBOT")
	       log_name(newplayer);
	   log_text((char *) " created by ");
	   log_name(player);
	   ENDLOG
       } else {
	   move_object(newplayer, mudconf.start_room);
	   notify_quiet(player,
		        unsafe_tprintf("New player '%.32s' created with password '%.32s' [dbref #%d]",
			        name2, pass, newplayer));
	   STARTLOG(LOG_PCREATES | LOG_WIZARD, "WIZ", "PCREA")
	       log_name(newplayer);
	   log_text((char *) " created by ");
	   log_name(player);
	   ENDLOG
       }
    }
}

/* ---------------------------------------------------------------------------
 * destroy_exit, destroy_thing, destroy_player, do_destroy: Destroy things.
 */

static void 
destroy_exit(dbref player, dbref exit, int purge)
{
    dbref loc;

    loc = Exits(exit);
    if (!Good_obj(loc)) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    if ((loc != Location(player)) && (loc != player)
	&& !Admin(player)) {
	notify_quiet(player, "You can not destroy exits in another room.");
	return;
    }
    if (Good_obj(Exits(loc)))
      s_Exits(loc, remove_first(Exits(loc), exit));
    destroy_obj(player, exit, purge);
}

static void 
destroy_thing(dbref player, dbref thing, int purge)
{
    move_via_generic(thing, NOTHING, player, 0);
    empty_obj(thing);
    destroy_obj(player, thing, purge);
}

/* ---------------------------------------------------------------------------
 * destroyable: Indicates if target of a @destroy is a 'special' object in
 * the database.
 */

static int 
destroyable(dbref victim)
{
    if ((victim == mudconf.default_home) ||
	(victim == mudconf.start_home) ||
	(victim == mudconf.start_room) ||
	(victim == mudconf.master_room) ||
	(God(victim)))
	return 0;
    return 1;
}


static void 
destroy_player(dbref player, dbref victim, int purge)
{
    dbref aowner;
    int count, aflags;
    char *buf, *logbuf, *s_strtok, *s_strtokr;

    if (!Admin(player)) {
	notify_quiet(player, "Sorry, no suicide allowed.");
	return;
    }
    if (Wizard(victim) && !Robot(victim)) {
	notify_quiet(player, "Even you can't do that!");
	return;
    }
    if ( Robot(victim) && (victim == Owner(victim)) && Wizard(victim) ) {
	notify_quiet(player, "Even you can't do that!");
	return;
    }
    /* Bye bye... */
    STARTLOG(LOG_WIZARD, "WIZ", "NUKE")
	logbuf = alloc_lbuf("nuke.LOG");
    sprintf(logbuf,
	    "%.30s(#%d) nuked %.30s(#%d)", Name(player), player,
	    Name(victim), victim);
    log_text(logbuf);
    free_lbuf(logbuf);
    ENDLOG

    boot_off(victim, (char *) "You have been destroyed!");
    halt_que(victim, NOTHING);
    count = chown_all(victim, player);
    /* MMMail Addition for hardcoded mailer */
    mudstate.nuke_status = 1;
    areg_del_player(victim);
    do_mail(victim, victim, M_WRITE, "+forget", NULL);
    do_wmail(player, player, WM_WIPE, myitoa(victim), "");
    mail_rem_dump();
    mudstate.nuke_status = 0;

    /* news nuke hook */
    news_player_nuke_hook( player, victim);

    /* Remove the name from the name hash table */

    delete_player_name(victim, Name(victim));
    buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
    delete_player_name(victim, buf);
    atr_clr(victim, A_ALIAS);
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

    move_via_generic(victim, NOTHING, player, 0);
    destroy_obj(player, victim, purge);
    notify_quiet(player, unsafe_tprintf("(%d objects @chowned to you)", count));

}

void 
do_destroy(dbref player, dbref cause, int key, char *what)
{
    dbref thing, newplayer, aowner2;
    int aflags2, i_array[LIMIT_MAX], i;
    char *s_chkattr, *s_buffptr, *s_mbuf, *tpr_buff, *tprp_buff;


    if (!mudconf.recycle) {
	notify(player, "Sorry, destroying objects is not enabled.");
	return;
    }
    if (key & DEST_PURGE) {
      if (!Immortal(player) && !HasPriv(player,NOTHING,POWER_OPURGE,POWER5,POWER_LEVEL_NA)) {
	notify(player,"Unrecognized switch 'purge' for command '@destroy'.");
	return;
      }
    }
    /* You can destroy anything you control */

    thing = match_controlled_quiet(player, what);

    /* If you own a location, you can destroy its exits */

    if ((thing == NOTHING) && Good_obj(Location(player)) && controls(player, Location(player))) {
	init_match(player, what, TYPE_EXIT);
	match_exit();
	thing = last_match_result();
    }
    /* You may destroy DESTROY_OK things in your inventory */

    if (thing == NOTHING) {
	init_match(player, what, TYPE_THING);
	match_possession();
	thing = last_match_result();
	if ((thing != NOTHING) && (!(isThing(thing) && Destroy_ok(thing)))) 
	    thing = NOPERM;
    }
    /* Return an error if we didn't find anything to destroy */

    if (!Good_obj(match_status(player, thing))) {
	return;
    }
    /* Check SAFE and DESTROY_OK flags */

    if (Safe(thing, player) && !(key & DEST_OVERRIDE) &&
	!(isThing(thing) && Destroy_ok(thing))) {
        if ( key & SIDEEFFECT )
           notify_quiet(player, "Sorry, you can't destroy() protected objects. Use @destroy.");
        else
	   notify_quiet(player,
	   	        "Sorry, that object is protected.  Use @destroy/override to destroy it.");
	return;
    }
    /* Make sure we're not trying to destroy a special object */

    if (!destroyable(thing)) {
	notify_quiet(player, "You can't destroy that!");
	return;
    }
    if (Indestructable(thing)) {
	notify_quiet(player, "Sorry, that's indestructable.");
	return;
    }
    if ( Cluster(thing)) {
        notify_quiet(player, "Sorry, you can't destroy part of a cluster.");
        return;
    }
    if (Going(thing) || Recover(thing)) {
	notify_quiet(player, "That thing does not exist.");
	return;
    }

#ifndef STANDALONE
    if ( Good_obj(player) && !((Wizard(player) || (Good_obj(Owner(player)) && Wizard(Owner(player)))) &&
                             !mudconf.vattr_limit_checkwiz) ) {
       newplayer = NOTHING;
       if ( isPlayer(player) ) {
          newplayer = player;
       } else {
          newplayer = Owner(player);
          if ( !(Good_obj(newplayer) && isPlayer(newplayer)) )
             newplayer = NOTHING;
       }
       if ( newplayer != NOTHING ) {
          s_chkattr = atr_get(newplayer, A_DESTVATTRMAX, &aowner2, &aflags2);
          if ( *s_chkattr ) {
             i_array[0] = i_array[2] = 0;
             i_array[4] = i_array[1] = i_array[3] = -2;
             for (s_buffptr = (char *) strtok(s_chkattr, " "), i = 0;
                  s_buffptr && (i < LIMIT_MAX);
                  s_buffptr = (char *) strtok(NULL, " "), i++) {
                 i_array[i] = atoi(s_buffptr);
             }
             if ( (i_array[3] != -1) && !((i_array[3] == -2) && ((Wizard(player) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) == -1)) ) {
                if ( (i_array[2]+1) > (i_array[3] == -2 ? (Wizard(player) ? mudconf.wizmax_dest_limit : mudconf.max_dest_limit) : i_array[3]) ) {
                   notify_quiet(player,"@destruction limit maximum reached.");
                   STARTLOG(LOG_SECURITY, "SEC", "DESTROY")
                     log_text("@destruction limit maximum reached -> Player: ");
                     log_name(player);
                     log_text(" Object: ");
                     log_name(thing);
                   ENDLOG
                   broadcast_monitor(player,MF_VLIMIT,"DESTROY MAXIMUM",
                           NULL, NULL, thing, 0, 0, NULL);
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
             s_mbuf = alloc_mbuf("vattr_check");
             sprintf(s_mbuf, "0 -2 1 -2 %d", -2);
             atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
             free_mbuf(s_mbuf);
          }
          free_lbuf(s_chkattr);
       }
    }
#endif

    /* Go do it */

    switch (Typeof(thing)) {
    case TYPE_EXIT:
	destroy_exit(player, thing, key & DEST_PURGE);
        if (thing == mudconf.exit_parent)
           mudconf.exit_parent = -1;
	break;
    case TYPE_THING:
	destroy_thing(player, thing, key & DEST_PURGE);
        if (thing == mudconf.thing_parent)
           mudconf.thing_parent = -1;
	break;
    case TYPE_PLAYER:
	notify_quiet(player, "Sorry, use @nuke for players.");
        if (thing == mudconf.player_parent)
           mudconf.player_parent = -1;
	break;
    case TYPE_ROOM:
	if (Going(thing) || Recover(thing) || Byeroom(thing)) {
	    notify_quiet(player, "No sense beating a dead room.");
	} else {
	    notify_all(thing, player,
		       "The room shakes and begins to crumble.");
	    if (!Quiet(thing) && !Quiet(Owner(thing))) {
                tprp_buff = tpr_buff = alloc_lbuf("do_destroy");
		notify_quiet(Owner(thing),
			safe_tprintf(tpr_buff, &tprp_buff, "You will be rewarded shortly for %s(#%d).",
				Name(thing), thing));
		if (Owner(thing) != player) {
                  tprp_buff = tpr_buff;
		  notify_quiet(player,
			safe_tprintf(tpr_buff, &tprp_buff, "%s(#%d) shakes and begins to crumble.",
				Name(thing), thing));
                }
                free_lbuf(tpr_buff);
	    }
	    s_Flags2(thing, Flags2(thing) | BYEROOM);
	    if (key & DEST_PURGE) {
	      s_Flags2(thing, Flags2(thing) | BYEROOM );
	      s_Flags3(thing, Flags3(thing) | DR_PURGE);
	    }
	}
        if (thing == mudconf.room_parent)
           mudconf.room_parent = -1;
    }
}

void 
do_nuke(dbref player, dbref cause, int key, char *name)
{
    dbref thing, aowner2, newplayer;
    int aflags2,  i_array[LIMIT_MAX], i;
    char *s_chkattr, *s_buffptr, *s_mbuf;

    /* XXX This is different from Pern. */

    if (key & NUKE_PURGE) {
      if (!Immortal(player) && !HasPriv(player,NOTHING,POWER_OPURGE,POWER5,POWER_LEVEL_NA)) {
	notify(player,"Unrecognized switch 'purge' for command '@destroy'.");
	return;
      }
    }
    if (!mudconf.recycle) {
	notify(player, "Sorry, destroying objects is not enabled.");
	return;
    }
    init_match(player, name, NOTYPE);
    match_everything(0);
    thing = noisy_match_result();

    if (thing != NOTHING) {
	if (Typeof(thing) == TYPE_PLAYER) {
	    if (Indestructable(thing)) {
		notify_quiet(player, "Sorry, that player is indestructable.");
		return;
	    }
	    if (DePriv(player, thing, DP_NUKE, POWER6, NOTHING)) {
		notify_quiet(player, "Permission denied.");
		return;
	    }
	    if (Going(thing) || Recover(thing)) {
		notify_quiet(player, "You don't @nuke objects, you @destroy them.");
		return;
	    } else {
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
                           STARTLOG(LOG_SECURITY, "SEC", "NUKE")
                             log_text("@destruction limit maximum reached -> Player: ");
                             log_name(player);
                             log_text(" Object: ");
                             log_name(thing);
                           ENDLOG
                           broadcast_monitor(player,MF_VLIMIT,"[NUKE] DESTROY MAXIMUM",
                                   NULL, NULL, thing, 0, 0, NULL);
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
                     s_mbuf = alloc_mbuf("vattr_check");
                     sprintf(s_mbuf, "0 -2 1 -2 %d", -2);
                     atr_add_raw(player, A_DESTVATTRMAX, s_mbuf);
                     free_mbuf(s_mbuf);
                  }
                  free_lbuf(s_chkattr);
               }
               destroy_player(player, thing, key & NUKE_PURGE);
            }
	} else
	    notify(player, "You don't @nuke objects, you @destroy them.");
    } else
	notify(player, unsafe_tprintf("I don't know %s.", name));
}
