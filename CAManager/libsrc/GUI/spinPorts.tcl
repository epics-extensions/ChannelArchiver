proc spinPort {row} {
  global var
  if {$var($row,oport) > $var($row,port)} {
    set inc -1
  } else {
    set inc 1
  }
  if {[info exists camGUI::aEngines($row,$::iRun)] &&
      [regexp "^since" $camGUI::aEngines($row,$::iRun)]} return
  set op [expr max(1000,min(65535,$var($row,port)))]
  while {[lsearch $::ports $op] >= 0} {
    incr op $inc
  }
  if {($op < 1000) || ($op > 65535)} return
  if {[lsearch $::ports $op] < 0} {
    set var($row,port) $op
  }
  set var($row,oport) $var($row,port)
}
