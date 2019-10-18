#!/bin/bash
lc_script="./shell_patch/Server/game/scripts"
lc_bin="./shell_patch/Server/bin"
lc_src="./shell_patch/Server/src"
lc_main="./shell_patch/Server"
lc_readme="./shell_patch/Server/readme"
lc_date=$(date +"%s")
function update_git
{
   if [ -d ./shell_patch ]
   then
      echo "Cleaning up stale directory for git repo..."
      cleanup_git
   fi
   git --version > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "Oopse!  You do not have git installed.  You need that to use $0.  Sorry!"
      exit 1
   fi
   gl_branch="trunk"
   gl_label="Rhost 4.0+"
   if [ "$1" = "39" ]
   then
      gl_branch="Rhost-3.9"
      gl_label="Rhost 3.9"
   fi
   echo "Updating sources..."|tr -d '\012'
   echo "downloading ${gl_label}..."|tr -d '\012'
   git clone https://github.com/RhostMUSH/${gl_branch} shell_patch > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "error."
      echo "Https failed, trying http...."|tr -d '\012'
      git clone http://github.com/RhostMUSH/${gl_branch} shell_patch > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         echo "error"
         echo "Http failed, trying git..."|tr -d '\012'
         git clone git://github.com/RhostMUSH/${gl_branch} shell_patch > /dev/null 2>&1
         if [ $? -ne 0 ]
         then
            echo "error."
            echo "Can't pull latest sources.  Try again later."
            exit 1
         fi
      fi
   fi
   echo "update pulled!"
}

function cleanup_git
{
   rm -rf ./shell_patch
}

function help
{
   echo "Syntax:  $0 <argument>"
   echo ""
   echo "This will update your scripts.  This expects an argument."
   echo "Please select the following:"
   echo "   -h -- show this help."
   echo "   -e -- update all execscripts."
   echo "   -b -- update all bin scripts."
   echo "   -r -- update all readmes."
   echo "   -p -- update patcher and this script."
   echo "   -c -- convert old makefile with new makefile."
}

function update_execscripts
{
   update_git
   lc_update=""
   for i in $(ls ${lc_script}/*)
   do
      lc_file="$(echo "${i##*/}")"
      diff $i game/scripts/${lc_file} > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f ./game/scripts/${lc_file} ./game/scripts/${lc_file}.${lc_date} 2>/dev/null
         cp -pf ${lc_script}/${lc_file} ./game/scripts/${lc_file}
         if [ -z "${lc_update}" ]
         then
            lc_update="${lc_file}"
         else
            lc_update="${lc_update} ${lc_file}"
         fi
      fi
   done
   if [ -n "${lc_update}" ]
   then
      echo "Updated scripts: ${lc_update}"
   else
      echo "Nothing updated."
   fi
   cleanup_git
}

function update_bins
{
   update_git
   lc_update=""
   for i in $(ls ${lc_bin}/asksource.save_template ${lc_bin}/mysql.blank ${lc_bin}/mysql_config ${lc_bin}/*.sh)
   do
      lc_file="$(echo "${i##*/}")"
      if [ "${lc_file}" = "asksource.sh" ]
      then
         continue
      fi
      diff $i ./bin/${lc_file} > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f ./bin/${lc_file} ./bin/${lc_file}.${lc_date} 2>/dev/null
         cp -pf ${lc_bin}/${lc_file} ./bin/${lc_file}
         if [ -z "${lc_update}" ]
         then
            lc_update="${lc_file}"
         else
            lc_update="${lc_update} ${lc_file}"
         fi
      fi 
   done
   if [ -n "${lc_update}" ]
   then
      echo "Updated scripts: ${lc_update}"
   else
      echo "Nothing updated."
   fi
   cleanup_git
}

function update_patcher
{
   update_git
   lc_update=""
   for i in $(ls ${lc_main}/*.sh)
   do
      lc_file="$(echo "${i##*/}")"
      diff $i "${lc_file}" > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f "${lc_file}" "${lc_file}.${lc_date}" 2>/dev/null
         cp -pf "${lc_main}/${lc_file}" "${lc_file}"
         if [ -z "${lc_update}" ]
         then
            lc_update="${lc_file}"
         else
            lc_update="${lc_update} ${lc_file}"
         fi
      fi 
   done
   if [ -n "${lc_update}" ]
   then
      echo "Updated patcher shells: ${lc_update}"
   else
      echo "Nothing updated."
   fi
   cleanup_git
}

function update_readme
{
   update_git
   lc_update=""
   for i in $(ls ${lc_main}/README*)
   do
      lc_file="$(echo "${i##*/}")"
      diff $i "${lc_file}" > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f "${lc_file}" "${lc_file}.${lc_date}" 2>/dev/null
         cp -pf "${lc_main}/${lc_file}" "${lc_file}"
         if [ -z "${lc_update}" ]
         then
            lc_update="${lc_file}"
         else
            lc_update="${lc_update} ${lc_file}"
         fi
      fi 
   done

   for i in $(ls ${lc_readme}/*)
   do
      lc_file="$(echo "${i##*/}")"
      diff $i ./readme/${lc_file} > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f ./readme/${lc_file} ./readme/${lc_file}.${lc_date} 2>/dev/null
         cp -pf ${lc_readme}/${lc_file} ./readme/${lc_file}
         if [ -z "${lc_update}" ]
         then
            lc_update="${lc_file}"
         else
            lc_update="${lc_update} ${lc_file}"
         fi
      fi 
   done
   if [ -n "${lc_update}" ]
   then
      echo "Updated readmes: ${lc_update}"
   else
      echo "Nothing updated."
   fi
   cleanup_git
}

function update_convert
{
   update_git
   lc_update=""
   lc_defs="$(grep ^DEFS src/Makefile|tail -1)"
   if [ $(echo "${lc_defs}"|grep -c CUSTDEFS) -gt 0 ]
   then
      lc_defs=""
   fi
   if [ ! -f src/do_compile.var ]
   then
      lc_libs="$(grep ^LIBS src/Makefile|head -1)"
      if [ $(echo "${lc_libs}"|grep -c gdbm) -gt 0 ]
      then
         echo "COMP=gdbm" > ./src/do_compile.var
      else
         echo "COMP=qdbm" > ./src/do_compile.var
      fi
      if [ -z "${lc_update}" ]
      then
         lc_update="do_compile.var"
      else
         lc_update="${lc_update} do_compile.var"
      fi
   fi
   if [ ! -f src/custom.defs ]
   then
      if [ -n "${lc_defs}" ]
      then
         lc_defs="$(echo ${lc_defs}|cut -f3- -d' ')"
         lc_morelibs="$(grep ^MORELIBS src/Makefile|tail -1)"
         lc_morelibs="$(echo ${lc_morelibs}|cut -f3- -d' ')"
cat > ./src/custom.defs << EOF
# Custom Definitions
CUSTDEFS = ${lc_defs}
# Custom Libraries
CUSTMORELIBS = ${lc_morelibs}
# Main @door engine
DR_DEF = -DENABLE_DOORS -DEXAMPLE_DOOR_CODE
# Mush @door
DRMUSHSRC = door_mush.c
DRMUSHOBJ = door_mush.o
# Empire @door
DREMPIRESRC = empire.c
DREMPIREOBJ = empire.o
DREMPIREHDR = empire.h
DR_HDR = \$(DREMPIREHDR)
# DB used for Mush Engine
CUSTLIBS = -L../src/qdbm/ -lqdbm
# MySQL compatibility engine
# USEMYSQL = 1
EOF
      
      for i in $(ls ${lc_bin}/asksource.blank ${lc_bin}/asksource.sh)
      do
         lc_file="$(echo "${i##*/}")"
         diff $i ./bin/${lc_file} > /dev/null 2>&1
         if [ $? -ne 0 ]
         then
            mv -f ./bin/${lc_file} ./bin/${lc_file}.${lc_date}
            cp -pf ${lc_bin}/${lc_file} ./bin/${lc_file}
            if [ -z "${lc_update}" ]
            then
               lc_update="${lc_file}"
            else
               lc_update="${lc_update} ${lc_file}"
            fi
         fi
      done
      for i in $(ls ${lc_src}/*.sh ${lc_src}/Makefile)
      do
         lc_file="$(echo "${i##*/}")"
         diff $i ./src/${lc_file} > /dev/null 2>&1
         if [ $? -ne 0 ]
         then
            mv -f ./src/${lc_file} ./src/${lc_file}.${lc_date}
            cp -pf ${lc_src}/${lc_file} ./src/${lc_file}
            if [ -z "${lc_update}" ]
            then
               lc_update="${lc_file}"
            else
               lc_update="${lc_update} ${lc_file}"
            fi
         fi
      done
      fi
      diff ${lc_main}/Makefile ./Makefile >/dev/null 2>&1
      if [ $? -ne 0 ]
      then
         mv -f ./Makefile ./Makefile.${lc_date}
         cp -pf ${lc_main}/Makefile ./Makefile
         if [ -z "${lc_update}" ]
         then
            lc_update="[Main: Makefile]"
         else
            lc_update="${lc_update} [Main: Makefile]"
         fi
      fi
   fi
   if [ -n "${lc_update}" ]
   then
      echo "Updated config: custom.defs ${lc_update}"
   else
      echo "Nothing updated."
   fi
   cleanup_git
}

#### main
lc_path="$(pwd)"
lc_path="${lc_path##*/}"
if [ "${lc_path}" != "Server" -o ! -d "game" ]
then
   echo "$0: error -- must execute from 'Server' subdirectory"
   exit 1
fi
case "$@" in
   -e|-E|--exec|--execscript|--execscripts) # execscripts
      update_execscripts
      ;;
   -b|-B|--bin) # bins
      update_bins
      ;;
   -c|-C|--convert) # do convert
      update_convert
      ;;
   -r|-R|--readme) # update readmes
      update_readme
      ;;
   -p|-P|--patch|--patcher) # update patcher
      update_patcher
      ;;
   *) # help only
      help
      ;;
esac
