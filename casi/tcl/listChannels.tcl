
source atacTools.tcl

proc usage {} {
	global argv0

	puts "USAGE: $argv0 archive \[ channelPattern \]"
	exit 1
}

set argc [ llength $argv ]
if { $argc < 1 } { usage }

set archiveName [ lindex $argv 0 ]
if { $argc > 1 } {
	set channelPattern [ lindex $argv 1 ]
} else {
	set channelPattern ""
}

set archive [ archive ]
set channel [ channel ]
set value   [ value ]
$archive open $archiveName
$archive findChannelByPattern $channelPattern $channel
while { [ $channel valid ] } {
	puts -nonewline [ $channel name ]
	$channel getLastValue $value
	if { [ $value valid ] } {
		puts "\t[ $value type ]\t[ $value time ]"
	} else {
		puts "\t- no values -"
	}
	$channel next
}

rename $value {}
rename $channel {}
rename $archive {}

exit 0
