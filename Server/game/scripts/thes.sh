#!/usr/bin/env bash
if [ -f /usr/bin/wn ]
then
   if [ -z "$@" ]
   then
      echo "Define what?"
   else
      /usr/bin/wn "$@" -coorn 2>&1
   fi
else
   echo "Dictionary is offline."
fi
