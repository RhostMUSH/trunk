#!/bin/bash
echo "This will run the RhostMUSH database recovery tool."
echo "IT does this by recovering the data from a previous flatfile."
echo "Do you wish to proceed? (Y/N): "|tr -d '\012'
read ans
if [ "${ans}" != "y" -a "${ans}" != "Y" ]
then
   echo "Aborted by user."
   exit 1
fi
echo "1.  Finding and validating previous flatfiles..."
lc_found=0
if [ -f ./data/netrhost.db.flat  ]
then
   lc_x=$(tail -1 ./data/netrhost.db.flat|grep -c "END OF DUMP")
   if [ ${lc_x} -eq 1 ]
   then
      lc_file=./data/netrhost.db.flat
      lc_found=1
   fi
fi
if [ ${lc_found} -eq 0 ]
then
   if [ -f ./prevflat/netrhost.db.flat  ]
   then
      lc_x=$(tail -1 ./prevflat/netrhost.db.flat|grep -c "END OF DUMP")
      if [ ${lc_x} -eq 1 ]
      then
         lc_file=./prevflat/netrhost.db.flat
         lc_found=1
      fi
   fi
fi
if [ ${lc_found} -eq 0 ]
then
   if [ -f ./oldflat/RhostMUSH.dbflat1.tar.gz  ]
   then
      cd ./oldflat
      [[ ! -d ./tmp ]] && mkdir ./tmp
      cd ./tmp
      tar -xzf ../RhostMUSH.dbflat1.tar.gz
      cd ../..
      cp -f ./oldflat/tmp/data/netrhost.db.flat .
      rm -rf ./oldflat/tmp
      lc_x=$(tail -1 ./oldflat/netrhost.db.flat|grep -c "END OF DUMP")
      if [ ${lc_x} -eq 1 ]
      then
         lc_file=./oldflat/netrhost.db.flat
         lc_found=1
      fi
   fi
fi
if [ ${lc_found} -eq 0 ]
then
   echo "No valid flatfiles were found.  Please copy a flatfile to the 'prevflat' with the name 'netrhost.db.flat"
   exit 1
fi
echo "2.  Verifying if mush is running."
ps -p $(cat netrhost.pid) > /dev/null 2>&1
if [ $? -eq 0 ]
then
   echo "RhostMUSH is currently running"
   ps vux -p $(cat netrhost.pid)
   echo "Shutdown the mush? (Saying no will abort this script) Y/N: "|tr -d '\012'
   read ans
   if [ "${ans}" != "y" -a "${ans}" != "Y" ]
   then
      echo "Aborted by user."
      exit 1
   fi
   echo "--- Trying safe shutdown."
   kill -USR2 $(cat netrhost.pid)
   sleep 5
   ps -p $(cat netrhost.pid) > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      echo "--- Trying kill -TERM emergency shutdown."
      kill -TERM $(cat netrhost.pid)
      sleep 5
   fi
   ps -p $(cat netrhost.pid) > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      echo "--- Sorry, having to kill -9"
      kill -9 $(cat netrhost.pid)
      sleep 5
   fi
   ps -p $(cat netrhost.pid) > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      echo "Unable to kill mush process.  Aborting!"
      exit 1
   fi
fi
s_dir="$(date +"%m%d%Y%H%M%S")"
echo "3.  Moving old data files to ./data/backup_${s_dir}"
mkdir "./data/backup_${s_dir}" 2>/dev/null
if [ ! -d "./data/backup_${s_dir}" ]
then
   echo "Unable to create backup directory."
   exit 1
fi
mv -f ./data/netrhost.db* "./data/backup_${s_dir}"
mv -f ./data/netrhost.gdbm* "./data/backup_${s_dir}"
mv -f "./data/backup_${s_dir}/netrhost.db.flat" ./data
echo "4.  Restoring previous database.  Once up if mail is down, issue: wmail/load"
./db_load data/netrhost.gdbm ${lc_file} data/netrhost.db.new
if [ $? -ne 0 ]
then
   echo "Loading flatfile failed.  Aborting."
   exit 1
fi
echo "5.  Starting up mush."
./Startmush -f
