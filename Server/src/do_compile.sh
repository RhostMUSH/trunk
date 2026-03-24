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

# MDBX needs extra CFLAGS to suppress warnings-as-errors on newer GCC
MDBX_CFLAGS="-O2 -g -Wall -Werror -Wextra -Wpedantic -ffunction-sections -fPIC -fvisibility=hidden -std=gnu11 -pthread -Wno-error=attributes -Wno-error=array-bounds"

#
cd $gdbmdir
if [ -f ./Makefile ]
then
   if [ "$(uname -s)" = "Darwin" -a "${gdbmdir}" = "./qdbm" ]
   then
      make mac
   elif [ $(gmake --version 2>&1|grep -ic GNU) -gt 0 ]
   then
      if [ "${gdbmdir}" = "./mdbx" ]
      then
        gmake CFLAGS="${MDBX_CFLAGS}" libmdbx.a
      else
        gmake
      fi
   else
      if [ "${gdbmdir}" = "./mdbx" ]
      then
        make CFLAGS="${MDBX_CFLAGS}" libmdbx.a
      else
        make
      fi
   fi
else
   ./configure
   if [ "$(uname -s)" = "Darwin" -a "${gdbmdir}" = "./qdbm" ]
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
