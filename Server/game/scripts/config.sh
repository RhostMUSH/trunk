#!/bin/sh
gl_playerid=`echo "${MUSH_PLAYER}"|cut -f1 -d' '`
if [ "${gl_playerid}" != "#1" ]
then
   echo -n "This requires player #1 to execute." 
   exit 0
fi
if [ ! -f ../src/custom.defs ]
then
   echo -n "custom file was not identified."
   exit 0
fi
gl_config=`grep "^CUSTDEFS" ../src/custom.defs`
echo "_ASKSOURCE_CUSTDEFS #1 ${gl_config}" > $0.set
echo "CUSTDEFS found in ~/src/custom.defs and loaded."
if [ -f ../bin/asksource.save_default ]
then
   gl_def=`cat ../bin/asksource.save_default`
   echo "_ASKSOURCE_DEFAULT #1 ${gl_def}"|sed 's/$/\r/g' >> $0.set
   echo "" >> $0.set
   echo "Defailt compile options (~/bin/asksource.save_default) found and loded."
fi
cp -f $0.set /tmp
echo "#1 has compile time options saved on the player."
