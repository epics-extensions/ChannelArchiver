proc bgerror {args} {
  global errorInfo
#  Puts $args error
  foreach l [split $errorInfo "\n"] {
    puts stderr $l
    Puts "$l" error
  }
}
