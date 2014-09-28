/* flags.c - flag manipulation routines */

#ifdef SOLARIS
/* Solaris has borked header declarations */
char *index(const char *, int);
#endif

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "command.h"
#include "flags.h"
#include "alloc.h"
#include "rhost_ansi.h"

#include "rwho_clilib.h"

#ifndef STANDALONE

extern NAMETAB access_nametab[];


/* ---------------------------------------------------------------------------
 * fh_any: set or clear indicated bit, no security checking
 */

int 
fh_any(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (fflags & FLAG2) {
	if (reset)
	    s_Flags2(target, Flags2(target) & ~flag);
	else
	    s_Flags2(target, Flags2(target) | flag);
    } else if (fflags & FLAG3) {
	if (reset)
	    s_Flags3(target, Flags3(target) & ~flag);
	else
	    s_Flags3(target, Flags3(target) | flag);
    } else if (fflags & FLAG4) {
	if (reset)
	    s_Flags4(target, Flags4(target) & ~flag);
	else
	    s_Flags4(target, Flags4(target) | flag);
    } else {
	if (reset)
	    s_Flags(target, Flags(target) & ~flag);
	else
	    s_Flags(target, Flags(target) | flag);
    }
    return 1;
}

	/* set a toggle - Thorin 3/95 */
int 
th_any(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
   if (tflags & TOGGLE2) {
	if (reset)
	    s_Toggles2(target, Toggles2(target) & ~toggle);
	else
	    s_Toggles2(target, Toggles2(target) | toggle);
    } else {
	if (reset)
	    s_Toggles(target, Toggles(target) & ~toggle);
	else
	    s_Toggles(target, Toggles(target) | toggle);
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * fh_unfind_bit
 */

int 
fh_unfind_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
#ifdef RWHO_IN_USE
    char *tbuf, *buf;

    buf = alloc_mbuf("fh_unfind_bit");
    tbuf = alloc_mbuf("fh_unfind_bit_rwho");
    if (reset) {
	if (mudconf.rwho_transmit && (mudconf.control_flags & CF_RWHO_XMIT)) {
	    sprintf(buf, "%d@%.100s", target, mudconf.mud_name);
	    sprintf(tbuf, "%.30s(%.30s)", Name(target), Guild(target));
	    rwhocli_userlogin(buf, tbuf, time((time_t *) 0));
	}
    } else {
	if (mudconf.rwho_transmit &&
	    (mudconf.control_flags & CF_RWHO_XMIT)) {
	    sprintf(buf, "%d@%.100s", target, mudconf.mud_name);
	    sprintf(tbuf, "%.30s(%.30s)", Name(target), Guild(target));
	    rwhocli_userlogout(buf);
	}
    }
    free_mbuf(tbuf);
    free_mbuf(buf);
#endif
    return (fh_any(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_god: only GOD may set or clear the bit
 */

int 
fh_god(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!God(player))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_zonemaster(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
  if( Typeof(target) != TYPE_ROOM &&
      Typeof(target) != TYPE_THING ) {
    return 0;
  }

  if( db[target].zonelist ) {
    notify_quiet(player, "You can't alter this flag while there are entries in the zone list.");
    return 0;
  }
  if ( ZoneContents(target) && reset ) {
    notify_quiet(player, "You can't remove this flag while the ZONECONTENTS flag is still set.");
    return 0;
  }

  return (fh_any(target, player, flag, fflags, reset));
}

int
fh_zonecontents(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
   if ( Typeof(target) != TYPE_ROOM && Typeof(target) != TYPE_THING ) {
      return 0;
   }
   if ( !ZoneMaster(target) && !reset ) {
      notify_quiet(player, "You can't set this flag on a target not set ZONEMASTER.");
      return 0;
   }
   return (fh_any(target, player, flag, fflags, reset));
}

int
fh_any_sec(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int priv;

    priv = HasPriv(player,target,POWER_SECURITY,POWER4,NOTHING);
    if (!priv && !Controls(player,target) && !could_doit(player,target,A_LTWINK,0))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}
int
fh_controls(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Controls(player,target) && !could_doit(player,target,A_LTWINK,0))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}
/* ---------------------------------------------------------------------------
 * fh_wiz: only WIZARDS (or GOD) may set or clear the bit
 */

int 
fh_wiz(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Wizard(player) && !God(player))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_wiz_sec(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int priv;

    priv = HasPriv(player,target,POWER_SECURITY,POWER4,NOTHING);
    if (!priv && !Controls(player,target) && !could_doit(player,target,A_LTWINK,0))
	return 0;
    if (!Wizard(player) && !God(player) && !priv)
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

int
th_player(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
   if ( Good_obj(target) && isPlayer(target) ) {
      return (th_any(target, player, toggle, tflags, reset));
   } else {
      notify(player, "You can only set this toggle on players.");
      return 0;
   }
}

int
th_noset(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
   return 0;
}

int 
th_wiz(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
    if (!Wizard(player) && !God(player))
	return 0;
    return (th_any(target, player, toggle, tflags, reset));
}

int
th_extansi(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
    dbref aowner;
    int aflags;
    char *buff;
    buff = atr_get(target, A_ANSINAME, &aowner, &aflags);
    if ( reset && index(buff, ESC_CHAR) ) {
       free_lbuf(buff);
       notify(player, "You must remove raw ansi from your ANSINAME attribute first.");
       return 0;
    } 
    free_lbuf(buff);
    return (th_any(target, player, toggle, tflags, reset));
}

int 
th_immortal(dbref target, dbref player, FLAG toggle, int tflags, int reset)
{
    if (!Immortal(player) && !God(player))
	return 0;
    return (th_any(target, player, toggle, tflags, reset));
}

/* fh_fubar */

int 
fh_fubar(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (Wizard(player) && Immortal(target)) {
       if (!(((Immortal(player) && !God(target)) || God(player)) && reset))
	return 0;
    }
    return (fh_wiz(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_admin: only WIZARDS or ADIMNS (or GOD) may set or clear the bit
 */

int 
fh_admin(dbref target, dbref player, int flag, int fflags, int reset)
{
    if (!God(player) && !Admin(player))
	return 0;
    if ((God(target) && !God(player)) || (Immortal(target) && !Immortal(player)) || (Wizard(target) && !Wizard(player))) {
	notify(player, "You cannot change that flag on this player.");
	return (0);
    }
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_admin2(dbref target, dbref player, int flag, int fflags, int reset)
{
    if (!God(player) && !Admin(player))
	return 0;
    if ((God(target) || Immortal(target)) && !reset)
	return 0;
    if (Wizard(target) && !Wizard(player)) {
	notify(player, "You cannot change that flag on this player.");
	return (0);
    }
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_builder(dbref target, dbref player, int flag, int fflags, int reset)
{
    if (!God(player) &&
	!Builder(player))
	return 0;
    if ((God(target) && !God(player)) ||
	(Immortal(target) && !Immortal(player)) ||
	(Wizard(target) && !Wizard(player)) ||
	(Admin(target) && !Admin(player))) {
	notify(player, "You cannot change that flag on that player.");
	return 0;
    }
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_builder_sec(dbref target, dbref player, int flag, int fflags, int reset)
{
    int priv;

    priv = HasPriv(player,target,POWER_SECURITY,POWER4,NOTHING);
    if (!priv && !Controls(player,target) && !could_doit(player,target,A_LTWINK,0))
	return 0;
    if (!God(player) &&
	!Builder(player) && !priv) 
	return 0;
    if (((God(target) && !God(player)) ||
	(Immortal(target) && !Immortal(player)) ||
	(Wizard(target) && !Wizard(player)) ||
	(Admin(target) && !Admin(player))) && !priv) {
	notify(player, "You cannot change that flag on that player.");
	return 0;
    }
    return (fh_any(target, player, flag, fflags, reset));
}

int 
th_builder(dbref target, dbref player, int toggle, int tflags, int reset)
{
    if (!God(player) &&
	!Builder(player))
	return 0;
    if ((God(target) && !God(player)) ||
	(Immortal(target) && !Immortal(player)) ||
	(Wizard(target) && !Wizard(player)) ||
	(Admin(target) && !Admin(player))) {
	notify(player, "You cannot change that toggle on that player.");
	return 0;
    }
    return (th_any(target, player, toggle, tflags, reset));
}

int 
th_monitor(dbref target, dbref player, int toggle, int tflags, int reset)
{
    if (!God(player) &&
	!Builder(player))
	return 0;
    if ((God(target) && !God(player)) ||
	(Immortal(target) && !Immortal(player)) ||
	(Wizard(target) && !Wizard(player)) ||
	(Admin(target) && !Admin(player)) ||
	(!Wizard(player) && !Builder(target))) {
	notify(player, "You cannot change that toggle on that player.");
	return 0;
    }
    return (th_any(target, player, toggle, tflags, reset));
}

int 
pw_any(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    int tpw;

    if (level == POWER_LEVEL_OFF)
	return 0;
    if ((level == POWER_LEVEL_NA) && (slevel != POWER_LEVEL_OFF))
	slevel = POWER_LEVEL_GUILD;
    else if (slevel > level)
	slevel = level;
    tpw = slevel;
    tpw <<= powerpos;
    if (pflags == POWER3) {
	s_Toggles3(target, ((Toggles3(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else if (pflags == POWER4) {
	s_Toggles4(target, ((Toggles4(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else {
	s_Toggles5(target, ((Toggles5(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    }
    return 1;
}

int 
pw_any2(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    int tpw;

    if (level == POWER_REMOVE)
	return 0;
    if (!Wizard(target) && (slevel == POWER_LEVEL_COUNC))
	return 0;
    if ((level == POWER_LEVEL_NA) && (slevel != POWER_REMOVE))
	slevel = POWER_LEVEL_GUILD;
    else if (slevel < level)
	slevel = level;
    if ((slevel == POWER_LEVEL_OFF) && DPShift(target)) {
	notify(player, "Warning, clearing shift flag.");
	s_Flags3(target, Flags3(target) & (~DPSHIFT));
    } else if ((slevel == POWER_LEVEL_COUNC) && !DPShift(target)) {
	notify(player, "Warning, setting shift flag.");
	s_Flags3(target, Flags3(target) | DPSHIFT);
    }
    if (slevel == POWER_REMOVE)
	tpw = 0;
    else if (!DPShift(target) && (level != POWER_LEVEL_NA))
	tpw = slevel + 1;
    else
	tpw = slevel;
    tpw <<= powerpos;
    if (pflags == POWER6) {
	s_Toggles6(target, ((Toggles6(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else if (pflags == POWER7) {
	s_Toggles7(target, ((Toggles7(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else {
	s_Toggles8(target, ((Toggles8(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    }
    return 1;
}

int 
pw_any3(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    int tpw;

    if (level == POWER_REMOVE)
	return 0;
    if ((level == POWER_LEVEL_NA) && (slevel != POWER_REMOVE))
	slevel = POWER_LEVEL_GUILD;
    else if (slevel < level)
	slevel = level;
    if ((slevel == POWER_LEVEL_OFF) && DPShift(target)) {
	notify(player, "Warning, clearing shift flag.");
	s_Flags3(target, Flags3(target) & (~DPSHIFT));
    } else if ((slevel == POWER_LEVEL_COUNC) && !DPShift(target)) {
	notify(player, "Warning, setting shift flag.");
	s_Flags3(target, Flags3(target) | DPSHIFT);
    }
    if (slevel == POWER_REMOVE)
	tpw = 0;
    else if (!DPShift(target) && (level != POWER_LEVEL_NA))
	tpw = slevel + 1;
    else
	tpw = slevel;
    tpw <<= powerpos;
    if (pflags == POWER6) {
	s_Toggles6(target, ((Toggles6(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else if (pflags == POWER7) {
	s_Toggles7(target, ((Toggles7(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    } else {
	s_Toggles8(target, ((Toggles8(target) & (~(POWER_LEVEL_COUNC << powerpos))) | tpw));
    }
    return 1;
}

int 
pw_wiz(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    if (!God(player) && !Wizard(player)) {
	return 0;
    }
    return (pw_any(target, player, powerpos, pflags, level, slevel));
}

int 
pw_imm(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    if (!Immortal(player)) {
	return 0;
    }
    return (pw_any(target, player, powerpos, pflags, level, slevel));
}

int 
pw_wiz2(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    if (!God(player) && !Wizard(player)) {
	return 0;
    }
    if (Wizard(target) && !Immortal(target) && !Immortal(player)) {
	return 0;
    }
    if (Immortal(target)) {
	return 0;
    }
    return (pw_any2(target, player, powerpos, pflags, level, slevel));
}

int 
pw_wiz3(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
    if (!God(player) && !Wizard(player)) {
	return 0;
    }
    if (Wizard(target) && !Immortal(target) && !Immortal(player)) {
	return 0;
    }
    if (Immortal(target)) {
	return 0;
    }
    return (pw_any3(target, player, powerpos, pflags, level, slevel));
}

int
pw_im2(dbref target, dbref player, int powerpos, int pflags, int level, int slevel)
{
  if ((!God(player) && !Immortal(player)) || God(target) || Immortal(target))
    return 0;
  return (pw_any2(target, player, powerpos, pflags, level, slevel));
}

/* ---------------------------------------------------------------------------
 * fh_inherit: only players may set or clear this bit.
 */

int 
fh_inherit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Inherits(player))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_wiz_bit: Only GOD may set/clear this bit on others.
 */

int 
fh_wiz_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!God(player))
	return 0;
    if (God(target) && reset) {
	notify(player, "You cannot make yourself mortal.");
	return 0;
    }
    return (fh_any(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_dark_bit: manipulate the dark bit. Nonwizards may not set on players.
 */

int 
fh_dark_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!reset && (Typeof(target) == TYPE_PLAYER) &&
	(player != target) &&
	(!Builder(player)))
	return 0;
    if ( mudconf.player_dark || 
	 (!mudconf.player_dark && ((Typeof(target) != TYPE_PLAYER) || 
	 (target != player))) )
      return (fh_any(target, player, flag, fflags, reset));
    else
      return (fh_wiz(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_going_bit: manipulate the going bit.  Non-gods may only clear on rooms.
 */

int 
fh_byeroom_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (isRoom(target) && Byeroom(target) && reset) {
	notify(player, "Your room has been spared from destruction.");
	if (fh_any(target, player, flag, fflags, reset)) {
	  s_Flags3(target,Flags3(target) & ~DR_PURGE);
	  return 1;
	}
	else
	  return 0;
    }
    if (!God(player) && !Immortal(player))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

/* ---------------------------------------------------------------------------
 * fh_hear_bit: set or clear bits that affect hearing.
 */

int 
fh_hear_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int could_hear;

    could_hear = Hearer(target);
    fh_any(target, player, flag, fflags, reset);
    handle_ears(target, could_hear, Hearer(target));
    return 1;
}

/* ---------------------------------------------------------------------------
 * fh_immortal_bit: Only Immortal may set/clear this bit on others.
 */


int 
fh_immortal_bit(dbref target, dbref player, int flag, int fflags, int reset)
{
    if (!Immortal(player) && !God(player))
	return 0;
    return (fh_any(target, player, flag, fflags, reset));
}

int 
fh_gobj(dbref target, dbref player, int flag, int fflags, int reset)
{
    if (reset) {
	if (!Wizard(player) && !God(player))
	    return 0;
    } else {
	if (!Admin(player) && !God(player))
	    return 0;
    }
    return (fh_any(target, player, flag, fflags, reset));
}

int fh_none(dbref target, dbref player, int flag, int fflags, int reset)
{
  return 0;
}

/* Toggle table, 4th item tells which toggle word, 0 for 1st word, TOGGLE2, TOGGLE3, or TOGGLE4 */
/* Lensy:
 *   Note: Toggle[01] is Toggle .. Toggle[234] is power .. Toggle[567] is depower
 */
TOGENT tog_table[] =
{
  {"MONITOR", TOG_MONITOR, 'M', 0, CA_BUILDER, th_monitor},
  {"MONITOR_USERID", TOG_MONITOR_USERID, 'U', 0, CA_WIZARD, th_wiz},
  {"MONITOR_SITE", TOG_MONITOR_SITE, 'S', 0, CA_IMMORTAL, th_immortal},
  {"MONITOR_STATS", TOG_MONITOR_STATS, 'T', 0, CA_WIZARD, th_wiz},
  {"MONITOR_FAIL", TOG_MONITOR_FAIL, 'F', 0, CA_WIZARD, th_wiz},
  {"MONITOR_CONN", TOG_MONITOR_CONN, 'C', 0, CA_IMMORTAL, th_immortal},
  {"MONITOR_TIME", TOG_MONITOR_TIME, 'I', 0, CA_BUILDER, th_monitor},
  {"MONITOR_DISREASON", TOG_MONITOR_DISREASON, 'R', 0, CA_WIZARD, th_wiz},
  {"MONITOR_VLIMIT", TOG_MONITOR_VLIMIT, 'L', 0, CA_IMMORTAL, th_immortal},
  {"MONITOR_CPU", TOG_MONITOR_CPU, 'c', 0, CA_IMMORTAL, th_immortal},
  {"VANILLA_ERRORS", TOG_VANILLA_ERRORS, 'V', 0, 0, th_any},
  {"NO_ANSI_EX", TOG_NO_ANSI_EX, 'E', 0, 0, th_any},
  {"CPUTIME", TOG_CPUTIME, 'P', 0, 0, th_any},
  {"NOTIFY_LINK", TOG_NOTIFY_LINK, 'l', 0, 0, th_any},
  {"MONITOR_AREG", TOG_MONITOR_AREG, 'A', 0, CA_IMMORTAL, th_immortal},
  {"NO_ANSI_PLAYER", TOG_NOANSI_PLAYER, 'p', 0, 0, th_any},
  {"NO_ANSI_THING", TOG_NOANSI_THING, 't', 0, 0, th_any},
  {"NO_ANSI_EXIT", TOG_NOANSI_EXIT, 'e', 0, 0, th_any},
  {"NO_ANSI_ROOM", TOG_NOANSI_ROOM, 'r', 0, 0, th_any},
  {"NO_FORMAT", TOG_NO_FORMAT, 'f', 0, CA_IMMORTAL, th_immortal},
  {"NO_TIMESTAMP", TOG_NO_TIMESTAMP, 's', 0, CA_IMMORTAL, th_immortal},
  {"ZONE_AUTOADD", TOG_ZONE_AUTOADD, 'z', 0, 0, th_any},
  {"ZONE_AUTOADDALL", TOG_ZONE_AUTOADDALL, 'Z', 0, 0, th_any},
  {"WIELDED", TOG_WIELDABLE, 'W', 0, CA_WIZARD, th_wiz},
  {"WORN", TOG_WEARABLE, 'w', 0, CA_WIZARD, th_wiz},
  {"SEE_SUSPECT", TOG_SEE_SUSPECT, '+', 0, CA_WIZARD, th_wiz},
  {"BRANDY_MAIL", TOG_BRANDY_MAIL, 'b', 0, 0, th_any},
  {"FORCEHALTED", TOG_FORCEHALTED, 'h', 0, 0, th_immortal},
  {"PROG", TOG_PROG, 'g', 0, 0, th_wiz},
  {"NOSHPROG", TOG_NOSHELLPROG, 'o', 0, CA_WIZARD, th_wiz},
  {"EXTANSI", TOG_EXTANSI, 'E', 1, 0, th_extansi},
  {"IMMPROG", TOG_IMMPROG, 'I', 1, CA_IMMORTAL, th_immortal},
  {"MONITOR_BAD", TOG_MONITOR_BFAIL, 'B', 1, CA_IMMORTAL, th_immortal},
  {"PROG_ON_CONNECT", TOG_PROG_ON_CONNECT, 'O', 1, CA_WIZARD, th_wiz},
  {"MAIL_STRIPRETURN", TOG_MAIL_STRIPRETURN, 'm', 1, 0, th_any},
  {"PENN_MAIL", TOG_PENN_MAIL, '@', 1, 0, th_any},
  {"SILENTEFFECT", TOG_SILENTEFFECTS, 'q', 1, CA_WIZARD, th_wiz},
  {"IGNOREZONE", TOG_IGNOREZONE, 'i', 1, CA_WIZARD, th_immortal},
  {"VPAGE", TOG_VPAGE, 'v', 1, 0, th_any},
  {"PAGELOCK", TOG_PAGELOCK, 'P', 1, 0, th_wiz},
  {"MAIL_NOPARSE", TOG_MAIL_NOPARSE, 'n', 1, 0, th_any},
  {"MAIL_LOCKDOWN", TOG_MAIL_LOCKDOWN, 'd', 1, CA_IMMORTAL, th_immortal},
  {"MUXPAGE", TOG_MUXPAGE, 'p', 1, 0, th_any},
  {"NOZONEPARENT", TOG_NOZONEPARENT, 'y', 1, 0, th_any},
  {"ATRUSE", TOG_ATRUSE, 'a', 1, 0, th_wiz},
  {"NOGLOBPARENT", TOG_NOGLOBPARENT, 'G', 1, 0, th_wiz},
  {"LOGROOM", TOG_LOGROOM, '!', 1, CA_WIZARD, th_wiz},
  {"VARIABLE", TOG_VARIABLE, '~', 1, 0, th_any},
#ifdef ENH_LOGROOM
  {"LOGROOMENH", TOG_LOGROOMENH, '^', 1, CA_IMMORTAL, th_immortal},
#endif
  {"EXFULLWIZATTR", TOG_EXFULLWIZATTR, 'x', 1, CA_IMMORTAL, th_immortal},
  {"NODEFAULT", TOG_NODEFAULT, 'D', 1, CA_WIZARD, th_wiz},
  {"KEEPALIVE", TOG_KEEPALIVE, 'K', 1, 0, th_player},
  {"CHKREALITY", TOG_CHKREALITY, 'C', 1, 0, th_wiz},
  {"NOISY", TOG_NOISY, 'N', 1, 0, th_player},
  {"ZONECMDCHK", TOG_ZONECMDCHK, 'k', 1, 0, th_player},
  {"HIDEIDLE", TOG_HIDEIDLE, 'h', 1, 0, th_wiz},
  {"MORTALREALITY", TOG_MORTALREALITY, 'M', 1, 0, th_wiz},
  {"ACCENTS", TOG_ACCENTS, 'X', 1, 0, th_player},
  {"MAILVALIDATE", TOG_PREMAILVALIDATE, '-', 1, 0, th_player},
  {"CLUSTER", TOG_CLUSTER, '~', 0, CA_IMMORTAL, th_noset},
  {"SAFELOG", TOG_SAFELOG, 'Y', 1, 0, th_player},
  {"SNUFFDARK", TOG_SNUFFDARK, 'u', 0, CA_WIZARD, th_wiz},
  {NULL, 0, ' ', 0, 0, NULL}
};

POWENT pow_table[] =
{
  {"CHANGE_QUOTAS", POWER_CHANGE_QUOTAS, 'A', POWER3, 0, POWER_LEVEL_NA, pw_wiz},
  {"CHOWN_ANYWHERE", POWER_CHOWN_ANYWHERE, 'B', POWER3, 0, POWER_LEVEL_NA, pw_wiz},
  {"CHOWN_ME", POWER_CHOWN_ME, 'C', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"CHOWN_OTHER", POWER_CHOWN_OTHER, 'D', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"WIZ_WHO", POWER_WIZ_WHO, 'E', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"EXAMINE_ALL", POWER_EX_ALL, 'F', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"NOFORCE", POWER_NOFORCE, 'G', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"SEE_QUEUE_ALL", POWER_SEE_QUEUE_ALL, 'H', POWER3, 0, POWER_LEVEL_NA, pw_imm},
  {"FREE_QUOTA", POWER_FREE_QUOTA, 'I', POWER3, 0, POWER_LEVEL_NA, pw_wiz},
  {"GRAB_PLAYER", POWER_GRAB_PLAYER, 'J', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"JOIN_PLAYER", POWER_JOIN_PLAYER, 'K', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"LONG_FINGERS", POWER_LONG_FINGERS, 'L', POWER3, 0, POWER_LEVEL_NA, pw_wiz},
  {"NO_BOOT", POWER_NO_BOOT, 'M', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"BOOT", POWER_BOOT, 'N', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"STEAL", POWER_STEAL, 'O', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"SEE_QUEUE", POWER_SEE_QUEUE, 'P', POWER3, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"TEL_ANYWHERE", POWER_TEL_ANYWHERE, 'R', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"TEL_ANYTHING", POWER_TEL_ANYTHING, 'S', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"STAT_ANY", POWER_STAT_ANY, 'T', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"PCREATE", POWER_PCREATE, 'U', POWER4, 0, POWER_LEVEL_NA, pw_wiz},
  {"FREE_WALL", POWER_FREE_WALL, 'V', POWER4, 0, POWER_LEVEL_NA, pw_wiz},
  {"NOWHO", POWER_NOWHO, 'W', POWER5, 0, POWER_LEVEL_NA, pw_wiz},
  {"FREE_PAGE", POWER_FREE_PAGE, 'X', POWER4, 0, POWER_LEVEL_NA, pw_wiz},
  {"HALT_QUEUE", POWER_HALT_QUEUE, 'Y', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"HALT_QUEUE_ALL", POWER_HALT_QUEUE_ALL, 'Z', POWER4, 0, POWER_LEVEL_NA, pw_imm},
  {"NOKILL", POWER_NOKILL, 'b', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"SEARCH_ANY", POWER_SEARCH_ANY, 'c', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"SECURITY", POWER_SECURITY, 'd', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"WHO_UNFIND", POWER_WHO_UNFIND, 'e', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"WRAITH", POWER_WRAITH, 'f', POWER4, 0, POWER_LEVEL_COUNC, pw_wiz},
  {"SHUTDOWN", POWER_SHUTDOWN, 'g', POWER4, 0, POWER_LEVEL_NA, pw_wiz},
  {"HIDEBIT", POWER_HIDEBIT, 'h', POWER5, 0, POWER_LEVEL_COUNC, pw_imm},
  {"PURGE", POWER_OPURGE, 'p', POWER5, 0, POWER_LEVEL_NA, pw_imm},
  {"FULLTEL", POWER_FULLTEL_ANYWHERE, 't', POWER5, 0, POWER_LEVEL_NA, pw_imm},
  {"EXAMINE_FULL", POWER_EX_FULL, 'x', POWER5, 0, POWER_LEVEL_NA, pw_imm},
  {NULL, 0, 0, 0, 0, 0, NULL}
};

POWENT depow_table[] =
{
  {"WALL", DP_WALL, 'A', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"LONG_FINGERS", DP_LONG_FINGERS, 'B', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"STEAL", DP_STEAL, 'C', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"CREATE", DP_CREATE, 'D', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"WIZ_WHO", DP_WIZ_WHO, 'E', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"CLOAK", DP_CLOAK, 'F', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"BOOT", DP_BOOT, 'G', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"PAGE", DP_PAGE, 'H', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"FORCE", DP_FORCE, 'I', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"LOCKS", DP_LOCKS, 'J', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"COM", DP_COM, 'K', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"COMMAND", DP_COMMAND, 'L', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"MASTER", DP_MASTER, 'M', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"EXAMINE", DP_EXAMINE, 'N', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"NUKE", DP_NUKE, 'O', POWER6, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"FREE", DP_FREE, 'P', POWER6, 0, POWER_LEVEL_NA, pw_wiz2},
  {"OVERRIDE", DP_OVERRIDE, 'Q', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"TEL_ANYWHERE", DP_TEL_ANYWHERE, 'R', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"TEL_ANYTHING", DP_TEL_ANYTHING, 'S', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"PCREATE", DP_PCREATE, 'T', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"POWER", DP_POWER, 'U', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"QUOTA", DP_QUOTA, 'V', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"MODIFY", DP_MODIFY, 'W', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"CHOWN_ME", DP_CHOWN_ME, 'X', POWER7, 0, POWER_LEVEL_OFF, pw_im2},
  {"CHOWN_OTHER", DP_CHOWN_OTHER, 'Y', POWER7, 0, POWER_LEVEL_OFF, pw_im2},
  {"ABUSE", DP_ABUSE, 'Z', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"UNL_QUOTA", DP_UNL_QUOTA, 'a', POWER7, 0, POWER_LEVEL_NA, pw_wiz2},
  {"SEARCH_ANY", DP_SEARCH_ANY, 'b', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"GIVE", DP_GIVE, 'c', POWER7, 0, POWER_LEVEL_OFF, pw_wiz3},
  {"RECEIVE",  DP_RECEIVE, 'd', POWER7, 0, POWER_LEVEL_OFF, pw_wiz3},
  {"NOGOLD", DP_NOGOLD, 'e', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"NOSTEAL", DP_NOSTEAL, 'f', POWER7, 0, POWER_LEVEL_OFF, pw_wiz2},
  {"PASSWORD", DP_PASSWORD, 'g', POWER8, 0, POWER_LEVEL_NA, pw_wiz2},
  {"MORTAL_EXAMINE", DP_MORTAL_EXAMINE, 'h', POWER8, 0, POWER_LEVEL_NA, pw_wiz2},
  {"PERSONAL_COMMAND", DP_PERSONAL_COMMANDS, 'i', POWER8, 0, POWER_LEVEL_NA, pw_wiz2},
  {NULL, 0, 0, 0, 0, 0, NULL}
};

/* Okay, it is important that for flags, there is space for 16 characters. */
/* 15 + the terminating null.  This is needed for the flag renaming system */

FLAGENT gen_flags[] =
{
  /* First Set of Flags */
  {"AUDIBLE", HEARTHRU, 'n', 0, 0, 0, 0, fh_hear_bit},
  {"CHOWN_OK", CHOWN_OK, 'C', 0, 0, 0, 0, fh_any},
  {"CONTROL_OK", CONTROL_OK, 'z', 0, 0, 0, 0, fh_any},
  {"DARK", DARK, 'D', 0, 0, 0, 0, fh_dark_bit},
  {"DESTROY_OK", DESTROY_OK, 'd', 0, 0, 0, 0, fh_any},
  {"ENTER_OK", ENTER_OK, 'e', 0, 0, 0, 0, fh_any},
  {"GOING", GOING, 'G', 0, CA_NO_DECOMP, 0, 0, fh_god},
  {"HALTED", HALT, 'h', 0, 0, 0, 0, fh_any},
  {"HAS_STARTUP", HAS_STARTUP, '+', 0, CA_GOD | CA_NO_DECOMP, 0, 0, fh_god},
  {"HAVEN", HAVEN, 'H', 0, 0, 0, 0, fh_any},
  {"IMMORTAL", IMMORTAL, 'i', 0, 0, 0, 0, fh_god},
  {"INHERIT", INHERIT, 'I', 0, 0, 0, 0, fh_inherit},
  {"JUMP_OK", JUMP_OK, 'J', 0, 0, 0, 0, fh_any},
  {"LINK_OK", LINK_OK, 'L', 0, 0, 0, 0, fh_any},
  {"MONITOR", MONITOR, 'M', 0, 0, 0, 0, fh_hear_bit},
  {"MYOPIC", MYOPIC, 'm', 0, 0, 0, 0, fh_any},
  {"NO_SPOOF", NOSPOOF, 'N', 0, 0, 0, 0, fh_any},
  {"OPAQUE", OPAQUE, 'O', 0, 0, 0, 0, fh_any},
  {"PUPPET", PUPPET, 'p', 0, 0, 0, 0, fh_hear_bit},
  {"QUIET", QUIET, 'Q', 0, 0, 0, 0, fh_any},
  {"ROBOT", ROBOT, 'r', 0, 0, 0, 0, fh_any},
  {"SAFE", SAFE, 's', 0, 0, 0, 0, fh_any},
  {"STICKY", STICKY, 'S', 0, 0, 0, 0, fh_any},
  {"TERSE", TERSE, 't', 0, 0, 0, 0, fh_any},
  {"TRACE", TRACE, 'T', 0, 0, 0, 0, fh_any},
  {"TRANSPARENT", SEETHRU, 'T', 0, 0, 0, 0, fh_any},
  {"VERBOSE", VERBOSE, 'v', 0, 0, 0, 0, fh_any},
  {"VISUAL", VISUAL, 'V', 0, 0, 0, 0, fh_any},
  {"ROYALTY", WIZARD, 'W', 0, 0, 0, 0, fh_immortal_bit},
  /* Second Set of Flags */
  {"ABODE", ABODE, 'A', FLAG2, 0, 0, 0, fh_any},
  {"ANSI", ANSI, '<', FLAG2, 0, 0, 0, fh_any},
  {"ANSICOLOR", ANSICOLOR, '>', FLAG2, 0, 0, 0, fh_any},
  {"ARCHITECT", BUILDER, 'B', FLAG2, 0, 0, 0, fh_wiz},
  {"BYEROOM", BYEROOM, '=', FLAG2, CA_NO_DECOMP, 0, 0, fh_byeroom_bit},
  {"CLOAK", CLOAK, 'b', FLAG2, 0, 0, 0, fh_wiz},
  {"CONNECTED", CONNECTED, 'c', FLAG2, CA_NO_DECOMP, 0, 0, fh_immortal_bit},
  {"COUNCILOR", ADMIN, 'a', FLAG2, 0, 0, 0, fh_wiz},
  {"FLOATING", FLOATING, 'F', FLAG2, 0, 0, 0, fh_any},
  {"FREE", FREE, 'X', FLAG2, 0, 0, 0, fh_admin},
  {"FUBAR", FUBAR, 'f', FLAG2, CA_WIZARD, 0, 0, fh_fubar},
  {"GUEST", GUEST_FLAG, '!', FLAG2, CA_WIZARD, 0, 0, fh_wiz},
  {"GUILDMASTER", GUILDMASTER, 'g', FLAG2, 0, 0, 0, fh_wiz},
  {"GUILDOBJ", GUILDOBJ, 'j', FLAG2, CA_ADMIN, 0, 0, fh_gobj},
  {"HAS_FWDLIST", HAS_FWDLIST, '&', FLAG2, CA_GOD | CA_NO_DECOMP, 0, 0, fh_god},
  {"HAS_LISTEN", HAS_LISTEN, '@', FLAG2, CA_GOD | CA_NO_DECOMP, 0, 0, fh_god},
  {"INDESTRUCTABLE", INDESTRUCTABLE, '~', FLAG2, 0, 0, 0, fh_wiz},
  {"KEY", KEY, 'K', FLAG2, 0, 0, 0, fh_any},
  {"LIGHT", LIGHT, 'l', FLAG2, 0, 0, 0, fh_any},
  {"NO_FLASH", NOFLASH, '-', FLAG2, 0, 0, 0, fh_any},
  {"NO_TEL", NO_TEL, 'o', FLAG2, CA_WIZARD | FA_HANDLER, 0, 0, fh_wiz_sec},
  {"NO_WALLS", NO_WALLS, 'w', FLAG2, 0, 0, 0, fh_any},
  {"NO_YELL", NO_YELL, 'y', FLAG2, FA_HANDLER, 0, 0, fh_builder_sec},
  {"PARENT_OK", PARENT_OK, 'Y', FLAG2, 0, 0, 0, fh_any},
  {"RECOVER", RECOVER, '$', FLAG2, CA_NO_DECOMP, 0, 0, fh_god},
  {"SCLOAK", SCLOAK, 'Z', FLAG2, CA_IMMORTAL, 0, 0, fh_immortal_bit},
  {"SLAVE", SLAVE, 'x', FLAG2, CA_BUILDER | FA_HANDLER, 0, 0, fh_builder_sec},
  {"SUSPECT", SUSPECT, 'u', FLAG2, CA_WIZARD, 0, 0, fh_wiz},
  {"UNFINDABLE", UNFINDABLE, 'U', FLAG2, 0, 0, 0, fh_unfind_bit},
  {"WANDERER", WANDERER, '^', FLAG2, CA_ADMIN, 0, 0, fh_admin},
  /* Third set of flags */
  {"ALTQUOTA", ALTQUOTA, 'Q', FLAG3, CA_NO_DECOMP, 0, 0, fh_none},
  {"ANONYMOUS", ANONYMOUS, 'Y', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"AUDITORIUM", AUDIT, 'a', FLAG3, 0, 0, 0, fh_any},
  {"BACKSTAGE", BACKSTAGE, 'b', FLAG3, CA_IMMORTAL, 0, 0, fh_immortal_bit},
  {"CMDCHECK", CMDCHECK, 'K', FLAG3, CA_IMMORTAL|CA_NO_DECOMP, 0, 0, fh_immortal_bit},
  {"COMBAT", COMBAT, 'X', FLAG3, 0, 0, 0, fh_any},
  {"DOORED", DOORRED, 'R', FLAG3, 0, 0, 0, fh_any},
  {"DPSHIFT", DPSHIFT, 'B', FLAG3, CA_IMMORTAL|CA_NO_DECOMP, 0, 0, fh_immortal_bit},
  {"DR_PURGE", DR_PURGE, 'x', FLAG3, CA_NO_DECOMP, 0, 0, fh_immortal_bit},
  {"IC", IC, 'I', FLAG3, 0, 0, 0, fh_any},
  {"NO_ANSINAME", NO_ANSINAME, 'n', FLAG3, 0, 0, 0, fh_wiz},
  {"NO_COMMAND", NOCOMMAND, 'c', FLAG3, 0, 0, 0, fh_any},
  {"NO_CONNECT", NOCONNECT, 'A', FLAG3, CA_ADMIN, 0, 0, fh_admin2},
  {"NO_EXAMINE", NOEXAMINE, 'E', FLAG3, 0, 0, 0, fh_wiz},
  {"NO_GOBJ", NO_GOBJ, 'G', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_MODIFY", NOMODIFY, 'M', FLAG3, 0, 0, 0, fh_wiz},
  {"NO_MOVE", NOMOVE, 'N', FLAG3, CA_WIZARD | FA_HANDLER, 0, 0, fh_wiz_sec},
  {"NO_OVERRIDE", NO_OVERRIDE, 'V', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_PESTER", NO_PESTER, 'p', FLAG3, 0, 0, 0, fh_wiz},
  {"NO_POSSESS", NOPOSSESS, 'C', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_STOP", NOSTOP, 'F', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_USELOCK", NO_USELOCK, 'U', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_WHO", NOWHO, 'W', FLAG3, CA_IMMORTAL|CA_NO_DECOMP, 0, 0, fh_immortal_bit},
  {"PRIVATE", PRIVATE, 'P', FLAG3, 0, 0, 0, fh_wiz},
  {"SEE_OEMIT", SEE_OEMIT, 'O', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"SIDEFX", SIDEFX, 's', FLAG3, 0, 0, 0, fh_any},
  {"SPOOF", SPOOF, 'S', FLAG3, CA_IMMORTAL, 0, 0, fh_immortal_bit},
  {"STOP", STOP, 'D', FLAG3, CA_WIZARD, 0, 0, fh_wiz},
  {"TELOK", TELOK, 'T', FLAG3, 0, 0, 0, fh_any},
  {"ZONECONTENTS", ZONECONTENTS, 'z', FLAG3, 0, 0, 0, fh_zonecontents},
  {"ZONEMASTER", ZONEMASTER, 'Z', FLAG3, 0, 0, 0, fh_zonemaster},
  /* Fourth set of flags */
  {"BLIND", BLIND, 'g', FLAG4, 0, 0, 0, fh_any},
  {"BOUNCE", BOUNCE, 'o', FLAG4, 0, 0, 0, fh_any},
#ifdef ENABLE_COMMAND_FLAG
  {"COMMANDS", COMMANDS, '$', FLAG4, 0, 0, 0, fh_any},
#else
  {"COMMANDS", COMMANDS, '$', FLAG4, CA_GOD | CA_NO_DECOMP, 0, 0, fh_none},
#endif
  {"INPROGRAM", INPROGRAM, 'i', FLAG4, CA_GOD | CA_NO_DECOMP, 0, 0, fh_god},
  {"LOGIN", LOGIN, 'L', FLAG4, CA_WIZARD, 0, 0, fh_wiz},
#ifdef MARKER_FLAGS
  {"MARKER0", MARKER0, '0', FLAG4, 0, 0, 0, fh_god},
  {"MARKER1", MARKER1, '1', FLAG4, 0, 0, 0, fh_god},
  {"MARKER2", MARKER2, '2', FLAG4, 0, 0, 0, fh_god},
  {"MARKER3", MARKER3, '3', FLAG4, 0, 0, 0, fh_god},
  {"MARKER4", MARKER4, '4', FLAG4, 0, 0, 0, fh_god},
  {"MARKER5", MARKER5, '5', FLAG4, 0, 0, 0, fh_god},
  {"MARKER6", MARKER6, '6', FLAG4, 0, 0, 0, fh_god},
  {"MARKER7", MARKER7, '7', FLAG4, 0, 0, 0, fh_god},
  {"MARKER8", MARKER8, '8', FLAG4, 0, 0, 0, fh_god},
  {"MARKER9", MARKER9, '9', FLAG4, 0, 0, 0, fh_god},
#endif
  {"NO_BACKSTAGE", NOBACKSTAGE, 'd', FLAG4, CA_IMMORTAL, 0, 0, fh_immortal_bit},
  {"NO_CODE", NOCODE, 'j', FLAG4, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_NAME", NONAME, 'm', FLAG4, CA_WIZARD, 0, 0, fh_wiz},
  {"NO_UNDERLINE", NOUNDERLINE, 'u', FLAG4, 0, 0, 0, fh_any},
  {"SHOWFAILCMD", SHOWFAILCMD, 'f', FLAG4, 0, 0, 0, fh_any},
  {"SPAMMONITOR", SPAMMONITOR, 'w', FLAG4, CA_IMMORTAL, 0, 0, fh_immortal_bit},
  {"ZONEPARENT", ZONEPARENT, 'y', FLAG4, 0, 0, 0, fh_any},
  {"HAS_PROTECT", HAS_PROTECT, '+', FLAG4, CA_GOD | CA_NO_DECOMP, 0, 0, fh_god},
  {"XTERMCOLOR", XTERMCOLOR, 't', FLAG4, 0, 0, 0, fh_any},
  {"", 0, ' ', 0, 0, 0, 0, NULL}
};

#endif /* STANDALONE */

OBJENT object_types[8] =
{
  {"ROOM", 'R', CA_PUBLIC, OF_CONTENTS | OF_EXITS | OF_DROPTO | OF_HOME},
  {"THING", ' ', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_SIBLINGS},
  {"EXIT", 'E', CA_PUBLIC, OF_SIBLINGS},
  {"PLAYER", 'P', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_OWNER | OF_SIBLINGS},
  {"ZONE", 'Z', CA_PUBLIC, OF_CONTENTS | OF_LOCATION | OF_EXITS | OF_HOME | OF_OWNER | OF_SIBLINGS},
  {"TYPE5", '+', CA_GOD, 0},
  {"TYPE6", '-', CA_GOD, 0},
  {"GARBAGE", '*', CA_GOD, 0}
};

#ifndef STANDALONE

/* ---------------------------------------------------------------------------
 * init_flagtab: initialize flag hash tables.
 */

void 
NDECL(init_flagtab)
{
    FLAGENT *fp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.flags_htab, 521);
    nbuf = alloc_sbuf("init_flagtab");
    for (fp = gen_flags; fp->flagname && *fp->flagname != '\0'; fp++) {
      for (np = nbuf, bp = (char *) fp->flagname; *bp; np++, bp++)
	*np = ToLower((int)*bp);
      *np = '\0';
      hashadd2(nbuf, (int *) fp, &mudstate.flags_htab, 1);
    }
    free_sbuf(nbuf);
}

void 
NDECL(init_toggletab)
{
    TOGENT *tp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.toggles_htab, 131);
    nbuf = alloc_sbuf("init_toggletab");
    for (tp = tog_table; tp->togglename; tp++) {
	for (np = nbuf, bp = (char *) tp->togglename; *bp; np++, bp++)
	    *np = ToLower((int)*bp);
	*np = '\0';
	hashadd(nbuf, (int *) tp, &mudstate.toggles_htab);
    }
    free_sbuf(nbuf);
}

void 
NDECL(init_powertab)
{
    POWENT *tp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.powers_htab, 131);
    nbuf = alloc_sbuf("init_powertab");
    for (tp = pow_table; tp->powername; tp++) {
	for (np = nbuf, bp = (char *) tp->powername; *bp; np++, bp++)
	    *np = ToLower((int)*bp);
	*np = '\0';
	hashadd(nbuf, (int *) tp, &mudstate.powers_htab);
    }
    free_sbuf(nbuf);
}

void 
NDECL(init_depowertab)
{
    POWENT *tp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.depowers_htab, 131);
    nbuf = alloc_sbuf("init_powertab");
    for (tp = depow_table; tp->powername; tp++) {
	for (np = nbuf, bp = (char *) tp->powername; *bp; np++, bp++)
	    *np = ToLower((int)*bp);
	*np = '\0';
	hashadd(nbuf, (int *) tp, &mudstate.depowers_htab);
    }
    free_sbuf(nbuf);
}

/* ---------------------------------------------------------------------------
 * display_flags: display available flags.
 */

static int flagent_comp(const void *s1, const void *s2)
{
  return strcmp((*(FLAGENT **) s1)->flagname, (*(FLAGENT **) s2)->flagname);
}

void display_flagtab2(dbref player, char *buff, char **bufcx)
{
    char *buf, *bp;
    const FLAGENT *ptrs[LBUF_SIZE/2];
    int f_int, nptrs, i;
    FLAGENT *fp;

    bp = buf = alloc_lbuf("display_flagtab");
    f_int = 0;
    nptrs = 0;

    for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	 fp;
	 fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {

	if ((fp->listperm & CA_BUILDER) && !Builder(player))
	    continue;
	if ((fp->listperm & CA_ADMIN) && !Admin(player))
	    continue;
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    continue;
	if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
	    continue;
	if ((fp->listperm & CA_GOD) && !God(player))
	    continue;
	ptrs[nptrs] = fp;
	nptrs++;
    }       

    qsort(ptrs, nptrs, sizeof(FLAGENT *), flagent_comp);

    for (i = 0 ; i < nptrs ; i++) {
      if ( f_int )
	safe_chr(' ', buf, &bp);
      f_int = 1;
      safe_str((char *) ptrs[i]->flagname, buf, &bp);
      safe_chr('(', buf, &bp);
      if ( (ptrs[i]->flagflag & FLAG3) || (ptrs[i]->flagflag & FLAG4) )
         safe_chr('[', buf, &bp);
      safe_chr(ptrs[i]->flaglett, buf, &bp);
      if ( (ptrs[i]->flagflag & FLAG3) || (ptrs[i]->flagflag & FLAG4) )
         safe_chr(']', buf, &bp);
      safe_chr(')', buf, &bp);
    }

    *bp = '\0';
    safe_str(buf, buff, bufcx);
    free_lbuf(buf);

}

void 
display_flagtab(dbref player)
{
    char *buf, *bp;

    bp = buf = alloc_lbuf("display_flagtab");
    safe_str((char *) "Flags : ", buf, &bp);
    display_flagtab2(player, buf, &bp);
    notify(player, buf);
    free_lbuf(buf);
}

static int togent_comp(const void *s1, const void *s2)
{
  return strcmp((*(TOGENT **) s1)->togglename, (*(TOGENT **) s2)->togglename);
}

void 
display_toggletab2(dbref player, char *buff, char **bufcx)
{
    char *buf, *bp;
    const TOGENT *ptrs[LBUF_SIZE/2];
    int f_int, nptrs, i;
    TOGENT *tp;

    bp = buf = alloc_lbuf("display_toggletab");
    f_int = 0;
    nptrs = 0;

    for (tp = (TOGENT *) hash_firstentry(&mudstate.toggles_htab);
         tp;
	 tp = (TOGENT *) hash_nextentry(&mudstate.toggles_htab)) {

	if ((tp->listperm & CA_BUILDER) && !Builder(player))
	    continue;
	if ((tp->listperm & CA_ADMIN) && !Admin(player))
	    continue;
	if ((tp->listperm & CA_WIZARD) && !Wizard(player))
	    continue;
	if ((tp->listperm & CA_IMMORTAL) && !Immortal(player))
	    continue;
	if ((tp->listperm & CA_GOD) && !God(player))
	    continue;
	
	ptrs[nptrs] = tp;
	nptrs++;
    }    
    qsort(ptrs, nptrs, sizeof(TOGENT *), togent_comp); 
     
    for(i = 0; i < nptrs ; i++) {
        if ( f_int )
	   safe_chr(' ', buf, &bp);
        f_int = 1;
	safe_str((char *) ptrs[i]->togglename, buf, &bp);
	safe_chr('(', buf, &bp);
	safe_chr(ptrs[i]->togglelett, buf, &bp);
	safe_chr(')', buf, &bp);
    }
    
    *bp = '\0';
    safe_str(buf, buff, bufcx);
    free_lbuf(buf);
}

void 
display_toggletab(dbref player)
{
    char *buf, *bp;

    bp = buf = alloc_lbuf("display_toggletab");
    
    safe_str((char *) "Toggles : ", buf, &bp);
    display_toggletab2(player, buf, &bp);
    notify(player, buf);
    free_lbuf(buf);
}

FLAGENT *
find_flag_perm(dbref thing, char *flagname, dbref player)
{
    char *cp;
    FLAGENT *fp;

    /* Make sure the flag name is valid */

    for (cp = flagname; *cp; cp++)
	*cp = ToLower((int)*cp);

    fp = (FLAGENT *)hashfind(flagname, &mudstate.flags_htab);
    if ( fp ) {
       if ((fp->listperm & CA_BUILDER) && !Builder(player))
          fp = (FLAGENT*)NULL;
       else if ((fp->listperm & CA_ADMIN) && !Admin(player))
          fp = (FLAGENT*)NULL;
       else if ((fp->listperm & CA_WIZARD) && !Wizard(player))
          fp = (FLAGENT*)NULL;
       else if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
          fp = (FLAGENT*)NULL;
       else if ((fp->listperm & CA_GOD) && !God(player))
          fp = (FLAGENT*)NULL;
    }

    return fp;
}

FLAGENT *
find_flag(dbref thing, char *flagname)
{
    char *cp;

    /* Make sure the flag name is valid */

    for (cp = flagname; *cp; cp++)
	*cp = ToLower((int)*cp);
    return (FLAGENT *) hashfind(flagname, &mudstate.flags_htab);
}

TOGENT *
find_toggle_perm(dbref thing, char *togglename, dbref player)
{
    char *cp;
    TOGENT *tp;

    /* Make sure the flag name is valid */

    for (cp = togglename; *cp; cp++)
	*cp = ToLower((int)*cp);

    tp = (TOGENT *)hashfind(togglename, &mudstate.toggles_htab);

    if ( tp ) {
       if ((tp->listperm & CA_GUILDMASTER) && !Guildmaster(player))
          tp = (TOGENT *)NULL;
       else if ((tp->listperm & CA_BUILDER) && !Builder(player))
          tp = (TOGENT *)NULL;
       else if ((tp->listperm & CA_ADMIN) && !Admin(player))
          tp = (TOGENT *)NULL;
       else if ((tp->listperm & CA_WIZARD) && !Wizard(player))
          tp = (TOGENT *)NULL;
       else if ((tp->listperm & CA_IMMORTAL) && !Immortal(player))
          tp = (TOGENT *)NULL;
       else if ((tp->listperm & CA_GOD) && !God(player))
          tp = (TOGENT *)NULL;
    }

    return tp;
}

TOGENT *
find_toggle(dbref thing, char *togglename)
{
    char *cp;

    /* Make sure the flag name is valid */

    for (cp = togglename; *cp; cp++)
	*cp = ToLower((int)*cp);
    return (TOGENT *) hashfind(togglename, &mudstate.toggles_htab);
}

POWENT *
find_power(dbref thing, char *powername)
{
    char *cp;

    for (cp = powername; *cp; cp++)
	*cp = ToLower((int)*cp);
    return (POWENT *) hashfind(powername, &mudstate.powers_htab);
}

POWENT *
find_depower(dbref thing, char *powername)
{
    char *cp;

    for (cp = powername; *cp; cp++)
	*cp = ToLower((int)*cp);
    return (POWENT *) hashfind(powername, &mudstate.depowers_htab);
}

/* ---------------------------------------------------------------------------
 * flag_set: Set or clear a specified flag on an object. 
 */

void 
flag_set(dbref target, dbref player, char *flag, int key)
{
    FLAGENT *fp;
    TOGENT *tp;
    int negate, result, perm, i_ovperm, i_uovperm, i_chkflags;
    char *pt1, *pt2, st, *tbuff, *tpr_buff, *tprp_buff, *tpr_buff2, *tprp_buff2;

    /* Trim spaces, and handle the negation character */

    result = i_chkflags = 0;
    pt1 = flag;
    st = 1;
    tbuff = alloc_lbuf("log_flag_set_information");
    tprp_buff = tpr_buff = alloc_lbuf("flag_set");
    tprp_buff2 = tpr_buff2 = alloc_lbuf("flag_set");
    while (st) {
	negate = 0;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	if (*pt1 == '!') {
	    negate = 1;
	    pt1++;
	}
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	pt2 = pt1;
	while (*pt2 && !isspace((int)*pt2))
	    pt2++;
	if (*pt2 == '\0')
	    st = 0;
	else
	    (*pt2 = 0);

	/* Make sure a flag name was specified */

	if (*pt1 == '\0') {
	    if (negate)
		notify(player, "You must specify a flag or flags to clear.");
	    else
		notify(player, "You must specify a flag or flags to set.");
	} else {

	    fp = find_flag(target, pt1);
	    if (fp == NULL) {
                tp = find_toggle_perm(target, pt1, player);
                if ( tp == NULL ) {
		   notify(player, "I don't understand that flag.");
                } else {
                   notify(player, safe_tprintf(tpr_buff, &tprp_buff, "I don't understand that flag [Did you mean @toggle me=%s?]", pt1));
                   tprp_buff = tpr_buff;
                }
	    } else {
		if ((NoMod(target) && !WizMod(player)) || (DePriv(player,Owner(target),DP_MODIFY,POWER7,NOTHING) &&
			(Owner(player) != Owner(target))) || (Backstage(player) && NoBackstage(target) &&
                        !Immortal(player)))
		  notify(player, "Permission denied.");
		else {
		  perm = 1;
		  if (!(fp->flagflag & FA_HANDLER)) {
		    if (!Controls(player,target) && 
			!HasPriv(player,target,POWER_SECURITY,POWER4,NOTHING)) {
		      notify(player, "Permission denied.");
		      perm = 0;
		    }
		  }
		    
                  /* NOMODIFY was changed to be immortal only settable/removable */
                  if ( perm && (fp->flagvalue & NOMODIFY) && (mudconf.imm_nomod) &&
                       !Immortal(player) && (fp->flagflag & FLAG3) ) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

		  /* Only Immortals can set JUMP_OK */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 7) && 
                       !Immortal(player) && 
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
		  }

		  /* Wizards can only set it on ROOMS */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 6) && 
                       !Immortal(player) && !isRoom(target) &&
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

		  /* Only Wizards/Royalty can set it */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 5) && 
                       !Wizard(player) && 
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
		  }

		  /* Councilors can only set it on ROOMS */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 4) && 
                       !Wizard(player) && !isRoom(target) &&
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

		  /* Only councilors can set it */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 3) && 
                       !Admin(player) && 
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
		  }

		  /* Architect can only set it on ROOMS */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 2) && 
                       !Admin(player) && !isRoom(target) &&
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

		  /* Only architects can set it */
                  if ( perm && (fp->flagvalue & JUMP_OK) && (mudconf.secure_jumpok > 1) && 
                       !Builder(player) && 
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
		  }

		  /* Mortals can only set it on ROOMS */
                  if ( perm && (fp->flagvalue & JUMP_OK) && mudconf.secure_jumpok && 
                       !Builder(player) && !isRoom(target) &&
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

                  if ( perm && (fp->flagvalue & DARK) && mudconf.secure_dark &&
                       !Wizard(player) && isPlayer(target) &&
                       !(fp->flagflag & (FLAG2 | FLAG3 | FLAG4))) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }
		  if ( perm && (fp->flagvalue & SIDEFX) && (fp->flagflag & FLAG3) &&
                             !((God(player) && (mudconf.restrict_sidefx <= 7)) ||
                               (Immortal(player) && (mudconf.restrict_sidefx <= 6 )) ||
                               (Wizard(player) && (mudconf.restrict_sidefx <= 5 )) ||
                               (Admin(player) && (mudconf.restrict_sidefx <= 4 )) ||
                               (Builder(player) && (mudconf.restrict_sidefx <= 3 )) ||
                               (Guildmaster(player) && (mudconf.restrict_sidefx <= 2 )) ||
			       ((Wanderer(player) || Guest(player)) && (mudconf.restrict_sidefx <=  1)) ||
                               (mudconf.restrict_sidefx == 0 )) ) {
		     notify(player, "Permission denied.");
                     perm = 0;
                  }

		/* Invoke the flag handler, and print feedback */

		  if (perm) {
                    switch (fp->flagflag) {
                       case FLAG2:
                          i_chkflags = ((Flags2(target) & fp->flagvalue) ? 1 : 0);
                          break;
                       case FLAG3:
                          i_chkflags = ((Flags3(target) & fp->flagvalue) ? 1 : 0);
                          break;
                       case FLAG4:
                          i_chkflags = ((Flags4(target) & fp->flagvalue) ? 1 : 0);
                          break;
                       default:
                          i_chkflags = ((Flags(target) & fp->flagvalue) ? 1 : 0);
                          break;
                    }
                    i_ovperm = (fp->setovperm &~ CA_LOGFLAG);
                    i_uovperm = (fp->usetovperm &~ CA_LOGFLAG);
                    if (((i_ovperm > 0) && !negate) || 
                        ((i_uovperm > 0) && negate)) {
                       if ((i_ovperm > 0) && !negate) 
                          result = check_access(player, i_ovperm, 0, 0);
                       else if ((i_uovperm > 0) && negate)
                          result = check_access(player, i_uovperm, 0, 0);
                       /* Some things you just can *not* override */
                       if ( result && ((fp->handler == fh_byeroom_bit) ||
                                       (fp->handler == fh_zonemaster) ||
                                       (fp->handler == fh_zonecontents) ||
                                       (fp->handler == fh_none) ||
                                       (fp->handler == fh_unfind_bit) ||
                                       (fp->handler == fh_hear_bit) ||
                                       (fp->handler == fh_inherit)) ) {
                          result = fp->handler(target, player, fp->flagvalue,
				               fp->flagflag, negate);
                       /* Some things have to check against powers */
                       } else if ( (fp->handler == fh_builder_sec) ||
                                   (fp->handler == fh_wiz_sec) ) {
                          if ( result )
                             fh_any(target, player, fp->flagvalue, 
                                    fp->flagflag, negate);
                          else
                             result = fh_any_sec(target, player, fp->flagvalue, 
                                                 fp->flagflag, negate);
                       } else if (result) {
                          fh_any(target, player, fp->flagvalue, 
                                 fp->flagflag, negate);
                       }
                    } else
		       result = fp->handler(target, player, fp->flagvalue,
				        fp->flagflag, negate);
		    if (!result)
		      notify(player, "Permission denied.");
		    else if (!(key & SET_QUIET) && !Quiet(player)) {
		      if ( (key & SET_NOISY) || (TogNoisy(player)) ) {
                        tprp_buff = tpr_buff;
                        tprp_buff2 = tpr_buff2;
		        notify( player, (negate ? safe_tprintf(tpr_buff, &tprp_buff, 
                                                 "Set - %s (cleared flag%s%s).", 
                                                 Name(target), (i_chkflags ? " " : " [again] "),
                                                 fp->flagname) : safe_tprintf(tpr_buff2, 
                                                           &tprp_buff2, 
                                                           "Set - %s (set flag%s%s).", 
                                                           Name(target), (i_chkflags ? " [again] " : " "),
                                                           fp->flagname)) );
		      } else
		        notify(player, (negate ? "Cleared." : "Set."));
		    }
                    if ( result ) {
                       if ( negate && (fp->usetovperm & CA_LOGFLAG) ) {
                          STARTLOG(LOG_ALWAYS, "SEC", "FLAG")
                             sprintf(tbuff, "Flag %s removed by %s(#%d) on %s(#%d).", 
                                      fp->flagname, Name(player), player, Name(target), target);
                             log_text(tbuff);
                          ENDLOG
                       } else if (!negate && fp->setovperm & CA_LOGFLAG) {
                          STARTLOG(LOG_ALWAYS, "SEC", "FLAG")
                             sprintf(tbuff, "Flag %s set by %s(#%d) on %s(#%d).", 
                                      fp->flagname, Name(player), player, Name(target), target);
                             log_text(tbuff);
                          ENDLOG
                       } 
                    }
		  }
		}
	    }
	}
	if (st)
	    pt1 = pt2 + 1;
    }
    free_lbuf(tpr_buff);
    free_lbuf(tpr_buff2);
    free_lbuf(tbuff);
}

void 
toggle_set(dbref target, dbref player, char *toggle, int key)
{
    FLAGENT *fp;
    TOGENT *tp;
    FLAG i_flag;
    int negate, result, i_flagchk;
    char *pt1, *pt2, st, *tpr_buff, *tprp_buff;

    /* Trim spaces, and handle the negation character */

    pt1 = toggle;
    st = 1;
    while (st) {
	negate = 0;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	if (*pt1 == '!') {
	    negate = 1;
	    pt1++;
	}
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	pt2 = pt1;
	while (*pt2 && !isspace((int)*pt2))
	    pt2++;
	if (*pt2 == '\0')
	    st = 0;
	else
	    (*pt2 = 0);

	/* Make sure a toggle name was specified */

	if (*pt1 == '\0') {
	    if (negate) {
              if ( !(key & SIDEEFFECT) )
		notify(player, "You must specify a toggle or toggles to clear.");
	    } else {
              if ( !(key & SIDEEFFECT) )
		notify(player, "You must specify a toggle or toggles to set.");
            }
	} else {
	    tp = find_toggle(target, pt1);
	    if (tp == NULL) {
              if ( !(key & SIDEEFFECT) || ((*pt1 != '\0') && (key & SIDEEFFECT) && !(key & SET_QUIET)) )
                fp = find_flag_perm(target, pt1, player);
                if ( fp == NULL ) {
		   notify(player, "I don't understand that toggle.");
                } else {
                   tprp_buff = tpr_buff = alloc_lbuf("toggle_message");
                   notify(player, safe_tprintf(tpr_buff, &tprp_buff, "I don't understand that toggle [Did you mean @set me=%s?]", pt1));
                   free_lbuf(tpr_buff);
                }
	    } else {
		if ((NoMod(target) && !WizMod(player)) || 
                    (DePriv(player,Owner(target),DP_MODIFY,POWER7,NOTHING) &&
		    (Owner(player) != Owner(target))) || (Backstage(player) && NoBackstage(target) && 
                    !Immortal(player))) {
		     notify(player, "Permission denied.");
		} else {

		/* Invoke the toggle handler, and print feedback */

                  if ( tp->toggleflag & TOGGLE2 ) {
                     i_flag = Toggles2(target);
                  } else {
                     i_flag = Toggles(target);
                  }
                  i_flagchk = !(tp->togglevalue & i_flag);
		  result = tp->handler(target, player, tp->togglevalue,
				     tp->toggleflag, negate);
		  if (!result)
		    notify(player, "Permission denied.");
		  else if (!(key & (SET_QUIET|SIDEEFFECT)) && !Quiet(player)) {
                    if ( (key & SET_NOISY) || TogNoisy(player) ) {
                       tprp_buff = tpr_buff = alloc_lbuf("do_set");
                       if ( negate ) {
                          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s (cleared toggle%s%s).",
                                       Name(target), (!i_flagchk ? " " : " [again] "),
                                       tp->togglename) );
                       } else {
                          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Set - %s (set toggle%s%s).",
                                       Name(target), (i_flagchk ? " " : " [again] "),
                                       tp->togglename) );
                       }
                       free_lbuf(tpr_buff);
                    } else {
		       notify(player, (negate ? "Cleared." : "Set."));
                    }
                  }
		}
	    }
	}
	if (st)
	    pt1 = pt2 + 1;
    }
}

void 
power_set(dbref target, dbref player, char *power, int key)
{
    POWENT *tp;
    int negate, result, slevel;
    char *pt1, *pt2, st;

    if (key == POWER_PURGE) {
	s_Toggles3(target, 0);
	s_Toggles4(target, 0);
	s_Toggles5(target, 0);
	notify(player, "Powers purged.");
	return;
    }
    /* Trim spaces, and handle the negation character */
    pt1 = power;
    st = 1;
    while (st) {
	negate = 0;
	slevel = key;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	if (*pt1 == '!') {
	    negate = 1;
	    pt1++;
	} else if (*pt1 == '@') {
	    pt1++;
	    switch (toupper(*pt1)) {
	    case 'O':
		slevel = POWER_LEVEL_OFF;
		pt1++;
		break;
	    case 'G':
		slevel = POWER_LEVEL_GUILD;
		pt1++;
		break;
	    case 'A':
		slevel = POWER_LEVEL_ARCH;
		pt1++;
		break;
	    case 'C':
		slevel = POWER_LEVEL_COUNC;
		pt1++;
		break;
	    default:
		notify(player, "Unknown level. Using switch value.");
	    }
	}
	if (negate)
	    slevel = POWER_LEVEL_OFF;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	pt2 = pt1;
	while (*pt2 && !isspace((int)*pt2))
	    pt2++;
	if (*pt2 == '\0')
	    st = 0;
	else
	    (*pt2 = 0);

	/* Make sure a power name was specified */

	if (*pt1 == '\0') {
	    if (slevel == POWER_LEVEL_OFF)
		notify(player, "You must specify a power or powers to clear.");
	    else
		notify(player, "You must specify a power or powers to set.");
	} else {

	    tp = find_power(target, pt1);
	    if (tp == NULL) {
		notify(player, "I don't understand that power.");
	    } else {
		if ((NoMod(target) && !WizMod(player)) || (DePriv(player,Owner(target),DP_MODIFY,POWER7,NOTHING) &&
			(Owner(player) != Owner(target))) || (Backstage(player) && NoBackstage(target) &&
                        !Immortal(player)))
		  notify(player, "Permission denied.");
		else {

		/* Invoke the power handler, and print feedback */

		  result = tp->handler(target, player, tp->powerpos,
				     tp->powerflag, tp->powerlev, slevel);
		  if (!result)
		    notify(player, "Permission denied.");
		  else if (!Quiet(player)) {
		    if (slevel == POWER_LEVEL_OFF)
			notify(player, "Cleared.");
		    else
			notify(player, "Set.");
		  }
		}
	    }
	}
	if (st)
	    pt1 = pt2 + 1;
    }
}

void 
depower_set(dbref target, dbref player, char *power, int key)
{
    POWENT *tp;
    int negate, result, slevel;
    char *pt1, *pt2, st;

    if (key == POWER_PURGE) {
	s_Toggles6(target, 0);
	s_Toggles7(target, 0);
	s_Toggles8(target, 0);
	notify(player, "Depowers purged.");
	return;
    }
    /* Trim spaces, and handle the negation character */
    pt1 = power;
    st = 1;
    while (st) {
	negate = 0;
	slevel = key;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	if (*pt1 == '!') {
	    negate = 1;
	    pt1++;
	} else if (*pt1 == '@') {
	    pt1++;
	    switch (toupper(*pt1)) {
	    case 'O':
		slevel = POWER_LEVEL_OFF;
		pt1++;
		break;
	    case 'G':
		slevel = POWER_LEVEL_GUILD;
		pt1++;
		break;
	    case 'A':
		slevel = POWER_LEVEL_ARCH;
		pt1++;
		break;
	    case 'C':
		slevel = POWER_LEVEL_COUNC;
		pt1++;
		break;
	    case 'R':
		slevel = POWER_REMOVE;
		pt1++;
		break;
	    default:
		notify(player, "Unknown level. Using switch value.");
	    }
	}
	if (negate)
	    slevel = POWER_REMOVE;
	while (*pt1 && isspace((int)*pt1))
	    pt1++;
	pt2 = pt1;
	while (*pt2 && !isspace((int)*pt2))
	    pt2++;
	if (*pt2 == '\0')
	    st = 0;
	else
	    (*pt2 = 0);

	/* Make sure a power name was specified */

	if (*pt1 == '\0') {
	    if (slevel == POWER_REMOVE)
		notify(player, "You must specify a depower or depowers to clear.");
	    else
		notify(player, "You must specify a depower or depowers to set.");
	} else {

	    tp = find_depower(target, pt1);
	    if (tp == NULL) {
		notify(player, "I don't understand that depower.");
	    } else {
		if ((NoMod(target) && !WizMod(player)) || (DePriv(player,Owner(target),DP_MODIFY,POWER7,NOTHING) &&
			(Owner(player) != Owner(target))) || (Backstage(player) && NoBackstage(target) &&
                        !Immortal(player)))
		  notify(player, "Permission denied.");
		else {

		/* Invoke the power handler, and print feedback */

		  result = tp->handler(target, player, tp->powerpos,
				     tp->powerflag, tp->powerlev, slevel);
		  if (!result)
		    notify(player, "Permission denied.");
		  else if (!Quiet(player)) {
		    if (slevel == POWER_REMOVE)
			notify(player, "Cleared.");
		    else
			notify(player, "Set.");
		  }
		}
	    }
	}
	if (st)
	    pt1 = pt2 + 1;
    }
}
/* ---------------------------------------------------------------------------
 * decode_flags: converts a flags word into corresponding letters.
 */

char *
decode_flags(dbref player, dbref target, FLAG flagword, FLAG flag2word,
	     FLAG flag3word, FLAG flag4word)
{
    char *buf, *bp, *buf2, *bp2;
    FLAGENT *fp;
    int flagtype, check;
    FLAG fv;

    buf = bp = alloc_mbuf("decode_flags");
    buf2 = bp2 = alloc_mbuf("decode_flags2");
    *bp2 = *bp = '\0';

    if (!Good_obj(player) || !Good_obj(target)) {
	strcpy(buf, "#-2 ERROR");
	return buf;
    }
    check = 0;
    flagtype = (flagword & TYPE_MASK);
    if (object_types[flagtype].lett != ' ')
	safe_mb_chr(object_types[flagtype].lett, buf, &bp);

    for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	 fp;
	 fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {
	if (fp->flagflag & FLAG2)
	    fv = flag2word;
	else if (fp->flagflag & FLAG3) {
	    fv = flag3word;
	} else if (fp->flagflag & FLAG4) {
	    fv = flag4word;
	} else
	    fv = flagword;
	if (fv & fp->flagvalue) {
	    if ((fp->listperm & CA_BUILDER) && !Builder(player))
		continue;
	    if ((fp->listperm & CA_ADMIN) && !Admin(player))
		continue;
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;

	    /* don't show CONNECT on dark wizards to mortals */
	    if ((flagtype == TYPE_PLAYER) &&
		(fp->flagflag & FLAG2) &&
		(fp->flagvalue == CONNECTED) ) { /* connect */
              if( Unfindable(target) && mudconf.who_unfindable && !HasPriv(player, target, POWER_WIZ_WHO,
                                             POWER3, NOTHING) && 
                  !Examinable(player, target) &&
                  !(Wizard(player) && Wizard(target)) ) {
                continue;
	      }
              if( SCloak(target) && Cloak(target) && !Immortal(player) ) {
                continue;    
	      }
            }
            /* If hide_nospoof enabled only show NOSPOOF flag to controller */
            if ( (mudconf.hide_nospoof) && (!(fp->flagflag) && (fp->flagvalue == NOSPOOF)) &&
                 !Controls(player, target) ) {
               continue;
            }
            /* If DARK to be tinymush compatible, don't show connect flag */
            if ( (!(mudconf.who_unfindable) && !(mudconf.player_dark) && mudconf.allow_whodark) &&
                 !Admin(player) && Wizard(target) && Dark(target) && (player != target) && 
                 ((fp->flagflag & FLAG2) && (fp->flagvalue == CONNECTED)) )
               continue;

            /* Don't show staff flag if protected */
            if ( (((fp->flagflag & FLAG2) && ((fp->flagvalue == GUILDMASTER) ||
                  (fp->flagvalue == ADMIN) || (fp->flagvalue == BUILDER))) ||
                 (!(fp->flagflag) && ((fp->flagvalue == IMMORTAL) ||
                  (fp->flagvalue == WIZARD)))) &&
                 (HasPriv(target, player, POWER_HIDEBIT, POWER5, NOTHING) && 
                  (Owner(player) != Owner(target) || mudstate.objevalst)) ) {
               continue;
            }
	    if (fp->flagflag & FLAG3 || fp->flagflag & FLAG4) {
	      safe_mb_chr(fp->flaglett, buf2, &bp2);
	    } else {
	      safe_mb_chr(fp->flaglett, buf, &bp);
	    }
	}
    }
    if (buf2[0] != '\0') {
      *bp2 = '\0';
      safe_mb_chr('[', buf, &bp);
      safe_str(buf2, buf, &bp);
      safe_mb_chr(']', buf, &bp);
    }

    free_mbuf(buf2);

    *bp = '\0';
    return buf;
}
/* ---------------------------------------------------------------------------
 * decode_flags_func: converts a flags word into corresponding letters.
 * This will break it up into two sets.  First is flag1/flag2, second is
 * flag3/flag4
 */

void
decode_flags_func(dbref player, dbref target, FLAG flagword, FLAG flag2word,
	     FLAG flag3word, FLAG flag4word, char *flaglow, char *flaghigh)
{
    char *buf, *bp, *buf2, *bp2;
    FLAGENT *fp;
    int flagtype, check;
    FLAG fv;

    buf = bp = alloc_mbuf("decode_flags_func_mbuf");
    buf2 = bp2 = alloc_mbuf("decode_flags_func_mbuf2");
    *bp = '\0';

    if (!Good_obj(player) || !Good_obj(target)) {
	strcpy(buf, "#-2 ERROR");
	return;
    }
    flagtype = (flagword & TYPE_MASK);
    if (object_types[flagtype].lett != ' ')
	safe_mb_chr(object_types[flagtype].lett, buf, &bp);

    for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	 fp;
	 fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {
        check = 0;
	if (fp->flagflag & FLAG2)
	    fv = flag2word;
	else if (fp->flagflag & FLAG3) {
	    fv = flag3word;
	} else if (fp->flagflag & FLAG4) {
	    fv = flag4word;
	} else
	    fv = flagword;
	if (fv & fp->flagvalue) {
	    if ((fp->listperm & CA_BUILDER) && !Builder(player))
		continue;
	    if ((fp->listperm & CA_ADMIN) && !Admin(player))
		continue;
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;
	    if (fp->flagflag & (FLAG3 | FLAG4)) {
		if (!check && (flag3word || flag4word)) {
		    check = 1;
		}
	    }
	    /* don't show CONNECT on dark wizards to mortals */
	    if ((flagtype == TYPE_PLAYER) &&
		(fp->flagflag & FLAG2) &&
		(fp->flagvalue == CONNECTED) ) { /* connect */
              if( Unfindable(target) && mudconf.who_unfindable && !HasPriv(player, target, POWER_WIZ_WHO,
                                             POWER3, NOTHING) && 
                  !Examinable(player, target) &&
                  !(Wizard(player) && Wizard(target)) ) {
                continue;
	      }
              if( SCloak(target) && Cloak(target) && !Immortal(player) ) {
                continue;    
	      }
            }
            /* If hide_nospoof enabled only show NOSPOOF flag to controller */
            if ( (mudconf.hide_nospoof) && (!(fp->flagflag) && (fp->flagvalue == NOSPOOF)) &&
                 !Controls(player, target) ) {
               continue;
            }

            /* If DARK to be tinymush compatible, don't show connect flag */
            if ( (!(mudconf.who_unfindable) && !(mudconf.player_dark) && mudconf.allow_whodark) &&
                 !Admin(player) && Wizard(target) && Dark(target) && (player != target) && 
                 ((fp->flagflag & FLAG2) && (fp->flagvalue == CONNECTED)) )
               continue;

            /* Don't show staff flag if protected */
            if ( (((fp->flagflag & FLAG2) && ((fp->flagvalue == GUILDMASTER) ||
                  (fp->flagvalue == ADMIN) || (fp->flagvalue == BUILDER))) ||
                 (!(fp->flagflag) && ((fp->flagvalue == IMMORTAL) ||
                  (fp->flagvalue == WIZARD)))) &&
                 (HasPriv(target, player, POWER_HIDEBIT, POWER5, NOTHING) && 
                  (Owner(player) != Owner(target) || mudstate.objevalst)) ) {
               continue;
            }

	    if ( !check )
	       safe_mb_chr(fp->flaglett, buf, &bp);
	    else
	       safe_mb_chr(fp->flaglett, buf2, &bp2);
	}
    }

    *bp = '\0';
    *bp2 = '\0';
    strcpy(flaglow, buf);
    strcpy(flaghigh, buf2);
    free_mbuf(buf);
    free_mbuf(buf2);
}

/* ---------------------------------------------------------------------------
 * has_flag: does object have flag visible to player?
 */

int 
has_flag(dbref player, dbref it, char *flagname)
{
    FLAGENT *fp;
    FLAG fv;

    fp = find_flag(it, flagname);
    if (fp == NULL)
	return 0;

    if (fp->flagflag & FLAG2)
	fv = Flags2(it);
    else if (fp->flagflag & FLAG3)
	fv = Flags3(it);
    else if (fp->flagflag & FLAG4)
	fv = Flags4(it);
    else
	fv = Flags(it);

    if (fv & fp->flagvalue) {
	if ((fp->listperm & CA_BUILDER) && !Builder(player))
	    return 0;
	if ((fp->listperm & CA_ADMIN) && !Admin(player))
	    return 0;
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    return 0;
	if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
	    return 0;
	if ((fp->listperm & CA_GOD) && !God(player))
	    return 0;
	/* don't show CONNECT on dark wizards to mortals */
	if (isPlayer(it) &&
	    (fp->flagflag & FLAG2) &&
	    (fp->flagvalue == CONNECTED)) { /* this is a connected flag */
          if( Unfindable(it) && mudconf.who_unfindable && !HasPriv(player, it, POWER_WIZ_WHO,
                                         POWER3, NOTHING) && 
              !Examinable(player, it) &&
              !(Wizard(player) && Wizard(it)) )
            return 0;
          if( SCloak(it) && Cloak(it) && !Immortal(player) ) 
            return 0;    
        }
        /* If hide_nospoof enabled only show NOSPOOF flag to controller */
        if ( (mudconf.hide_nospoof) && (!(fp->flagflag) && (fp->flagvalue == NOSPOOF)) &&
             !Controls(player, it) ) {
           return 0;
        }
        /* If DARK to be tinymush compatible, don't show connect flag */
        if ( (!(mudconf.who_unfindable) && !(mudconf.player_dark) && mudconf.allow_whodark) &&
             !Admin(player) && Wizard(it) && Dark(it) && (player != it) && 
             ((fp->flagflag & FLAG2) && (fp->flagvalue == CONNECTED)) )
           return 0;

        /* If person has HIDEBIT power hide bitlevel from player */
        if ( (((fp->flagflag & FLAG2) && ((fp->flagvalue == GUILDMASTER) ||
              (fp->flagvalue == ADMIN) || (fp->flagvalue == BUILDER))) ||
             (!(fp->flagflag) && ((fp->flagvalue == IMMORTAL) ||
              (fp->flagvalue == WIZARD)))) &&
             (HasPriv(it, player, POWER_HIDEBIT, POWER5, NOTHING) &&
              (Owner(it) != Owner(player) || mudstate.objevalst)) ) {
           return 0;
        }

	return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * flag_description: Return an lbuf containing the type and flags on thing.
 */

char *
flag_description(dbref player, dbref target, int test, int *vp, int i_type)
{
    char *buff, *bp;
    FLAGENT *fp; 
    FLAGSET *fset;
    int otype;
    FLAG fv;

    /* Allocate the return buffer */

    fset = (FLAGSET *) vp;

    if ( vp )
       otype = i_type;
    else
       otype = Typeof(target);
    bp = buff = alloc_lbuf("flag_description");

    /* Store the header strings and object type */

    if (test) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "Type: ", buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    safe_str((char *) object_types[otype].name, buff, &bp);
    if (test) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) " Flags:", buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    if (object_types[otype].perm != CA_PUBLIC) {
	*bp = '\0';
	return buff;
    }
    /* Store the type-invariant flags */

    for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	 fp;
	 fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {
        if ( vp ) {
	   if (fp->flagflag & FLAG2)
               fv = (*fset).word2;
	   else if (fp->flagflag & FLAG3)
	       fv = (*fset).word3;
	   else if (fp->flagflag & FLAG4)
	       fv = (*fset).word4;
	   else
	       fv = (*fset).word1;
        } else {
	   if (fp->flagflag & FLAG2)
	       fv = Flags2(target);
	   else if (fp->flagflag & FLAG3)
	       fv = Flags3(target);
	   else if (fp->flagflag & FLAG4)
	       fv = Flags4(target);
	   else
	       fv = Flags(target);
        }
	if (fv & fp->flagvalue) {
	    if ((fp->listperm & CA_BUILDER) && !Builder(player))
		continue;
	    if ((fp->listperm & CA_ADMIN) && !Admin(player))
		continue;
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;
	    /* don't show CONNECT on dark wizards to mortals */
	    if (isPlayer(target) &&
		(fp->flagflag & FLAG2) &&
		(fp->flagvalue == CONNECTED)) { /* it's a connect */
              if( Unfindable(target) && mudconf.who_unfindable && !HasPriv(player, target, POWER_WIZ_WHO,
                                             POWER3, NOTHING) &&
                  !Examinable(player, target) &&
                  !(Wizard(player) && Wizard(target)) )
                continue;
              if( SCloak(target) && Cloak(target) && !Immortal(player) ) 
                continue;    
            }
            /* If hide_nospoof enabled only show NOSPOOF flag to controller */
            if ( (mudconf.hide_nospoof) && (!(fp->flagflag) && (fp->flagvalue == NOSPOOF)) &&
                 !Controls(player, target) ) {
               continue;
            }
            /* If DARK to be tinymush compatible, don't show connect flag */
            if ( (!(mudconf.who_unfindable) && !(mudconf.player_dark) && mudconf.allow_whodark) &&
                 !Admin(player) && Wizard(target) && Dark(target) && (player != target) && 
                 ((fp->flagflag & FLAG2) && (fp->flagvalue == CONNECTED)) )
               continue;

            /* Don't show bit flags if protected */
            if ( (((fp->flagflag & FLAG2) && ((fp->flagvalue == GUILDMASTER) ||
                  (fp->flagvalue == ADMIN) || (fp->flagvalue == BUILDER))) ||
                 (!(fp->flagflag) && ((fp->flagvalue == IMMORTAL) ||
                  (fp->flagvalue == WIZARD)))) &&
                 (HasPriv(target, player, POWER_HIDEBIT, POWER5, NOTHING) &&
                  (Owner(target) != Owner(player) || mudstate.objevalst)) ) {
               continue;
            }
	    safe_chr(' ', buff, &bp);
	    safe_str((char *) fp->flagname, buff, &bp);
	}
    }

    /* Terminate the string, and return the buffer to the caller */

    *bp = '\0';
    return buff;
}

char *
power_description(dbref player, dbref target, int flag, int f2)
{
    char *buff, *bp;
    POWENT *tp;
    int tv, work, level, f2a;

    bp = buff = alloc_lbuf("power_description");
    if (f2) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "Powers", buff, &bp);
	if (flag) {
	    safe_str((char *) " for ", buff, &bp);
	    safe_str(Name(target), buff, &bp);
	}
	safe_chr(':', buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
	if ((!Wizard(player) && (player != target)) || DePriv(player,NOTHING,DP_POWER,POWER7,POWER_LEVEL_NA)) {
	    *bp = '\0';
	    return buff;
	}
    }
    f2a = f2;
    for (tp = pow_table; tp->powername; tp++) {
	if (tp->powerflag & POWER3)
	    tv = Toggles3(target);
	else if (tp->powerflag & POWER4)
	    tv = Toggles4(target);
	else
	    tv = Toggles5(target);
	work = tv;
	work >>= (tp->powerpos);
	level = work & POWER_LEVEL_COUNC;
	if (level) {
	    if ((tp->powerperm & CA_GUILDMASTER) && !Guildmaster(player))
		continue;
	    if ((tp->powerperm & CA_BUILDER) && !Builder(player))
		continue;
	    if ((tp->powerperm & CA_ADMIN) && !Admin(player))
		continue;
	    if ((tp->powerperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((tp->powerperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((tp->powerperm & CA_GOD) && !God(player))
		continue;
	    if (tp->powerlev == POWER_LEVEL_OFF)
		continue;
	    if (f2a)
		safe_chr(' ', buff, &bp);
	    else
		f2a = 1;
	    safe_str((char *) tp->powername, buff, &bp);
	    if (tp->powerlev == POWER_LEVEL_NA)
		continue;
	    safe_chr('(', buff, &bp);
	    switch (level) {
	    case POWER_LEVEL_GUILD:
		safe_str("Guildmaster", buff, &bp);
		break;
	    case POWER_LEVEL_ARCH:
		safe_str("Architect", buff, &bp);
		break;
	    case POWER_LEVEL_COUNC:
		safe_str("Councilor", buff, &bp);
	    }
	    safe_chr(')', buff, &bp);
	}
    }
    *bp = '\0';
    return buff;
}

char *
depower_description(dbref player, dbref target, int flag, int f2)
{
    char *buff, *bp;
    POWENT *tp;
    int tv, work, level, f2a;

    bp = buff = alloc_lbuf("depower_description");
    if (!Immortal(player)) {
	*bp = '\0';
	return bp;
    }
    if (f2) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "Depowers", buff, &bp);
	if (flag) {
	    safe_str((char *) " for ", buff, &bp);
	    safe_str(Name(target), buff, &bp);
	}
	safe_chr(':', buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    f2a = f2;
    for (tp = depow_table; tp->powername; tp++) {
	if (tp->powerflag & POWER6)
	    tv = Toggles6(target);
	else if (tp->powerflag & POWER7)
	    tv = Toggles7(target);
	else
	    tv = Toggles8(target);
	work = tv;
	work >>= (tp->powerpos);
	level = work & POWER_LEVEL_COUNC;
	if (level) {
	    if ((tp->powerperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((tp->powerperm & CA_GOD) && !God(player))
		continue;
	    if (tp->powerlev == POWER_REMOVE)
		continue;
	    if (f2a)
		safe_chr(' ', buff, &bp);
	    else
		f2a = 1;
	    safe_str((char *) tp->powername, buff, &bp);
	    if (tp->powerlev == POWER_LEVEL_NA)
		continue;
	    safe_chr('(', buff, &bp);
	    if (DPShift(target)) {
		switch (level) {
		case POWER_LEVEL_GUILD:
		    safe_str("Guildmaster", buff, &bp);
		    break;
		case POWER_LEVEL_ARCH:
		    safe_str("Architect", buff, &bp);
		    break;
		case POWER_LEVEL_COUNC:
		    safe_str("Councilor", buff, &bp);
		}
	    } else {
		switch (level) {
		case POWER_LEVEL_GUILD:
		    safe_str("Off", buff, &bp);
		    break;
		case POWER_LEVEL_ARCH:
		    safe_str("Guildmaster", buff, &bp);
		    break;
		case POWER_LEVEL_COUNC:
		    safe_str("Architect", buff, &bp);
		}
	    }
	    safe_chr(')', buff, &bp);
	}
    }
    *bp = '\0';
    return buff;
}

char *
toggle_description(dbref player, dbref target, int test, int keyval, int *vp)
{
    char *buff, *bp;
    TOGENT *tp;
    FLAGSET *fset;
    FLAG tv;
    int t2;

    /* Allocate the return buffer */

    bp = buff = alloc_lbuf("toggle_description");
 
    fset = (FLAGSET *) vp;

    if (test) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "Toggles:", buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    t2 = test;

    for (tp = (TOGENT *) hash_firstentry(&mudstate.toggles_htab); 
    	 tp;
	 tp = (TOGENT *) hash_nextentry(&mudstate.toggles_htab)) {
    
        if ( vp ) {
	   if (tp->toggleflag & TOGGLE2)
	       tv = (*fset).word2;
	   else
	       tv = (*fset).word1;
        } else {
	   if (tp->toggleflag & TOGGLE2)
	       tv = Toggles2(target);
	   else
	       tv = Toggles(target);
        }
	if (tv & tp->togglevalue) {
	    if ((tp->listperm & CA_GUILDMASTER) && !Guildmaster(player))
		continue;
	    if ((tp->listperm & CA_BUILDER) && !Builder(player))
		continue;
	    if ((tp->listperm & CA_ADMIN) && !Admin(player))
		continue;
	    if ((tp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((tp->listperm & CA_IMMORTAL) && !Immortal(player))
		continue;
	    if ((tp->listperm & CA_GOD) && !God(player))
		continue;
	    if (t2)
		safe_chr(' ', buff, &bp);
	    else
		t2 = 1;
            if ( keyval )
               safe_chr('!', buff, &bp);
	    safe_str((char *) tp->togglename, buff, &bp);
	}
    }

    /* Terminate the string, and return the buffer to the caller */

    *bp = '\0';
    return buff;
}

/* ---------------------------------------------------------------------------
 * Return an lbuf containing the name and number of an object
 */

char *
unparse_object_numonly(dbref target)
{
    char *buf;

    buf = alloc_lbuf("unparse_object_numonly");
    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
	sprintf(buf, "%.3900s(#%d)", Name(target), target);
    }
    return buf;
}

/* ---------------------------------------------------------------------------
 * Return an lbuf pointing to the object name and possibly the db# and flags
 */

char *
unparse_object(dbref player, dbref target, int obey_myopic)
{
    char *buf, *fp;
    int exam;

    buf = alloc_lbuf("unparse_object");
    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (target == AMBIGUOUS) {
	strcpy(buf, "*AMBIGUOUS*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
	if (obey_myopic)
	    exam = MyopicExam(player, target);
	else
	    exam = Examinable(player, target);
	if (exam || (!NoEx(target) && 
	    ((Flags2(target) & ABODE) ||
	     (Flags(target) & CHOWN_OK && (!Guildmaster(target))) ||
	     (Flags(target) & (JUMP_OK | LINK_OK | DESTROY_OK)))) ) {

	    /* show everything */
	    fp = unparse_flags(player, target);
	    sprintf(buf, "%.3500s(#%d%.400s)", Name(target), target, fp);
	    free_mbuf(fp);
	} else {
	    /* show only the name. */
            sprintf(buf, "%.3900s", Name(target));
	}
    }
    if ( Good_obj(target) && isRoom(target) && NoName(target) && !Wizard(player) )
       memset(buf, 0, LBUF_SIZE);

    return buf;
}
char *
unparse_object_altname(dbref player, dbref target, int obey_myopic)
{
    char *buf, *buf2, *fp, name_str[31];
    int exam, aowner;
    dbref aflags;

    buf = alloc_lbuf("unparse_object");
    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (target == AMBIGUOUS) {
	strcpy(buf, "*AMBIGUOUS*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
        buf2 = atr_get(target, A_ALTNAME, &aowner, &aflags);
        memset(name_str, 0, sizeof(name_str));
        strncpy(name_str, buf2, 30); 
        free_lbuf(buf2);
	if (obey_myopic)
	    exam = MyopicExam(player, target);
	else
	    exam = Examinable(player, target);
	if (exam || (!NoEx(target) && 
	    ((Flags2(target) & ABODE) ||
	     (Flags(target) & CHOWN_OK && (!Guildmaster(target))) ||
	     (Flags(target) & (JUMP_OK | LINK_OK | DESTROY_OK)))) ) {

	    /* show everything */
	    fp = unparse_flags(player, target);
            if ( !Wizard(player) && name_str[0] != '\0' &&
                 !could_doit(player,target,A_LALTNAME,0) )
	       sprintf(buf, "%s(#%d%s)", name_str, target, fp);
            else if ( name_str[0] == '\0' )
	       sprintf(buf, "%.3500s(#%d%.400s)", Name(target), target, fp );
            else 
	       sprintf(buf, "%.3500s(#%d%.400s) {%.60s}", Name(target), target, fp, name_str );
	    free_mbuf(fp);
	} else {
	    /* show only the name. */
            if ( !Wizard(player) && name_str[0] != '\0' &&
                 !could_doit(player,target,A_LALTNAME,0) )
               strcpy(buf, name_str);
            else if ( name_str[0] == '\0' )
               sprintf(buf, "%.3900s", Name(target));
            else
               sprintf(buf, "%.3900s {%.60s}", Name(target), name_str );
	}
    }
    if ( Good_obj(target) && NoName(target) && !Wizard(player) )
       memset(buf, 0, LBUF_SIZE);

    return buf;
}

#ifdef ZENTY_ANSI
char *
parse_ansi_name(dbref target, char *ansibuf)
{
   char *buf2, *s, ansitmp[30], ansitmp2[30];

       buf2 = alloc_lbuf("ansi_exitname");
       memset(ansitmp, 0, sizeof(ansitmp));
       strncpy(ansitmp, ansibuf, 20);
       s = ansitmp;
       while (*s) {
          switch (*s) {
             case '0': /* Is this hex? */
                 if ( (*(s+1) == 'x') || (*(s+1) == 'X') ) {
                    if ( isxdigit(*(s+2)) && isxdigit(*(s+3)) ) {
                       sprintf(ansitmp2, "%s0%c%c%c", (char *)SAFE_CHRST, *(s+1), *(s+2), *(s+3));
                       strcat(buf2, ansitmp2);
                       s+=3;
                    }
                 }
             case 'h':		/* hilite */
                 if ( mudconf.global_ansimask & MASK_HILITE )
	            strcat(buf2, SAFE_ANSI_HILITE);
	         break;
             case 'i':		/* inverse */
                 if ( mudconf.global_ansimask & MASK_INVERSE )
	            strcat(buf2, SAFE_ANSI_INVERSE);
	         break;
             case 'f':		/* flash */
                 if ( mudconf.global_ansimask & MASK_BLINK )
	            strcat(buf2, SAFE_ANSI_BLINK);
	         break;
             case 'n':		/* normal */
	         strcat(buf2, SAFE_ANSI_NORMAL);
	         break;
             case 'x':		/* black fg */
                 if ( mudconf.global_ansimask & MASK_BLACK )
	            strcat(buf2, SAFE_ANSI_BLACK);
	         break;
             case 'r':		/* red fg */
                 if ( mudconf.global_ansimask & MASK_RED )
	            strcat(buf2, SAFE_ANSI_RED);
	         break;
             case 'g':		/* green fg */
                 if ( mudconf.global_ansimask & MASK_GREEN )
	            strcat(buf2, SAFE_ANSI_GREEN);
	         break;
             case 'y':		/* yellow fg */
                 if ( mudconf.global_ansimask & MASK_YELLOW )
	            strcat(buf2, SAFE_ANSI_YELLOW);
	         break;
             case 'b':		/* blue fg */
                 if ( mudconf.global_ansimask & MASK_BLUE )
	            strcat(buf2, SAFE_ANSI_BLUE);
	         break;
             case 'm':		/* magenta fg */
                 if ( mudconf.global_ansimask & MASK_MAGENTA )
	            strcat(buf2, SAFE_ANSI_MAGENTA);
	         break;
             case 'c':		/* cyan fg */
                 if ( mudconf.global_ansimask & MASK_CYAN )
	            strcat(buf2, SAFE_ANSI_CYAN);
	         break;
             case 'w':		/* white fg */
                 if ( mudconf.global_ansimask & MASK_WHITE )
	            strcat(buf2, SAFE_ANSI_WHITE);
	         break;
             case 'X':		/* black bg */
                 if ( mudconf.global_ansimask & MASK_BBLACK )
	            strcat(buf2, SAFE_ANSI_BBLACK);
	         break;
             case 'R':		/* red bg */
                 if ( mudconf.global_ansimask & MASK_BRED )
	            strcat(buf2, SAFE_ANSI_BRED);
	         break;
             case 'G':		/* green bg */
                 if ( mudconf.global_ansimask & MASK_BGREEN )
	            strcat(buf2, SAFE_ANSI_BGREEN);
	         break;
             case 'Y':		/* yellow bg */
                 if ( mudconf.global_ansimask & MASK_BYELLOW )
	            strcat(buf2, SAFE_ANSI_BYELLOW);
	         break;
             case 'B':		/* blue bg */
                 if ( mudconf.global_ansimask & MASK_BBLUE )
	            strcat(buf2, SAFE_ANSI_BBLUE);
	         break;
             case 'M':		/* magenta bg */
                 if ( mudconf.global_ansimask & MASK_BMAGENTA )
	            strcat(buf2, SAFE_ANSI_BMAGENTA);
	         break;
             case 'C':		/* cyan bg */
                 if ( mudconf.global_ansimask & MASK_BCYAN )
	            strcat(buf2, SAFE_ANSI_BCYAN);
	         break;
             case 'W':		/* white bg */
                 if ( mudconf.global_ansimask & MASK_BWHITE )
	            strcat(buf2, SAFE_ANSI_BWHITE);
	         break;
          }
          s++;
       }
    return buf2;
}
#else
char *
parse_ansi_name(dbref target, char *ansibuf)
{
   int i_cntr;
   char *buf2, *s;

   buf2 = alloc_lbuf("parse_ansi_name");
   memset(buf2, '\0', LBUF_SIZE);

   s = ansibuf;
   i_cntr=0;
   while (*s) {
      i_cntr++;
      if ( i_cntr > 30 )
         break;
      switch (*s) {
         case 'h':		/* hilite */
            if ( mudconf.global_ansimask & MASK_HILITE )
               strcat(buf2, ANSI_HILITE);
            break;
         case 'i':		/* inverse */
            if ( mudconf.global_ansimask & MASK_INVERSE )
               strcat(buf2, ANSI_INVERSE);
            break;
         case 'f':		/* flash */
            if ( mudconf.global_ansimask & MASK_BLINK )
                strcat(buf2, ANSI_BLINK);
            break;
         case 'n':		/* normal */
            strcat(buf2, ANSI_NORMAL);
            break;
         case 'x':		/* black fg */
            if ( mudconf.global_ansimask & MASK_BLACK )
               strcat(buf2, ANSI_BLACK);
            break;
         case 'r':		/* red fg */
            if ( mudconf.global_ansimask & MASK_RED )
               strcat(buf2, ANSI_RED);
            break;
         case 'g':		/* green fg */
            if ( mudconf.global_ansimask & MASK_GREEN )
              strcat(buf2, ANSI_GREEN);
            break;
         case 'y':		/* yellow fg */
            if ( mudconf.global_ansimask & MASK_YELLOW )
               strcat(buf2, ANSI_YELLOW);
            break;
         case 'b':		/* blue fg */
            if ( mudconf.global_ansimask & MASK_BLUE )
               strcat(buf2, ANSI_BLUE);
            break;
         case 'm':		/* magenta fg */
            if ( mudconf.global_ansimask & MASK_MAGENTA )
               strcat(buf2, ANSI_MAGENTA);
            break;
         case 'c':		/* cyan fg */
            if ( mudconf.global_ansimask & MASK_CYAN )
               strcat(buf2, ANSI_CYAN);
            break;
         case 'w':		/* white fg */
            if ( mudconf.global_ansimask & MASK_WHITE )
               strcat(buf2, ANSI_WHITE);
            break;
         case 'X':		/* black bg */
            if ( mudconf.global_ansimask & MASK_BBLACK )
               strcat(buf2, ANSI_BBLACK);
            break;
         case 'R':		/* red bg */
            if ( mudconf.global_ansimask & MASK_BRED )
               strcat(buf2, ANSI_BRED);
            break;
         case 'G':		/* green bg */
            if ( mudconf.global_ansimask & MASK_BGREEN )
               strcat(buf2, ANSI_BGREEN);
            break;
         case 'Y':		/* yellow bg */
            if ( mudconf.global_ansimask & MASK_BYELLOW )
               strcat(buf2, ANSI_BYELLOW);
            break;
         case 'B':		/* blue bg */
            if ( mudconf.global_ansimask & MASK_BBLUE )
               strcat(buf2, ANSI_BBLUE);
            break;
         case 'M':		/* magenta bg */
            if ( mudconf.global_ansimask & MASK_BMAGENTA )
               strcat(buf2, ANSI_BMAGENTA);
            break;
         case 'C':		/* cyan bg */
            if ( mudconf.global_ansimask & MASK_BCYAN )
               strcat(buf2, ANSI_BCYAN);
            break;
         case 'W':		/* white bg */
            if ( mudconf.global_ansimask & MASK_BWHITE )
               strcat(buf2, ANSI_BWHITE);
            break;
      } /* Switch */
      s++;
   } /* While */
   return buf2;
}
#endif

char *
unparse_object_ansi_altname(dbref player, dbref target, int obey_myopic)
{
    char *buf, *buf2, *buf3, *fp, *ansibuf, ansitmp[30], name_str[31];
    int exam, aowner, ansi_ok;
    dbref aflags;
#ifndef ZENTY_ANSI
    char *s;
#endif

    ansi_ok = 0;
    buf = alloc_lbuf("unparse_object_ansi_a");
    memset(ansitmp, 0, sizeof(ansitmp));

    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (target == AMBIGUOUS) {
	strcpy(buf, "*AMBIGUOUS*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
#ifdef ZENTY_ANSI
        ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
        if ( !(ExtAnsi(target) && ((ansi_ok = strcmp(Name(target), strip_all_special(ansibuf))) == 0)) )
            buf2 = parse_ansi_name(target, ansibuf);
        else
            buf2=alloc_lbuf("unparse_object_ansi_b");
#else
        buf2=alloc_lbuf("unparse_object_ansi_b");
           ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
       if ( !(ExtAnsi(target) && ((ansi_ok = strcmp(Name(target), strip_ansi(ansibuf))) == 0)) ) {
          strncpy(ansitmp, ansibuf, 20);
          s = ansitmp;
          while (*s) {
             switch (*s) {
             case 'h':		/* hilite */
                 if ( mudconf.global_ansimask & MASK_HILITE )
	            strcat(buf2, ANSI_HILITE);
	         break;
             case 'i':		/* inverse */
                 if ( mudconf.global_ansimask & MASK_INVERSE )
	            strcat(buf2, ANSI_INVERSE);
	         break;
             case 'f':		/* flash */
                 if ( mudconf.global_ansimask & MASK_BLINK )
	            strcat(buf2, ANSI_BLINK);
	         break;
             case 'n':		/* normal */
	         strcat(buf2, ANSI_NORMAL);
	         break;
             case 'x':		/* black fg */
                 if ( mudconf.global_ansimask & MASK_BLACK )
	            strcat(buf2, ANSI_BLACK);
	         break;
             case 'r':		/* red fg */
                 if ( mudconf.global_ansimask & MASK_RED )
	            strcat(buf2, ANSI_RED);
	         break;
             case 'g':		/* green fg */
                 if ( mudconf.global_ansimask & MASK_GREEN )
	            strcat(buf2, ANSI_GREEN);
	         break;
             case 'y':		/* yellow fg */
                 if ( mudconf.global_ansimask & MASK_YELLOW )
	            strcat(buf2, ANSI_YELLOW);
	         break;
             case 'b':		/* blue fg */
                 if ( mudconf.global_ansimask & MASK_BLUE )
	            strcat(buf2, ANSI_BLUE);
	         break;
             case 'm':		/* magenta fg */
                 if ( mudconf.global_ansimask & MASK_MAGENTA )
	            strcat(buf2, ANSI_MAGENTA);
	         break;
             case 'c':		/* cyan fg */
                 if ( mudconf.global_ansimask & MASK_CYAN )
	            strcat(buf2, ANSI_CYAN);
	         break;
             case 'w':		/* white fg */
                 if ( mudconf.global_ansimask & MASK_WHITE )
	            strcat(buf2, ANSI_WHITE);
	         break;
             case 'X':		/* black bg */
                 if ( mudconf.global_ansimask & MASK_BBLACK )
	            strcat(buf2, ANSI_BBLACK);
	         break;
             case 'R':		/* red bg */
                 if ( mudconf.global_ansimask & MASK_BRED )
	            strcat(buf2, ANSI_BRED);
	         break;
             case 'G':		/* green bg */
                 if ( mudconf.global_ansimask & MASK_BGREEN )
	            strcat(buf2, ANSI_BGREEN);
	         break;
             case 'Y':		/* yellow bg */
                 if ( mudconf.global_ansimask & MASK_BYELLOW )
	            strcat(buf2, ANSI_BYELLOW);
	         break;
             case 'B':		/* blue bg */
                 if ( mudconf.global_ansimask & MASK_BBLUE )
	            strcat(buf2, ANSI_BBLUE);
	         break;
             case 'M':		/* magenta bg */
                 if ( mudconf.global_ansimask & MASK_BMAGENTA )
	            strcat(buf2, ANSI_BMAGENTA);
	         break;
             case 'C':		/* cyan bg */
                 if ( mudconf.global_ansimask & MASK_BCYAN )
	            strcat(buf2, ANSI_BCYAN);
	         break;
             case 'W':		/* white bg */
                 if ( mudconf.global_ansimask & MASK_BWHITE )
	            strcat(buf2, ANSI_BWHITE);
	         break;
             }
             s++;
           }
        }
#endif       
        buf3 = atr_get(target, A_ALTNAME, &aowner, &aflags);
        memset(name_str, 0, sizeof(name_str));
        strncpy(name_str, buf3, 30);
        free_lbuf(buf3);
	if (obey_myopic)
	    exam = MyopicExam(player, target);
	else
	    exam = Examinable(player, target);
	if (exam || (!NoEx(target) &&
	    ((Flags2(target) & ABODE) ||
	     (Flags(target) & CHOWN_OK && (!Guildmaster(target))) ||
	     (Flags(target) & (JUMP_OK | LINK_OK | DESTROY_OK)))) ) {

	    /* show everything */
	    fp = unparse_flags(player, target);
            if ( !Wizard(player) && name_str[0] != '\0' &&
                 !could_doit(player,target,A_LALTNAME,0) )
	       sprintf(buf, "%s%s%s(#%d%s)", buf2, name_str,  ANSI_NORMAL, target, fp);
            else if ( name_str[0] == '\0' ) {
               if ( ExtAnsi(target) ) {
                  if ( ansi_ok == 0 )
	             sprintf(buf, "%.3500s%s(#%d%.400s)", ansibuf, ANSI_NORMAL, target, fp);
                  else
	             sprintf(buf, "%.3500s(#%d%.400s)", Name(target), target, fp);
               } else
	          sprintf(buf, "%.100s%.3400s%s(#%d%.400s)", buf2, Name(target), ANSI_NORMAL, target, fp);
            } else {
               if ( ExtAnsi(target) ) {
                  if ( ansi_ok == 0 )
	             sprintf(buf, "%.3500s%s(#%d%.400s) {%.60s}", ansibuf, ANSI_NORMAL, target, fp, name_str);
                  else
	             sprintf(buf, "%.3500s(#%d%.400s) {%.60s}", Name(target), target, fp, name_str);
               } else
	          sprintf(buf, "%.100s%.3400s%s(#%d%.400s) {%.60s}", buf2, Name(target), 
                          ANSI_NORMAL, target, fp, name_str);
            }
	    free_mbuf(fp);
	} else {
	    /* show only the name. */
            if ( !Wizard(player) && name_str[0] != '\0' &&
                 !could_doit(player,target,A_LALTNAME,0) )
               sprintf(buf, "%.100s%.3800s%s", buf2, name_str,  ANSI_NORMAL);
            else if ( name_str[0] == '\0' ) {
               if ( ExtAnsi(target) ) {
                  if ( ansi_ok == 0 )
                     sprintf(buf, "%.3900s%s", ansibuf, ANSI_NORMAL);
                  else
                     sprintf(buf, "%.3900s", Name(target));
               } else
                  sprintf(buf, "%.100s%.3800s%s", buf2, Name(target), ANSI_NORMAL);
            } else {
               if ( ExtAnsi(target) ) {
                  if ( ansi_ok == 0 )
                     sprintf(buf, "%.3800s%s {%.60s}", ansibuf, ANSI_NORMAL, name_str);
                  else
                     sprintf(buf, "%.3800s {%.60s}", Name(target), name_str);
               } else
                  sprintf(buf, "%.100s%.3800s%s {%.60s}", buf2, Name(target), ANSI_NORMAL, name_str);
            }
	}
    free_lbuf(ansibuf);
    }
    if ( Good_obj(target) && NoName(target) && !Wizard(player) )
       memset(buf, 0, LBUF_SIZE);

    free_lbuf(buf2);    
    return buf;
}

char *
unparse_object_ansi(dbref player, dbref target, int obey_myopic)
{
    char *buf, *buf2, *fp, *ansibuf, ansitmp[30];
    int exam, aowner, ansi_ok;
    dbref aflags;
#ifndef ZENTY_ANSI
    char *s;
#endif

    ansi_ok = 0;
    buf = alloc_lbuf("unparse_object_ansi_c");
    memset(ansitmp, 0, sizeof(ansitmp));

    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (target == AMBIGUOUS) {
	strcpy(buf, "*AMBIGUOUS*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
#ifdef ZENTY_ANSI
        ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
        if ( !(ExtAnsi(target) && ((ansi_ok = strcmp(Name(target), strip_all_special(ansibuf))) == 0)) ) {
           if ( !Accents(player) )
              buf2 = parse_ansi_name(target, strip_safe_accents(ansibuf));
           else
              buf2 = parse_ansi_name(target, ansibuf);
        } else {
            buf2=alloc_lbuf("unparse_object_ansi_d");
        }
#else
        buf2 = alloc_lbuf("unparse_object_ansi_d");        
       ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
       if ( !(ExtAnsi(target) && ((ansi_ok = strcmp(Name(target), strip_ansi(ansibuf))) == 0)) ) {
          strncpy(ansitmp, ansibuf, 20);
          s = ansitmp;
          while (*s) {
             switch (*s) {
                case 'h':		/* hilite */
                    if ( mudconf.global_ansimask & MASK_HILITE )
	               strcat(buf2, ANSI_HILITE);
	            break;
                case 'i':		/* inverse */
                    if ( mudconf.global_ansimask & MASK_INVERSE )
	               strcat(buf2, ANSI_INVERSE);
	            break;
                case 'f':		/* flash */
                    if ( mudconf.global_ansimask & MASK_BLINK )
	               strcat(buf2, ANSI_BLINK);
	            break;
                case 'n':		/* normal */
	            strcat(buf2, ANSI_NORMAL);
	            break;
                case 'x':		/* black fg */
                    if ( mudconf.global_ansimask & MASK_BLACK )
	               strcat(buf2, ANSI_BLACK);
	            break;
                case 'r':		/* red fg */
                    if ( mudconf.global_ansimask & MASK_RED )
	               strcat(buf2, ANSI_RED);
	            break;
                case 'g':		/* green fg */
                    if ( mudconf.global_ansimask & MASK_GREEN )
	               strcat(buf2, ANSI_GREEN);
	            break;
                case 'y':		/* yellow fg */
                    if ( mudconf.global_ansimask & MASK_YELLOW )
	               strcat(buf2, ANSI_YELLOW);
	            break;
                case 'b':		/* blue fg */
                    if ( mudconf.global_ansimask & MASK_BLUE )
	               strcat(buf2, ANSI_BLUE);
	            break;
                case 'm':		/* magenta fg */
                    if ( mudconf.global_ansimask & MASK_MAGENTA )
	               strcat(buf2, ANSI_MAGENTA);
	            break;
                case 'c':		/* cyan fg */
                    if ( mudconf.global_ansimask & MASK_CYAN )
	               strcat(buf2, ANSI_CYAN);
	            break;
                case 'w':		/* white fg */
                    if ( mudconf.global_ansimask & MASK_WHITE )
	               strcat(buf2, ANSI_WHITE);
	            break;
                case 'X':		/* black bg */
                    if ( mudconf.global_ansimask & MASK_BBLACK )
	               strcat(buf2, ANSI_BBLACK);
	            break;
                case 'R':		/* red bg */
                    if ( mudconf.global_ansimask & MASK_BRED )
	               strcat(buf2, ANSI_BRED);
	            break;
                case 'G':		/* green bg */
                    if ( mudconf.global_ansimask & MASK_BGREEN )
	               strcat(buf2, ANSI_BGREEN);
	            break;
                case 'Y':		/* yellow bg */
                    if ( mudconf.global_ansimask & MASK_BYELLOW )
	               strcat(buf2, ANSI_BYELLOW);
	            break;
                case 'B':		/* blue bg */
                    if ( mudconf.global_ansimask & MASK_BBLUE )
	               strcat(buf2, ANSI_BBLUE);
	            break;
                case 'M':		/* magenta bg */
                    if ( mudconf.global_ansimask & MASK_BMAGENTA )
	               strcat(buf2, ANSI_BMAGENTA);
	            break;
                case 'C':		/* cyan bg */
                    if ( mudconf.global_ansimask & MASK_BCYAN )
	               strcat(buf2, ANSI_BCYAN);
	            break;
                case 'W':		/* white bg */
                    if ( mudconf.global_ansimask & MASK_BWHITE )
	               strcat(buf2, ANSI_BWHITE);
	            break;
             }
             s++;
           }
        }
#endif       
  	if (obey_myopic)
	   exam = MyopicExam(player, target);
	else
	   exam = Examinable(player, target);
	if (exam || (!NoEx(target) &&
	    ((Flags2(target) & ABODE) ||
	     (Flags(target) & CHOWN_OK && (!Guildmaster(target))) ||
	     (Flags(target) & (JUMP_OK | LINK_OK | DESTROY_OK)))) ) {
   
	       /* show everything */
	    fp = unparse_flags(player, target);
            if ( ExtAnsi(target) ) {
               if ( ansi_ok == 0 )
	          sprintf(buf, "%.3500s%s(#%d%.400s)", ansibuf, ANSI_NORMAL, target, fp);
               else
	          sprintf(buf, "%.3500s(#%d%.400s)", Name(target), target, fp);
            } else
	       sprintf(buf, "%.100s%.3400s%s(#%d%.400s)", buf2, Name(target), ANSI_NORMAL, target, fp);
	    free_mbuf(fp);
	} else {
	       /* show only the name. */
            if ( ExtAnsi(target) ) {
               if ( ansi_ok == 0 )
                  sprintf(buf, "%.3900s%s", ansibuf, ANSI_NORMAL);
               else
                  sprintf(buf, "%.3900s", Name(target));
            } else
               sprintf(buf, "%.100s%.3800s%s", buf2, Name(target), ANSI_NORMAL);
	}
    free_lbuf(ansibuf);
    }
    if ( Good_obj(target) && isRoom(target) && NoName(target) && !Wizard(player) )
       memset(buf, 0, LBUF_SIZE);

    free_lbuf(buf2);    
    return buf;
}

char *
ansi_exitname(dbref target)
{
   char *buf2, *ansibuf;
   int aowner;
   dbref aflags;

#ifdef ZENTY_ANSI
   ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
   buf2 = parse_ansi_name(target, ansibuf);
   free_lbuf(ansibuf);
#else
   char *s, ansitmp[30];

       buf2 = alloc_lbuf("ansi_exitname");
       memset(ansitmp, 0, sizeof(ansitmp));
       ansibuf = atr_get(target, A_ANSINAME, &aowner, &aflags);
       strncpy(ansitmp, ansibuf, 20);
       free_lbuf(ansibuf);
       s = ansitmp;
       while (*s) {
          switch (*s) {
             case 'h':		/* hilite */
                 if ( mudconf.global_ansimask & MASK_HILITE )
	            strcat(buf2, ANSI_HILITE);
	         break;
             case 'i':		/* inverse */
                 if ( mudconf.global_ansimask & MASK_INVERSE )
	            strcat(buf2, ANSI_INVERSE);
	         break;
             case 'f':		/* flash */
                 if ( mudconf.global_ansimask & MASK_BLINK )
	            strcat(buf2, ANSI_BLINK);
	         break;
             case 'n':		/* normal */
	         strcat(buf2, ANSI_NORMAL);
	         break;
             case 'x':		/* black fg */
                 if ( mudconf.global_ansimask & MASK_BLACK )
	            strcat(buf2, ANSI_BLACK);
	         break;
             case 'r':		/* red fg */
                 if ( mudconf.global_ansimask & MASK_RED )
	            strcat(buf2, ANSI_RED);
	         break;
             case 'g':		/* green fg */
                 if ( mudconf.global_ansimask & MASK_GREEN )
	            strcat(buf2, ANSI_GREEN);
	         break;
             case 'y':		/* yellow fg */
                 if ( mudconf.global_ansimask & MASK_YELLOW )
	            strcat(buf2, ANSI_YELLOW);
	         break;
             case 'b':		/* blue fg */
                 if ( mudconf.global_ansimask & MASK_BLUE )
	            strcat(buf2, ANSI_BLUE);
	         break;
             case 'm':		/* magenta fg */
                 if ( mudconf.global_ansimask & MASK_MAGENTA )
	            strcat(buf2, ANSI_MAGENTA);
	         break;
             case 'c':		/* cyan fg */
                 if ( mudconf.global_ansimask & MASK_CYAN )
	            strcat(buf2, ANSI_CYAN);
	         break;
             case 'w':		/* white fg */
                 if ( mudconf.global_ansimask & MASK_WHITE )
	            strcat(buf2, ANSI_WHITE);
	         break;
             case 'X':		/* black bg */
                 if ( mudconf.global_ansimask & MASK_BBLACK )
	            strcat(buf2, ANSI_BBLACK);
	         break;
             case 'R':		/* red bg */
                 if ( mudconf.global_ansimask & MASK_BRED )
	            strcat(buf2, ANSI_BRED);
	         break;
             case 'G':		/* green bg */
                 if ( mudconf.global_ansimask & MASK_BGREEN )
	            strcat(buf2, ANSI_BGREEN);
	         break;
             case 'Y':		/* yellow bg */
                 if ( mudconf.global_ansimask & MASK_BYELLOW )
	            strcat(buf2, ANSI_BYELLOW);
	         break;
             case 'B':		/* blue bg */
                 if ( mudconf.global_ansimask & MASK_BBLUE )
	            strcat(buf2, ANSI_BBLUE);
	         break;
             case 'M':		/* magenta bg */
                 if ( mudconf.global_ansimask & MASK_BMAGENTA )
	            strcat(buf2, ANSI_BMAGENTA);
	         break;
             case 'C':		/* cyan bg */
                 if ( mudconf.global_ansimask & MASK_BCYAN )
	            strcat(buf2, ANSI_BCYAN);
	         break;
             case 'W':		/* white bg */
                 if ( mudconf.global_ansimask & MASK_BWHITE )
	            strcat(buf2, ANSI_BWHITE);
	         break;
          }
          s++;
       }
#endif       
    return buf2;
}

char *
unparse_object2(dbref player, dbref target, int obey_myopic)
{
    char *buf, *fp;
    int exam;

    buf = alloc_lbuf("unparse_object");
    if (target == NOTHING) {
	strcpy(buf, "*NOTHING*");
    } else if (target == HOME) {
	strcpy(buf, "*HOME*");
    } else if (target == AMBIGUOUS) {
	strcpy(buf, "*AMBIGUOUS*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
	if (obey_myopic)
	    exam = MyopicExam(player, target);
	else
	    exam = Examinable(player, target);
	if (exam || (!NoEx(target) &&
	    ((Flags2(target) & ABODE) ||
	     (HasPriv(player, target, POWER_SEE_QUEUE, POWER3, NOTHING)) ||
	     (Flags(target) & CHOWN_OK && (!Guildmaster(target))) ||
	     (Flags(target) & (JUMP_OK | LINK_OK | DESTROY_OK)))) ) {

	    /* show everything */
	    fp = unparse_flags(player, target);
	    sprintf(buf, "%.3500s(#%d%.400s)", Name(target), target, fp);
	    free_mbuf(fp);
	} else {
	    /* show only the name. */
            sprintf(buf, "%.3900s", Name(target));
	}
    }
    return buf;
}

/* ---------------------------------------------------------------------------
 * convert_flags: convert a list of flag letters into its bit pattern.
 * Also set the type qualifier if specified and not already set.
 */

int 
convert_flags(dbref player, char *flaglist, FLAGSET * fset, FLAG * p_type, int word)
{
    int i, handled;
    char *s, *tpr_buff, *tprp_buff;
    FLAG flag1mask, flag2mask, flag3mask, flag4mask, type;
    FLAGENT *fp;

    flag1mask = flag2mask = flag3mask = flag4mask = 0;
    type = NOTYPE;

    for (s = flaglist; *s; s++) {
	handled = 0;

	/* Check for object type */

	if (!word) {
	    for (i = 0; (i <= 7) && !handled; i++) {
		if ((object_types[i].lett == *s) &&
		    !(((object_types[i].perm & CA_WIZARD) &&
		       !Wizard(player)) ||
		      ((object_types[i].perm & CA_BUILDER) &&
		       !Builder(player)) ||
		      ((object_types[i].perm & CA_ADMIN) &&
		       !Admin(player)) ||
		      ((object_types[i].perm & CA_IMMORTAL) &&
		       !Immortal(player)) ||
		      ((object_types[i].perm & CA_GOD) &&
		       !God(player)))) {
		    if ((type != NOTYPE) && (type != i)) {
                        tprp_buff = tpr_buff = alloc_lbuf("convert_flags");
			notify(player,
			       safe_tprintf(tpr_buff, &tprp_buff, 
                                            "%c: Conflicting type specifications.", *s));
                        free_lbuf(tpr_buff);
			return 0;
		    }
		    type = i;
		    handled = 1;
		}
	    }
	}
	/* Check generic flags */

	if (handled)
	    continue;
	for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	     fp;
	     fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {
	    if (!word && (fp->flagflag & (FLAG3 | FLAG4)))
		continue;
	    if (word && (!(fp->flagflag) || (fp->flagflag & FLAG2)))
		continue;
	    if ((fp->flaglett == *s) &&
		!(((fp->listperm & CA_WIZARD) &&
		   !Wizard(player)) ||
		  ((fp->listperm & CA_BUILDER) &&
		   !Builder(player)) ||
		  ((fp->listperm & CA_ADMIN) &&
		   !Admin(player)) ||
		  ((fp->listperm & CA_IMMORTAL) &&
		   !Immortal(player)) ||
		  ((fp->listperm & CA_GOD) &&
		   !God(player)))) {
		if (fp->flagflag & FLAG2)
		    flag2mask |= fp->flagvalue;
		else if (fp->flagflag & FLAG3)
		    flag3mask |= fp->flagvalue;
		else if (fp->flagflag & FLAG4)
		    flag4mask |= fp->flagvalue;
		else
		    flag1mask |= fp->flagvalue;
		handled = 1;
	    }
	}

	if (!handled) {
            tprp_buff = tpr_buff = alloc_lbuf("convert_flags");
	    notify(player,
		   safe_tprintf(tpr_buff, &tprp_buff, 
                                "%c: Flag unknown or not valid for specified object type", *s));
            free_lbuf(tpr_buff);
	    return 0;
	}
    }

    /* return flags to search for and type */

    (*fset).word1 = flag1mask;
    (*fset).word2 = flag2mask;
    (*fset).word3 = flag3mask;
    (*fset).word4 = flag4mask;
    *p_type = type;
    return 1;
}

/* ---------------------------------------------------------------------------
 * decompile_flags: Produce commands to set flags on target.
 */

void 
decompile_flags(dbref player, dbref thing, char *thingname)
{
    char *tpr_buff, *tprp_buff;
    FLAG f1, f2, f3, f4;
    FLAGENT *fp;

    /* Report generic flags */

    f1 = Flags(thing);
    f2 = Flags2(thing);
    f3 = Flags3(thing);
    f4 = Flags4(thing);

    for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	 fp;
	 fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)){

	/* Skip if we shouldn't decompile this flag */

	if (fp->listperm & CA_NO_DECOMP)
	    continue;

	/* Skip if this flag is not set */

	if (fp->flagflag & FLAG2) {
	    if (!(f2 & fp->flagvalue))
		continue;
	} else if (fp->flagflag & FLAG3) {
	    if (!(f3 & fp->flagvalue))
		continue;
	} else if (fp->flagflag & FLAG4) {
	    if (!(f4 & fp->flagvalue))
		continue;
	} else {
	    if (!(f1 & fp->flagvalue))
		continue;
	}

	/* Skip if we can't see this flag */

        /* If hide_nospoof enabled only show NOSPOOF flag to controller */
        if ( (mudconf.hide_nospoof) && (!(fp->flagflag) && (fp->flagvalue == NOSPOOF)) &&
             !Controls(player, thing) ) {
           continue;
        }
        if ( (((fp->flagflag & FLAG2) && ((fp->flagvalue == GUILDMASTER) ||
              (fp->flagvalue == ADMIN) || (fp->flagvalue == BUILDER))) ||
              (!(fp->flagflag) && ((fp->flagvalue == IMMORTAL) ||
              (fp->flagvalue == WIZARD)))) &&
              (HasPriv(thing, player, POWER_HIDEBIT, POWER5, NOTHING) &&
               (Owner(thing) != Owner(player) || mudstate.objevalst)) ) {
               continue;
        }

	if (!check_access(player, fp->listperm, 0, 0))
	    continue;

	/* We made it this far, report this flag */

        tprp_buff = tpr_buff = alloc_lbuf("decompile_flags");
	noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@set %s=%s", thingname, fp->flagname));
        free_lbuf(tpr_buff);
    }
}
#endif /* STANDALONE */

int 
HasPriv(dbref thing, dbref target, int powerpos, int powerword, int check)
{
    int pwrs, pwrs2, pwrs3, ocheck;
    dbref thing2, target2;

    if ((check & POWER_CHECK_OWNER) && (check != NOTHING)) {
	check &= ~POWER_CHECK_OWNER;
	ocheck = 1;
	if (!check)
	  check = NOTHING;
    }
    else
	ocheck = 0;

    pwrs3 = 0;
    if ( !mudconf.power_objects ) {
       if (Inherits(thing))
	   thing2 = Owner(thing);
       else
	   thing2 = thing;
       if (powerword == POWER3)
	   pwrs = Toggles3(thing2);
       else if (powerword == POWER4)
	   pwrs = Toggles4(thing2);
       else
	   pwrs = Toggles5(thing2);
       pwrs >>= powerpos;
       pwrs &= POWER_LEVEL_COUNC;
       if (!pwrs)
	   return 0;
    } else {
       thing2 = Owner(thing);
       if (powerword == POWER3)
           pwrs = Toggles3(thing);
       else if (powerword == POWER4)
           pwrs = Toggles4(thing);
       else
           pwrs = Toggles5(thing);
       pwrs >>= powerpos;
       pwrs &= POWER_LEVEL_COUNC;
       if ( (thing != thing2) && Inherits(thing) ) {
          if (powerword == POWER3)
              pwrs3 = Toggles3(thing2);
          else if (powerword == POWER4)
              pwrs3 = Toggles4(thing2);
          else
              pwrs3 = Toggles5(thing2);
          pwrs3 >>= powerpos;
          pwrs3 &= POWER_LEVEL_COUNC;
          if ( pwrs3 > pwrs )
             pwrs = pwrs3;
       }
       if ( !pwrs && !pwrs3 )
          return 0;
    }
    if (target == NOTHING)
	target2 = NOTHING;
    else if (ocheck)
	target2 = Owner(target);
    else
	target2 = target;
    if (target2 == NOTHING)
	pwrs2 = 1;
    else if (God(target2) || Wizard(target2))
	pwrs2 = 4;
    else if (Admin(target2))
	pwrs2 = 3;
    else if (Builder(target2))
	pwrs2 = 2;
    else if (Guildmaster(target2))
	pwrs2 = 1;
    else
	pwrs2 = 0;
    if (check == POWER_LEVEL_NA)
	return pwrs;
    else if (check == NOTHING) 
  	return (pwrs >= pwrs2);
    else
  	return (pwrs >= check); 
}

int 
DePriv(dbref thing, dbref target, int powerpos, int powerword, int check)
{
    int pwrs, pwrs2, pwrs3;
    dbref thing2;

    thing2 = Owner(thing);
    pwrs3 = 0;
    if ( !mudconf.power_objects ) {
       if (powerword == POWER6)
	   pwrs = Toggles6(thing2);
       else if (powerword == POWER7)
	   pwrs = Toggles7(thing2);
       else
	   pwrs = Toggles8(thing2);
       pwrs >>= powerpos;
       pwrs &= POWER_LEVEL_COUNC;
       if (!pwrs)
	   return 0;
    } else {
       if (powerword == POWER6)
           pwrs = Toggles6(thing);
       else if (powerword == POWER7)
           pwrs = Toggles7(thing);
       else
           pwrs = Toggles8(thing);
       pwrs >>= powerpos;
       pwrs &= POWER_LEVEL_COUNC;
       if ( (thing != thing2 ) ) {
          if (powerword == POWER6)
              pwrs3 = Toggles6(thing2);
          else if (powerword == POWER7)
              pwrs3 = Toggles7(thing2);
          else
              pwrs3 = Toggles8(thing2);
          pwrs3 >>= powerpos;
          pwrs3 &= POWER_LEVEL_COUNC;
          if ( pwrs3 && (pwrs > pwrs3) )
             pwrs = pwrs3;
       }
       if (!pwrs && !pwrs3 )
           return 0;
    }
    if (target == NOTHING)
	pwrs2 = 1;
    else if (Immortal(target))
	pwrs2 = 5;
    else if (Wizard(target))
	pwrs2 = 4;
    else if (Admin(target))
	pwrs2 = 3;
    else if (Builder(target))
	pwrs2 = 2;
    else if (Guildmaster(target))
	pwrs2 = 1;
    else
	pwrs2 = 0;
    if (check == POWER_LEVEL_NA)
	return pwrs;
    if (!DPShift(thing2))
	pwrs--;
    if (check == NOTHING)
	return (pwrs <= pwrs2);
    else if (check == POWER_LEVEL_SPC)
	return (pwrs >= pwrs2);
    else
	return (pwrs <= check);
}

int good_flag(char *test)
{
  char *pt1;

  if ((strlen(test) > 15) || (strlen(test) < 3))
    return 0;
  pt1 = test;
  if (!isalnum((int)*pt1))
    return 0;
  while (*pt1 && (isalnum((int)*pt1) || (*pt1 == '_') || (*pt1 == '-')))
    pt1++;
  if (*pt1)
    return 0;
  return 1;
}

int flagstuff_internal(char *alias, char *newname)
{
#ifndef STANDALONE
  FLAGENT *flag1, *flag2;
  char buff1[16], buff2[16], buff3[16], *pt1, *pt2, *pt3;

  if (!good_flag(newname))
    return -1;
  if (!good_flag(alias))
    return -1;
  pt1 = alias;
  pt2 = buff1;
  while (*pt1)
    *pt2++ = tolower(*pt1++);
  *pt2 = '\0';
  strcpy(buff3, buff1);
  flag1 = (FLAGENT *)hashfind(buff1, &mudstate.flags_htab);
  if (flag1 != NULL) {
    flag2 = (FLAGENT *)hashfind(newname, &mudstate.flags_htab);
    if (flag2 == NULL) {
      pt1 = newname;
      pt2 = buff1;
      pt3 = buff2;
      while (*pt1) {
	*pt2++ = tolower(*pt1);
	*pt3++ = toupper(*pt1++);
      }
      *pt2 = '\0';
      *pt3 = '\0';
      hashrepl2(buff3, (int *) flag1, &mudstate.flags_htab, 0);
      strcpy(flag1->flagname,buff2);
      hashadd2(buff1, (int *) flag1, &mudstate.flags_htab, 1);
      return 0;
    }
  }
#endif
  return -1;
}
void do_flagdef(dbref player, dbref cause, int key, char *flag1, char *flag2)
{
#ifndef STANDALONE
   FLAGENT *fp;
   char listpermary[33], setovpermary[33], usetovpermary[33], *lp_ptr, *sop_ptr, *usop_ptr;
   char static_list[33], *tmp_ptr, c_bef, c_aft, *tpr_buff, *tprp_buff;
   int  cntr, nodecomp, stripmask, srch_return, mask_add, mask_del, wild_mtch, fnd;

   /* Copy 32 characters */
   /* Note: 'M' (0x40000000) is not a valid flag - tagged for MORTAL use */
   /* G - Ghod, W - Wizard, A - Architect, I - Immortal, C - Councilor, S - Guildmaster 
    * g -!Ghod, m -!mortal, s -!guildmaster, a - !architect, c - !councilor, w - !wizard
    * i -!immortal, + -!suspect, ! -!guest, ^ -!wanderer, M - mortal, L - LOGFLAG */
   strcpy(static_list, "GWAI CS gmsacwiL   +!^        M ");
   static_list[32]='\0';
   stripmask=0xBFCB0090;
   c_bef = ' ';
   c_aft = ' ';

   if ( strlen(flag1) > 0 )
      wild_mtch = 1;
   else
      wild_mtch = 0;

   if ( (key & FLAGDEF_LIST) || key == 0 ) {
      notify_quiet(player, "|Flagname        |Flg|Set             |Unset" \
                           "           |See             |NoM|");
      notify_quiet(player, "+----------------+---+----------------+" \
                           "----------------+----------------+---+");
      fnd = 0;
      tprp_buff = tpr_buff = alloc_lbuf("do_flagdef");
      for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	   fp;
	   fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)){
         if ( wild_mtch ) {
            if (!quick_wild(flag1, (char *)fp->flagname))
               continue;
         }
         fnd = 1;
         memset(listpermary, 0, sizeof(listpermary));
         memset(setovpermary, 0, sizeof(listpermary));
         memset(usetovpermary, 0, sizeof(listpermary));
         lp_ptr = listpermary;
         sop_ptr = setovpermary;
         usop_ptr = usetovpermary;
         cntr = 0;
         nodecomp = (fp->listperm & CA_NO_DECOMP);
         nodecomp = (nodecomp | ((fp->flagvalue & IMMORTAL) &&
                                 (fp->flagflag == 0)));
         while ( cntr < 32 ) {
            if ( (fp->listperm &~ stripmask) & (1 << cntr) ) {
               *lp_ptr = static_list[cntr];
                lp_ptr++;
            }
            if ( (fp->setovperm &~ stripmask) & (1 << cntr) ) {
               *sop_ptr = static_list[cntr];
                sop_ptr++;
            }
            if ( (fp->usetovperm &~ stripmask) & (1 << cntr) ) {
               *usop_ptr = static_list[cntr];
                usop_ptr++;
            }
            cntr++;
         }
         if (fp->flagflag & (FLAG3 | FLAG4)) {
            c_bef = '[';
            c_aft = ']';
         } else {
            c_bef = ' ';
            c_aft = ' ';
         }
         tprp_buff = tpr_buff;
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "|%-16s|%c%c%c|%-16s|%-16s|%-16s| %c |", 
                                           fp->flagname, c_bef, fp->flaglett, c_aft, setovpermary, 
                                           usetovpermary, listpermary, (nodecomp ? 'Y' : ' ')));
      }
      free_lbuf(tpr_buff);
      if ( !fnd )
         notify_quiet(player, "                        *** NO MATCHING FLAGS FOUND ***");

      notify_quiet(player, "+----------------+---+----------------+" \
                           "----------------+----------------+---+");
   } else { 
      for (fp = (FLAGENT *) hash_firstentry2(&mudstate.flags_htab, 1); 
	   fp;
	   fp = (FLAGENT *) hash_nextentry(&mudstate.flags_htab)) {
         if (minmatch(flag1, fp->flagname, strlen(fp->flagname))) 
            break;
      }
      if ( !fp || !(fp->flagname)) {
         notify_quiet(player, "Bad flag given to @flagdef");
         return;
      }
      if ( (fp->listperm & CA_NO_DECOMP) | 
                  ((fp->flagvalue & IMMORTAL) &&
                   (fp->flagflag == 0)) ) {
         notify_quiet(player, "Sorry, you can not modify that flag.");
         return;
      }
      if ( key & FLAGDEF_CHAR ) {
         if ( (strlen(flag2) != 1) || !*flag2 || isspace(*flag2) || !isprint(*flag2)) {
            notify_quiet(player, "Flag letter must be a single character.");
         } else {
            tprp_buff = tpr_buff = alloc_lbuf("do_flagdef");
            notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Modified 'flagletter' for flag %s.  Old letter '%c', new letter '%c'",
                                           fp->flagname, fp->flaglett, *flag2));
            fp->flaglett = *flag2;
            free_lbuf(tpr_buff);
         }
         return;
      }
      tmp_ptr = strtok(flag2, " \t");
      nodecomp = 0;
      mask_add = mask_del = 0;
      while (tmp_ptr != NULL) {
         if ( *tmp_ptr == '!' ) {
            nodecomp = 1;
            tmp_ptr++;
         } else
            nodecomp = 0;
         
         srch_return = search_nametab(GOD, access_nametab, tmp_ptr);
         if ( srch_return != -1 ) {
            if (nodecomp)
               mask_del |= srch_return;
            else
               mask_add |= srch_return;
         } else {
            if (minmatch(tmp_ptr, "mortal", strlen(tmp_ptr))) {
               if ( nodecomp )
                  mask_del |= 0x40000000;
               else
                  mask_add |= 0x40000000;
            } 
         }
         tmp_ptr = strtok(NULL, " \t");
      }
      if ( !(mask_add & ~stripmask) && !(mask_del & ~stripmask) ) {
         notify_quiet(player, "Nothing for @flagdef to do.");
         return;
      }
      tprp_buff = tpr_buff = alloc_lbuf("do_flagdef");
      if ( key & FLAGDEF_SET ) {
         fp->setovperm &= ~mask_del;
         fp->setovperm |= mask_add;
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Modified 'set' permissions for flag %s",
                                           fp->flagname));
      } else if ( key & FLAGDEF_UNSET ) {
         fp->usetovperm &= ~mask_del;
         fp->usetovperm |= mask_add;
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Modified 'unset' permissions for flag %s",
                                           fp->flagname));
      } else if ( key & FLAGDEF_SEE ) {
         fp->listperm &= ~mask_del;
         fp->listperm |= mask_add;
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Modified 'see' permissions for flag %s",
                                           fp->flagname));
      } else {
         notify_quiet(player, "Invalid switch/argument to @flagdef");
      }
      free_lbuf(tpr_buff);
   }
#endif
}

void do_flagstuff(dbref player, dbref cause, int key, char *flag1, char *flag2)
{
#ifndef STANDALONE
  FLAGENT *lookup;
  char *tpr_buff, *tprp_buff;
 
  if (key & FLAGSW_REMOVE) {
    if (*flag2)
      notify(player,"Extra argument ignored.");
    lookup = (FLAGENT *)hashfind(flag1, &mudstate.flags_htab);
    if (lookup != NULL) {
      if (!stricmp(lookup->flagname,flag1))
	notify(player, "Error: You can't remove the present flag name from the hash table.");
      else {
	hashdelete(flag1, &mudstate.flags_htab);
        tprp_buff = tpr_buff = alloc_lbuf("flag_stuff");
	notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Flag name '%s' removed from the hash table.",flag1));
        free_lbuf(tpr_buff);
      }
    } else {
        notify(player, "Error: Flag alias does not exist.");
    }
  }
  else
    if (flagstuff_internal(flag1, flag2) == -1)
      notify(player,"Error: Bad flagname given or flag not found.");
    else
      notify(player,"Flag name changed.");
#endif
}
