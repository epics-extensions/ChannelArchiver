proc camComm::condSet {var val} {
  if {[set $var] != $val} {
    uplevel 1 "set $var \"$val\""
  }
}
