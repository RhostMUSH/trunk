/* rob.c -- Commands dealing with giving/taking/killing things or money */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"

void do_kill (dbref player, dbref cause, int key, char *what, char *costchar)
{
dbref	victim;
char	*buf1, *buf2;
int	cost;

	init_match(player, what, TYPE_PLAYER);
	match_neighbor();
	match_me();
	match_here();
	if (Wizard(player)) {
		match_player();
		match_absolute();
	}
	victim = match_result();

	switch (victim) {
	case NOTHING:
		notify(player, "I don't see that player here.");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		if ((Typeof(victim) != TYPE_PLAYER) &&
		    (Typeof(victim) != TYPE_THING)) {
			notify(player,
				"Sorry, you can only kill players and things.");
			break;
		}
		if ((Haven(Location(victim)) && !Wizard(player)) ||
		    (controls(victim, Location(victim)) &&
		     !controls(player, Location(victim))) ||
		    Immortal(victim)) {
			notify(player, "Sorry.");
			break;
		}
		if (key == KILL_SLAY) {
		  if (Builder(player) && Builder(victim)) {
			notify(player, "Sorry.");
			break;
		  }
		}

		/* go for it */

		cost = atoi(costchar);
		if (key == KILL_KILL) {
			if (HasPriv(victim,player,POWER_NOKILL,POWER4,NOTHING)) {
			  notify(player, "Sorry.");
			  break;
			}
			if (cost < mudconf.killmin)
				cost = mudconf.killmin;
			if (cost > mudconf.killmax)
				cost = mudconf.killmax;

			/* see if it works */

			if (!payfor(player, cost)) {
				notify(player,
					unsafe_tprintf("You don't have enough %s.",
						mudconf.many_coins));
				return;
			}
		} else {
			cost = 0;
		}

		if (!(((random() % mudconf.killguarantee) < cost) ||
		      (key == KILL_SLAY)) ||
		    Wizard(victim)) {

			/* Failure: notify player and victim only */

			notify(player, "Your murder attempt failed.");
			buf1 = alloc_lbuf("do_kill.failed");
			sprintf(buf1, "%s tried to kill you!", Name(player));
			notify_with_cause(victim, player, buf1);
			if (Suspect(player)) {
				strcpy(buf1, Name(player));
				if (player == Owner(player)) {
					raw_broadcast(0, WIZARD,
						"[Suspect] %s tried to kill %s(#%d).",
						buf1, Name(victim), victim);
				} else {
					buf2 = alloc_lbuf("do_kill.SUSP.failed");
					strcpy(buf2, Name(Owner(player)));
					raw_broadcast(0, WIZARD,
						"[Suspect] %s <via %s(#%d)> tried to kill %s(#%d).",
						buf2, buf1, player,
						Name(victim), victim);
					free_lbuf(buf2);
				}
			}
			free_lbuf(buf1);
			break;
		}

		/* Success!  You killed him */

		buf1 = alloc_lbuf("do_kill.succ.1");
		buf2 = alloc_lbuf("do_kill.succ.2");
		if (Suspect(player)) {
			strcpy(buf1, Name(player));
			if (player == Owner(player)) {
				raw_broadcast(0, WIZARD,
					"[Suspect] %s killed %s(#%d).",
					buf1, Name(victim), victim);
			} else {
				strcpy(buf2, Name(Owner(player)));
				raw_broadcast(0, WIZARD,
					"[Suspect] %s <via %s(#%d)> killed %s(#%d).",
					buf2, buf1, player, Name(victim),
					victim);
			}
		}
		sprintf(buf1, "You killed %s!", Name(victim));
		sprintf(buf2, "killed %s!", Name(victim));
		if (Typeof(victim) != TYPE_PLAYER)
			if (halt_que(NOTHING, victim) > 0)
				if (!Quiet(victim))
					notify(Owner(victim), "Halted.");
		did_it(player, victim, A_KILL, buf1, A_OKILL, buf2, A_AKILL,
			(char **)NULL, 0);

		/* notify victim */

		sprintf(buf1, "%s killed you!", Name(player));
		notify_with_cause(victim, player, buf1);

		/* Pay off the bonus */

		if (key == KILL_KILL) {
			cost /= 2; 	/* victim gets half */
			if (Pennies(Owner(victim)) < mudconf.paylimit) {
				sprintf(buf1,
					"Your insurance policy pays %d %s.",
					cost, mudconf.many_coins);
				notify(victim, buf1);
				giveto(Owner(victim), cost, NOTHING);
			} else {
				notify(victim,
					"Your insurance policy has been revoked.");
			}
		}
		free_lbuf(buf1);
		free_lbuf(buf2);

		/* send him home */

		move_via_generic(victim, HOME, NOTHING, 0);
		divest_object(victim);
		break;
	}
}

/* ---------------------------------------------------------------------------
 * give_thing, give_money, do_give: Give away money or things.
 */

static void give_thing (dbref giver, dbref recipient, int key, char *what)
{
dbref	thing, loc;
char	*str, *sp;

	init_match(giver, what, TYPE_THING);
	match_possession();
	match_me();
	thing = match_result();

	switch (thing) {
	case NOTHING:
		notify(giver, "You don't have that!");
		return;
	case AMBIGUOUS:
		notify(giver, "I don't know which you mean!");
		return;
	}

	if (thing == giver) {
		notify(giver, "You can't give yourself away!");
		return;
	}

	if (((Typeof(thing) != TYPE_THING) &&
	     (Typeof(thing) != TYPE_PLAYER)) ||
	    !(Enter_ok(recipient) || controls(giver, recipient))) {
		notify(giver, "Permission denied.");
		return;
	}
	if ( (mudstate.remotep == thing) || ((Flags3(thing) & NOMOVE) && !Wizard(giver)) ) {
		notify(giver, "Permission denied.");
		return;
	}

	if (!could_doit(giver, thing, A_LGIVE, 1, 0)) {
		sp = str = alloc_lbuf("do_give.gfail");
		safe_str((char *)"You can't give ", str, &sp);
		safe_str(Name(thing), str, &sp);
		safe_str((char *)" away.", str, &sp);
		*sp = '\0';

		did_it(giver, thing, A_GFAIL, str, A_OGFAIL, NULL,
			A_AGFAIL, (char **)NULL, 0);
		free_lbuf(str);
		return;
	}
	if (!could_doit(thing, recipient, A_LRECEIVE, 1, 0)) {
		sp = str = alloc_lbuf("do_give.rfail");
		safe_str(Name(recipient), str, &sp);
		safe_str((char *)" doesn't want ", str, &sp);
		safe_str(Name(thing), str, &sp);
		safe_chr('.', str, &sp);
		*sp = '\0';

		did_it(giver, recipient, A_RFAIL, str, A_ORFAIL, NULL,
			A_ARFAIL, (char **)NULL, 0);
		free_lbuf(str);
		return;
	}
	loc = Location(giver);
	if ( !Good_obj(loc) || loc == NOTHING || loc == AMBIGUOUS || Recover(loc) || Going(loc) )
           loc = giver;
	if (!could_doit(giver, loc, A_LGIVETO, 1, 0)) {
		sp = str = alloc_lbuf("do_giveto.rfail");
		safe_str((char *)"You can not give ", str, &sp);
		safe_str(Name(thing), str, &sp);
		safe_str((char *)" away at this location.", str, &sp);
		*sp = '\0';
		notify(giver, str);
		free_lbuf(str);
		return;
	}

	move_via_generic(thing, recipient, giver, 0);
	divest_object(thing);
	
	if (!(key & GIVE_QUIET)) {
		str = alloc_lbuf("do_give.thing.ok");
		strcpy(str, Name(giver));
		notify_with_cause(recipient, giver,
			unsafe_tprintf("%s gave you %s.", str, Name(thing)));
		notify(giver, "Given.");
		notify_with_cause(thing, giver,
			unsafe_tprintf("%s gave you to %s.", str, Name(recipient)));
		free_lbuf(str);
	}
	else {
		notify(giver, "Given. (quiet)");
	}
	did_it(giver, thing, A_DROP, NULL, A_ODROP, NULL, A_ADROP,
		(char **)NULL, 0);
	did_it(recipient, thing, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC,
		(char **)NULL, 0);
}

static void give_money (dbref giver, dbref recipient, int key, int amount)
{
dbref	aowner;
int	cost, pcost, rcost, aflags, dpamount;
char	*str;

	/* do amount consistency check */

	if (amount < 0 && ((!Builder(giver) && !HasPriv(giver,recipient,POWER_STEAL,POWER3,NOTHING)) ||
				DePriv(giver,recipient,DP_STEAL,POWER6,NOTHING))) {
		notify(giver,
			unsafe_tprintf("You look through your pockets. Nope, no negative %s.",
				mudconf.many_coins));
		return;
	}

	if (!amount) {
		notify(giver,
			unsafe_tprintf("You must specify a positive number of %s.",
							mudconf.many_coins));
		return;
	}

	dpamount = 0;
	if (amount < 0) 
	  dpamount = DePriv(giver,NOTHING,DP_NOSTEAL,POWER7,POWER_LEVEL_NA);
	else
	  dpamount = DePriv(giver,NOTHING,DP_NOGOLD,POWER7,POWER_LEVEL_NA);
	if (dpamount) {
	  if (DPShift(giver))
	    dpamount--;
	  dpamount = mudconf.money_limit[dpamount];
	}
	else
	  dpamount = -1;
	if (!Admin(Owner(giver))) {
		if ((Typeof(recipient) == TYPE_PLAYER) && (Pennies(recipient) + amount > mudconf.paylimit)) {
			notify(giver,
				unsafe_tprintf("That player doesn't need that many %s!",
					mudconf.many_coins));
			return;
		}
		if ((Typeof(recipient) != TYPE_PLAYER) && (!could_doit(giver, recipient, A_LUSE, 1, 1))) {
			notify(giver,
				unsafe_tprintf("%s won't take your money.",
					Name(recipient)));
			return;
		}
	}

	str = atr_get(Owner(giver), A_PAYLIM, &aowner, &aflags);
	pcost = atoi(str);
	free_lbuf(str);
	if (!Immortal(Owner(giver)) && pcost) {
		if ((Typeof(recipient) == TYPE_PLAYER) && (amount > 0) &&
			 (Pennies(recipient) + amount > pcost)) {
			notify(giver,
				unsafe_tprintf("That player doesn't need that many %s!",
					mudconf.many_coins));
			return;
		}
		else if (Pennies(recipient) + amount < (-pcost)) {
			notify(giver,"That player doesn't need that much debt!");
			return;
		}
	}
        str = atr_get(Owner(recipient), A_RECEIVELIM, &aowner, &aflags);
        rcost = atoi(str);
        free_lbuf(str);
	if (!Immortal(Owner(giver)) && rcost) {
		if ((Typeof(recipient) == TYPE_PLAYER) && (amount > 0) &&
			 (Pennies(recipient) + amount > rcost)) {
			notify(giver,
				unsafe_tprintf("That player doesn't need that many %s!",
					mudconf.many_coins));
			return;
		}
		else if (Pennies(recipient) + amount < (-rcost)) {
			notify(giver,"That player doesn't need that much debt!");
			return;
		}
	}

	if (!Immortal(Owner(giver))) {
	  if (dpamount >= 0) {
	    if (amount > 0) {
	      if ((Typeof(recipient) == TYPE_PLAYER) && (Pennies(recipient) + amount > dpamount)) {
		notify(giver,
			unsafe_tprintf("That player doesn't need that many %s!",
				mudconf.many_coins));
		return;
	      }
	      else if (amount > dpamount) {
		notify(giver, "Permission denied.");
		return;
	      }
	    }
	    else {
	      if ((Typeof(recipient) == TYPE_PLAYER) && (Pennies(recipient) + amount < (-dpamount))) {
		notify(giver,
			unsafe_tprintf("That player doesn't need that many %s!",
				mudconf.many_coins));
		return;
	      }
	      else if (amount < (-dpamount)) {
		notify(giver, "Permission denied.");
		return;
	      }
	    }
	  }
	}

	/* try to do the give */

	if (!payfor_give(giver, amount)) {
		notify(giver,
			unsafe_tprintf("You don't have that many %s to give!",
				mudconf.many_coins));
		return;
	}

	/* Find out cost if an object */

	if (Typeof(recipient) == TYPE_THING) {
		str = atr_pget(recipient, A_COST, &aowner, &aflags);
		cost = atoi(str);
		free_lbuf(str);

		/* Can't afford it? */

		if (amount < cost) {
			notify(giver, "Feeling poor today?");
			giveto(giver, amount, NOTHING);
			return;
		}

		/* Negative cost */

		if (cost < 0) {
			return;
		}
	} else {
		cost = amount;
	}

	if (!(key & GIVE_QUIET)) {
		if (amount == 1) {
			notify(giver,
				unsafe_tprintf("You give a %s to %s.",
					mudconf.one_coin, Name(recipient)));
			notify_with_cause(recipient, giver,
				unsafe_tprintf("%s gives you a %s.", Name(giver),
					mudconf.one_coin));
		} else {
			notify(giver,
				unsafe_tprintf("You give %d %s to %s.", amount,
					mudconf.many_coins, Name(recipient)));
			notify_with_cause(recipient, giver,
				unsafe_tprintf("%s gives you %d %s.", Name(giver),
				amount, mudconf.many_coins));
		}
	}
	else {
		if (amount == 1) {
			notify(giver,
				unsafe_tprintf("You give a %s to %s. (quiet)",
					mudconf.one_coin, Name(recipient)));
		}
		else {
			notify(giver,
				unsafe_tprintf("You give %d %s to %s. (quiet)",
					amount, mudconf.many_coins,
					Name(recipient)));
		}
	}

	/* Report change given */

	if((amount - cost) == 1) {
        	notify(giver,
			unsafe_tprintf("You get 1 %s in change.", mudconf.one_coin));
		giveto(giver, 1, NOTHING);
	} else if (amount != cost) {
		notify(giver,
			unsafe_tprintf("You get %d %s in change.",
				(amount - cost), mudconf.many_coins));
		giveto(giver, (amount - cost), NOTHING);
	}
	if (pcost && (Pennies(Owner(recipient)) + cost > pcost)) {
	  pcost = pcost - Pennies(Owner(recipient));
	  if (pcost < 0)
	    pcost = 0;
	}
	else
	  pcost = cost;
	if (!giveto(recipient, pcost, giver))
		giveto(giver, cost, NOTHING);

	/* Transfer the money and run PAY attributes */
        /* Rooms should not kick off the PAY attribute */

        if ( !isRoom(giver) )
	   did_it(giver, recipient, A_PAY, NULL, A_OPAY, NULL, A_APAY,
	   	  (char **)NULL, 0);
	return;
}

void do_give(dbref player, dbref cause, int key, char *who, char *amnt)
{
  dbref	recipient;

	/* check recipient */

	init_match(player, who, TYPE_PLAYER);
	match_neighbor();
	match_possession();
	match_me();
	if (Privilaged(player) || HasPriv(player,NOTHING,POWER_LONG_FINGERS,POWER3,NOTHING)) {
		match_player();
		match_absolute();
	}

	recipient = match_result();
	switch (recipient) {
	case NOTHING:
		notify(player, "Give to whom?");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		return;
	}

	if (DePriv(player,recipient,DP_GIVE,POWER7,POWER_LEVEL_SPC)) {
		notify(player, "Permission denied.");
		return;
	}
	if (DePriv(recipient,player,DP_RECEIVE,POWER7,POWER_LEVEL_SPC)) {
		notify(player, "Permission denied.");
		return;
	}
	if (is_number(amnt)) {
		give_money(player, recipient, key, atoi(amnt));
	} else if(Guest(recipient)) {
		notify(player, "Guest really doesn't need money or anything.");
		return;
	} else {
            if ( Typeof(player) != TYPE_ROOM )
		give_thing(player, recipient, key, amnt);
            else
                notify(player, "Command incompatible with invoker type.");
	}
}
