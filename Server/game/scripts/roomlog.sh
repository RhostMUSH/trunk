#!/bin/bash
if [ -z "$@" ]
then
   echo -e "Which room log do you wish to view?"
   cntr=0
   cd roomlogs
   for i in $(ls room*)
   do
      lc_lines="$(wc -l $i|awk '{print $1}')"
      lc_pages="$(expr ${lc_lines} / 20 + 1)"
      if [ $(expr $cntr % 3) -eq 0 ]
      then
         printf "\n%-18s [%d pages]" "$(echo "$i"|cut -f1 -d".")" "${lc_pages}"
      else
         printf "n%-18s [%d pages]" "$(echo "$i"|cut -f1 -d".")" "${lc_pages}"
      fi
   done
else
   lc_file=$(echo "$@"|awk '{print $1}')
   lc_page=$(echo "$@"|awk '{print $2}')
   if [ -z "${lc_page}" ]
   then
      lc_page=1
   fi
   lc_chk="$(echo "${lc_page}"|tr -cd '[:digit:]')"
   if [ "${lc_chk}" != "${lc_page}" ]
   then
      lc_page=1
   fi
   lc_chk="$(echo "${lc_file}"|grep -c "^room")"
   if [ "${lc_chk}" -eq 0 ]
   then
      echo -e "Invalid room log name."
   elif [ ! -f "roomlogs/${lc_file}.log" ]
   then
      echo -e "Invalid room log name."
   else
      ((lc_pagestart=((${lc_page}-1)*20)+1))
      ((lc_pageend=${lc_pagestart}+20))
      echo "------------------------------------------------------------------------------"
      sed -n "${lc_pagestart},${lc_pageend}p" roomlogs/${lc_file}.log
      echo -e "------------------------------------------------------------------------------"
   fi
fi
