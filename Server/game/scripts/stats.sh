#!/bin/bash
echo "-----------------------------------------------------------------------------"
ps aux > /dev/null 2>&1
if [ $? -eq 0 ]
then
   psarg="aux"
else
   ps -aux > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      psarg=""
   else
      psarg="-aux"
   fi
fi
ps ${psarg}|head -1
if [ -f netrhost.pid ]
then
   ps ${psarg}|grep $(cat netrhost.pid)|grep netrhost
elif [ -f ../netrhost.pid ]
then
   ps ${psarg}|grep $(cat ../netrhost.pid)|grep netrhost
else
   ps ${psarg}|grep netrhost
fi
echo "-----------------------------------------------------------------------------"
uptime
echo "-----------------------------------------------------------------------------"
free > /dev/null 2>&1
if [ $? -eq 0 ]
then
   free -l
else
   top -n 1|grep -iE "(^mem|^swap)"
fi
echo "-----------------------------------------------------------------------------"
df -h .
echo "-----------------------------------------------------------------------------"
xx=$(quota -s)
if [ -n "$xx" ]
then
   echo "$xx"
   echo "-----------------------------------------------------------------------------"
fi
