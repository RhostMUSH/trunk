#!/bin/sh
#RhostMUSH installation script. Fetches you a fresh Rhost, compiles it
#and starts it up. And even comes with dialogs!
#
#Made by Odin, May 4 2007
#
#Standard defines

 
if ! which dialog > /dev/null
then
   echo "There's no 'dialog' on your system, please install it!"
   exit 1
fi

TEMP=/tmp/answer$$
RHOST_USER=member
RHOST_SITE=ftp.rhostmush.org
RHOSTDIST=rhostdistro4.tar.gz
if [ "$EDITOR" = "" ]; then
	EDITOR=/usr/bin/vi
fi
#Cleanup procedure
cleanup() {
	clear
	rm -f $TEMP
	rm -f $RHOSTDIST
	exit
}

	dialog --title "RhostMUSH installer" --msgbox "Welcome to the Rhostmush installation script.\\n\\n We will now connect to the RhostMUSH repository on Google Code. To be able to do this, you must have subversion installed.\\n\\nIf you do not wish to download and install RhostMUSH at this time, press esc now. Otherwise, press enter to continue." 30 80
	if [ "$?" != "0" ]
	then
		dialog --title "RhostMUSH installer" --msgbox "Installation was cancelled by user." 10 50
	else
		dialog --backtitle "Choose distribution" \
			--radiolist "Select distribution" 10 40 2 \
			1 "RhostMUSH 3.9 (Latest SVN)" 'on' \
			2 "RhostMUSH 3.9 (Last stable release)" 'off' 2>$TEMP
		if [ "$?" != "0" ] ; then return; fi
		choice=`cat $TEMP`
		case $choice in
			1) RHOSTDIST=SVN-release;;
			2) RHOSTDIST=stable-release;;
		esac
	fi

	dialog --infobox "Downloading $RHOSTDIST from Google Code, please wait..." 10 30 ; sleep 4
	#wget --http-user=member --http-passwd=`cat $TEMP` http://192.168.0.104/$RHOSTDIST 2>$TEMP
	svn checkout http://rhostmush.googlecode.com/svn/trunk/ rhostmush-read-only
if [ "$?" != "0" ]
then
	dialog --title "RhostMUSH installer" --msgbox "Subversion returned an error, and the download failed. Press enter to see the log of Subversion's attempt to download. Afterwards, the installer will exit." 10 30
	dialog --title "Subversion error log" --textbox $TEMP 13 65 2>/dev/null
	cleanup
	exit
else
	dialog --title "RhostMUSH installer" --msgbox "RhostMUSH has been successfully downloaded." 10 30
	#tar -xzf $RHOSTDIST 2>$TEMP
fi

if [ "$?" != "0" ]
then
	dialog --title "RhostMUSH installer" --msgbox "The RhostMUSH archive did not extract correctly. Press enter to see the error log." 10 30
	dialog --title "Untar error log" --textbox $TEMP 13 65 2>/dev/null
	cleanup
	exit
else
	dialog --title "RhostMUSH installer" --msgbox "The RhostMUSH installer will now call the RhostMUSH configuration script. Configure Rhost to your liking, and then select 'Run' to finalize the configuration." 10 40
	cd rhostmush-read-only/Server
	bin/script_setup.sh
	make confsource
	dialog --title "Checking installation" --infobox "Checking for working binaries..." 10 30; sleep 3
	if [ ! -r game/netrhost ]; then
		dialog --title "Missing binary" --msgbox "It doesn't seem like Rhost compiled correctly. Check the error log and make sure everything went well." 10 30
		cleanup
		exit
	else
		dialog --title "Starting netrhost configuration" --msgbox "The game seems to have compiled correctly. I will now start $EDITOR so you can edit the configuration file to your liking. Once you save and exit, Rhost will be started for the first time." 10 40
		$EDITOR game/netrhost.conf

		 dialog --backtitle "Start Rhost Now?" \
                         --radiolist "Would you like to start Rhost now?" 10 40 2 \
                                      1 "Yes, start Rhost" 'on' \
                                      2 "No, do not start Rhost" 'off' 2>$TEMP
                 if [ "$?" != "0" ] ; then return; fi
            choice=`cat $TEMP`
   	 if [ "$CHOICE" = 1 ]
   	 then
		dialog --title "Starting Rhost" --msgbox "Rhost will now (hopefully) start. The installer will now exit, and you can try to connect to your game." 10 30
	 	cleanup
		cd game
		Startmush
	else
		dialog --title "Installer exiting" --msgbox "The Rhost installer will now exit. You can start your game with ./Startmush in the game directory." 10 30
	fi
	fi
fi

exit
