proc camComm::condSet {var val} {
  if {![info exists $var] || ([set $var] != $val)} {
    uplevel 1 "set $var \"$val\""
  }
}
