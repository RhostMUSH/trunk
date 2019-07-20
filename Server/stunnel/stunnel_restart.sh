#!/bin/sh
##############################################################################
# restart the pid process
##############################################################################
gl_pidfile=`grep ^pid stunnel.conf|awk '{print $3}'`
if [ ! -f ${gl_pidfile} ]
then
   echo "No PID file found to restart"
   exit 1
fi
gl_pid=`cat ${gl_pidfile}`
gl_chk=`ps -p ${gl_pid}|grep -c stunnel`
if [ ${gl_chk} -eq 0 ]
then
   echo "No matching PID for ${gl_pid} running stunnel"
   exit 1
fi
echo "Restartng stunnel process..."
kill -HUP ${gl_pid}
echo "Restarted."
