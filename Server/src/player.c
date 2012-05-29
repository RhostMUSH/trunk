/* player.c */

#ifdef SOLARIS
/* Solaris is stupid on declarations */
char *index(const char *, int);
#endif

#include "autoconf.h"
#include <netdb.h>
#include "copyright.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"

#define	NUM_GOOD	4	/* # of successful logins to save data for */
#define NUM_BAD		3	/* # of failed logins to save data for */

typedef struct hostdtm HOSTDTM;
struct hostdtm {
	char	*host;
	char	*dtm;
};

typedef struct logindata LDATA;
struct logindata {
	HOSTDTM good[NUM_GOOD];
	HOSTDTM bad[NUM_BAD];
	int	tot_good;
	int	tot_bad;
	int	new_bad;
};

extern time_t	FDECL(time, (time_t *));
extern const char *	FDECL(time_format_1, (time_t));

/* ---------------------------------------------------------------------------
 * decrypt_logindata, encrypt_logindata: Decode and encode login info.
 */

static void decrypt_logindata (char *atrbuf, LDATA *info)
{
int	i;

	info->tot_good = 0;
	info->tot_bad = 0;
	info->new_bad = 0;
	for (i=0; i<NUM_GOOD; i++) {
		info->good[i].host = NULL;
		info->good[i].dtm = NULL;
	}
	for (i=0; i<NUM_BAD; i++) {
		info->bad[i].host = NULL;
		info->bad[i].dtm = NULL;
	}

	if (*atrbuf == '#') {
		atrbuf++;
		info->tot_good = atoi(grabto(&atrbuf, ';'));
		for (i=0; i<NUM_GOOD; i++) {
			info->good[i].host = grabto(&atrbuf, ';');
			info->good[i].dtm = grabto(&atrbuf, ';');
		}
		info->new_bad = atoi(grabto(&atrbuf, ';'));
		info->tot_bad = atoi(grabto(&atrbuf, ';'));
		for (i=0; i<NUM_BAD; i++) {
			info->bad[i].host = grabto(&atrbuf, ';');
			info->bad[i].dtm = grabto(&atrbuf, ';');
		}
	}
}

static void encrypt_logindata (char *atrbuf, LDATA *info)
{
char	*bp, nullc;
int	i;

	/* Make sure the SPRINTF call tracks NUM_GOOD and NUM_BAD for the
	 * number of host/dtm pairs of each type.
	 */

	nullc = '\0';
	for (i=0; i<NUM_GOOD; i++) {
		if (!info->good[i].host)
			info->good[i].host = &nullc;
		if (!info->good[i].dtm)
			info->good[i].dtm = &nullc;
	}
	for (i=0; i<NUM_BAD; i++) {
		if (!info->bad[i].host)
			info->bad[i].host = &nullc;
		if (!info->bad[i].dtm)
			info->bad[i].dtm = &nullc;
	}
	bp = alloc_lbuf("encrypt_logindata");
	sprintf(bp, "#%d;%s;%s;%s;%s;%s;%s;%s;%s;%d;%d;%s;%s;%s;%s;%s;%s;",
		info->tot_good,
		info->good[0].host, info->good[0].dtm,
		info->good[1].host, info->good[1].dtm,
		info->good[2].host, info->good[2].dtm,
		info->good[3].host, info->good[3].dtm,
		info->new_bad, info->tot_bad,
		info->bad[0].host, info->bad[0].dtm,
		info->bad[1].host, info->bad[1].dtm,
		info->bad[2].host, info->bad[2].dtm);
	strcpy(atrbuf, bp);
	free_lbuf(bp);
}

/* ---------------------------------------------------------------------------
 * record_login: Record successful or failed login attempt.
 * If successful, report last successful login and number of failures since
 * last successful login.
 */

void record_login (dbref player, int isgood, char *ldate, char *lhost, int *totsucc, int *totfail, int *newfail)
{
LDATA	login_info;
char	*atrbuf, *tpr_buff, *tprp_buff;
dbref	aowner;
int	aflags, i;

	atrbuf = atr_get(player, A_LOGINDATA, &aowner, &aflags);
	decrypt_logindata(atrbuf, &login_info);
	if (isgood) {
		if (login_info.new_bad > 0) {
			notify(player, " ");
                        tprp_buff = tpr_buff = alloc_lbuf("record_login");
			notify(player,
				safe_tprintf(tpr_buff, &tprp_buff, "**** %d failed connect%s since your last successful connect. ****",
					login_info.new_bad,
					(login_info.new_bad == 1 ? "" : "s")));
                        free_lbuf(tpr_buff);
                        tprp_buff = tpr_buff = alloc_lbuf("record_login");
			notify(player,
				safe_tprintf(tpr_buff, &tprp_buff, "Most recent attempt was from %s on %s.",
					login_info.bad[0].host,
					login_info.bad[0].dtm));
                        free_lbuf(tpr_buff);
			notify(player, " ");
			*newfail = login_info.new_bad;
			login_info.new_bad = 0;
		}
		else
			*newfail = login_info.new_bad;
		if (login_info.good[0].host && *login_info.good[0].host &&
		    login_info.good[0].dtm && *login_info.good[0].dtm) {
                        tprp_buff = tpr_buff = alloc_lbuf("record_login");
			notify(player,
				safe_tprintf(tpr_buff, &tprp_buff, "Last connect was from %s on %s.",
					login_info.good[0].host,
					login_info.good[0].dtm));
                        free_lbuf(tpr_buff);
		}
		for (i=NUM_GOOD-1; i>0; i--) {
			login_info.good[i].dtm = login_info.good[i-1].dtm;
			login_info.good[i].host = login_info.good[i-1].host;
		}
		login_info.good[0].dtm = ldate;
		login_info.good[0].host = lhost;
		login_info.tot_good++;
		atr_add_raw(player, A_LASTSITE, lhost);
	} else {
		for (i=NUM_BAD-1; i>0; i--) {
			login_info.bad[i].dtm = login_info.bad[i-1].dtm;
			login_info.bad[i].host = login_info.bad[i-1].host;
		}
		login_info.bad[0].dtm = ldate;
		login_info.bad[0].host = lhost;
		login_info.tot_bad++;
		login_info.new_bad++;
		*newfail = login_info.new_bad;
	}
	*totsucc = login_info.tot_good;
	*totfail = login_info.tot_bad;
	encrypt_logindata(atrbuf, &login_info);
	atr_add_raw(player, A_LOGINDATA, atrbuf);
	free_lbuf(atrbuf);
}

/* ---------------------------------------------------------------------------
 * check_pass: Test a password to see if it is correct.
 */

int check_pass(dbref player, const char *password, int flag)
{
dbref	aowner;
int	aflags;
char	*target;

/* This is needed to prevent entering the raw encrypted password from
 * working.  Do it better if you like, but it's needed. */

 if (((strlen(password) == 13) || strlen(password) == 22)
     &&
     (password[0] == 'X') && (password[1] == 'X'))
   return 0;
 
 if (flag == 0) {
   target = atr_get(player, A_PASS, &aowner, &aflags);
 }
 else {
   target = atr_get(player, A_MPASS, &aowner, &aflags);
 }
 
 if (*target && mush_crypt_validate(player, password, target, flag)) {
   free_lbuf(target);
   return 1;
 }

 free_lbuf(target);
 return 0;
}

/* ---------------------------------------------------------------------------
 * check_site: Check for allowed/disallowed sites.
 */

int lookup(char *host, char *data)
{
  char *parse, buff[MBUF_SIZE];

  memset(buff, 0, sizeof(buff));
  strncpy(buff,host,(MBUF_SIZE-1));
  parse = strtok(data,",/\t ");
  while (parse) {
    if (quick_wild(parse,buff))
      break;
    parse = strtok(NULL,",/\t ");
  }
  if (parse)
    return 1;
  else
   return 0;
}
    

int check_site(dbref player, DESC *d)
{
  char *siteinfo, *host;
  dbref	aowner;
  int	aflags, ok;

  host = inet_ntoa(d->address.sin_addr);
  siteinfo = atr_get(player,A_SITEGOOD, &aowner, &aflags);
  if (*siteinfo == '\0')
    ok = 1;
  else 
    ok = lookup(host,siteinfo);
  free_lbuf(siteinfo);
  if (ok) {
    siteinfo = atr_get(player,A_SITEBAD, &aowner, &aflags);
    if (*siteinfo != '\0') 
      ok = !lookup(host,siteinfo);
    free_lbuf(siteinfo);
  }
  return ok;
}
      

/* ---------------------------------------------------------------------------
 * connect_player: Try to connect to an existing player.
 */

dbref connect_player(char *name, char *password, char *d2)
{
dbref	player, aowner;
int	aflags;
time_t	tt;
char	*time_str, *player_last, *allowance, *identbuff;
char	*host, *userid;
DESC	*d;
int totfail, newfail, totsucc;

	d = (DESC *)d2;
	host = d->addr;
	userid = d->userid;
	time(&tt);
	time_str = ctime(&tt);
	time_str[strlen(time_str) - 1] = '\0';

	if ((player = lookup_player(NOTHING, name, 0)) == NOTHING)
		return NOTHING;
	if (!check_site(player,d)) {
		broadcast_monitor(player,MF_SITE|MF_FAIL,"BADSITE",userid,host,0,0,0,NULL);
		record_login(player, 0, time_str, host, &totsucc, &totfail, &newfail);
		return NOPERM;
	} 
	if (!check_pass(player, password, 0)) {
		broadcast_monitor(player,MF_SITE|MF_FAIL|MF_COMPFAIL,"FAIL",userid,host,0,0,0,NULL);
		record_login(player, 0, time_str, host, &totsucc, &totfail, &newfail);
		return NOTHING;
	} 
	time(&tt);
	time_str = ctime(&tt);
	time_str[strlen(time_str) - 1] = '\0';

	/* compare to last connect see if player gets salary */
	player_last = atr_get(player, A_LAST, &aowner, &aflags);
	if (strncmp(player_last, time_str, 10) != 0) {
		allowance = atr_pget(player, A_ALLOWANCE, &aowner, &aflags);
		if (*allowance == '\0') 
			giveto(player, mudconf.paycheck, NOTHING);
		else 
			giveto(player, atoi(allowance), NOTHING);
		free_lbuf(allowance);
	}
	atr_add_raw(player, A_LAST, time_str);
	free_lbuf(player_last);
        identbuff = alloc_lbuf("ident");
        if( *userid ) {
          sprintf(identbuff, "%s@%s",userid,host);
        }
        else {
          strcpy(identbuff, host);
        }
        atr_add_raw(player, A_IDENT, identbuff);
        free_lbuf(identbuff);
	return player;
}

/* ---------------------------------------------------------------------------
 * create_player: Create a new player.
 */

dbref create_player(char *name, char *password, dbref creator, int isrobot)
{
  dbref	player;
  char	*pbuf;
  int atr = 0;

	/* Make sure the password is OK.  Name is checked in create_obj */

	pbuf = trim_spaces(password);
	if (!ok_password(pbuf, creator, 0)) {
		free_lbuf(pbuf);
		return NOTHING;
	}

	/* If so, go create him */

	player = create_obj(creator, TYPE_PLAYER, name, isrobot);
	if (player == NOTHING) {
		free_lbuf(pbuf);
		return NOTHING;
	}

	/* initialize everything */

	s_Pass(player, mush_crypt(pbuf));
	s_Home(player, start_home());
	if (!mudconf.start_build)
          s_Flags2(player, Flags2(player) | WANDERER);

	
	/* We need to try find the attr... */
	if (mudconf.guild_attrname[0] != '\0') {
	   atr = mkattr(mudconf.guild_attrname);
	   if (atr > 0) {
	     if ( strlen(strip_ansi(mudconf.guild_default)) < 1 )
	       atr_add_raw(player, atr,(char *)"Wanderer");
             else
	       atr_add_raw(player, atr,(char *)strip_ansi(mudconf.guild_default));
	   }
	}

	sprintf(pbuf,"%d",mudconf.start_quota);
	atr_add_raw(player, A_QUOTA,pbuf);
	sprintf(pbuf,"%d",mudconf.start_quota - 1);
	atr_add_raw(player, A_RQUOTA,pbuf);
	free_lbuf(pbuf);
	return player;
}

/* ---------------------------------------------------------------------------
 * do_password: Change the password for a player
 */

void do_password(dbref player, dbref cause, int key, char *oldpass, char *newpass)
{
dbref	aowner;
int	aflags;
char	*target;

	target = atr_get(player, A_PASS, &aowner, &aflags);
	if (!*target || !check_pass(player, oldpass, 0)) {
		notify(player, "Sorry.");
	} else if (!ok_password(newpass, player, 0)) {
 		notify(player, "Bad new password.");
	} else if (!Immortal(player) && DePriv(player,NOTHING,DP_PASSWORD,POWER8,POWER_LEVEL_NA)) {
		notify(player, "Sorry.");
	} else {
		atr_add_raw(player, A_PASS, mush_crypt(newpass));
		notify(player, "Password changed.");
	}
	free_lbuf(target);
}

/* ---------------------------------------------------------------------------
 * do_last Display login history data.
 */

static void disp_from_on (dbref player, char *dtm_str, char *host_str)
{
  char *tpr_buff, *tprp_buff;
	if (dtm_str && *dtm_str && host_str && *host_str) {
		tprp_buff = tpr_buff = alloc_lbuf("disp_from_on");
		notify(player,
			safe_tprintf(tpr_buff, &tprp_buff, "     From: %s   On: %s", dtm_str, host_str));
		free_lbuf(tpr_buff);
	}
}

void do_last (dbref player, dbref cause, int key, char *who)
{
dbref	target, aowner;
LDATA	login_info;
char	*atrbuf;
int	i, aflags;

	if (!who || !*who) {
		target = Owner(player);
	} else if (!(string_compare(who, "me"))) {
		target = Owner(player);
	} else {
		target = lookup_player(player, who, 1);
	}

	if (target == NOTHING) {
		notify(player, "I couldn't find that player.");
	} else if (!Controls(player, target)) {
		notify(player, "Permission denied.");
	} else {
		atrbuf = atr_get(target, A_LOGINDATA, &aowner, &aflags);
		decrypt_logindata(atrbuf, &login_info);

		notify(player, unsafe_tprintf("Total successful connects: %d",
			login_info.tot_good));
		for (i=0; i<NUM_GOOD; i++) {
			disp_from_on(player,
				login_info.good[i].host,
				login_info.good[i].dtm);
		}
		notify(player, unsafe_tprintf("Total failed connects: %d",
			login_info.tot_bad));
		for (i=0; i<NUM_BAD; i++) {
			disp_from_on(player,
				login_info.bad[i].host,
				login_info.bad[i].dtm);
		}
		free_lbuf(atrbuf);
	}
}

/* ---------------------------------------------------------------------------
 * add_player_name, delete_player_name, lookup_player:
 * Manage playername->dbref mapping
 */

int add_player_name (dbref player, char *name)
{
int	stat;
dbref	p;
char	*temp, *tp;

	/* Convert to all lowercase */

	tp = temp = alloc_lbuf("add_player_name");
	safe_str(name, temp, &tp);
	*tp = '\0';
	for (tp=temp; *tp; tp++)
		*tp = ToLower((int)*tp);

	p = (int)hashfind(temp, &mudstate.player_htab);
	if (p) {

		/* Entry found in the hashtable.  If a player, succeed if the
		 * numbers match (already correctly in the hash table), fail
		 * if they don't.  Fail if the name is a disallowed name
		 * (value AMBIGUOUS).
		 */

		if (p == AMBIGUOUS) {
			free_lbuf(temp);
			return 0;
		}

		if (Good_obj(p) && (Typeof(p) == TYPE_PLAYER)) {
			free_lbuf(temp);
			if (p == player) {
				return 1;
			} else {
				return 0;
			}
		}

		/* It's an alias (or an incorrect entry).  Clobber it */

		stat = hashrepl(temp, (int *)player, &mudstate.player_htab);
		free_lbuf(temp);
	} else {
		stat = hashadd(temp, (int *)player, &mudstate.player_htab);
		free_lbuf(temp);
		stat = (stat < 0) ? 0 : 1;
	}
	return stat;
}

int delete_player_name (dbref player, char *name)
{
dbref	p;
char	*temp, *tp;

	tp = temp = alloc_lbuf("delete_player_name");
	safe_str(name, temp, &tp);
	*tp = '\0';
	for (tp=temp; *tp; tp++)
		*tp = ToLower((int)*tp);
	p = (int)hashfind(temp, &mudstate.player_htab);
	if (!p || (p == NOTHING) || ((player != NOTHING) && (p != player))) {
		free_lbuf(temp);
		return 0;
	}
	hashdelete(temp, &mudstate.player_htab);
	free_lbuf(temp);
	return 1;
}

dbref lookup_player (dbref doer, char *name, int check_who)
{
dbref	p;
char	*temp, *tp;

	if (!string_compare(name, "me"))
		return doer;

	if (*name == NUMBER_TOKEN) {
		name++;
		if (!is_number(name))
			return NOTHING;
		p = atoi(name);
		if (!Good_obj(p) || Recover(p))
			return NOTHING;
		if (!((Typeof(p) == TYPE_PLAYER) || God(doer)))
			p = NOTHING;
		return p;
	}
	tp = temp = alloc_lbuf("lookup_player");
        if (*name == '*') {
           safe_str(name+1, temp, &tp);
        } else
	   safe_str(name, temp, &tp);
	*tp = '\0';
	for (tp=temp; *tp; tp++)
		*tp = ToLower((int)*tp);
	p = (int)hashfind(temp, &mudstate.player_htab);
	free_lbuf(temp);
	if (!p) {
		if (check_who)
			p = find_connected_name(doer, name);
		else
			p = NOTHING;
	} else if (!Good_obj(p)) {
		p = NOTHING;
	}
	return p;
}

void NDECL(load_player_names)
{
dbref	i, j, aowner;
int	aflags;
char	*alias;
int	ibf = -1;

	j = 0;
	DO_WHOLE_DB(i) {
 	if ((Typeof(i) == TYPE_PLAYER) && !Recover(i)) {
			add_player_name (i, Name(i));
			if (++j > 20) {
				cache_reset(0);
				j = 0;
			}
		}
	}
	alias = alloc_lbuf("load_player_names");
	j = 0;
	DO_WHOLE_DB(i) {
	if ((Typeof(i) == TYPE_PLAYER) && !Recover(i)) {
			alias = atr_pget_str(alias, i, A_ALIAS,
				&aowner, &aflags, &ibf);
			if (*alias)
				add_player_name (i, alias);
			if (++j > 20) {
				cache_reset(0);
				j = 0;
			}
		}
	}
	free_lbuf(alias);
}

/* ---------------------------------------------------------------------------
 * badname_add, badname_check, badname_list: Add/look for/display bad names.
 * protectname has similiar matches
 */

int protectname_add (char *protect_name, dbref player)
{
   PROTECTNAME *bp;
   int i_namecnt;

   i_namecnt=0;
   for ( bp=mudstate.protectname_head; bp; bp=bp->next ) {
      if ( bp->i_name == player ) {
         i_namecnt++;
         if ( !stricmp(protect_name, bp->name) ) {
            return 2;
         }
      }
   }
   if ( i_namecnt >= mudconf.max_name_protect ) {
      return 1;
   }
   bp = (PROTECTNAME *)XMALLOC(sizeof(PROTECTNAME), "protectname.struc");
   bp->name = XMALLOC(strlen(protect_name) + 1, "protectname.name");
   bp->i_name = player;
   bp->i_key = 0;
   bp->next = mudstate.protectname_head;
   mudstate.protectname_head = bp;
   strcpy(bp->name, protect_name);
   return 0;
}

void badname_add (char *bad_name)
{
BADNAME	*bp;

	/* Make a new node and link it in at the top */

	bp = (BADNAME *)XMALLOC(sizeof(BADNAME), "badname.struc");
	bp->name = XMALLOC(strlen(bad_name) + 1, "badname.name");
	bp->next = mudstate.badname_head;
	mudstate.badname_head = bp;
	strcpy(bp->name, bad_name);
}

dbref protectname_remove (char *protect_name, dbref player)
{
   PROTECTNAME *bp, *backp;
   dbref target, aowner;
   int aflags;
   char *s_alias;

   backp = NULL;
   for ( bp=mudstate.protectname_head; bp; backp=bp, bp=bp->next ) {
      if ( Wizard(player) || (bp->i_name == player) ) {
         if ( !string_compare( protect_name, bp->name ) ) {
            s_alias = atr_get(bp->i_name, A_ALIAS, &aowner, &aflags);
            target = bp->i_name;
            if ( backp ) {
               backp->next = bp->next;
            } else {
               mudstate.protectname_head = bp->next;
            }
            if ( bp->i_key ) {
               if ( string_compare( protect_name, Name(bp->i_name) ) &&
                    string_compare( protect_name, s_alias ) ) {
                  delete_player_name(target, bp->name);
               }
            }
            XFREE(bp->name, "protectname.name");
            XFREE(bp, "protectname.struc");
            free_lbuf(s_alias);
            return target;
         }
      }
   }
   return -1;
}

dbref protectname_unalias (char *protect_name, dbref player)
{
   PROTECTNAME *bp, *backp;
   dbref target, aowner;
   int aflags;
   char *s_alias;

   backp = NULL;
   for ( bp=mudstate.protectname_head; bp; backp=bp, bp=bp->next ) {
      if ( Wizard(player) || (bp->i_name == player) ) {
         if ( !string_compare( protect_name, Name(bp->i_name) ) ) {
            return -3;
         }
         s_alias = atr_get(bp->i_name, A_ALIAS, &aowner, &aflags);
         if ( !string_compare( protect_name, s_alias ) ) {
            free_lbuf(s_alias);
            return -4;
         }
         free_lbuf(s_alias);
         if ( !string_compare( protect_name, bp->name ) ) {
            target = bp->i_name;
            if ( bp->i_key ) {
               delete_player_name(target, bp->name);
               bp->i_key=0;
               return target;
            } else {
               return -2;
            }
         }
      }
   }
   return -1;
}

dbref protectname_alias (char *protect_name, dbref player)
{
   PROTECTNAME *bp, *backp;
   dbref target, aowner;
   int aflags;
   char *s_alias;

   backp = NULL;
   for ( bp=mudstate.protectname_head; bp; backp=bp, bp=bp->next ) {
      if ( Wizard(player) || (bp->i_name == player) ) {
         if ( !string_compare( protect_name, bp->name ) ) {
            target = bp->i_name;
            s_alias = atr_get(bp->i_name, A_ALIAS, &aowner, &aflags);
            if ( !string_compare( protect_name, s_alias ) ) {
               free_lbuf(s_alias);
               return -3;
            }
            free_lbuf(s_alias);
            if ( !bp->i_key ) {
               add_player_name(target, bp->name);
               bp->i_key=1;
               return target;
            } else {
               return -2;
            }
         }
      }
   }
   return -1;
}


void badname_remove (char *bad_name)
{
BADNAME	*bp, *backp;

	/* Look for an exact match on the bad name and remove if found */

	backp = NULL;
	for (bp=mudstate.badname_head; bp; backp=bp,bp=bp->next) {
		if (!string_compare(bad_name, bp->name)) {
			if (backp)
				backp->next = bp->next;
			else
				mudstate.badname_head = bp->next;
			XFREE(bp->name, "badname.name");
			XFREE(bp, "badname.struc");
			return;
		}
	}
}

int protectname_check ( char *protect_name, dbref checker, int key ) {
   PROTECTNAME *bp;

   for ( bp=mudstate.protectname_head; bp; bp=bp->next) {
      if ( quick_wild(bp->name, protect_name) ) {
         if ( !key && (bp->i_name == checker) ) {
            if ( bp->i_key ) 
               return 2;
            else
               return 1;
         } else {
            if ( Immortal(checker) && !key )
               return 1;
            return 0;
         }
      }
   }
   return 1;
}

int badname_check (char *bad_name, dbref checker)
{
BADNAME *bp;

	/* Walk the badname list, doing wildcard matching.  If we get a hit
	 * then return false.  If no matches in the list, return true.
	 */

	if (Immortal(checker))
		return 1;

	for (bp=mudstate.badname_head; bp; bp=bp->next) {
		if (quick_wild(bp->name, bad_name))
			return 0;
	}
	return 1;
}

#define PROT_LIST 0
#define PROT_SUMM 1
#define PROT_BYPLAYER 2
void protectname_list (dbref player, int key, dbref target) 
{
   PROTECTNAME *bp;
   char *buff, *bufp, *s_fieldbuff;
   int i_cntr, i_alias;
   typedef struct tmp_holder {
              dbref i_name;
              int i_cntr;
              int i_alias;
              struct tmp_holder *next;
           } THOLD;
   THOLD *st_holder = NULL, *st_ptr = NULL, *st_ptr2 = NULL;

   buff = bufp = alloc_lbuf("protectname_list");
   /* we can have significantly more than LBUF names here */
   switch (key ) {
      case PROT_LIST:
         notify(player, "                                 FULL LISTING                                ");
         notify(player, "+------------------------------+------------------------------------+-------+");
         notify(player, "| Player Name Protected        | Dbref#   [ Player Name           ] | Alias |");
         notify(player, "+------------------------------+------------------------------------+-------+");
         i_cntr=0;
         for ( bp=mudstate.protectname_head; bp; bp=bp->next) {
            if ( Wizard(player) || (player == bp->i_name) ) {
               if ( player == bp->i_name ) {
                  i_cntr++;
               }
               bufp = buff;
               notify(player, safe_tprintf(buff, &bufp, "| %-28.28s | #%-7d [ %-21.21s ] |   %c   |", 
                              bp->name, bp->i_name, (Good_chk(bp->i_name) ? Name(bp->i_name) : "<INVALID>"),
                              ((bp->i_key > 0) ? 'Y' : 'N') ));
            }
         }
         notify(player, "+------------------------------+------------------------------------+-------+");
         notify(player, safe_tprintf(buff, &bufp, "You have %d out of %d names protected.", i_cntr, mudconf.max_name_protect));
         break;
      case PROT_SUMM:
         for ( bp=mudstate.protectname_head; bp; bp=bp->next) {
            for (st_ptr = st_holder; st_ptr; st_ptr = st_ptr->next) {
               if ( st_ptr->i_name == bp->i_name ) {
                  st_ptr->i_cntr++;
                  if ( bp->i_key > 0 ) 
                     st_ptr->i_alias++;
                  break;
               }
            }
            if ( !st_ptr ) {
               st_ptr2 = malloc(sizeof(THOLD));
               st_ptr2->i_name = bp->i_name;
               st_ptr2->i_cntr = 1;
               if ( bp->i_key > 0 ) 
                  st_ptr2->i_alias = 1;
               else
                  st_ptr2->i_alias = 0;
               st_ptr2->next = NULL;
               if ( !st_holder )
                  st_holder = st_ptr2;
               else {
                  for (st_ptr = st_holder; st_ptr && st_ptr->next; st_ptr = st_ptr->next);
                  st_ptr->next = st_ptr2;
               }
            }
         }
         st_ptr = st_holder;
         notify(player, "                              BY SUMMARY LISTING                             ");
         notify(player, "+-----------------------------------------+-------------------------+");
         notify(player, "| Dbref# Owner [ Player Name            ] | Total Number of Entries |");
         notify(player, "+-----------------------------------------+-------------------------+");
         s_fieldbuff = alloc_mbuf("do_protect_list");
         while ( st_ptr ) {
            sprintf(s_fieldbuff, "| #%-11d [ %-22.22s ] | %-5d/%-5d [%3d Alias] |", 
                    st_ptr->i_name, (Good_chk(st_ptr->i_name) ? Name(st_ptr->i_name) : "<INVALID>"), 
                    st_ptr->i_cntr, mudconf.max_name_protect, st_ptr->i_alias);
            notify(player, s_fieldbuff);
            st_ptr2 = st_ptr->next;
            free(st_ptr);
            st_ptr = st_ptr2;
         }
         free_mbuf(s_fieldbuff);
         notify(player, "+-----------------------------------------+-------------------------+");
         break;
      case PROT_BYPLAYER:
         notify(player, "                              BY PLAYER LISTING                              ");
         notify(player, "+------------------------------+------------------------------------+-------+");
         notify(player, "| Player Name Protected        | Dbref#   [ Player Name           ] | Alias |");
         notify(player, "+------------------------------+------------------------------------+-------+");
         i_cntr=i_alias=0;
         for ( bp=mudstate.protectname_head; bp; bp=bp->next) {
            if ( Wizard(player) && (target == bp->i_name) ) {
               i_cntr++;
               if ( bp->i_key )
                  i_alias++;
               bufp = buff;
               notify(player, safe_tprintf(buff, &bufp, "| %-28.28s | #%-7d [ %-21.21s ] |   %c   |", 
                              bp->name, bp->i_name, (Good_chk(bp->i_name) ? Name(bp->i_name) : "<INVALID>"),
                              ((bp->i_key > 0) ? 'Y' : 'N') ));
            }
         }
         notify(player, "+------------------------------+------------------------------------+-------+");
         notify(player, safe_tprintf(buff, &bufp, "Target player has %d out of %d names protected (%d aliased).", i_cntr, mudconf.max_name_protect, i_alias));
         break;
   }
   free_lbuf(buff);
}

void badname_list (dbref player, const char *prefix)
{
BADNAME *bp;
char	*buff, *bufp;

	/* Construct an lbuf with all the names separated by spaces */

	buff = bufp = alloc_lbuf("badname_list");
	safe_str((char *)prefix, buff, &bufp);
	for (bp=mudstate.badname_head; bp; bp=bp->next) {
		safe_chr(' ', buff, &bufp);
		safe_str(bp->name, buff, &bufp);
	}
	*bufp = '\0';

	/* Now display it */

	notify(player, buff);
	free_lbuf(buff);
}

int reg_internal(char *name, char *email, char *dum, int key)
{
  DESC *d;
  char rpass[9], work, *pt1, *pt2, *buff, readstr[80], instr_buff[1000], *tmp_email, *tmp_email_ptr, *s_filename;
  int x, code, abortmail, tot_lines, tot_badlines, allow_through, good_email;
  dbref player;
  char *tpr_buff, *tprp_buff;
  FILE *fpt, *fpt2;
  time_t now;

  fpt2 = NULL;
  abortmail = 0;
  d = (DESC *)dum;
  buff = alloc_lbuf("reg_internal");
  pt1 = email;
  while (*pt1 && !isspace((int)*pt1))
    pt1++;
  good_email = 1;
  for (pt2 = email; *pt2; pt2++ ) {
    if ( isalnum((int)*pt2) )
      continue;
    if ( index("-_+=#@:~.", *pt2) )
      continue;
    good_email = 0;
    break;
  }

  if (*pt1 || !good_email ) {
    if ( !key ) {
       STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "BAD")
         tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
         log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                  "Autoregistration create of '%s' failed, bad email (special character detected).  Email: %s, Site: %s, Id: %s",
		  name,email,d->addr,d->userid));
         free_lbuf(tpr_buff);
       ENDLOG
    }
    pt1 = buff;
    safe_str(name, buff, &pt1);
    safe_chr('/', buff, &pt1);
    safe_str(email, buff, &pt1);
    if ( !key )
       broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE FAIL, BAD EMAIL (INVCHAR)", d->userid, d->addr, d->descriptor, 0, 0, buff);
    free_lbuf(buff);
    return (*pt1 ? 5 : 7);
  }

  if (*pt1 || (strchr(email, '@') == NULL)) {
    if ( !key ) {
       STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "BAD")
         tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
         log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                  "Autoregistration create of '%s' failed, bad email.  Email: %s, Site: %s, Id: %s",
                   name,email,d->addr,d->userid));
         free_lbuf(tpr_buff);
       ENDLOG
    }
    pt1 = buff;
    safe_str(name, buff, &pt1);
    safe_chr('/', buff, &pt1);
    safe_str(email, buff, &pt1);
    if ( !key )
       broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE FAIL, BAD EMAIL", d->userid, d->addr, d->descriptor, 0, 0, buff);
    free_lbuf(buff);
    return ((strchr(email, '@') == NULL) ? 5 : 3);
  }
  /* These are same length - 1000 */
  allow_through = 0;
  /* First see if the mail is valid 
   * this will clobber instr_buff so we'll have to reset it later
   */
  strcpy(instr_buff, mudconf.goodmail_host);
  tmp_email_ptr = tmp_email = alloc_mbuf("tmp_email");
  safe_str(email, tmp_email, &tmp_email_ptr);
  if (((char *)mudconf.goodmail_host) && lookup(tmp_email, instr_buff)) {
     allow_through = 1;
  }
  /* Now let's see if the mail is invalid */
  strcpy(instr_buff, mudconf.validate_host);
  if (((char *)mudconf.validate_host) && lookup(tmp_email, instr_buff) && !allow_through) {
    if ( !key ) {
       STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "BAD")
         tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
         log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                  "Autoregistration create of '%s' failed, invalid email.  Email: %s, Site: %s, Id: %s",
                  name,email,d->addr,d->userid));
         free_lbuf(tpr_buff);
       ENDLOG
    }
    pt1 = buff;
    safe_str(name, buff, &pt1);
    safe_chr('/', buff, &pt1);
    safe_str(email, buff, &pt1);
    if ( !key )
       broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE FAIL, INVALID EMAIL", d->userid, d->addr, d->descriptor, 0, 0, buff);
    free_lbuf(buff);
    free_mbuf(tmp_email);
    return 6;
  }
  free_mbuf(tmp_email);
  if (!areg_check(email,0)) {
    if ( !key ) {
       STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "BAD")
         tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
         log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                  "Autoregistration create of '%s' failed, email limit.  Email: %s, Site: %s, Id: %s",
                  name,email,d->addr,d->userid));
         free_lbuf(tpr_buff);
       ENDLOG
    }
    pt1 = buff;
    safe_str(name, buff, &pt1);
    safe_chr('/', buff, &pt1);
    safe_str(email, buff, &pt1);
    if ( !key )
       broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE FAIL, EMAIL", d->userid, d->addr, d->descriptor, 0, 0, buff);
    free_lbuf(buff);
    return 4;
  }
  for (x = 0; x < 8; x++) {
    work = (char)(rand() % 62);
    if (work < 10)
      work += 48;
    else if (work < 36)
      work += 55;
    else
      work += 61;
    rpass[x] = work;
  }
  rpass[8] = '\0';
  player = create_player(name, rpass, NOTHING, 0);
  if (player == NOTHING) {
    if ( !key ) {
       STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "BAD")
         tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
         log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                  "Autoregistration create of '%s' failed.  Email: %s, Site: %s, Id: %s",
                  name,email,d->addr,d->userid));
         free_lbuf(tpr_buff);
       ENDLOG
    }
    pt1 = buff;
    safe_str(name, buff, &pt1);
    safe_chr('/', buff, &pt1);
    safe_str(email, buff, &pt1);
    if ( !key )
       broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE FAIL", d->userid, d->addr, d->descriptor, 0, 0, buff);
    code = 1;
  }
  else {
    move_object(player, mudconf.start_room);
    time(&now);
    sprintf(buff,"Email: %.1000s, Site: %.1000s, Id: %.1000s, Time: %s",email,d->addr,d->userid,ctime(&now));
    *(buff + strlen(buff) -1) = '\0';
    atr_add_raw(player, A_AUTOREG, buff);
    areg_add(email,player);
    fpt = fopen("mailtemp","wt+");
    s_filename = alloc_mbuf("filename_to_mail");
    sprintf(s_filename, "%.50s/%.140s", mudconf.txt_dir, mudconf.mailinclude_file);
    if ( strstr(s_filename, ".txt") != NULL )
       fpt2 = fopen(s_filename,"r");
    free_mbuf(s_filename);
    if (fpt != NULL) {
      fprintf(fpt,"Your autoregistration password for '%s' is: %s\n",name,rpass);
      if ( fpt2 != NULL ) {
         fprintf(fpt, "\n");
         tot_lines = 0;
         while ( !feof(fpt2) && !abortmail ) {
           memset(readstr, 0, sizeof(readstr));
           fgets(readstr, sizeof(readstr)-1, fpt2);
           if ( strchr(readstr, '\n') == 0 )
              fprintf(fpt, "%s\n", readstr);
           else
              fprintf(fpt, "%s", readstr);
           tot_badlines = 0;
           while ( (strchr(readstr, '\n') == 0) && !feof(fpt2) && !abortmail ) {
              fgets(readstr, sizeof(readstr)-1, fpt2);
              tot_badlines++;
              if ( tot_badlines == 1000 )
                 abortmail = 1;
           }
           tot_lines++;
           if ( tot_lines == 10000 )
              abortmail = 1;
         }
         fclose(fpt2);
      }
      fclose(fpt);
      if ( abortmail ) {
         fpt = fopen("mailtemp","wt+");
         fprintf(fpt,"Your autoregistration password for '%s' is: %s\n",name,rpass);
         fclose(fpt);
         if ( !key )
            broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE WARNING", d->userid, d->addr, 
                              d->descriptor, 0, 0, "There is an error in the autoreg_include file!");
      }
      if (mudconf.mailsub)
        sprintf(buff,"%s -s \"%s autoregistration\" %s < mailtemp",mudconf.mailprog,mudconf.mud_name,email);
      else
        sprintf(buff,"%s %s < mailtemp",mudconf.mailprog,email);
      system(buff);
      system("rm mailtemp");
      if ( !key ) {
         STARTLOG(LOG_SECURITY | LOG_PCREATES, "AUTOREG", "CREATE")
           tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
	   log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                    "Autoregistration character '%s', Email: %s, Site: %s, Id: %s",
                    name,email,d->addr,d->userid));
           free_lbuf(tpr_buff);
         ENDLOG
      }
      pt1 = buff;
      safe_str(name, buff, &pt1);
      safe_chr('/', buff, &pt1);
      safe_str(email, buff, &pt1);
      if ( !key )
         broadcast_monitor(NOTHING, MF_AREG, "AUTOREG CREATE", d->userid, d->addr, d->descriptor, 0, 0, buff);
      code = 0;
    }
    else {
      if ( !key ) {
         STARTLOG(LOG_SECURITY | LOG_PCREATES | LOG_BUGS | LOG_PROBLEMS, "AUTOREG", "BAD")
           tpr_buff = tprp_buff = alloc_lbuf("reg_internal");
	   log_text(safe_tprintf(tpr_buff, &tprp_buff, 
                    "Temporary file creation/mail for '%s' failed.  Email: %s, Site: %s, Id: %s",
                    name,email,d->addr,d->userid));
           free_lbuf(tpr_buff);
         ENDLOG
      }
      pt1 = buff;
      safe_str(name, buff, &pt1);
      safe_chr('/', buff, &pt1);
      safe_str(email, buff, &pt1);
      if ( !key )
         broadcast_monitor(NOTHING, MF_AREG, "AUTOREG FILE FAIL", d->userid, d->addr, d->descriptor, 0, 0, buff);
      code = 2;
    }
  }
  free_lbuf(buff);
  return code;
}

void do_register(dbref player, dbref cause, int key, char *name, char *email)
{
  dbref p2;
  DESC *d, *e;
  time_t now, dtime;
  char *buff;

  if (!mudconf.online_reg) {
    notify(player, "Autoregistration is disabled.");
  }
  else if (!Guest(player)) {
    notify(player,"You already have a character.");
  }
  else if ((strlen(name) > MBUF_SIZE) || (strlen(email) > MBUF_SIZE)) {
    notify(player,"Arguments too long.");
  }
  else {
    time(&now);
    dtime = 0;
    p2 = player;
    e = NULL;
    DESC_ITER_PLAYER(p2, d) {
      if (!dtime || (now - d->last_time) < dtime) {
	e = d;
	dtime = now - d->last_time;
      }
    }
    if (e && (e->host_info & H_NOAUTOREG)) {
      notify(player,"Permission denied.");
      buff = alloc_lbuf("do_register");
      strcpy(buff,name);
      strcat(buff,"/");
      strcat(buff,email);
      broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL (NoAutoReg)", e->userid, e->addr, e->descriptor, 0, 0, buff);
      free_lbuf(buff);
    }
    else if (e && (e->regtries_left <= 0)) {
      notify(player,"Registration limit reached.");
      buff = alloc_lbuf("do_register");
      strcpy(buff,name);
      strcat(buff,"/");
      strcat(buff,email);
      broadcast_monitor(NOTHING, MF_AREG, "NOAUTOREG FAIL LIMIT", e->userid, e->addr, e->descriptor, 0, 0, buff);
      free_lbuf(buff);
    }
    else if (e) {
      switch (reg_internal(name,email,(char *)e, 0)) {
        case 0:
	  (e->regtries_left)--;
	  notify(player,"Autoregistration completed.");
	  break;
        case 1:
	  notify(player,"Bad character name in autoregistration.  Invalid name or player already exists.");
	  break;
        case 2:
	  notify(player,"Autoregistration email send failed.");
	  break;
	case 3:
	  notify(player,"Space in email address.");
	  break;
	case 4:
	  notify(player,"Register limit for email specified reached.  Permission denied.");
	  break;
	case 5:
	  notify(player, "Invalid email specified.");
          break;
        case 6:
	  notify(player, "That email address is not allowed.");
          break;
        case 7:
          notify(player, "Invalid character detected in email.");
          break;
      }
    }
  }
}
