#!/bin/bash
lynx --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "unable to run process to pull weather."
   exit 1
fi
if [ -z "$1" ]
then
   echo "Argument expected"
   exit 1
fi
gl_arg=$(echo "$1"|tr 'A-Z' 'a-z')
export LANG=en_US
if [ "${gl_arg}" = "moon" ]
then
   lynx --dump wttr.in/moon|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'|head -23
#  curl -B wttr.in/Moon|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'
else
   lynx --dump wttr.in/$1?0|perl -MTerm::ANSIColor=colorstrip -ne 'print colorstrip($_)'|head -8
fi
