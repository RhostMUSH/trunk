#!/usr/bin/env bash
tst=$(lynx --version 2>&1|grep -c "not found")
if [ $tst -eq 0 ]
then
   lynx -width=1000 --dump "http://pastebin.com/$@"|egrep -B5000 "RAW Paste Data"|egrep -A5000 "raw\[.*clone"|egrep -v "(RAW Paste Data|raw\[.*clone)"
   #lynx --dump "http://pastebin.com/api_scrape_item.php?i=$@" -- uncomment if you use a PAID api with a whitelist IP
else
   echo "+pastebinread: not supported at this time."
fi
