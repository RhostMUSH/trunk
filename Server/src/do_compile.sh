#!/bin/sh
#
. ../src/do_compile.var
case ${COMP:=qdbm} in
   qdbm) gdbmdir=./qdbm
         ;;
   mdbx) gdbmdir=./mdbx
         ;;
esac

# MDBX uses a single amalgamated .c file and needs direct compilation
# rather than the GNUmakefile (which requires files not in the amalgamation).
MDBX_CFLAGS="-O2 -g -Wall -ffunction-sections -fPIC -fvisibility=hidden -std=gnu11 -pthread -Wno-error"

#
if [ "${gdbmdir}" = "./mdbx" ]
then
   cd $gdbmdir
   CC=${CC:-gcc}
   (echo '#define MDBX_BUILD_TIMESTAMP "'$(date +%Y-%m-%dT%H:%M:%S)'"'
    echo '#define MDBX_BUILD_FLAGS "'"${MDBX_CFLAGS}"'"'
    echo '#define MDBX_BUILD_COMPILER "'$(${CC} --version 2>/dev/null | head -1)'"'
    echo '#define MDBX_BUILD_TARGET "'$(uname -m)-$(uname -s | tr 'A-Z' 'a-z')'"'
   ) > config.h &&
   ${CC} ${MDBX_CFLAGS} -DNDEBUG=1 -DMDBX_CONFIG_H=\"config.h\" -ULIBMDBX_EXPORTS -c mdbx.c -o mdbx-static.o &&
   ar rs libmdbx.a mdbx-static.o
else
   cd $gdbmdir
   if [ -f ./Makefile ]
   then
      if [ "$(uname -s)" = "Darwin" -a "${gdbmdir}" = "./qdbm" ]
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
fi
exit 0
