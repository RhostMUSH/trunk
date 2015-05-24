#!/bin/sh
echo "Directory setup and configuration."
echo "Identifying user and group..."|tr -d '\012'
VAR="$(id|cut -f2 -d'('|cut -f1 -d')'):$(id|cut -f3 -d'('|cut -f1 -d')')"
echo "$VAR"
echo "Chowning directories and files..."|tr -d '\012'
chown -R $VAR .
echo "done."
echo "Protecting directory and file..."|tr -d '\012'
chmod 700 .
chmod 700 ../Rhost* 2>/dev/null
echo "done."
if [ ! -f /bin/bash ]
then
   ./bin/script_setup.sh
fi
