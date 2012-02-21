/* db_rw.c -- I/O to/from flat file databases */
#ifdef SOLARIS
/* Solaris has borked header declarations */
char *index(const char *, int);
#endif

#include "copyright.h"
#include "autoconf.h"

#include <sys/file.h>

#define __DB_C
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "rhost_ansi.h"
#include <signal.h>

extern const char *FDECL(getstring_noalloc, (FILE *));
extern void FDECL(putstring, (FILE *, const char *));
extern void FDECL(db_grow, (dbref));

extern struct object *db;

static int g_version;
static int g_format;
static int g_flags;

#ifdef STANDALONE
char *
strip_ansi(const char *raw)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    while (p && *p) {
	if (*p == ESC_CHAR) {
	    /* Start of ANSI code. Skip to end. */
	    while (*p && !isalpha((int)*p))
		p++;
	    if (*p)
		p++;
	} else
	    *q++ = *p++;
    }
    *q = '\0';
    return buf;
}

char *
strip_returntab(const char *raw, int key)
{
    static char buf[LBUF_SIZE];
    char *p = (char *) raw;
    char *q = buf;

    while (p && *p) {
	if ((*p == '\t' && (key & 1)) || ((*p == '\n' || *p == '\r') && (key & 2)) ) {
	   p++;
	} else
	    *q++ = *p++;
    }
    *q = '\0';
    return buf;
}

#endif

/* ---------------------------------------------------------------------------
 * getboolexp1: Get boolean subexpression from file.
 */

static BOOLEXP *
getboolexp1(FILE * f)
{
    BOOLEXP *b;
    char *buff, *s;
    int c, d, anum;

    c = getc(f);
    switch (c) {
    case '\n':
	ungetc(c, f);
	return TRUE_BOOLEXP;
	/* break; */
    case EOF:
	abort();		/* unexpected EOF in boolexp */
	break;
    case '(':
	b = alloc_bool("getboolexp1.openparen");
	switch (c = getc(f)) {
	case NOT_TOKEN:
	    b->type = BOOLEXP_NOT;
	    b->sub1 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	case INDIR_TOKEN:
	    b->type = BOOLEXP_INDIR;
	    b->sub1 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	case IS_TOKEN:
	    b->type = BOOLEXP_IS;
	    b->sub1 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	case CARRY_TOKEN:
	    b->type = BOOLEXP_CARRY;
	    b->sub1 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	case OWNER_TOKEN:
	    b->type = BOOLEXP_OWNER;
	    b->sub1 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	default:
	    ungetc(c, f);
	    b->sub1 = getboolexp1(f);
	    if ((c = getc(f)) == '\n')
		c = getc(f);
	    switch (c) {
	    case AND_TOKEN:
		b->type = BOOLEXP_AND;
		break;
	    case OR_TOKEN:
		b->type = BOOLEXP_OR;
		break;
	    default:
		goto error;
	    }
	    b->sub2 = getboolexp1(f);
	    if ((d = getc(f)) == '\n')
		d = getc(f);
	    if (d != ')')
		goto error;
	    return b;
	}
    case '-':			/* obsolete NOTHING key, eat it */
	while ((c = getc(f)) != '\n')
	    if (c == EOF)
		abort();	/* unexp EOF */
	ungetc(c, f);
	return TRUE_BOOLEXP;
    default:			/* dbref or attribute */
	ungetc(c, f);
	b = alloc_bool("getboolexp1.default");
	b->type = BOOLEXP_CONST;
	b->thing = 0;

	/* This is either an attribute, eval, or constant lock.
	 * Constant locks are of the form <num>, while attribute and
	 * eval locks are of the form <anam-or-anum>:<string> or
	 * <aname-or-anum>/<string> respectively.
	 * The characters <nl>, |, and & terminate the string.
	 */

	if (isdigit((int)c)) {
	    while (isdigit((int)(c = getc(f)))) {
		b->thing = b->thing * 10 + c - '0';
	    }
        } else if ( isValidAttrStartChar(c) ) {
	    buff = alloc_lbuf("getboolexp1.atr_name");
	    for (s = buff;
		 ((c = getc(f)) != EOF) && (c != '\n') &&
		 (c != ':') && (c != '/');
		 *s++ = c);
	    if (c == EOF) {
		free_lbuf(buff);
		free_bool(b);
		goto error;
	    }
	    *s = '\0';

	    /* Look the name up as an attribute.  If not found,
	     * create a new attribute.
	     */

	    anum = mkattr(buff);
	    if (anum <= 0) {
		free_bool(b);
		free_lbuf(buff);
		goto error;
	    }
	    free_lbuf(buff);
	    b->thing = anum;
	} else {
	    free_bool(b);
	    goto error;
	}

	/* if last character is : then this is an attribute lock.
	 * A last character of / means an eval lock */

	if ((c == ':') || (c == '/')) {
	    if (c == '/')
		b->type = BOOLEXP_EVAL;
	    else
		b->type = BOOLEXP_ATR;
	    buff = alloc_lbuf("getboolexp1.attr_lock");
	    for (s = buff;
		 ((c = getc(f)) != EOF) && (c != '\n') && (c != ')') &&
		 (c != OR_TOKEN) && (c != AND_TOKEN);
		 *s++ = c);
	    if (c == EOF)
		goto error;
	    *s++ = 0;
	    b->sub1 = (BOOLEXP *) strsave(buff);
	    free_lbuf(buff);
	}
	ungetc(c, f);
	return b;
    }

  error:
    abort();			/* bomb out */
    return TRUE_BOOLEXP;
}

/* ---------------------------------------------------------------------------
 * getboolexp: Read a boolean expression from the flat file.
 */

static BOOLEXP *
getboolexp(FILE * f)
{
    BOOLEXP *b;
    char c;

    b = getboolexp1(f);
    if (getc(f) != '\n')
	abort();		/* parse error, we lose */

    /* MUSH (except for PernMUSH) and MUSE can have an extra CR,
     * MUD does not. */

    if (((g_format == F_MUSH) && (g_version != 2)) ||
	(g_format == F_MUSE)) {
	if ((c = getc(f)) != '\n')
	    ungetc(c, f);
    }
    return b;
}

/* ---------------------------------------------------------------------------
 * unscramble_attrnum: Fix up attribute numbers from foreign muds
 */

static int 
unscramble_attrnum(int attrnum)
{
    char anam[4];

    switch (g_format) {
    case F_MUSE:
	switch (attrnum) {
	case 39:
	    return A_IDLE;
	case 40:
	    return A_AWAY;
	case 41:
	    return 0;		/* mailk */
	case 42:
	    return A_ALIAS;
	case 43:
	    return A_EFAIL;
	case 44:
	    return A_OEFAIL;
	case 45:
	    return A_AEFAIL;
	case 46:
	    return 0;		/* it */
	case 47:
	    return A_LEAVE;
	case 48:
	    return A_OLEAVE;
	case 49:
	    return A_ALEAVE;
	case 50:
	    return 0;		/* channel */
	case 51:
	    return A_QUOTA;
	case 52:
	    return A_TEMP;	/* temp for pennies */
	case 53:
	    return 0;		/* huhto */
	case 54:
	    return 0;		/* haven */
	case 57:
	    return mkattr((char *) "TZ");
	case 58:
	    return 0;		/* doomsday */
	case 59:
	    return mkattr((char *) "Email");
	case 98:
	    return mkattr((char *) "Status");
	case 99:
	    return mkattr((char *) "Race");
	default:
	    return attrnum;
	}
    case F_MUSH:

	/* Only need to muck with Pern variants */

	if (g_version != 2)
	    return attrnum;
	switch (attrnum) {
	case 34:
	    return A_OENTER;
	case 41:
	    return A_LEAVE;
	case 42:
	    return A_ALEAVE;
	case 43:
	    return A_OLEAVE;
	case 44:
	    return A_OXENTER;
	case 45:
	    return A_OXLEAVE;
	default:
	    if ((attrnum >= 126) && (attrnum < 152)) {
		anam[0] = 'W';
		anam[1] = attrnum - 126 + 'A';
		anam[2] = '\0';
		return mkattr(anam);
	    }
	    if ((attrnum >= 152) && (attrnum < 178)) {
		anam[0] = 'X';
		anam[1] = attrnum - 152 + 'A';
		anam[2] = '\0';
		return mkattr(anam);
	    }
	    return attrnum;
	}
    default:
	return attrnum;
    }
}

/* ---------------------------------------------------------------------------
 * get_list: Read attribute list from flat file.
 */

static int 
get_list(FILE * f, dbref i, int key, int *i_count)
{
    dbref atr, aowner;
    int c, aflags, xflags, anum;
    char *buff, *buf2, *buf2p, *ownp, *flagp;
    ATTR *a_atr;

    buff = alloc_lbuf("get_list");
    mudstate.vlplay = NOTHING;
    while (1) {
	switch (c = getc(f)) {
	case '>':		/* read # then string */
            if ( key > 0 ) {
               buf2 = (char *)getstring_noalloc(f);
               if ( key != 2 ) {
                  a_atr = atr_str((char *)buf2);
                  if ( a_atr ) {
                     atr = a_atr->number;
                  } else {
                     atr = mkattr((char *)buf2);
                     if ( atr > 0 )
                        (*i_count)++;
                  }
               } else {
                  buf2 = (char *) getstring_noalloc(f);
                  break;
               }
            } else {
	       atr = unscramble_attrnum(getref(f));
            }
	    if (atr > 0) {
		/* Store the attr */
		atr_add_raw(i, atr,
			    (char *) getstring_noalloc(f));
	    } else {
		/* Silently discard */

		getstring_noalloc(f);
	    }
	    break;
	case ']':		/* Pern 1.13 style text attribute */
	    strcpy(buff, (char *) getstring_noalloc(f));

	    /* Get owner number */

	    ownp = (char *) index(buff, '^');
	    if (!ownp) {
		fprintf(stderr,
			"Bad format in attribute on object %d\n",
			i);
		free_lbuf(buff);
		return 0;
	    }
	    *ownp++ = '\0';

	    /* Get attribute flags */

	    flagp = (char *) index(ownp, '^');
	    if (!flagp) {
		fprintf(stderr,
			"Bad format in attribute on object %d\n",
			i);
		free_lbuf(buff);
		return 0;
	    }
	    *flagp++ = '\0';

	    /* Convert Pern-style owner and flags to 2.0 format */

	    aowner = atoi(ownp);
	    xflags = atoi(flagp);
	    aflags = 0;

	    if (!aowner)
		aowner = NOTHING;
	    if (xflags & 0x10)
		aflags |= AF_LOCK | AF_NOPROG;
	    if (xflags & 0x20)
		aflags |= AF_NOPROG;

	    /* Look up the attribute name in the attribute table.
	     * If the name isn't found, create a new attribute.
	     * If the create fails, try prefixing the attr name
	     * with ATR_ (Pern allows attributes to start with a
	     * non-alphabetic character.
	     */

	    anum = mkattr(buff);
	    if (anum < 0) {
		buf2 = alloc_mbuf("get_list.new_attr_name");
		buf2p = buf2;
		safe_mb_str((char *) "ATR_", buf2, &buf2p);
		safe_mb_str(buff, buf2, &buf2p);
		*buf2p = '\0';
		anum = mkattr(buf2);
		free_mbuf(buf2);
	    }
	    if (anum < 0) {
		fprintf(stderr,
		      "Bad attribute name '%s' on object %d, ignoring...\n",
			buff, i);
		(void) getstring_noalloc(f);
	    } else {
		atr_add(i, anum, (char *) getstring_noalloc(f),
			aowner, aflags);
	    }
	    break;
	case '\n':		/* ignore newlines. They're due to v(r). */
	    break;
	case '<':		/* end of list */
	    free_lbuf(buff);
	    c = getc(f);
	    if (c != '\n') {
		ungetc(c, f);
		fprintf(stderr,
			"No line feed on object %d\n", i);
		return 1;
	    }
	    return 1;
	default:
            if ( key == 2 ) {
               free_lbuf(buff);
               return 0;
            }
	    fprintf(stderr,
		"Bad character '%c' when getting attributes on object %d\n",
		    c, i);
	    /* We've found a bad spot.  I hope things aren't
	     * too bad. */

	    (void) getstring_noalloc(f);
	}
        if ( key && feof(f) ) {
           free_lbuf(buff);
           return 0;
        }
    }
}

/* ---------------------------------------------------------------------------
 * putbool_subexp: Write a boolean sub-expression to the flat file.
 */
static void 
putbool_subexp(FILE * f, BOOLEXP * b)
{
    switch (b->type) {
    case BOOLEXP_IS:
	putc('(', f);
	putc(IS_TOKEN, f);
	putbool_subexp(f, b->sub1);
	putc(')', f);
	break;
    case BOOLEXP_CARRY:
	putc('(', f);
	putc(CARRY_TOKEN, f);
	putbool_subexp(f, b->sub1);
	putc(')', f);
	break;
    case BOOLEXP_INDIR:
	putc('(', f);
	putc(INDIR_TOKEN, f);
	putbool_subexp(f, b->sub1);
	putc(')', f);
	break;
    case BOOLEXP_OWNER:
	putc('(', f);
	putc(OWNER_TOKEN, f);
	putbool_subexp(f, b->sub1);
	putc(')', f);
	break;
    case BOOLEXP_AND:
	putc('(', f);
	putbool_subexp(f, b->sub1);
	putc(AND_TOKEN, f);
	putbool_subexp(f, b->sub2);
	putc(')', f);
	break;
    case BOOLEXP_OR:
	putc('(', f);
	putbool_subexp(f, b->sub1);
	putc(OR_TOKEN, f);
	putbool_subexp(f, b->sub2);
	putc(')', f);
	break;
    case BOOLEXP_NOT:
	putc('(', f);
	putc(NOT_TOKEN, f);
	putbool_subexp(f, b->sub1);
	putc(')', f);
	break;
    case BOOLEXP_CONST:
	fprintf(f, "%d", b->thing);
	break;
    case BOOLEXP_ATR:
	fprintf(f, "%d:%s\n", b->thing, (char *) b->sub1);
	break;
    case BOOLEXP_EVAL:
	fprintf(f, "%d/%s\n", b->thing, (char *) b->sub1);
	break;
    default:
	fprintf(stderr, "Unknown boolean type in putbool_subexp: %d\n",
		b->type);
    }
}

/* ---------------------------------------------------------------------------
 * putboolexp: Write boolean expression to the flat file.
 */

static void 
putboolexp(FILE * f, BOOLEXP * b)
{
    if (b != TRUE_BOOLEXP) {
	putbool_subexp(f, b);
    }
    putc('\n', f);
}

/* ---------------------------------------------------------------------------
 * upgrade_flags: Convert foreign flags to MUSH format.
 */

static void 
upgrade_flags(FLAG * flags1, FLAG * flags2, dbref thing,
	      int db_format, int db_version)
{
    FLAG f1, f2, newf1, newf2;

    f1 = *flags1;
    f2 = *flags2;
    newf1 = 0;
    newf2 = 0;
    if (db_format == F_MUD) {

	/* Old TinyMUD format */

	newf1 = f1 & (TYPE_MASK | WIZARD | LINK_OK | DARK | STICKY | HAVEN);
	if (f1 & MUD_ABODE)
	    newf2 |= ABODE;
	if (f1 & MUD_ROBOT)
	    newf1 |= ROBOT;
	if (f1 & MUD_CHOWN_OK)
	    newf1 |= CHOWN_OK;

    } else if (db_format == F_MUSE) {
	if (db_version == 1)
	    return;

	/* Convert level-based players to normal */

	switch (f1 & 0xf) {
	case 0:		/* room */
	case 1:		/* thing */
	case 2:		/* exit */
	    newf1 = f1 & 0x3;
	    break;
	case 8:		/* guest */
	case 9:		/* trial player */
	case 10:		/* member */
	case 11:		/* junior official */
	case 12:		/* official */
	    newf1 = TYPE_PLAYER;
	    break;
	case 13:		/* honorary wizard */
	case 14:		/* administrator */
	case 15:		/* director */
	    newf1 = TYPE_PLAYER | WIZARD;
	    break;
	default:		/* A bad type, mark going */
	    fprintf(stderr, "Funny object type for #%d\n", thing);
	    *flags1 = GOING;
	    return;
	}

	/* Player #1 is always a wizard */

	if (thing == (dbref) 1)
	    newf1 |= WIZARD;

	/* Set type-specific flags */

	switch (newf1 & TYPE_MASK) {
	case TYPE_PLAYER:	/* Lose CONNECT TERSE QUITE NOWALLS WARPTEXT */
	    if (f1 & MUSE_BUILD)
		newf2 |= BUILDER;
	    if (f1 & MUSE_SLAVE)
		newf2 |= SLAVE;
	    if (f1 & MUSE_UNFIND)
		newf2 |= UNFINDABLE;
	    break;
	case TYPE_THING:	/* lose LIGHT SACR_OK */
	    if (f1 & MUSE_KEY)
		newf2 |= KEY;
	    if (f1 & MUSE_DEST_OK)
		newf1 |= DESTROY_OK;
	    break;
	case TYPE_ROOM:
	    if (f1 & MUSE_ABODE)
		newf2 |= ABODE;
	    break;
	case TYPE_EXIT:
	    if (f1 & MUSE_SEETHRU)
		newf1 |= SEETHRU;
	default:
	    break;
	}

	/* Convert common flags */
	/* Lose: MORTAL ACCESSED MARKED SEE_OK UNIVERSAL */

	if (f1 & MUSE_CHOWN_OK)
	    newf1 |= CHOWN_OK;
	if (f1 & MUSE_DARK)
	    newf1 |= DARK;
	if (f1 & MUSE_STICKY)
	    newf1 |= STICKY;
	if (f1 & MUSE_HAVEN)
	    newf1 |= HAVEN;
	if (f1 & MUSE_INHERIT)
	    newf1 |= INHERIT;
	if (f1 & MUSE_GOING)
	    newf1 |= GOING;
	if (f1 & MUSE_PUPPET)
	    newf1 |= PUPPET;
	if (f1 & MUSE_LINK_OK)
	    newf1 |= LINK_OK;
	if (f1 & MUSE_ENTER_OK)
	    newf1 |= ENTER_OK;
	if (f1 & MUSE_VISUAL)
	    newf1 |= VISUAL;
	if (f1 & MUSE_OPAQUE)
	    newf1 |= OPAQUE;
	if (f1 & MUSE_QUIET)
	    newf1 |= QUIET;

    } else if ((db_format == F_MUSH) && (db_version == 2)) {

	/* Pern variants */

	newf1 = f1 &
	    (TYPE_MASK | LINK_OK | HAVEN | QUIET | HALT | DARK |
	     GOING | PUPPET | CHOWN_OK | ENTER_OK | VISUAL | OPAQUE);
	if (f1 & 0x1000000)
	    newf1 |= INHERIT;
	if (f1 & 0x10000000)
	    newf1 |= HEARTHRU;
	switch (newf1 & TYPE_MASK) {
	case TYPE_PLAYER:
	    newf1 |= f1 &
		(WIZARD | BUILDER | UNFINDABLE);
	    if (f1 & PERN_SLAVE)
		newf2 |= SLAVE;
	    if (f1 & PERN_NOSPOOF)
		newf1 |= NOSPOOF;
	    if (f1 & PERN_SUSPECT)
		newf2 |= SUSPECT;
	    break;
	case TYPE_EXIT:
	    newf1 |= f1 & (STICKY | SEETHRU);
	    if (f1 & PERN_KEY)
		newf2 |= KEY;
	    if (f1 & WIZARD)
		newf1 |= INHERIT;
	    break;
	case TYPE_THING:
	    newf1 |= f1 & (DESTROY_OK | STICKY);
	    if (f1 & KEY)
		newf2 |= KEY;
	    if (f1 & WIZARD)
		newf1 |= INHERIT;
	    if (f1 & PERN_VERBOSE)
		newf1 |= VERBOSE;
	    if (f1 & PERN_IMMORTAL)
		newf1 |= IMMORTAL;
	    if (f1 & PERN_MONITOR)
		newf1 |= MONITOR;
	    if (f1 & PERN_SAFE)
		newf1 |= SAFE;
	    break;
	case TYPE_ROOM:
	    newf1 |= f1 &
		(FLOATING | ABODE | JUMP_OK);
	    if (f1 & WIZARD)
		newf1 |= INHERIT;
	    if (f1 & PERN_UNFIND)
		newf2 |= UNFINDABLE;
	}
    } else if ((db_format == F_MUSH) && (db_version >= 3)) {
	newf1 = f1;
	newf2 = f2;
	switch (db_version) {
	case 3:
	    (newf1 &= ~V2_ACCESSED);	/* Clear ACCESSED */
	case 4:
	    (newf1 &= ~V3_MARKED);	/* Clear MARKED */
	case 5:
	    /* Merge GAGGED into SLAVE, move SUSPECT */

	    if ((newf1 & TYPE_MASK) == TYPE_PLAYER) {
		if (newf1 & V4_GAGGED) {
		    newf2 |= SLAVE;
		    newf1 &= ~V4_GAGGED;
		}
		if (newf1 & V4_SUSPECT) {
		    newf2 |= SUSPECT;
		    newf1 &= ~V4_SUSPECT;
		}
	    }
	case 6:
	    switch (newf1 & TYPE_MASK) {
	    case TYPE_PLAYER:
		if (newf1 & V6_BUILDER) {
		    newf2 |= BUILDER;
		    newf1 &= ~V6_BUILDER;
		}
		if (newf1 & V6_SUSPECT) {
		    newf2 |= SUSPECT;
		    newf1 &= ~V6_SUSPECT;
		}
		if (newf1 & V6PLYR_UNFIND) {
		    newf2 |= UNFINDABLE;
		    newf1 &= ~V6PLYR_UNFIND;
		}
		if (newf1 & V6_SLAVE) {
		    newf2 |= SLAVE;
		    newf1 &= ~V6_SLAVE;
		}
		break;
	    case TYPE_ROOM:
		if (newf1 & V6_FLOATING) {
		    newf2 |= FLOATING;
		    newf1 &= ~V6_FLOATING;
		}
		if (newf1 & V6_ABODE) {
		    newf2 |= ABODE;
		    newf1 &= ~V6_ABODE;
		}
		if (newf1 & V6ROOM_JUMPOK) {
		    newf1 |= JUMP_OK;
		    newf1 &= ~V6ROOM_JUMPOK;
		}
		if (newf1 & V6ROOM_UNFIND) {
		    newf2 |= UNFINDABLE;
		    newf1 &= ~V6ROOM_UNFIND;
		}
		break;
	    case TYPE_THING:
		if (newf1 & V6OBJ_KEY) {
		    newf2 |= KEY;
		    newf1 &= ~V6OBJ_KEY;
		}
		break;
	    case TYPE_EXIT:
		if (newf1 & V6EXIT_KEY) {
		    newf2 |= KEY;
		    newf1 &= ~V6EXIT_KEY;
		}
		break;
	    }

	}
    }
    *flags1 = newf1;
    *flags2 = newf2;
    return;
}

/* ---------------------------------------------------------------------------
 * efo_convert: Fix things up for Exits-From-Objects
 */

void 
NDECL(efo_convert)
{
    int i;
    dbref link;

    DO_WHOLE_DB(i) {
	switch (Typeof(i)) {
	case TYPE_PLAYER:
	case TYPE_THING:

	    /* swap Exits and Link */

	    link = Link(i);
	    s_Link(i, Exits(i));
	    s_Exits(i, link);
	    break;
	}
    }
}
/* ---------------------------------------------------------------------------
 * unscraw_foreign: Fix up strange object linking conventions for other formats
 */

static void 
unscraw_pern_object(dbref i, int db_flags)
{
    dbref aowner;
    int aflags;
    char *p_str;

    if (db_flags & V_PERNKEY) {

	/* Use lock on players is really the page lock */

	if (Typeof(i) == TYPE_PLAYER) {
	    p_str = atr_get(i, A_LUSE, &aowner, &aflags);
	    if (*p_str) {
		atr_add_raw(i, A_LPAGE, p_str);
		atr_clr(i, A_LUSE);
	    }
	    free_lbuf(p_str);
	}
	/* Enter lock on rooms is the teleport lock */

	if (Typeof(i) == TYPE_ROOM) {
	    p_str = atr_get(i, A_LENTER, &aowner, &aflags);
	    if (*p_str) {
		atr_add_raw(i, A_LTPORT, p_str);
		atr_clr(i, A_LENTER);
	    }
	    free_lbuf(p_str);
	}
    }
}

void 
unscraw_foreign(int db_format, int db_version, int db_flags)
{
    dbref tmp, i, aowner;
    int aflags;
    char *p_str;

    switch (db_format) {
    case F_MUSE:
	DO_WHOLE_DB(i) {
	    if (Typeof(i) == TYPE_EXIT) {

		/* MUSE exits are bass-ackwards */

		tmp = Exits(i);
		s_Exits(i, Location(i));
		s_Location(i, tmp);
	    }
	    if (db_version > 3) {

		/* MUSEs with pennies in an attribute have
		 * it stored in attr 255 (see
		 * unscramble_attrnum)
		 */

		p_str = atr_get(i, A_TEMP, &aowner, &aflags);
		s_Pennies(i, atoi(p_str));
		free_lbuf(p_str);
		atr_clr(i, A_TEMP);
	    }
	}
	if (!(db_flags & V_LINK)) {
	    efo_convert();
	}
	break;
    case F_MUSH:
	if (db_version <= 3) {

	    if (db_version == 2) {
		DO_WHOLE_DB(i) {
		    unscraw_pern_object(i, db_flags);
		}
	    }
	    efo_convert();
	}
	if ((db_version <= 5) && (db_flags & V_GDBM)) {

	    /* Check for FORWARDLIST attribute */

	    DO_WHOLE_DB(i) {
		if (atr_get_raw(i, A_FORWARDLIST))
		    s_Flags2(i, Flags2(i) | HAS_FWDLIST);
	    }
	}
	if (db_version <= 6) {

	    DO_WHOLE_DB(i) {

		/* Make sure A_QUEUEMAX is empty */

		atr_clr(i, A_QUEUEMAX);

		if (db_flags & V_GDBM) {

		    /* HAS_LISTEN now tracks LISTEN attr */

		    if (atr_get_raw(i, A_LISTEN))
			s_Flags2(i, Flags2(i) | HAS_LISTEN);

		    /* Undo V6 overloading of HAS_STARTUP
		     * with HAS_FWDLIST
		     */

		    if ((db_version == 6) &&
			(Flags2(i) & HAS_STARTUP) &&
			atr_get_raw(i, A_FORWARDLIST)) {

			/* We have FORWARDLIST */

			s_Flags2(i, Flags2(i) | HAS_FWDLIST);

			/* Maybe no STARTUP */

			if (!atr_get_raw(i, A_STARTUP))
			    s_Flags2(i, Flags2(i) & ~HAS_STARTUP);
		    }
		}
	    }
	}
	break;
    case F_MUD:
	efo_convert();
    }
}

/* ---------------------------------------------------------------------------
 * getlist_discard, get_atrdefs_discard: Throw away data from MUSE that we
 * don't use.
 */

static void 
getlist_discard(FILE * f)
{
    int count;

    for (count = getref(f); count > 0; count--)
	(void) getref(f);
}

static void 
get_atrdefs_discard(FILE * f)
{
    const char *sp;

    for (;;) {
	sp = getstring_noalloc(f);	/* flags or endmarker */
	if (*sp == '\\')
	    return;
	sp = getstring_noalloc(f);	/* object */
	sp = getstring_noalloc(f);	/* name */
    }
}

dbref 
db_read(FILE * f, int *db_format, int *db_version, int *db_flags)
{
    dbref i, anum;
    char ch, peek;
    const char *tstr;
    int header_gotten, size_gotten, nextattr_gotten;
    int read_attribs, read_name, read_zone, read_link, read_key, read_parent;
    int read_extflags, read_money, read_timestamps;
    int read_pern_key, read_pern_comm, read_pern_parent;
    int read_muse_parents, read_muse_atrdefs;
    int read_powers_player, read_powers_any;
    int deduce_version, deduce_name, deduce_zone, deduce_timestamps;
    int aflags, f1, f2, i_trash;
    int zonedata;
    BOOLEXP *tempbool;

    i_trash = 0;
    header_gotten = 0;
    size_gotten = 0;
    nextattr_gotten = 0;
    g_format = F_UNKNOWN;
    g_version = 0;
    g_flags = 0;
    read_attribs = 1;
    read_name = 1;
    read_zone = 0;
    read_link = 0;
    read_key = 1;
    read_parent = 0;
    read_money = 1;
    read_extflags = 0;
    read_timestamps = 0;
    read_pern_key = 0;
    read_pern_comm = 0;
    read_pern_parent = 0;
    read_powers_player = 0;
    read_powers_any = 0;
    read_muse_parents = 0;
    read_muse_atrdefs = 0;
    deduce_version = 1;
    deduce_zone = 1;
    deduce_name = 1;
    deduce_timestamps = 1;

#ifdef STANDALONE
    fprintf(stderr, "Reading ");
    fflush(stderr);
#endif
    db_free();
    for (i = 0;; i++) {

	if (!(i % 25)) {
	    cache_reset(0);
	}
#ifdef STANDALONE
	if (!(i % 100)) {
	    fputc('.', stderr);
	    fflush(stderr);
	}
#endif
	switch (ch = getc(f)) {
	case '~':		/* Database size tag */
	    if (size_gotten) {
		fprintf(stderr,
			"\nDuplicate size entry at object %d, ignored.\n",
			i);
		tstr = getstring_noalloc(f);	/* junk */
		break;
	    }
	    mudstate.min_size = getref(f);
	    size_gotten = 1;
	    break;
	case '+':		/* MUSH 2.0 header */
	    switch (ch = getc(f)) {	/* 2nd char selects type */
	    case 'V':		/* VERSION */
		if (header_gotten) {
		    fprintf(stderr,
			    "\nDuplicate MUSH version header entry at object %d, ignored.\n",
			    i);
		    tstr = getstring_noalloc(f);	/* junk */
		    break;
		}
		header_gotten = 1;
		deduce_version = 0;
		g_format = F_MUSH;
		g_version = getref(f);

		if ((g_version & V_MASK) == 2) {

		    /* Handle Pern veriants specially */

		    switch (g_version >> 8) {
		    case 4:
		    case 3:
			read_pern_parent = 1;
		    case 2:
			if ((g_version >> 8) != 4) {
			    read_pern_comm = 1;
			    g_flags |= V_COMM;
			}
		    case 1:
			read_pern_key = 1;
			read_parent = 1;
			g_flags |= V_PERNKEY | V_ZONE;
		    }
		} else {

		    /* Otherwise extract feature flags */

		    if (g_version & V_GDBM) {
			read_attribs = 0;
			read_name = !(g_version & V_ATRNAME);
		    }
		    read_zone = (g_version & V_ZONE);
		    read_link = (g_version & V_LINK);
		    read_key = !(g_version & V_ATRKEY);
		    read_parent = (g_version & V_PARENT);
		    read_money = !(g_version & V_ATRMONEY);
		    read_extflags = (g_version & V_XFLAGS);
		    g_flags = g_version & ~V_MASK;
		}
		g_version &= V_MASK;
		deduce_name = 0;
		deduce_version = 0;
		deduce_zone = 0;
		break;
	    case 'S':		/* SIZE */
		if (size_gotten) {
		    fprintf(stderr,
			  "\nDuplicate size entry at object %d, ignored.\n",
			    i);
		    tstr = getstring_noalloc(f);	/* junk */
		} else {
		    mudstate.min_size = getref(f);
		}
		size_gotten = 1;
		break;
	    case 'A':		/* USER-NAMED ATTRIBUTE */
		anum = getref(f);
		tstr = getstring_noalloc(f);
		if (isdigit((int)*tstr)) {
		    aflags = 0;
		    while (isdigit((int)*tstr))
			aflags = (aflags * 10) +
			    (*tstr++ - '0');
		    tstr++;	/* skip ':' */
		} else {
		    aflags = mudconf.vattr_flags;
		}
                if ( anum < 0 ) {
                   fprintf(stderr, "\nInvalid attribute number %d for attribute %s\n", anum, tstr);
                } else {
		   vattr_define((char *) tstr, anum, aflags);
                }
		break;
	    case 'F':		/* OPEN USER ATTRIBUTE SLOT */
		anum = getref(f);
		break;
	    case 'N':		/* NEXT ATTR TO ALLOC WHEN NO FREELIST */
		if (nextattr_gotten) {
		    fprintf(stderr,
			    "\nDuplicate next free vattr entry at object %d, ignored.\n",
			    i);
		    tstr = getstring_noalloc(f);	/* junk */
		} else {
		    mudstate.attr_next = getref(f);
		    nextattr_gotten = 1;
		}
		break;
	    default:
		fprintf(stderr,
			"\nUnexpected character '%c' in MUSH header near object #%d, ignored.\n",
			ch, i);
		tstr = getstring_noalloc(f);	/* toss line */
	    }
	    break;
	case '@':		/* MUSE header */
	    if (header_gotten) {
		fprintf(stderr,
			"\nDuplicate MUSE header entry at object #%d.\n",
			i);
		return -1;
	    }
	    header_gotten = 1;
	    deduce_version = 0;
	    g_format = F_MUSE;
	    g_version = getref(f);
	    deduce_name = 0;
	    deduce_zone = 1;
	    read_money = (g_version <= 3);
	    read_link = (g_version >= 5);
	    read_powers_player = (g_version >= 6);
	    read_powers_any = (g_version == 6);
	    read_muse_parents = (g_version >= 8);
	    read_muse_atrdefs = (g_version >= 8);
	    if (read_link)
		g_flags |= V_LINK;
	    break;
	case '#':
	    if (deduce_version) {
		g_format = F_MUD;
		g_version = 1;
		deduce_version = 0;
	    }
	    if (g_format != F_MUD) {
		fprintf(stderr,
			"\nMUD-style object found in non-MUD database at object #%d\n",
			i);
		return -1;
	    }
	    if (i != getref(f)) {
		fprintf(stderr,
			"\nSequence error at object #%d\n", i);
		return -1;
	    }
	    db_grow(i + 1);
	    s_Name(i, strip_ansi(getstring_noalloc(f)));
	    atr_add_raw(i, A_DESC, (char *) getstring_noalloc(f));
	    s_Location(i, getref(f));
	    s_Contents(i, getref(f));
	    s_Exits(i, getref(f));
	    s_Link(i, NOTHING);
	    s_Next(i, getref(f));
/*                      s_Zone(i, NOTHING); */
	    tempbool = getboolexp(f);
	    atr_add_raw(i, A_LOCK,
			unparse_boolexp_quiet(1, tempbool));
	    free_boolexp(tempbool);
	    atr_add_raw(i, A_FAIL, (char *) getstring_noalloc(f));
	    atr_add_raw(i, A_SUCC, (char *) getstring_noalloc(f));
	    atr_add_raw(i, A_OFAIL, (char *) getstring_noalloc(f));
	    atr_add_raw(i, A_OSUCC, (char *) getstring_noalloc(f));
	    s_Owner(i, getref(f));
	    s_Parent(i, NOTHING);
	    s_Pennies(i, getref(f));
	    f1 = getref(f);
	    f2 = 0;
	    upgrade_flags(&f1, &f2, i, g_format, g_version);
	    s_Flags(i, f1);
	    s_Flags2(i, f2);
	    s_Flags3(i, 0);
	    s_Flags4(i, 0);
	    s_Toggles(i, 0);
	    s_Toggles2(i, 0);
	    s_Toggles3(i, 0);
	    s_Toggles4(i, 0);
	    s_Toggles5(i, 0);
	    s_Toggles6(i, 0);
	    s_Toggles7(i, 0);
	    s_Toggles8(i, 0);
	    s_Pass(i, getstring_noalloc(f));
	    if (deduce_timestamps) {
		peek = getc(f);
		if ((peek != '#') && (peek != '*')) {
		    read_timestamps = 1;
		}
		deduce_timestamps = 0;
		ungetc(peek, f);
	    }
	    if (read_timestamps) {
		aflags = getref(f);	/* created */
		aflags = getref(f);	/* lastused */
		aflags = getref(f);	/* usecount */
	    }
	    break;
	case '&':		/* MUSH 2.0a stub entry/MUSE zoned entry */
	    if (deduce_version) {
		deduce_version = 0;
		g_format = F_MUSH;
		g_version = 1;
		deduce_name = 1;
		deduce_zone = 0;
		read_key = 0;
		read_attribs = 0;
	    } else if (deduce_zone) {
		deduce_zone = 0;
		read_zone = 1;
		g_flags |= V_ZONE;
	    }
	case '!':		/* MUSH entry/MUSE non-zoned entry */
	    if (deduce_version) {
		g_format = F_MUSH;
		g_version = 1;
		deduce_name = 0;
		deduce_zone = 0;
		deduce_version = 0;
	    } else if (deduce_zone) {
		deduce_zone = 0;
		read_zone = 0;
	    }
	    i = getref(f);
	    db_grow(i + 1);

	    /* NAME and LOCATION */

	    if (read_name) {
		tstr = getstring_noalloc(f);
		if (deduce_name) {
		    if (isdigit((int)*tstr)) {
			read_name = 0;
			s_Location(i, atoi(tstr));
		    } else {
			s_Name(i, tstr);
			s_Location(i, getref(f));
		    }
		    deduce_name = 0;
		} else {
		    s_Name(i, tstr);
		    s_Location(i, getref(f));
		}
	    } else {
		s_Location(i, getref(f));
	    }

	    /* ZONE on MUSE databases and some others */

	    if (read_zone)
		s_Zone(i, getref(f));
/*                      else
   s_Zone(i, NOTHING); */

	    /* CONTENTS and EXITS */

	    s_Contents(i, getref(f));
	    s_Exits(i, getref(f));

	    /* LINK */

	    if (read_link)
		s_Link(i, getref(f));
	    else
		s_Link(i, NOTHING);

	    /* NEXT */

	    s_Next(i, getref(f));

	    /* PARENT on PennMUSH 1.19.01+ databases */

	    if (read_pern_parent)
		s_Parent(i, getref(f));

	    /* LOCK (and PennMUSH extended locks) */

	    if (read_key) {
		tempbool = getboolexp(f);
		atr_add_raw(i, A_LOCK,
			    unparse_boolexp_quiet(1, tempbool));
		free_boolexp(tempbool);
		if (read_pern_key) {

		    /* Read Pern 1.17-style locks */

		    tempbool = getboolexp(f);
		    atr_add_raw(i, A_LUSE,
				unparse_boolexp_quiet(1,
						      tempbool));
		    free_boolexp(tempbool);
		    tempbool = getboolexp(f);
		    atr_add_raw(i, A_LENTER,
				unparse_boolexp_quiet(1,
						      tempbool));
		    free_boolexp(tempbool);
		}
	    }
	    /* OWNER */

	    s_Owner(i, getref(f));

	    /* PARENT: PennMUSH uses this field for ZONE (which we
	     * use as PARENT if we didn't already read in a
	     * non-NOTHING parent.
	     */

	    if (read_parent) {
		if (read_pern_parent) {
		    if (Parent(i) != NOTHING)
			(void) getref(f);
		    else
			s_Parent(i, getref(f));
		} else {
		    s_Parent(i, getref(f));
		}
	    } else {
		s_Parent(i, NOTHING);
	    }

	    /* PENNIES */

	    if (read_money)	/* if not fix in unscraw_foreign */
		s_Pennies(i, getref(f));

	    /* FLAGS */

	    f1 = getref(f);
	    if (read_extflags)
		f2 = getref(f);
	    else
		f2 = 0;
	    upgrade_flags(&f1, &f2, i, g_format, g_version);
	    s_Flags(i, f1);
	    s_Flags2(i, f2);
	    if (read_extflags) {
		s_Flags3(i, getref(f));
		s_Flags4(i, getref(f));
		s_Toggles(i, getref(f));
		s_Toggles2(i, getref(f));
		s_Toggles3(i, getref(f));
		s_Toggles4(i, getref(f));
		s_Toggles5(i, getref(f));
		s_Toggles6(i, getref(f));
		s_Toggles7(i, getref(f));
		s_Toggles8(i, getref(f));

                db[i].zonelist = NULL;
                for( zonedata = getref(f); 
                     zonedata != -1; 
                     zonedata = getref(f)) {
                  zlist_add(i, zonedata); 
                }
	    }
	    /* POWERS from MUSE.  Discard. */

	    if (read_powers_any ||
		((Typeof(i) == TYPE_PLAYER) && read_powers_player))
		(void) getstring_noalloc(f);

	    /* COMM from PennMUSH */

	    if (read_pern_comm)
		(void) getref(f);

	    /* ATTRIBUTES */
	    if (read_attribs) {
		if (!get_list(f, i, 0, &i_trash)) {
		    fprintf(stderr,
			    "\nError reading attrs for object #%d\n",
			    i);
		    return -1;
		}
	    }
	    /* PARENTS from MUSE.  Ewwww. */

	    if (read_muse_parents) {
		getlist_discard(f);
		getlist_discard(f);
	    }
	    /* ATTRIBUTE DEFINITIONS from MUSE.  Ewwww. Ewwww. */

	    if (read_muse_atrdefs) {
		get_atrdefs_discard(f);
	    }
	    /* check to see if it's a player */
	    if (Typeof(i) == TYPE_PLAYER) {
		c_Connected(i);
	    }
	    break;
	case '*':		/* EOF marker */
	    tstr = getstring_noalloc(f);
	    if (strcmp(tstr, "**END OF DUMP***")) {
		fprintf(stderr,
			"\nBad EOF marker at object #%d\n",
			i);
		return -1;
	    } else {
#ifdef STANDALONE
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
		/* Fix up bizarro foreign DBs */

		unscraw_foreign(g_format, g_version, g_flags);
		*db_version = g_version;
		*db_format = g_format;
		*db_flags = g_flags;
#ifndef STANDALONE
		load_player_names();
#endif
		return mudstate.db_top;
	    }
	default:
	    fprintf(stderr, "\nIllegal character '%c' near object #%d\n",
		    ch, i);
	    return -1;
	}
    }
}

static int 
db_write_object(FILE * f, dbref i, int db_format, int flags, int key)
{
#ifndef STANDALONE
    ATTR *a;

#endif
    char *got, *as;
    dbref aowner;
    int ca, aflags, save, j;
    BOOLEXP *tempbool;
    ZLISTNODE* zlistnodeptr;

    /* no debugging stack manip here since we could be forked */

    if (!(flags & V_ATRNAME))
	putstring(f, Name(i));
    putref(f, Location(i));
    if (flags & V_ZONE)
	putref(f, Zone(i));
    putref(f, Contents(i));
    putref(f, Exits(i));
    if (flags & V_LINK)
	putref(f, Link(i));
    putref(f, Next(i));
    fflush(f);
    if (!(flags & V_ATRKEY)) {
	got = atr_get(i, A_LOCK, &aowner, &aflags);
	tempbool = parse_boolexp(GOD, got, 1);
	free_lbuf(got);
        putboolexp(f, tempbool);
        if( tempbool ) {
          free_boolexp(tempbool);
        }
    }
    putref(f, Owner(i));
    if (flags & V_PARENT)
	putref(f, Parent(i));
    if (!(flags & V_ATRMONEY))
	putref(f, Pennies(i));
    putref(f, Flags(i));
    if (flags & V_XFLAGS) {
	putref(f, Flags2(i));
	putref(f, Flags3(i));
	putref(f, Flags4(i));
	putref(f, Toggles(i));
	putref(f, Toggles2(i));
	putref(f, Toggles3(i));
	putref(f, Toggles4(i));
	putref(f, Toggles5(i));
	putref(f, Toggles6(i));
	putref(f, Toggles7(i));
	putref(f, Toggles8(i));

        for(zlistnodeptr = db[i].zonelist; 
            zlistnodeptr;
            zlistnodeptr = zlistnodeptr->next ) {
          putref(f, zlistnodeptr->object);
        } 
        putref(f, -1);
    }
    /* write the attribute list */

    if (!(flags & V_GDBM)) {
	for (ca = atr_head(i, &as); ca; ca = atr_next(&as)) {
	    save = 0;
#ifndef STANDALONE
	    a = atr_num(ca);
	    if (a)
		j = a->number;
	    else
		j = -1;
#else
	    j = ca;
#endif

	    if (j > 0) {
		switch (j) {
		case A_NAME:
		    if (flags & V_ATRNAME)
			save = 1;
		    break;
		case A_LOCK:
		    if (flags & V_ATRKEY)
			save = 1;
		    break;
		case A_LIST:
		case A_MONEY:
		    break;
		default:
		    save = 1;
		}
	    }
	    if (save) {
		got = atr_get_raw(i, j);
#ifndef STANDALONE
                if ( key && a && *a->name )
		   fprintf(f, ">%s\n%s\n", a->name, got);
                else
#endif
		   fprintf(f, ">%d\n%s\n", j, got);
	    }
	}
	fprintf(f, "<\n");
    }
    return 0;
}

int 
remote_read_sanitize(FILE *f, dbref i, int db_format, int flags)
{
   int i_ref, i_count;
   char *t_str;
   BOOLEXP *tempbool;

   i_count = 0;
   i_ref = getref(f);
   if ( feof(f) || (i_ref < 0) || (i_ref > 7) )
      return(1);
   if (!(flags & V_ATRNAME)) 
      t_str = (char *)getstring_noalloc(f); /* Player name */
   if ( feof(f) || !*t_str )
      return(1);
   t_str = (char *)getstring_noalloc(f); /* Location */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (flags & V_ZONE)
      t_str = (char *)getstring_noalloc(f); /* Zone */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   t_str = (char *)getstring_noalloc(f); /* Contents */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   t_str = (char *)getstring_noalloc(f); /* Exits */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (flags & V_LINK)
      t_str = (char *)getstring_noalloc(f); /* Link */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   t_str = (char *)getstring_noalloc(f); /* Next */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (!(flags & V_ATRKEY)) {	/* Lock */
      tempbool = getboolexp(f);
      free_boolexp(tempbool);
   }
   if ( feof(f) )
      return(1);
   t_str = (char *)getstring_noalloc(f); 	/* Owner */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (flags & V_PARENT)
      t_str = (char *)getstring_noalloc(f);	/* Parent */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (!(flags & V_ATRMONEY))
      t_str = (char *)getstring_noalloc(f);	/* Pennies */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   t_str = (char *)getstring_noalloc(f);	/* Flags */
   if ( feof(f) || (*t_str == '>') )
      return(1);
   if (flags & V_XFLAGS) {
      t_str = (char *)getstring_noalloc(f);	/* Flags2 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f); 	/* Flags3 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Flags4 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Toggles */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Toggles2 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Toggles3 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Toggles4 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Powers */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Powers2 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Depowers */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Depowers2 */
      if ( feof(f) || (*t_str == '>') )
         return(1);
      t_str = (char *)getstring_noalloc(f);	/* Zones - Rhost */
      while ( *t_str && ( strstr((char *)t_str, (char *)"-1") == NULL ) ) {
         if ( feof(f) || (*t_str == '>') )
            return(1);
         t_str = (char *)getstring_noalloc(f);	/* Zones - Rhost */
      }
      if ( feof(f) )
         return(1);
   }
   if (!get_list(f, i, 2, &i_count)) {
      return(1);
   }
   if ( feof(f) )
      return(1);
   return(0);
}

int
remote_read_obj(FILE *f, dbref i, int db_format, int flags, int *i_count)
{
   int i_ref;
   char *t_str;
   BOOLEXP *tempbool;

   
   i_ref = remote_read_sanitize(f, i, db_format, flags);
   if ( i_ref != 0 ) {
      return(3);
   }
   rewind(f);
   i_ref = getref(f);
   if ( i_ref != Typeof(i) ) {
      return(1);
   }
   if (!(flags & V_ATRNAME)) {
      t_str = (char *)getstring_noalloc(f);
      if ( Typeof(i) != TYPE_PLAYER ) /* Ignore name if player */
         s_Name(i, t_str);
   }
   i_ref = getref(f); 		/* Grab location - toss it away */
   if (flags & V_ZONE)
      i_ref = getref(f); 	/* Grab Zone - toss it away */
   i_ref = getref(f); 		/* Grab Contents - toss it away */
   i_ref = getref(f);		/* Grab Exits - toss it away */
   if (flags & V_LINK)
      i_ref = getref(f);	/* Grab Link - toss it away */
   i_ref = getref(f);		/* Grab Next - toss it away */
   if (!(flags & V_ATRKEY)) {
      tempbool = getboolexp(f);
      atr_add_raw(i, A_LOCK, unparse_boolexp_quiet(1, tempbool));
      free_boolexp(tempbool);
   }
   i_ref = getref(f);		/* Grab owner - toss it away */
   if (flags & V_PARENT) {
      i_ref = getref(f);	/* Grab parent - settem if valid */
      if ( Good_chk(i_ref) )
         s_Parent(i, i_ref);
   }
   if (!(flags & V_ATRMONEY))
      i_ref = getref(f);	/* Grab pennies - toss it away */
   i_ref = getref(f);		/* Grab flags - settem */
   s_Flags(i, i_ref);
   if (flags & V_XFLAGS) {
      i_ref = getref(f);	/* Grab flags2 - settem */
      s_Flags2(i, i_ref);
      i_ref = getref(f);	/* Grab flags3 - settem */
      s_Flags3(i, i_ref);
      i_ref = getref(f);	/* Grab flags4 - settem */
      s_Flags4(i, i_ref);
      i_ref = getref(f);	/* Grab Toggles - settem */
      s_Toggles(i, i_ref);
      i_ref = getref(f);	/* Grab Toggles2 - settem */
      s_Toggles2(i, i_ref);
      i_ref = getref(f);	/* Grab Toggles3 - settem */
      s_Toggles3(i, i_ref);
      i_ref = getref(f);	/* Grab Toggles4 - settem */
      s_Toggles4(i, i_ref);
      i_ref = getref(f);	/* Grab Powers1 - settem */
      s_Toggles5(i, i_ref);
      i_ref = getref(f);	/* Grab Powers2 - settem */
      s_Toggles6(i, i_ref);
      i_ref = getref(f);	/* Grab DePowers1 - settem */
      s_Toggles7(i, i_ref);
      i_ref = getref(f);	/* Grab DePowers2 - settem */
      s_Toggles8(i, i_ref);
      db[i].zonelist = NULL;	/* Grab zones - settem if valid */
      for( i_ref = getref(f); 
         i_ref != -1; 
         i_ref = getref(f)) {
         if ( Good_chk(i_ref) && (ZoneMaster(i_ref) || (ZoneMaster(i) && !ZoneMaster(i_ref))) )
            zlist_add(i, i_ref); 
      }
   }
   if (!get_list(f, i, 1, i_count)) {
      fprintf(stderr, "\nError reading attrs for object #%d\n", i);
      return(2);
   }
   return(0);
}

void
remote_write_obj(FILE *f, dbref i, int db_format, int flags)
{
   putref(f, Typeof(i));
   (void) db_write_object(f, i, db_format, flags, 1);
}

dbref 
db_write(FILE * f, int format, int version)
{
    dbref i;
    int flags;
    VATTR *vp;

    /* no debugging stack manip because we could be forked */

    al_store();
    switch (format) {
    case F_MUSH:
	flags = version;
	break;
    default:
	fprintf(stderr, "Can only write MUSH format.\n");
	return -1;
    }
#ifdef STANDALONE
    fprintf(stderr, "Writing ");
    fflush(stderr);
#endif
    i = mudstate.attr_next;
    fprintf(f, "+V%d\n+S%d\n+N%d\n", flags, mudstate.db_top, i);

    /* Dump user-named attribute info */

    vp = vattr_first();
    while (vp != NULL) {
	if (!(vp->flags & AF_DELETED))
	    fprintf(f, "+A%d\n%d:%s\n",
		    vp->number, vp->flags, vp->name);
	vp = vattr_next(vp);
    }

    DO_WHOLE_DB(i) {
	if (!(i % 25)) {
	    cache_reset(0);
	}
#ifdef STANDALONE
	if (!(i % 100)) {
	    fputc('.', stderr);
	    fflush(stderr);
	}
#endif
	fprintf(f, "!%d\n", i);
	db_write_object(f, i, format, flags, 0);
    }
    fputs("***END OF DUMP***\n", f);
    fflush(f);
#ifdef STANDALONE
    fprintf(stderr, "\n");
    fflush(stderr);
#endif
    return (mudstate.db_top);
}
