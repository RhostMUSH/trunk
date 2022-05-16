#!/usr/bin/env bash
##############################################################################
# This is a simple example bash script to demonstrate some of the power
# of the execscript module of RhostMUSH.
##############################################################################
# display the mush executor dbref#
lc_exec=$(echo ${MUSH_CAUSE%% *})
echo "Mush Executer: ${MUSH_CAUSE}"
# display the mush variables being read from the executor
echo "Variables passed: ${MUSHL_VARS}"
# execute the path from the shell script
lc_add=$(expr ${MUSHV_ADD} + 2 2>/dev/null)
lc_sub=$(expr ${MUSHV_SUB} - 2 2>/dev/null)
lc_mul=$(expr ${MUSHV_MUL} \* 2 2>/dev/null)
lc_div=$(expr ${MUSHV_DIV} / 2 2>/dev/null)
# display how math will be achieved with the script
echo "Math passed (no error checking):"
echo "Add: ${MUSHV_ADD} + 2 = ${lc_add}"
echo "Sub: ${MUSHV_SUB} - 2 = ${lc_sub}"
echo "Mul: ${MUSHV_MUL} * 2 = ${lc_mul}"
echo "Div: ${MUSHV_DIV} / 2 = ${lc_div}"
# build the call back script that will read back into the mush
echo "Passing values back into registers"
echo "ADD ${MUSH_CAUSE%% *} ${lc_add}" > $0.set
echo "SUB ${MUSH_CAUSE%% *} ${lc_sub}" >> $0.set
echo "MUL ${MUSH_CAUSE%% *} ${lc_mul}" >> $0.set
echo "DIV ${MUSH_CAUSE%% *} ${lc_div}" >> $0.set
echo "Callback script $0.set:"
cat $0.set
# when the script exits, the call back script will be read into the mush
# the script will be deleted automatically after this effort
# the callback script is always created in the 'scripts' directory
exit 0
