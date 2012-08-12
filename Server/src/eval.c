/* eval.c - command evaulation and cracking */

#ifdef SOLARIS
/* borked declarations in Solaris header files */
char *index(const char *, int);
#endif

#include <ctype.h>
#include <string.h>
#include "copyright.h"
#include "autoconf.h"

#include "db.h"
#include "externs.h"
#include "attrs.h"
#include "functions.h"
#include "alloc.h"
#include "rhost_ansi.h"
#include "command.h"
#include "match.h"

#include "debug.h"
#define FILENUM EVAL_C

extern dbref FDECL(match_thing_quiet, (dbref, char *));

/* ---------------------------------------------------------------------------
 * parse_to: Split a line at a character, obeying nesting.  The line is
 * destructively modified (a null is inserted where the delimiter was found)
 * dstr is modified to point to the char after the delimiter, and the function
 * return value points to the found string (space compressed if specified).
 * If we ran off the end of the string without finding the delimiter, dstr is
 * returned as NULL.
 */

static char *
parse_to_cleanup(int eval, int first, char *cstr,
		 char *rstr, char *zstr)
{
    DPUSH; /* #59 */
    if ((mudconf.space_compress || (eval & EV_STRIP_TS)) &&
	!first && (cstr[-1] == ' '))
	zstr--;
    if ((eval & EV_STRIP_AROUND) && (*rstr == '{') && (zstr[-1] == '}')) {
	rstr++;
	if (mudconf.space_compress || (eval & EV_STRIP_LS))
	    while (*rstr && isspace((int)*rstr))
		rstr++;
	rstr[-1] = '\0';
	zstr--;
	if (mudconf.space_compress || (eval & EV_STRIP_TS))
	    while (zstr[-1] && isspace((int)(zstr[-1])))
		zstr--;
	*zstr = '\0';
    }
    *zstr = '\0';
    DPOP; /* #59 */
    return rstr;
}

#define NEXTCHAR \
	if (cstr == zstr) { \
		cstr++; \
		zstr++; \
	} else \
		*zstr++ = *cstr++

char *
parse_to(char **dstr, char delim, int eval)
{
#define stacklim 32
    char stack[stacklim];
    char *rstr, *cstr, *zstr;
    int sp, tp, first, bracketlev;

    DPUSH; /* #60 */
    if ((dstr == NULL) || (*dstr == NULL)) {
        DPOP; /* #60 */
	return NULL;
    }
    if (**dstr == '\0') {
	rstr = *dstr;
	*dstr = NULL;
        DPOP; /* #60 */
	return rstr;
    }
    sp = 0;
    first = 1;
    rstr = *dstr;
    if (mudconf.space_compress | (eval & EV_STRIP_LS)) {
	while (*rstr && isspace((int)*rstr))
	    rstr++;
	*dstr = rstr;
    }
    zstr = cstr = rstr;
    while (*cstr) {
	switch (*cstr) {
	case '\\':		/* general escape */
	case '%':		/* also escapes chars */
	  if ((*cstr == '\\') && (eval & EV_STRIP_ESC)) {
	    cstr++;
	  } else {
	    NEXTCHAR;
	  }
	  if (*cstr) {
	    NEXTCHAR;
	  }
	    first = 0;
	    break;
	case ']':
	case ')':
	    for (tp = sp - 1; (tp >= 0) && (stack[tp] != *cstr); tp--);

	    /* If we hit something on the stack, unwind to it
	     * Otherwise (it's not on stack), if it's our delim
	     * we are done, and we convert the delim to a null
	     * and return a ptr to the char after the null.
	     * If it's not our delimiter, skip over it normally */

	    if (tp >= 0)
		sp = tp;
	    else if (*cstr == delim) {
		rstr = parse_to_cleanup(eval, first,
					cstr, rstr, zstr);
		*dstr = ++cstr;
                DPOP; /* #60 */
		return rstr;
	    }
	    first = 0;
	    NEXTCHAR;
	    break;
	case '{':
	    bracketlev = 1;
	    if (eval & EV_STRIP) {
		cstr++;
	    } else {
		NEXTCHAR;
	    }
	    while (*cstr && (bracketlev > 0)) {
		switch (*cstr) {
		case '\\':
		case '%':
		    if (cstr[1]) {
			if ((*cstr == '\\') &&
			    (eval & EV_STRIP_ESC))
			    cstr++;
			else
			    NEXTCHAR;
		    }
		    break;
		case '{':
		    bracketlev++;
		    break;
		case '}':
		    bracketlev--;
		    break;
		}
		if (bracketlev > 0) {
		    NEXTCHAR;
		}
	    }
	    if ((eval & EV_STRIP) && (bracketlev == 0)) {
		cstr++;
	    } else if (bracketlev == 0) {
		NEXTCHAR;
	    }
	    first = 0;
	    break;
	default:
	    if ((*cstr == delim) && (sp == 0)) {
		rstr = parse_to_cleanup(eval, first,
					cstr, rstr, zstr);
		*dstr = ++cstr;
                DPOP; /* #60 */
		return rstr;
	    }
	    switch (*cstr) {
	    case ' ':		/* space */
		if (mudconf.space_compress) {
		    if (first)
			rstr++;
		    else if (cstr[-1] == ' ')
			zstr--;
		}
		break;
	    case '[':
		if (sp < stacklim)
		    stack[sp++] = ']';
		first = 0;
		break;
	    case '(':
		if (sp < stacklim)
		    stack[sp++] = ')';
		first = 0;
		break;
	    default:
		first = 0;
	    }
	    NEXTCHAR;
	}
    }
    rstr = parse_to_cleanup(eval, first, cstr, rstr, zstr);
    *dstr = NULL;
    DPOP; /* #60 */
    return rstr;
}

/* ---------------------------------------------------------------------------
 * parse_arglist: Parse a line into an argument list contained in lbufs.
 * A pointer is returned to whatever follows the final delimiter.
 * If the arglist is unterminated, a NULL is returned.  The original arglist 
 * is destructively modified.
 */

char *
parse_arglist(dbref player, dbref cause, dbref caller, char *dstr, 
              char delim, dbref eval,
	      char *fargs[], dbref nfargs, char *cargs[],
	      dbref ncargs)
{
    char *rstr, *tstr;
    int arg, peval;
    DPUSH; /* #61 */

    for (arg = 0; arg < nfargs; arg++)
	fargs[arg] = NULL;
    if (dstr == NULL) {
        DPOP; /* #61 */
	return NULL;
    }
    rstr = parse_to(&dstr, delim, 0);
    arg = 0;

    if (eval & EV_EVAL) {
	peval = 0;
	if (eval & EV_STRIP_LS)
	    peval |= EV_STRIP_LS;
	if (eval & EV_STRIP_TS)
	    peval |= EV_STRIP_TS;
	if (eval & EV_STRIP_ESC)
	    peval |= EV_STRIP_ESC;
	if (eval & EV_TOP)
	    peval |= EV_TOP;
	if (eval & EV_NOTRACE)
	    peval |= EV_NOTRACE;
    } else {
	peval = eval;
    }

    while ((arg < nfargs) && rstr) {
	if (arg < (nfargs - 1))
	    tstr = parse_to(&rstr, ',', peval);
	else
	    tstr = parse_to(&rstr, '\0', peval);
	if (eval & EV_EVAL) {
	    fargs[arg] = exec(player, cause, caller, eval | EV_FCHECK, tstr,
			      cargs, ncargs);
	} else {
	    fargs[arg] = alloc_lbuf("parse_arglist");
	    strcpy(fargs[arg], tstr);
	}
	arg++;
    }
    DPOP; /* #61 */
    return dstr;
}

/* ---------------------------------------------------------------------------
 * exec: Process a command line, evaluating function calls and %-substitutions.
 */

int 
get_gender(dbref player)
{
    char first, *atr_gotten;
    dbref aowner;
    int aflags;

    DPUSH; /* #62 */
    atr_gotten = atr_pget(player, A_SEX, &aowner, &aflags);
    first = *atr_gotten;
    free_lbuf(atr_gotten);
    switch (first) {
    case 'P':
    case 'p':
        DPOP; /* #62 */
	return 4;
    case 'M':
    case 'm':
        DPOP; /* #62 */
	return 3;
    case 'F':
    case 'f':
    case 'W':
    case 'w':
        DPOP; /* #62 */
	return 2;
    default:
        DPOP; /* #62 */
	return 1;
    }
/*NOTREACHED */
    DPOP; /* #62 */
    return 0;
}

/* ---------------------------------------------------------------------------
 * Trace cache routines.
 */

typedef struct tcache_ent TCENT;
struct tcache_ent {
    dbref player;
    char *orig;
    char *result;
    struct tcache_ent *next;
}         *tcache_head;
int tcache_top, tcache_count;

void 
NDECL(tcache_init)
{
    DPUSH; /* #63 */
    tcache_head = NULL;
    tcache_top = 1;
    tcache_count = 0;
    DPOP; /* #63 */
}

int 
NDECL(tcache_empty)
{
    DPUSH; /* #64 */
    if (tcache_top) {
	tcache_top = 0;
	tcache_count = 0;
        DPOP; /* #64 */
	return 1;
    }
    DPOP; /* #64 */
    return 0;
}

static void 
tcache_add(dbref player, char *orig, char *result)
{
    char *tp;
    TCENT *xp;
    DPUSH; /* #65 */

    if (strcmp(orig, result)) {
	tcache_count++;
	if (tcache_count <= mudconf.trace_limit) {
	    xp = (TCENT *) alloc_sbuf("tcache_add.sbuf");
	    tp = alloc_lbuf("tcache_add.lbuf");
	    strcpy(tp, result);
            xp->player = player;
	    xp->orig = orig;
	    xp->result = tp;
	    xp->next = tcache_head;
	    tcache_head = xp;
	} else {
	    free_lbuf(orig);
	}
    } else {
	free_lbuf(orig);
    }
    DPOP; /* #65 */
}

static void 
tcache_finish(void)
{
    TCENT *xp;
    char *tpr_buff = NULL, *tprp_buff = NULL, *s_aptext = NULL, *s_aptextptr = NULL, *s_strtokr = NULL, *tbuff = NULL;
    int i_apflags, i_targetlist;
    dbref i_apowner, passtarget, targetlist[LBUF_SIZE], i;
    ATTR *ap_log;

    DPUSH; /* #66 */

    for (i = 0; i < LBUF_SIZE; i++)
       targetlist[i]=-2000000;

    tprp_buff = tpr_buff = alloc_lbuf("tcache_finish");
    tbuff = alloc_lbuf("bounce_on_notify_exec");
    while (tcache_head != NULL) {
	xp = tcache_head;
	tcache_head = xp->next;
        tprp_buff = tpr_buff;
	notify(Owner(xp->player),
	       safe_tprintf(tpr_buff, &tprp_buff, "%s(#%d)} '%s' -> '%s'", Name(xp->player), xp->player,
		       xp->orig, xp->result));

       if ( Bouncer(xp->player) ) {
            ap_log = atr_str("BOUNCEFORWARD");
            if ( ap_log ) {
               s_aptext = atr_get(xp->player, ap_log->number, &i_apowner, &i_apflags);
               if ( s_aptext && *s_aptext ) {
                  s_aptextptr = strtok_r(s_aptext, " ", &s_strtokr);
                  i_targetlist = 0;
                  for (i = 0; i < LBUF_SIZE; i++)
                     targetlist[i]=-2000000;
                  while ( s_aptextptr ) {
                     passtarget = match_thing_quiet(xp->player, s_aptextptr);
                     for (i = 0; i < LBUF_SIZE; i++) {
                        if ( (targetlist[i] == -2000000) || (targetlist[i] == passtarget) )
                           break;
                     }
                     if ( (targetlist[i] == -2000000) && Good_chk(passtarget) && isPlayer(passtarget) && (passtarget != xp->player) && (Owner(xp->player) != passtarget) ) {
                        if ( !No_Ansi_Ex(passtarget) )
                           sprintf(tbuff, "%sBounce [#%d]>%s %.3950s", ANSI_HILITE, xp->player, ANSI_NORMAL, tpr_buff);
                        else
                           sprintf(tbuff, "Bounce [#%d]> %.3950s", xp->player, tpr_buff);
                        notify_quiet(passtarget, tbuff);
                     }
                     s_aptextptr = strtok_r(NULL, " ", &s_strtokr);
                     targetlist[i_targetlist] = passtarget;
                     i_targetlist++;
                  }
               }
               free_lbuf(s_aptext);
            }
        }


	free_lbuf(xp->orig);
	free_lbuf(xp->result);
	free_sbuf(xp);
    }
    free_lbuf(tbuff);
    free_lbuf(tpr_buff);
    tcache_top = 1;
    tcache_count = 0;
    DPOP; /* #66 */
}

#ifdef ZENTY_ANSI
/* Ok, let's try to do the accents, baybee 
 * Code ripped from MUX 2.6 with permission 
 */

static const unsigned char AccentCombo1[256] =
{   
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
//  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1
    0,18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,19, 0,20,17,  // 3
    0, 1, 0, 3,24, 5, 0, 0, 0, 7, 0, 0, 0, 0, 9,11,  // 4
   22, 0, 0, 0, 0,13, 0, 0, 0,15, 0, 0, 0, 0, 0, 0,  // 5
    0, 2, 0, 4, 0, 6, 0, 0, 0, 8, 0, 0, 0, 0,10,12,  // 6
   23, 0, 0,21, 0,14, 0, 0, 0,16, 0, 0, 0, 0, 0, 0,  // 7
       
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // B
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // C
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // D
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // E
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // F
};  
        
// Accent:      `'^~:o,u"B|-&Ee
//
static const unsigned char AccentCombo2[256] =
{
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
//
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1
    0, 0, 9, 0, 0, 0,13, 2, 0, 0, 0, 0, 7,12, 0, 0,  // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0,  // 3
    0, 0,10, 0, 0,14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,  // 5
    1, 0, 0, 0, 0,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,  // 6
    0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0,11, 0, 4, 0,  // 7
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // B
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // C
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // D
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // E
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // F
};

static const unsigned char AccentCombo3[24][16] =
{
    //  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
    //        `     '     ^     ~     :     o     ,     u     "     B     |     -     &     E     e
    //
    {  0x00, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x00 }, //  1 'A'
    {  0x00, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE6 }, //  2 'a'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  3 'C'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  4 'c'
    {  0x00, 0xC8, 0xC9, 0xCA, 0x00, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  5 'E'
    {  0x00, 0xE8, 0xE9, 0xEA, 0x00, 0xEB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  6 'e'
    {  0x00, 0xCC, 0xCD, 0xCE, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  7 'I'
    {  0x00, 0xEC, 0xED, 0xEE, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  8 'i'
    
    {  0x00, 0x00, 0x00, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //  9 'N'
    {  0x00, 0x00, 0x00, 0x00, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 10 'n'
    {  0x00, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 11 'O'
    {  0x00, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00 }, // 12 'o'
    {  0x00, 0xD9, 0xDA, 0xDB, 0x00, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 13 'U'
    {  0x00, 0xF9, 0xFA, 0xFB, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 14 'u'
    {  0x00, 0x00, 0xDD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 15 'Y'
    {  0x00, 0x00, 0xFD, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 16 'y'
            
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 17 '?'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 18 '!'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 19 '<'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 20 '>'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 21 's'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDE, 0x00, 0x00, 0x00, 0x00 }, // 22 'P'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00 }, // 23 'p'
    {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00 }  // 24 'D'
};      

static const int mux_isprint[256] =
{
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
//
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  // 7

    0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0,  // 8
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1,  // 9
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // A
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // B
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // C
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // D
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // E
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1   // F
};

void parse_accents(char *string, char *buff, char **bufptr)
{
   char *bufc, s_intbuf[4];
   unsigned char ch1, ch2, ch;
   int accent_toggle;
   int i_extendallow, i_extendcnt, i_extendnum;
   bufc = *bufptr;
   accent_toggle = 0;
   i_extendallow = 3;
   i_extendcnt = 0;
   s_intbuf[3] = '\0';
   while ( *string && ((bufc - buff) < (LBUF_SIZE-10)) ) {
      if (*string == '\\') {
         string++;
         if (*string != '%') {
            safe_chr('\\', buff, &bufc);
         }
         if (*string && (*string != '\\'))
            safe_chr(*string, buff, &bufc);
      } else if (*string == '%') {
         string++;
         if (*string == '\\') {
            safe_chr(*string, buff, &bufc);
         } else if ( (*string == '%') && (*(string+1) == 'f') ) {
            safe_str("%f", buff, &bufc);
            string++;
         } else if ( (*string == '%') && (*(string+1) == '<') ) { 
            string+=2;
            while ( *string ) {
               if ( isdigit(*(string)) && isdigit(*(string+1)) && isdigit(*(string+2)) ) {
                  s_intbuf[0] = *(string);
                  s_intbuf[1] = *(string+1);
                  s_intbuf[2] = *(string+2);
                  i_extendnum = atoi(s_intbuf);
                  if ( (i_extendnum >= 160) && (i_extendnum <= 250) )
                     safe_chr((char) i_extendnum, buff, &bufc);
                  else {
                     switch(i_extendnum) {
                        case 251:
                        case 252: safe_chr('u', buff, &bufc);
                                  break;
                        case 253:
                        case 255: safe_chr('y', buff, &bufc);
                                  break;
                        case 254: safe_chr('p', buff, &bufc);
                                  break;
                     }
                  }
                  i_extendcnt+=3;
                  string+=3;
                  if (*string == '>' ) {
                     break;
                  }
               } else {
                  safe_chr(*string, buff, &bufc);
                  break;
               }
            }
         } else if (*string != 'f') { 
            safe_chr('%', buff, &bufc);
            safe_chr(*string, buff, &bufc);
         } else {
            ch = (unsigned char)*(++string);
            if ( ch == 'n' ) {
               accent_toggle = 0;
            } else {
               ch2 = AccentCombo2[(int)ch];
               if ( ch2 > 0 ) {
                  accent_toggle = 1;
               } else {
                  if ( !isprint(*string) )
                     safe_chr(*string, buff, &bufc);
               }
            }
         }
      } else {
         if ( accent_toggle ) {
            ch1 = AccentCombo1[(int)*string];
            if ( ch1 > 0 ) {
               ch = AccentCombo3[(int)(ch1 - 1)][(int)ch2];
               if ( !mux_isprint[(int)ch] ) 
                  safe_chr(*string, buff, &bufc);
               else
                  safe_chr(ch, buff, &bufc);
            } else {
               safe_chr(*string, buff, &bufc);
            }
         } else {
            safe_chr(*string, buff, &bufc);
         }
      }
      if ( *string )
        string++;
   }
   *bufptr = bufc;
}

// Change %c/%x substitutions into real ansi now.
void parse_ansi(char *string, char *buff, char **bufptr)
{
    char *bufc, s_twochar[3], s_final[80];
    int i_tohex;

    memset(s_twochar, '\0', sizeof(s_twochar));
    memset(s_final, '\0', sizeof(s_final));
    bufc = *bufptr;
    while(*string && ((bufc - buff) < (LBUF_SIZE-24))) {
        if(*string == '\\') {
            string++;
            if(*string != '%') {
                safe_chr('\\', buff, &bufc);
            }
            if(*string && (*string != '\\'))
                safe_chr(*string, buff, &bufc);
        } else if(*string == '%') {
            string++;
            if(*string == '\\') {
                safe_chr(*string, buff, &bufc);
            } else if((*string == '%') && (*(string+1) == SAFE_CHR )) {
                safe_str((char*)SAFE_CHRST, buff, &bufc);
                string++;
            } else if(*string != SAFE_CHR) {
                safe_chr('%', buff, &bufc);
                safe_chr(*string, buff, &bufc);
            } else {
                switch (*++string) {
                case '\0':
                    safe_chr(*string, buff, &bufc);
                    break;
                case '0': /* Do XTERM color here */
                    switch ( *(string+1) ) {
                       case 'X': /* Background color */
                          if ( (*(string+2) && isxdigit(*(string+2))) && (*(string+3) && isxdigit(*(string+3))) ) {
                             s_twochar[0]=*(string+2);
                             s_twochar[1]=*(string+3);
                             sscanf(s_twochar, "%x", &i_tohex);
                             string+=3;
                             sprintf(s_final, "%s%dm", (char *)ANSI_XTERM_BG, i_tohex);
                             safe_str(s_final, buff, &bufc);
                             sprintf(s_final, "%dm", i_tohex);
                          }
                          break;
                       case 'x': /* Foreground color */
                          if ( (*(string+2) && isxdigit(*(string+2))) && (*(string+3) && isxdigit(*(string+3))) ) {
                             s_twochar[0]=*(string+2);
                             s_twochar[1]=*(string+3);
                             sscanf(s_twochar, "%x", &i_tohex);
                             sprintf(s_final, "%s%dm", (char *)ANSI_XTERM_FG, i_tohex);
                             string+=3;
                             safe_str(s_final, buff, &bufc);
                             sprintf(s_final, "%dm", i_tohex);
                          }
                          break;
                       default:
                          safe_str((char *)SAFE_CHRST, buff, &bufc);
                          safe_chr(*string, buff, &bufc);
                          break;
                    }  
                    break;
                case 'n':
                    safe_str((char *) ANSI_NORMAL, buff, &bufc);
                    break;
                case 'f':
                    if ( mudconf.global_ansimask & MASK_BLINK )
                        safe_str((char *) ANSI_BLINK, buff, &bufc);
                    break;
                case 'u':
                    if ( mudconf.global_ansimask & MASK_UNDERSCORE )
                        safe_str((char *) ANSI_UNDERSCORE, buff, &bufc);
                    break;
                case 'i':
                    if ( mudconf.global_ansimask & MASK_INVERSE )
                        safe_str((char *) ANSI_INVERSE, buff, &bufc);
                    break;
                case 'h':
                    if ( mudconf.global_ansimask & MASK_HILITE )
                        safe_str((char *) ANSI_HILITE, buff, &bufc);
                    break;
                case 'x':
                    if ( mudconf.global_ansimask & MASK_BLACK )
                        safe_str((char *) ANSI_BLACK, buff, &bufc);
                    break;
                case 'X':
                    if ( mudconf.global_ansimask & MASK_BBLACK )
                        safe_str((char *) ANSI_BBLACK, buff, &bufc);
                    break;
                case 'r':
                    if ( mudconf.global_ansimask & MASK_RED )
                        safe_str((char *) ANSI_RED, buff, &bufc);
                    break;
                case 'R':
                    if ( mudconf.global_ansimask & MASK_BRED )
                        safe_str((char *) ANSI_BRED, buff, &bufc);
                    break;
                case 'g':
                    if ( mudconf.global_ansimask & MASK_GREEN )
                        safe_str((char *) ANSI_GREEN, buff, &bufc);
                    break;
                case 'G':
                    if ( mudconf.global_ansimask & MASK_BGREEN )
                        safe_str((char *) ANSI_BGREEN, buff, &bufc);
                    break;
                case 'y':
                    if ( mudconf.global_ansimask & MASK_YELLOW )
                        safe_str((char *) ANSI_YELLOW, buff, &bufc);
                    break;
                case 'Y':
                    if ( mudconf.global_ansimask & MASK_BYELLOW )
                        safe_str((char *) ANSI_BYELLOW, buff, &bufc);
                    break;
                case 'b':
                    if ( mudconf.global_ansimask & MASK_BLUE )
                        safe_str((char *) ANSI_BLUE, buff, &bufc);
                    break;
                case 'B':
                    if ( mudconf.global_ansimask & MASK_BBLUE )
                        safe_str((char *) ANSI_BBLUE, buff, &bufc);
                    break;
                case 'm':
                    if ( mudconf.global_ansimask & MASK_MAGENTA )
                        safe_str((char *) ANSI_MAGENTA, buff, &bufc);
                    break;
                case 'M':
                    if ( mudconf.global_ansimask & MASK_BMAGENTA )
                        safe_str((char *) ANSI_BMAGENTA, buff, &bufc);
                    break;
                case 'c':
                    if ( mudconf.global_ansimask & MASK_CYAN )
                        safe_str((char *) ANSI_CYAN, buff, &bufc);
                    break;
                case 'C':
                    if ( mudconf.global_ansimask & MASK_BCYAN )
                        safe_str((char *) ANSI_BCYAN, buff, &bufc);
                    break;
                case 'w':
                    if ( mudconf.global_ansimask & MASK_WHITE )
                        safe_str((char *) ANSI_WHITE, buff, &bufc);
                    break;
                case 'W':
                    if ( mudconf.global_ansimask & MASK_BWHITE )
                        safe_str((char *) ANSI_BWHITE, buff, &bufc);
                    break;
                default:
                    safe_str((char *)SAFE_CHRST, buff, &bufc);
                    safe_chr(*string, buff, &bufc);
                    break;
                }
            }
        } else {
            safe_chr(*string, buff, &bufc);
        }
        if ( *string )
           string++;
    }
    // toss in the normal
    safe_str(ANSI_NORMAL, buff, &bufc); 
    *bufptr = bufc;
}

#endif

char *
exec(dbref player, dbref cause, dbref caller, int eval, char *dstr,
     char *cargs[], int ncargs)
{
/* MAX_ARGS is located in externs.h - default is 30 */
#ifdef MAX_ARGS
#define	NFARGS	MAX_ARGS
#else
#define NFARGS 30
#endif

    char *fargs[NFARGS], *sub_txt, *sub_buf, *sub_txt2, *sub_buf2, *orig_dstr, sub_char;
    char *buff, *bufc, *tstr, *tbuf, *tbufc, *savepos, *atr_gotten, *savestr;
    char savec, ch, *ptsavereg, *savereg[MAX_GLOBAL_REGS], *t_bufa, *t_bufb;
    static char tfunbuff[33];
    dbref aowner, twhere, sub_aowner;
    int at_space, nfargs, gender, i, j, alldone, aflags, feval, sub_aflags;
    int is_trace, is_top, save_count, x, y, z, w, sub_delim, sub_cntr, sub_value, sub_valuecnt;
    FUN *fp;
    UFUN *ufp;
    ATTR *sub_ap;
    time_t starttme, endtme;
    struct itimerval cpuchk;
    double timechk, intervalchk;
    static unsigned long tstart, tend, tinterval;
#ifdef BANGS
    int bang_not, bang_string, bang_yes;
    char *tbangc;
#endif
    static const char *subj[5] =
    {"", "it", "she", "he", "they"};
    static const char *poss[5] =
    {"", "its", "her", "his", "their"};
    static const char *obj[5] =
    {"", "it", "her", "him", "them"};
    static const char *absp[5] =
    {"", "its", "hers", "his", "theirs"};

    DPUSH; /* #67 */
		
    feval = sub_delim = sub_cntr = sub_value = sub_valuecnt = 0;
    w = 0;
    mudstate.evalcount++;

    if (dstr == NULL) {
	RETURN(NULL); /* #67 */
    }

    if ( mudconf.func_nest_lim > 300 )
       mudconf.func_nest_lim = 300;
    tstart = 1000 * 100;
    getitimer(ITIMER_PROF, &cpuchk);
    tend = (cpuchk.it_value.tv_sec * 100) + (cpuchk.it_value.tv_usec / 10000);

    if ( tend <= tstart )
       tinterval = tstart - tend;
    else
       tinterval = 0;
    endtme = time(NULL);
    starttme = mudstate.chkcpu_stopper;

    if ( mudconf.cputimechk < 10 )
       timechk = 10;
    else if ( mudconf.cputimechk > 3600 )
       timechk = 3600;
    else
       timechk = mudconf.cputimechk;
    if ( mudconf.cpuintervalchk < 10 )
       intervalchk = 10;
    else if ( mudconf.cpuintervalchk > 100 )
       intervalchk = 100;
    else
       intervalchk = mudconf.cpuintervalchk;

    bufc = buff = alloc_lbuf("exec.buff");

    if ( mudstate.chkcpu_toggle || (((endtme - starttme) > timechk) && ((tinterval/100) > intervalchk)) ) {
        mudstate.chkcpu_toggle = 1;
        RETURN(buff); /* #67 */
    }

    if ( !Good_chk(player) || !Good_chk(caller) || !Good_chk(cause) ) {
        RETURN(buff); /* #67 */
    }
    // Requires strict ansi compliance, but it looks pretty.
    if ( mudstate.stack_val > mudconf.stack_limit 
	 &&
	 (mudconf.stack_limit > 0 || (mudconf.stack_limit = 1))) {
        mudstate.stack_toggle = 1;
        RETURN(buff); /* #67 */
    }
    if ( mudstate.sidefx_currcalls >= mudconf.sidefx_maxcalls 
	 &&
	 (mudconf.sidefx_maxcalls > 0 || (mudconf.sidefx_maxcalls = 1))) {
        mudstate.sidefx_toggle = 1;
        RETURN(buff); /* #67 */
    }

    at_space = 1;
    gender = -1;
    alldone = 0;
    is_trace = Trace(player) && !(eval & EV_NOTRACE);
    is_top = 0;
    mudstate.eval_rec++;

    /* If we are tracing, save a copy of the starting buffer */

    savestr = NULL;
    if (is_trace) {
	is_top = tcache_empty();
	savestr = alloc_lbuf("exec.save");
	strcpy(savestr, dstr);
    }
    if (index(dstr, ESC_CHAR)) {
	strcpy(dstr, strip_ansi(dstr));
    }
    while (*dstr && !alldone) {
      if ( mudstate.curr_percentsubs < mudconf.max_percentsubs )
	switch (*dstr) {
	case ' ':
	    /* A space.  Add a space if not compressing or if
	     * previous char was not a space */

	    if (!(mudconf.space_compress && at_space)) {
		safe_chr(' ', buff, &bufc);
		at_space = 1;
	    }
	    break;
	case '\\':
	    /* General escape.  Add the following char without
	     * special processing */

	    at_space = 0;
	    dstr++;
#ifdef ZENTY_ANSI
            // If the caracter after the \ is a commenting char, keep it
            if((*dstr == '\\') || (*dstr == '%'))
               safe_chr('\\', buff, &bufc);
#endif
	    if (*dstr)
               safe_chr(*dstr, buff, &bufc);
	    else
               dstr--;
	    break;
	case '[':
            mudstate.stack_val++;
	    /* Function start.  Evaluate the contents of the
	     * square brackets as a function.  If no closing
	     * bracket, insert the [ and continue. */

	    at_space = 0;
	    tstr = dstr++;
	    tbuf = parse_to(&dstr, ']', 0);
	    if (dstr == NULL) {
		safe_chr('[', buff, &bufc);
		dstr = tstr;
	    } else {
                mudstate.stack_val--;
		tstr = exec(player, cause, caller,
			    (eval | EV_FCHECK | EV_FMAND),
			    tbuf, cargs, ncargs);
		safe_str(tstr, buff, &bufc);
		free_lbuf(tstr);
		dstr--;
	    }
	    break;
	case '{':
            mudstate.stack_val++;
	    /* Literal start.  Insert everything up to the
	     * terminating } without parsing.  If no closing
	     * brace, insert the { and continue. */

	    at_space = 0;
	    tstr = dstr++;
	    tbuf = parse_to(&dstr, '}', 0);
	    if (dstr == NULL) {
		safe_chr('{', buff, &bufc);
		dstr = tstr;
	    } else {
                mudstate.stack_val--;
		if (!(eval & EV_STRIP)) {
		    safe_chr('{', buff, &bufc);
		}
		/* Preserve leading spaces (Felan) */

		if (*tbuf == ' ') {
		    safe_chr(' ', buff, &bufc);
		    tbuf++;
		}
		tstr = exec(player, cause, caller,
			    (eval & ~(EV_STRIP | EV_FCHECK)),
			    tbuf, cargs, ncargs);
		safe_str(tstr, buff, &bufc);
		if (!(eval & EV_STRIP)) {
		    safe_chr('}', buff, &bufc);
		}
		free_lbuf(tstr);
		dstr--;
	    }
	    break;

	case '%':
	    /* Percent-replace start.  Evaluate the chars following
	     * and perform the appropriate substitution. */

	    at_space = 0;
	    dstr++;
	    savec = *dstr;
	    savepos = bufc;
            mudstate.curr_percentsubs++;
            
            if ( mudstate.chkcpu_toggle || (mudstate.curr_percentsubs >= mudconf.max_percentsubs) ) {
                mudstate.tog_percentsubs = 1;
/*              RETURN(buff); */ /* #67 */
                break;
            }

            if ( mudstate.curr_percentsubs >= mudconf.max_percentsubs )
               break;
	    switch (savec) {
	    case '\0':		/* Null - all done */
		dstr--;
		break;
#ifdef ZENTY_ANSI            
            case '\\':
            safe_str("%\\", buff, &bufc);
            break;
#endif
	    case '%':		/* Percent - a literal % */
#ifdef ZENTY_ANSI            
               if(*(dstr + 1) == SAFE_CHR)
                  safe_str("%%", buff, &bufc);
               else if ( *(dstr + 1) == 'f' )
                  safe_str("%%", buff, &bufc);
               else
#endif                
                  safe_chr('%', buff, &bufc);            
		  break;
#ifndef NOEXTSUBS
#ifdef TINY_SUB
	    case 'x':
	    case 'X':		/* ansi subs */
#else
	    case 'c':
	    case 'C':		/* ansi subs */
#endif
#endif
                if ( (mudconf.sub_override & SUB_C) && 
                     !(mudstate.sub_overridestate & SUB_C) && 
                     Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_C");
                   if (sub_ap) {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( sub_txt  ) {
                         if ( *sub_txt ) {
                            mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_C;
                            sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                            mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_C;
                            safe_str(sub_buf, buff, &bufc);
                            free_lbuf(sub_txt);
                            free_lbuf(sub_buf);
                            break;
                         }
                         free_lbuf(sub_txt);
                      }
                   }
                } 
#ifdef ZENTY_ANSI
                // Leave the ansi code intact
                if(!(eval & EV_PARSE_ANSI)) {        
                    safe_chr('%', buff, &bufc);
                    safe_chr(SAFE_CHR, buff, &bufc);
                    break;
                }
#endif
		dstr++;
		if (mudstate.eval_rec != 1) {
		    if (!*dstr)
			dstr--;
		    break;
		}
		switch (*dstr) {
		case '\0':
		    dstr--;
		    break;
		case 'n':
		    safe_str((char *) ANSI_NORMAL, buff, &bufc);
		    break;
		case 'f':
                    if ( mudconf.global_ansimask & MASK_BLINK )
		       safe_str((char *) ANSI_BLINK, buff, &bufc);
		    break;
		case 'u':
                    if ( mudconf.global_ansimask & MASK_UNDERSCORE )
		       safe_str((char *) ANSI_UNDERSCORE, buff, &bufc);
		    break;
		case 'i':
                    if ( mudconf.global_ansimask & MASK_INVERSE )
		       safe_str((char *) ANSI_INVERSE, buff, &bufc);
		    break;
		case 'h':
                    if ( mudconf.global_ansimask & MASK_HILITE )
		       safe_str((char *) ANSI_HILITE, buff, &bufc);
		    break;
		case 'x':
                    if ( mudconf.global_ansimask & MASK_BLACK )
		       safe_str((char *) ANSI_BLACK, buff, &bufc);
		    break;
		case 'X':
                    if ( mudconf.global_ansimask & MASK_BBLACK )
		       safe_str((char *) ANSI_BBLACK, buff, &bufc);
		    break;
		case 'r':
                    if ( mudconf.global_ansimask & MASK_RED )
		       safe_str((char *) ANSI_RED, buff, &bufc);
		    break;
		case 'R':
                    if ( mudconf.global_ansimask & MASK_BRED )
		       safe_str((char *) ANSI_BRED, buff, &bufc);
		    break;
		case 'g':
                    if ( mudconf.global_ansimask & MASK_GREEN )
		       safe_str((char *) ANSI_GREEN, buff, &bufc);
		    break;
		case 'G':
                    if ( mudconf.global_ansimask & MASK_BGREEN )
		       safe_str((char *) ANSI_BGREEN, buff, &bufc);
		    break;
		case 'y':
                    if ( mudconf.global_ansimask & MASK_YELLOW )
		       safe_str((char *) ANSI_YELLOW, buff, &bufc);
		    break;
		case 'Y':
                    if ( mudconf.global_ansimask & MASK_BYELLOW )
		       safe_str((char *) ANSI_BYELLOW, buff, &bufc);
		    break;
		case 'b':
                    if ( mudconf.global_ansimask & MASK_BLUE )
		       safe_str((char *) ANSI_BLUE, buff, &bufc);
		    break;
		case 'B':
                    if ( mudconf.global_ansimask & MASK_BBLUE )
		       safe_str((char *) ANSI_BBLUE, buff, &bufc);
		    break;
		case 'm':
                    if ( mudconf.global_ansimask & MASK_MAGENTA )
		       safe_str((char *) ANSI_MAGENTA, buff, &bufc);
		    break;
		case 'M':
                    if ( mudconf.global_ansimask & MASK_BMAGENTA )
		       safe_str((char *) ANSI_BMAGENTA, buff, &bufc);
		    break;
		case 'c':
                    if ( mudconf.global_ansimask & MASK_CYAN )
		       safe_str((char *) ANSI_CYAN, buff, &bufc);
		    break;
		case 'C':
                    if ( mudconf.global_ansimask & MASK_BCYAN )
		       safe_str((char *) ANSI_BCYAN, buff, &bufc);
		    break;
		case 'w':
                    if ( mudconf.global_ansimask & MASK_WHITE )
		       safe_str((char *) ANSI_WHITE, buff, &bufc);
		    break;
		case 'W':
                    if ( mudconf.global_ansimask & MASK_BWHITE )
		       safe_str((char *) ANSI_BWHITE, buff, &bufc);
		    break;
		default:
		    safe_chr(*dstr, buff, &bufc);
		}
		break;
            case '<':		/* High-Bit/UTF8 characters */
                safe_str("%%<", buff, &bufc);
                break;
            case 'f':		/* Accents */
            case 'F':	
                if ( (mudconf.sub_override & SUB_R) && !(mudstate.sub_overridestate & SUB_R) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_F");
                   if (!sub_ap)
                      safe_str("%f", buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
                         safe_str("%f", buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_F;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
                            safe_str("%f", buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_F;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
                   safe_str("%f", buff, &bufc);
                break;
	    case 'r':		/* Carriage return */
	    case 'R':
                if ( (mudconf.sub_override & SUB_R) && !(mudstate.sub_overridestate & SUB_R) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_R");
                   if (!sub_ap)
		      safe_str((char *) "\r\n", buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str((char *) "\r\n", buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_R;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str((char *) "\r\n", buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_R;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str((char *) "\r\n", buff, &bufc);
		break;
	    case 't':		/* Tab */
	    case 'T':
                if ( (mudconf.sub_override & SUB_T) && !(mudstate.sub_overridestate & SUB_T) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_T");
                   if (!sub_ap)
		      safe_chr('\t', buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_chr('\t', buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_T;
                         sub_buf = exec(mudconf.hook_obj, cause, player, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_chr('\t', buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_T;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_chr('\t', buff, &bufc);
		break;
	    case 'B':		/* Blank */
	    case 'b':
		safe_chr(' ', buff, &bufc);
		break;
	    case '0':		/* Command argument number N */
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		i = (*dstr - '0') + (10 * mudstate.shifted);
		if ((i < ncargs) && (cargs[i] != NULL))
		    safe_str(cargs[i], buff, &bufc);
		break;
            case '-':
                if (ncargs >= 10) {
                   for (i=10; ((i < ncargs) && (i <= MAX_ARGS) && cargs[i] != NULL); i++) {
                    safe_str(cargs[i], buff, &bufc);
                      if ( i < (ncargs - 1) )
                       safe_chr(',', buff, &bufc);
                   }
                }
                break;
	    case 'V':		/* Variable attribute */
	    case 'v':
		dstr++;
		ch = ToUpper((int)*dstr);
		if (!*dstr)
		    dstr--;
		if ((ch < 'A') || (ch > 'Z'))
		    break;
		i = 100 + ch - 'A';
		atr_gotten = atr_pget(player, i, &aowner,
				      &aflags);
		safe_str(atr_gotten, buff, &bufc);
		free_lbuf(atr_gotten);
		break;
#ifndef NO_ENH
	    case 'Q':
	    case 'q':
		dstr++;
                if ( *dstr == '<' ) {
                   sub_cntr = 0;
                   t_bufb = t_bufa = alloc_sbuf("sub_include_setq");
                   orig_dstr = dstr;
                   dstr++;
                   while ( *dstr && (*dstr != '>') && (sub_cntr < (SBUF_SIZE - 1)) ) {
                      *t_bufb = *dstr;
                      dstr++;
                      t_bufb++;
                      sub_cntr++;
                   }
                   *t_bufb = '\0';
                   if ( (*dstr != '>') ) {
                      dstr = orig_dstr;
                   } else {
                      for ( sub_cntr = 0 ; sub_cntr < MAX_GLOBAL_REGS; sub_cntr++ ) {
                         if (  mudstate.global_regsname[sub_cntr] &&
                               !stricmp(mudstate.global_regsname[sub_cntr], t_bufa) ) {
		            safe_str(mudstate.global_regs[sub_cntr], buff, &bufc);
                            break;
                         }
                      }
                   }
                   free_sbuf(t_bufa);
                } else {
		   i = (*dstr - '0');
#ifdef EXPANDED_QREGS
                   if ( *dstr && isalpha((int)*dstr) ) {
                      for ( w = 0; w < 37; w++ ) {
                         if ( mudstate.nameofqreg[w] == tolower(*dstr) )
                            break;
                      }
                      i = w;
                   }  else if ( (i < 0 || i > 9) ) {
                      i = -1;
                   }
		   if (!*dstr)
		       dstr--;
		   if ((i >= 0) && (i <= 35) &&
		       mudstate.global_regs[i]) {
		       safe_str(mudstate.global_regs[i],
			        buff, &bufc);
		   }
#else
		   if (!*dstr)
		       dstr--;
		   if ((i >= 0) && (i <= 9) &&
		       mudstate.global_regs[i]) {
		       safe_str(mudstate.global_regs[i],
			        buff, &bufc);
		   }
#endif
                }
		break;
#endif
	    case 'O':		/* Objective pronoun */
	    case 'o':
		if (gender < 0)
		    gender = get_gender(cause);
		if (!gender)
		    tbuf = Name(cause);
		else
		    tbuf = (char *) obj[gender];
                if ( (mudconf.sub_override & SUB_O) && !(mudstate.sub_overridestate & SUB_O) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_O");
                   if (!sub_ap)
		      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_O;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_O;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(tbuf, buff, &bufc);
		break;
	    case 'P':		/* Personal pronoun */
	    case 'p':
		tbuf = alloc_lbuf("exec.pronoun");
		if (gender < 0)
		    gender = get_gender(cause);
		if (!gender) {
                    sprintf(tbuf, "%.1000ss", Name(cause));
		} else {
                    sprintf(tbuf, "%.1000s", (char *) poss[gender]);
		}
                if ( (mudconf.sub_override & SUB_P) && !(mudstate.sub_overridestate & SUB_P) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_P");
                   if (!sub_ap)
                      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
                         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_P;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
                            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_P;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
                   safe_str(tbuf, buff, &bufc);
                free_lbuf(tbuf);
		break;
	    case 'S':		/* Subjective pronoun */
	    case 's':
		if (gender < 0)
		    gender = get_gender(cause);
		if (!gender)
		    tbuf = Name(cause);
		else
		    tbuf = (char *) subj[gender];
                if ( (mudconf.sub_override & SUB_S) && !(mudstate.sub_overridestate & SUB_S) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_S");
                   if (!sub_ap)
		      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_S;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_S;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(tbuf, buff, &bufc);
		break;
	    case 'A':		/* Absolute posessive */
	    case 'a':		/* idea from Empedocles */
		tbuf = alloc_lbuf("exec.absolutepossessive");
		if (gender < 0)
		    gender = get_gender(cause);
		if (!gender) {
                    sprintf(tbuf, "%.1000ss", Name(cause));
		} else {
                    sprintf(tbuf, "%.1000s", (char *) absp[gender]);
		}
                if ( (mudconf.sub_override & SUB_A) && !(mudstate.sub_overridestate & SUB_A) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_A");
                   if (!sub_ap)
                      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
                         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_A;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
                            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_A;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
                   safe_str(tbuf, buff, &bufc);
                free_lbuf(tbuf);
		break;
	    case '#':		/* Invoker DB number */
		tbuf = alloc_sbuf("exec.invoker");
		sprintf(tbuf, "#%d", cause);
                if ( (mudconf.sub_override & SUB_NUM) && !(mudstate.sub_overridestate & SUB_NUM) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_NUM");
                   if (!sub_ap)
		      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_NUM;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_NUM;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(tbuf, buff, &bufc);
		free_sbuf(tbuf);
		break;
	    case '!':		/* Executor DB number */
		tbuf = alloc_sbuf("exec.executor");
		sprintf(tbuf, "#%d", player);
                if ( (mudconf.sub_override & SUB_BANG) && !(mudstate.sub_overridestate & SUB_BANG) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_BANG");
                   if (!sub_ap)
		      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_BANG;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_BANG;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(tbuf, buff, &bufc);
		free_sbuf(tbuf);
		break;
            case '@':           /* Immediate Executor DB number */
                tbuf = alloc_sbuf("exec.executor");
                sprintf(tbuf, "#%d", caller);
                if ( (mudconf.sub_override & SUB_AT) && !(mudstate.sub_overridestate & SUB_AT) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_AT");
                   if (!sub_ap)
                      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
                         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_AT;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
                            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_AT;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
                   safe_str(tbuf, buff, &bufc);
                free_sbuf(tbuf);
                break;
             case '+':         /* Number of args passed */
                 tbuf = alloc_sbuf("exec.numargcalls");
                 sprintf(tbuf, "%d", ncargs);
                 safe_str(tbuf, buff, &bufc);
                 free_sbuf(tbuf);
                 break;
             case '?':         /* Function invocation and depth counts */
                 tbuf = alloc_sbuf("exec.functiondepths");
                 sprintf(tbuf, "%d %d", mudstate.func_invk_ctr, (mudstate.ufunc_nest_lev + mudstate.func_nest_lev));
                 safe_str(tbuf, buff, &bufc);
                 free_sbuf(tbuf);
                 break;
        case 'I':       /* itext */
        case 'i':
            dstr++;
            int inum_val;
            inum_val = atoi(dstr);
            if( inum_val < 0 || ( inum_val > mudstate.iter_inum ) )
            {   
                safe_str( "#-1 ARGUMENT OUT OF RANGE", buff, &bufc );
            }
            else
            {   
                safe_str( mudstate.iter_arr[mudstate.iter_inum - inum_val], buff, &bufc );
            }
            break;
	    case 'N':		/* Invoker name */
	    case 'n':
                if ( (mudconf.sub_override & SUB_N) && !(mudstate.sub_overridestate & SUB_N) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_N");
                   if (!sub_ap)
                      safe_str(Name(cause), buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
                         safe_str(Name(cause), buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_N;
		         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
                            safe_str(Name(cause), buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_N;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(Name(cause), buff, &bufc);
		break;
	    case 'L':		/* Invoker location db# */
	    case 'l':
		twhere = where_is(cause);
		if (Immortal(Owner(twhere)) && Dark(twhere) && Unfindable(twhere) && SCloak(twhere) && !Immortal(cause))
		  twhere = -1;
		else if (Wizard(Owner(twhere)) && Dark(twhere) && Unfindable(twhere) && !Wizard(cause))
		  twhere = -1;
                else if (mudconf.enforce_unfindable &&
                         ((Immortal(Owner(cause)) && Dark(cause) && Unfindable(cause) && SCloak(cause) && !Immortal(player)) ||
                          (Wizard(Owner(cause)) && Dark(cause) && Unfindable(cause) && !Wizard(player)) ||
                          ((Unfindable(twhere) || Unfindable(cause)) && !Admin(player))) )
		  twhere = -1;
		tbuf = alloc_sbuf("exec.exloc");
		sprintf(tbuf, "#%d", twhere);
                if ( (mudconf.sub_override & SUB_L) && !(mudstate.sub_overridestate & SUB_L) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_L");
                   if (!sub_ap)
		      safe_str(tbuf, buff, &bufc);
                   else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt )
		         safe_str(tbuf, buff, &bufc);
                      else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_L;
                         sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                         if ( !*sub_buf )
		            safe_str(tbuf, buff, &bufc);
                         else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_L;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else
		   safe_str(tbuf, buff, &bufc);
		free_sbuf(tbuf);
		break;
#ifndef NOEXTSUBS
#ifdef TINY_SUB
            case 'C':		/* Command substitution */
            case 'c':
#else
            case 'X':		/* Command substitution */
            case 'x':
#endif
#endif
                if ( (mudconf.sub_override & SUB_X) && 
                     !(mudstate.sub_overridestate & SUB_X) && 
                     Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_X");
                   if (sub_ap) {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( sub_txt ) {
                         if ( *sub_txt ) {
                            mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_X;
                            sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0);
                            mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_X;
                            safe_str(sub_buf, buff, &bufc);
                            free_lbuf(sub_txt);
                            free_lbuf(sub_buf);
                            break;
                         }
                         free_lbuf(sub_txt);
                      }
                   }
                } 
#ifdef TINY_SUB                                                            
                if ( mudstate.password_nochk == 0 ) {
                   t_bufa = replace_string("%C", "  ", mudstate.curr_cmd, 0);
                   t_bufb = replace_string("%c", "  ", t_bufa, 0);
                   safe_str(t_bufb, buff, &bufc);
                   free_lbuf(t_bufa);
                   free_lbuf(t_bufb);
                } else {
                   safe_str("XXX", buff, &bufc);
                }
#else                                                                      
                if ( mudstate.password_nochk == 0 ) {
                   t_bufa = replace_string("%X", "  ", mudstate.curr_cmd, 0);
                   t_bufb = replace_string("%x", "  ", t_bufa, 0);
                   safe_str(t_bufb, buff, &bufc);
                   free_lbuf(t_bufa);
                   free_lbuf(t_bufb);
                } else {
                   safe_str("XXX", buff, &bufc);
                }
#endif                                                                     
                break;
	    default:		/* Just copy */
                if ( !mudstate.sub_includestate && *mudconf.sub_include && 
                     Good_obj(mudconf.hook_obj) && (strchr(mudconf.sub_include, ToLower(*dstr)) != NULL) ) {
                   t_bufa = alloc_sbuf("sub_include");
                   sprintf(t_bufa, "SUB_%c", *dstr);
                   sub_ap = atr_str(t_bufa);
                   sub_delim = sub_value = 0;
                   if (sub_ap) {
                      mudstate.sub_includestate = 1;
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      sprintf(t_bufa, "CHR_%c", *dstr);
                      sub_ap = atr_str(t_bufa);
                      if ( sub_ap ) {
                         sub_txt2 = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                         if ( sub_txt2 && *sub_txt2) {
                            sub_buf2 = exec(mudconf.hook_obj, cause, caller, 
                                            EV_EVAL|EV_STRIP|EV_FCHECK, sub_txt2, (char **)NULL, 0);
                            sub_delim = 1;
                            if ( *sub_buf2 ) {
                               if ( is_integer(sub_buf2) ) {
                                  sub_value = atoi(sub_buf2);
                                  sub_char = ' ';
                               } else {
                                  sub_char = *sub_buf2;
                               }
                            } else
                               sub_char = ' ';
                            free_lbuf(sub_buf2);
                            memset(t_bufa, '\0', SBUF_SIZE);
                            t_bufb = t_bufa;
                            orig_dstr = dstr-1;
                            sub_cntr = 1;
                            dstr++;
                            sub_valuecnt = 0;
                            if ( sub_value > 0 ) {
                               while ( *dstr && (sub_valuecnt < sub_value) && (sub_cntr < (SBUF_SIZE - 1)) ) {
                                  *t_bufb = *dstr;
                                  dstr++;
                                  t_bufb++;
                                  sub_cntr++;
                                  sub_valuecnt++;
                               }
                               if ( sub_valuecnt == sub_value )
                                  dstr--;
                               if ( sub_valuecnt != sub_value ) {
                                  sub_delim = 0;
                                  dstr = orig_dstr;
                               }
                            } else {
                               while ( *dstr && (*dstr != sub_char) && (sub_cntr < (SBUF_SIZE - 1)) ) {
                                  *t_bufb = *dstr;
                                  dstr++;
                                  t_bufb++;
                                  sub_cntr++;
                               }
                               if ( *dstr != sub_char ) {
                                  sub_delim = 0;
                                  dstr = orig_dstr;
                               }
                            }
                         }
                         free_lbuf(sub_txt2);
                      }
                      if ( sub_txt  ) {
                         if ( *sub_txt ) {
                            if ( sub_delim )
                               sub_buf = exec(mudconf.hook_obj, player, caller, EV_EVAL|EV_STRIP|EV_FCHECK, 
                                         sub_txt, (char **)&t_bufa, 1);
                            else
                               sub_buf = exec(mudconf.hook_obj, player, caller, EV_EVAL|EV_STRIP|EV_FCHECK, 
                                         sub_txt, (char **)NULL, 0);
                            safe_str(sub_buf, buff, &bufc);
                            free_lbuf(sub_buf);
                         }
                         free_lbuf(sub_txt);
                      } else {
		         safe_chr(*dstr, buff, &bufc);
                      }
                      mudstate.sub_includestate = 0;
                   } else {
		      safe_chr(*dstr, buff, &bufc);
                   }
                   free_sbuf(t_bufa);
                } else {
		   safe_chr(*dstr, buff, &bufc);
                }
	    }
	    if (isupper((int)savec))
		*savepos = ToUpper((int)*savepos);
	    break;
	case '(':
	    /* Arglist start.  See if what precedes is a function.
	     * If so, execute it if we should. */

	    at_space = 0;
	    if (!(eval & EV_FCHECK)) {
		safe_chr('(', buff, &bufc);
		break;
	    }
	    /* Load an sbuf with an uppercase version of the func
	     * name, and see if the func exists.  Trim trailing
	     * spaces from the name if configured. */

	    *bufc = '\0';
	    tbufc = tbuf = alloc_sbuf("exec.tbuf");
	    safe_sb_str(buff, tbuf, &tbufc);
	    *tbufc = '\0';
	    if (mudconf.space_compress) {
		while ((--tbufc >= tbuf) && isspace((int)*tbufc));
		tbufc++;
	    }
	    for (tbufc = tbuf; *tbufc; tbufc++)
		*tbufc = ToLower((int)*tbufc);
#ifdef BANGS
	    /* C++ Inspired Bangs
	     * 
	     * Possible combinations are: ! !! !$ !!$
	     * Stepping past the possible combinations at the beginning of
	     * the function's name, pushing a pointer up as we go. The
	     * modified pointer will be used to search the hash table
	     * if a bang condition is found.
	     */
	    tbangc = tbuf;
	    bang_not = 0;
	    bang_yes = 0;
	    bang_string = 0;
	    if (*tbangc == '!') {
		bang_not = 1;
		tbangc++;
		}
	    if (*tbangc == '!') {
		bang_not = 0;
		bang_yes = 1;
		tbangc++;
		}
	    if ((bang_not || bang_yes) && *tbangc == '$') {
		bang_string = 1;
		tbangc++;
		}
	    if (bang_not || bang_yes) {
		fp = (FUN *) hashfind(tbangc, &mudstate.func_htab);
		ufp = NULL;
                if ( fp == NULL ) {
		   ufp = (UFUN *) hashfind(tbangc, &mudstate.ufunc_htab);
		}
            } else {
		fp = (FUN *) hashfind(tbuf, &mudstate.func_htab);
    		ufp = NULL;
                if ( fp == NULL ) {
		   ufp = (UFUN *) hashfind(tbuf, &mudstate.ufunc_htab);
		}
            }
#else
	    fp = (FUN *) hashfind(tbuf, &mudstate.func_htab);
	    /* If not a builtin func, check for global func */

	    ufp = NULL;
	    if (fp == NULL) {
		ufp = (UFUN *) hashfind(tbuf, &mudstate.ufunc_htab);
	    }
#endif
            /* Compare to see if it has an IGNORE mask */
            if ( fp && (fp->perms & 0x00007F00) ) {
	       check_access(player, fp->perms, fp->perms2, 0);
               if ( mudstate.func_ignore && !mudstate.func_bypass) {
                  memset(tfunbuff, 0, sizeof(tfunbuff));
                  if ( bang_not || bang_yes ) {
                     sprintf(tfunbuff, "_%.31s", tbangc);
                  } else {
                     sprintf(tfunbuff, "_%.31s", tbuf);
                  }
	          ufp = (UFUN *) hashfind((char *)tfunbuff,
					   &mudstate.ufunc_htab);
               }
            }
	    /* Do the right thing if it doesn't exist */

	    if (!fp && !ufp) {
		if (eval & EV_FMAND) {
		    bufc = buff;
		    safe_str((char *) "#-1 FUNCTION (",
			     buff, &bufc);
		    safe_str(tbuf, buff, &bufc);
		    safe_str((char *) ") NOT FOUND",
			     buff, &bufc);
		    alldone = 1;
		} else {
		    safe_chr('(', buff, &bufc);
		}
		free_sbuf(tbuf);
		eval &= ~EV_FCHECK;
		break;
	    }
	    free_sbuf(tbuf);

	    /* Get the arglist and count the number of args
	     * Neg # of args means catenate subsequent args
	     */

	    if (ufp)
		nfargs = NFARGS;
	    else if (fp->nargs < 0)
		nfargs = -fp->nargs;
	    else
		nfargs = NFARGS;
	    tstr = dstr;
            if ( ((fp && ((fp->flags & FN_NO_EVAL) || (fp->perms2 & CA_NO_EVAL)) && !(fp->perms2 & CA_EVAL)) ||
                  (ufp && (ufp->perms2 & CA_NO_EVAL))) &&
                 !(ufp && (ufp->perms2 & CA_EVAL)) ) {
                if ( mudconf.brace_compatibility )
                   feval = (eval & ~EV_EVAL & ~EV_STRIP) | EV_STRIP_ESC;
                else
                   feval = (eval & ~EV_EVAL) | EV_STRIP_ESC;
            } else
                feval = eval;
            if ( ufp && (ufp->perms & CA_EVAL) ) {
                feval = (feval | EV_EVAL | EV_STRIP);
            }
	    dstr = parse_arglist(player, cause, caller, dstr + 1,
				 ')', feval, fargs, nfargs,
				 cargs, ncargs);

	    /* If no closing delim, just insert the '(' and
	     * continue normally */

	    if (!dstr) {
		dstr = tstr;
		safe_chr(*dstr, buff, &bufc);
		for (i = 0; i < nfargs; i++)
		    if (fargs[i] != NULL)
			free_lbuf(fargs[i]);
		eval &= ~EV_FCHECK;
		break;
	    }
            /*  no stopping us now, we're either going to issue an
                error message or execute a function, so toast the function
                name in the buffer */

            bufc = buff;

	    /* Count number of args returned */

	    dstr--;
	    j = 0;
	    for (i = 0; i < nfargs; i++)
		if (fargs[i] != NULL) {
		    j = i + 1;
		    if (index(fargs[i], ESC_CHAR)) {
			strcpy(fargs[i], strip_ansi(fargs[i]));
		    }
		}
	    nfargs = j;

	    /* If it's a user-defined function, perform it now. */

	    if (ufp) {
		mudstate.func_nest_lev++;
                if ( ((ufp->minargs != -1) && (nfargs < ufp->minargs)) ||
                     ((ufp->maxargs != -1) && (nfargs > ufp->maxargs)) ) {
                 bufc = buff;
                 tstr = alloc_sbuf("exec.funcargs");
                 safe_str((char *) "#-1 FUNCTION (",
                          buff, &bufc);
                 safe_str((char *) ufp->name, buff, &bufc);
                   if ( abs(ufp->minargs) != ufp->maxargs ) {
                      if ( ufp->maxargs == -1 ) {
                       safe_str((char *) ") EXPECTS ",
                                buff, &bufc);
                      } else {
                       safe_str((char *) ") EXPECTS BETWEEN ",
                                buff, &bufc);
                      }
                      if ( ufp->minargs == -1 ) {
                         sprintf(tstr, "%d", (int) 1);
                      } else {
                         sprintf(tstr, "%d", ufp->minargs);
                      }
                    safe_str(tstr, buff, &bufc);
                      if ( ufp->maxargs == -1 ) {
                         safe_str((char *) " OR MORE",
                                buff, &bufc);
                      } else {
                       safe_str((char *) " AND ",
                                buff, &bufc);
                         sprintf(tstr, "%d", ufp->maxargs);
                       safe_str(tstr, buff, &bufc);
                      }
                   } else {
                    safe_str((char *) ") EXPECTS ",
                             buff, &bufc);
                      sprintf(tstr, "%d", ufp->maxargs);
                    safe_str(tstr, buff, &bufc);
                   }
                   if ( ufp->maxargs == 1 ) {
                    safe_str((char *) " ARGUMENT [RECEIVED ",
                             buff, &bufc);
                   } else {
                    safe_str((char *) " ARGUMENTS [RECEIVED ",
                             buff, &bufc);
                   }
                   sprintf(tstr, "%d]", nfargs);
                   safe_str(tstr, buff, &bufc);
                 free_sbuf(tstr);
                } else if ( mudstate.ufunc_nest_lev >= mudconf.func_nest_lim ) {
		    safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED",
                             buff, &bufc);
                } else if ( Fubar(player) ) {
                    safe_str("#-1 PERMISSION DENIED", buff, &bufc);
		} else if (!check_access(player, ufp->perms, ufp->perms2, 0)) {
		    safe_str("#-1 PERMISSION DENIED", buff, &bufc);
		} else {
		    tstr = atr_get(ufp->obj, ufp->atr,
				   &aowner, &aflags);
		    if (ufp->flags & FN_PRIV)
			i = ufp->obj;
		    else
			i = player;
		    mudstate.ufunc_nest_lev++;
		    mudstate.func_invk_ctr++;
                    if ( ufp->flags & FN_PRES ) {
                       for (z = 0; z < MAX_GLOBAL_REGS; z++) {
                          savereg[z] = alloc_lbuf("ulocal_reg");
                          ptsavereg = savereg[z];
                          safe_str(mudstate.global_regs[z],savereg[z],&ptsavereg);
                          if ( ufp->flags & FN_PROTECT ) {
                             *mudstate.global_regs[z] = '\0';
                          }
                       }
                    }
		    mudstate.allowbypass = 1;
		    tbuf = exec(i, cause, player, feval,
				tstr, fargs, nfargs);
		    mudstate.allowbypass = 0;
                    if ( ufp->flags & FN_PRES ) {
                       for (z = 0; z < MAX_GLOBAL_REGS; z++) {
                          ptsavereg = mudstate.global_regs[z];
                          safe_str(savereg[z],mudstate.global_regs[z],&ptsavereg);
                          free_lbuf(savereg[z]);
                       }
                    }
		    mudstate.ufunc_nest_lev--;
#ifdef BANGS
		    /* Ufun handling of bangs
		     * 
		     * Real straight-forward. Handle the bangs inside-out,
		     * strings first, then negate or yes-gate it.
		     */
		    if (bang_string && *tbuf) {
			tbuf[0] = '1';
			tbuf[1] = '\0';
			} else if (bang_string) {
			tbuf[0] = '0';
			tbuf[1] = '\0';
			}
		    if (bang_not && atoi(tbuf)) {
			tbuf[0] = '0';
			} else if (bang_not && !atoi(tbuf)) {
			tbuf[0] = '1';
			} else if (bang_yes && atoi(tbuf)) {
			tbuf[0] = '1';
			} else if (bang_yes && !atoi(tbuf)) {
			tbuf[0] = '0';
			}
		    if (bang_not || bang_yes) {
			tbuf[1] = '\0';
			}
#endif
		    safe_str(tbuf, buff, &bufc);
		    free_lbuf(tstr);
		    free_lbuf(tbuf);
		}

		/* Return the space allocated for the args */

		mudstate.func_nest_lev--;
		for (i = 0; i < nfargs; i++)
		    if (fargs[i] != NULL)
			free_lbuf(fargs[i]);
		eval &= ~EV_FCHECK;
                mudstate.funccount++;
		break;
	    }
	    /* If the number of args is right, perform the func.
	     * Otherwise return an error message.  Note that
	     * parse_arglist returns zero args as one null arg,
	     * so we have to handle that case specially.
	     */

	    if ((fp->nargs == 0) && (nfargs == 1)) {
		if (!*fargs[0]) {
		    free_lbuf(fargs[0]);
		    fargs[0] = NULL;
		    nfargs = 0;
		}
	    }
	    if ((nfargs == fp->nargs) ||
		(nfargs == -fp->nargs) ||
		(fp->flags & FN_VARARGS)) {

		/* Check recursion limit */

		mudstate.func_nest_lev++;
		mudstate.func_invk_ctr++;
		if (mudstate.func_nest_lev >=
		    mudconf.func_nest_lim) {
		    safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED",
                             buff, &bufc);
                } else if ( Fubar(player) ) {
		    safe_str("#-1 PERMISSION DENIED", buff, &bufc);
		} else if (mudstate.func_invk_ctr >=
			   mudconf.func_invk_lim) {
		    safe_str("#-1 FUNCTION INVOCATION LIMIT EXCEEDED",
                             buff, &bufc);
		} else if ( !check_access(player, fp->perms, fp->perms2, 0) &&
                            !((fp->perms & 0x00007F00) && (mudstate.func_ignore && mudstate.func_bypass)) ) {
		  if ( mudstate.func_ignore) {
		    safe_str((char *) "#-1 FUNCTION (",
			     buff, &bufc);
		    y = strlen(fp->name);
		    for (x = 0; x < y; x++)
		      safe_chr(tolower(*(fp->name + x)), buff, &bufc);
		    safe_str((char *) ") NOT FOUND",
			     buff, &bufc);
		  }
		  else
		    safe_str("#-1 PERMISSION DENIED", buff, &bufc);
		} else if (mudstate.func_invk_ctr <
			   mudconf.func_invk_lim) {
		    fp->fun(buff, &bufc, player, cause, caller,
			    fargs, nfargs, cargs, ncargs);
                    mudstate.funccount++;
#ifdef BANGS
		    /* Standard function handling
		     * This is sideways. A null result from a builtin
		     * function doesn't return a null string. Buff will
		     * contain the name of the function, from before 
		     * passing it to the hash table. BUT ... bufc will
		     * point to the beginning of the array.
		     *
		     * safe_str is used to load the string with a "0"
		     * if the result was null. I don't want to consider
		     * what would happen otherwise.
		     */
		    if (bang_string && (buff != bufc)) {
			buff[0] = '1';
			buff[1] = '\0';
			} else if (bang_string) {
			safe_str("0", buff, &bufc);
			}
		    if (bang_not && atoi(buff)) {
			buff[0] = '0';
			} else if (bang_not && !atoi(buff)) {
			buff[0] = '1';
			} else if (bang_yes && atoi(buff)) {
			buff[0] = '1';
			} else if (bang_yes && !atoi(buff)) {
			buff[0] = '0';
			}
		    if (bang_not || bang_yes) {
			buff[1] = '\0';
			}
#endif

		}
		mudstate.func_nest_lev--;
	    } else {
		bufc = buff;
		tstr = alloc_sbuf("exec.funcargs");
		sprintf(tstr, "%d", fp->nargs);
		safe_str((char *) "#-1 FUNCTION (",
			 buff, &bufc);
		safe_str((char *) fp->name, buff, &bufc);
		safe_str((char *) ") EXPECTS ",
			 buff, &bufc);
		safe_str(tstr, buff, &bufc);
		safe_str((char *) " ARGUMENTS [RECEIVED ",
			 buff, &bufc);
                sprintf(tstr, "%d]", nfargs);
                safe_str(tstr, buff, &bufc);
		free_sbuf(tstr);
	    }

	    /* Return the space allocated for the arguments */

	    for (i = 0; i < nfargs; i++)
		if (fargs[i] != NULL)
		    free_lbuf(fargs[i]);
	    eval &= ~EV_FCHECK;
	    break;
	default:
	    /* A mundane character.  Just copy it */

	    at_space = 0;
	    safe_chr(*dstr, buff, &bufc);
	}
	dstr++;
    }

    /* If we're eating spaces, and the last thing was a space,
     * eat it up. Complicated by the fact that at_space is
     * initially true. So check to see if we actually put something
     * in the buffer, too.
     */

    if (mudconf.space_compress && at_space && (bufc != buff))
	bufc--;

    *bufc = '\0';

    /* Report trace information */

    if (is_trace) {
	tcache_add(player, savestr, buff);
	save_count = tcache_count - mudconf.trace_limit;;
	if (is_top || !mudconf.trace_topdown)
	    tcache_finish();
	if (is_top && (save_count > 0)) {
	    tbuf = alloc_mbuf("exec.trace_diag");
	    sprintf(tbuf,
		    "%d lines of trace output discarded.",
		    save_count);
	    notify(player, tbuf);
	    free_mbuf(tbuf);
	}
    }
    mudstate.eval_rec--;

    RETURN(buff); /* #67 */
}
