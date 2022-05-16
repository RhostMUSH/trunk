#!/usr/bin/env bash
export GREP_COLORS="ms=31:mc=31:sl=:cx=:fn=35:ln=32:bn=32:se=36" 
export gl_black="\o33\[30m"
export gl_red="\o33\[31m"
export gl_green="\o33\[32m"
export gl_yellow="\o33\[33m"
export gl_blue="\o33\[34m"
export gl_magenta="\o33\[35m"
export gl_cyan="\o33\[36m"
export gl_white="\o33\[37m"
export gl_normal="\o33\[0m"
export gl_normal2="\o33\[m"
export gl_erase="\o33\[K"
export gl_hilite="\o33\[1m"
if [ -z "$@" ]
then
   echo -n "Argument expected for logfile search"
   exit 1
fi
. ./mush.config
  lc_chk="$(/bin/grep --color=always -n -e "$@" $GAMENAME.gamelog)"
  lc_toolong=0
  if [ $(echo "${lc_chk}"|wc -l) -gt 100 ] 
  then
     lc_chk="$(echo "${lc_chk}"|tail -100)"
     lc_toolong=1
  fi
  if [ -z "${lc_chk}" ]
  then
     echo -n "nothing found."
  else
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_red}/%cr/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_green}/%cg/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_yellow}/%cy/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_blue}/%cb/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_magenta}/%cm/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_cyan}/%cc/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_white}/%cw/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_normal}/%cn/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_normal2}/%cn/g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_erase}//g")"
      lc_chk="$(echo "${lc_chk}"|sed -e "s/${gl_hilite}/%ch/g")"
      if [ ${lc_toolong} -gt 0 ]
      then
         echo "Warning: output exceeded 100 lines"
      fi
      echo "------------------------------------------------------------------------------"
      echo "${lc_chk}"
      echo "------------------------------------------------------------------------------"
  fi
