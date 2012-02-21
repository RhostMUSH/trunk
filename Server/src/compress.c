/* compress.c */

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "mudconf.h"
#include "externs.h"
#include "attrs.h"
#include "alloc.h"

/* Compression routines */

/* These use a pathetically simple encoding that takes advantage of the */
/* eighth bit on a char; if you are using an international character set, */
/* they may need substantial patching. */

#ifndef STANDALONE

#define TOKEN_BIT 0x80		/* if on, it's a token */
#define TOKEN_MASK 0x7f		/* for stripping out token value */
#define NUM_TOKENS (128)
#define MAX_CHAR (128)

/* Top 128 bigrams in the CMU TinyMUD database as of 2/13/90 */
static const char *tokens[NUM_TOKENS] =
{
    "e ", " t", "th", "he", "s ", " a", "ou", "in",
    "t ", " s", "er", "d ", "re", "an", "n ", " i",
    " o", "es", "st", "to", "or", "nd", "o ", "ar",
    "r ", ", ", "on", " b", "ea", "it", "u ", " w",
    "ng", "le", "is", "te", "en", "at", " c", "y ",
    "ro", " f", "oo", "al", ". ", "a ", " d", "ut",
    " h", "se", "nt", "ll", "g ", "yo", " l", " y",
    " p", "ve", "f ", "as", "om", "of", "ha", "ed",
    "h ", "hi", " r", "lo", "Yo", " m", "ne", "l ",
    "li", "de", "el", "ta", "wa", "ri", "ee", "ti",
    "no", "do", "Th", " e", "ck", "ur", "ow", "la",
    "ac", "et", "me", "il", " g", "ra", "co", "ch",
    "ma", "un", "so", "rt", "ai", "ce", "ic", "be",
    " n", "k ", "ge", "ot", "si", "pe", "tr", "wi",
    "e.", "ca", "rs", "ly", "ad", "we", "bo", "ho",
    "ir", "fo", "ke", "us", "m ", " T", "di", ".."};


typedef char TOKTAB[MAX_CHAR][MAX_CHAR];

static TOKTAB *token_table = NULL;
static int table_initialized = 0;

static char *compress_buf = NULL;

static void 
NDECL(init_compress)
{
    if (mudconf.intern_comp) {
	int i;
	int j;

	if (!compress_buf)
	    compress_buf = alloc_lbuf("init_compress");
	if (!token_table)
	    token_table = (TOKTAB *) malloc(sizeof(TOKTAB));
	for (i = 0; i < MAX_CHAR; i++) {
	    for (j = 0; j < MAX_CHAR; j++) {
		*token_table[i][j] = 0;
	    }
	}

	for (i = 0; i < NUM_TOKENS; i++) {
	  /* Lensy: This looks supremly dodgy to me, but I'll go with it for now */
	    *token_table[(int) tokens[i][0]] [(int) tokens[i][1]] = i | TOKEN_BIT;
	}

	table_initialized = 1;
    }
}
#endif

const char *
compress(const char *s, int atr)
{
#ifndef STANDALONE
    if (mudconf.intern_comp && (atr != A_LIST)) {
	char *to;
	char token;

	if (!table_initialized)
	    init_compress();

	/* tokenize the first characters */
	for (to = compress_buf; s[0] && s[1]; to++) {
	  /* Lensy: This looks supremly dodgy to me, but I'll go with it for now */
	    if ((token = *token_table[(int)s[0]][(int)s[1]])) {
		*to = token;
		s += 2;
	    } else {
		*to = s[0];
		s++;
	    }
	}

	/* copy the last character (if any) and null */
	while ((*to++ = *s++));

	return compress_buf;
    } else
	return s;
#else
    return s;
#endif
}

const char *
uncompress(const char *s, int atr)
{
#ifndef STANDALONE
    if (mudconf.intern_comp && (atr != A_LIST)) {
	char *to;
	const char *token;

	if (!table_initialized)
	    init_compress();
	for (to = compress_buf; *s; s++) {
	    if (*s & TOKEN_BIT) {
		token = tokens[*s & TOKEN_MASK];
		*to++ = *token++;
		*to++ = *token;
	    } else {
		*to++ = *s;
	    }
	}
	*to++ = *s;
	return compress_buf;
    } else
	return s;
#else
    return s;
#endif
}

char *
uncompress_str(char *dest, const char *s, int atr)
{
#ifndef STANDALONE
    if (mudconf.intern_comp && (atr != A_LIST)) {
	char *to;
	const char *token;

	for (to = dest; s && *s; s++) {
	    if (*s & TOKEN_BIT) {
		token = tokens[*s & TOKEN_MASK];
		*to++ = *token++;
		*to++ = *token;
	    } else {
		*to++ = *s;
	    }
	}
	*to++ = *s;
	return (dest);
    } else {
	strcpy(dest, s);
	return dest;
    }
#else
    strcpy(dest, s);
    return dest;
#endif
}
