#!/bin/sh
#
wget http://torstatus.blutmagie.de/ip_list_exit.php/Tor_ip_list_EXIT.csv
if [ $? -ne 0 ]
then
   wget http://torstatus.rueckgr.at/ip_list_exit.php/Tor_ip_list_EXIT.csv
fi
if [ -f Tor_ip_list_EXIT.csv ]
then
  rm -f blacklist.txt
  cat Tor_ip_list_EXIT.csv > blacklist.txt
fi

# Additional TorProject list.
# Checks which nodes can reach your IP. Accepts an optional &port=<port> param.

wget -q -O - "https://check.torproject.org/cgi-bin/TorBulkExitList.py?ip=`hostname -i|cut -f1 -d" "`" -U NoSuchBrowser/1.0 > bulk.txt
if [ -f bulk.txt ]
then
    cat bulk.txt |fgrep -v '#' > bulk2.txt
    if [ -f Tor_ip_list_EXIT.csv ]
    then
      cat bulk2.txt >> blacklist.txt
    else
      rm -f blacklist.txt
      cat bulk2.txt 2>/dev/null|sort -u > blacklist.txt
    fi
fi

cat hidemyass_static.txt >> blacklist.txt

wget -q -O - http://proxy-ip-list.com/download/free-proxy-list.txt|tr '\015' '\012'|egrep -v "(^$|^#)"|cut -f1 -d ":" >> blacklist.txt

# This is massive, and will grab a ton of sites
wget -q -O - http://www.shallalist.de/Downloads/shallalist.tar.gz|tar -zxvOf - BL/anonvpn/domains 2>/dev/null|grep -v "[A-Za-z]" >> blacklist.txt

cat blacklist.txt | sort -u > blacklist.tmp
cat blacklist.tmp > blacklist.txt

rm -f Tor_ip_list_EXIT.csv bulk.txt bulk2.txt blacklist.tmp
