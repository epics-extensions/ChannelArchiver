proc scheduleStop {i} {
  set now [clock seconds]
  set duetime [lindex [duetime $i] 1]
  if {$duetime < 0} return

  camMisc::arcSet $i duetime $duetime
  if {$duetime > 0} {
    if {[info exists ::sched($i,stop,time)] && ($::sched($i,stop,time) == $duetime)} return
    catch {after cancel $::sched($i,stop,job)}
#    Puts "stopping \"[camMisc::arcGet $i descr]\" @ [expr ( $duetime - $now )]s" schedule
    Puts "stopping \"[camMisc::arcGet $i descr]\" @ [httpd::time $duetime]" schedule
    set ::sched($i,stop,job) [after [expr ( $duetime - $now ) * 1000] "stopArchiver $i"]
    set ::sched($i,stop,time) $duetime
  }
}
