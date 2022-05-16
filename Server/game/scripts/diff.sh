#!/usr/bin/env bash
wdiff --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "#-1 DIFF UTILITY NOT AVAILABLE"
   exit 0
fi
lc_date=$(date "+%s")
lc_arg1=$(echo "$1"|awk -F '|' '{print $1}')
lc_arg2=$(echo "$1"|awk -F '|' '{print $2}')
if [ -z "${lc_arg1}" -o -z "${lc_arg2}" ]
then
   echo "#-1 REQUIRES TWO ARGUMENTS"
   exit 0
fi
echo "${lc_arg1}" > /tmp/x$$x${lc_date}
echo "${lc_arg2}" > /tmp/y$$y${lc_date}
wdiff -w "[-%ch%cr" -x "%cn-]" -y "[+%ch%cg" -z "%cn+]" /tmp/x$$x${lc_date} /tmp/y$$y${lc_date}
rm -f /tmp/x$$x${lc_date} /tmp/y$$y${lc_date}
