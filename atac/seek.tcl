source atacTools.tcl

proc usage {} {
	global argv0
	puts "USAGE: $argv0 archive channelName time"
	puts "       time as \"YYYY/MM/DD hh:mm:ss\" in 24h format"

	exit 1
}

#################################
# Parse args
#
set argc [ llength $argv ]
if { $argc != 3 } { usage }

set archiveName [ lindex $argv 0 ]
set channelName [ lindex $argv 1 ]
set startStamp [ lindex $argv 2 ]

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

set valueId [ channel getValueBeforeTime $channelId $startStamp ]
if { [ value valid $valueId ] } {
	puts "before: [ formatValue $valueId ]"
} else {
	puts "before: - not found -"
}
value close $valueId

set valueId [ channel getValueNearTime $channelId $startStamp ]
if { [ value valid $valueId ] } {
	puts "near:   [ formatValue $valueId ]"
} else {
	puts "near:   - not found -"
}
value close $valueId

set valueId [ channel getValueAfterTime $channelId $startStamp ]
if { [ value valid $valueId ] } {
	puts "after:  [ formatValue $valueId ]"
} else {
	puts "after:  - not found -"
}
value close $valueId

channel close $channelId
archive close $archiveId

exit 0
