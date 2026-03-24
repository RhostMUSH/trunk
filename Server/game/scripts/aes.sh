#!/bin/bash
if [ -f /bin/openssl ]
then
   gl_openssl="/bin/openssl"
elif [ -f /usr/bin/openssl ]
then
   gl_openssl="/usr/bin/openssl"
elif [ -f /usr/local/bin/openssl ]
then
   gl_openssl="/usr/local/bin/openssl"
else
   gl_openssl="openssl"
fi


if [ -z "$@" ]
then
echo "$@"
   echo -n "First value expected password, second enc or dec, third value expected string."
   exit 0
fi

lc_chk="$(echo "$@"|wc -w)"
if [ "${lc_chk}" -lt 3 ]
then
   echo -n "First value expected password, second enc or dec, third value expected string."
   exit 0
fi

gl_pass="$(echo "$@"|cut -f1 -d" ")"
gl_encr="$(echo "$@"|cut -f2 -d" ")"
gl_str="$(echo "$@"|cut -f3- -d" ")"

if [ "${gl_encr}" = "enc" ]
then
   lc_value="$(echo "${gl_str}" |  ${gl_openssl}  enc -A -a -aes-256-cbc -base64 -pass pass:${gl_pass})"
else
   lc_value="$(echo "${gl_str}" |  ${gl_openssl}  enc -A -a -aes-256-cbc -d -base64 -pass pass:${gl_pass})"
fi
echo -n "${lc_value}"
