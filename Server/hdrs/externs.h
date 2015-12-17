/* externs.h - Prototypes for externs not defined elsewhere */

#include "copyright.h"

#define MYSQL_VER "Version 1.1 Beta"

#ifndef _M__EXTERNS__H
#define	_M__EXTERNS__H

#include "db.h"
#include "mudconf.h"
#include "pcre.h"
#include "tprintf.h"

#ifndef _DB_C
#define INLINE
#else /* _DB_C */
  #ifdef __GNUC__
  #define INLINE inline
  #else /* __GNUC__ */
  #define INLINE
  #endif /* __GNUC__ */
#endif /* _DB_C */

#define	ToLower(s) (isupper(s) ? tolower(s) : (s))
#define	ToUpper(s) (islower(s) ? toupper(s) : (s))
#define DOING_SIZE 32	/* @doing and @doing/header size */
#define isValidAttrStartChar(c) (isalpha((int)c) || (c == '_') || (c == '~') || (c == '#') || (c == '.') || (c == '+'))

#define atrpCit(s) (s == 1)
#define atrpGuild(s) (s == 2)
#define atrpArch(s) (s == 3)
#define atrpCounc(s) (s == 4)
#define atrpWiz(s) (s == 5)
#define atrvWiz(s) (s <= 5)
#define atrpImm(s) (s == 6)
#define atrpGod(s) (s == 7)
#define atrpPreReg(s) (s == 0)


#define SPLIT_NORMAL		0x00
#define SPLIT_HILITE		0x01
#define SPLIT_FLASH		0x02
#define SPLIT_UNDERSCORE	0x04
#define SPLIT_INVERSE		0x08
#define SPLIT_NOANSI		0x10

typedef struct ansisplit {
	char	s_fghex[5];	/* Hex representation - foreground */
	char	s_bghex[5];	/* Hex representation - background */
	char	c_fgansi;	/* Normal foreground ansi */
	char	c_bgansi;	/* Normal background ansi */
	int	i_special;	/* Special ansi characters */
	char	c_accent;	/* Various accent characters */
        int	i_ascii8;	/* ASCII-8 encoding */
} ANSISPLIT;

typedef struct atrp {
        char   *name;       /* function name */
        int     flag_set;       /* who can set/clear attrib */
        int     flag_see;       /* who can see attrib */
        dbref	owner;		/* Owner of who set it (if not global which is -1) */
        dbref   controller;	/* Who controlls the attribute setting */
        dbref   target;		/* The actual target object */
	dbref	enactor;	/* The actual enactor of the command itself */
        struct atrp *next;      /* Next ufun in chain */
} ATRP;

extern long	FDECL(count_player,(dbref, int));
/* From conf.c */
extern int	FDECL(cf_modify_bits, (int *, char *, long, long, dbref, char *));
extern int	FDECL(cf_modify_multibits, (int *, int *, char *, long, long, dbref, char *));
extern void	FDECL(list_options_boolean, (dbref, int, char *));
extern void	FDECL(list_options_values, (dbref, int, char *));
extern void	FDECL(cf_display, (dbref, char *, int, char *, char **));
extern void     FDECL(sideEffectMaskToString, (int, char *, char **));
/* From udb_achunk.c */
extern int	NDECL(dddb_close);
extern int	FDECL(dddb_setfile, (char *));
extern int	NDECL(dddb_init);

extern void	NDECL(cache_var_init);
extern void	NDECL(dddb_var_init);

/* From bsd.c */
extern void	FDECL(reset_signals, ());
extern void	FDECL(ignore_signals, ());

/* From netcommon.c */
extern void	FDECL(make_ulist, (dbref, char *, char **, int, dbref));
extern int	FDECL(fetch_idle, (dbref));
extern int	FDECL(fetch_connect, (dbref));
extern void	FDECL(broadcast_monitor, (dbref,int,char*,char*,char*,int,int,int,char *));
extern void	VDECL(raw_broadcast, (dbref,int, char *, ...));
extern void	VDECL(raw_broadcast2, (int, char *, ...));
extern char *	FDECL(strip_ansi, (const char *));
#ifdef ZENTY_ANSI
extern char *   FDECL(strip_safe_ansi, (const char *));
extern char *   FDECL(strip_safe_accents, (const char *));
extern char *   FDECL(strip_all_ansi, (const char *));
extern char *   FDECL(strip_all_special, (const char *));
#else
#define strip_safe_ansi(x) (x)
#define strip_safe_accents(x) (x)
#define strip_all_ansi(x) strip_ansi(x)
#define strip_all_special(x) strip_ansi(x)
#endif
extern char *	FDECL(strip_nonprint, (const char *));
extern char *	FDECL(strip_returntab, (const char *, int));

extern int	NDECL(areg_init);
extern void	NDECL(areg_close);
extern int	FDECL(areg_check, (char *, int));
extern void	FDECL(areg_add, (char *, dbref));
extern int	FDECL(areg_del_player, (dbref));

/* From cque.c */
extern void     FDECL(show_que_func, (dbref, char *, int, char, char *, char *[], char));

extern int	FDECL(nfy_que, (dbref, int, int, int));
extern int	FDECL(halt_que, (dbref, dbref));
extern int	FDECL(halt_que_pid, (dbref, int, int));
extern int	FDECL(halt_que_all, (void));
extern void	FDECL(wait_que, (dbref, dbref, double, dbref, char *, char *[],
			int, char *[], char *[]));
extern double	NDECL(que_next);
extern int	FDECL(do_top, (int ncmds));
extern void	NDECL(recover_queue_deposits);

/* From eval.c */
extern void	NDECL(tcache_init);
extern char *	FDECL(parse_to, (char **, char, int));
extern char *	FDECL(parse_arglist, (dbref, dbref, dbref, char *, char, int,
			char *[], int, char*[], int, int, char *[], int));
extern int	FDECL(get_gender, (dbref));
#ifdef ZENTY_ANSI
extern void     FDECL(parse_ansi, (char *, char *, char **, char *, char **));
extern int      FDECL(parse_comments, (char *, char *, char **));
#endif
extern char *	FDECL(exec, (dbref, dbref, dbref, int, char *, char *[], int, char *[], int));

/* From flags.c */
extern int      FDECL(DePriv, (dbref, dbref, int, int, int));

/* From game.c */
#define	notify(p,m)			notify_check(p,p,m,0, \
                        MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#ifdef ZENTY_ANSI
#define noansi_notify(p,m)              notify_check(p,p,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_NO_ANSI, 0)
#define	noansi_notify_all_from_inside(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_NO_ANSI, 0)
#define	noansi_notify_all_from_inside_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE|MSG_NO_ANSI, x)
#define	noansi_notify_with_cause(p,c,m)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_NO_ANSI, 0)
#define	noansi_notify_with_cause_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_NO_ANSI, x)
#define	noansi_notify_with_cause2(p,c,m)	notify_check(NOTHING,c,m,p, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN|MSG_NO_ANSI, 0)
#else
#define	noansi_notify(p,m)			notify_check(p,p,m,0, \
                        MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#define	noansi_notify_all_from_inside(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE, 0)
#define	noansi_notify_all_from_inside_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE, x)
#define	noansi_notify_with_cause(p,c,m)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#define	noansi_notify_with_cause_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, x)
#define	noansi_notify_with_cause2(p,c,m)	notify_check(NOTHING,c,m,p, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#endif
#define	notify_quiet(p,m)		notify_check(p,p,m,0, \
						MSG_PUP_ALWAYS|MSG_ME, 0)
#define	notify_with_cause(p,c,m)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#define	notify_with_cause_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, x)
#define	notify_with_cause2(p,c,m)	notify_check(NOTHING,c,m,p, \
						MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN, 0)
#define	notify_quiet_with_cause(p,c,m)	notify_check(p,c,m,0, \
						MSG_PUP_ALWAYS|MSG_ME, 0)
#define	notify_puppet(p,c,m)		notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_F_DOWN, 0)
#define	notify_quiet_puppet(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME, 0)
#define	notify_all(p,c,m)		notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS|MSG_F_UP|MSG_F_CONTENTS, 0)
#define	notify_all_from_inside(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE, 0)
#define	notify_all_from_inside_real(p,c,m,x)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS_A|MSG_F_UP|MSG_F_CONTENTS|MSG_S_INSIDE, x)
#define	notify_all_from_outside(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME_ALL|MSG_NBR_EXITS|MSG_F_UP|MSG_F_CONTENTS|MSG_S_OUTSIDE, 0)
#define	notify_all_from_inside_quiet(p,c,m)	notify_check(p,c,m,0, \
						MSG_ME|MSG_F_CONTENTS|MSG_SILENT, 0)
#define noansi_notify_except(p,c,m,x)	notify_except(p,c,m,x,MSG_NO_ANSI)
extern void	FDECL(notify_except_str, (dbref, dbref, dbref [LBUF_SIZE/2], int, 
			const char *, int));
extern void	FDECL(notify_except, (dbref, dbref, dbref,
			const char *, int));
extern void	FDECL(notify_except2, (dbref, dbref, dbref, dbref,
			 const char *));
extern void	FDECL(notify_except3, (dbref, dbref, dbref, dbref, int,
			 const char *));
extern void	FDECL(notify_except_someone, (dbref, dbref, dbref,
			const char *, const char *));
extern int	FDECL(check_filter, (dbref, dbref, int,
			const char *));
extern void	FDECL(notify_check, (dbref, dbref, const char *, int, int, int));
extern int	FDECL(Hearer, (dbref));
extern void	NDECL(report);
extern void	FDECL(do_rwho, (dbref, dbref, int));
extern int	FDECL(atr_match, (dbref, dbref, char, char *, int, int));
extern int	FDECL(list_check, (dbref, dbref, char, char *, int, int, int));
	
/* From help.c */
extern int	FDECL(helpindex_read, (HASHTAB *, char *));
extern void	FDECL(helpindex_load, (dbref));
extern void	NDECL(helpindex_init);
extern char *	FDECL(errmsg, (dbref));
extern char *	FDECL(doorparm, (char *));

/* From htab.c */
extern int	FDECL(cf_ntab_access, (int *, char *, long, long, dbref, char *));

/* From log.c */
extern int	FDECL(start_log, (const char *, const char *));
extern void	NDECL(end_log);
extern void	FDECL(log_perror, (const char *, const char *,const char *,
			const char *));
extern void	FDECL(log_text, (char *));
extern void	FDECL(log_number, (int));
extern void	FDECL(log_name, (dbref));
extern void	FDECL(log_name_and_loc, (dbref));
extern char *	FDECL(OBJTYP, (dbref));
extern void	FDECL(log_type_and_name, (dbref));
extern void	FDECL(log_type_and_num, (dbref));

/* From look.c */
extern void	FDECL(look_in, (dbref,dbref, int));
extern long	FDECL(count_atrs, (dbref));
extern char *	FDECL(grep_internal, (dbref, dbref, char *, char *, int));
extern void     FDECL(viewzonelist, (dbref, dbref));
/* MMMail decs */
extern void	NDECL(mail_rem_dump);
extern int	NDECL(mail_init);
extern void	NDECL(mail_close);
extern void	FDECL(mail_mark, (dbref,int,char *,dbref,int));
extern void	FDECL(mail_md1, (dbref, dbref, int, int));
extern int	FDECL(stricmp, (char *, char*));
extern long	FDECL(mcount_size, (dbref, char *));
extern long	NDECL(mcount_size_tot);
extern void	FDECL(mail_gblout, (dbref));
extern char *   FDECL(mailquick, (dbref, char*));
extern char *   FDECL(mail_alias_function, (dbref, int, char *, char *));
extern char *   FDECL(mail_quick_function, (dbref, char *, int));
extern void     FDECL(mail_status, (dbref, char *, dbref, int, int, char *, char *));
extern void	FDECL(folder_plist_function, (dbref, char *, char *, char *));
extern void	FDECL(folder_current_function, (dbref, int, char *, char *, char *));


/* From match.c */
extern dbref    FDECL(match_controlled_quiet, (dbref, const char *));
/* From move.c */
extern void	FDECL(move_object, (dbref, dbref));
extern void	FDECL(move_via_generic, (dbref, dbref, dbref, int));
extern void	FDECL(move_via_exit, (dbref, dbref, dbref, dbref, int));
extern int	FDECL(move_via_teleport, (dbref, dbref, dbref, int, int));
extern void	FDECL(move_exit, (dbref, dbref, int, const char *, int));
extern void	FDECL(do_enter_internal, (dbref, dbref, int));

/* From news.c */
extern void NDECL(start_news_system);
extern void NDECL(shutdown_news_system);
extern void FDECL(news_player_nuke_hook, (dbref, dbref));

/* From object.c */
extern dbref	NDECL(start_home);
extern dbref	NDECL(default_home);
extern int	FDECL(can_set_home, (dbref, dbref, dbref));
extern dbref	FDECL(new_home, (dbref));
extern dbref	FDECL(clone_home, (dbref, dbref));
extern void	FDECL(divest_object, (dbref));
extern dbref	FDECL(create_obj, (dbref, int, char *, int));
extern void	FDECL(destroy_obj, (dbref, dbref, int));
extern void	FDECL(empty_obj, (dbref));

/* From player.c */
extern void	FDECL(record_login, (dbref, int, char *, char *,int *, int *, int *));
extern int	FDECL(check_pass, (dbref, const char *, int));
extern dbref	FDECL(connect_player, (char *, char *, char *));
extern dbref	FDECL(create_player, (char *, char *, dbref, int));
extern int	FDECL(add_player_name, (dbref, char *));
extern int	FDECL(delete_player_name, (dbref, char *));
extern dbref	FDECL(lookup_player, (dbref, char *, int));
extern void	NDECL(load_player_names);
extern void	FDECL(badname_add, (char *));
extern void	FDECL(badname_remove, (char *));
extern int	FDECL(badname_check, (char *, dbref));
extern void	FDECL(badname_list, (dbref, const char *));
extern int	FDECL(protectname_add, (char *, dbref));
extern dbref	FDECL(protectname_remove, (char *, dbref));
extern int	FDECL(protectname_check, (char *, dbref, int));
extern void	FDECL(protectname_list, (dbref, int, dbref));
extern dbref	FDECL(protectname_alias, (char *, dbref));
extern dbref	FDECL(protectname_unalias, (char *, dbref));
extern int	FDECL(reg_internal, (char *, char *, char *, int));

/* From predicates.c */
extern int	FDECL(isreal_chk, (dbref, dbref, int));
extern int      FDECL(Read_attr, (dbref, dbref, ATTR*, dbref, int, int));
extern int      FDECL(See_attr, (dbref, dbref, ATTR*, dbref, int, int));
extern int	FDECL(Controls, (dbref, dbref));
extern int	FDECL(Examinable, (dbref, dbref));
extern int	FDECL(Controlsforattr, (dbref, dbref, ATTR*, int));
extern dbref	FDECL(Location, (dbref));
extern dbref	FDECL(Location_safe, (dbref, int));
extern dbref	FDECL(absloc, (dbref));
extern int	FDECL(evlevchk, (dbref, int));
extern dbref	FDECL(insert_first, (dbref, dbref));
extern dbref	FDECL(remove_first, (dbref, dbref));
extern dbref	FDECL(reverse_list, (dbref));
extern int	FDECL(member, (dbref, dbref));
extern int	FDECL(is_integer, (char *));
extern int	FDECL(is_number, (char *));
extern int	FDECL(is_rhointeger, (char *));
extern int	FDECL(is_float, (char *));
extern int	FDECL(is_float2, (char *));
extern int	FDECL(could_doit, (dbref, dbref, int, int, int));
extern int	FDECL(can_see, (dbref, dbref, int));
extern int	FDECL(can_see2, (dbref, dbref, int));
extern void	FDECL(add_quota, (dbref, int, int));
extern int	FDECL(canpayfees, (dbref, dbref, int, int, int));
extern int	FDECL(giveto, (dbref,int,dbref));
extern int	FDECL(payfor, (dbref,int));
extern int	FDECL(payfor_give, (dbref,int));
extern int	FDECL(payfor_force, (dbref, int));
extern int	FDECL(pay_quota_force, (dbref, int, int));
extern int	FDECL(pay_quota, (dbref, dbref, int, int, int));
extern int	FDECL(q_check, (dbref, dbref, int, char, char, int, int));
extern int	FDECL(ok_name, (const char *));
extern int	FDECL(ok_player_name, (const char *));
extern int	FDECL(ok_attr_name, (const char *));
extern int	FDECL(ok_password, (const char *, dbref, int));
extern void	FDECL(handle_ears, (dbref, int, int));
extern dbref	FDECL(match_possessed, (dbref, dbref, char *, dbref, int));
extern void	FDECL(parse_range, (char **, dbref *, dbref *));
extern int	FDECL(parse_thing_slash, (dbref, char *, char **, dbref *));
extern int	FDECL(get_obj_and_lock, (dbref, char *, dbref *, ATTR **,
			char *, char**));
extern dbref	FDECL(where_is, (dbref));
extern dbref	FDECL(where_room, (dbref));
extern int	FDECL(locatable, (dbref, dbref, dbref));
extern int	FDECL(nearby, (dbref, dbref));
extern int	FDECL(exit_visible, (dbref, dbref, int));
extern int	FDECL(exit_displayable, (dbref, dbref, int));
extern void	FDECL(did_it, (dbref, dbref, int, const char *, int,
			const char *, int, char *[], int));
extern void	FDECL(list_bufstats, (dbref));
extern void	FDECL(list_buftrace, (dbref));

/* From set.c */
extern int	FDECL(parse_attrib, (dbref, char *, dbref *, int *));
extern int	FDECL(parse_attrib_zone, (dbref, char *, dbref *, int *));
extern int	FDECL(parse_attrib_wild, (dbref, char *, dbref *, int,
			int, int, OBLOCKMASTER *, int, int, int));
extern void	FDECL(edit_string, (char *, char **, char **, char *, char *, int, int, int, int));
extern dbref	FDECL(match_controlled, (dbref, const char *));
extern dbref	FDECL(match_controlled_or_twinked, (dbref, const char *));
extern dbref	FDECL(match_affected, (dbref, const char *));
extern dbref	FDECL(match_examinable, (dbref,const char *));

/* From stringutil.c */
extern char *	FDECL(munge_space, (char *));
extern char *	FDECL(trim_spaces, (char *));
extern char *	FDECL(grabto, (char **, char));
extern int	FDECL(string_compare, (const char *, const char *));
extern int	FDECL(string_prefix, (const char *, const char *));
extern const char *	FDECL(string_match, (const char * ,const char *));
extern char *	FDECL(dollar_to_space, (const char *));
extern char *	FDECL(replace_string, (const char *, const char *,
			const char *, int));
extern char *	FDECL(replace_string_ansi, (const char *, const char *,
			const char *, int, int));
extern char *	FDECL(replace_tokens, (const char *, const char *, const char *, const char *));
extern void     FDECL(split_ansi, (char *, char *, ANSISPLIT *));
extern char *   FDECL(rebuild_ansi, (char *, ANSISPLIT *));

extern char *	FDECL(replace_string_inplace, (const char *,  const char *,
			char *));
extern char *	FDECL(skip_space, (const char *));
extern char *	FDECL(seek_char, (const char *, char));
extern int	FDECL(prefix_match, (const char *, const char *));
extern int	FDECL(minmatch, (char *, char *, int));
extern char *	FDECL(strsave, (const char *));
extern int	FDECL(safe_copy_str, (char *, char *, char **, int));
extern int	FDECL(safe_copy_strmax, (char *, char *, char **, int));
extern int	FDECL(safe_copy_chr, (char, char *, char **, int));
extern int	FDECL(matches_exit_from_list, (char *, char *));
extern char *	FDECL(myitoa, (int));
extern char *   FDECL(translate_string, (const char *, int));
extern int FDECL(tboolchk,(char *));
extern char *	FDECL(find_cluster, (dbref, dbref, int));
extern void  	FDECL(trigger_cluster_action, (dbref, dbref));

/* From boolexp.c */
extern int	FDECL(eval_boolexp, (dbref, dbref, dbref, BOOLEXP *, int));
extern BOOLEXP *FDECL(parse_boolexp, (dbref,const char *, int));
extern int	FDECL(eval_boolexp_atr, (dbref, dbref, dbref, char *,int, int));

/* From functions.c */
#ifndef SINGLETHREAD
extern void 	FDECL(initialize_ansisplitter, (ANSISPLIT *, int));
extern void 	FDECL(clone_ansisplitter, (ANSISPLIT *, ANSISPLIT *));
extern void 	FDECL(clone_ansisplitter_two, (ANSISPLIT *, ANSISPLIT *, ANSISPLIT *));
extern void     FDECL(search_and_replace_ansi, (char *, ANSISPLIT *, ANSISPLIT *, ANSISPLIT *, int, int));

#else
#define		initialize_ansisplitter(x, y) (0)
#define		clone_ansisplitter(x, y) (0)
#define		clone_ansisplitter_two(x, y, z) (0)
#define		search_and_replace_ansi(w, x, y, z, a, b) (0)
#endif
extern int	FDECL(xlate, (char *));

/* From unparse.c */
extern char *	FDECL(unparse_boolexp, (dbref, BOOLEXP *));
extern char *	FDECL(unparse_boolexp_quiet, (dbref, BOOLEXP *));
extern char *	FDECL(unparse_boolexp_decompile, (dbref, BOOLEXP *));
extern char *	FDECL(unparse_boolexp_function, (dbref, BOOLEXP *));

/* From walkdb.c */
extern int	FDECL(chown_all, (dbref, dbref, int));

extern void	FDECL(olist_init,(OBLOCKMASTER *));
extern void     FDECL(olist_cleanup,(OBLOCKMASTER *));
extern void	FDECL(olist_add, (OBLOCKMASTER *,dbref));
extern dbref	FDECL(olist_first, (OBLOCKMASTER *));
extern dbref	FDECL(olist_next, (OBLOCKMASTER *));
extern void	FDECL(file_olist_init,(FILE **, const char *));
extern void     FDECL(file_olist_cleanup,(FILE **));
extern void	FDECL(file_olist_add, (FILE **,dbref));
extern dbref	FDECL(file_olist_first, (FILE **));
extern dbref	FDECL(file_olist_next, (FILE **));

/* From wild.c */
extern int	FDECL(wild, (char *, char *, char *[], int));
extern int	FDECL(wild_match, (char *, char *, char *[], int, int));
extern int	FDECL(quick_wild, (char *, char *));

/* From match.c */
extern dbref	FDECL(pref_match, (dbref, dbref, const char *));

/* From compress.c */
extern const char *	FDECL(uncompress, (const char *, int));
extern const char *	FDECL(compress, (const char *, int));
extern char *	FDECL(uncompress_str, (char *, const char *, int));

/* From command.c */
extern int	FDECL(check_access, (dbref, int, int, int));
extern int	FDECL(flagcheck, (char *, char*));
extern int      FDECL(cmdtest, (dbref, char *));
extern int      FDECL(zonecmdtest, (dbref, char *));


/* from db.c */
extern int	FDECL(Commer, (dbref));
extern void	FDECL(s_Pass, (dbref, const char *));
extern void     FDECL(s_MPass, (dbref, const char *));
extern void	FDECL(s_Name, (dbref, const char *));
extern char *	FDECL(Guild, (dbref));
extern char *	FDECL(Name, (dbref));
extern int	FDECL(fwdlist_load, (FWDLIST *, dbref, char *));
extern void	FDECL(fwdlist_set, (dbref, FWDLIST *));
extern void	FDECL(fwdlist_clr, (dbref));
extern int	FDECL(fwdlist_rewrite, (FWDLIST *, char *));
extern FWDLIST *FDECL(fwdlist_get, (dbref));
extern void	FDECL(clone_object, (dbref, dbref));
extern void	NDECL(init_min_db);
extern void	NDECL(atr_push);
extern void	NDECL(atr_pop);
extern int	FDECL(atr_head, (dbref, char **));
extern int	FDECL(atr_next, (char **));
extern int	FDECL(init_gdbm_db, (char *));
extern void	FDECL(atr_cpy, (dbref, dbref, dbref));
extern void	FDECL(atr_chown, (dbref));
extern void	FDECL(atr_clr, (dbref, int));
extern void	FDECL(atr_add_raw, (dbref, int, char *));
extern void	FDECL(atr_add, (dbref, int, char *, dbref, int));
extern void	FDECL(atr_set_owner, (dbref, int, dbref));
extern void	FDECL(atr_set_flags, (dbref, int, int));
extern char *	FDECL(atr_get_raw, (dbref, int));
extern char *	FDECL(atr_get_ash, (dbref, int, dbref *, int *, int, char *));
extern char *	FDECL(atr_pget_ash, (dbref, int, dbref *, int *, int, char *));
extern char *	FDECL(atr_get_str, (char *, dbref, int, dbref *, int *));
extern char *	FDECL(atr_pget_str, (char *, dbref, int, dbref *, int *, int *));
extern int	FDECL(atr_get_info, (dbref, int, dbref *, int *));
extern int	FDECL(atr_pget_info, (dbref, int, dbref *, int *));
extern void	FDECL(atr_free, (dbref));

/* from mushcrypt.c */
extern char *   FDECL(mush_crypt, (const char *));
extern int      FDECL(mush_crypt_validate, (dbref, const char *, const char *, int));

/* Powers keys */

/* IMPORTANT -- Keep this table in sync with the powers table in command.c
 * The constants are used as array indexes into the powers table.
 */

#define	POW_CHG_QUOTAS	0	/* May change quotas of controlled players */
#define	POW_CHOWN_ANY	1	/* Ignore loc/holding restrictions on @chown */
#define	POW_CHOWN_ME	2	/* May @chown things to me */
#define	POW_CHOWN_OTHER	3	/* May @chown things to others */
#define	POW_CHOWN_PLYR	4	/* May @chown players */
#define	POW_CONTROL_ALL	5	/* I control everything */
#define	POW_WIZARD_WHO	6	/* See extra WHO information */
#define	POW_EXAM_ALL	7	/* I can examine everything */
#define	POW_FIND_UNFIND	8	/* Can find unfindable players */
#define	POW_FREE_MONEY	9	/* I have infinite money */
#define	POW_FREE_QUOTA	10	/* I have infinite quota */
#define	POW_GRAB_PLAYER	11	/* Can @tel players to me */
#define	POW_IGNORE_RST	12	/* Ignore penalty flags in access checks */
#define	POW_JOIN_PLAYER	13	/* Can @tel to loc of player */
#define	POW_LONGFINGERS	14	/* Can get/whisper/etc from a distance */
#define	POW_NO_BOOT	15	/* Player is immune from @booting */
#define	POW_NO_TOAD	16	/* Player is immune from @toading/@destroying */
#define	POW_RENAME_ME	17	/* Player can change name or password */
#define	POW_SEE_AFLAGS	18	/* Player can see admin (hidden) flags */
#define	POW_SEE_QUEUE	19	/* Player can see the entire queue */
#define	POW_SEE_HIDDEN	20	/* Player can see hidden players on WHO list */
#define	POW_SEE_IATTRS	21	/* Player can see invisible attributes */
#define	POW_SEE_MFLAGS	22	/* Player can see game maintenance flags */
#define	POW_SEE_AATTRS	23	/* Player can set administration attributes */
#define	POW_SET_AFLAGS	24	/* Player can set admin flags */
#define	POW_SET_MATTRS	25	/* Player can set maintenance attributes */
#define	POW_SET_MFLAGS	26	/* Player can set game maintenance flags */
#define	POW_STAT_ANY	27	/* Can @stat anyone */
#define	POW_STEAL	28	/* Can give negative money */
#define	POW_TEL_ANYWHR	29	/* Ignore control/JUMP_OK on @tel */
#define	POW_TEL_UNRST	30	/* Ignore object type restrictions on @tel */
#define	POW_UNKILLABLE	31	/* Can't be killed */

/* Command handler keys */

#define AFLAGS_FULL	1
#define AFLAGS_PERM	2
#define AFLAGS_ADD	4
#define AFLAGS_MOD	8
#define AFLAGS_DEL	16
#define AFLAGS_SEARCH	32
#define AREG_LOAD	1
#define AREG_UNLOAD	2
#define AREG_LIST	4
#define AREG_ADD	8
#define AREG_ADD_FORCE	16
#define AREG_DEL_PLAY	32
#define AREG_DEL_EMAIL	64
#define AREG_LIMIT	128
#define AREG_WIPE	256
#define AREG_DEL_AEMAIL 512
#define AREG_CLEAN	1024
#define	ATTRIB_ACCESS	1	/* Change access to attribute */
#define	ATTRIB_RENAME	2	/* Rename attribute */
#define	ATTRIB_DELETE	4	/* Delete attribute */
#define	BOOT_QUIET	1	/* Inhibit boot message to victim */
#define	BOOT_PORT	2	/* Boot by port number */
#define	CHOWN_ONE	1	/* item = new_owner */
#define	CHOWN_ALL	2	/* old_owner = new_owner */
#define CHOWN_PRESERVE  4       /* wiz+ only. Don't set halt, keep flags */
#define CHOWN_ROOM	8
#define CHOWN_EXIT	16
#define CHOWN_PLAYER	32
#define CHOWN_THING	64
#define	CLONE_LOCATION	0	/* Create cloned object in my location */
#define	CLONE_INHERIT	1	/* Keep INHERIT bit if set */
#define	CLONE_PRESERVE	2	/* Preserve the owner of the object */
#define	CLONE_INVENTORY	4	/* Create cloned object in my inventory */
#define	CLONE_SET_COST	8	/* ARG2 is cost of cloned object */
#define	CLONE_SET_LOC	16	/* ARG2 is location of cloned object */
#define	CLONE_SET_NAME	32	/* ARG2 is alternate name of cloned object */
#define	CLONE_PARENT	64	/* Set parent on obj instd of cloning attrs */
#define CONV_ALTERNATE	1
#define CONV_ALL	2
#define CONV_OVER	4
#define CPATTR_CLEAR	1
#define CPATTR_VERB	2
#define	CRE_INVENTORY	0	/* Create object in my inventory */
#define	CRE_LOCATION	1	/* Create object in my location */
#define	CRE_SET_LOC	2	/* ARG2 is location of new object */
#define DECOMP_ALL	0	/* Decompile everything - default */
#define DECOMP_FLAGS    1	/* Decompile flags */
#define DECOMP_ATTRS	2	/* Decompile Attrs */
#define DECOMP_TREE	4	/* Decompile Penn Trees */
#define DECOMP_REGEXP	8	/* Decompile by Regexp */
#define DECOMP_TF	16	/* Stupid /tf compatibility to @decompile for PennMUSH */
#define	DBCK_DEFAULT	1	/* Get default tests too */
#define	DBCK_REPORT	2	/* Report info to invoker */
#define	DBCK_FULL	4	/* Do all tests */
#define	DBCK_FLOATING	8	/* Look for floating rooms */
#define	DBCK_PURGE	16	/* Purge the db of refs to going objects */
#define	DBCK_LINKS	32	/* Validate exit and object chains */
#define	DBCK_WEALTH	64	/* Validate object value/wealth */
#define	DBCK_OWNER	128	/* Do more extensive owner checking */
#define	DBCK_OWN_EXIT	256	/* Check exit owner owns src or dest */
#define	DBCK_WIZARD	512	/* Check for wizards/wiz objects */
#define	DBCK_TYPES	1024	/* Check for valid & appropriate types */
#define	DBCK_SPARE	2048	/* Make sure spare header fields are NOTHING */
#define	DBCK_HOMES	4096	/* Make sure homes and droptos are valid */
#define DBCLEAN_CHECK	1	/* Just run a test run on what dbclean would show */
#define	DEST_ONE	1	/* object */
#define	DEST_ALL	2	/* owner */
#define	DEST_OVERRIDE	4	/* override Safe() */
#define DEST_PURGE	8
#define	DIG_TELEPORT	1	/* teleport to room after @digging */
#define DOLIST_SPACE    0       /* expect spaces as delimiter */
#define DOLIST_DELIMIT  1       /* expect custom delimiter */
#define DOLIST_NOTIFY   2	/* queue a '@notify me' at end */
#define DOLIST_PID	4	/* Queue a '@notify/pid me=<pid>' at end */
#define DOLIST_CLEARREG 8	/* Clear local registers */
#define DOLIST_LOCALIZE 16	/* Localize the registers */
#define DOLIST_INLINE	32	/* @dolist inline and not queued */
#define DOLIST_NOBREAK  64	/* Only do breaking for the @dolist portion */
#define	DOING_MESSAGE	0	/* Set my DOING message */
#define	DOING_HEADER	1	/* Set the DOING header */
#define	DOING_POLL	2	/* List DOING header */
#define DOING_UNIQUE	4
#define DOOR_SW_LIST	1
#define DOOR_SW_OPEN	2
#define DOOR_SW_CLOSE	4
#define DOOR_SW_STATUS  8
#define DOOR_SW_FULL    16
#define DOOR_SW_PUSH	32
#define DOOR_SW_KICK	64
#define	DROP_QUIET	1	/* Don't do odrop/adrop if control */
#define	DUMP_STRUCT	1	/* Dump flat structure file */
#define	DUMP_TEXT	2	/* Dump text (gdbm) file */
#define	DUMP_FLAT	4	/* Dump db to flatfile   */
#define	EXAM_DEFAULT	0	/* Default */
#define	EXAM_BRIEF	1	/* Nonowner sees basic info (non-attribute) */
#define	EXAM_LONG	2	/* Nonowner sees public attrs too */
#define	EXAM_DEBUG	4	/* Display more info for finding db problems */
#define	EXAM_PARENT	8	/* Get attr from parent when exam obj/attr */
#define EXAM_QUICK      16      /* Nonowner sees just owner */
#define EXAM_TREE	32	/* Examine Tree like Penn */
#define EXAM_REGEXP	64	/* Examine by Regexp */
#define	FIXDB_OWNER	1	/* Fix OWNER field */
#define	FIXDB_LOC	2	/* Fix LOCATION field */
#define	FIXDB_CON	4	/* Fix CONTENTS field */
#define	FIXDB_EXITS	8	/* Fix EXITS field */
#define	FIXDB_NEXT	16	/* Fix NEXT field */
#define	FIXDB_PENNIES	32	/* Fix PENNIES field */
#define	FIXDB_ZONE	64	/* Fix ZONE field */
#define	FIXDB_LINK	128	/* Fix LINK field */
#define	FIXDB_PARENT	256	/* Fix PARENT field */
#define	FIXDB_DEL_PN	512	/* Remove player name from player name index */
#define	FIXDB_ADD_PN	1024	/* Add player name to player name index */
#define	FIXDB_NAME	2048	/* Set NAME attribute */
#define FIXDB_TYPE	4096	/* Fix type of object - DANGEROUS */
#define FLAGSW_REMOVE	1
#define FLAGDEF_SET     1       /* Set flag 'set' permissions */
#define FLAGDEF_UNSET   2	/* Set flag 'unset' permissions */
#define FLAGDEF_SEE     4	/* Set flag 'see' permissions */
#define FLAGDEF_LIST	8	/* List current flags and permissions (default) */
#define FLAGDEF_CHAR  	16	/* Redefine the character for the flag */
#define FLAGDEF_INDEX	32	/* Show the permission index allowed */
#define FLAGDEF_TYPE	64	/* Define type restrictions */
#define	FRC_PREFIX	0	/* #num command */
#define	FRC_COMMAND	1	/* what=command */
#define	GET_QUIET	1	/* Don't do osucc/asucc if control */
#define	GIVE_MONEY	1	/* Give money */
#define	GIVE_QUOTA	2	/* Give quota */
#define	GIVE_QUIET	64	/* Inhibit give messages */
#define	GLOB_ENABLE	1	/* key to enable */
#define	GLOB_DISABLE	2	/* key to disable */
#define	GLOB_LIST	3	/* key to list */
#define GREP_QUIET	1
#define GREP_REGEXP	2	/* regexp handler */
#define GREP_PARENT	4	/* grep parent */
#define	HALT_ALL	1	/* halt everything */
#define HALT_PID	2
#define HALT_PIDSTOP	4	/* stop pid processing */
#define HALT_PIDCONT	8	/* restore pid processing */
#define HALT_QUIET	16	/* Quiet mode on halting */
#define HIDE_ON         1       /* Hide from WHO */
#define HIDE_OFF        2       /* Unhide from WHO */
#define	HELP_HELP	1	/* get data from help file */
#define	HELP_NEWS	2	/* get data from news file */
#define	HELP_WIZHELP	3	/* get data from wizard help file */
#define HELP_PLUSHELP   4       /* get data from plus help file */
#define LIMIT_MAX	5	/* Max arguments in @limit variable */
#define LIMIT_LIST	1	/* Set global @limits */
#define LIMIT_VADD	2	/* VAttr limit */
#define LIMIT_DADD	4	/* @destroy limit */
#define LIMIT_RESET	8	/* Reset limits to global defaults */
#define LIMIT_LFUN	16	/* Maximum @lfunctions per player */
#define ICMD_DISABLE	0
#define ICMD_IGNORE	1
#define ICMD_ON		2
#define ICMD_OFF	4
#define ICMD_CLEAR	8
#define ICMD_CHECK	16
#define ICMD_DROOM	32
#define ICMD_IROOM	64
#define ICMD_CROOM	128
#define ICMD_LROOM      256
#define ICMD_LALLROOM   512
#define ICMD_EVAL       1024
#define ICMD_EROOM      2048
#define	KILL_KILL	1	/* gives victim insurance */
#define	KILL_SLAY	2	/* no insurance */
#define	LOOK_LOOK	1	/* list desc (and succ/fail if room) */
#define	LOOK_EXAM	2	/* full listing of object */
#define	LOOK_DEXAM	3	/* debug listing of object */
#define	LOOK_INVENTORY	4	/* list inventory of object */
#define	LOOK_SCORE	5	/* list score (# coins) */
#define	LOOK_OUTSIDE    8       /* look for object in container of player */
#define	MARK_SET	0	/* Set mark bits */
#define	MARK_CLEAR	1	/* Clear mark bits */
#define	MOTD_ALL	0	/* login message for all */
#define	MOTD_WIZ	1	/* login message for wizards */
#define	MOTD_DOWN	2	/* login message when logins disabled */
#define	MOTD_FULL	4	/* login message when too many players on */
#define	MOTD_LIST	8	/* Display current login messages */
#define	MOTD_BRIEF	16	/* Suppress motd file display for wizards */
#define	MOVE_QUIET	1	/* Dont do osucc/ofail/asucc/afail if ctrl */

#define NEWS_DEFAULT      0x00000000
#define NEWS_POST         0x00000001
#define NEWS_POSTLIST     0x00000002
#define NEWS_REPOST       0x00000004
#define NEWS_READ         0x00000008
#define NEWS_JUMP         0x00000010
#define NEWS_CHECK        0x00000020
#define NEWS_VERBOSE      0x00000040
#define NEWS_LOGIN        0x00000080
#define NEWS_YANK         0x00000100 
#define NEWS_EXTEND       0x00000200
#define NEWS_SUBSCRIBE    0x00000400
#define NEWS_UNSUBSCRIBE  0x00000800
#define NEWS_GROUPADD     0x00001000
#define NEWS_GROUPDEL     0x00002000
#define NEWS_GROUPINFO    0x00004000
#define NEWS_GROUPLIST    0x00008000
#define NEWS_GROUPMEM     0x00010000
#define NEWS_ADMINLOCK    0x00020000
#define NEWS_GROUPLIMIT   0x00040000
#define NEWS_USERLIMIT    0x00080000
#define NEWS_USERINFO     0x00100000
#define NEWS_READLOCK     0x00200000
#define NEWS_POSTLOCK     0x00400000
#define NEWS_EXPIRE       0x00800000
#define NEWS_ON           0x01000000
#define NEWS_OFF          0x02000000
#define NEWS_USERMEM      0x04000000
#define NEWS_STATUS       0x08000000
#define NEWS_MAILTO       0x10000000
#define NEWS_READALL      0x20000000
#define NEWS_ARTICLELIFE  0x40000000

#define HELP_SEARCH       0x00000040

#define NEWSDB_DEFAULT      0x00000000
#define NEWSDB_UNLOAD       0x00000001
#define NEWSDB_LOAD         0x00000002
#define NEWSDB_DBINFO       0x00000004
#define NEWSDB_DBCK         0x00000008
#define NUKE_PURGE	2	/* This is *NOT* 1 on purpose */

#define ZONE_ADD	0	/* add a zone master to an object's list */
#define ZONE_DELETE	1	/* delete a zone master from an object's list */
#define ZONE_PURGE	2	/* purge an object's zone list (zmo too) */
#define	NFY_NFY		0	/* Notify first waiting command */
#define	NFY_NFYALL	1	/* Notify all waiting commands */
#define	NFY_DRAIN	2	/* Delete waiting commands */
#define NFY_QUIET       4	/* Do not notify player if happening */
#define NFY_PID		8	/* Notify or Drain based on PID */
#define	OPEN_LOCATION	0	/* Open exit in my location */
#define	OPEN_INVENTORY	1	/* Open exit in me */
#define PAGE_LAST	1
#define PAGE_RET	2
#define PAGE_PORT	4
#define PAGE_RETMULTI	8	/* Respond but to multi players */
#define PAGE_LOC        16      /* (muxpage) page/loc notifies a plyr of your loc */
#define PAGE_NOEVAL     32      /* Don't evaluate text prior to sending */
#define PAGE_NOANSI	64	/* Don't ansifi the page */
#define	PASS_ANY	1	/* name=newpass */
#define	PASS_MINE	2	/* oldpass=newpass */
#define	PCRE_PLAYER	1	/* create new player */
#define PCRE_REG     	2	/* Register on @pcreate */
#define	PCRE_ROBOT	4	/* create robot player */
#define	PEMIT_PEMIT	1	/* emit to named player */
#define	PEMIT_OEMIT	2	/* emit to all in current room except named */
#define	PEMIT_WHISPER	4	/* whisper to player in current room */
#define	PEMIT_FSAY	8	/* force controlled obj to say */
#define	PEMIT_FEMIT	16	/* force controlled obj to emit */
#define	PEMIT_FPOSE	32	/* force controlled obj to pose */
#define	PEMIT_FPOSE_NS	64	/* force controlled obj to pose w/o space */
#define	PEMIT_CONTENTS	128	/* Send to contents (additive) */
#define	PEMIT_HERE	256	/* Send to location (@femit, additive) */
#define	PEMIT_ROOM	512	/* Send to containing rm (@femit, additive) */
#define PEMIT_LIST	1024
#define PEMIT_PORT	2048
#define PEMIT_WPORT	4096
#define PEMIT_NOISY	8192
#define PEMIT_NOEVAL    16384
#define PEMIT_ZONE      65536	/* SIDEEFFECT define is 32768 */
#define PEMIT_NOSUB	131072	/* Don't substitute '##' for @pemit/list */
#define PEMIT_NOANSI    262144  /* Don't print ANSI */
#define PEMIT_REALITY   524288  /* Follow Reality Level Permissions */
#define PEMIT_TOREALITY 1048576 /* Pemit to specic realities */
#define PEMIT_ONEEVAL	2097152 /* One eval for @pemit/list */
#define PEMIT_OSTR	4194304 /* @oemit uses multi-parameters */
#define PIPE_ON         1       /* Enable @pipe to attribute */
#define PIPE_OFF        2	/* Disable @pipe to attribute */
#define PIPE_TEE        4	/* Enable @pipe to attribute + normal output */
#define PIPE_STATUS	8	/* Status of piping */
#define	PS_BRIEF	0	/* Short PS report */
#define	PS_LONG		1	/* Long PS report */
#define	PS_SUMM		2	/* Queue counts only */
#define	PS_ALL		4	/* List entire queue */
#define PS_FREEZE	8	/* List frozen queue entries */
#define PURGE_ALL	1
#define PURGE_TIME	2
#define PURGE_TYPE	4
#define PURGE_OWNER	8
#define PROTECT_LIST	1
#define PROTECT_ADD	2
#define PROTECT_DEL	4
#define PROTECT_BYPLAYER	8
#define PROTECT_SUMMARY		16
#define PROTECT_ALIAS	32
#define PROTECT_UNALIAS	64
#define PROTECT_LISTALL 128
#define	QUEUE_KICK	1	/* Process commands from queue */
#define	QUEUE_WARP	2	/* Advance or set back wait queue clock */
#define QUEUE_KICK_PID	4
#define QUOTA_SET	0
#define QUOTA_MAX	1
#define QUOTA_FIX	2
#define QUOTA_TAKE	4
#define QUOTA_XFER	8
#define QUOTA_REDO	16
#define QUOTA_LOCK	32
#define QUOTA_UNLOCK	64
#define QUOTA_CLEAR	128
#define QUOTA_GEN	256
#define QUOTA_THING	512
#define QUOTA_PLAYER	1024
#define QUOTA_EXIT	2048
#define QUOTA_ROOM	4096
#define QUOTA_ALL	8192
#define REBOOT_PORT	1
#define REC_TYPE	1
#define REC_OWNER	2
#define REC_COUNT	4
#define REC_AGE		8
#define REC_DEST	16
#define REC_FREE	32
#define REBOOT_SILENT 0x00000001  /* @reboot silently */
#define	RWHO_START	1	/* Start transmitting to remote RWHO srvr */
#define	RWHO_STOP	2	/* Close connection to remote RWHO srvr */
#define	SAY_SAY		1	/* say in current room */
#define	SAY_NOSPACE	1	/* OR with xx_EMIT to get nospace form */
#define	SAY_POSE	2	/* pose in current room */
#define	SAY_POSE_NOSPC	3	/* pose w/o space in current room */
#define	SAY_PREFIX	4	/* first char indicates foratting */
#define	SAY_EMIT	5	/* emit in current room */
#define	SAY_SHOUT	8	/* shout to all logged-in players */
#define	SAY_WALLPOSE	9	/* Pose to all logged-in players */
#define	SAY_WALLEMIT	10	/* Emit to all logged-in players */
#define	SAY_WIZSHOUT	12	/* shout to all logged-in wizards */
#define	SAY_WIZPOSE	13	/* Pose to all logged-in wizards */
#define	SAY_WIZEMIT	14	/* Emit to all logged-in wizards */
#define	SAY_GRIPE	16	/* Complain to management */
#define	SAY_NOTE	17	/* Comment to log for wizards */
#define	SAY_NOTAG	32	/* Don't put Broadcast: in front (additive) */
#define	SAY_HERE	64	/* Output to current location */
#define	SAY_ROOM	128	/* Output to containing room */
#define SAY_NOEVAL      256     /* Don't parse message */
#define SAY_PNOEVAL     512     /* Don't parse pose message */
#define SAY_SUBSTITUTE 1024     /* Substitute ## for player dbref# */
#define SAY_NOANSI     2048	/* No Ansi on messages */
#define	SET_QUIET	1	/* Don't display 'Set.' message. */
#define SET_NOISY	2
#define SET_RSET	4	/* set() is really rset() */
#define SET_TREE	8	/* set() the entire trees */
#define SET_TREECHK	16	/* Verify we can set trees */
#define SET_MUFFLE	32	/* Totally  muffle set messages */
#define	SHUTDN_NORMAL	0	/* Normal shutdown */
#define	SHUTDN_PANIC	1	/* Write a panic dump file */
#define	SHUTDN_EXIT	2	/* Exit from shutdown code */
#define	SHUTDN_COREDUMP	4	/* Produce a coredump */
#define SIDEEFFECT	32768	/* Check if side-effect function */
#define SIDE_SET        1       /* Side-effect set() */
#define SIDE_CREATE     2       /* Side-effect create() */
#define SIDE_LINK	4	/* Side-effect link() */
#define SIDE_PEMIT	8	/* Side-effect pemit() */
#define SIDE_TEL	16	/* Side-effect tel() */
#define SIDE_LIST	32	/* Side-effect list() */
#define SIDE_DIG	64	/* Side-effect dig() */
#define SIDE_OPEN	128	/* Side-effect open() */
#define SIDE_EMIT	256	/* Side-effect emit() */
#define SIDE_OEMIT	512	/* Side-effect oemit() */
#define SIDE_CLONE	1024	/* Side-effect clone() */
#define SIDE_PARENT	2048	/* Side-effect parent() */
#define SIDE_LOCK	4096	/* Side-effect lock() */
#define SIDE_LEMIT      8192    /* Side-effect lemit() */
#define SIDE_REMIT      16384   /* Side-effect remit() */
#define SIDE_WIPE       32768   /* Side-effect wipe() */
#define SIDE_DESTROY    65536   /* Side-effect destroy() */
#define SIDE_ZEMIT      131072  /* Side-effect zemit() */
#define SIDE_NAME       262144  /* Side-effect name() */
#define SIDE_TOGGLE	524288	/* Side-effect toggle() */
#define SIDE_TXLEVEL    1048576 /* Side-effect txlevel() */
#define SIDE_RXLEVEL    2097152 /* Side-effect rxlevel() */
#define SIDE_RSET       4194304 /* Side-effect rset() */
#define SIDE_MOVE       8388608 /* Side-effect move() */       
#define SIDE_CLUSTER_ADD 16777216 /* Side-effect cluster_add() */
#define	SIDE_MAIL	33554432  /* mail send side effect */
#define	SNAPSHOT_NOOPT	0	/* No option specified */
#define SNAPSHOT_LIST	1	/* Show files in snapshot directory */
#define SNAPSHOT_UNLOAD	2	/* Unload a snapshot from the db */
#define SNAPSHOT_DEL	4	/* Delete a snapshot */
#define SNAPSHOT_LOAD	8	/* Load a snapshot into the db */
#define SNAPSHOT_VERIFY 16	/* Verify and sanity check snapshot file */
#define SITE_REG	1
#define SITE_FOR	2
#define SITE_SUS	4
#define SITE_NOG	8
#define SITE_NOAR	16
#define SITE_NOAUTH	32
#define SITE_NODNS	64
#define SITE_ALL	128
#define SITE_PER	256
#define SITE_TRU	512
#define SITE_LIST	1024	/* List @site/list information */
#define SKIP_IFELSE	1	/* @ifelse conversion for @skip */
#define SNOOP_ON	1	/* Start snooping */
#define SNOOP_OFF	2	/* Stop snooping */
#define SNOOP_STAT	4	/* show status */
#define SNOOP_LOG	8
#define	SRCH_SEARCH	1	/* Do a normal search */
#define	SRCH_MARK	2	/* Set mark bit for matches */
#define	SRCH_UNMARK	3	/* Clear mark bit for matches */
#define SEARCH_NOGARBAGE 256	/* Garbage collector */
#define	STAT_PLAYER	0	/* Display stats for one player or tot objs */
#define	STAT_ALL	1	/* Display global stats */
#define	STAT_ME		2	/* Display stats for me */
#define SELFBOOT_LIST   1	/* List all ports you have for selfboot */
#define SELFBOOT_PORT	2	/* boot the specified port for your self */
#define	SWITCH_DEFAULT	0	/* Use the configured default for switch */
#define	SWITCH_ANY	1	/* Execute all cases that match */
#define	SWITCH_ONE	2	/* Execute only first case that matches */
#define SWITCH_REGANY   4	/* Same as all but regexp */
#define SWITCH_REGONE	8	/* Same as one but regexp */
#define SWITCH_CASE	16	/* Case sensitive switch to well, @switch */
#define	SWITCH_NOTIFY	32	/* Do a @notify at the end */
#define SWITCH_INLINE	64	/* inline switches -- yayy */
#define SWITCH_LOCALIZE 128	/* Localize registers if inline */
#define SWITCH_CLEARREG 256	/* Clear registers if inline */
#define SWITCH_NOBREAK	512	/* Don't break out from local @break */
#define	SWEEP_ME	1	/* Check my inventory */
#define	SWEEP_HERE	2	/* Check my location */
#define	SWEEP_COMMANDS	4	/* Check for $-commands */
#define	SWEEP_LISTEN	8	/* Check for @listen-ers */
#define	SWEEP_PLAYER	16	/* Check for players and puppets */
#define	SWEEP_CONNECT	32	/* Search for connected players/puppets */
#define	SWEEP_EXITS	64	/* Search the exits for audible flags */
#define	SWEEP_SCAN	128	/* Scan for pattern matching */
#define	SWEEP_VERBOSE	256	/* Display what pattern matches */
#define WIPE_PRESERVE	1	/* Reverse effect of @wipe */
#define WIPE_REGEXP	2	/* Wipe using regexp */
#define TEL_GRAB	1
#define TEL_JOIN	2
#define TEL_LIST	4
#define TEL_QUIET	8
#define	TOAD_NO_CHOWN	1	/* Don't change ownership */
#define	TOAD_UNIQUE	2	/* Unique Rename Object */
#define TOGGLE_CHECK	1
#define TOGGLE_CLEAR	2	/* Clear the toggle list */
#define TOR_LIST	1	/* List the TOR information */
#define TOR_CACHE	2	/* Recache the TOR information */
#define	TRIG_QUIET	1	/* Don't display 'Triggered.' message. */
#define TRIG_PROGRAM    2       /* Trigger is actually a @program */
#define TRIG_COMMAND    4       /* Can Trigger $commands */
#define INCLUDE_COMMAND	1	/* Can @insert trigger $commands */
#define INCLUDE_LOCAL	2	/* Localize all the @included foo */
#define INCLUDE_CLEAR	4	/* Clear the attributes locally */
#define INCLUDE_NOBREAK 8	/* Do not @break other than inside @include */
#define INCLUDE_TARGET	16	/* Allow the target item (if you control it) to be executor */
#define INCLUDE_OVERRIDE 32	/* Trigger include like well trigger */
#define SUDO_GLOBAL	1	/* Reverse of localized */
#define SUDO_CLEAR	2	/* Clear registers */
#define	TWARP_QUEUE	1	/* Warp the wait and sem queues */
#define	TWARP_DUMP	2	/* Warp the dump interval */
#define	TWARP_CLEAN	4	/* Warp the cleaning interval */
#define	TWARP_IDLE	8	/* Warp the idle check interval */
#define	TWARP_RWHO	16	/* Warp the RWHO dump interval */
#define MLOG_MANUAL     1       /* Define manual log switch */
#define MLOG_STATS      2       /* Log Statistics */
#define MLOG_READ       4       /* Read log page by page (page = 10 lines) */
#define MLOG_FILE	8	/* Specify file name for manual log (128 chars max) */
#define MLOG_ROOM	16	/* Log Room's output */
#define LOGROTATE_STATUS 1	/* Status of current log */
#define BLACKLIST_LIST	1	/* List blacklist */
#define BLACKLIST_CLEAR	2	/* Clear blacklist */
#define BLACKLIST_LOAD	4	/* Load blacklist.txt file */
#define WAIT_PID        1       /* Re-wait a PID process */
#define WAIT_UNTIL	2	/* Wait until specified time */
#define WAIT_RECPID	4	/* Record PID of wait to specified setq register */
#define THAW_DEL	1	/* Drop the frozen FTIME pid process */
#define DYN_PARSE       1	/* Parse the help */
#define DYN_SEARCH	2	/* Issue a contextual search of help */
#define EDIT_CHECK	1	/* Just check @edit, don't set */
#define EDIT_SINGLE	2	/* Just do a single @edit, not multiple */
#define EDIT_STRICT	4	/* MUX/PENN ANSI Editing compatibility */
#define EDIT_RAW	8	/* Raw ANSI editor for strings (old edit method) */
#define CLUSTER_NEW	1	/* create a new cluster */
#define CLUSTER_ADD	2	/* add a dbref to a cluster */
#define CLUSTER_DEL	4	/* delete a dbref from a cluster */
#define CLUSTER_CLEAR	8	/* Purge the cluster */
#define CLUSTER_LIST	16	/* List out the specifics of the cluster */
#define CLUSTER_THRESHOLD 32	/* Set the treshold for the cluster */
#define CLUSTER_ACTION	64	/* Specify the action for the cluster */
#define CLUSTER_EDIT	128	/* Edit the cluster's action */
#define CLUSTER_REPAIR  256	/* Fix the cluster smartly */
#define CLUSTER_SET	512	/* @set for the cluster */
#define CLUSTER_WIPE	1024	/* @wipe for the cluster */
#define CLUSTER_CUT	2048	/* Forcefully remove a cluster item */
#define CLUSTER_GREP	4096	/* Grep for a cluster */
#define CLUSTER_REACTION 8192	/* Edit action for a cluster */
#define CLUSTER_TRIGGER 16384	/* Trigger attribute on cluster */
#define CLUSTER_FUNC    32768	/* Trigger function action instead of command action */
#define CLUSTER_REGEXP  65536	/* Allow regexp matching where applicable */

/* Hush codes for movement messages */

#define	HUSH_ENTER	1	/* xENTER/xEFAIL */
#define	HUSH_LEAVE	2	/* xLEAVE/xLFAIL */
#define	HUSH_EXIT	4	/* xSUCC/xDROP/xFAIL from exits */
#define HUSH_QUIET	8
#define HUSH_BLIND        16      /* Exit/Destination has BLIND flag */

/* Evaluation directives */

#define	EV_FMASK	0x00000300	/* Mask for function type check */
#define	EV_FIGNORE	0x00000000	/* Don't look for func if () found */
#define	EV_FMAND	0x00000100	/* Text before () must be func name */
#define	EV_FCHECK	0x00000200	/* Check text before () for function */
#define	EV_STRIP	0x00000400	/* Strip one level of brackets */
#define	EV_EVAL		0x00000800	/* Evaluate results before returning */
#define	EV_STRIP_TS	0x00001000	/* Strip trailing spaces */
#define	EV_STRIP_LS	0x00002000	/* Strip leading spaces */
#define	EV_STRIP_ESC	0x00004000	/* Strip one level of \ characters */
#define	EV_STRIP_AROUND	0x00008000	/* Strip {} only at ends of string */
#define	EV_TOP		0x00010000	/* This is a toplevel call to eval() */
#define	EV_NOTRACE	0x00020000	/* Don't trace this call to eval */
#define EV_PARSE_ANSI   0x00040000 	/* Parse the ansi in EXEC */

/* %-SUB overriding */
#define SUB_N           0x00000001
#define SUB_NUM         0x00000002
#define SUB_BANG        0x00000004
#define SUB_AT          0x00000008
#define SUB_L           0x00000010
#define SUB_S           0x00000020
#define SUB_O           0x00000040
#define SUB_P           0x00000080
#define SUB_A           0x00000100
#define SUB_R           0x00000200
#define SUB_T           0x00000400
#define SUB_C           0x00000800
#define SUB_X           0x00001000
#define SUB_F		0x00002000
#define SUB_K		0x00004000
#define SUB_W           0x00008000
#define SUB_M           0x00010000

/* Message forwarding directives */

#define	MSG_PUP_ALWAYS	1	/* Always forward msg to puppet own */
#define	MSG_INV		2	/* Forward msg to contents */
#define	MSG_INV_L	4	/* ... only if msg passes my @listen */
#define	MSG_INV_EXITS	8	/* Forward through my audible exits */
#define	MSG_NBR		16	/* Forward msg to neighbors */
#define	MSG_NBR_A	32	/* ... only if I am audible */
#define	MSG_NBR_EXITS	64	/* Also forward to neighbor exits */
#define	MSG_NBR_EXITS_A	128	/* ... only if I am audible */
#define	MSG_LOC		256	/* Send to my location */
#define	MSG_LOC_A	512	/* ... only if I am audible */
#define	MSG_FWDLIST	1024	/* Forward to my fwdlist members if aud */
#define	MSG_ME		2048	/* Send to me */
#define	MSG_S_INSIDE	4096	/* Originator is inside target */
#define	MSG_S_OUTSIDE	8192	/* Originator is outside target */
#define MSG_SILENT     16384    /* Output is silent and non-redirected */
#define MSG_NO_ANSI    32768    /* Output shouldn't be parsed for ansi */
#define	MSG_ME_ALL	(MSG_ME|MSG_INV_EXITS|MSG_FWDLIST)
#define	MSG_F_CONTENTS	(MSG_INV)
#define	MSG_F_UP	(MSG_NBR_A|MSG_LOC_A)
#define	MSG_F_DOWN	(MSG_INV_L)

/* Look primitive directives */

#define	LK_IDESC	0x0001
#define	LK_OBEYTERSE	0x0002
#define	LK_SHOWATTR	0x0004
#define	LK_SHOWEXIT	0x0008

/* Exit visibility precalculation codes */

#define	VE_LOC_XAM	0x01	/* Location is examinable */
#define	VE_LOC_DARK	0x02	/* Location is dark */
#define	VE_LOC_LIGHT	0x04	/* Location is light */
#define	VE_BASE_XAM	0x08	/* Base location (pre-parent) is examinable */
#define	VE_BASE_DARK	0x10	/* Base location (pre-parent) is dark */
#define	VE_BASE_LIGHT	0x20	/* Base location (pre-parent) is light */
#define VE_ACK		0x40

/* Signal handling directives */

#define	SA_RESIG	1	/* Re-signal from withing signal handler */
#define	SA_RETRY	2	/* Disable sigs, return to point-of-failure */
#define	SA_DFLT		3	/* Don't catch fatal signals */

#define	STARTLOG(key,p,s) \
	if ((((key) & mudconf.log_options) != 0) && start_log(p, s)) {
#define	ENDLOG \
	end_log(); }
#define	LOG_SIMPLE(key,p,s,m) \
	STARTLOG(key,p,s) \
		log_text(m); \
	ENDLOG

#define	test_top()		((mudstate.qfirst != NULL) ? 1 : 0)
#define	controls(p,x)		Controls(p,x)

#define atr_get(w,x,y,z)	atr_get_ash(w,x,y,z,__LINE__,__FILE__)
#define atr_pget(w,x,y,z)	atr_pget_ash(w,x,y,z,__LINE__,__FILE__)
/* Define this to value other than 30 for total args pass to a function
 * like switch().  The number of arguments will always be (MAX_ARGS / 2)
 * This is just used in 'eval.c' and 'command.c'
 * For MUX 2.0 compatibility, you may wish to put this to 100
 */
#define MAX_ARGS        30

#ifndef PCRE_EXEC
#define regexp_wild_match(v,w,x,y,z) (0)
#define grep_internal_regexp(v,w,x,y,z,a) alloc_lbuf("grep_internal_regexp")
#define load_regexp_functions(x) (0)
#define PCRE_EXEC 	0
#endif
#endif	/* __EXTERNS_H */
