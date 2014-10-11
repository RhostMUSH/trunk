#!/bin/sh
#RhostMUSH installation script. Fetches you a fresh Rhost, compiles it
#and starts it up. And even comes with dialogs!
#
#Made by Odin, May 4 2007
#
#Standard defines

echo "This will pull the latest SVN distribution from the primary trunk."
echo "This script will also autorun the make confsource to compile the code"
echo "and issue ./Startmush to start up your mush."
echo ""
echo "This can now grab any optional branch as well.  Good news!"
echo "If you already have the latest code, you probably don't want to run this."
echo ""
echo "After compiling, you would then go into the game directory and type:"
echo "                   ./Startmush"
echo ""
echo "Do you wish to continue on with this script? (Y/n): " |tr -d '\012'
read ANS
if [ -n "${LINES}" ]
then
   gl_rows=${LINES}
fi
if [ -n "${COLUNNS}" ]
then
   gl_cols=${COLUNNS}
fi
if [ -z "${gl_rows}" ]
then
   gl_rows=$(stty -a|grep col|cut -f2 -d";"|awk '{print $2}')
fi
if [ -z "${gl_cols}" ]
then
   gl_rows=$(stty -a|grep col|cut -f3 -d";"|awk '{print $2}')
fi
if [ -z "${gl_rows}" ]
then
   gl_rows=24
fi
if [ -z "${gl_cols}" ]
then
   gl_cols=80
fi

# this should be the size of the trunk repository
full=19564

ANS2=`echo ${ANS}|tr '[:upper:]' '[:lower:]'`
if [ "${ANS2}" = "n" -o "${ANS2}" = "no" ]
then
   echo "Aborted by user."
   exit 0
fi
 
if ! which dialog > /dev/null
then
   echo "There's no 'dialog' on your system, please install it!"
   exit 1
fi

if ! which svn > /dev/null
then
   echo "There's no 'svn' on your system, please install it!"
   exit 1
fi

if ! which git > /dev/null
then
   echo "There's no 'git' on your system, please install it!"
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
        if [ "$1" != "noexit" ]
        then
	   exit
        fi
}

	dialog --title "RhostMUSH installer" --msgbox "Welcome to the Rhostmush installation script.\\n\\n We will now connect to the RhostMUSH repository on Google Code. To be able to do this, you must have subversion installed.\\n\\nIf you do not wish to download and install RhostMUSH at this time, press esc now. Otherwise, press enter to continue." 30 80
	if [ "$?" != "0" ]
	then
		dialog --title "RhostMUSH installer" --msgbox "Installation was cancelled by user." 10 50
	else
		dialog --backtitle "Choose distribution" \
			--radiolist "Select distribution" 10 50 3 \
                        1 "RhostMUSH 3.9.5 (Latest GIT Hub)" 'on' \
			2 "RhostMUSH 3.9.4 (Last SVN available)" 'off' \
			3 "RhostMUSH 3.9.4 (Last stable release)" 'off' 2>$TEMP
		if [ "$?" != "0" ] ; then exit 1; fi

		choice=`cat $TEMP`
		case $choice in
                        1) RHOSTDIST=git-hub;;
			2) RHOSTDIST=SVN-release;;
			3) RHOSTDIST=stable-release;;
		esac
	fi

        if [ "${RHOSTDIST}" = "git-hub" ]
        then
           dialog --backtitle "Which branch do you wish to use?" \
                  --radiolist "The following exist:" 10 40 1 \
                                         1 "The Main Trunc" 'on' 2>$TEMP
	   if [ "$?" != "0" ] ; then exit 1; fi
        else
           dialog --backtitle "Which branch do you wish to use?" \
                  --radiolist "The following exist:" 10 40 5 \
                                         1 "The Main Trunc" 'on' \
                                         2 "Ashen-Shugar's personal branch" 'off' \
                                         3 "Ambrosia's personal branch" 'off' \
                                         4 "Odin's personal branch" 'off' \
                                         5 "Kage's personal branch" 'off' 2>$TEMP
	   if [ "$?" != "0" ] ; then exit 1; fi
        fi
        
	choice=`cat $TEMP`
        mydist="trunk"
        case "${choice}" in
           1) mydist="trunk"
              ;;
           2) mydist="branches/ashen-shugar"
              ;;
           3) mydist="branches/ambrosia"
              ;;
           4) mydist="branches/odin"
              ;;
           5) mydist="branches/kage"
              ;;
           *) mydist="trunk"
              ;;
        esac
        if [ "${RHOSTDIST}" = "git-hub" ]
        then
           git clone https://github.com/RhostMUSH/${mydist} rhostmush-read-only > /dev/null 2>&1 &
           ret=$!
        else
    	   svn checkout http://rhostmush.googlecode.com/svn/${mydist}/ rhostmush-read-only > /dev/null 2>&1 &
           ret=$!
        fi
        ln -s rhostmush-read-only Rhost
        ps -p ${ret} > /dev/null 2>&1
        xxx=$?
        export percent=0
        ((myrow=${gl_rows}/2-16))
        ((mycol=${gl_cols}/2-15))
#clear
#echo "Columns: $mycol [${gl_cols}], $myrow [${gl_rows}]"
#exit 1
        while [ $xxx -eq 0 ]
        do
           sleep 1
           ps -p ${ret} > /dev/null 2>&1
           xxx=$?
           test=$(du -sk rhostmush-read-only 2>/dev/null|awk '{print $1}')
           if [ -z "$test" ]
           then
              test=0
           fi
           ((percent=${test}*100/${full}))
           echo ${percent}
        done| dialog --gauge "Downloading $RHOSTDIST from Google Code, please wait..." 10 30 
#       done| dialog --infobox "Downloading $RHOSTDIST from Google Code, please wait..." 10 30   \
#	     --and-widget --no-lines --begin ${mycol} ${myrow} --gauge "" 6 30
        ret=0
	#wget --http-user=member --http-passwd=`cat $TEMP` http://192.168.0.104/$RHOSTDIST 2>$TEMP
if [ "$ret" != "0" ]
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
   	 if [ "$choice" -eq 1 ]
   	 then
		dialog --title "Starting Rhost" --msgbox "Rhost will now (hopefully) start. The installer will now exit, and you can try to connect to your game." 10 30
	 	cleanup noexit
		cd game
		./Startmush
	else
		dialog --title "Installer exiting" --msgbox "The Rhost installer will now exit. You can start your game with ./Startmush in the game directory." 10 30
	fi
	fi
fi

exit
