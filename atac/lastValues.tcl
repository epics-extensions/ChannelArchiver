# $Id$

source atacTools.tcl

proc usage {} {
	global argv0
	puts "USAGE: $argv0 archive"
	puts "\tFor all channels in archive,"
	puts "\tthe last value..."

	exit 1
}

#################################
# Parse args
#
set argc [ llength $argv ]
if { $argc != 1 } { usage }

set archiveName [ lindex $argv 0 ]

#################################
# Open archive,
# get first channel
#
set archiveId [ archive open $archiveName ]
set channelId [ archive findFirstChannel $archiveId ]

while { [ channel valid $channelId ] } {
	set valueId [ channel getLastValue $channelId ]
	puts "[channel name $channelId ] : [ formatValue $valueId ]"
	value close $valueId
	channel next $channelId
}

channel close $channelId
archive close $archiveId

exit 0
