#!/bin/bash
echo "This will pull a src tarball from the Server location and attempt to rebuild."
echo "Once fiished, you can @reboot in the Mush to load the new code."
echo "Do not forget to @readcache as well to read in the new text files."
echo ""
echo "Do you wish to attempt to update your source code now? (Y/N): "|tr -d '\012'
read ANS
if [ "${ANS}" = "Y" -o "${ANS}" = "y" ]
then
   echo "Rebuilding source tree... please wait..."
else
   echo "Abortng by user request."
   exit 1
fi
if [ -f ./src.tbz ]
then
   echo "I found a source file."
   ls -l src.tbz
   echo "Use this file? (Y/N): "|tr -d '\012'
   read ANS
   if [ "${ANS}" = "Y" -o "${ANS}" = "y" ]
   then
      echo "Ok, I'm rebuilding of that source file.  Hold on to your hat..."
   else
      echo "Verywell.  Please move/copy in the src.tbz file you wish to use in your $(pwd) directory."
      exit 1
   fi
else
   echo "I'm sorry, I did not find a src.tbz at '$(pwd)'.  Please copy that file in and try again."
   exit 1
fi
echo "Copying your binary ... just in case."
cp -f src/netrhost src/netrhost.automate
bunzip -cd src.tbz|tar -xvf -
cd src
make clean;make
cd ../game/txt
../mkindx help.txt help.indx
../mkindx wizhelp.txt wizhelp.indx
echo "Ok, we're done.  Ignore any warnings.  If you had errors, please report it to the developers."
exit 0
