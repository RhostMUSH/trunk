#!/bin/sh
#
COMP=qdbm
case ${COMP} in
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
   if [ "$(uname -s)" = "Darwin" -a "${qdbmdir}" = "./qdbm" ]
   then
      make mac
   else
      make
   fi
else
   ./configure
   if [ "$(uname -s)" = "Darwin" -a "${qdbmdir}" = "./qdbm" ]
   then
      make mac
   else
      make
   fi
fi
rm -f libqdbm*.so* >/dev/null 2>&1
exit 0
