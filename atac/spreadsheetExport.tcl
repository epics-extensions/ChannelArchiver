
source atacTools.tcl

# Helper routines to print value/status
#
# values will be a list of values, elements might be {} if no value found.
# stati is a list of status strings, mostly useful for the case that value=={}
# because the channel was disconnected (IOC down)
# or archiving was disabled
#
proc printValuesAndStatus { time values stati } {
	puts -nonewline "$time : "
	foreach value $values status $stati {
		if { $value == {} } { set value "-" }
		if { $status != {} } { set status "($status)" }
		puts -nonewline "\t$value\t$status"
	}
	puts ""
}

proc printValues { time values stati } {
	puts -nonewline "$time : "
	foreach value $values status $stati {
		if { $value == {} } {
			puts -nonewline "\t($status)"
		} else {
			puts -nonewline "\t$value"
		}
	}
	puts ""
}


# Archive, channels to use
#set archiveName "/mnt/cdrom/dir"
set archiveName "/archives/aptdvl_archives/master_arch/chan_arch/freq_directory"
set channelNameList [ list "is_hvps_actv" "is_hvps_acti" "RFQ_BMDCTor_D_Avg" ]

# Example: calculate "yesterday, 00:00:00"
#
# Get "24h ago",
# round down to midnight
#
set yesterday [ secs2stamp [ expr [ clock seconds ] - 24*60*60 ] ]
stamp2values $yesterday year month day hour minute second 
set time [ values2stamp $year $month $day 0 0 0 ]

# Main
#
set go [ openSpreadsheetContext $archiveName $time $channelNameList context values stati ]
while { $go } {
	printValues $time $values $stati
	set go [ nextSpeadsheetContext context time values stati ]
}
