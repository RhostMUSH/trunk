#!/bin/bash
aspell --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "#-1 Speller is not installed."
   exit 0
else
   echo "$@"|aspell -a|egrep -v "(^[*@]|^$)"
fi
