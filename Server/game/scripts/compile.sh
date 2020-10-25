#!/bin/bash
if [ "$#" -ne "1" ]
then
   echo "One(1) argument is expected.  Allowed arguments:"
   echo "   patch     -- patch the source."
   echo "   status    -- status of the source (compiling or completed)."
   echo "   rollback  -- roll back to the previous combined binary."
   echo "   recompile -- recompile the source."
   echo "   force     -- if a previous compile hung, try again."
   echo "   update    -- update the patch.sh."
   echo "   makefile  -- update Makefiles."
   exit 0
fi
arg=$(echo $1)
case "${arg}" in
   makefile) # Makefile updater
      lc_src="https://raw.githubusercontent.com/RhostMUSH/trunk/master/Server/scripts.sh"
      lc_pull=0
      # verify we even have the script updater.
      if [ ! -f ./scripts.sh ]
      then
         echo "Script updater was not found.  We'll pull it down for you."
         wget --version > /dev/null 2>&1
         if [ $? -eq 0 ]
         then
            wget -O scripts.sh "${lc_src}"
            lc_pull=1
         else
            echo "Wget not found.  Falling back to curl..."
            curl --version > /dev/null 2>&1
            if [ $? -eq 0 ]
            then
               curl -O "${lc_src}"
               lc_pull=1
            else
               echo "Curl not found.  falling back to lynx..."
               lynx --version > /dev/null 2>&1
               if [ $? -eq 0 ]
               then
                  lynx --dump "${lc_src}" > ./scripts.sh 2>/dev/null
               else
                  echo "Could not find wget, curl, or lynx to download."
                  echo "please manually download scripts.sh from the git hub site and place it in Server."
                  exit 1
               fi
            fi
         fi
      fi
      chmod 755 ./scripts.sh
      # verify the -m option exists
      lc_chk="$(./scripts.sh -h|grep -c "update main Makefile")"
      if [ ${lc_chk} -eq 0 ]
      then
         echo "patcher is an old patcher.  Please update patch.sh first."
         exit 1
      fi
      if [ -f ../src/x2 ]
      then
         lc_chk="$(grep -c "#EXIT" ../src/x2)"
         if [ ${lc_chk} -eq 0 ]
         then
            echo "Still updating patcher and/or Makefile."
            exit 1
         else
            echo "Old makefile update log file seen.  Cleaning up..."
            rm -f ../src/x2
         fi
      fi
      if [ -f ../src/x ]
      then
         echo "Verifying active compile is not occuring."
         lc_activecompile="$(find ../src/x -type f -cmin -10)"
         if [ -n "${lc_activecompile}" ]
         then
            echo "You can not update the patcher or Makefile during an active compile."
            exit 1
         fi
         echo "Cleaning up old compile marker..."
         rm -f ../src/x
      fi
      ./scripts.sh -m > ./src/x2 2>&1 &
      ;;
   update) # update patcher
      lc_src="https://raw.githubusercontent.com/RhostMUSH/trunk/master/Server/scripts.sh"
      lc_pull=0
      # verify we even have the script updater.
      if [ ! -f ./scripts.sh ]
      then
         echo "Script updater was not found.  We'll pull it down for you."
         wget --version > /dev/null 2>&1
         if [ $? -eq 0 ]
         then
            wget -O scripts.sh "${lc_src}"
            lc_pull=1
         else
            echo "Wget not found.  Falling back to curl..."
            curl --version > /dev/null 2>&1
            if [ $? -eq 0 ]
            then
               curl -O "${lc_src}"
               lc_pull=1
            else
               echo "Curl not found.  falling back to lynx..."
               lynx --version > /dev/null 2>&1
               if [ $? -eq 0 ]
               then
                  lynx --dump "${lc_src}" > ./scripts.sh 2>/dev/null
               else
                  echo "Could not find wget, curl, or lynx to download."
                  echo "please manually download scripts.sh from the git hub site and place it in Server."
                  exit 1
               fi
            fi
         fi
      fi
      chmod 755 ./scripts.sh
      if [ -f ../src/x2 ]
      then
         lc_chk="$(grep -c "#EXIT" ../src/x2)"
         if [ ${lc_chk} -eq 0 ]
         then
            echo "Still updating patcher and/or Makefile."
            exit 1
         else
            echo "Old patch update log file seen.  Cleaning up..."
            rm -f ../src/x2
         fi
      fi
      if [ -f ../src/x ]
      then
         echo "Verifying active compile is not occuring."
         lc_activecompile="$(find ../src/x -type f -cmin -10)"
         if [ -n "${lc_activecompile}" ]
         then
            echo "You can not update the patcher or Makefile during an active compile."
            exit 1
         fi
         echo "Cleaning up old compile marker..."
         rm -f ../src/x
      fi
      ./scripts.sh -p > ./src/x2 2>&1 &
      ;;
   force) # force recompile
      if [ -d ../rhost_tmp ]
      then
         echo "Cleaning up cached source files..."
         rm -rf ../rhost_tmp
      fi
      if [ -f ../src/x ]
      then
         echo "Cleaning up old compile marker..."
         rm -f ../src/x
      fi
      echo "Y"|./patch.sh > ./src/x 2>&1 &
      echo "Compiling..."
      ;;
   patch) # compile source
      if [ -d ../rhost_tmp ]
      then
         echo "..."
         tail -2 ../src/x
         echo ""
         echo "Still compiling.  Be patient..."
      elif [ -f ../bin/dbconvert -a -f ../src/x ]
      then
         echo "Completed compile."
         echo "Removing compile marker"
         rm -f ../src/x
      elif [ -f ../src/x ]
      then
         echo "..."
         tail -2 ../src/x
         echo ""
         echo "Still compiling.  Be patient..."
      else
         cd ../
         echo "Y"|./patch.sh > ./src/x 2>&1 &
         echo "Compiling..."
      fi
      ;;
   status) # show status of compile
      if [ -d ../rhost_tmp -o ! -f ../bin/dbconvert ]
      then
         echo "..."
         tail -2 ../src/x
         echo ""
         echo "Still compiling.  Be patient..."
      else
         echo "Completed."
         rm -f ../src/x
      fi
      ;;
   rollback) # rollback the source
      if [ -f /usr/bin/cmp ]
      then
         /usr/bin/cmp ../bin/netrhost ../bin/netrhost~ > /dev/null 2>&1
         chk=$?
      else
         chk=1
      fi
      if [ -d ../rhost_tmp -o ! -f ../bin/dbconvert ]
      then
         echo "Currently patching the code.  We can't roll back."
      elif [ ! -f ../bin/netrhost~ ]
      then
         echo "There is no binary to roll back to."
      elif [ ${chk} -eq 0 ]
      then
         echo "The previous binary has already been rolled back."
      else
         mv -f ../bin/netrhost ../bin/netrhost.new
         cp -f ../bin/netrhost~ ../bin/netrhost
         echo "Previous binary has been rolled back.  Please @reboot now."
      fi
      ;;
   recompile) # recompile the source
      if [ -d ../rhost_tmp -o ! -f ../bin/dbconvert ]
      then
         echo "Currently patching the code.  If patching was broken, have devs fix the patch and re-issue patch" 
      else
         cd ../src
         make clean > ../src/x 2>&1
         make >> ../src/x 2>&1 &
         echo "Recompiling the source.  See 'status' for status of compile."
      fi
      ;;
esac
