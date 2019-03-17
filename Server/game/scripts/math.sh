#!/bin/bash
export BC_LINE_LENGTH=32000
bc --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "#-1 I AM NOT A MATH-HEAD (no supporting library)"
else
   gl_arg=$(echo "$@"|cut -f1 -d"|")
   gl_scale=$(echo "$@"|cut -f2 -d"|")
   gl_scale=$(expr ${gl_scale} + 0 2>/dev/null)
   if [ -z "${gl_scale}" ]
   then
      gl_scale=20000
   fi
   echo "scale=${gl_scale};${gl_arg}"|bc 2>&1
fi
