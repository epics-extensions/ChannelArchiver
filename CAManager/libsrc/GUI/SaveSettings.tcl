set ::MagicLine "# and now the settings.... (BTW: don't modify this line!!!)"

proc camGUI::SaveSettings {} {
  if {"$::tcl_platform(platform)" == "unix"} {
    if [catch {set wh [open $camMisc::cfg_file.N w+]}] exit
    set hadit 0
    for_file line $camMisc::cfg_file {
      if {!$hadit && ($line != $::MagicLine)} {
	puts $wh $line
      } else {
	set hadit 1
      }
    }
    puts $wh "$::MagicLine\n"
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
  close $wh
  file rename -force $camMisc::cfg_file.N $camMisc::cfg_file
}

proc save {wh var val desc} {
  if {"$::tcl_platform(platform)" == "unix"} {
    puts $wh "set $var {$val}"
  } else {
    registry set "$camMisc::reg_stem\\Settings" $desc $val
  }
}
