proc checkstate {arr ind op} {
  if {"$::_run($ind)" == "NO"} {
    trace vdelete ::_run($ind) w checkstate
    if {("[camMisc::arcGet $ind start]" == "NO")} {return}
    if {("[camMisc::arcGet $ind start]" != "timerange")} {
      set ::sched($ind,start,job) [after 1 "runArchiver $ind"]
    } else {
      set timespec [camMisc::arcGet $ind timespec]
      set Starttime [clock scan "[lindex $timespec 0] [lindex $timespec 1]"]
      set starttime [expr ( $Starttime - [clock seconds] ) * 1000]
      set stoptime [clock scan "[lindex $timespec 3] [lindex $timespec 4]"]
      set stoptime [expr ( $stoptime - [clock seconds] ) * 1000]
      if {$stoptime <= 0} return
      if {$starttime < 0} {set starttime 0}
      if {[info exists ::sched($ind,start,time)] && ($::sched($ind,start,time) == $Starttime)} return
      if {$starttime != 0} {
#	Puts "starting \"[camMisc::arcGet $ind descr]\" in [expr $starttime / 1000]s" schedule
	Puts "starting \"[camMisc::arcGet $ind descr]\" @ [httpd::time $Starttime]" schedule
      }
      if {$starttime >= 0} {
	set ::sched($ind,start,time) $Starttime
	set ::sched($ind,start,job) [after $starttime "runArchiver $ind"]
      }
    }
  } elseif [regexp "^(BLOCKED|since)" $::_run($ind)] {
    trace vdelete ::_run($ind) w checkstate
  }
}
