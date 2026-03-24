#!/bin/bash
if [ $# -lt 1 ]
then
   echo "#-1 FUNCTION (TRIGGER) EXPECTS 1 OR MORE ARGUMENTS"|tr -d '\012'
   exit 0
fi
lc_player="$(echo ${MUSH_CAUSE}|cut -f1 -d' ')"
echo "@exec ${lc_player} @tr/quiet ${MUSHV_ARG0}={${MUSHV_ARG1}},"\
     "{${MUSHV_ARG2}},{${MUSHV_ARG3}},{${MUSHV_ARG4}},{${MUSHV_ARG5}},"\
     "{${MUSHV_ARG6}},{${MUSHV_ARG7}},{${MUSHV_ARG8}},{${MUSHV_ARG9}},"\
     "{${MUSHV_ARG10}}" > $0.set
