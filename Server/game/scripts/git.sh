#!/bin/bash
x="$@"
if [ -z "$x" ]
then
   git rev-parse HEAD
elif [ "$x" = "short" ]
then
   git rev-parse --short HEAD
else
   git rev-parse HEAD
fi
