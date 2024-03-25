#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "flags.h"
#include "externs.h"
#include "interface.h"
#include "match.h"
#include "command.h"
#include "functions.h"
#include "misc.h"
#include "alloc.h"
#include "rhost_ansi.h"
#include "debug.h"
#include "interface.h"

#define FILENUM UTILS_C

int bittype(dbref player) {
  int level = 0;
  DPUSH; /* */
  if ( !Good_chk(player) )
     return 0;

  if( God(player) ) {
    level = 7;
  } else if( Immortal(player) ) {
    level = 6;
  } else if( Wizard(player) ) {
    level = 5;
  } else if( Admin(player) ) {
    level = 4;
  } else if( Builder(player) ) {
    level = 3;
  } else if( Guildmaster(player) ) {
    level = 2;
  } else if( Wanderer(player) || Guest(player) ) {
    level = 0;
  } else {
    level = 1;
  }
  RETURN(level); /* */
}

void
handle_conninfo_write(DESC *din, dbref player, int i_key)
{
   char *s_buff;
   dbref aowner;
   int aflags, i_ctime, i_clong, i_clast, i_ctotal;
   time_t i_clogout, i_tmp;

   if ( !din || !Good_obj(din->player) || (Typeof(din->player) != TYPE_PLAYER) ) {
      return;
   }

   /* If target descriptor is not connected ignore */
   if ( !(din->flags & DS_CONNECTED) ) {
      return;
   }

   i_ctime = i_clong = i_clast = i_ctotal = 0;
   i_clogout = 0;

   /* We do not want a parent get here */
   s_buff = atr_get(din->player, A_CONNINFO, &aowner, &aflags);
   if ( *s_buff ) {
      sscanf(s_buff, "%d %d %d %d %ld", &i_ctime, &i_clong, &i_clast, &i_ctotal, &i_clogout);
   }
   free_lbuf(s_buff);
   
   switch(i_key) {
      case CONN_ALL:
         i_tmp = mudstate.now - din->connected_at;
         if ( i_tmp > i_clong ) {
            i_clong = (int)i_tmp;
         }
         i_ctime += (int)i_tmp;
         i_clogout = mudstate.now;
         i_ctotal++;
         i_clast = i_tmp;
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      case CONN_TIME:
         i_tmp = mudstate.now - din->connected_at;
         i_ctime += (int)i_tmp;
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      case CONN_LONGEST:
         i_tmp = mudstate.now - din->connected_at;
         if ( i_tmp > i_clong ) {
            i_clong = (int)i_tmp;
         }
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      case CONN_LAST:
         i_tmp = mudstate.now - din->connected_at;
         i_clast = (int)i_tmp;
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      case CONN_TOTAL:
         i_ctotal++;
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      case CONN_LOGOUT:
         i_clogout = mudstate.now;
         s_buff = alloc_lbuf("handle_conninfo");
         sprintf(s_buff, "%d %d %d %d %ld", i_ctime, i_clong, i_clast, i_ctotal, i_clogout);
         atr_add_raw(din->player, A_CONNINFO, s_buff);
         free_lbuf(s_buff);
         break;
      default: /* Error handling */
         break;
   } 
   return;
}

int
handle_conninfo_read(char *s_target, dbref player, int i_key)
{
   char *s_buff;
   dbref aowner, target;
   int aflags, i_ctime, i_clong, i_clast, i_ctotal;
   time_t i_clogout, i_tmp, i_return;
   DESC *d;

   if ( !Good_chk(player) || !s_target || !*s_target ) {
      return -1;
   }

   init_match(player, s_target, TYPE_PLAYER);
   match_neighbor();
   match_me();
   match_here();
   if (Wizard(player)) {
      match_player();
      match_absolute();
      match_player_absolute();
   }
   target = match_result();

   if ( !Good_chk(target) || !isPlayer(target) || !Controls(player,target) ) {
      return -1;
   }
   
   i_ctime = i_clong = i_clast = i_ctotal = 0;
   i_clogout = 0;

   /* We do not want a parent get here */
   s_buff = atr_get(target, A_CONNINFO, &aowner, &aflags);
   if ( *s_buff ) {
      sscanf(s_buff, "%d %d %d %d %ld", &i_ctime, &i_clong, &i_clast, &i_ctotal, &i_clogout);
   }
   free_lbuf(s_buff);
   
   i_return = -1;

   switch(i_key) {
      case CONN_TIME:
         DESC_ITER_CONN(d) {
            if ( d->player == target ) {
               i_tmp = mudstate.now - d->connected_at;
               i_ctime += (int)i_tmp;
            }
         }   
         i_return =  i_ctime;
         break;
      case CONN_LONGEST:
         DESC_ITER_CONN(d) {
            if ( d->player == target ) {
               i_tmp = mudstate.now - d->connected_at;
               if ( i_tmp > i_clong ) {
                  i_clong = (int)i_tmp;
               }
            }
         }   
         i_return =  i_clong;
         break;
      case CONN_LAST:
         i_return = i_clast;
         break;
      case CONN_TOTAL:
         DESC_ITER_CONN(d) {
            if ( d->player == target ) {
               i_ctotal++;
            }
         }
         i_return = i_ctotal;
         break;
      case CONN_LOGOUT:
         i_return = i_clogout;
         break;
      default: /* Error handling */
         i_return = -1;
         break;
   } 
   return i_return;
}

