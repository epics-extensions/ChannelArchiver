# Simple example for a "GUI" tool
#
# Run this with wish

source atacTools.tcl

proc usage {} {
	global argv0
	puts "USAGE: $argv0 archive"

	exit 1
}

#################################
# Parse args
#
set argc [ llength $argv ]
if { $argc != 1 } { usage }

set archiveName [ lindex $argv 0 ]

# GUI setup
#
frame .archivename
label .archivename.lbl -text "Archive:"
entry .archivename.entry -textvariable archiveName -width 45
grid .archivename.lbl .archivename.entry

frame .channellist
label .channellist.label -text "Channels:" -anchor w
set channelList [ \
	listbox .channellist.list -width 30 -selectmode multiple \
		-yscrollcommand ".channellist.scroll set"\
	]
scrollbar .channellist.scroll -command ".channellist.list yview"
pack .channellist.label -side top -fill x
pack .channellist.list -side left
pack .channellist.scroll -side right -fill y

frame .buttons
button .buttons.info -text "Channel Info" -command showInfo
button .buttons.clear -text "Clear Output" -command clearOutput
pack .buttons.info
pack .buttons.clear

frame .out
label .out.label -text "Output:" -anchor w
set output [ \
	listbox .out.list -width 40 -selectmode multiple \
		-yscrollcommand ".out.scroll set"\
	]
scrollbar .out.scroll -command ".out.list yview"
pack .out.label -side top -fill x
pack .out.list -side left
pack .out.scroll -side right -fill y


grid .archivename -row 0 -columnspan 3 -sticky w
grid .channellist .buttons .out -row 1


# Main
#
proc listChannels {} {
	global archiveId
	global channelList

	set channelId [ archive findFirstChannel $archiveId ]
	while { [ channel valid $channelId ] } {
		$channelList insert 0 [ channel name $channelId ]
		channel next $channelId
	}
	channel close $channelId
}

proc showInfo {} {
	global archiveId
	global channelList
	global output

	foreach index [ $channelList curselection ] {
		set channelName [ $channelList get $index ]
		$output insert end "$channelName:"
		set channelId [ archive findChannelByName $archiveId $channelName ]
		if { [ channel valid $channelId ] } {
			$output insert end " First: [ channel getFirstTime $channelId ]"
			$output insert end " Last : [ channel getLastTime  $channelId ]"   
			channel close $channelId
		} else {
			$output insert end " - not found -"
		}
	}
}

proc clearOutput {} {
	global output

	$output delete 0 end
}

# Main
#

set archiveId [ archive open $archiveName ]

showAtacNag
listChannels
