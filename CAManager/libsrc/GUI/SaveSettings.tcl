set ::MagicLine "# and now the settings.... (BTW: don't modify this line!!!)"
set ::settingsSaved 1

proc camGUI::SaveSettings {{fh -1}} {
  if {$::settingsSaved && ($fh == -1)} return
  set wh 0
  if {$camMisc::force_cfg_file} {
    if {$fh == -1} {
      if [catch {set wh [open $camMisc::cfg_file.N w+]}] exit
      set hadit 0
      for_file line $camMisc::cfg_file {
	if {!$hadit && ($line != $::MagicLine)} {
	  puts $wh $line
	} else {
	  set hadit 1
	}
      }
    } else {
      set wh $fh
    }
    puts $wh "$::MagicLine\n"
  }
  foreach k [array names ::selected] {
    set l [luniq $::selected($k)]
    set ll [llength $l]
    set l [lrange $l [expr $ll - 20] end] 
    save $wh ::selected($k) $l $k "\\selected"
  }
  save $wh ::_port $::_port _port
  save $wh ::dontCheckAtAll $::dontCheckAtAll dontCheckAtAll
  save $wh ::checkInt $::checkInt checkInt
  save $wh ::checkBgMan $::checkBgMan checkBgMan
  save $wh ::bgCheckInt $::bgCheckInt bgCheckInt
  save $wh ::bgUpdateInt $::bgUpdateInt bgUpdateInt
  save $wh ::multiVersion $::multiVersion multiVersion
  if {$wh != $fh} { 
    close $wh 
    file rename -force $camMisc::cfg_file.N $camMisc::cfg_file
  }
  set ::settingsSaved 1
}

proc save {wh var val desc {p ""}} {
  if {"$::tcl_platform(platform)" == "unix"} {
    puts $wh "set $var {$val}"
  } else {
    registry set "$camMisc::reg_stem\\Settings$p" $desc $val
  }
}
