#!/bin/bash
:> temp.html
for i in $(cat shit)
do
   echo "Processing $i..."|tr -d '\012'
   wget -O index.html $i > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "[1;31mtimeout.[0m"
   else
      ./html2text.py index.html > index.txt 2>/dev/null
      if [ $? -ne 0 ]
      then
         echo "[1;33mparse failure.[0m"
      else
         cat index.txt >> temp.tmp
         echo "[1;32mdone.[0m"
      fi
      rm -f index.html index.txt 2>/dev/null
   fi
done
grep -E -o "([0-9]{1,3}[\.]){3}[0-9]{1,3}" temp.tmp|grep ^[0-9]|cut -f1 -d":"|sort -nu > temp.txt
rm -f temp.tmp
