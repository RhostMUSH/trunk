#!/bin/sh
#RhostMUSH installation script. Fetches you a fresh Rhost, compiles it
#and starts it up. And even comes with dialogs!
#
#Made by Odin, May 4 2007
#
#Standard defines

TEMP=/tmp/answer$$
RHOST_USER=member
RHOST_SITE=ftp.rhostmush.org
RHOSTDIST=rhostdistro4.tar.gz
if [ "$EDITOR" != "" ]; then
	EDITOR=/usr/bin/vi
fi
#Cleanup procedure
cleanup() {
	clear
	rm -f $TEMP
	rm -f $RHOSTDIST
	exit
}

	dialog --title "RhostMUSH installer" --msgbox "Welcome to the Rhostmush installation script.\\n\\n We will now connect to the RhostMUSH distribution server. To be able to do this, you must have a valid username and password for the distribution FTP site, rhostmush.org. You must also have wget installed.\\n\\nIf you do not wish to download and install RhostMUSH at this time, press esc now. Otherwise, press enter to continue." 30 80
	if [ "$?" != "0" ]
	then
		dialog --title "RhostMUSH installer" --msgbox "Installation was cancelled by user." 10 50
	else
		dialog --backtitle "Choose distribution" \
			--radiolist "Select distribution" 10 40 2 \
			1 "RhostMUSH 3.9 (Development)" 'on' \
			2 "RhostMUSH 3.4 (Stable)" 'off' 2>$TEMP
		if [ "$?" != "0" ] ; then return; fi
		choice=`cat $TEMP`
		case $choice in
			1) RHOSTDIST=rhostdistro9.tar.gz;;
			2) RHOSTDIST=rhostdistro4.tar.gz;;
		esac
	fi

	dialog --inputbox "Enter password for rhostmush.org:" 8 40 2>$TEMP
	dialog --infobox "Downloading $RHOSTDIST from $RHOST_SITE, please wait..." 10 30 ; sleep 4
	wget --http-user=member --http-passwd=`cat $TEMP` http://192.168.0.104/$RHOSTDIST 2>$TEMP

if [ "$?" != "0" ]
then
	dialog --title "RhostMUSH installer" --msgbox "Wget returned an error, download failed. Press enter to see the wget log. Afterwards, the installer will exit." 10 30
	dialog --title "Wget error log" --textbox $TEMP 13 65 2>/dev/null
	cleanup
	exit
else
	dialog --title "RhostMUSH installer" --msgbox "RhostMUSH will now be extracted." 10 30
	tar -xzf $RHOSTDIST 2>$TEMP
fi

if [ "$?" != "0" ]
then
	dialog --title "RhostMUSH installer" --msgbox "The RhostMUSH archive did not extract correctly. Press enter to see the error log." 10 30
	dialog --title "Untar error log" --textbox $TEMP 13 65 2>/dev/null
	cleanup
	exit
else
	dialog --title "RhostMUSH installer" --msgbox "The RhostMUSH installer will now call the configuration script..." 10 30
	cd Server
	make confsource
	dialog --title "Checking installation" --infobox "Checking for working binaries..." 10 30; sleep 3
	if [ ! -r game/netrhost ]; then
		dialog --title "Missing binary" --msgbox "It doesn't seem like Rhost compiled correctly. Check the error log and make sure everything went well." 10 30
		cleanup
		exit
	else
		dialog --title "Starting netrhost configuration" --msgbox "The game seems to have compiled correctly. I will now start your standard editor so you can edit the configuration file to your liking. Once you save and exit, Rhost will be started for the first time." 10 30
		$EDITOR game/netrhost.conf
		dialog --title "Starting Rhost" --msgbox "Rhost will now (hopefully) start. The installer will now exit, and you can try to connect to your game." 10 30
		cleanup;;
		cd game
		./Startmush
	fi
fi

exit
