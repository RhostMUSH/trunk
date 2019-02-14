#!/bin/sh
##############################################################################
# kill the stunnel process
##############################################################################
gl_pidfile=`grep ^pid stunnel.conf|awk '{print $3}'`
if [ ! -f ${gl_pidfile} ]
then
   echo "No PID file found to kill."
   exit 1
fi
gl_pid=`cat ${gl_pidfile}`
gl_chk=`ps -p ${gl_pid}|grep -c stunnel`
if [ ${gl_chk} -eq 0 ]
then
   echo "No matching PID for ${gl_pid} running stunnel"
   exit 1
fi
echo "Attempting to kill process gracefully..."
kill ${gl_pid}
sleep 2
gl_chk=`ps -p ${gl_pid}|grep -c stunnel`
if [ ${gl_chk} -ne 0 ]
then
   echo "Attempting to kill process not so gracefully..."
   kill -TERM ${gl_pid}
   sleep 2
   gl_chk=`ps -p ${gl_pid}|grep -c stunnel`
   if [ ${gl_chk} -ne 0 ]
   then
      echo "Glassing the process and nuking from orbit..."
      kill -9 ${gl_pid}
      sleep 2
   fi
fi
gl_chk=`ps -p ${gl_pid}|grep -c stunnel`
if [ ${gl_chk} -ne 0 ]
then
   echo "Warning:  We could NOT stop the stunnel process ${gl_pid}"
   echo "This is what I see for the process, manual intervention required."
   ps -p ${gl_pid}
else
   echo "Process has been killed."
fi
