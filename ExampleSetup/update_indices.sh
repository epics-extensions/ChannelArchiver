#!/bin/sh
#
# Usually, update_server.sh/pl are used.
# See comments in there.
#
# Example crontab entry for this script (if its used at all directly):
#
# MAILTO=""
#
# Run every 20 minutes
# min hour day month weekday
# 0-59/20 * * * * /arch/scripts/update_indices.sh &

cd /arch
source setup.sh

LOG=/tmp/update_indices.log
DATED=/tmp/update_indices_`date +%Y-%m-%d_%H:%M:%S`.log 

# Prevent multiple runs of the update tool
if [ -f $LOG ]
then
    exit 0
fi

# Run the update
perl scripts/update_indices.pl >$LOG 2>&1

# If update_indices.pl was a NOP, leaving an empty log file, remove that.
# Otherwise rename into a dated file
if [ -s $LOG ]
then
	mv $LOG $DATED
else
	rm $LOG

fi

