#!/bin/sh
if [ -n "$1" ]
then
   echo "hello from the script with args: '$@'"
else
   echo "hello from the script."
fi
