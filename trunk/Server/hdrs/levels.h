#ifdef REALITY_LEVELS
#ifndef _M_LEVELS_H
#define _M_LEVELS_H

extern int	FDECL(isreal_chk, (dbref, dbref, int));

#define IsReal(R, T)		isreal_chk(R, T, 0)
#define IsRealArg(R, T, K)	isreal_chk(R, T, K)

extern RLEVEL   FDECL(RxLevel, (dbref));
extern RLEVEL   FDECL(TxLevel, (dbref));
extern char *   FDECL(rxlevel_description, (dbref, dbref, int, int));
extern char *   FDECL(txlevel_description, (dbref, dbref, int, int));
extern void	FDECL(decompile_rlevels, (dbref, dbref, char *));
extern void 	FDECL(did_it_rlevel, (dbref, dbref, int, const char *, int,
				const char *, int, char *[], int));
extern void 	FDECL(notify_except_rlevel, (dbref, dbref, dbref,
				const char *));
extern void     FDECL(notify_except2_rlevel, (dbref, dbref, dbref, dbref,
				const char *));
extern void	FDECL(notify_except2_rlevel2, (dbref, dbref, dbref, dbref,
				const char *));
#endif
#endif
