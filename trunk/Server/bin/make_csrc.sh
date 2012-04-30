#!/bin/sh
#### Gotta keep Ashen happy, right?
####
#### @ Lensy

echo "##########################################################"
echo "# make_csrc                                              #"
echo "#    This script will create a new directory 'csrc' that #"
echo "#    that contains symlinks to all required source files #"
echo "#                                                        #"
echo "#    This directory can then be used in the same way as  #"
echo "#    pre-3.2.4pl14 source trees. Binaries will still be  #"
echo "#    placed in the 'bin' directory however.              #"
echo "##########################################################"

if [ -d csrc ]
then
   echo ERROR: The directory csrc already exists
else
   mkdir csrc
   for i in `ls src/*.c | cut -d"/" -f 2`
   do
      ln -s ../src/$i csrc/$i
   done

   for i in `ls hdrs/*.h | cut -d"/" -f 2`
   do
      ln -s ../hdrs/$i csrc/$i
   done

   ln -s ../src/Makefile csrc/Makefile
   ln -s ../src/gdbm-1.7.3 csrc/gdbm-1.7.3
   echo "" > csrc/.depend
   echo SUCCESS: The directory csrc has been created and can now be used.
fi

