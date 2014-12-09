#!/bin/sh
#

[ -f ../src/do_compile.defs ] && . ../src/do_compile.defs

# ${COMP:=gdbm} uses COMP or, if COMP is unset, sets COMP to gdbm and uses gdbm

case ${COMP:=gdbm} in
   gdbm) gdbmdir=./gdbm-1.8.3
         ;;
   qdbm) gdbmdir=./qdbm
         ;;
  tokyo) gdbmdir=./tokyocabinet-1.4.21
         ;;
esac
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
