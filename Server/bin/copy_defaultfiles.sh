#!/bin/sh
for a in src game game/txt
do
  for b in defaults/$a/*.default
  do 
    NEWFILE=$(basename -s .default $b)
    if [ ! -f $NEWFILE ]
      then
        cp "$b" "$a/$NEWFILE"
        if [ "$?" -ne "0" ]
          then
            echo ">> Failure to copy $b!"
          else
            echo ">> Copied defaults/$a/$NEWFILE.default -> $a/$NEWFILE"
        fi
    fi
  done
done
