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
	puts "USAGE: $argv0 archive startTime endTime channelName { channelName }"
	puts ""
	puts "       start/endTime as \"YYYY/MM/DD hh:mm:ss\" in 24h format;"
	puts "       startTime may be 'yesterday', endTime may be 'end'"
	puts ""
	puts "- Dump values for given channels between start- and end time"
	puts "  in spreadsheed format,"
	puts "  filling/repeating values for missing time stamps"
	puts ""
	puts "Example"
	puts "-------"
	puts "spreadsheetExport.tcl dir \"2000/03/23 10:19:09.000000000\" end fred freddy"
	puts "Time                            fred            freddy"
	puts "2000/03/23 10:19:09.000000000   0.622230947018  -0.370439261198"
	puts "2000/03/23 10:19:10.460167853   0.698971152306  -0.370439261198"
	puts "2000/03/23 10:19:10.570359760   0.698971152306  -0.307258069515"
	puts "2000/03/23 10:19:12.463064310   0.781148672104  -0.307258069515"
	puts "2000/03/23 10:19:12.573179112   0.781148672104  -0.32039347291"
	puts "2000/03/23 10:19:14.465887014   0.681179642677  -0.32039347291"
	puts "2000/03/23 10:19:14.576083950   0.681179642677  -0.411299735308"
	puts "2000/03/23 10:19:16.468838785   0.586771547794  -0.411299735308"
	puts "2000/03/23 10:19:16.578913359   0.586771547794  -0.499803304672 "

	exit 1
}

#################################
# Parse args
#
set argc [ llength $argv ]
if { $argc < 4 } { usage }

set archiveName [ lindex $argv 0 ]
set startStamp [ lindex $argv 1 ]
set endStamp [ lindex $argv 2 ]

set channelNameList [ lrange $argv 3 end ]

if { [ string match "y*" $startStamp ] } {
	# calculate "yesterday, 00:00:00"
	#
	# Get "24h ago",
	# round down to midnight
	#
	set yesterday [ secs2stamp [ expr [ clock seconds ] - 24*60*60 ] ]
	stamp2values $yesterday year month day hour minute second 
	set startStamp [ values2stamp $year $month $day 0 0 0 ]
}
if { [ string match "e*" $startStamp ] } {
	set endStamp "9999/99/99 24:59:59"
}

#################################
# Helper routines to print value/status
#
# values will be a list of values, elements might be {} if no value found.
# stati is a list of status strings, mostly useful for the case that value=={}
# because the channel was disconnected (IOC down)
# or archiving was disabled
#
proc printValuesAndStatus { time values stati } {
	puts -nonewline "$time"
	foreach value $values status $stati {
		if { $value == {} } { set value "-" }
		if { $status != {} } { set status "($status)" }
		puts -nonewline "\t$value\t$status"
	}
	puts ""
}

proc printValues { time values stati } {
	puts -nonewline "$time"
	foreach value $values status $stati {
		if { $value == {} } {
			puts -nonewline "\t($status)"
		} else {
			puts -nonewline "\t$value"
		}
	}
	puts ""
}


# Header
#
puts -nonewline "Time                         "
foreach channel $channelNameList {
	puts -nonewline "\t$channel"
}
puts ""

# Main
#
set time $startStamp
set go [ openSpreadsheetContext $archiveName $time $channelNameList context values stati ]
while { $go } {
	printValues $time $values $stati
	if { $time > $endStamp } break
	set go [ nextSpreadsheetContext context time values stati ]
}

closeSpreadsheetContext context
