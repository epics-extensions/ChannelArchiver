proc httpd::sendOutput {fd} {
  global tcl_platform
  variable _starttime
  variable _timefmt
  variable _proto
  variable _query
#  puts stderr [join $_query($fd) "\n"]
   if {$_proto($fd) != ""} {
      puts $fd "$_proto($fd) 200 OK"
      puts $fd "Server: Channel Archiver bgManager $tcl_platform(user)@$::_host:$::_port"
      puts $fd "Content-type: text/html"
      puts $fd ""
   }
  puts $fd "<html><head><title>Channel Archiver - bgManager ($tcl_platform(user)@$::_host:$::_port)</title>"
  puts $fd "<meta http-equiv=refresh content=10>"
  puts $fd "</head>"
  puts $fd "<body bgcolor=\"\#aec9d2\">"
  puts $fd "<TABLE BORDER=3>"
  puts $fd "<TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5>"
  puts $fd "Channel Archiver - bgManager"
  puts $fd "</FONT></TD></TR>"
  puts $fd "</TABLE>"

  puts $fd "<p>is running since [time $_starttime]</p>"
  puts $fd "<table border=0 cellpadding=5>"
  puts $fd "<tr><th colspan=5 bgcolor=black><font color=white>Configured ArchiveEngines for user <font color=yellow><em>$tcl_platform(user)</em></font> on host <font color=yellow><em>$::_host</em></font></font></th></tr>"
  puts $fd "<tr><th bgcolor=white>ArchiveEngine</th><th bgcolor=white>Port</th><th bgcolor=white>running?</th><th bgcolor=white>run/rerun</th><th bgcolor=white>command</th></tr>"
  foreach i [camMisc::arcIdx] {
    if {"[camMisc::arcGet $i host]" != "$::_host"} continue
    set run $::_run($i)
    set r $run
    set p [camMisc::arcGet $i port]
    set A [set E ""]
    if {"$r" != "NO"} {
      set A "<a href=\"http://[info hostname]:$p/\" target=\"_blank\">"
      set E "</a>"
    }
    puts $fd "<tr>"
    puts $fd "<td>$A[camMisc::arcGet $i descr]$E</td>"
    puts $fd "<td align=center>$A$p$E</td>"

    set start [camMisc::arcGet $i start]
    set ts [camMisc::arcGet $i timespec]
    regsub "NO" $r "<font color=red>$r</font>" r
    if {("$start" != "NO") && 
	(("$start" != "timerange") || 
	 (([clock seconds] >= [clock scan "[lindex $ts 0] [lindex $ts 1]"]) && 
	  ([clock seconds] <= [clock scan "[lindex $ts 3] [lindex $ts 4]"]) ) )} {
      regsub "NO" $r "<b>$r</b>" r
    }
    puts $fd "<td align=center>$r</td>"
    puts $fd "<td align=center>"
    switch -regexp $start {
      always|NO {
	puts -nonewline $fd "$start"
      }
      timerange {
	regsub -- "- [lindex $ts 0]" $ts "-" ts
	puts -nonewline $fd "$ts"
      }
      weekly {
	puts -nonewline $fd "[lindex $ts 0]s @ [lindex $ts 1]"
      }
      monthly {
	puts -nonewline $fd "$start: [lindex $ts 0]. @ [lindex $ts 1]"
      }
      hourly {
	puts -nonewline $fd "$start"
	if {$ts != 0} { puts -nonewline $fd " @ $ts. minute" }
      }
      default {
	puts -nonewline $fd "$start: @ $ts"
      }
    }
    puts $fd "</td>"
    puts $fd "<td>"
    puts $fd "<table cellpadding=2 border=1 width=\"150\"><tr>"
    if {[camMisc::arcGet $i busy] != ""} {
      puts $fd "<td bgcolor=orange align=center>[camMisc::arcGet $i busy]</td>"
    } else {    
      if {$run != "NO"} {
	if 1 {
	# is running
	puts $fd "<td bgcolor=red align=center>"
	puts $fd "<a href=\"/stop?ind=$i\"><b>STOP</b></a>"
	puts $fd "</td>"
        } else {
	  puts $fd "<td><form method=GET action=\"/stop\">"
          puts $fd "<input type=HIDDEN NAME=ind value=$i>"
	  puts $fd "<input type=submit value=STOP>"
	  puts $fd "</form></td>"
	}
      } else {
	# is NOT running
	puts $fd "<td bgcolor=green align=center>"
	puts $fd "<a href=\"/start?ind=$i\"><font color=black underline=0><b>START</b></font></a>"
	puts $fd "</td>"
      }
    }
    puts $fd "</tr></table>"
    puts $fd "</td>"
    puts $fd "</tr>"
  }
  puts $fd "<tr><td>&nbsp;</td></tr>"
  puts $fd "<tr><th colspan=5 bgcolor=black><font color=white>"
  puts $fd "Messages (most recent first)</font>"
  puts $fd "</font></th></tr>"
  puts $fd "<tr><td colspan=5 align=center>"
  puts $fd "<font color=red>Errors/Warnings</font> / <font color=blue>Starts/Stops</font> / <font color=green>Scheduled jobs</font> / Misc."
  puts $fd "</td></tr></table>"
  foreach m $::_msg {
    puts $fd "$m<br>"
  }
  puts $fd "<hr>"
  puts $fd "<em>This page updates every 10 seconds.<br>"
  puts $fd "If not, use the reload button of your browser.</em><br>"
  puts $fd "<font color=blue>Last update: [time]</font>"
  puts $fd "</body>"
  puts $fd "</html>"
  close $fd
}
