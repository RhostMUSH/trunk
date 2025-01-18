/* match.h */
#include "copyright.h"

#ifndef M_MATCH_H
#define M_MATCH_H

#include "db.h"

typedef struct match_state MSTATE;
  struct match_state {
      dbref exact_match;
      int have_exact;
      int check_keys;
      dbref last_match;
      int match_count;
      dbref match_who;
      char *match_name;
      int preferred_type;
      int local_match;
  };

/* Match functions
 * Usage:
 *	init_match(player, name, type);
 *	match_this();
 *	match_that();
 *	...
 *	thing = match_result()
 */

extern void init_match(dbref, const char *, int);
extern void init_match_real(dbref, const char *, int, int);
extern void init_match_check_keys(dbref, const char *, int);
extern void match_player(void);
extern void match_player_absolute(void);
extern void match_absolute(void);
extern void match_numeric(void);
extern void match_me(void);
extern void match_here(void);
extern void match_home(void);
extern void match_possession(void);
extern void match_possession_altname(void);
extern void match_neighbor(void);
extern void match_altname(void);
extern void match_exit(void);
extern void match_exit_with_parents(void);
extern void match_carried_exit(void);
extern void match_carried_exit_with_parents(void);
extern void match_master_exit(void);
extern void match_everything(int);
extern dbref match_result(void);
extern dbref last_match_result(void);
extern dbref match_status(dbref, dbref);
extern dbref noisy_match_result(void);
extern dbref dispatched_match_result(dbref);
extern int matched_locally(void);
extern void save_match_state(MSTATE *);
extern void restore_match_state(MSTATE *);

#define NOMATCH_MESSAGE "I don't see that here."
#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"
#define NOPERM_MESSAGE "Permission denied."

#define	MAT_NO_EXITS		1	/* Don't check for exits */
#define	MAT_EXIT_PARENTS	2	/* Check for exits in parents */
#define	MAT_NUMERIC		4	/* Check for un-#ified dbrefs */
#define	MAT_HOME		8	/* Check for 'home' */

#endif
