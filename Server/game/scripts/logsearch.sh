#!/bin/bash
if [ -z "$@" ]
then
   echo -n "Argument expected for logfile search"
   exit 1
fi
. ./mush.config
  lc_chk="$(/bin/grep -n "$@" $GAMENAME.gamelog)"
  if [ -z "${lc_chk}" ]
  then
     echo -n "nothing found."
  else
     echo "${lc_chk}"
  fi
