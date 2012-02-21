#!/bin/sh
# make 'mailprog' this script.
# make MYADDY the address you want it sent from
# DISABLE subjects from the rhost config 'mailsub'
#
# following settings need to be set within rhost
# mailprog ./mailhide.sh
# mailsub 0
#
MYADDY="someaddy@wherever.net"
SUBJ="RhostMUSH Autoregistration."
#
/usr/bin/nail -r "${MYADDY}" -s "${SUBJ}" $@
