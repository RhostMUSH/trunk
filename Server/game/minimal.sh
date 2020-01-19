#!/bin/sh
echo "This will copy in the minimal_db and prep for your mush."
echo "Do you wish to continue? (Y/N): "|tr -d '\012'
read ans
if [ "$ans" != "Y" -a "$ans" != "y" ]
then
   echo "Aborted by user."
   exit 1
fi
ls data/netrhost.gdbm* > /dev/null 2>&1
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
   mkdir -f data/minimal 2>/dev/null
   mv -f data/netrhost.gdbm* data/minimal
   mv -f data/netrhost.db* data/minimal
fi
echo "Backing up original netrhost.conf and copying new one over..."
mv -f netrhost.conf netrhost.conf.orig
cp -f ../minimal-DBs/minimal_db/netrhost.conf .
echo "Loading in flatfile..."
./db_load data/netrhost.gdbm ../minimal-DBs/minimal_db/netrhost.db.flat data/netrhost.db.new
echo "All information loaded.  Please modify port, debug_id, and any other information in netrhost.conf now."
exit 0

