#!/bin/bash
if [ -z "$1" ]
then
   echo "Argument expected"
   exit 1
fi
gl_arg=$(echo "$1"|tr 'A-Z' 'a-z')
if [ "${gl_arg}" = "moon" ]
then
   lynx --dump wttr.in/moon|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'|head -23
#  curl -B wttr.in/Moon|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'
else
   lynx --dump wttr.in/$1?0|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'|head -8
fi
