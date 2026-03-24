#!/bin/bash
if [ ! -f "$1" ]
then
   echo "Syntax: $0 netrhost.db.flat"
   exit 1
fi
gl_chk="$(tail -1 "$1"|grep -c "***END OF DUMP***")"
if [ ${gl_chk} -eq 0 ]
then
   echo "$1 is not a valid netrhost.db.flat file."
   exit 1
fi

gl_custdefs="$(grep -B1 "^1:_ASKSOURCE_CUSTDEFS" "$1")"
gl_default="$(grep -B1 "^1:_ASKSOURCE_DEFAULT" "$1")"
if [ -z "${gl_custdefs}" ]
then
   echo "Warning:  CUSTDEFS was not found in your flatfile."
fi
if [ -z "${gl_default}" ]
then
   echo "Error: the default compile options were not found in your flatfile."
   echo "Aborting."
   exit 1
fi
export gl_custatr="$(echo "${gl_custdefs}"|head -1|cut -f2 -d'A')"
export gl_defatr="$(echo "${gl_default}"|head -1|cut -f2 -d'A')"

if [ -f ../bin/asksource.save_default ]
then
   echo "Found asksource.save_default.  Backing up."
   gl_date="$(date +"%m%d%y%H%M%S")"
   mv -f ../bin/asksource.save_default ../bin/asksource.save_default.${gl_date}
fi
awk '
   /BEGIN/ { 
              nark=0
              gl_begin=0 
              glc=ENVIRON["gl_custatr"]
              gld=ENVIRON["gl_defatr"]
           }
   /^!/    { 
              if ( match($0, "^!2") > 0 ) {
                 exit
              }
           }
   /^!1$/  { 
              gl_begin=1 
           }
           {  if ( match($0, "^<") > 0 ) 
                 gl_begin=0
              if ( gl_begin > 0 ) {
                i_match=0
                if ( match($0, sprintf("^>%d", glc)) > 0 ) {
                   nark=2
                   i_match=1
                } else if ( match($0, sprintf("^>%d", gld)) > 0 ) {
                   nark=1
                   i_match=1
                } else if ( match($0, "^>") > 0 ) {
                   nark=0
                }
                if ( (nark > 0) && (i_match == 0) ) {
                   if ( nark > 1 ) {
                      printf("# %s\n", $0)
                   } else {
                      printf("%s\n", $0)
                   }
                }
             }
           }
   ' < "$1" | tr -d '\015' > ../bin/asksource.save_default

echo "asksource.save_default was successfully pulled from the flatfile and populated into your build config."
echo "You may now 'make config' or 'make confsource' to compile with the compile time options from the flatfile."
