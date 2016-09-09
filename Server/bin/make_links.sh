#!/bin/sh
for i in `ls . | grep -v -e bin -e src -e hdrs -e readme -e convert -e autoshutdown -e minimal-DBs -e portredirector `
do
   if [ -d $i ]; then
      cd $i
      echo Entered directory: $i
      for x in dbconvert mkhtml mkindx netrhost netrhost.debugmon
      do
	 echo "     Creating Link: $x"|tr -d '\012'
         if [ ! -f ../bin/$x ] 
         then  
            echo " unable to find binary!"
            exit 1
         fi
         rm $x 2>/dev/null
         if [ ! -h $x ] 
         then
            ln -s ../bin/$x $x 2>/dev/null 
            if [ $? -ne 0 ]
            then
               echo "..[ skipping ln ]"|tr -d '\012' 
            fi
         fi
         echo " completed."
      done
      cd ..
   fi
done
