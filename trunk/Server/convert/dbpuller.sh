#!/bin/sh
if [ -z "$1" -o -z "$2" ]
then
   echo "Syntax: $0 <flatfile> <dbref# (without the #)>"
   exit 1
fi
CHKVAL=$(expr $2 + 0 2>/dev/null)
if [ -z "${CHKVAL}" ]
then
   echo "Syntax: $0 <flatfile> <dbref# (without the #)>"
   exit 1
fi
TST=$(head -1 $1|cut -c1-2)
if [ "${TST}" != "+V" ]
then
   echo "The db puller currently only works with RhostMUSH flatfiles."
   exit 1
fi
echo "Pulling attribute header information from flatfile for object #$2..."|tr -d '\012'
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
}' < $1 > data/myflat.dat 2>/dev/null
echo "...Finished."

export BOB=$2
echo "Reading flatfile..."|tr -d '\012'
awk '
BEGIN {
      buff = ""
      foo = ENVIRON["BOB"]
      inattr = 0
      namedbobj = 0
}
{
   if ( match($1, "^![0-9]*" ) ) 
      obj=substr($1,2)
   if ( obj == foo ) {
      xxx=1
      if ( namedbobj == 0 )
         namedbobj=1
   }
   if ( obj != foo )
      xxx=0
   else if ( match($1, "^>[0-9]*") ) {
      inattr=1
      if ( length(buff) > 1 ) {
         printf("%d #%d=%s\n", atr, obj, buff)
      }
      atr=substr($1,2)
      buff=sprintf("") 
   } else if ( match($1, "^<") ) {
      if ( length(buff) > 1 ) {
         printf("%d #%d=%s\n", atr, obj, buff)
      }
      obj=999999999
   } else if ( xxx == 1 && inattr ) {
      if ( match($0, "^\001.*") ) {
         zzz=substr($0,index($0,":")+1)
         buff=sprintf("%s%s", buff, substr(zzz,index(zzz,":")+1))
      } else
         buff=sprintf("%s%s", buff, $0)
   } else if ( namedbobj == 2 ) {
      printf("@@ %s\n", $0);
      namedbobj=3
   } else if ( namedbobj == 1 ) {
      namedbobj++
   }
}' < $1 > data/pull.out
echo "completed."
echo "Translating attributes..."|tr -d '\012'
CNTR=0
> data/pull.wrk
cat data/pull.out | while read BOB
do
   CNTR=$(expr $CNTR + 1)
   if [ $CNTR -eq 1 ]
   then
      echo "${BOB}" >> data/pull.wrk
      continue
   fi
   VAL=$(echo "${BOB}"|cut -f1 -d" ")
   VAL2=$(grep " ${VAL}$" data/rhostattrs.dat|cut -f1 -d" ")
   CHK=$(grep -c " ${VAL}$" data/rhostlocks.dat|cut -f1 -d" ")
   if [ -z "${VAL2}" ]
   then
      VAL=$(echo "${BOB}"|cut -f1 -d" ")
      VAL2=$(grep " ${VAL}$" data/myflat.dat|cut -f1 -d" ")
   fi
   if [ ${CHK} -gt 0 ]
   then
      echo "@lock/${VAL2} $(echo "${BOB}"|cut -f2- -d" ")" >> data/pull.wrk 2>/dev/null
   else
      echo "&${VAL2} $(echo "${BOB}"|cut -f2- -d" ")" >> data/pull.wrk 2>/dev/null
   fi
done
mv -f data/pull.wrk data/pull_$2.out
echo "done.  File is 'data/pull_$2.out'"
