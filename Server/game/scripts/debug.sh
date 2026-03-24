#!/bin/sh
gdb -h > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "GDB debugger is not installed."
   exit 1
fi

lc_core=`echo core*|awk '{print $1}'`
if [ -z "${lc_core}" ]
then
   echo "No core file found."
   exit 1
fi

echo "where"|gdb -c ${lc_core} netrhost
