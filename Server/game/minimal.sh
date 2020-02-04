#!/bin/sh
echo "This will copy in the minimal_db and prep for your mush."
echo "Do you wish to continue? (Y/N): "|tr -d '\012'
read ans
if [ "$ans" != "Y" -a "$ans" != "y" ]
then
   echo "Aborted by user."
   exit 1
fi

if [ -f "netrhost.pid" ]
then
   ps -p `cat netrhost.pid`> /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      echo "You have a mush running.  You must shut it down to copy in the new database."
      echo "Shut it down? (Y/N): "|tr -d '\012'
      read ans
      if [ "$ans" == "Y" -o "$ans" == "y" ]
      then
         kill -USR2 `cat netrhost.pid`
         sleep 5
         ps -p `cat netrhost.pid`> /dev/null 2>&1
         if [ $? -eq 0 ]
         then
            echo "Did not shutdown cleanly.  Attempting TERM."
            kill -TERM `cat netrhost.pid`
            sleep 5
            ps -p `cat netrhost.pid`> /dev/null 2>&1
            if [ $? -eq 0 ]
            then
               echo "Process was not killed with TERM.  Please shutdown your mush then rerun script."
               exit 1
            fi
         fi
      else
         echo "Aborted by user."
         exit 1
      fi
      echo "Mush has been shutdown."
   fi
fi
   
ls ./data/netrhost.gdbm* > /dev/null 2>&1
if [ $? -eq 0 ]
then
   echo "You currently have an active game and database."
   echo "Do you wish to overwrite this? (Y/N): "|tr -d '\012'
   read ans
   if [ "$ans" != "Y" -a "$ans" != "y" ]
   then
      echo "Aborted by user."
      exit 1
   fi
   mkdir ./data/minimal 2>/dev/null
   if [ ! -d ./data/minimal ]
   then
      echo "Unable to make directory ./data/minimal.  Aborting"
      exit 1
   fi
   mv -f ./data/netrhost.gdbm* data/minimal
   mv -f ./data/netrhost.db* data/minimal
fi
echo "Backing up original netrhost.conf and copying new one over..."
mv -f netrhost.conf netrhost.conf.orig
cp -f ../minimal-DBs/minimal_db/netrhost.conf .
echo "Loading in flatfile..."
./db_load data/netrhost.gdbm ../minimal-DBs/minimal_db/netrhost.db.flat data/netrhost.db.new
echo "Loading in customized txt files to allign with the database and indexing them..."
curr=`pwd`
cd ../minimal-DBs/minimal_db/txt/
for i in *.txt
do
   cp -f $i $curr/txt
   xxx=`echo $i|cut -f1 -d"."`
   $curr/mkindx $curr/txt/$i $curr/txt/$xxx.indx
done
echo "All information loaded.  Please modify port, debug_id, and any other information in netrhost.conf now."
exit 0

