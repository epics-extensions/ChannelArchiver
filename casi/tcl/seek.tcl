# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

source casiTools.tcl

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
set archive [ archive ]
set channel [ channel ]
$archive open $archiveName
$archive findChannelByName $channelName $channel
if { ! [ $channel valid ] } {
	puts "$channelName: not found"
	exit 1
}

set value [ value ]
$channel getValueBeforeTime $startStamp $value
if { [ $value valid ] } {
	puts "before: [ formatValue $value ]"
} else {
	puts "before: - not found -"
}

$channel getValueNearTime $startStamp $value
if { [ $value valid ] } {
	puts "near:   [ formatValue $value ]"
} else {
	puts "near:   - not found -"
}

$channel getValueAfterTime $startStamp $value
if { [ $value valid ] } {
	puts "after:  [ formatValue $value ]"
} else {
	puts "after:  - not found -"
}
rename $value {}
rename $channel {}
rename $archive {}

exit 0












