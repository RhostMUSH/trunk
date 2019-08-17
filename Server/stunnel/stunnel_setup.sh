#!/bin/sh
##############################################################################
#
# Setup stunnel in a basic manner to allow for SSL/TLS security on games
#
# No arguments needed. Just run the script. It asks the one question it needs
# answered, and makes assumptions for the rest that should suit 99.999% of alll
# users.
#
##############################################################################

##############################################################################
#
# Figure some things out
#
##############################################################################
current=`pwd`
stunnel_template="stunnel.conf.example"
game="${current}/../game"
bubble="${current}/warpbubble.pl"
configfile="warpbubble.conf"
configtmp="$current/$configfile"
timeoutdef=12

##############################################################################
# validate we have stunnel in our path, if not build it
##############################################################################
stunnel -version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   ./stunnel -version > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "You do not have a OS built stunnel installed.  We'll install local."
      echo "Pulling down latest stunnel and compiling.  Please wait..."|tr -d '\012'
      cd ./stunnel_src
      ./compile.sh > /dev/null 2>&1
      echo "...completed"
      cd ..
      ./stunnel -version > /dev/null 2>&1
      if [ $? -ne 0 ]
      then
         echo "We were unable to compile stunnel locally."
         echo "please to into the 'stunnel_src' directory and type './compile.sh'"
         echo "Any errors if you can not work through, please report to the"
         echo "staff of RhostMUSH on their Dev site or on github."
         exit 1
      fi
   fi
   gl_stunnel_ver=`./stunnel -version 2>&1|head -1`
else
   gl_stunnel_ver=`stunnel -version 2>&1|head -1`
fi
##############################################################################
# validate the config of the game has the sconnect settings
##############################################################################
gl_chk_reip=`grep -c "^sconnect_reip" ${game}/netrhost.conf`
gl_arg_reip=`grep "^sconnect_reip" ${game}/netrhost.conf|awk '{print $2}'`
if [ ${gl_chk_reip} -eq 0 ]
then
   gl_chk_reip=`grep -c "^sconnect_reip" ${game}/rhost_ingame.conf`
   gl_arg_reip=`grep "^sconnect_reip" ${game}/rhost_ingame.conf|awk '{print $2}'`
fi
gl_chk_cmd=`grep -c "^sconnect_cmd" ${game}/netrhost.conf`
gl_arg_cmd=`grep "^sconnect_cmd" ${game}/netrhost.conf|awk '{print $2}'`
if [ ${gl_chk_cmd} -eq 0 ]
then
   gl_chk_cmd=`grep -c "^sconnect_cmd" ${game}/rhost_ingame.conf`
   gl_arg_cmd=`grep "^sconnect_cmd" ${game}/rhost_ingame.conf|awk '{print $2}'`
fi

if [ ${gl_chk_reip} -eq 0 ]
then
   echo "You need to define sconnect_reip to '1' in your netrhost.conf or rhost_ingame.conf file."
   echo "netrhost.conf syntax:  sconnect_reip 1"
   exit 1
fi
if [ ${gl_chk_cmd} -eq 0 ]
then
   echo "You need to define sconnect_cmd to a one word string for the stunnel command"
   echo "in your netrhost.conf or rhost_ingame.conf file.  Any case-sensitive non-space"
   echo "one word string is allowed. You will want this to be hard to guess."
   echo ""
   echo "syntax example:  sconnect_cmd MyCommandName"
   exit 1
fi
echo "Your STUNNEL version is: ${gl_stunnel_ver}"
if [ "${gl_arg_reip}" = "1" ]
then
   echo "Your sconnect_reip is: '${gl_arg_reip}'"
else
   echo "Your sconnect_reip is: '${gl_arg_reip}'  (Note: You need it set to '1' to enable SSL)"
fi
gl_cmdlen=`echo "${gl_arg_cmd}"|wc -c`
if [ "${gl_cmdlen}" -le 8 ]
then
   echo "Your sconnect_cmd is: '${gl_arg_cmd}' (Note: You likely want this  8 characters or longer)"
else
   echo "Your sconnect_cmd is: '${gl_arg_cmd}'"
fi

##############################################################################
#
# Check the config of the game for some things.
#
##############################################################################
port=`awk '$1=="port" {print $2}' ${game}/netrhost.conf`
apiport=`awk '$1=="api_port" {print $2}' ${game}/netrhost.conf`
sslporttmp=`expr $port + 1`
if [ "${sslporttmp}" = "${apiport}" ]
then
   sslporttmp=`expr $port + 2`
fi
name=`awk '$1=="mud_name"' ${game}/netrhost.conf | sed -e 's/mud_name //g'`
#sconnect_command=`awk '$1=="sconnect_cmd" {print $2}' ${game}/netrhost.conf`
sconnect_command="${gl_arg_cmd}"


##############################################################################
#
# Here we go
#
##############################################################################
echo "
This will generate a self-signed certificate and key that can be used
in your stunnel config to provide security for your game.

This will ask you where you want to store the keys (default is your current directory),
and will then modify your stunnel.conf with that location
"

echo -n "Where would you like the certificate files stored? [default: $current]: "
read where
echo -n "Where would you like the stunnel log file stored? [default: $current]: "
read logfile
echo -n "Where would you like the stunnel pid lock file stored? [default: $current]: "
read pidfile
echo -n "what do you wish to have as your sslport? [default: $sslporttmp]: "
read sslport
echo -n "What idle timeout in hours do you want for SSL sessions? [default: $timeoutdef hours]: "
read timeoutnew
echo -n "What is the name of your MUSH? [default: $name]: "
read newname

if [ -z "$newname" ]; then
   newname=$name
fi

if [ -z "$timeoutnew" ]; then
   timeoutnew=$timeoutdef
fi # timeoutnew

timeoutchk=`echo $timeoutnew|tr -cd '[0-9]'`
if [ "$timeoutchk" != "$timeoutnew" ]; then
   timeoutnew=$timeoutdef
fi

timeoutexp=`expr $timeoutnew \* 3600`

if [ -z "$sslport" ]; then
    sslport=$sslporttmp
fi # sslport

if [ -z "$logfile" ]; then
    logfile=$current
fi # logfile

if [ -z "$pidfile" ]; then
    pidfile=$current
fi # pidfile

if [ -z "$where" ]; then
    where=$current
fi # where

echo "Creating certificate files..."

##############################################################################
#
# Figure out where the files are going to be, then create some strings we can
# use with sed in a moment to do our substitituions for these filenames in the
# config file.
#
##############################################################################
cert="${where}/stunnel.cert"
key="${where}/stunnel.key"
ecert=`echo "$cert" | tr '/' '|'`
ekey=`echo "$key" | tr '/' '|'`

##############################################################################
#
# Create the key and cert. A fair bit of this data is bogus, but the MU*
# clients do not seem to give a damn about self-signed certs, just that there
# is a cert there.
#
# We are going to fast and easy, here. The fewer questions we have to ask, the
# better for everyone concerned. Power users who want better info than this
# will not be using this script, anyway.
#
##############################################################################
echo "
[req]
distinguished_name=req

[san]
subjectAltName=DNS:example.com,DNS:example.net,IP:10.0.0.1
" > /tmp/mconf.$$

if [ -f $key -o -f $cert ]
then
   echo "It appears you already have your key and cert files:"
   ls -l $key
   ls -l $cert
   echo -n "Do you really wish to overwrite these with new keys? [Default: no]: "
   read ans
   if [ -z "$ans" ]
   then
      ans="no"
   fi
   ans=`echo $ans|tr 'a-z' 'A-Z'`
   if [ "$ans" != "NO" ]
   then
      openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 -nodes -keyout $key -out $cert -extensions san -subj '/CN=example.com' -config /tmp/mconf.$$
      echo "Certificate creation complete..."
      rm -f /tmp/mconf.$$
   fi
else
  openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 -nodes -keyout $key -out $cert -extensions san -subj '/CN=example.com' -config /tmp/mconf.$$
  echo "Certificate creation complete..."
  rm -f /tmp/mconf.$$
fi


echo "

Updating stunnel configuration..."

##############################################################################
#
# Make a substitution to put the proper ket/cert paths in place
#
##############################################################################
sed -e "s~CERT_FILE~${ecert}~g" -e "s~KEY_FILE~${ekey}~g" -e "s~LOG_FILE~${logfile}~g" -e "s~PID_FILE~${pidfile}~g" ${stunnel_template} | tr '|' '/' > stunnel.conf

##############################################################################
#
# Write out a server block based on the information we gathered on port
# and name and such.
#
##############################################################################
echo -n "Enter full config file path or NONE to just pass as arguments [default: $configtmp]: "
read configpath
if [ -z $configpath ]; then
    configpath=$configtmp
fi

touch $configpath
if [ ! -f $configpath ]
then
   configpath="NONE"
fi

if [ "$configpath" = "NONE" ]
then
echo "
[${newname}-SSL]
accept = $sslport
exec = $bubble
execargs = $bubble $sconnect_command localhost $port
; the default value of this is 43200 (12 hours)
TIMEOUTidle=$timeoutexp
" >> stunnel.conf
else
echo "
[${newname}-SSL]
accept = $sslport
exec = $bubble
execargs = $bubble --conf=$configpath
; the default value of this is 43200 (12 hours)
TIMEOUTidle=$timeoutexp
" >> stunnel.conf
mv -f $configpath $configpath.prev 2>/dev/null
echo "host: localhost
port: $port
command: $sconnect_command" > $configpath
fi

echo "
stunnel configuration complete...Enjoy!
"
