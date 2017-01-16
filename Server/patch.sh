#!/bin/sh
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
   echo "Aborting by user request."
   exit 1
fi
type=0
if [ -f ./src.tbz ]
then
   echo "I found a source file."
   ls -l src.tbz
   echo "Use this file? (Y/N): "|tr -d '\012'
   read ANS
   if [ "${ANS}" = "Y" -o "${ANS}" = "y" ]
   then
      echo "Ok, I'm rebuilding off that source file.  Hold on to your hat..."
   else
      echo "Verywell.  Please move/copy in the src.tbz file you wish to use in your $(pwd) directory."
      exit 1
   fi
else
   git --version > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "Oopse!  You do not have git installed.  You need that to use patch.sh.  Sorry!"
      exit 1
   fi
   gl_branch="trunk"
   gl_label="Rhost 4.0+"
   if [ "$1" = "39" ]
   then
      gl_branch="Rhost-3.9"
      gl_label="Rhost 3.9"
   fi
   echo "Hum.  No source files.  I'll tell git to yoink the source files for you then."
   echo "downloading ${gl_label}..."|tr -d '\012'
   git clone https://github.com/RhostMUSH/${gl_branch} rhost_tmp > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "error."
      echo "Ugh.  Https failed, let's try normal http...."|tr -d '\012'
      git clone http://github.com/RhostMUSH/${gl_branch} rhost_tmp > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         echo "error"
         echo "Double ugh.  Http failed, too.  Let's try the full git itself..."|tr -d '\012'
         git clone git://github.com/RhostMUSH/${gl_branch} rhost_tmp > /dev/null 2>&1
         if [ $? -ne 0 ]
         then
            echo "error."
            echo "I'm sorry, the git repository is not responding.  Try again later."
            exit 1
         fi
      fi
   fi

   echo "done!"
   type=1
fi
echo "Making a backup of all your files, please wait..."|tr -d '\012'
lc_date=$(date +%m%d%y%H%M%S)
tar -czf src_backup_${lc_date}.tgz src/*.c hdrs/*.h game/txt/help.txt game/txt/wizhelp.txt > /dev/null 2>&1
echo "... completed.  Filename is src_backup_${lc_date}.tgz"
echo "Copying your binary ... just in case.  Backup will be src/netrhost.automate (or bin/netrhost.automate)"
if [ -f src/netrhost ] 
then 
   cp -f src/netrhost src/netrhost.automate
fi
if [ -f bin/netrhost ] 
then
   cp -f bin/netrhost bin/netrhost.automate
fi
if [ ${type} -eq 0 ]
then
   mv -f src/local.c src/local.c.backup
   bunzip2 -cd src.tbz|tar -xf -
   cp -f src/local.c.backup src/local.c
else
   mv -f src/local.c src/local.c.backup
   cp -f rhost_tmp/Server/src/*.c src
   cp -f src/local.c.backup src/local.c
   cp -f rhost_tmp/Server/hdrs/*.h hdrs
   cp -f rhost_tmp/Server/bin/asksource* bin
   cp -f rhost_tmp/Server/game/txt/help.txt game/txt/help.txt
   cp -f rhost_tmp/Server/game/txt/wizhelp.txt game/txt/wizhelp.txt
   cp -f rhost_tmp/Server/readme/RHOST.CHANGES readme/RHOST.CHANGES
   rm -rf ./rhost_tmp
fi
cd src
if [ $(uname -s|grep -ic bsd) -gt 0 ]
then
   gmake clean;gmake
else
   make clean;make
fi
cd ..

gl_mypath=$(pwd)
gl_dirs=$(find . -maxdepth 1 -type d -name "[0-9A-Za-z]*" -exec basename {} \;)
for i in ${gl_dirs}
do
  if [ -d ${gl_mypath}/${i}/txt ]
  then
     echo "Indexing ${gl_mypath}/${i}..."
     cd ${gl_mypath}/$i/txt
     if [ -f ../mkindx ]
     then
        ../mkindx help.txt help.indx
        ../mkindx wizhelp.txt wizhelp.indx
     fi
  fi
done
echo "Ok, we're done.  Ignore any warnings.  If you had errors, please report it to the developers."
echo "Once you @reboot the mush, please issue @readcache to read in the new help files."
exit 0
