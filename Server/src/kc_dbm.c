/* 
 * Kyoto Cabinet DBM wrapper for RhostMUSH
 * 
 * By Odin@RhostMUSH
 *
 */
 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include "kc_dbm.h"

KCDB * dbm_open(char *file, int flags, int mode){
  struct statb;
  register KCDB *db;
  int kcdflags;
  
  /* First, transform NDBM flags into KCDB flag equivalents.
   *
   *
   */
   
 } 