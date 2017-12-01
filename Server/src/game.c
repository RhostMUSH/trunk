/* game.c -- initialization stuff */

#include "copyright.h"
#include "autoconf.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef HAS_OPENSSL
#include <openssl/sha.h>
#include <openssl/evp.h>
#endif

#include "mudconf.h"
#include "config.h"
#include "file_c.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "rwho_clilib.h"
#include "alloc.h"
#include "local.h"

#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */

#include "debug.h"
#define FILENUM GAME_C

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
#include <sys/ucontext.h>
#endif
#include "rhost_ansi.h"
#include "door.h"

#include <math.h>

extern void NDECL(init_attrtab);
extern void NDECL(init_cmdtab);
extern void NDECL(cf_init);
extern void NDECL(pcache_init);
extern int FDECL(cf_read, (char *fn));
extern void NDECL(init_functab);
extern void NDECL(init_ansitab);
extern void FDECL(close_sockets, (int emergency, char *message));
extern void NDECL(close_main_socket);
extern void NDECL(init_version);
extern void NDECL(init_logout_cmdtab);
extern void NDECL(init_timer);
extern void FDECL(raw_notify, (dbref, const char *, int, int));
extern void NDECL(do_second);
extern void FDECL(do_dbck, (dbref, dbref, int));
extern void NDECL(init_pid_table);
extern void NDECL(init_depowertab);
extern int NDECL(load_reboot_db);
extern int NDECL(dump_reboot_db);
extern void FDECL(do_log, (dbref, dbref, int, char *, char *));
extern void FDECL(ignore_signals, ());
extern void FDECL(reset_signals, ());
extern dbref FDECL(match_thing, (dbref, char *));
extern dbref FDECL(match_thing_quiet, (dbref, char *));
extern void FDECL(init_logfile, ());
extern void FDECL(close_logfile, ());

void FDECL(fork_and_dump, (int, char *));
void NDECL(dump_database);
void NDECL(pcache_sync);
static void FDECL(dump_database_internal, (int));
static void NDECL(init_rlimit);

extern double FDECL(time_ng, (double*));
extern int FDECL(alarm_msec, (double));
extern int NDECL(alarm_stop);

int reserved;

/*
 * used to allocate storage for temporary stuff, cleared before command
 * execution
 */

void
setq_templates(dbref thing)
{
   char *s_instr, *s_intok, *s_intokr, *s_intok2, *s_intokr2, c_field[2];
   dbref target_id;
   int target_attr, regnum;
   ATTR *m_attr;
#ifdef EXPANDED_QREGS
   int i;
#endif
 
   regnum = -1;
   m_attr = atr_str("setq_template");
   if ( m_attr ) {
      s_instr = atr_get(thing, m_attr->number, &target_id, &target_attr);
      if ( s_instr && *s_instr ) {
         s_intok = strtok_r( s_instr, " ", &s_intokr);
         while ( s_intok && *s_intok ) {
            sprintf(c_field, "%c", *s_intok);
            s_intok2 = strtok_r( s_intok, ":", &s_intokr2);
            if ( s_intok2 && *s_intok2 )
               s_intok2 = strtok_r( NULL, ":", &s_intokr2);
            if ( s_intok2 && *s_intok2 ) {
#ifdef EXPANDED_QREGS
               if ( isalpha(*c_field) ) {
                  for ( i = 0; i < MAX_GLOBAL_REGS; i++ ) {
                     if ( mudstate.nameofqreg[i] == tolower(*c_field) )
                        break;
                  }
                  if ( i < MAX_GLOBAL_REGS )
                     regnum = i;
               } else if ( isdigit(*c_field) ) {
                  regnum = atoi(c_field);
               }
#else
               if ( isdigit(*c_field) ) {
                  regnum = atoi(c_field);
               }
#endif
               if ( regnum != -1 ) {
                  strncpy(mudstate.global_regsname[regnum], s_intok2, (SBUF_SIZE - 1));
               }
            }
            s_intok = strtok_r( NULL, " ", &s_intokr);
         }
      }
      if ( s_instr )
         free_lbuf(s_instr);
   }
}

void 
do_dump(dbref player, dbref cause, int key, char *msg)
{
    char *ptr, prv_ptr;
    int i_valid;

    if ( mudstate.forceusr2 ) {
       notify(player, "In the middle of a SIGUSR2, unable to dump.");
       return;
    }
    DPUSH; /* #68 */
    i_valid = 1;
    prv_ptr = '\0';
    if ( key & DUMP_FLAT ) {
       if ( strlen(msg) > 150 || (*msg && !isalnum((int)*msg)) )
          i_valid = 0;
       else {
          ptr = msg;
          while ( *ptr ) {
             if ( !(isalnum((int)*ptr) || (*ptr == '_') || (*ptr == '-') || 
                  ((*ptr == '.') && (prv_ptr != '.'))) ) {
                i_valid = 0;
                break;
             }
             prv_ptr = *ptr;
             ptr++;
          }
       }
    }
    if ( !i_valid ) {
       notify(player, "Dump aborted.  Invalid filename in argument");
    } else {
       if ( key & DUMP_FLAT ) {
          notify(player, "Flatfile dumping...");
          fork_and_dump(key, msg);
       } else {
          notify(player, "Dumping...");
          fork_and_dump(key, (char *)NULL);
       }
       notify(player, "...dump completed.");
    }
    VOIDRETURN; /* #68 */
}

/* print out stuff into error file */

void 
NDECL(report)
{
    DPUSH; /* #69 */
    STARTLOG(LOG_BUGS, "BUG", "INFO")
	log_text((char *) "Command: '");
    log_text(mudstate.debug_cmd);
    log_text((char *) "'");
    ENDLOG
	if (Good_obj(mudstate.curr_player)) {
	STARTLOG(LOG_BUGS, "BUG", "INFO")
	    log_text((char *) "Player: ");
	log_name_and_loc(mudstate.curr_player);
	if ((mudstate.curr_enactor != mudstate.curr_player) &&
	    Good_obj(mudstate.curr_enactor)) {
	    log_text((char *) " Enactor: ");
	    log_name_and_loc(mudstate.curr_enactor);
	}
	ENDLOG
    }
    VOIDRETURN; /* #69 */
}

/* ----------------------------------------------------------------------
 * atr_match: Check attribute list for wild card matches and queue them.
 */

static int 
atr_match1(dbref thing, dbref parent, dbref player, char type,
	   char *str, int check_exclude,
	   int hash_insert, int dpcheck, int cmast, int *i_lock)
{
    dbref aowner, thing2;
    int match, attr, aflags, i, ck, ck2, ck3, loc, attrib2, x, i_cpuslam, 
        do_brk, aflags_set, oldchk, chkwild, i_inparen;
    char *buff, *s, *s2, *s3, *as, *s_uselock, *atext, *result, buff2[LBUF_SIZE+1];
    char *args[10], *savereg[MAX_GLOBAL_REGS], *pt, *cpuslam, *cputext, *cpulbuf,
         *saveregname[MAX_GLOBAL_REGS], *npt;
    ATTR *ap, *ap2;

    DPUSH; /* #70 */

    /* See if we can do it.  Silently fail if we can't. */

    ck3 = ck2 = chkwild = i_inparen = 0;
    oldchk = mudstate.chkcpu_toggle;
    if (!could_doit(player, parent, A_LUSE, 1, 1)) {
        if ( mudstate.chkcpu_toggle && !oldchk ) {
           broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (USELOCK)",
                             (char *)"(internal-attribute)", NULL, parent, 0, 0, NULL);
           STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
           log_name_and_loc(player);
           cpulbuf = alloc_lbuf("log_uselock_cpuslam");
           sprintf(cpulbuf, " CPU/USELOCK overload: #%d(#%d) - UseLock Eval", parent, player);
           log_text(cpulbuf);
           free_lbuf(cpulbuf);
           ENDLOG
        }
        if ( Good_obj(thing) && ShowFailCmd(thing) ) {
          *i_lock = 1;
          ck3 = 1;
        } else {
	  RETURN(-1); /* #70 */
        }
    }
    if ( *i_lock )
       ck3 = 1;
    /* Mimic PENN's ability to only have players access $cmds on themselves/invent */
    if ( mudconf.penn_playercmds && (Good_obj(thing) && isPlayer(thing)) ) {
       if ( Good_obj(player) )
          loc = Location(player);
       else
          loc = NOTHING;
       if ( (loc != thing) && (thing != player) ) {
          RETURN(-1); /* #70 */
       }
    }
    match = 0;
    buff = alloc_lbuf("atr_match1");
    atr_push();
    i_cpuslam = 0;
    for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
	ap = atr_num(attr);

	/* Never check NOPROG attributes. */

	if (!ap || (ap->flags & AF_NOPROG))
	    continue;

	/* If we aren't the bottom level check if we saw this attr
	 * before.  Also exclude it if the attribute type is PRIVATE.
	 */

	if (check_exclude &&
	    ((ap->flags & AF_PRIVATE) ||
	     nhashfind(ap->number, &mudstate.parent_htab))) {
	    continue;
	}
	atr_get_str(buff, parent, attr, &aowner, &aflags);

	/* Skip if private and on a parent */

	if (check_exclude && (aflags & AF_PRIVATE)) {
	    continue;
	}


	/* If we aren't the top level remember this attr so we exclude
	 * it from now on.
	 */

	if (hash_insert)
	    nhashadd(ap->number, (int *) &attr,
		     &mudstate.parent_htab);

	/* Check for the leadin character after excluding the attrib
	 * This lets non-command attribs on the child block commands
	 * on the parent.
	 */

	if ((buff[0] != type) || (aflags & AF_NOPROG))
	    continue;

	/* decode it: search for first un escaped : */

        for (s = buff + 1; *s && (*s != ':' || (*(s-1) == '\\' && *(s-2) != '\\')); s++);
/*	for (s = buff + 1; *s && (*s != ':'); s++); */
	if (!*s)
	    continue;
	*s++ = 0;
        /* Allow attributes set NO_PARSE to pass in what player types verbatum */
        if ( PCRE_EXEC && ((aflags & AF_REGEXP) || (ap->flags & AF_REGEXP)) ) {
           s3 = buff + 1;
           memset(buff2, '\0', sizeof(buff2));
           s2 = buff2;
           while ( *s3 ) {
              if ( *s3 == '(' ) 
                 i_inparen++;
              if ( *s3 == ')' ) 
                 i_inparen--;
              if ( (*s3 == '\\') && (i_inparen > 0) ) {
                 if ( *(s3+1) && (*(s3+1) != '.') && (*(s3+1) != '+') && (*(s3+1) != '*') )
                    s3++;
              } 
              *s2 = *s3;
              if ( *s3 )
                 s3++;
              s2++;
           }
           chkwild = regexp_wild_match(buff2,
                                ((aflags & AF_NOPARSE) ? mudstate.last_command : str), args, 10, 1);
        } else {
           chkwild = wild_match(buff + 1, 
                                ((aflags & AF_NOPARSE) ? mudstate.last_command : str), args, 10, 0);
        }
	if ( chkwild ) {
	  if (!dpcheck || (dpcheck && ((Owner(player) == Owner(thing)) || 
	    !DePriv(player,NOTHING,DP_ABUSE,POWER7,POWER_LEVEL_NA)) &&
            !((Owner(player) == Owner(thing)) && DePriv(player,NOTHING,DP_PERSONAL_COMMANDS,POWER8,POWER_LEVEL_NA))) ) {
            do_brk = 0;
#ifdef ATTR_HACK
            /* Ok, let's check for attribute uselocks */
            if ( ((aflags & AF_USELOCK) || (ap->flags & AF_USELOCK)) && !mudstate.chkcpu_toggle && !(aflags & AF_NOPROG) ) {
               if ( !(Wizard(player) && !NoOverride(player) && mudconf.wiz_override &&
                    !DePriv(player,NOTHING,DP_OVERRIDE,POWER7,POWER_LEVEL_NA) &&
                    !NoUselock(player) && mudconf.wiz_uselock) ) {
                  /* Is the attribute SBUF-2 chars or less in size? */
                  if ( strlen(ap->name) < (SBUF_SIZE - 2) ) {
                     s_uselock = alloc_sbuf("attr_uselock");
                     memset(s_uselock, 0, SBUF_SIZE);
                     sprintf(s_uselock, "~%.30s", ap->name);
                     ap2 = atr_str(s_uselock);
                     if ( ap2 ) {
                        atext = atr_get(parent, ap2->number, &thing2, &attrib2);
                        aflags_set = ap2->number;
                        if ( atext ) {
                           if ( !(attrib2 & AF_NOPROG) ) {
                              for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                                 savereg[x] = alloc_lbuf("ulocal_reg");
                                 saveregname[x] = alloc_sbuf("ulocal_regname");
                                 pt = savereg[x];
                                 npt = saveregname[x];
                                 safe_str(mudstate.global_regs[x],savereg[x],&pt);
                                 safe_str(mudstate.global_regsname[x],saveregname[x],&npt);
                              }
                              cputext = alloc_lbuf("store_text_attruselock");
                              strcpy(cputext, atext);
                              result = cpuexec(parent, player, player, EV_FCHECK | EV_EVAL, atext,
                                               args, 10, (char **)NULL, 0);
                              if ( !i_cpuslam && mudstate.chkcpu_toggle ) {
                                 i_cpuslam = 1;
                                 cpuslam = alloc_lbuf("uselock_cpuslam");
                                 memset(cpuslam, 0, LBUF_SIZE);
                                 sprintf(cpuslam, "(ATTR:%.32s):%.3900s", s_uselock, cputext);
                                 broadcast_monitor(player, MF_CPU, "CPU RUNAWAY PROCESS (ATRULCK)",
                                                   (char *)cpuslam, NULL, parent, 0, 0, NULL);
                                 cpulbuf = alloc_lbuf("log_uselock_cpuslam");
                                 STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                                 log_name_and_loc(player);
                                 sprintf(cpulbuf, " CPU/ULCK overload: (#%d) %.3940s", parent, cpuslam);
                                 log_text(cpulbuf);
                                 free_lbuf(cpulbuf);
                                 ENDLOG
                                 free_lbuf(cpuslam);
                                 atr_set_flags(parent, aflags_set, (attrib2 | AF_NOPROG) );
                              }
                              free_lbuf(cputext);
                              for (x = 0; x < MAX_GLOBAL_REGS; x++) {
                                 pt = mudstate.global_regs[x];
                                 npt = mudstate.global_regsname[x];
                                 safe_str(savereg[x],mudstate.global_regs[x],&pt);
                                 safe_str(saveregname[x],mudstate.global_regsname[x],&npt);
                                 free_lbuf(savereg[x]);
                                 free_sbuf(saveregname[x]);
                              }
                              if ( *result && result && (atoi(result) != 1) ) 
                                 do_brk=1;
                              free_lbuf(result);
                           }
                           free_lbuf(atext);
                        }
                     }
                     free_sbuf(s_uselock);
                  }
               }
            }
#endif
            if ( !do_brk ) {
	       match = 1;
	       if (!cmast && (Flags3(thing) & NOSTOP) && (thing != mudconf.master_room) &&
                   (Location(thing) != mudconf.master_room)) {
	         ck = atr_match1(mudconf.master_room, mudconf.master_room, player, type, str, 0, 0, dpcheck, 1, i_lock);
	         if (ck < 2) {
	           ck2 = list_check(Contents(mudconf.master_room), player, AMATCH_CMD, str, 0, 0, 0);
	         }
	         if ((ck > 0) || (ck2 > 0)) {
		   match = 2;
		   for (i = 0; i < 10; i++) {
		     if (args[i])
		       free_lbuf(args[i]);
		   }
		   break;
	         }
	       }
               if ( ck3 == 0 ) {
                  setq_templates(thing);
	          wait_que(thing, player, 0, NOTHING, s, args, 10,
		           mudstate.global_regs, mudstate.global_regsname);
                  if ( (ap->flags & AF_SINGLETHREAD) || (aflags & AF_SINGLETHREAD) ) {
                     atr_set_flags(parent, attr, (aflags | AF_NOPROG));
                  }
               }
	       for (i = 0; i < 10; i++) {
		   if (args[i])
		       free_lbuf(args[i]);
	       }
               if ( ck3 == 0 ) {
	          if (Flags3(thing) & STOP) {
	            match = 3;
	            break;
	          }
               }
            } else {
               for (i = 0; i < 10; i++) {
                  if (args[i])
                     free_lbuf(args[i]);
               }
            }/* Do_Brk */
	  } /* DePriv */
	} /* Wild Match */
    } /* For loop */
    atr_pop();
    free_lbuf(buff);
    if ( (ck3 == 1) && (match > 0) ) {
       did_it(player, thing, A_UFAIL,
             "You can not use $-commands on that.",
              A_OUFAIL, NULL, A_AUFAIL, (char **)NULL, 0);
       if ( Flags3(thing) & STOP ) {
          RETURN(3); /* #70 */
       } else {
          RETURN(-1); /* #70 */
       }
    } else {
       RETURN(match); /* #70 */
    }
    /* Should never hit this */
    RETURN(0); /* #70 */
}

int 
atr_match(dbref thing, dbref player, char type, char *str, int check_parents, int dpcheck)
{
    int match, lev, result, exclude, insert;
    int retval, i_lock;
    dbref parent;

    DPUSH; /* #71 */

    /* If COMMANDS is defined, check if exists, if not, don't even bother */
#ifdef ENABLE_COMMAND_FLAG
    if ( (type == AMATCH_CMD) && !(Flags4(thing) & COMMANDS) ) {
       RETURN(0); /* #71 */
    }
#endif
    /* If thing is halted, don't check anything */

    if (Halted(thing) || (Flags3(thing) & NOCOMMAND) || ((Flags2(thing) & GUILDOBJ) && (Flags3(player) & NO_GOBJ))) {
	RETURN(0); /* #71 */
    }

    /* If not checking parents, just check the thing */

    match = i_lock = 0;
    if (!check_parents) {
        retval = atr_match1(thing, thing, player, type, str, 0, 0, dpcheck, 0, &i_lock);
        RETURN(retval); /* #71 */
    }

    /* Check parents, ignoring halted objects */

    exclude = 0;
    insert = 1;
    nhashflush(&mudstate.parent_htab, 0);
    ITER_PARENTS(thing, parent, lev) {
	if (!Good_obj(Parent(parent)))
	    insert = 0;
	result = atr_match1(thing, parent, player, type, str,
			    exclude, insert, dpcheck, 0, &i_lock);
	if ((result == 3) || (result == 2)) {
	  match = 2;
	  break;
	}
	else if (result > 0) {
	    match = 1;
	    if (result == 2)
	      break;
	} else if (result < 0) {
	    RETURN(match); /* #71 */
	}
	exclude = 1;
    }

    RETURN(match); /* #71 */
}

/* ---------------------------------------------------------------------------
 * notify_check: notifies the object #target of the message msg, and
 * optionally notify the contents, neighbors, and location also.
 */

int 
check_filter(dbref object, dbref player, int filter, const char *msg)
{
    int aflags;
    dbref aowner;
    char *buf, *nbuf, *cp, *dp;

    DPUSH; /* #72 */

    buf = atr_pget(object, filter, &aowner, &aflags);
    if (!*buf) {
	free_lbuf(buf);
	RETURN(1); /* #72 */
    }
    dp = nbuf = cpuexec(object, player, player, EV_FIGNORE | EV_EVAL | EV_TOP, buf,
		        (char **) NULL, 0, (char **)NULL, 0);
    free_lbuf(buf);
    do {
	cp = parse_to(&dp, ',', EV_STRIP);
	if (quick_wild(cp, (char *) msg)) {
	    free_lbuf(nbuf);
	    RETURN(0); /* #72 */
	}
    } while (dp != NULL);
    free_lbuf(nbuf);
    RETURN(1); /* #72 */
}

static char *
add_prefix(dbref object, dbref player, int prefix,
	   const char *msg, const char *dflt)
{
    int aflags;
    dbref aowner;
    char *buf, *nbuf, *cp;

    DPUSH; /* #73 */

    buf = atr_pget(object, prefix, &aowner, &aflags);
    if (!*buf) {
	cp = buf;
	safe_str((char *) dflt, buf, &cp);
    } else {
	nbuf = cpuexec(object, player, player, EV_FIGNORE | EV_EVAL | EV_TOP, buf,
		       (char **) NULL, 0, (char **)NULL, 0);
	free_lbuf(buf);
	buf = nbuf;
	cp = &buf[strlen(buf)];
    }
    if (cp != buf)
	safe_str((char *) " ", buf, &cp);
    safe_str((char *) msg, buf, &cp);
    *cp = '\0';
    RETURN(buf); /* #73 */
}

static char *
dflt_from_msg(dbref sender, dbref sendloc)
{
    char *tp, *tbuff;

    DPUSH; /* #74 */

    tp = tbuff = alloc_lbuf("notify_check.fwdlist");
    safe_str((char *) "From ", tbuff, &tp);
    if (Good_obj(sendloc))
	safe_str(Name(sendloc), tbuff, &tp);
    else
	safe_str(Name(sender), tbuff, &tp);
    safe_chr(',', tbuff, &tp);
    *tp = '\0';
    RETURN(tbuff); /* #74 */
}

void 
notify_check(dbref target, dbref sender, const char *msg, int port, int key, int i_type)
{
#ifdef ZENTY_ANSI
    char *mp2, *msg_utf, *mp_utf, *pvap[4];
#endif
    char *msg_ns, *mp, *msg_ns2, *tbuff, *tp, *buff, *s_tstr, *s_tbuff, *vap[4];
    char *args[10], *s_logroom, *cpulbuf, *s_aptext, *s_aptextptr, *s_strtokr, *s_pipeattr, *s_pipeattr2, *s_pipebuff, *s_pipebuffptr;
    dbref aowner, targetloc, recip, obj, i_apowner, passtarget;
    int i, nargs, aflags, has_neighbors, pass_listen, noansi=0, i_pipetype, i_brokenotify = 0, i_chkcpu;
    int check_listens, pass_uselock, is_audible, i_apflags, i_aptextvalidate = 0, i_targetlist = 0, targetlist[LBUF_SIZE];
    struct tm *ttm2;
    FWDLIST *fp;
    ATTR *ap_log, *ap_attrpipe;

    DPUSH; /* #75 */

    for (i=0; i<10; i++ )
       args[i] = NULL;

    msg_ns2 = NULL;
    cpulbuf = NULL;
    i_chkcpu = 0;

    /* If speaker is invalid or message is empty, just exit */

    if ( !Good_obj(sender) || (!Good_obj(target) && !port) || !msg || !*msg) {
	mudstate.pageref = NOTHING;
	VOIDRETURN; /* #75 */
    }

    /* Enforce a recursion limit */

    mudstate.ntfy_nest_lev++;
    if (mudstate.ntfy_nest_lev >= mudconf.ntfy_nest_lim) {
	mudstate.ntfy_nest_lev--;
	VOIDRETURN; /* #75 */
    }
    /* Let's optionally log to a file if specified -- Note:  This bypasses spoof output obviously */ 
    if ( !port && H_Attrpipe(target) ) {
       i_pipetype = 0;
       ap_attrpipe = atr_str_notify("___ATTRPIPE");
       if ( ap_attrpipe ) {
          s_pipeattr = atr_get(target, ap_attrpipe->number, &aowner, &aflags);
          if ( *s_pipeattr ) {
             if ( (s_pipeattr2 = strchr(s_pipeattr, ' ')) != NULL ) {
                *s_pipeattr2 = '\0';
                i_pipetype = atoi(s_pipeattr2+1);
             } else {
                i_pipetype = 0;
             }
             ap_attrpipe = atr_str_notify(s_pipeattr);
             if ( ap_attrpipe ) {
                free_lbuf(s_pipeattr);
                s_pipeattr = atr_get(target, ap_attrpipe->number, &aowner, &aflags);
                if ( Controlsforattr(target, target, ap_attrpipe, aflags)) {
                   if ( strlen(s_pipeattr) + strlen(msg) < (LBUF_SIZE - 40) ) {
                      s_pipebuffptr = s_pipebuff = alloc_lbuf("pipe_buffering");
                      if ( *s_pipeattr ) {
                         safe_str(s_pipeattr, s_pipebuff, &s_pipebuffptr);
                         safe_str("\r\n", s_pipebuff, &s_pipebuffptr);
                      }
                      safe_str((char *)msg, s_pipebuff, &s_pipebuffptr);
                      atr_add_raw(target, ap_attrpipe->number, s_pipebuff);
                      free_lbuf(s_pipebuff);
                      if ( i_pipetype == 0 ) {
                         free_lbuf(s_pipeattr);
                         if ( !Quiet(target) && TogNoisy(target) )
                            raw_notify(target, (char *)"Piping output to attribute.", 0, 1);
	                 mudstate.ntfy_nest_lev--;
	                 VOIDRETURN; /* #75 */
                      }
                   } else {
                      i_pipetype = -1;
                   }
                }
             }
          } 
          free_lbuf(s_pipeattr);
       }
       if ( !ap_attrpipe ) {
          raw_notify(target, (char *)"Notice: Piping attribute no longer exists.  Piping has been auto-disabled.", 0, 1);
          i_pipetype = -2;
       } else {
          if ( (i_pipetype == -1) || (i_pipetype == 0) ) {
             raw_notify(target, (char *)"Notice: Piping attribute full.  Piping has been auto-disabled.", 0, 1);
             i_pipetype = -2;
          }
       }
       if ( i_pipetype == -2 ) {
          s_Flags4(target, (Flags4(target) & (~HAS_ATTRPIPE)));
          ap_attrpipe = atr_str_notify("___ATTRPIPE");
          if ( ap_attrpipe )
             atr_clr(target, ap_attrpipe->number);
       }
    }
    /* If we want NOSPOOF output, generate it.  It is only needed if
     * we are sending the message to the target object */
    for (i = 0; i < LBUF_SIZE; i++)
       targetlist[i]=-2000000;

    if (key & MSG_ME) {
	mp = msg_ns = alloc_lbuf("notify_check");
#ifdef ZENTY_ANSI
	mp2 = msg_ns2 = alloc_lbuf("notify_check_accents");
	mp_utf = msg_utf = alloc_lbuf("notify_check_utf");
#endif
	if ( !port && Nospoof(target) && (target != sender) &&
  	     ( !(Wizard(sender) || HasPriv(sender, target, POWER_WIZ_SPOOF, POWER5, NOTHING)) || 
               Immortal(target) || Spoof(sender) || Spoof(Owner(sender)) ) &&
	     (target != mudstate.curr_enactor) && (target != mudstate.curr_player)) {

	    /* I'd really like to use unsafe_tprintf here but I can't
	     * because the caller may have.
	     * notify(target, unsafe_tprintf(...)) is quite common in
	     * the code.
	     */

	    tbuff = alloc_sbuf("notify_check.nospoof");
	    safe_chr('[', msg_ns, &mp);
	    safe_str(Name(sender), msg_ns, &mp);
	    sprintf(tbuff, "(#%d)", sender);
	    safe_str(tbuff, msg_ns, &mp);

	    if (sender != Owner(sender)) {
		safe_chr('{', msg_ns, &mp);
		safe_str(Name(Owner(sender)), msg_ns, &mp);
		safe_chr('}', msg_ns, &mp);
	    }
	    if (sender != mudstate.curr_enactor) {
		sprintf(tbuff, "<-(#%d)",
			mudstate.curr_enactor);
		safe_str(tbuff, msg_ns, &mp);
	    }
	    safe_str((char *) "] ", msg_ns, &mp);
	    free_sbuf(tbuff);
#ifdef ZENTY_ANSI
            safe_str(msg_ns, msg_ns2, &mp2);
            safe_str(msg_ns, msg_utf, &mp_utf);
#endif
	}
#ifdef ZENTY_ANSI       
       if(!(key & MSG_NO_ANSI)) {
           parse_ansi((char *) msg, msg_ns, &mp, msg_ns2, &mp2, msg_utf, &mp_utf);
           *mp = '\0';
           *mp2 = '\0';
           *mp_utf = '\0';
           if ( !port ) {
              if ( UTF8(target) ) {
                 memcpy(msg_ns, msg_utf, LBUF_SIZE);
              } else if ( Accents(target) ) {
                 memcpy(msg_ns, msg_ns2, LBUF_SIZE);
              } 
           }
       } else
#endif
           safe_str((char *) msg, msg_ns, &mp);
    
#ifdef ZENTY_ANSI       
       free_lbuf(msg_ns2);
       free_lbuf(msg_utf);
#endif
    } else {
	msg_ns = NULL;
    }
    if(key & MSG_NO_ANSI)
        noansi=MSG_NO_ANSI;
    /* msg contains the raw message, msg_ns contains the NOSPOOFed msg */

  if (port) {
      raw_notify(target, msg_ns, port, 1);
  } else {
    check_listens = Halted(target) ? 0 : 1;
    switch (Typeof(target)) {
    case TYPE_PLAYER:
        i_chkcpu = mudstate.chkcpu_toggle;
        if ( mudstate.posesay_fluff && Connected(target) ) {
           ap_attrpipe = atr_str_notify("SPEECH_PREFIX");
           if ( ap_attrpipe ) {
              s_pipeattr = atr_get(target, ap_attrpipe->number, &aowner, &aflags);
              if ( *s_pipeattr && !(aflags & AF_NOPROG) ) {
                 vap[0] = msg_ns;
                 vap[1] = alloc_mbuf("speech_prefix");
                 vap[2] = alloc_mbuf("speech_prefix2");
                 vap[3] = alloc_mbuf("speech_prefix3");
                 s_pipeattr2 = alloc_lbuf("speech_cpu");
                 sprintf(s_pipeattr2, "%.*s", (LBUF_SIZE - 100), s_pipeattr);
                 if ( Good_chk(mudstate.posesay_dbref) && 
	              ( !(Wizard(mudstate.posesay_dbref) || HasPriv(mudstate.posesay_dbref, target, POWER_WIZ_SPOOF, POWER5, NOTHING)) || 
                        Immortal(target) || Spoof(mudstate.posesay_dbref) || Spoof(Owner(mudstate.posesay_dbref)) ) ) {
                    sprintf(vap[3], "#%d", mudstate.posesay_dbref);
                 } else {
                    sprintf(vap[3], "#%d", -1);
                 }
                 ttm2 = localtime(&mudstate.now);
                 ttm2->tm_year += 1900;
                 sprintf(vap[1], "%02d/%02d/%d", ttm2->tm_mday, ttm2->tm_mon + 1, ttm2->tm_year);
                 sprintf(vap[2], "%02d:%02d:%02d", ttm2->tm_hour, ttm2->tm_min, ttm2->tm_sec);
                 if ( mudconf.posesay_funct && !mudstate.chkcpu_toggle ) {
                    mudstate.posesay_fluff = 0;
                    s_pipebuffptr = cpuexec(target, target, target, EV_FCHECK | EV_EVAL | EV_STRIP, s_pipeattr,
                                            vap, 4, (char **)NULL, 0);
                    mudstate.posesay_fluff = 1;
                    if ( mudstate.chkcpu_toggle ) {
                       aflags |= AF_NOPROG;
                       atr_set_flags(target, ap_attrpipe->number, aflags);
                       broadcast_monitor(target, MF_CPU, "CPU RUNAWAY PROCESS (SPEECH_PREFIX)",
                                         (char *)s_pipeattr2, NULL, target, 0, 0, NULL);
                       STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                          log_name_and_loc(target);
                          sprintf(s_pipeattr, " CPU/SPEECH_PREFIX overload: (#%d) %.*s", target, (LBUF_SIZE - 100), s_pipeattr2);
                          log_text(s_pipeattr);
                       ENDLOG
                    }
                 } else {
                    s_pipebuffptr = cpuexec(target, target, target, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_pipeattr,
                                            vap, 4, (char **)NULL, 0);
                 }
                 free_mbuf(vap[1]);
                 free_mbuf(vap[2]);
                 free_mbuf(vap[3]);
                 if ( *s_pipebuffptr ) {
#ifdef ZENTY_ANSI
                    pvap[0] = vap[0] = alloc_lbuf("vap0");
                    pvap[1] = vap[1] = alloc_lbuf("vap1");
                    pvap[2] = vap[2] = alloc_lbuf("vap2");
                    parse_ansi((char *) s_pipebuffptr, vap[0], &pvap[0], vap[1], &pvap[1], vap[2], &pvap[2]);
                    raw_notify(target, vap[0], port, 1);
                    free_lbuf(vap[0]);
                    free_lbuf(vap[1]);
                    free_lbuf(vap[2]);
#else
                    raw_notify(target, s_pipebuffptr, port, 1);
#endif
                 }
                 free_lbuf(s_pipebuffptr);
                 free_lbuf(s_pipeattr2);
              }
              free_lbuf(s_pipeattr);
           }
        }
	if (key & MSG_ME) {
           if ( mudstate.emit_substitute ) {
              s_tbuff = alloc_sbuf("emit_substitute");
              sprintf(s_tbuff, "#%d", target);
              s_tstr = replace_string(BOUND_VAR, s_tbuff, msg_ns, 0);
              free_sbuf(s_tbuff);
              raw_notify(target, s_tstr, port, 1);
              free_lbuf(s_tstr);
           } else {
             raw_notify(target, msg_ns, port, 1);
           }
        }
        if ( mudstate.posesay_fluff && Connected(target) ) {
           ap_attrpipe = atr_str_notify("SPEECH_SUFFIX");
           if ( ap_attrpipe ) {
              s_pipeattr = atr_get(target, ap_attrpipe->number, &aowner, &aflags);
              if ( *s_pipeattr && !(aflags & AF_NOPROG) ) {
                 vap[0] = msg_ns;
                 vap[1] = alloc_mbuf("speech_prefix");
                 vap[2] = alloc_mbuf("speech_prefix2");
                 vap[3] = alloc_mbuf("speech_prefix3");
                 s_pipeattr2 = alloc_lbuf("speech_cpu");
                 sprintf(s_pipeattr2, "%.*s", (LBUF_SIZE - 100), s_pipeattr);
                 if ( Good_chk(mudstate.posesay_dbref) && 
	              ( !(Wizard(mudstate.posesay_dbref) || HasPriv(mudstate.posesay_dbref, target, POWER_WIZ_SPOOF, POWER5, NOTHING)) || 
                        Immortal(target) || Spoof(mudstate.posesay_dbref) || Spoof(Owner(mudstate.posesay_dbref)) ) ) {
                    sprintf(vap[3], "#%d", mudstate.posesay_dbref);
                 } else {
                    sprintf(vap[3], "#%d", -1);
                 }
                 ttm2 = localtime(&mudstate.now);
                 ttm2->tm_year += 1900;
                 sprintf(vap[1], "%02d/%02d/%d", ttm2->tm_mday, ttm2->tm_mon + 1, ttm2->tm_year);
                 sprintf(vap[2], "%02d:%02d:%02d", ttm2->tm_hour, ttm2->tm_min, ttm2->tm_sec);
                 if ( mudconf.posesay_funct && !mudstate.chkcpu_toggle ) {
                    mudstate.posesay_fluff = 0;
                    s_pipebuffptr = cpuexec(target, target, target, EV_FCHECK | EV_EVAL | EV_STRIP, s_pipeattr,
                                            vap, 4, (char **)NULL, 0);
                    mudstate.posesay_fluff = 1;
                    if ( mudstate.chkcpu_toggle ) {
                       aflags |= AF_NOPROG;
                       atr_set_flags(target, ap_attrpipe->number, aflags);
                       broadcast_monitor(target, MF_CPU, "CPU RUNAWAY PROCESS (SPEECH_SUFFIX)",
                                         (char *)s_pipeattr2, NULL, target, 0, 0, NULL);
                       STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                          log_name_and_loc(target);
                          sprintf(s_pipeattr, " CPU/SPEECH overload: (#%d) %.*s", target, (LBUF_SIZE - 100), s_pipeattr2);
                          log_text(s_pipeattr);
                       ENDLOG
                    }
                 } else {
                    s_pipebuffptr = cpuexec(target, target, target, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_pipeattr,
                                            vap, 4, (char **)NULL, 0);
                 }
                 free_mbuf(vap[1]);
                 free_mbuf(vap[2]);
                 free_mbuf(vap[3]);
                 if ( *s_pipebuffptr ) {
#ifdef ZENTY_ANSI
                    pvap[0] = vap[0] = alloc_lbuf("vap0");
                    pvap[1] = vap[1] = alloc_lbuf("vap1");
                    pvap[2] = vap[2] = alloc_lbuf("vap2");
                    parse_ansi((char *) s_pipebuffptr, vap[0], &pvap[0], vap[1], &pvap[1], vap[2], &pvap[2]);
                    raw_notify(target, vap[0], port, 1);
                    free_lbuf(vap[0]);
                    free_lbuf(vap[1]);
                    free_lbuf(vap[2]);
#else
                    raw_notify(target, s_pipebuffptr, port, 1);
#endif
                 }
                 free_lbuf(s_pipebuffptr);
                 free_lbuf(s_pipeattr2);
              }
              free_lbuf(s_pipeattr);
           }
        }
        mudstate.chkcpu_toggle = i_chkcpu;
	if (!mudconf.player_listen)
	    check_listens = 0;
    case TYPE_THING:
    case TYPE_ROOM:

	/* Forward puppet message if it is for me */

	has_neighbors = Has_location(target);
	targetloc = where_is(target);
	is_audible = Audible(target);

	if ((key & MSG_ME) &&
	    Puppet(target) &&
	    (target != Owner(target)) &&
	    ((key & MSG_PUP_ALWAYS) ||
	     ((targetloc != Location(Owner(target))) &&
	      (targetloc != Owner(target))))) {
	    tp = tbuff = alloc_lbuf("notify_check.puppet");
            if ( !No_Ansi_Ex(Owner(target)) )
               safe_str(ANSI_HILITE, tbuff, &tp);
	    safe_str(Name(target), tbuff, &tp);
	    safe_str((char *) "> ", tbuff, &tp);
            if ( !No_Ansi_Ex(Owner(target)) )
               safe_str(ANSI_NORMAL, tbuff, &tp);
	    safe_str(msg_ns, tbuff, &tp);
	    *tp = '\0';
            if ( mudstate.emit_substitute ) {
	      s_tbuff = alloc_sbuf("emit_substitute");
              sprintf(s_tbuff, "#%d", Owner(target));
              s_tstr = replace_string(BOUND_VAR, s_tbuff, tbuff, 0);
              free_sbuf(s_tbuff);
              raw_notify(Owner(target), s_tstr, port, 1);
              free_lbuf(s_tstr);
	    } else {
              raw_notify(Owner(target), tbuff, port, 1);
	    }
	    free_lbuf(tbuff);
	}
	/* Check for @Listen match if it will be useful */

	pass_listen = 0;
	nargs = 0;
	if (check_listens && (key & (MSG_ME | MSG_INV_L)) &&
	    H_Listen(target) && !(Flags3(target) & NOCOMMAND)) {
	    tp = atr_get(target, A_LISTEN, &aowner, &aflags);
	    if (*tp && wild_match(tp, (char *) msg, args, 10, 0)) {
		for (nargs = 10;
		     nargs &&
		     (!args[nargs - 1] || !(*args[nargs - 1]));
		     nargs--);
		pass_listen = 1;
	    }
	    free_lbuf(tp);
	}
	/* If we matched the @listen or are monitoring, check the
	 * USE lock
	 */

	pass_uselock = 0;
	if ((key & MSG_ME) && check_listens &&
	    (pass_listen || Monitor(target)))
	    pass_uselock = could_doit(sender, target, A_LUSE, 1, 2);

	/* Process AxHEAR if we pass LISTEN, USElock and it's for me */

	if ((key & MSG_ME) && pass_listen && pass_uselock) {
	    if ( (mudstate.ahear_lastplr == target) &&
                 (mudstate.ahear_count >= mudconf.ahear_maxcount) && 
	         ((mudstate.ahear_currtime + mudconf.ahear_maxtime) > time(NULL)) ) {
               if ( mudstate.ahear_count == mudconf.ahear_maxcount ) {
                  cpulbuf = alloc_lbuf("log_uselock_attrib");
                  sprintf(cpulbuf, "RUNAWAY PROCESS (A*HEAR) by #%d(#%d) [%d/%ds]",
                          sender, target, mudconf.ahear_maxcount, mudconf.ahear_maxtime);
                  broadcast_monitor(sender, MF_CPU, cpulbuf,
                                  (char *)"(internal-attribute)", NULL, target, 0, 0, NULL);
                  STARTLOG(LOG_ALWAYS, "WIZ", "CPU");
                  log_name_and_loc(sender);
                  sprintf(cpulbuf, " A*HEAR overload: #%d (#%d) [%d/%ds]- Listen Eval", sender, target,
	                 mudconf.ahear_maxcount, mudconf.ahear_maxtime);
                  log_text(cpulbuf);
                  free_lbuf(cpulbuf);
                  ENDLOG
                  mudstate.ahear_count++;
               }
	    } else {
	       if (sender != target)
		   did_it(sender, target, 0, NULL, 0, NULL,
		          A_AHEAR, args, nargs);
	       else
		   did_it(sender, target, 0, NULL, 0, NULL,
		          A_AMHEAR, args, nargs);
	       did_it(sender, target, 0, NULL, 0, NULL,
		      A_AAHEAR, args, nargs);
	    }
	}
	/* Get rid of match arguments. We don't need them anymore */

	for (i = 0; i < 10; i++)
           if (args[i] != NULL)
              free_lbuf(args[i]);
	/* Process ^-listens if for me, MONITOR, and we pass USElock */

	if ((key & MSG_ME) && pass_uselock && (sender != target) &&
	    Monitor(target)) {
	    (void) atr_match(target, sender,
			     AMATCH_LISTEN, (char *) msg, mudconf.listen_parents, 0);
	}
	/* Deliver message to forwardlist members */

	if ((key & MSG_FWDLIST) && Audible(target) &&
	    check_filter(target, sender, A_FILTER, msg)) {
	    tbuff = dflt_from_msg(sender, target);
	    buff = add_prefix(target, sender, A_PREFIX,
			      msg, tbuff);
	    free_lbuf(tbuff);

	    fp = fwdlist_get(target);
	    if (fp) {
		for (i = 0; i < fp->count; i++) {
		    recip = fp->data[i];
		    if (!Good_obj(recip) ||
			(recip == target))
			continue;
		    notify_check(recip, sender, buff, port,
		       (MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | noansi), i_type);
		}
	    }
	    free_lbuf(buff);
	}
	/* Deliver message through audible exits */

	if (key & MSG_INV_EXITS) {
	    DOLIST(obj, Exits(target)) {
		recip = Location(obj);
		if (Good_obj(recip) && Audible(obj) && ((recip != target) &&
				check_filter(obj, sender, A_FILTER, msg))) {
		    buff = add_prefix(obj, target,
				      A_PREFIX, msg,
				      "From a distance,");
		    notify_check(recip, sender, buff, port,
			 MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | noansi, i_type);
		    free_lbuf(buff);
		}
	    }
	}
	/* Deliver message through neighboring audible exits */

	if (has_neighbors &&
	    ((key & MSG_NBR_EXITS) ||
	     ((key & MSG_NBR_EXITS_A) && is_audible))) {

	    /* If from inside, we have to add the prefix string of
	     * the container.
	     */

	    if (key & MSG_S_INSIDE) {
		tbuff = dflt_from_msg(sender, target);
		buff = add_prefix(target, sender, A_PREFIX,
				  msg, tbuff);
		free_lbuf(tbuff);
	    } else {
		buff = (char *) msg;
	    }

	    if (Good_obj(Location(target))) {
	      DOLIST(obj, Exits(Location(target))) {
		recip = Location(obj);
		if (Good_obj(recip) && Audible(obj) &&
		    (recip != targetloc) &&
		    (recip != target) &&
		    check_filter(obj, sender, A_FILTER, msg)) {
		    tbuff = add_prefix(obj, target,
				       A_PREFIX, buff,
				       "From a distance,");
		    notify_check(recip, sender, tbuff, port,
			 MSG_ME | MSG_F_UP | MSG_F_CONTENTS | MSG_S_INSIDE | noansi, i_type);
		    free_lbuf(tbuff);
		}
	      }
	    }
	    if (key & MSG_S_INSIDE) {
		free_lbuf(buff);
	    }
	}
/* Bounce if you got them */
        for (i = 0; i < LBUF_SIZE; i++)
           targetlist[i]=-2000000;

	if ( Bouncer(target) ) {
	    pass_listen=1; 
            ap_log = atr_str("BOUNCEFORWARD");
            if ( ap_log ) {
               s_aptext = atr_get(target, ap_log->number, &i_apowner, &i_apflags);
               if ( s_aptext && *s_aptext ) {
                  s_aptextptr = strtok_r(s_aptext, " ", &s_strtokr);
                  tbuff = alloc_lbuf("bounce_on_notify");
                  i_targetlist = 0;
                  while ( s_aptextptr ) {
                     passtarget = match_thing_quiet(target, s_aptextptr);
                     i_brokenotify = 0;
                     for (i = 0; i < LBUF_SIZE; i++) {
                        if ( (targetlist[i] == -2000000) || (targetlist[i] == passtarget) ) {
                           if ( targetlist[i] == -2000000 )
                              i_brokenotify = 1;
                           break;
                        }
                     }
                     if ( i_brokenotify && Good_chk(passtarget) && isPlayer(passtarget) && (passtarget != target) && !((passtarget == Owner(target)) && Puppet(target)) ) {
                        if ( !No_Ansi_Ex(passtarget) )
                           sprintf(tbuff, "%sBounce [#%d]>%s %.3950s", ANSI_HILITE, target, ANSI_NORMAL, msg);
                        else
                           sprintf(tbuff, "Bounce [#%d]> %.3950s", target, msg);
                        notify_quiet(passtarget, tbuff);
                     }
                     s_aptextptr = strtok_r(NULL, " ", &s_strtokr);
                     targetlist[i_targetlist] = passtarget;
                     i_targetlist++;
                  }
                  free_lbuf(tbuff);
               }
               free_lbuf(s_aptext);
            }
        }

        if ( LogRoom(target) && ( isRoom(target) || (!isRoom(target) && (Location(sender) == target)) ) ) {
           s_logroom = alloc_mbuf("log_room");
           memset(s_logroom, '\0', MBUF_SIZE);
           ap_log = atr_str("LOGNAME");
           if ( ap_log ) {
              s_aptext = atr_get(target, ap_log->number, &i_apowner, &i_apflags);
              if ( s_aptext && *s_aptext ) {
                 /* Check for alphanumeric */
                 s_aptextptr = s_aptext;
                 i_aptextvalidate = 0;
                 if ( strlen(s_aptextptr) < 70 ) {
                    while ( *s_aptextptr ) {
                       if ( !isalnum(*s_aptextptr) ) {
                          i_aptextvalidate = 1;
                          break;
                       }
                       s_aptextptr++;
                    }   
                 } else {
                    i_aptextvalidate = 1;
                 }
                 if ( !i_aptextvalidate )
                    sprintf(s_logroom, "%.128s/%.70s", mudconf.roomlog_path, s_aptext);
                 else
                    sprintf(s_logroom, "%.128s/room_%d", mudconf.roomlog_path, target);
              } else {
                 sprintf(s_logroom, "%.128s/room_%d", mudconf.roomlog_path, target);
              }
              free_lbuf(s_aptext);
           } else {
              sprintf(s_logroom, "%.128s/room_%d", mudconf.roomlog_path, target);
           }
           do_log(sender, target, (MLOG_FILE|MLOG_ROOM), s_logroom, (char *)msg);
           free_mbuf(s_logroom);
        }

	/* Deliver message to contents */

	if (((key & MSG_INV) || ((key & MSG_INV_L) && pass_listen)) &&
	    (check_filter(target, sender, A_INFILTER, msg))) {

	    /* Don't prefix the message if we were given the
	     * MSG_NOPREFIX key.
	     */

	    if (key & MSG_S_OUTSIDE) {
		buff = add_prefix(target, sender, A_INPREFIX,
				  msg, "");
	    } else {
		buff = (char *) msg;
	    }
	    DOLIST(obj, Contents(target)) {
		if (obj != target) {
#ifdef REALITY_LEVELS
                    if ( !mudstate.reality_notify || (mudstate.reality_notify && IsRealArg(obj, sender, i_type)) )
#endif
		       notify_check(obj, sender, buff, port,
				    MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | noansi, i_type);
		}
	    }
	    if (key & MSG_S_OUTSIDE)
		free_lbuf(buff);
	}
	/* Deliver message to neighbors */

	if (has_neighbors &&
	    ((key & MSG_NBR) ||
	     ((key & MSG_NBR_A) && is_audible &&
	      check_filter(target, sender, A_FILTER, msg)))) {
	    if (key & MSG_S_INSIDE) {
		tbuff = dflt_from_msg(sender, target);
		buff = add_prefix(target, sender, A_PREFIX,
				  msg, "");
		free_lbuf(tbuff);
	    } else {
		buff = (char *) msg;
	    }
	    DOLIST(obj, Contents(targetloc)) {
		if ((obj != target) && (obj != targetloc)) {
#ifdef REALITY_LEVELS
                    if ( !mudstate.reality_notify || (mudstate.reality_notify && IsRealArg(obj, sender, i_type)) )
#endif
		       notify_check(obj, sender, buff, port,
				    MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | noansi, i_type);
		}
	    }
	    if (key & MSG_S_INSIDE) {
		free_lbuf(buff);
	    }
	}
	/* Deliver message to container */

	if (has_neighbors &&
	    ((key & MSG_LOC) ||
	     ((key & MSG_LOC_A) && is_audible &&
	      check_filter(target, sender, A_FILTER, msg)))) {
	    if (key & MSG_S_INSIDE) {
		tbuff = dflt_from_msg(sender, target);
		buff = add_prefix(target, sender, A_PREFIX,
				  msg, tbuff);
		free_lbuf(tbuff);
	    } else {
		buff = (char *) msg;
	    }
	    notify_check(targetloc, sender, buff, port,
			 MSG_ME | MSG_F_UP | MSG_S_INSIDE | noansi, i_type);
	    if (key & MSG_S_INSIDE) {
		free_lbuf(buff);
	    }
	}
    }
  }
  if (msg_ns)
     free_lbuf(msg_ns);
  mudstate.ntfy_nest_lev--;
  VOIDRETURN; /* #75 */
}

void 
notify_except_someone(dbref loc, dbref player, dbref exception, const char *msg, const char *msg2)
{
    dbref first;

    DPUSH; /* #76 */

      if (loc != exception && (!Cloak(player) || (Cloak(player) && Wizard(loc))) )
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exception) {
            if ( !Cloak(player) || (Cloak(player) && Wizard(first)) )
	       notify_check(first, player, msg, 0,
			    (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
            else
	       notify_check(first, player, msg2, 0,
			    (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
	}
      }
    VOIDRETURN; /* #76 */
}

void 
notify_except_str(dbref loc, dbref player, dbref darray[LBUF_SIZE/2], int dcnt, const char *msg, int key)
{
    dbref first;
    int i, exception;

    DPUSH; /* #77 */

    exception = 0;
    if (loc != exception)
       notify_check(loc, player, msg, 0, (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | key), 0);

    DOLIST(first, Contents(loc)) {
       exception = 0;
       for ( i = 0; i < dcnt; i++) {
          if ( first == darray[i] ) {
             exception = 1;
             break;
          } 
       }
       if (!exception) {
          notify_check(first, player, msg, 0, (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | key), 0);
       }
    }

    VOIDRETURN; /* #77 */
}

void 
notify_except(dbref loc, dbref player, dbref exception, const char *msg, int key)
{
    dbref first;

    DPUSH; /* #77 */

      if (loc != exception)
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | key), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exception) {
	    notify_check(first, player, msg, 0,
			 (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | key), 0);
	}
      }
    VOIDRETURN; /* #77 */
}

void 
notify_except2(dbref loc, dbref player, dbref exc1, dbref exc2,
	       const char *msg)
{
    dbref first;

    DPUSH; /* #78 */

      if ((loc != exc1) && (loc != exc2))
	notify_check(loc, player, msg, 0,
		  (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A), 0);
      DOLIST(first, Contents(loc)) {
	if (first != exc1 && first != exc2) {
	    notify_check(first, player, msg, 0,
			 (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
	}
      }
    VOIDRETURN; /* #78 */
}

void 
notify_except3(dbref loc, dbref player, dbref exc1, dbref exc2, int level,
	       const char *msg)
{
    dbref first;

    DPUSH; /* #79 */
      if ( !Good_obj(loc) )
         VOIDRETURN; /* #79 */

      DOLIST(first, Contents(loc)) {
	if (first != exc1 && first != exc2) {
	    if ( !Private(first) && ((level && Immortal(first)) || (!level && Wizard(first))) )
		notify_check(first, player, msg, 0,
			     (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE), 0);
	}
      }
      if ( isPlayer(loc) || isThing(loc) ) {
	 if ( !Private(loc) && ((level && Immortal(loc)) || (!level && Wizard(loc))) )
             notify_quiet(loc, msg);
      }
    VOIDRETURN; /* #79 */
}

void 
do_shutdown(dbref player, dbref cause, int key, char *message)
{
    int fd;

    DPUSH; /* #81 */

    if ((!Wizard(player) && 
	 !HasPriv(player, NOTHING, POWER_SHUTDOWN, POWER4, POWER_LEVEL_NA))) {
	notify(player, "Permission denied.");
	DPOP; /* #81 */
	return;
    }
    if (HasPriv(player, NOTHING, POWER_SHUTDOWN, POWER4, POWER_LEVEL_NA)) {
       if ( (player != cause) && !Wizard(player)) {
           notify(cause, "Permission denied.");
           notify(player, unsafe_tprintf("%s tried to make you @shutdown.", Name(cause)));
	   DPOP; /* #81 */
           return;
       }
       else if (desc_in_use == NULL) {
          notify(player, "You can not queue a shutdown with the power shutdown.");
	  DPOP; /* #81 */
          return;
       } 
    }
    if (player != NOTHING) {
	raw_broadcast(0, 0, "Game: Shutdown by %s", Name(Owner(player)));
	STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
	    log_text((char *) "Shutdown by ");
	log_name(player);
	ENDLOG
    } else {
	raw_broadcast(0, 0, "Game: Fatal Error: %s", message);
	STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
	    log_text((char *) "Fatal error: ");
	log_text(message);
	ENDLOG
    }
    STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN")
	log_text((char *) "Shutdown status: ");
    log_text(message);
    ENDLOG
	fd = tf_open(mudconf.status_file, O_RDWR | O_CREAT | O_TRUNC);
    (void) write(fd, message, strlen(message));
    (void) write(fd, (char *) "\n", 1);
    tf_close(fd);

    /* Do we perform a normal or an emergency shutdown?  Normal shutdown
     * is handled by exiting the main loop in shovechars, emergency
     * shutdown is done here.
     */

    if (key & SHUTDN_PANIC) {

	/* Close down the network interface */

	emergency_shutdown();
	
	/* Stop handling signals. */
	
        mudstate.dumpstatechk=1;
	ignore_signals();
	
	/* Close the attribute text db and dump the header db */

	pcache_sync();
	SYNC;
	CLOSE;
	STARTLOG(LOG_ALWAYS, "DMP", "PANIC")
	    log_text((char *) "Panic dump: ");
	log_text(mudconf.crashdb);
	ENDLOG
	    dump_database_internal(1);
	STARTLOG(LOG_ALWAYS, "DMP", "DONE")
	    log_text((char *) "Panic dump complete: ");
	log_text(mudconf.crashdb);
	ENDLOG
    }
    /* Set up for normal shutdown */

    mudstate.shutdown_flag = 1;
    VOIDRETURN; /* #81 */
}

void
do_reboot(dbref player, dbref cause, int key)
{
  int port;
  FILE *f;
  struct stat statbuf;
  DESC *d;

  DPUSH; /* #80 */

  if ( mudstate.forceusr2 ) {
      notify(player, "In the middle of a SIGUSR2, unable to @reboot.");
      DPOP; /* #80 */
      return;
  }

  if ((!Wizard(player) &&
      !HasPriv(player, NOTHING, POWER_SHUTDOWN, POWER4, POWER_LEVEL_NA))) {
      notify(player, "Permission denied.");
      DPOP; /* #80 */
      return;
  }
  if (HasPriv(player, NOTHING, POWER_SHUTDOWN, POWER4, POWER_LEVEL_NA)) {
     if ( (player != cause) && !Immortal(player)) {
         notify(cause, "Permission denied.");
         notify(player, unsafe_tprintf("%s tried to make you @reboot.", Name(cause)));
         DPOP; /* #80 */
         return;
     }
     else if (desc_in_use == NULL) {
        notify(player, "You can not queue a reboot with the power shutdown.");
        DPOP; /* #80 */
        return;
     }
  }
#ifndef VMS
  if (stat((char *)"netrhost", &statbuf) != 0) {
     notify(player, "There is no netrhost executable to reboot to!");
     DPOP; /* #80 */
     return;
  }
#endif

  alarm_msec(0);
  alarm_stop();
  mudstate.dumpstatechk=1;
  ignore_signals();
  port = mudconf.port;

  if (key == REBOOT_SILENT) {
     f = fopen("reboot.silent", "w+");

     DESC_ITER_CONN(d) {
        if ( d->player == player ) {
           if (f == NULL) {
              queue_string(d,"Cannot write silent reboot file. Final message will not be snuffed.");
           } else {
              queue_string(d,"Game: Rebooting Silently.");
           }
           queue_write(d, "\r\n", 2);
           process_output(d);
        }
     }
     if ( f ) {
        fprintf(f, "%d\n", player);
        fclose(f);
     }
  } else {
    raw_broadcast(0, 0, "Game: Restart by %s.", Name(Owner(player)));
    raw_broadcast(0, 0, "Game: Your connection will pause, but will remain connected. Please wait...");
  }
  if ( mudstate.shutdown_flag ) {
     raw_broadcast(0, 0, "Game: Signal USR2 caught in middle of reboot.  Shutting down the game.");
     do_shutdown(NOTHING, NOTHING, 0, (char *)"Caught signal SIGUSR2");
  }
  STARTLOG(LOG_ALWAYS, "WIZ", "RBT")
    log_text((char *) "Reboot by ");
    log_name(player);
  ENDLOG
  mudstate.reboot_flag = port;
  VOIDRETURN; /* #80 */
}

static void 
dump_database_internal(int panic_dump)
{
    char *dmpfile, *outdbfile, *tmpfile, *outfn, *prevfile;
    FILE *f;

    DPUSH; /* #82 */
    
    /* This is broke with timers, I don't know why yet */
    ignore_signals(); 	 /* Stop handling signals. */
 
    dmpfile = alloc_mbuf("dump_database_internal");
    outdbfile = alloc_mbuf("dump_database_internal");
    tmpfile = alloc_mbuf("dump_database_internal");
    outfn = alloc_mbuf("dump_database_internal");
    prevfile = alloc_mbuf("dump_database_internal");

    sprintf(dmpfile,   "%.128s/%.150s", mudconf.data_dir, mudconf.crashdb);
    sprintf(outdbfile, "%.128s/%.150s", mudconf.data_dir, mudconf.outdb);
    sprintf(prevfile,  "%.128s/%.150s.prev", mudconf.data_dir, mudconf.outdb);
    sprintf(tmpfile,   "%.128s/%.150s.#%d#", mudconf.data_dir, mudconf.outdb, mudstate.epoch - 1);
    sprintf(outfn,     "%s/%s.Z", mudconf.data_dir, mudconf.outdb);

    /* this could be in a child process, so don't do any debugging
       stack stuff (even for functions called from here) */

    if (panic_dump) {
	unlink(dmpfile);
	f = tf_fopen(dmpfile, O_WRONLY | O_CREAT | O_TRUNC);
	if (f != NULL) {
	    db_write(f, F_MUSH, OUTPUT_VERSION | OUTPUT_FLAGS);
	    tf_fclose(f);
	} else {
	    log_perror("DMP", "FAIL", "Opening crash file",
		       mudconf.crashdb);
	}
        /* This is broke, I don't know why yet */
        reset_signals(); /* Resume normal signal handling. */
        if ( mudstate.shutdown_flag ) {
           do_shutdown(NOTHING, NOTHING, 0, (char *)"Caught signal SIGUSR2");
        }
	VOIDRETURN; /* #82 */
    }

    unlink(tmpfile);		/* nuke our predecessor */
    sprintf(tmpfile, "%.128s/%.150s.-%d-", mudconf.data_dir, mudconf.outdb, mudstate.epoch);

#ifndef VMS
    if (mudconf.compress_db) {
	sprintf(tmpfile, "%.128s/%.150s.#%d#.Z", mudconf.data_dir, mudconf.outdb, mudstate.epoch - 1);
	unlink(tmpfile);
	sprintf(tmpfile, "%.128s/%.150s.#%d#.Z", mudconf.data_dir, mudconf.outdb, mudstate.epoch);
	f = tf_popen(unsafe_tprintf("%s > %s", mudconf.compress, tmpfile), O_WRONLY);
	if (f) {
	    db_write(f, F_MUSH, OUTPUT_VERSION | OUTPUT_FLAGS);
	    tf_pclose(f);
	    rename(outdbfile, prevfile);
	    if (rename(tmpfile, outfn) < 0)
		log_perror("SAV", "FAIL",
			   "Renaming output file to DB file",
			   tmpfile);
            /* This is broke, I don't know why yet */
            reset_signals(); 	/* Resume normal signal handling. */
            if ( mudstate.shutdown_flag ) {
               do_shutdown(NOTHING, NOTHING, 0, (char *)"Caught signal SIGUSR2");
            }
	    VOIDRETURN; /* #82 */
	} else {
	    log_perror("SAV", "FAIL", "Opening", tmpfile);
	}
    }
#endif /* VMS */

    f = tf_fopen(tmpfile, O_WRONLY | O_CREAT | O_TRUNC);
    if (f) {
	db_write(f, F_MUSH, OUTPUT_VERSION | OUTPUT_FLAGS);
	tf_fclose(f);
	rename(outdbfile, prevfile);
	if (rename(tmpfile, outdbfile) < 0)
	    log_perror("SAV", "FAIL",
		       "Renaming output file to DB file", tmpfile);
    } else {
	log_perror("SAV", "FAIL", "Opening", tmpfile);
    }

    free_mbuf(dmpfile);
    free_mbuf(outdbfile);
    free_mbuf(tmpfile);
    free_mbuf(outfn);
    free_mbuf(prevfile);
    local_dump(panic_dump);

    /* This is broke, I don't know why yet */
    reset_signals();  	/* Resume normal signal handling. */

    if ( mudstate.shutdown_flag ) {
       do_shutdown(NOTHING, NOTHING, 0, (char *)"Caught signal SIGUSR2");
    }
    
    VOIDRETURN; /* #82 */
}

void 
NDECL(dump_database)
{
    char *buff;

    mudstate.dumpstatechk=1;
    DPUSH; /* #83 */

    mudstate.epoch++;
    buff = alloc_mbuf("dump_database");
#ifndef VMS
    sprintf(buff, "%.150s/%.150s.#%d#", mudconf.data_dir, mudconf.outdb, mudstate.epoch);
#else
    sprintf(buff, "%.150s/%.150s.-%d-", mudconf.data_dir, mudconf.outdb, mudstate.epoch);
#endif	/* VMS */
    STARTLOG(LOG_DBSAVES, "DMP", "DUMP")
	log_text((char *) "Dumping: ");
    log_text(buff);
    ENDLOG
	pcache_sync();
    SYNC;
    dump_database_internal(0);
    STARTLOG(LOG_DBSAVES, "DMP", "DONE")
	log_text((char *) "Dump complete: ");
    log_text(buff);
    ENDLOG
    free_mbuf(buff);
    mudstate.dumpstatechk=0;
    VOIDRETURN; /* #83 */
}

void 
fork_and_dump(int key, char *msg)
{
  int child;
  char *buff;
  char *flatfilename;
  FILE *f;

  /* If we're in the middle of a kill -USR2 don't dump -- the shutdown will take care of it */
  if ( mudstate.forceusr2 )
     return;

  mudstate.dumpstatechk=1;
  DPUSH; /* #84 */
  /* always sync up working files before dumping a flatfile database */
  if( key & DUMP_FLAT ) {
    key |= DUMP_STRUCT | DUMP_TEXT;
  }

  if (*mudconf.dump_msg)
    raw_broadcast(0, NO_WALLS, "%s", mudconf.dump_msg);

  mudstate.epoch++;
  buff = alloc_mbuf("fork_and_dump");
#ifndef VMS
  sprintf(buff, "%.150s/%.150s.#%d#", mudconf.data_dir, mudconf.outdb, mudstate.epoch);
#else
  sprintf(buff, "%.150s/%.150s.-%d-", mudconf.data_dir, mudconf.outdb, mudstate.epoch);
#endif	/* VMS */
  STARTLOG(LOG_DBSAVES, "DMP", "CHKPT")
    if (!key || (key & DUMP_TEXT)) {
      log_text((char *) "SYNCing");
      if (!key || (key & DUMP_STRUCT))
        log_text((char *) " and ");
    }
    if (!key || (key & DUMP_STRUCT)) {
      log_text((char *) "Checkpointing: ");
      log_text(buff);
    }
  ENDLOG
  DPOP; /* #84 */
  DPUSH; /* #85 */
  free_mbuf(buff);
  al_store();			/* Save cached modified attribute list */
  if (!key || (key & DUMP_TEXT))
    pcache_sync();

  SYNC;
  DPOP; /* #85 */
  if (!key || (key & DUMP_STRUCT)) {
#ifndef VMS
    if (mudconf.fork_dump) {
      if (mudconf.fork_vfork) {
	child = vfork();
      } else {
	child = fork();
      }
    } else {
      child = 0;
    }
#else
    child = 0;
#endif	/* VMS */

    if (child == 0) {
      dump_database_internal(0);

      if (mudconf.fork_dump)
        _exit(0);

      /* now that all of the requisite syncing has been performed, dump
         a flat file version of the database if one is requested */

      if( key & DUMP_FLAT ) {
        flatfilename = alloc_mbuf("dump/flat");

        if ( !msg || !*msg ) 
           sprintf(flatfilename, "%.150s/%.150s.flat", mudconf.data_dir, mudconf.indb);
        else
           sprintf(flatfilename, "%.150s/%.150s.flat", mudconf.data_dir, msg);

	f = fopen(flatfilename, "w");

        free_mbuf(flatfilename);

        if (f) {
          /* This is broke with timers, I don't know why yet */
          ignore_signals(); /* Ignore signals while dumping. */
          STARTLOG(LOG_DBSAVES, "DMP", "FLAT")
            log_text((char*)"Dumping db to flatfile...");
          ENDLOG
          db_write(f, F_MUSH, OUTPUT_VERSION | UNLOAD_OUTFLAGS);
          STARTLOG(LOG_DBSAVES, "DMP", "FLAT")
            log_text((char*)"Dump complete.");
          ENDLOG
          fclose(f);
          time(&mudstate.mushflat_time);
          /* This is broke, I don't know why yet */
           reset_signals(); /* All done, resume signal handling. */
        }
        else {
          STARTLOG(LOG_PROBLEMS, "DMP", "FLAT")
            log_text((char*)"Unable to open flatfile.");
          ENDLOG
        }
      }
      if (*mudconf.postdump_msg)
        raw_broadcast(0, NO_WALLS, "%s", mudconf.postdump_msg);

    } else if (child < 0) {
      log_perror("DMP", "FORK", NULL, "fork()");
    }
  }
  if ( mudstate.shutdown_flag ) {
     do_shutdown(NOTHING, NOTHING, 0, (char *)"Caught signal SIGUSR2");
  }
  mudstate.dumpstatechk=0;
}


static int 
FDECL(load_game, (int rebooting))
{
    FILE *f;
    int compressed;
    char *infile;
    char *newfile;
    struct stat statbuf;
    int db_format, db_version, db_flags;

    DPUSH; /* #86 */
    infile = alloc_mbuf("load_game");
    newfile = alloc_mbuf("load_game");

    f = NULL;
    compressed = 0;

    sprintf(infile, "%s/%s", mudconf.data_dir, mudconf.indb);

#ifndef VMS
    if (mudconf.compress_db) {
	strcat(infile, ".Z");
        if( rebooting ) {
          strcpy(newfile, infile);
          strcat(newfile, ".new.Z");
          rename(newfile, infile);
        }
	if (stat(infile, &statbuf) == 0) {
	    if ((f = tf_popen(unsafe_tprintf(" %s < %s",
			    mudconf.uncompress, infile), O_RDONLY)) != NULL)
		compressed = 1;
	}
    }
    else {
      if( rebooting ) {
	sprintf(newfile, "%s.new", infile);
        rename(newfile, infile);
        STARTLOG(LOG_ALWAYS, "RBT", "MOV")
          log_text((char*)"New db dump moved to input db.");
        ENDLOG
      }
    }
#endif	/* VMS */
    if (compressed == 0) {
	if ((f = tf_fopen(infile, O_RDONLY)) == NULL) {
	    RETURN(-1); /* #86 */
        }
    }
    /* ok, read it in */

    STARTLOG(LOG_STARTUP, "INI", "LOAD")
	log_text((char *) "Loading: ");
    log_text(infile);
    ENDLOG
    if (db_read(f, &db_format, &db_version, &db_flags) < 0) {
	STARTLOG(LOG_ALWAYS, "INI", "FATAL")
	    log_text((char *) "Error loading ");
	log_text(infile);
	ENDLOG
	RETURN(-1); /* #86 */
    }
    STARTLOG(LOG_STARTUP, "INI", "LOAD")
	log_text((char *) "Load complete.");
    ENDLOG

    /* everything ok */
#ifndef VMS
	if (compressed)
	tf_pclose(f);
    else
	tf_fclose(f);
#else
	tf_fclose(f);
#endif	/* VMS */
    free_mbuf(infile);
    free_mbuf(newfile);
    RETURN(0); /* #86 */
}

/* match a list of things */

int 
list_check(dbref thing, dbref player, char type, char *str, int check_parent, int dpcheck, int zone_chk)
{
    int match, limit, scheck;
    ZLISTNODE *zonelistnodeptr;

    DPUSH; /* #87 */

    match = 0;
    limit = mudstate.db_top;
    while (thing != NOTHING) {
#ifdef REALITY_LEVELS
        if ((thing != player) && IsReal(thing, player) && 
           ((Typeof(thing) != TYPE_PLAYER) || mudconf.match_pl)) {
#else
        if ((thing != player) && ((Typeof(thing) != TYPE_PLAYER) || mudconf.match_pl)) {
#endif
            /* Do zone command tests */
            if ( zone_chk && ZoneCmdChk(thing) ) {
               if( db[thing].zonelist && !ZoneMaster(thing)) {
                    for( zonelistnodeptr = db[thing].zonelist;
                         zonelistnodeptr;
                         zonelistnodeptr = zonelistnodeptr->next ) {
                       if ((scheck = atr_match(zonelistnodeptr->object, player,
                                               type, str, check_parent, dpcheck)) > 0) {
                          if (scheck > 1) {
                             match = 2;
                             break;
                          }
                          match = 1;
                       }
                       /* check for contents of zone masters */
                       if ( !(scheck <= 0) && Good_obj(zonelistnodeptr->object) &&
                            ZoneContents(zonelistnodeptr->object) ) {
                            scheck = list_check(Contents(zonelistnodeptr->object),
                                                player, type, str, 0, 0, 0);
                            if (scheck > 1) {
                               match = 2;
                               break;
                            } else if (scheck > 0) {
                               match = 1;
                            }
                       }
                    }
                 }
            } else if ( (scheck = atr_match(thing, player, type, str, check_parent, dpcheck)) > 0) {
	       if (scheck > 1) {
	         match = 2;
	         break;
	       }
	       match = 1;
            }
	}

	thing = Next(thing);
	if (--limit < 0){
	   RETURN(match); /* #87 */
        }
    }
    RETURN(match); /* #87 */
}

int 
Hearer(dbref thing)
{
    char *as, *buff, *s;
    dbref aowner, parent;
    int attr, aflags, lev;
    ATTR *ap;

    DPUSH; /* #88 */

    if (Connected(thing) || Puppet(thing)) {
       RETURN(1); /* #88 */
    }

    if (Monitor(thing))
	buff = alloc_lbuf("Hearer");
    else
	buff = NULL;
    atr_push();
    if ( (mudconf.listen_parents == 0) || !Monitor(thing) ) {
       for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
	   if (attr == A_LISTEN) {
	       if (buff)
		   free_lbuf(buff);
	       atr_pop();
	       RETURN(1); /* #88 */
	   }
	   if (Monitor(thing)) {
	       ap = atr_num(attr);
	       if (!ap || (ap->flags & AF_NOPROG))
		   continue;
   
	       atr_get_str(buff, thing, attr, &aowner, &aflags);
   
	       /* Make sure we can execute it */
   
	       if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
		   continue;
   
	       /* Make sure there's a : in it */
   
               for (s = buff + 1; *s && (*s != ':' || (*(s-1) == '\\' && *(s-2) != '\\')); s++);
            /* for (s = buff + 1; *s && (*s != ':'); s++); */
	       if (s) {
		   free_lbuf(buff);
		   atr_pop();
		   RETURN(1); /* #88 */
	       }
	   }
       }
    } else {
       ITER_PARENTS(thing, parent, lev) {
          for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
	      if ((thing == parent) && (attr == A_LISTEN)) {
	          if (buff)
		      free_lbuf(buff);
	          atr_pop();
	          RETURN(1); /* #88 */
	      }
	      if (Monitor(thing)) {
	          ap = atr_num(attr);
	          if (!ap || (ap->flags & AF_NOPROG))
		      continue;
      
	          atr_get_str(buff, parent, attr, &aowner, &aflags);
      
	          /* Make sure we can execute it */

                  if ( (thing != parent) && ((ap->flags & AF_PRIVATE) || (aflags & AF_PRIVATE)) )
                      continue;
      
	          if ((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
		      continue;
      
	          /* Make sure there's a : in it */
      
                  for (s = buff + 1; *s && (*s != ':' || (*(s-1) == '\\' && *(s-2) != '\\')); s++);
               /* for (s = buff + 1; *s && (*s != ':'); s++); */
	          if (s) {
		      free_lbuf(buff);
		      atr_pop();
		      RETURN(1); /* #88 */
	          }
	      }
          }
       }
    }
    if (buff)
	free_lbuf(buff);
    atr_pop();
    RETURN(0); /* #88 */
}

void 
do_rwho(dbref player, dbref cause, int key)
{
    DPUSH; /* #89 */
#ifdef RWHO_IN_USE
    if (key == RWHO_START) {
	if (!mudstate.rwho_on) {
	    rwhocli_setup(mudconf.rwho_host,
			  mudconf.rwho_info_port,
			  mudconf.rwho_pass, mudconf.mud_name,
			  mudstate.short_ver);
	    rwho_update();
	    if (!Quiet(player))
		notify(player, "RWHO transmission started.");
	    mudstate.rwho_on = 1;
	} else {
	    notify(player, "RWHO transmission already on.");
	}
    } else if (key == RWHO_STOP) {
	if (mudstate.rwho_on) {
	    rwhocli_shutdown();
	    if (!Quiet(player))
		notify(player, "RWHO transmission stopped.");
	    mudstate.rwho_on = 0;
	} else {
	    notify(player, "RWHO transmission already off.");
	}
    } else {
	notify(player, "Illegal combination of switches.");
    }
#else
    notify(player, "RWHO support has not been compiled in to the server.");
#endif
    VOIDRETURN; /* #89 */
}

void 
do_readcache(dbref player, dbref cause, int key)
{
    DPUSH; /* #90 */
    helpindex_load(player);
    fcache_load(player);
    VOIDRETURN; /* #90 */
}

static void 
NDECL(process_preload)
{
    dbref thing, parent, aowner;
    int aflags, lev, i_matchint;
    char *tstr, *tstr2, *s_strtok, *s_strtokr, *s_matchstr;
    FWDLIST *fp;

    DPUSH; /* #91 */

    fp = (FWDLIST *) alloc_lbuf("process_preload.fwdlist");
    tstr = alloc_lbuf("process_preload.string");
    tstr2 = alloc_lbuf("process_preload.string");
    DO_WHOLE_DB(thing) {

	/* Ignore GOING objects */

	if (Going(thing))
	    continue;

	do_top(10);

	/* Look for a STARTUP attribute in parents */

        /* Clear semaphores on objects */
        /* atr_clr(thing, A_SEMAPHORE); */

        if ( !mudconf.no_startup ) {
	   ITER_PARENTS(thing, parent, lev) {
	       if (Flags(thing) & HAS_STARTUP) {
		   did_it(Owner(thing), thing, 0, NULL, 0, NULL,
		          A_STARTUP, (char **) NULL, 0);
		   /* Process queue entries as we add them */
   
		   do_second();
		   do_top(10);
		   cache_reset(0);
		   break;
	       }
	   }
        }

	/* Look for a FORWARDLIST attribute */

        if (H_Protect(thing)) {
	    (void) atr_get_str(tstr, thing, A_PROTECTNAME,
			       &aowner, &aflags);
            if ( *tstr ) {
               strcpy(tstr2, tstr);
               s_strtok = strtok_r(tstr2, "\t", &s_strtokr);
               while ( s_strtok ) {
                  s_matchstr = strchr(s_strtok, ':');
                  if ( s_matchstr ) {
                     *s_matchstr = '\0';
                     i_matchint = atoi(s_matchstr+1);
                     if ( i_matchint == 1 ) {
                        protectname_add(s_strtok, thing);
                        protectname_alias(s_strtok, thing);
                     }
                  } else {
                     protectname_add(s_strtok, thing);
                  }
                  s_strtok = strtok_r(NULL, "\t", &s_strtokr);
               }
            }
        }
	if (H_Fwdlist(thing)) {
	    (void) atr_get_str(tstr, thing, A_FORWARDLIST,
			       &aowner, &aflags);
	    if (*tstr) {
		fwdlist_load(fp, GOD, tstr);
		if (fp->count > 0)
		    fwdlist_set(thing, fp);
	    }
	    cache_reset(0);
	}
    }
    free_lbuf(fp);
    free_lbuf(tstr);
    free_lbuf(tstr2);
    VOIDRETURN; /* #91 */
}

#ifndef VMS
int
#endif	/* VMS */
main(int argc, char *argv[])
{
    DESC *d;
    int mindb;
    char gdbmDbPath[300];
    int argidx;
    int got_config = 0;
    int rebooting = 0;
#ifndef NODEBUGMONITOR
    const int shmaddr;
    int shmid;
#endif

    db = NULL;
    dddb_var_init();
    cache_var_init(); 


    fclose(stdin);  
/*    fclose(stdout); */

#if defined(HAVE_IEEEFP_H) && defined(HAVE_SYS_UCONTEXT_H)
    /* Inhibit IEEE fp exception on overflow */

    fpsetmask(fpgetmask() & ~FP_X_OFL);
#endif

    tf_init();
    mindb = 0;			/* Are we creating a new db? */
    time(&mudstate.start_time);
    time(&mudstate.reboot_time);
    time(&mudstate.mushflat_time);
    time(&mudstate.aregflat_time);
    time(&mudstate.newsflat_time);
    time(&mudstate.mailflat_time);

    pool_init(POOL_LBUF, LBUF_SIZE);
    pool_init(POOL_MBUF, MBUF_SIZE);
    pool_init(POOL_SBUF, SBUF_SIZE);
    pool_init(POOL_BOOL, sizeof(struct boolexp));

    pool_init(POOL_DESC, sizeof(DESC));
    pool_init(POOL_QENTRY, sizeof(BQUE));
    pool_init(POOL_ZLISTNODE, sizeof(ZLISTNODE));

    init_pid_table();
    tcache_init();
    pcache_init();
    cf_init();
    if ( (argc > 1) && !strcmp(argv[1], "-REB00T") )
       mudstate.log_chk_reboot = 1;
    init_logfile();
    mudstate.log_chk_reboot = 0;
    init_rlimit();
    init_cmdtab();
    init_logout_cmdtab();
    init_flagtab();
    init_toggletab();
    init_powertab();
    init_depowertab();
    init_functab();
    init_ansitab();
    init_attrtab();
    init_version();
#ifdef ENABLE_DOORS
    initDoorSystem();
#endif
    hashinit(&mudstate.player_htab, 521);
    nhashinit(&mudstate.fwdlist_htab, 131);
    nhashinit(&mudstate.parent_htab, 131);
    nhashinit(&mudstate.desc_htab, 131);
#ifdef HAS_OPENSSL
    OpenSSL_add_all_digests();
#endif
    /* Clean the conf to avoid naughtyness */
    unlink("rhost_vattr.conf");

    for( argidx = 1; argidx < argc; argidx++ ) {
      if( !strcmp(argv[argidx], "-s") ) {
        STARTLOG(LOG_ALWAYS, "INI", "ARG")
          log_text((char*) "New minimal database selected.");
        ENDLOG
	mindb = 1;
      }
      else if( !strcmp(argv[argidx], "-REB00T") ) {
        STARTLOG(LOG_ALWAYS, "INI", "ARG")
          log_text((char*) "Rebooting...");
        ENDLOG
        rebooting = 1;
      }
      else if( argidx == argc - 1 ) {
	cf_read(argv[argidx]);
        got_config = argidx;
      }
      else {
        /* don't describe the reboot flag to users */
	fprintf(stderr, "Usage: %s [-s] [config-file]\n", argv[0]);
	exit(1);
      }
    }

    if( !got_config ) {
      cf_read((char *) CONF_FILE);
    }

    fcache_init();
    helpindex_init();

#ifndef NODEBUGMONITOR
    debugmem = shmConnect(mudconf.debug_id, 0, &shmid);   
    if (debugmem != NULL) { /* i.e. We could allocate the shm segment */

      if (debugmem->debugport != getppid()) {
	  fprintf(stderr,
		  "debug_id clash detected:\n"
		  "** A mush on port '%d' is already using debug_id '%d' **\n",
		  debugmem->mushport, mudconf.debug_id);
	  /* detach from this shared memory segment */
#ifdef SOLARIS
	  if (shmdt( (char *) (&shmaddr)) < 0) {
#elif BSD_LIKE
		if (shmdt( (void *) (&shmaddr)) < 0){
#else
	  if (shmdt( (const void *) (&shmaddr)) < 0) {
#endif
	    fprintf(stderr, 
		    "** Could not disconnect from shared memory segment! **\n");
	  }
      } else {
	debugmem->mushport = mudconf.port;
	fprintf(stderr, 
		"Slave IPC Debugging stack connected w/ key = %d\n",
		mudconf.debug_id);
      }
    }
    /* It might get set to NULL by the above if statement */
    if (debugmem == NULL) {
      fprintf(stderr, "** IPC Debugging disabled **\n");
      debugmem = (Debugmem *)malloc(sizeof(Debugmem));
      
      if( !debugmem ) {
        fprintf(stderr, "Unable to allocate fake debug memory.\n");
        abort();
      }
      INITDEBUG(debugmem);
    } 
#else
    fprintf(stderr, "** IPC Debugging disabled per request **\n");
    debugmem = (Debugmem *)malloc(sizeof(Debugmem));
    if( !debugmem ) {
      fprintf(stderr, "Unable to allocate fake debug memory.\n");
      abort();
    }
    INITDEBUG(debugmem);

#endif /* !NODEBUGMONITOR */

    DPUSH; /* #92 */

    sprintf(gdbmDbPath, "%s/%s", mudconf.data_dir, mudconf.gdbm);
    if (mindb)
	unlink(gdbmDbPath);
    if (init_gdbm_db(gdbmDbPath) < 0) {
	STARTLOG(LOG_ALWAYS, "INI", "LOAD")
	    log_text((char *) "Couldn't load text database: ");
	log_text(mudconf.gdbm);
	ENDLOG
        DPOP; /* #92 */
	exit(2);
    }
    if (mindb)
	db_make_minimal();
    else if (load_game(rebooting) < 0) {
	STARTLOG(LOG_ALWAYS, "INI", "LOAD")
	    log_text((char *) "Couldn't load: ");
	log_text(mudconf.indb);
	ENDLOG
	DPOP; /* #92 */
	exit(2);
    }
    srandom(getpid());
    set_signals();

    /* initialize the buffers and variables */
    for (mindb = 0; mindb < MAX_GLOBAL_REGS; mindb++) {
	mudstate.global_regs[mindb] = alloc_lbuf("main.global_reg");
	mudstate.global_regsname[mindb] = alloc_sbuf("main.global_regname");
	mudstate.global_regs_backup[mindb] = alloc_lbuf("main.global_regbkup");
    }

    /* Do a consistency check and set up the freelist */
    do_dbck(NOTHING, NOTHING, 0);

    /* Reset all the hash stats */

    hashreset(&mudstate.command_htab);
    hashreset(&mudstate.command_vattr_htab);
    hashreset(&mudstate.logout_cmd_htab);
    hashreset(&mudstate.func_htab);
    hashreset(&mudstate.toggles_htab);
    hashreset(&mudstate.powers_htab);
    hashreset(&mudstate.depowers_htab);
    hashreset(&mudstate.flags_htab);
    hashreset(&mudstate.attr_name_htab);
    nhashreset(&mudstate.attr_num_htab);
    hashreset(&mudstate.player_htab);
    nhashreset(&mudstate.fwdlist_htab);
    hashreset(&mudstate.news_htab);
    hashreset(&mudstate.help_htab);
#ifdef PLUSHELP
    hashreset(&mudstate.plushelp_htab);
#endif
    hashreset(&mudstate.wizhelp_htab);
    hashreset(&mudstate.error_htab);
    nhashreset(&mudstate.desc_htab);

/*  Missing hash resets */
    hashreset(&mudstate.cmd_alias_htab);
    hashreset(&mudstate.ufunc_htab);
    hashreset(&mudstate.ulfunc_htab);
    nhashreset(&mudstate.parent_htab);
    hashreset(&mudstate.ansi_htab);

    mudstate.nowmsec = time_ng(NULL);
    mudstate.now = (time_t) floor(mudstate.nowmsec);
    mudstate.lastnowmsec = mudstate.nowmsec;
    mudstate.lastnow = mudstate.now;
    mudstate.evalnum = 0;
    mudstate.guest_num = 0;
    mudstate.guest_status = 0;
    mudstate.free_num = NOTHING;
    mudstate.nuke_status = 0;
    mudstate.exitcheck = 0;
    mudstate.droveride = 0;
    mudstate.scheck = 0;
    mudstate.mail_state = mail_init();
    mudstate.autoreg = areg_init();
    start_news_system();
    val_count();

    /* Load in the command hashes for vattrs before startup foo */
    cf_read((char *)"rhost_vattr.conf");
    unlink("rhost_vattr.conf");

    process_preload();
    if (mudconf.rwho_transmit)
	do_rwho(NOTHING, NOTHING, RWHO_START);

    /* Reset #1's password if value is right */
    if ( mudconf.newpass_god == 777 ) {
       if ( Good_chk(GOD) ) {
          s_Pass(GOD, mush_crypt((const char *)"Nyctasia", 1));
          STARTLOG(LOG_ALWAYS, "WIZ", "PASS")
             log_text((char *) "GOD password reset to 'Nyctasia'");
          ENDLOG
       }
       mudconf.newpass_god = 0;
    }
    /* go do it */


    mudstate.nowmsec = time_ng(NULL);
    mudstate.now = (time_t) floor(mudstate.nowmsec);
    mudstate.lastnowmsec = mudstate.nowmsec;
    mudstate.lastnow = mudstate.now;
    init_timer();

    if( rebooting ) {
      if( !load_reboot_db() ) {
	DPOP; /* #92 */
        exit(1);
      }
    }
    local_startup();
    /* --- main mush loop --- */
    shovechars(mudconf.port, mudconf.ip_address);
    /* --- end main mush loop --- */
    local_shutdown();
    rebooting = mudstate.reboot_flag;

    if (!rebooting)
      close_sockets(0, (char *) "Going down - Bye");
    else {
      close_main_socket();
      DESC_ITER_CONN(d) {
	freeqs(d,0);
      }
    }

    dump_database();
/* MMMail Mail db close */
    mail_close();
    areg_close();
    shutdown_news_system();
    CLOSE;
    close_logfile();

    if( rebooting ) {
      if( dump_reboot_db() ) {
        if( got_config ) {
          DPOP; /* #92 */
          execl(argv[0], argv[0], "-REB00T", argv[got_config], NULL);
        }
        else {
          DPOP; /* #92 */
          execl(argv[0], argv[0], "-REB00T", NULL);
        }

        /* original process ends here on normal execl */
        DPUSH; /* #93 */
        STARTLOG(LOG_PROBLEMS, "RBT", "EXCL")
          log_text((char *) "Failed to exec new process");
        ENDLOG
	DPOP; /* #93 */
        exit(1);
      }
      DPOP; /* #92 */
      exit(1);
    }
        
    DPOP; /* #92 */
#ifndef VMS
  return(0);
#else
  return(0);
#endif
}

static void 
NDECL(init_rlimit)
{
#ifdef HAVE_SETRLIMIT
#ifdef RLIMIT_NOFILE
    struct rlimit *rlp;
    DPUSH; /* #94 */

    rlp = (struct rlimit *) alloc_lbuf("rlimit");

    if (getrlimit(RLIMIT_NOFILE, rlp)) {
	log_perror("RLM", "FAIL", NULL, "getrlimit()");
	free_lbuf(rlp);
	VOIDRETURN; /* #94 */
    }
    rlp->rlim_cur = rlp->rlim_max;
    if (setrlimit(RLIMIT_NOFILE, rlp))
	log_perror("RLM", "FAIL", NULL, "setrlimit()");
    free_lbuf(rlp);

    VOIDRETURN; /* #94 */
#endif /* RLIMIT_NOFILE */
#endif /* HAVE_SETRLIMIT */
}
