proc scheduleStop {i} {
  set now [clock seconds]
  lassign [duetime $i] starttime stoptime
  if {$starttime > $now} return
  if {$stoptime < $now} return

  camMisc::arcSet $i stoptime $stoptime
  if {[info exists ::sched($i,stop,time)] && ($::sched($i,stop,time) == $stoptime)} return
  if {[info exists ::sched($i,stop,job)]} {after cancel $::sched($i,stop,job)}
  Puts "stopping \"[camMisc::arcGet $i descr]\" @ [httpd::time $stoptime]" schedule
  set ::sched($i,stop,job) [after [expr ( $stoptime - $now ) * 1000] "stopArchiver $i"]
  set ::sched($i,stop,time) $stoptime
}
