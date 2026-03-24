#!/usr/bin/env bash
curl --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "unable to run process to pull weather."
   exit 1
fi
export LANG=en_US
curl wttr.in/$1 2>/dev/null|grep -e "^[+|]"|sed "s/mph /mph/g"|sed "s/\.\.\./ /g"
