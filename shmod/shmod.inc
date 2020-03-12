###############################################################################
# Log function
#
# Required information/VARIABLES
# VERBOSE: true|false - if set to true, all will be echoed to stdout.
# LOGFILE: Location of Logfile to write data into.
# WRITELOG: true|false - write log message to file?
# SYSLOG: true|false - Send data to Syslog?
# LOGGER: Binary of program sending data to syslog (only required if one wants 
#         to send data to Syslog)
# LOGGEROPT: options to provide to the logger program. If the syslog server is 
#            on a remote server/IP, provide the location here.
# USER: id of user executing the script.
# 
# Call with: log <MESSAGE>
log() {

    # Be verbose
    ! $VERBOSE || echo ">>> ${PROGNAME}[${PROG_PID}]: $*"

    # no logger found, no syslog capabilities
    if [ $SYSLOG ]
    then
        if [ -x $LOGGER ]
        then
            ! $SYSLOG || $LOGGER $LOGGEROPT -t "${PROGNAME}[${PROG_PID}]:" "$*"
        else
            echo "* FATAL: No $LOGGER logger binary found. Aborting script execution!"
	    exit 1
        fi
    fi

    if [ $WRITELOG ]
    then
        if [ -f $LogFile ]
        then
            # Write it all out to a Log-File.
            echo "> $LDATE by $USER ${PROGNAME}: $*" >> $LogFile
        else
	    # In case this is the first run, the LogFile will not yet
	    # exist. Create it and do the same logic
	    touch $LogFile
	    if [ -f $LogFile ]
            then
                # Write it all out to a Log-File.
                echo "> $LDATE by $USER ${PROGNAME}: $*" >> $LogFile
            else
                # Final error message - we can't write out logs even
                # though configuration told us to do so. Emergency
                # Exit.
                echo "* FATAL: No existing $LogFile logfile found. Aborting logfile writing!"
                exit 1
            fi
        fi
    fi

} # log function.


###############################################################################
# errors function
#
# Required information/VARIABLES
# errlvl: return code of the last ran program
# MSG: Message to display
# 
# Call with: "errors" at the appropriate location in the code.
errors() {
    # Set default exist level
    : ${errlvl:=9}
    # Set default error message
    : ${MSG:="No Error message - Probably user interruption"}

    # 
    if [ $errlvl -gt 0 ] ;
    then
	# Level 15 is just a Warning. So issue a Warning
	if [ $errlvl = 15 ] ;
	then
            # Using the Log function from our previous examples
            log "WARNING: $MSG"
	else
            # We have an error different from 0 or 15! Exit issuing a Warning message 
            # audio notification if possible.
            echo -e "\a"
	    echo "========================================================================" 
            echo "*** FATAL:  An error occured in \"${PROGNAME}(${FUNCTION})\" code $errlvl. Bailing out..."
            log "FATAL: $MSG"
	    Unlock
            exit $errlvl
	fi
    fi
}


##############################################################################
#
# Lockfile Generation
# tolock: LockFile location with full path, define in main script.
# action: number, exit-code to be fed to the error-handler function
# 
# Call with: Lock tolock Action

Lock() {
# Lockfile to create
tolock="$LockFile" # Lockfile location with full path
Action="$2" # number, exit-code to be fed to the error-handler function
#
# Lock file if lockfile does not exist.
if [ -s $tolock ]
then
    # If we have provided a second Var, set Exit status using  it.
    if [ ! -n "$Action" ]
    then
	COUNT=0
        while [ -s $tolock ]
        do
            # Oops, we  found a lockfile. Loop while checking if still exists.
	    EXISTINGPID=`cat $tolock` # Extract PID out of Lock file
	    CHECKPID=`pidof -x $0`    # extract PID's of scripts with same name
	    CHECKRES=`echo $CHECKPID | grep $EXISTINGPID` # Check if locked PID is in running list
	    VERBOSE=true
	    # Issue error message
	    MSG="Lockfile for PID $EXISTINGPID exists. Waiting 5secs until next check"

	    # Checking if PID's exist
	    if [ -n "$CHECKRES" ]
	    then
		errlvl=15
		errors
		sleep 5;
		let COUNT=($COUNT + 1)
		if [ $COUNT -eq 3 ]
		then
		    VERBOSE=true
		    log "Unable to gain lock \"$tolock\". Exiting!"
		    exit 1
		fi
	    else
		MSG="Found stale lockfile... Removing it..."
		errlvl=15 # Send out Warning message
		errors
		rm -f $tolock
		errlvl=$?
		errors
		break
	    fi
	done
	
        MSG="Creating lockfile $tolock failed after 5 secs"
        # write PID into Lock-File.
	CreateLock	
    else
        Pid="`cat $tolock`"
        Exists="`ps auxw | grep " $Pid " | grep -c $PROGNAME`"
        if [ $Exists = 1 ]
        then
            MSG=""$PROGNAME" already running. Exiting..."
            errlvl=$Action
            errors
        else
            MSG="Found stale lockfile... Removing it..."
            errlvl=15 # Send out Warning message
            errors
            rm -f $tolock
            errlvl=$?
            errors
            MSG="Creating lockfile $tolock failed"
            echo $$ > $tolock
            errlvl=$?
            errors
        fi
    fi
else
    # Lock it
    CreateLock
fi
} # Lock

# ACtually create the lock-file.
CreateLock (){
    # Lock it
    MSG="Creating lockfile $tolock failed"
    echo $$ > $tolock
    errlvl=$?
    errors
}


##############################################################################
#
# Lockfile removal
# No arguments required. Works as is.
#
Unlock(){
# Name of Lockfile to unlock
unlock="$LockFile"
# Unlock the file.
if [ -s $unlock ]
then
    PID=$$
    if [ "`cat $unlock`" != "$PID" ]
    then
        # Lock it
        errlvl=15
        MSG="Wrong lock-file PID. Probably a race-condition happened..."
        errors
    else
        # Removing Lockfile
        rm -f $unlock
    fi
fi
#
} # Unlock

##############################################################################
#  Formatting functions
##############################################################################

# space - Inserts an empty space. Can also use an echo in the code, only if an
# overall change is to be done, one change only will be required if this function
# is used.
space () {
    MSG="space: Can't grab console"  
    echo ""
    errlvl=$?
    errors
}

##############################################################################
# separator - will add a separator line, 80 times =
separator () {
    MSG="separator: Can't grab console"
    echo "================================================================================" 
    errlvl=$?
    errors
}

##############################################################################
# entry - will add a text entry and format what it can.
entry () {

    # Set to
    line=""
    lg=`echo $* | wc -c`
    let length=($lg + 8)
    while [ $length -lt 80 ]; 
    do
        line="${line}="
        let length=($length + 1)
    done

    # Add a space
    space
    MSG="entry: Can't grab console"
    echo "=== $* !  $line"
    errlvl=$?
    errors
}

##############################################################################
# title - will Embedd the title in between 2 lines.
title () {
    # Set to
    line=""
    lg=`echo $* | wc -c`
    let length=($lg + 4)
    while [ $length -lt 80 ]; 
    do
        line="${line}="
        let length=($length + 1)
    done

    space
    space
    # Note - we will skip the error handler here. If we can't grab the
    # console, the space function will bail out first anyway.
    echo "==============================================================================="
    echo "== $* $line"
    echo "==============================================================================="
    echo
}

##############################################################################
# cleanup - Cleans up lockfile and exits cleanly
cleanup () {
    # Set a human readable message/cause
    MSG="Script interrupted by user request. Exiting!"  
    # release the lock file
    Unlock
    errlvl=1
    errors
}
