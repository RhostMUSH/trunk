/* boolexp.c */

#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "match.h"
#include "mudconf.h"
#include "externs.h"
#include "interface.h"
#include "flags.h"
#include "alloc.h"

#ifndef STANDALONE

static int parsing_internal = 0;

/* ---------------------------------------------------------------------------
 * check_attr: indicate if attribute ATTR on player passes key when checked by
 * the object lockobj
 */

static int 
check_attr(dbref player, dbref lockobj, ATTR * attr, char *key)
{
    char *buff;
    dbref aowner;
    int aflags, checkit;

    buff = atr_pget(player, attr->number, &aowner, &aflags);
    checkit = 0;

    if (See_attr(lockobj, player, attr, aowner, aflags, 0)) {
	checkit = 1;
    } else if (attr->number == A_NAME) {
	checkit = 1;
    }
    if (checkit && (!wild_match(key, buff, (char **) NULL, 0, 1))) {
	checkit = 0;
    }
    free_lbuf(buff);
    return checkit;
}

int 
eval_boolexp(dbref player, dbref thing, dbref from, BOOLEXP * b, int i_evaltype)
{
    dbref aowner, obj, source;
    int aflags, c, checkit, boolchk, lockchk;
    char *key, *buff, *buff2, *mybuff[3];
    ATTR *a;

    if (b == TRUE_BOOLEXP)
	return 1;

    lockchk = 0;
    switch (b->type) {
    case BOOLEXP_AND:
	return (eval_boolexp(player, thing, from, b->sub1, i_evaltype) &&
		eval_boolexp(player, thing, from, b->sub2, i_evaltype));
    case BOOLEXP_OR:
	return (eval_boolexp(player, thing, from, b->sub1, i_evaltype) ||
		eval_boolexp(player, thing, from, b->sub2, i_evaltype));
    case BOOLEXP_NOT:
	return !eval_boolexp(player, thing, from, b->sub1, i_evaltype);
    case BOOLEXP_INDIR:
	/*
	 * BOOLEXP_INDIR (i.e. @) is a unary operation which is replaced at
	 * evaluation time by the lock of the object whose number is the
	 * argument of the operation.
	 */

	mudstate.lock_nest_lev++;
	if (mudstate.lock_nest_lev >= mudconf.lock_nest_lim) {
#ifndef STANDALONE
	    STARTLOG(LOG_BUGS, "BUG", "LOCK")
		log_name_and_loc(player);
	    log_text((char *) ": Lock exceeded recursion limit.");
	    ENDLOG
		notify(player, "Sorry, broken lock!");
#else
	    fprintf(stderr, "Lock exceeded recursion limit.\n");
#endif
	    mudstate.lock_nest_lev--;
	    return (0);
	}
	if ((b->sub1->type != BOOLEXP_CONST) || (b->sub1->thing < 0)) {
#ifndef STANDALONE
	    STARTLOG(LOG_BUGS, "BUG", "LOCK")
		log_name_and_loc(player);
	    buff = alloc_mbuf("eval_boolexp.LOG.indir");
	    sprintf(buff,
		    ": Lock had bad indirection (%c, type %d)",
		    INDIR_TOKEN, b->sub1->type);
	    log_text(buff);
	    free_mbuf(buff);
	    ENDLOG
		notify(player, "Sorry, broken lock!");
#else
	    fprintf(stderr, "Broken lock.\n");
#endif
	    mudstate.lock_nest_lev--;
	    return (0);
	}
        if ( mudconf.parent_control )
	   key = atr_pget(b->sub1->thing, A_LOCK, &aowner, &aflags);
        else
	   key = atr_get(b->sub1->thing, A_LOCK, &aowner, &aflags);
	c = eval_boolexp_atr(player, b->sub1->thing, from, key,1, i_evaltype);
	free_lbuf(key);
	mudstate.lock_nest_lev--;
	return (c);
    case BOOLEXP_CONST:
	return (b->thing == player ||
		member(b->thing, Contents(player)));
    case BOOLEXP_ATR:
	a = atr_num_bool(b->thing);
	if (!a)
	    return 0;		/* no such attribute */

	/* First check the object itself, then its contents */

	if (check_attr(player, from, a, (char *) b->sub1))
	    return 1;
	DOLIST(obj, Contents(player)) {
	    if (check_attr(obj, from, a, (char *) b->sub1))
		return 1;
	}
	return 0;
    case BOOLEXP_EVAL:
	a = atr_num_bool(b->thing);
	if (!a)
	    return 0;		/* no such attribute */
	source = from;
	buff = atr_pget(from, a->number, &aowner, &aflags);
	if (!buff || !*buff) {
	    free_lbuf(buff);
	    buff = atr_pget(thing, a->number, &aowner, &aflags);
	    source = thing;
	}
	checkit = 0;
	if (Read_attr(source, source, a, aowner, aflags, 1)) {
	    checkit = 1;
	} else if (a->number == A_NAME) {
	    checkit = 1;
	}
	if (checkit) {
            boolchk = mudstate.inside_locks;
            if (Toggles2(source) & TOG_SILENTEFFECTS) {
               checkit = 0;
            } else {
               mudstate.inside_locks = 1;
               mudstate.chkcpu_locktog = 0;
               if ( mudstate.chkcpu_toggle )
                  lockchk = 1;
               mybuff[0] = alloc_sbuf("boolexp_eval");
               mybuff[1] = alloc_lbuf("boolexp_argpass");
               mybuff[2] = NULL;
               sprintf(mybuff[0], "%d", i_evaltype);
               /* alloc_lbuf is already null terminated -- this copies the value *unevaluated* as %1 */
               if ( b && *((char *)b->sub1) ) {
                  strcpy(mybuff[1], (char *)b->sub1);
               }
	       buff2 = cpuexec(source, player, player, EV_FIGNORE | EV_EVAL | EV_TOP,
			       buff, mybuff, 2, (char **)NULL, 0);
               free_sbuf(mybuff[0]);
               free_lbuf(mybuff[1]);
               if ( mudstate.chkcpu_toggle && !lockchk )
                  mudstate.chkcpu_locktog = 1;
               mudstate.inside_locks = boolchk;
	       checkit = !string_compare(buff2, (char *) b->sub1);
	       free_lbuf(buff2);
            }
	}
	free_lbuf(buff);
	return checkit;
    case BOOLEXP_IS:

	/* If an object check, do that */

	if (b->sub1->type == BOOLEXP_CONST)
	    return (b->sub1->thing == player);

	/* Nope, do an attribute check */

	a = atr_num_bool(b->sub1->thing);
	if (!a)
	    return 0;
	return (check_attr(player, from, a, (char *) (b->sub1)->sub1));
    case BOOLEXP_CARRY:

	/* If an object check, do that */

	if (b->sub1->type == BOOLEXP_CONST)
	    return (member(b->sub1->thing, Contents(player)));

	/* Nope, do an attribute check */

	a = atr_num_bool(b->sub1->thing);
	if (!a)
	    return 0;
	DOLIST(obj, Contents(player)) {
	    if (check_attr(obj, from, a, (char *) (b->sub1)->sub1))
		return 1;
	}
	return 0;
    case BOOLEXP_OWNER:
	return (Owner(b->sub1->thing) == Owner(player));
    default:
	abort();		/* bad type */
	return 0;
    }
}

int 
eval_boolexp_atr(dbref player, dbref thing, dbref from, char *key, int def, int i_evaltype)
{
    BOOLEXP *b;
    int ret_value;

    b = parse_boolexp(player, key, 1);
    if (b == NULL) {
	ret_value = def;
    } else {
	ret_value = eval_boolexp(player, thing, from, b, i_evaltype);
	free_boolexp(b);
    }
    return (ret_value);
}


#endif /* STANDALONE */

/* If the parser returns TRUE_BOOLEXP, you lose */
/* TRUE_BOOLEXP cannot be typed in by the user; use @unlock instead */

static const char *parsebuf;
static char parsestore[LBUF_SIZE];
static dbref parse_player;

static void 
NDECL(skip_whitespace)
{
    while (*parsebuf && isspace((int)*parsebuf))
	parsebuf++;
}

static BOOLEXP *NDECL(parse_boolexp_E);		/* defined below */

static BOOLEXP *
test_atr(char *s)
{
    ATTR *attrib;
    BOOLEXP *b;
    char *buff, *s1;
    int anum, locktype;

    buff = alloc_lbuf("test_atr");
    strcpy(buff, s);
    for (s = buff; *s && (*s != ':') && (*s != '/'); s++);
    if (!*s) {
	free_lbuf(buff);
	return ((BOOLEXP *) NULL);
    }
    if (*s == '/')
	locktype = BOOLEXP_EVAL;
    else
	locktype = BOOLEXP_ATR;

    *s++ = '\0';
    /* see if left side is valid attribute.  Access to attr is checked on eval
     * Also allow numeric references to attributes.  It can't hurt us, and
     * lets us import stuff that stores attr locks by number instead of by
     * name.
     */
    if (!(attrib = atr_str_bool(buff))) {

	/* Only #1 can lock on numbers */
	if (!God(parse_player)) {
	    free_lbuf(buff);
	    return ((BOOLEXP *) NULL);
	}
	s1 = buff;
	for (s1 = buff; isdigit((int)*s1); s1++);
	if (*s1) {
	    free_lbuf(buff);
	    return ((BOOLEXP *) NULL);
	}
	anum = atoi(buff);
    } else {
	anum = attrib->number;
    }

    /* made it now make the parse tree node */
    b = alloc_bool("test_str");
    b->type = locktype;
    b->thing = (dbref) anum;
    b->sub1 = (BOOLEXP *) strsave(s);
    free_lbuf(buff);
    return (b);
}

/* L -> (E); L -> object identifier */

static BOOLEXP *
NDECL(parse_boolexp_L)
{
    BOOLEXP *b;
    char *p, *buf;

#ifndef STANDALONE
    MSTATE mstate;
    char *tpr_buff = NULL, *tprp_buff = NULL;

#endif

    buf = NULL;
    skip_whitespace();

    switch (*parsebuf) {
    case '(':
	parsebuf++;
	b = parse_boolexp_E();
	skip_whitespace();
	if (b == TRUE_BOOLEXP || *parsebuf++ != ')') {
	    free_boolexp(b);
	    return TRUE_BOOLEXP;
	}
	break;
    default:

	/* must have hit an object ref.  Load the name into our
	 * buffer
	 */

	buf = alloc_lbuf("parse_boolexp_L");
	p = buf;
	while (*parsebuf && (*parsebuf != AND_TOKEN) &&
	       (*parsebuf != OR_TOKEN) && (*parsebuf != ')')) {
	    *p++ = *parsebuf++;
	}

	/* strip trailing whitespace */

	*p-- = '\0';
	while (isspace((int)*p))
	    *p-- = '\0';

	/* check for an attribute */

	if ((b = test_atr(buf)) != NULL) {
	    free_lbuf(buf);
	    return (b);
	}
	b = alloc_bool("parse_boolexp_L");
	b->type = BOOLEXP_CONST;

	/* do the match */

#ifndef STANDALONE

	/* If we are parsing a boolexp that was a stored lock then
	 * we know that object refs are all dbrefs, so we skip the
	 * expensive match code.
	 */

	if (parsing_internal) {
	    if (buf[0] != '#') {
		free_lbuf(buf);
		free_bool(b);
		return TRUE_BOOLEXP;
	    }
	    b->thing = atoi(&buf[1]);
	    if (!Good_obj(b->thing)) {
		free_lbuf(buf);
		free_bool(b);
		return TRUE_BOOLEXP;
	    }
	} else {
	    save_match_state(&mstate);
	    init_match(parse_player, buf, TYPE_THING);
	    match_everything(MAT_EXIT_PARENTS);
	    b->thing = match_result();
	    restore_match_state(&mstate);
	}

	if (b->thing == NOTHING) {
            tprp_buff = tpr_buff = alloc_lbuf("parse_boolexp_L");
	    notify(parse_player,
		   safe_tprintf(tpr_buff, &tprp_buff, "I don't see %s here.", buf));
            free_lbuf(tpr_buff);
	    free_lbuf(buf);
	    free_bool(b);
	    return TRUE_BOOLEXP;
	}
	if (b->thing == AMBIGUOUS) {
            tprp_buff = tpr_buff = alloc_lbuf("parse_boolexp_L");
	    notify(parse_player,
		   safe_tprintf(tpr_buff, &tprp_buff, "I don't know which %s you mean!", buf));
            free_lbuf(tpr_buff);
	    free_lbuf(buf);
	    free_bool(b);
	    return TRUE_BOOLEXP;
	}
#else /* STANDALONE ... had better be #<num> or we're hosed */
	if (buf[0] != '#') {
	    free_lbuf(buf);
	    free_bool(b);
	    return TRUE_BOOLEXP;
	}
	b->thing = atoi(&buf[1]);
	if (b->thing < 0) {
	    free_lbuf(buf);
	    free_bool(b);
	    return TRUE_BOOLEXP;
	}
#endif
	free_lbuf(buf);
    }
    return b;
}

/* F -> !F; F -> @L; F -> =L; F -> +L; F -> $L */
/* The argument L must be type BOOLEXP_CONST */

static BOOLEXP *
NDECL(parse_boolexp_F)
{
    BOOLEXP *b2;

    skip_whitespace();
    switch (*parsebuf) {
    case NOT_TOKEN:
	parsebuf++;
	b2 = alloc_bool("parse_boolexp_F.not");
	b2->type = BOOLEXP_NOT;
	if ((b2->sub1 = parse_boolexp_F()) == TRUE_BOOLEXP) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else
	    return (b2);
	/*NOTREACHED */
	break;
    case INDIR_TOKEN:
	parsebuf++;
	b2 = alloc_bool("parse_boolexp_F.indir");
	b2->type = BOOLEXP_INDIR;
	b2->sub1 = parse_boolexp_L();
	if ((b2->sub1) == TRUE_BOOLEXP) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else if ((b2->sub1->type) != BOOLEXP_CONST) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else
	    return (b2);
	/*NOTREACHED */
	break;
    case IS_TOKEN:
	parsebuf++;
	b2 = alloc_bool("parse_boolexp_F.is");
	b2->type = BOOLEXP_IS;
	b2->sub1 = parse_boolexp_L();
	if ((b2->sub1) == TRUE_BOOLEXP) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else if (((b2->sub1->type) != BOOLEXP_CONST) &&
		   ((b2->sub1->type) != BOOLEXP_ATR)) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else
	    return (b2);
	/*NOTREACHED */
	break;
    case CARRY_TOKEN:
	parsebuf++;
	b2 = alloc_bool("parse_boolexp_F.carry");
	b2->type = BOOLEXP_CARRY;
	b2->sub1 = parse_boolexp_L();
	if ((b2->sub1) == TRUE_BOOLEXP) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else if (((b2->sub1->type) != BOOLEXP_CONST) &&
		   ((b2->sub1->type) != BOOLEXP_ATR)) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else
	    return (b2);
	/*NOTREACHED */
	break;
    case OWNER_TOKEN:
	parsebuf++;
	b2 = alloc_bool("parse_boolexp_F.owner");
	b2->type = BOOLEXP_OWNER;
	b2->sub1 = parse_boolexp_L();
	if ((b2->sub1) == TRUE_BOOLEXP) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else if ((b2->sub1->type) != BOOLEXP_CONST) {
	    free_boolexp(b2);
	    return (TRUE_BOOLEXP);
	} else
	    return (b2);
	/*NOTREACHED */
	break;
    default:
	return (parse_boolexp_L());
    }
}

/* T -> F; T -> F & T */

static BOOLEXP *
NDECL(parse_boolexp_T)
{
    BOOLEXP *b, *b2;

    if ((b = parse_boolexp_F()) != TRUE_BOOLEXP) {
	skip_whitespace();
	if (*parsebuf == AND_TOKEN) {
	    parsebuf++;

	    b2 = alloc_bool("parse_boolexp_T");
	    b2->type = BOOLEXP_AND;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    }
	    b = b2;
	}
    }
    return b;
}

/* E -> T; E -> T | E */

static BOOLEXP *
NDECL(parse_boolexp_E)
{
    BOOLEXP *b, *b2;

    if ((b = parse_boolexp_T()) != TRUE_BOOLEXP) {
	skip_whitespace();
	if (*parsebuf == OR_TOKEN) {
	    parsebuf++;

	    b2 = alloc_bool("parse_boolexp_E");
	    b2->type = BOOLEXP_OR;
	    b2->sub1 = b;
	    if ((b2->sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
		free_boolexp(b2);
		return TRUE_BOOLEXP;
	    }
	    b = b2;
	}
    }
    return b;
}

BOOLEXP *
parse_boolexp(dbref player, const char *buf, int internal)
{
    strcpy(parsestore, buf);
    parsebuf = parsestore;
    parse_player = player;
    if ((buf == NULL) || (*buf == '\0'))
	return (TRUE_BOOLEXP);
#ifndef STANDALONE
    parsing_internal = internal;
#endif
    return parse_boolexp_E();
}
