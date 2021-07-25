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
open_exit(dbref player, dbref loc, char *direction, char *linkto)
{
    dbref exit;

    if (!Good_obj(loc))
	return;

    if (!direction || !*direction) {
	notify_quiet(player, "Open where?");
	return;
    } else if (!controls(player, loc)) {
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

    notify_quiet(player, "Opened.");

    /* See if we should do a link */

    if (!linkto || !*linkto)
	return;

    loc = parse_linkable_room(player, linkto);
    if (loc != NOTHING) {

	/* Make sure the player passes the link lock */

	if (!could_doit(player, loc, A_LLINK, 0, 0)) {
	    notify_quiet(player, "You can't link to there.");
	    return;
	}
	/* Link it if the player can pay for it */

	if (!payfor(player, mudconf.linkcost)) {
	    notify_quiet(player,
			 unsafe_tprintf("You don't have enough %s to link.",
				 mudconf.many_coins));
	} else {
	    s_Location(exit, loc);
	    notify_quiet(player, "Linked.");
	}
    }
}

char *
get_free_num(dbref player, char *buff)
{
    char tbuff[10], *pt, *retpt;
    dbref ftemp;

    pt = buff + 1;
    retpt = buff;
    while (isdigit(*pt))
	pt++;
    if (*pt != '\0') {
	if ((pt == (buff + 1)) || (!isspace(*pt)))
	    notify(player, "No or bad dbref given.");
	else {
	    *pt = '\0';
	    do {
		pt++;
	    } while (isspace(*pt));
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

    if (key == OPEN_INVENTORY)
	loc = player;
    else
	loc = Location(player);

    open_exit(player, loc, dir2, dest);

    /* Open the back link if we can */

    if (nlinks >= 2) {
	destnum = parse_linkable_room(player, dest);
	if (destnum != NOTHING) {
	    open_exit(player, destnum, links[1],
		      unsafe_tprintf("%d", loc));
	}
    }
}

/* ---------------------------------------------------------------------------
 * link_exit, do_link: Set destination(exits), dropto(rooms) or
 * home(player,thing)
 */

static void 
link_exit(dbref player, dbref exit, dbref dest)
{
    int cost, quot;

    /* Make sure we can link there */

    if ((dest != HOME) &&
	((!controls(player, dest) && !Link_ok(dest)) ||
	 !could_doit(player, dest, A_LLINK, 0, 0))) {
	notify_quiet(player, "Permission denied.");
	return;
    }
    /* Exit must be unlinked or controlled by you */

    if ((Location(exit) != NOTHING) && !controls(player, exit)) {
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

    if (Owner(exit) != Owner(player)) {
	giveto(Owner(exit), mudconf.opencost);
	add_quota(Owner(exit), quot, TYPE_EXIT);
	s_Owner(exit, Owner(player));
	s_Flags(exit, (Flags(exit) & ~(INHERIT | WIZARD)) | HALT);
    }
    /* link has been validated and paid for, do it and tell the player */

    s_Location(exit, dest);
    if (!Quiet(player))
	notify_quiet(player, "Linked.");
}

void 
do_link(dbref player, dbref cause, int key, char *what, char *where)
{
    dbref thing, room;
    char *buff;

    /* Find the thing to link */

    init_match(player, what, TYPE_EXIT);
    match_everything(0);
    thing = noisy_match_result();
    if (thing == NOTHING)
	return;

    /* Allow unlink if where is not specified */

    if (!where || !*where) {
	do_unlink(player, cause, key, what);
	return;
    }
    switch (Typeof(thing)) {
    case TYPE_EXIT:

	/* Set destination */

	room = parse_linkable_room(player, where);
	if (room != NOTHING)
	    link_exit(player, thing, room);
	break;
    case TYPE_PLAYER:
    case TYPE_THING:

	/* Set home */

	if (!Controls(player, thing)) {
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
	    !could_doit(player, room, A_LLINK, 0, 0)) {
	    notify_quiet(player, "Permission denied.");
	} else if (room == HOME) {
	    notify_quiet(player, "Can't set home to home.");
	} else {
	    s_Home(thing, room);
	    if (!Quiet(player))
		notify_quiet(player, "Home set.");
	}
	break;
    case TYPE_ROOM:

	/* Set dropto */

	if (!Controls(player, thing)) {
	    notify_quiet(player, "Permission denied.");
	    break;
	}
	room = parse_linkable_room(player, where);
	if (!(Good_obj(room) || (room == HOME)))
	    break;

	if ((room != HOME) && !isRoom(room)) {
	    notify_quiet(player, "That is not a room!");
	} else if ((room != HOME) &&
		   ((!controls(player, room) && !Link_ok(room)) ||
		    !could_doit(player, room, A_LLINK, 0, 0))) {
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
 * do_zoneadd: Add a zonemaster to an object's zone list
 */

void
do_zoneadd(dbref player, dbref cause, int key, char *tname, char *pname)
{
  dbref thing, zonemaster;

  init_match(player, tname, NOTYPE);
  match_everything(0);
  thing = noisy_match_result();
  if( thing == NOTHING )
    return;

  /* Make sure we can do it */

  if (!Controls(player, thing)) {
     notify_quiet(player, "Permission denied.");
     return;
  }
  /* Find out what the new zone is */

  if( !*pname ) {
    notify_quiet(player, "You must provide a Zone Master reference.");
    return;
  }

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
     !) {
     notify_quiet(player, "Permission denied.");
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

    if (!Controls(player, thing)) {
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
    if (!Quiet(thing) && !Quiet(player)) {
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
    dbref room;
    char *buff, *name2;

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

    notify(player,
	   unsafe_tprintf("%s created with room number %d.", name2, room));

    buff = alloc_sbuf("do_dig");
    if ((nargs >= 1) && args[0] && *args[0]) {
	sprintf(buff, "%d", room);
	open_exit(player, Location(player), args[0], buff);
    }
    if ((nargs >= 2) && args[1] && *args[1]) {
	sprintf(buff, "%d", Location(player));
	open_exit(player, room, args[1], buff);
    }
    free_sbuf(buff);
    if (key == DIG_TELEPORT)
	(void) move_via_teleport(player, room, cause, 0);
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

    thing = create_obj(player, TYPE_THING, name2, cost);
    if (thing == NOTHING)
	return;

    move_via_generic(thing, player, NOTHING, 0);
    s_Home(thing, new_home(player));
    if (!Quiet(player)) {
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
    int cost;

    if ((key & CLONE_INVENTORY) || !Has_location(player))
	loc = player;
    else
	loc = Location(player);

    if (!Good_obj(loc))
	return;

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
	notify_quiet(player,
		     unsafe_tprintf("You don't control %s, ignoring /parent.",
			     Name(thing)));
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

    al_destroy(clone);
    if (key & CLONE_PARENT)
	s_Parent(clone, thing);
    else
	atr_cpy(player, clone, thing);

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

    if (!Quiet(player)) {
	if (arg2 && *arg2)
	    notify(player,
		   unsafe_tprintf("%s cloned as %s, new copy is object #%d.",
			   Name(thing), arg2, clone));
	else
	    notify(player,
		   unsafe_tprintf("%s cloned, new copy is object #%d.",
			   Name(thing), clone));
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
	    link_exit(player, clone, Dropto(thing));
	break;
    case TYPE_EXIT:
	s_Exits(loc, insert_first(Exits(loc), clone));
	s_Exits(clone, loc);
	s_Location(clone, NOTHING);
	if (Location(thing) != NOTHING)
	    link_exit(player, clone, Location(thing));
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
    dbref newplayer;
    char *name2;

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
    isrobot = (key == PCRE_ROBOT) ? 1 : 0;
    newplayer = create_player(name2, pass, player, isrobot);
    if (newplayer == NOTHING) {
	notify_quiet(player, unsafe_tprintf("Failure creating '%s'", name2));
	return;
    }
    if (isrobot) {
	move_object(newplayer, Location(player));
	notify_quiet(player,
		     unsafe_tprintf("New robot '%s' created with password '%s'",
			     name2, pass));
	notify_quiet(player, "Your robot has arrived.");
	STARTLOG(LOG_PCREATES, "CRE", "ROBOT")
	    log_name(newplayer);
	log_text((char *) " created by ");
	log_name(player);
	ENDLOG
    } else {
	move_object(newplayer, mudconf.start_room);
	notify_quiet(player,
		     unsafe_tprintf("New player '%s' created with password '%s'",
			     name2, pass));
	STARTLOG(LOG_PCREATES | LOG_WIZARD, "WIZ", "PCREA")
	    log_name(newplayer);
	log_text((char *) " created by ");
	log_name(player);
	ENDLOG
    }
}

/* ---------------------------------------------------------------------------
 * destroy_exit, destroy_thing, destroy_player, do_destroy: Destroy things.
 */

static void 
destroy_exit(dbref player, dbref exit)
{
    dbref loc;

    loc = Exits(exit);
    if ((loc != Location(player)) && (loc != player)
	&& !Admin(player)) {
	notify_quiet(player, "You can not destroy exits in another room.");
	return;
    }
    s_Exits(loc, remove_first(Exits(loc), exit));
    destroy_obj(player, exit);
}

static void 
destroy_thing(dbref player, dbref thing)
{
    move_via_generic(thing, NOTHING, player, 0);
    empty_obj(thing);
    destroy_obj(player, thing);
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
destroy_player(dbref player, dbref victim)
{
    dbref aowner;
    int count, aflags;
    char *buf, *logbuf;

    if (!Admin(player)) {
	notify_quiet(player, "Sorry, no suicide allowed.");
	return;
    }
    if (Wizard(victim) && !Robot(victim)) {
	notify_quiet(player, "Even you can't do that!");
	return;
    }
    /* Bye bye... */
    STARTLOG(LOG_WIZARD, "WIZ", "NUKE")
	logbuf = alloc_lbuf("nuke.LOG");
    sprintf(logbuf,
	    "%s(#%d) nuked %s(#%d)", Name(player), player,
	    Name(victim), victim);
    log_text(logbuf);
    free_lbuf(logbuf);
    ENDLOG

	boot_off(victim, (char *) "You have been destroyed!");
    halt_que(victim, NOTHING);
    count = chown_all(victim, player, 0);
    /* MMMail Addition for hardcoded mailer */
    mudstate.nuke_status = 1;
    do_wmail(player, player, WM_WIPE, Name(victim), "");
    mudstate.nuke_status = 0;

    /* Remove the name from the name hash table */

    delete_player_name(victim, Name(victim));
    buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
    delete_player_name(victim, buf);
    atr_clr(victim, A_ALIAS);
    free_lbuf(buf);

    move_via_generic(victim, NOTHING, player, 0);
    destroy_obj(player, victim);
    notify_quiet(player, unsafe_tprintf("(%d objects @chowned to you)", count));

}

void 
do_destroy(dbref player, dbref cause, int key, char *what)
{
    dbref thing;
    int ctl, destok;

    /* You can destroy anything you control */

    thing = match_controlled(player, what);

    /* If you own a location, you can destroy its exits */

    if ((thing == NOTHING) && controls(player, Location(player))) {
	init_match(player, what, TYPE_EXIT);
	match_exit();
	thing = last_match_result();
    }
    /* You may destroy DESTROY_OK things in your inventory */

    if (thing == NOTHING) {
	init_match(player, what, TYPE_THING);
	match_possession();
	thing = last_match_result();
	if (!(isThing(thing) && Destroy_ok(thing))) {
	    thing = NOTHING;
	    notify_quiet(player, "Permission denied.");
	}
    }
    /* Return an error if we didn't find anything to destroy */

    if (match_status(player, thing) == NOTHING) {
	return;
    }
    /* Check SAFE and DESTROY_OK flags */

    if (Safe(thing, player) && !(key & DEST_OVERRIDE) &&
	!(isThing(thing) && Destroy_ok(thing))) {
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
	notify_quiet(player, "Sorry, that's indestructible.");
	return;
    }
    if (Going(thing) || Recover(thing)) {
	notify_quiet(player, "That thing does not exist.");
	return;
    }
    /* Go do it */

    switch (Typeof(thing)) {
    case TYPE_EXIT:
	destroy_exit(player, thing);
	break;
    case TYPE_THING:
	destroy_thing(player, thing);
	break;
    case TYPE_PLAYER:
	notify_quiet(player, "Sorry, use @nuke for players.");
	break;
    case TYPE_ROOM:
	if (Going(thing) || Recover(thing) || Byeroom(thing)) {
	    notify_quiet(player, "No sense beating a dead room.");
	} else {
	    notify_all(thing, player,
		       "The room shakes and begins to crumble.");
	    if (!Quiet(thing) && !Quiet(Owner(thing)))
		notify_quiet(Owner(thing),
			unsafe_tprintf("You will be rewarded shortly for %s(#%d).",
				Name(thing), thing));
	    s_Flags2(thing, Flags2(thing) | BYEROOM);
	}
    }
}

void 
do_nuke(dbref player, dbref cause, int key, char *name)
{
    dbref thing;

    /* XXX This is different from Pern. */

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
		notify_quiet(player, "Sorry, that player is indestructible.");
		return;
	    }
	    if (DePriv(player, thing, DP_NUKE, POWER6, NOTHING)) {
		notify_quiet(player, "Permission denied.");
		return;
	    }
	    if (Going(thing) || Recover(thing)) {
		notify_quiet(player, "You don't @nuke objects, you @destroy them.");
		return;
	    } else
		destroy_player(player, thing);
	} else
	    notify(player, "You don't @nuke objects, you @destroy them.");
    } else
	notify(player, unsafe_tprintf("I don't know %s.", name));
}
