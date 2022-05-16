#!/bin/sh
if [ -f /usr/games/fortune ]
then
   /usr/games/fortune
else
   echo "You are quite unfortunate."
fi
