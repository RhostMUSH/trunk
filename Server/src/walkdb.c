/* walkdb.c -- Support for commands that walk the entire db */

#ifdef SOLARIS
/* Borked header files don't have declarations for Solaris */
char *rindex(const char *, int);
#endif

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "command.h"
#include "flags.h"
#include "alloc.h"
#include "misc.h"

/* Bind occurrences of the universal var in ACTION to ARG, then run ACTION.
   Cmds run in low-prio Q after a 1 sec delay for the first one. */

static void bind_and_queue (dbref player, dbref cause, char *action, 
		char *argstr, char *cargs[], int ncargs)
{
char	*command;		/* allocated by replace_string  */

	command = replace_string (BOUND_VAR, argstr, action, 0),
	wait_que (player, cause, 0, NOTHING, command, cargs, ncargs,
		mudstate.global_regs, mudstate.global_regsname);
	free_lbuf(command);
}

/* New @dolist  i.e.:
 * @dolist #12 #34 #45 #123 #34644=@emit [name(##)]
 *
 * New switches added 12/92, /space (default) delimits list using spaces,
 * and /delimit allows specification of a delimiter.
 */

int
convert_totemslist(dbref player, char *flaglist, SEARCH **fset)
{
   TOTEMENT *tp;
   char *s_strtok, *s_strtokr, *t_buff, *t_buff2, *t_buff2ptr;
   int i_len, i_found, i_valid, i_cnt;

   if (!flaglist || !*flaglist ) {
      return 0;
   }

   t_buff = alloc_lbuf("convert_totemlist");
   t_buff2ptr = t_buff2 = alloc_lbuf("convert_totemlist2");
   strcpy(t_buff, flaglist);
   s_strtok = strtok_r(t_buff, " \t", &s_strtokr);
   i_found = i_valid = i_cnt = 0;
   safe_str("WARNING - Totems unrecognized: ", t_buff2, &t_buff2ptr);
   while ( s_strtok && *s_strtok) {
      i_len = strlen(s_strtok);
      for (tp = (TOTEMENT *) hash_firstentry2(&mudstate.totem_htab, 1);
           tp;
           tp = (TOTEMENT *) hash_nextentry(&mudstate.totem_htab)) {

         /* Player can not see flag, continue */
         if ( !totem_cansee_bit(player, player, tp->listperm) ) {
            continue;
         }

         if ( !strncasecmp(s_strtok, tp->flagname, i_len) ) {
            (*fset)->i_totems[tp->flagpos] |= tp->flagvalue;
            i_found = i_valid = 1;
            break;
         }
      }
      if ( i_valid == 0 ) {
         if ( i_cnt ) {
            safe_str(", ", t_buff2, &t_buff2ptr);
         }
         safe_str(s_strtok, t_buff2, &t_buff2ptr);
         i_cnt++;
      }
      i_valid = 0;
      s_strtok = strtok_r(NULL, " \t", &s_strtokr);
   }
   if ( i_cnt ) {
      notify_quiet(player, t_buff2);
   }
   free_lbuf(t_buff);
   free_lbuf(t_buff2);
   return i_found;
}

int
convert_totems(dbref player, char *flaglist, SEARCH **fset, int word)
{
   TOTEMENT *tp;
   int i_err = 0;
   char *s_tmp, *s_tmpptr;

   if ( !flaglist || !*flaglist ) {
      return 0;
   }

   s_tmpptr = s_tmp = alloc_lbuf("convert_totems");
   for (tp = (TOTEMENT *) hash_firstentry2(&mudstate.totem_htab, 1);
        tp;
        tp = (TOTEMENT *) hash_nextentry(&mudstate.totem_htab)) {
      /* Tier does not match, continue */
      if ( tp->flagtier != word ) {
         continue;
      }

      /* Flag letter char is default char, continue */
      if ( (tp->flagtier == 0) && (tp->flaglett == '?') ) {
         continue;
      }

      /* Player can not see flag, continue */
      if ( !totem_cansee_bit(player, player, tp->listperm) ) {
         continue;
      }

      /* If target character does not match list passed, continue */
      if ( strchr(flaglist, tp->flaglett) == NULL ) {
         continue;
      }

      safe_chr(tp->flaglett, s_tmp, &s_tmpptr);
      /* All worked out, add the slot to the value */
      (*fset)->i_totems[tp->flagpos] |= tp->flagvalue;
   }

   s_tmpptr = flaglist;
   while ( *s_tmpptr ) {
      if ( strchr(s_tmp, *s_tmpptr) == NULL ) {
         i_err = (int)*s_tmpptr;
         break;
      }
      s_tmpptr++;
   }
   if ( i_err ) {
      sprintf(s_tmp, "%c: Totem unknown or not valid for specified object type", (char)i_err);
      notify(player, s_tmp);
      i_err = 0;
      free_lbuf(s_tmp);
   } else {
      i_err = 1;
   }

   free_lbuf(s_tmp);
   return i_err;
}


void do_dolist (dbref player, dbref cause, int key, char *list, 
		char *command, char *cargs[], int ncargs)
{
   char	*tbuf, *curr, *objstring, *buff2, *buff3, *buff3ptr, delimiter = ' ', *tempstr, *tpr_buff, *tprp_buff, 
        *buff3tok, *pt, *savereg[MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST], *dbfr, *npt, *saveregname[MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST], *s_rollback;
   time_t i_now;
   int x, cntr, pid_val, i_localize, i_clearreg, i_nobreak, i_inline, i_storebreak, i_jump, i_rollback, i_chkinline, i_hook;

   pid_val = 0;
   i_storebreak = mudstate.breakst;

   if ( ((key & DOLIST_PID) && (key & DOLIST_NOTIFY)) ||
        (((key & DOLIST_CLEARREG) || (key & DOLIST_NOBREAK) || (key & DOLIST_LOCALIZE)) && !(key & DOLIST_INLINE)) ) {
      notify(player, "Illegal combination of switches.");
      return;
   }

   if (!list || *list == '\0') {
      notify (player, "That's terrific, but what should I do with the list?");
      return;
   }

   i_localize = i_clearreg = i_nobreak = i_inline = 0;
   if ( key & DOLIST_LOCALIZE ) {
      key &= ~DOLIST_LOCALIZE;
      i_localize = 1;
   }
   if ( key & DOLIST_CLEARREG ) {
      key &= ~DOLIST_CLEARREG;
      i_clearreg = 1;
   }
   if ( key & DOLIST_NOBREAK ) {
      key &= ~DOLIST_NOBREAK;
      i_nobreak = 1;
   }
   if ( key & DOLIST_INLINE ) {
      key &= ~DOLIST_INLINE;
      i_inline = 1;
   }
   curr = list;
   tempstr = NULL;

   if( key & DOLIST_DELIMIT ) {
      if(strlen(( tempstr = parse_to(&curr, ' ', EV_STRIP))) > 1) {
         notify (player, "The delimiter must be a single character!");
         return;
      }

      delimiter = *tempstr;
   }

   if (key & DOLIST_PID ) {
      tempstr = parse_to(&curr, ' ', EV_STRIP);
      pid_val = atoi(tempstr);
   }

   x = 0;
   cntr=1;
   tprp_buff = tpr_buff = alloc_lbuf("do_dolist");
   if ( i_clearreg || i_localize ) {
      for (x = 0; x < (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); x++) {
         savereg[x] = alloc_lbuf("ulocal_reg");
         saveregname[x] = alloc_sbuf("ulocal_regname");
      }
   }
   s_rollback = alloc_lbuf("s_rollback_dolist");
   while (curr && *curr) {
      if ((x % 25) == 0)
         cache_reset(0);
      x++;
      while (*curr==delimiter) 
         curr++;
      if (*curr) {
         objstring = parse_to(&curr, delimiter, EV_STRIP);
         tprp_buff = tpr_buff;
         buff2 = replace_string(NUMERIC_VAR, safe_tprintf(tpr_buff, &tprp_buff, "%d", cntr), command, 0);
         if ( i_inline ) {
            if ( (mudstate.dolistnest >= 0) && (mudstate.dolistnest < 50) ) {
               if ( !mudstate.dol_arr[mudstate.dolistnest] )
                  mudstate.dol_arr[mudstate.dolistnest] = alloc_lbuf("dolist_nesting");
               dbfr = mudstate.dol_arr[mudstate.dolistnest];
               safe_str(objstring, mudstate.dol_arr[mudstate.dolistnest], &dbfr);
               mudstate.dol_inumarr[mudstate.dolistnest] = cntr;
            }
            mudstate.dolistnest++;
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); x++) {
                  *savereg[x] = '\0';
                  *saveregname[x] = '\0';
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
            if ( mudstate.chkcpu_inline ) {
               i_now = mudstate.now;
            } else {
               i_now = time(NULL);
            }
            buff3 = replace_string(BOUND_VAR, objstring, buff2, 0);
            buff3tok = buff3;
            strcpy(s_rollback, mudstate.rollback);
            i_jump = mudstate.jumpst;
            i_rollback = mudstate.rollbackcnt;
            mudstate.jumpst = mudstate.rollbackcnt = 0;
            if ( buff3tok ) {
               strcpy(mudstate.rollback, buff3tok);
            }
            i_chkinline = mudstate.chkcpu_inline;
            sprintf(mudstate.chkcpu_inlinestr, "%s", (char *)"@dolist/inline");
            mudstate.chkcpu_inline = 1;
            i_hook = mudstate.no_hook;
            while ( !mudstate.breakdolist && !mudstate.chkcpu_toggle && buff3tok && !mudstate.breakst ) { 
               buff3ptr = parse_to(&buff3tok, ';', 0);
               if ( buff3ptr && *buff3ptr ) {
                  if ( i_hook ) {
                     process_command(player, cause, 0, buff3ptr, cargs, ncargs, InProgram(player), 1, mudstate.no_space_compress);
                  } else {
                     process_command(player, cause, 0, buff3ptr, cargs, ncargs, InProgram(player), mudstate.no_hook, mudstate.no_space_compress);
                  }
               }
               if ( mudstate.chkcpu_toggle || (time(NULL) > (i_now + 3)) ) {
                   if ( !mudstate.breakdolist ) {
                      notify(player, unsafe_tprintf("@dolist/inline:  Aborted for high utilization [nest level %d].", mudstate.dolistnest));
                   }
                   i_nobreak=0;
                   mudstate.breakdolist = 1;
                   break;
               }
            }
            mudstate.no_hook = i_hook;
            mudstate.chkcpu_inline = i_chkinline;
            mudstate.jumpst = i_jump;
            mudstate.rollbackcnt = i_rollback;
            strcpy(mudstate.rollback, s_rollback);
            if ( i_clearreg || i_localize ) {
               for (x = 0; x < (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); x++) {
                  pt = mudstate.global_regs[x];
                  npt = mudstate.global_regsname[x];
                  safe_str(savereg[x],mudstate.global_regs[x],&pt);
                  safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                  *savereg[x] = '\0';
                  *saveregname[x] = '\0';
               }
            }
            free_lbuf(buff3);
            if ( i_nobreak ) {
               mudstate.breakst = i_storebreak;
            }
            mudstate.dolistnest--;
            if ( mudstate.dolistnest >= 0 ) {
               free_lbuf(mudstate.dol_arr[mudstate.dolistnest]);
               mudstate.dol_arr[mudstate.dolistnest] = NULL;
            }
         } else {
            bind_and_queue (player, cause, buff2, objstring, cargs, ncargs);
         }
         free_lbuf(buff2);
      }
      cntr++;
   }
   free_lbuf(s_rollback);
   if ( i_inline ) {
      if ( desc_in_use != NULL ) {
         mudstate.breakst = i_storebreak;
      }
   }
   if ( i_clearreg || i_localize ) {
      for (x = 0; x < (MAX_GLOBAL_REGS + MAX_GLOBAL_BOOST); x++) {
         free_lbuf(savereg[x]);
         free_sbuf(saveregname[x]);
      }
   }
   free_lbuf(tpr_buff);
   if (key & (DOLIST_NOTIFY|DOLIST_PID)) {
      tbuf = alloc_lbuf("dolist.notify_cmd");
      if ( key & DOLIST_PID ) {
         strcpy(tbuf, unsafe_tprintf("@notify/pid me=%d", pid_val));
      } else {
         strcpy(tbuf, (char *) "@notify me");
      }
      wait_que(player, cause, 0, NOTHING, tbuf, cargs, ncargs,
               mudstate.global_regs, mudstate.global_regsname);
      free_lbuf(tbuf);
   }
   if ( cntr == 1 )
      notify (player, "That's terrific, but what should I do with the list?");
/*
   if ( (mudstate.dolistnest == 0) && mudstate.breakdolist ) {
      mudstate.chkcpu_toggle = 1;
   }
*/
}

/* Regular @find command */

void do_find (dbref player, dbref cause, int key, char *name)
{
dbref	i, low_bound, high_bound;
char	*buff;

	if (!payfor (player, mudconf.searchcost)) {
		notify_quiet(player,
			unsafe_tprintf("You don't have enough %s.",
				mudconf.many_coins));
		return;
	}

	parse_range(&name, &low_bound, &high_bound);
	for (i=low_bound; i<=high_bound; i++) {
		if ((i % 100) == 0)
			cache_reset(0);
		if ((Typeof (i) != TYPE_EXIT) &&
		    controls (player, i) &&
		    (!*name || string_match (Name (i), name))) {
			buff = unparse_object (player, i, 0);
			notify (player, buff);
			free_lbuf(buff);
		}
	}
	notify (player, "***End of List***");
}

/* ---------------------------------------------------------------------------
 * get_stats, do_stats: Get counts of items in the db.
 */

int get_stats (dbref player, dbref who, STATS *info)
{
dbref	i;

	info->s_total = 0;
	info->s_rooms = 0;
	info->s_exits = 0;
	info->s_things = 0;
	info->s_players = 0;
	info->s_zone = 0;
	info->s_garbage = 0;

	/* Do we have permission? */

	if (Good_obj(who) && !Controls(player, who) &&
	    !HasPriv(player,who,POWER_STAT_ANY,POWER4,NOTHING)) {
		notify(player, "Permission denied.");
		return 0;
	}

	/* Can we afford it? */

	if (!payfor (player, mudconf.searchcost)) {
		notify (player, unsafe_tprintf("You don't have enough %s.",
			mudconf.many_coins));
		return 0;
	}

	DO_WHOLE_DB(i) {
		if ((who == NOTHING) || (who == Owner (i))) {
			info->s_total++;
			if ((Recover(i) && !Byeroom(i)) || Going(i)) {
				info->s_garbage++;
				continue;
			}
			switch (Typeof (i)) {
			case TYPE_ROOM:		info->s_rooms++;	break;
			case TYPE_EXIT:		info->s_exits++;	break;
			case TYPE_THING:	info->s_things++;	break;
			case TYPE_PLAYER:	info->s_players++;	break;
			case TYPE_ZONE:		info->s_zone++;		break;
			default:		info->s_garbage++;
			}
		}
	}
	return 1;
}

void do_stats (dbref player, dbref cause, int key, char *name)
/* reworked by R'nice */
{
   dbref owner;
   char *t_buff;
   STATS statinfo;
   dbref i;
   int i_count;

   switch (key) {
      case STAT_ALL:
         owner = NOTHING;
         break;
      case STAT_ME:
         owner = Owner(player);
         break;
      case STAT_FREE:
         if ( Wizard(player) || HasPriv(player, NOTHING, POWER_USE_FREELIST, POWER5, NOTHING) ) {
            i = mudstate.freelist;
            if ( i != NOTHING ) {
               notify(player, "Free dbref list:");
            } else {
               notify(player, "The free list is currently empty.");
            }
            t_buff = alloc_sbuf("do_stats");
            i_count = 0;
            while ( i != NOTHING ) {
               sprintf(t_buff, "#%d", i);
               if ( !*name || (*name && quick_wild(name, t_buff)) ) {
                  sprintf(t_buff, "Dbref: #%d", i);
                  notify(player, t_buff);
               }
               i = Link(i);
               i_count++;
            }
            if ( i_count > 0 ) {
               /* Keep this under 32 chars for SBUF size */
               sprintf(t_buff, "Total free objs: %d", i_count);
               notify(player, t_buff);
            }
            free_sbuf(t_buff);
         } 
         if ( mudstate.freelist >= 0 ) {
            notify(player, unsafe_tprintf("The next free dbref is #%d", mudstate.freelist));
         } else {
            notify(player, unsafe_tprintf("The next free dbref is #%d (new object)", statinfo.s_total));
         }
         return;
      case STAT_PLAYER:
         if (!(name && *name)) {
            if(!get_stats(player,NOTHING,&statinfo)) {
               return;
            }
            notify(player, "The universe contains:");
            notify(player, unsafe_tprintf("  %5d valid objects", statinfo.s_total - statinfo.s_garbage));
            notify(player, unsafe_tprintf("  %5d garbage objects", statinfo.s_garbage));
            notify(player, unsafe_tprintf("  %5d total objects", statinfo.s_total));
            if (mudconf.max_size > 0) {
               notify(player, "Current database cap is:");
               notify(player, unsafe_tprintf("  %5d objects", mudconf.max_size));
            }
            if ( mudstate.freelist >= 0 ) {
               notify(player, unsafe_tprintf("The next free dbref is #%d", mudstate.freelist));
            } else {
               notify(player, unsafe_tprintf("The next free dbref is #%d (new object)", statinfo.s_total));
            }
            return;
         }
         owner = lookup_player(player, name, 1);
         if (owner == NOTHING) {
            notify(player, "Not found.");
            return;
         }
         break;
      default:
         notify(player, "Illegal combination of switches.");
         return;
   }

   if (!get_stats(player, owner, &statinfo)) {
      return;
   }
   notify (player, unsafe_tprintf ("%d objects = %d rooms, %d exits, %d things, %d players, %d zones. (%d garbage)",
                      statinfo.s_total, statinfo.s_rooms, statinfo.s_exits,
                      statinfo.s_things, statinfo.s_players, statinfo.s_zone,
                      statinfo.s_garbage));

#ifdef TEST_MALLOC
   if (Wizard (player)) {
      notify (player, unsafe_tprintf ("Malloc count = %d.", malloc_count));
   }
#endif /* TEST_MALLOC */
#ifdef MSTATS
   if (Wizard(player)) {
      struct mstats_value mval;
      int i;
      extern unsigned int malloc_sbrk_used, malloc_sbrk_unused;
      extern int nmal, nfre;
      extern struct mstats_value malloc_stats();
      extern int malloc_mem_used(), malloc_mem_free();

      notify(player, unsafe_tprintf("Sbrk Unused: %d -- Sbrk Used: %d", malloc_sbrk_unused, malloc_sbrk_used));
      notify(player, unsafe_tprintf("Raw malloc cnt: %d -- Raw free cnt: %d",
			   nmal, nfre));
      for (i = 0; i < 15; i++) {
         mval = malloc_stats(i);
         notify(player, unsafe_tprintf("Blocksz: %d -- Free: %d -- Used: %d", mval.blocksize, mval.nfree, mval.nused));
      }
      notify(player, unsafe_tprintf("Mem used: %d -- Mem free: %d", malloc_mem_used(), malloc_mem_free()));
   }
#endif	/* MSTATS */
}

int chown_all (dbref from_player, dbref to_player, int key)
{
   int i, count, quota_out[4], quota_in[4];
   int i_room, i_exit, i_player, i_thing;

   if (Typeof(from_player) != TYPE_PLAYER)
      from_player = Owner(from_player);
   if (Typeof(to_player) != TYPE_PLAYER)
      to_player = Owner(to_player);
   count = 0;
   for (i = 0; i < 4; i++)
      quota_out[i] = quota_in[i] = 0;
   i_room   = (key < 2) || (key & CHOWN_ROOM);
   i_exit   = (key < 2) || (key & CHOWN_EXIT);
   i_player = (key < 2) || (key & CHOWN_PLAYER);
   i_thing  = (key < 2) || (key & CHOWN_THING);

   DO_WHOLE_DB(i) {
      if (Recover(i))
         continue;
      if ((Owner(i) == from_player) && (Owner(i) != i)) {
         switch (Typeof(i)) {
            case TYPE_PLAYER:
               if ( !i_player )
                  continue;
               if ( (key & 1) && Robot(i) && (Owner(i) != i) )
                  s_Owner (i, to_player);
               else
                  s_Owner (i, i);
               quota_out[TYPE_PLAYER] += mudconf.player_quota;
               break;
            case TYPE_THING:
               if ( !i_thing )
                  continue;
               s_Owner (i, to_player);
               quota_out[TYPE_THING] += mudconf.thing_quota;
               quota_in[TYPE_THING] += mudconf.thing_quota;
               break;
            case TYPE_ROOM:
               if ( !i_room )
                  continue;
               s_Owner (i, to_player);
               quota_out[TYPE_ROOM] += mudconf.room_quota;
               quota_in[TYPE_ROOM] += mudconf.room_quota;
               break;
            case TYPE_EXIT:
               if ( !i_exit )
                  continue;
               s_Owner (i, to_player);
               quota_out[TYPE_EXIT] += mudconf.exit_quota;
               quota_in[TYPE_EXIT] += mudconf.exit_quota;
               break;
            default:
               s_Owner (i, to_player);
         }
         if ( !(key & 1) ) {
/*          s_Flags(i, (Flags(i) & ~(CHOWN_OK|INHERIT)) | HALT); */
            s_Flags(i, (Flags(i) & ~(CHOWN_OK|INHERIT|WIZARD)) | HALT);
            s_Flags2(i, (Flags2(i) & ~(IMMORTAL|ADMIN|BUILDER|GUILDMASTER)));
         }
         count++;
      }
   }
   for (i = 0; i < 4; i++) {
      add_quota(from_player, quota_out[i], i);
      pay_quota_force(to_player, quota_in[i], i);
   }
   return count;
}

void do_chownall (dbref player, dbref cause, int key, char *from, char *to)
{
int	count, bLeaveFlags;
dbref	victim, recipient;

	init_match (player, from, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
        match_player_absolute();
	if ((victim = noisy_match_result ()) == NOTHING)
		return;

	if ((to != NULL) && *to) {
		init_match (player, to, TYPE_PLAYER);
		match_neighbor ();
		match_absolute ();
		match_player ();
		match_player_absolute ();
		if ((recipient = noisy_match_result ()) == NOTHING)
			return;
	} else {
		recipient = player;
	}

	if ((DePriv(player,victim,DP_CHOWN_ME,POWER7,NOTHING) && (player == recipient)) ||
	    (DePriv(player,victim,DP_CHOWN_OTHER,POWER7,NOTHING) && (player != victim)) ||
	    (DePriv(player,recipient,DP_CHOWN_OTHER,POWER7,NOTHING) && (player != recipient))) {
		notify(player, "Permission denied.");
		return;
	}
        bLeaveFlags = (key & CHOWN_PRESERVE) && Wizard(player);
        bLeaveFlags = bLeaveFlags | (key & (CHOWN_ROOM|CHOWN_EXIT|CHOWN_THING|CHOWN_PLAYER));
	count = chown_all(victim, recipient, bLeaveFlags);
	if (!Quiet(player)) {
		notify(player, unsafe_tprintf("%d objects @chowned.", count));
	}
}

#define ANY_OWNER -2

void er_mark_disabled (dbref player)
{
	notify(player,
		"The mark commands are not allowed while DB cleaning is enabled.");
	notify(player,
		"Use the '@disable cleaning' command to disable automatic cleaning.");
	notify(player,
		"Remember to '@unmark_all' before re-enabling automatic cleaning.");
}


/* ---------------------------------------------------------------------------
 * do_search: Walk the db reporting various things (or setting/clearing
 * mark bits)
 */

int search_setup (dbref player, char *searchfor, SEARCH *parm, int mc, int i_zone)
{
char	*pname, *searchtype, *t;
int	err, i;

	/* Crack arg into <pname> <type>=<targ>,<low>,<high> */

	pname = parse_to(&searchfor, '=', EV_STRIP_TS);
	if (!pname || !*pname) {
		pname = (char *) "me";
	} else for (t=pname; *t; t++) {
		if (isupper((int)*t))
			*t = tolower(*t);
	}

	if (searchfor && *searchfor) {
		searchtype = (char *)rindex(pname, ' ');
		if (searchtype) {
			*searchtype++ = '\0';
		} else {
			searchtype = pname;
			pname = (char *) "";
		}
	} else {
		searchtype = (char *) "";
	}

	/* If the player name is quoted, strip the quotes */

	if (*pname == '\"') {
		err = strlen(pname)-1;
		if (pname[err] == '"') {
			pname[err] = '\0';
			pname++;
		}
	}

	/* Strip any range arguments */

	parse_range(&searchfor, &parm->low_bound, &parm->high_bound);

	/* set limits on who we search */

	parm->s_owner = Owner(player);
	parm->s_wizard = Builder(player);
	parm->s_rst_owner = NOTHING;
	if (!*pname) {
		parm->s_rst_owner = parm->s_wizard ? ANY_OWNER : player;
	} else if (pname[0] == '#') {
		parm->s_rst_owner = atoi (&pname[1]);
		if (!Good_obj(parm->s_rst_owner)) {
			parm->s_rst_owner = NOTHING;
		} else if (i_zone && !ZoneMaster(parm->s_rst_owner)) {
			parm->s_rst_owner = NOTHING;
		} else if (!i_zone && Typeof(parm->s_rst_owner) != TYPE_PLAYER) {
			parm->s_rst_owner = NOTHING;
		}

	} else if (strcmp (pname, "me") == 0) {
		parm->s_rst_owner = player;
	} else {
		if ( i_zone ) {
			init_match (player, pname, TYPE_PLAYER);
			match_neighbor();
			match_absolute();
			match_player();
        		match_player_absolute();
			parm->s_rst_owner = noisy_match_result();
		} else {
			parm->s_rst_owner = lookup_player (player, pname, 1);
		}
	}
	if ( i_zone && (!Good_chk(parm->s_rst_owner) || !ZoneMaster(parm->s_rst_owner)) ) {
		notify (player, unsafe_tprintf ("%s: not a valid zonemaster", pname));
		return 0;
	}

	if (parm->s_rst_owner == NOTHING) {
		notify (player, unsafe_tprintf ("%s: No such player", pname));
		return 0;
	}

	/* set limits on what we search for */

	err = 0;
        for ( i = 0; i < TOTEM_SLOTS; i++ ) {
           parm->i_totems[i] = 0;
        }
	parm->s_rst_name = NULL;
	parm->s_rst_eval = NULL;
	parm->s_rst_type = NOTYPE;
	parm->s_parent = NOTHING;
	parm->s_fset.word1 = 0;
	parm->s_fset.word2 = 0;
	parm->s_fset.word3 = 0;
	parm->s_fset.word4 = 0;

	switch (searchtype[0]) {
	case '\0':		/* the no class requested class  :)  */
		break;
	case 'e':
		if (string_prefix ("exits", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_EXIT;
		} else if (string_prefix ("evaluate", searchtype)) {
			parm->s_rst_eval = searchfor;
		} else if (string_prefix ("eplayer", searchtype)) {
			parm->s_rst_type = TYPE_PLAYER;
			parm->s_rst_eval = searchfor;
		} else if (string_prefix ("eroom", searchtype)) {
			parm->s_rst_type = TYPE_ROOM;
			parm->s_rst_eval = searchfor;
		} else if (string_prefix ("eobject", searchtype)) {
			parm->s_rst_type = TYPE_THING;
			parm->s_rst_eval = searchfor;
		} else if (string_prefix ("ething", searchtype)) {
			parm->s_rst_type = TYPE_THING;
			parm->s_rst_eval = searchfor;
		} else if (string_prefix ("eexit", searchtype)) {
			parm->s_rst_type = TYPE_EXIT;
			parm->s_rst_eval = searchfor;
		} else {
			err = 1;
		}
		break;
	case 'f':
		if (string_prefix ("flags", searchtype)) {

			/* convert_flags ignores previous values of flag_mask
			 * and s_rst_type while setting them */

			if (!convert_flags (player, searchfor, &parm->s_fset,
			    &parm->s_rst_type,0))
				return 0;
		}
		else if (string_prefix("flags2", searchtype)) {
			if (!convert_flags (player, searchfor, &parm->s_fset,
			    &parm->s_rst_type,1))
				return 0;

		} else {
			err = 1;
		}
		break;
	case 'n':
		if (string_prefix ("name", searchtype)) {
			parm->s_rst_name = searchfor;
		} else {
			err = 1;
		}
		break;
	case 'o':
		if (string_prefix ("objects", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_THING;
		} else {
			err = 1;
		}
		break;
	case 'p':
		if (string_prefix ("players", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_PLAYER;
			if (!*pname)
				parm->s_rst_owner = ANY_OWNER;
		} else if (string_prefix ("parent", searchtype)) {
			parm->s_parent = match_controlled(player, searchfor);
			if (!Good_obj(parm->s_parent))
				return 0;
			if (!*pname)
				parm->s_rst_owner = ANY_OWNER;
		} else {
			err = 1;

		}
		break;
	case 'r':
		if (string_prefix ("rooms", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_ROOM;
		} else {
			err = 1;
		}
		break;
	case 't':
                if ( string_prefix("totemlist", searchtype)) { 
                        if ( !convert_totemslist(player, searchfor, &parm) )
                           return 0;
		} else if (string_prefix ("totems", searchtype)) {
			if ( !convert_totems(player, searchfor, &parm, 0) )
                           return 0;
		} else if (string_prefix ("totems2", searchtype)) {
			if ( !convert_totems(player, searchfor, &parm, 1) )
                           return 0;
		} else if (string_prefix ("totems3", searchtype)) {
			if ( !convert_totems(player, searchfor, &parm, 2) )
                           return 0;
		} else if (string_prefix ("type", searchtype)) {
			if (searchfor[0] == '\0')
				break;
			if (string_prefix ("rooms", searchfor))
				parm->s_rst_type = TYPE_ROOM;
			else if (string_prefix ("exits", searchfor))
				parm->s_rst_type = TYPE_EXIT;
			else if (string_prefix ("objects", searchfor))
				parm->s_rst_type = TYPE_THING;
			else if (string_prefix ("things", searchfor))
				parm->s_rst_type = TYPE_THING;
			else if (string_prefix ("players", searchfor)) {
				parm->s_rst_type = TYPE_PLAYER;
				if (!*pname)
					parm->s_rst_owner = ANY_OWNER;
			} else {
				notify(player,
					unsafe_tprintf("%s: unknown type",
						searchfor));
				return 0;
			}
		} else if (string_prefix ("things", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_THING;
		} else {
			err = 1;
		}
		break;
	default:
		err = 1;
	}

	if (err) {
		notify (player, unsafe_tprintf ("%s: unknown class", searchtype));
		return 0;
	}

	/* Make sure player is authorized to do the search */

	if (!parm->s_wizard &&
		 (parm->s_rst_type != TYPE_PLAYER) &&
		 (parm->s_rst_owner != player) &&
		 (parm->s_rst_owner != ANY_OWNER)) {
		notify (player, "You need a search warrant to do that!");
		return 0;
	}

	/* make sure player has money to do the search */

	if (mc)
	  mudstate.evalnum--;
	if (!payfor(player, mudconf.searchcost)) {
		notify(player,
			unsafe_tprintf("You don't have enough %s to search. (You need %d)",
				mudconf.many_coins, mudconf.searchcost));
		if (mc)
		  mudstate.evalnum++;
		return 0;
	}
	if (mc)
	  mudstate.evalnum++;
	return 1;
}

void do_search_for_players(dbref player, dbref cause, int key, char *arg)
{
	dbref   thing, aowner;
	char    *buff, *result;
	int     save_invk_ctr;
	int     aflags;

	save_invk_ctr = mudstate.func_invk_ctr;
	DO_WHOLE_DB(thing) {

		if ((thing % 100) == 0) {
			cache_reset(0);
		}
		mudstate.func_invk_ctr = save_invk_ctr;
		switch (Typeof(thing)) {
			case TYPE_PLAYER:
				if (Controls(player,thing)) {
					result = atr_get(thing, A_LAST, &aowner, &aflags);
					buff = alloc_lbuf("fcache_load");
					sprintf(buff,"%s(#%d) [%s]:%s",Name(thing),
												thing,result,Guild(thing));
					notify(player, buff);
					free_lbuf(buff);
					free_lbuf(result);
				}
				break;
			default:
				break;
		}

	}
	mudstate.func_invk_ctr = save_invk_ctr;
}

void search_perform (dbref player, dbref cause, SEARCH *parm, FILE** master, int i_zone)
{
   FLAG thing1flags, thing2flags, thing3flags, thing4flags;
   dbref thing;
   char *buff, *buff2, *result;
   int save_invk_ctr, i, i_break;
   ZLISTNODE *z_ptr;

   buff = alloc_sbuf("search_perform.num");
   save_invk_ctr = mudstate.func_invk_ctr;

   if ((!(parm->s_fset.word1 & IMMORTAL) && !(parm->s_fset.word2 & SCLOAK)) || Immortal(player)) {
      if ( i_zone ) {
         /* This should already be caught, but we love paranoia */
         if ( !Good_chk(parm->s_rst_owner) )
            return;

         for ( z_ptr = db[parm->s_rst_owner].zonelist; z_ptr; z_ptr = z_ptr->next ) {
            thing = z_ptr->object;
            if ( !Good_chk(thing) )
               break;

            if ((thing % 100) == 0) {
               cache_reset(0);
            }
            mudstate.func_invk_ctr = save_invk_ctr;

            /* Check for matching type */
            if ((parm->s_rst_type != NOTYPE) && (parm->s_rst_type != Typeof(thing)))
               continue;

            /* Toss out destroyed things */
            thing1flags = Flags(thing);
            if ((Typeof(thing) == TYPE_THING) && (thing1flags & GOING))
               continue;

            thing2flags = Flags2(thing);
            if ((thing2flags & RECOVER) && (!Immortal(player)))
               continue;
   
            if (Cloak(thing) && !Wizard(player)) 
               continue;
            if (Cloak(thing) && SCloak(thing) && !Immortal(player)) 
               continue;
            if (Unfindable(thing) && !Examinable(player,thing)) 
               continue;
   
            /* Check for matching parent */
            if ((parm->s_parent != NOTHING) && (parm->s_parent != Parent(thing)))
               continue;
   
            /* Check for matching flags */
            thing3flags = Flags3(thing);
            thing4flags = Flags4(thing);
            if ((thing1flags & parm->s_fset.word1) != parm->s_fset.word1)
               continue;
            if ((thing2flags & parm->s_fset.word2) != parm->s_fset.word2)
               continue;
            if ((thing3flags & parm->s_fset.word3) != parm->s_fset.word3)
               continue;
            if ((thing4flags & parm->s_fset.word4) != parm->s_fset.word4)
               continue;
            if ( Good_obj(thing) ) {
               i_break = 0;
               for ( i = 0; i < TOTEM_SLOTS; i++ ) {
                  if ( (dbtotem[thing].flags[i] & parm->i_totems[i]) != parm->i_totems[i]) {
                     i_break = 1;
                     break;
                  }
               }
               if ( i_break ) {
                  continue;
               }
            }
   
            /* Check for matching name */
            if (parm->s_rst_name != NULL) {
               if (!string_prefix(Name(thing), parm->s_rst_name))
                  continue;
            }
   
            /* Check for successful evaluation */
            if (parm->s_rst_eval != NULL) {
               sprintf(buff, "#%d", thing);
               buff2 = replace_string(BOUND_VAR, buff, parm->s_rst_eval, 0);
               result = exec(player, cause, cause,
                             EV_FCHECK|EV_EVAL|EV_NOTRACE, buff2,
                             (char **)NULL, 0, (char **)NULL, 0);
               free_lbuf(buff2);
               if (!*result || !xlate(result)) {
                  free_lbuf(result);
                  continue;
               }
               free_lbuf(result);
            }
   
            /* It passed everything.  Amazing. */

            file_olist_add(master,thing);
         }
      } else {
         for (thing=parm->low_bound; thing<=parm->high_bound; thing++) {
            if ((thing % 100) == 0) {
               cache_reset(0);
            }
            mudstate.func_invk_ctr = save_invk_ctr;

            /* Check for matching type */
            if ((parm->s_rst_type != NOTYPE) && (parm->s_rst_type != Typeof(thing)))
               continue;

            /* Check for matching owner */
            if ((parm->s_rst_owner != ANY_OWNER) && (parm->s_rst_owner != Owner(thing)))
               continue;

            /* Toss out destroyed things */
            thing1flags = Flags(thing);
            if ((Typeof(thing) == TYPE_THING) && (thing1flags & GOING))
               continue;

            thing2flags = Flags2(thing);
            if ((thing2flags & RECOVER) && (!Immortal(player)))
               continue;
   
            if (Cloak(thing) && !Wizard(player)) 
               continue;
            if (Cloak(thing) && SCloak(thing) && !Immortal(player)) 
               continue;
            if (Unfindable(thing) && !Examinable(player,thing)) 
               continue;
   
            /* Check for matching parent */
            if ((parm->s_parent != NOTHING) && (parm->s_parent != Parent(thing)))
               continue;
   
            /* Check for matching flags */
            thing3flags = Flags3(thing);
            thing4flags = Flags4(thing);
            if ((thing1flags & parm->s_fset.word1) != parm->s_fset.word1)
               continue;
            if ((thing2flags & parm->s_fset.word2) != parm->s_fset.word2)
               continue;
            if ((thing3flags & parm->s_fset.word3) != parm->s_fset.word3)
               continue;
            if ((thing4flags & parm->s_fset.word4) != parm->s_fset.word4)
               continue;
            if ( Good_obj(thing) ) {
               i_break = 0;
               for ( i = 0; i < TOTEM_SLOTS; i++ ) {
                  if ( (dbtotem[thing].flags[i] & parm->i_totems[i]) != parm->i_totems[i]) {
                     i_break = 1;
                     break;
                  }
               }
               if ( i_break ) {
                  continue;
               }
            }
   
            /* Check for matching name */
            if (parm->s_rst_name != NULL) {
               if (!string_prefix(Name(thing), parm->s_rst_name))
                  continue;
            }
   
            /* Check for successful evaluation */
            if (parm->s_rst_eval != NULL) {
               sprintf(buff, "#%d", thing);
               buff2 = replace_string(BOUND_VAR, buff, parm->s_rst_eval, 0);
               result = exec(player, cause, cause,
                             EV_FCHECK|EV_EVAL|EV_NOTRACE, buff2,
                             (char **)NULL, 0, (char **)NULL, 0);
               free_lbuf(buff2);
               if (!*result || !xlate(result)) {
                  free_lbuf(result);
                  continue;
               }
               free_lbuf(result);
            }
   
            /* It passed everything.  Amazing. */
            file_olist_add(master,thing);
         }
      }
   }
   free_sbuf(buff);
   mudstate.func_invk_ctr = save_invk_ctr;
}

static void search_mark (dbref player, int key, FILE** master)
{
   dbref thing;
   int nchanged, is_marked;

   nchanged = 0;
   for (thing=file_olist_first(master); thing!=NOTHING; thing=file_olist_next(master)) {
      is_marked = Marked(thing);

      /* Don't bother checking if marking and already marked
       * (or if unmarking and not marked) 
       */

      if (((key == SRCH_MARK) && is_marked) || ((key == SRCH_UNMARK) && !is_marked)) {
         continue;
      }
      /* Toggle the mark bit and update the counters */
      if (key == SRCH_MARK) {
         Mark(thing);
         nchanged++;
      } else {
         Unmark(thing);
         nchanged++;
      }
   }
   notify(player, unsafe_tprintf("%d objects %smarked",
                                 nchanged, ((key==SRCH_MARK) ? "" : "un")));
   return;
}
					
void do_search (dbref player, dbref cause, int key, char *arg)
{
int	flag, destitute, evc, playercnt=0, roomcnt=0, exitcnt=0, objectcnt=0, nogarb=0, i_ansi=0, i_zone=0;
char	*buff, *outbuf, *bp;
dbref	thing, from, to;
SEARCH	searchparm;
FILE* master;

        if ( mudconf.switch_search != 0 ) {
           nogarb = 1;
        }
        if ( key & SEARCH_ANSI ) {
           i_ansi = 1;
           key &= ~SEARCH_ANSI;
        }
        if ( key & SEARCH_ZONE ) {
           i_zone = 1;
           key &= ~SEARCH_ZONE;
        }
	if ( key & SEARCH_NOGARBAGE ) {
           if ( mudconf.switch_search == 0 )
	      nogarb = 1;
           else
	      nogarb = 0;
	   key = key & ~SEARCH_NOGARBAGE;
	}

	if ((key != SRCH_SEARCH) && (mudconf.control_flags & CF_DBCHECK)) {
		er_mark_disabled(player);
		return;
	}

	evc = 0;
	if (mudstate.evalnum < MAXEVLEVEL) {
          if (DePriv(player,player,DP_SEARCH_ANY,POWER7,NOTHING)) {
            evc = DePriv(player,NOTHING,DP_SEARCH_ANY,POWER7,POWER_LEVEL_NA);
            if (!DPShift(player))
              mudstate.evalstate[mudstate.evalnum] = evc - 1;
	    else
              mudstate.evalstate[mudstate.evalnum] = evc;
            mudstate.evaldb[mudstate.evalnum++] = player;
          } 
          else if (HasPriv(player,player,POWER_SEARCH_ANY,POWER4,NOTHING)) {
            evc = HasPriv(player,NOTHING,POWER_SEARCH_ANY,POWER4,POWER_LEVEL_NA);
            mudstate.evalstate[mudstate.evalnum] = evc;
            mudstate.evaldb[mudstate.evalnum++] = player;
          }
	}
        file_olist_init(&master,"dosearch.tmp");
	if (evc) {
	  if (!search_setup (player, arg, &searchparm, 1, i_zone)) {
		mudstate.evalnum--;
		return;
	  }
	}
	else {
	  if (!search_setup (player, arg, &searchparm, 0, i_zone))
		return;
	}
	search_perform (player, cause, &searchparm, &master, i_zone);
	destitute = 1;

	/* If we are doing a @mark command, handle that here. */

	if (key != SRCH_SEARCH) {
		search_mark(player, key, &master);
		if (evc)
		  mudstate.evalnum--;
		return;
	}

	outbuf = alloc_lbuf("do_search.outbuf");

	/* room search */
	if (searchparm.s_rst_type == TYPE_ROOM ||
	    searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for (thing=file_olist_first(&master); thing!=NOTHING; thing=file_olist_next(&master)) {
			if (Typeof(thing) != TYPE_ROOM) continue;
			if ( nogarb && (Going(thing) || Recover(thing)) ) continue;
			if (flag) {
				flag = 0;
				destitute = 0;
				notify (player, "\r\nROOMS:");
			}
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, thing, 0, 1);
                        } else {
			   buff = unparse_object (player, thing, 0);
                        }
			notify (player, buff);
			free_lbuf(buff);
                        roomcnt++;
		}
	}

	/* exit search */
	if (searchparm.s_rst_type == TYPE_EXIT ||
	    searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for (thing=file_olist_first(&master); thing!=NOTHING; thing=file_olist_next(&master)) {
			if (Typeof(thing) != TYPE_EXIT) continue;
			if ( nogarb && (Going(thing) || Recover(thing)) ) continue;
			if (flag) {
				flag = 0;
				destitute = 0;
				notify (player, "\r\nEXITS:");
			}
			from = Exits (thing);
			to = Location (thing);

			bp = outbuf;
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, thing, 0, 1);
                        } else {
			   buff = unparse_object (player, thing, 0);
                        }
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *)" [from ", outbuf, &bp);
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, from, 0, 1);
                        } else {
			   buff = unparse_object (player, from, 0);
                        }
			safe_str(((from==NOTHING) ? "NOWHERE" : buff),
				outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *)" to ", outbuf, &bp);
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, to, 0, 1);
                        } else {
			   buff = unparse_object (player, to, 0);
                        }
			safe_str(((to==NOTHING) ? "NOWHERE" : buff),
				outbuf, &bp);
			free_lbuf(buff);

			safe_chr(']', outbuf, &bp);
			// *bp = '\0';
			notify (player, outbuf);
                        exitcnt++;
		}
	}

	/* object search */
	if (searchparm.s_rst_type == TYPE_THING ||
	    searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for (thing=file_olist_first(&master); thing!=NOTHING; thing=file_olist_next(&master)) {
			if (Typeof(thing) != TYPE_THING) continue;
			if ( nogarb && (Going(thing) || Recover(thing)) ) continue;
			if (flag) {
				flag = 0;
				destitute = 0;
				notify (player, "\r\nOBJECTS:");
			}

			bp = outbuf;
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, thing, 0, 1);
                        } else {
			   buff = unparse_object (player, thing, 0);
                        }
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *)" [owner: ", outbuf, &bp);
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, Owner (thing), 0, 1);
                        } else {
			   buff = unparse_object (player, Owner (thing), 0);
                        }
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_chr(']', outbuf, &bp);
			// *bp = '\0';
			notify (player, outbuf);
                        objectcnt++;
		}
	}

	/* player search */
	if (searchparm.s_rst_type == TYPE_PLAYER ||
	    searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for (thing=file_olist_first(&master); thing!=NOTHING; thing=file_olist_next(&master)) {
			if (Typeof(thing) != TYPE_PLAYER) continue;
			if ( nogarb && (Going(thing) || Recover(thing)) ) continue;
			if (flag) {
				flag = 0;
				destitute = 0;
				notify (player, "\r\nPLAYERS:");
			}
			bp = outbuf;
                        if ( i_ansi ) {
			   buff = unparse_object_ansi(player, thing, 0, 1);
                        } else {
			   buff = unparse_object (player, thing, 0);
                        }
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);
			if (searchparm.s_wizard) {
			  if (Immortal(cause) || (Wizard(cause) && !Immortal(thing))) {
				safe_str((char *)" [location: ",
					outbuf, &bp);
                                if ( i_ansi ) {
				   buff = unparse_object_ansi(player, Location (thing), 0, 1);
                                } else {
				   buff = unparse_object (player, Location (thing), 0);
                                }
				safe_str(buff, outbuf, &bp);
				free_lbuf(buff);
				safe_chr(']', outbuf, &bp);
			  }
			}
			// *bp = '\0';
			notify (player, outbuf);
                        playercnt++;
		}
	}

	/* if nothing found matching search criteria */

	if (destitute) {
		notify (player, "Nothing found.");
	} else {
                notify(player, unsafe_tprintf("\r\nTotals Found:  Rooms: %d  Exits: %d  Objects: %d  Players: %d",
                       roomcnt, exitcnt, objectcnt, playercnt));
        }
	free_lbuf(outbuf);
	file_olist_cleanup(&master);
	if (evc)
	  mudstate.evalnum--;
}

/* ---------------------------------------------------------------------------
 * do_markall: set or clear the mark bits of all objects in the db.
 */

void do_markall (dbref player, dbref cause, int key)
{
int	i;

	if (mudconf.control_flags & CF_DBCHECK) {
		er_mark_disabled(player);
		return;
	}
	if (key == MARK_SET)
		Mark_all(i);
	else if (key == MARK_CLEAR)
		Unmark_all(i);
	if (!Quiet(player)) notify(player, "Done.");
}

/* ---------------------------------------------------------------------------
 * do_apply_marked: Perform a command for each marked obj in the db.
 */

void do_apply_marked (dbref player, dbref cause, int key, 
		char *command, char *cargs[], int ncargs)
{
char	*buff;
int	i;

	if (mudconf.control_flags & CF_DBCHECK) {
		er_mark_disabled(player);
		return;
	}
	buff = alloc_sbuf("do_apply_marked");
	DO_WHOLE_DB(i) {
		if (Marked(i)) {
			sprintf(buff, "#%d", i);
			bind_and_queue (player, cause, command, buff,
				cargs, ncargs);
		}
	}
	free_sbuf(buff);
	if (!Quiet(player)) notify(player, "Done.");
}

long count_player(dbref target, int i_val)
{
  long rval;
  dbref d;

  rval = 0;
  if ( i_val == 0 ) {  /* All objects and attributes of target */
     DO_WHOLE_DB(d) {
       if (Going(d) || Recover(d)) continue;
       if (Owner(d) == target) {
	   rval += sizeof(struct object) + strlen(Name(d)) + 1;
	   rval += count_atrs(d);
       }
     }
  } else if ( i_val == 1 ) { /* All objects of target but not attributes */
     DO_WHOLE_DB(d) {
       if (Going(d) || Recover(d)) continue;
       if (Owner(d) == target) {
	   rval += sizeof(struct object) + strlen(Name(d)) + 1;
       }
     }
  } else { /* Just target and attributes of target - Works for condition 2 & 3 */
      rval += sizeof(struct object) + strlen(Name(target)) + 1;
      rval += count_atrs(target);
  }
  return rval;
}

/* ---------------------------------------------------------------------------
 * olist_init, olist_cleanup, olist_add, olist_first, olist_next: Object list
 * management routines.
 */

/* olist_init: Clear and initialize the object list */

void FDECL(olist_init,(OBLOCKMASTER *master))
{
  (*master).olist_head = NULL;
  (*master).olist_tail = NULL;
  (*master).olist_cblock = NULL;
  (*master).olist_count = 0;
  (*master).olist_citm = 0;
}

void file_olist_init(FILE** master, const char *filename)
{
  *master = fopen(filename,"w+");
}

/* olist_cleanup: cleanup an object list after use */
void FDECL(olist_cleanup,(OBLOCKMASTER *master))
{
  OBLOCK *op, *onext;

  for( op = (*master).olist_head; op!=NULL; op=onext) {
    onext = op->next;
    free_lbuf(op);
  }
  (*master).olist_head = NULL;
  (*master).olist_tail = NULL;
  (*master).olist_cblock = NULL;
  (*master).olist_count = 0;
  (*master).olist_citm = 0;
}

void file_olist_cleanup(FILE** master)
{
  if( !*master ) return;
  fclose(*master);
  /*unlink("search.tmp");*/
}

/* olist_add: Add an entry to the object list */

void olist_add (OBLOCKMASTER *master, dbref item)
{
OBLOCK	*op;

	if (!(*master).olist_head) {
		op = (OBLOCK *)alloc_lbuf("olist_add.first");
		(*master).olist_head = (*master).olist_tail = op;
		(*master).olist_count = 0;
		op->next = NULL;
	} else if ((*master).olist_count >= OBLOCK_SIZE) {
		op = (OBLOCK *)alloc_lbuf("olist_add.next");
		(*master).olist_tail->next = op;
		(*master).olist_tail = op;
		(*master).olist_count = 0;
		op->next = NULL;
	} else {
		op = (*master).olist_tail;
	}
	op->data[(*master).olist_count++] = item;
}

void file_olist_add(FILE** master, dbref item)
{
  if( !*master ) return;

  fwrite( &item, sizeof(dbref), 1, *master);
}

/* olist_first: Return the first entry in the object list */

dbref FDECL(olist_first,(OBLOCKMASTER *master))
{
	if (!(*master).olist_head)
		return NOTHING;
	if (((*master).olist_head == (*master).olist_tail) &&
	    ((*master).olist_count == 0))
		return NOTHING;
	(*master).olist_cblock = (*master).olist_head;
	(*master).olist_citm = 0;
	return (*master).olist_cblock -> data[(*master).olist_citm++];
}

dbref file_olist_first(FILE** master)
{
  dbref temp;

  if( !*master ) return NOTHING;

  fseek(*master, 0, SEEK_SET);

  if( fread(&temp, sizeof(dbref), 1, *master) != 1 ) {
    return NOTHING;
  }

  return temp;
}

dbref FDECL(olist_next, (OBLOCKMASTER *master))
{
dbref	thing;
	if (!(*master).olist_cblock)
		return NOTHING;
	if (((*master).olist_cblock == (*master).olist_tail) &&
	    ((*master).olist_citm >= (*master).olist_count))
		return NOTHING;
	thing = (*master).olist_cblock->data[(*master).olist_citm++];
	if ((*master).olist_citm >= OBLOCK_SIZE) {
		(*master).olist_cblock = (*master).olist_cblock->next;
		(*master).olist_citm = 0;
	}
	return thing;
}

dbref file_olist_next(FILE** master)
{
  dbref temp;

  if( !*master ) return NOTHING;

  if( fread(&temp, sizeof(dbref), 1, *master) != 1 ) {
    return NOTHING;
  }

  return temp;
}
