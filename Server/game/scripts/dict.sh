#!/bin/bash
if [ -f /usr/bin/wn ]
then
   /usr/bin/wn "$@" -over 2>&1
else
   echo "Dictionary is offline."
fi
