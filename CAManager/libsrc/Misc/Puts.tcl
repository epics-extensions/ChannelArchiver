array set colormap {
  error		red
  command	blue
  schedule	green
  debug1	no
  debug2	no
  normal	normal
}

if {$::debug} {
  set ::colormap(debug1) normal
  set ::colormap(debug2) orange
}

proc Puts {s {color normal}} {
  set color $::colormap($color)
  set now "[clock format [clock seconds] -format %Y/%m/%d\ %H:%M:%S]: "
  if {$::debug} {puts stderr "$now$s"}
  if {$color == "no"} return
  if {$color != "normal"} {
    set s "<font color=$color>$s</font>"
  }
  set ::_msg [lrange [linsert $::_msg 0 "$now$s"] 0 99]
}
