# $Id$

source atacTools.tcl

proc usage {} {
	global argv0
	puts "USAGE: $argv0 archive"
	puts "\tFor all channels in archive,"
	puts "\tthe first time a channel was archived is shown..."

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
# get first value
#
set archiveId [ archive open $archiveName ]
set channelId [ archive findFirstChannel $archiveId ]

while { [ channel valid $channelId ] } {
	puts "[ channel getFirstTime $channelId ]\t[ channel name $channelId ]"
	channel next $channelId
}

channel close $channelId
archive close $archiveId

exit 0
