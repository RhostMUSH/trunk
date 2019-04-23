#!/bin/sh
lynx --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "unable to process web entries at this time."
   exit 1
fi
export LANG=en_US
lynx -width=1000 --dump "http://$@"
