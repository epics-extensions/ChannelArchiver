#!/bin/sh

# This is an example how the tools can be run automatically
#
# When this is launched around 1am,
# it will create a plot for the previous day,
# update the index web page
# and then sleep until the next day.
#
# When started for the first time, do it like this:
#
# sleep <hours until 1am>h; sh autorun.sh
while true
do
    perl makePlot.pl
    perl makeIndex.pl
    echo "Last run: `date`, sleeping 24h..."
    sleep 1d
done
