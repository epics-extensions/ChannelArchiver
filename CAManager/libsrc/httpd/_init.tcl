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

source Busy.tcl
source connect.tcl
source getInput.tcl
source sendCmdResponse.tcl
source sendError.tcl
source sendOutput.tcl
