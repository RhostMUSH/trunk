#!/bin/sh
# This script made by Ashen-Shugar (c) 1999, all rights reserved
# This script is used to execute the conversion of the code.
# To run, please type:  
#                       doconvert <input flat> <output flat>
if [ "$2" = "" -o "$1" = "" ]
then
   echo "syntax: $0 <filein> <fileout>"
   exit 1
fi
if [ ! -f "$1" ]
then
   echo "File '$1' does not exist.  Aborting."
   exit 1
fi
CHK1=$(head -1 $1|wc -c)
CHK2=$(head -1 $1|tr -d '\015'|wc -c)
if [ ${CHK1} -ne ${CHK2} ]
then
   echo "File '$1' appears to be in DOS/WINDOWS format.  Please strip LF (line feeds) and try again."
   exit 1
fi
CONT=0
while [ $CONT -eq 0 ]
do
   echo "What version are you converting to RhostMUSH:"
   echo "1)  TinyMUSH 2.0.x to TinyMUSH 2.2.x"
   echo "2)  TinyMUSH 3.0/3.1 without timestamps"
   echo "3)  TinyMUSH 3.1 with timestamps"
   echo "4)  TinyMUX 1.X"
   echo "5)  PennMUSH 1.7.2 (all patches) - ALPHA converter"
   echo "6)  PennMUSH 1.7.5-1.7.7 (MIGHT work with 1.8) (all patches) - BETA converter."
   echo "7)  TinyMUX 2.0-2.8"
   echo "8)  TinyMUX 2.9+"
   echo "------------------------------------------------------------------------------"
   echo "Q)  Quit"
   echo ""
   echo "Please enter number of selection: "|tr -d '\012'
   read ANS
   if [ "${ANS}" = "Q" -o "${ANS}" = "q" ]
   then
      echo "Converter aborted by user."
      exit 0
   fi
   VANS=$(expr $ANS + 0 2>&1|grep -c non-numeric)
   if [ "$VANS" -gt 0 ]
   then
      echo "Value must be numeric."
      continue
   fi
   if [ "$ANS" -lt 1 -o "$ANS" -gt 9 ]
   then
      echo "Value must be between 1 and 8."
      continue
   else
      if [ "$ANS" -eq 1 ]
      then
         TYPE="TinyMUSH"
      elif [ "$ANS" -eq 2 ]
      then
         TYPE="TinyMUSH 3.0"
      elif [ "$ANS" -eq 3 ]
      then
         TYPE="TinyMUSH 3.1"
      elif [ "$ANS" -eq 4 ]
      then
         TYPE="MUX"
      elif [ "$ANS" -eq 5 ]
      then
         TYPE="PennMUSH"
      elif [ "$ANS" -eq 6 ]
      then
         TYPE="PennMUSH5"
      elif [ "$ANS" -eq 7 ]
      then
         TYPE="MUX2"
      elif [ "$ANS" -eq 8 ]
      then
         TYPE="MUX2NEW"
      fi
      CONT=1
   fi
done
if [ -f "./convm2lock" -a -f "./quote2" -a -f "./conv" -a -f "./tinyquote" -a -f "./tinyconv" -a -f "./pennconv" -a -f "./convm2" -a -f "./convm2new" -a -f "./convtm3" -a -f "./convtm31" -a -f "./quote2tm31" ]
then
   echo  "Binaries detected. Continuing..."
else
   echo "Binaries not detected.  Compiling source."
   echo "Detecting C compiler..."|tr -d '\012'
   CTYPE=$(gcc 2>&1|grep -c "not found")
   if [ $CTYPE -eq 0 ]
   then
      CC=gcc
      echo "GCC detected."
   else
      CTYPE=$(cc 2>&1|grep -c "not found")
      if [ $CTYPE -eq 0 ]
      then
         echo "CC detected."
         CC=cc
      else
         echo "undetected."
         echo "Please type in the name of the C compiler you use (ie: cc) : "|tr -d '\012'
         read CC
      fi
   fi
   echo "Compiling quote remover..."|tr -d '\012'
   $CC quote2.c -o quote2 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "(quote2.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC tinyquote.c -o tinyquote 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "(tinyquote.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   echo "...finished."
   echo "Compiling converters..."|tr -d '\012'
   $CC conv.c -o conv 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "(conv.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC tinyconv.c -o tinyconv 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "(tinyconv.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC pennconv.c -o pennconv 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "(pennconv.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC convm2.c -o convm2 2> /dev/null
   if [ $? -ne 0 ]
   then
      echo "(convm2.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC convm2new.c -o convm2new 2> /dev/null
   if [ $? -ne 0 ]
   then
      echo "(convm2new.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC convtm3.c -o convtm3 2> /dev/null
   if [ $? -ne 0 ]
   then
      echo "(convtm3.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC convtm31.c -o convtm31 2> /dev/null
   if [ $? -ne 0 ]
   then
      echo "(convtm31.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC quote2tm31.c -o quote2tm31 2> /dev/null
   if [ $? -ne 0 ]
   then
      echo "(quote2tm31.c -> error)"
      echo "Failed on compile.  Aborted."
      exit 1
   fi
   $CC convm2lock.c -o convm2lock
   if [ $? -eq 0 ]
   then
      echo "Compiling MUX 2.8+ lock identification tool..."
   else
      echo "(convm2lock.c -> error) failed to compile mux 2.8+ lock identification tool..."
      exit 1
   fi
   echo "...finished."
fi
echo "Checking for valid flatfile..."|tr -d '\012'
CNT=0
CNT=$(tail -1 $1|grep -c "**END OF DUMP**")
if [ "$CNT" -eq 0 ]
then
   echo "...bad file.  Aborted."
   exit 1
else
   echo "...validated."
fi
if [ "$TYPE" = "MUX2NEW" -o "$TYPE" = "MUX" -o "$TYPE" = "TinyMUSH 3.0" -o "$TYPE" = "TinyMUSH 3.1" -o "$TYPE" = "MUX2" ]
then
   echo "Converting COLUMNS() to COLUMN()..."|tr -d '\012'
   sed "s/columns(/column(/g" $1 > $1.sed2 2>/dev/null
   echo "...Finished."
   if [ "${TYPE}" = "MUX2NEW" -o "${TYPE}" = "MUX2" -o "${TYPE}" = "TinyMUSH 3.1" ]
   then
      echo "Converting ELEMENTS() to ELEMENTSMUX()..."|tr -d '\012'
      sed "s/elements(/elementsmux(/g" $1.sed2 > $1.sed 2>/dev/null
      echo "...Finished."
   fi
   echo ""
   echo "RhostMUSH allows %c _or_ %x to be ANSI.  It depends how you have it configured."
   echo "Do you wish to convert TM3's %x ansi to Rhost's %c ansi? (Y/N) :"|tr -d '\012'
   read ANS
   if [ "$ANS" = "Y" -o "$ANS" = "y" ]
   then
      echo "Converting %x to %c..."|tr -d '\012'
      sed "s/%x/%c/g" $1.sed > $1.sed2 2>/dev/null
      mv -f $1.sed2 $1.sed 2>/dev/null
      echo "...Finished."
   fi
   echo "MUX 2.4 and later uses %m for the last command.  By default this is %x for Rhost."
   echo "You should only answer yes to this for MUX 2.4 or later versions."
   echo "Do you wish to convert MUX2.4's %m last-command? (Y/N) :"|tr -d '\012'
   read ANS
   if [ "$ANS" = "Y" -o "$ANS" = "y" ]
   then
      echo "Does your Rhost use %x for last command? (Y/N) :"|tr -d '\012'
      read ANS
      if [ "$ANS" = "Y" -o "$ANS" = "y" ]
      then
         CVT="%x"
      else
         CVT="%c"
      fi
      echo "Converting %m to ${CVT}..."|tr -d '\012'
      sed "s/%m/${CVT}/g" $1.sed > $1.sed2 2>/dev/null
      mv -f $1.sed2 $1.sed 2>/dev/null
      echo "...Finished."
   fi
elif [ "$TYPE" = "PennMUSH5" ]
then
   echo "" > /dev/null
else
   cp $1 $1.sed
fi
echo "Converting $TYPE attribute storage to RhostMUSH..."|tr -d '\012'
if [ "$TYPE" = "TinyMUSH" ]
then
   ./tinyquote $1.sed $1.noquote 2>/dev/null
elif [ "$TYPE" = "PennMUSH" ]
then
   ./tinyquote $1.sed $1.noquote 2>/dev/null
elif [ "$TYPE" = "PennMUSH5" ]
then
   echo "" > /dev/null
elif [ "$TYPE" = "TinyMUSH 3.1" ]
then
   ./quote2 $1.sed $1.noquote 2>/dev/null
else
   ./quote2 $1.sed $1.noquote 2>/dev/null
fi
if [ $? -ne 0 ]
then
   echo "Error on attribute conversion.  Aborting."
   exit 1
fi
echo "...Finished."
echo "Converting $TYPE to RhostMUSH..."|tr -d '\012'
if [ "$TYPE" = "TinyMUSH" ]
then
   ./tinyconv < $1.noquote > $2 2>err.log
elif [ "$TYPE" = "PennMUSH" ]
then
   ./pennconv < $1.noquote > $2 2>err.log
elif [ "$TYPE" = "MUX2" ]
then
   ./convm2 < $1.noquote > $2 2>err.log
elif [ "$TYPE" = "MUX2NEW" ]
then
   ./convm2lock $1.noquote muxlocks.out
   ./convm2new < $1.noquote > $2 2>err.log
   chk=$(wc -l muxlocks.out|cut -f1 -d" ")
   if [ ${chk} -gt 0 ]
   then
      echo ""
      echo "There are unconverted MUX @locks you need to manually load."
      echo "When loaded, please load in muxlocks.out into the mush."
   fi
elif [ "$TYPE" = "TinyMUSH 3.0" ]
then
   ./convtm3 < $1.noquote > $2 2>err.log
elif [ "$TYPE" = "TinyMUSH 3.1" ]
then
   ./convtm31 < $1.noquote > $2 2>err.log
elif [ "$TYPE" = "PennMUSH5" ]
then
   echo ""
   ./conv.pl $1 $2 2>/dev/null
   if [ $? -ne 0 ]
   then
      echo "Conversion error..."|tr -d '\012'
   else
      echo "Penn conversion..."|tr -d '\012'
   fi
else
   ./conv < $1.noquote > $2
fi
if [ $? -ne 0 ]
then
   echo "Error on convertion.  Aborting."
   exit 1
fi
echo "...Finished."
echo "Pulling attribute header information from flatfile..."|tr -d '\012'
awk '
BEGIN {
  i_chk = 0
}
{
   if ( i_chk ) {
      d_atr=substr($1,index($1,":")+1)
      printf("%s %s\n", d_atr, v_atr);
      i_chk = 0;
   }
   if ( match($1, "^+A[0-9]*") ) {
      i_chk = 1
      v_atr=substr($1,3)
   }
}' < $2 > data/myflat.dat 2>/dev/null
echo "...Finished."
rm -f $1.noquote
rm -f $1.sed
echo "Conversion complete."
if [ -f "err.log" ]
then
   ERR=$(cat err.log 2>/dev/null|wc -l)
else
   ERR=0
fi
if [ "$ERR" -gt 0 ]
then
   echo "Converting error log messages to readable format"|tr -d '\012'
   if [ "$TYPE" = "MUX2" ]
   then
      IFIL=data/muxattrs.dat
   else
      IFIL=data/tm3attrs.dat
   fi
   FSIZE=$(expr $ERR / 10)
   if [ -z "$FSIZE" ]
   then
      FSIZE=1
   elif [ $FSIZE -eq 0 ]
   then
      FSIZE=1
   fi
   CNTR=0
   cat err.log |
   while read BOB
   do
      if [ $(expr ${CNTR} % ${FSIZE}) -eq 0 ]
      then
         echo "."|tr -d '\012'
      fi
      CNTR=$(expr ${CNTR} + 1)
      TST=$(echo "${BOB}"|grep -c "^Warning:")
      if [ $TST -eq 1 ]
      then
         VAL=$(echo "${BOB}"|cut -f6 -d" "|cut -f2 -d"("|cut -f1 -d")")
         VAL2=$(grep " ${VAL}$" "${IFIL}"|cut -f1 -d" ")
         if [ -z "${VAL2}" ]
         then
            VAL=$(echo "${BOB}"|cut -f6 -d" "|cut -f2 -d"("|cut -f1 -d")")
            VAL2=$(grep " ${VAL}$" data/myflat.dat|cut -f1 -d" ")
            if [ ! -z "${VAL2}" ]
            then
               echo "${BOB}" | sed "s/${VAL}/${VAL2}/g" >> err.log.d
            else
               echo "${BOB}" >> err.log.d
            fi
         else
            echo "${BOB}" | sed "s/${VAL}/${VAL2}/g" >> err.log.d
         fi
      else
         echo "${BOB}" >> err.log.d
      fi
   done
   echo "...completed."
   grep "over LBUF.$" err.log.d > err.log
   grep "Notice:" err.log.d > over.log
   grep -v "over LBUF.$" err.log.d|grep -v "Notice:" > missing.log
   rm -f err.log.d 
   echo "Please type:"
   echo "        'more err.log' : $(wc -l err.log|cut -f1 -d" ") attributes were cut off for length limitations."
   echo "        'more missing.log' : $(wc -l missing.log|cut -f1 -d" ") attributes are unused by RhostMUSH."
   echo "        'more over.log' : $(wc -l over.log|cut -f1 -d" ") objects had over 750 attributes."
   if [ -s muxlocks.out ]
   then
      echo "        'more muxlocks.out' : $(wc -l muxlocks.out|cut -f1 -d" ") locks need to be manually loaded into RhostMUSH."
   fi
   echo "Note: #1's password was forcefully reset to 'Nyctasia' incase the passwords did not convert over."
fi
