proc scheduleStop {i} {
  set run [camMisc::arcGet $i start]
  set now [clock seconds]
  set timespec [camMisc::arcGet $i timespec]
  switch $run {
    hourly {
      set duetime [expr ( [clock seconds] / 3600 ) * 3600 + $timespec * 60]
      if {$duetime < $now} {
	incr duetime 3600
      }
    }
    daily {
      set duetime [clock scan $timespec]
      if {$duetime < $now} {
	incr duetime 86400
      }
    }
    weekly {
      set duetime [clock scan [lindex $timespec 1]]
      set dd [lsearch $::weekdays [lindex $timespec 0]]
      set wd [clock format $now -format %w]
      incr duetime [expr ( $dd - $wd ) * 86400]
      if {$duetime < $now} {incr duetime [expr 7 * 86400]}
    }
    monthly {
      set dd [lindex $timespec 0]
      if { ( $dd > 28 ) || ( $dd < 1 ) } { return 1 }
      set td [expr 1[clock format $now -format %d] - 100]
      set tm [expr 1[clock format $now -format %m] - 100]
      set ty [clock format $now -format %Y]
      while {[set duetime [clock scan "$tm/$dd/$ty [lindex $timespec 1]"]] < $now} {
	incr tm
	if {$tm == 12} { incr ty; set tm 1 }
      }
    }
    timerange {
      set duetime [clock scan "[lindex $timespec 3] [lindex $timespec 4]"]
      if {$duetime < $now} {set duetime 0}
    }
    always {
      set duetime 0
    }
    default {
      Puts "invalid schedule-mode" error
      return
    }
  }
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
