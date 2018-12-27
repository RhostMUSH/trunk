#!/bin/sh
tst=$(pastebinit -h 2>&1|grep -c "not found")
if [ $tst -gt 0 ]
then
   echo "+pastebinwrite: not currently supported."
   exit 1
fi
if [ -n "$1" ]
then
   lc_tst=$(echo "$1"|sed 's/@@@/\r\n/g')
   echo "${lc_tst}"|pastebinit -a SW:DoD -b http://pastebin.com
else
   echo "Push what text?"
fi

