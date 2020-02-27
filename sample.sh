#!/bin/bash
#
# Script Programm Version
VER="Revision: 1.11"

# Path list to search for binaries
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin

# Get Program-Name, shortened Version. Removing .sh extension
PROGNAME="`basename $0 .sh`"

# Execution PID.
PROG_PID=$$

# Logger section
# I want it to be verbose.
VERBOSE=false

# Logfile to use
WRITELOG=true
LogFile=./log/${PROGNAME}.log
# Syslog and options
SYSLOG=true
LOGGEROPT="-n localhost -P 514 -T"
LOGGER=/usr/bin/logger

# Identify user
USER=`whoami`

# Define Lock-File
LockFile=/var/lock/${PROGNAME}.lock

# Include file path
INCPATH=./shmod

# Build Date in reverse , for logging function
DATE=`date +"%Y%m%d"`
# Date + Time
LDATE=`date +"%F @ %T"`

# Source common functions used in scripts
if [ -f ${INCPATH}/shmod.inc ]
then
    source ${INCPATH}/shmod.inc
else
    # In case the shared modules file does not exist, this script
    # cannot function.  Emergency Exit with error message.
    echo "*** FATAL: Missing shared mod ${INCPATH}/shmod.inc!"
    exit 0   
fi

#######################################################################
# Actual script - do not modify anything below this point
#######################################################################

# Prevent double execution
Lock

# One way of using the log-message, put it all into the current MSG
# variable and invoque internal funciton "log"
MSG="Starting program $PROGNAME"
title $LDATE $MSG
#log $MSG
entry "Execution output below"

#sleep 30

if [ -f file.ba ]
then
    # Apply error message assuming the worst-case.
    MSG="Move file.ba to /tmp/foo.ba failed"
    mv -f file.ba /tmp/foo.ba 2>/dev/null
    # Assign the return code of the last program execution
    errlvl=$?
    errors
    
    # Echo the error code back to the console so we can see it.
    MSG="Move file.ba to /tmp/foo.ba Succeeded"
    
else
    separator
    entry "WARNING: File does not exist"
    echo -n " => File file.ba does not exist. Proceed [y/n]: "
    read YesNo

    if [ "$YesNo" == "y" ]
    then
	# Create a configuration-file that will hold database access information
	MSG="Creating DB config file"
	cat /dev/null > config.cnf
	errlvl=$?
	errors
	MSG="Changing DB config file mode"
	chmod 600 config.cnf
	errlvl=$?
	errors
	# All remaining manipulation of the config.cnf file should
	# work. If an error would occure, it would have occured before
	# this point.
	echo "[client]" >> config.cnf
	
	entry "Execution can continue after user choice"
	echo -n " => Please provide the user name for DB access: "
	read DBUSER
	echo "user = $DBUSER" >> config.cnf
	log "Username for accessing DB: $DBUSER"

	echo -n " => Please provide the password for the DB access: "
	read -s DBPWD
	echo "password = $DBPWD" >> config.cnf
	log "Password for accessing DB: XXXXXXXX"
	echo

	echo -n " => Please provide the Database name to use: "
	read DBNAME
	log "Database name to be accessed: $DBNAME"

	echo -n " => Please provide host the database is hosted: "
	read DBHOST
	echo "host = $DBHOST" >> config.cnf
	log "Database host: $DBHOST"
	echo

	entry "Waiting for Mysql to become ready"
	# Waiting for mysql to become ready
	RET=1
	VERBOSE=true
	while [[ RET -ne 0 ]]; do
	    MSG="=> Waiting for confirmation of MySQL service startup"
            log $MSG
            sleep 2
            # Looking for the Uptime string of the Status message. As long as it won't show up, the DB
            # is not ready
            mysql --defaults-extra-file=config.cnf $PCM_MYSQL_DB_NAME -e "status" | grep -q ^Uptime
            RET=$?
	done
	VERBOSE=false
	
    else
	MSG="file.ba does not exist"
	errlvl=1
	errors
    fi
    
fi

MSG="Program execution finished"
# Log the operation. In case an error occured, we exit anyway
log $MSG
entry "$MSG"
space
# remove lock-file
Unlock
