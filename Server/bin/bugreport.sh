#!/bin/bash
#################################
# Test shell capabilities
#################################
TST[1]=test
if [ $? -ne 0 ]
then
   echo "/bin/sh does not accept array variables."
   echo "please run the ./bin/script_setup.sh script in attempts to fix this."
   echo "You must have a KSH or BASH compatible shell."
   exit 1
fi
#################################
# Environments
VERSION="1.1"
MAILID="rhostmush@gmail.com"
SUBJ="RhostMUSH Automated Bug Report"
LOGID="$(id|cut -f1 -d")"|cut -f2 -d"(")"
MYHOST=`hostname`
SYSTYPE="$(uname -a)"
OUTLOOP=1
LOOP=1
FIRST=1
MAILPRG="mail"
USEREMAIL=""
CONTACTNAME=""
MUSHCOUNT=-1
LINECOUNT=-1
DATE=`date "+%r%D"`
SYNOPSIS=""

FILENAME=
#################################

display_menu()
{
    display_header
    echo ""
    echo "Choose an option from the following:"
    echo "  1) Modify the synopsis."
    echo "  2) Modify the contact details."
    echo "  3) Modify information about the affected mush(es)"
    echo "  4) Modify the bug report"
    echo "  5) Preview email"
    echo "  6) Send email"
    echo "  7) Save Bugreport"
    echo "  8) Load saved Bugreport"
    echo "  9) Delete saved Bugreport"
    echo " 10) QUIT"
    echo ""


    echo "Input> "|tr -d '\012'
    read ANS	

    case $ANS in
	1)
	    enter_synopsis
	    ;;
	2)
	    enter_contact_details
	    ;;
	3)
	    enter_mush_information
	    ;;
	4)
	    enter_bug_report
	    ;;
	5)
	    clear
	    parse_email
	    ;;
	6)
	    send_email
	    ;;
	7)
	    save_email
	    ;;
	8)
	    load_bugreport
	    ;;
	9) 
	    del_bugreport
	    ;;
	10)
	    echo "Are you sure? [Y/N]"
	    LOOP=1
	    while [ ${LOOP} -eq 1 ]
	      do
	      echo "Input> "|tr -d '\012'
	      read ANS
	      
	      if [ "${ANS}" = "Y" ]
		  then
		  echo "Goodbye."
		  exit 0
	      elif [ "${ANS}" = "N" ]
		  then
		  LOOP=0
	      else
		  echo "Only enter Y for 'yes' or N for 'no'"
	      fi
	    done
	    ;;
	*)
	    echo "Unknown option. Expected a number between 1 and 10"
	    ;;
    esac

   echo "Press <enter> to continue"
   read IGNORE

}

display_header()
{
    clear
    echo "-----------------------------------------------------------------------------"
    echo "      Welcome to the RhostMUSH bug reporting engine version $VERSION."
    echo "-----------------------------------------------------------------------------"
    echo "Bug report being entered by: $LOGID@$MYHOST"
    if [ ! "${SYNOPSIS}" = "" ]
    then
	echo "Synopsis: ${SYNOPSIS}"
    fi
    if [ ! "${USEREMAIL}" = "" ]
    then
	echo "Using contact email address: ${USEREMAIL}"
    fi
    if [ ! "${CONTACTNAME}" = "" ]
    then
	echo "Using contact name: ${CONTACTNAME}"
    fi
    if [ ${MUSHCOUNT} -gt 0 ]
    then
	echo "${MUSHCOUNT} mush(es) have been identified as having this problem."
    fi
    if [ ${LINECOUNT} -gt 0 ]
    then
	echo "${LINECOUNT} line(s) of bug information have been written."
    fi
    echo "-----------------------------------------------------------------------------"
}

enter_synopsis()
{
    display_header

    OUTLOOP=1
    while [ ${OUTLOOP} -eq 1 ]
      do
      if [ ! "${SYNOPSIS}" = "" ]
	  then
	  echo "The synopsis is current set to:"
	  echo " > ${SYNOPSIS}"
	  echo "If this is correct press 'enter' now, else type a new one."
      else
	  echo "Please enter a synopsis:"
      fi	

      LOOP=1
      while [ ${LOOP} -eq 1 ]
      do
	echo "Input> "|tr -d '\012'
	read ANS
	
	if [ "${ANS}" = "" ]
	    then
	    LOOP=0
	    OUTLOOP=0
	else
	    SYNOPSIS=${ANS}
	    LOOP=0
	fi
      done	
    done
}

enter_contact_details()
{
    display_header

    # Only going to be the case first time in.
    if [ "${USEREMAIL}" = "" ]
    then
	USEREMAIL=${USER}@${HOSTNAME}
	echo "The bug engine has determined your email address to be:"
	echo "> ${USEREMAIL}"
	echo ""
    else
	echo "Your email address is currently set to: $USEREMAIL"
	echo ""
    fi

    OUTLOOP=1
    while [ ${OUTLOOP} -eq 1 ]
    do
	echo "If this is correct press 'enter' now, else type in your email address."
	
	LOOP=1
	while [ ${LOOP} -eq 1 ]
	do
	    echo "Input> "|tr -d '\012'
	    read ANS
	    
	    if [ "${ANS}" = "" ]
	    then
		LOOP=0
		OUTLOOP=0
	    else
		if [ `echo ${ANS} | grep -c ".*@.*\..*"` == 1 ]
		then
		    USEREMAIL=${ANS}
		    LOOP=0
		else
		    echo "${ANS} doesn't look to be a valid email address.. try again?"
		fi
	    fi
	done	
	
    done

    OUTLOOP=1
    while [ ${OUTLOOP} -eq 1 ]
    do
      if [ ! "${CONTACTNAME}" = "" ]
	  then
	  echo "Your contact name is currently set to: $CONTACTNAME"
	  echo "If this is correct press 'enter' now, else type in a contact name."
      else
	  echo "Please enter a contact name:"
      fi	
      LOOP=1
      while [ ${LOOP} -eq 1 ]
	do
	echo "Input> "|tr -d '\012'
	read ANS
	
	if [ "${ANS}" = "" ]
	    then
	    LOOP=0
	    OUTLOOP=0
	else
	    CONTACTNAME=${ANS}
	    LOOP=0
	fi
      done	
    done

    echo ""
    echo "Using contact email: $USEREMAIL"
    echo "Using contact name : $CONTACTNAME"
    echo ""
}


enter_mush_information()
{
    display_header

    ERROR=0

    if [ -d game ]
    then
	echo ""
	echo "At this stage, you will be prompted with what mushes you have running"
	echo "in your present environment."
	echo ""
	echo "This information is optional, although may enable us to support you more"
	echo "easily as it gives us another place to contact you and try verify the"
	echo "issue."
	echo ""
	echo "Press <enter> to continue"
	read IGNORE
	clear
	display_header
	echo ""

	COUNT=0

	echo "Running from `pwd`"
	for i in `ls -d */Startmush 2>/dev/null | cut -d"/" -f1 `
        do
            COUNT=`expr $COUNT + 1`
	    echo "... Processing directory: $i"

            # These are from mush.conf
            GAMENAME=`grep ^GAMENAME ${i}/mush.config | cut -d"=" -f2`
            OWNER=`grep ^OWNER ${i}/mush.config | cut -d"=" -f2`

            # These are from netrhost.conf
            MUDNAME=`grep ^mud_name ${i}/${GAMENAME}.conf |awk '{printf("%s",substr($0,index($0,$2)))}'`
            PORT=`grep ^port ${i}/${GAMENAME}.conf |awk '{printf("%s",substr($0,index($0,$2)))}'`

            LINE_[${COUNT}]="$MUDNAME at ${HOSTNAME}:${PORT}"
            BY_[${COUNT}]="${OWNER}"
        done

	echo ""
	if [ ${COUNT} -eq 0 ]
	then
	    echo "Hmmm, I couldn't seem to find any directories with Startmush scripts..."
	    ERROR=1
	else
	    X=1
	    
	    while [ ${X} -le ${COUNT} ]
	    do
		echo "${X}) ${LINE_[${COUNT}]} .. Run by ${BY_[${COUNT}]}"
		X=`expr ${X} + 1`
	    done
	fi
	echo ""

    else
        echo ""
        echo "I couldn't seem to find the game directory"
	ERROR=1
    fi

    if [ "${ERROR}" = "0" ]   
    then
        LOOP=1

	echo "Please select any mushes with the issue <n1,n2,...nx> " 
        echo "Press <enter> to skip this step." 
	echo "Input: "| tr -d '\012'
	read MUSH

	LC=1
	MUSHCOUNT=0
	MUSHLIST=""

	while [ ${LC} -eq 1 ]
	do
	  
	  X=`echo $MUSH | cut -d"," -f1`
	  Y=`expr $X + 0 2>/dev/null`	      
	  Z=`expr ${X} + 0 2>/dev/null`

	  if [ -z "${X}" ]
	  then
	      LC=0
	  elif [ -z "${Y}" -o -z "${Z}" ]
	  then		  
	      echo "${X} is not a valid value."
	  else
	      MUSHCOUNT=`expr $MUSHCOUNT + 1`
	      MUSHLIST="${MUSHLIST}\n${LINE_[${Y}]} .. ${BY_[${Y}]}"
	  fi
	  
	  #Hacky.. if no delimeter exists in the line, then cut
	  #returns the line - this was causing an infinite loop
	  #on the last element.
	  TMP_MUSH=`echo $MUSH | cut -d"," -f2-`
	  if [ "${TMP_MUSH}" = "${MUSH}" ]
          then
	      LC=0
	  else
	      MUSH=${TMP_MUSH}
	  fi
	  
	done
	echo ""
	if [ ${MUSHCOUNT} -gt 0 ]
	then
	    echo -e "You've chosen: ${MUSHLIST}"
	else
	    echo "No detail will be included about running mushes."
	fi
    else
        echo "As this is a default part of the Rhost installation I'm going"
        echo "to assume that either this installation is incomplete, or that"
        echo "the script is not being run from the top-level Rhost directory."
    fi

    echo ""

}

enter_bug_report()
{
    display_header

    echo "Enter description of the bug below."
    echo "Type '.' or 'END' on a seperate line to finish."
    echo "Type HELP for help, QUIT to exit."
    echo "All options require upper case."

    LOOP=1

    if [ ${LINECOUNT} -lt 0 ]
    then
	LINECOUNT=0
    fi

    while [ ${LOOP} -eq 1 ]
    do
      echo "Input> "|tr -d '\012'
      read ANS

      CMD="$(echo "${ANS}" | cut -d" " -f1)"
      ARG1="$(echo "${ANS}" | cut -d" " -f2- | cut -d":" -f1)"
      ARG2="$(echo "${ANS}" | cut -d" " -f2- | cut -d":" -f2)"
	
      if [ "${ANS}" = "." -o "${ANS}" = "END" ]
      then
	  LOOP=0
	  OUTLOOP=0
      elif [ "${ANS}" = "HELP" ]
      then
	  echo "The following keys exist for the editor:"
	  echo "    HELP              : This will display this text."
	  echo "    .                 : This will end the editor."
	  echo "    END               : This will end the editor."
	  echo "    RESET             : Reset (clear) the message and start fresh."
	  echo "    LIST              : List the current contents of the buffer."
	  echo "    DEL <X>           : Delete the line specified by <x>"
	  echo "    SUB <X>:<Y>/<Z>   : Edit line X, replace text <Y> with text <Z>"
	  echo "    REPL <X>:<Y>      : Replace line X with the text <Y>"
	  echo "    IMPORT <filename> : This imports the file to be included."
	  echo "    QUIT              : Quits the bug report script."
	  echo ""
	  
      elif [ "${ANS}" = "RESET" ]
      then
	  echo "Message reset."
	  LINECOUNT=0
	  
      elif [ "${ANS}" = "QUIT" ]
      then
	  echo "Script aborted."
      exit 0

      elif [ "${CMD}" = "SUB" ]
      then
	  TMP=`expr $ARG1 + 0 2>/dev/null`   
	  if [ -z "${TMP}" ]
	  then
	      echo "Numerical value expected.  Please choose from 1 to $LINECOUNT."
	  elif [ ${TMP} -lt 1 -o ${TMP} -gt ${LINECOUNT} ]
	  then
	      echo "Numerical value out of range.  Please choose from 1 to $LINECOUNT."
	  else 
	      LHS=`echo $ARG2 | cut -f1 -d"/"`
	      RHS=`echo $ARG2 | cut -f2 -d"/"`
	      if [ -z "${LHS}" ]
	      then
		  echo "The Left Hand Expression cannot be an empty string."
	      else
		  X=`expr ${ARG1} - 1`
		  BUGREPORT_[${X}]=`echo "${BUGREPORT_[${X}]}" | sed -e "s/${LHS}/${RHS}/g"`
		  echo "Line ${ARG1} was modified."
		  echo " > ${BUGREPORT_[${X}]}"
	      fi
	  fi
      elif [ "${CMD}" = "DEL" ]
      then
	  TMP=`expr $ARG1 + 0 2>/dev/null`   
	  if [ -z "${TMP}" ]
	  then
	      echo "Numerical value expected.  Please choose from 1 to $LINECOUNT."
	  elif [ ${TMP} -lt 1 -o ${TMP} -gt ${LINECOUNT} ]
	  then
	      echo "Numerical value out of range.  Please choose from 1 to $LINECOUNT."
	  else 
	      TMP=`expr ${LINECOUNT} - 1`
	      TMPARG=`expr ${ARG1} - 1`
	      X=0
	      Y=0
	      while [ ${X} -lt $TMP ]
	      do
		if [ ${X} = ${TMPARG} ]
		then
		    Y=`expr ${Y} + 1`
		fi

	      BUGREPORT_[${X}]=${BUGREPORT_[${Y}]}
	      Y=`expr ${Y} + 1`	    
	      X=`expr ${X} + 1`	      
	      done	      

	      LINECOUNT=`expr ${LINECOUNT} - 1`
	      echo "Line ${ARG1} was deleted."

	  fi
	  
      elif [ "${CMD}" = "REPL" ]
      then
	  TMP=`expr $ARG1 + 0 2>/dev/null`   
	  if [ -z "${TMP}" ]
	  then
	      echo "Numerical value expected.  Please choose from 1 to $LINECOUNT."
	  elif [ ${TMP} -lt 1 -o ${TMP} -gt ${LINECOUNT} ]
	  then
	      echo "Numerical value out of range.  Please choose from 1 to $LINECOUNT."
	  else      
	      BUGREPORT_[`expr ${TMP} - 1`]="${ARG2}"
	      echo "Line ${TMP} replaced."
	  fi
	  
      elif [ "${ANS}" = "LIST" ]
      then
	  echo "-----------------------------------------------------------------------"
	  X=0
	  while [ ${X} -lt ${LINECOUNT} ]
	  do
	    echo "${BUGREPORT_[${X}]}"
	    X=`expr ${X} + 1`
	  done
	  echo "-----------------------------------------------------------------------"

      elif [ "${CMD}" = "IMPORT" ]
      then
      if [ ! -e ${ARG1} ]
      then
	  echo "File \"${ARG1}\" does not exist."
      elif [ ! -r ${ARG1} ]
      then
	  echo "Cannot read file \"${ARG1}\"."
      elif [ `du -k ${ARG1} | cut -f"1"` -gt 2048 ]
      then
	  echo "Cannot import files bigger than 2megabytes, sorry."	  
      else
	  FTYPE="$(file ${ARG1} 2>&1|grep -c text)"
	  if [ ${FTYPE} -gt 0 ]
	  then
	      while read fline
	      do
		BUGREPORT_[${LINECOUNT}]="${fline}"
		LINECOUNT=`expr $LINECOUNT + 1`
	      done < ${ARG1}
	      echo "File included."
	      echo "Bug report now contains ${LINECOUNT} line(s)"		   
	  else
	      echo "That is not a valid text file."
	      echo "If you wish to include an attachment, then please mail"
	      echo "seperately to bugs@rhostmush.org with the attachment"
	  fi
      fi
      else
	  BUGREPORT_[${LINECOUNT}]="${ANS}"
	  LINECOUNT=`expr $LINECOUNT + 1`
	  echo "Bug report now contains ${LINECOUNT} line(s)"
      fi
    done
}

save_email() {
    if [ -z "$FILENAME" ]
    then
	FILENAME=".rhostbugreport_save.$$"
    fi
    
    touch ${FILENAME}

    echo "# Rhost Bug Reporting script, v.$VERSION" > ${FILENAME}
    echo "# Last Modified:`date +%d%m%y.%H%M`" >> ${FILENAME}
    
    echo "LOGID=\"${LOGID}\"" >> ${FILENAME}
    echo "HOSTNAME=\"${HOSTNAME}\""  >> ${FILENAME}
    echo "USEREMAIL=\"${USEREMAIL}\"" >> ${FILENAME}
    echo "CONTACTNAME=\"${CONTACTNAME}\"" >> ${FILENAME}
    echo "SYNOPSIS=\"${SYNOPSIS}\"" >> ${FILENAME}
    X=1
    while [ ${X} -le ${MUSHCOUNT} ]
    do
      echo "LINE_[${X}]=\"${LINE_[${X}]}\"" >> ${FILENAME}
      echo "BY_[${X}]=\"${BY_[${X}]}\"" >> ${FILENAME}
      X=`expr ${X} + 1`
    done
    echo "MUSHCOUNT=${MUSHCOUNT}" >> ${FILENAME}

    X=0
    while [ ${X} -lt ${LINECOUNT} ]
    do
      echo "BUGREPORT_[${X}]=\"${BUGREPORT_[${X}]}\"" >> ${FILENAME}
      X=`expr ${X} + 1`
    done    
    echo "LINECOUNT=${LINECOUNT}" >> ${FILENAME}

    echo "File saved as: $FILENAME"
}

load_bugreport()
{
    OPERATION=LOAD
    display_header
    echo ""
    echo "Loading a saved bugreport."
    process_bugreport_file
    OPERATION=
}

del_bugreport()
{
    OPERATION=DEL
    display_header
    echo ""
    echo "Deleting a saved bugreport."
    process_bugreport_file
    OPERATION=
}

process_bugreport_file()
{
    NFILES=1
    for i in `ls .rhostbugreport_save* 2>/dev/null`
    do
      FILENAME_[${NFILES}]="$i"
      NFILES=`expr ${NFILES} + 1`
    done

    echo ""
    echo ""
    echo "0) Return to Menu"
    X=1
    while [ ${X} -lt ${NFILES} ]
    do
      SYNOPSIS="`grep ^SYNOPSIS ${FILENAME_[${X}]} | cut -f2 -d'='`"
      if [ "${SYNOPSIS}" = "" ]
      then
	  SYNOPSIS="<No synopsis>"
      fi
      echo "${X}) ${FILENAME_[${X}]} (${SYNOPSIS})"
      echo "                                                    Last modified: `grep '^# Last Modified' ${FILENAME_[${X}]} | cut -f2 -d:`"
      X=`expr ${X} + 1`
    done

    echo ""
    echo "Enter the number of the desired bugreport:"   
    echo "Input> "|tr -d '\012'
    read ANS

    if [ "`expr $ANS + 0 2>/dev/null`" = "" ]
    then
	echo "Invalid input: Pick a number from the above list."
    elif [ $ANS -lt 0 -o $ANS -gt $NFILES ]
    then
	echo "Invalid input: Pick a number from the above list."
    elif [ $ANS -eq 0 ]
    then
	echo "Operation cancelled."
    else
	case $OPERATION in
	    LOAD)
		source ${FILENAME_[${ANS}]}
		echo "${FILENAME_[${ANS}]} loaded."
		;;
	    DEL)
		rm ${FILENAME_[${ANS}]}
		echo "${FILENAME_[${ANS}]} deleted."
		;;
	    *)
		echo "Unknown operation $OPERATION - report this to lensman@rhostmush.org"
	esac
    fi

}


send_email()
{

    if [ ${LINECOUNT} -le 0 ]
    then
	echo "Cannot send an empty bugreport."
    elif [ "${SYNOPSIS}" = "" ]
    then
	echo "Bugreports need a synopsis before they can be sent."
    else
	FILENAME="/tmp/_rhostbugreport.$$"
	parse_email > ${FILENAME}
	mail -s "${SUBJ}: ${SYNOPSIS}" $MAILID < $FILENAME \
	    && echo Mail sent successfully. || echo Failed to send email
	rm ${FILENAME}
    fi

}

parse_email()
{
    echo "<RHOST_BUG_REPORT ver=\"$VERSION\">"
    echo ""
    echo "  <DATE>${DATE}/</DATE>"
    echo "  <CREATED_BY>${LOGID}@${HOSTNAME}</CREATED_BY>"
    echo "  <CONTACT_EMAIL>${USEREMAIL}</CONTACT_EMAIL>"
    echo "  <CONTACT_NAME>${CONTACTNAME}</CONTACT_NAME>"
    echo ""
    echo "  <SYNOPSIS>${SYNOPSIS}</SYNOPSIS>"
    echo ""
    echo "  <MUSHES_AFFECTED>"
    X=1
    while [ ${X} -le ${MUSHCOUNT} ]
    do
      echo "    <MUSH>"
      echo "      <NAME=\"`echo ${LINE_[${X}]} | cut -f1 -d" "`\"/>"
      echo "      <ADDI=\"`echo ${LINE_[${X}]} | cut -f3 -d" "`\"/>"
      echo "      <CONTACT=\"${BY_[${X}]}\"/>"
      X=`expr ${X} + 1`
    echo "    </MUSH>"
    done
    echo "  </MUSHES_AFFECTED>"
    echo ""
    echo "  <REPORT>"
    X=0
    while [ ${X} -lt ${LINECOUNT} ]
    do
      echo "    <LINE>${BUGREPORT_[${X}]}</LINE>"
      X=`expr ${X} + 1`
    done
    echo "  </REPORT>"
    echo "</RHOST_BUG_REPORT>"   
    echo ""
}

handle_intr()
{
  CTRLC=`expr ${CTRLC} + 1`
  if [ ${CTRLC} -gt 1 ]
  then
      echo -e "\nScript interrupted.\n"
      exit -1
  else
      echo -e "\nHit interrupt once more to abort the script.\n"
  fi
}

CTRLC=0
trap handle_intr 2

while [ 1 -eq 1 ]
do
    display_menu
done
