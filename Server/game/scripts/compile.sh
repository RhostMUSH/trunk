#!/bin/bash
if [ "$#" -ne "1" ]
then
   echo "One(1) argument is expected.  Allowed arguments:"
   echo "   patch     -- patch the source."
   echo "   status    -- status of the source (compiling or completed)."
   echo "   rollback  -- roll back to the previous combined binary."
   echo "   recompile -- recompile the source."
   exit 0
fi
case "$1" in
   compile) # compile source
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
