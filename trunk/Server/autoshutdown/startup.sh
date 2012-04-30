#!/bin/sh
#########################################################
# Define Environment Variables here
########  -- This is the path to your RhostMUSH root --
MYPATH=~/RhostMUSH
#########################################################
# Verify path information and location
if [ ! -d "$MYPATH" ]
then
   echo "Unable to change directory to '$MYPATH'."
   exit 1
fi
cd ${MYPATH}
if [ ! -f ./Startmush ]
then
   echo "Unable to find Startmush script.  Wrong directory?"
   exit 1
fi
# Start the mush
./Startmush
