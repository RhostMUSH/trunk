/* object.c - low-level object manipulation routines */

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "mudconf.h"
#include "command.h"
#include "externs.h"
#include "flags.h"
#include "alloc.h"
#include "local.h"

#define IS_CLEAN(i)	(IS(i, TYPE_THING, GOING) && \
			 (Location(i) == NOTHING) && \
			 (Contents(i) == NOTHING) && (Exits(i) == NOTHING) && \
			 (Next(i) == NOTHING) && (Owner(i) == GOD))

#define ZAP_LOC(i)	{ s_Location(i, NOTHING); s_Next(i, NOTHING); }

extern double NDECL(next_timer);

extern int FDECL(alarm_msec, (double));

static int check_type;

#ifdef STANDALONE

/* Functions needed in standalone mode */

/* move_object: taken from move.c with look and penny check extracted */

void 
move_object(dbref thing, dbref dest)
{
    dbref src;

    /* Remove from the source location */

    src = Location(thing);
    if (src != NOTHING)
	s_Contents(src, remove_first(Contents(src), thing));

    /* Special check for HOME */

    if (dest == HOME)
	dest = Home(thing);

    /* Add to destination location */

    if (dest != NOTHING) {
	s_Contents(dest, insert_first(Contents(dest), thing));
    } else {
	s_Next(thing, NOTHING);
    }
    s_Location(thing, dest);
}

#define move_via_generic(obj,where,extra,hush) move_object(obj,where)

#endif

/* ---------------------------------------------------------------------------
 * Log_pointer_err, Log_header_err, Log_simple_damage: Write errors to the
 * log file.
 */

static void 
Log_pointer_err(dbref prior, dbref obj, dbref loc,
		dbref ref, const char *reftype, const char *errtype)
{
    STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG")
	log_type_and_name(obj);
    if (loc != NOTHING) {
	log_text((char *) " in ");
	log_type_and_name(loc);
    }
    log_text((char *) ": ");
    if (prior == NOTHING) {
	log_text((char *) reftype);
    } else {
	log_text((char *) "Next pointer");
    }
    log_text((char *) " ");
    log_type_and_name(ref);
    log_text((char *) " ");
    log_text((char *) errtype);
    ENDLOG
}

static void 
Log_header_err(dbref obj, dbref loc, dbref val, int is_object,
	       const char *valtype, const char *errtype)
{
    STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG")
	log_type_and_name(obj);
    if (loc != NOTHING) {
	log_text((char *) " in ");
	log_type_and_name(loc);
    }
    log_text((char *) ": ");
    log_text((char *) valtype);
    log_text((char *) " ");
    if (is_object)
	log_type_and_name(val);
    else
	log_number(val);
    log_text((char *) " ");
    log_text((char *) errtype);
    ENDLOG
}

static void 
Log_simple_err(dbref obj, dbref loc, const char *errtype)
{
    STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG")
	log_type_and_name(obj);
    if (loc != NOTHING) {
	log_text((char *) " in ");
	log_type_and_name(loc);
    }
    log_text((char *) ": ");
    log_text((char *) errtype);
    ENDLOG
}

/* ---------------------------------------------------------------------------
 * start_home, default_home, can_set_home, new_home, clone_home:
 * Routines for validating and determining homes.
 */

dbref 
NDECL(start_home)
{
    if (mudconf.start_home != NOTHING)
	return mudconf.start_home;
    return mudconf.start_room;
}

dbref 
NDECL(default_home)
{
    if (mudconf.default_home != NOTHING)
	return mudconf.default_home;
    if (mudconf.start_home != NOTHING)
	return mudconf.start_home;
    return mudconf.start_room;
}

int 
can_set_home(dbref player, dbref thing, dbref home)
{
    if (!Good_obj(player) || !Good_obj(home) || (thing == home))
	return 0;

    switch (Typeof(home)) {
    case TYPE_PLAYER:
    case TYPE_ROOM:
    case TYPE_THING:
	if (Going(home) || Recover(home))
	    return 0;
	if (Controls(player, home) || Abode(home))
	    return 1;
    }
    return 0;
}

dbref 
new_home(dbref player)
{
    dbref loc;

    loc = Location(player);
    if (can_set_home(Owner(player), player, loc))
	return loc;
    loc = Home(Owner(player));
    if (can_set_home(Owner(player), player, loc))
	return loc;
    return default_home();
}

dbref 
clone_home(dbref player, dbref thing)
{
    dbref loc;

    loc = Home(thing);
    if (can_set_home(Owner(player), player, loc))
	return loc;
    return new_home(player);
}

/* ---------------------------------------------------------------------------
 * create_obj: Create an object of the indicated type IF the player can
 * afford it.
 */

#ifndef STANDALONE

dbref 
create_obj(dbref player, int objtype, char *name, int cost)
{
    dbref obj, owner, tlink, aowner;
    ZLISTNODE* z_ptr;
    int quota, okname, value, self_owned, require_inherit, intarray[4], aflags, i;
    FLAG f1, f2, f3, f4, t1, t2, t3, t4, t5, t6, t7, t8;
    time_t tt;
    char *buff, *lastcreatebuff, *p, *tpr_buff, *tprp_buff;
    const char *tname;

    if (DePriv(player, NOTHING, DP_CREATE, POWER6, POWER_LEVEL_NA)) {
	notify(player, "Permission denied.");
	return NOTHING;
    }
    value = 0;
    quota = 0;
    self_owned = 0;
    require_inherit = 0;

    switch (objtype) {
    case TYPE_ROOM:
	cost = mudconf.digcost;
	quota = mudconf.room_quota;
	f1 = mudconf.room_flags.word1;
	f2 = mudconf.room_flags.word2;
	f3 = mudconf.room_flags.word3;
	f4 = mudconf.room_flags.word4;
	t1 = mudconf.room_toggles.word1;
	t2 = mudconf.room_toggles.word2;
	t3 = mudconf.room_toggles.word3;
	t4 = mudconf.room_toggles.word4;
	t5 = mudconf.room_toggles.word5;
	t6 = mudconf.room_toggles.word6;
	t7 = mudconf.room_toggles.word7;
	t8 = mudconf.room_toggles.word8;

	okname = ok_name(name);
	tname = "a room";
	break;
    case TYPE_THING:
	if (cost < mudconf.createmin)
	    cost = mudconf.createmin;
	if (cost > mudconf.createmax)
	    cost = mudconf.createmax;
	quota = mudconf.thing_quota;
	f1 = mudconf.thing_flags.word1;
	f2 = mudconf.thing_flags.word2;
	f3 = mudconf.thing_flags.word3;
	f4 = mudconf.thing_flags.word4;
	t1 = mudconf.thing_toggles.word1;
	t2 = mudconf.thing_toggles.word2;
	t3 = mudconf.thing_toggles.word3;
	t4 = mudconf.thing_toggles.word4;
	t5 = mudconf.thing_toggles.word5;
	t6 = mudconf.thing_toggles.word6;
	t7 = mudconf.thing_toggles.word7;
	t8 = mudconf.thing_toggles.word8;
	value = OBJECT_ENDOWMENT(cost);
	okname = ok_name(name);
	tname = "a thing";
	break;
    case TYPE_EXIT:
	cost = mudconf.opencost;
	quota = mudconf.exit_quota;
	f1 = mudconf.exit_flags.word1;
	f2 = mudconf.exit_flags.word2;
	f3 = mudconf.exit_flags.word3;
	f4 = mudconf.exit_flags.word4;
	t1 = mudconf.exit_toggles.word1;
	t2 = mudconf.exit_toggles.word2;
	t3 = mudconf.exit_toggles.word3;
	t4 = mudconf.exit_toggles.word4;
	t5 = mudconf.exit_toggles.word5;
	t6 = mudconf.exit_toggles.word6;
	t7 = mudconf.exit_toggles.word7;
	t8 = mudconf.exit_toggles.word8;
	okname = ok_name(name);
	tname = "an exit";
	break;
    case TYPE_PLAYER:
	if (cost) {
	    cost = mudconf.robotcost;
	    quota = mudconf.player_quota;
	    f1 = mudconf.robot_flags.word1;
	    f2 = mudconf.robot_flags.word2;
	    f3 = mudconf.robot_flags.word3;
	    f4 = mudconf.robot_flags.word4;
	    t1 = mudconf.robot_toggles.word1;
	    t2 = mudconf.robot_toggles.word2;
	    t3 = mudconf.robot_toggles.word3;
	    t4 = mudconf.robot_toggles.word4;
	    t5 = mudconf.robot_toggles.word5;
	    t6 = mudconf.robot_toggles.word6;
	    t7 = mudconf.robot_toggles.word7;
	    t8 = mudconf.robot_toggles.word8;
	    value = 0;
	    tname = "a robot";
	    require_inherit = 1;
	} else {
	    cost = 0;
	    quota = 0;
	    f1 = mudconf.player_flags.word1;
	    f2 = mudconf.player_flags.word2;
	    f3 = mudconf.player_flags.word3;
	    f4 = mudconf.player_flags.word4;
	    t1 = mudconf.player_toggles.word1;
	    t2 = mudconf.player_toggles.word2;
	    t3 = mudconf.player_toggles.word3;
	    t4 = mudconf.player_toggles.word4;
	    t5 = mudconf.player_toggles.word5;
	    t6 = mudconf.player_toggles.word6;
	    t7 = mudconf.player_toggles.word7;
	    t8 = mudconf.player_toggles.word8;
	    value = mudconf.paystart;
	    quota = mudconf.start_quota;
	    self_owned = 1;
	    tname = "a player";
	}
	buff = munge_space(strip_all_special(name));
	okname = (*buff && badname_check(buff, player) && (protectname_check(buff, player, 1) > 0) );
	if (okname)
	    okname = ok_player_name(buff);
	if (okname)
	    okname = (lookup_player(NOTHING, buff, 0) == NOTHING);
	free_lbuf(buff);
	break;
    case TYPE_ZONE:
/*              cost = mudconf.zonecost;        */
/*              quota = mudconf.zone_quota;     */
/*              f1 = mudconf.zone_flags.word1;  */
/*              f2 = mudconf.zone_flags.word2;  */
	cost = 0;
	quota = 0;
	f1 = 0;
	f2 = 0;
	f3 = 0;
	f4 = 0;
	t1 = 0;
	t2 = 0;
	t3 = 0;
	t4 = 0;
	t5 = 0;
	t6 = 0;
	t7 = 0;
	t8 = 0;
	okname = ok_name(name);
	tname = "a zone";
	break;
    default:
        tprp_buff = tpr_buff = alloc_lbuf("create_obj");
	LOG_SIMPLE(LOG_BUGS, "BUG", "OTYPE",
		   safe_tprintf(tpr_buff, &tprp_buff, "Bad object type in create_obj: %d.",
			   objtype));
        free_lbuf(tpr_buff);
	return NOTHING;
    }

    if (!self_owned) {
	if (!Good_obj(player))
	    return NOTHING;
	owner = Owner(player);
	if (!Good_obj(owner))
	    return NOTHING;
    } else {
	owner = NOTHING;
    }

    if (require_inherit) {
	if (!Inherits(player)) {
	    notify(player, "Permission denied.");
	    return NOTHING;
	}
    }
    if (!okname) {
        tprp_buff = tpr_buff = alloc_lbuf("create_obj");
	notify(player, safe_tprintf(tpr_buff, &tprp_buff, "That's a silly name for %s!", tname));
        free_lbuf(tpr_buff);
	return NOTHING;
    }
    /* Make sure the creator can pay for the object. */

    if ((player != NOTHING) && !canpayfees(player, player, cost, quota, objtype))
	return NOTHING;

    /* Get the first object from the freelist.  If the object is not clean,
     * discard the remainder of the freelist and go get a completely new
     * object.
     */

    obj = NOTHING;
    if (mudstate.freelist != NOTHING) {
	obj = mudstate.freelist;
	if (mudstate.free_num != NOTHING) {
	    tlink = obj; /* used to be an IS_CLEAN in the following */
	    while (Good_obj(obj) && (Link(obj) != NOTHING) && (obj != mudstate.free_num)) {
		if (tlink != obj)
		    tlink = obj;
		obj = Link(tlink);
	    } /* Used to be an IS_CLEAN in the following */
	    if (Good_obj(obj) && (obj == mudstate.free_num)) {
		if (mudstate.freelist == obj) {
		    mudstate.freelist = Link(obj);
		} else {
		    s_Link(tlink, Link(obj));
		}
	    } else {
		obj = mudstate.freelist; /* used to be IS_CLEAN in the following */
		if (Good_obj(obj)) {
		    mudstate.freelist = Link(obj);
		} else {
                    tprp_buff = tpr_buff = alloc_lbuf("create_obj");
		    LOG_SIMPLE(LOG_PROBLEMS, "FRL", "DAMAG",
			       safe_tprintf(tpr_buff, &tprp_buff, "Freelist damaged, bad object #%d.",
				       obj));
                    free_lbuf(tpr_buff);
		    obj = NOTHING;
		    mudstate.freelist = NOTHING;
		}
	    }
	    mudstate.free_num = NOTHING;
	} else { /* used to be an is clean in the following */
	    if (Good_obj(obj)) {
		mudstate.freelist = Link(obj);
	    } else {
                tprp_buff = tpr_buff = alloc_lbuf("create_obj");
		LOG_SIMPLE(LOG_PROBLEMS, "FRL", "DAMAG",
			   safe_tprintf(tpr_buff, &tprp_buff, "Freelist damaged, bad object #%d.",
				   obj));
                free_lbuf(tpr_buff);
		obj = NOTHING;
		mudstate.freelist = NOTHING;
	    }
	}
    }
    if (obj == NOTHING) {
	if (mudconf.max_size > 0) {
	    if (mudstate.db_top >= mudconf.max_size) {
		notify(player, "The database has reached it MAXIMUM number of objects.");
		notify(player, "Please recycle some old objects!");
		return NOTHING;
	    }
	}
	obj = mudstate.db_top;
	db_grow(mudstate.db_top + 1);
    }
    atr_free(obj);		/* just in case... */

    /* Set things up according to the object type */

    s_Location(obj, NOTHING);
    s_Contents(obj, NOTHING);
    s_Exits(obj, NOTHING);
    s_Next(obj, NOTHING);
    s_Link(obj, NOTHING);
    if (objtype == TYPE_PLAYER && Good_obj(mudconf.player_parent) && 
        !Recover(mudconf.player_parent) && obj != mudconf.player_parent)
       s_Parent(obj, mudconf.player_parent);
    else if (objtype == TYPE_ROOM && Good_obj(mudconf.room_parent) &&
        !Recover(mudconf.room_parent) && obj != mudconf.room_parent)
       s_Parent(obj, mudconf.room_parent);
    else if (objtype == TYPE_EXIT && Good_obj(mudconf.exit_parent) &&
        !Recover(mudconf.exit_parent) && obj != mudconf.exit_parent)
       s_Parent(obj, mudconf.exit_parent);
    else if (objtype == TYPE_THING && Good_obj(mudconf.thing_parent) &&
        !Recover(mudconf.thing_parent) && obj != mudconf.thing_parent)
       s_Parent(obj, mudconf.thing_parent);
    else 
       s_Parent(obj, NOTHING);
/*      s_Zone(obj, NOTHING); */
    s_Flags(obj, objtype | f1);
    s_Flags2(obj, f2);
    s_Flags3(obj, f3);
    s_Flags4(obj, f4);
    s_Toggles(obj, t1);
    s_Toggles2(obj, t2);
    s_Toggles3(obj, t3);
    s_Toggles4(obj, t4);
    s_Toggles5(obj, t5);
    s_Toggles6(obj, t6);
    s_Toggles7(obj, t7);
    s_Toggles8(obj, t8);
    if ( (objtype == TYPE_PLAYER) &&
         Good_obj(mudconf.global_clone_player) && 
         !Recover(mudconf.global_clone_player) &&
         !Going(mudconf.global_clone_player) ) {
       atr_cpy(1, obj, mudconf.global_clone_player);
    } else if ( (objtype == TYPE_ROOM) &&
         Good_obj(mudconf.global_clone_room) && 
         !Recover(mudconf.global_clone_room) &&
         !Going(mudconf.global_clone_room) ) {
       atr_cpy(1, obj, mudconf.global_clone_room);
    } else if ( (objtype == TYPE_THING) &&
         Good_obj(mudconf.global_clone_thing) && 
         !Recover(mudconf.global_clone_thing) &&
         !Going(mudconf.global_clone_thing) ) {
       atr_cpy(1, obj, mudconf.global_clone_thing);
    } else if ( (objtype == TYPE_EXIT) &&
         Good_obj(mudconf.global_clone_exit) && 
         !Recover(mudconf.global_clone_exit) &&
         !Going(mudconf.global_clone_exit) ) {
       atr_cpy(1, obj, mudconf.global_clone_exit);
    } else if ( Good_obj(mudconf.global_clone_obj) && 
         !Recover(mudconf.global_clone_obj) &&
         !Going(mudconf.global_clone_obj) ) {
       atr_cpy(1, obj, mudconf.global_clone_obj);
    }
    s_Owner(obj, (self_owned ? obj : owner));

    if ( AutoZoneAll(player) && db[player].zonelist ) {
       for( z_ptr = db[player].zonelist; z_ptr; z_ptr = z_ptr->next ) {
          zlist_add(obj, z_ptr->object);
          zlist_add(z_ptr->object, obj);
       }
    } else if ( AutoZoneAdd(player) && db[player].zonelist ) {
       z_ptr = db[player].zonelist;
       zlist_add(obj, z_ptr->object);
       zlist_add(z_ptr->object, obj);
    } else {
       db[obj].zonelist = NULL;
    }

    s_Pennies(obj, value);
    Unmark(obj);
    buff = munge_space((char *) strip_all_special(name));
    s_Name(obj, buff);
    free_lbuf(buff);

    if (objtype == TYPE_PLAYER) {
	time(&tt);
	buff = (char *) ctime(&tt);
	buff[strlen(buff) - 1] = '\0';
	atr_add_raw(obj, A_LAST, buff);

	buff = alloc_sbuf("create_obj.quota");
	sprintf(buff, "%d", quota);
	atr_add_raw(obj, A_QUOTA, buff);
	atr_add_raw(obj, A_RQUOTA, buff);
	add_player_name(obj, Name(obj));
	free_sbuf(buff);
    } else if (objtype == TYPE_ZONE) {
	atr_add_raw(obj, A_QUOTA, "0");
	atr_add_raw(obj, A_RQUOTA, "0");
    }
    if ( mudconf.enable_tstamps && !NoTimestamp(obj) ) {
       time(&tt);
       buff = (char *) ctime(&tt);
       buff[strlen(buff) - 1] = '\0';
       atr_add_raw(obj, A_CREATED_TIME, buff);
       atr_add_raw(obj, A_MODIFY_TIME, buff);
    }
    atr_add_raw(obj, A_LASTCREATE, "-1 -1 -1 -1");
    lastcreatebuff = atr_get(player, A_LASTCREATE, &aowner, &aflags);
    /* Copy attributes from clone if clone exits and is valid */
    mudstate.store_lastcr = obj;
    if ( !*lastcreatebuff ) {
       tprp_buff = tpr_buff = alloc_lbuf("create_obj");
       if ( objtype == TYPE_ROOM )
          atr_add_raw(player,  A_LASTCREATE, safe_tprintf(tpr_buff, &tprp_buff, "%d -1 -1 -1", obj));
       else if ( objtype == TYPE_EXIT )
          atr_add_raw(player,  A_LASTCREATE, safe_tprintf(tpr_buff, &tprp_buff, "-1 %d -1 -1", obj));
       else if ( objtype == TYPE_THING )
          atr_add_raw(player,  A_LASTCREATE, safe_tprintf(tpr_buff, &tprp_buff, "-1 -1 %d -1", obj));
       else if ( objtype == TYPE_PLAYER )
          atr_add_raw(player,  A_LASTCREATE, safe_tprintf(tpr_buff, &tprp_buff, "-1 -1 -1 %d", obj));
       else
          atr_add_raw(player,  A_LASTCREATE, (char *)"-1 -1 -1 -1");
       free_lbuf(tpr_buff);
    } else {
       for (i = 0; i < 4; i++)
          intarray[i] = -1;
       for (p = (char *) strtok(lastcreatebuff, " "), i = 0;
            p && (i < 4);
            p = (char *) strtok(NULL, " "), i++) {
           intarray[i] = atoi(p);
       }
       switch( objtype ) {
          case TYPE_ROOM  : intarray[0] = obj;
                            break;
          case TYPE_EXIT  : intarray[1] = obj;
                            break;
          case TYPE_THING : intarray[2] = obj;
                            break;
          case TYPE_PLAYER: intarray[3] = obj;
                            break;
       }
       tprp_buff = tpr_buff = alloc_lbuf("create_obj");
       atr_add_raw(player, A_LASTCREATE, safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d %d", 
                   intarray[0], intarray[1], intarray[2], intarray[3]));
       free_lbuf(tpr_buff);
    }
    free_lbuf(lastcreatebuff);
    return obj;
}

#endif

/* ---------------------------------------------------------------------------
 * destroy_obj: Destroy an object.  Assumes it has already been removed from
 * all lists and has no contents or exits.
 */

void 
destroy_obj(dbref player, dbref obj, int purge)
{
    dbref owner;
    int good_owner, val, quota;

#ifndef STANDALONE
    char *tname;
    char tbuff[20];
    char *tpr_buff = NULL, *tprp_buff = NULL;
#endif

    if (!Good_obj(obj))
	return;

    /* Validate the owner */

    owner = Owner(obj);
    good_owner = Good_owner(owner);

    /* Halt any pending commands (waiting or semaphore) */

#ifndef STANDALONE
    if (halt_que(NOTHING, obj) > 0) {
	if (good_owner && !Quiet(obj) && !Quiet(owner)) {
	    notify(owner, "Halted.");
	}
    }
    nfy_que(obj, NFY_DRAIN, 0, -1);
#endif

    /* Compensate the owner for the object */

    val = 1;
    quota = 1;
    if (good_owner && (owner != obj)) {
	switch (Typeof(obj)) {
	case TYPE_ROOM:
	    val = mudconf.digcost;
	    quota = mudconf.room_quota;
	    break;
	case TYPE_THING:
	    val = OBJECT_DEPOSIT(Pennies(obj));
	    quota = mudconf.thing_quota;
	    break;
	case TYPE_EXIT:
	    val = mudconf.opencost;
	    quota = mudconf.exit_quota;
	    break;
	case TYPE_PLAYER:
	    if (Robot(obj))
		val = mudconf.robotcost;
	    else
		val = 0;
	    quota = mudconf.player_quota;
	}
	if (!FreeFlag(owner))
	  giveto(owner, val, NOTHING);
	if (mudconf.quotas)
	    add_quota(owner, quota, Typeof(obj));

#ifndef STANDALONE
	if (!Quiet(owner) && !Quiet(obj)) {
            tprp_buff = tpr_buff = alloc_lbuf("destroy_obj");
	    notify(owner,
		   safe_tprintf(tpr_buff, &tprp_buff, "You get back your %d %s deposit for %s(#%d).",
			        val, mudconf.one_coin, Name(obj), obj));
            free_lbuf(tpr_buff);
        }
#endif
    }
#ifndef STANDALONE
    if ((player != NOTHING) && !Quiet(player)) {
	if (good_owner && Owner(player) != owner) {
            tprp_buff = tpr_buff = alloc_lbuf("destroy_obj");
	    if (owner == obj) {
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "Destroyed. %s(#%d)",
			            Name(obj), obj));
	    } else {
		tname = alloc_sbuf("destroy_obj");
		strcpy(tname, Name(owner));
		notify(player,
		       safe_tprintf(tpr_buff, &tprp_buff, "Destroyed. %s's %s(#%d)",
			            tname, Name(obj), obj));
		free_sbuf(tname);
	    }
            free_lbuf(tpr_buff);
	} else if (!Quiet(obj)) {
	    notify(player, "Destroyed.");
	}
    }
    sprintf(tbuff, "%ld", mudstate.now);
    atr_add_raw(obj, A_RECTIME, tbuff);
#endif
    zlist_destroy(obj);
    s_Location(obj, NOTHING);
    s_Link(obj, NOTHING);
    s_Parent(obj, NOTHING);
    s_Exits(obj, NOTHING);
    if (purge) {
	atr_free(obj);
	s_Name(obj, NULL);
	s_Flags3(obj, 0);
	s_Flags4(obj, 0);
	s_Toggles(obj, 0);
	s_Toggles2(obj, 0);
	s_Toggles3(obj, 0);
	s_Toggles4(obj, 0);
	s_Contents(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Owner(obj, GOD);
	s_Pennies(obj, 0);
	s_Flags(obj, (TYPE_THING | GOING));
	s_Flags2(obj, 0);
    }
    else {
      s_Flags4(obj, Flags4(obj) & ~BOUNCE);
      s_Flags2(obj, (Flags2(obj) | RECOVER) & ~BYEROOM);
      s_Flags(obj, (Flags(obj) | HALT) & ~PUPPET);
      s_Next(obj, Owner(obj));
      s_Owner(obj, GOD);
      s_Contents(obj, player);
    }
    cache_reset(0);
    return;
}

#ifndef STANDALONE
void 
do_recover(dbref player, dbref cause, int key, char *buff)
{
    dbref obj, owner, place, aowner;
    int aflags;
    char *tname, *aregbuff, *pt1;

    if (*buff != '#') {
	notify(player, "No dbref specified.");
	return;
    }
    tname = buff + 1;
    while (isdigit((int)*tname))
	tname++;
    if (*tname) {
	notify(player, "Bad dbref specified.");
	return;
    }
    obj = atoi(buff + 1);
    if ((obj < 0) || (obj > mudstate.db_top)) {
	notify(player, "Bad dbref specified.");
	return;
    }
    owner = mudstate.recoverlist;
    place = NOTHING;
    while ((owner != NOTHING) && (owner != obj)) {
	place = owner;
	owner = Link(owner);
    }
    if (owner != obj) {
	notify(player, "Dbref not found in recover list.");
	return;
    }
    if ((Typeof(obj) == TYPE_PLAYER) && (lookup_player(NOTHING, Name(obj), 0) != NOTHING)) {
	notify(player, "Player name already exists.");
	return;
    }
    if (place == NOTHING) {
	mudstate.recoverlist = Link(obj);
    } else {
	s_Link(place, Link(obj));
    }
    s_Flags2(obj, Flags2(obj) & ~RECOVER);
    atr_clr(obj, A_RECTIME);
    switch (Typeof(obj)) {
    case TYPE_ROOM:
	s_Link(obj, NOTHING);
	s_Owner(obj, player);
	s_Next(obj, NOTHING);
	break;
    case TYPE_THING:
	s_Owner(obj, player);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);
	move_via_generic(obj, player, NOTHING, 0);
	s_Home(obj, player);
	break;
    case TYPE_PLAYER:
	s_Link(obj, NOTHING);
	if (Robot(obj)) {
	    payfor_force(Next(obj), mudconf.robotcost);
	    pay_quota_force(Next(obj), mudconf.player_quota, TYPE_PLAYER);
	}
	add_player_name(obj, Name(obj));
	s_Owner(obj, Next(obj));
	s_Home(obj, start_home());
	s_Next(obj, NOTHING);
	aregbuff = atr_get(obj,A_AUTOREG,&aowner,&aflags);
	if (*aregbuff && (strlen(aregbuff) > 14)) {
	  pt1 = strstr(aregbuff+7,", Site: ");
	  if (pt1) {
	    *pt1 = '\0';
	    if (areg_check(aregbuff+7,1))
	      areg_add(aregbuff+7,obj);
	  }
	}
	free_lbuf(aregbuff);
	move_object(obj, mudconf.start_room);
	break;
    case TYPE_EXIT:
	s_Link(obj, NOTHING);
	s_Owner(obj, player);
	s_Exits(obj, Location(player));
	s_Next(obj, Exits(Location(player)));
	s_Exits(Location(player), obj);
	break;
    default:
	notify(player, "Unknown object type in recover.");
	return;
    }
    s_Contents(obj, NOTHING);
    Unmark(obj);
    notify(player, "Restored.");
}
#endif

void 
do_reclist(dbref player, dbref cause, int key, char *buff)
{
    dbref i, comp;
    int count, typecomp;
    char buff2[10];
    double timecomp;
#ifndef STANDALONE
    char cstat, *tpr_buff = NULL, *tprp_buff = NULL;
    time_t timevar;
    dbref dummy1;
    int dummy2;
    char *pt1;
#endif

    if (key & REC_COUNT) {
	key &= ~REC_COUNT;
#ifndef STANDALONE
	cstat = 1;
    } else {
	cstat = 0;
#endif
    }
    count = 0;
    typecomp = -1;
    comp = -1;
    timecomp = -1;
    if (key == REC_TYPE) {
	switch (toupper(*buff)) {
	case 'P':
	    typecomp = TYPE_PLAYER;
	    break;
	case 'T':
	    typecomp = TYPE_THING;
	    break;
	case 'R':
	    typecomp = TYPE_ROOM;
	    break;
	case 'E':
	    typecomp = TYPE_EXIT;
	    break;
	default:
	    typecomp = NOTYPE;
	}
    } else if ((key == REC_OWNER) || (key == REC_DEST)) {
	if ((*buff == '#') && (is_number(buff + 1))) {
	    comp = atoi(buff + 1);
	    if (!Good_obj(comp))
		comp = -1;
	} else {
#ifndef STANDALONE
	    comp = lookup_player(player, buff, 0);
#endif
	}
    } else if (key == REC_AGE) {
	if (!is_number(buff)) {
#ifndef STANDALONE
	    notify(player, "Bad number in reclist command.");
#endif
	    timecomp = -1.0;
	} else {
	    timecomp = atof(buff);
	    if ((timecomp < 1.0) || (timecomp > 100.0)) {
#ifndef STANDALONE
		notify(player, "Bad number in reclist command.");
#endif
		timecomp = -1.0;
	    } else
		timecomp *= 86400.0;
	}
    }
    else if ((*buff == '#') && (is_number(buff+1))) {
	comp = atoi(buff+1);
    }
    if (key == REC_FREE) 
      i = mudstate.freelist;
    else
      i = mudstate.recoverlist;
#ifndef STANDALONE
    tprp_buff = tpr_buff = alloc_lbuf("destroy_obj");
#endif
    while (i != NOTHING) {
	if ((typecomp > -1) && (Typeof(i) != typecomp)) {
	    i = Link(i);
	    continue;
	} else if ((key == REC_OWNER) && (comp > -1) && (Next(i) != comp)) {
	    i = Link(i);
	    continue;
	} else if ((key == REC_DEST) && (comp > -1) && (Contents(i) != comp)) {
	    i = Link(i);
	    continue;
	} else if ((comp > -1) && (key != REC_OWNER) && (key != REC_DEST) && (i != comp)) {
	    i = Link(i);
	    continue;
	}
#ifndef STANDALONE
	else if (timecomp > -1.0) {
	    pt1 = atr_get(i, A_RECTIME, &dummy1, &dummy2);
	    timevar = atol(pt1);
	    free_lbuf(pt1);
	    if (difftime(mudstate.now, timevar) <= timecomp) {
		i = Link(i);
		continue;
	    }
	}
#endif
	switch (Typeof(i)) {
	case TYPE_THING:
	    strcpy(buff2, "Thing");
	    break;
	case TYPE_ROOM:
	    strcpy(buff2, "Room");
	    break;
	case TYPE_PLAYER:
	    strcpy(buff2, "Player");
	    break;
	case TYPE_EXIT:
	    strcpy(buff2, "Exit");
	    break;
	default:
	    strcpy(buff, "Unknown");
	}
#ifndef STANDALONE
	if (!cstat) {
	  if (key != REC_FREE) {
            tprp_buff = tpr_buff;
	    notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                        "Dbref: #%-5d  Type: %-6.6s  Name: %-40.40s", 
                                        i, buff2, Name(i)));
            tprp_buff = tpr_buff;
	    notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                        "\tFormer Owner: %-16.16s  Destroyed by: %-16.16s",
                                        Name(Next(i)),Name(Contents(i))));
	  }
	  else {
            tprp_buff = tpr_buff;
	    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Dbref: #%d", i));
	  }
	}
#endif
	count++;
	i = Link(i);
    }
#ifndef STANDALONE
    tprp_buff = tpr_buff;
    notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Total number of items: %d", count));
    free_lbuf(tpr_buff);
    notify(player, "Done.");
#endif
}

void 
do_purge(dbref player, dbref cause, int key, char *buff)
{
    dbref owner, obj, place, dummy1;
    int count;
    char *tname, *mybreak;
    double timecomp;
#ifndef STANDALONE
    dbref d1;
    int dummy2, d2;
    time_t timevar;
    char *pt1;
#endif
    dummy1 = NOTHING;
    timecomp = 0L;
    if (!key) {
	if (*buff != '#') {
#ifndef STANDALONE
	    notify(player, "No dbref specified.");
#endif
	    return;
	}
	tname = buff + 1;
	while (isdigit((int)*tname))
	    tname++;
	if (*tname) {
#ifndef STANDALONE
	    notify(player, "Bad dbref specified.");
#endif
	    return;
	}
	obj = atoi(buff + 1);
	if ((obj < 0) || (obj > mudstate.db_top)) {
#ifndef STANDALONE
	    notify(player, "Bad dbref specified.");
#endif
	    return;
	}
	owner = mudstate.recoverlist;
	place = NOTHING;
	while ((owner != NOTHING) && (owner != obj)) {
	    place = owner;
	    owner = Link(owner);
	}
	if (owner != obj) {
#ifndef STANDALONE
	    notify(player, "Dbref not found in recover list.");
#endif
	    return;
	}
	if (place == NOTHING) {
	    mudstate.recoverlist = Link(obj);
	} else {
	    s_Link(place, Link(obj));
	}
	count = 1;
	atr_free(obj);
	s_Name(obj, NULL);
	s_Flags3(obj, 0);
	s_Flags4(obj, 0);
	s_Toggles(obj, 0);
	s_Toggles2(obj, 0);
	s_Toggles3(obj, 0);
	s_Toggles4(obj, 0);
	s_Location(obj, NOTHING);
	s_Contents(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);
	s_Owner(obj, GOD);
	s_Pennies(obj, 0);
	s_Flags(obj, (TYPE_THING | GOING));
	s_Flags2(obj, 0);
	s_Exits(obj, NOTHING);
    } else {
        if (key == PURGE_TIMEOWNER) {
            if ( (mybreak = strchr(buff, '/')) == NULL ) {
#ifndef STANDALONE
               notify(player, "Bad syntax with @purge/timeowner.  Requires: @purge/towner <time>/<player>");
#endif
               return;
            }
            timecomp = atof(buff);
            timecomp = atof(buff);
	    if ((timecomp < 1.0) || (timecomp > 100.0)) {
#ifndef STANDALONE
		notify(player, "Bad number in purge command.");
#endif
		return;
	    }
	    timecomp *= 86400.0;
	    if ((*(mybreak+1) == '#') && (is_number(mybreak + 2))) {
		dummy1 = atoi(mybreak + 2);
		if (!Good_obj(dummy1))
		    dummy1 = -1;
	    } else {
#ifndef STANDALONE
		dummy1 = lookup_player(player, mybreak+1, 0);
#endif
	    }
	    if (dummy1 == -1) {
#ifndef STANDALONE
		notify(player, "Bad reference in purge command.");
#endif
		return;
            }   
        } else if (key == PURGE_TIMETYPE) {
           if ( (mybreak = strchr(buff, '/')) == NULL ) {
#ifndef STANDALONE
              notify(player, "Bad syntax with @purge/timetype.  Requires: @purge/ttype <time>/<type>");
#endif
              return;
           }
           timecomp = atof(buff);
	   if ((timecomp < 1.0) || (timecomp > 100.0)) {
#ifndef STANDALONE
		notify(player, "Bad number in purge command.");
#endif
		return;
	   }
	   timecomp *= 86400.0;
           switch(toupper(*(mybreak+1))) {
#ifndef STANDALONE
	    case 'T':
		dummy2 = TYPE_THING;
		break;
	    case 'R':
		dummy2 = TYPE_ROOM;
		break;
	    case 'E':
		dummy2 = TYPE_EXIT;
		break;
	    case 'P':
		dummy2 = TYPE_PLAYER;
		break;
#endif
	    default:
#ifndef STANDALONE
		notify(player, "Bad type in purge command.");
#endif
		return;
           }
        } else if (key == PURGE_TIME) {
	    if (!is_number(buff)) {
#ifndef STANDALONE
		notify(player, "Bad number in purge command.");
#endif
		return;
	    }
	    timecomp = atof(buff);
	    if ((timecomp < 1.0) || (timecomp > 100.0)) {
#ifndef STANDALONE
		notify(player, "Bad number in purge command.");
#endif
		return;
	    }
	    timecomp *= 86400.0;
	} else if (key == PURGE_TYPE) {
	    switch (toupper(*buff)) {
#ifndef STANDALONE
	    case 'T':
		dummy2 = TYPE_THING;
		break;
	    case 'R':
		dummy2 = TYPE_ROOM;
		break;
	    case 'E':
		dummy2 = TYPE_EXIT;
		break;
	    case 'P':
		dummy2 = TYPE_PLAYER;
		break;
#endif
	    default:
#ifndef STANDALONE
		notify(player, "Bad type in purge command.");
#endif
		return;
	    }
	} else if (key == PURGE_OWNER) {
	    if ((*buff == '#') && (is_number(buff + 1))) {
		dummy1 = atoi(buff + 1);
		if (!Good_obj(dummy1))
		    dummy1 = -1;
	    } else {
#ifndef STANDALONE
		dummy1 = lookup_player(player, buff, 0);
#endif
	    }
	    if (dummy1 == -1) {
#ifndef STANDALONE
		notify(player, "Bad reference in purge command.");
#endif
		return;
	    }
	}
	obj = mudstate.recoverlist;
	place = NOTHING;
	count = 0;
	while (obj != NOTHING) {
#ifndef STANDALONE
            if (key == PURGE_TIMEOWNER) {
		pt1 = atr_get(obj, A_RECTIME, &d1, &d2);
		timevar = atol(pt1);
		free_lbuf(pt1);
		if (Next(obj) != dummy1) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
		if (difftime(mudstate.now, timevar) <= timecomp) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
            } else if (key == PURGE_TIMETYPE) {
		pt1 = atr_get(obj, A_RECTIME, &d1, &d2);
		timevar = atol(pt1);
		free_lbuf(pt1);
		if (Typeof(obj) != dummy2) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
		if (difftime(mudstate.now, timevar) <= timecomp) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
	    } else if (key == PURGE_TIME) {
		pt1 = atr_get(obj, A_RECTIME, &dummy1, &dummy2);
		timevar = atol(pt1);
		free_lbuf(pt1);
		if (difftime(mudstate.now, timevar) <= timecomp) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
	    } else if (key == PURGE_TYPE) {
		if (Typeof(obj) != dummy2) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
	    } else if (key == PURGE_OWNER) {
		if (Next(obj) != dummy1) {
		    place = obj;
		    obj = Link(obj);
		    continue;
		}
	    }
#endif
	    count++;
	    if (place == NOTHING)
		mudstate.recoverlist = Link(obj);
	    else
		s_Link(place, Link(obj));
	    atr_free(obj);
	    s_Name(obj, NULL);
	    s_Flags3(obj, 0);
	    s_Flags4(obj, 0);
	    s_Toggles(obj, 0);
	    s_Toggles2(obj, 0);
	    s_Toggles3(obj, 0);
	    s_Toggles4(obj, 0);
	    s_Location(obj, NOTHING);
	    s_Contents(obj, NOTHING);
	    s_Next(obj, NOTHING);
	    s_Owner(obj, GOD);
	    s_Pennies(obj, 0);
	    s_Flags(obj, (TYPE_THING | GOING));
	    s_Flags2(obj, 0);
	    s_Exits(obj, NOTHING);
	    owner = Link(obj);
	    s_Link(obj, NOTHING);
	    obj = owner;
	}
    }
#ifndef STANDALONE
    notify(player, unsafe_tprintf("%d Object(s) Purged.", count));
#endif
    return;
}

/* ---------------------------------------------------------------------------
 * make_freelist: Build a freelist
 */

static void 
NDECL(make_freelist)
{
    dbref i;

    mudstate.freelist = NOTHING;
    mudstate.recoverlist = NOTHING;
    DO_WHOLE_DB(i) {
	if (Going(i) && Recover(i)) {
	    s_Flags(i, Flags(i) & ~GOING);
	}
	if (Recover(i) && !Byeroom(i)) {
	    s_Link(i, mudstate.recoverlist);
	    mudstate.recoverlist = i;
	}
	else if (Going(i)) {			/* was  IS_CLEAN */
	    s_Link(i, mudstate.freelist);
	    mudstate.freelist = i;
	}
    }
}

/* ---------------------------------------------------------------------------
 * divest_object: Get rid of KEY contents of object.
 */

void 
divest_object(dbref thing)
{
    dbref curr, temp;

    SAFE_DOLIST(curr, temp, Contents(thing)) {
	if (!Controls(thing, curr) &&
	    Has_location(curr) && Key(curr)) {
	    move_via_generic(curr, HOME, NOTHING, 0);
	}
    }
}

/* ---------------------------------------------------------------------------
 * empty_obj, purge_going: Get rid of GOING objects in the db.
 */

void 
empty_obj(dbref obj)
{
    dbref targ, next;

    /* Send the contents home */

    SAFE_DOLIST(targ, next, Contents(obj)) {
	if (!Has_location(targ)) {
	    Log_simple_err(targ, obj,
			   "Funny object type in contents list of GOING location. Flush terminated.");
	    break;
	} else if (Location(targ) != obj) {
	    Log_header_err(targ, obj, Location(targ), 1,
			   "Location",
			   "indicates object really in another location during cleanup of GOING location.  Flush terminated.");
	    break;
	} else {
	    ZAP_LOC(targ);
	    if (Home(targ) == obj) {
		s_Home(targ, new_home(targ));
	    }
	    move_via_generic(targ, HOME, NOTHING, 0);
	    divest_object(targ);
	}
    }

    /* Destroy the exits */

    SAFE_DOLIST(targ, next, Exits(obj)) {
	if (!isExit(targ)) {
	    Log_simple_err(targ, obj,
			   "Funny object type in exit list of GOING location. Flush terminated.");
	    break;
	} else if (Exits(targ) != obj) {
	    Log_header_err(targ, obj, Exits(targ), 1,
			   "Location",
			   "indicates exit really in another location during cleanup of GOING location.  Flush terminated.");
	    break;
	} else {
	    destroy_obj(NOTHING, targ, 0);
	}
    }
}

static void 
NDECL(purge_going)
{
    dbref i;

    DO_WHOLE_DB(i) {
	if (!Byeroom(i))
	    continue;

	switch (Typeof(i)) {
	case TYPE_ROOM:

	    /* Room scheduled for destruction... do it */

	    empty_obj(i);
	    if (Dr_Purge(i))
	      destroy_obj(NOTHING, i, 1);
	    else
	      destroy_obj(NOTHING, i, 0);
	    cache_reset(0);
	    break;
	default:

	    break;
	}
    }
}

/* ---------------------------------------------------------------------------
 * check_dead_refs: Look for references to GOING or illegal objects.
 */

static void 
check_pennies(dbref thing, int limit, const char *qual)
{
    int j;

    if (Going(thing))
	return;
    if (Recover(thing))
	return;
    j = Pennies(thing);
    if (isRoom(thing) || isExit(thing)) {
	if (j) {
	    Log_header_err(thing, NOTHING, j, 0,
			   qual, "is strange.  Reset.");
	    s_Pennies(thing, 0);
	}
    } else if (j == 0) {
	Log_header_err(thing, NOTHING, j, 0, qual, "is zero.");
    } else if (j < 0) {
	Log_header_err(thing, NOTHING, j, 0, qual, "is negative.");
    } else if (j > limit) {
	Log_header_err(thing, NOTHING, j, 0, qual, "is excessive.");
    }
}

static 
NDECL(void check_dead_refs)
{
    dbref targ, owner, i, j, loc;
    int aflags, dirty;
    char *str;
    FWDLIST *fp;

#ifndef STANDALONE
    char *tpr_buff = NULL, *tprp_buff = NULL;
    tprp_buff = tpr_buff = alloc_lbuf("check_dead_refs");
#endif
    DO_WHOLE_DB(i) {

	/* Check the parent */

	targ = Parent(i);
	if (Good_obj(targ)) {
	    if (Going(targ) || Recover(targ)) {
		s_Parent(i, NOTHING);
#ifndef STANDALONE
		owner = Owner(i);
		if (Good_owner(owner) &&
		    !Quiet(i) && !Quiet(owner)) {
                    tprp_buff = tpr_buff;
		    notify(owner,
			   safe_tprintf(tpr_buff, &tprp_buff, "Parent cleared on %s(#%d)",
				   Name(i), i));
		}
#else
		Log_header_err(i, Location(i), targ, 1,
			       "Parent", "is invalid.  Cleared.");
#endif
	    }
	} else if (targ != NOTHING) {
	    Log_header_err(i, Location(i), targ, 1,
			   "Parent", "is invalid.  Cleared.");
	    s_Parent(i, NOTHING);
	}
	switch (Typeof(i)) {
	case TYPE_PLAYER:
	case TYPE_THING:

	    if (Going(i) || Recover(i))
		break;

	    /* Check the home */

	    targ = Home(i);
	    if (Good_obj(targ)) {
		if (Going(targ) || Recover(targ)) {
		    s_Home(i, new_home(i));
#ifndef STANDALONE
		    owner = Owner(i);
		    if (Good_owner(owner) &&
			!Quiet(i) && !Quiet(owner)) {
                        tprp_buff = tpr_buff;
			notify(owner,
			       safe_tprintf(tpr_buff, &tprp_buff, "Home reset on %s(#%d)", Name(i), i));
		    }
#else
		    Log_header_err(i, Location(i), targ, 1,
				   "Home",
				   "is invalid.  Reset.");
#endif
		}
	    } else if (targ != NOTHING) {
		Log_header_err(i, Location(i), targ, 1,
			       "Home", "is invalid.  Cleared.");
		s_Home(i, new_home(i));
	    }
	    /* Check the location */

	    targ = Location(i);
	    if (!Good_obj(targ)) {
		Log_pointer_err(NOTHING, i, NOTHING, targ,
				"Location",
				"is invalid.  Moved to home.");
		ZAP_LOC(i);
		move_object(i, HOME);
	    }
	    /* Check for self-referential Next() */

	    if (Next(i) == i) {
		Log_simple_err(i, NOTHING,
			       "Next points to self.  Next cleared.");
		s_Next(i, NOTHING);
	    }
	    if (check_type & DBCK_FULL) {

		/* Check wealth or value */

		targ = OBJECT_ENDOWMENT(mudconf.createmax);
		if (OwnsOthers(i)) {
		    targ += mudconf.paylimit;
		    check_pennies(i, targ, "Wealth");
		} else {
		    check_pennies(i, targ, "Value");
		}
	    }
	    break;
	case TYPE_ROOM:

	    /* Check the dropto */

	    targ = Dropto(i);
	    if (Good_obj(targ)) {
		if (Going(targ) || Recover(targ)) {
		    s_Dropto(i, NOTHING);
#ifndef STANDALONE
		    owner = Owner(i);
		    if (Good_owner(owner) &&
			!Quiet(i) && !Quiet(owner)) {
                        tprp_buff = tpr_buff;
			notify(owner,
			       safe_tprintf(tpr_buff, &tprp_buff, "Dropto removed from %s(#%d)",
				       Name(i), i));
		    }
#else
		    Log_header_err(i, NOTHING, targ, 1,
				   "Dropto",
				   "is invalid.  Removed.");
#endif
		}
	    } else if ((targ != NOTHING) && (targ != HOME)) {
		Log_header_err(i, NOTHING, targ, 1,
			       "Dropto", "is invalid.  Cleared.");
		s_Dropto(i, NOTHING);
	    }
	    if (check_type & DBCK_FULL) {

		/* NEXT should be null */

		if (Next(i) != NOTHING) {
		    Log_header_err(i, NOTHING, Next(i), 1,
				   "Next pointer",
				   "should be NOTHING.  Reset.");
		    s_Next(i, NOTHING);
		}
		/* LINK should be null */

		if (Link(i) != NOTHING) {
		    Log_header_err(i, NOTHING, Link(i), 1,
				   "Link pointer ",
				   "should be NOTHING.  Reset.");
		    s_Link(i, NOTHING);
		}
		/* Check value */

		check_pennies(i, 1, "Value");
	    }
	    break;
	case TYPE_EXIT:

	    /* If it points to something GOING, set it going */

	    targ = Location(i);
	    if (!Going(i) && !Recover(i)) {
	      if (Good_obj(targ)) {
		if (Going(targ) || Recover(targ)) {
		  loc = Exits(i);
		  s_Exits(loc,remove_first(Exits(loc),i));
		  destroy_obj(NOTHING,i,0);
		}
	      } else if (targ == HOME) {
		/* null case, HOME is always valid */
	      } else if (targ != NOTHING) {
		Log_header_err(i, Exits(i), targ, 1,
			       "Destination",
			       "is invalid.  Exit destroyed.");
		loc = Exits(i);
		s_Exits(loc,remove_first(Exits(loc),i));
		destroy_obj(NOTHING,i,0);
	      } else {
		if (!Has_contents(targ)) {
		    Log_header_err(i, Exits(i), targ, 1,
				   "Destination",
				   "is not a valid type.  Exit destroyed.");
		  loc = Exits(i);
		  s_Exits(loc,remove_first(Exits(loc),i));
		  destroy_obj(NOTHING,i,0);
		}
	      }
	    }

	    /* Check for self-referential Next() */

	    if (Next(i) == i) {
		Log_simple_err(i, NOTHING,
			       "Next points to self.  Next cleared.");
		s_Next(i, NOTHING);
	    }
	    if (check_type & DBCK_FULL) {

		/* CONTENTS should be null */

		if (Contents(i) != NOTHING) {
		    Log_header_err(i, Exits(i),
				   Contents(i), 1, "Contents",
				   "should be NOTHING.  Reset.");
		    s_Contents(i, NOTHING);
		}
		/* LINK should be null */

		if (Link(i) != NOTHING) {
		    Log_header_err(i, Exits(i), Link(i), 1,
				   "Link",
				   "should be NOTHING.  Reset.");
		    s_Link(i, NOTHING);
		}
		/* Check value */

		check_pennies(i, 1, "Value");
	    }
	    break;
	default:

	    /* Funny object type, destroy it */

	    Log_simple_err(i, NOTHING,
			   "Funny object type.  Destroyed.");
	    destroy_obj(NOTHING, i, 0);
	}

	/* Check forwardlist */

	dirty = 0;
	fp = fwdlist_get(i);
	if (fp) {
	    for (j = 0; j < fp->count; j++) {
		targ = fp->data[j];
		if (Good_obj(targ) && (Going(targ) || Recover(targ))) {
		    fp->data[j] = NOTHING;
		    dirty = 1;
		} else if (!Good_obj(targ) &&
			   (targ != NOTHING)) {
		    fp->data[j] = NOTHING;
		    dirty = 1;
		}
	    }
	}
	if (dirty) {
	    str = alloc_lbuf("purge_going");
	    (void) fwdlist_rewrite(fp, str);
	    atr_get_info(i, A_FORWARDLIST, &owner, &aflags);
	    atr_add(i, A_FORWARDLIST, str, owner, aflags);
	    free_lbuf(str);
	}
	/* Check owner */

	owner = Owner(i);
	if (!Good_obj(owner)) {
	    Log_header_err(i, NOTHING, owner, 1,
			   "Owner", "is invalid.  Set to GOD.");
	    owner = GOD;
	    s_Owner(i, owner);
#ifndef STANDALONE
	    halt_que(NOTHING, i);
#endif
	    s_Halted(i);
	} else if (check_type & DBCK_FULL) {
	    if (Going(owner) || Recover(owner)) {
		Log_header_err(i, NOTHING, owner, 1,
			       "Owner", "is set GOING.  Set to GOD.");
		s_Owner(i, owner);
#ifndef STANDALONE
		halt_que(NOTHING, i);
#endif
		s_Halted(i);
	    } else if (!OwnsOthers(owner)) {
		Log_header_err(i, NOTHING, owner, 1,
			       "Owner", "is not a valid owner type.");
	    } else if (isPlayer(i) && (owner != i)) {
		Log_header_err(i, NOTHING, owner, 1,
			   "Player", "is the owner instead of the player.");
	    }
	}
	if (check_type & DBCK_FULL) {

	    /* Check for wizards */

	    if (Wizard(i)) {
		if (isPlayer(i)) {
		    Log_simple_err(i, NOTHING,
				   "Player is a WIZARD.");
		}
		if (!Wizard(Owner(i))) {
		    Log_header_err(i, NOTHING, Owner(i), 1,
				   "Owner",
				   "of a WIZARD object is not a wizard");
		}
	    }
	}
    }
#ifndef STANDALONE
    free_lbuf(tpr_buff);
#endif
}

/* ---------------------------------------------------------------------------
 * check_loc_exits, check_exit_chains: Validate the exits chains
 * of objects and attempt to correct problems.  The following errors are
 * found and corrected:
 *      Location not in database                        - skip it.
 *      Location GOING                                  - skip it.
 *      Location not a PLAYER, ROOM, or THING           - skip it.
 *      Location already visited                        - skip it.
 *      Exit/next pointer not in database               - NULL it.
 *      Member is not an EXIT                           - terminate chain.
 *      Member is GOING                                 - destroy exit.
 *      Member already checked (is in another list)     - terminate chain.
 *      Member in another chain (recursive check)       - terminate chain.
 *      Location of member is not specified location    - reset it.
 */

static void 
check_loc_exits(dbref loc)
{
    dbref exit, back, temp, exitloc, dest;

    if (!Good_obj(loc))
	return;

    /* Only check players, rooms, and things that aren't GOING */

    if (isExit(loc) || Going(loc) || Recover(loc))
	return;

    /* If marked, we've checked here already */

    if (Marked(loc))
	return;
    Mark(loc);

    /* Check all the exits */

    back = NOTHING;
    exit = Exits(loc);
    while (exit != NOTHING) {

	exitloc = NOTHING;
	dest = NOTHING;

	if (Good_obj(exit)) {
	    exitloc = Exits(exit);
	    dest = Location(exit);
	}
	if (!Good_obj(exit)) {

	    /* A bad pointer - terminate chain */

	    Log_pointer_err(back, loc, NOTHING, exit, "Exit list",
			    "is invalid.  List nulled.");
	    if (back != NOTHING) {
		s_Next(back, NOTHING);
	    } else {
		s_Exits(loc, NOTHING);
	    }
	    exit = NOTHING;
	} else if (!isExit(exit)) {

	    /* Not an exit - terminate chain */

	    Log_pointer_err(back, loc, NOTHING, exit,
			    "Exitlist member",
			    "is not an exit.  List terminated.");
	    if (back != NOTHING) {
		s_Next(back, NOTHING);
	    } else {
		s_Exits(loc, NOTHING);
	    }
	    exit = NOTHING;
	} else if (Going(exit)) {

	    /* Going - silently filter out */

	    temp = Next(exit);
	    if (back != NOTHING) {
		s_Next(back, temp);
	    } else {
		s_Exits(loc, temp);
	    }
	    destroy_obj(NOTHING, exit, 0);
	    exit = temp;
	    continue;
	} else if (Marked(exit)) {

	    /* Already in another list - terminate chain */

	    Log_pointer_err(back, loc, NOTHING, exit,
			    "Exitlist member",
			    "is in another exitlist.  Cleared.");
	    if (back != NOTHING) {
		s_Next(back, NOTHING);
	    } else {
		s_Exits(loc, NOTHING);
	    }
	    exit = NOTHING;
	} else if (!Good_obj(dest) && (dest != HOME) &&
		   (dest != NOTHING)) {

	    /* Destination is not in the db.  Null it. */

	    Log_pointer_err(back, loc, NOTHING, exit,
			    "Destination", "is invalid.  Cleared.");
	    s_Location(exit, NOTHING);

	} else if (exitloc != loc) {

	    /* Exit thinks it's in another place.  Check the
	     * exitlist there and see if it contains this exit.
	     * If it does, then our exitlist somehow pointed
	     * into the middle of their exitlist.  If not,
	     * assume we own the exit.
	     */

	    check_loc_exits(exitloc);
	    if (Marked(exit)) {

		/* It's in the other list, give it up */

		Log_pointer_err(back, loc, NOTHING, exit, "",
				"is in another exitlist.  List terminated.");
		if (back != NOTHING) {
		    s_Next(back, NOTHING);
		} else {
		    s_Exits(loc, NOTHING);
		}
		exit = NOTHING;
	    } else {

		/* Not in the other list, assume in ours */

		Log_header_err(exit, loc, exitloc, 1,
			       "Not on chain for location",
			       "Reset.");
		s_Exits(exit, loc);
	    }
	}
	if (exit != NOTHING) {

	    /* All OK (or all was made OK) */

	    if (check_type & DBCK_FULL) {

		/* Make sure exit owner owns at least one of
		 * the source or destination.  Just warn if
		 * he doesn't.
		 */

		temp = Owner(exit);
		if ((temp != Owner(loc)) &&
		    (temp != Owner(Location(exit)))) {
		    Log_header_err(exit, loc, temp, 1,
				   "Owner",
			  "does not own either the source or destination.");
		}
	    }
	    Mark(exit);
	    back = exit;
	    exit = Next(exit);
	}
    }
    return;
}

static void 
NDECL(check_exit_chains)
{
    dbref i;

    Unmark_all(i);
    DO_WHOLE_DB(i)
	check_loc_exits(i);
    DO_WHOLE_DB(i) {
	if (isExit(i) && !Marked(i) && !Recover(i)) {
	    Log_simple_err(i, NOTHING,
			   "Disconnected exit.  Destroyed.");
	    destroy_obj(NOTHING, i, 0);
	}
    }
}

/* ---------------------------------------------------------------------------
 * check_misplaced_obj, check_loc_contents, check_contents_chains: Validate
 * the contents chains of objects and attempt to correct problems.  The
 * following errors are found and corrected:
 *      Location not in database                        - skip it.
 *      Location GOING                                  - skip it.
 *      Location not a PLAYER, ROOM, or THING           - skip it.
 *      Location already visited                        - skip it.
 *      Contents/next pointer not in database           - NULL it.
 *      Member is not an PLAYER or THING                - terminate chain.
 *      Member is GOING                                 - destroy exit.
 *      Member already checked (is in another list)     - terminate chain.
 *      Member in another chain (recursive check)       - terminate chain.
 *      Location of member is not specified location    - reset it.
 */

static void FDECL(check_loc_contents, (dbref));

static void 
check_misplaced_obj(dbref * obj, dbref back, dbref loc)
{
    /* Object thinks it's in another place.  Check the contents list there
     * and see if it contains this object.  If it does, then our contents
     * list somehow pointed into the middle of their contents list and
     * we should truncate our list. If not, assume we own the object.
     */

    if (!Good_obj(*obj))
	return;
    loc = Location(*obj);
    Unmark(*obj);
    if (Good_obj(loc)) {
	check_loc_contents(loc);
    }
    if (Marked(*obj)) {

	/* It's in the other list, give it up */

	Log_pointer_err(back, loc, NOTHING, *obj, "",
			"is in another contents list.  Cleared.");
	if (back != NOTHING) {
	    s_Next(back, NOTHING);
	} else {
	    s_Contents(loc, NOTHING);
	}
	*obj = NOTHING;
    } else {
	/* Not in the other list, assume in ours */

	Log_header_err(*obj, loc, Contents(*obj), 1,
		       "Location", "is invalid.  Reset.");
	s_Contents(*obj, loc);
    }
    return;
}

static void 
check_loc_contents(dbref loc)
{
    dbref obj, back, temp;

    if (!Good_obj(loc))
	return;

    /* Only check players, rooms, and things that aren't GOING */

    if (isExit(loc) || Going(loc) || Recover(loc))
	return;

    /* Check all the exits */

    back = NOTHING;
    obj = Contents(loc);
    while (obj != NOTHING) {
	if (!Good_obj(obj)) {

	    /* A bad pointer - terminate chain */

	    Log_pointer_err(back, loc, NOTHING, obj,
			    "Contents list", "is invalid.  Cleared.");
	    if (back != NOTHING) {
		s_Next(back, NOTHING);
	    } else {
		s_Contents(loc, NOTHING);
	    }
	    obj = NOTHING;
	} else if (!Has_location(obj)) {

	    /* Not a player or thing - terminate chain */

	    Log_pointer_err(back, loc, NOTHING, obj, "",
			    "is not a player or thing.  Cleared.");
	    if (back != NOTHING) {
		s_Next(back, NOTHING);
	    } else {
		s_Contents(loc, NOTHING);
	    }
	    obj = NOTHING;
	} else if (Going(obj) || Recover(obj)) {

	    /* Going - silently filter out */

	    temp = Next(obj);
	    if (back != NOTHING) {
		s_Next(back, temp);
	    } else {
		s_Contents(loc, temp);
	    }
	    destroy_obj(NOTHING, obj, 0);
	    obj = temp;
	    continue;
	} else if (Marked(obj)) {

	    /* Already visited - either truncate or ignore */

	    if (Location(obj) != loc) {

		/* Location wrong - either truncate or fix */

		check_misplaced_obj(&obj, back, loc);
	    } else {

		/* Location right - recursive contents */
	    }
	} else if (Location(obj) != loc) {

	    /* Location wrong - either truncate or fix */

	    check_misplaced_obj(&obj, back, loc);
	}
	if (obj != NOTHING) {

	    /* All OK (or all was made OK) */

	    if (check_type & DBCK_FULL) {

		/* Check for wizard command-handlers inside
		 * nonwiz. Just warn if we find one.
		 */

		if (Wizard(obj) && !Wizard(loc)) {
		    if (Commer(obj)) {
			Log_simple_err(obj, loc,
			"Wizard command handling object inside nonwizard.");
		    }
		}
		/* Check for nonwizard objects inside wizard
		 * objects.
		 */

		if (Wizard(loc) &&
		    !Wizard(obj) && !Wizard(Owner(obj))) {
		    Log_simple_err(obj, loc,
				   "Nonwizard object inside wizard.");
		}
	    }
	    Mark(obj);
	    back = obj;
	    obj = Next(obj);
	}
    }
    return;
}

static void 
NDECL(check_contents_chains)
{
    dbref i;

    Unmark_all(i);
    DO_WHOLE_DB(i)
	check_loc_contents(i);
    DO_WHOLE_DB(i)
	if (!Going(i) && !Recover(i) && !Marked(i) && Has_location(i)) {
	Log_simple_err(i, Location(i),
		       "Orphaned object, moved home.");
	ZAP_LOC(i);
	move_via_generic(i, HOME, NOTHING, 0);
    }
}

/* ---------------------------------------------------------------------------
 * mark_place, check_floating: Look for floating rooms not set FLOATING.
 */

void 
mark_place(dbref loc)
{
    dbref exit;

    /* If already marked, exit.  Otherwise set marked. */

    if (!Good_obj(loc))
	return;
    if (Marked(loc))
	return;
    Mark(loc);

    /* Visit all places you can get to via exits from here. */

    for (exit = Exits(loc); exit != NOTHING; exit = Next(exit)) {
	if (Good_obj(Location(exit)))
	    mark_place(Location(exit));
    }
}

static 
NDECL(void check_floating)
{
    dbref owner, i;
#ifndef STANDALONE
    char *tpr_buff = NULL, *tprp_buff = NULL;
    tprp_buff = tpr_buff = alloc_lbuf("check_floating");
#endif

    /* Mark everyplace you can get to via exits from the starting room */

    Unmark_all(i);
    mark_place(mudconf.start_room);

    /* Look for rooms not marked and not set FLOATING */

    DO_WHOLE_DB(i) {
	if (!Going(i) && !Recover(i)) {
	  owner = Owner(i);
	  if (isRoom(i) 
	      && 
#ifndef STANDALONE
	      !(Floating(i) || ((Good_obj(owner) && Floating(owner)) || (mudconf.exits_conn_rooms == 1 && Exits(i))))
#else
	      !(Floating(i) || (Good_obj(owner) && Floating(owner)))
#endif
	      && !Marked(i)) {
#ifndef STANDALONE
	    if (Good_owner(owner)) {
                tprp_buff = tpr_buff;
		notify(owner, safe_tprintf(tpr_buff, &tprp_buff,
					 "You own a floating room: %s(#%d)",
					 Name(i), i));
	    }
#else
	    Log_simple_err(i, NOTHING, "Disconnected room.");
#endif
	  }
	  if ((isExit(i) || isThing(i)) && (Toggles(owner) & TOG_NOTIFY_LINK) && !(Toggles(i) & TOG_NOTIFY_LINK)) {
	    if (isExit(i) && !Good_obj(Location(i))) {
#ifndef STANDALONE
	      if (Good_owner(owner)) {
                tprp_buff = tpr_buff;
		notify(owner, safe_tprintf(tpr_buff, &tprp_buff, 
					 "You own an unlinked exit: %s(#%d)",
					 Name(i), i));
	      }
#else
	      Log_simple_err(i, NOTHING, "Unlinked exit.");
#endif
	    }
	    if (isThing(i) && !Good_obj(Link(i))) {
#ifndef STANDALONE
	      if (Good_owner(owner)) {
                tprp_buff = tpr_buff;
		notify(owner, safe_tprintf(tpr_buff, &tprp_buff,
					 "You own an unlinked object: %s(#%d)",
					 Name(i), i));
	      }
#else
	      Log_simple_err(i, NOTHING, "Unlinked object.");
#endif
	    }
	  }
	}
    }
#ifndef STANDALONE
    free_lbuf(tpr_buff);
#endif
}

/* ---------------------------------------------------------------------------
 * do_dbck: Perform a database consistency check and clean up damage.
 */

void 
do_dbck(dbref player, dbref cause, int key)
{
#ifndef STANDALONE
    if (mudstate.remotep != NOTHING) {
       notify(player, "Can not dbck remotely.");
       return;
    }
#endif
    check_type = key;
    make_freelist();
    purge_going();
    check_dead_refs();
    check_exit_chains();
    check_contents_chains();
    check_floating();
#ifndef STANDALONE
    if (player != NOTHING) {
	alarm_msec(next_timer());
	if (!Quiet(player))
	    notify(player, "Done.");
    }
    local_dbck();
#endif
}
