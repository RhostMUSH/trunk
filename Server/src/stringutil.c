/* stringutil.c -- string utilities */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "match.h"
#include "config.h"
#include "externs.h"
#include "alloc.h"
#include "rhost_ansi.h"


int safe_copy_buf(const char *src, int nLen, char *buff, char **bufc);
extern dbref    FDECL(match_thing, (dbref, char *));

/*
 * returns a pointer to the non-space character in s, or a NULL if s == NULL
 * or *s == NULL or s has only spaces.
 */
char *skip_space(const char *s)
{
char *cp;

	cp = (char *)s;
	while (cp && *cp && isspace((int)*cp))
		cp++;
	return (cp);
}

/*
 * returns a pointer to the next character in s matching c, or a pointer to
 * the \0 at the end of s.  Yes, this is a lot like index, but not exactly.
 */
char *seek_char(const char *s, char c)
{
char	*cp;

	cp = (char *)s;
	while (cp && *cp && (*cp != c))
		cp++;
	return (cp);
}

/* ---------------------------------------------------------------------------
 * munge_space: Compress multiple spaces to one space, also remove leading and
 * trailing spaces.
 */

char *munge_space(char *string)
{
char	*buffer, *p, *q;

	buffer = alloc_lbuf("munge_space");
	p = string;
	q = buffer;
	while (p && *p && isspace((int)*p))
		p++;		/* remove inital spaces */
	while (p && *p) {
		while (*p && !isspace((int)*p))
			*q++ = *p++;
		while (*p && isspace((int)*++p)) ;
			if (*p)
				*q++ = ' ';
	}
	*q = '\0';		/* remove terminal spaces and terminate
				 * string */
	return (buffer);
}

/* ---------------------------------------------------------------------------
 * trim_spaces: Remove leading and trailing spaces.
 */

char *trim_spaces(char *string)
{
char	*buffer, *p, *q;

	buffer = alloc_lbuf("trim_spaces");
	p = string;
	q = buffer;
	while (p && *p && isspace((int)*p))		/* remove inital spaces */
		p++;
	while (p && *p) {
		while (*p && !isspace((int)*p))	/* copy nonspace chars */
			*q++ = *p++;
		while (*p && isspace((int)*p))	/* compress spaces */
			p++;
		if (*p) *q++ = ' ';		/* leave one space */
	}
	*q = '\0';				/* terminate string */
	return (buffer);
}

/* ---------------------------------------------------------------------------
 * grabto: Return portion of a string up to the indicated character.  Also
 * returns a modified pointer to the string ready for another call.
 */

char *grabto (char **str, char targ)
{
char	*savec, *cp;

	if (!str || !*str || !**str)
		return NULL;

	savec = cp = *str;
	while (*cp && *cp != targ)
		cp++;
	if (*cp)
		*cp++ = '\0';
	*str = cp;
	return savec;
}

int string_compare(const char *s1, const char *s2)
{
#ifndef STANDALONE
  if(!mudconf.space_compress) {
    while (*s1 && *s2 && ToLower((int)*s1) == ToLower((int)*s2))
      s1++, s2++;

    return (ToLower((int)*s1) - ToLower((int)*s2));
  } else {
#endif
    while (isspace((int)*s1))
      s1++;
    while (isspace((int)*s2))
      s2++;
    while (*s1 && *s2 && ((ToLower((int)*s1) == ToLower((int)*s2)) ||
			  (isspace((int)*s1) && isspace((int)*s2)))) {
      if (isspace((int)*s1) && isspace((int)*s2)) {	/* skip all other spaces */
        while (isspace((int)*s1))
	  s1++;
        while (isspace((int)*s2))
	  s2++;
      } else {
        s1++;
        s2++;
      }
    }
    if ((*s1) && (*s2))
      return (1);
    if (isspace((int)*s1)) {
      while (isspace((int)*s1))
        s1++;
      return (*s1);
    }
    if (isspace((int)*s2)) {
      while (isspace((int)*s2))
        s2++;
      return (*s2);
    }
    if ((*s1) || (*s2))
      return (1);
    return (0);
#ifndef STANDALONE
  }
#endif
}

int string_prefix(const char *string, const char *prefix)
{
  int count=0;

  while (*string && *prefix && ToLower((int)*string) == ToLower((int)*prefix))
    string++, prefix++, count++;
  if (*prefix == '\0')  /* Matched all of prefix */
    return(count);
  else
    return(0);
}

/* accepts only nonempty matches starting at the beginning of a word */

const char *string_match(const char *src, const char *sub)
{
  if ((*sub != '\0') && (src)) {
    while (*src) {
      if (string_prefix(src, sub))
	return src;
      /* else scan to beginning of next word */
      while (*src && isalnum((int)*src))
	src++;
      while (*src && !isalnum((int)*src))
	src++;
    }
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 * replace_string: Returns an lbuf containing string STRING with all occurances
 * of OLD replaced by NEW. OLD and NEW may be different lengths.
 * (mitch 1 feb 91)
 */

char *replace_string(const char *old, const char *new, 
		const char *string, int i_value)
{
char	*result, *r, *s;
int	olen, i_once;

	if (string == NULL) return NULL;
	s=(char *)string;
	olen = strlen(old);
	r = result = alloc_lbuf("replace_string");
        i_once = 0;
	while (*s) {

		/* Copy up to the next occurrence of the first char of OLD */

		while (*s && *s!=*old) {
			safe_chr(*s, result, &r);
			s++;
		}

		/* If we are really at an OLD, append NEW to the result and
		 * bump the input string past the occurrence of OLD.
		 * Otherwise, copy the char and try again.
		 */
			
		if (*s) {
			if (!i_once && !strncmp(old, s, olen)) {
				safe_str((char *)new, result, &r);
				s += olen;
                                if ( i_value ) 
                                   i_once = 1;
			} else {
				safe_chr(*s, result, &r);
				s++;
			}
		}
	}
	*r = '\0';
	return result;
}

/* ---------------------------------------------------------------------------
 * replace_tokens: Performs ## and #@ substitution.
 */
char *
replace_tokens(const char *s, const char *pBound, const char *pListPlace, const char *pSwitch)
{
    char *result, *r, *p;
    int n;

    if (!s) {
        return NULL;
    }
    r = result = alloc_lbuf("replace_tokens");
    while (*s) {
        // Find next '#'.
        //
        p = strchr(s, '#');
        if (p) {
            // Copy up to the next occurrence of the first character.
            //
            n = p - s;
            if (n) {
                safe_copy_buf(s, n, result, &r);
                s += n;
            }

            if (  s[1] == '#' && pBound) {
                // BOUND_VAR
                //
                safe_str((char *)pBound, result, &r);
                s += 2;
            } else if (  s[1] == '@' && pListPlace) {
                // LISTPLACE_VAR
                //
                safe_str((char *)pListPlace, result, &r);
                s += 2;
            } else if (  s[1] == '$' && pSwitch) {
                // SWITCH_VAR
                //
                safe_str((char *)pSwitch, result, &r);
                s += 2;
            } else {
                safe_chr(*s, result, &r);
                s++;
            }
        } else {
            // Finish copying source string. No matches. No further
            // work to perform.
            //
            safe_str((char *)s, result, &r);
            break;
        }
    }
    return result;
}

/*
 * Returns string STRING with all occurances * of OLD replaced by NEW. OLD
 * and NEW may be different lengths. Modifies string, so: Note - STRING must
 * already be allocated large enough to handle the new size. (mitch 1 feb 91)
 */

char *replace_string_inplace(const char *old, const char *new, char *string)
{
char	*s;

	s = replace_string(old, new, string, 0);
	strcpy(string, s);
	free_lbuf(s);
	return string;
}

/* Counts occurances of C in STR. - mnp 7 feb 91 */

int count_chars(const char *str, const char c)
{
  register int out = 0;
  register const char *p = str;
  if (p)
    while (*p != '\0')
      if (*p++ == c)
	out++;
  return out;
}

int count_extended(char *str) {
   char *s;
   int i_val;

   s = str;
   i_val = 0;
#ifdef ZENTY_ANSI
   while ( *s ) {
      if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
           isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
           (*(s+5) == '>') ) {
         i_val += 5;
         s+=5;
      }
      s++;
   }
#endif
   return i_val;
}

/*
 * Returns an allocated, null-terminated array of strings, broken on SEP. The
 * array returned points into the original, >> modified << string. - mnp 7
 * feb 91
 */

/*
char **string2list(char *str, const char sep)
{
  int count = 0;
  char **out = NULL;
  char *end, *beg = str;
  if (str) {
    if (!(out = (char **) XMALLOC(sizeof(char *)*strlen(str),"string2list"))) {
      log_perror("ALC", "FAIL", NULL, "NO MEM in string2list()");
      return NULL;
    }
    for (;;) {
      while (*beg == sep)
	beg++;
      if (*beg == '\0')
	break;
      out[count++] = beg;
      for (end = beg; *end != '\0' && *end != sep; end++) ;
      if (*end == '\0')
	break;
      *end++ = '\0';
      beg = end;
    }
    out[count] = NULL;
  }
  if (out)
    out = (char **) realloc((char *) out, sizeof(char *) * count + 1);
  return out;
}
*/

/* returns the number of identical characters in the two strings */
int prefix_match(const char *s1, const char *s2)
{
  int count=0;

  while (*s1 && *s2 && (ToLower((int)*s1) == ToLower((int)*s2)))
    s1++, s2++, count++;
  /* If the whole string matched, count the null.  (Yes really.) */
  if (!*s1 && !*s2)
    count++;
  return count;
}

int minmatch (char *str, char *target, int min)
{
  while (*str && *target && (ToLower((int)*str) == ToLower((int)*target))) {
    str++;
    target++;
    min--;
  }
  if (*str) return 0;
  if (!*target) return 1;
  return ((min <= 0) ? 1 : 0);
}

char *strsave(const char *s)
{
  char *p;
  p = (char *)XMALLOC(sizeof(char) * (strlen(s)+1), "strsave");

  if(p)
    strcpy(p,s);
  return p;
}

/* ---------------------------------------------------------------------------
 * safe_copy_str, safe_copy_chr - Copy buffers, watching for overflows.
 */

int safe_copy_buf(const char *src, int nLen, char *buff, char **bufc)
{
    int left = LBUF_SIZE - (*bufc - buff) - 1;
    if (left < nLen)
    {
        nLen = left;
    }
    memcpy(*bufc, src, nLen);
    *bufc += nLen;
    return nLen;
}

int safe_copy_str(char *src, char *buff, char **bufp, int max)
{
char	*tp;

	tp = *bufp;
	if (src == NULL) return 0;
	while (*src && ((tp - buff) < max))
		*tp++ = *src++;
	*tp = '\0';
/*	*(buff + max) = '\0'; */
	*bufp = tp;
	return strlen(src);
}

int safe_copy_strmax(char *src, char *buff, char **bufp, int max)
{
char	*tp;
int	left;
	tp = *bufp;
	if (src == NULL) return 0;
        left = (buff + max) - tp - strlen(src);
	if ( left < 1 )
           return 0;
	while (*src && ((tp - buff) < max))
		*tp++ = *src++;
	*tp = '\0';
	*bufp = tp;
	return strlen(src);
}

int safe_copy_chr(char src, char *buff, char **bufp, int max)
{
char	*tp;
int	retval;

	tp = *bufp;
	retval = 0;
	if ((tp - buff) < max) {
		*tp++ = src;
	} else {
		retval = 1;
	}
	*bufp = tp;
	*tp = '\0';
/*	*(buff + max) = '\0'; */
	return retval;
}

int matches_exit_from_list (char *str, char *pattern)
{
char	*s;

	while (*pattern) {
		for (s=str;	/* check out this one */
		     (*s && (ToLower((int)*s) == ToLower((int)*pattern)) &&
		      *pattern && (*pattern != EXIT_DELIMITER));
		     s++,pattern++) ;

		/* Did we match it all? */

		if (*s == '\0') {

			/* Make sure nothing afterwards */

			while (*pattern && isspace((int)*pattern))
				pattern++;

			/* Did we get it? */

			if (!*pattern || (*pattern == EXIT_DELIMITER))
				return 1;		  
		}

		/* We didn't get it, find next string to test */

		while (*pattern && *pattern++ != EXIT_DELIMITER) ;
		while (isspace((int)*pattern))
			pattern++;
	}
	return 0;
}

char *myitoa(int n)
{
  static char itoabuf[40];
  int i, sign;

  if ((sign = n) < 0)
    n = -n;
  itoabuf[39] = '\0';
  i = 38;
  do {
    itoabuf[i--] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    itoabuf[i] = '-';
  else
    i++;
  return (itoabuf + i);
}

/*
 * Translate string unparsing results
 */
char 
*translate_string(const char *str, int type)
{
    char old[LBUF_SIZE+1];
    static char new0[LBUF_SIZE+1];
    char *j, *c, *bp;
    int i, i_keyval;

    memset(new0, 0, sizeof(new0));
    memset(old, 0, sizeof(old));
    bp = new0;
    strncpy(old, str, LBUF_SIZE);
    i_keyval = 0;
        
    for (j = old; *j != '\0'; j++) {
        switch (*j) {
        case ESC_CHAR:
            c = strchr(j, 'm');
            if (c) {
                if (!type) {
                    j = c;
                    break;
                }
                if ( i_keyval == 0 ) {
                   safe_str("[ansi(", new0, &bp);
                   i_keyval = 1;
                }
                *c = '\0';
                i = atoi(j + 2);
                if ( (i_keyval == 3) && (i != 0) ) {
                   safe_chr(',', new0, &bp);
                   i_keyval = 1;
                }
                if ( (i_keyval == 4) && (i != 0) ) {
                   safe_chr(',', new0, &bp);
                   i_keyval = 1;
                }
                switch (i) {
                case 0:
                    if ( i_keyval == 4 )
                       safe_str(",n", new0, &bp);
                    i_keyval = ((i_keyval == 4) ? 1 : 4);
                    break;
                case 1:
                    safe_chr('h', new0, &bp);
                    break;
                case 5:
                    safe_chr('f', new0, &bp);
                    break;
                case 7:
                    safe_chr('i', new0, &bp);
                    break;
                case 30:
                    safe_chr('x', new0, &bp);
                    break;
                case 31:
                    safe_chr('r', new0, &bp);
                    break;
                case 32:
                    safe_chr('g', new0, &bp);
                    break;
                case 33:
                    safe_chr('y', new0, &bp);
                    break;
                case 34:
                    safe_chr('b', new0, &bp);
                    break;
                case 35:
                    safe_chr('m', new0, &bp);
                    break;
                case 36:
                    safe_chr('c', new0, &bp);
                    break;
                case 37:
                    safe_chr('w', new0, &bp);
                    break;
                case 40:
                    safe_chr('X', new0, &bp);
                    break;
                case 41:
                    safe_chr('R', new0, &bp);
                    break;
                case 42:
                    safe_chr('G', new0, &bp);
                    break;
                case 43:
                    safe_chr('Y', new0, &bp);
                    break;
                case 44:
                    safe_chr('B', new0, &bp);
                    break;
                case 45:
                    safe_chr('M', new0, &bp);
                    break;
                case 46:
                    safe_chr('C', new0, &bp);
                    break;
                case 47:
                    safe_chr('W', new0, &bp);
                    break;
                }
                j = c;
            } else {
                safe_chr(*j, new0, &bp);
            }
            break;
        case ' ':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if ((*(j+1) == ' ') && type)
                safe_str("%b", new0, &bp);
            else 
                safe_chr(' ', new0, &bp);
            break;
        case '\\':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("\\", new0, &bp);
            else
                safe_chr('\\', new0, &bp);
            break;
        case '%':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%%", new0, &bp);
            else
                safe_chr('%', new0, &bp);
            break;
        case '[':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%[", new0, &bp);
            else
                safe_chr('[', new0, &bp);
            break;
        case ']':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%]", new0, &bp);
            else
                safe_chr(']', new0, &bp);
            break;
        case '{':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%{", new0, &bp);
            else
                safe_chr('{', new0, &bp);
            break;
        case '}':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%}", new0, &bp);
            else
                safe_chr('}', new0, &bp);
            break;
        case '(':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%(", new0, &bp);
            else
                safe_chr('(', new0, &bp);
            break;
        case ')':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%)", new0, &bp);
            else
                safe_chr(')', new0, &bp);
            break;
        case '\r':
            break;
        case '\n':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%r", new0, &bp);
            else
                safe_chr(' ', new0, &bp);
            break;
        case '\t':
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            if (type)
                safe_str("%t", new0, &bp);
            else
                safe_chr(' ', new0, &bp);
	    break;
        default:
            if ( i_keyval == 1 ) {
               safe_chr(',', new0, &bp);
               i_keyval = 3;
            }
            if ( i_keyval == 4 ) {
               safe_str(")]", new0, &bp);
               i_keyval = 0;
            }
            safe_chr(*j, new0, &bp);
        }
    }
    if ( i_keyval != 0 ) 
       safe_str(")]", new0, &bp);
    *bp = '\0';
    return new0;
}

#ifndef STANDALONE
char *
find_cluster(dbref thing, dbref player, int anum)
{
   char *s_instr, *s_instrptr, *s_text, *s_strtok, *s_strtokptr, 
        *s_useme, *s_shoveattr;
   dbref thing2, aowner;
   int aflags, i_value;
   ATTR *a_clust;
 
   s_instrptr = s_instr = alloc_lbuf("find_cluster");
   
   if ( !Good_chk(thing) || !Cluster(thing) ) {
      safe_str("#-1", s_instr, &s_instrptr);
      return(s_instr);
   }
   if ( anum > 0 ) {
      a_clust = atr_num(anum);
      if ( a_clust ) {
         anum = a_clust->number;
      } else {
         anum = -1;
      }
   } else {
      anum = -1;
   }
   a_clust = atr_str("_CLUSTER");
   if ( !a_clust ) {
      safe_str("#-1", s_instr, &s_instrptr);
      return(s_instr);
   }

   s_text = atr_get(thing, a_clust->number, &aowner, &aflags);
   if ( !*s_text ) {
      free_lbuf(s_text);
      safe_str("#-1", s_instr, &s_instrptr);
      return(s_instr);
   }

   if ( anum > 0 )
      s_shoveattr = alloc_lbuf("find_cluster_alloc");

   i_value = 2000000000;
   s_useme = NULL;

   s_strtok = strtok_r(s_text, " ", &s_strtokptr);
   while ( s_strtok ) {
      thing2 = match_thing(player, s_strtok);
      if ( Good_chk(thing2) && Cluster(thing2) ) {
         if ( i_value > db[thing2].nvattr ) {
            i_value = db[thing2].nvattr;
            s_useme = s_strtok;
         }
         if ( anum > 0 ) {
            atr_get_str(s_shoveattr, thing2, anum, &aowner, &aflags);
            if ( *s_shoveattr ) {
               s_useme = s_strtok;
               break;
            }
         }
      }
      s_strtok = strtok_r(NULL, " ", &s_strtokptr);
   }
   if ( anum > 0 )
      free_lbuf(s_shoveattr);
   if ( s_useme )
      safe_str(s_useme, s_instr, &s_instrptr);
   else
      safe_str("#-1", s_instr, &s_instrptr);

   free_lbuf(s_text);
   return(s_instr);
}

void
trigger_cluster_action(dbref thing, dbref player)
{
   char *s_tmpstr, *s_strtok, *s_strtokptr;
   int i_lowball, i_highball, aflags;
   dbref aowner, thing2;
   ATTR *attr;

   if ( !Good_chk(thing) || !Cluster(thing) )
      return;

   attr = atr_str("_CLUSTER");
   if ( !attr )
      return;

   s_tmpstr = atr_get(thing, attr->number, &aowner, &aflags);
   s_strtok = strtok_r(s_tmpstr, " ", &s_strtokptr);
   i_lowball = 2000000000;
   while ( s_strtok ) {
      thing2 = match_thing(player, s_strtok);
      if ( Good_chk(thing2) && isThing(thing2) && Cluster(thing2) ) {
         if ( (db[thing2].nvattr - 1) < i_lowball ) {
            i_lowball = (db[thing2].nvattr - 1);
         }
      }
      s_strtok = strtok_r(NULL, " ", &s_strtokptr);
   }
   free_lbuf(s_tmpstr);
   attr = atr_str("_CLUSTER_THRESH");
   i_highball = 0;
   if ( attr ) {
      s_tmpstr = atr_get(thing, attr->number, &aowner, &aflags);
      if ( *s_tmpstr ) {
         i_highball = atoi(s_tmpstr) - 1;
      }
      free_lbuf(s_tmpstr);
      if ( (i_highball > 0) && (i_highball < i_lowball) ) {
         attr = atr_str("_CLUSTER_ACTION_FUNC");
         if ( !attr ) {
            attr = atr_str("_CLUSTER_ACTION");
            if ( attr && (mudstate.clust_time + mudconf.cluster_cap) < mudstate.now ) {
               did_it(thing, thing, 0, NULL, 0, NULL, attr->number, (char **) NULL, 0);
               mudstate.clust_time = mudstate.now;
            }
         } else if ( (mudstate.clust_time + mudconf.clusterfunc_cap) < mudstate.now ) {
            s_strtok = atr_get(thing, attr->number, &aowner, &aflags);
            if ( s_strtok && *s_strtok ) {
               s_tmpstr = exec(thing, thing, thing, EV_STRIP | EV_FCHECK | EV_EVAL, 
                               s_strtok, (char **)NULL, 0);
               free_lbuf(s_tmpstr);
               mudstate.clust_time = mudstate.now;
            }
            free_lbuf(s_strtok);
         }
      }
   }
}
#endif
