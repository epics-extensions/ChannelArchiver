source atacTools.tcl

proc usage {} {
	global argv0
	puts "USAGE: $argv0 archive channelName \[ startTime \[ endTime \] \]"
	puts "       start/endTime as \"YYYY/MM/DD hh:mm:ss\" in 24h format"

	exit 1
}

#################################
# Parse args
#
set argc [ llength $argv ]
if { $argc < 2 } { usage }

set archiveName [ lindex $argv 0 ]
set channelName [ lindex $argv 1 ]
if { $argc > 2 } {
	set startStamp [ lindex $argv 2 ]
} else {
	set startStamp 0
}
if { $argc > 3 } {
	set endStamp [ lindex $argv 3 ]
} else {
	set endStamp "9999/99/99 24:59:59"
}

#################################
# Open archive,
# get first value
#
set archiveId [ archive open $archiveName ]
set channelId [ archive findChannelByName $archiveId $channelName ]
if { ! [ channel valid $channelId ] } {
	puts "$channelName: not found"
	exit 1
}

if { $startStamp > 0 } {
	set valueId [ channel getValueAfterTime $channelId $startStamp ]
} else {
	puts "First Sample: [ channel getFirstTime $channelId ]"
	puts "Last  Sample: [ channel getLastTime  $channelId ]"
	set valueId [ channel getFirstValue $channelId ]
}

# Loop until end of archive or end time exceeded
while { [ value valid $valueId ] && [ value time $valueId ] <= $endStamp } {
	puts [ formatValue $valueId ]
	value next $valueId
}

channel close $channelId
archive close $archiveId
