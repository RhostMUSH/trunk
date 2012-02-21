#!/bin/sh
#############################################################################
# Filename: backup_flat.sh
# Version : 2.3
# Author  : Ashen-Shugar (c) 1998-1999, all rights reserved.
# Description:
#              Script used to backup flat files and archive them
#              Allows possible remote mirroring and auto-kill
#              of previously running script.
#
# Updates:     modify with -s argument for singular run entities (for cron)
#              if you use the '-s' singlular run switch, you must change
#              directory to the game directory prior to running the script.
#              The script assumes local directory it's issued from.
#
#              Add ability to send mail attachments (with supported mail prg)
#              Fix for mail attachment ;)
#
################################ NOTE #######################################
# You must have a MUSH process do a @dump/flat to make on-line flatfiles.
# If you want news, have it do a newsdb/unload
# If you want mail, have it do a wmail/unload
#
# Make sure in the mush, you archive the flatfiles by these:
# --- @dump/flat -- full mush flatfile
# --- wmail/unload -- unload mail db
# --- newsdb/unload -- unload news db
# You can run these by a @tr system.
# ie: @startup Obj=@wait 500=@tr me/back
#     &BACK Obj=@dump/flat;wmail/unload;newsdb/unload;@wait 80000=@tr me/back
#############################################################################

#############################################################################
# INIT_RCP - initialize the RCP/SCP and/or NCFTPPUT remote archivers
#----------------------------------------------------------------------------
# RCP - path and binary to use for rcp/scp copy
# NCFTP - path and binary to use for ncftpput
# NCFTPFILE - filename that is used to store login/password info for ncftpput
#             We force this file user read only for safty
#             You need to edit this file with your login/password/host info!
# EMAIL - Login/password combination to use for rcp/scp backups
# BKUPPATH - remote path you wish backups to go
# ENABLEREMOTE - toggle to enable/disable remote backup and specify type
#                0 - none, 1 - rcp/scp, 2 - ncftp, 3 - nail (or other mailprg)
# MAILPROG - mail program to use that accepts attachment
# MAILATTACH - switch used for mail program to do attachments
# MAILSUBJ - subject for the mail system
# MAILTO - where to send the mail to
# MAILMSG - message you want included in any mail with the logs
#############################################################################
init_rcp() {
   RCP=/usr/bin/scp	
   EMAIL=none@none.com
   BKUPPATH=/home/none
   NCFTP=/usr/bin/ncftpput
   NCFTPFILE=./.rfile
   chmod 400 ${NCFTPFILE} 2>/dev/null
   ENABLEREMOTE=0
   MAILPROG=nail
   MAILATTACH="-a"
   MAILSUBJ="RhostMUSH Backup"
   MAILTO="$LOGNAME@localhost"
   MAILMSG="The backup was successful"
}
#############################################################################

#############################################################################
# INIT_LOGGING - initialize error logging facilities
#----------------------------------------------------------------------------
# ERRMAIL - the user that receives failed backup msgs (any email address allowed)
# MAILPROG - the mail program that is used to email
#############################################################################
init_logging() {
   ERRMAIL=${LOGNAME}
   ERRMAILPROG=mail		
}
#############################################################################

#############################################################################
# INIT_DATAPATHS - initialize the data paths for the backups
#----------------------------------------------------------------------------
# DATA - data directory
# CURDIR = current working directory
# OLDFLAT - old archives are stored here
# PREVFLAT - the last previous archive is stored here
# MAILNEWSDB - The mail and news db names (taken from muddb_name)
# MAINDBNAME - The main database name (taken from input_database)
#############################################################################
init_datapaths() {
   DATA=./data/		
   CURDIR=$(pwd)		
   OLDFLAT=./oldflat       
   PREVFLAT=./prevflat     
   MAILNEWSDB=$(grep "^muddb_name" netrhost.conf|awk '{printf("%s",substr($0,index($0,$2)))}')
   MAILNEWSDB=$(echo ${MAILNEWSDB})
   MAINDBNAME=$(grep "^input_database" netrhost.conf|awk '{printf("%s",substr($0,index($0,$2)))}'|cut -f1 -d".")
   MAINDBNAME=$(echo ${MAINDBNAME})
   if [ -z "${MAILNEWSDB}" ]
   then
      MAILNEWSDB="RhostMUSH"
   fi
   if [ -z "${MAINDBNAME}" ]
   then
      MAINDBNAME="netrhost"
   fi
}
#############################################################################

#############################################################################
# INIT_VARS - initialize all the variables
#----------------------------------------------------------------------------
# MAXBACKUPS - total number of backups you desire
# SLEEPTIME - time in seconds you wish to wait between backups
# COMPREXE - compression program 
# COMPREXT - compression extension to use
# DOFULL - "yes" or "no" to specify full backups
# DOARCH - "yes" puts the most recient archive in 'prevflat' if desired
# TXTFILES - all your .txt files you wish archived
# IMPORTANTFILES - any other file other than .txt files you want included
#############################################################################
init_vars() {
   MAXBACKUPS=7
   SLEEPTIME=43200
   COMPREXE=gzip
   COMPREXT=gz
   DOFULL="yes"
   DOARCH="yes"
   TXTFILES="txt/areghost.txt txt/autoreg.txt txt/autoreg_include.txt \
             txt/badsite.txt txt/connect.txt txt/create_reg.txt \
             txt/doorconf.txt txt/down.txt txt/error.txt txt/full.txt \
             txt/guest.txt txt/motd.txt txt/news.txt txt/newuser.txt \
             txt/noguest.txt txt/quit.txt txt/register.txt txt/wizmotd.txt \
             txt/plushelp.txt"
   IMPORTANTFILES="netrhost.conf"
}
#############################################################################

#############################################################################
# CHECK_PATHS - check if the backup directories are configured right
#############################################################################
check_paths() {
   if [ ! -d "$OLDFLAT" ]
   then
      echo "Directory specified by \$OLDFLAT : $CURDIR/$(basename $OLDFLAT)"\
           "does not exist."
      exit 1
   fi
   if [ ! -d "$PREVFLAT" ]
   then
      echo "Directory specified by \$PREVFLAT : $CURDIR/$(basename $PREVFLAT)"\
           "does not exist."
      exit 1
   fi
}
#############################################################################

#############################################################################
# KILL_OLD - kill the old processes if they're running and recreate killer
#############################################################################
kill_old() {
   if [ "$1" != "-s" ]
   then
      ./kill_back.sh 2>/dev/null 1>&2
      ./kill_sleep.sh 2>/dev/null 1>&2
      echo "kill -9 $$ 2>/dev/null 1>&2" > ./kill_back.sh
      echo "" > ./kill_sleep.sh
      chmod 700 ./kill_back.sh
   fi
}
#############################################################################

#############################################################################
# CHECK_FOR_FLAT - check if the flatfile exists and is valid
#                  if flatfile is not complete it waits for 5 minutes
#############################################################################
check_for_flat() {
   ZCNT=0;
   ZCNTCHK=1;
   while [ ${ZCNTCHK} -eq 1 ]
   do
      ZTAIL=$(tail -1 ./data/"${MAINDBNAME}".db.flat 2>/dev/null)
      ZCHK=$(echo "${ZTAIL}"|grep -c "***END OF DUMP***")
      if [ ${ZCHK} -eq 0 ]
      then
         if [ "$1" = "-s" ]
         then
            echo "File ./data/${MAINDBNAME}.db.flat does not exist."
            ZCNTCHK=0
         else
            sleep 60
            ZCNT=$(expr $ZCNT + 1)
         fi
      else
         ZCNT=0
         ZCNTCHK=0
      fi
      if [ ${ZCNT} -gt 5 ]
      then
         ZCNTCHK=0
      fi
   done
}
#############################################################################

#############################################################################
# DO_FULL_BACKUP - do a full backup with all files
#############################################################################
do_full_backup() {
         CNTR=${MAXBACKUPS}
         while [ ${CNTR} -ne 0 ]
         do
            NEWCNTR=$(expr $CNTR + 1)
            test -f ${OLDFLAT}/"${MAILNEWSDB}".dbflat${CNTR}.tar.gz && \
                    mv -f ${OLDFLAT}/"${MAILNEWSDB}".dbflat${CNTR}.tar.gz \
                             ${OLDFLAT}/"${MAILNEWSDB}".dbflat${NEWCNTR}.tar.gz
            CNTR=$(expr $CNTR - 1)
         done
         tar -cvf $OLDFLAT/"${MAILNEWSDB}".dbflat1.tar ./data/"${MAINDBNAME}".db.flat \
                  ./data/"${MAILNEWSDB}".news.flat ./data/"${MAILNEWSDB}".dump.mail \
                  ./data/"${MAILNEWSDB}".areg.dump  ./data/"${MAILNEWSDB}".dump.folder \
                  ${TXTFILES} ${IMPORTANTFILES} 2>/dev/null 1>&2
         ${COMPREXE} -f ${OLDFLAT}/"${MAILNEWSDB}".dbflat1.tar
         if [ "${DOARCH}" = "yes" ]
         then
            mv -f ./data/"${MAINDBNAME}".db.flat ${PREVFLAT} 2>/dev/null
            mv -f ./data/"${MAILNEWSDB}".news.flat ${PREVFLAT} 2>/dev/null
            mv -f ./data/"${MAILNEWSDB}".dump.mail ${PREVFLAT} 2>/dev/null
            mv -f ./data/"${MAILNEWSDB}".dump.folder ${PREVFLAT} 2>/dev/null
            mv -f ./data/"${MAILNEWSDB}".areg.dump ${PREVFLAT} 2>/dev/null
         fi
}
#############################################################################

#############################################################################
# DO_PARTIAL_BACKUP - do partial (only db) backup
#############################################################################
do_partial_backup() {
         CNTR=${MAXBACKUPS}
         while [ ${CNTR} -ne 0 ]
         do
            NEWCNTR=$(expr $CNTR + 1)
            test -f ${OLDFLAT}/"${MAILNEWSDB}".flat.${CNTR}.gz && \
                  mv -f ${OLDFLAT}/"${MAILNEWSDB}".flat.${CNTR}.gz \
                            ${OLDFLAT}/"${MAILNEWSDB}".flat.${NEWCNTR}.gz
            CNTR=$(expr ${CNTR} - 1)
         done
         mv -f ./data/"${MAINDBNAME}".db.flat ${OLDFLAT}/"${MAILNEWSDB}".flat.1 2>/dev/null
         ${COMPREXE} -f ${OLDFLAT}/"${MAILNEWSDB}".flat.1
}
#############################################################################

#############################################################################
# DO_ERROR - something bad happened.  Report it.
#############################################################################
do_error() {
   case $1 in
      1) echo "The flatfile ./data/${MAINDBNAME}.db.flat is incomplete : $(date)."|${ERRMAILPROG} -s "RhostMUSH Backup Failure." ${ERRMAIL}
         ;;
      *) echo "Unrecognized error occured at $(date)"|${ERRMAILPROG} -s "RhostMUSH Backup Failure." ${ERRMAIL}
         ;;
   esac
}
#############################################################################

#############################################################################
# INITIALIZE_KILL - initialize the kill script
#############################################################################
initialize_kill() {
   echo "GRP=\`ps -l|grep ${MYPPID}|grep -c sleep\`" > ./kill_sleep.sh
   echo "if [ \"\$GRP\" -ne 0 ]" >> ./kill_sleep.sh
   echo "then" >> ./kill_sleep.sh
   echo "   kill -9 ${MYPPID} 2>/dev/null 1>&2" >> ./kill_sleep.sh
   echo "fi" >> ./kill_sleep.sh
   chmod 700 ./kill_sleep.sh
}
#############################################################################

#############################################################################
# DO_REMOTE_COPY - handle remote scp/rcp/other
############################################################################# 
do_remote_copy() {
   if [ ! -z "${DOFULL}" ]
   then
      if [ ${ENABLEREMOTE} -eq 1 ]
      then
         ${RCP} ${OLDFLAT}/"${MAILNEWSDB}".dbflat1.tar.${COMPREXT} ${EMAIL}:${BKUPPATH}
      elif [ ${ENABLEREMOTE} -eq 2 ]
      then
         ${NCFTP} -f ${NCFTPFILE} ${BKUPPATH} ${OLDFLAT}/"${MAILNEWSDB}".dbflat1.tar.${COMPREXT}
      elif [ ${ENABLEREMOTE} -eq 3 ]
      then
         echo "${MAILMSG}"|${MAILPROG} -s "${MAILSUBJ}" ${MAILATTACH} ${OLDFLAT}/"${MAILNEWSDB}".dbflat1.tar.${COMPREXT} ${MAILTO}
      else
         echo "" > /dev/null
      fi
   else
      if [ ${ENABLEREMOTE} -eq 1 ]
      then
         ${RCP} ${OLDFLAT}/"${MAILNEWSDB}".flat.1.${COMPREXT} ${EMAIL}:${BKUPPATH}
      elif [ ${ENABLEREMOTE} -eq 2 ]
      then
         ${NCFTP} -f ${NCFTPFILE} ${BKUPPATH} ${OLDFLAT}/"${MAILNEWSDB}".flat.1.${COMPREXT}
      elif [ ${ENABLEREMOTE} -eq 3 ]
      then
         echo "${MAILMSG}"|${MAILPROG} -s "${MAILSUBJ}" ${MAILATTACH} ${OLDFLAT}/"${MAILNEWSDB}".flat.1.${COMPREXT} ${MAILTO}
      else
         echo "" > /dev/null
      fi
   fi
}
#############################################################################

#############################################################################
# INITIALIZE - initialize variables and call all subset initializers
#############################################################################
initialize() {
   init_rcp $1
   init_logging $1
   init_datapaths $1
   init_vars $1
   check_paths $1
}
#############################################################################

#############################################################################
# MAIN - run the main process
#############################################################################
main() {
# Loop forever
MAIN_LOOP=1
while [ ${MAIN_LOOP} -eq 1 ]
do
   if [ "$1" = "-s" ]
   then
      MAIN_LOOP=0
   fi
   check_for_flat $1
   if [ -f ./data/"${MAINDBNAME}".db.flat -a ${ZCNT} -eq 0 ]
   then
      if [ ! -z "${DOFULL}" ]
      then
         do_full_backup $1
      else
         do_partial_backup $1
      fi
      if [ ${ENABLEREMOTE} -ne 0 ]
      then
         do_remote_copy $1
      fi
      if [ "$1" = "-s" ]
      then
         echo "Backup completed."
      fi
   else
     if [ -f ./data/"${MAINDBNAME}".db.flat ]
     then
         do_error 1
     fi
   fi
   if [ ${MAIN_LOOP} -eq 1 ]
   then
      sleep ${SLEEPTIME} &
      MYPPID=$!
      initialize_kill $1
      wait ${MYPPID}
   fi
done
}

#############################################################################
# Run program 
#############################################################################
kill_old $1
initialize $1
main $1
