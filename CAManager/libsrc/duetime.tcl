# returns start- and stop-time, when an archiver is SUPPOSED to run!
proc duetime {i {run X} {timespec X}} {
  if {$run == "X"} {set run [camMisc::arcGet $i start]}
  if {$timespec == "X"} {set timespec [camMisc::arcGet $i timespec]}
  set now [clock seconds]
  switch $run {
    minute {
      lassign $timespec st ev
      set delta [expr $ev * 60]
      set m [expr 1[clock format $now -format %M] - 100]
      set fh [clock scan [clock format $now -format "%m/%d/%Y %H:00"]]
      set starttime [expr ( ( $m / $ev ) * $ev + $st ) * 60 + $fh]
    }
    hour {
      lassign $timespec time ev
      set delta [expr $ev * 3600]
      set h [expr 1[clock format $now -format %H] - 100]
      set starttime [expr (( $h / $ev ) * $ev) * 3600 + [clock scan $time]]
    }
    day {
      lassign $timespec time ev
      set delta [expr 86400 * $ev]
      set day0 [expr ($now / $delta) * $delta]
      set starttime [clock scan [clock format $day0 -format "%m/%d/%Y $time"]]
    }
    week {
      lassign $timespec day time ev
      set starttime [clock scan $time]
      set dd [lsearch $::weekdays $day]
      set wd [clock format $now -format %w]
      incr starttime [expr ( $dd - $wd ) * 86400]
      set delta [expr $ev * 7 * 86400]
    }
    month {
      lassign $timespec day time ev
      if { ( $day > 28 ) || ( $day < 1 ) } { return {-1 -1} }
      set td [expr 1[clock format $now -format %d] - 100]
      set tm [expr 1[clock format $now -format %m] - 101]
      set ty [clock format $now -format %Y]
      if {$tm == 0} { set tm 12; incr ty -1 }
      while {[set d1 [clock scan "$tm/$day/$ty $time"]] < $now} {
	set starttime $d1
	incr tm
	if {$tm == 13} { incr ty; set tm 1 }
      }
      set delta [expr $d1 - $starttime]
    }
    timerange {
      set starttime [clock scan "[lindex $timespec 0] [lindex $timespec 1]"]
      set delta [clock scan "[lindex $timespec 3] [lindex $timespec 4]"]
      incr delta -$starttime
#      if {$starttime < $now} {set starttime 0}
    }
    always {
      set starttime 0
      set delta 0
    }
    NO {
      set starttime 0
      set delta 0
    }
    default {
      Puts "invalid schedule-mode" error
      return {-1 -1}
    }
  }

  if {$delta != 0} {
    while {$starttime > $now} { incr starttime -$delta }
    while {[expr $starttime + $delta] <= $now} {incr starttime $delta}
  }
  return [list $starttime [expr $starttime + $delta]]
}

if {$::debug} {
  set ::weekdays {Sunday Monday Tuesday Wednesday Thursday Friday Saturday}
  set xamples {
    {minute "0 1"}
    {minute "12 12"}
    {minute "12 15"}
    {hour "00:27 1"}
    {hour "06:00 8"}
    {day "14:30 1"}
    {day "02:00 2"}
    {day "23:59 3"}
    {week "Sunday 12:00 1"}
    {week "Monday 12:00 2"}
    {week "Tuesday 12:00 3"}
    {week "Wednesday 12:00 4"}
    {week "Thursday 12:00 5"}
    {week "Friday 12:00 6"}
    {week "Saturday 12:00 7"}
    {month "1 12:00"}
    {month "10 12:00"}
    {month "20 12:00"}
    {month "28 12:00"}
  }
  foreach sample $xamples {
    lassign $sample run timespec
    lassign [duetime 0 $run $timespec] starttime stoptime
    if {$starttime < 0} {
      set a "invalid"
    } else {
      set a "[clock format $starttime] .. [clock format $stoptime]"
    }
    puts [format "%-25s %s" "$sample:" $a]
  }
}
