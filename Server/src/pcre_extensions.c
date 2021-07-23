/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/
#include "copyright.h"
#include "autoconf.h"
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include "mudconf.h"
#include "config.h"
#if defined PCRE_SYSLIB || defined PCRE_BUILTIN
#include <pcre.h>
#else
#include "pcre.h"
#endif
#include "externs.h"
#include "match.h"
#include "flags.h"
#include "alloc.h"
#include "local.h"
#include "functions.h"
#include "command.h"

const unsigned char *tables = NULL;


static void

/* Static int's can't be externed */
ival(char *buff, char **bufcx, int result)
{
  static char tempbuff[LBUF_SIZE/2];

  sprintf(tempbuff, "%d", result);
  safe_str(tempbuff, buff, bufcx);
}

signed char qreg_indexes[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* Mush built in functions needing externs */
extern dbref       FDECL(match_thing, (dbref, char *));

/* regexp foo */
/** Regexp match, possibly case-sensitive, and with no memory.
 *
 * This routine will cause crashes if fed NULLs instead of strings.
 *
 * \param s regexp to match against.
 * \param d string to check.
 * \param cs if 1, case-sensitive; if 0, case-insensitive.
 * \retval 1 d matches s.
 * \retval 0 d doesn't match s.
 */
int
quick_regexp_match(const char *RESTRICT s, const char *RESTRICT d, int cs)
{
  pcre *re;
  const char *errptr;
  int erroffset;
  int offsets[99];
  int r;
  int flags = 0;                /* There's a PCRE_NO_AUTO_CAPTURE flag to turn all raw
                                   ()'s into (?:)'s, which would be nice to use,
                                   except that people might use backreferences in
                                   their patterns. Argh. */

  if (!cs)
    flags |= PCRE_CASELESS;

  if ((re = pcre_compile(s, flags, &errptr, &erroffset, tables)) == NULL) {
    /*
     * This is a matching error. We have an error message in
     * errptr that we can ignore, since we're doing
     * command-matching.
     */
    return 0;
  }
  /*
   * Now we try to match the pattern. The relevant fields will
   * automatically be filled in by this.
   */
  r = pcre_exec(re, NULL, d, strlen(d), 0, 0, offsets, 99);

  free(re);

  return (r >= 0);
}

/* ---------------------------------------------------------------------------
 * Initialize regular expression (regexp) conditions
 * ---------------------------------------------------------------------------
 */
int re_subpatterns = -1;  /**< Number of subpatterns in regexp */
int *re_offsets;          /**< Array of offsets to subpatterns */
char *re_from = NULL;     /**< Pointer to last match position */
extern signed char qreg_indexes[UCHAR_MAX + 1];

/* ---------------------------------------------------------------------------
 * fun_regmatch - like match but use regular expression matching
 */

void
do_regmatch(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
           char *fargs[], int nfargs, char *cargs[], int ncargs, int key, char *name)
{
  int i, nqregs, curq, erroffset, offsets[99], subpatterns, qindex, flags, len;
  char *qregs[MAX_GLOBAL_REGS];
  pcre *re;
  const char *errptr;

  if (!fn_range_check(name, nfargs, 2, 3, buff, bufcx))
    return;

  flags = 0;
  if ( key & 1 )
    flags = PCRE_CASELESS;

  /* Don't care about saving sub expressions */
  if ( (nfargs == 2) || ((nfargs > 2) && !*fargs[2]) ) {
    ival(buff, bufcx, quick_regexp_match(fargs[1], fargs[0], (flags ? 0 : 1)));
    return;
  }

  if ((re = pcre_compile(fargs[1], flags, &errptr, &erroffset, tables)) == NULL) {
    /* Matching error. */
    safe_str("#-1 REGEXP ERROR: ", buff, bufcx);
    safe_str((char *)errptr, buff, bufcx);
    return;
  }
  len = strlen(fargs[0]);
  subpatterns = pcre_exec(re, NULL, fargs[0], len, 0, 0, offsets, 99);
  ival(buff, bufcx, (subpatterns >=0));

  /* We need to parse the list of registers.  Anything that we don't parse
   * is assumed to be -1.  If we didn't match, or the match went wonky,
   * then set the register to empty.  Otherwise, fill the register with
   * the subexpression.
   */
  if (subpatterns == 0)
    subpatterns = 33;
  nqregs = list2arr(qregs, MAX_GLOBAL_REGS, fargs[2], ' ');
  for (i = 0; i < nqregs; i++) {
    if (qregs[i] && qregs[i][0] && !qregs[i][1] &&
        ((qindex = qreg_indexes[(unsigned char) qregs[i][0]]) != -1))
      curq = qindex;
    else
      curq = -1;
    if (curq < 0 || curq >= MAX_GLOBAL_REGS)
      continue;
    if (subpatterns < 0)
      mudstate.global_regs[curq][0] = '\0';
    else
      pcre_copy_substring(fargs[0], offsets, subpatterns, i,
                          mudstate.global_regs[curq], (LBUF_SIZE-1));
  }
  free(re);
}

FUNCTION(fun_regmatch)
{
   do_regmatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 0, (char *)"REGMATCH");
}

FUNCTION(fun_regmatchi)
{
   do_regmatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 1, (char *)"REGMATCHI");
}

void
do_regedit(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
           char *fargs[], int nfargs, char *cargs[], int ncargs, int key)
{
  pcre *re;
  pcre_extra *study = NULL;
  const char *errptr;
  char *start, *abufptr, *abuf, *prebuf, *prep, *postbuf, *postp, *str, 
       *obufptr, *obuf, *obuf2, *mybuff, *mybuffptr, tmp, *sregs[100];
  int subpatterns, offsets[99], erroffset, flags, all, i_key,
      match_offset, len, i, p, loop, j;
  time_t t_now, t_later;

  flags = all = match_offset = 0;

  i_key = flags = 0;

  if ( key == 8 ) {
     key = 3;
     i_key = 8;
  }

  if ( key & 1 )
    flags = PCRE_CASELESS;

  if ( key & 2 )
    all = 1;

  prep = prebuf = alloc_lbuf("do_regedit_prebuf");
  postp = postbuf = alloc_lbuf("do_regedit_postbuf");
  if ( i_key == 8 ) {
     abuf = alloc_lbuf("do_regedit");
     strcpy(abuf, fargs[0]);
  } else {
  abuf = exec(player, cause, caller,
              EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs, (char **)NULL, 0);
  }
  safe_str(strip_ansi(abuf), postbuf, &postp);
  free_lbuf(abuf);

  t_now = time(NULL);
  for (i = 1; i < nfargs - 1; i += 2) {
    if ( mudstate.chkcpu_toggle )
       break;
    postp = postbuf;
    prep = prebuf;
    /* old postbuf is new prebuf */
    safe_str(postbuf, prebuf, &prep);
    if ( i_key == 8 ) {
       abuf = alloc_lbuf("do_regedit");
       strcpy(abuf, fargs[i]);
    } else {
       abuf = exec(player, cause, caller,
                   EV_STRIP | EV_FCHECK | EV_EVAL, fargs[i], cargs, ncargs, (char **)NULL, 0);
    }

    if ((re = pcre_compile(abuf, flags, &errptr, &erroffset, tables)) == NULL) {
      if ( i_key == 8 ) {
         safe_str(postbuf, buff, bufcx);
      } else {
         /* Matching error. */
         safe_str("#-1 REGEXP ERROR: ", buff, bufcx);
         safe_str((char *)errptr, buff, bufcx);
      }
      free_lbuf(abuf);
      free_lbuf(prebuf);
      free_lbuf(postbuf);
      return;
    }
    free_lbuf(abuf);
    if (all) {
      study = pcre_study(re, 0, &errptr);
      if (errptr != NULL) {
        free(re);
        if ( i_key == 8 ) {
           safe_str(postbuf, buff, bufcx);
        } else {
           safe_str("#-1 REGEXP ERROR: ", buff, bufcx);
           safe_str((char *)errptr, buff, bufcx);
        }
        free_lbuf(prebuf);
        free_lbuf(postbuf);
        return;
      }
    }
    len = strlen(prebuf);
    start = prebuf;
    subpatterns = pcre_exec(re, study, prebuf, len, 0, 0, offsets, 99);

    /* Match wasn't found... we're done */
    if (subpatterns < 0) {
      safe_str(prebuf, postbuf, &postp);
      free(re);
      if (study)
        free(study);
      continue;
    }
    do {
      t_later = time(NULL);
      /* Copy up to the start of the matched area */
      if ( t_later > (t_now + 5) ) {
         mudstate.chkcpu_toggle = 1;
      }
      if ( mudstate.chkcpu_toggle )
         break;
      tmp = prebuf[offsets[0]];
      prebuf[offsets[0]] = '\0';
      safe_str(start, postbuf, &postp);
      prebuf[offsets[0]] = tmp;
    /* Now copy in the replacement, putting in captured sub-expressions */
      re_from = prebuf;
      re_offsets = offsets;
      re_subpatterns = subpatterns;

      str = fargs[i+1];
      obuf = alloc_lbuf("regedit_sub_dollars");
      mybuffptr = mybuff = alloc_lbuf("regedit_sub_dollars");
      while (*str) {
         if ( *str == '$' )
            break;
         *mybuffptr = *str;
         str++;
         mybuffptr++;
      }
      /* Do '$##' substitutions before parsing the string */
      if ( *str == '$' ) {
         loop = 1;
         while (*str) {
            while ( loop ) {
              str++;
              if (isdigit((unsigned char) *str)) {
                 p = *str - '0';
                 str++;
                 if (isdigit((unsigned char) *str)) {
                    p *= 10;
                    p += *str - '0';
                    str++;
                 }
              } else
                 break;
      
              if (p >= re_subpatterns || re_offsets == NULL || re_from == NULL)
                 break;
         
              sprintf(obuf, "%c$%d$", '%', p);
              safe_str(obuf, mybuff, &mybuffptr);
              break;
            }
            while ( *str && *str != '$' && (strlen(mybuff) < (LBUF_SIZE - 1)) ) {
               *mybuffptr = *str;
               str++;
               mybuffptr++;
            }
         } /* While *str */
         *mybuffptr = '\0';
         if ( key & 4 ) { 
            memset(obuf, '\0', LBUF_SIZE);
            abufptr = mybuff;
            obufptr = obuf;
            obuf2 = alloc_lbuf("pcre_edit_crap");
            while ( *abufptr ) { 
               if ( *abufptr == '%' && *(abufptr+1) == '$' ) {
                  abufptr+=2;
                  p = atoi(abufptr);
                  while ( *abufptr && *abufptr != '$' )
                     abufptr++;
                  pcre_copy_substring(re_from, re_offsets, re_subpatterns, p, obuf2,
                                      LBUF_SIZE);
                  safe_str(obuf2, obuf, &obufptr);
               } else {
                  safe_chr(*abufptr, obuf, &obufptr);
               }
               abufptr++;
            }            
            free_lbuf(obuf2);
            safe_str(obuf, postbuf, &postp);
         } else {
            for ( j = 0; j < 100; j++ ) {
               sregs[j] = NULL;
               if ( j < re_subpatterns ) {
                  sregs[j] = alloc_lbuf("regexp_crap");
                  pcre_copy_substring(re_from, re_offsets, re_subpatterns, j, sregs[j],
                                      LBUF_SIZE);
               }
            }
            abuf = exec(player, cause, caller,
                        EV_STRIP | EV_FCHECK | EV_EVAL, mybuff, cargs, ncargs, sregs, re_subpatterns);
            for ( j = 0; j < re_subpatterns; j++ ) {
               if ( j >= 100 )
                  break;
               free_lbuf(sregs[j]);
            }
            safe_str(abuf, postbuf, &postp);
            free_lbuf(abuf);
         } 
      } else {
         if ( key & 4 ) {
            safe_str(fargs[i+1], postbuf, &postp);
         } else {
            abuf = exec(player, cause, caller,
                        EV_STRIP | EV_FCHECK | EV_EVAL, fargs[i+1], cargs, ncargs, (char **)NULL, 0);
            safe_str(abuf, postbuf, &postp);
            free_lbuf(abuf);
         }
      }

      free_lbuf(mybuff);
      free_lbuf(obuf);
      if ( (*bufcx == (buff + LBUF_SIZE - 1)) &&
           (mudstate.ufunc_nest_lev >= mudconf.func_nest_lim) ) {
        break;
      }

      start = prebuf + offsets[1];
      match_offset = offsets[1];
      /* Make sure we advance at least 1 char */
      if (offsets[0] == match_offset)
        match_offset++;
    } while (all && match_offset < len && (subpatterns =
                                           pcre_exec(re, study, prebuf, len,
                                                     match_offset, 0, offsets,
                                                     99)) >= 0);

    /* Now copy everything after the matched bit */
    safe_str(start, postbuf, &postp);
    free(re);
    if (study)
      free(study);

    re_offsets = NULL;
    re_subpatterns = -1;
    re_from = NULL;
  }

  safe_str(postbuf, buff, bufcx);
  free_lbuf(postbuf);
  free_lbuf(prebuf);
}

FUNCTION(fun_regedit)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDIT) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 0);
   }
}

FUNCTION(fun_regediti)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITI) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 1);
   }
}

FUNCTION(fun_regeditall)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITALL) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 2);
   }
}

FUNCTION(fun_regeditalli)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITALLI) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 3);
   }
}
FUNCTION(fun_regeditlit)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITLIT) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 4);
   }
}

FUNCTION(fun_regeditilit)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITLITI) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 5);
   }
}

FUNCTION(fun_regeditalllit)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITALLLIT) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 6);
   }
}

FUNCTION(fun_regeditallilit)
{
   if ( nfargs < 3 ) {
       safe_str("#-1 FUNCTION (REGEDITALLILIT) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
   } else {
      do_regedit(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 7);
   }
}

void
do_regrab(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
          char *fargs[], int nfargs, char *cargs[], int ncargs, int key,
          char *name, int i_topval)
{
  char *r, *s, sep, *osep, osepd[2];
  pcre *re;
  pcre_extra *study;
  const char *errptr;
  int erroffset, offsets[99], flags, all, first;

  flags = all = first = 0;
  memset(osepd, '\0', sizeof(osepd));

  if (!fn_range_check(name, nfargs, 2, i_topval, buff, bufcx))
      return;
  if ( (nfargs >=3) && *fargs[2] )
     sep = *fargs[2];
  else
     sep = ' ';

  if ( (nfargs >= 4) && *fargs[3] ) {
    if ( mudconf.delim_null && (strcmp(fargs[3], (char *)"@@") == 0) ) {
       osep = osepd;
    } else {
       osep = fargs[3];
    }
  } else {
    osepd[0] = sep;
    osep = osepd;
  }

  s = trim_space_sep(fargs[0], sep);

  if ( key & 1 )
    flags = PCRE_CASELESS;

  if ( key & 2 )
    all = 1;

  if ((re = pcre_compile(fargs[1], flags, &errptr, &erroffset, tables)) == NULL) {
    /* Matching error. */
    safe_str("#-1 REGEXP ERROR: ", buff, bufcx);
    safe_str((char *)errptr, buff, bufcx);
    return;
  }

  study = pcre_study(re, 0, &errptr);
  if (errptr != NULL) {
    safe_str("#-1 REGEXP ERROR: ", buff, bufcx);
    safe_str((char *)errptr, buff, bufcx);
    free(re);
    return;
  }

  do {
    r = split_token(&s, sep);
    if (pcre_exec(re, study, r, strlen(r), 0, 0, offsets, 99) >= 0) {
      if (all && first && *osep) 
        safe_str(osep, buff, bufcx);
      safe_str(r, buff, bufcx);
      first = 1;
      if (!all)
        break;
    }
  } while (s);

  free(re);
  if (study)
    free(study);
}

FUNCTION(fun_regrab)
{
   do_regrab(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 0, "REGRAB", 3);
}

FUNCTION(fun_regrabi)
{
   do_regrab(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 1, "REGRABI", 3);
}

FUNCTION(fun_regraball)
{
   do_regrab(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 2, "REGRABALL", 4);
}

FUNCTION(fun_regraballi)
{
   do_regrab(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 3, "REGRABALLI", 4);
}

char *
grep_internal_regexp(dbref player, dbref thing, char *wcheck, char *watr, int flags, int i_atrreg)
{
    dbref aowner, othing;
    int ca, aflags, go2;
    ATTR *attr;
    char *as, *buf, *bp, *retbuff, *go1;
    char tbuf[80];

    retbuff = alloc_lbuf("grep_regexp_int");
    othing = thing;
    go1 = strpbrk(watr, "?\\*");

    bp = retbuff;
    for (ca = atr_head(thing, &as); ca; ca = atr_next(&as)) {
        if ( mudstate.chkcpu_toggle )
           break;
        attr = atr_num(ca);
        if (!attr)
            continue;
        strcpy(tbuf, attr->name);
        if (go1) {
            if ( i_atrreg ) {
               go2 = quick_regexp_match(watr, tbuf, (flags ? 0 : 1));
            } else {
               go2 = quick_wild(watr, tbuf);
            }
        } else
            go2 = !stricmp(watr, tbuf);
        if (go2) {
            buf = atr_get(thing, ca, &aowner, &aflags);
            if ( (God(player) || !( ((attr->flags & AF_GOD) || (aflags & AF_GOD)) &&
                                    ((attr->flags & AF_PINVIS) || (aflags & AF_PINVIS)))) &&
                 (Wizard(player) || (!(attr->flags & AF_PINVIS) && !(aflags & AF_PINVIS))) &&
                 (Read_attr(player, othing, attr, aowner, aflags, 0)) ) {

                if ( quick_regexp_match(wcheck, buf, (flags ? 0 : 1)) ) {
                    safe_str(tbuf, retbuff, &bp);
                    safe_chr(' ', retbuff, &bp);
                }
            }
            free_lbuf(buf);
        }
    }
    if (bp != retbuff)
        *(bp - 1) = '\0';
    else
        *bp = '\0';

    return retbuff;
}



void
do_regrep(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
          char *fargs[], int nfargs, char *cargs[], int ncargs, int key, int i_cluster, char *name)
{
   dbref object, aowner;
   int aflags, first, last_ret, i_atrreg;
   char *ret, *s_text, *s_strtok, *s_strtokptr;
   ATTR *attr;

   if (!fn_range_check(name, nfargs, 3, 4, buff, bufcx))
        return;

   i_atrreg = 0;
   if ( (nfargs > 3) && *fargs[3] )
      i_atrreg = atoi(fargs[3]);

   if ( i_atrreg != 0 )
      i_atrreg = 1;

   init_match(player, fargs[0], NOTYPE);
   match_everything(MAT_EXIT_PARENTS);
   object = noisy_match_result();
   if (object == NOTHING) {
      safe_str("#-1", buff, bufcx);
   } else if (!Examinable(player, object)) {
      safe_str("#-1", buff, bufcx);
   } else if ( i_cluster && Good_chk(object) && Cluster(object) ) {
      attr = atr_str("_CLUSTER");
      if ( attr ) {
         s_text = atr_get(object, attr->number, &aowner, &aflags);
         if ( *s_text ) {
            s_strtok = strtok_r(s_text, " ", &s_strtokptr);
            first = last_ret = 0;
            while (s_strtok) {
               aowner = match_thing(player, s_strtok);
               if ( !mudstate.chkcpu_toggle && Good_chk(aowner) && Cluster(aowner) ) {
                  ret = grep_internal_regexp(player, aowner, fargs[2], fargs[1], key, i_atrreg);
                  if ( first && last_ret )
                     safe_chr(' ', buff, bufcx);
                  safe_str(ret, buff, bufcx);
                  if ( *ret )
                     last_ret = 1;
                  else
                     last_ret = 0;
                  free_lbuf(ret);
                  first = 1;
               }
               s_strtok = strtok_r(NULL, " ", &s_strtokptr);
            }
         } else {
            safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
         }
         free_lbuf(s_text);
      } else {
         safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      }
   } else if ( !i_cluster ) {
      ret = grep_internal_regexp(player, object, fargs[2], fargs[1], key, i_atrreg);
      safe_str(ret, buff, bufcx);
      free_lbuf(ret);
   } else {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
   }
}

FUNCTION(fun_regrep)
{
   do_regrep(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 0, 0, (char *)"REGREP");
}

FUNCTION(fun_regrepi)
{
   do_regrep(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 1, 0, (char *)"REGREPI");
}

FUNCTION(fun_cluster_regrep)
{
   do_regrep(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 0, 1, (char *)"CLUSTER_REGREP");
}

FUNCTION(fun_cluster_regrepi)
{
   do_regrep(buff, bufcx, player, cause, caller, fargs, nfargs, 
             cargs, ncargs, 1, 1, (char *)"CLUSTER_REGREPI");
}

void
do_reswitch(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
            char *fargs[], int nfargs, char *cargs[], int ncargs, int key)
{
  /* this works a bit like the @switch/regexp command */

  int first, found, cs, j;
  char *mstr, *pstr, *tbuf1, *tbuff;

  if (nfargs < 2) {
     return;
  }

  first = cs = 1;
  found = 0;

  if ( key & 2 )
    first = 0;

  if ( (key & 1) )
    cs = 0;

  mstr = exec(player, cause, caller,
              EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs, (char **)NULL, 0);

  /* try matching, return match immediately when found */


  for (j = 1; j < (nfargs - 1); j += 2) {
    pstr = exec(player, cause, caller,
                EV_STRIP | EV_FCHECK | EV_EVAL, fargs[j], cargs, ncargs, (char **)NULL, 0);

    if (quick_regexp_match(pstr, mstr, cs)) {
      /* If there's a #$ in a switch's action-part, replace it with
       * the value of the conditional (mstr) before evaluating it.
       */

       if ( mudconf.switch_substitutions ) {
          tbuf1 = replace_tokens(fargs[j+1], NULL, NULL, mstr);
          tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                       tbuf1, cargs, ncargs, (char **)NULL, 0);
          free_lbuf(tbuf1);
       } else {
          tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                       fargs[j+1], cargs, ncargs, (char **)NULL, 0);
       }

       safe_str(tbuff, buff, bufcx);
       free_lbuf(tbuff);
       found = 1;
       if (mudstate.chkcpu_toggle || first) {
          free_lbuf(pstr);
          break;
       }
    }
    free_lbuf(pstr);
  }

  if (!(nfargs & 1) && !found) {
    /* Default case */
     if ( mudconf.switch_substitutions ) {
        tbuf1 = replace_tokens(fargs[nfargs - 1], NULL, NULL, mstr);
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     tbuf1, cargs, ncargs, (char **)NULL, 0);
        free_lbuf(tbuf1);
     } else {
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[nfargs - 1], cargs, ncargs, (char **)NULL, 0);
     }
    safe_str(tbuff, buff, bufcx);
    free_lbuf(tbuff);
  }
  free_lbuf(mstr);
}

FUNCTION(fun_reswitch)
{
   do_reswitch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 0);
}

FUNCTION(fun_reswitchi)
{
   do_reswitch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 1);
}

FUNCTION(fun_reswitchall)
{
   do_reswitch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 2);
}
 
FUNCTION(fun_reswitchalli)
{
   do_reswitch(buff, bufcx, player, cause, caller, fargs, nfargs, 
               cargs, ncargs, 3);
}

void
do_remultimatch(char *buff, char **bufcx, dbref player, dbref cause, dbref caller, 
                char *fargs[], int nfargs, char *cargs[], int ncargs, int key, int i_count, 
                char *name, int i_onlyone)
{
    int wcount, pcount, gotone = 0;
    char *r, *s, sep, *working, *tbuf;

    if (!fn_range_check(name, nfargs, 2, 3, buff, bufcx))
        return;

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }

    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];
    else
       sep = ' ';


    pcount = 0;
    working = alloc_lbuf("remultimatch");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    tbuf = alloc_mbuf("remultimatch");
    wcount = 1;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_regexp_match(fargs[1], r, (key ? 0 : 1)) ) {
              if ( !i_count ) {
                 sprintf(tbuf, "%d", wcount);
                 if (gotone)
                   safe_str(" ", buff, bufcx);
                 safe_str(tbuf, buff, bufcx);
              }
              gotone++;
              wcount++;
              break;
           }
           wcount++;
       } while (s);
       if ( gotone && i_onlyone )
          break;
       if (!s)
           break;
       if ( !i_count )
          strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    free_mbuf(tbuf);
    if ( i_count )
       ival(buff, bufcx, gotone);
    else if (!i_count && !gotone)
       safe_str("0", buff, bufcx);
}

FUNCTION(fun_reglmatch)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 0, 0, (char *)"REGLMATCHALL", 1);
}

FUNCTION(fun_reglmatchi)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 1, 0, (char *)"REGLMATCHALLI", 1);
}

FUNCTION(fun_reglmatchall)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 0, 0, (char *)"REGLMATCHALL", 0);
}

FUNCTION(fun_reglmatchalli)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 1, 0, (char *)"REGLMATCHALLI", 0);
}

FUNCTION(fun_regnummatch)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 0, 1, (char *)"RENUMMATCH", 0);
}

FUNCTION(fun_regnummatchi)
{
   do_remultimatch(buff, bufcx, player, cause, caller, fargs, nfargs, 
                   cargs, ncargs, 1, 1, (char *)"RENUMMATCHI", 0);
}


FUN flist_regexp[] =
{
    {"CLUSTER_REGREP", fun_cluster_regrep, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_REGREPI", fun_cluster_regrepi, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGMATCH", fun_regmatch, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGMATCHI", fun_regmatchi, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGLMATCH", fun_reglmatch, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGLMATCHI", fun_reglmatchi, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGLMATCHALL", fun_reglmatchall, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGLMATCHALLI", fun_reglmatchalli, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGNUMMATCH", fun_regnummatch, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGNUMMATCHI", fun_regnummatchi, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGEDIT", fun_regedit, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITI", fun_regediti, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITALL", fun_regeditall, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITALLI", fun_regeditalli, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITLIT", fun_regeditlit, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITILIT", fun_regeditilit, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITALLLIT", fun_regeditalllit, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGEDITALLILIT", fun_regeditallilit, 3, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, 0},
    {"REGRAB", fun_regrab, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGRABI", fun_regrabi, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGRABALL", fun_regraball, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGRABALLI", fun_regraballi, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REGREP", fun_regrep, 3, FN_VARARGS, CA_PUBLIC, 0},
    {"REGREPI", fun_regrepi, 3, FN_VARARGS, CA_PUBLIC, 0},
    {"RESWITCH", fun_reswitch, 0, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"RESWITCHI", fun_reswitchi, 0, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"RESWITCHALL", fun_reswitchall, 0, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"RESWITCHALLI", fun_reswitchalli, 0, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {NULL, NULL, 0, 0, 0, 0}
};

void
load_regexp_functions()
{
    FUN *fp;
    char *buff, *cp, *dp;

    buff = alloc_sbuf("init_functab");
    for (fp = flist_regexp; fp->name; fp++) {
      cp = (char *) fp->name;
      dp = buff;
      while (*cp) {
        *dp = ToLower((int)*cp);
        cp++;
        dp++;
      }
      *dp = '\0';
      hashadd2(buff, (int *) fp, &mudstate.func_htab, 1);
    }
    free_sbuf(buff);
}

/* ----------------------------------------------------------------------
 * regexp_match: Load a regular expression match and insert it into
 * registers.
 */
/* extern int      FDECL(regexp_wild_match, (char *, char *, char *[], int, int)); */


int
regexp_wild_match(char *pattern, char *str, char *args[], int nargs, int case_opt)
{
    int matches, i, len, erroffset, offsets[99];
    const char *errptr;
    pcre *re;

    /*
     * Load the regexp pattern. This allocates memory which must be
     * later freed. A free() of the regexp does free all structures
     * under it.
     */

    if ( (re = pcre_compile(pattern, case_opt, &errptr, &erroffset, tables)) == NULL) {
        /*
         * This is a matching error. We have an error message in
         * regexp_errbuf that we can ignore, since we're doing
         * command-matching.
         */
        return 0;
    }

    /*
     * Now we try to match the pattern. The relevant fields will
     * automatically be filled in by this.
     */

    matches = pcre_exec(re, NULL, str, strlen(str), 0, 0, offsets, 99);
    if (matches <= 0) {
        free(re);
        return 0;
    }

    /*
     * Now we fill in our args vector. Note that in regexp matching,
     * 0 is the entire string matched, and the parenthesized strings
     * go from 1 to 9. We DO PRESERVE THIS PARADIGM, for consistency
     * with other languages.
     */

    if ( matches > nargs )
       matches = nargs;

    for (i = 0; i < nargs; i++) {
        args[i] = NULL;
    }

    /* Convenient: nargs and NSUBEXP are the same.
     * We are also guaranteed that our buffer is going to be LBUF_SIZE
     * so we can copy without fear.
     */

    for (i = 0; i < matches; ++i) {
        if (offsets[i*2] == -1) {
            continue;
        }
        len = offsets[(i*2)+1] - offsets[i*2];
        if ( len > (LBUF_SIZE - 1) )
           len = LBUF_SIZE - 1;
        args[i] = alloc_lbuf("regexp_match");
        strncpy(args[i], str + offsets[i*2], len);
        args[i][len] = '\0';        /* strncpy() does not null-terminate */
    }

    free(re);
    return 1;
}


