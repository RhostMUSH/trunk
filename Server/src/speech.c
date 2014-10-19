/* speech.c -- Commands which involve speaking */

#ifdef SOLARIS
/* Solaris has borked declarations */
char *strtok_r(char *, const char *, char **);
#endif

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "alloc.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */

extern void FDECL(do_page_one, (dbref, dbref, int, char *));

int sp_ok(dbref player)
{
	if(!mudconf.robot_speak) {
		if (Robot(player) && !controls(player, Location(player))) {
			notify(player, "Sorry robots may not speak in public");
			return 0;
		}
	}
	return 1;
}

static void say_shout (int target, const char *prefix, int flags, 
		dbref player, char *message)
{
	if (flags & SAY_NOTAG)
        raw_broadcast(player,target, "%s%.3900s", Name(player), message);
	else
		raw_broadcast(player,target, "%s%s%.3900s", prefix, Name(player), message);
}

static const char *announce_msg = "Announcement: ";
static const char *broadcast_msg = "Broadcast: ";

void do_think (dbref player, dbref cause, int key, char *message)
{
   char *s_buff, *s_buffptr;
   s_buffptr = s_buff = alloc_lbuf("do_think");
   if ( key == SAY_NOANSI ) {
      noansi_notify(player, safe_tprintf(s_buff, &s_buffptr, "%s",message));
   } else {
      notify(player, safe_tprintf(s_buff, &s_buffptr, "%s",message));
   }
   free_lbuf(s_buff);
}

void do_say (dbref player, dbref cause, int key, char *message)
{
  dbref	loc, aowner;
  char	*buf2, *bp, *pbuf, *tpr_buff, *tprp_buff;
  int	say_flags, depth, aflags, say_flags2, no_ansi;
  
	/* Convert prefix-coded messages into the normal type */
        no_ansi = key & (SAY_NOANSI);
        say_flags2 = key & (SAY_SUBSTITUTE);
	say_flags  = key & (SAY_NOTAG|SAY_HERE|SAY_ROOM);
	key &= ~(SAY_NOTAG|SAY_HERE|SAY_ROOM|SAY_SUBSTITUTE|SAY_NOANSI);

	if (key == SAY_PREFIX) {
		switch (*message++) {
		case '"':	key = SAY_SAY;
				break;
		case ':':	if (*message == ' ') {
					message++;
					key = SAY_POSE_NOSPC;
				} else {
					key = SAY_POSE;
				}
				break;
		case ';':	key = SAY_POSE_NOSPC;
				break;
		case '\\':	key = SAY_EMIT;
#ifdef ZENTY_ANSI            
            message++;
#endif            
				break;
		default:	return;
		}
	}

	/* Make sure speaker is somewhere if speaking in a place */

	loc = where_is(player);
	switch (key) {
	case SAY_SAY:
	case SAY_POSE:
	case SAY_POSE_NOSPC:
	case SAY_EMIT:
		if (!Good_obj(loc)) return;
		if (!sp_ok(player)) return;
		if ((Flags3(loc) & AUDIT) && !Admin(player) && !could_doit(player,loc,A_LSPEECH, 1)) {
		  did_it(player,loc,A_SFAIL, "You are not allowed to speak in this location.",
			0, NULL, A_ASFAIL, (char **)NULL, 0);
		  return;
		}
	}

	/* Send the message on its way  */
	switch (key) {
	case SAY_SHOUT:
	case SAY_WIZSHOUT:
	case SAY_WALLPOSE:
	case SAY_WIZPOSE:
	case SAY_WALLEMIT:
	case SAY_WIZEMIT:
		if (DePriv(player,NOTHING,DP_WALL,POWER6,POWER_LEVEL_NA)) {
		  notify(player, "Permission denied");
		  return;
		}
	}

	switch (key) {
	case SAY_SAY:
                pbuf = atr_get(player, A_SAYSTRING, &aowner, &aflags);
                tprp_buff = tpr_buff = alloc_lbuf("do_say");
                if ( SafeLog(player) ) {
                   if ( pbuf && *pbuf ) {
                      if ( no_ansi ) {
		         noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message));
                      } else {
		         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message));
                      }
                   } else {
                      if ( no_ansi ) {
		         noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message));
                      } else {
		         notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message));
                      }
                   }
                } else {
                   if ( no_ansi ) {
		      noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You say \"%s\"", message));
                   } else {
		      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You say \"%s\"", message));
                   }
                }
                tprp_buff = tpr_buff;
                if ( Anonymous(player) && Cloak(player) ) {
                   if ( pbuf && *pbuf ) {
#ifdef REALITY_LEVELS
                      
                      if ( no_ansi ) {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone %.30s \"%s\"", pbuf, message), MSG_NO_ANSI );
                      } else {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone %.30s \"%s\"", pbuf, message), 0);
                      }
#else
                      if ( no_ansi ) {
                         noansi_notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone %.30s \"%s\"", pbuf, message));
                      } else {
                         notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone %.30s \"%s\"", pbuf, message), 0);
                      }
#endif /* REALITY_LEVELS */
                   } else {
#ifdef REALITY_LEVELS
                      if ( no_ansi ) {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone says \"%s\"", message), MSG_NO_ANSI);
                      } else {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone says \"%s\"", message), 0);
                      }
#else
                      if ( no_ansi ) {
                         noansi_notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone says \"%s\"", message));
                      } else {
                         notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "Someone says \"%s\"", message), 0);
                      }
#endif /* REALITY_LEVELS */
                   }
                } else {
                   if ( pbuf && *pbuf ) {
#ifdef REALITY_LEVELS
                      if ( no_ansi ) {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message), MSG_NO_ANSI);
                      } else {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message), 0);
                      }
#else
                      if ( no_ansi ) {
                         noansi_notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message));
                      } else {
                         notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(player), pbuf, message), 0);
                      }
#endif /* REALITY_LEVELS */
                   } else {
#ifdef REALITY_LEVELS
                      if ( no_ansi ) {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message), MSG_NO_ANSI);
                      } else {
                         notify_except_rlevel(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message), 0);
                      }
#else
                      if ( no_ansi ) {
                         noansi_notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message));
                      } else {
                         notify_except(loc, player, player,
                              safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(player), message), 0);
                      }
#endif /* REALITY_LEVELS */
                   }
                }
                free_lbuf(tpr_buff);
                free_lbuf(pbuf);
		break;
	case SAY_POSE:
                tprp_buff = tpr_buff = alloc_lbuf("do_say");
                if ( Anonymous(player) && Cloak(player) ) {
#ifdef REALITY_LEVELS
                   if ( no_ansi ) {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone %s", message), MSG_NO_ANSI);
                   } else {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone %s", message), 0);
                   }
#else
                   if ( no_ansi ) {
                      noansi_notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone %s", message));
                   } else {
                      notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone %s", message));
                   }
#endif /* REALITY_LEVELS */
                } else {
#ifdef REALITY_LEVELS
                   if ( no_ansi ) {
                      notify_except_rlevel(loc, player, -1,
                        safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), message), MSG_NO_ANSI);
                   } else {
                      notify_except_rlevel(loc, player, -1,
                        safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), message), 0);
                   }
#else
                   if ( no_ansi ) {
                      noansi_notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), message));
                   } else {
                      notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), message));
                   }
#endif /* REALITY_LEVELS */
                }
                free_lbuf(tpr_buff);
		break;
	case SAY_POSE_NOSPC:
                tprp_buff = tpr_buff = alloc_lbuf("do_say");
                if ( Anonymous(player) && Cloak(player) ) {
#ifdef REALITY_LEVELS
                   if ( no_ansi ) {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone%s", message), MSG_NO_ANSI);
                   } else {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone%s", message), 0);
                   }
#else
                   if ( no_ansi ) {
                      noansi_notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone%s", message));
                   } else {
                      notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "Someone%s", message));
                   }
#endif /* REALITY_LEVELS */
                } else {
#ifdef REALITY_LEVELS
                   if ( no_ansi ) {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s%s", Name(player), message), MSG_NO_ANSI);
                   } else {
                      notify_except_rlevel(loc, player, -1,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s%s", Name(player), message), 0);
                   }
#else
                   if ( no_ansi ) {
                      noansi_notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s%s", Name(player), message));
                   } else {
                      notify_all_from_inside(loc, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s%s", Name(player), message));
                   }
#endif /* REALITY_LEVELS */
                }
                free_lbuf(tpr_buff);
		break;
	case SAY_EMIT:
	        if (say_flags2 & SAY_SUBSTITUTE)
		  mudstate.emit_substitute = 1;
		if ((say_flags & SAY_HERE) || !say_flags) {
#ifdef REALITY_LEVELS
                   if ( no_ansi ) {
                      notify_except_rlevel(loc, player, -1, message, MSG_NO_ANSI);
                   } else {
                      notify_except_rlevel(loc, player, -1, message, 0);
                   }
#else
                   if ( no_ansi ) {
                      noansi_notify_all_from_inside(loc, player, message);
                   } else {
                      notify_all_from_inside(loc, player, message);
                   }
#endif /* REALITY_LEVELS */
                }
		if (say_flags & SAY_ROOM) {
                   if ((Typeof(loc) == TYPE_ROOM) && (say_flags & SAY_HERE)) {
                      mudstate.emit_substitute = 0;
                      return;
                   }
                   depth = 0;
                   while((Typeof(loc) != TYPE_ROOM) && (depth++ < 20)) {
                      loc = Location(loc);
                      if ((loc == NOTHING) || (loc == Location(loc))) {
                         mudstate.emit_substitute = 0;
                         return;
                      }
                   }
                   if (Typeof(loc) == TYPE_ROOM) {
#ifdef REALITY_LEVELS
                      if ( no_ansi ) {
                         notify_except_rlevel(loc, player, -1, message, MSG_NO_ANSI);
                      } else {
                         notify_except_rlevel(loc, player, -1, message, 0);
                      }
#else
                      if ( no_ansi ) {
                         noansi_notify_all_from_inside(loc, player, message);
                      } else {
                         notify_all_from_inside(loc, player, message);
                      }
#endif /* REALITY_LEVELS */
                   }
		}
		mudstate.emit_substitute = 0;
		break;
	case SAY_SHOUT:
		if (say_flags & SAY_NOTAG) 
                   mudstate.nowall_over = 1;
		if (Admin(player)) {
		  /* let them pass quietly */
                } else if (Flags2(player) & NO_WALLS) {
                   notify(player, "You are set NO_WALLS and cannot send an announcement.");
                   return;
                } else if (No_yell(player)) {
                   notify(player, "Your @wall privilages are presently suspended.");
                   return;
                } else if ((Guildmaster(player) || HasPriv(player,NOTHING,POWER_FREE_WALL,POWER4,NOTHING) || FreeFlag(player)) &&
                           !DePriv(player,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) {
                   /* skip payment */
		} else if (!payfor(player,mudconf.wall_cost)) {
                   tprp_buff = tpr_buff = alloc_lbuf("do_say");
                   notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You don't have enough %s to shout.", mudconf.many_coins));
                   free_lbuf(tpr_buff);
                   return;
		} else {
                    tprp_buff = tpr_buff = alloc_lbuf("do_say");
                    notify(player, safe_tprintf(tpr_buff, &tprp_buff, 
                                   "You have been charged %d %s for that announcement.",
                                   mudconf.wall_cost,
                                   mudconf.wall_cost == 1 ? mudconf.one_coin : mudconf.many_coins));
                    free_lbuf(tpr_buff);
		}
                switch (*message) {
		case ':':
			message[0] = ' ';
			say_shout(0, announce_msg, say_flags, player, message);
			break;
		case ';':
			message++;
			say_shout(0, announce_msg, say_flags, player, message);
			break;
		case '"':
			message++;
		default:
			buf2 = alloc_lbuf("do_say.shout");
			bp = buf2;
			safe_str((char *)" shouts \"", buf2, &bp);
			safe_str(message, buf2, &bp);
			safe_chr('"', buf2, &bp);
			*bp = '\0';
			say_shout(0, announce_msg, say_flags, player, buf2);
			free_lbuf(buf2);
		}
		STARTLOG(LOG_SHOUTS,"WIZ","SHOUT")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.shout");
			sprintf(buf2, " shouts: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	case SAY_WIZSHOUT:
/*		if (!Admin(player) && No_yell(player)) {
			notify(player,
			  "Your @wall privilages are presently suspended.");
			return;
		}
*/		switch (*message) {
		case ':':
			message[0] = ' ';
			say_shout(BUILDER, broadcast_msg, say_flags, player,
				message);
			break;
		case ';':
			message++;
			say_shout(BUILDER, broadcast_msg, say_flags, player,
				message);
			break;
		case '"':
			message++;
		default:
			buf2 = alloc_lbuf("do_say.wizshout");
			bp = buf2;
                        pbuf = atr_get(player, A_SAYSTRING, &aowner, &aflags);
                        if ( pbuf && *pbuf ) {
                           tprp_buff = tpr_buff = alloc_lbuf("do_say");
                           safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %.30s \"", pbuf), buf2, &bp);
                           free_lbuf(tpr_buff);
                        } else 
			   safe_str((char *)" says \"", buf2, &bp);
                        free_lbuf(pbuf);
			safe_str(message, buf2, &bp);
			safe_chr('"', buf2, &bp);
			*bp = '\0';
			say_shout(BUILDER, broadcast_msg, say_flags, player,
				buf2);
			free_lbuf(buf2);
		}
		STARTLOG(LOG_SHOUTS,"WIZ","BCAST")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.wizshout");
			sprintf(buf2, " broadcasts: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	case SAY_WALLPOSE:
		if (say_flags & SAY_NOTAG)
			raw_broadcast(player, 0, "%s %.3900s", Name(player), message);
		else
			raw_broadcast(player, 0, "Announcement: %s %.3900s", Name(player),
				message);
		STARTLOG(LOG_SHOUTS,"WIZ","SHOUT")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.wallpose");
			sprintf(buf2, " WALLposes: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	case SAY_WIZPOSE:
		if (say_flags & SAY_NOTAG)
			raw_broadcast(player, BUILDER, "%s %.3900s", Name(player), message);
		else
			raw_broadcast(player, BUILDER, "Broadcast: %s %.3900s", Name(player),
				message);
		STARTLOG(LOG_SHOUTS,"WIZ","BCAST")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.wizpose");
			sprintf(buf2, " WIZposes: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	case SAY_WALLEMIT:
		if (say_flags & SAY_NOTAG)
			raw_broadcast(player, 0, "%.3900s", message);
		else
			raw_broadcast(player, 0, "Announcement: %.3900s", message);
		STARTLOG(LOG_SHOUTS,"WIZ","SHOUT")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.wallemit");
			sprintf(buf2, " WALLemits: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	case SAY_WIZEMIT:
		if (say_flags & SAY_NOTAG)
			raw_broadcast(player, BUILDER, "%.3900s", message);
		else
			raw_broadcast(player, BUILDER, "Broadcast: %.3900s", message);
		STARTLOG(LOG_SHOUTS,"WIZ","BCAST")
			log_name(player);
			buf2=alloc_lbuf("do_say.LOG.wizemit");
			sprintf(buf2, " WIZemit: '%.3900s'", message);
			log_text(buf2);
			free_lbuf(buf2);
		ENDLOG
		break;
	}
	mudstate.nowall_over = 0;
}

/* ---------------------------------------------------------------------------
 * do_page: Handle the page command.
 * Page-pose code from shadow@prelude.cc.purdue.
 */

static void page_pose (dbref player, dbref target, int port, char *message, int key)
{
  char	*nbuf, *tpr_buff, *tprp_buff;
  
  nbuf = alloc_lbuf("page_pose");
  strcpy(nbuf, Name(player));
  mudstate.pageref = player;
  if (Wizard(player))
    mudstate.droveride = 1;
  tprp_buff = tpr_buff = alloc_lbuf("page_pose");
  if ( key & PAGE_NOANSI ) {
     noansi_notify_with_cause2(port, player,
		        safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), message));
  } else {
     notify_with_cause2(port, player,
		        safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), message));
  }
  free_lbuf(tpr_buff);
  mudstate.droveride = 0;
  if (mudstate.pageref != NOTHING) {
    tprp_buff = tpr_buff = alloc_lbuf("page_pose");
    if ( key & PAGE_NOANSI ) {
       noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Long distance to port %d: %s%s",
                      port, nbuf, message));
    } else {
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Long distance to port %d: %s%s",
                      port, nbuf, message));
    }
    free_lbuf(tpr_buff);
  } else
    notify(player, "Bad Port.");
  free_lbuf(nbuf);
}

static void page_return (dbref player, dbref target, const char *tag, 
                         int anum, const char *dflt, const char *s_pagestr)
{
   dbref aowner;
   int aflags, i_pagebuff;
   char *str, *str2, *tpr_buff, *tprp_buff, *s_pagebuff[2];
   struct tm *tp;
   time_t t;

   if (Wizard(player))
      mudstate.droveride = 1;
   str = atr_pget(target, anum, &aowner, &aflags);
   if (*str) {
      mudstate.chkcpu_stopper = time(NULL);
      mudstate.chkcpu_toggle = 0;
      s_pagebuff[1] = NULL;
      if ( s_pagestr && *s_pagestr ) {
         s_pagebuff[0] = (char *)s_pagestr;
         i_pagebuff = 1;
      } else {
         s_pagebuff[0] = NULL;
         i_pagebuff = 0;
      }
      str2 = exec(target, player, player, EV_FCHECK|EV_EVAL|EV_TOP, str,
                  s_pagebuff, i_pagebuff);
      t = time(NULL);
      tp = localtime(&t);
      if (*str2) {
         tprp_buff = tpr_buff = alloc_lbuf("page_return");
         notify_with_cause(player, target,
                           safe_tprintf(tpr_buff, &tprp_buff, "%s message from %s: %s",
                                        tag, Name(target), str2));
         tprp_buff = tpr_buff;
         notify_with_cause(target, player,
                           safe_tprintf(tpr_buff, &tprp_buff, "[%d:%02d] %s message sent to %s.",
                                        tp->tm_hour, tp->tm_min, tag, Name(player)));
         free_lbuf(tpr_buff);
      }
      free_lbuf(str2);
   } else if (dflt && *dflt) {
      notify_with_cause(player, target, dflt);
   }
   mudstate.droveride = 0;
   free_lbuf(str);
}

static int page_check (dbref player, dbref target, int num, int length)
{
  char *buff, *tpr_buff, *tprp_buff;
  int cost = 0;

  // c = cost, l = length, p = mudconf.pagecost
  // c = (l + p^2 - 1) / p^2
  //      cost=(length+(1<<mudconf.pagecost)-1)>>mudconf.pagecost;
  if(mudconf.pagecost > 0)
      cost = (length + (mudconf.pagecost-1)) / mudconf.pagecost;
  
  
	buff = alloc_lbuf("page_check");
	sprintf(buff,"Sorry, %s is not connected.",Name(target));
	if ((!HasPriv(player,NOTHING,POWER_FREE_PAGE,POWER4,NOTHING) ||
	      DePriv(player,NOTHING,DP_FREE,POWER6,POWER_LEVEL_NA)) && 
	    (num < 1) && (!payfor(player, Guest(player) ? 0 : cost))) {
                tprp_buff = tpr_buff = alloc_lbuf("page_check");
		notify(player,
			safe_tprintf(tpr_buff, &tprp_buff, "You don't have enough %s.",
				     mudconf.many_coins));
                free_lbuf(tpr_buff);
	} else if (!Connected(target)) {
		page_return(player, target, "Away", A_AWAY, buff, NULL);
	} else if (Cloak(target) && ((SCloak(target) && !Immortal(player)) || !Wizard(player))) {
		page_return(player, target, "Away", A_AWAY, buff, NULL);
	} else if (DePriv(player, target, DP_PAGE, POWER6, NOTHING)) {
		notify(player, "Permission denied.");
	} else if (!could_doit(player, target, A_LPAGE,1)) {
		if (Wizard(target) && Cloak(target))
			page_return(player, target, "Away", A_AWAY, buff, NULL);
		else {
			sprintf(buff,"Sorry, %s is not accepting pages.", Name(target));
			page_return(player, target, "Reject", A_REJECT, buff, NULL);
		}
	} else if (!could_doit(target, player, A_LPAGE,3)) {
                tprp_buff = tpr_buff = alloc_lbuf("page_check");
		if (Wizard(player)) {
			notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Warning: %s can't return your page.",Name(target)));
			free_lbuf(buff);
                        free_lbuf(tpr_buff);
			return 1;
		} else {
			notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Sorry, %s can't return your page.",Name(target)));
			free_lbuf(buff);
                        free_lbuf(tpr_buff);
			return 0;
		}
	} else {
		free_lbuf(buff);
		return 1;
	}
	free_lbuf(buff);
	return 0;
}

void do_page(dbref player, dbref cause, int key, char *tname, char *message)
{
  dbref	target, owner, pl_aowner;
  char	*nbuf, *p1, *p2, buff[50], *dbuff, *dbx, *fbuff, *fbx, *sbuff, sep, *pos1, 
        *pos2, *pos3, *px, *lbuff, *lbx, *alias_pos1, *alias_px, *pl_alias, *mpg, 
        *t_msg, *t_msgp, *tpr_buff, *tprp_buff, *s_ret_warn, *s_ret_warnptr;
  int	flags, port, got, got2, num, nuts, pc, fnum, lnum, *ilist, pl_aflags, 
        mpr_chk, nkey, s_ret_warnkey, ansikey;
  
        /* Lensy:
         *  If they type 'page' by itself, then tell them who they last paged
         */
        pc = mpr_chk = nkey = ansikey = 0;
        if ( key & PAGE_NOANSI ) {
           ansikey = PAGE_NOANSI;
        }
        key = (key &~ PAGE_NOANSI);
        if ((((!tname || !*tname) && !message) || (*tname == '\0' && *message == '\0'))
              && MuxPage(player) && !(key & PAGE_LOC)) {
          // get last page attr
	  if ( key & PAGE_PORT) {
	     notify(player, "Last page/return page not avaiable to ports switch.");
	     return;
	  }
          p1 = atr_get(player,A_LASTPAGE,&owner,&flags);
          if (!p1 || !*p1 ) {
            notify(player, "You have not yet paged anyone.");
          } else {
            lbx = lbuff = alloc_lbuf("do_page");
            safe_str("You last paged ", lbuff, &lbx);
            // work out the name
            got = 0;
            mpg = (char *)strtok_r(p1, " ", &t_msg);
            while ( mpg && *mpg ) {
               target = lookup_player(player, mpg, 1);
               if ( Good_obj(target) ) {
                  if ( got )
                     safe_str(", ", lbuff, &lbx);
                  safe_str(Name(target), lbuff, &lbx);
                  got = 1;
               }
               mpg = (char *)strtok_r(NULL, " ", &t_msg);
            }
            if ( !got ) {
               safe_str("a now non-existant player.", lbuff, &lbx);
            } else {
               safe_chr('.', lbuff, &lbx);
            }
            // notify the player
            notify(player, lbuff);
            free_lbuf(lbuff);
          }
          free_lbuf(p1);
          return;
        }
 
        if ( MuxPage(player) && !(key & (PAGE_RET | PAGE_LAST | PAGE_RETMULTI)) && 
             ((!*message && tname && *tname) || (*message && (!tname || !*tname))) ) {
           if ( key & PAGE_PORT ) {
	     notify(player, "Last page/return page not avaiable to ports switch.");
	     return;
	   }
	   dbuff = sbuff = alloc_lbuf("do_page");
           if ( tname && *tname )
              safe_str(tname, sbuff, &dbuff);
           if ( *message ) {
              if ( tname && *tname )
                 safe_chr('=', sbuff, &dbuff);
              safe_str(message, sbuff, &dbuff);
           }
	   nkey = PAGE_LAST;
	   nkey |= (key & PAGE_LOC) ? PAGE_LOC : 0;
           (void) do_page_one(player, cause, nkey, sbuff);
           free_lbuf(sbuff);
           return;
        }

	sbuff = alloc_lbuf("do_page");
	dbuff = alloc_lbuf("do_page");
	fbuff = alloc_lbuf("do_page");
	lbuff = alloc_lbuf("do_page");
	*sbuff = '\0';
	*dbuff = '\0';
	*fbuff = '\0';
	*lbuff = '\0';
	if (key == PAGE_RET) {
		p1 = atr_get(player,A_RETPAGE,&owner,&flags);
		if (!*p1) {
		  notify(player,"Your ReturnPage attribute is empty.");
		  free_lbuf(p1);
		  free_lbuf(sbuff);
		  free_lbuf(dbuff);
		  free_lbuf(fbuff);
		  free_lbuf(lbuff);
		  return;
		}
                mpg = strtok(p1, " ");
		strcpy(sbuff,mpg);
		free_lbuf(p1);
		p1 = tname;
		sep = ' ';
	}
	else if (key == PAGE_RETMULTI) {
		p1 = atr_get(player,A_RETPAGE,&owner,&flags);
		if (!*p1) {
		  notify(player,"Your ReturnPage attribute is empty.");
		  free_lbuf(p1);
		  free_lbuf(sbuff);
		  free_lbuf(dbuff);
		  free_lbuf(fbuff);
		  free_lbuf(lbuff);
		  return;
		}
		strcpy(sbuff,p1);
		free_lbuf(p1);
		p1 = tname;
		sep = ' ';
	}
	else if (key == PAGE_LAST) {
		p1 = atr_get(player,A_LASTPAGE,&owner,&flags);
		if (!*p1) {
		  notify(player,"Your LastPage attribute is empty.");
		  free_lbuf(p1);
		  free_lbuf(sbuff);
		  free_lbuf(dbuff);
		  free_lbuf(fbuff);
		  free_lbuf(lbuff);
		  return;
		}
		strcpy(sbuff,p1);
		free_lbuf(p1);
		p1 = tname;
		sep = ' ';
	}
	else if (key == PAGE_PORT) {
	  strcpy(sbuff,tname);
	  p1 = message;
	  sep = ' ';
	}
	else {
	  strcpy(sbuff,tname);
	  p1 = message;
/* Disabled the name_spaces checking.  Enable if you want it */
	  if (mudconf.name_spaces && 0)
	    sep = ',';
	  else {
	    pos1 = strchr(sbuff,',');
	    if (pos1 != NULL)
	      sep = ',';
	    else
	      sep = ' ';
	  }
	}
	num = 0;
	got = 0;
	got2 = 0;
	fnum = 0;
	lnum = 0;
	pos1 = sbuff;
	dbx = dbuff;
	fbx = fbuff;
	lbx = lbuff;
	while (*pos1 && (isspace((int)*pos1) || (*pos1 == sep)))
	  pos1++;
        tprp_buff = tpr_buff = alloc_lbuf("do_page");
	while (*pos1) {
	  pos2 = pos1 + 1;
	  while (*pos2 && (*pos2 != sep))
	    pos2++;
	  pos3 = pos2 - 1;
	  if (*pos2) 
	    pos2++;
	  while (isspace((int)*pos3))
	    pos3--;
	  pos3++;
	  *pos3 = '\0';
	  if (key == PAGE_PORT) {
	    target = NOTHING;
	    port = atoi(pos1);
	    if (port < 1)
	      port = 0;
	  }
	  else {
	    pc = 0;
	    port = 0;
	    p2 = strstr(lbuff,pos1);
	    if (p2 && ((*(p2 + strlen(pos1)) == ',') || !*(p2 + strlen(pos1)))) {
	      pos1 = pos2;
	      while (*pos1 && (isspace((int)*pos1) || (*pos1 == sep)))
		pos1++;
	      continue;
	    }
	    target = lookup_player(player,pos1,1);
            mpr_chk = 0;
            if ( (target == player) && (key == PAGE_RETMULTI) ) {
               target = NOTHING;
               mpr_chk = 1;
            }
	    if (target != NOTHING) {
	      sprintf(buff,"%d",target);
	      p2 = strstr(fbuff,buff);
	      if (!(p2 && (isspace((int)*(p2 + strlen(buff))) || !*(p2 + strlen(buff))))) {
		sprintf(buff,"#%d",target);
	        p2 = strstr(dbuff,buff);
	        if (p2 && (isspace((int)*(p2 + strlen(buff))) || !*(p2 + strlen(buff)))) {
	          pos1 = pos2;
	          while (*pos1 && (isspace((int)*pos1) || (*pos1 == sep)))
	            pos1++;
	          continue;
	        }
	        if (page_check(player, target, got2, strlen(p1))) {
		  pc = 1;
	          if (num > 0)
	            safe_chr(' ',dbuff,&dbx);
	          safe_str(buff,dbuff,&dbx);
	        }
		else {
		  if (fnum)
		    safe_chr(' ',fbuff,&fbx);
		  safe_str(buff,fbuff,&fbx);
		  fnum = 1;
		}
	      }
	      got2 = 1;
	    }
	    else {
	      if (lnum)
	        safe_chr(',',lbuff,&lbx);
	      safe_str(pos1,lbuff,&lbx);
	      lnum = 1;
	    }
	  }
	  got = 1;
	  if ((target == NOTHING) && !port) {
             if ( !mpr_chk )
		notify(player, "I don't recognize that name.");
	  } else if ((key != PAGE_PORT) && !pc) {
		;
	  } else if (!*p1) {
		if (key != PAGE_PORT) {
/* Players should get their page updated on a return-page */
/*		  if (key != PAGE_RET) { */
		    sprintf(buff,"#%d",player);
		    atr_add_raw(target,A_RETPAGE,buff);
/*		  } */
		  num++;
		}
		nbuf = alloc_lbuf("do_page");
		strcpy(nbuf, Name(where_is(player)));
		if (port) {
		  	mudstate.pageref = player;
		  	if (Wizard(player))
		    	  mudstate.droveride = 1;
                        tprp_buff = tpr_buff;
			notify_with_cause2(port, player,
			     safe_tprintf(tpr_buff, &tprp_buff, "You sense that %s is looking for you in %s",
				          Name(player), nbuf));
		  	mudstate.droveride = 0;
			if (mudstate.pageref == NOTHING)
				notify(player, "Bad Port.");
			else  {
                                tprp_buff = tpr_buff;
				notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You notified port %d of your location.",
					port));
                        }
		}
		free_lbuf(nbuf);
	  } else {
		if (key != PAGE_PORT) {
/*		  if (key != PAGE_RET) { */
		    sprintf(buff,"#%d",player);
		    atr_add_raw(target,A_RETPAGE,buff);
/*		  } */
		  num++;
		}
	      if (port) {
		nuts = 0;
		switch (*p1) {
		case ':':
			p1[0] = ' ';
			page_pose(player, target, port, p1, ansikey);
			p1[0] = ':';
			break;
		case ';':
			page_pose(player, target, port, p1+1, ansikey);
			break;
		case '"':
			p1++;
			nuts = 1;
		default:
		  	mudstate.pageref = player;
		  	if (Wizard(player))
		    	  mudstate.droveride = 1;
                        tprp_buff = tpr_buff;
                        if ( !ansikey ) {
			   notify_with_cause2(port, player,
				   safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                        } else {
			   noansi_notify_with_cause2(port, player,
				   safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                        }
			mudstate.droveride = 0;
			if (mudstate.pageref == NOTHING)
			   notify(player, "Bad Port.");
			else  {
                           tprp_buff = tpr_buff;
                           if ( !ansikey ) {
			      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged port %d with '%s'.",
			             port, p1));
                           } else {
			      noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged port %d with '%s'.",
			             port, p1));
                           }
			}
			if (nuts)
			  p1--;
		}
	      }
	  }
	  pos1 = pos2;
	  while (*pos1 && (isspace((int)*pos1) || (*pos1 == sep)))
	    pos1++;
	}
	free_lbuf(tpr_buff);
	if (!got)
	  notify(player, "I don't recognize that name."); 
	if ((num > 0) && (key != PAGE_PORT) && (*dbuff)) {
	  ilist = (int *)malloc(sizeof(int) * num);
	  if (ilist == NULL) {
	    free_lbuf(sbuff);
	    free_lbuf(dbuff);
	    free_lbuf(fbuff);
	    free_lbuf(lbuff);
	    return;
	  }
          alias_pos1 = alloc_lbuf("do_page_alias");
          alias_px = alias_pos1;
	  pos1 = alloc_lbuf("do_page");
	  px = pos1;
	  nbuf = alloc_lbuf("do_page");
	  safe_str("(To: ",pos1,&px);
	  safe_str("(To: ",alias_pos1,&alias_px);
	  if (num > 0) {
	    pos2 = dbuff;
	    for (got = 0; got < num; got++) {
	      pos3 = pos2+2;
	      while (*pos3 && !isspace((int)*pos3))
		pos3++;
	      *pos3 = '\0';
	      target = atoi(pos2+1);
	      *(ilist + got) = target;
	      if ((got > 0) && (got == (num - 1))) {
		if (num > 2) {
		  safe_str(", and ",pos1,&px);
		  safe_str(", and ",alias_pos1,&alias_px);
		} else {
		  safe_str(" and ",pos1,&px);
		  safe_str(" and ",alias_pos1,&alias_px);
                }
	      }
	      else if (got > 0) {
		safe_str(", ",pos1,&px);
		safe_str(", ",alias_pos1,&alias_px);
              }
	      safe_str(Name(target),pos1,&px);
	      safe_str(Name(target),alias_pos1,&alias_px);
              pl_alias = atr_get(target, A_ALIAS, &pl_aowner, &pl_aflags);
              if (*pl_alias) {
                 safe_chr('(', alias_pos1, &alias_px);
                 safe_str(pl_alias, alias_pos1, &alias_px);
                 safe_chr(')', alias_pos1, &alias_px);
              }
              free_lbuf(pl_alias);
	      if (got < (num - 1)) {
		*pos3 = ' ';
		pos2 = pos3+1;
	      }
	    }
	    safe_chr(')',pos1,&px);
	    safe_chr(')',alias_pos1,&alias_px);
	  }
	  strcpy(nbuf, Name(where_is(player)));
          tprp_buff = tpr_buff = alloc_lbuf("do_page");
          s_ret_warnptr = s_ret_warn = alloc_lbuf("do_page_warn");
          s_ret_warnkey = 0;
	  for (got = 0; got < num; got++) {
	    target = *(ilist + got);
            if ( Good_chk(target) && ((Cloak(player) && !Wizard(target)) ||
                                      (SCloak(player) && !Immortal(target))) ) {
               if ( s_ret_warnkey )
                  safe_str(", ", s_ret_warn, &s_ret_warnptr);
               safe_str(Name(target), s_ret_warn, &s_ret_warnptr);
               s_ret_warnkey=1;
            }
	    if (!*p1) {
	      if (Wizard(player))
	        mudstate.droveride = 1;
              pl_alias = atr_get(player, A_ALIAS, &pl_aowner, &pl_aflags);
              tprp_buff = tpr_buff;
	      if (num == 1) {
                if ( Good_obj(target) && VPage(target) ) {
                   if (*pl_alias) {
	              notify_with_cause(target, player,
			      safe_tprintf(tpr_buff, &tprp_buff, "You sense that %s(%s) is looking for you in %s",
				      Name(player), pl_alias, nbuf));
                   } else {
	              notify_with_cause(target, player,
	                  safe_tprintf(tpr_buff, &tprp_buff, "You sense that %s is looking for you in %s", 
				   Name(player), nbuf));
                   }
                } else {
	           notify_with_cause(target, player,
			   safe_tprintf(tpr_buff, &tprp_buff, "You sense that %s is looking for you in %s",
				   Name(player), nbuf));
                }
	      } else {
                if ( Good_obj(target) && VPage(target) ) {
                   if (*pl_alias) {
	              notify_with_cause(target, player,
			      safe_tprintf(tpr_buff, &tprp_buff, "%s You sense that %s(%s) is looking for you in %s", alias_pos1,
				      Name(player), pl_alias, nbuf));
                   } else {
	              notify_with_cause(target, player,
			      safe_tprintf(tpr_buff, &tprp_buff, "%s You sense that %s is looking for you in %s", alias_pos1,
				      Name(player), nbuf));
                   }
                } else {
	           notify_with_cause(target, player,
			   safe_tprintf(tpr_buff, &tprp_buff, "%s You sense that %s is looking for you in %s", pos1,
				   Name(player), nbuf));
                }
              }
              free_lbuf(pl_alias);
	      mudstate.droveride = 0;
	      page_return(player, target, "Idle", A_IDLE, NULL, tpr_buff);
	    } else {
		nuts = 0;
		switch (*p1) {
		case ':':
			p1[0] = ' ';
		case ';':
			if (p1[0] == ';') {
			  p1++;
			  nuts = 1;
			}
			if (Wizard(player))
			  mudstate.droveride = 1;
                        pl_alias = atr_get(player, A_ALIAS, &pl_aowner, &pl_aflags);
                        tprp_buff = tpr_buff;
			if (num == 1) {
                          if ( Good_obj(target) && VPage(target) ) {
                             if (*pl_alias) {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s(%s)%s", Name(player), pl_alias, p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s(%s)%s", Name(player), pl_alias, p1));
                                }
                             } else {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), p1));
                                }
                             }
                          } else {
                             if ( !ansikey ) {
	        	        notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), p1));
                             } else {
	        	        noansi_notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "From afar, %s%s", Name(player), p1));
                             }
                          }
			} else {
                          if ( Good_obj(target) && VPage(target) ) {
                             if (*pl_alias) {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s(%s)%s", alias_pos1, Name(player), 
                                                 pl_alias, p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s(%s)%s", alias_pos1, Name(player), 
                                                 pl_alias, p1));
                                }
                             } else {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s%s", alias_pos1, Name(player), p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s%s", alias_pos1, Name(player), p1));
                                }
                             }
                          } else {
                             if ( !ansikey ) {
			        notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s%s", pos1, Name(player), p1));
                             } else {
			        noansi_notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s From afar, %s%s", pos1, Name(player), p1));
                             }
                          }
                        }
                        free_lbuf(pl_alias);
			if (nuts)
			  p1--;
			else
			  p1[0] = ':';
			break;
		case '"':
			p1++;
			nuts = 1;
		default:
		  	if (Wizard(player))
		    	  mudstate.droveride = 1;
                        pl_alias = atr_get(player, A_ALIAS, &pl_aowner, &pl_aflags);
                        tprp_buff = tpr_buff;
			if (num == 1) {
                          if ( Good_obj(target) && VPage(target) ) {
                             if (*pl_alias) {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s(%s) pages: %s", Name(player), pl_alias, p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s(%s) pages: %s", Name(player), pl_alias, p1));
                                }
                             } else {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                                }
                             }
                          } else {
                             if ( !ansikey ) {
			        notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                             } else {
			        noansi_notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s pages: %s", Name(player), p1));
                             }
                          }
			} else {
                          if ( Good_obj(target) && VPage(target) ) {
                             if (*pl_alias) {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s %s(%s) pages: %s", alias_pos1, Name(player), 
                                                  pl_alias, p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s %s(%s) pages: %s", alias_pos1, Name(player), 
                                                  pl_alias, p1));
                                }
                             } else {
                                if ( !ansikey ) {
			           notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s %s pages: %s", alias_pos1, Name(player), p1));
                                } else {
			           noansi_notify_with_cause(target, player,
				         safe_tprintf(tpr_buff, &tprp_buff, "%s %s pages: %s", alias_pos1, Name(player), p1));
                                }
                             }
                          } else {
                             if ( !ansikey ) {
			        notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s %s pages: %s", pos1, Name(player), p1));
                             } else {
			        noansi_notify_with_cause(target, player,
				      safe_tprintf(tpr_buff, &tprp_buff, "%s %s pages: %s", pos1, Name(player), p1));
                             }
                          }
                        }
                        free_lbuf(pl_alias);
			mudstate.droveride = 0;
			if (nuts)
			  p1--;
		} 
		page_return(player, target, "Idle", A_IDLE, NULL, tpr_buff);
	    } 
            if ( (key == 0) || (key == PAGE_RETMULTI) || (key == PAGE_LAST)) {
               t_msgp = t_msg = alloc_lbuf("retpage_multi");
               tprp_buff = tpr_buff;
               safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#%d %s", player, dbuff), t_msg, &t_msgp);
	       atr_add_raw(target,A_RETPAGE,t_msg);
               free_lbuf(t_msg);
            }
	  }
          free_lbuf(tpr_buff);
	  if (!*p1) {
            tprp_buff = tpr_buff = alloc_lbuf("do_page");
	    if (num == 1) {
	      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You notified %s of your location.",
		Name(*ilist)));
	    }
	    else {
	      *(pos1 + strlen(pos1) - 1) = '\0';
	      notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You notified %s of your location.",
		pos1 + 5));
	    }
            free_lbuf(tpr_buff);
	  } else {
	    switch (*p1) {
	      case ':':
		p1[0] = ' ';
	      case ';':
		pos3 = alloc_lbuf("do_page");
		strcpy(pos3, Name(player));
		if (p1[0] == ';')
		  p1++;
                tprp_buff = tpr_buff = alloc_lbuf("do_page");
		if (num == 1) {
                  if ( !ansikey ) {
		     notify(player,safe_tprintf(tpr_buff, &tprp_buff, "Long distance to %s: %s%s", Name(*ilist), pos3, p1));
                  } else {
		     noansi_notify(player,safe_tprintf(tpr_buff, &tprp_buff, "Long distance to %s: %s%s", Name(*ilist), pos3, p1));
                  }
		}
		else {
		  *(pos1 + strlen(pos1) - 1) = '\0';
                  if ( !ansikey ) {
		     notify(player,safe_tprintf(tpr_buff, &tprp_buff, "Long distance to %s: %s%s", pos1 + 5, pos3, p1));
                  } else {
		     noansi_notify(player,safe_tprintf(tpr_buff, &tprp_buff, "Long distance to %s: %s%s", pos1 + 5, pos3, p1));
                  }
		}
                free_lbuf(tpr_buff);
		free_lbuf(pos3);
		break;
	      case '"':
		p1++;
	      default:
                tprp_buff = tpr_buff = alloc_lbuf("do_page");
		if (num == 1) {
                  if ( !ansikey ) {
		     notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged %s with '%s'.", Name(*ilist), p1));
                  } else {
		     noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged %s with '%s'.", Name(*ilist), p1));
                  }
		}
		else {
		  *(pos1 + strlen(pos1) - 1) = '\0';
                  if ( !ansikey ) {
		     notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged %s with '%s'.", pos1 + 5, p1));
                  } else {
		     noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You paged %s with '%s'.", pos1 + 5, p1));
                  }
		}
                free_lbuf(tpr_buff);
	    }
	  }
          if ( *s_ret_warn ) {
             tprp_buff = tpr_buff = alloc_lbuf("do_page");
             tprp_buff = tpr_buff;
             notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Warning - You are hidden from the following users: %s", 
                                         s_ret_warn));
             free_lbuf(tpr_buff);
          }
          free_lbuf(s_ret_warn);
	  free_lbuf(nbuf);
	  free_lbuf(pos1);
          free_lbuf(alias_pos1);
	  free(ilist);
          if ( (key != PAGE_RETMULTI) && (key != PAGE_RET) )
	     atr_add_raw(player,A_LASTPAGE,dbuff);
	}
	free_lbuf(sbuff);
	free_lbuf(dbuff);
	free_lbuf(fbuff);
	free_lbuf(lbuff);
}

void do_page_one(dbref player, dbref cause, int key, char *arg)
{
  char *tbuff;

  tbuff = alloc_lbuf("page_one");
  *tbuff = '\0';
  do_page(player, cause, key, arg, tbuff);
  free_lbuf(tbuff);
}

/* ---------------------------------------------------------------------------
 * do_pemit: Messages to specific players, or to all but specific players.
 */

void whisper_pose (dbref player, dbref target, char *message)
{
   char *buff, *s_buff, *s_buffptr;

   buff=alloc_lbuf("do_pemit.whisper.pose");
   s_buffptr = s_buff = alloc_lbuf("whisper_pose");
   strcpy (buff, Name(player));
   if (mudstate.pageref == NOTHING) {
      if ((SCloak(target) && Cloak(target) && !Immortal(player)) ||
          (Cloak(target) && !Wizard(player))) {
      } else {
         notify(player, safe_tprintf(s_buff, &s_buffptr, "%s senses \"%s%s\"", Name(target), buff, message));
      }
      s_buffptr = s_buff;
      notify_with_cause(target, player, safe_tprintf(s_buff, &s_buffptr, "You sense %s%s", buff, message));
   } else {
      notify_with_cause2((int)target, player, safe_tprintf(s_buff, &s_buffptr, "You sense %s%s", buff, message));
      if (mudstate.pageref != NOTHING) {
         s_buffptr = s_buff;
         notify(player, safe_tprintf(s_buff, &s_buffptr, "Port %d senses \"%s%s\"", target, buff, message));
      } else {
         notify(player, "Bad Port.");
      }
   }
   free_lbuf(buff);
   free_lbuf(s_buff);
}


#define DBG_PT(x) fprintf(stderr, "DBG PT%s\n", x);
void do_pemit (dbref player, dbref cause, int key, char *recipient, 
		char *message, char *cargs[], int ncargs)
{
dbref	target, loc, aowner, darray[LBUF_SIZE/2];
char	*buf2, *bp, *recip2, *rcpt, list, plist, *buff3, *buff4, *result, *pt1, *pt2;
char	*pc1, *tell, *tx, sep1, *pbuf, *tpr_buff, *tprp_buff, *recipient_buff;
char    *strtok, *strtokr, *strtokbuf;
#ifdef REALITY_LEVELS
char    *pt3, *r_bufr, *s_ptr, *reality_buff;
#endif
int	do_contents, ok_to_do, depth, pemit_flags, port, dobreak, got, cstuff, cntr; 
int     do_zone, in_zone, aflags, is_zonemaster, noisy, nosub, noansi, noeval, is_rlevelon, i_realitybit;
int     xxx_x, xxx_y, xxx_z, i_oneeval, i_snufftoofar, i_oemitstr, dcntr, dcntrtmp;

ZLISTNODE *z_ptr, *y_ptr;

   port = 0;
   rcpt = NULL;
   i_snufftoofar = is_rlevelon = i_realitybit = i_oneeval = i_oemitstr = dcntr = 0;
   xxx_x = xxx_y = xxx_z = 0;

   if ((Flags3(player) & NO_PESTER) && (key & (PEMIT_PEMIT|PEMIT_LIST|PEMIT_WHISPER|PEMIT_CONTENTS))) {
      notify(player, "Permission denied.");
      return;
   }
   if ( key & SIDEEFFECT ) {
      key &=~SIDEEFFECT;
   }
   if ( key & PEMIT_NOSUB ) {
      nosub = 1;
      key &= ~PEMIT_NOSUB;
   } else {
      nosub = 0;
   }
   if(key & PEMIT_NOANSI ) {
      noansi = 1;
      key &= ~PEMIT_NOANSI;
   } else {
      noansi = 0;
   }        
   if ( key & PEMIT_OSTR ) {
      key &= ~PEMIT_OSTR;
      i_oemitstr = 1;
   }
   recipient_buff = recipient;
#ifdef REALITY_LEVELS
   if (  ((key & PEMIT_TOREALITY) && !(key & PEMIT_CONTENTS)) ) {
      notify(player, "Illegal combination of switches.");
      return;
   }
   if (  ((key & PEMIT_ONEEVAL) && !(key & PEMIT_LIST)) ) {
      notify(player, "Illegal combination of switches.");
      return;
   }
   if ( key & (PEMIT_ONEEVAL) ) {
      i_oneeval = 1;
      key &= ~PEMIT_ONEEVAL;
   }
   if ( key & (PEMIT_REALITY|PEMIT_TOREALITY) ) {
      is_rlevelon = 1;
      key &= ~PEMIT_REALITY;
   }
   if ( key & PEMIT_TOREALITY ) {
      key &= ~PEMIT_TOREALITY;
      s_ptr =  recipient;
      pt3 = reality_buff = alloc_lbuf("reality_buff");
      while ( *s_ptr && *s_ptr != '/' ) {
         safe_chr(*s_ptr, reality_buff, &pt3);
         s_ptr++;
      }
      if ( *s_ptr ) s_ptr++;
      recipient_buff = s_ptr;

      if ( *reality_buff && (mudconf.no_levels > 0) ) {
         xxx_x = (int)sizeof(mudconf.reality_level);
         xxx_y = (int)sizeof(mudconf.reality_level[0]);
         if ( xxx_y == 0 )
            xxx_z = 0;
         else
            xxx_z = xxx_x / xxx_y;
         pt3 = (char *)strtok_r(reality_buff, " ", &r_bufr);
         while ( pt3 && *pt3 ) {
            for ( cntr = 0; (cntr < mudconf.no_levels) && (cntr < xxx_z); cntr++) {
               if ( !stricmp(pt3, mudconf.reality_level[cntr].name) &&
                    (Immortal(player) || (TxLevel(player) & mudconf.reality_level[cntr].value)) )
                  i_realitybit |= mudconf.reality_level[cntr].value;
            }
            pt3 = (char *)strtok_r(NULL, " ", &r_bufr);
         }
      }
      free_lbuf(reality_buff);
   }
#endif
   sep1 = '\0';
   mudstate.pageref = NOTHING;
   if (key & PEMIT_CONTENTS) {
      do_contents = 1;
      key &= ~PEMIT_CONTENTS;
   } else {
      do_contents = 0;
   }
   if ( key & PEMIT_ZONE) {
      do_zone = 1;
      key &= ~PEMIT_ZONE;
   } else {
      do_zone = 0;
   }
   if (key & PEMIT_NOISY) {
      noisy = 1;
      key &= ~PEMIT_NOISY;
   } else {
      noisy = 0;
   }
   if (key & PEMIT_NOEVAL) {
      noeval = 1;
      key &= ~PEMIT_NOEVAL;
   } else {
      noeval = 0;
   }
   pemit_flags = key & (PEMIT_HERE|PEMIT_ROOM);
   key &= ~(PEMIT_HERE|PEMIT_ROOM);
   if (key & PEMIT_LIST) {
      plist = 1;
      key &= ~PEMIT_LIST;
      pt1 = exec(player, cause, cause, EV_STRIP|EV_FCHECK|EV_EVAL,
                 recipient_buff, cargs, ncargs);
      recip2 = pt1;
      while (isspace((int)*recip2))
         recip2++;
      pt2 = recip2 + strlen(recip2) - 1;
      while (isspace((int)*pt2)) 
         pt2--;
      *(pt2+1) = '\0';
   } else {
      plist = 0;
      recip2 = recipient_buff;
   }
   if (key == (PEMIT_PORT | PEMIT_PEMIT)) {
      key = PEMIT_PORT;
   } else if (key == (PEMIT_WHISPER | PEMIT_PORT)) {
      key = PEMIT_WPORT;
   }
   list = 1;
   result = message;
   got = 0;
   cstuff = 0;
   cntr=1;
   if ( i_oneeval && !noeval ) {
      result = exec(player, cause, cause, EV_EVAL|EV_STRIP|EV_FCHECK|EV_TOP,
                    message, cargs, ncargs);
   }
   while (list) {
      ok_to_do = 0;
      mudstate.whisper_state = 1;
      if (plist) {
         rcpt = recip2;
         if (!sep1)
            while ((*rcpt) && (*rcpt != ' ') && (*rcpt != ',')) rcpt++;
         else
            while ((*rcpt) && (*rcpt != sep1)) rcpt++;
         if (*rcpt == '\0')
            list = 0;
         else {
            sep1 = *rcpt;
            *rcpt = '\0';
         }
         if ( !i_oneeval ) {
            if ( !nosub ) {
               buff3 = replace_string(BOUND_VAR,recip2,message, 0);
               tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
               buff4 = replace_string(NUMERIC_VAR, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), buff3, 0);
               free_lbuf(tpr_buff);
               if (!noeval) {
                  result = exec(player, cause, cause, EV_STRIP|EV_FCHECK|EV_EVAL,
                                buff4, cargs, ncargs);
               }
               free_lbuf(buff3);
               free_lbuf(buff4);
            } else {
               buff4 = buff3 = alloc_lbuf("pemit_list_nosub");
               safe_str(message, buff3, &buff4);
               if (!noeval) {
                  result = exec(player, cause, cause, EV_STRIP|EV_FCHECK|EV_EVAL,
                                buff3, cargs, ncargs);
               }
               free_lbuf(buff3);
            }
         }
         cntr++;
      } else {
         list = 0;
         if ((key == PEMIT_PEMIT) || (key == PEMIT_PORT)) {
            if (!noeval && !i_oneeval) {
               result = exec(player, cause, cause, EV_EVAL|EV_STRIP|EV_FCHECK|EV_TOP,
                             message, cargs, ncargs);
            }
         }
      }
      switch (key) {
         case PEMIT_FSAY:
         case PEMIT_FPOSE:
         case PEMIT_FPOSE_NS:
         case PEMIT_FEMIT:
            target = match_affected(player, recip2);
            if (target == NOTHING) 
               return;
            ok_to_do = 1;
            break;
         case PEMIT_PORT:
         case PEMIT_WPORT:
            target = NOTHING;
            pc1 = recip2;
            while (*pc1 && isdigit((int)*pc1))
               pc1++;
            if (!*pc1)
               port = atoi(recip2);
            else
               port = 0;
            mudstate.pageref = player;
            break;
         default:
            if ( i_oemitstr ) {
               strtokbuf = alloc_lbuf("oemit_multi");
               memcpy(strtokbuf, recip2, LBUF_SIZE - 1);
               strtok = strtok_r(strtokbuf, " \t", &strtokr);
               while ( strtok ) {
                  init_match_real(player, strtok, TYPE_PLAYER, i_realitybit);
                  match_everything(0);
                  target = match_result();
                  if ( Good_chk(target) ) {
                     if ((SCloak(target) && Cloak(target) && !Immortal(player)) ||
                         (Cloak(target) && !Wizard(player))) {
                        target = NOTHING;
                     } else {
                        for ( dcntrtmp = 0; dcntrtmp < dcntr; dcntrtmp++ ) {
                           if ( darray[dcntrtmp] == target ) {
                              dcntrtmp = -1;
                              break;
                           }
                        }
                        if ( dcntrtmp >= 0 ) {
                           darray[dcntr] = target;
                           dcntr++;
                        }
                     }
                  }
                  strtok = strtok_r(NULL, " \t", &strtokr);
               }
               free_lbuf(strtokbuf);
               if ( dcntr > 0 )
                  target = darray[0];
               else
                  target = NOTHING;
            } else {
               init_match_real(player, recip2, TYPE_PLAYER, i_realitybit);
               match_everything(0);
               target = match_result();
            }
      }
      mudstate.whisper_state = 0;
      if ( (target != NOTHING) && (target != AMBIGUOUS) ) {
         if ((SCloak(target) && Cloak(target) && !Immortal(player)) ||
             (Cloak(target) && !Wizard(player))) {
            switch (key) {
               case PEMIT_WHISPER:
                  notify(player, "Whisper to whom?");
                  if (Privilaged(Owner(player)) ||
                      HasPriv(player,NOTHING,POWER_LONG_FINGERS,POWER3,NOTHING)) { 
                     cstuff = 1;
                  } else
                     cstuff = 2;
                  break;
               case PEMIT_PEMIT:
                  notify(player, "Emit to whom?");
                  i_snufftoofar = 1;
                  break;
               case PEMIT_OEMIT:
                  target = NOTHING;
            }
         }
      }
      switch (target) {
         case AMBIGUOUS:
            notify(player, "I don't know who you mean!");
            break;
         case NOTHING:
            switch (key) {
               case PEMIT_WHISPER:
               case PEMIT_WPORT:
                  if (key == PEMIT_WPORT) {
                     switch (*result) {
                        case ':':
                           result[0] = ' ';
                           whisper_pose(player, (dbref)port, result);
                           break;
                        case ';':
                           result++;
                           whisper_pose(player, (dbref)port, result);
                           break;
                        case '"':
                           result++;
                        default:
                           tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                           notify_with_cause2(port, player,
                                  safe_tprintf(tpr_buff, &tprp_buff, "%s whispers \"%s\"", Name(player), result));
                           free_lbuf(tpr_buff);
                           if (mudstate.pageref != NOTHING) {
                              tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                              notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You whisper \"%s\" to port %d.",
                                     result, port));
                              free_lbuf(tpr_buff);
                           } else
                              notify(player,"Bad Port.");
                     }
                  } else {
                     notify(player, "Whisper to whom?");
                  }
                  break;
               case PEMIT_PEMIT:
                  notify(player, "Emit to whom?");
                  i_snufftoofar = 1;
                  break;
               case PEMIT_OEMIT:
                  if ( !i_oemitstr || (i_oemitstr && (dcntr == 0)) )
                     notify(player, "Emit except to whom?");
                  break;
               case PEMIT_PORT:
                  notify_with_cause2(port, player, result);
                  if (mudstate.pageref == NOTHING)
                     notify(player,"Bad Port.");
                  break;
               default:
                  notify(player, "Sorry.");
            }
            break;
         default:
            /* Enforce locality constraints */
            if (!ok_to_do && (Privilaged(Owner(player)) ||
               HasPriv(player,NOTHING,POWER_LONG_FINGERS,POWER3,NOTHING))) {
               ok_to_do = 1;
            }
            if (ok_to_do && ((Cloak(player) && !Wizard(player)) || (SCloak(player) && !Immortal(player)))) {
               ok_to_do = 0;
            }
            if (!ok_to_do && (nearby(player,target) || Controls(player, target))) {
               ok_to_do = 1;
            }
            if (!ok_to_do && (key == PEMIT_PEMIT) && (Typeof(target) == TYPE_PLAYER) && mudconf.pemit_players) {
               if (!page_check(player, target, got, strlen(result)))
                  break;
               ok_to_do = 1;
            }
            if (!ok_to_do && (cstuff != 1) && (!mudconf.pemit_any || (key != PEMIT_PEMIT))) {
               if (!cstuff && !i_snufftoofar)
                  notify(player, "You are too far away to do that.");
               break;
            }
            if (do_contents && !Controls(player, target) && !mudconf.pemit_any) {
               notify(player, "Permission denied.");
               break;
            }
            if (ZoneMaster(target))
               is_zonemaster=1;
            else
               is_zonemaster=0;
            if (do_zone) {
               in_zone = 0;
               if (!is_zonemaster) {
                  for ( z_ptr = db[player].zonelist; z_ptr; z_ptr = z_ptr->next) {
                     for ( y_ptr = db[target].zonelist; y_ptr; y_ptr = y_ptr->next) {
                        if ( z_ptr->object == y_ptr->object ) {
                           in_zone=1;
                           break;
                        }
                     }
                     if (in_zone)
                        break;
                  }
               }
               if ( !in_zone && !is_zonemaster) {
                  notify(player, "Invalid zone.");
                  ok_to_do = 0;
                  break;
               }
               if ( (!Controls(player,target) && !could_doit(player,target,A_LZONEWIZ,0)) ) {
                  notify(player, "Invalid zone.");
                  ok_to_do = 0;
                  break;
               }
            }
            loc = where_is(target);
            if ((key == PEMIT_PEMIT) && (Flags3(target) & AUDIT) && !Admin(player) &&
                !could_doit(player,target,A_LSPEECH,1)) {
               did_it(player,target,A_SFAIL, "You are not allowed to speak to that location.",
                      0, NULL, A_ASFAIL, (char **)NULL, 0);
               break;
            }
            dobreak = 0;
            switch (key) {
               case PEMIT_OEMIT:
                  if ((Flags3(loc) & AUDIT) && !Admin(player) && !could_doit(player,loc,A_LSPEECH,1)) {
                     did_it(player,loc,A_SFAIL, "You are not allowed to speak to that location.",
                            0, NULL, A_ASFAIL, (char **)NULL, 0);
                     dobreak = 1;
                  }
                  break;
               case PEMIT_FSAY:
               case PEMIT_FPOSE:
               case PEMIT_FPOSE_NS:
               case PEMIT_FEMIT:
                  if ((Flags3(loc) & AUDIT) && !Admin(player) && !could_doit(player,loc,A_LSPEECH,1)) {
                     did_it(player,target,A_SFAIL, "You are not allowed to speak to that location.",
                            0, NULL, A_ASFAIL, (char **)NULL, 0);
                     dobreak = 1;
                  }
            }
            if (dobreak)
               break;
            switch (key) {
               case PEMIT_PEMIT:
                  if (do_contents || do_zone) {
                     if (is_zonemaster && do_zone) {
                        for ( z_ptr = db[target].zonelist; z_ptr; z_ptr = z_ptr->next) {
                           if ( z_ptr->object != target ) {
                              if (isRoom(z_ptr->object)) {
#ifdef REALITY_LEVELS
                                 if ( !is_rlevelon || (is_rlevelon && IsRealArg(z_ptr->object, player, 0)) ) {
                                    if ( is_rlevelon )
                                       mudstate.reality_notify = 1;
#endif
                                    if(noansi)
                                       noansi_notify_all_from_inside_real(z_ptr->object, player, result, i_realitybit);
                                    else
                                       notify_all_from_inside_real(z_ptr->object, player, result, i_realitybit);
#ifdef REALITY_LEVELS
                                    mudstate.reality_notify = 0;
                                 }
#endif
                              }
                           }
                        }
                     }
                     if (Has_contents(target)) {
#ifdef REALITY_LEVELS
                        if ( !is_rlevelon || (is_rlevelon && IsRealArg(target, player, 0)) ) {
                           if ( is_rlevelon )
                              mudstate.reality_notify = 1;
#endif
                           if(noansi) {
                              noansi_notify_all_from_inside_real(target, player, result, i_realitybit);
                           } else {
                              notify_all_from_inside_real(target, player, result, i_realitybit);
                           }
#ifdef REALITY_LEVELS
                           mudstate.reality_notify = 0;
                        }
#endif
                     }
                  } else {
#ifdef REALITY_LEVELS
                     if ( !is_rlevelon || (is_rlevelon && IsRealArg(target, player, 0)) ) {
#endif
                        if(noansi)
                           noansi_notify_with_cause_real(target, player, result, i_realitybit);
                        else
                           notify_with_cause_real(target, player, result, i_realitybit);
#ifdef REALITY_LEVELS
                     }
#endif
                  }
                  got = 1;
                  break;
               case PEMIT_OEMIT:
                  if (loc != NOTHING) {
                     if ( i_oemitstr ) 
                        notify_except_str(loc, player, darray, dcntr, result, 0);
                     else
                        notify_except(loc, player, target, result, 0);
                     if ( i_oemitstr && (dcntr > 0) ) {
                        tell = alloc_lbuf("see_omit");
                        tx = tell;
                        safe_str("[oemit] ",tell,&tx);
                        safe_str(result,tell,&tx);
                        for (dcntrtmp = 0; dcntrtmp < dcntr; dcntrtmp++) {
                           if ( Flags3(darray[dcntrtmp]) & SEE_OEMIT ) {
                              if ( noansi ) {
                                 noansi_notify_with_cause(darray[dcntrtmp],player,tell);
                              } else {
                                 notify_with_cause(darray[dcntrtmp],player,tell);
                              }
                           }
                        }
                        free_lbuf(tell);
                     } else {
                        if (Flags3(target) & SEE_OEMIT) {
                           tell = alloc_lbuf("see_omit");
                           tx = tell;
                           safe_str("[oemit] ",tell,&tx);
                           safe_str(result,tell,&tx);
                           if ( noansi ) {
                              noansi_notify_with_cause(target,player,tell);
                           } else {
                              notify_with_cause(target,player,tell);
                           }
                           free_lbuf(tell);
                        }
                     }
                  }
                  break;
               case PEMIT_WHISPER:
                  switch (*result) {
                     case ':':
                        result[0] = ' ';
                        whisper_pose(player, target, result);
                        break;
                     case ';':
                        result++;
                        whisper_pose(player, target, result);
                        break;
                     case '"':
                        result++;
                     default:
                        if (!((SCloak(target) && Cloak(target) && !Immortal(player)) ||
                            (Cloak(target) && !Wizard(player)))) {
                           tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                           notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You whisper \"%s\" to %s.",
                                  result, Name(target)));
                           free_lbuf(tpr_buff);
                        }
                        tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                        notify_with_cause(target, player, safe_tprintf(tpr_buff, &tprp_buff, "%s whispers \"%s\"",
                                          Name(player), result));
                        free_lbuf(tpr_buff);
                  }
                  if ((!mudconf.quiet_whisper) && !(Wizard(player) && Cloak(player))) {
                     loc = where_is(player);
                     if (loc != NOTHING && !(Wizard(target) && Cloak(target))) {
                        buf2=alloc_lbuf("do_pemit.whisper.buzz");
                        bp = buf2;
                        safe_str(Name(player), buf2, &bp);
                        safe_str((char *)" whispers something to ", buf2, &bp);
                        tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                        safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s.",Name(target)), buf2, &bp);
                        free_lbuf(tpr_buff);
                        *bp = '\0';
                        notify_except2(loc, player, player, target, buf2);
                        free_lbuf(buf2);
                     }
                  }
                  break;
               case PEMIT_FSAY:
                  tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                  pbuf = atr_get(player, A_SAYSTRING, &aowner, &aflags);
                  if ( SafeLog(target) && (target == player) ) {
                     if ( pbuf && *pbuf ) {
		        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(target), pbuf, result));
                     } else {
		        notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(target), result));
                     }
                  } else {
		     notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You force %s to say \"%s\"", Name(target), result));
                  }
                  if (loc != NOTHING) {
                     tprp_buff = tpr_buff;
                     if ( pbuf && *pbuf )
                        notify_except(loc, player, target,
                                      safe_tprintf(tpr_buff, &tprp_buff, "%s %.30s \"%s\"", Name(target), pbuf, result), 0);
                     else
                        notify_except(loc, player, target,
                                      safe_tprintf(tpr_buff, &tprp_buff, "%s says \"%s\"", Name(target), result), 0);
                  }
                  free_lbuf(pbuf);
                  free_lbuf(tpr_buff);
                  break;
               case PEMIT_FPOSE:
                  tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                  notify_all_from_inside(loc, player, safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(target), result));
                  free_lbuf(tpr_buff);
                  break;
               case PEMIT_FPOSE_NS:
                  tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
                  notify_all_from_inside(loc, player, safe_tprintf(tpr_buff, &tprp_buff, "%s%s", Name(target), result));
                  free_lbuf(tpr_buff);
                  break;
               case PEMIT_FEMIT:
                  if ((pemit_flags & PEMIT_HERE) || !pemit_flags)
#ifdef REALITY_LEVELS
                     if ( !is_rlevelon || (is_rlevelon && IsRealArg(loc, player, 0)) ) {
                        if ( is_rlevelon )
                           mudstate.reality_notify = 1;
#endif
                        notify_all_from_inside_real(loc, player, result, i_realitybit);
#ifdef REALITY_LEVELS
                        mudstate.reality_notify = 0;
                     }
#endif
                  if (pemit_flags & PEMIT_ROOM) {
                     if ((Typeof(loc) == TYPE_ROOM) && (pemit_flags & PEMIT_HERE)) {
                        break;
                     }
                     depth = 0;
                     while((Typeof(loc) != TYPE_ROOM) && (depth++ < 20)) {
                        loc = Location(loc);
                        if ((loc == NOTHING) || (loc == Location(loc)))
                           break;
                     }
                     if (Typeof(loc) == TYPE_ROOM) {
                        if ((Flags3(loc) & AUDIT) && !Admin(player) &&
                            !could_doit(player,loc,A_LSPEECH,1)) {
                           did_it(player,target,A_SFAIL, "You are not allowed to speak to that location.",
                                  0, NULL, A_ASFAIL, (char **)NULL, 0);
                           break;
                        } else {
#ifdef REALITY_LEVELS
                           if ( !is_rlevelon || (is_rlevelon && IsRealArg(loc, player, 0)) ) {
                              if ( is_rlevelon )
                                 mudstate.reality_notify = 1;
#endif
                              notify_all_from_inside_real(loc, player, message, i_realitybit);
#ifdef REALITY_LEVELS
                              mudstate.reality_notify = 0;
                           }
#endif
                        }
                     }
                  }
                  break;
            }
      }
      if (plist || (key == PEMIT_PEMIT) || (key == PEMIT_PORT)) {
         if (noisy) {
            tprp_buff = tpr_buff = alloc_lbuf("do_pemit");
            if ( Good_obj(target) ) {
               if(noansi)
                  noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You pemited '%s' to %s.",result,Name(target)));
               else
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You pemited '%s' to %s.",result,Name(target)));
            } else {
               if(noansi)
                  noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You pemited '%s'",result));
               else
                  notify(player, safe_tprintf(tpr_buff, &tprp_buff, "You pemited '%s'",result));
            }
            free_lbuf(tpr_buff);
         }
         if (!noeval && !i_oneeval) {	  
            free_lbuf(result);
         }
      }
      if (list) {
         recip2 = rcpt + 1;
         while (isspace((int)*recip2)) 
            recip2++;
      }
   }
   if ( i_oneeval && !noeval ) {
      free_lbuf(result);
   }
   if (plist)
      free_lbuf(pt1);
}

void com_who(),com_send();
int getword(char*,char*);

void do_channel(dbref player, dbref cause, int key, char *arg1)
{
  char buf[LBUF_SIZE], buff2[LBUF_SIZE];
  int aflags;
  dbref aowner;
  char *tmp, *tpr_buff, *tprp_buff;
  char *tmp_word;
  char *chk_ansi;
  int inx, iny;

  inx = iny = 0;

  if (DePriv(player,NOTHING,DP_COM,POWER6,POWER_LEVEL_NA)) {
    notify(player,"Permission denied.");
    return;
  }
  tmp = arg1;
  buf[0] = '\0';
  while ((*tmp != '\0') && (*tmp != ' '))
    tmp++;
  if (*tmp == ' ')
    *tmp = '\0';
  if(*arg1 == '\0') {
	 if(*(tmp = atr_get(player,A_CHANNEL,&aowner,&aflags))) {
		notify(player,"You are currently on the following channels: ");
      tmp_word = strtok( tmp, " " );
      tprp_buff = tpr_buff = alloc_lbuf("do_channel");
      while( tmp_word ) {
         if ( iny == 0 ) {
            iny = 1;
            strcpy( buf, tmp_word );
         }
         else if ( inx > 1 ) {
            tprp_buff = tpr_buff = alloc_lbuf("do_channel");
            notify(player, safe_tprintf(tpr_buff, &tprp_buff, "   %s", buf ) );
            strcpy( buf, tmp_word );
            inx = 0;
         }
         else if ( inx < 1 ) {
            sprintf( buf, "%-20.20s     %-20.20s", buf, tmp_word );
            inx++;
         }
         else {
            sprintf( buf, "%-45.45s     %-20.20s", buf, tmp_word );
            inx++;
         }
      tmp_word = strtok( NULL, " " );
      } /* While */
      free_lbuf(tpr_buff);
      notify(player, unsafe_tprintf( "   %s", buf ) ); /* Print last line */
      notify(player, "Channel listing Completed." );
    } /* If */
	 else {
		notify(player,"You aren't currently on any channels. For a general chatting channel, turn to");
		notify(player,"channel 'public'");
	 }
	 free_lbuf(tmp);
	 return;
  }
  if(*arg1 == '+') {
	 char xxx[LBUF_SIZE];
	 char word[LBUF_SIZE];

	 arg1++;
	 if(Typeof(player)!=TYPE_PLAYER) {
		notify(player,"Permission denied, You must be a player.");
		return;
	 }
   if(!(Admin(player)) && *arg1 == '*') {
		notify(player,"Permission denied!");
		return;
	 }
	 if(!(Builder(player)) && *arg1 == '.') {
      notify(player,"Permission denied!");
      return;
    }
	 if(!(Wizard(player)) && *arg1 == '+') {
      notify(player,"Permission denied!");
      return;
    }
  
    tmp = atr_get(player,A_CHANNEL,&aowner,&aflags);
    strncpy(xxx,tmp,LBUF_SIZE - 1);
    *(xxx + LBUF_SIZE - 1) = '\0';
    free_lbuf(tmp);

    for ( chk_ansi = arg1; chk_ansi && *chk_ansi; chk_ansi++ ) {
        if ( !isprint( (int)*chk_ansi )) {
           notify(player, "You can't have that as a channel name!");
           return;
        }
    }
    if( *arg1 == ' ' || *arg1 == '\0' ) {
      notify(player, "You can't have that as a channel name!");
      return;
    }
    if(strlen(arg1) > 20 ) {
      notify(player, "You can't have channels over 20 characters long.");
      return;
    }
    while(getword(word,xxx)) {
      if(!strcmp(word,arg1)) {
	notify(player,"You are already on that channel!");
	return;
      }
    }

    if(!*(tmp=atr_get(player,A_CHANNEL,&aowner,&aflags)))
      atr_add(player,A_CHANNEL,arg1,aowner,aflags);
    else {
      if( strlen(arg1) + strlen(tmp) >= LBUF_SIZE - 1 ) {
        notify(player,"Your channel list is too long to add another!");
        free_lbuf(tmp);
        return;
      }
      sprintf(buff2,"%s %s",arg1,tmp);
      atr_add(player,A_CHANNEL,buff2,aowner,aflags);
    }
    free_lbuf(tmp);

    if (!Wizard(player)) {
        sprintf(buf,"[%s] %s has joined this channel.",arg1, Name(player));
        com_send(arg1,buf);
        sprintf(buf,"%s has joined this channel.",Name(player));
    }
    notify(player,unsafe_tprintf("%s has been added to your channel list.",arg1));
    return;
  }

  if(*arg1 == '-') {
    char xxx[LBUF_SIZE];
    char word[LBUF_SIZE];
    int ck = 0,start=0;

    arg1++;
    if (*arg1 == ' ' || *arg1 == '\0') {
      notify(player, "You must specify a valid channel name to delete.");
      return;
    }
    for ( chk_ansi = arg1; chk_ansi && *chk_ansi; chk_ansi++ ) {
        if ( !isprint( (int)*chk_ansi )) {
           notify(player, "You must specify a valid channel name to delete.");
           return;
        }
    }
    tmp = atr_get(player,A_CHANNEL,&aowner,&aflags);

    xxx[0] = 0;
    while(getword(word,tmp)) {
      if(strcmp(word,arg1)) {
	if (start) strcat(xxx," ");
        start=1;
        strcat(xxx,word);
      } else ck = 1;
    }
    free_lbuf(tmp);
 
    if(!ck) {
      notify(player,unsafe_tprintf("You aren't on channel %s.",arg1));
      return;
    }

    if (!Wizard(player)) {
       sprintf(buf,"[%s] %s has left this channel.",arg1,Name(player));
       com_send(arg1,buf);
       sprintf(buf,"%s has left this channel.",Name(player));
	 }

    notify(player,unsafe_tprintf("%s has been deleted from your channel list.",arg1));
   
    if (xxx[0]) atr_add(player,A_CHANNEL,xxx,aowner,aflags);
    else atr_clr(player,A_CHANNEL);
    return;
  }
  notify(player,"Usage:");
  notify(player,"  +channel +<channel>    :adds a channel");
  notify(player,"  +channel -<channel>    :deletes a channel");
  notify(player,"  +channel               :lists your channels.");
  notify(player,"For a general chatting channel, try channel 'public'.");
}

void do_com(dbref player, dbref cause, int key, 
	const char *arg1, const char *a2)
{
  char arg2[2048],chan[1000],*ptr, *pbuf;
  static char mess[2000];
  int aflags, aflags2;
  dbref aowner, aowner2;
  char *tmp;
  char tmp_word[LBUF_SIZE];
  int failure = 1;

  if(Typeof(player)!=TYPE_PLAYER) {
    notify(player,"Players only please.");
    return;
  }
  if (DePriv(player,NOTHING,DP_COM,POWER6,POWER_LEVEL_NA)) {
    notify(player,"Permission denied.");
    return;
  }
  memset(arg2, '\0', sizeof(arg2));
  strncpy(arg2,a2,2047);
  if ((strcmp(arg1,"public") != 0) && (strcmp(arg1,"sting") != 0))
    if(!payfor(player,mudconf.comcost)) {
        notify(player,"You don't have enough gold pieces.");
        return;
    }
  if(!*arg2) {
    notify(player,"No message.");
    notify(player,"Syntax:" );
    notify(player,"   +com <channel> = who        :shows who is on the channel");
    notify(player,"   +com <channel> = <message>  :say something on the channel");
    notify(player,"For a general chatting channel, try channel 'public'.");
    return;
  }
  memset(chan, '\0', sizeof(chan));
  if(*arg1)
    strncpy(chan,arg1,999);
  else {
    strcpy(chan,"public");
  }
  if(!*chan) {
    notify(player,"no channel.");
    return;
  }
  tmp = atr_get(player,A_CHANNEL,&aowner,&aflags);
  while( getword(tmp_word, tmp) )
      if(!strcmp(tmp_word,chan)) 
         failure = 0;
  if ( failure && ( !(Wizard(player) && !string_compare(arg2, "who")) ) ) {
    notify(player,"You are not on that channel.");
    free_lbuf(tmp);
    return;
  }
  for(ptr=chan;*ptr;ptr++)
    if(*ptr==' ') {*ptr='\0';break;}
  if(!string_compare(arg2,"who"))
    com_who(chan,player);
  else if (arg2[0] == ':') {
    arg2[0] = ' ';
/*  sprintf(mess,"[%s] %s%s",chan,Name(player),
            arg2);   -EEK!  Overflow! */
    sprintf(mess,"[%s] %s",chan,Name(player));
    strncat(mess, arg2, 2047 - strlen( mess )); 
    com_send(chan,mess);
  }
  else if (arg2[0] == ';'){
    sprintf(mess, "[%s] %s",chan,Name(player));
    strncat(mess, &arg2[1], 2047 - strlen( mess ));
    com_send(chan,mess);
  }
  else if (arg2[0] == '"'){
    pbuf = atr_get(player, A_SAYSTRING, &aowner2, &aflags2);
    if ( pbuf && *pbuf )
       sprintf(mess, "[%s] %s %.30s, ",chan,Name(player), pbuf);
    else
       sprintf(mess, "[%s] %s says, ",chan,Name(player));
    free_lbuf(pbuf);
    strncat(mess, arg2, 2046 - strlen( mess ));
    strcat(mess, "\"" );
    com_send(chan,mess);
  }
  else if (arg2[0] == '!' && (Immortal(player) || Wizard(player)) ) {
    arg2[0] = ' ';
    sprintf(mess,"[%s]", chan );
    strncat(mess, arg2, 2047 - strlen( mess ));
    com_send(chan,mess);
  }
  else {
/*  sprintf(mess,"[%s] %s: %s",chan,Name(player),
            arg2); */
    sprintf(mess,"[%s] %s: ",chan,Name(player));
    strncat(mess, arg2, 2047 - strlen( mess )); 
    com_send(chan,mess);
  }
  free_lbuf(tmp);
}

void com_send(char *chan, char *mess)
{
  struct descriptor_data *d;
  char *yyy;
  int aflags;
  dbref aowner;

  DESC_ITER_CONN(d) {
      char *foo;
      yyy=atr_get(d->player,A_CHANNEL,&aowner,&aflags);
      if(*yyy) {
        char *ptr;
        strcat(yyy," ");
        foo=yyy;
        while(*foo) {
          for(ptr=foo;*ptr && *ptr != ' ';ptr++);
          if(*ptr == ' ') *ptr = '\0';
          if(!strcmp(foo,chan)) /* tell the person. */
            notify(d->player,mess);
          foo+=strlen(foo)+1;
        }
      }
      free_lbuf(yyy);
  }
}

void com_who(char *chan, dbref who)
{
  struct descriptor_data *d;
  char *yyy,*membuff, *tpr_buff, *tprp_buff;
  int aflags;
  dbref aowner;

  notify(who,unsafe_tprintf("-- Scanning for connected users on channel %s --", chan ) );
  tprp_buff = tpr_buff = alloc_lbuf("com_who");
  DESC_ITER_CONN(d) {
    if(Findable(d->player) || Wizard(who)) {
      char *foo;
      if (Immortal(d->player) && Cloak(d->player) && SCloak(d->player) && !Immortal(who))
	continue;
      yyy = atr_get(d->player,A_CHANNEL,&aowner,&aflags);
      if(*yyy) {
        char *ptr;
        strcat(yyy," ");
        foo=yyy;
        while(*foo) {
          for(ptr=foo;*ptr && *ptr != ' ';ptr++);
          if(*ptr == ' ') *ptr = '\0';
          if(!strcmp(foo,chan)) {
            membuff = unparse_object(who,d->player,0);
            tprp_buff = tpr_buff;
            notify(who,safe_tprintf(tpr_buff, &tprp_buff, "%s is on channel %s.",membuff,chan));
            free_lbuf(membuff);
          }
          foo+=strlen(foo)+1;
        }
      }
      free_lbuf(yyy);
    }
  }
  free_lbuf(tpr_buff);
  notify(who,unsafe_tprintf("-- End of listing for channel %s --",chan));
}


int getword(char *s, char *t)
{
  int flag = 0;
  int inx = 0;

  while (*t == ' ') t++; /* Lensy: It was *t++ */
  /* inx is a counter so that char *s doesn't overflow */
  /* Currently this value is LBUF_SIZE for a char array      */
  while (*t != '\0' && *t != ' ' && inx < LBUF_SIZE - 1) {
        *s++ = *t;
        *t++ = ' ';
        if (!flag) flag = 1;
        inx++;
  }
  *s = '\0';
  return(flag);
}

