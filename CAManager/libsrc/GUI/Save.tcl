proc camGUI::aSaveAs {} {
  set fn [tk_getSaveFile]
  if {"$fn" != ""} {
    set camMisc::cfg_file $fn
    set camMisc::force_cfg_file 1
    aSave
  }
}

proc camGUI::aSaveAsDefault {} {
  set camMisc::cfg_file $camMisc::cfg_file_d
  set camMisc::force_cfg_file $camMisc::force_cfg_file_d
  aSave
}

proc camGUI::aSave {} {
  set camMisc::cfg_file_tail [file tail $camMisc::cfg_file]
  if {$camMisc::force_cfg_file} {
    set o [open $camMisc::cfg_file w]
    
    puts $o "# -*- tcl -*-"
    puts $o ""
    puts $o "set Archivers {"
    foreach idx [camMisc::arcIdx] {
      set arc [camMisc::arcGet $idx]
      global $arc
      puts $o " {"
      foreach k [array names $arc] {
	puts $o "   $k \"[camMisc::arcGet $idx $k]\""
      }
      puts $o " }"
    }
    puts $o "}\n"
    SaveSettings $o
    close $o
  } else {
    
    set stem $camMisc::reg_stem
    registry delete "$stem"
    set cnt 1
    foreach idx [camMisc::arcIdx] {
      set arc [camMisc::arcGet $idx]
      global $arc
      foreach k [array names $arc] {
	registry set [format "$stem\\arc%06d" $cnt] $k [camMisc::arcGet $idx $k]
      }
      incr cnt
    }
    registry set "$stem" ts [clock seconds]
  }
}
