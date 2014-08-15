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
   if [ $? -ne 0 ]
   then
      wget http://torstatus.blutmagie.de/ip_list_exit.php/Tor_ip_list_EXIT.csv
   fi
fi

# Additional TorProject list. Needs you to specify your IP before uncommenting!
# Checks which nodes can reach your IP. Accepts an optional &port=<port> param.
#wget -q -O - "https://check.torproject.org/cgi-bin/TorBulkExitList.py?ip=YOUR.IP.ADDRESS.HERE" -U NoSuchBrowser/1.0 > bulk.txt
#cat bulk.txt |fgrep -v '#' > bulk2.txt

# combine them
cat blacklist.tmp blacklist.tmp2 Tor_ip_list_EXIT.csv bulk2.txt 2>/dev/null|sort -u > blacklist.txt

rm -f blacklist.tmp blacklist.tmp2 proxy.txt Tor_ip_list_EXIT.csv bulk.txt bulk2.txt
