# This is to get the CVS-revision-code into the source...
set Revision ""
set Date ""
set Author ""
set CVS(Revision,Comm) "$Revision$"
set CVS(Date,Comm) "$Date$"
set CVS(Author,Comm) "$Author$"

regsub ": (.*) \\$" $CVS(Revision,Comm) "\\1" CVS(Revision,Comm)
regsub ": (.*) \\$" $CVS(Date,Comm) "\\1" CVS(Date,Comm)
regsub ": (.*) \\$" $CVS(Author,Comm) "\\1" CVS(Author,Comm)

namespace eval camComm {
}

proc camComm::CheckRunning {i rvar} {
  global fstate fsvar fsto

  regexp "\\((\[^,\]*)\[,\\)\]" $rvar all row
  if {![info exists ::lastArc($row)]} {set ::lastArc($row) ""}

  set start [clock seconds]
  if [catch {set sock [socket [camMisc::arcGet $i host] [camMisc::arcGet $i port]]}] {
    if {[file exists [file join [file dirname [camMisc::arcGet $i cfg]] BLOCKED]]} {
      condSet $rvar "BLOCKED"
    } else {
      if {[info exists ::tk_version]} {
	condSet $rvar "NO"
      } else {
	set $rvar "NO"
      }
    }
    return
  }
  lappend ::Socks $sock

  puts $sock "GET / HTTP/1.0"
  puts $sock ""
  flush $sock

  set fsvar($sock) $rvar
  set fsvar($sock,arc) ::lastArc($row)
  set fstate($sock) open
  set ::bytes($sock) ""
  fileevent $sock readable "camComm::processAnswer $sock; update"
  set fsto($sock) [after 5000 "camComm::processTimeout $sock; update"]
}

proc camComm::Close {sock} {
  global fstate fsto
  catch {after cancel $fsto($sock)}
  set fstate($sock) closed
  close $sock
  if {[set i [lsearch -exact $::socks $sock]] >= 0} { set ::socks [lreplace $::socks $i $i] }
}

proc camComm::condSet {var val} {
  if {![info exists $var] || ([set $var] != $val)} {
    uplevel 1 "set $var \"$val\""
  }
}

set ::inAnswer 0
proc camComm::processAnswer {sock} {
  if {$::inAnswer} return
  set ::inAnswer 1
  global fstate fsvar
  append ::bytes($sock) [read $sock]
  set bl [string bytelength $::bytes($sock)]
  if {[eof $sock]} { append ::bytes($sock) "\n" }
  set nl [string first "\n" $::bytes($sock)]

  # no full line yet...
  if {$nl < 0} return

  set line [string range $::bytes($sock) 0 [expr $nl - 1]]
  set ::bytes($sock) [string replace $::bytes($sock) 0 $nl ""]

  switch $fstate($sock) {
    open {
      if {![regexp "^HTTP/.* 200 OK" $line]} {
	condSet $fsvar($sock) "invalid response"
	camComm::Close $sock
      } else {
	set fstate($sock) http
      }
    }
    http {
      if {[regexp "^Server: (.*)" $line all server]} {
	if {"$server" != "ArchiveEngine"} {
	  condSet $fsvar($sock) "unknown Server"
	  camComm::Close $sock
	} else {
	  set fstate($sock) server
	}
      }
    }
    server {
      if {"$line" == ""} {
	set fstate($sock) body
      }
    }
    body {
      if {[regexp ".*Started.*>(\[^<\]+)<" $line all started]} {
	condSet $fsvar($sock) "since [string range $started 0 18]"
	set fstate($sock) started
      }
    }
    started {
      if {[regexp ".*Archive.*>(\[^<\]+)<" $line all archive]} {
	condSet $fsvar($sock,arc) "$archive"
	set fstate($sock) end
      }
    }
    end {
      if {[eof $sock]} {
	camComm::Close $sock
      }
    }
    closed {
    }
  }
  set ::inAnswer 0
}

proc camComm::processTimeout {sock} {
  global fstate fsvar fsto
  if {"$fstate($sock)" != "closed"} {
    condSet $fsvar($sock) "TIMEOUT"
    set fstate($sock) closed
    camComm::Close $sock
  }
  update
}
