#!/usr/bin/env bash
if [ -n "$1" ]
then
   echo "hello from the script with args: '$@'"
else
   echo "hello from the script."
fi
echo "Player: ${MUSH_PLAYER}"
echo "Cause: ${MUSH_CAUSE}"
echo "Caller: ${MUSH_CALLER}"
echo "Owner: ${MUSH_OWNER}"
echo "Flags: ${MUSH_FLAGS}"
echo "Toggles: ${MUSH_TOGGLES}"
echo "OFlags: ${MUSH_OWNERFLAGS}"
echo "OToggles: ${MUSH_OWNERTOGGLES}"
echo "Variables that are passed as MUSH_<variable>: ${MUSHL_VARS}"
echo "My HELLO variable (if set): ${MUSHV_HELLO}"

# show regs
echo "-----------------------SetQ Regs:"
regs="0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"
for list in ${regs}
do
   eval echo "Register ${list}: \${MUSHQ_${list}}"
done
echo "-----------------------Variables:"
for vars in ${MUSHL_VARS}
do
   x=$(echo "${vars}"|tr -cd '[:alnum:]')
   if [ "$x" = "${vars}" ]
   then
      eval echo "Variable ${vars}: \${MUSHV_${vars}}"
   else
      if [ -n "${skipped}" ]
      then
         skipped="${skipped} ${vars}"
      else
         skipped="${vars}"
      fi
   fi
done
if [ -n "${skipped}" ]
then
   echo "Variables skipped as invalid in bash: ${skipped}"
fi
echo "-----------------------"
echo "Setting W register to 'FROM SHELL'"
echo "W Q FROM SHELL" > $0.set
echo "Setting SHELLED variable on player ${MUSH_PLAYER}"
echo "SHELLED ${MUSH_PLAYER} This is my shelled script to player" >> $0.set
if [ "${MUSH_PLAYER}" != "${MUSH_CAUSE}" ]
then
   echo "Setting SHELLED variable on cause ${MUSH_CAUSE}"
   echo "SHELLED ${MUSH_CAUSE} This is my shelled script to cause" >> $0.set
   echo "Appending to SHELLED variable on cause ${MUSH_CAUSE}"
   echo "And more text on the cause." >> $0.set
fi
