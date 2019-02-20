#!/bin/sh
if [ -f ./stunnel ]
then
   ./stunnel stunnel.conf > /dev/null 2>&1
   retcode=$?
else
   stunnel stunnel.conf > /dev/null 2>&1
   retcode=$?
fi
if [ $retcode -eq 0 ]
then
   echo "stunnel SSL layer has started."
else
   echo "Error starting stunnel.  Error code $retcode"
fi
