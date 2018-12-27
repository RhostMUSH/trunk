#!/bin/sh
##############################################################################
# this script will go through all directories that contain a valid 
# netrhost.pid and kill -HUP the process which will automatically make
# a flatfile dump in the latest RhostMUSH 4.0.0p2 version (and later).
#
# You may call this in a crontab.
#
# The flatfile it will create will be in the data directory and
# will be named:
#                  netrhost.SIGHUP.flat
##############################################################################
for i in `/bin/ls -d *`
do
   if [ -f "$i/netrhost.pid" ]
   then
      /bin/ps -p `/bin/cat $i/netrhost.pid` > /dev/null 2>&1
      if [ $? -eq 0 ]
      then
         /bin/kill -HUP `/bin/cat $i/netrhost.pid` > /dev/null 2>&1
      fi
   fi
done
