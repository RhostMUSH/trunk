/* predicates.c */

#ifdef SOLARIS
/* Broken Solaris declaration headers */
char *index(const char *, int);
#endif
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
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */

extern dbref	FDECL(match_thing, (dbref, char *));
#ifndef STANDALONE
extern int attrib_cansee(dbref, const char *, dbref, dbref);
extern int attrib_canset(dbref, const char *, dbref, dbref);
#endif

/* ---------------------------------------------------------------------------
 * insert_first, remove_first: Insert or remove objects from lists.
 */

dbref insert_first (dbref head, dbref thing)
{
	s_Next(thing, head);
	return thing;
}

dbref remove_first (dbref head, dbref thing)
{
dbref	prev;

	if (head == thing)
		return (Next(thing));

	DOLIST(prev, head) {
		if (Next(prev) == thing) {
			s_Next(prev, Next(thing));
			return head;
		}
	}
	return head;
}

/* ---------------------------------------------------------------------------
 * reverse_list: Reverse the order of members in a list.
 */

dbref reverse_list(dbref list)
{
dbref	newlist, rest;

	newlist = NOTHING;
	while (list != NOTHING) {
		rest = Next(list);
		s_Next(list, newlist);
		newlist = list;
		list = rest;
	}
	return newlist;
}

/* ---------------------------------------------------------------------------
 * member - indicate if thing is in list
 */

int member(dbref thing, dbref list)
{
	DOLIST(list, list) {
		if (list == thing)
			return 1;
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * is_rhointeger, is_number: see if string contains just a number.
 * different in that it checks + as well as - cases
 */

int is_rhointeger (char *str)
{
      while (*str && isspace((int)*str)) str++;       /* Leading spaces */
      if ( (*str == '-') || (*str == '+') ) {         /* Leading -/+ */
              str++;
              if (!*str) return 0;            /* but not if just a -/+ */
      }
      if (!isdigit((int)*str))                        /* Need at least 1 integer */
              return 0;
      while (*str && isdigit((int)*str)) str++;       /* The number (int) */
      while (*str && isspace((int)*str)) str++;       /* Trailing spaces */
      return (*str ? 0 : 1);
}


/* ---------------------------------------------------------------------------
 * is_integer, is_number: see if string contains just a number.
 */

int is_integer (char *str)
{
	while (*str && isspace((int)*str)) str++;	/* Leading spaces */
	if (*str == '-') {			/* Leading minus */
		str++;
		if (!*str) return 0;		/* but not if just a minus */
	}
	if (!isdigit((int)*str))			/* Need at least 1 integer */
		return 0;
	while (*str && isdigit((int)*str)) str++;	/* The number (int) */
	while (*str && isspace((int)*str)) str++;	/* Trailing spaces */
	return (*str ? 0 : 1);
}

int is_float2 (char *str) 
{
  char *chk;
  
  if(strtod( str, &chk ))
  ; /* Then  do nothing */

  return (*chk ? 0 : 1);
}

int is_float (char *str)
{
   int i_dot = 0;
   while (*str && isspace((int)*str)) str++;	/* Leading spaces */
   if (*str == '-') {			/* Leading minus */
      str++;
      if (!*str) 
         return 0;		/* but not if just a minus */
   }
   if (*str == '.') {
      str++;
      if ( !*str )
         return 0;
      i_dot = 1;
   }
   if (!isdigit((int)*str))			/* Need at least 1 integer */
      return 0;
   while (*str && (isdigit((int)*str) || ((*str == '.') && !i_dot)) ) { 
      if ( *str == '.' )
         i_dot = 1;
      str++;	/* The number (int) */
   }
   while (*str && isspace((int)*str))
       str++;	/* Trailing spaces */
   return (*str ? 0 : 1);
}

#define MAXABSLOCDEPTH 20

dbref Location(dbref target)
{
  return Location_safe(target,0);
}
dbref Location_safe(dbref target, int override)
{
  if((mudstate.remote < 0) || (override != 0))
    return db[target].location;
  else
    return mudstate.remote;
}
dbref absloc(dbref player)
{
  dbref prev, curr;
  int depth;

  if( !Good_obj(player) ) {
    return -1;
  }

  if( Typeof(player) == TYPE_ROOM ) {
    return player;
  }
 
  for( depth = 0, curr = Location(player), prev = -1;
       Good_obj(curr) && depth < MAXABSLOCDEPTH && curr != prev;
       prev = curr, curr = Location(curr), depth++ ) {
    /* leave objects until you hit final destination */
  }

  if( !Good_obj(prev) || depth == MAXABSLOCDEPTH) 
    return -1;

  if( Typeof(prev) != TYPE_ROOM )
    return -1;

  return prev;
}

#ifndef STANDALONE
int ZoneWizard(dbref player, dbref target)
{
  ZLISTNODE *ptr; 

  if( !Inherits(player) ) 
    return 0;

  if( db[target].zonelist &&
      !ZoneMaster(target) ) {
    for( ptr = db[target].zonelist; ptr; ptr = ptr->next ) {
      if( could_doit(Owner(player), ptr->object, A_LZONEWIZ, 0, 0) ) 
        return 1;
    }
  }
  return 0;
}
#endif

/* Had to move some of these ex-macros into functions since they were
   getting too fricking big, and they didn't fit in my virtual memory */
/* 1 = check against NOEXAMINE */
/* 2 = check against @Locks for flag sets */

int See_attr(dbref p, dbref x, ATTR* a, dbref o, int f, int key)
{

  if ( key & 2 ) {
     if ( ((a)->flags & AF_INTERNAL) || (f & AF_INTERNAL) ) {
        return 0;
     }
  } else {
     if ( ((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) || (f & (AF_INTERNAL|AF_IS_LOCK)) ) {
        return 0;
     }
  }

  if ( God(p) || Immortal(p) )
     return 1;

  if ( ((a)->flags & (AF_DARK)) || (f & (AF_DARK)) )
     return 0;

  if ( (((a)->flags & (AF_MDARK)) || 
       (f & (AF_MDARK))) && !(Wizard(p) || (HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) &&
                                            ExFullWizAttr(p))) )
     return 0;

  if ( !(key & 1) && NoEx(x) && !Wizard(p))
     return 0;

  if (Backstage(p) && NoBackstage(x))
     return 0;

#ifndef STANDALONE
#ifdef ATTR_HACK
  if ( (mudconf.hackattr_see == 0) && 
      !(Wizard(p) || (HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && ExFullWizAttr(p))) && 
      ((a)->name[0] == '_') ) 
     return 0;
#endif
  if ( !(attrib_cansee(p, (a)->name, Owner(x), x)) )
     return 0;
#endif

  if ( DePriv(p, Owner(x), DP_EXAMINE, POWER6, NOTHING) )
     return 0;

  if ( DePriv(p, o, DP_EXAMINE, POWER6, NOTHING) )
     return 0;

  if ( DePriv(p, Owner(x), DP_MORTAL_EXAMINE, POWER8, NOTHING) ) {
     if ( ((a)->flags & (AF_DARK|AF_MDARK)) ||
          (f & (AF_DARK|AF_MDARK)) )
        return 0;
     if ( !(Visual(x) || (Owner(p) == Owner(x))) )
        return 0;
  }

  if ( ((a)->flags & AF_VISUAL) || (f & AF_VISUAL) )
     return 1;

  if( (Examinable(p,x) || (Owner(p) == o)) &&
      (!(f & AF_LOCK) || Controls(p,x)) &&
      !(((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK))) )
     return 1;

  if( !(((a)->flags & (AF_MDARK|AF_ODARK|AF_DARK)) || (f & (AF_MDARK|AF_ODARK|AF_DARK))) ) 
     return 1;
 
  if ((((a)->flags & (AF_MDARK|AF_ODARK)) || (f & (AF_MDARK|AF_ODARK))) && Wizard(p))
     return 1;

  return 0;
}

int Read_attr(dbref p, dbref x, ATTR* a, dbref o, int f, int key )
{
  if( ((a)->flags & (AF_INTERNAL)) || (f & (AF_INTERNAL)) )
    return 0;

  if( God(p) )
    return 1;

  if( Immortal(p) )
    return 1;

  if( God(Owner(x)) )
    return 0;

  if ( !(key & 1) && NoEx(x) && !Wizard(p))
    return 0;

  if (Backstage(p) && NoBackstage(x))
    return 0;

#ifndef STANDALONE
#ifdef ATTR_HACK
  if ( (mudconf.hackattr_see == 0) && 
      !(Wizard(p) || (HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && ExFullWizAttr(p))) && 
      ((a)->name[0] == '_') ) 
    return 0;
#endif
  if ( !(attrib_cansee(p, (a)->name, Owner(x), x)) )
    return 0;
#endif
  if ((((a)->flags & AF_IMMORTAL) || 
       ((a)->flags & AF_GOD) || (f & AF_IMMORTAL) || (f & AF_GOD)) && 
      (((a)->flags & (AF_DARK | AF_MDARK)) || (f & (AF_DARK|AF_MDARK)))) {
    return 0;
  }

  if( ((a)->flags & AF_VISUAL) || (f & AF_VISUAL) ) {
    return 1;
  }

  if( DePriv(p,Owner(x),DP_EXAMINE,POWER6,NOTHING) && (Owner(p) != Owner(x))) {
    return 0;
  }

  if ( DePriv(p, Owner(x), DP_MORTAL_EXAMINE, POWER8, NOTHING) ) {
     if ( ((a)->flags & (AF_DARK|AF_MDARK)) || (f & (AF_DARK|AF_MDARK)) ) {
        return 0;
     }
     if ( !(Visual(x) || (Owner(p) == Owner(x))) ) {
        return 0;
     }
  }

  if( (Wizard(p) || (HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && ExFullWizAttr(p))) &&
      (!Immortal(Owner(x)) || (((a)->flags & AF_WIZARD) || (f & AF_WIZARD))) &&
       !(((a)->flags & AF_DARK) || (f & AF_DARK)) ) {
    return 1;
  } else if ((((a)->flags & AF_WIZARD) || (f & AF_WIZARD)) && 
             (((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK)))) {
    return 0;
  }

  if( Admin(p) && ((!Wizard(Owner(x)) && !Immortal(Owner(x))) ||
      (((a)->flags & AF_ADMIN) || (f & AF_ADMIN))) ) {
    return 1;
  } else if ((((a)->flags & AF_ADMIN) || (f & AF_ADMIN)) && 
             (((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK)))) {
    return 0;
  }

  if( Builder(p) && ((!Admin(Owner(x)) && !Wizard(Owner(x)) && !Immortal(Owner(x))) ||
      (((a)->flags & AF_BUILDER) || (f & AF_BUILDER))) ) {
    return 1;
  } else if ((((a)->flags & AF_BUILDER) || (f & AF_BUILDER)) && 
             (((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK)))) {
    return 0;
  }
  if( Guildmaster(p) && ((!Guildmaster(Owner(x)) && !Builder(Owner(x)) &&
        !Admin(Owner(x)) && !Wizard(Owner(x)) && !Immortal(Owner(x))) ||
       (((a)->flags & AF_GUILDMASTER) || (f & AF_GUILDMASTER))) ) {
    return 1;
  } else if ((((a)->flags & AF_GUILDMASTER) || (f & AF_GUILDMASTER)) && 
             (((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK)))) {
    return 0;
  }

  if( (Examinable(p,x) || (Owner(p) == o)) &&
      (!(f & AF_LOCK) || Controls(p,x)) &&
      !(((a)->flags & (AF_MDARK|AF_DARK)) || (f & (AF_MDARK|AF_DARK))) ) {
    return 1;
  }
  if( !(((a)->flags & (AF_MDARK|AF_ODARK|AF_DARK)) || 
        (f & (AF_MDARK|AF_ODARK|AF_DARK))) ) {
    return 1;
  }

  return 0;
}

#ifndef STANDALONE
int Examinable(dbref p, dbref x) 
{
  if ( !Good_obj(x) )
    return 0;

  if( (Recover(x) || Going(x)) && !Immortal(Owner(p)) )
    return 0;

  if ( HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && 
       (!Cloak(x) && !Recover(x) && !NoEx(x)) )
    return 1;

  if (NoEx(x) && !Wizard(p))
    return 0;

  if( Owner(p) == Owner(x) )
    return 1;

  if( Flags(x) & VISUAL )   
    return 1;
  
  if( Immortal(p) )
    return 1;

  if( God(Owner(x)) ) 
    return 0;

  if (Backstage(p) && NoBackstage(x))
    return 0;

  if( could_doit(p,x,A_LTWINK,0,0) )
    return 1; 

  if( Immortal(Owner(x)) )
    return 0;

  if( DePriv(p, Owner(x), DP_EXAMINE, POWER6, NOTHING) ) 
    return 0;

  if( DePriv(p, Owner(x), DP_MORTAL_EXAMINE,POWER8,NOTHING) &&
      !(Visual(x) || (Owner(p) == Owner(x))) )
    return 0;

  if( HasPriv(p,Owner(x),POWER_EX_ALL,POWER3,NOTHING) ) 
    return 1;

  if( Wizard(p) )
    return 1;

  if( Wizard(Owner(x)) )
    return 0;

  if( ZoneWizard(p,x) )
    return 1;

  if( Admin(p) ) 
    return 1;

  if( Admin(Owner(x)) )
    return 0;

  if( Builder(p) )
    return 1;

  if( Builder(Owner(x)) )
    return 0;
 
  if( Guildmaster(p) )
    return 1;

  if( Guildmaster(Owner(x)) )
    return 0;

  return 0;
}
#else
int Examinable(dbref p,dbref x)
{
  if (NoEx(x) && !Wizard(p))
    return 0;

  if( Owner(p) == Owner(x) )
    return 1;

  if( Flags(x) & VISUAL )   
    return 1;
  
  if( Immortal(p) )
    return 1;

  if( God(Owner(x)) ) 
    return 0;
 
  if( Immortal(Owner(x)) )
    return 0;

  if (Backstage(p) && NoBackstage(x))
    return 0;

  if( Wizard(p) )
    return 1;

  if( Wizard(Owner(x)) )
    return 0;

  if( Admin(p) ) 
    return 1;

  if( Admin(Owner(x)) )
    return 0;

  if( Builder(p) )
    return 1;

  if( Builder(Owner(x)) )
    return 0;
 
  if( Guildmaster(p) )
    return 1;

  if( Guildmaster(Owner(x)) )
    return 0;

  return 0;
}
#endif

int Controls(dbref p, dbref x)
{
  if( !Good_obj(x) )
    return 0;
  
  if( Going(x) )
    return 0;

  if( Owner(x) == Owner(p) &&
      (Inherits(p) || !Inherits(x)) )
    return 1;

  if( God(p) )
    return 1;

  if( God(Owner(x)) )
    return 0;

  if( Immortal(p) )
    return 1;

  if( Immortal(Owner(x)) )
    return 0;

  if (Backstage(p) && NoBackstage(x))
    return 0;

  if( Wizard(p) )
    return 1;

  if( Wizard(Owner(x)) )
    return 0;

  if( Admin(p) )
    return 1;

  if( Admin(Owner(x)) )
    return 0;

  if( Builder(p) )
    return 1;

  if( Builder(Owner(x)) )
    return 0;

#ifndef STANDALONE
  if( ZoneWizard(p,x) )
    return 1;
#endif

  return 0;
}

int Controlsforattr(dbref p, dbref x, ATTR *a, int f)
{
#ifndef STANDALONE
  if ((Good_obj(x) && !WizMod(p) && NoMod(x)) || (Good_obj(x) && DePriv(p,Owner(x),DP_MODIFY,POWER7,NOTHING) &&
#else
  if ((Good_obj(x) && DePriv(p,Owner(x),DP_MODIFY,POWER7,NOTHING) &&
#endif
	(Owner(p) != Owner(x))))
    return 0;

  if (Backstage(p) && NoBackstage(x) && !Immortal(p))
    return 0;

#ifndef STANDALONE

  if ( ((f & AF_SAFE) && !(f & 0x80000000)) &&
       !(Immortal(p) && !(NoOverride(p) || (mudconf.wiz_override == 0))) )
    return 0;

#ifdef ATTR_HACK
  if ( ((a)->name[0] == '_') && !(Wizard(p) ||
        (HasPriv(p, NOTHING, POWER_EX_FULL, POWER5, NOTHING) && ExFullWizAttr(p))) && 
        !(mudconf.hackattr_nowiz) )
    return 0;
#endif
  if ( !(attrib_canset(p, (a)->name, Owner(x), x)) )
    return 0;

#endif

  return (Good_obj(x) && 
          (!(God(x) && !God(p))) && 
          (ControlsforattrImmortal(p,x,a,f) || 
           ControlsforattrWizard(p,x,a,f) || 
           ControlsforattrAdmin(p,x,a,f) || 
           ControlsforattrBuilder(p,x,a,f) || 
           ControlsforattrGuildmaster(p,x,a,f) || 
           ControlsforattrZoneWizard(p,x,a,f) || 
           ControlsforattrOwner(p,x,a,f) 
          ));
}

int is_number (char *str)
{
int	got_one;

	while (*str && isspace((int)*str)) str++;	/* Leading spaces */
	if (*str == '-') {			/* Leading minus */
		str++;
		if (!*str) return 0;		/* but not if just a minus */
	}
	got_one = 0;
	if (isdigit((int)*str)) got_one = 1;		/* Need at least one digit */
	while (*str && isdigit((int)*str)) str++;	/* The number (int) */
	if (*str == '.') str++;			/* decimal point */
	if (isdigit((int)*str)) got_one = 1;		/* Need at least one digit */
	while (*str && isdigit((int)*str)) str++;	/* The number (fract) */
	while (*str && isspace((int)*str)) str++;	/* Trailing spaces */
	return ((*str || !got_one) ? 0 : 1);
}


#ifndef STANDALONE

int can_listen(dbref thing)
{
  char *buff, *as, *s;
  int attr, aflags, canhear, loop;
  dbref aowner, parent;
  ATTR *ap;

  canhear = 0;
  if ((Typeof(thing) == TYPE_PLAYER) || Puppet(thing))
    canhear = 1;
  else {
    if (Monitor(thing))
      buff = alloc_lbuf("can_listen");
    else
      buff = NULL;
    if ( (mudconf.listen_parents == 0) || !Monitor(thing) ) {
       for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
         if (attr == A_LISTEN) {
           canhear = 1;
           break;
         }
         if (Monitor(thing)) {
           ap = atr_num(attr);
           if (!ap || (ap->flags & AF_NOPROG))
	     continue;
           atr_get_str(buff, thing, attr, &aowner, &aflags);
           if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
	     continue;
           for (s = buff + 1; *s && (*s != ':'); s++);
           if (s) {
	     canhear = 1;
	     break;
           }
         }
       }
    } else {
       ITER_PARENTS(thing, parent, loop) {
          for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
            if ((parent == thing) && (attr == A_LISTEN)) {
              canhear = 1;
              break;
            }
            if (Monitor(thing)) {
              ap = atr_num(attr);
              if (!ap || (ap->flags & AF_NOPROG))
	        continue;
              atr_get_str(buff, parent, attr, &aowner, &aflags);
              if ( (thing != parent) && ((ap->flags & AF_PRIVATE) || (aflags & AF_PRIVATE)) )
                continue;
              if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
	        continue;
              for (s = buff + 1; *s && (*s != ':'); s++);
              if (s) {
	        canhear = 1;
	        break;
              }
            }
          }
       }
    }
    if (buff)
      free_lbuf(buff);
    if (!canhear && !mudconf.comm_dark) {
      ITER_PARENTS(thing, parent, loop) {
	if (Commer(parent))
	  canhear = 1;
      }
    }
  }
  return canhear;
}

int can_see(dbref player, dbref thing, int can_see_loc)
{
	/* Don't show if all the following apply:
	 * Sleeping players should not be seen.
	 * The thing is a disconnected player.
	 * The player is not a puppet.
	 */

	if (mudconf.dark_sleepers && isPlayer(thing) &&
	    !Connected(thing) && !Puppet(thing)) {
		return 0;
	}

	/* You don't see yourself or exits */

	if ((player == thing) || isExit(thing)) {
		return 0;
	}

	/* If loc is not dark, you see it if it's not dark or you control it.
	 * If loc is dark, you see it if you control it.  Seeing your own
	 * dark objects is controlled by mudconf.see_own_dark.
	 * In dark locations, you also see things that are LIGHT and !DARK.
	 */

	if (can_see_loc) {
#ifdef REALITY_LEVELS
                return (IsReal(player, thing) && (!Dark(thing) || (Dark(thing) && could_doit(player, thing, A_LDARK, 0, 0) &&
                        !Cloak(thing)) || (mudconf.see_own_dark && MyopicExam(player, thing))));
#else
                return (!Dark(thing) || (Dark(thing) && could_doit(player, thing, A_LDARK, 0, 0) &&
                        !Cloak(thing)) || (mudconf.see_own_dark && MyopicExam(player, thing)));
#endif /* REALITY_LEVELS */
	} else {
#ifdef REALITY_LEVELS
                return (IsReal(player, thing) && ((Light(thing) && !Dark(thing)) ||
                        (Dark(thing) && could_doit(player,thing, A_LDARK, 0, 0) && !Cloak(thing)) ||
                        (mudconf.see_own_dark && MyopicExam(player, thing))));
#else
                return ((Light(thing) && !Dark(thing)) ||
                        (Dark(thing) && could_doit(player,thing, A_LDARK, 0, 0) && !Cloak(thing)) ||
                        (mudconf.see_own_dark && MyopicExam(player, thing)));
#endif /* REALITY_LEVELS */
	}
}

int can_see2(dbref player, dbref thing, int can_see_loc)
{
	/* Don't show if all the following apply:
	 * Sleeping players should not be seen.
	 * The thing is a disconnected player.
	 * The player is not a puppet.
	 */

	if (mudconf.dark_sleepers && isPlayer(thing) &&
	    !Connected(thing) && !Puppet(thing)) {
		return 0;
	}

	/* You don't see yourself or exits */

	if ((player == thing) || isExit(thing)) {
		return 0;
	}

	/* If loc is not dark, you see it if it's not dark or you control it.
	 * If loc is dark, you see it if you control it.  Seeing your own
	 * dark objects is controlled by mudconf.see_own_dark.
	 * In dark locations, you also see things that are LIGHT and !DARK.
	 */

	if (can_see_loc) {
#ifdef REALITY_LEVELS
                return (IsReal(player, thing) && (!Dark(thing) || (Dark(thing) && 
                        !Wizard(Owner(thing)) && can_listen(thing) && !isPlayer(thing)) || 
                         (mudconf.see_own_dark && MyopicExam(player, thing)) ||
                         (Dark(thing) && could_doit( player, thing, A_LDARK, 0, 0) && !Cloak(thing))));
#else
                return (!Dark(thing) || (Dark(thing) && !Wizard(Owner(thing)) && can_listen(thing) &&
                        !isPlayer(thing)) || (mudconf.see_own_dark && MyopicExam(player, thing)) ||
                         (Dark(thing) && could_doit( player, thing, A_LDARK, 0, 0) && !Cloak(thing)));
#endif /* REALITY_LEVELS */
	} else {
#ifdef REALITY_LEVELS
                return (IsReal(player, thing) && ((Light(thing) && !Dark(thing)) || 
                        (Dark(thing) && !Wizard(Owner(thing)) && can_listen(thing) && 
                        !isPlayer(thing)) || (Dark(thing) && could_doit( player, thing, A_LDARK, 0, 0) &&
                         !Cloak(thing)) || (mudconf.see_own_dark && MyopicExam(player, thing))));
#else
                return ((Light(thing) && !Dark(thing)) || (Dark(thing) && !Wizard(Owner(thing)) &&
                         can_listen(thing) && !isPlayer(thing)) || (Dark(thing) && 
                         could_doit(player, thing, A_LDARK, 0, 0) && !Cloak(thing)) || 
                        (mudconf.see_own_dark && MyopicExam(player, thing)));
#endif /* REALITY_LEVELS */
	}
}

#ifndef STANDALONE
int pay_quota(dbref player, dbref who, int cost, int ttype, int pay)
{
  dbref owner;
  int flags, rval, diff;
  int s[5] = { 0, 0, 0, 0, 0 };
  char *atr1, *atr2, *pt1, dest;


  who = Owner(who);

  if ((!Builder(who) && !HasPriv(who,NOTHING,POWER_FREE_QUOTA,POWER3,POWER_LEVEL_NA)) ||
	(DePriv(player,NOTHING,DP_UNL_QUOTA,POWER7,POWER_LEVEL_NA))) {
    rval = 0;
    atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
    if (!Altq(who)) {
      s[0] = atoi(atr1);
      if ((s[0] - cost) >= 0) {
	if (pay) {
	  sprintf(atr1,"%d",s[0]-cost);
          atr_add_raw(who,A_RQUOTA,atr1);
	}
	rval = 1;
      }
      free_lbuf(atr1);
      return rval;
    }
    sscanf(atr1,"%d %d %d %d %d",&s[0],&s[1],&s[2],&s[3],&s[4]);
    diff = s[ttype+1] - cost;
    if (diff < 0) {
      diff = 0-diff;
      atr2 = atr_get(who,A_TQUOTA,&owner,&flags);
      if (*atr2) {
	strcpy(atr1,"G ");
	strcat(atr1,atr2);
      }
      else {
	strcpy(atr1,"G");
      }
      switch (ttype) {
	case TYPE_PLAYER: dest = 'P'; break;
	case TYPE_THING: dest = 'T'; break;
	case TYPE_EXIT: dest = 'E'; break;
	case TYPE_ROOM: dest = 'R'; break;
	default: dest = 'U';
      }
      pt1 = atr1;
      while (!rval && *pt1) {
	if (*pt1 != dest) {
	  if (pay)
	    rval = q_check(player, who,diff,*pt1,dest,1,1);
	  else
	    rval = q_check(player, who,diff,*pt1,dest,1,0);
	}
	pt1++;
	while (isspace((int)*pt1)) pt1++;
      }
      free_lbuf(atr2);
    }
    else {
      rval = 1;
      if (pay) {
        s[ttype+1] = diff;
        sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
        atr_add_raw(who,A_RQUOTA,atr1);
      }
    }
    free_lbuf(atr1);
  }
  else if (pay) {
    pay_quota_force(who, cost, ttype);
    rval = 1;
  }
  else
    rval = 1;
  return rval;
}
#endif

int pay_quota_force(dbref who, int cost, int ttype)
{
  dbref owner;
  int flags, diff;
  int s[5] = { 0, 0, 0, 0, 0 }, 
      d[5] = { 0, 0, 0, 0, 0 }, 
      m[5] = { 0, 0, 0, 0, 0 };
  char *atr1, *atr2, *atr3;

  atr1 = atr_get(who,A_RQUOTA,&owner,&flags);
  if (!Altq(who)) {
    s[0] = atoi(atr1);
    diff = s[0] - cost;
    if (diff >= 0) {
      sprintf(atr1,"%d",diff);
      atr_add_raw(who,A_RQUOTA,atr1);
    }
    else {
      if (s[0] <= 0)
	diff = cost;
      else {
	diff = cost - s[0];
	atr_add_raw(who,A_RQUOTA,"0");
      }
      free_lbuf(atr1);
      atr1 = atr_get(who,A_QUOTA,&owner,&flags);
      s[0] = atoi(atr1) + diff;
      sprintf(atr1,"%d",s[0]);
      atr_add_raw(who,A_QUOTA,atr1);
    }
    free_lbuf(atr1);
    return 1;
  }
  sscanf(atr1,"%d %d %d %d %d",&s[0],&s[1],&s[2],&s[3],&s[4]);
  diff = s[ttype+1] - cost;
  if (diff >= 0)
    s[ttype+1] = diff;
  else {
    s[ttype+1] = 0;
    diff = 0-diff;
    atr2 = atr_get(who,A_QUOTA,&owner,&flags);
    atr3 = atr_get(who,A_MQUOTA,&owner,&flags);
    sscanf(atr2,"%d %d %d %d %d",&d[0],&d[1],&d[2],&d[3],&d[4]);
    sscanf(atr3,"%d %d %d %d %d",&m[0],&m[1],&m[2],&m[3],&m[4]);
    d[ttype+1] += diff;
    if (d[ttype+1] > m[ttype+1]) {
      m[ttype+1] = d[ttype+1];
      sprintf(atr3,"%d %d %d %d %d",m[0],m[1],m[2],m[3],m[4]);
      atr_add_raw(who,A_MQUOTA,atr3);
    }
    sprintf(atr2,"%d %d %d %d %d",d[0],d[1],d[2],d[3],d[4]);
    atr_add_raw(who,A_QUOTA,atr2);
    free_lbuf(atr2);
    free_lbuf(atr3);
  }
  sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
  atr_add_raw(who,A_RQUOTA,atr1);
  free_lbuf(atr1);
  return 1;
}

int canpayfees(dbref player, dbref who, int pennies, int quota, int ttype)
{
    char *tpr_buff, *tprp_buff;

	if (((!Builder(who) && !Builder(Owner(who)) &&
	    !FreeFlag(who) && !FreeFlag(Owner(who))) ||
	    DePriv(who,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA) ||
	    DePriv(Owner(who),NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) 
	    && (Pennies(Owner(who)) < pennies)) {
                tprp_buff = tpr_buff = alloc_lbuf("canpayfees");
		if (player == who) {
			notify(player,
				safe_tprintf(tpr_buff, &tprp_buff, "Sorry, you don't have enough %s.",
					mudconf.many_coins));
		} else {
			notify(player,
				safe_tprintf(tpr_buff, &tprp_buff, "Sorry, that player doesn't have enough %s.",
					mudconf.many_coins));
		}
                free_lbuf(tpr_buff);
		return 0;
	}

	if(mudconf.quotas) {
		if (!pay_quota(player, who, quota, ttype, 1)) {
			if (player == who) {
				notify(player,
					"Sorry, your building contract has run out.");
			} else {
				notify(player,
					"Sorry, that player's building contract has run out.");
			}
			return 0;
		}
	}
	payfor(who, pennies);
	return 1;
}

int payfor(dbref who, int cost)
{
	dbref tmp;

	if (!DePriv(who,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA) &&
	    !DePriv(Owner(who),NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) {
	  if (Builder(who) || Builder(Owner(who)) ||
		 FreeFlag(who) || FreeFlag(Owner(who))) {
		return 1;
	  }
	}

	who = Owner(who);
	if ((tmp = Pennies(who)) >= cost) {
		s_Pennies(who, tmp - cost);
		return 1;
	}

	return 0;
}

int payfor_force(dbref who, int cost)
{
  int tmp;

	if (!DePriv(who,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA) &&
	    !DePriv(Owner(who),NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) {
	  if (Builder(who) || Builder(Owner(who)) ||
		 FreeFlag(who) || FreeFlag(Owner(who))) {
		return 1;
	  }
	}

	who = Owner(who);
   tmp = Pennies(who);
	s_Pennies(who, tmp - cost);
		return 1;
}

int payfor_give(dbref who, int cost)
{
	dbref tmp;

	if (!DePriv(who,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA) &&
	    !DePriv(Owner(who),NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) {
	  if( Builder(who) || Builder(Owner(who)) 
	    ) {
	  return 1;
	  }
	}

	who = Owner(who);
	if((tmp = Pennies(who)) >= cost) {
		s_Pennies(who, tmp - cost);
		return 1;
	}
	return 0;
}

#endif	/* STANDALONE */
void add_quota(dbref who, int payment, int tthing)
{
  dbref owner;
  int flags;
  int s[5] = { 0, 0, 0, 0, 0 };
  char *atr1;

  if (payment < 1)
    return;
  atr1 = atr_get(who, A_RQUOTA, &owner, &flags);
  if (Altq(who)) {
    sscanf(atr1,"%d %d %d %d %d",&s[0],&s[1],&s[2],&s[3],&s[4]);
    s[tthing+1] += payment;
    sprintf(atr1,"%d %d %d %d %d",s[0],s[1],s[2],s[3],s[4]);
  }
  else {
    s[0] = atoi(atr1);
    s[0] += payment;
    sprintf(atr1,"%d",s[0]);
  }
  atr_add_raw(who, A_RQUOTA, atr1);
  free_lbuf(atr1);
}

int giveto(dbref who, int pennies, dbref from)
{
#ifndef STANDALONE
	if (!DePriv(who,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA) &&
	    !DePriv(Owner(who),NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) {
#endif
          if (Builder(who) || Builder(Owner(who)) ) {
	    return 1;
	  }
	  if ((FreeFlag(who) || FreeFlag(Owner(who))) && (from == NOTHING)) {
	    who = Owner(who);
	    s_Pennies(who, Pennies(who) + pennies);
	    return 1;
	  }
	  if ((FreeFlag(who) || FreeFlag(Owner(who))) && ((Owner(from) == who) || (Owner(from) == Owner(who)))) {
	    return 0;
	  }
#ifndef STANDALONE
	}
#endif
	who = Owner(who);
	s_Pennies(who, Pennies(who) + pennies);
	return 1;
}

int ok_name(const char *name)
{
const char	*cp;

	/* Disallow leading spaces */

	if (isspace((int)*name))
		return 0;
	if (strlen(name) > 160)
		return 0;

	/* Only printable characters */

	for (cp=name; cp && *cp; cp++) {
					 if (!isprint((int)*cp)) return 0;
		  }

	/* Disallow trailing spaces */
	cp--;
	if (isspace((int)*cp))
		return 0;

	/* Exclude names that start with or contain certain magic cookies */

	return (name &&
		*name &&
		*name != LOOKUP_TOKEN &&
		*name != NUMBER_TOKEN &&
		*name != NOT_TOKEN &&
		!index(name, ARG_DELIMITER) &&
		!index(name, AND_TOKEN) &&
		!index(name, OR_TOKEN) &&
#ifndef STANDALONE
		!strcmp(name, strip_all_special(name)) &&
#endif
		string_compare(name, "me") &&
		string_compare(name, "home") &&
		string_compare(name, "here"));
}

int ok_player_name(const char *name)
{
const char *cp, *good_chars;

	/* No leading spaces */

	if (isspace((int)*name))
		return 0;

	/* Not too long and a good name for a thing */

	if (!ok_name(name) || (strlen(name) >= PLAYER_NAME_LIMIT))
		return 0;

#ifndef STANDALONE
	if (mudconf.name_spaces)
		good_chars = " `$_-.,'";
	else
		good_chars = "`$_-.,'";
#else
	good_chars = " `$_-.,'";
#endif
	/* Make sure name only contains legal characters */

	for (cp=name; cp && *cp; cp++) {
		if (isalnum((int)*cp))
			continue;
		if (!index(good_chars, *cp))
			return 0;
	}
	return 1;
}

int ok_attr_name(const char *attrname)
{
const char *scan;

#ifdef ATTR_HACK
        if ( !(isValidAttrStartChar(*attrname)) ) return 0;
#else
	if (!isalpha((int)*attrname)) return 0;
#endif
	for (scan=attrname; *scan; scan++) {
		if (isalnum((int)*scan)) continue;
/* TinyMUSH Compatible Attribute Naming Convention */
/*		if (!(index("-_.@#$^&~=+", *scan))) return 0; */
/* Mux Compatible Attribute Naming Convention */
  		if (!(index("'?!`/-_.@#$^&~=+<>()%", *scan))) return 0;

	}
	return 1;
}

int ok_password(const char *password, dbref player, int key)
{
  const char *scan;
  int num_upper, num_lower, num_special;

  if (*password == '\0')
    return 0;

  if ( strlen(password) > 160 ) {
#ifndef STANDALONE
    notify_quiet(player, "The password must be less than 160 characters long.");
#endif
    return 0;
  }

  num_upper = num_lower = num_special = 0;
  for (scan = password; *scan; scan++) {
    if (!isprint((int)*scan) || isspace((int)*scan)) {
      return 0;
    }
    if (isupper((int)*scan))
       num_upper++;
    else if (islower((int)*scan))
       num_lower++;
    else if ((*scan != '\'') && (*scan != '-'))
       num_special++;
  }

  /* Needed.  Change it if you like, but be sure yours is the same. */
  if (((strlen(password) == 13) || strlen(password) == 22)
      &&
      (password[0] == 'X') && (password[1] == 'X'))
    return 0;

/* Some of the following borrowed from TinyMUSH 2.2.4 */
/* Key is a toggle to say if you want the return or not to the player */
#ifndef STANDALONE
  if ( mudconf.safer_passwords && (strcmp(password, "guest") != 0) && (strcmp(password, "Nyctasia") != 0) ) {
     /* length must be 5 or more */
     if ( strlen(password) < 5 ) {
        if ( (key == 0) && Good_chk(player) )
           notify_quiet(player, "The password must be at least 5 characters long.");
        return 0;
     }
     if (num_upper < 1) {
        if ( (key == 0) && Good_chk(player) )
           notify_quiet(player, "The password must contain at least one capital letter.");
        return 0;
     }
     if (num_lower < 1) {
        if ( (key == 0) && Good_chk(player) )
           notify_quiet(player, "The password must contain at least one lowercase letter.");
        return 0;
     }
     if (num_special < 1) {
        if ( (key == 0) && Good_chk(player) )
           notify_quiet(player,
           "The password must contain at least one number or a symbol other than an apostrophe or dash.");
        return 0;
     }                        
  }
#endif
  return 1;
}

#ifndef STANDALONE

/* ---------------------------------------------------------------------------
 * handle_ears: Generate the 'grows ears' and 'loses ears' messages.
 */

void handle_ears (dbref thing, int could_hear, int can_hear)
{
char	*buff, *bp, *tpr_buff, *tprp_buff;
int	gender;
static const char *poss[5] = {"", "its", "her", "his", "their"};

	if (OCloak(thing)) return;
	if (!could_hear && can_hear) {
		buff = alloc_lbuf("handle_ears.grow");
		strcpy(buff, Name(thing));
		if (isExit(thing)) {
			for (bp=buff; *bp && (*bp!=';'); bp++) ;
			*bp = '\0';
		}

		gender = get_gender(thing);
                tprp_buff = tpr_buff = alloc_lbuf("handle_ears");
		notify_check(thing, thing, 
			safe_tprintf(tpr_buff, &tprp_buff, "%s grow%s ears and can now hear.",
				buff, (gender == 4) ? "" : "s"), 0,
				(MSG_ME|MSG_NBR|MSG_LOC|MSG_INV), 0);
                free_lbuf(tpr_buff);
		free_lbuf(buff);
	} else if (could_hear && !can_hear) {
		buff = alloc_lbuf("handle_ears.lose");
		strcpy(buff, Name(thing));
		if (isExit(thing)) {
			for (bp=buff; *bp && (*bp!=';'); bp++) ;
			*bp = '\0';
		}
		
		gender = get_gender(thing);
                tprp_buff = tpr_buff = alloc_lbuf("handle_ears");
		notify_check(thing, thing, 
			safe_tprintf(tpr_buff, &tprp_buff, "%s lose%s %s ears and become%s deaf.",
				buff, (gender == 4) ? "" : "s",
				poss[gender], (gender == 4) ? "" : "s"), 0,
				(MSG_ME|MSG_NBR|MSG_LOC|MSG_INV), 0);
                free_lbuf(tpr_buff);
		free_lbuf(buff);
	}
}

/* for lack of better place the @switch code is here */

void do_switch (dbref player, dbref cause, int key, char *expr, 
		char *args[], int nargs, char *cargs[], int ncargs)
{
   int a, any, chkwild, i_regexp, i_case, i_notify, i_inline, x,
        i_nobreak, i_breakst, i_localize, i_clearreg;
   char *cp, *s_buff, *s_buffptr, *buff, *retbuff, *s_switch_notify, *pt, *savereg[MAX_GLOBAL_REGS],
        *npt, *saveregname[MAX_GLOBAL_REGS];

   if (!expr || (nargs <= 0))
      return;

   i_case = i_notify = i_inline = i_nobreak = i_localize = i_clearreg = i_breakst = 0;

   if ( key & SWITCH_INLINE) {
      key = key & ~SWITCH_INLINE;
      i_inline = 1;
   }
   if ( !i_inline && ((key & SWITCH_NOBREAK) || (key & SWITCH_LOCALIZE) ||
                      (key & SWITCH_CLEARREG)) ) {
      notify(player, "Illegal combination of switches.");
      return;
   }
   if ( key & SWITCH_NOBREAK ) {
      key = key & ~SWITCH_NOBREAK;
      i_nobreak = 1;
   }
   if ( key & SWITCH_LOCALIZE ) {
      key = key & ~SWITCH_LOCALIZE;
      i_localize = 1;
   }
   if ( key & SWITCH_CLEARREG ) {
      key = key & ~SWITCH_CLEARREG;
      i_clearreg = 1;
   }
   if (key & SWITCH_NOTIFY) {
      key = key & ~SWITCH_NOTIFY;
      i_notify = 1;
   }
   if (key & SWITCH_CASE) {
      key = key & ~(SWITCH_CASE|0x80000000);
      i_case = 1;
   }

   if (key == SWITCH_DEFAULT) {
      if (mudconf.switch_df_all) 
         key = SWITCH_ANY;
      else
         key = SWITCH_ONE;
   }

   /* now try a wild card match of buff with stuff in coms */
   any = chkwild = i_regexp = 0;
   if (key == SWITCH_REGANY) {
      key = SWITCH_ANY;
      i_regexp = 1;
   } else if ( key == SWITCH_REGONE) {
      key = SWITCH_ONE;
      i_regexp = 1;
   }
   for (a=0; (a<(nargs-1)) && args[a] && args[a+1]; a+=2) {
      buff = exec(player, cause, cause, EV_FCHECK|EV_EVAL|EV_TOP, args[a],
                  cargs, ncargs, (char **)NULL, 0);
      if ( PCRE_EXEC && i_regexp ) {
         chkwild = regexp_wild_match(buff, expr, (char **)NULL, 0, 1);
      } else {
         if ( i_case )
            chkwild = (strcmp(buff, expr) == 0);
         else
            chkwild = wild_match(buff, expr, (char **)NULL, 0, 1);
      }
      if (chkwild) {
         if ( mudconf.switch_substitutions ) {
            retbuff = replace_tokens(args[a+1], NULL, NULL, expr);
            if ( i_inline ) {
               if ( i_clearreg || i_localize ) {
                  for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                     savereg[x] = alloc_lbuf("ulocal_reg");
                     saveregname[x] = alloc_sbuf("ulocal_regname");
                     pt = savereg[x];
                     npt = saveregname[x];
                     safe_str(mudstate.global_regs[x],savereg[x],&pt);
                     safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                     if ( i_clearreg ) {
                        *mudstate.global_regs[x] = '\0';
                        *mudstate.global_regsname[x] = '\0';
                     }
                  }
               }
               i_breakst = mudstate.breakst;
               s_buffptr = s_buff = alloc_lbuf("switch_inline");
               strcpy(s_buffptr, retbuff);
               while (s_buffptr) {
                  if ( mudstate.breakst )
                     break;
                  cp = parse_to(&s_buffptr, ';', 0);
                  if (cp && *cp) {
                     process_command(player, cause, 0, cp, cargs, ncargs, 0, mudstate.no_hook);
                  }
               }
               free_lbuf(s_buff);
               if ( (desc_in_use != NULL) || i_nobreak ) {
                  mudstate.breakst = i_breakst;
               }
               if ( i_clearreg || i_localize ) {
                  for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                     pt = mudstate.global_regs[x];
                     npt = mudstate.global_regsname[x];
                     safe_str(savereg[x],mudstate.global_regs[x],&pt);
                     safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                     free_lbuf(savereg[x]);
                     free_sbuf(saveregname[x]);
                  }
               }
            } else {
               wait_que(player, cause, 0, NOTHING, retbuff,
                        cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
            }
            free_lbuf(retbuff);
         } else {
            if ( i_inline ) {
               if ( i_clearreg || i_localize ) {
                  for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                     savereg[x] = alloc_lbuf("ulocal_reg");
                     saveregname[x] = alloc_sbuf("ulocal_regname");
                     pt = savereg[x];
                     npt = saveregname[x];
                     safe_str(mudstate.global_regs[x],savereg[x],&pt);
                     safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                     if ( i_clearreg ) {
                        *mudstate.global_regs[x] = '\0';
                        *mudstate.global_regsname[x] = '\0';
                     }
                  }
               }
               i_breakst = mudstate.breakst;
               s_buffptr = s_buff = alloc_lbuf("switch_inline");
               strcpy(s_buffptr, args[a+1]);
               while (s_buffptr) {
                  if ( mudstate.breakst )
                     break;
                  cp = parse_to(&s_buffptr, ';', 0);
                  if (cp && *cp) {
                     process_command(player, cause, 0, cp, cargs, ncargs, 0, mudstate.no_hook);
                  }
               }
               free_lbuf(s_buff);
               if ( (desc_in_use != NULL) || i_nobreak ) {
                  mudstate.breakst = i_breakst;
               }
               if ( i_clearreg || i_localize ) {
                  for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                     pt = mudstate.global_regs[x];
                     npt = mudstate.global_regsname[x];
                     safe_str(savereg[x],mudstate.global_regs[x],&pt);
                     safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                     free_lbuf(savereg[x]);
                     free_sbuf(saveregname[x]);
                  }
               }
            } else {
               wait_que(player, cause, 0, NOTHING, args[a+1],
                        cargs, ncargs, mudstate.global_regs, mudstate.global_regsname);
            }
         }
         if (key == SWITCH_ONE) {
            free_lbuf(buff);
            return;
         }
         any = 1;
      }
      free_lbuf(buff);
   }
   if ((a < nargs) && !any && args[a]) {
      if ( mudconf.switch_substitutions ) {
         retbuff = replace_tokens(args[a], NULL, NULL, expr);
         if ( i_inline ) {
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                  savereg[x] = alloc_lbuf("ulocal_reg");
                  saveregname[x] = alloc_sbuf("ulocal_regname");
                  pt = savereg[x];
                  npt = saveregname[x];
                  safe_str(mudstate.global_regs[x],savereg[x],&pt);
                  safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                  if ( i_clearreg ) {
                     *mudstate.global_regs[x] = '\0';
                     *mudstate.global_regsname[x] = '\0';
                  }
               }
            }
            i_breakst = mudstate.breakst;
            s_buffptr = s_buff = alloc_lbuf("switch_inline");
            strcpy(s_buffptr, retbuff);
            while (s_buffptr) {
               if ( mudstate.breakst )
                  break;
               cp = parse_to(&s_buffptr, ';', 0);
               if (cp && *cp) {
                  process_command(player, cause, 0, cp, cargs, ncargs, 0, mudstate.no_hook);
               }
            }
            free_lbuf(s_buff);
            if ( (desc_in_use != NULL) || i_nobreak ) {
               mudstate.breakst = i_breakst;
            }
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                  pt = mudstate.global_regs[x];
                  npt = mudstate.global_regsname[x];
                  safe_str(savereg[x],mudstate.global_regs[x],&pt);
                  safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                  free_lbuf(savereg[x]);
                  free_sbuf(saveregname[x]);
               }
            }
         } else {
            wait_que(player, cause, 0, NOTHING, retbuff, cargs, ncargs,
                     mudstate.global_regs, mudstate.global_regsname);
         }
         free_lbuf(retbuff);
      } else {
         if ( i_inline ) {
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                  savereg[x] = alloc_lbuf("ulocal_reg");
                  saveregname[x] = alloc_sbuf("ulocal_regname");
                  pt = savereg[x];
                  npt = saveregname[x];
                  safe_str(mudstate.global_regs[x],savereg[x],&pt);
                  safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                  if ( i_clearreg ) {
                     *mudstate.global_regs[x] = '\0';
                     *mudstate.global_regsname[x] = '\0';
                  }
               }
            }
            i_breakst = mudstate.breakst;
            s_buffptr = s_buff = alloc_lbuf("switch_inline");
            strcpy(s_buffptr, args[a]);
            while (s_buffptr) {
               if ( mudstate.breakst )
                  break;
               cp = parse_to(&s_buffptr, ';', 0);
               if (cp && *cp) {
                  process_command(player, cause, 0, cp, cargs, ncargs, 0, mudstate.no_hook);
               }
            }
            free_lbuf(s_buff);
            if ( (desc_in_use != NULL) || i_nobreak ) {
               mudstate.breakst = i_breakst;
            }
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                  pt = mudstate.global_regs[x];
                  npt = mudstate.global_regsname[x];
                  safe_str(savereg[x],mudstate.global_regs[x],&pt);
                  safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                  free_lbuf(savereg[x]);
                  free_sbuf(saveregname[x]);
               }
            }
         } else {
            wait_que(player, cause, 0, NOTHING, args[a], cargs, ncargs,
                     mudstate.global_regs, mudstate.global_regsname);
         }
      }
   }
   if ( i_notify ) {
      s_switch_notify = alloc_lbuf("switch_notify");
      sprintf(s_switch_notify, "#%d", player);
      do_notify(player, cause, NFY_QUIET, s_switch_notify, (char *)NULL);
      free_lbuf(s_switch_notify);
   }
}

/* ---------------------------------------------------------------------------
 * do_comment: Implement the @@ (comment) command. Very cpu-intensive :-)
 */

void do_comment (dbref player, dbref cause, int key)
{
}

/* Taken from TinyMUSH3.  Works like @@-comment, but evals */
void do_eval(dbref player, dbref cause, int key, char *str)
{
}

static dbref promote_dflt (dbref old, dbref new)
{
	switch (new) {
	case NOPERM:
		return NOPERM;
	case AMBIGUOUS:
		if (old == NOPERM)
			return old;
		else
			return new;
	}

	if ((old == NOPERM) || (old == AMBIGUOUS))
		return old;

	return NOTHING;
}

dbref match_possessed (dbref player, dbref thing, char *target, dbref dflt, 
			int check_enter)
{
dbref	result, result1;
int	control;
char	*buff, *start, *place, *s1, *d1, *temp;

	/* First, check normally */

	if (Good_obj(dflt))
		return dflt;

	/* Didn't find it directly.  Recursively do a contents check */

	start = target;
	while (*target) {

		/* Fail if no ' characters */

		place = target;
		target = (char *)index(place, '\'');
		if ((target == NULL) || !*target)
			return dflt;

		/* If string started with a ', skip past it */

		if (place == target) {
			target++;
			continue;
		}

		/* If next character is not an s or a space, skip past */

		temp = target++;
		if (!*target)
			return dflt;
		if ((*target != 's') && (*target != 'S') && (*target != ' '))
			continue;

		/* If character was not a space make sure the following
		 * character is a space.
		 */

		if (*target != ' ') {
			target++;
			if (!*target)
				return dflt;
			if (*target != ' ')
				continue;
		}

		/* Copy the container name to a new buffer so we can
		 * terminate it.
		 */

		buff = alloc_lbuf("is_posess");
		for (s1=start,d1=buff; *s1&&(s1<temp); *d1++=(*s1++)) ;
		*d1 = '\0';

		/* Look for the container here and in our inventory.  Skip
		 * past if we can't find it.
		 */

		init_match(thing, buff, NOTYPE);
		if (player == thing) {
			match_neighbor();
			match_possession();
		} else {
			match_possession();
		}
		result1 = match_result();

		free_lbuf(buff);
		if (!Good_obj(result1)) {
			dflt = promote_dflt(dflt, result1);
			continue;
		}

		/* If we don't control it and it is either dark or opaque,
		 * skip past.
		 */

		control = Controls(player, result1);
		if ((Dark(result1) || Opaque(result1)) && !control) {
			dflt = promote_dflt(dflt, NOTHING);
			continue;
		}

		/* Validate object has the ENTER bit set, if requested */

		if ((check_enter) && !Enter_ok(result1) && !control) {
			dflt = promote_dflt(dflt, NOPERM);
			continue;
		}

		/* Look for the object in the container */

		init_match(result1, target, NOTYPE);
		match_possession();
		result = match_result();
		result = match_possessed(player, result1, target, result,
			check_enter);
		if (Good_obj(result))
			return result;
		dflt = promote_dflt(dflt, result);
	}
	return dflt;
}

/* ---------------------------------------------------------------------------
 * parse_range: break up <what>,<low>,<high> syntax
 */

void parse_range (char **name, dbref *low_bound, dbref *high_bound)
{
char	*buff1, *buff2;

	buff1 = *name;
	if (buff1 && *buff1)
		*name = parse_to(&buff1, ',', EV_STRIP_TS);
	if (buff1 && *buff1) {
		buff2 = parse_to(&buff1, ',', EV_STRIP_TS);
		if (buff1 && *buff1) {
			while (*buff1 && isspace((int)*buff1)) buff1++;
			if (*buff1 == NUMBER_TOKEN) buff1++;
			*high_bound = atoi(buff1);
			if (*high_bound >= mudstate.db_top)
				*high_bound = mudstate.db_top - 1;
		} else {
			*high_bound = mudstate.db_top - 1;
		}
		while (*buff2 && isspace((int)*buff2)) buff2++;
		if (*buff2 == NUMBER_TOKEN) buff2++;
		*low_bound = atoi(buff2);
		if (*low_bound < 0)
			*low_bound = 0;
	} else {
		*low_bound = 0;
		*high_bound = mudstate.db_top - 1;
	}
}			

int parse_thing_slash (dbref player, char *thing, char **after, dbref *it)
{
char	*str;

	/* get name up to / */
	for (str=thing; *str && (*str!='/'); str++) ;

	/* If no / in string, return failure */

	if (!*str) {
		*after = NULL;
		*it = NOTHING;
		return 0;
	}
	*str++ = '\0';
	*after = str;

	/* Look for the object */

	init_match(player, thing, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	*it = match_result();

	/* Return status of search */

	return (Good_obj(*it));
}

extern NAMETAB lock_sw[];

int get_obj_and_lock (dbref player, char *what, dbref *it, ATTR **attr, 
			char *errmsg, char **errmsgcx)
{
char	*str, *tbuf;
int	anum;

	tbuf = alloc_lbuf("get_obj_and_lock");
	strcpy(tbuf, what);
	if (parse_thing_slash(player, tbuf, &str, it)) {

		/* <obj>/<lock> syntax, use the named lock */

		anum = search_nametab(player, lock_sw, str);
		if (anum == -1) {
			free_lbuf(tbuf);
			safe_str("#-1 LOCK NOT FOUND", errmsg, errmsgcx);
			return 0;
		}
	} else {

		/* Not <obj>/<lock>, do a normal get of the default lock */

		*it = match_thing(player, what);
		if (!Good_obj(*it)) {
			free_lbuf(tbuf);
			safe_str("#-1 NOT FOUND", errmsg, errmsgcx);
			return 0;
		}
		anum = A_LOCK;
	}

	/* Get the attribute definition, fail if not found */

	free_lbuf(tbuf);
	*attr = atr_num(anum);
	if (!(*attr)) {
		safe_str("#-1 LOCK NOT FOUND", errmsg, errmsgcx);
		return 0;
	}

	return 1;
}

#endif	/* STANDALONE */

/* ---------------------------------------------------------------------------
 * where_is: Returns place where obj is linked into a list.
 * ie. location for players/things, source for exits, NOTHING for rooms.
 */

dbref where_is (dbref what)
{
dbref	loc;

	if (!Good_obj(what))
		return NOTHING;

	switch (Typeof(what)) {
	case TYPE_PLAYER:
	case TYPE_THING:
	case TYPE_ZONE:
		loc = Location(what);
		break;
	case TYPE_EXIT:
		loc = Exits(what);
		break;
	default:
		loc = NOTHING;
		break;
	}
	return loc;
}

/* ---------------------------------------------------------------------------
 * where_room: Return room containing player, or NOTHING if no room or
 * recursion exceeded.  If player is a room, returns itself.
 */

dbref where_room (dbref what)
{
int	count;

	for (count=mudconf.ntfy_nest_lim; count>0; count--) {
		if (!Good_obj(what))
			break;
		if (isRoom(what))
			return what;
		if (!Has_location(what))
			break;
		what = Location(what);
	}
	return NOTHING;
}

int locatable(dbref player, dbref it, dbref cause)
{
dbref	loc_it, room_it;
int	findable_room;

	/* No sense if trying to locate a bad object */

	if (!Good_obj(it))
		return 0;

	loc_it = where_is(it);

	/* Succeed if we can examine the target, if we are the target,
	 * if we can examine the location, if a wizard caused the lookup,
	 * or if the target caused the lookup.
	 */

	if (Examinable(player, it) ||
	    (loc_it == player) ||
	    ((loc_it != NOTHING) &&
	     (Examinable(player, loc_it) || loc_it == where_is(player))) ||
  	    (it == (mudconf.enforce_unfindable ? player : cause))) {
              if ((Cloak(it) && !Examinable(player,it) && !Wizard(player)) ||
		 (Cloak(it) && SCloak(it) && !Immortal(player))) 
		return 0;
	      else
		return 1;
	    }
	room_it = where_room(it);
	if (Good_obj(room_it))
		findable_room = !Hideout(room_it);
	else
		findable_room = 1;

	/* Succeed if we control the containing room or if the target is
	 * findable and the containing room is not unfindable.
	 */

	if (((room_it != NOTHING) && Examinable(player, room_it)) ||
	    (Findable(it) && findable_room)) {
                if ((Cloak(it) && !Examinable(player,it) && !Wizard(player)) ||
		   (Cloak(it) && SCloak(it) && !Immortal(player))) 
		  return 0;
		else
		  return 1;

	}

	if (Wizard(player) && !(Unfindable(it) && Hidden(it) && SCloak(it))) {
	  if (!(Unfindable(room_it) && Hidden(room_it) && SCloak(room_it)))
	    return 1;
	}

	/* We can't do it. */

	return 0;
}

/* ---------------------------------------------------------------------------
 * nearby: Check if thing is nearby player (in inventory, in same room, or
 * IS the room.
 */

int nearby (dbref player, dbref thing)
{
        int thing_loc, player_loc;

	if (!Good_obj(player) || !Good_obj(thing))
		return 0;
	thing_loc = where_is(thing);
	if (thing_loc == player)
		return 1;
	player_loc = where_is(player);
	if ((thing_loc == player_loc) || (thing == player_loc))
		return 1;
	return 0;
}

/* ---------------------------------------------------------------------------
 * exit_visible, exit_displayable: Is exit visible?
 */

int exit_visible(dbref exit, dbref player, int key)	/* exit visible to lexits() */
{
	if (Unfindable(exit) && Hidden(exit) && Wizard(Owner(exit)) && !Controls(player,Owner(exit))) {
              return 0;
        }
#ifndef STANDALONE
#ifdef REALITY_LEVELS
        if (!IsReal(player, exit))
                return 0;
#endif /* REALITY_LEVELS */
#endif
	if (key & VE_LOC_XAM)		return 1;	/* Exam exit's loc */
	if (Examinable(player, exit))	return 1;	/* Exam exit */
	if (Light(exit))		return 1;	/* Exit is light */
	if (key & (VE_LOC_DARK|VE_BASE_DARK))
					return 0;	/* Dark Loc or base */
	if (Dark(exit)) {
#ifndef STANDALONE
           if ( !could_doit(player, exit, A_LDARK, 0, 0) )
#endif
              return 0;	/* Dark exit */
        }
	return 1;					/* Default */
}

int exit_displayable(dbref exit, dbref player, int key)	/* exit visible to look */
{
	if (Dark(exit)) {
	  if (Unfindable(exit) && Hidden(exit) && SCloak(exit) && 
		Immortal(Owner(exit)) && !Immortal(player)) {
	    if (key & VE_ACK)
	      return 1;
	    else
	      return 0;
	  }
	  else if (Unfindable(exit) && Hidden(exit) && Wizard(Owner(exit)) && !Wizard(player)) {
	    if (key & VE_ACK)
	      return 1;
	    else
	      return 0;
	  }
#ifndef STANDALONE
          else if ( could_doit(player, exit, A_LDARK, 0, 0) )
            return 1;
#endif
	  else
	    return 0;
	}
	if (Light(exit))		return 1;	/* Light exit */
	if (key & (VE_LOC_DARK|VE_BASE_DARK)) {
	  if (Unfindable(exit) && Hidden(exit) && SCloak(exit) && 
		Immortal(Owner(exit)) && !Immortal(player)) {
	    if (key & VE_ACK)
	      return 1;
	    else
	      return 0;
	  }
	  else if (Unfindable(exit) && Hidden(exit) && Wizard(Owner(exit)) && !Wizard(player)) {
	    if (key & VE_ACK)
	      return 1;
	    else
	      return 0;
	  }
#ifndef STANDALONE
          else if ( could_doit(player, exit, A_LDARK, 0, 0) )
            return 1;
#endif
	  else
	    return 0;
	}
	return 1;					/* Default */
}
/* ---------------------------------------------------------------------------
 * next_exit: return next exit that is ok to see.
 */

dbref next_exit(dbref player, dbref this, int exam_here)
{
	if (isRoom(this))
		return NOTHING;
	if (isExit(this) && exam_here)
		return this;

	while ((this != NOTHING) && Dark(this) && !Light(this) &&
	       !Examinable(player, this))
		this = Next(this);

	return this;
}

#ifndef STANDALONE

/* ---------------------------------------------------------------------------
 * did_it: Have player do something to/with thing
 */

void did_it(dbref player, dbref thing, int what, const char *def, int owhat, 
	const char *odef, int awhat, char *args[], int nargs)
{
  char	*d, *buff, *act, *charges, *lcbuf, *pt, *npc_name, *savereg[MAX_GLOBAL_REGS], *master_str, *master_ret;
  char *pt2, *savereg2[MAX_GLOBAL_REGS], *tmpformat_buff, *tpr_buff, *tprp_buff,
       *npt2, *savereg2name[MAX_GLOBAL_REGS], *npt, *saveregname[MAX_GLOBAL_REGS];
  dbref	loc, aowner, aowner2, master, aowner3;
  int	num, aflags, cpustopper, nocandoforyou, x, aflags2, o_chkr, tst_attr, aflags3, chkoldstate, i_didsave;
  int   did_allocate_buff;
  ATTR *o_atr, *tst_glb, *format_atr;

	/* message to player */

        nocandoforyou=0;
	if (Toggles2(thing) & TOG_SILENTEFFECTS) {
           nocandoforyou=1;
	}

        mudstate.chkcpu_stopper = time(NULL);
        chkoldstate = mudstate.chkcpu_toggle;
	cpustopper = 0;
        did_allocate_buff = 0;
	/* Default code idea from TM3 */
	if ( TogNoDefault(thing) ) {
	   master = NOTHING;
        } else {
           switch (Typeof(thing)) {
               case TYPE_ROOM:
                   master = mudconf.room_defobj;
                   break;
               case TYPE_EXIT:
                   master = mudconf.exit_defobj;
                   break;
               case TYPE_PLAYER:
                   master = mudconf.player_defobj;
                   break;
               default:
                   master = mudconf.thing_defobj;
           }
           if (master == thing || !Good_obj(master) || Recover(master) || Going(master))
               master = NOTHING;
        }
        i_didsave = 0;
	if (what > 0) {
           if ( !(( (what == A_SUCC) || (what == A_FAIL) || (what == A_ENTER) || (what == A_LEAVE) ) &&
                  ( ((master != NOTHING) && SideFX(master)) || SideFX(thing) ) &&
                  ( (Cloak(player) && !Wizard(Owner(thing))) ||
                    ((Cloak(player) && SCloak(player)) &&
                    !Immortal(Owner(thing))) ))) {
                if ( mudconf.descs_are_local ) {
                   i_didsave = 1;
                   for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                      savereg2[x] = alloc_lbuf("ulocal_reg");
                      savereg2name[x] = alloc_sbuf("ulocal_regname");
                      pt2 = savereg2[x];
                      npt2 = savereg2name[x];
                      safe_str(mudstate.global_regs[x],savereg2[x],&pt2);
                      safe_str(mudstate.global_regsname[x],savereg2name[x],&npt2);
                   }
                }
		d = atr_pget(thing, what, &aowner, &aflags);
                tst_glb = atr_num(what);
		tst_attr = ((tst_glb && (tst_glb->flags & AF_DEFAULT)) || (aflags & AF_DEFAULT)) ? 1 : 0;
                if ( tst_attr ) {
                   master_str = NULL;
                   tmpformat_buff = alloc_sbuf("didit_format");
                   format_atr = atr_num(what);
                   if ( mudconf.format_compatibility )
                      sprintf(tmpformat_buff, "%.25sformat", format_atr->name);
                   else
                      sprintf(tmpformat_buff, "format%.25s", format_atr->name);
                   format_atr = atr_str(tmpformat_buff);
                   free_sbuf(tmpformat_buff);
                   did_allocate_buff = 0;
                   if ( format_atr ) {
                      master_str = atr_pget(thing, format_atr->number, &aowner3, &aflags3);
                      did_allocate_buff = 1;
                   }
                   if ( (!format_atr || !master_str || !*master_str) && (master != NOTHING) ) {
                      if ( master_str )
                         free_lbuf(master_str);
                      master_str = atr_pget(master, what, &aowner3, &aflags3);
                   }
                   if ( !master_str ) 
                      master_str = alloc_lbuf("master_filler");
                }
		if ( *d || (tst_attr && *master_str) ) {
		   if ( nocandoforyou ) {
			notify(player, "Message has been nullified.");
		   } else {
                        if ( mudconf.formats_are_local &&
                             ((what == A_LCON_FMT) ||
                              (what == A_LEXIT_FMT) ||
                              (what == A_LDEXIT_FMT)) ) {
                           for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                             savereg[x] = alloc_lbuf("ulocal_reg");
                             saveregname[x] = alloc_sbuf("ulocal_reg");
                             pt = savereg[x];
                             npt = saveregname[x];
                             safe_str(mudstate.global_regs[x],savereg[x],&pt);
                             safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                           }
                        }
			if ( tst_attr && *master_str ) {
                           if ( *d ) {
			      master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				                d, args, nargs, (char **)NULL, 0);
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, &master_ret, 1, (char **)NULL, 0);
                              free_lbuf(master_ret);
                           } else {
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, args, nargs, (char **)NULL, 0);
                           }
			} else {
			   buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				       d, args, nargs, (char **)NULL, 0);
                        }
                        if ( mudconf.formats_are_local &&
                              ((what == A_LCON_FMT) ||
                              (what == A_LEXIT_FMT) ||
                              (what == A_LDEXIT_FMT)) ) {
                           for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                             pt = mudstate.global_regs[x];
                             npt = mudstate.global_regsname[x];
                             safe_str(savereg[x],mudstate.global_regs[x],&pt);
                             safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                             free_lbuf(savereg[x]);
                             free_sbuf(saveregname[x]);
                           }
                        }
			notify(player, buff);
			free_lbuf(buff);
		   }
		} else if (def) {
		   if ( nocandoforyou ) {
			notify(player, "Message has been nullified.");
		   } else {
			notify(player, def);
		   }
		}
		cpustopper += mudstate.chkcpu_toggle;
                if ( tst_attr )
                   free_lbuf(master_str);
		free_lbuf(d);
                if ( (what == A_DESC) || (what == A_IDESC) ) {
		  d = atr_pget(thing, A_DESC2, &aowner, &aflags);
                  tst_glb = atr_num(A_DESC2);
                  tst_attr = ((tst_glb && (tst_glb->flags & AF_DEFAULT)) || (aflags & AF_DEFAULT)) ? 1 : 0;
                  if ( tst_attr ) {
                     master_str = NULL;
                     tmpformat_buff = alloc_sbuf("didit_format");
                     format_atr = atr_num(A_DESC2);
                     if ( mudconf.format_compatibility )
                        sprintf(tmpformat_buff, "%.25sformat", format_atr->name);
                     else
                        sprintf(tmpformat_buff, "format%.25s", format_atr->name);
                     format_atr = atr_str(tmpformat_buff);
                     free_sbuf(tmpformat_buff);
                     did_allocate_buff = 0;
                     if ( format_atr ) {
                        master_str = atr_pget(thing, format_atr->number, &aowner3, &aflags3);
                        did_allocate_buff = 1;
                     }
                     if ( (!format_atr || !master_str || !*master_str) && (master != NOTHING) ) {
                        if ( did_allocate_buff )
                           free_lbuf(master_str);
                        master_str = atr_pget(master, A_DESC2, &aowner3, &aflags3);
                     }
                     if ( !master_str )
                        master_str = alloc_lbuf("master_filler");
                  }
		  if ( *d || (tst_attr && *master_str) ) {
		    if ( nocandoforyou ) {
			notify(player, "Message has been nullified.");
		    } else {
			if ( tst_attr && *master_str ) {
			   if ( *d ) {
			      master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				                d, args, nargs, (char **)NULL, 0);
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, &master_ret, 1, (char **)NULL, 0);
                              free_lbuf(master_ret);
			   } else {
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, args, nargs, (char **)NULL, 0);
			   }
                        } else {
			   buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				      d, args, nargs, (char **)NULL, 0);
                        }
			notify(player, buff);
			free_lbuf(buff);
		    }
		  }
                  if ( tst_attr )
                     free_lbuf(master_str);
		  free_lbuf(d);
		}
		cpustopper += mudstate.chkcpu_toggle;
           } else {
                notify(player, "Sideeffect enabled target snuffed while wizcloaked.");
           }

	} else if ((what < 0) && def) {
	   if ( nocandoforyou ) {
		notify(player, "Message has been nullified.");
	   } else {
		notify(player, def);
	   }
	}

	/* message to neighbors */

      if ((!Cloak(player) && (!NCloak(player) || (NCloak(player) && (player != thing)))) ||
	((NCloak(player) || Cloak(player)) && (Immortal(Owner(thing)))) ||
        (awhat == A_STARTUP)) {
	if ((owhat > 0) && Has_location(player) &&
	    Good_obj(loc = Location(player))) {
		d = atr_pget(thing, owhat, &aowner, &aflags);

                tst_glb = atr_num(owhat);
                tst_attr = ((tst_glb && (tst_glb->flags & AF_DEFAULT)) || (aflags & AF_DEFAULT)) ? 1 : 0;
                if ( tst_attr ) {
                   master_str = NULL;
                   tmpformat_buff = alloc_sbuf("didit_format");
                   format_atr = atr_num(owhat);
                   if ( mudconf.format_compatibility )
                      sprintf(tmpformat_buff, "%.25sformat", format_atr->name);
                   else
                      sprintf(tmpformat_buff, "format%.25s", format_atr->name);
                   format_atr = atr_str(tmpformat_buff);
                   free_sbuf(tmpformat_buff);
                   did_allocate_buff = 0;
                   if ( format_atr ) {
                      master_str = atr_pget(thing, format_atr->number, &aowner3, &aflags3);
                      did_allocate_buff = 1;
                   }
                   if ( (!format_atr || !master_str || !*master_str) && (master != NOTHING) ) {
                      if ( did_allocate_buff )
                         free_lbuf(master_str);
                      master_str = atr_pget(master, owhat, &aowner3, &aflags3);
                   }
                   if ( !master_str )
                      master_str = alloc_lbuf("master_filler");
                }
		if ( *d || (tst_attr && *master_str) ) {
		   if ( !nocandoforyou ) {
                        if ( tst_attr && *master_str ) {
			   if ( *d ) {
			      master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				                d, args, nargs, (char **)NULL, 0);
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, &master_ret, 1, (char **)NULL, 0);
                              free_lbuf(master_ret);
			   } else {
			      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				          master_str, args, nargs, (char **)NULL, 0);
			   }
                        } else {
			   buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
				      d, args, nargs, (char **)NULL, 0);
                        }
			if ( *buff ) {
			  if ( mudconf.oattr_enable_altname &&
			       ((owhat == A_ODROP) || (owhat == A_OSUCC) || (owhat == A_OFAIL)) ) {
			    o_chkr = 0;
			    if ( strlen(mudconf.oattr_uses_altname) > 0 )
			      o_atr = atr_str(mudconf.oattr_uses_altname);
			    else
			      o_atr = atr_str("_NPC");
			    npc_name = NULL;
			    if ( o_atr ) {
			      npc_name = atr_get(player, o_atr->number, &aowner2, &aflags2);
			      if ( npc_name && *npc_name )
				o_chkr = 1;
			    }
                            tprp_buff = tpr_buff = alloc_lbuf("did_it");
#ifdef REALITY_LEVELS
			    notify_except2_rlevel2(loc, player, player, thing,
						   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", (o_chkr ? npc_name : Name(player)), buff));
#else
			    notify_except2(loc, player, player, thing,
					   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", (o_chkr ? npc_name : Name(player)), buff));
#endif /* REALITY_LEVELS */
                            free_lbuf(tpr_buff);
			    if ( npc_name )
			      free_lbuf(npc_name);
			  } else {

                            tprp_buff = tpr_buff = alloc_lbuf("did_it");
#ifdef REALITY_LEVELS
			    notify_except2_rlevel2(loc, player, player, thing,
						   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), buff));
#else
			    notify_except2(loc, player, player, thing,
					   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), buff));
#endif /* REALITY_LEVELS */
                            free_lbuf(tpr_buff);
			  } /* If valid O-Attribute */
			} /* If *buff exists */
			free_lbuf(buff);
		      }
		   } else if (odef) {
		     if ( !nocandoforyou ) {
                       tprp_buff = tpr_buff = alloc_lbuf("did_it");
#ifdef REALITY_LEVELS
		       notify_except2_rlevel2(loc, player, player, thing,
					      safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
#else
		       notify_except2(loc, player, player, thing,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
#endif /* REALITY_LEVELS */
                       free_lbuf(tpr_buff);
		     }
		   }
		   cpustopper += mudstate.chkcpu_toggle;
                   if ( tst_attr )
                      free_lbuf(master_str);
		   free_lbuf(d);
		} else if ((owhat < 0) && odef && Has_location(player) &&
			   Good_obj(loc = Location(player))) {
		  if ( !nocandoforyou ) {
                    tprp_buff = tpr_buff = alloc_lbuf("did_it");
#ifdef REALITY_LEVELS
		    notify_except2_rlevel2(loc, player, player, thing,
					   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
#else
		    notify_except2(loc, player, player, thing,
				   safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
#endif /* REALITY_LEVELS */
                    free_lbuf(tpr_buff);
		  }
		}		
		
	/* do the action attribute */
        if ( i_didsave && mudconf.descs_are_local ) {
           i_didsave = 0;
           for (x = 0; x < MAX_GLOBAL_REGS; x++) {
              pt2 = mudstate.global_regs[x];
              npt2 = mudstate.global_regsname[x];
              safe_str(savereg2[x],mudstate.global_regs[x],&pt2);
              safe_str(savereg2name[x],mudstate.global_regsname[x],&npt2);
              free_lbuf(savereg2[x]);
              free_sbuf(savereg2name[x]);
           }
        }

#ifdef REALITY_LEVELS
        if (awhat > 0 && IsReal(thing, player)) {
#else
        if (awhat > 0) {
#endif /* REALITY_LEVELS */
		if (*(act = atr_pget(thing, awhat, &aowner, &aflags))) {
                        if ( (awhat == A_AHEAR) || (awhat == A_AAHEAR) || (awhat == A_AMHEAR) ) {
                           mudstate.ahear_count++;
                           mudstate.ahear_lastplr = thing;
                           if ( (mudstate.ahear_currtime + mudconf.ahear_maxtime) < time(NULL) ) {
                              mudstate.ahear_count = 0;
                              mudstate.ahear_currtime = time(NULL);
                           }
                        }
			charges = atr_pget(thing, A_CHARGES, &aowner, &aflags);
			if (*charges && !mudstate.chkcpu_toggle) {
				num = atoi(charges);
				if (num > 0) {
					buff = alloc_sbuf("did_it.charges");
					sprintf(buff, "%d", num - 1);
					atr_add_raw(thing, A_CHARGES, buff);
					free_sbuf(buff);
				} else if (*(buff = atr_pget(thing, A_RUNOUT, &aowner, &aflags))) {
					free_lbuf(act);
					act = buff;
				} else {
					free_lbuf(act);
					free_lbuf(buff);
					free_lbuf(charges);
                                        mudstate.chkcpu_toggle = chkoldstate;
					return;
				}
			}
			free_lbuf(charges);
                        if ( !mudstate.chkcpu_toggle ) {
			   wait_que(thing, player, 0, NOTHING, act, args, nargs,
				   mudstate.global_regs, mudstate.global_regsname);
                        }
		}
		free_lbuf(act);
		if (awhat == A_ADESC) {
		  act = atr_pget(thing, A_ADESC2, &aowner, &aflags);
		  if (*act && !mudstate.chkcpu_toggle) {
		    wait_que(thing, player, 0, NOTHING, act, args, nargs, 
                             mudstate.global_regs, mudstate.global_regsname);
                  }
		  free_lbuf(act);
		}
	}
      }
      cpustopper += mudstate.chkcpu_toggle;
      if ( cpustopper && !chkoldstate ) {
           if ( mudstate.curr_cpu_user != thing ) {
              mudstate.curr_cpu_user = thing;
              mudstate.curr_cpu_cycle = 0;
           }
           mudstate.curr_cpu_cycle++;
           notify(player, "Overload on CPU detected.  Process aborted.");
           broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS",
                                          (char *)"(built-in-attr)", NULL, thing, 0, 0, NULL);
           lcbuf = alloc_lbuf("process_commands.LOG.badcpu");
           STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
           log_name_and_loc(player);
           sprintf(lcbuf, " CPU overload: %.3900s on #%d", "(built-in-attr)", thing);
           log_text(lcbuf);
           if ( mudstate.curr_cpu_cycle >= mudconf.max_cpu_cycles ) {
              if (Good_obj(thing)) {
                 tprp_buff = tpr_buff = alloc_lbuf("did_it");
                 do_halt(thing, thing, 0, safe_tprintf(tpr_buff, &tprp_buff, "#%d", thing));
                 free_lbuf(tpr_buff);
                 s_Toggles2(thing, (Toggles2(thing) | TOG_SILENTEFFECTS));
	         s_Flags(thing, (Flags(thing) | HALT));
                 s_Toggles(thing, (Toggles(thing) & ~TOG_FORCEHALTED));
                 mudstate.breakst = 1;
              }
              log_text(" - Object toggled SILENTEFFECT and HALTED.");
              broadcast_monitor(player, MF_CPUEXT, "MULTIPLE CPU RUNAWAYS - INTERNAL ATTRIB",
                               (char *)"Object @toggled SILENTEFFECT and HALTED", NULL, thing, 0, 0, NULL);
              mudstate.last_cpu_user = mudstate.curr_cpu_user;
              mudstate.curr_cpu_user = NOTHING;
              mudstate.curr_cpu_cycle=0;
           }
           free_lbuf(lcbuf);
           ENDLOG
      }
      if ( i_didsave && mudconf.descs_are_local ) {
         i_didsave = 0;
         for (x = 0; x < MAX_GLOBAL_REGS; x++) {
            pt2 = mudstate.global_regs[x];
            npt2 = mudstate.global_regsname[x];
            safe_str(savereg2[x],mudstate.global_regs[x],&pt2);
            safe_str(savereg2name[x],mudstate.global_regsname[x],&npt2);
            free_lbuf(savereg2[x]);
            free_sbuf(savereg2name[x]);
         }
      }
      mudstate.chkcpu_toggle = chkoldstate;
}

/* ---------------------------------------------------------------------------
 * do_verb: Command interface to did_it.
 */

void do_verb (dbref player, dbref cause, int key, char *victim_str, 
		char *args[], int nargs)
{
dbref	actor, victim, aowner;
int	what, owhat, awhat, nxargs, myrestrict, aflags, i;
ATTR	*ap;
const char *whatd, *owhatd;
char	*xargs[10];

	/* Look for the victim */

	if (!victim_str || !*victim_str) {
		notify(player, "Nothing to do.");
		return;
	}

	/* Get the victim */

	init_match(player, victim_str, NOTYPE);
	match_everything(MAT_EXIT_PARENTS);
	victim = noisy_match_result();
	if (!Good_obj(victim))
		return;

	/* Get the actor.  Default is my cause */

	if ((nargs >= 1) && args[0] && *args[0]) {
		init_match(player, args[0], NOTYPE);
		match_everything(MAT_EXIT_PARENTS);
		actor = noisy_match_result();
		if (!Good_obj(actor))
			return;
	} else {
		actor = cause;
	}

	/* Check permissions.  There are two possibilities
	 * 1: Player controls both victim and actor.  In this case victim runs
	 *    his action list.
	 * 2: Player controls actor.  In this case victim does not run his
	 *    action list and any attributes that player cannot read from
	 *    victim are defaulted.
	 */

	if (!controls(player, actor)) {
		notify_quiet(player, "Permission denied,");
		return;
	}
	myrestrict = !controls(player, victim);

	what = -1;
	owhat = -1;
	awhat = -1;
	whatd = NULL;
	owhatd = NULL;
	nxargs = 0;

	/* Get invoker message attribute */

	if (nargs >= 2) {
		ap = atr_str(args[1]);
		if (ap && (ap->number > 0))
			what = ap->number;
	}

	/* Get invoker message default */

	if ((nargs >= 3) && args[2] && *args[2]) {
		whatd = args[2];
	}

	/* Get others message attribute */

	if (nargs >= 4) {
		ap = atr_str(args[3]);
		if (ap && (ap->number > 0))
			owhat = ap->number;
	}

	/* Get others message default */

	if ((nargs >= 5) && args[4] && *args[4]) {
		owhatd = args[4];
	}

	/* Get action attribute */

	if (nargs >= 6) {
		ap = atr_str(args[5]);
		if (ap)
			awhat = ap->number;
	}

	/* Get arguments */

	if (nargs >= 7) {
		parse_arglist(victim, actor, actor, args[6], '\0',
			EV_STRIP_LS|EV_STRIP_TS, xargs, 10, (char **)NULL, 0, 0, (char **)NULL, 0);
		for (nxargs=0; (nxargs<10) && xargs[nxargs]; nxargs++) ;
	}

	/* If player doesn't control both, enforce visibility myrestrictions */

	if (myrestrict) {
		ap = NULL;
		if (what != -1) {
			atr_get_info(victim, what, &aowner, &aflags);
			ap = atr_num(what);
		}
		if (!ap || !Read_attr(player, victim, ap, aowner, aflags, 0) ||
		    ((ap->number == A_DESC) && !mudconf.read_rem_desc &&
		     !Examinable(player, victim) && !nearby(player, victim)))
			what = -1;

		ap = NULL;
		if (owhat != -1) {
			atr_get_info(victim, owhat, &aowner, &aflags);
			ap = atr_num(owhat);
		}
		if (!ap || !Read_attr(player, victim, ap, aowner, aflags, 0) ||
		    ((ap->number == A_DESC) && !mudconf.read_rem_desc &&
		     !Examinable(player, victim) && !nearby(player, victim)))
			owhat = -1;

		awhat = 0;
	}

	/* Go do it */

	did_it(actor, victim, what, whatd, owhat, owhatd, awhat,
		xargs, nxargs);

	/* Free user args */

	for (i=0; i<nxargs; i++)
		free_lbuf(xargs[i]);

}

int evlevchk(dbref player, int ckl)
{
  int x;

  for (x = mudstate.evalnum - 1; x >= 0; x--) {
    if (mudstate.evaldb[x] == player) {
      mudstate.evalresult = (mudstate.evalstate[x] >= ckl);
      return 1;
    }
  }
  return 0;
}

#endif	/* STANDALONE */

#ifndef STANDALONE
int isreal_chk(dbref player, dbref thing, int real_bitwise) 
{
#ifdef REALITY_LEVELS
   int i_realitybitwise;

   if ( (player == thing) && !(real_bitwise == 0) )
      return 1;
   i_realitybitwise = TxLevel(thing);
   if ( real_bitwise != 0 )
      i_realitybitwise = real_bitwise;

   if ( (RxLevel(player) & i_realitybitwise) ) {
      if ( ((mudconf.reality_locktype < 2) || !mudconf.reality_locks) )
         return 1;
      if ( ((mudconf.reality_locktype > 1)  && !mudconf.reality_locks) )
         return 1;
#ifndef OLD_REALITIES
      if ( ((mudconf.reality_locktype == 2) && !ChkReality(thing)) )
         return 1;
      if ( ((mudconf.reality_locktype == 3) && !ChkReality(player)) )
         return 1;
#endif
   } else if ( mudconf.reality_locks && (mudconf.reality_locktype < 2) ) {
      mudstate.recurse_rlevel++;
      if ( mudstate.recurse_rlevel < 10 ) {
         if ( (mudconf.reality_locktype == 0) && ChkReality(thing) &&
               could_doit(player, thing, A_LUSER, 0, 0) ) {
            mudstate.recurse_rlevel--;
            return 1;
         } else if ( (mudconf.reality_locktype == 1) && ChkReality(player) &&
               could_doit(thing, player, A_LUSER, 0, 0) ) {
            mudstate.recurse_rlevel--;
            return 1;
         }
      }
      mudstate.recurse_rlevel--;
   }
   if ( RxLevel(player) & i_realitybitwise && (mudconf.reality_locktype > 1) ) {
      mudstate.recurse_rlevel++;
      if ( mudstate.recurse_rlevel < 10 ) {
         if ( mudconf.reality_locks ) {
            if ( (((mudconf.reality_locktype == 2) && ChkReality(thing)) || 
                  (mudconf.reality_locktype == 4)) &&
                  could_doit(player, thing, A_LUSER, ((mudconf.reality_locktype == 2) ? 1 : 0), 0)) {
               mudstate.recurse_rlevel--;
               return 1;
            } else if ( (((mudconf.reality_locktype == 3) && ChkReality(player)) || 
                         (mudconf.reality_locktype == 5)) &&
                          could_doit(thing, player, A_LUSER, ((mudconf.reality_locktype == 3) ? 1 : 0), 0)) {
               mudstate.recurse_rlevel--;
               return 1;
            }
         }
      } else {
         mudstate.recurse_rlevel--;
         return 1;
      }
      mudstate.recurse_rlevel--;
   }
#endif
   return 0;
}

/* default is the value to return if the attribute is empty */
int could_doit(dbref player, dbref thing, int locknum, int def, int i_uselocktype)
{
char	*key;
dbref	aowner, thing_bak;
int	aflags, tog_val,
        doit = 0;

  /* no if nonplayer trys to get key */

  if ( !Good_obj(thing) || !Good_obj(player) )
    return 0;

  if ( !Immortal(player) && (Recover(thing) || Going(thing)))
    return 0;

  if (!isPlayer(player) && !Wizard(player) && Key(thing)) {
    return 0;
  }

  tog_val = 0;
  if ( def > 1 ) {
     tog_val = 1;
     def = def - 2;
  }
  if (Immortal(player) && !God(Owner(thing)) && (locknum != A_LDARK) &&
      mudconf.wiz_override && !NoOverride(player))
    if ( (!((locknum == A_LUSE) && (mudconf.wiz_uselock == 0 || NoUselock(player)))) &&
         (!((locknum == A_LPAGE) && PageLock(thing) && tog_val)) ) 
       return 1;

  if (Wizard(player) && (locknum != A_LDARK ) &&
      !NoOverride(player) && mudconf.wiz_override &&
      !DePriv(player,Owner(thing),DP_LOCKS,POWER6,NOTHING) &&
      !DePriv(player,NOTHING,DP_OVERRIDE,POWER7,POWER_LEVEL_NA) && 
      !Immortal(Owner(thing)) ) {
    if ( !(locknum == A_LUSE && (mudconf.wiz_uselock == 0 || NoUselock(player))) &&
         !(locknum == A_LPAGE && PageLock(thing) && tog_val) )
       return 1;
  }

  if (DePriv(player,Owner(thing),DP_LOCKS,POWER6,NOTHING))
    return 0;

  if ((((locknum == A_LOCK) && isExit(thing)) || (locknum == A_LENTER) || (locknum == A_LLEAVE)) &&
	HasPriv(player,Owner(thing),POWER_WRAITH,POWER4,NOTHING))
    return 1;

  if ( mudconf.parent_control )
     key = atr_pget(thing, locknum, &aowner, &aflags);
  else
     key = atr_get(thing, locknum, &aowner, &aflags);
  /* TWINKLOCK is an inheritable lock -- why you need to be careful of player setting!!! */
  thing_bak = mudstate.twinknum;
  if ( (locknum == A_LTWINK) && !isPlayer(thing) && !*key && Good_chk(Owner(thing)) ) {
     free_lbuf(key);
     mudstate.twinknum = thing;
     thing = Owner(thing);
     key = atr_get(thing, locknum, &aowner, &aflags);
  } else if ( isPlayer(thing) && (locknum == A_LTWINK) && *key && (mudstate.twinknum == -1) ) {
     mudstate.twinknum = thing;
  }
  doit = eval_boolexp_atr(player, thing, thing, key, def, i_uselocktype);
  mudstate.twinknum = thing_bak;
  free_lbuf(key);
  return doit;
}
#endif
