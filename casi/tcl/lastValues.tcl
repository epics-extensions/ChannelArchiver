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
set archive [ archive ]
set channel [ channel ]
set value   [ value ]

$archive open $archiveName
$archive findFirstChannel $channel

while { [ $channel valid ] } {
	$channel getLastValue $value
	puts "[$channel name ] : [ formatValue $value ]"
	$channel next
}

rename $value {}
rename $channel {}
rename $archive {}

exit 0
