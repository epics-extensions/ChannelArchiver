proc camComm::processAnswer {sock} {
  global fstate fsvar
  gets $sock line
  switch $fstate($sock) {
    open {
      if {![regexp "^HTTP/.* 200 OK" $line]} {
	condSet $fsvar($sock) "invalid response"
#	set fstate($sock) closed
	camComm::Close $sock
      } else {
	set fstate($sock) http
      }
    }
    http {
      if {[regexp "^Server: (.*)" $line all server]} {
	if {"$server" != "ArchiveEngine"} {
	  condSet $fsvar($sock) "unknown Server"
#	  set fstate($sock) closed
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
#	set fstate($sock) closed
	camComm::Close $sock
      }
    }
    closed {
    }
  }
}
