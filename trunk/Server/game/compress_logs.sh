#!/bin/sh
################
compressor=gzip
###############
if [ ! -d oldlogs ]
then
   echo "Oldlogs directory does not exist.  Aborting."
   exit 1
fi
. ./mush.config
if [ -z "${GAMENAME}" ]
then
   echo "Unable to identify game name.  Make sure GAMENAME is defined in mush.config."
   exit 1
fi
ls oldlogs/${GAMENAME}.gamelog.???????????? >/dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "No files to compress."
   exit 0
fi
echo "Compressing files"
for i in oldlogs/${GAMENAME}.gamelog.????????????
do
   echo "--> compressing $i..."|tr -d '\012'
   gzip $i >/dev/null 2>&1
   echo "done."
done
echo "Completed."
