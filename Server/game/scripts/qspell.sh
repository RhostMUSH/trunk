#!/usr/bin/env bash
aspell --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "#-1 Speller is not installed."
   exit 0
else
   lc_words=$(echo "$@" | aspell -a | egrep -v "(^\*|^\@|^$)"|cut -d ' ' -f 2)
   for i in $@
   do
      if [ $(echo "${lc_words}"|grep -wc "$i") -gt 0 ]
      then
         lc_i="%ch%cr${i}%cn"
      else
         lc_i="${i}"
      fi
      if [ -z "$list" ]
      then
         list="${lc_i}"
      else
         list="${list} ${lc_i}"
      fi
   done
   echo -n "${list}"
fi
