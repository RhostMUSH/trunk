#!/usr/bin/env bash
x="$@"
if [ -z "$x" ]
then
   git rev-parse HEAD 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "Not a git repo."
   fi
elif [ "$x" = "short" ]
then
   git rev-parse --short HEAD
   if [ $? -ne 0 ]
   then
      echo "Not a git repo."
   fi
else
   git rev-parse HEAD
   if [ $? -ne 0 ]
   then
      echo "Not a git repo."
   fi
fi
