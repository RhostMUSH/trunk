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
