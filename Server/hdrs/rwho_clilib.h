/* rwho_clilib.h - prototypes for functions in rwho_clilib.c */

extern int	rwhocli_setup(char *, int, char *, char *, char *);
extern int	rwhocli_shutdown(void);
extern int	rwhocli_pingalive(void);
extern int	rwhocli_userlogin(char *, char *, time_t);
extern int	rwhocli_userlogout(char *);
extern void	rwho_update(void);

