#!/bin/sh
#
if [ -f Tor_ip_list_EXIT.csv ]
then
   cat /dev/null > blacklist.tmp
   touch blacklist.txt
   while read line
   do
      xx=`grep -c ${line} Tor_ip_list_EXIT.csv`
      if [ ${xx} -eq 0 ]
      then
         echo "${line}" >> blacklist.tmp
      fi
   done < blacklist.txt
   rm -f Tor_ip_list_EXIT.csv
else
   cp -f blacklist.txt blacklist.tmp
fi
wget http://torstatus.kgprog.com/ip_list_exit.php/Tor_ip_list_EXIT.csv
if [ $? -ne 0 ]
then
   wget http://torstatus.blutmagie.de/ip_list_exit.php/Tor_ip_list_EXIT.csv
fi
cat blacklist.tmp Tor_ip_list_EXIT.csv 2>/dev/null|sort -u > blacklist.txt
rm -f blacklist.tmp
