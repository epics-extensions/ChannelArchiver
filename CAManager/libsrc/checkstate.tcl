proc checkstate {arr ind op} {
  if {"$::_run($ind)" == "NO"} {
    trace vdelete ::_run($ind) w checkstate
    if {("[camMisc::arcGet $ind start]" == "NO")} {return}
    if {("[camMisc::arcGet $ind start]" != "timerange")} {
      catch {after cancel $::sched($ind,start,job)}
      set ::sched($ind,start,job) [after 100 "runArchiver $ind"]
    } else {
      # timerange
      lassign [duetime $ind] Starttime Stoptime
      set starttime [expr ( $Starttime - [clock seconds] ) * 1000]
      set stoptime [expr ( $Stoptime - [clock seconds] ) * 1000]
      if {$stoptime <= 0} return
      if {$starttime < 0} {set starttime 0}
      if {[info exists ::sched($ind,start,time)] && ($::sched($ind,start,time) == $Starttime)} return
      Puts "starting \"[camMisc::arcGet $ind descr]\" @ [httpd::time $Starttime]" schedule
      set ::sched($ind,start,time) $Starttime
      set ::sched($ind,start,job) [after $starttime "runArchiver $ind"]
    }
  } elseif [regexp "^(BLOCKED|since)" $::_run($ind)] {
    trace vdelete ::_run($ind) w checkstate
  }
}
