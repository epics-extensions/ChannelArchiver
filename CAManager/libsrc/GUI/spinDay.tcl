proc spinDay {win step} {
  set index [expr [lsearch $::weekdays [$win get]] + $step]
  
  if {$index < 0} {set index 0}
  if {$index > 6} {set index 6}
  
  $win delete 0 end
  $win insert 0 [lindex $::weekdays $index]
}
