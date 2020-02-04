#!/bin/sh
cd defaults
for a in $(find * -type d)
do
  for b in $a/*.default
  do 
    NEWFILE=$(basename -s .default $b)
    if [ ! -f "../$a/$NEWFILE" ]
      then
        cp "$b" "../$a/$NEWFILE"
        if [ "$?" -ne "0" ]
          then
            echo ">> Failure to copy $b!"
          else
            echo ">> Copied defaults/$a/$NEWFILE.default -> $a/$NEWFILE"
        fi
    fi
  done
done
