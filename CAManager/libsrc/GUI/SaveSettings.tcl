proc camGUI::SaveSettings {} {
  if {"$::tcl_platform(platform)" == "unix"} {
    if [catch {set wh [open "$camMisc::rcdir/settings" w]}] exit
  }
  foreach k [array names ::selected] {
    set l [luniq $::selected($k)]
    set ll [llength $l]
    set l [lrange $l [expr $ll - 20] end] 
    save $wh ::selected($k) $l selected
  }
  save $wh ::_port $::_port _port
  save $wh ::dontCheckAtAll $::dontCheckAtAll dontCheckAtAll
  save $wh ::checkInt $::checkInt checkInt
  save $wh ::checkBgMan $::checkBgMan checkBgMan
}

proc save {wh var val desc} {
  if {"$::tcl_platform(platform)" == "unix"} {
    puts $wh "set $var {$val}"
  } else {
    registry set "$camMisc::reg_stem\\Settings" $desc $val
  }
}
