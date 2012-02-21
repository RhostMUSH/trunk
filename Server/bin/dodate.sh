#!/bin/sh
CHKD=`date -R 2>&1`
CHK=`echo ${CHKD}|grep -ic invalid`
if [ $CHK -eq 0 ]
then
   CHK=`echo ${CHKD}|grep -ic recog`
fi
if [ $CHK -eq 0 ]
then
   CHK=`echo ${CHKD}|grep -ic usage`
fi
if [ $CHK -gt 0 ]
then
   date > date.txt
else
   date -R > date.txt
fi
