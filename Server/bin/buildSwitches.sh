#!/bin/sh
#
# Make the table of switches
# We run this from the bin directory
#
# 'Borrowed' from PennMUSH

echo "Rebuilding command switch file"
snum=1
rm -f ../hdrs/switches.h
rm -f ../src/switchinc.c
for s in `cat ../src/SWITCHES | sort`; do
  echo "#define SWITCH_$s $snum" >> ../hdrs/switches.h
  echo "{"                       >> ../src/switchinc.c
  echo "  \"$s\", SWITCH_$s"     >> ../src/switchinc.c
  echo "}"                       >> ../src/switchinc.c
  echo ","                       >> ../src/switchinc.c
  snum=`expr $snum + 1`
 done

