proc camGUI::aOpen {} {
  # open a fileselector
  # if there's a file selected -> open it
  if {[info exists camMisc::rcdir]} {cd $camMisc::rcdir}
  set fn [tk_getOpenFile]
  if {"$fn" != ""} {
    set camMisc::force_cfg_file 1
    set camMisc::cfg_file $fn
    camGUI::reOpen
  }
}

proc camGUI::aOpenDefault {} {
  # open the (platform-specific) default file
  set camMisc::force_cfg_file $camMisc::force_cfg_file_d
  set camMisc::cfg_file $camMisc::cfg_file_d
  camGUI::reOpen
}

proc camGUI::reOpen {} {
  # reopen the currently opened file
  camMisc::init
  camGUI::init
  camGUI::initTable .tf.t
  wm deiconify .
  raise .
  focus -force .
}
