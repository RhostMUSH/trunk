/* regular expression foo */
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif
#if __GNUC_PREREQ(2, 92)
#define RESTRICT __restrict
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define RESTRICT restrict
#else
#define RESTRICT
#endif

#define PCRE_EXEC 1

#define AF_REGEXP 0x20000000

extern const unsigned char *tables;
extern int      quick_regexp_match(const char *RESTRICT, const char *RESTRICT, int);
extern void	load_regexp_functions();
extern int      regexp_wild_match(char *, char *, char *[], int, int);
/* end of regular expression foo */

