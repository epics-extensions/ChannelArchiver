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
	puts "USAGE: $argv0 archive channelName \[ startTime \[ endTime \] \]"
	puts "       start/endTime as \"YYYY/MM/DD hh:mm:ss\" in 24h format"
	puts ""
	puts "- Dump values for given channel between start- and end time"
	puts ""
	puts "Example"
	puts "-------"
	puts "tcl dump.tcl ../../Engine/Test/freq_directory fred \"2000/03/23 10:16:09\" \"2000/03/23 10:18:54\""
	puts "2000/03/23 10:16:09.800541744 0.4891"
	puts "2000/03/23 10:16:11.803278963 0.5086"
	puts "2000/03/23 10:16:13.806193020 0.4196"
	puts "2000/03/23 10:16:14.806193020 Archive_Off"
	puts "2000/03/23 10:18:35.469894785 0.3443"
	puts "2000/03/23 10:18:52.424309954 0.4355"

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
set archive [ archive ]
set channel [ channel ]
set value   [ value ]
if { ! [ $archive open $archiveName ] } {
	puts "$archiveName: cannot open"
	exit 1
}
if { ! [ $archive findChannelByName $channelName $channel ] } {
	puts "$channelName: not found"
	exit 1
}

if { $startStamp > 0 } {
	$channel getValueAfterTime $startStamp $value
} else {
	puts "First Sample: [ $channel getFirstTime ]"
	puts "Last  Sample: [ $channel getLastTime  ]"
	$channel getFirstValue $value
}

# Loop until end of archive or end time exceeded
while { [ $value valid ] && [ $value time ] <= $endStamp } {
	puts [ formatValue $value ]
	$value next
}

rename $value {}
rename $channel {}
rename $archive {}

exit 0