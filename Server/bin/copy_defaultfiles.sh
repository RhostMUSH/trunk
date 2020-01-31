#!/bin/sh
cd game
for a in *.default
do 
  NEWFILE=$(basename -s .default $a)
  if [ ! -f $NEWFILE ]
    then
      cp $a $NEWFILE
      echo ">> Copied game/$a -> game/$NEWFILE"
  fi
done
cd txt
for a in *.default
do 
  NEWFILE=$(basename -s .default $a)
  if [ ! -f $NEWFILE ]
    then 
      cp $a $NEWFILE
      echo ">> Copied game/txt/$a -> game/txt/$NEWFILE"
  fi
done
cd ../..
