#!/bin/bash
lynx --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "unable to run process to pull weather."
   exit 1
fi
export LANG=en_US
lynx -width=1000 --dump wttr.in/$1|grep -e "^[+|]"|sed "s/mph /mph/g"|sed "s/\.\.\./ /g"
