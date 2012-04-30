#!/bin/sh
#
gdbmdir=./gdbm-1.8.3
#gdbmdir=./qdbm
#
cd $gdbmdir
if [ -f ./Makefile ]
then
   make
else
   ./configure
   make
fi
rm -f libqdbm*.so* >/dev/null 2>&1
exit 0
