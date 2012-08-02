#ifdef REALITY_LEVELS

#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "rhost_ansi.h"
#include "externs.h"
#include "debug.h"
#include "levels.h"

extern void do_halt(dbref player, dbref cause, int key, char *target);


RLEVEL RxLevel(dbref thing)
{
    char *buff;
    int i;
	RLEVEL rx;
	
	if (mudconf.wiz_always_real && Typeof(thing) == TYPE_PLAYER && Wizard(thing) && !TogMortReal(thing))
		return(~(RLEVEL)0);
    buff = atr_get_raw(thing, A_RLEVEL);
    if (!buff || strlen(buff) != 17)
        switch(Typeof(thing))
        {
            case TYPE_ROOM:
            	return(mudconf.def_room_rx);
            case TYPE_PLAYER:
            	return(mudconf.def_player_rx);
            case TYPE_EXIT:
            	return(mudconf.def_exit_rx);
            default:
            	return(mudconf.def_thing_rx);
        }
    for(rx=0, i=0; buff[i] && isxdigit((int)buff[i]); ++i)
    	rx = 16 * rx + ((buff[i] <= '9')?(buff[i]-'0'):(10 + toupper(buff[i]) - 'A'));
	return(rx);
}

RLEVEL TxLevel(dbref thing)
{
    char *buff;
    int i;
	RLEVEL tx;

	if (mudconf.wiz_always_real && Typeof(thing) == TYPE_PLAYER && Wizard(thing) && !TogMortReal(thing))
		return(~(RLEVEL)0);
    buff = atr_get_raw(thing, A_RLEVEL);
    if (!buff || strlen(buff) != 17)
        switch(Typeof(thing))
        {
            case TYPE_ROOM:
            	return(mudconf.def_room_tx);
            case TYPE_PLAYER:
            	return(mudconf.def_player_tx);
            case TYPE_EXIT:
            	return(mudconf.def_exit_tx);
            default:
            	return(mudconf.def_thing_tx);
        }
    for(tx=0, i=0; buff[i] && !isspace((int)buff[i]); ++i);
    if(buff[i])
    	for(++i; buff[i] && isxdigit((int)buff[i]); ++i)
    		tx = 16 * tx + ((buff[i] <= '9')?(buff[i]-'0'):(10 + toupper(buff[i]) - 'A'));
    return(tx);
}

RLEVEL find_rlevel(char *name)
{
    int i;

    for(i=0; i < mudconf.no_levels; ++i)
        if(!strcasecmp(name, mudconf.reality_level[i].name))
             return mudconf.reality_level[i].value;
    return 0;
}

char *
rxlevel_description(dbref player, dbref target, int flag, int f2)
{
    char *buff, *bp;
    RLEVEL level;
    int i, f2a, chk_x, chk_y, chk_z;

    bp = buff = alloc_lbuf("rxlevel_description");
    if (f2) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "RxLevel", buff, &bp);
	if (flag) {
	    safe_str((char *) " for ", buff, &bp);
	    safe_str(Name(target), buff, &bp);
	}
	safe_chr(':', buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    f2a = f2;
    level = RxLevel(target);
    chk_x = sizeof( mudconf.reality_level );
    chk_y = sizeof( mudconf.reality_level[0] );
    if ( chk_y == 0 )
       chk_z = 0;
    else
       chk_z = chk_x / chk_y;
    for(i = 0; (i < mudconf.no_levels) && (i < chk_z); ++i) {
    	if((level & mudconf.reality_level[i].value) == mudconf.reality_level[i].value) {
    	    if(f2a)
    	    	safe_chr(' ', buff, &bp);
    	    else
    	    	f2a = 1;
    	    safe_str(mudconf.reality_level[i].name, buff, &bp);
    	}
    }
/* This is for debugging only */
/*  if ( chk_z != mudconf.no_levels )
       notify(player, unsafe_tprintf("Error in reality levels: %d found/%d total.",
                      chk_z, mudconf.no_levels));
 */
    *bp = '\0';
    return buff;
}

char *
txlevel_description(dbref player, dbref target, int flag, int f2)
{
    char *buff, *bp;
    RLEVEL level;
    int i, f2a, chk_x, chk_y, chk_z;

    bp = buff = alloc_lbuf("txlevel_description");
    if (f2) {
        safe_str((char *) ANSIEX(ANSI_HILITE), buff, &bp);
	safe_str((char *) "TxLevel", buff, &bp);
	if (flag) {
	    safe_str((char *) " for ", buff, &bp);
	    safe_str(Name(target), buff, &bp);
	}
	safe_chr(':', buff, &bp);
        safe_str((char *) ANSIEX(ANSI_NORMAL), buff, &bp);
    }
    f2a = f2;
    level = TxLevel(target);
    chk_x = sizeof( mudconf.reality_level );
    chk_y = sizeof( mudconf.reality_level[0] );
    if ( chk_y == 0 )
       chk_z = 0;
    else
       chk_z = chk_x / chk_y;
    for(i = 0; (i < mudconf.no_levels) && (i < chk_z); ++i) {
    	if((level & mudconf.reality_level[i].value) == mudconf.reality_level[i].value) {
    	    if(f2a)
    	    	safe_chr(' ', buff, &bp);
    	    else
    	    	f2a = 1;
    	    safe_str(mudconf.reality_level[i].name, buff, &bp);
    	}
    }
/* This is for debugging only */
/*  if ( chk_z != mudconf.no_levels )
       notify(player, unsafe_tprintf("Error in reality levels: %d found/%d total.",
                      chk_z, mudconf.no_levels)); 
 */
    *bp = '\0';
    return buff;
}

void decompile_rlevels(dbref player, dbref thing, char *thingname)
{
    char *buf, *tpr_buff, *tprp_buff;

    buf = rxlevel_description(player, thing, 0, 0);
    tprp_buff = tpr_buff = alloc_lbuf("decompile_rlevels");
    noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@rxlevel %s=%s", strip_ansi(thingname), buf));
    free_lbuf(buf);

    buf = txlevel_description(player, thing, 0, 0);
    tprp_buff = tpr_buff;
    noansi_notify(player, safe_tprintf(tpr_buff, &tprp_buff, "@txlevel %s=%s", strip_ansi(thingname), buf));
    free_lbuf(tpr_buff);
    free_lbuf(buf);
}

void do_rxlevel(dbref player, dbref cause, int key, char *object, char *arg)
{
    dbref thing;
    int negate, i;
    RLEVEL result, ormask, andmask;
    char lname[9], *buff;

    /* find thing */
    if ((thing = match_controlled(player, object)) == NOTHING)
        return;
    
    if (!arg || !*arg) {
        notify_quiet(player, "I don't know what you want to set!");
        return;
    }
    ormask = 0;
    andmask = ~ormask;
    while(*arg)
    {
        negate = 0;
        while(*arg && isspace((int)*arg))
            arg++;
        if(*arg == '!')
        {
            negate = 1;
            ++arg;
        }
        for(i=0; *arg && !isspace((int)*arg); ++arg)
            if(i < 8)
                lname[i++] = *arg;
        lname[i] = '\0';
        if(!lname[0])
        {
            if(negate)
                notify_quiet(player, "You must specify a reality level to clear.");
            else
                notify_quiet(player, "You must specify a reality level to set.");
            return;
        }
        result = find_rlevel(lname);
        if(!result)
        {
            notify_quiet(player, "No such reality level.");
            continue;
        }
        if(negate)
        {
            andmask &= ~result;
            notify_quiet(player, "Cleared.");
        }
        else
        {
            ormask |= result;
            notify_quiet(player, "Set.");
        }
    }
    /* Set the RxLevel */
    buff = alloc_lbuf("do_rxlevel");
    sprintf(buff, "%08X %08X", ((RxLevel(thing) & andmask) | ormask), TxLevel(thing));
    atr_add_raw(thing, A_RLEVEL, buff);
    free_lbuf(buff);
}

void do_txlevel(dbref player, dbref cause, int key, char *object, char *arg)
{
    dbref thing;
    int negate, i;
    RLEVEL result, ormask, andmask;
    char lname[9], *buff;

    /* find thing */
    if ((thing = match_controlled(player, object)) == NOTHING)
        return;
    
    if (!arg || !*arg) {
        notify_quiet(player, "I don't know what you want to set!");
        return;
    }
    ormask = 0;
    andmask = ~ormask;
    while(*arg)
    {
        negate = 0;
        while(*arg && isspace((int)*arg))
            arg++;
        if(*arg == '!')
        {
            negate = 1;
            ++arg;
        }
        for(i=0; *arg && !isspace((int)*arg); ++arg)
            if(i < 8)
                lname[i++] = *arg;
        lname[i] = '\0';
        if(!lname[0])
        {
            if(negate)
                notify_quiet(player, "You must specify a reality level to clear.");
            else
                notify_quiet(player, "You must specify a reality level to set.");
            return;
        }
        result = find_rlevel(lname);
        if(!result)
        {
            notify_quiet(player, "No such reality level.");
            continue;
        }
        if(negate)
        {
            andmask &= ~result;
            notify_quiet(player, "Cleared.");
        }
        else
        {
            ormask |= result;
            notify_quiet(player, "Set.");
        }
    }
    /* Set the TxLevel */
    buff = alloc_lbuf("do_txlevel");
    sprintf(buff, "%08X %08X", RxLevel(thing), ((TxLevel(thing) & andmask) | ormask));
    atr_add_raw(thing, A_RLEVEL, buff);
    free_lbuf(buff);
}

int *desclist_match(dbref player, dbref thing)
{
    RLEVEL match;
    int i, j;
    char *tpr_buff, *tprp_buff;
    ATTR *at;
    static int descbuffer[33];
    int chk_x, chk_y, chk_z;

    descbuffer[0] = 1;
    match = RxLevel(player) & TxLevel(thing);
    chk_x = sizeof(mudconf.reality_level);
    chk_y = sizeof(mudconf.reality_level[0]);
    if ( chk_y == 0 )
       chk_z = 0;
    else
       chk_z = chk_x / chk_y;
/*  if ( chk_z != mudconf.no_levels ) { */
    if ( 0 ) {
       tprp_buff = tpr_buff = alloc_lbuf("desclist_match");
       notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Error in reality levels: %d found/%d total.",
                                   chk_z, mudconf.no_levels));
       free_lbuf(tpr_buff);
       for (i = 1; i < mudconf.no_levels; i++)
          descbuffer[i] = 0;
    } else {
       for(i = 0; i < mudconf.no_levels; ++i) {
           if((match & mudconf.reality_level[i].value) == mudconf.reality_level[i].value)
           {
               at = atr_str(mudconf.reality_level[i].attr);
               if(at)
               {
                   for(j = 1; j < descbuffer[0]; ++j)
                       if(at->number == descbuffer[j])
                           break;
                   if(j == descbuffer[0])
                       descbuffer[descbuffer[0]++] = at->number;
               }
           }
       }
    }
    return(descbuffer);
}

/* ---------------------------------------------------------------------------
 * did_it_rlevel: Have player do something to/with thing
 * Just like did_it, but 'what' is ignored, the desclist match
 * being used instead
 */

void did_it_rlevel(dbref player, dbref thing, int what, const char *def, int owhat, 
	const char *odef, int awhat, char *args[], int nargs)
{
char	*d, *buff, *act, *charges, *lcbuf, *master_str, *master_ret, *savereg[MAX_GLOBAL_REGS], *pt;
char	*tmpformat_buff, *tpr_buff, *tprp_buff;
dbref	loc, aowner, aowner3, master;
int	num, aflags, cpustopper, nocandoforyou, aflags3, tst_attr, chkoldstate;
int     i, *desclist, found_a_desc, x, i_didsave, i_currattr, did_allocate_buff;
ATTR 	*tst_glb, *format_atr;

	nocandoforyou = !(!(Toggles2(thing) & TOG_SILENTEFFECTS));
	/* message to player */

        mudstate.chkcpu_stopper = time(NULL);
        chkoldstate = mudstate.chkcpu_toggle;
	cpustopper = 0;
        i_didsave = did_allocate_buff = 0;

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


	if (what > 0) {
	  /* Get description list */
	  desclist = desclist_match(player, thing);
	  found_a_desc = 0;
          if ( mudconf.descs_are_local ) {
             i_didsave = 1;
             for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                savereg[x] = alloc_lbuf("ulocal_reg");
                pt = savereg[x];
                safe_str(mudstate.global_regs[x],savereg[x],&pt);
             }
          }
	  for(i = 1; i < desclist[0]; ++i)
	  {
                /* Ok, if it's A_DESC, we need to check against A_IDESC */
                if ( (what == A_IDESC) && ((desclist[i] == A_DESC) || (stricmp(mudconf.reality_level[i].name, "Real") == 0)) ) {
		   d = atr_pget(thing, A_IDESC, &aowner, &aflags);
                   i_currattr = A_IDESC;
                } else {
		   d = atr_pget(thing, desclist[i], &aowner, &aflags);
                   i_currattr = desclist[i];
                }
                tst_glb = atr_num(i_currattr);
                tst_attr = ((tst_glb && (tst_glb->flags & AF_DEFAULT)) || (aflags & AF_DEFAULT)) ? 1 : 0;
                if ( tst_attr ) {
                   master_str = NULL;
                   tmpformat_buff = alloc_sbuf("didit_format");
                   format_atr = atr_num(i_currattr);
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
                      master_str = atr_pget(master, i_currattr, &aowner3, &aflags3);
                   }
                   if ( !master_str )
                      master_str = alloc_lbuf("master_filler");
                }
		if ( *d || (tst_attr && *master_str) ) {
			found_a_desc = 1;		/* We don't need the 'def' message */
			if(nocandoforyou) {
			   notify(player, "Message has been nullified.");
			} else {
                           if ( *d && tst_attr && *master_str ) {
                              if ( *d ) {
                                 master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                                   d, args, nargs);
                                 buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                             master_str, &master_ret, 1);
                                 free_lbuf(master_ret);
                              } else {
                                 buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                             master_str, args, nargs);
                              }
                           } else {
                              buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                          d, args, nargs);
                           }
			   notify(player, buff);
			   free_lbuf(buff);
			}
		}
                if ( tst_attr )
                   free_lbuf(master_str);
		free_lbuf(d);
	  }
	  if(!found_a_desc) {
	      /* Nothing matched. Try the default desc (again)
	       * 'what' should be A_DESC or A_IDESC.
	       * Worst case we just look for it twice
	       */
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
                    if ( did_allocate_buff )
                       free_lbuf(master_str);
                    master_str = atr_pget(master, what, &aowner3, &aflags3);
                 }
                 if ( !master_str )
                    master_str = alloc_lbuf("master_filler");
              }
	      if ( *d || (tst_attr && *master_str) ) {
	        if(nocandoforyou) {
	           notify(player, "Message has been nullified.");
	        } else {
                   if ( tst_attr && *master_str ) {
                      if ( *d ) {
                         master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                           d, args, nargs);
                         buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                     master_str, &master_ret, 1);
                         free_lbuf(master_ret);
                      } else {
                         buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                     master_str, args, nargs);
                      }
                   } else {
                      buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                  d, args, nargs);
                   }
	  	   notify(player, buff);
		   free_lbuf(buff);
		}
	      } else if (def) {
	        if(nocandoforyou) {
	           notify(player, "Message has been nullified.");
	        } else {
		   notify(player, def);
		}
	      }
              if ( tst_attr )
                 free_lbuf(master_str);
  	      free_lbuf(d);
	   }
	   cpustopper += mudstate.chkcpu_toggle;
           if ( ((what == A_IDESC) || (what == A_DESC)) ) {
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
	      if (*d || (tst_attr && *master_str) ) {
                 if ( tst_attr && *master_str ) {
                    if ( *d ) {
                       master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                         d, args, nargs);
                       buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                   master_str, &master_ret, 1);
                       free_lbuf(master_ret);
                    } else {
                       buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                   master_str, args, nargs);
                    }
                 } else {
                    buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                d, args, nargs);
                 }
		 notify(player, buff);
		 free_lbuf(buff);
	      }
              if ( tst_attr )
                 free_lbuf(master_str);
	      free_lbuf(d);
	   }
	   cpustopper += mudstate.chkcpu_toggle;
	} else if ((what < 0) && def) {
	  if(nocandoforyou) {
	    notify(player, "Message has been nullified.");
	  } else {
		notify(player, def);
	  }
	}

	/* message to neighbors */
    /* only neighbors that can see both 'player' and 'thing' should see it */
      if ((!Cloak(player) && (!NCloak(player) || (NCloak(player) && (player != thing)))) ||
	((NCloak(player) || Cloak(player)) && (Immortal(Owner(thing))))) {
	if ((owhat > 0) && Has_location(player) &&
	    Good_obj(loc = Location(player)) && !nocandoforyou) {
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
                  if ( tst_attr && *master_str ) {
                    if ( *d ) {
                       master_ret = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                         d, args, nargs);
                       buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                   master_str, &master_ret, 1);
                       free_lbuf(master_ret);
                    } else {
                       buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                   master_str, args, nargs);
                    }
                  } else {
                    buff = exec(thing, player, player, EV_EVAL|EV_FIGNORE|EV_TOP,
                                d, args, nargs);
                  }
                  if ( *buff ) {
                     tprp_buff = tpr_buff = alloc_lbuf("did_it_rlevel");
		     notify_except2_rlevel2(loc, player, player, thing,
				            safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), buff));
                     free_lbuf(tpr_buff);
                  }
		  free_lbuf(buff);
		} else if (odef) {
                        tprp_buff = tpr_buff = alloc_lbuf("did_it_rlevel");
			notify_except2_rlevel2(loc, player, player, thing,
				  safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
                        free_lbuf(tpr_buff);
		}
                if ( tst_attr )
                   free_lbuf(master_str);
		cpustopper += mudstate.chkcpu_toggle;
		free_lbuf(d);
	} else if ((owhat < 0) && odef && Has_location(player) &&
	    Good_obj(loc = Location(player)) && !nocandoforyou) {
                tprp_buff = tpr_buff = alloc_lbuf("did_it_rlevel");
		notify_except2_rlevel2(loc, player, player, thing,
			  safe_tprintf(tpr_buff, &tprp_buff, "%s %s", Name(player), odef));
                free_lbuf(tpr_buff);
	}		
        if ( i_didsave && mudconf.descs_are_local ) {
           i_didsave = 0;
           for (x = 0; x < MAX_GLOBAL_REGS; x++) {
              pt = mudstate.global_regs[x];
              safe_str(savereg[x],mudstate.global_regs[x],&pt);
              free_lbuf(savereg[x]);
           }
        }

	/* do the action attribute */
    /* only if thing can see player in turn */
	if (awhat > 0 && !nocandoforyou && IsReal(thing, player)) {
		if (*(act = atr_pget(thing, awhat, &aowner, &aflags))) {
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
		  if (*act && !mudstate.chkcpu_toggle) 
		    wait_que(thing, player, 0, NOTHING, act, args, nargs, 
                             mudstate.global_regs, mudstate.global_regsname);
		  free_lbuf(act);
		}
	}
      }
      cpustopper += mudstate.chkcpu_toggle;
      if ( cpustopper && !chkoldstate ) {
           notify(player, "Overload on CPU detected.  Process aborted.");
           broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS",
                                          (char *)"(built-in-attr)", NULL, thing, 0, 0, NULL);
           lcbuf = alloc_lbuf("process_commands.LOG.badcpu");
           STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
           log_name_and_loc(player);
           sprintf(lcbuf, " CPU overload: %.3900s", "(built-in-attr)");
           log_text(lcbuf);
           if ( mudstate.curr_cpu_cycle >= mudconf.max_cpu_cycles ) {
              if (Good_obj(thing)) {
                 tprp_buff = tpr_buff = alloc_lbuf("did_it_rlevel");
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
            pt = mudstate.global_regs[x];
            safe_str(savereg[x],mudstate.global_regs[x],&pt);
            free_lbuf(savereg[x]);
         }
      }
      mudstate.chkcpu_toggle = chkoldstate;
}

void 
notify_except_rlevel(dbref loc, dbref player, dbref exception, const char *msg, int key)
{
    dbref first;

      if (loc != exception && IsReal(loc, player))
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | key), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exception && IsReal(first, player)) {
	    notify_check(first, player, msg, 0,
			 (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | key), 0);
	}
      }
}

void 
notify_except2_rlevel(dbref loc, dbref player, dbref exc1, dbref exc2,
	       const char *msg)
{
    dbref first;


      if ((loc != exc1) && (loc != exc2))
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exc1 && first != exc2 && IsReal(first, player)) {
	    notify_check(first, player, msg, 0,
			 (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
	}
      }
}

void 
notify_except2_rlevel2(dbref loc, dbref player, dbref exc1, dbref exc2,
	       const char *msg)
{
    dbref first;


      if ((loc != exc1) && (loc != exc2))
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exc1 && first != exc2 
		&& IsReal(first, exc1) && IsReal(first, exc2)) {
	    notify_check(first, player, msg, 0,
			 (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
	}
      }
}

#endif
