
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

set archiveId [ archive open $archiveName ]

set channelId [ archive findChannelByPattern $archiveId $channelPattern ]
while { [ channel valid $channelId ] } {
	puts -nonewline [ channel name $channelId ]
	set valueId [ channel getLastValue $channelId ]
	if { [ value valid $valueId ] } {
		puts "\t[ value type $valueId ]\t[ value time $valueId ]"
		value close $valueId
	} else {
		puts "\t- no values -"
	}
	channel next $channelId
}
channel close $channelId

archive close $archiveId
