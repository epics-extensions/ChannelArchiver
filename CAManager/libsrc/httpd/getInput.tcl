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
