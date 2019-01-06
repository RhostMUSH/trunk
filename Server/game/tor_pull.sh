#!/bin/sh
#
wget http://torstatus.blutmagie.de/ip_list_exit.php/Tor_ip_list_EXIT.csv
if [ $? -ne 0 ]
then
   wget http://torstatus.rueckgr.at/ip_list_exit.php/Tor_ip_list_EXIT.csv
fi
if [ -f Tor_ip_list_EXIT.csv ]
then
  mv -f blacklist.txt blacklist.tor
  cat Tor_ip_list_EXIT.csv blacklist.tor 2>/dev/null| sort -u > blacklist.txt
  rm -f blacklist.tor
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
      mv -f blacklist.txt blacklist.tor
      cat bulk2.txt blacklist.tor 2>/dev/null|sort -u > blacklist.txt
      rm -f blacklist.tor
    fi
fi

cat hidemyass_static.txt >> blacklist.txt
cat freeproxy.txt >> blacklist.txt
cat rebroproxy.txt >> blacklist.txt

wget -q -O - https://free-proxy-list.net/|sed "s/<td>/\n/g"|sed "s/<\/td>/\n/g"|grep "^[1-9].*\." >> blacklist.txt
wget -q -O - http://proxy-ip-list.com/download/free-proxy-list.txt|tr '\015' '\012'|egrep -v "(^$|^#)"|cut -f1 -d ":" >> blacklist.txt

# This is massive, and will grab a ton of sites
wget -q -O - http://www.shallalist.de/Downloads/shallalist.tar.gz|tar -zxvOf - BL/anonvpn/domains 2>/dev/null|grep -v "[A-Za-z]" >> blacklist.txt

# This will grab the ninja free lists
wget -q -O - https://freevpn.ninja/free-proxy/txt|cut -f1 -d":"|grep -v sock >> blacklist.txt

# daily proxy lists
year=$(date +%Y)
mon=$(date +%m)
day=$(date +%d)
curl http://proxy-daily.com/${year}/${mon}/${day}-${mon}-${year}-proxy-list/ 2>/dev/null|sed 's/> /\n/g'|sed 's/</\n/g'|grep ^[0-9]|grep ':'|cut -f1 -d":" >> blacklist.txt

./proxysnarf.sh
cat blacklist.txt temp.txt| sort -u > blacklist.tmp
cat blacklist.tmp > blacklist.txt

rm -f Tor_ip_list_EXIT.csv bulk.txt bulk2.txt blacklist.tmp
