#!/bin/sh
xx=0
od --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo -n "#-1 SUPPORTING FUNCTIONS NOT AVAILABLE"
   exit 1
fi
if [ -e /dev/urandom ]
then
   xx=`od -vAn -N8 -tu8 < /dev/urandom`
else
   xx=`od -vAn -N8 -tu8 < /dev/random`
fi
if [ -n "$@" ]
then
   bc --version > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      yy=`expr $@ + 0 2>/dev/null`
      if [ -n "$yy" -a "$yy" != "0" ]
      then
         xx=`echo "$xx % $yy"|bc`
      fi
   fi
fi
echo -n $xx
