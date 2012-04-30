#!/bin/sh
for i in `ls . | grep -v -e bin -e src -e hdrs -e readme -e convert -e autoshutdown -e minimal_db -e portredirector `
do
   if [ -d $i ]; then
      cd $i
      echo Entered directory: $i
      for x in dbconvert mkhtml mkindx netrhost netrhost.debugmon
      do
	 echo "     Creating Link: $x"
         rm $x 2>/dev/null
         ln -s ../src/$x ../bin/$x
         ln -s ../bin/$x $x
      done
      cd ..
   fi
done
