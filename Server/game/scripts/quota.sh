#!/usr/bin/env bash
quota 2>/dev/null
x=$(df -h . 2>/dev/null)
if [ -n "$x" ]
then
   df -h .
fi
