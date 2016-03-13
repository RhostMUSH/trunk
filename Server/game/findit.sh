#!/bin/sh
if [ -z "$1" ]
then
   exit
fi
chk=`tail -2 $1|grep -c "^<"`
if [ "$chk" -eq 0 ]
then
   exit
fi
line=`tail -1 $1`
head=`head -1 $1`
if [ "${line}" = "***END OF DUMP***" -a "${head}" = "+V74247" ]
then
   echo "$1"
fi
