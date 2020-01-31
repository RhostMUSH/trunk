#!/bin/sh
cd src
for a in *.default
do 
  NEWFILE=$(basename -s .default $a)
  if [ ! -f $NEWFILE ]
    then
      cp "$a" "$NEWFILE"
      if [ "$?" -ne "0" ]
        then
          echo ">> Failure to copy src/$a!"
        else
          echo ">> Copied src/$a -> src/$NEWFILE"
      fi
  fi
done
cd ..
cd game
for a in *.default
do 
  NEWFILE=$(basename -s .default $a)
  if [ ! -f $NEWFILE ]
    then
      cp "$a" "$NEWFILE"
      if [ "$?" -ne "0" ]
        then
          echo ">> Failure to copy game/$a!"
        else
          echo ">> Copied game/$a -> game/$NEWFILE"
      fi
  fi
done
cd txt
for a in *.default
do 
  NEWFILE=$(basename -s .default $a)
  if [ ! -f $NEWFILE ]
    then 
      cp "$a" "$NEWFILE"
      if [ "$?" -ne "0" ]
        then
          echo ">> Failure to copy game/txt/$a!"
        else
          echo ">> Copied game/txt/$a -> game/txt/$NEWFILE"
      fi
  fi
done
cd ../..
