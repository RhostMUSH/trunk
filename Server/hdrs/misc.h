/* misc.h - miscellaneous structures that are needed in more than one file */

#include "copyright.h"

#ifndef _M_MISC_H
#define _M_MISC_H

#include "db.h"
#include "flags.h"

/* Search structure, used by @search and search(). */

typedef struct search_type SEARCH;
struct search_type {
	int	s_wizard;
	dbref	s_owner;
	dbref	s_rst_owner;
	int	s_rst_type;
	FLAGSET	s_fset;
	dbref	s_parent;
	char	*s_rst_name;
	char	*s_rst_eval;
	int	low_bound;
	int	high_bound;
        int	i_totems[TOTEM_SLOTS];
};

/* Stats structure, used by @stats and stats(). */

typedef struct stats_type STATS;
struct stats_type {
	int	s_total;
	int	s_rooms;
	int	s_exits;
	int	s_things;
	int	s_players;
	int	s_zone;
	int	s_garbage;
};

extern int	FDECL(search_setup, (dbref, char *, SEARCH *, int, int));
extern void	FDECL(search_perform, (dbref, dbref, SEARCH *,FILE **, int));
extern int	FDECL(get_stats, (dbref, dbref, STATS *));

#endif




