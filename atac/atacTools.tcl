# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

package provide atacTools 1.0

########################################################
#
#//PACKAGE atacTools.tcl
#//
#// <H2>Initialization file for ATAC (A Tcl Archive Client)</H2>
#//
#// This file has to be configured for your system so that it
#// knows how to find the actual ATAC extension.
#// <P>
#// When this is done properly, atacTools.tcl...
#// <UL>
#// <LI>loads the atac extension
#//     if it was build as a shared library and not linked
#//     to the tcl shell that you are using.
#// <LI>defines some helper procs for dealing with time stamps.
#// <LI>defines some helpers for BLT graphs
#// </UL>
#//
#// <H3>Time stamps</H3>
#// Please note the two fundamentally different time formats
#// which are used in the following description:
#// <UL>
#// <LI><B>secs:</B><BR>
#//		Time in seconds since some epoch as used by the TCL <I>clock</I> command
#//        (EXCEPT: these seconds are doubles,
#//         whereas the clock command accepts only integers)
#// <LI><B>stamp:</B><BR>
#//		Time string in "YYYY/MM/DD hh:mm:ss" format
#//       as used by the archiver,
#//       with 24h hours and (maybe) fractional seconds
#//		up to the nanosecond level.
#//     <BR>
#//		This string time stamp is certainly slower to handle,
#//		but it is user-readable and does not require knowledge
#//		of a specific epoch, offsets between epochs, leap seconds etc.
#//     The order year/month/day was chosen to have an ASCII-sortable
#//     date.
#// </UL>
#// The preferred format should be "stamp" because it's user-readable
#// and can as well be used in time comparisons.
#// The "secs" format is useful for calculations based on <I>differences</I> in time,
#// but no assumption should be made as far as the absolute epoch is concerned.

# Configure this to load the atac commands
# on your machine
#
if { $tcl_platform(platform) == "windows" } {
	load Debug/atac.dll
} elseif { $tcl_platform(platform) == "unix" } {
	if { $tcl_platform(os) == "Linux" } {
		load /home/kasemir/Epics/extensions/src/ChannelArchiver/atac/O.Linux/atac.so
	}
}

# Solaris (LEDA@LANL)
# - nothing to load as long as shared lib. cannot be build
# The current tcl-shell was hopefully build with atac hardlinked in.

###################################################
#//* <B>Syntax:</B> stampNow
#//
#// <B>Returns:</B> stamp string
#//
#// Get current time as stamp
#//

proc stampNow { } {
	secs2stamp [ clock seconds ]
}

###################################################
#//* <B>Syntax:</B> secs2stamp secs
#//
#// <B>Returns:</B> stamp string
#//
#// Convert time in seconds into stamp
#//

proc secs2stamp { secs } {
	set fullsecs [ expr int($secs) ]
	set fraction [ expr $secs - $fullsecs ]
	set full [ clock format $fullsecs -format "%Y/%m/%d %H:%M:%S" ]
	if { $fraction > 0 } {
		return [ format "%s.%09d" $full [ expr int ($fraction * 1000000000) ] ]
	}
	return $full
}

###################################################
#//* <B>Syntax:</B> stamp2secs stamp
#//
#// <B>Returns:</B> secs as double
#//
#// Convert stamp into seconds
#//

proc stamp2secs { stamp } {
	stamp2values $stamp year month day hour minute second
	set fullsec [ expr int($second) ]
	set fraction [ expr $second - $fullsec]
	set fullsec [ clock scan "$month/$day/$year $hour:$minute:$fullsec" ]
	expr $fullsec + $fraction
}

###################################################
#//* <B>Syntax:</B> stamp2values stamp year month day hour minute second
#//
#// Extract numeric values for year, month, ... from stamp
#// and set remaining arguments accordingly.
#//

proc stamp2values { stamp year month day hour minute second } {
	upvar $year Y
	upvar $month M
	upvar $day D
	upvar $hour h
	upvar $minute m
	upvar $second s

	set date_time [ split $stamp " " ]
	set date [ split [ lindex $date_time 0 ] "/" ]
	set time [ split [ lindex $date_time 1 ] ":" ]
	set Y [ lindex $date 0 ]
	set M [ lindex $date 1 ]
	set D [ lindex $date 2 ]
	set h [ lindex $time 0 ]
	set m [ lindex $time 1 ]
	set s [ lindex $time 2 ]
}

###################################################
#//* <B>Syntax:</B> values2stamp year month day hour minute second
#//
#// Returns stamp generated from numeric values for year, month, ...
#//

proc values2stamp { year month day hour minute second } {
	if {$year < 1900} { error "Invalid year: $year" }

	# Month like "08" would be invalid if evaluated as octal numbers.
	# This string->double->int conversion is expensive
	# but I don't know a better way, yet:
	set month  [ expr int($month.) ]
	set day    [ expr int($day.) ]
	set hour   [ expr int($hour.) ]
	set minute [ expr int($minute.) ]
	set second [ expr int($second.) ]
	
	format "%04d/%02d/%02d %02d:%02d:%02d" \
            $year $month $day $hour $minute $second

}

###################################################
#//* <B>Syntax:</B> formatValue valueId
#//
#// Returns string with valueId's value as time, number, status
#//

proc formatValue { valueId } {
	if { [ value valid $valueId ] } {
		if { [ value isInfo $valueId ] } {
			return "[ value time $valueId ] [ value status $valueId ]"
		}
		return "[ value time $valueId ] [ value text $valueId ] [ value status $valueId ]"
	}

	"<invalid value>"
}

###################################################
#//* <B>Syntax:</B> showAtacNag
#//
#// Display "Nag" panel with version info etc.
#//

proc showAtacNag {} {
	global atac_version
	global tcl_version
	global blt_version
	global tcl_platform

	toplevel .nag
	label .nag.info0 -text "ATAC Version $atac_version"
	label .nag.info1 -text "TCL Version $tcl_version"
	if { [ info exists blt_version ] } {
		label .nag.info2 -text "BLT Version $blt_version"
	} else {
		label .nag.info2 -text "BLT Version -- not loaded --"
	}
	label .nag.info3 -text "Platform: $tcl_platform(platform)"
	button .nag.close -text "Exit" -command "destroy .nag"
	grid .nag.info0 -row 0
	grid .nag.info1 -row 1
	grid .nag.info2 -row 2
	grid .nag.info3 -row 3
	grid .nag.close -row 4 -sticky e
	after 10000 { destroy .nag }
}              

###################################################
#//* <B>Syntax:</B> createChannelInfo archiveName channelName
#//
#// Create a new toplevel ".channelInfo"
#// that displays information about that channel:
#// <UL>
#// <LI>Name of channel
#// <LI>Time when channel was first archived
#// <LI>Time when channel was last archived
#// </UL>
#//

proc createChannelInfo { archiveName channelName } {
	# Delete pervious panel - if exists
	catch { destroy .channelInfo }
	toplevel .channelInfo
	wm title .channelInfo "Channel Info"

	label .channelInfo.channelL -text "Channel:"
	entry .channelInfo.channelE -width 30
	.channelInfo.channelE insert 0 $channelName

	set archiveId [ archive open $archiveName ]
	set channelId [ archive findChannelByName $archiveId $channelName ]
	if { ! [ channel valid $channelId ] } {
		set first "- not in archive -"
		set last  ""
	} else {
		set first [ channel getFirstTime $channelId ]
		set last [ channel getLastTime $channelId ]
	}

	label .channelInfo.firstL -text "First Value:"
	entry .channelInfo.firstE -width 30
	.channelInfo.firstE insert 0 $first
	label .channelInfo.lastL  -text "Last  Value:"
	entry .channelInfo.lastE -width 30
	.channelInfo.lastE insert 0 $last

	button .channelInfo.close -command "destroy .channelInfo" -text "Exit"

	grid .channelInfo.channelL -row 0 -column 0
	grid .channelInfo.channelE -row 0 -column 1
	grid .channelInfo.firstL   -row 1 -column 0
	grid .channelInfo.firstE   -row 1 -column 1
	grid .channelInfo.lastL    -row 2 -column 0
	grid .channelInfo.lastE    -row 2 -column 1
	grid .channelInfo.close    -row 3 -column 1 -sticky e
}

###################################################
#//* <B>Syntax:</B> openSpreadsheetContext archiveName startStamp channelNameList contextVar valuesVar statiVar
#//
#// <B>Returns:</B> true if values were found
#//
#// Open archive, find channels and get values/stati for given startStamp.
#// <BR>
#// Fills context variable for subsequent calls to nextSpeadsheetContext.
#// <BR>
#// Will report error if archive cannot be opened or channels don't exist at all.
#//

proc openSpreadsheetContext { archiveName startStamp channelNameList contextVar valuesVar statiVar } {
	upvar $contextVar context
	upvar $valuesVar values
	upvar $statiVar stati

	set values {}
	# open archive
	set archiveId [ archive open $archiveName ]

	foreach channelName $channelNameList {
		# find channel
		set channelId [ archive findChannelByName $archiveId $channelName ]
		if { ! [ channel valid $channelId ] } {
			error "'$channelName': Cannot find this channel!"
		}
		lappend channelIds $channelId

		# find valid value after startStamp,
		# i.e. the value put into archive _before_ startStamp
		# that is still valid at that point in time
		set value {}
		set status {}
		if { [ channel getLastTime $channelId ] < $startStamp } {
			return false
		} else {
			set valueId [ channel getValueBeforeTime $channelId $startStamp ]
			if { [ value valid $valueId ] } {
				if { ! [ value isInfo $valueId ] } {
					set value [ value get $valueId ]
				}
				set status [ value status $valueId ]
				value next $valueId
			}
		}

		lappend valueIds $valueId
		lappend values $value
		lappend stati $status
	}

	set context [ list $archiveId $channelIds $valueIds $values $stati ]

	return true
}

#####################################################
#//* <B>Syntax:</B> nextSpeadsheetContext contextVar timeStampVar valuesVar statiVar
#//
#// <B>Returns:</B> true if values were found
#//
#// Find next set of values/stati, set timeStamp to were that set was found.
#//

proc nextSpeadsheetContext { contextVar timeStampVar valuesVar statiVar } {
	upvar $contextVar context
	upvar $timeStampVar timeStamp
	upvar $valuesVar values
	upvar $statiVar stati

	set archiveId  [ lindex $context 0 ]
	set channelIds [ lindex $context 1 ]
	set valueIds   [ lindex $context 2 ]
	set oldValues  [ lindex $context 3 ]
	set oldStati   [ lindex $context 4 ]

	# Find oldest timestamp amongst channels
	set timeStamp "9999/99/99 99:99:99"
	foreach valueId $valueIds {
		if { [ value valid $valueId ] } {
			set t [ value time $valueId ]
			if { $t < $timeStamp } { set timeStamp $t }
		}
	}
	# Any valid value at all?
	if { [ string equal $timeStamp "9999/99/99 99:99:99" ] } {
		return false
	}

	set values {}
	set stati  {}
	foreach valueId $valueIds value $oldValues status $oldStati {
		# have a new value for that timeStamp?
		if { [ value valid $valueId ] } {
			if { [ value time $valueId ] <= $timeStamp } {
				if { [ value isInfo $valueId ] } {
					set value {}
				} else {
					set value [ value get $valueId    ]
				}
				set status [ value status $valueId ]
				# Get next value
				value next $valueId
			}
		}
		lappend values $value
		lappend stati  $status
	}
	set context [ list $archiveId $channelIds $valueIds $values $stati ]

	return true
}

proc closeSpeadsheetContext { contextVar } {
	upvar $contextVar context

	set archiveId  [ lindex $context 0 ]
	set channelIds [ lindex $context 1 ]
	set valueIds   [ lindex $context 2 ]

	foreach valueId $valueIds {
		value close $valueId
	}
	foreach channelId $channelIds {
		channel close $channelId
	}
	archive close $archiveId
}
