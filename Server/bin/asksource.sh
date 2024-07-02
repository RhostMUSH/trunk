#!/bin/bash
###################################################################
# Test shell capabilities
###################################################################
TST[1]=test
if [ $? -ne 0 ]
then
   echo "/bin/sh does not accept array variables."
   echo "please run the ./bin/script_setup.sh script in attempts to fix this."
   echo "You must have a KSH or BASH compatable shell."
   exit 1
fi
###################################################################
# check for mysql goodness
###################################################################
FORCE_MYSQL=0
MYSQL_VER=$(mysql_config --version 2>/dev/null)
if [ -z "${MYSQL_VER}" ]
then
   MYSQL_VER=$(./mysql_config --version 2>/dev/null)
fi
if [ -z "${MYSQL_VER}" ]
then
   MYSQL_VER=0
   MYSQL_VER2=$(mysql -V|awk '{print $5}')
fi
# load in mysql goodness
mysql_host=$(grep ^mysql_host ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
mysql_user=$(grep ^mysql_user ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
mysql_pass=$(grep ^mysql_pass ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
mysql_dbname=$(grep ^mysql_base ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
mysql_socket=$(grep ^mysql_socket ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
mysql_port=$(grep ^mysql_port ../game/rhost_mysql.conf 2>/dev/null|awk '{print $2}')
###################################################################
# Fix for save
###################################################################
if [ -f asksource.save ]
then
   mv -f asksource.save asksource.save0
fi
###################################################################
# Environments
###################################################################
LDCONFIG=""
### MANUALLY DEFINE YOUR GCC BELOW IF IT CAN NOT FIND IT
MYGCC="cc"
if [ $(uname -s|grep -ic "bsd") -gt 0 ]
then
   LDOPT="-r"
else
   LDOPT="-p"
fi
if [ -f /sbin/ldconfig ]
then
   LDCONFIG="/sbin/ldconfig"
else
   for i in $(slocate ldconfig 2>/dev/null)
   do
      if [ $(file "$i"|grep -c ELF) -gt 0 ]
      then
        LDCONFIG="$i"
      fi
   done
fi
if [ -f /usr/bin/clang ]
then
   MYGCC=/usr/bin/clang
elif [ -f /usr/bin/gcc ]
then
   MYGCC=/usr/bin/gcc
else
   for i in $(slocate gcc 2>/dev/null)
   do
      if [ $(file "$i"|grep -c ELF) -gt 0 ]
      then
        MYGCC="$i"
      fi
   done
fi
if [ $(${MYGCC} 2>&1 |grep -c "not found") -gt 0 ]
then
   echo "No compiler found, it defaulted back to just 'cc' but we can't find it."
   echo "Please either install a compiler (gcc, clang, etc) or define it in your path."
   exit 1
fi
BETAOPT=0
DEFS="-Wall"
DATE="$(date +"%m%d%y")"
MORELIBS="\$(EXTLIBS)"
OPTIONS="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31"
C_OPTIONS=$(echo $OPTIONS|wc -w)
BOPTIONS="1 2 3 4 5 6"
C_BOPTIONS=$(echo $BOPTIONS|wc -w)
DOPTIONS="1 2 3"
C_DOPTIONS=$(echo $DOPTIONS|wc -w)
LOPTIONS="1 2 3 4 5"
C_LOPTIONS=$(echo $LOPTIONS|wc -w)
AOPTIONS="1 2 3"
C_AOPTIONS=$(echo $AOPTIONS|wc -w)
DBOPTIONS="1 2 3"
C_DBOPTIONS=$(echo $DBOPTIONS|wc -w)
DCONFIGS="29 30 31"
REPEAT=1
for i in ${OPTIONS}
do
   X[${i}]=" "
done
# Check if libpcre is installed
if [ $(${MYGCC} -lpcre 2>&1|grep -c -e "to find" -e "ot find" -e "cannot find") -eq 0 ]
then
   X[25]="X"
fi

# check if openssl is installed -- if not disable option
if [ $(${MYGCC} -lssl 2>&1|grep -c -e "to find" -e "ot find" -e "cannot find") -eq 1 ]
then
   X[24]="X"
   NOSSL=1
fi

# disable debugmon if on win10 bash
if [ -f /proc/version ]
then
   if [ "$(grep -c Microsoft /proc/version 2>/dev/null)" -gt 0 -o "$(grep -ic cygwin /proc/version 2>/dev/null)" -gt 0 ]
   then
      X[19]="X"
   fi
fi
ldd --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   pkg --version > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      gl_ldd=$(pkg info|grep -i "^glib-"|cut -f2 -d".")
      gl_ldd1=$(echo ${gl_ldd}|cut -f2 -d"-"|cut -f1 -d".")
      gl_ldd2=$(echo ${gl_ldd}|cut -f2 -d".")
   else
      # we have no idea of the version.  Pretend no.
      gl_ldd1=0
      gl_ldd2=0
   fi        
else
   gl_ldd=$(ldd --version 2>/dev/null|head -1|cut -f2 -d')')
   gl_ldd1=$(echo ${gl_ldd}|cut -f1 -d".")
   gl_ldd2=$(echo ${gl_ldd}|cut -f2 -d".")
fi
if [ -z "${gl_ldd1}" ]
then
   gl_ldd1=0
fi
if [ -z "${gl_ldd2}" ]
then
   gl_ldd2=0
fi
if [ ${gl_ldd1} -ge 2 -a ${gl_ldd2} -ge 7 ]
then
   X[26]="X"
fi
for i in ${BOPTIONS}
do
   XB[${i}]=" "
done
# default SBUF to 64 chars
XB[3]="X"
# default to QDBM database
XB[5]="X"
# default to MAX_GLOBAL_BOOST
X[29]="#"
# default to ATRCACHE_MAX
X[30]="#"
# default to MAXPIDS
X[31]="#"

for i in ${DOPTIONS}
do
   XD[${i}]=" "
done

for i in ${LOPTIONS}
do
   XL[${i}]=" "
done
XL[1]="X"

for i in ${AOPTIONS}
do
   XA[${i}]=" "
done
XA[2]="X"

for i in ${DBOPTIONS}
do
   MS[${i}]=" "
done

# Load default options
if [ -f ./asksource.default ]
then
   . $(pwd)/asksource.default
fi
DEF[1]="-DUSE_SIDEEFFECT"
DEF[2]="-DTINY_U"
DEF[3]="-DMUX_INCDEC"
DEF[4]="-DSOFTCOM"
DEF[5]=""
DEF[6]="-DUSECRYPT"
DEF[7]="-DPLUSHELP"
DEF[8]="-DPROG_LIKEMUX"
DEF[9]="-DENABLE_COMMAND_FLAG"
DEF[10]="-DATTR_HACK"
DEF[11]="-DREALITY_LEVELS"
DEF[12]="-DEXPANDED_QREGS"
DEF[13]="-DZENTY_ANSI"
DEF[14]="-DMARKER_FLAGS"
DEF[15]="-DBANGS"
DEF[16]="-DPARIS"
DEF[17]="-DOLD_SETQ"
DEF[18]="-DSECURE_SIDEEFFECT"
DEF[19]="-DNODEBUGMONITOR"
DEF[20]="-DIGNORE_SIGNALS"
DEF[21]="-DOLD_REALITIES"
DEF[22]="-DMUXCRYPT"
DEF[23]=""
DEF[24]=""
DEF[25]="-DPCRE_SYSLIB"
DEF[26]="-DCRYPT_GLIB2"
DEF[27]="-DENABLE_WEBSOCKETS"
DEF[28]="-DENABLE_LUA"
DEF[29]=""
DEF[30]=""
DEF[31]=""

# set config parameters if found in pre-existing file
#---- Max global registers
lc_value="$(grep -ow "DDYN_MAX_GLOBAL_BOOST=[0-9]*" ../src/custom.defs|cut -f2 -d"=")"
echo ${lc_value}
lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
if [ -n "${lc_value}" ]
then
   DEFS[29]="-DDYN_MAX_GLOBAL_BOOST=${lc_value}"
fi
#---- Max atrcaches
lc_value="$(grep -ow "DDYN_ATRCACHE_MAX=[0-9]*" ../src/custom.defs|cut -f2 -d"=")"
echo ${lc_value}
lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
if [ -n "${lc_value}" ]
then
   DEFS[30]="-DDYN_ATRCACHE_MAX=${lc_value}"
fi
#---- Max pids
lc_value="$(grep -ow "DDYN_MAXPIDS=[0-9]*" ../src/custom.defs|cut -f2 -d"=")"
echo ${lc_value}
lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
if [ -n "${lc_value}" ]
then
   DEFS[31]="-DDYN_MAXPIDS=${lc_value}"
fi

if [ "${MYSQL_VER}" != "0" ]
then
   DEFDB[1]="\$(MYSQL_VERSION)"
else
   DEFDB[1]=""
fi
DEFDB[2]=""

DEFB[1]=""
DEFB[2]="\$(DR_DEF)"
DEFB[3]="-DSBUF64"
DEFB[4]="-DSQLITE"
DEFB[5]="-DQDBM"

DEFD[1]="-DMUSH_DOORS"
DEFD[2]="-DEMPIRE_DOORS"
DEFD[3]="-DPOP3_DOORS"
DEFL[1]=""
DEFL[2]="-DLBUF8"
DEFL[3]="-DLBUF16"
DEFL[4]="-DLBUF32"
DEFL[5]="-DLBUF64"
DEFA[1]="-DTINY_SUB"
DEFA[2]="-DC_SUB"
DEFA[3]="-DM_SUB"

###################################################################
# MENU - Main Menu for RhostMUSH Configuration Utility
###################################################################
mysql_set_args() {
looper=1
sql_vals=0
while [ ${looper} -eq 1 ]
do
clear
   echo "------------------------------------------------------------------------------"
   echo "1.  MySQL HostName: ${mysql_host}"
   echo "2.  MySQL UserName: ${mysql_user}"
   echo "3.  MySQL Password: ${mysql_pass}"
   echo "4.  MySQL Database: ${mysql_dbname}"
   echo "5.  MySQL Socket  : ${mysql_socket}"
   echo "6.  MySQL Port    : ${mysql_port}"
   echo "------------------------------------------------------------------------------"
   echo "[Q]   Go Back to Previous Menu"
   echo "------------------------------------------------------------------------------"
   echo ""
   echo "Select response: "|tr -d '\012'
   read ANS
   case ${ANS} in
      q|Q) looper=0
           ANS=""
           break
           ;;
      1) echo "[${mysql_host}]: "|tr -d '\012'
         sqlval=mysql_host
         ((sql_vals++))
         ;;
      2) echo "[${mysql_user}]: "|tr -d '\012'
         sqlval=mysql_user
         ((sql_vals++))
         ;;
      3) echo "[${mysql_pass}]: "|tr -d '\012'
         sqlval=mysql_pass
         ((sql_vals++))
         ;;
      4) echo "[${mysql_dbname}]: "|tr -d '\012'
         sqlval=mysql_dbname
         ((sql_vals++))
         ;;
      5) echo "[${mysql_socket}]: "|tr -d '\012'
         sqlval=mysql_socket
         ((sql_vals++))
         ;;
      6) echo "[${mysql_port}]: "|tr -d '\012'
         sqlval=mysql_port
         ((sql_vals++))
         ;;
      *) echo "Invalid argument"
         echo "<ENTER RETURN TO CONTINUE>"
         read ANS
         continue
         ;;
   esac
   read SQLANS
   if [ -n "${SQLANS}" ]
   then
      eval ${sqlval}=\${SQLANS}
   fi
done
if [ ${sql_vals} -eq 0 ]
then
   echo "No changes.  Aborted by user."
else
   echo "Creating new file."
   cat ../bin/mysql.blank > ../game/rhost_mysql.conf 2>/dev/null
   echo "# Generated from make config" >> ../game/rhost_mysql.conf
   echo "mysql_host ${mysql_host}" >> ../game/rhost_mysql.conf
   echo "mysql_user ${mysql_user}" >> ../game/rhost_mysql.conf
   echo "mysql_pass ${mysql_pass}" >> ../game/rhost_mysql.conf
   echo "mysql_base ${mysql_dbname}" >> ../game/rhost_mysql.conf
   echo "mysql_socket ${mysql_socket}" >> ../game/rhost_mysql.conf
   echo "mysql_port ${mysql_port}" >> ../game/rhost_mysql.conf
   echo "Finished."
fi
echo "< HIT RETURN KEY TO CONTINUE >"
read ANS
}

mysqlmenu() {
if [ "${MYSQL_VER}" = "0" ]
then
   echo "WARNING: MySQL config is not being detected on this server."
   echo "         if you do not have mysql libraries installed, enabling"
   echo "         this will cause the compile to fail."
   if [ -n "$MYSQL_VER2}" ]
   then
      echo "         I think MySQL may be installed and version is ${MYSQL_VER2}"
   fi
   echo "<ENTER RETURN TO CONTINUE>"
   read ANS
fi
clear
echo "               RhostMUSH MySQL / MariaDB Configuration Utility"
echo " NOTE:  This will only manually configure the 'game' directory.  Custom"
echo "        directories will have to be manually modified through rhost_mysql.conf"
echo ""
echo " For LOCAL INSTALLS please specify a valid MySQL Socket.  For ubuntu systems"
echo " this is generally located in /var/run/mysqld/mysqld.sock"
echo " Feel free to specify 'NULL' as the socket for it to use the default value."
echo ""
echo " For REMOTE INSTALLS you may specify /dev/null for the socket."
echo "------------------------------------------------------------------------------"
echo "[${MS[1]}]  1. Toggle MySQL On/Off"
echo "[#]  2. Change MySQL Data"
echo "[${MS[3]}]  3. Force MySQL even if not detected. (Will not compile if no SQL!)"
echo "------------------------------------------------------------------------------"
echo "MySQL HostName: ${mysql_host}"
echo "MySQL UserName: ${mysql_user}"
echo "MySQL Password: ${mysql_pass}"
echo "MySQL Database: ${mysql_dbname}"
echo "MySQL Socket  : ${mysql_socket}"
echo "MySQL Port    : ${mysql_port}"
echo "------------------------------------------------------------------------------"
echo "[Q]   Go Back to Previous Menu"
echo "------------------------------------------------------------------------------"
echo ""
echo "Keys: [h]elp [i]nfo"
echo "      Or, you may select a number to toggle"
echo ""
echo "Please Enter selection: "|tr -d '\012'
}

ansimenu() {
clear
echo "             RhostMUSH ANSI / LAST COMMAND Configuration Utility"
echo "------------------------------------------------------------------------------"
echo ""
echo " ** IF NOT ENABLED, THE SUBSTITUTION ACTS AS A 'LAST COMMAND' SUBSTITUTION **"
echo "             ** ONE OF THESE SUBSTITUTIONS MUST BE SET TO ANSI **"
echo ""
echo "[${XA[1]}]   1. %x is ANSI sub     [${XA[2]}]  2. %c is ANSI sub     [${XA[3]}]  3. %m is ANSI sub"
echo ""
echo ""
echo "------------------------------------------------------------------------------"
echo "[Q]   Go Back to Previous Menu"
echo "------------------------------------------------------------------------------"
echo ""
echo "Keys: [h]elp [i]nfo"
echo "      Or, you may select a number to toggle"
echo ""
echo "Please Enter selection: "|tr -d '\012'
}
lbufmenu() {
lc_st1="$(ulimit -s)"
lc_st2="${lc_st1}"
if [ "${lc_st1}" = "unlimited" ]
then
   lc_st2="2000000000"
fi
clear
echo "                      RhostMUSH LBUFFER Configuration Utility"
echo "------------------------------------------------------------------------------"
echo ""
echo "  **REMEMBER TO SET YOUR OUTPUT_LIMIT CONFIG OPTION TO 4 TIMES THIS AMOUNT**  "
echo "  Your current stack size is: ${lc_st1} [${lc_st2}]"
echo ""
echo "[${XL[1]}]   1. 4K (Rhost)         [${XL[2]}]  2. 8K (Penn/TM3/Mux)  [${XL[3]}]  3. 16K"
echo "[${XL[4]}]   4. 32K                [${XL[5]}]  5. 64K"
echo ""
echo "------------------------------------------------------------------------------"
echo "[Q]   Go Back to Previous Menu"
echo "------------------------------------------------------------------------------"
echo ""
echo "Keys: [h]elp [i]nfo"
echo "      Or, you may select a number to toggle"
echo ""
echo "Please Enter selection: "|tr -d '\012'
}
doormenu() {
clear
echo "                    RhostMUSH Door Source Configuration Utility"
echo "------------------------------------------------------------------------------"
echo ""
echo "[${XD[1]}]   1. Mush Doors         [${XD[2]}]  2. Empire Doors       [${XD[3]}]  3. POP3 Doors"
echo ""
echo "------------------------------------------------------------------------------"
echo "[Q]   Go Back to Previous Menu"
echo "------------------------------------------------------------------------------"
echo ""
echo "Keys: [h]elp [i]nfo"
echo "      Or, you may select a number to toggle on/off"
echo ""
echo "Please Enter selection: "|tr -d '\012'
}

menu() {
clear
echo "                       RhostMUSH Source Configuration Utility"
echo ""
echo "------------------------------------------------------------------------------"
echo "[${X[1]}]  1. Sideeffects        [${X[2]}]  2. MUSH/MUX u()/zfun  [${X[3]}]  3. MUX inc()/dec()"
echo "[${X[4]}]  4. Disabled Comsys    [#]  5. ANSI SUBS (menu)   [${X[6]}]  6. crypt()/decrypt()"
echo "[${X[7]}]  7. +help hardcoded    [${X[8]}]  8. MUX @program       [${X[9]}]  9. COMMAND flag"
echo "[${X[10]}] 10. ~/_ attributes     [${X[11]}] 11. Reality Levels     [${X[12]}] 12. a-z setq support"
echo "[${X[13]}] 13. Enhanced ANSI      [${X[14]}] 14. Marker Flags       [${X[15]}] 15. Bang support"
echo "[${X[16]}] 16. Alternate WHO      [${X[17]}] 17. Old SETQ/SETR      [${X[18]}] 18. Secured Sideeffects"
echo "[${X[19]}] 19. Disable DebugMon   [${X[20]}] 20. Disable SIGNALS    [${X[21]}] 21. Old Reality Lvls" 
echo "[${X[22]}] 22. Read Mux Passwds   [${X[23]}] 23. Low-Mem Compile    [${X[24]}] 24. Disable OpenSSL"
echo "[${X[25]}] 25. Pcre System Libs   [${X[26]}] 26. SHA512 Passwords   [${X[27]}] 27. Websocket support"
echo "[${X[28]}] 28. Enable LUA in API  [${X[29]}] 29. Boost Setq Regs    [${X[30]}] 30. AtrCache Maximum"
echo "[${X[31]}] 31. Max PID Queues"
echo "--------------------------- Extended Support Additions -----------------------"
echo "[#] B1. MySQL Support      [${XB[2]}] B2. Door Support(Menu) [${XB[3]}] B3. 64 Char attribs"
echo "[${XB[4]}] B4. SQLite Support     [${XB[5]}] B5. QDBM DB Support    [#] B6. LBUF Settings (Menu)"
echo "------------------------------------------------------------------------------"
echo ""
echo "Keys: [h]elp [i]nfo [s]ave [l]oad [d]elete [c]lear [m]ark [b]rowse [r]un [q]uit"
echo "      [x]tra default cores (MUX, TinyMUSH3, Penn, Rhost-Default)"
echo "      Or, you may select a numer to toggle on/off"
echo "      '#' means you may drill down into that option for a sub-menu."
echo ""
echo "Please Enter selection: "|tr -d '\012'
}

###################################################################
# HELP - Main help interface for configuration utility
###################################################################
help() {
echo ""
echo "The following options are available..."
echo "h - help   : This shows this screen"
echo "i - info <option> : This gives information on the specified numeric toggle."
if [ $BETAOPT -eq 0 ]
then
   echo "s - save   : This optionally saves your configuration to one of 4 slots."
   echo "l - load   : This loads the options that you previously saved from a slot."
   echo "r - run    : This 'runs' the current configuration as selected."
   echo "d - delete : This deletes the saved configuration file."
   echo "m - mark   : This labels the saved file with a specified marker"
   echo "b - browse : This will allow you to browse your config files"
   echo "x - extras : This will allow you to load optional default configurations."
   echo "q - quit   : This quits/aborts the configurator without writing options."
fi
echo ""
echo "----------------------------------------------------------------------------"
echo "The selection screen allows you to toggle any option with [ ].  You may also"
echo "type 'i' for info which will give you detailed information of the number of"
echo "choice.  For example: i 15   (this will give you information on number 15)"
echo "----------------------------------------------------------------------------"
}

###################################################################
# INFO <arg> - Information for all the toggle options available
###################################################################
info() {
   [ -z "$RUNBETA" ] && RUNBETA=0
   if [ $BETAOPT -eq 1 ]
   then
      CNTCHK=$(expr $1 + 0 2>/dev/null)
      if [ -z "$CNTCHK" -o -z "$1" ]
      then
         INFOARG=0
      elif [ "$CNTCHK" -lt 1 -o "$CNTCHK" -gt 5 ]
      then
         INFOARG=0
      else
         INFOARG=$1
      fi
   else
      INFOARG=$1
   fi
   echo ""
   echo "----------------------------------------------------------------------------"
   case $INFOARG in
      1) if [ $BETAOPT -eq 1 ]
         then
            echo "This will enable the MUSH Doors portion of @door connectivity."
            echo "This is used with the doors.txt/doors.indx files for sites."
         elif [ $BETAOPT -eq 2 ]
         then
            echo ""
         elif [ $RUNBETA -eq 1 ]
         then
            echo "This enables you to automatically include the MYSQL definitions"
            echo "to the DEFs line in the Makefile.  This is to support if you have"
            echo "this package installed and should only be used then.  Otherwise,"
            echo "it's safe to leave it at the default."
         else
            echo "Sideeffects are functions like set(), pemit(), create(), and the like."
            echo "This must be enabled to be able to use these functions.  In addition,"
            echo "when enabeled, the SIDEFX flag must be individually set on anything"
            echo "you wish to use the sideeffects."
         fi
         ;;
      2) if [ $BETAOPT -eq 1 ]
         then
            echo "This will enable the Empire (Unix Game) door.  You must have"
            echo "A currently running unix game server running and listening on a port."
         elif [ $BETAOPT -eq 2 ]
         then
            echo ""
         elif [ $RUNBETA -eq 1 ]
         then
            echo "This option drills down to various door options that you can enable."
            echo "The currently supported production doors are the ones listed."
            echo "Keep in mind each one does require tweeking files."
         else
            echo "RhostMUSH, by default, have u(), ufun(), and zfun() functions parse by"
            echo "relation of the enactor instead of by relation of the target.  This is"
            echo "more MUSE compatable than MUSH.  If you wish to have it more compatable"
            echo "to MUX/TinyMUSH/Penn, then you need this enabled.  Keep in mind, turning"
            echo "off this compatibility WILL BREAK MUX/PENN/MUSH COMPATIBILITY!"
         fi
         ;;
      3) if [ $BETAOPT -eq 1 ]
         then
            echo "This enables the POP3 door interface.  This is still very limited"
            echo "in functionality and currently only displays total mail counts."
         elif [ $BETAOPT -eq 2 ]
         then
            echo ""
         elif [ $RUNBETA -eq 1 ]
         then
            echo "This allows you to have 64 character attribute length.  This in effect"
            echo "will set all SBUF's to 64 bit length instead of 32.  This has not been"
            echo "fully tested or regression tested so there could be some concern if"
            echo "you choose to use this.  If you find errors, report to us immediately."
         else
            echo "RhostMUSH, by default, uses inc() and dec() to increment and decrement"
            echo "setq registers.  This is, unfortunately, not the default behavior for"
            echo "MUX, Penn, or TinyMUSH.  By enabling this options, you switch the"
            echo "functionality of inc() and dec() to be like MUX/Penn/TinyMUSH."
         fi
         ;;
      4) if [ $RUNBETA -eq 1 ]
         then
            echo "This enables the SQLite library bindings. SQLite is a zero-config"
            echo "file-based SQL relational database system, similar to MySQL or"
            echo "PostgreSQL, but without the complexity (or fragility) of running a"
            echo "server."
         elif [ $BETAOPT -eq 2 ]
         then
            echo ""
         else
            echo "RhostMUSH has a very archiac and obtuse comsystem.  It does work, and"
            echo "is very secure and solid, but it lacks significant functionality."
            echo "You probably want to toggle this on and use a softcoded comsystem."
         fi
         ;;
      5) if [ $RUNBETA -eq 1 ]
         then
            echo "This enables the QDBM database manager instead of the default GDBM"
            echo "database manager.  This may be the preferred database eventhough"
            echo "it is considered 'beta' as this is not hampered by the attribute"
            echo "cap per dbref# (750 default) and is generally a faster and more"
            echo "robust database engine.  Be warned, however, that QDBM is NOT"
            echo "binary compatible with GDBM, so any existing databases"
            echo "WILL NOT LOAD.  You have to flatfile dump the database then"
            echo "db_load the flatfile into the database once qdbm is compiled."
         elif [ $BETAOPT -eq 2 ]
         then
            echo ""
         else
            echo "This submenu allows you to choose, which substitution act as ANSI"
            echo "code substitutions. These are the settings to mimic other codebases:"
            echo "       MUX: %x and %c are ANSI, %m is last command."
            echo "      PENN: %c is last command. There are no ANSI substutions."
            echo "TinyMUSH 3: %x is ANSI sub, %m is last command. %x depends on config."
         fi
         ;;
      6) if [ $RUNBETA -eq 1 ]
         then
            echo "This selection displays the menu allowing you to change the default"
            echo "size of Rhost's text buffers. The default is traditionally 4000 for"
            echo "Rhost, and 8192 for PennMUSH, TinyMUSH and MUX."
            echo "Now you can select your favorite length yourself, with the options "
            echo "of clasic 4k, 8K like other codebases, or even 16, 32 and 64K!     "
            echo "This increases both, the max length of attribute contents as well  "
            echo "as output length going to the client.                              "
            echo "IMPORTANT: the config parameter output_length should always be     "
            echo "4 times this setting. Rhost has a default output_limit config      "
            echo "setting of '16384' with the 4K LBUFs. Increase accordingly before  "
            echo "starting your game."
         else
            echo "RhostMUSH supports crypt() and decrypt() functions.  Toggle this"
            echo "if you wish to use them."
         fi
         ;;
      7) echo "RhostMUSH allows you to use a plushelp.txt file for +help.  This"
         echo "supports MUX/TinyMUSH3 in how +help is hardcoded to a text file."
         echo "Enable this if you wish to have a plushelp.txt file be used."
         ;;
      8) echo "RhostMUSH, by default, allows multiple arguments to be passed"
         echo "to @program.  This, unfortunately, is not how MUX does it, so"
         echo "if you wish a MUX compatable @program, enable this."
         ;;
      9) echo "RhostMUSH, optionally, allows you to use the COMMAND flag to"
         echo "specify what objects are allowed to run \$comands/^listens."
         echo "If you wish to have this flag enabled, toggle this option."
         ;;
     10) echo "RhostMUSH allows 'special' attributes that start with _ or ~."
         echo "By default, _ attributes are wiz settable and seeable only."
         echo "If you wish to have these special attributes, enable this option."
         ;;
     11) echo "Reality Levels is a document onto itself.  In essence, reality"
         echo "levels allow a single location to have multiple 'realities' where"
         echo "you can belong to one or more realities and interact with one or"
         echo "more realities.  It's an excellent way to handle invisibility,"
         echo "shrouding, or the like.  Enable this if you want it."
         ;;
     12) echo "Sometimes, the standard 10 (0-9) setq registers just arn't enough."
         echo "If you, too, find this to be the case, enable this option and it"
         echo "will allow you to have a-z as well."
         ;;
     13) echo "RhostMUSH handles ansi totally securely.  It does this by stripping"
         echo "ansi from all evaluation.  This does, however, have the unfortunate"
         echo "effect of making ansi very cumbersome and difficult to use.  If you"
         echo "have this uncontrollable desire to have color everywhere, then you"
         echo "probably want this enabled as it allows you to use ansi in most"
         echo "places without worry of stripping."
         ;;
     14) echo "Marker flags are your 10 dummy flags (marker0 to marker9).  These"
         echo "flags, are essentially 'markers' that you can rename at leasure."
         echo "If you have a desire for marker flags, enable this option."
         ;;
     15) echo "Bang support.  Very cool stuff.  It allows you to use ! for false"
         echo "and !! for true.  An example would be [!match(this,that)].  It"
         echo "also allows $! for 'not a string' and $!! for 'is a string'."
         echo "Such an example would be [$!get(me/blah)].  If you like this"
         echo "feature, enable this option.  You want it.  Really."
         ;;
     16) echo "This is an alternate WHO listing.  It's a tad longer for the"
         echo "display and will switch ports to Total Cmds on the WHO listings."
         ;;
     17) echo "This goes back to an old implementation of the setq/setr register"
         echo "emplementation.  This may make it more difficult to use ansi but"
         echo "the newer functions would still exist.  In essence, this will"
         echo "switch SETQ with SETQ_OLD and SETR with SETR_OLD in the code."
         ;;
     18) echo "This shores up sideeffects for pemit(), remit(), zemit(), and"
         echo "others to avoid a double evaluation.  You probably want this"
         echo "enabled as it mimics MUX/PENN, but if you have an old Rhost"
         echo "that used sideeffects, this can essentially break compatabiliity."
         ;;
     19) echo "This disables the debug monitor (debugmon) from working within Rhost."
         echo "There is really no need to disable this, unless you are in a chroot"
         echo "jail or in some way are on a restricted unix shell that does not allow"
         echo "you to use shared memory segments."
         ;;
     20) echo "This is intended to disable signal handling in RhostMUSH.  Usually,"
         echo "you can send TERM, USR1, and USR2 signals to reboot, flatfile, or"
         echo "shutdown the mush."
         ;;
     21) echo "The Reality Levels in use prior required the chkreality toggle to"
         echo "be used for options 2 & 3 in locktypes.  If the lock did not exist"
         echo "it would assume failure even if no chkreality toggle was set.  This"
         echo "was obviously an error.  However, if this is enabled, then this"
         echo "behavior will remain so that any preexisting code will work as is."
         ;;
     22) echo "This allows you to natively read MUX2 set SHA1 passwords in a"
         echo "converted database.  If you change the password, it will use the"
         echo "Rhost specific password system and overwrite the SHA1 password."
         echo "You only need to use this if you are using a MUX2 converted flatfile."
         ;;
     23) echo "This is a low memory compile option.  If you are running under a"
         echo "Virtual Machine, or have low available memory, the compile may"
         echo "error out saying out of memory, unable to allocate memory, or"
         echo "similiar messages.  Enabling this option should bypass this."
         ;;
     24) echo "Sometimes, you may have a third party SSL package that is"
         echo "incompatible with the development library for OpenSSL.  In such"
         echo "a case, select this option to disable OpenSSL from compiling."
         ;;
     25) echo "The system dependant PCRE Library will be much  much faster"
         echo "than the one included with the source.  Enabling this option"
         echo "uses the system's PCRE Library. Disabling it uses a PCRE"
         echo "library supplied by RhostMUSH that should only be used if the"
         echo "system library caused compilation issues."
         ;;
     26) echo "This option encrypts your passwords using a random seed and"
         echo "the SHA512 encryption method.  It will fall back to standard"
         echo "DES encryption for compatibility regardless."
         ;;
     27) echo "This option enables the support for Websockets introduced in"
         echo "RhostMUSH release 4.1.0p2. This option allows you to use the"
         echo "Websocket protocol for connections with the games you run."
         ;;
     28) echo "This option enables the support for using LUA in the WebAPI"
         echo "calls that you send to RhostMUSH."
         ;;
     29) echo "This option allows you to expand on the normal setq registers you have"
         echo "defined.  If you select just 0-9 or 0-9 and a-z it will still apply.  The"
         echo "default value for this is '0' but it has been tested with upwards to"
         echo "10,000 or more.  Be aware that each additional register has an overhead"
         echo "of (LBUF_SIZE * 2) + SBUF_SIZE in total memory  So with 16K lbufs, about"
         echo "33K overhead for every additional register.  It can get expensive fast."
         ;;
     30) echo "This option is used to set the attribute cache maximum. @atrcache is used"
         echo "to set global buffering for code that could potentially cause a high load"
         echo "on the mush from executing code.  This is useful for things such as"
         echo "extreme use of searches or other code that may not change consistently"
         echo "and have moststly static values.  @atrcache allows you to specify a time"
         echo "on recaching.  The default value for this is 200.  The overhead of each"
         echo "cache is (LBUF_SIZE * 2) + SBUF_SIZE"
         echo ""
         echo "Note: These only take up memory when in use.  Otherwise the only overhead"
         echo "      is the array allocation (40 bytes per allocation)."
         ;;
     31) echo "This specifies the maximum PID processes allowed at any given time."
         echo "The default is 32k but can be anywhere between 2000-999000"
         echo ""
         echo "Note: This has a minimal hit on memory usage the larger the pids."
         ;;
     B*|b*) RUNBETA=1
         info $(echo $1|cut -c2-)
         RUNBETA=0
         ;;
      *) echo "Not a valid number.  "
         if [ $BETAOPT -eq 1 ]
         then
            echo "Please select between 1 and ${C_DOPTIONS} for information."
         elif [ $BETAOPT -eq 2 ]
         then
            echo "Please select between 1 and ${C_LOPTIONS} for information."
         else
            echo "Please select between 1 and ${C_OPTIONS} for information."
            echo "Please select between B1 and B${C_BOPTIONS} for beta."
         fi
         ;;
   esac
   echo "----------------------------------------------------------------------------"
}

###################################################################
# PARSE - Parse the argument the player types
###################################################################
parse() {
   ARG=$(echo $1|tr '[:upper:]' '[:lower:]')
   ARGNUM=$(expr ${ARG} + 0 2>/dev/null)
   if [ $BETAOPT -eq 1 ]
   then
      if [ "$ARG" != "q" -a "$ARG" != "i" -a "$ARG" != "h" ]
      then
         if [ -z "$ARGNUM" ]
         then
            ARG="NULL"
         elif [ $ARGNUM -lt 1 -o $ARGNUM -gt 3 ]
         then
            ARG="NULL"
         fi
      fi
   fi
   if [ $BETAOPT -eq 2 ]
   then
      if [ "$ARG" != "q" -a "$ARG" != "h" ]
      then
         if [ -z "$ARGNUM" ]
         then
            ARG="NULL"
         elif [ $ARGNUM -lt 1 -o $ARGNUM -gt 6 ]
         then
            ARG="NULL"
         fi
      fi
   fi
   if [ $BETAOPT -eq 3 ]
   then
      if [ "$ARG" != "q" -a "$ARG" != "h" ]
      then
         if [ -z "$ARGNUM" ]
         then
            ARG="NULL"
         elif [ $ARGNUM -lt 1 -o $ARGNUM -gt 3 ]
         then
            ARG="NULL"
         fi
      fi
   fi
   if [ $BETAOPT -eq 4 ]
   then
      if [ "$ARG" != "q" -a "$ARG" != "h" ]
      then
         if [ -z "$ARGNUM" ]
         then
            ARG="NULL"
         elif [ $ARGNUM -lt 1 -o $ARGNUM -gt 2 ]
         then
            ARG="NULL"
         fi
      fi
   fi
   case ${ARG} in
      x) xtraopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      r) REPEAT=0 
         echo "Running with current configuration..."
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      l) loadopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      q) if [ ${BETAOPT} -ne 0 ]
         then
            BETACONTINUE=0
         else
            echo "Aborting"
            echo "< HIT RETURN KEY TO CONTINUE >"
            read ANS
            exit 1
         fi
         ;;
      s) saveopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      m) markopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      b) browseopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      c) clearopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      d) deleteopts
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      i) info $2
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      h) help
         echo "< HIT RETURN KEY TO CONTINUE >"
         read ANS
         ;;
      b*) TST=$(expr $(echo $1|cut -c2-) + 0 2>/dev/null)
         BETAOPT=0
         if [ -z "$TST" ]
         then
            echo "ERROR: Invalid option '$1'"
            echo "< HIT RETURN KEY TO CONTINUE >"
            read ANS
         elif [ "$TST" -gt 0 -a "$TST" -le ${C_BOPTIONS} ]
         then  
            if [ $TST -eq 2 ]
            then
               BETAOPT=1
            elif [ $TST -eq 6 ]
            then
               BETAOPT=2
            elif [ $TST -eq 1 ]
            then
               BETAOPT=4
            else
               if [ "${XB[${TST}]}" = "X" ]
               then
                  if [ "${TST}" -eq 5 ]
                  then
                    if [ "${XL[1]}" = "X" ]
                    then
                      XB[${TST}]=" "
                    fi
                  else
                    XB[${TST}]=" "
                  fi
               else
                  XB[${TST}]="X"
               fi
            fi
         else
            echo "ERROR: Invalid option '$1'"
            echo "< HIT RETURN KEY TO CONTINUE >"
            read ANS
         fi
         ;;
      *) TST=$(expr $1 + 0 2>/dev/null)
         if [ -z "$TST" ]
         then
            echo "ERROR: Invalid option '$1'"
            echo "< HIT RETURN KEY TO CONTINUE >"
            read ANS
         elif [ ${BETAOPT} -eq 0 -a "$TST" -eq 5 ]
         then
            BETAOPT=3
         elif [ ${BETAOPT} -eq 1 -a "$TST" -gt 0 -a "$TST" -le ${C_DOPTIONS} ]
         then
            if [ "${XD[$1]}" = "X" ]
            then
               XD[$1]=" "
            else
               XD[$1]="X"
            fi
         elif [ ${BETAOPT} -eq 2 -a "$TST" -gt 0 -a "$TST" -le ${C_LOPTIONS} ]
         then
            for i in ${LOPTIONS}
            do
              XL[${i}]=" "
            done
            XL[$1]="X"
            if [ ${i} -ne 1 ]
            then
              XB[5]="X"
            fi
         elif [ ${BETAOPT} -eq 3 -a "$TST" -gt 0 -a "$TST" -le ${C_AOPTIONS} ]
         then
            if [ "${XA[$1]}" = "X" ]
            then
               XA[$1]=" "
            else
               XA[$1]="X"
            fi
            if [ "${XA[1]}" != "X" -a "${XA[2]}" != "X" -a "${XA[3]}" != "X" ]
            then
               XA[2]="X"
            fi
         elif [ ${BETAOPT} -eq 4 -a "$TST" -gt 0 -a "$TST" -le ${C_DBOPTIONS} ]
         then
            if [ "${MS[$1]}" = "X" ]
            then
               MS[$1]=" "
            else
               MS[$1]="X"
            fi
            if [ "$TST" -eq 2 ]
            then
               mysql_set_args
            fi
         elif [ ${BETAOPT} -eq 0 -a "$TST" -gt 0 -a "$TST" -le ${C_OPTIONS} ]
         then  
            case "$1" in
               24) # ssl/websocket special handler
                  if [ "$NOSSL" = "1" ]
                  then
                     echo "SSL is not detected on this system.  Please install OpenSSL libraries."
                     echo "< HIT RETURN KEY TO CONTINUE >"
                     X[$1]="X"
                     read ANS
                  else
                     if [ "${X[$1]}" = "X" ]
                     then
                        X[$1]=" "
                     else
                        X[$1]="X"
                        if [ "${X[27]}" = "X" ]
                        then
                           echo "Disabling WebSockets as it requires SSL."
                           X[27]=" "
                           read ANS
                        fi
                     fi
                  fi
                  ;;
               27) # websocket special handler
                  if [ "$NOSSL" = "1" ]
                  then
                     echo "WebSockets requires SSL libraries."
                     echo "SSL is not detected on this system.  Please install OpenSSL libraries."
                     echo "< HIT RETURN KEY TO CONTINUE >"
                     X[$1]=" "
                     read ANS
                  elif [ "${X[24]}" = "X" ]
                  then
                     echo "WebSockets requires SSL libraries."
                     echo "Please enable them first"
                     read ANS
                  else
                     if [ "${X[$1]}" = "X" ]
                     then
                        X[$1]=" "
                     else
                        X[$1]="X"
                     fi
                  fi
                  ;;
               29) # handle additional registers here
                   clear
                   lc_stack=$(ulimit -s)
                   if [ "${lc_stack}" = "unlimited" ]
                   then
                      lc_stack="2000000000"
                   fi
                   # load the buffer for this make file option
                   if [ -n "${lc_opt29}" ]
                   then
                      lc_value="${lc_opt29}"
                   else
                      # first let's query the current configs for the game
                      lc_value="$(cat ../src/custom.defs|grep -v "^#"|grep -ow "DDYN_MAX_GLOBAL_BOOST=[0-9]*"|cut -f2 -d"=")"
                      # sanitize the value just incase
                      lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
                      # if empty pull from the config file
                      if [ -z "${lc_value}" ]
                      then
                         lc_value=$(grep "^#define MAX_GLOBAL_BOOST" ../hdrs/config.h|grep -v DYN|awk '{print $3}')
                      fi
                   fi
                   if [ "${XL[1]}" = "X" ]
                   then
                      lc_bufsize=4000
                   elif [ "${XL[2]}" = "X" ]
                   then
                      lc_bufsize=8192
                   elif [ "${XL[3]}" = "X" ]
                   then
                      lc_bufsize=16384
                   elif [ "${XL[4]}" = "X" ]
                   then
                      lc_bufsize=32768
                   elif [ "${XL[5]}" = "X" ]
                   then
                      lc_bufsize=65536
                   else
                      lc_bufsize=4000
                   fi

                   ((lc_exbuf=(${lc_bufsize}*2+64)*${lc_value}))
                   if [ ${lc_exbuf} -gt 1000000000 ]
                   then
                      ((lc_exbuf=${lc_exbuf}/1000000000))
                      lc_exbufc="Gb"
                   elif [ ${lc_exbuf} -gt 1000000 ]
                   then
                      ((lc_exbuf=${lc_exbuf}/1000000))
                      lc_exbufc="Mb"
                   else
                      ((lc_exbuf=${lc_exbuf}/1000))
                      lc_exbufc="Kb"
                   fi
                   ((lc_extrastack=${lc_bufsize} * ${lc_value} / 1000 * 2 ))
                   ((lc_test=${lc_stack} - 16536 - ${lc_extrastack}))
                   echo "           RhostMUSH config.h Modification & Configuration Utility"
                   echo "------------------------------------------------------------------------------"
                   echo "Each additional register will cause (LBUF_SIZE * 2) + SBUF_SIZE overhead."
                   echo "Please plan accordingly.  With a-z enabled you have 36 registers pre-allocated."
                   echo "This is generally sufficient.  If not, it's recommended not to exceed more than"
                   echo "a few hundred, but with some performance pentalties as well as significantly"
                   echo "higher memory utilization you could potentially have several thousand."
                   echo "Increased registers will also increase process stack space requirements."
                   echo "The general suggestion is use sparingly."
                   echo ""
                   echo "Note: to increase stack space use ulimit -s <size> at the shell."
                   echo ""
                   echo "The default value for this parameter is generally '0'"
                   echo ""
                   if [ ${lc_test} -lt 0 ]
                   then
                      if [ ${lc_stack} -lt 16536 ]
                      then
                         ((lc_test=16536 + ${lc_extrastack} - ${lc_stack}))
                      else
                         lc_test=${lc_extrastack}
                      fi
                      echo "Note: Your stack space is ${lc_stack}.  You should increase your stack space by: ${lc_test}"
                   else
                      if [ ${lc_stack} -gt 1999999999 ]
                      then
                         echo "Note: Your stack space is unlimited and should be sufficient"
                      else
                         echo "Note: Your stack space is ${lc_stack} and should be sufficient"
                      fi
                   fi
                   echo ""
                   echo "Currently you have [34;1m${lc_value}[0m additional registers [${lc_exbuf}${lc_exbufc} memory]."
                   echo "------------------------------------------------------------------------------"
                   echo ""
                   echo "Enter value (RETURN TO ABORT): "|tr -d '\012'
                   read ANS
                   lc_conv=$(echo ${ANS}|tr -cd '0-9')
                   if [ -z "${lc_conv}" ]
                   then
                      echo "Warning: number not given.  Defaulting back to '${lc_value}'"
                      lc_conv=${lc_value}
                   fi
                   if [ ${lc_conv} -eq ${lc_value} ]
                   then
                      echo "Values are unchanged.  No changes will be applied.  Hit <RETURN> to continue."
                      read ANS
                   else
                      ((lc_exbuf2=(${lc_bufsize}*2+64)*${lc_conv}))
                      if [ ${lc_exbuf2} -gt 1000000000 ]
                      then
                         ((lc_exbuf2=${lc_exbuf2}/1000000000))
                         lc_exbuf2c="Gb"
                      elif [ ${lc_exbuf2} -gt 1000000 ]
                      then
                         ((lc_exbuf2=${lc_exbuf2}/1000000))
                         lc_exbuf2c="Mb"
                      else
                         ((lc_exbuf2=${lc_exbuf2}/1000))
                         lc_exbuf2c="Kb"
                      fi
                      ((lc_extrastack2=${lc_bufsize} * ${lc_conv} / 1000 * 2 ))
                       ((lc_test2=${lc_stack} - 16536 - ${lc_extrastack2}))
                      echo "Old memory use was ${lc_exbuf}${lc_exbufc} new memory use will be ${lc_exbuf2}${lc_exbuf2c}."
                      echo "Old stack extra usage was ${lc_extrastack} new stack extra ussage will be ${lc_extrastack2}."
                      if [ ${lc_test2} -lt 0 -a ${lc_conv} -gt 0 ]
                      then
                         echo "[33;1mWarning:[0m Current stack of ${lc_stack} insufficient for new register value."
                         if [ ${lc_stack} -lt 16536 ]
                         then
                            ((lc_recommend=16536 + ${lc_extrastack2} + ${lc_extrastack2}))
                         else
                            ((lc_recommend=${lc_stack} + ${lc_extrastack2} + ${lc_extrastack2}))
                         fi
                         echo "[33;1mRecommanded MINIMUM setting is [32;1m${lc_recommend}[0m"
                      else
                         echo "Current stack of ${lc_stack} should be sufficient for the extra registers."
                      fi
                      echo "Old value was [34;1m${lc_value}[0m new value is [31;1m${lc_conv}[0m.  Is this correct? (Y/N): "|tr -d '\012'
                      read ANS
                      if [ "${ANS}" = "Y" -o "${ANS}" = "y" ]
                      then
                         # let's not modify the config file
                         # sed -i "s^#define MAX_GLOBAL_BOOST.*/\*^#define MAX_GLOBAL_BOOST\t${lc_conv}\t/*^g" ../hdrs/config.h
                         DEFS[29]="-DDYN_MAX_GLOBAL_BOOST=${lc_conv}"
                         lc_opt29="${lc_conv}"
                         echo "Valuue modified.  Hit <RETURN> to continue."
                         read ANS
                      else
                         echo "Changes aborted by user.  Hit <RETURN> to continue."
                         read ANS
                      fi
                   fi
                  ;;
               30) # handle additional registers here
                   clear
                   # load the buffer for this make file option
                   if [ -n "${lc_opt30}" ]
                   then
                      lc_value="${lc_opt30}"
                   else
                      # first let's query the current configs for the game
                      lc_value="$(grep -ow "DDYN_ATRCACHE_MAX=[0-9]*" ../src/custom.defs|cut -f2 -d"=")"
                      # sanitize the value just incase
                      lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
                      # if empty pull from the config file
                      if [ -z "${lc_value}" ]
                      then
                         lc_value=$(grep "^#define ATRCACHE_MAX" ../hdrs/config.h|grep -v DYN|awk '{print $3}')
                      fi
                   fi
                   if [ "${XL[1]}" = "X" ]
                   then
                      lc_bufsize=4000
                   elif [ "${XL[2]}" = "X" ]
                   then
                      lc_bufsize=8192
                   elif [ "${XL[3]}" = "X" ]
                   then
                      lc_bufsize=16384
                   elif [ "${XL[4]}" = "X" ]
                   then
                      lc_bufsize=32768
                   elif [ "${XL[5]}" = "X" ]
                   then
                      lc_bufsize=65536
                   else
                      lc_bufsize=4000
                   fi
                   
                   ((lc_exbuf=(${lc_bufsize}*2+64)*${lc_value}))
                   if [ ${lc_exbuf} -gt 1000000000 ]
                   then
                      ((lc_exbuf=${lc_exbuf}/1000000000))
                      lc_exbufc="Gb"
                   elif [ ${lc_exbuf} -gt 1000000 ]
                   then
                      ((lc_exbuf=${lc_exbuf}/1000000))
                      lc_exbufc="Mb"
                   else
                      ((lc_exbuf=${lc_exbuf}/1000))
                      lc_exbufc="Kb"
                   fi
                   echo "           RhostMUSH config.h Modification & Configuration Utility"
                   echo "------------------------------------------------------------------------------"
                   echo "This is used with the @atrcache command set."
                   echo "Each additional AtrCache will cause (LBUF_SIZE * 2) + SBUF_SIZE overhead."
                   echo "Please plan accordingly.  200 (the default) is generally sufficient."
                   echo "If not, it's recommended not to exceed more than a thousand"
                   echo "as higher memory utilization could negatively impact your game's performance."
                   echo "The general suggestion is use sparingly."
                   echo ""
                   echo "These will only be allocated when in use.  Overhead otherwise is 40 bytes each."
                   echo "The default value for this parameter is generally '200'"
                   echo ""
                   echo "Currently you have [34;1m${lc_value}[0m atrcaches [${lc_exbuf}${lc_exbufc} memory]."
                   echo "------------------------------------------------------------------------------"
                   echo ""
                   echo "Enter value (RETURN TO ABORT): "|tr -d '\012'
                   read ANS
                   lc_conv=$(echo ${ANS}|tr -cd '0-9')
                   if [ -z "${lc_conv}" ]
                   then
                      echo "Warning: number not given.  Defaulting back to '${lc_value}'"
                      lc_conv=${lc_value}
                   fi
                   if [ ${lc_conv} -eq ${lc_value} ]
                   then
                      echo "Values are unchanged.  No changes will be applied.  Hit <RETURN> to continue."
                      read ANS
                   else
                      ((lc_exbuf2=(${lc_bufsize}*2+64)*${lc_conv}))
                      if [ ${lc_exbuf2} -gt 1000000000 ]
                      then
                         ((lc_exbuf2=${lc_exbuf2}/1000000000))
                         lc_exbuf2c="Gb"
                      elif [ ${lc_exbuf2} -gt 1000000 ]
                      then
                         ((lc_exbuf2=${lc_exbuf2}/1000000))
                         lc_exbuf2c="Mb"
                      else
                         ((lc_exbuf2=${lc_exbuf2}/1000))
                         lc_exbuf2c="Kb"
                      fi
                      echo "Old memory use was ${lc_exbuf}${lc_exbufc} new memory use will be ${lc_exbuf2}${lc_exbuf2c}."
                      echo "Old value was [34;1m${lc_value}[0m new value is [31;1m${lc_conv}[0m.  Is this correct? (Y/N): "|tr -d '\012'
                      read ANS
                      if [ "${ANS}" = "Y" -o "${ANS}" = "y" ]
                      then
                         # let's not modify the config file
                         # sed -i "s^#define ATRCACHE_MAX.*/\*^#define ATRCACHE_MAX\t\t${lc_conv}\t/*^g" ../hdrs/config.h
                         echo "Valuue modified.  Hit <RETURN> to continue."
                         DEFS[30]="-DDYN_ATRCACHE_MAX=${lc_conv}"
                         lc_opt30="${lc_conv}"
                         read ANS
                      else
                         echo "Changes aborted by user.  Hit <RETURN> to continue."
                         read ANS
                      fi
                   fi
                  ;;
               31) # handle additional registers here
                   clear
                   # load the buffer for this make file option
                   if [ -n "${lc_opt31}" ]
                   then
                      lc_value="${lc_opt31}"
                   else
                      # first let's query the current configs for the game
                      lc_value="$(grep -ow "DDYN_MAXPIDS=[0-9]*" ../src/custom.defs|cut -f2 -d"=")"
                      # sanitize the value just incase
                      lc_value=$(echo "${lc_value}"|tr -cd '[0-9]')
                      # if empty pull from the config file
                      if [ -z "${lc_value}" ]
                      then
                         lc_value=$(grep "^#define MUMAXPID" ../src/cque.c|grep -v DYN|awk '{print $3}')
                         if [ -z "${lc_value}" ]
                         then
                            # yo shit broke set to long time default
                            lc_value=32767
                         fi
                      fi
                   fi
                   echo "           RhostMUSH config.h Modification & Configuration Utility"
                   echo "------------------------------------------------------------------------------"
                   echo "The PID processes will take up 2 bytes of memory for every pid allocated."
                   echo "By default there is 32,767 process PIDs that can be queud at any given time."
                   echo "This is generally sufficient.  If not, you may specify a value that is between"
                   echo "2000 and 999000.  2000 is the absolute minimum queuss that should ever be defined"
                   echo "as anything lower could stop your game from running successfully or properly."
                   echo ""
                   echo "The default value for this parameter is generally '32767'"
                   echo ""
                   echo "The max PID count is currently  [34;1m${lc_value}[0m."
                   echo "------------------------------------------------------------------------------"
                   echo ""
                   echo "Enter value (RETURN TO ABORT): "|tr -d '\012'
                   read ANS
                   lc_conv=$(echo ${ANS}|tr -cd '0-9')
                   if [ -z "${lc_conv}" ]
                   then
                      echo "Warning: number not given.  Defaulting back to '${lc_value}'"
                      lc_conv=${lc_value}
                   fi
                   if [ ${lc_conv} -eq ${lc_value} ]
                   then
                      echo "Values are unchanged.  No changes will be applied.  Hit <RETURN> to continue."
                      read ANS
                   elif [ ${lc_conv} -lt 2000 -o ${lc_conv} -gt 990000 ]
                   then
                      echo "Value is invalid.  Must be > 2000 or < 990000.  No changes will be applied.  Hit <RETURN> to continue."
                      read ANS
                   else
                      lc_value=${lc_conv}
                      echo "New value set to ${lc_conv}.  Hit <RETURN> to continue."
                      DEFS[31]="-DDYN_MAXPIDS=${lc_conv}"
                      lc_opt31="${lc_conv}"
                      read ANS
                   fi
                  ;;
                *) # handle all other conditions
                  if [ "${X[$1]}" = "X" ]
                  then
                     X[$1]=" "
                  else
                     X[$1]="X"
                  fi
                  ;;
            esac
         else
            echo "ERROR: Invalid option '$1'"
            echo "< HIT RETURN KEY TO CONTINUE >"
            read ANS
         fi
         ;;        
   esac  
}

###################################################################
# CLEAROPTS - clear current configurations to blank slate
###################################################################
clearopts() {
   for i in ${OPTIONS}
   do
      X[${i}]=" "
   done
   for i in ${BOPTIONS}
   do
      XB[${i}]=" "
   done
   for i in ${DOPTIONS}
   do
      XD[${i}]=" "
   done
   for i in ${LOPTIONS}
   do
      XL[${i}]=" "
   done
      XL[1]="X"
   for i in ${AOPTIONS}
   do
      XA[${i}]=" "
   done
      XA[2]="X"
   echo "Options have been cleared."
}

###################################################################
# DELETEOPTS - Delete previous config options
###################################################################
deleteopts() {
   LOOPIE=1
   VALID=""
   echo "Delete which slot:"
   while [ ${LOOPIE} -eq 1 ]
   do
      for i in 0 1 2 3
      do
         if [ -f asksource.save${i}.mark ]
         then
            MARKER=$(cat asksource.save${i}.mark)
         else
            MARKER=""
         fi
         if [ -s asksource.save${i} ]
         then
            echo "${i}. Slot #${i} In-Use [${MARKER:-GENERIC}]"
            VALID="${VALID} ${i}"
         fi
      done
      VALID=$(echo ${VALID})
      if [ -z "${VALID}" ]
      then
         echo "You have no options to delete."
         LOOPIE=0
         VALID=XXX
         break
      fi
      VALID="${VALID} Q q"
      echo "Q. Abort screen."
      echo ""
      echo "Please enter selection: "|tr -d '\012'
      read SAVEANS
      CHK=$(echo ${VALID}|grep -c "${SAVEANS}")
      if [ ${CHK} -eq 0 ]
      then
         SAVEANS=XXX
      fi
      LOOPIE=0
      case ${SAVEANS} in
         0) DUMPFILE=asksource.save0
            ;;
         1) DUMPFILE=asksource.save1
            ;;
         2) DUMPFILE=asksource.save2
            ;;
         3) DUMPFILE=asksource.save3
            ;;
         Q) SAVEANS=XXX
            ;;
         q) SAVEANS=XXX
            ;;
         *) echo "Invalid option.  Please choose one of '${VALID}'"
            LOOPIE=1
            ;;
      esac
   done
   if [ "${SAVEANS}" != "XXX" -a "${VALID}" != "XXX" ]
   then
      MARKER=$(cat asksource.save${SAVEANS}.mark)
      echo "You want to delete slot ${SAVEANS} [${MARKER:-GENERIC}]"
      LOOPIE=1
      while [ ${LOOPIE} -eq 1 ]
      do
         echo "Is this correct? (Y/N) : "|tr -d '\012'
         read ANS2
         if [ "${ANS2}" = "y" -o "${ANS2}" = "Y" ]
         then
            rm -f ${DUMPFILE}
            rm -f ${DUMPFILE}.mark
            LOOPIE=0
            echo "Deleted."
         elif [ "${ANS2}" = "n" -o "${ANS2}" = "N" ]
         then
            LOOPIE=0
            echo "Deletion aborted."
         else
            echo "Please enter Y or N."
         fi 
      done
   fi
}

###################################################################
# BROWSEOPTS - Browse previous config options
###################################################################
browseopts() {
   for i in 0 1 2 3
   do
      if [ -f asksource.save${i}.mark ]
      then
         MARKER=$(cat asksource.save${i}.mark)
      else
         MARKER=""
      fi
      if [ -s asksource.save${i} ]
      then
         echo "${i}. Slot #${i} In-Use [${MARKER:-GENERIC}]"
         VALID="${VALID} ${i}"
      fi
   done
   VALID=$(echo ${VALID})
   if [ -z "${VALID}" ]
   then
      echo "You have no options."
   fi
}

###################################################################
# MARKOPTS - Load previous config options
###################################################################
markopts() {
   LOOPIE=1
   VALID=""
   echo "Label which slot:"
   while [ ${LOOPIE} -eq 1 ]
   do
      for i in 0 1 2 3
      do
         if [ -f asksource.save${i}.mark ]
         then
            MARKER=$(cat asksource.save${i}.mark)
         else
            MARKER=""
         fi
         if [ -s asksource.save${i} ]
         then
            echo "${i}. Slot #${i} In-Use [${MARKER:-GENERIC}]"
            VALID="${VALID} ${i}"
         fi
      done
      VALID=$(echo ${VALID})
      if [ -z "${VALID}" ]
      then
         echo "You have no options to mark."
         LOOPIE=0
         break
      fi
      VALID="${VALID} Q q"
      echo "Q. Abort screen."
      echo ""
      echo "Please enter selection: "|tr -d '\012'
      read SAVEANS
      CHK=$(echo ${VALID}|grep -c "${SAVEANS}")
      if [ ${CHK} -eq 0 ]
      then
         SAVEANS=XXX
      fi
      LOOPIE=0
      case ${SAVEANS} in
         0) DUMPFILE=asksource.save0.mark
            ;;
         1) DUMPFILE=asksource.save1.mark
            ;;
         2) DUMPFILE=asksource.save2.mark
            ;;
         3) DUMPFILE=asksource.save3.mark
            ;;
         q) SAVEANS=XXX
            ;;
         Q) SAVEANS=XXX
            ;;
         *) echo "Invalid option.  Please choose one of '${VALID}'"
            LOOPIE=1
            ;;
      esac
   done
   if [ "${SAVEANS}" = "XXX" ]
   then
      LOOPIE=0
   else
      LOOPIE=1
   fi
   while [ ${LOOPIE} -eq 1 ]
   do
      echo "Enter label now: "|tr -d '\012'
      read ANS
      echo ""
      echo "You want to label slot ${SAVEANS} with ${ANS}"
      echo "Is this correct? (Y/N) : "|tr -d '\012'
      read ANS2
      if [ "${ANS2}" = "y" -o "${ANS2}" = "Y" ]
      then
         LOOPIE=0
         echo "${ANS}" > ${DUMPFILE}
         echo "Label marked."
      fi
   done
}

###################################################################
# XTRAOPTS - Load extra/default config options
###################################################################
xtraopts() {
   LOOPIE=1
   VALID=""
   echo "Load which default configuration:"
   while [ ${LOOPIE} -eq 1 ]
   do
      CNTR=0
      for i in mux penn tm3 default
      do
         CNTR=$(expr $CNTR + 1)
         if [ -f asksource.${i}.mark ]
         then
            MARKER=$(cat asksource.${i}.mark)
         else
            MARKER=""
         fi
         if [ -s asksource.${i} ]
         then
            echo "${CNTR}. Slot #${CNTR} [${MARKER:-GENERIC}]"
            VALID="${VALID} ${CNTR}"
         fi
      done
      VALID=$(echo ${VALID})
      if [ -z "${VALID}" ]
      then
         LOOPIE=0
         break
      fi
      VALID="${VALID} Q q"
      echo "Q. Abort screen."
      echo ""
      echo "Please enter selection: "|tr -d '\012'
      read SAVEANS
      CHK=$(echo ${VALID}|grep -c "${SAVEANS}")
      if [ ${CHK} -eq 0 ]
      then
         SAVEANS=XXX
      fi
      LOOPIE=0
      case ${SAVEANS} in
         1) DUMPFILE=asksource.mux
            ;;
         2) DUMPFILE=asksource.penn
            ;;
         3) DUMPFILE=asksource.tm3
            ;;
         4) DUMPFILE=asksource.default
            ;;
         Q) DUMPFILE="/"
            ;;
         q) DUMPFILE="/"
            ;;
         *) echo "Invalid option.  Please choose one of '${VALID}'"
            LOOPIE=1
            ;;
      esac
   done
   if [ -f "${DUMPFILE}" ]
   then
      if [ -f "${DUMPFILE}.mark" ]
      then
         MARKER=$(cat ${DUMPFILE}.mark)
      else
         MARKER=""
      fi
      . $(pwd)/${DUMPFILE} 2>/dev/null
      echo "Loading default config options from slot ${SAVEANS} [${MARKER:-GENERIC}]"
   elif [ "${DUMPFILE}" = "/" ]
   then
      echo ""
   else
      echo "ERROR: No previous config options found to load."
   fi
   DUMPFILE=""
}

###################################################################
# LOADOPTS - Load previous config options
###################################################################
loadopts() {
   LOOPIE=1
   VALID=""
   echo "Load from which slot:"
   while [ ${LOOPIE} -eq 1 ]
   do
      for i in 0 1 2 3
      do
         if [ -f asksource.save${i}.mark ]
         then
            MARKER=$(cat asksource.save${i}.mark)
         else
            MARKER=""
         fi
         if [ -s asksource.save${i} ]
         then
            echo "${i}. Slot #${i} In-Use [${MARKER:-GENERIC}]"
            VALID="${VALID} ${i}"
         fi
      done
      VALID=$(echo ${VALID})
      if [ -z "${VALID}" ]
      then
         LOOPIE=0
         break
      fi
      VALID="${VALID} Q q"
      echo "Q. Abort screen."
      echo ""
      echo "Please enter selection: "|tr -d '\012'
      read SAVEANS
      CHK=$(echo ${VALID}|grep -c "${SAVEANS}")
      if [ ${CHK} -eq 0 ]
      then
         SAVEANS=XXX
      fi
      LOOPIE=0
      case ${SAVEANS} in
         0) DUMPFILE=asksource.save0
            ;;
         1) DUMPFILE=asksource.save1
            ;;
         2) DUMPFILE=asksource.save2
            ;;
         3) DUMPFILE=asksource.save3
            ;;
         Q) DUMPFILE="/"
            ;;
         q) DUMPFILE="/"
            ;;
         *) echo "Invalid option.  Please choose one of '${VALID}'"
            LOOPIE=1
            ;;
      esac
   done
   if [ -f "${DUMPFILE}" ]
   then
      if [ -f "${DUMPFILE}.mark" ]
      then
         MARKER=$(cat ${DUMPFILE}.mark)
      else
         MARKER=""
      fi
      . $(pwd)/${DUMPFILE} 2>/dev/null
      echo "Loading previous config options from slot ${SAVEANS} [${MARKER:-GENERIC}]"
   elif [ "${DUMPFILE}" = "/" ]
   then
      echo ""
   else
      echo "ERROR: No previous config options found to load."
   fi
   DUMPFILE=""
   if [ "${MS[1]}" = "X" ]
   then
      echo "1" > ../src/usesql.toggle
   fi
}

###################################################################
# SAVEOPTS - Save the options to the DEFS to a specified slot
###################################################################
saveopts() {
   LOOPIE=1
   echo "Save in which slot:"
   while [ ${LOOPIE} -eq 1 ]
   do
      for i in 0 1 2 3
      do
         if [ -f asksource.save${i}.mark ]
         then
            MARKER=$(cat asksource.save${i}.mark)
         else
            MARKER=""
         fi
         if [ -s asksource.save${i} ]
         then
            echo "${i}. Slot #${i} In-Use [${MARKER:-GENERIC}]"
         else
            echo "${i}. Slot #${i} - EMPTY"
         fi
      done
      echo "Q. Abort screen."
      echo ""
      echo "Please enter selection: "|tr -d '\012'
      read SAVEANS
      LOOPIE=0
      case ${SAVEANS} in
         0) DUMPFILE=asksource.save0
            ;;
         1) DUMPFILE=asksource.save1
            ;;
         2) DUMPFILE=asksource.save2
            ;;
         3) DUMPFILE=asksource.save3
            ;;
         Q) DUMPFILE="/"
            ;;
         q) DUMPFILE="/"
            ;;
         *) echo "Invalid option.  Please choose one of '0 1 2 3'"
            LOOPIE=1
            ;;
      esac
   done
   if [ "${DUMPFILE}" != "/" ]
   then
      cat /dev/null > ${DUMPFILE}
      for i in ${OPTIONS}
      do
         echo "X[$i]=\"${X[$i]}\"" >> ${DUMPFILE}
      done
      for i in ${BOPTIONS}
      do
         echo "XB[$i]=\"${XB[$i]}\"" >> ${DUMPFILE}
      done
      for i in ${DOPTIONS}
      do
         echo "XD[$i]=\"${XD[$i]}\"" >> ${DUMPFILE}
      done
      for i in ${LOPTIONS}
      do
         echo "XL[$i]=\"${XL[$i]}\"" >> ${DUMPFILE}
      done
      for i in ${AOPTIONS}
      do
         echo "XA[$i]=\"${XA[$i]}\"" >> ${DUMPFILE}
      done
      for i in ${DBOPTIONS}
      do
         echo "MS[$i]=\"${MS[$i]}\"" >> ${DUMPFILE}
      done
      if [ -f "${DUMPFILE}.mark" ]
      then
         MARKER=$(cat ${DUMPFILE}.mark)
      else
         MARKER=""
      fi
      echo "Options saved to slot ${SAVEANS} [${MARKER:-GENERIC}]"
   fi
}

###################################################################
# LOADTEMPLATE - Load the template for default compiles
###################################################################
loadtemplate() {
   if [ -f asksource.save_template ]
   then
      . $(pwd)/asksource.save_template 2>/dev/null
   fi
}
###################################################################

###################################################################
# LOADLASTSTATE - Load the last state
###################################################################
loadlaststate() {
   DUMPFILE="asksource.save_default"
   if [ ! -f ${DUMPFILE} ]
   then
      return
   fi
   . $(pwd)/${DUMPFILE} 2>/dev/null
}

###################################################################
# SAVELASTSTATE - save the last state upon leaving and load it
# in if you reload it
###################################################################
savelaststate() {
   DUMPFILE=asksource.save_default
   cat /dev/null > ${DUMPFILE}
   for i in ${OPTIONS}
   do
      echo "X[$i]=\"${X[$i]}\"" >> ${DUMPFILE}
   done
   for i in ${BOPTIONS}
   do
      echo "XB[$i]=\"${XB[$i]}\"" >> ${DUMPFILE}
   done
   for i in ${DOPTIONS}
   do
      echo "XD[$i]=\"${XD[$i]}\"" >> ${DUMPFILE}
   done
   for i in ${LOPTIONS}
   do
      echo "XL[$i]=\"${XL[$i]}\"" >> ${DUMPFILE}
   done
   for i in ${AOPTIONS}
   do
      echo "XA[$i]=\"${XA[$i]}\"" >> ${DUMPFILE}
   done
   for i in ${DBOPTIONS}
   do
      echo "MS[$i]=\"${MS[$i]}\"" >> ${DUMPFILE}
   done
   if [ -f "${DUMPFILE}.mark" ]
   then
      MARKER=$(cat ${DUMPFILE}.mark)
   else
      MARKER=""
   fi
   echo "Options saved to slot ${SAVEANS} [${MARKER:-GENERIC}]"
}

###################################################################
# SETOPTS - Set options for compiletime run for makefile mod
###################################################################
setopts() {
   for i in ${OPTIONS}
   do
      if [ "${X[$i]}" = "X" ]
      then
         DEFS="${DEF[$i]} ${DEFS}"
      fi
   done
   for i in ${BOPTIONS}
   do
      if [ "${XB[$i]}" = "X" ]
      then
         DEFS="${DEFB[$i]} ${DEFS}"
      fi
   done
   for i in ${DOPTIONS}
   do
      if [ "${XD[$i]}" = "X" ]
      then
         DEFS="${DEFD[$i]} ${DEFS}"
      fi
   done
   for i in ${LOPTIONS}
   do
      if [ "${XL[$i]}" = "X" ]
      then
         DEFS="${DEFL[$i]} ${DEFS}"
      fi
   done
   for i in ${AOPTIONS}
   do
      if [ "${XA[$i]}" = "X" ]
      then
         DEFS="${DEFA[$i]} ${DEFS}"
      fi
   done
   for i in ${DBOPTIONS}
   do
      if [ "${MS[$i]}" = "X" ]
      then
         DEFS="${DEFDB[$i]} ${DEFS}"
      fi
   done

   # Handle special CONFIG def handler here
   for i in ${DCONFIGS}
   do
      if [ -n "${DEFS[$i]}" ]
      then
         echo "Applying option $i config change"
         DEFS="${DEFS[$i]} ${DEFS}"
      fi
   done
}

###################################################################
# SETDEFAULTS - Assign default DEFS required for platform
###################################################################
setdefaults() {
  # check for utmpx.h else fall back to utmp.h
  lc_utmp=$(echo '#include <utmpx.h>' | cpp -H -o /dev/null 2>&1 | head -n1|grep -c "/utmpx.h")
  if [ ${lc_utmp} -eq 0 ]
  then
     lc_utmp=$(echo '#include <utmp.h>' | cpp -H -o /dev/null 2>&1 | head -n1|grep -c "/utmp.h")
     if [ ${lc_utmp} -eq 0 ]
     then
        DEFS="-DUTMP_MISSING ${DEFS}"
     else
        DEFS="-DUTMP_ONLY ${DEFS}"
     fi
  fi

  # compile timeout.c incase we need it
  ${MYGCC} ../src/timeout.c -o ../bin/timeout > /dev/null 2>&1
  echo "Configuring default definitions..."
  ${MYGCC} ../src/intsize.c -o ../src/intsize >/dev/null 2>&1
  if [ $? -eq 0 ]
  then
     if [ "$(../src/intsize)" -gt 4 ]
     then
        echo "Patching for 64bit platform..."
        DEFS="-DBIT64 ${DEFS}"
     fi
  fi
  echo "Testing MMU support on server..."
  ${MYGCC} ../src/sockopts_test.c -o ../src/sockopts_test > /dev/null 2>&1
  if [ $? -ne 0 ]
  then
     echo "Your server doesn't handle advanced socket protocols.  Disabling..."
     DEFS="-DBROKEN_PROXY ${DEFS}"
  fi
  ${MYGCC} ../src/scantst.c -o ../src/scantst >/dev/null 2>&1
  if [ $? -ne 0 ]
  then
     DEFS="-DNEED_SCANDIR ${DEFS}"
  fi
  if [ "$(uname -a|grep -ci aix)" -ne 0 ]
  then
     echo "Patching for AIX..."
     DEFS="-DFIX_AIX ${DEFS}"
  fi

  gl_chkerrno=0
  if [ -f /usr/include/sys/errno.h ]
  then
     echo  "Patching for errno.h <sys>..."
     DEFS="-DBROKEN_ERRNO_SYS ${DEFS}"
     if [ -f "/usr/include/errno.h" ]
     then
       echo "   # It may not work.  If it doesn't, first remove -DBROKEN_ERRNO_SYS from the DEFS line."
       echo "   # If that doesn't work, change to -DBROKEN_ERRNO"
     fi
  elif [ -f /usr/include/asm/errno.h ]
  then
     echo  "Patching for errno.h <asm>..."
     if [ -f /usr/include/errno.h ]
     then
       echo "   # It may not work.  If it doesn't, Remove -DBROKEN_ERRNO from the DEFS line."
     fi
     DEFS="-DBROKEN_ERRNO ${DEFS}"
  elif [ -f /usr/include/errno.h ]
  then
     DEFS="-DHAVE_ERRNO_H ${DEFS}"
     gl_chkerrno=1
  fi
  if [ "$(uname -s)" == "Darwin" ]
  then
     ${MYGCC} ../src/gettime_test.c -o ../src/gettime_test > /dev/null 2>&1
     if [ $? -eq 0 ]
     then
        DEFS="-DMACH_TIMER ${DEFS}"
     fi
     if [ ${gl_chkerrno} -eq 0 ]
     then
        echo "Patching errno.h for MAC compatibility..."
        DEFS="-DHAVE_ERRNO_H ${DEFS}"
     fi
     echo "Tossing in Malloc for stdlib definition for MAC..."
     DEFS="-DMALLOC_IN_STDLIB_H ${DEFS}"
  fi
  DEFS="-DBROKEN_NDBM ${DEFS}"
  if [ ! -f /usr/include/wait.h ]
  then
     echo "Patching for wait.h <sys>..."
     DEFS="-DSYSWAIT ${DEFS}"
  fi
  if [ -f /usr/include/sys/malloc.h ]
  then
     echo "Patching for weird malloc.h..."
     DEFS="-DSYS_MALLOC ${DEFS}"
  fi
  if [ -f /usr/include/bsd/bsd.h -o ! -f /usr/include/values.h ]
  then
     lc_tst="$(uname -a |grep -ic linux)"
     if [ ${lc_tst} -eq 0 ]
     then
         echo "BSD identified.  Configuring..."
         DEFS="-DBSD_LIKE ${DEFS}"
     fi
  fi
  if [ "${XB[4]}" = "X" ]
  then
     Z1=0
     Z2=0
     Z3=0
     if [ -n "${LDCONFIG}" ]
     then
        Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libsqlite3.so$")
     else
        Z4=0
     fi
     if [ "$Z4" -eq 0 ]
     then
        if [ "$(uname -s)" != "Darwin" ]
        then
           Z4=$(ld -lsqlite3 2>&1|grep -c "ld: warning: can")
        fi
     fi
     if [ "$Z4" -eq 0 ]
     then
        Z1=$(ls /lib/libsqlite3.* 2>/dev/null|wc -l)
        Z2=$(ls /usr/lib/libsqlite3.* 2>/dev/null|wc -l)
        Z3=$(ls /usr/local/lib/libsqlite3.* 2>/dev/null|wc -l)
     fi
     if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
     then
        echo "Patching -lsqlite3..."
        MORELIBS="-lsqlite3 ${MORELIBS}"
     fi
  fi
  if [ "${MS[1]}" = "X" ]
  then
     if [ "${MYSQL_VER}" = "0" -a "${MS[3]}" != "X" ]
     then
        MS[1]=" "
        echo "MySQL was not found.  Stripping it..."
     else
        if [ "${MS[3]}" == "X" ]
        then
           FORCE_MYSQL=1
           echo "Force compiling in MYSQL libs.  You can fail compiling with this."
        fi
        MORELIBS="\$(MYSQL_LIB) ${MORELIBS}"
     fi
  fi
  BOB1=$(uname -r|cut -f1 -d".")
  BOB2=$(uname -s)
  if [ -d /usr/ucbinclude -a "${BOB2}" = "SunOS" ]
  then
     if [ "${BOB1}" -lt 5 ]
     then
        echo "SunOS 4.x compatable.  Configuring..."
        DEFS="${DEFS} -I\/usr\/ucbinclude -DSOLARIS"
     else
        echo "SunOS 5.x compatable.  Configuring..."
        DEFS="${DEFS} -DSOLARIS"
     fi
  fi
  if [ -n "${LDCONFIG}" ]
  then
     Z1=$(${LDCONFIG} ${LDOPT}|grep -c "openssl.so$")
     if [ ${Z1} -eq 0 ]
     then
        Z1=$(${LDCONFIG} ${LDOPT}|grep -c "libssl.so$")
     fi
  else
     Z1=0
  fi
  if [ "$Z1" -eq 0 ]
  then
     if [ "$(uname -s)" != "Darwin" ]
     then
        Z1=$(ld -lssl 2>&1|grep -c "ld: warning: can")
     fi
  fi
  if [ "${X[24]}" != "X" ]
  then
     if [ -f /usr/include/openssl/sha.h -a -f /usr/include/openssl/evp.h -a -f /usr/include/openssl/bio.h ]
     then
        if [ $(uname -a|grep -ic ubuntu) -eq 0 ]
        then
           echo "OpenSSL identified.  Configuring..."
           DEFS="${DEFS} -DHAS_OPENSSL"
        else
           if [ $Z1 -gt 0 ]
           then
              echo "OpenSSL identified.  Configuring..."
              DEFS="${DEFS} -DHAS_OPENSSL"
           fi
        fi
     elif [ $Z1 -gt 0 ]
     then
        echo "OpenSSL identified.  Configuring..."
        DEFS="${DEFS} -DHAS_OPENSSL"
     fi
  fi
  if [ "${MS[1]}" = "X" ]
  then
     if [ $FORCE_MYSQL -eq 1 ]
     then
        echo "MySQL has been forced on.  This may fail compiling."
        DEFS="${DEFS} -DFORCE_MYSQL"
     fi
     echo "MySQL identified.  Configuring..."
     DEFS="${DEFS} \$(MYSQL_INCLUDE)"
  fi
  if [ "$(uname -s)" = "Darwin" ]
  then
     echo "Removing inline ctype as Mac borks it..."
     DEFS="${DEFS} -D_DONT_USE_CTYPE_INLINE_"
  fi
  DEFS="CUSTDEFS = ${DEFS}"
}

###################################################################
# SETLIBS - Set the libraries required for platform
###################################################################
setlibs() {
   echo "Configuring default libraries..."
   Z1=0
   Z2=0
   Z3=0
   if [ -n "${LDCONFIG}" ]
   then
      Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libcrypt.so$")
   else
      Z4=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lcrypt 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "$Z4" -eq 0 ]
   then
      Z1=$(ls /lib/libcrypt.* 2>/dev/null|wc -l)
      Z2=$(ls /usr/lib/libcrypt.* 2>/dev/null|wc -l)
      Z3=$(ls /usr/local/lib/libcrypt.* 2>/dev/null|wc -l)
   fi
   if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
   then
      echo "Patching -lcrypt..."
      MORELIBS="-lcrypt ${MORELIBS}"
   fi
   Z1=0
   Z2=0
   Z3=0
   if [ -n "${LDCONFIG}" ]
   then
      Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libsocket.so$")
   else
      Z4=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lsocket 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "$Z4" -eq 0 ]
   then
      Z1=$(ls /lib/libsocket.* 2>/dev/null|wc -l)
      Z2=$(ls /usr/lib/libsocket.* 2>/dev/null|wc -l)
      Z3=$(ls /usr/local/lib/libsocket.* 2>/dev/null|wc -l)
   fi
   if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
   then
      echo "Patching -lsocket..."
      MORELIBS="-lsocket ${MORELIBS}"
   fi
   Z1=0
   Z2=0
   Z3=0
   if [ -n "${LDCONFIG}" ]
   then
      Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libresolv.so$")
   else
      Z4=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lresolv 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "$Z4" -eq 0 ]
   then
      Z1=$(ls /lib/libresolv.* 2>/dev/null|wc -l)
      Z2=$(ls /usr/lib/libresolv.* 2>/dev/null|wc -l)
      Z3=$(ls /usr/local/lib/libresolv.* 2>/dev/null|wc -l)
   fi
   if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
   then
      echo "Patching -lresolv..."
      MORELIBS="-lresolv ${MORELIBS}"
   fi
   Z1=0
   Z2=0
   Z3=0
   if [ -n "${LDCONFIG}" ]
   then
      Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libnsl.so$")
   else
      Z4=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lnsl 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "$Z4" -eq 0 ]
   then
      Z1=$(ls /lib/libnsl.* 2>/dev/null|wc -l)
      Z2=$(ls /usr/lib/libnsl.* 2>/dev/null|wc -l)
      Z3=$(ls /usr/local/lib/libnsl.* 2>/dev/null|wc -l)
   fi
   if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
   then
      echo "Patching -lnsl..."
      MORELIBS="-lnsl ${MORELIBS}"
   fi
   Z1=0
   Z2=0
   Z3=0
   if [ -n "${LDCONFIG}" ]
   then
      Z4=$(${LDCONFIG} ${LDOPT}|grep -c "libm.so$")
   else
      Z4=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lm 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "$Z4" -eq 0 ]
   then
      Z1=$(ls /lib/libm.* 2>/dev/null|wc -l)
      Z2=$(ls /usr/lib/libm* 2>/dev/null|wc -l)
      Z3=$(ls /usr/local/lib/libm* 2>/dev/null|wc -l)
   fi
   if [ "$Z1" -gt 0 -o "$Z2" -gt 0 -o "$Z3" -gt 0 -o "$Z4" -gt 0 ]
   then
      echo "Patching -lm..."
      MORELIBS="-lm ${MORELIBS}"
   fi
   BOB1=$(uname -r|cut -f1 -d".")
   BOB2=$(uname -s)
   if [ -d /usr/ucblib -a "${BOB2}" = "SunOS" -a "${BOB1}" -lt 5 ]
   then
      echo "MIPS SunOS 4.x identified.  Configuring..."
      MORELIBS="${MORELIBS} -L\/usr\/ucblib -l ucb"
   fi
   if [ -n "${LDCONFIG}" ]
   then
      Z1=$(${LDCONFIG} ${LDOPT}|grep -c "openssl.so$")
   else
      Z1=0
   fi
   if [ "$Z4" -eq 0 ]
   then
      if [ "$(uname -s)" != "Darwin" ]
      then
         Z4=$(ld -lssl 2>&1|grep -c "ld: warning: can")
      fi
   fi
   if [ "${X[24]}" != "X" ]
   then
      if [ -f /usr/include/openssl/sha.h -a -f /usr/include/openssl/evp.h ]
      then
         Z1=0
         if [ -n "${LDCONFIG}" ]
         then
            Z2=$(${LDCONFIG} ${LDOPT}|grep -c "libcrypto.so$")
         else
            Z2=0
         fi
         if [ "$Z2" -eq 0 ]
         then
            if [ "$(uname -s)" != "Darwin" ]
            then
               Z2=$(ld -lcrypto 2>&1|grep -c "ld: warning: can")
            fi
         fi
         if [ "$Z2" -eq 0 ]
         then
            Z1=$(ls /usr/lib/libcrypto* 2>/dev/null|wc -l)
         fi
         if [ "$Z1" -gt 0 -o "$Z2" -gt 0 ]
         then
            echo "Configuring libcrypto..."
            MORELIBS="${MORELIBS} -lcrypto"
         fi
      echo "Configuring libssl..."
      MORELIBS="${MORELIBS} -lssl"
      elif [ "$Z1" -gt 0 ]
      then
         Z1=0
         if [ -n "${LDCONFIG}" ]
         then
            Z2=$(${LDCONFIG} ${LDOPT}|grep -c "libcrypto.so$")
         else
            Z2=0
         fi
         if [ "$Z2" -eq 0 ]
         then
            if [ "$(uname -s)" != "Darwin" ]
            then
               Z2=$(ld -lcrypto 2>&1|grep -c "ld: warning: can")
            fi
         fi
         if [ "$Z2" -eq 0 ]
         then
            Z1=$(ls /usr/lib/libcrypto* 2>/dev/null|wc -l)
         fi
         if [ "$Z1" -gt 0 -o "$Z2" -gt 0 ]
         then
            echo "Configuring libcrypto..."
            MORELIBS="${MORELIBS} -lcrypto"
         fi
         echo "Configuring libssl..."
         MORELIBS="${MORELIBS} -lssl"
      fi
   fi
   if [ "${X[25]}" = "X" ]
   then
      echo "Compiling with system pcre library..."
      MORELIBS="${MORELIBS} -lpcre"
   fi
   MORELIBS="CUSTMORELIBS = ${MORELIBS}"
}

###################################################################
# UPDATEMAKEFILE - Update the makefile with the changes
###################################################################
updatemakefile() {
   echo "Generating custom DEFS section for the Makefile now.  Please wait..."
   echo "# Custom Definitions" > ../src/custom.defs
   echo "${DEFS}" >> ../src/custom.defs
   echo "Generating the custom LIBS section of the Makefile now.  Please wait..."
   echo "# Custom Libraries" >> ../src/custom.defs
   echo "${MORELIBS}" >> ../src/custom.defs
   if [ "${XB[2]}" = "X" ]
   then
      echo "Generating @door section for the Makefile now.  Please wait..."
      echo "# Main @door engine" >> ../src/custom.defs
      echo "DR_DEF = -DENABLE_DOORS -DEXAMPLE_DOOR_CODE" >> ../src/custom.defs
      if [ "${XD[1]}" = "X" ]
      then
         echo "# Mush @door" >> ../src/custom.defs
         echo "DRMUSHSRC = door_mush.c" >> ../src/custom.defs
         echo "DRMUSHOBJ = door_mush.o" >> ../src/custom.defs
      fi
      if [ "${XD[2]}" = "X" ]
      then
         echo "# Empire @door" >> ../src/custom.defs
         echo "DREMPIRESRC = empire.c" >> ../src/custom.defs
         echo "DREMPIREOBJ = empire.o" >> ../src/custom.defs
         echo "DREMPIREHDR = empire.h" >> ../src/custom.defs
         echo "DR_HDR = \$(DREMPIREHDR)" >> ../src/custom.defs
      fi
      if [ "${XD[3]}" = "X" ]
      then
         echo "# POP mail @door" >> ../src/custom.defs
         echo "DRMAILSRC = door_mail.c" >> ../src/custom.defs
         echo "DRMAILOBJ = door_mail.o" >> ../src/custom.defs
      fi
   fi
   echo "Generating DB link library for the Makefile now.  Please wait..."
   echo "# DB used for Mush Engine" >> ../src/custom.defs
   if [ "${XB[5]}" = "X" ]
   then
      echo "CUSTLIBS = -L../src/qdbm/ -lqdbm" >> ../src/custom.defs
      echo "COMP=qdbm" > ../src/do_compile.var
   else
      echo "CUSTLIBS = -L../src/gdbm/.libs/ -lgdbm_compat -L../src/gdbm/ -lgdbm" >> ../src/custom.defs
      echo "COMP=gdbm" > ../src/do_compile.var
   fi
   chmod 755 ../src/do_compile.var
   if [ "${X[23]}" = "X" ]
   then
      echo "Generating Makefile for low-memory compiling.  Please wait..."
      echo "# This is needed if server hosting us has extreme low memory and no swap" >> ../src/custom.defs
      echo "CFLAGS = --param ggc-min-expand=0 --param ggc-min-heapsize=8192" >> ../src/custom.defs
   fi
   if [ "${X[28]}" = "X" ]
   then
      echo "# Enable LUA integration for WebAPI" >> ../src/custom.defs
      echo "USELUA = 1" >> ../src/custom.defs
   fi
   if [ "${MS[1]}" = "X" ]
   then
      echo "# MySQL compatibility engine" >> ../src/custom.defs
      echo "Generating Makefile for MySQL generation.  Please wait..."
      echo "USEMYSQL = 1" >> ../src/custom.defs
   fi
}

###################################################################
# MAIN - Main system call and loop
###################################################################
main() {
   if [ "$1" = "default" ]
   then
      loadtemplate
      REPEAT=0
   fi
   loadlaststate
   while [ ${REPEAT} -eq 1 ]
   do
      menu
      read ANS
      parse $ANS
      if [ ${BETAOPT} -eq 1 ]
      then
          BETACONTINUE=1
          while [ $BETACONTINUE -eq 1 ]
          do
             doormenu
             read ANS
             parse $ANS
          done
          BETAOPT=0
          if [ "${XD[1]}" != "X" -a "${XD[2]}" != "X" -a "${XD[3]}" != "X" ]
          then
             XB[2]=" "
          else
             XB[2]="X"
          fi
      fi
      if [ ${BETAOPT} -eq 2 ]
      then
          BETACONTINUE=2
          lc_stack=$(ulimit -s)
          if [ "${lc_stack}" = "unlimited" ]
          then
             lc_stack="2000000000"
          fi
          if [ -z "${lc_stack}" ]
          then
             lc_stack=1024
          fi
          while [ $BETACONTINUE -eq 2 ]
          do
             lbufmenu
             read ANS
             parse $ANS
             if [ -z "${lc_stack}" ]
             then
                lc_stack=1024
             fi
             if [ "${XL[5]}" = "X" ]
             then
                if [ ${lc_stack} -lt 16384 ]
                then
                   echo "You must have stack size set to 16384 or higher to use 64K lbufs."
                   echo "Yours is currently set to ${lc_stack}.  ulimit -s 16500 to set."
                   echo "< HIT RETURN KEY TO CONTINUE >"
                   read ANS
                   XL[5]=" "
                   if [ ${lc_stack} -ge 8192 ]
                   then
                      XL[4]="X"
                   elif [ ${lc_stack} -ge 4096 ]
                   then
                      XL[3]="X"
                   elif [ ${lc_stack} -ge 2048 ]
                   then
                      XL[2]="X"
                   else
                      XL[1]="X"
                   fi
                fi
             elif [ "${XL[4]}" = "X" ]
             then
                if [ ${lc_stack} -lt 8192 ]
                then
                   echo "You must have stack size set to 8192 or higher to use 32K lbufs."
                   echo "Yours is currently set to ${lc_stack}.  ulimit -s 8400 to set."
                   echo "< HIT RETURN KEY TO CONTINUE >"
                   read ANS
                   XL[4]=" "
                   if [ ${lc_stack} -ge 4096 ]
                   then
                      XL[3]="X"
                   elif [ ${lc_stack} -ge 2048 ]
                   then
                      XL[2]="X"
                   else
                      XL[1]="X"
                   fi
                fi
             elif [ "${XL[3]}" = "X" ]
             then
                if [ ${lc_stack} -lt 4096 ]
                then
                   echo "You must have stack size set to 4096 or higher to use 16K lbufs."
                   echo "Yours is currently set to ${lc_stack}.  ulimit -s 4200 to set."
                   echo "< HIT RETURN KEY TO CONTINUE >"
                   read ANS
                   XL[3]=" "
                   if [ ${lc_stack} -ge 2048 ]
                   then
                      XL[2]="X"
                   else
                      XL[1]="X"
                   fi
                fi
             elif [ "${XL[2]}" = "X" ]
             then
                if [ ${lc_stack} -lt 2048 ]
                then
                   echo "You must have stack size set to 2048 or higher to use 8K lbufs."
                   echo "Yours is currently set to ${lc_stack}.  ulimit -s 2100 to set."
                   echo "< HIT RETURN KEY TO CONTINUE >"
                   read ANS
                   XL[2]=" "
                   XL[1]="X"
                fi
             fi
          done
          BETAOPT=0
      fi
      if [ ${BETAOPT} -eq 3 ]
      then
          BETACONTINUE=3
          while [ $BETACONTINUE -eq 3 ]
          do
             ansimenu
             read ANS
             parse $ANS
          done
          BETAOPT=0
      fi
      if [ ${BETAOPT} -eq 4 ]
      then
          BETACONTINUE=4
          while [ $BETACONTINUE -eq 4 ]
          do
             mysqlmenu
             if [ $? -ne 200 ]
             then
                read ANS
                parse $ANS
             else
                BETACONTINUE=0
             fi
          done
          BETAOPT=0
      fi

   done
   setopts
   setdefaults
   setlibs
   updatemakefile
   savelaststate
   if [ "$1" != "default" ]
   then
      echo "< HIT RETURN KEY TO CONTINUE >"
      read ANS
   fi
}

###################################################################
# Ok, let's run it
###################################################################
main "$1"
