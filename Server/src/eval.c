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
extern char * parse_ansi_name(dbref, char *);
extern void fun_ansi(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern void fun_objid(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern void do_regedit(char *, char **, dbref, dbref, dbref, char **, int, char **, int, int);

/* ---------------------------------------------------------------------------
 * parse_to: Split a line at a character, obeying nesting.  The line is
 * destructively modified (a null is inserted where the delimiter was found)
 * dstr is modified to point to the char after the delimiter, and the function
 * return value points to the found string (space compressed if specified).
 * If we ran off the end of the string without finding the delimiter, dstr is
 * returned as NULL.
 */

int
sub_override_process(int i_include, char *s_include, char *s_chr, char *buff, char **bufc, dbref cause, dbref caller, int feval) {
   ATTR *sub_ap;
   int sub_aflags;
   dbref sub_aowner;
   char *s_buf, *sub_txt, *sub_buf;

   if ( Good_obj(mudconf.hook_obj) && (mudconf.sub_override & i_include) && !(mudstate.sub_overridestate & i_include) ) {
      s_buf = alloc_sbuf("sub_override_process");
      sprintf(s_buf, "SUB_%s", s_chr);
      sub_ap = atr_str(s_buf);
      if ( !sub_ap ) {
         safe_str(s_include, buff, bufc);
      } else {
         sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
         if ( !sub_txt )
            safe_str(s_include, buff, bufc);
         else {
            mudstate.sub_overridestate = mudstate.sub_overridestate | i_include;
            sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0, (char **)NULL, 0);
            if ( !*sub_buf )
               safe_str(s_include, buff, bufc);
            else
               safe_str(sub_buf, buff, bufc);
            mudstate.sub_overridestate = mudstate.sub_overridestate & ~i_include;
            free_lbuf(sub_txt);
            free_lbuf(sub_buf);
         }
      }
      free_sbuf(s_buf);
      return 1;
   } else {
      return 0;
   }
}

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
    case '\\':      /* general escape */
    case '%':       /* also escapes chars */
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
        case ' ':       /* space */
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
          dbref ncargs, int i_type, char *regargs[], int nregargs)
{
    char *rstr, *tstr, *mychar, *mycharptr, *s;
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

    if ( i_type ) {
       peval = peval | EV_EVAL | ~EV_STRIP_ESC;
    }
    while ((arg < nfargs) && rstr) {
    if (arg < (nfargs - 1))
        tstr = parse_to(&rstr, ',', peval);
    else
        tstr = parse_to(&rstr, '\0', peval);
    if (eval & EV_EVAL) {
        fargs[arg] = exec(player, cause, caller, eval | EV_FCHECK, tstr,
                  cargs, ncargs, regargs, nregargs);
    } else {
            if (  i_type  ) {
               mychar = mycharptr = alloc_lbuf("no_eval_parse_arglist");
               s = tstr;
               while (*s) {
                  switch (*s) {
                     case '%':
                     case '\\':
                     case '[':
                     case ']':
                     case '{':
                     case '}':
                     case '(':   /* Added 7/00 Ash */
                     case ')':   /* Added 7/00 Ash */
                     case ',':   /* Added 7/00 Ash */
                         if ( (*s != '%') || ((*s == '%') && !isdigit(*(s+1))) )
                            safe_chr('\\', mychar, &mycharptr);
                     default:
                         safe_chr(*s, mychar, &mycharptr);
                  }
                  s++;
               }
           fargs[arg] = exec(player, cause, caller, eval | EV_FCHECK | EV_EVAL | ~EV_STRIP_ESC, mychar,
                     cargs, ncargs, regargs, nregargs);
               free_lbuf(mychar);
            } else {
           fargs[arg] = alloc_lbuf("parse_arglist");
           strcpy(fargs[arg], tstr);
            }
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
    char *label;
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
tcache_add(dbref player, char *orig, char *result, char *s_label)
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
            xp->label = s_label;
        xp->next = tcache_head;
        tcache_head = xp;
    } else {
        free_lbuf(orig);
            free_sbuf(s_label);
    }
    } else {
    free_lbuf(orig);
        free_sbuf(s_label);
    }
    DPOP; /* #65 */
}

static void 
tcache_finish(void)
{
    TCENT *xp;
    char *tpr_buff = NULL, *tprp_buff = NULL, *s_aptext = NULL, *s_aptextptr = NULL, *s_strtokr = NULL, *tbuff = NULL, 
         *tstr, *tstr2, *s_grep;
    int i_apflags, i_targetlist;
#ifdef PCRE_EXEC
    char *trace_buffptr, *trace_array[4], *trace_tmp;
    int i_trace;
#endif
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
        ap_log = atr_str("TRACE_GREP");
        if ( ap_log ) {
           s_grep = atr_get(xp->player, ap_log->number, &i_apowner, &i_apflags);
           if ( s_grep && *s_grep ) {
#ifdef PCRE_EXEC
              if ( (i_apflags & AF_REGEXP) || (ap_log->flags & AF_REGEXP) ) {
                 trace_buffptr = tstr2 = alloc_lbuf("grep_regexp");
                 trace_tmp = alloc_lbuf("grep_regexp_tmp");
#ifdef ZENTY_ANSI
                 sprintf(trace_tmp, "%s$0%s", SAFE_ANSI_RED, SAFE_ANSI_NORMAL);
#else
                 sprintf(trace_tmp, "%s$0%s", ANSI_RED, ANSI_NORMAL);
#endif
                 trace_array[0] = xp->orig;
                 trace_array[1] = s_grep;
                 trace_array[2] = trace_tmp;
                 trace_array[3] = NULL;
                 i_trace = mudstate.notrace;
                 mudstate.notrace = 1;
                 do_regedit(tstr2, &trace_buffptr, xp->player, xp->player, xp->player, trace_array, 3, (char **)NULL, 0, 8);
                 mudstate.notrace = i_trace;
                 free_lbuf(trace_tmp);
              } else {
                 edit_string(xp->orig, &tstr, &tstr2, s_grep, s_grep, 0, 0, 2, 1);
                 free_lbuf(tstr);
              }
#else
              edit_string(xp->orig, &tstr, &tstr2, s_grep, s_grep, 0, 0, 2, 1);
              free_lbuf(tstr);
#endif
           } else {
              tstr2 = alloc_lbuf("fun_with_grep");
              strcpy(tstr2, xp->orig);
           }
           free_lbuf(s_grep);
        } else {
           tstr2 = alloc_lbuf("fun_with_grep");
           strcpy(tstr2, xp->orig);
        }

        if ( *(xp->label) ) {
       notify(Owner(xp->player),
              safe_tprintf(tpr_buff, &tprp_buff, "%s(#%d) [%s%s%s]} '%s' -> '%s'", Name(xp->player), xp->player,
                  ANSI_HILITE, xp->label, ANSI_NORMAL, tstr2, xp->result));
        } else {
       notify(Owner(xp->player),
              safe_tprintf(tpr_buff, &tprp_buff, "%s(#%d)} '%s' -> '%s'", Name(xp->player), xp->player,
                  tstr2, xp->result));
        }
        free_lbuf(tstr2);

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
        free_sbuf(xp->label);
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
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F     */

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
        
/* Accent:      `'^~:o,u"B|-&Ee                       */
static const unsigned char AccentCombo2[256] =
{
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    */

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
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    */

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

/******************************************************
 * This handles accents as well!
 * buff/bufptr is the ansi
 * buff2/buf2ptr is the accents + ansi
 * Change %c/%x/%m substitutions into real ansi now.
 ******************************************************/
void parse_ansi(char *string, char *buff, char **bufptr, char *buff2, char **buf2ptr, char *buff_utf, char **bufuptr)
{
    char *bufc, *bufc2, *bufc_utf, s_twochar[3], s_final[80], s_intbuf[4];
    char s_utfbuf[3], s_ucpbuf[7], *ptr, tmpbuf[10], *tmpptr = NULL, *tmp;
    unsigned char ch1, ch2, ch;
    int i_tohex, accent_toggle, i_extendcnt, i_extendnum, i_utfnum, i_utfcnt;


/* Debugging only
    fprintf(stderr, "Value: %s\n", string);
 */
    memset(s_twochar, '\0', sizeof(s_twochar));
    memset(s_final, '\0', sizeof(s_final));
    memset(s_utfbuf, '\0', sizeof(s_utfbuf));
    memset(s_ucpbuf, '\0', sizeof(s_ucpbuf));
    memset(tmpbuf, '\0', sizeof(tmpbuf));
    bufc = *bufptr;
    bufc2 = *buf2ptr;
    bufc_utf = *bufuptr;
    accent_toggle = 0;
    i_extendcnt = 0;
    i_utfnum = 0;
    i_utfcnt = 0;
    s_intbuf[3] = '\0';
    ch = ch1 = ch2 = '\0';
    while(*string && ((bufc - buff) < (LBUF_SIZE-24))) {
        if(*string == '\\') {
            string++;
            safe_chr('\\', buff, &bufc);
            safe_chr('\\', buff2, &bufc2);
            safe_chr('\\', buff_utf, &bufc_utf);
            safe_chr(*string, buff, &bufc);
            safe_chr(*string, buff2, &bufc2);
            safe_chr(*string, buff_utf, &bufc_utf);
        } else if(*string == '%') {
            string++;
            if(*string == '\\') {
                safe_chr('%', buff, &bufc);
                safe_chr('%', buff2, &bufc2);
                safe_chr('%', buff_utf, &bufc_utf);
                safe_chr(*string, buff, &bufc);
                safe_chr(*string, buff2, &bufc2);
                safe_chr(*string, buff_utf, &bufc_utf);
            } else if ((*string == '%') && ((*(string+1) == SAFE_CHR )
#ifdef SAFE_CHR2
                                        || (*(string+1) == SAFE_CHR2 )
#endif
#ifdef SAFE_CHR3
                                        || (*(string+1) == SAFE_CHR3 )
#endif
)) {
                if(*(string+1) == SAFE_CHR) {
                  safe_str((char*)SAFE_CHRST, buff, &bufc);
                  safe_str((char*)SAFE_CHRST, buff2, &bufc2);
                  safe_str((char*)SAFE_CHRST, buff_utf, &bufc_utf);
                }
#ifdef SAFE_CHR2
                else if(*(string+1) == SAFE_CHR2) {
                  safe_str((char*)SAFE_CHRST2, buff, &bufc);
                  safe_str((char*)SAFE_CHRST2, buff2, &bufc2);
                  safe_str((char*)SAFE_CHRST, buff_utf, &bufc_utf);
                }
#endif
#ifdef SAFE_CHR3
                else if(*(string+1) == SAFE_CHR3) {
                  safe_str((char*)SAFE_CHRST3, buff, &bufc);
                  safe_str((char*)SAFE_CHRST3, buff2, &bufc2);
                  safe_str((char*)SAFE_CHRST, buff_utf, &bufc_utf);
                }
#endif
                else {
                  safe_str((char*)SAFE_CHRST, buff, &bufc);
                  safe_str((char*)SAFE_CHRST, buff2, &bufc2);
                  safe_str((char*)SAFE_CHRST, buff_utf, &bufc_utf);
                }
                string++;
            } else if ((*string == '%') && (*(string+1) == 'f')) {
                safe_str("%f", buff, &bufc);
                safe_str("%f", buff2, &bufc2);
                safe_str("%f", buff_utf, &bufc_utf);
                string++;
            } else if ( (*string == '%') && (*(string+1) == '<') ) { 
                safe_str("%<", buff, &bufc);
                safe_str("%<", buff2, &bufc2);
                string++;
            } else if ( ((*string != SAFE_CHR)
#ifdef SAFE_CHR2
                           && (*string != SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                           && (*string != SAFE_CHR3)
#endif
                           ) && (*string != 'f') && (*string != '<') ) {
                safe_chr('%', buff, &bufc);
                safe_chr(*string, buff, &bufc);
                safe_chr('%', buff2, &bufc2);
                safe_chr(*string, buff2, &bufc2);
                safe_chr('%', buff_utf, &bufc_utf);
                safe_chr(*string, buff_utf, &bufc_utf);
/*          } else if ( (*string == '%') && (*(string+1) == '<') ) { 
                string+=2; */
            } else if ( (*string == '<') ) {
                string++;
                if ( (*string == 'u') && 
                     (((strlen(string)) > 5 && (*(string+5) == '>')) 
                       || ((strlen(string) > 6) && (*(string+6) == '>')) 
                       || ((strlen(string) > 7) && (*(string+7) == '>')))) {
                    string++;
                    i_utfcnt = 0;
                    while ( *string ) {
                        if (i_utfcnt >= 6 || *string == '>') {
                            break;
                        }
                        
                        s_ucpbuf[i_utfcnt] = *string;
                        i_utfcnt++;
                        string++;
                    }
                    
                    i_utfnum = atoi(s_ucpbuf);
                    
                   
                    //if (i_utfnum > 31) {
                        tmpptr = ucptoutf8(s_ucpbuf);

                        i_utfcnt = 0;
                        tmp = tmpptr;
                        while (*tmp) {
                            s_utfbuf[i_utfcnt % 2] = *tmp;                      
                            if (i_utfcnt % 2) {
                                i_utfnum = strtol(s_utfbuf, &ptr, 16);
                                safe_chr((char)i_utfnum, buff_utf, &bufc_utf);
                            }
                            
                            i_utfcnt++;
                            tmp++;
                        }
                        safe_chr(' ', buff2, &bufc2);
                        safe_chr(' ', buff, &bufc);
                    //}
                } else if ( isdigit(*(string)) && isdigit(*(string+1)) && isdigit(*(string+2)) && (*(string+3) == '>') ) {
                   s_intbuf[0] = *(string);
                   s_intbuf[1] = *(string+1);
                   s_intbuf[2] = *(string+2);
                   i_extendnum = atoi(s_intbuf);
                   if ( (i_extendnum >= 32) && (i_extendnum <= 126) ) {
                      safe_chr((char) i_extendnum, buff_utf, &bufc_utf);
                      safe_chr((char) i_extendnum, buff2, &bufc2);
                      safe_chr((char) i_extendnum, buff, &bufc);
                   } else if ( (i_extendnum >= 160) && 
                           ((!mudconf.accent_extend && (i_extendnum <= 250)) || (mudconf.accent_extend && (i_extendnum <=255))) ) {
                      if ( i_extendnum == 255 ) {
                         safe_chr((char) i_extendnum, buff2, &bufc2);
                         safe_chr((char) i_extendnum, buff_utf, &bufc_utf);
                      }
                      safe_chr((char) i_extendnum, buff_utf, &bufc_utf);
                      safe_chr((char) i_extendnum, buff2, &bufc2);
                      safe_chr(' ', buff, &bufc);                         
                   } else {
                      switch(i_extendnum) {
                         case 251:
                         case 252: safe_chr('u', buff2, &bufc2);
                                   safe_chr('u', buff_utf, &bufc_utf);
                                   break;
                         case 253:
                         case 255: safe_chr('y', buff2, &bufc2);
                                   safe_chr('y', buff_utf, &bufc_utf);                       
                                   break;
                         case 254: safe_chr('p', buff2, &bufc2);
                                   safe_chr('p', buff_utf, &bufc_utf);                       
                                   break;
                      }
                   }
                   i_extendcnt+=3;
                   string+=3;
                } else {
                   safe_chr(*string, buff, &bufc);
                   safe_chr(*string, buff2, &bufc2);
                   safe_chr(*string, buff_utf, &bufc_utf);
                }
            } else if ( (*string == 'f') ) {
                ch = (unsigned char)*(++string);
                if ( ch == 'n' ) {
                   accent_toggle = 0;
                } else {
                   ch2 = AccentCombo2[(int)ch];
                   if ( ch2 > 0 ) {
                      accent_toggle = 1;
                   } else {
                      if ( !isprint(*string) ) {
                         safe_chr(*string, buff, &bufc);
                         safe_chr(*string, buff2, &bufc2);
                         safe_chr(*string, buff_utf, &bufc_utf);
                      }
                   }
                }
            } else {
                switch (*++string) {
                case '\0':
                    safe_chr(*string, buff, &bufc);
                    safe_chr(*string, buff2, &bufc2);
                    safe_chr(*string, buff_utf, &bufc_utf);
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
                             safe_str(s_final, buff2, &bufc2);
                             safe_str(s_final, buff_utf, &bufc_utf);
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
                             safe_str(s_final, buff2, &bufc2);
                             safe_str(s_final, buff_utf, &bufc_utf);
                             sprintf(s_final, "%dm", i_tohex);
                          }
                          break;
                       default:
                          if(*(string-1) == SAFE_CHR) {
                            safe_str((char *)SAFE_CHRST, buff, &bufc);
                            safe_chr(*string, buff, &bufc);
                            safe_str((char *)SAFE_CHRST, buff2, &bufc2);
                            safe_chr(*string, buff2, &bufc2);
                            safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                            safe_chr(*string, buff_utf, &bufc_utf);                         
                          }
#ifdef SAFE_CHR2
                          else if(*(string-1) == SAFE_CHR2) {
                            safe_str((char *)SAFE_CHRST2, buff, &bufc);
                            safe_chr(*string, buff, &bufc);
                            safe_str((char *)SAFE_CHRST2, buff2, &bufc2);
                            safe_chr(*string, buff2, &bufc2);
                            safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                            safe_chr(*string, buff_utf, &bufc_utf);                         
                          }
#endif
#ifdef SAFE_CHR3
                          else if(*(string-1) == SAFE_CHR3) {
                            safe_str((char *)SAFE_CHRST3, buff, &bufc);
                            safe_chr(*string, buff, &bufc);
                            safe_str((char *)SAFE_CHRST3, buff2, &bufc2);
                            safe_chr(*string, buff2, &bufc2);
                            safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                            safe_chr(*string, buff_utf, &bufc_utf);                         
                          }
#endif
                          else {
                            safe_str((char *)SAFE_CHRST, buff, &bufc);
                            safe_chr(*string, buff, &bufc);
                            safe_str((char *)SAFE_CHRST, buff2, &bufc2);
                            safe_chr(*string, buff2, &bufc2);
                            safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                            safe_chr(*string, buff_utf, &bufc_utf);                         
                          }
                          break;
                    }  
                    break;
                case 'n':
                    safe_str((char *) ANSI_NORMAL, buff, &bufc);
                    safe_str((char *) ANSI_NORMAL, buff2, &bufc2);
                    safe_str((char *) ANSI_NORMAL, buff_utf, &bufc_utf);
                    break;
                case 'f':
                    if ( mudconf.global_ansimask & MASK_BLINK ) {
                        safe_str((char *) ANSI_BLINK, buff, &bufc);
                        safe_str((char *) ANSI_BLINK, buff2, &bufc2);
                        safe_str((char *) ANSI_BLINK, buff_utf, &bufc_utf);
                    }
                    break;
                case 'u':
                    if ( mudconf.global_ansimask & MASK_UNDERSCORE ) {
                        safe_str((char *) ANSI_UNDERSCORE, buff, &bufc);
                        safe_str((char *) ANSI_UNDERSCORE, buff2, &bufc2);
                        safe_str((char *) ANSI_UNDERSCORE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'i':
                    if ( mudconf.global_ansimask & MASK_INVERSE ) {
                        safe_str((char *) ANSI_INVERSE, buff, &bufc);
                        safe_str((char *) ANSI_INVERSE, buff2, &bufc2);
                        safe_str((char *) ANSI_INVERSE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'h':
                    if ( mudconf.global_ansimask & MASK_HILITE ) {
                        safe_str((char *) ANSI_HILITE, buff, &bufc);
                        safe_str((char *) ANSI_HILITE, buff2, &bufc2);
                        safe_str((char *) ANSI_HILITE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'x':
                    if ( mudconf.global_ansimask & MASK_BLACK ) {
                        safe_str((char *) ANSI_BLACK, buff, &bufc);
                        safe_str((char *) ANSI_BLACK, buff2, &bufc2);
                        safe_str((char *) ANSI_BLACK, buff_utf, &bufc_utf);
                    }
                    break;
                case 'X':
                    if ( mudconf.global_ansimask & MASK_BBLACK ) {
                        safe_str((char *) ANSI_BBLACK, buff, &bufc);
                        safe_str((char *) ANSI_BBLACK, buff2, &bufc2);
                        safe_str((char *) ANSI_BBLACK, buff_utf, &bufc_utf);
                    }
                    break;
                case 'r':
                    if ( mudconf.global_ansimask & MASK_RED ) {
                        safe_str((char *) ANSI_RED, buff, &bufc);
                        safe_str((char *) ANSI_RED, buff2, &bufc2);
                        safe_str((char *) ANSI_RED, buff_utf, &bufc_utf);
                    }
                    break;
                case 'R':
                    if ( mudconf.global_ansimask & MASK_BRED ) {
                        safe_str((char *) ANSI_BRED, buff, &bufc);
                        safe_str((char *) ANSI_BRED, buff2, &bufc2);
                        safe_str((char *) ANSI_BRED, buff_utf, &bufc_utf);
                    }
                    break;
                case 'g':
                    if ( mudconf.global_ansimask & MASK_GREEN ) {
                        safe_str((char *) ANSI_GREEN, buff, &bufc);
                        safe_str((char *) ANSI_GREEN, buff2, &bufc2);
                        safe_str((char *) ANSI_GREEN, buff_utf, &bufc_utf);
                    }
                    break;
                case 'G':
                    if ( mudconf.global_ansimask & MASK_BGREEN ) {
                        safe_str((char *) ANSI_BGREEN, buff, &bufc);
                        safe_str((char *) ANSI_BGREEN, buff2, &bufc2);
                        safe_str((char *) ANSI_BGREEN, buff_utf, &bufc_utf);
                    }
                    break;
                case 'y':
                    if ( mudconf.global_ansimask & MASK_YELLOW ) {
                        safe_str((char *) ANSI_YELLOW, buff, &bufc);
                        safe_str((char *) ANSI_YELLOW, buff2, &bufc2);
                        safe_str((char *) ANSI_YELLOW, buff_utf, &bufc_utf);
                    }
                    break;
                case 'Y':
                    if ( mudconf.global_ansimask & MASK_BYELLOW ) {
                        safe_str((char *) ANSI_BYELLOW, buff, &bufc);
                        safe_str((char *) ANSI_BYELLOW, buff2, &bufc2);
                        safe_str((char *) ANSI_BYELLOW, buff_utf, &bufc_utf);
                    }
                    break;
                case 'b':
                    if ( mudconf.global_ansimask & MASK_BLUE ) {
                        safe_str((char *) ANSI_BLUE, buff, &bufc);
                        safe_str((char *) ANSI_BLUE, buff2, &bufc2);
                        safe_str((char *) ANSI_BLUE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'B':
                    if ( mudconf.global_ansimask & MASK_BBLUE ) {
                        safe_str((char *) ANSI_BBLUE, buff, &bufc);
                        safe_str((char *) ANSI_BBLUE, buff2, &bufc2);
                        safe_str((char *) ANSI_BBLUE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'm':
                    if ( mudconf.global_ansimask & MASK_MAGENTA ) {
                        safe_str((char *) ANSI_MAGENTA, buff, &bufc);
                        safe_str((char *) ANSI_MAGENTA, buff2, &bufc2);
                        safe_str((char *) ANSI_MAGENTA, buff_utf, &bufc_utf);
                    }
                    break;
                case 'M':
                    if ( mudconf.global_ansimask & MASK_BMAGENTA ) {
                        safe_str((char *) ANSI_BMAGENTA, buff, &bufc);
                        safe_str((char *) ANSI_BMAGENTA, buff2, &bufc2);
                        safe_str((char *) ANSI_BMAGENTA, buff_utf, &bufc_utf);
                    }
                    break;
                case 'c':
                    if ( mudconf.global_ansimask & MASK_CYAN ) {
                        safe_str((char *) ANSI_CYAN, buff, &bufc);
                        safe_str((char *) ANSI_CYAN, buff2, &bufc2);
                        safe_str((char *) ANSI_CYAN, buff_utf, &bufc_utf);
                    }
                    break;
                case 'C':
                    if ( mudconf.global_ansimask & MASK_BCYAN ) {
                        safe_str((char *) ANSI_BCYAN, buff, &bufc);
                        safe_str((char *) ANSI_BCYAN, buff2, &bufc2);
                        safe_str((char *) ANSI_BCYAN, buff_utf, &bufc_utf);
                    }
                    break;
                case 'w':
                    if ( mudconf.global_ansimask & MASK_WHITE ) {
                        safe_str((char *) ANSI_WHITE, buff, &bufc);
                        safe_str((char *) ANSI_WHITE, buff2, &bufc2);
                        safe_str((char *) ANSI_WHITE, buff_utf, &bufc_utf);
                    }
                    break;
                case 'W':
                    if ( mudconf.global_ansimask & MASK_BWHITE ) {
                        safe_str((char *) ANSI_BWHITE, buff, &bufc);
                        safe_str((char *) ANSI_BWHITE, buff2, &bufc2);
                        safe_str((char *) ANSI_BWHITE, buff_utf, &bufc_utf);
                    }
                    break;
                default:
                    if(*(string-1) == SAFE_CHR) {
                      safe_str((char *)SAFE_CHRST, buff, &bufc);
                      safe_chr(*string, buff, &bufc);
                      safe_str((char *)SAFE_CHRST, buff2, &bufc2);
                      safe_chr(*string, buff2, &bufc2);
                      safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                      safe_chr(*string, buff_utf, &bufc_utf);                     
                    }
#ifdef SAFE_CHR2
                    else if(*(string-1) == SAFE_CHR2) {
                      safe_str((char *)SAFE_CHRST2, buff, &bufc);
                      safe_chr(*string, buff, &bufc);
                      safe_str((char *)SAFE_CHRST2, buff2, &bufc2);
                      safe_chr(*string, buff2, &bufc2);
                      safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                      safe_chr(*string, buff_utf, &bufc_utf);                     
                    }
#endif
#ifdef SAFE_CHR3
                    else if(*(string-1) == SAFE_CHR3) {
                      safe_str((char *)SAFE_CHRST3, buff, &bufc);
                      safe_chr(*string, buff, &bufc);
                      safe_str((char *)SAFE_CHRST3, buff2, &bufc2);
                      safe_chr(*string, buff2, &bufc2);
                      safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                      safe_chr(*string, buff_utf, &bufc_utf);                     
                    }
#endif
                    else {
                      safe_str((char *)SAFE_CHRST, buff, &bufc);
                      safe_chr(*string, buff, &bufc);
                      safe_str((char *)SAFE_CHRST, buff2, &bufc2);
                      safe_chr(*string, buff2, &bufc2);
                      safe_str((char *)SAFE_CHRST, buff_utf, &bufc_utf);
                      safe_chr(*string, buff_utf, &bufc_utf);                     
                    }
                    break;
                }
            }
        } else {
            if ( accent_toggle ) {
               ch1 = AccentCombo1[(int)*string];
               if ( ch1 > 0 ) {
                  ch = AccentCombo3[(int)(ch1 - 1)][(int)ch2];
                  if ( !mux_isprint[(int)ch] ) {
                     safe_chr(*string, buff2, &bufc2);
                     safe_chr(*string, buff_utf, &bufc_utf);
                  } else {
                    if ( ((int)ch == 253) || ((int)ch == 255)) {
                        safe_chr('y', buff2, &bufc2);
                        safe_chr('y', buff_utf, &bufc_utf);
                    } else {
                        safe_chr(ch, buff2, &bufc2);
                        
                        sprintf(tmpbuf, "%04x", (int)ch);
                        tmpptr = ucptoutf8(tmpbuf);
                         
                        i_utfcnt = 0;
                        tmp = tmpptr;
                        while (*tmp) {
                            s_utfbuf[i_utfcnt % 2] = *tmp;                      
                            if (i_utfcnt % 2) {
                                i_utfnum = strtol(s_utfbuf, &ptr, 16);
                                safe_chr((char)i_utfnum, buff_utf, &bufc_utf);
                            }
                            
                            i_utfcnt++;
                            tmp++;
                        }
                    }
                  }
               } else {
                  safe_chr(*string, buff2, &bufc2);
                  safe_chr(*string, buff_utf, &bufc_utf);
               }
            } else {
               safe_chr(*string, buff2, &bufc2);
               safe_chr(*string, buff_utf, &bufc_utf);
            }
            safe_chr(*string, buff, &bufc);
        }
        if ( *string )
           string++;
    }
    // toss in the normal
    safe_str(ANSI_NORMAL, buff, &bufc); 
    safe_str(ANSI_NORMAL, buff2, &bufc2); 
    safe_str(ANSI_NORMAL, buff_utf, &bufc_utf); 
    *bufptr = bufc;
    *buf2ptr = bufc2;
    *bufuptr = bufc_utf;
    
    if (tmpptr) {
        free(tmpptr);
    }
}

#endif

char t_label[100][SBUF_SIZE];
int i_label[100], i_label_lev = 0;

char *
exec(dbref player, dbref cause, dbref caller, int eval, char *dstr,
     char *cargs[], int ncargs, char *regargs[], int nregargs)
{
/* MAX_ARGS is located in externs.h - default is 30 */
#ifdef MAX_ARGS
#define NFARGS  MAX_ARGS
#else
#define NFARGS 30
#endif

    char *fargs[NFARGS], *sub_txt, *sub_buf, *sub_txt2, *sub_buf2, *orig_dstr, sub_char;
    char *buff, *bufc, *bufc2, *tstr, *tbuf, *tbufc, *savepos, *atr_gotten, *savestr, *s_label;
    char savec, ch, *ptsavereg, *savereg[MAX_GLOBAL_REGS], *t_bufa, *t_bufb, *t_bufc, c_last_chr;
    char *trace_array[3], *trace_buff, *trace_buffptr;
    static char tfunbuff[33], tfunlocal[100];
    dbref aowner, twhere, sub_aowner;
    int at_space, nfargs, gender, i, j, alldone, aflags, feval, sub_aflags, i_start, i_type, inum_val, i_last_chr;
    int is_trace, is_trace_bkup, is_top, save_count, x, y, z, sub_delim, sub_cntr, sub_value, sub_valuecnt;
#ifdef EXPANDED_QREGS
    int w;
#endif
    FUN *fp;
    UFUN *ufp, *ulfp;
    ATTR *sub_ap;
    time_t starttme, endtme;
    struct itimerval cpuchk;
    double timechk, intervalchk;
    static unsigned long tstart, tend, tinterval;
#ifdef BANGS
    int bang_not, bang_string, bang_truebool, bang_yes;
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
        
    i_start = feval = sub_delim = sub_cntr = sub_value = sub_valuecnt = 0;
#ifdef EXPANDED_QREGS
    w = 0;
#endif
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

    //if ( mudconf.cputimechk < 10 )
    //   timechk = 10;
    //else if ( mudconf.cputimechk > 3600 )
    if ( mudconf.cputimechk > 3600 )
       timechk = 3600;
    else
       timechk = mudconf.cputimechk;
    //if ( mudconf.cpuintervalchk < 10 )
    //   intervalchk = 10;
    //else if ( mudconf.cpuintervalchk > 100 )
    if ( mudconf.cpuintervalchk > 100 )
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
#ifndef NODEBUGMONITOR
    if ( (debugmem->stacktop + mudconf.func_nest_lim) > STACKMAX ) {
        mudstate.stack_toggle = 1;
        RETURN(buff);
    }
#endif
    if ( mudstate.sidefx_currcalls >= mudconf.sidefx_maxcalls 
     &&
     (mudconf.sidefx_maxcalls > 0 || (mudconf.sidefx_maxcalls = 1))) {
        mudstate.sidefx_toggle = 1;
        RETURN(buff); /* #67 */
    }

    at_space = 1;
    gender = -1;
    alldone = 0;
/*
    is_trace = Trace(player) && !(eval & EV_NOTRACE);
*/
    is_trace = (Trace(player) || ((mudstate.trace_nest_lev > 0) && i_label_lev)) && !(eval & EV_NOTRACE);
    if ( mudstate.notrace )
       is_trace = 0;
    is_top = 0;
    mudstate.eval_rec++;

    /* If we are tracing, save a copy of the starting buffer */

    savestr = NULL;
    if (is_trace) {
    is_top = tcache_empty();
    savestr = alloc_lbuf("exec.save");
        s_label = alloc_sbuf("exec.save_label");
    strcpy(savestr, dstr);
        if ( mudstate.trace_nest_lev < 99) {
           if ( (mudstate.trace_nest_lev > 0) && i_label_lev )
          strcpy(s_label, t_label[i_label_lev]);
        } else {
       strcpy(s_label, t_label[99]);
        }
    }
    if (index(dstr, ESC_CHAR)) {
    strcpy(dstr, strip_ansi(dstr));
    }

/* Debugging only
   fprintf(stderr, "EXECValue: %s\n", dstr);
*/

    memset(tfunlocal, '\0', sizeof(tfunlocal));
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

            if ( !i_start && mudstate.start_of_cmds && (*dstr == '\\') ) {
#ifdef ZENTY_ANSI
               /* With zenty ansi add an additional escape here */
               safe_chr('\\', buff, &bufc);
#endif
               mudstate.start_of_cmds = 0;
            }
        if (*dstr)
               safe_chr(*dstr, buff, &bufc);
        else
               dstr--;
            mudstate.start_of_cmds = 1;
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
                tbuf, cargs, ncargs, regargs, nregargs);
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
                tbuf, cargs, ncargs, regargs, nregargs);
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
            c_last_chr = ' ';
            
            if ( mudstate.chkcpu_toggle || (mudstate.curr_percentsubs >= mudconf.max_percentsubs) ) {
                mudstate.tog_percentsubs = 1;
/*              RETURN(buff); */ /* #67 */
                break;
            }

            if ( mudstate.curr_percentsubs >= mudconf.max_percentsubs )
               break;
        switch (savec) {
        case '\0':      /* Null - all done */
        dstr--;
        break;
#ifdef ZENTY_ANSI            
            case '\\':
/*          safe_str("%\\", buff, &bufc); */
            safe_chr('\\', buff, &bufc);
            break;
#endif
        case '%':       /* Percent - a literal % */
#ifdef ZENTY_ANSI            
               if ( ((*(dstr + 1) == SAFE_CHR) && 1) /* Needed && 1 to ignore clang warning */
#ifdef SAFE_CHR2
                                        || (*(dstr + 1) == SAFE_CHR2 )
#endif
#ifdef SAFE_CHR3
                                        || (*(dstr + 1) == SAFE_CHR3 )
#endif
)
                  safe_str("%%", buff, &bufc);
               else if ( *(dstr + 1) == 'f' )
                  safe_str("%%", buff, &bufc);
               else if ( *(dstr + 1) == '<' )
                  safe_str("%%", buff, &bufc);
               else
#endif                
                  safe_chr('%', buff, &bufc);            
          break;
#ifndef NOEXTSUBS
#ifdef TINY_SUB
        case 'x':
        case 'X':       /* ansi subs */
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'X';
#endif
#ifdef C_SUB
        case 'c':
        case 'C':       /* ansi subs */
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'C';
#endif
#ifdef M_SUB
        case 'm':
        case 'M':       /* ansi subs */
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'M';
#endif
#endif
                if ( Good_obj(mudconf.hook_obj) &&
                     (((c_last_chr == 'C') && (mudconf.sub_override & SUB_C) && !(mudstate.sub_overridestate & SUB_C)) ||
                      ((c_last_chr == 'X') && (mudconf.sub_override & SUB_X) && !(mudstate.sub_overridestate & SUB_X)) ||
                      ((c_last_chr == 'M') && (mudconf.sub_override & SUB_M) && !(mudstate.sub_overridestate & SUB_M))) ) {
                   switch(c_last_chr) {
                      case 'X':
                         sub_ap = atr_str("SUB_X");
                         i_last_chr = SUB_X;
                         break;
                      case 'M':
                         sub_ap = atr_str("SUB_M");
                         i_last_chr = SUB_M;
                         break;
                      default:
                         sub_ap = atr_str("SUB_C");
                         i_last_chr = SUB_C;
                         break;
                   }
                   if (sub_ap) {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( sub_txt  ) {
                         if ( *sub_txt ) {
                            mudstate.sub_overridestate = mudstate.sub_overridestate | i_last_chr;
                            sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0, (char **)NULL, 0);
                            mudstate.sub_overridestate = mudstate.sub_overridestate & ~i_last_chr;
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
                /* Leave the ansi code intact */
                if(!(eval & EV_PARSE_ANSI)) {        
                    safe_chr('%', buff, &bufc);
                    if(*dstr == SAFE_CHR)
                      safe_chr(SAFE_CHR, buff, &bufc);
#ifdef SAFE_CHR2
                    else if(*dstr == SAFE_CHR2)
                      safe_chr(SAFE_CHR2, buff, &bufc);
#endif
#ifdef SAFE_CHR3
                    else if(*dstr == SAFE_CHR3)
                      safe_chr(SAFE_CHR3, buff, &bufc);
#endif
                    else
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
            case '<':       /* High-Bit/UTF8 characters */
                safe_str("%<", buff, &bufc);
                break;
            case 'f':       /* Accents */
            case 'F':   
                if ( !sub_override_process(SUB_F, (char *)"%f", (char *)"F", buff, &bufc, cause, caller, feval) ) {
           safe_str("%f", buff, &bufc);
                }
                break;
        case 'r':       /* Carriage return */
        case 'R':
                if ( !sub_override_process(SUB_R, (char *)"\r\n", (char *)"R", buff, &bufc, cause, caller, feval) ) {
           safe_str((char *)"\r\n", buff, &bufc);
                }
        break;
        case 't':       /* Tab */
        case 'T':
                if ( !sub_override_process(SUB_T, (char *)"\t", (char *)"T", buff, &bufc, cause, caller, feval) ) {
           safe_str("\t", buff, &bufc);
                }
        break;
        case 'B':       /* Blank */
        case 'b':
        safe_chr(' ', buff, &bufc);
        break;
        case '0':       /* Command argument number N */
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
        case 'V':       /* Variable attribute */
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
            case '$':
                i = -1;
                dstr++;
                if ( isdigit((unsigned char)*dstr) ) {
                   i = *dstr - '0';
                   dstr++;
                   if ( isdigit((unsigned char)*dstr) ) {
                      i *= 10;
                      i += *dstr - '0';
                      dstr++;
                   }
                }
                if ( i != -1 ) {
                   if ( (i >= 0) && i < nregargs ) 
                      safe_str(regargs[i], buff, &bufc);
                }
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
#ifdef EXPANDED_QREGS
                      if ( *t_bufa && !*(t_bufa+1) && isalnum(*t_bufa) ) {
                         for ( w = 0; w < 37; w++ ) {
                            if ( mudstate.nameofqreg[w] == tolower(*t_bufa) )
                               break;
                         }
                         i = w;
                 if ( mudstate.global_regs[i] ) {
                    safe_str(mudstate.global_regs[i], buff, &bufc);
                         }
#else
                      if ( *t_bufa && !*(t_bufa+1) && isdigit(*t_bufa) ) {
                 i = (*t_bufa - '0');
                 if ((i >= 0) && (i <= 9) && mudstate.global_regs[i] ) {
                    safe_str(mudstate.global_regs[i], buff, &bufc);
                         }
#endif
                      } else {
                         for ( sub_cntr = 0 ; sub_cntr < MAX_GLOBAL_REGS; sub_cntr++ ) {
                            if (  mudstate.global_regsname[sub_cntr] &&
                                  !stricmp(mudstate.global_regsname[sub_cntr], t_bufa) ) {
                       safe_str(mudstate.global_regs[sub_cntr], buff, &bufc);
                               break;
                            }
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
        case 'O':       /* Objective pronoun */
        case 'o':
        if (gender < 0)
            gender = get_gender(cause);
        if (!gender)
            tbuf = Name(cause);
        else
            tbuf = (char *) obj[gender];
                if ( !sub_override_process(SUB_O, tbuf, (char *)"O", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        break;
        case 'P':       /* Personal pronoun */
        case 'p':
        tbuf = alloc_lbuf("exec.pronoun");
        if (gender < 0)
            gender = get_gender(cause);
        if (!gender) {
                    sprintf(tbuf, "%.1000ss", Name(cause));
        } else {
                    sprintf(tbuf, "%.1000s", (char *) poss[gender]);
        }
                if ( !sub_override_process(SUB_P, tbuf, (char *)"P", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
                free_lbuf(tbuf);
        break;
        case 'S':       /* Subjective pronoun */
        case 's':
        if (gender < 0)
            gender = get_gender(cause);
        if (!gender)
            tbuf = Name(cause);
        else
            tbuf = (char *) subj[gender];
                if ( !sub_override_process(SUB_S, tbuf, (char *)"S", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        break;
        case 'A':       /* Absolute posessive */
        case 'a':       /* idea from Empedocles */
        tbuf = alloc_lbuf("exec.absolutepossessive");
        if (gender < 0)
            gender = get_gender(cause);
        if (!gender) {
                    sprintf(tbuf, "%.1000ss", Name(cause));
        } else {
                    sprintf(tbuf, "%.1000s", (char *) absp[gender]);
        }
                if ( !sub_override_process(SUB_A, tbuf, (char *)"A", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
                free_lbuf(tbuf);
        break;
            case ':':           /* Invoker ObjID Number */
        tbuf = alloc_sbuf("exec.invoker");
        sprintf(tbuf, "#%d", cause);
                trace_buffptr = trace_buff = alloc_lbuf("buffer_for_objid");
                trace_array[0] = tbuf;
                trace_array[1] = NULL;
                trace_array[2] = NULL;
                fun_objid(trace_buff, &trace_buffptr, player, cause, cause, trace_array, 1, (char **)NULL, 0);
                if ( !sub_override_process(SUB_COLON, trace_buff, (char *)"COLON", buff, &bufc, cause, caller, feval) ) {
           safe_str(trace_buff, buff, &bufc);
                }
        free_sbuf(tbuf);
                free_lbuf(trace_buff);
                break;
        case '#':       /* Invoker DB number */
        tbuf = alloc_sbuf("exec.invoker");
        sprintf(tbuf, "#%d", cause);
                if ( !sub_override_process(SUB_NUM, tbuf, (char *)"NUM", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        free_sbuf(tbuf);
        break;
        case '!':       /* Executor DB number */
        tbuf = alloc_sbuf("exec.executor");
        sprintf(tbuf, "#%d", player);
                if ( !sub_override_process(SUB_BANG, tbuf, (char *)"BANG", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        free_sbuf(tbuf);
        break;
            case '@':           /* Immediate Executor DB number */
                tbuf = alloc_sbuf("exec.executor");
                sprintf(tbuf, "#%d", caller);
                if ( !sub_override_process(SUB_AT, tbuf, (char *)"AT", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
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
                 if ( dstr && *dstr ) {
                    inum_val = atoi(dstr);
                    if( inum_val < 0 || ( inum_val > mudstate.iter_inum ) ) {   
                        safe_str( "#-1 ARGUMENT OUT OF RANGE", buff, &bufc );
                    } else {   
                        if ( (*dstr == 'l') || (*dstr == 'L') ) {
                           safe_str( mudstate.iter_arr[0], buff, &bufc );
                        } else {
                           safe_str( mudstate.iter_arr[mudstate.iter_inum - inum_val], buff, &bufc );
                        }
                    }
                 } else {
                    if ( mudstate.iter_inum < 0 )
                        safe_str( "#-1 ARGUMENT OUT OF RANGE", buff, &bufc );
                    dstr--;
                 }
                 break;
            case 'd':       /* dtext */
            case 'D':
                 dstr++;
                 if ( dstr && *dstr ) {
                    inum_val = atoi(dstr);
                    if( inum_val < 0 || ( inum_val > (mudstate.dolistnest-1) ) ) {   
                       safe_str( "#-1 ARGUMENT OUT OF RANGE", buff, &bufc );
                    } else {   
                       if ( (*dstr == 'l') || (*dstr == 'L') ) {
                          safe_str( mudstate.dol_arr[0], buff, &bufc );
                       } else {
                          safe_str( mudstate.dol_arr[(mudstate.dolistnest - 1) - inum_val], buff, &bufc );
                       }
                    }
                 } else {
                    if ( (mudstate.dolistnest - 1) < 0)
                       safe_str( "#-1 ARGUMENT OUT OF RANGE", buff, &bufc );
                    dstr--;
                 }
                 break;
            case 'w':
            case 'W':       /* TwinkLock enactor */
        tbuf = alloc_sbuf("exec.twink");
        sprintf(tbuf, "#%d", mudstate.twinknum);
                if ( !sub_override_process(SUB_W, tbuf, (char *)"W", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        free_sbuf(tbuf);
                break;
        case 'K':       /* Invoker name */
        case 'k':
                /* We don't use sub_aowner or sub_aflags here so we don't care if they get clobbered  */
                t_bufa = atr_pget(cause, A_ANSINAME, &sub_aowner, &sub_aflags);
                t_bufb = NULL;
                if ( !ExtAnsi(cause) ) {
                   t_bufb = parse_ansi_name(cause, t_bufa);
                }
                if ( strcmp(Name(cause), strip_all_special2(t_bufa)) != 0 ) {
                   free_lbuf(t_bufa);
                   t_bufa = alloc_lbuf("normal name here");
                   strcpy(t_bufa, Name(cause));
                }
                t_bufc = alloc_sbuf("ansi_normal");
#ifdef ZENTY_ANSI
                strcpy(t_bufc, SAFE_ANSI_NORMAL);
#else
                strcpy(t_bufc, ANSI_NORMAL);
#endif
                if ( (mudconf.sub_override & SUB_K) && !(mudstate.sub_overridestate & SUB_K) && Good_obj(mudconf.hook_obj) ) {
                   sub_ap = atr_str("SUB_K");
                   if (!sub_ap) {
                      if ( t_bufb ) {
                         safe_str(t_bufb, buff, &bufc);
                         safe_str(Name(cause), buff, &bufc);
                         safe_str(t_bufc, buff, &bufc);
                      } else {
                         safe_str(t_bufa, buff, &bufc);
                      }
                   } else {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( !sub_txt ) {
                         if ( t_bufb ) {
                            safe_str(t_bufb, buff, &bufc);
                            safe_str(Name(cause), buff, &bufc);
                            safe_str(t_bufc, buff, &bufc);
                         } else {
                            safe_str(t_bufa, buff, &bufc);
                         }
                      } else {
                         mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_K;
                 sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0, (char **)NULL, 0);
                         if ( !*sub_buf ) {
                            if ( t_bufb ) {
                               safe_str(t_bufb, buff, &bufc);
                               safe_str(Name(cause), buff, &bufc);
                               safe_str(t_bufc, buff, &bufc);
                            } else {
                               safe_str(t_bufa, buff, &bufc);
                            }
                         } else
                            safe_str(sub_buf, buff, &bufc);
                         mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_K;
                         free_lbuf(sub_txt);
                         free_lbuf(sub_buf);
                      }
                   }
                } else {
                   if ( t_bufb ) {
                      safe_str(t_bufb, buff, &bufc);
                      safe_str(Name(cause), buff, &bufc);
                      safe_str(t_bufc, buff, &bufc);
                   } else {
                      safe_str(t_bufa, buff, &bufc);
                   }
                }
                free_lbuf(t_bufa);
                free_sbuf(t_bufc);
                if ( t_bufb )
                   free_lbuf(t_bufb);
        break;
        case 'N':       /* Invoker name */
        case 'n':
                if ( !sub_override_process(SUB_N, Name(cause), (char *)"N", buff, &bufc, cause, caller, feval) ) {
           safe_str(Name(cause), buff, &bufc);
                }
        break;
        case 'L':       /* Invoker location db# */
        case 'l':
        twhere = where_is(cause);
        if (Immortal(Owner(twhere)) && Dark(twhere) && Unfindable(twhere) && SCloak(twhere) && !Immortal(cause) && (twhere != cause)) {
          twhere = -1;
        } else if (Wizard(Owner(twhere)) && Dark(twhere) && Unfindable(twhere) && !Wizard(cause) && (twhere != cause)) {
          twhere = -1;
                } else if (mudconf.enforce_unfindable && (twhere != cause) &&
                            ((Immortal(Owner(cause)) && Dark(cause) && Unfindable(cause) && SCloak(cause) && !Immortal(player)) ||
                             (Wizard(Owner(cause)) && Dark(cause) && Unfindable(cause) && !Wizard(player)) ||
                             (Unfindable(cause) && !Controls(player,cause) && (cause != player)) || 
                             (Unfindable(twhere) && !Controls(player,twhere) && (Owner(twhere) != player) && (cause != player)) )) {
          twhere = -1;
                }
        tbuf = alloc_sbuf("exec.exloc");
        sprintf(tbuf, "#%d", twhere);
                if ( !sub_override_process(SUB_L, tbuf, (char *)"L", buff, &bufc, cause, caller, feval) ) {
           safe_str(tbuf, buff, &bufc);
                }
        free_sbuf(tbuf);
        break;
            case '_':           /* Yay is the trace breakpoint */
                if ( (dstr+1) && (*(dstr+1) == '<') && strchr(dstr, '>') ) {
                   dstr+=2;
                   if ( dstr && *dstr ) {
                      t_bufb = t_bufa = alloc_lbuf("trace_subs");
                      sub_value = 0;
                      while ( *dstr && (*dstr != '>') ) {
                         if ( isspace(*dstr) ) {
                            dstr++;
                            continue;
                         }
                         if ( sub_value < (SBUF_SIZE - 1) )
                            *t_bufb = ToLower(*dstr);
                         t_bufb++;
                         dstr++;
                         sub_value++;
                      }
                      *t_bufb = '\0';
                      *(t_bufa+SBUF_SIZE-1) = '\0';
                      sub_ap = atr_str("TRACE");
                      if ( sub_ap ) {
                         sub_txt = atr_get(player, sub_ap->number, &sub_aowner, &sub_aflags);
                         if ( *sub_txt && (strstr(sub_txt, t_bufa) != NULL) ) { 
                            mudstate.trace_nest_lev++;
                            i_label_lev = mudstate.trace_nest_lev;
                            if ( i_label_lev > 99 )
                               i_label_lev = 99;
                            if ( mudstate.trace_nest_lev < 98 ) {
                               trace_buff = alloc_mbuf("color_by_label");
                               sprintf(trace_buff, "TRACE_COLOR_%.*s", SBUF_SIZE - 1, t_bufa);
                               sub_ap = atr_str(trace_buff);
                               free_mbuf(trace_buff);
                               if ( sub_ap ) {
                                  sub_buf = atr_get(player, sub_ap->number, &sub_aowner, &sub_aflags);
                                  if ( !*sub_buf ) {
                                     sub_ap = atr_str("TRACE_COLOR");
                                  }
                                  free_lbuf(sub_buf);
                               } else {
                                  sub_ap = atr_str("TRACE_COLOR");
                               }
                               if ( sub_ap ) {
                                  sub_buf = atr_get(player, sub_ap->number, &sub_aowner, &sub_aflags);
                                  if ( *sub_buf ) {
                                     trace_array[0] = sub_buf;
                                     trace_array[1] = t_bufa;
                                     trace_array[2] = NULL;
                                     trace_buffptr = trace_buff = alloc_lbuf("buffer_for_trace");
                                     fun_ansi(trace_buff, &trace_buffptr, player, cause, cause, trace_array, 2, (char **)NULL, 0);
                                     if ( strlen(trace_buff) > (SBUF_SIZE - 1) ) {
                                        sprintf(t_label[mudstate.trace_nest_lev], "%.*s", SBUF_SIZE - 1, t_bufa);
                                     } else {
                                        sprintf(t_label[mudstate.trace_nest_lev], "%.*s", SBUF_SIZE - 1, trace_buff);
                                     }
                                     free_lbuf(trace_buff);
                                  } else {
                                     sprintf(t_label[mudstate.trace_nest_lev], "%.*s", SBUF_SIZE - 1, t_bufa);
                                  }
                                  free_lbuf(sub_buf);
                               } else {
                                     sprintf(t_label[mudstate.trace_nest_lev], "%.*s", SBUF_SIZE - 1, t_bufa);
                               }
                               i_label[mudstate.trace_nest_lev]=1;
                            } else {
                               sprintf(t_label[99], "%s%s%s", ANSI_RED, "* MAX-REACHED *", ANSI_NORMAL);
                            }
                         } else if ( *sub_txt && (*t_bufa == '-') && (strstr(sub_txt, t_bufa+1) != NULL) ) {
                            if ( mudstate.trace_nest_lev < 98 ) {
                               for ( inum_val = 0; inum_val <= mudstate.trace_nest_lev; inum_val++ ) {
                                  if ( *(t_label[inum_val]) && 
                                        (stricmp(strip_all_special2(t_label[inum_val]), t_bufa+1) == 0) &&
                                        i_label[inum_val] ) {
                                     i_label[inum_val] = 0;
                                     break;
                                  }
                               }
                               i_label_lev = 0;
                               for ( inum_val = mudstate.trace_nest_lev; inum_val >= 0; inum_val-- ) {
                                  if ( i_label[inum_val] == 1 ) {
                                     i_label_lev = inum_val;
                                     break;
                                  }
                               }
                            }
/*                          mudstate.trace_nest_lev--; */
                            if ( mudstate.trace_nest_lev < 0 )
                               mudstate.trace_nest_lev = 0;
                         } else if ( !stricmp(t_bufa, "off") ) {
                            i_label_lev = mudstate.trace_nest_lev = 0;
                            for ( inum_val = 0; inum_val < 100; inum_val++) {
                               *(t_label[inum_val]) = '\0';
                               i_label[inum_val] = 0;
                            }
                         }
                         free_lbuf(sub_txt);
                      }
                      free_lbuf(t_bufa);
                   }
                }
                break;
#ifndef NOEXTSUBS
#ifndef C_SUB
            case 'C':       /* Command substitution */
            case 'c':
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'C';
#endif
#ifndef TINY_SUB
            case 'X':       /* Command substitution */
            case 'x':
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'X';
#endif
#ifndef M_SUB
            case 'M':       /* Command substitution */
            case 'm':
                 if ( c_last_chr == ' ' )
                    c_last_chr = 'M';
#endif
#endif
                if ( Good_obj(mudconf.hook_obj) &&
                     (((c_last_chr == 'C') && (mudconf.sub_override & SUB_C) && !(mudstate.sub_overridestate & SUB_C)) ||
                      ((c_last_chr == 'X') && (mudconf.sub_override & SUB_X) && !(mudstate.sub_overridestate & SUB_X)) ||
                      ((c_last_chr == 'M') && (mudconf.sub_override & SUB_M) && !(mudstate.sub_overridestate & SUB_M))) ) {
                   switch(c_last_chr) {
                      case 'X':
                         sub_ap = atr_str("SUB_X");
                         i_last_chr = SUB_X;
                         break;
                      case 'M':
                         sub_ap = atr_str("SUB_M");
                         i_last_chr = SUB_M;
                         break;
                      default:
                         sub_ap = atr_str("SUB_C");
                         i_last_chr = SUB_C;
                         break;
                   }
                   if (sub_ap) {
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      if ( sub_txt ) {
                         if ( *sub_txt ) {
                            mudstate.sub_overridestate = mudstate.sub_overridestate | i_last_chr;
                            sub_buf = exec(mudconf.hook_obj, cause, caller, feval, sub_txt, (char **)NULL, 0, (char **)NULL, 0);
                            mudstate.sub_overridestate = mudstate.sub_overridestate & ~i_last_chr;
                            safe_str(sub_buf, buff, &bufc);
                            free_lbuf(sub_txt);
                            free_lbuf(sub_buf);
                            break;
                         }
                         free_lbuf(sub_txt);
                      }
                   }
                } 
                if ( mudstate.password_nochk == 0 ) {
                   t_bufb = alloc_lbuf("password_nochk");
                   strcpy(t_bufb, mudstate.curr_cmd);
#ifndef TINY_SUB                                                           
                   t_bufa = replace_string("%X", "  ", t_bufb, 0);
                   free_lbuf(t_bufb);
                   t_bufb = replace_string("%x", "  ", t_bufa, 0);
                   free_lbuf(t_bufa);
#endif
#ifndef C_SUB                                                           
                   t_bufa = replace_string("%C", "  ", t_bufb, 0);
                   free_lbuf(t_bufb);
                   t_bufb = replace_string("%c", "  ", t_bufa, 0);
                   free_lbuf(t_bufa);
#endif
#ifndef M_SUB                                                           
                   t_bufa = replace_string("%M", "  ", t_bufb, 0);
                   free_lbuf(t_bufb);
                   t_bufb = replace_string("%m", "  ", t_bufa, 0);
                   free_lbuf(t_bufa);
#endif
                   safe_str(t_bufb, buff, &bufc);
                   free_lbuf(t_bufb);
                } else {
                   safe_str("XXX", buff, &bufc);
                }
                break;
        default:        /* Just copy */
                if ( !mudstate.sub_includestate && *mudconf.sub_include && 
                     Good_obj(mudconf.hook_obj) && (strchr(mudconf.sub_include, ToLower(*dstr)) != NULL) ) {
                   t_bufa = alloc_sbuf("sub_include");
                   sprintf(t_bufa, "SUB_%c", *dstr);
                   if ( !ok_attr_name(t_bufa) ) {
                      sprintf(t_bufa, "SUB_%03d", (int)*dstr);
                   }
                   sub_ap = atr_str(t_bufa);
                   sub_delim = sub_value = 0;
                   if (sub_ap) {
                      mudstate.sub_includestate = 1;
                      sub_txt = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                      sprintf(t_bufa, "CHR_%c", *dstr);
                      if ( !ok_attr_name(t_bufa) ) {
                         sprintf(t_bufa, "CHR_%03d", (int)*dstr);
                      }
                      sub_ap = atr_str(t_bufa);
                      if ( sub_ap ) {
                         sub_txt2 = atr_pget(mudconf.hook_obj, sub_ap->number, &sub_aowner, &sub_aflags);
                         if ( sub_txt2 && *sub_txt2) {
                            sub_buf2 = exec(mudconf.hook_obj, cause, caller, 
                                            EV_EVAL|EV_STRIP|EV_FCHECK, sub_txt2, (char **)NULL, 0, (char **)NULL, 0);
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
                                         sub_txt, (char **)&t_bufa, 1, (char **)NULL, 0);
                            else
                               sub_buf = exec(mudconf.hook_obj, player, caller, EV_EVAL|EV_STRIP|EV_FCHECK, 
                                         sub_txt, (char **)NULL, 0, (char **)NULL, 0);
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
         * Possible combinations are: ! !! !$ !!$ !^ !!^
         * Stepping past the possible combinations at the beginning of
         * the function's name, pushing a pointer up as we go. The
         * modified pointer will be used to search the hash table
         * if a bang condition is found.
         */
        tbangc = tbuf;
        bang_not = 0;
        bang_yes = 0;
        bang_string = 0;
            bang_truebool = 0;
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
        } else if ((bang_not || bang_yes) && *tbangc == '^') {
        bang_string = 1;
                bang_truebool = 1;
        tbangc++;
            }
        if (bang_not || bang_yes) {
        fp = (FUN *) hashfind(tbangc, &mudstate.func_htab);
        ufp = NULL;
                ulfp = NULL;
                if ( fp == NULL ) {
           ufp = (UFUN *) hashfind(tbangc, &mudstate.ufunc_htab);
        }
                if ( ufp == NULL ) {
                   sprintf(tfunlocal, "%d_%s", Owner(player), tbangc);
           ulfp = (UFUN *) hashfind(tfunlocal, &mudstate.ulfunc_htab);
                   if ( ulfp && (!Good_chk(ulfp->obj) || (ulfp->orig_owner != Owner(ulfp->obj))) ) {
                      ulfp = NULL;
                   }
                }
            } else {
        fp = (FUN *) hashfind(tbuf, &mudstate.func_htab);
            ufp = NULL;
                ulfp = NULL;
                if ( fp == NULL ) {
           ufp = (UFUN *) hashfind(tbuf, &mudstate.ufunc_htab);
        }
                if ( ufp == NULL ) {
                   sprintf(tfunlocal, "%d_%s", Owner(player), tbuf);
           ulfp = (UFUN *) hashfind(tfunlocal, &mudstate.ulfunc_htab);
                   if ( ulfp && (!Good_chk(ulfp->obj) || (ulfp->orig_owner != Owner(ulfp->obj))) ) {
                      ulfp = NULL;
                   }
                }
            }
#else
        fp = (FUN *) hashfind(tbuf, &mudstate.func_htab);
        /* If not a builtin func, check for global func */

        ufp = NULL;
            ulfp = NULL;
        if (fp == NULL) {
        ufp = (UFUN *) hashfind(tbuf, &mudstate.ufunc_htab);
        }
            if ( ufp == NULL ) {
                sprintf(tfunlocal, "%d_%s", Owner(player), tbuf);
        ulfp = (UFUN *) hashfind(tfunlocal, &mudstate.ulfunc_htab);
                if ( ulfp && (!Good_chk(ulfp->obj) || (ulfp->orig_owner != Owner(ulfp->obj))) ) {
                   ulfp = NULL;
                }
            }
#endif
            /* Compare to see if it has an IGNORE mask */
            if ( fp && (fp->perms & 0x00007F00) ) {
           check_access(player, fp->perms, fp->perms2, 0);
               if ( mudstate.func_ignore && !mudstate.func_bypass) {
                  memset(tfunbuff, 0, sizeof(tfunbuff));
#ifdef BANGS
                  if ( bang_not || bang_yes ) {
                     sprintf(tfunbuff, "_%.31s", tbangc);
#else
                  if ( 0 ) {
                     sprintf(tfunbuff, "_%.31s", tbuf);
#endif
                  } else {
                     sprintf(tfunbuff, "_%.31s", tbuf);
                  }
              ufp = (UFUN *) hashfind((char *)tfunbuff,
                       &mudstate.ufunc_htab);
                  if ( ufp == NULL ) {
                      sprintf(tfunlocal, "%d_%s", Owner(player), tfunbuff);
              ulfp = (UFUN *) hashfind((char *)tfunlocal, &mudstate.ulfunc_htab);
                      if ( ulfp && (!Good_chk(ulfp->obj) || (ulfp->orig_owner != Owner(ulfp->obj))) ) {
                         ulfp = NULL;
                      }
                  }
               }
            }
        /* Do the right thing if it doesn't exist */

        if (!fp && !ufp && !ulfp) {
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

        if (ufp || ulfp)
        nfargs = NFARGS;
        else if (fp->nargs < 0)
        nfargs = -fp->nargs;
        else
        nfargs = NFARGS;
        tstr = dstr;
            i_type = 0;
            if ( ((fp && ((fp->flags & FN_NO_EVAL) || (fp->perms2 & CA_NO_EVAL)) && !(fp->perms2 & CA_EVAL)) ||
                  (ufp && (ufp->perms2 & CA_NO_EVAL)) ||
                  (ulfp && (ulfp->perms2 & CA_NO_EVAL))) &&
                 (!(ufp && (ufp->perms2 & CA_EVAL)) || !(ulfp && (ulfp->perms2 & CA_EVAL))) ) {
                if ( mudconf.brace_compatibility )
                   feval = (eval & ~EV_EVAL & ~EV_STRIP) | EV_STRIP_ESC;
                else
                   feval = (eval & ~EV_EVAL) | EV_STRIP_ESC;
            } else
                feval = eval;
            if ( (fp && (fp->perms2 & CA_NO_EVAL)) ||
                 (ufp && (ufp->perms2 & CA_NO_EVAL)) ||
                 (ulfp && (ulfp->perms2 & CA_NO_EVAL)) ) {
                i_type = 1;
            }
            if ( (ufp && (ufp->perms & CA_EVAL)) ||
                 (ulfp && (ulfp->perms & CA_EVAL)) ) {
                feval = (feval | EV_EVAL | EV_STRIP | ~EV_STRIP_ESC);
                i_type = 0;
            }
            if ( (ufp && (ufp->flags & FN_NOTRACE)) ||
                 (ulfp && (ulfp->perms & CA_EVAL)) ) {
                feval = (feval | EV_NOTRACE);
            }
        dstr = parse_arglist(player, cause, caller, dstr + 1,
                 ')', feval, fargs, nfargs,
                 cargs, ncargs, i_type, regargs, nregargs);
        /* If no closing delim, just insert the '(' and
         * continue normally */

            if ( i_type ) { 
               feval = (feval | EV_EVAL | EV_STRIP | ~EV_STRIP_ESC);
            }
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


            if ( !ufp )
               ufp = ulfp;

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
                    if ( ufp->flags & FN_NOTRACE ) {
                       is_trace_bkup = mudstate.notrace;
                       mudstate.notrace = 1;
                    }
            tbuf = exec(i, cause, player, feval, tstr, fargs, nfargs, regargs, nregargs);
                    is_trace_bkup = 0;
                    if ( ufp->flags & FN_NOTRACE ) {
                       mudstate.notrace = is_trace_bkup;
                    }
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
                        if ( bang_truebool ) {
                           if ( (*tbuf == '#') && (*(tbuf+1) == '-') ) {
                              tbuf[0] = '0';
                           } else if ( (*tbuf == '0') && !*(tbuf+1)) {
                              tbuf[0] = '0';
                           } else {
                              while (isspace(*tbuf) && *tbuf)
                                 tbuf++;
                              if (*tbuf) {
                                 tbuf[0] = '1';
                              } else {
                                 tbuf[0] = '1';
                                 tbuf[0] = '0';
                              }
                           }
                        } else {
               tbuf[0] = '1';
                        }
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
                        if ( bang_truebool ) {
                           bufc2 = buff;
                           if ( (*bufc2 == '#') && (*(bufc2+1) == '-') ) {
                              buff[0] = '0';
                           } else if ( (*bufc2 == '0') && !*(bufc2+1)) {
                              buff[0] = '0';
                           } else {
                              while (*bufc2 && isspace(*bufc2))
                                 bufc2++;
                              if (*bufc2) {
                                 buff[0] = '1';
                              } else {
                                 buff[0] = '0';
                              }
                           }
                        } else {
                           buff[0] = '1';
                        }
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
        i_start++;
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
    tcache_add(player, savestr, buff, s_label);
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
