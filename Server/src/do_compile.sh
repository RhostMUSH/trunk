#!/bin/sh
#
. ../src/do_compile.var
qdbmdir=./qdbm
cd $qdbmdir
if [ -f ./Makefile ]
then
   if [ "$(uname -s)" = "Darwin" -a "${qdbmdir}" = "./qdbm" ]
   then
      make mac
   elif [ $(gmake --version 2>&1|grep -ic GNU) -gt 0 ]
   then
      gmake
   else
      make
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
