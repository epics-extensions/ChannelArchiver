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

package require http

proc camComm::CheckRunning {i rvar} {
  regexp "\\((\[^,\]*)\[,\\)\]" $rvar all row
  if {![info exists ::lastArc($row)]} {set ::lastArc($row) ""}
  set start [clock seconds]
#  set $rvar "???"
  if {[catch {::http::geturl "http://[camMisc::arcGet $i host]:[camMisc::arcGet $i port]/"} token]} {
    condSet $rvar "NO"
  } else {
    set ::fsvar($token) $rvar
    set ::fsto($token) [after 5000 "camComm::processTimeout $token; update"]
    upvar #0 $token state
    lappend ::socks $state(sock)
    array set meta $state(meta)

    while {$state(status) != "ok"} {
      update
      after 100
    }

    if [info exists state(error)] {
      condSet $::fsvar($token) "NO"
    }

    if {![regexp "ArchiveEngine" $meta(Server)]} {
      condSet ::lastArc($row) "unknown Server"
      camComm::Close $token
    }
    foreach line [split $state(body) "\n"] {
      if {[regexp ".*Started.*>(\[^<\]+)<" $line all started]} {
	condSet $::fsvar($token) "since [string range $started 0 18]"
      } elseif {[regexp ".*Archive.*>(\[^<\]+)<" $line all archive]} {
	condSet ::lastArc($row) "$archive"
      }
    }
    camComm::Close $token
  }
}

proc camComm::Close {token} {
  upvar #0 $token state
  catch {after cancel $::fsto($token)}
  if {[set i [lsearch -exact $::socks $state(sock)]] >= 0} { set ::socks [lreplace $::socks $i $i] }
  ::http::cleanup $token
}

proc camComm::condSet {var val} {
  if {![info exists $var] || ([set $var] != $val)} {
    uplevel 1 "set $var \"$val\""
  }
}

proc camComm::processTimeout {token} {
  condSet $::fsvar($token) "TIMEOUT"
  camComm::Close $token
  update
}
