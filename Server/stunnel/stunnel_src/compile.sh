#!/bin/sh
wget --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "Requires WGET to pull down the file.  Sorry."
   exit 1
fi
wget ftp://ftp.stunnel.org/stunnel/stunnel-*.tar.gz
gunzip -cd stunnel*.gz|tar -xvf -
cd stunnel-*
./configure
make
if [ ! -f src/stunnel ]
then
   echo "Problem compiling.  Sorry."
fi
ln -s `pwd`/src/stunnel ../../stunnel
echo "Done!"
