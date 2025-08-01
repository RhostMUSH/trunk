#ifdef REALITY_LEVELS
#ifndef _M_LEVELS_H
#define _M_LEVELS_H

extern int	isreal_chk(dbref, dbref, int);

#define IsReal(R, T)		isreal_chk(R, T, 0)
#define IsRealArg(R, T, K)	isreal_chk(R, T, K)

extern RLEVEL   RxLevel(dbref);
extern RLEVEL   TxLevel(dbref);
extern char *   rxlevel_description(dbref, dbref, int, int);
extern char *   txlevel_description(dbref, dbref, int, int);
extern void	decompile_rlevels(dbref, dbref, char *, char *, int);
extern void 	did_it_rlevel(dbref, dbref, int, const char *, int, const char *, int, char *[], int);
extern void 	notify_except_rlevel(dbref, dbref, dbref, const char *, int);
extern void     notify_except2_rlevel(dbref, dbref, dbref, dbref, const char *);
extern void	notify_except2_rlevel2(dbref, dbref, dbref, dbref, const char *);
#endif
#endif
