#!/bin/sh
#
. ../src/do_compile.var
case ${COMP:=gdbm} in
   gdbm) gdbmdir=./gdbm
         ;;
   qdbm) gdbmdir=./qdbm
         ;;
   mdbx) gdbmdir=./mdbx
         ;;
esac
#
cd $gdbmdir
if [ -f ./Makefile ]
then
   if [ "$(uname -s)" = "Darwin" -a "${qdbmdir}" = "./qdbm" ]
   then
      make mac
   elif [ $(gmake --version 2>&1|grep -ic GNU) -gt 0 ]
   then
      if [ "${qdbmdir}" = "./mdbx" ]
      then
        gmake libmdbx.a
      else
        gmake
      fi
   else
      if [ "${qdbmdir}" = "./lmdb" ]
      then
        make libmdbx.a
      else
        make
      fi
   fi
else
   ./configure
   if [ "$(uname -s)" = "Darwin" -a "${qdbmdir}" = "./qdbm" ]
   then
      make mac
   elif [ $(gmake --version 2>&1|grep -ic GNU) -gt 0 ]
   then
      gmake
   else
      make
   fi
fi
rm -f libqdbm*.so* >/dev/null 2>&1
exit 0
