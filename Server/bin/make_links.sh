#!/bin/sh
for i in `ls . | grep -v -e bin -e src -e hdrs -e readme -e convert -e autoshutdown -e minimal_db -e portredirector `
do
   if [ -d $i ]; then
      cd $i
      echo Entered directory: $i
      for x in dbconvert mkhtml mkindx netrhost netrhost.debugmon
      do
	 echo "     Creating Link: $x"|tr -d '\012'
         rm $x 2>/dev/null
         [[ ! -f ../bin/$x ]] && echo " unable to find binary!" && exit 1
         [[ ! -h $x ]] && ln -s ../bin/$x $x 2>/dev/null || echo "..[ skipping ln ]"|tr -d '\012' 
         echo " completed."
      done
      cd ..
   fi
done
