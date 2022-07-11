
/* senses.c -- commands which trigger the other 4 senses (look done) of things 
 * -- Ashen-Shugar
*/


#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "command.h"
#include "alloc.h"
#include "rhost_ansi.h"

void do_touch(dbref player, dbref cause, int key, char *name)
{
   dbref thing, loc;

   loc = Location(player);
   if (!name || !*name) {
      thing = loc;
      if (!Good_obj(thing)) {
         return;
      }
   }
   else {
      thing = player;
      init_match(thing, name, NOTYPE);
      match_exit_with_parents();
      match_neighbor();
      match_possession();
      if (Privilaged(player) || HasPriv(player, NOTHING, POWER_LONG_FINGERS, POWER3, NOTHING)) {
         match_absolute();
	 match_player();
         match_player_absolute();
      }
      match_here();
      match_me();
      match_master_exit();
      thing = match_result();
   
       /* Not found locally, check possessive */
   
      if (!Good_obj(thing)) {
         thing = match_status(player, match_possessed(player, player, (char *) name, thing, 0));
      }
   }
   if (!Good_obj(thing))
      return;
       
   if ((!Cloak(thing)) || (Cloak(thing) && SCloak(thing) && Immortal(player)) ||
      (Cloak(thing) && Wizard(player) && (!SCloak(thing) || Immortal(player)))) {
       did_it(player, thing, A_STOUCH, "You feel nothing of interest.", A_SOTOUCH, NULL, 
              A_SATOUCH, (char **) NULL, 0);
   } else
       notify(player, "I don't see that here.");
}

void do_taste(dbref player, dbref cause, int key, char *name)
{
   dbref thing, loc;

   loc = Location(player);
   if (!name || !*name) {
      thing = loc;
      if (!Good_obj(thing)) {
         return;
      }
   }
   else {
      thing = player;
      init_match(thing, name, NOTYPE);
      match_exit_with_parents();
      match_neighbor();
      match_possession();
      if (Privilaged(player) || HasPriv(player, NOTHING, POWER_LONG_FINGERS, POWER3, NOTHING)) {
         match_absolute();
	 match_player();
         match_player_absolute();
      }
      match_here();
      match_me();
      match_master_exit();
      thing = match_result();
   
       /* Not found locally, check possessive */
   
      if (!Good_obj(thing)) {
         thing = match_status(player, match_possessed(player, player, (char *) name, thing, 0));
      }
   }
   if (!Good_obj(thing))
      return;
       
   if ((!Cloak(thing)) || (Cloak(thing) && SCloak(thing) && Immortal(player)) ||
      (Cloak(thing) && Wizard(player) && (!SCloak(thing) || Immortal(player)))) {
       did_it(player, thing, A_STASTE, "You taste nothing of interest.", A_SOTASTE, NULL, 
              A_SATASTE, (char **) NULL, 0);
   } else
       notify(player, "I don't see that here.");
}

void do_listen(dbref player, dbref cause, int key, char *name)
{
   dbref thing, loc;

   loc = Location(player);
   if (!name || !*name) {
      thing = loc;
      if (!Good_obj(thing)) {
         return;
      }
   }
   else {
      thing = player;
      init_match(thing, name, NOTYPE);
      match_exit_with_parents();
      match_neighbor();
      match_possession();
      if (Privilaged(player) || HasPriv(player, NOTHING, POWER_LONG_FINGERS, POWER3, NOTHING)) {
         match_absolute();
	 match_player();
         match_player_absolute();
      }
      match_here();
      match_me();
      match_master_exit();
      thing = match_result();
   
       /* Not found locally, check possessive */
   
      if (!Good_obj(thing)) {
         thing = match_status(player, match_possessed(player, player, (char *) name, thing, 0));
      }
   }
   if (!Good_obj(thing))
      return;
       
   if ((!Cloak(thing)) || (Cloak(thing) && SCloak(thing) && Immortal(player)) ||
      (Cloak(thing) && Wizard(player) && (!SCloak(thing) || Immortal(player)))) {
       did_it(player, thing, A_SLISTEN, "You hear nothing of interest.", A_SOLISTEN, NULL, 
              A_SALISTEN, (char **) NULL, 0);
   } else
       notify(player, "I don't see that here.");
}

void do_smell(dbref player, dbref cause, int key, char *name)
{
   dbref thing, loc;

   loc = Location(player);
   if (!name || !*name) {
      thing = loc;
      if (!Good_obj(thing)) {
/*       notify(player, "Object was not good."); */
         return;
      }
   }
   else {
      thing = player;
      init_match(thing, name, NOTYPE);
      match_exit_with_parents();
      match_neighbor();
      match_possession();
      if (Privilaged(player) || HasPriv(player, NOTHING, POWER_LONG_FINGERS, POWER3, NOTHING)) {
         match_absolute();
	 match_player();
         match_player_absolute();
      }
      match_here();
      match_me();
      match_master_exit();
      thing = match_result();
   
       /* Not found locally, check possessive */
   
      if (!Good_obj(thing)) {
         thing = match_status(player, match_possessed(player, player, (char *) name, thing, 0));
      }
   }
   if (!Good_obj(thing)) {
/*    notify(player, "Object was not good #2."); */
      return;
   }
       
   if ((!Cloak(thing)) || (Cloak(thing) && SCloak(thing) && Immortal(player)) ||
      (Cloak(thing) && Wizard(player) && (!SCloak(thing) || Immortal(player)))) {
       did_it(player, thing, A_SSMELL, "You smell nothing of interest.", A_SOSMELL, NULL, 
              A_SASMELL, (char **) NULL, 0);
   } else
       notify(player, "I don't see that here.");
}

