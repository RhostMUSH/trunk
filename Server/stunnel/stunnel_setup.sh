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

##############################################################################
# validate the config of the game has the sconnect settings
##############################################################################
gl_chk_reip=`grep -c "^sconnect_reip" ${game}/netrhost.conf`
gl_chk_cmd=`grep -c "^sconnect_cmd" ${game}/netrhost.conf`

if [ ${gl_chk_reip} -eq 0 ]
then
   echo "You need to define sconnect_reip to '1' in your netrhost.conf file."
   echo "netrhost.conf syntax:  sconnect_reip 1"
   exit 1
fi
if [ ${gl_chk_cmd} -eq 0 ]
then
   echo "You need to define sconnect_cmd to a string for the stunnel command"
   echo "in your netrhost.conf file.  Any case-sensitive non-space string allowed"
   echo "netrhost.conf syntax example:  sconnect_cmd MyCommandName"
   exit 1
fi

##############################################################################
#
# Check the config of the game for some things.
#
##############################################################################
port=`awk '$1=="port" {print $2}' ${game}/netrhost.conf`
sslporttmp=`expr $port + 1`
name=`awk '$1=="mud_name"' ${game}/netrhost.conf | sed -e 's/mud_name //g'`
sconnect_command=`awk '$1=="sconnect_cmd" {print $2}' ${game}/netrhost.conf`


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

if [ -z $sslport ]; then
    sslport=$sslporttmp
fi

if [ -z $logfile ]; then
    logfile=$current
fi # if [-z logfile]; then

if [ -z $pidfile ]; then
    pidfile=$current
fi # if [-z pidfile]; then

if [ -z $where ]; then
    where=$current
fi # if [ -z where ]; then

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

openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 -nodes -keyout $key -out $cert -extensions san -subj '/CN=example.com' -config /tmp/mconf.$$

rm -f /tmp/mconf.$$

echo "
Certificate creation complete...

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
echo "
[${name}-SSL]
accept = $sslport
exec = $bubble
execargs = $bubble localhost:$port $sconnect_command
" >> stunnel.conf

echo "
stunnel configuration complete...Enjoy!
"

