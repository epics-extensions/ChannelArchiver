proc checkBgManager {fd h} {
  gets $fd line
  if [regexp "Server: Channel Archiver bgManager (.*)@(.*):(\[0-9\]*)" $line \
	  all user host port] {
    set ::reply($h) "$user@$host:$port"
  }
  if {"$line" == "</html>"} {
    close $fd
    incr ::open -1
    if {$::open == 0} {set ::pipi 1}
  }
}

proc cfbgmTimeout {fd h} {
  catch {close $fd}
}

proc checkForBgManager {} {
  global tcl_platform
  if {$::checkBgMan == 0} return
  foreach arc [camMisc::arcIdx] {
    lappend hosts [camMisc::arcGet $arc host]
  }
  set ::open 0
  foreach h [lrmdups $hosts] {
    if [catch {set sock [socket $h $::_port]}] {
      continue
    }
    set ::reply($h) {}
    incr ::open
    puts $sock "GET / HTTP/1.1\n"
    flush $sock
    fconfigure $sock -blocking 1
    fileevent $sock readable "checkBgManager $sock $h"
    after 3000 "cfbgmTimeout $sock $h"
  }
  if {$::open > 0} { vwait ::pipi }
  foreach h [lrmdups $hosts] {
    set act none
    if {![info exists ::reply($h)]} {
      set msg "No CAbgManager running for $tcl_platform(user)@$h:$::_port!"
      set act start
    } elseif {[llength $::reply($h)] == 0} {
      set msg "CAbgManager $tcl_platform(user)@$h:$::_port didn't reply!"
      set act config
    } elseif {"$::reply($h)" != "$tcl_platform(user)@$h:$::_port"} {
      set msg "CAbgManager $tcl_platform(user)@$h:$::_port answered $::reply($h)!"
      set act config
    } else {
      set msg "CAbgManager $tcl_platform(user)@$h:$::_port OK!"
    }
    switch $act {
      start {
	if {[tk_dialog .w "CAbgManager not running" "$msg!" warning 0 "Continue without" "Start"] == 1} {
	  if {[regexp "Windows" $tcl_platform(os)]} {
	    set tclext .tcl
	  } else {
	    set tclext ""
	  }
	  exec CAbgManager$tclext &
	}
      }
      config {
	tk_dialog .w "CAbgManager: invalid response" "$msg\nPlease reconfigure!" warning 0 "Ok"
      }
    }
  }
}
