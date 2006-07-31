#!/bin/sh

# Example crontab entry:
#
# MAILTO=""
#
# Run every two minutes
# min hour day month weekday
# 0-59/2 * * * * /home/xfer/update_server.sh &

cd /arch
source setup.sh

LOG=/tmp/update_server.log
DATED=/tmp/update_server_`date +%Y-%m-%d_%H:%M:%S`.log 

# Prevent multiple runs of the update tool
if [ -f $LOG ]
then
    exit 0
fi

# Run the update
perl scripts/update_server.pl >$LOG 2>&1

# If update_server.pl was a NOP, leaving an empty log file, remove that.
# Otherwise rename into a dated file
if [ -s $LOG ]
then
	mv $LOG $DATED
else
	rm $LOG

fi

