/* match.c -- Routines for parsing arguments */

#include "autoconf.h"
#include "copyright.h"

#include "externs.h"
#include "match.h"
#include "attrs.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */

static dbref exact_match = NOTHING;	/* holds result of exact match */
static int have_exact = 0;	/* Have a precise match (dbref, 'me', etc) */
static int check_keys = 0;	/* if non-zero, check for keys */
static dbref last_match = NOTHING;	/* holds result of last match */
static int match_count = 0;	/* holds total number of inexact matches */
static dbref match_who = NOTHING;	/* player performing match */
static const char *match_name = "";	/* name to match */
static int preferred_type = NOTYPE;	/* preferred type */
static int local_match = 0;	/* Matched something locally, not by number */
static int reality_valuechk = 0;        /* Reality level check */

/*
 * This function removes repeated spaces from the template to which object
 * names are being matched.  It also removes inital and terminal spaces.
 */

static char *
munge_space_for_match(name)
    char *name;
{
    static char buffer[LBUF_SIZE];
    char *p, *q;

    p = name;
    q = buffer;
    while (isspace((int)*p))
	p++;			/* remove inital spaces */
    while (*p) {
	while (*p && !isspace((int)*p))
	    *q++ = *p++;
	while (*p && isspace((int)*++p));
	if (*p)
	    *q++ = ' ';
    }
    *q = '\0';			/* remove terminal spaces and terminate
				   * string */
    return (buffer);
}

/*
 * check a list for objects that are prefix's for string, && are controlled
 * || link_ok
 */

dbref
pref_match(player, list, string)
    dbref player, list;
    const char *string;
{
    dbref lmatch;
    int mlen;

    lmatch = NOTHING;
    mlen = 0;
    while (list != NOTHING) {
	if (string_prefix(string, Name(list)) &&
	    Puppet(list) &&
	    controls(player, list)) {
	    if (strlen(Name(list)) > mlen) {
		lmatch = list;
		mlen = strlen(Name(list));
	    }
	}
	list = Next(list);
    }
    return (lmatch);
}

void
init_match(player, name, type)
    dbref player;
    const char *name;
    int type;
{
    reality_valuechk = 0;
    exact_match = last_match = NOTHING;
    have_exact = 0;
    match_count = 0;
    match_who = player;
    match_name = munge_space_for_match((char *) name);
    check_keys = 0;
    preferred_type = type;
    local_match = 0;
}

void
init_match_real(dbref player, const char *name, int type, int real_bitvalue)
{
    reality_valuechk = real_bitvalue;
    exact_match = last_match = NOTHING;
    have_exact = 0;
    match_count = 0;
    match_who = player;
    match_name = munge_space_for_match((char *) name);
    check_keys = 0;
    preferred_type = type;
    local_match = 0;
}

void
init_match_check_keys(player, name, type)
    dbref player;
    const char *name;
    int type;
{
    init_match(player, name, type);
    check_keys = 1;
}

static dbref
choose_thing(thing1, thing2)
    dbref thing1, thing2;
{
    int has1;
    int has2;

    if (thing2 != NOTHING) {
	if (!mudstate.whisper_state && ((Cloak(thing2) && !Wizard(match_who)) || (Cloak(thing2) && SCloak(thing2) && !Immortal(match_who)))) {
	  if (!mudstate.exitcheck)
	    return NOTHING;
	}
    }
    if (thing1 == NOTHING) {
	return thing2;
    } else if (thing2 == NOTHING) {
	return thing1;
    }
    if (preferred_type != NOTYPE) {
	if (Typeof(thing1) == preferred_type) {
	    if (Typeof(thing2) != preferred_type) {
		return thing1;
	    }
	} else if (Typeof(thing2) == preferred_type) {
	    return thing2;
	}
    }
    if (check_keys) {
	has1 = could_doit(match_who, thing1, A_LOCK,1,0);
	has2 = could_doit(match_who, thing2, A_LOCK,1,0);

	if (has1 && !has2) {
	    return thing1;
	} else if (has2 && !has1) {
	    return thing2;
	}
	/* else fall through */
    }
    return (random() % 2 ? thing1 : thing2);
}

void
NDECL(match_player)
{
    dbref match;
    char *p;

    if (have_exact)
	return;
    if (*match_name == LOOKUP_TOKEN) {
	for (p = (char *) match_name + 1; isspace((int)*p); p++);
	if ((match = lookup_player(NOTHING, p, 1)) != NOTHING) {
	    exact_match = match;
	    have_exact = 1;
	    local_match = 0;
	}
    }
}
/* returns nnn if name = #nnn, else NOTHING */

static dbref
absolute_name(need_pound)
    int need_pound;
{
    dbref match;
    char *mname;

    mname = (char *) match_name;
    if (need_pound) {
	if (*match_name != NUMBER_TOKEN) {
	    return NOTHING;
	} else {
	    mname++;
	}
    }
    if (*mname) {
	match = parse_dbref(mname);
	if (Good_obj(match)) {
	    return match;
	}
    }
    return NOTHING;
}

void
NDECL(match_absolute)
{
    dbref match;

    if (have_exact)
	return;
    match = absolute_name(1);
    if (Good_obj(match)) {
	exact_match = match;
	have_exact = 1;
	local_match = 0;
    }
}

void
NDECL(match_numeric)
{
    dbref match;

    if (have_exact)
	return;
    match = absolute_name(0);
    if (Good_obj(match)) {
	exact_match = match;
	have_exact = 1;
	local_match = 0;
    }
}

void
NDECL(match_controlled_absolute)
{
    dbref match;

    if (have_exact)
	return;

    match = absolute_name(1);
    if (Good_obj(match) &&
	(Controls(match_who, match) || nearby(match_who, match))) {
	exact_match = match;
	have_exact = 1;
	local_match = 0;
    }
}

void
NDECL(match_me)
{
    if (have_exact)
	return;
    if (!string_compare(match_name, "me")) {
	exact_match = match_who;
	have_exact = 1;
	local_match = 1;
    }
}

void
NDECL(match_home)
{
    if (have_exact)
	return;
    if (!string_compare(match_name, "home")) {
	exact_match = HOME;
	have_exact = 1;
	local_match = 0;
    }
}

void
NDECL(match_here)
{
    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_location(match_who)) {
	if (!string_compare(match_name, "here") &&
	    Good_obj(Location(match_who))) {
	    exact_match = Location(match_who);
	    have_exact = 1;
	    local_match = 1;
	}
    }
}

static void
match_list_altname(first)
    dbref first;
{
    dbref absolute, aowner;
    char *namebuf, *namebuf2, *namebuf3;
    int aflags;

    dbref buff_exact_match = NOTHING;	/* holds result of exact match */
    int buff_have_exact = 0;	/* Have a precise match (dbref, 'me', etc) */
    int buff_check_keys = 0;	/* if non-zero, check for keys */
    dbref buff_last_match = NOTHING;	/* holds result of last match */
    int buff_match_count = 0;	/* holds total number of inexact matches */
    dbref buff_match_who = NOTHING;	/* player performing match */
    const char *buff_match_name = "";	/* name to match */
    int buff_preferred_type = NOTYPE;	/* preferred type */
    int buff_local_match = 0;	/* Matched something locally, not by number */
    int buff_reality_valuechk = 0;        /* Reality level check */

#ifdef REALITY_LEVELS
/* Buffer the results */
    dbref pres_exact_match = NOTHING;   /* holds result of exact match */
    int pres_have_exact = 0;    /* Have a precise match (dbref, 'me', etc) */
    int pres_check_keys = 0;    /* if non-zero, check for keys */
    dbref pres_last_match = NOTHING;    /* holds result of last match */
    int pres_match_count = 0;   /* holds total number of inexact matches */
    dbref pres_match_who = NOTHING;     /* player performing match */
    char *pres_match_name = "";   /* name to match */
    int pres_preferred_type = NOTYPE;   /* preferred type */
    int pres_local_match = 0;   /* Matched something locally, not by number */
/* End Buffering */
#endif


    if (have_exact)
	return;
    absolute = absolute_name(1);
    if (!Good_obj(absolute))
	absolute = NOTHING;

    DOLIST(first, first) {
#ifdef REALITY_LEVELS
        pres_exact_match = exact_match;
        pres_have_exact = have_exact;
        pres_check_keys = check_keys;
        pres_last_match = last_match;
        pres_match_count = match_count;
        pres_match_who = match_who;
        pres_match_name = alloc_lbuf("pres_match_name");
        strcpy((char *)pres_match_name, (char *)match_name);
        pres_preferred_type = preferred_type;
        pres_local_match = local_match;
        if (!IsRealArg(match_who, first, reality_valuechk)) {
          exact_match = pres_exact_match;
          have_exact = pres_have_exact;
          check_keys = pres_check_keys;
          last_match = pres_last_match;
          match_count = pres_match_count;
          match_who = pres_match_who;
          strcpy((char *)match_name, (char *)pres_match_name);
          free_lbuf(pres_match_name);
          preferred_type = pres_preferred_type;
          local_match = pres_local_match;
          continue; 
        }
        exact_match = pres_exact_match;
        have_exact = pres_have_exact;
        check_keys = pres_check_keys;
        last_match = pres_last_match;
        match_count = pres_match_count;
        match_who = pres_match_who;
        strcpy((char *)match_name, (char *)pres_match_name);
        free_lbuf(pres_match_name);
        preferred_type = pres_preferred_type;
        local_match = pres_local_match;
#endif /* REALITY_LEVELS */
	if (!mudstate.whisper_state && ((Cloak(first) && !Wizard(match_who)) || (Cloak(first) && SCloak(first) && !Immortal(match_who)))) {
	  if (!mudstate.exitcheck)
	    continue;
	}
	if (first == absolute) {
	    exact_match = first;
	    have_exact = 1;
	    local_match = 1;
	    return;
	}
	/* Warning: make sure there are no other calls to Name()
	 * in choose_thing or its called subroutines; they would
	 * overwrite Name()'s static buffer which is needed by
	 * string_match().
	 */
/*put it back here*/

	namebuf = atr_get(first, A_ALTNAME, &aowner, &aflags);
        if ( !*namebuf ) {
           if ( NoName(first) ) {
	      namebuf2 = atr_pget(first, A_NAME_FMT, &aowner, &aflags);
              if ( *namebuf2 ) {
                 /* We need this because exec can clobber the match stack */
                 buff_exact_match = exact_match;	/* holds result of exact match */
                 buff_have_exact = have_exact;	/* Have a precise match (dbref, 'me', etc) */
                 buff_check_keys = check_keys;	/* if non-zero, check for keys */
                 buff_last_match = last_match;	/* holds result of last match */
                 buff_match_count = match_count;	/* holds total number of inexact matches */
                 buff_match_who = match_who;	/* player performing match */
                 buff_match_name = alloc_lbuf("match_exec");
                 strcpy((char *)buff_match_name, (char *)match_name);
                 buff_preferred_type = preferred_type;	/* preferred type */
                 buff_local_match = local_match;	/* Matched something locally, not by number */
                 buff_reality_valuechk = reality_valuechk;        /* Reality level check */

                 namebuf3 = exec(first, match_who, match_who, EV_FIGNORE|EV_EVAL|EV_TOP, namebuf2, (char **) NULL, 0, (char **)NULL, 0);
                 strcpy(namebuf, strip_all_special(namebuf3));
                 free_lbuf(namebuf3);

                 /* And now we restore the match stack that exec likely clobbered */
                 exact_match = buff_exact_match;	/* holds result of exact match */
                 have_exact = buff_have_exact;	/* Have a precise match (dbref, 'me', etc) */
                 check_keys = buff_check_keys;	/* if non-zero, check for keys */
                 last_match = buff_last_match;	/* holds result of last match */
                 match_count = buff_match_count;	/* holds total number of inexact matches */
                 match_who = buff_match_who;	/* player performing match */
                 strcpy((char *)match_name, (char *)buff_match_name);
                 free_lbuf(buff_match_name);
                 preferred_type = buff_preferred_type;	/* preferred type */
                 local_match = buff_local_match;	/* Matched something locally, not by number */
                 reality_valuechk = buff_reality_valuechk;        /* Reality level check */
              }
              free_lbuf(namebuf2);
           }
           if ( !*namebuf ) {
              free_lbuf(namebuf);
              continue;
           }
        } 
	if (!string_compare(namebuf, match_name)) {
	    /* if multiple exact matches, randomly choose one */
	    exact_match = choose_thing(exact_match, first);
	    local_match = 1;
	} else if (string_match(namebuf, match_name)) {
	    last_match = first;
	    match_count++;
	    local_match = 1;
	}
        free_lbuf(namebuf);
    }
}

static void
match_list(first)
    dbref first;
{
    dbref absolute;
    char *namebuf;

#ifdef REALITY_LEVELS
/* Buffer the results */
    dbref pres_exact_match = NOTHING;   /* holds result of exact match */
    int pres_have_exact = 0;    /* Have a precise match (dbref, 'me', etc) */
    int pres_check_keys = 0;    /* if non-zero, check for keys */
    dbref pres_last_match = NOTHING;    /* holds result of last match */
    int pres_match_count = 0;   /* holds total number of inexact matches */
    dbref pres_match_who = NOTHING;     /* player performing match */
    char *pres_match_name = "";   /* name to match */
    int pres_preferred_type = NOTYPE;   /* preferred type */
    int pres_local_match = 0;   /* Matched something locally, not by number */
/* End Buffering */
#endif

    if (have_exact)
	return;
    absolute = absolute_name(1);
    if (!Good_obj(absolute))
	absolute = NOTHING;

    DOLIST(first, first) {
#ifdef REALITY_LEVELS
        pres_exact_match = exact_match;
        pres_have_exact = have_exact;
        pres_check_keys = check_keys;
        pres_last_match = last_match;
        pres_match_count = match_count;
        pres_match_who = match_who;
        pres_match_name = alloc_lbuf("pres_match_name");
        strcpy((char *)pres_match_name, (char *)match_name);
        pres_preferred_type = preferred_type;
        pres_local_match = local_match;
        if (!IsRealArg(match_who, first, reality_valuechk)) {
          exact_match = pres_exact_match;
          have_exact = pres_have_exact;
          check_keys = pres_check_keys;
          last_match = pres_last_match;
          match_count = pres_match_count;
          match_who = pres_match_who;
          strcpy((char *)match_name, (char *)pres_match_name);
          free_lbuf(pres_match_name);
          preferred_type = pres_preferred_type;
          local_match = pres_local_match;
          continue;
        }
        exact_match = pres_exact_match;
        have_exact = pres_have_exact;
        check_keys = pres_check_keys;
        last_match = pres_last_match;
        match_count = pres_match_count;
        match_who = pres_match_who;
        strcpy((char *)match_name, (char *)pres_match_name);
        free_lbuf(pres_match_name);
        preferred_type = pres_preferred_type;
        local_match = pres_local_match;
#endif /* REALITY_LEVELS */
	if (!mudstate.whisper_state && ((Cloak(first) && !Wizard(match_who)) || (Cloak(first) && SCloak(first) && !Immortal(match_who)))) {
	  if (!mudstate.exitcheck)
	    continue;
	}
	if (first == absolute) {
	    exact_match = first;
	    have_exact = 1;
	    local_match = 1;
	    return;
	}
	/* Warning: make sure there are no other calls to Name()
	 * in choose_thing or its called subroutines; they would
	 * overwrite Name()'s static buffer which is needed by
	 * string_match().
	 */
/*put it back here*/

	namebuf = Name(first);
	if (!string_compare(namebuf, match_name)) {
	    /* if multiple exact matches, randomly choose one */
	    exact_match = choose_thing(exact_match, first);
	    local_match = 1;
	} else if (string_match(namebuf, match_name)) {
	    last_match = first;
	    match_count++;
	    local_match = 1;
	}
    }
}

void
NDECL(match_possession_altname)
{
    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_contents(match_who))
	match_list_altname(Contents(match_who));
}

void
NDECL(match_possession)
{
    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_contents(match_who))
	match_list(Contents(match_who));
}

void
NDECL(match_neighbor)
{
    dbref loc;

    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_location(match_who)) {
	loc = Location(match_who);
	if (Good_obj(loc)) {
	    match_list(Contents(loc));
	}
    }
}

void
NDECL(match_altname)
{
    dbref loc;

    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_location(match_who)) {
	loc = Location(match_who);
	if (Good_obj(loc)) {
	    match_list_altname(Contents(loc));
	}
    }
}

static void
match_exit_internal(dbref loc, int pcheck)
{
    dbref exit, absolute;

#ifdef REALITY_LEVELS
/* Buffer the results */
    dbref pres_exact_match = NOTHING;   /* holds result of exact match */
    int pres_have_exact = 0;    /* Have a precise match (dbref, 'me', etc) */
    int pres_check_keys = 0;    /* if non-zero, check for keys */
    dbref pres_last_match = NOTHING;    /* holds result of last match */
    int pres_match_count = 0;   /* holds total number of inexact matches */
    dbref pres_match_who = NOTHING;     /* player performing match */
    char *pres_match_name = "";   /* name to match */
    int pres_preferred_type = NOTYPE;   /* preferred type */
    int pres_local_match = 0;   /* Matched something locally, not by number */
/* End Buffering */
#endif

    if (!Good_obj(loc) || !Has_exits(loc))
	return;
    absolute = absolute_name(1);
    if (!Good_obj(absolute) || !controls(match_who, absolute))
	absolute = NOTHING;

    DOLIST(exit, Exits(loc)) {
	if (pcheck && (Flags3(exit) & PRIVATE))
	    continue;
#ifdef REALITY_LEVELS
        pres_exact_match = exact_match;
        pres_have_exact = have_exact;
        pres_check_keys = check_keys;
        pres_last_match = last_match;
        pres_match_count = match_count;
        pres_match_who = match_who;
        pres_match_name = alloc_lbuf("pres_match_name");
        strcpy((char *)pres_match_name, (char *)match_name);
        pres_preferred_type = preferred_type;
        pres_local_match = local_match;
        if (!IsRealArg(match_who, exit, reality_valuechk)) {
          exact_match = pres_exact_match;
          have_exact = pres_have_exact;
          check_keys = pres_check_keys;
          last_match = pres_last_match;
          match_count = pres_match_count;
          match_who = pres_match_who;
          strcpy((char *)match_name, (char *)pres_match_name);
          free_lbuf(pres_match_name);
          preferred_type = pres_preferred_type;
          local_match = pres_local_match;
          continue;
        }
        exact_match = pres_exact_match;
        have_exact = pres_have_exact;
        check_keys = pres_check_keys;
        last_match = pres_last_match;
        match_count = pres_match_count;
        match_who = pres_match_who;
        strcpy((char *)match_name, (char *)pres_match_name);
        free_lbuf(pres_match_name);
        preferred_type = pres_preferred_type;
        local_match = pres_local_match;
#endif /* REALITY_LEVELS */ 
	if (exit == absolute) {
	    exact_match = exit;
	    local_match = 1;
	    return;
	}
	if (matches_exit_from_list((char *) match_name, Name(exit))) {
	    exact_match = choose_thing(exact_match, exit);
	    local_match = 1;
	}
    }
}

void
NDECL(match_exit)
{
    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_location(match_who))
	match_exit_internal(Location(match_who), 0);
}

void
NDECL(match_exit_with_parents)
{
    dbref parent, first;
    int lev;

    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_location(match_who)) {
	first = Location(match_who);
	if (Good_obj(first)) {
	  ITER_PARENTS(first, parent, lev) {
	    if (first == parent)
	      match_exit_internal(parent, 0);
	    else
	      match_exit_internal(parent, 1);
	    if (exact_match != NOTHING)
		break;
	  }
	}	
    }
}

void
NDECL(match_carried_exit)
{
    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_exits(match_who))
	match_exit_internal(match_who, 0);
}

void
NDECL(match_carried_exit_with_parents)
{
    dbref parent, first;
    int lev;

    if (have_exact)
	return;
    if (Good_obj(match_who) && Has_exits(match_who)) {
	first = match_who;
	ITER_PARENTS(match_who, parent, lev) {
	    if (first == match_who)
	      match_exit_internal(parent, 0);
	    else
	      match_exit_internal(parent, 1);
	    if (exact_match != NOTHING)
		break;
	}
    }
}

void
NDECL(match_master_exit)
{
    if (exact_match != NOTHING)
	return;
    if (Good_obj(mudconf.master_room))
	match_exit_internal(mudconf.master_room, 1);
}

void
match_everything(key)
    int key;
{
    /*
     * Try matching me, then here, then absolute, then player FIRST, since
     * this will hit most cases. STOP if we get something, since those are
     * exact matches.
     */

    match_me();
    match_here();
    match_absolute();
    if (key & MAT_NUMERIC)
	match_numeric();
    if (key & MAT_HOME)
	match_home();
    match_player();
    if (have_exact)
	return;

    if (!(key & MAT_NO_EXITS)) {
	if (key & MAT_EXIT_PARENTS) {
	    match_carried_exit_with_parents();
	    match_exit_with_parents();
	} else {
	    match_carried_exit();
	    match_exit();
	}
    }
    match_neighbor();
    match_possession();
}

dbref
NDECL(match_result)
{
    if (exact_match != NOTHING)
	return exact_match;
    switch (match_count) {
    case 0:
	return NOTHING;
    case 1:
	return last_match;
    default:
	return AMBIGUOUS;
    }
}

/* use this if you don't care about ambiguity */

dbref
NDECL(last_match_result)
{
    if (exact_match != NOTHING)
	return exact_match;
    return last_match;
}

dbref
match_status(player, match)
    dbref player, match;
{
    switch (match) {
    case NOTHING:
	notify(player, NOMATCH_MESSAGE);
	return NOTHING;
    case AMBIGUOUS:
	notify(player, AMBIGUOUS_MESSAGE);
	return NOTHING;
    case NOPERM:
	notify(player, NOPERM_MESSAGE);
	return NOTHING;
    }
    return match;
}

dbref
NDECL(noisy_match_result)
{
    return match_status(match_who, match_result());
}

dbref
dispatched_match_result(player)
    dbref player;
{
    dbref match, save_player;

    save_player = match_who;
    match_who = player;
    match = noisy_match_result();
    match_who = save_player;
    return match;
}

int
NDECL(matched_locally)
{
    return local_match;
}

void
save_match_state(mstate)
    MSTATE *mstate;
{
    mstate->exact_match = exact_match;
    mstate->have_exact = have_exact;
    mstate->check_keys = check_keys;
    mstate->last_match = last_match;
    mstate->match_count = match_count;
    mstate->match_who = match_who;
    mstate->match_name = alloc_lbuf("save_match_state");
    strcpy(mstate->match_name, match_name);
    mstate->preferred_type = preferred_type;
    mstate->local_match = local_match;
}

void
restore_match_state(mstate)
    MSTATE *mstate;
{
    exact_match = mstate->exact_match;
    have_exact = mstate->have_exact;
    check_keys = mstate->check_keys;
    last_match = mstate->last_match;
    match_count = mstate->match_count;
    match_who = mstate->match_who;
    strcpy((char *) match_name, mstate->match_name);
    free_lbuf(mstate->match_name);
    preferred_type = mstate->preferred_type;
    local_match = mstate->local_match;
}

dbref
match_controlled_quiet(player, name)
    dbref player;
    const char *name;
{
    dbref mat;

    init_match(player, name, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    mat = match_result();
    if (Good_obj(mat) && !Controls(player, mat)) {
	return NOTHING;
    } else {
	return (mat);
    }
}
