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
	puts "\tthe last time a channel was archived is shown..."

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
set archive [ archive ]
set channel [ channel ]
$archive open $archiveName
$archive findFirstChannel $channel

while { [ $channel valid ] } {
	puts "[ $channel getLastTime ]\t[ $channel name ]"
	$channel next
}

rename $channel {}
rename $archive {}

exit 0
