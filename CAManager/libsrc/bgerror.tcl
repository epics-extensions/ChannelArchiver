if [info exists tk_version] {
  set funame xxx
} else {
  set funame bgerror
}

proc $funame {args} {
  global errorInfo
  foreach l [split $errorInfo "\n"] {
    puts stderr $l
    Puts "$l" error
  }
}
