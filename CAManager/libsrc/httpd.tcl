namespace eval httpd {
  variable ::_port 4610
  variable _starttime
  variable _proto
  variable _page
  variable _query
  variable _method
  variable _gotargs
  variable _timefmt "%Y/%m/%d %H:%M:%S"
#  catch {source $camMisc::rcdir/settings}
}

proc httpd::init {} {
  variable _starttime [clock seconds]
  if [catch {socket -server httpd::connect $::_port}] {
    puts "Couldn't use port $::_port, port is already in use or privileged!"
    exit
  } else {
    puts "Info on http://[info hostname]:$::_port/"
  }
}

proc httpd::time {{time 0}} {
  variable _timefmt
  if {$time == 0} {set time [clock seconds]}
  return [clock format $time -format $_timefmt]
}

proc httpd::Busy {ind msg} {
  camMisc::arcSet $ind busy $msg
  after 5000 "camMisc::arcSet $ind busy \"\""
}

proc httpd::connect {fd addr port} {
  variable _query
  fconfigure $fd -blocking 0
  set _query($fd) {}
  fileevent $fd readable "httpd::getInput $fd"
}

proc httpd::getInput {fd} {
  variable _query
  variable _proto
  variable _page
  variable _method
  variable _gotargs

  gets $fd lastline
  lappend _query($fd) "$lastline"

  set lQ [llength $_query($fd)]
  if {$lQ == 1} {
    set _proto($fd) ""
    if {![regexp "^(GET|POOP) (/\[^ 	\]*)(.*)" [lindex $_query($fd) 0] all _method _page($fd) _proto($fd)]} {
      fileevent $fd readable {}
      sendError $fd "invalid request"
      return
    }
    set _proto($fd) [string trim $_proto($fd)]
    set _gotargs 0
  }

  if {($lQ > 2) && ([lindex $_query($fd) [expr [llength $_query($fd)] - 2]] == "")} {
    set $_query($fd) "$_query($fd)?$lastline"
    set _gotargs 1
  }

  if {(($_method == "GET") && 
       (($_proto($fd) == "") || (($lQ >= 2) && ("$lastline" == "")))) ||
      (($_method == "POST") && $_gotargs)} {
    if {"$_page($fd)" == "/exit"} {
      puts stderr "terminating on user-request."
      puts $fd "<html><head><title>CAbgManager exit</title></head>"
      puts $fd "<body bgcolor=\"\#aec9d2\">"
      puts $fd "<TABLE BORDER=3>"
      puts $fd "<TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5>"
      puts $fd "Channel Archiver - bgManager terminated!"
      puts $fd "</FONT></TD></TR>"
      puts $fd "</TABLE>"
      puts $fd "</body></html>"
      close $fd
      exit
    }
    if [regexp "(.*)\\?(.*)" $_page($fd) all page args] {
#      puts stderr [join $_query($fd) "\n"]
      foreach arg [split $args "&"] {
	if [regexp "(.*)=(.*)" $arg all name val] {
	  set var($name) $val
	}
      }
      if {![info exists var(ind)]} {
	sendError $fd "index for command missing"
	return
      }
      switch $page {
	"/start" {
	  Puts "manually starting \"[camMisc::arcGet $var(ind) descr]\""
	  Busy $var(ind) "start in progress"
	  camMisc::Release $var(ind)
	  runArchiver $var(ind)
	}
	"/stop" {
	  Puts "manually stopping \"[camMisc::arcGet $var(ind) descr]\""
	  Busy $var(ind) "stop in progress"
	  stopArchiver $var(ind)
	  camMisc::Block $var(ind)
	}
	default {
	  sendError $fd "illegal command \"$page\""
	  return
	}
      }
      sendCmdResponse $fd $page $var(ind)
    } else {
      sendOutput $fd
    }
  }
  array unset var ind
}

proc httpd::ed {p} {
  switch $p {
    stop {return stopped}
    start {return started}
  }
}

proc httpd::sendCmdResponse {fd page ind} {
  puts $fd "<HTML><HEAD>
<meta http-equiv=refresh content=\"0; URL=http://[info hostname]:$::_port/\">
<TITLE>ArchiveEngine \"[camMisc::arcGet $ind descr]\" ${page}ed</TITLE>
</HEAD><BODY bgcolor=\"\#aec9d2\">
<TABLE BORDER=3><TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5><em>[camMisc::arcGet $ind descr]</em> [ed ${page}]</FONT></TD></TR></TABLE>
<p>You'll be taken back to the main-page automatically in a few seconds...
</BODY></HTML>
"
  close $fd
}

proc httpd::sendError {fd msg} {
  variable _proto
  variable _query
  puts $fd "<HTML><HEAD>
<TITLE>$msg</TITLE>
</HEAD><BODY bgcolor=\"\#aec9d2\">
<TABLE BORDER=3><TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5>$msg</FONT></TD></TR></TABLE>
<PRE>
$_query($fd)
</PRE>
</BODY></HTML>"
  close $fd
}

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
  puts $fd "<meta http-equiv=refresh content=$::bgUpdateInt>"
  puts $fd "</head>"
  puts $fd "<body bgcolor=\"\#aec9d2\">"
  puts $fd "<TABLE BORDER=3>"
  puts $fd "<TR><TD BGCOLOR=\"#FFFFFF\"><FONT SIZE=5>"
  puts $fd "Channel Archiver - bgManager"
  puts $fd "</FONT></TD></TR>"
  puts $fd "</TABLE>"

  if {$::debug} {
    puts $fd "<p>is running since [time $_starttime] (currently $::openSocks open sockets)</p>"
  }
  puts $fd "<table border=0 cellpadding=5>"
  puts $fd "<tr><th colspan=5 bgcolor=black><font color=white>Configured ArchiveEngines for config <font color=yellow><em>[file tail $camMisc::cfg_file]</em></font> of user <font color=yellow><em>$tcl_platform(user)</em></font> on host <font color=yellow><em>$::_host</em></font></font></th></tr>"
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
      month {
	puts -nonewline $fd "every [lindex $ts 0]. of a month @ [lindex $ts 1]"
      }
      week {
	lassign $ts day time ev
	if {$ev > 1} {
	  puts -nonewline $fd "every $ev weeks, "
	}
	puts -nonewline $fd "${day}s @ $time"
      }
      day {
	lassign $ts time ev
	if {$ev > 1} {
	  puts -nonewline $fd "every $ev days @ $time"
	} else {
	  puts -nonewline $fd "daily @ $time"
	}
      }
      hour {
	lassign $ts time h
	if {$h > 1} {
	  puts -nonewline $fd "@ $time + every $h hours" 
	} else {
	  puts -nonewline $fd "@ *[string range $time 2 end]" 
	}
      }
      minute {
	lassign $ts m ev
	if {$ev > 1} {
	  puts -nonewline $fd "@ $m. min. + every $ev minutes"
	} else {
	  puts -nonewline $fd "every minute"
	}
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
	puts $fd "<a href=\"/start?ind=$i\"><font color=black><b>START</b></font></a>"
	puts $fd "</td>"
      }
    }
    puts $fd "</tr></table>"
    puts $fd "</td>"
    puts $fd "</tr>"
  }
  puts $fd "<tr><td>&nbsp;</td></tr>"
  puts $fd "<tr><th colspan=5 bgcolor=black><font color=white>"
  puts $fd "Messages (most recent first)"
  puts $fd "</font></th></tr>"
  puts $fd "<tr><td colspan=5 align=center>"
  puts $fd "<font color=red>Errors/Warnings</font> / <font color=blue>Starts/Stops</font> / <font color=green>Scheduled jobs</font> / Misc."
  puts $fd "</td></tr></table>"
  foreach m $::_msg {
    puts $fd "$m<br>"
  }
  puts $fd "<hr>"
  puts $fd "<em>This page updates every $::bgUpdateInt seconds.<br>"
  puts $fd "If not, use the reload button of your browser.</em><br>"
  puts $fd "<font color=blue>Last update: [time]</font>"
  puts $fd "</body>"
  puts $fd "</html>"
  close $fd
}
