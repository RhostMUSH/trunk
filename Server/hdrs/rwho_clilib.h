/* rwho_clilib.h - prototypes for functions in rwho_clilib.c */

extern int	FDECL(rwhocli_setup, (char *, int, char *, char *, char *));
extern int	NDECL(rwhocli_shutdown);
extern int	NDECL(rwhocli_pingalive);
extern int	FDECL(rwhocli_userlogin, (char *, char *, time_t));
extern int	FDECL(rwhocli_userlogout, (char *));
extern void	NDECL(rwho_update);

