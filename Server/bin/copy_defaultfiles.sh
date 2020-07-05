#!/bin/sh
cd defaults
for a in $(find * -type d)
do
  for b in $a/*.default
  do 
### - The -s switch does not exist on all unix flavors
#   NEWFILE=$(basename -s .default $b)
    NEWFILE=$(basename $b .default)

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
