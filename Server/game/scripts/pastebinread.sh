#!/bin/sh
tst=$(lynx --version 2>&1|grep -c "not found")
if [ $tst -eq 0 ]
then
   lynx --dump "http://pastebin.com/$@"|egrep -B5000 "RAW Paste Data"|egrep -A5000 "raw\[.*get\[.*clone"|egrep -v "(RAW Paste Data|raw\[.*get\[.*clone)"
   #lynx --dump "http://pastebin.com/api_scrape_item.php?i=$@" -- uncomment if you use a PAID api with a whitelist IP
else
   echo "+pastebinread: not supported at this time."
fi
