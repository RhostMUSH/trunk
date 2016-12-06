#!/bin/bash
bc --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "#-1 I AM NOT A MATH-HEAD"
else
   echo "$@"|bc 2>&1
fi
