proc endisable {v i m} {
  if {![info exists ::TdontCheckAtAll]} return
  if {$::TdontCheckAtAll} {
    $::spb configure -state disabled
  } else {
    $::spb configure -state normal
  }
}

proc camGUI::aPrefs {} {
  set ::_oport $::_port

  toplevel .prefs
  wm title .prefs "Prefferences"
  label .prefs.xxx

  set ::TdontCheckAtAll $::dontCheckAtAll
  set ::TcheckInt $::checkInt
  set ::TcheckBgMan $::checkBgMan
  set ::T_port $::_port

  NoteBook .prefs.nb -font [.prefs.xxx cget -font]
  set w [.prefs.nb insert end "gen" -text "General"]
  packTree $w {
    {checkbutton dontcheck {-variable ::TdontCheckAtAll \
				-text "Don't check for running Archivers regularly"} {-pady 4} {}}
    {LabelFrame inter {-side left -anchor w -text "check-interval:"} {-fill x -pady 4} {
      {SpinBox e {-entrybg white -bd 1 -textvariable ::TcheckInt -range {1 3600 1}} {} {} ::spb}
    }}
  }

  set w [.prefs.nb insert end "bgman" -text "bgManager"]
  packTree $w {
    {LabelFrame head {-side left -anchor w -text "Port for CAbgManager: "} {-fill x -padx 4 -pady 4} {
      {entry port {-textvariable ::_port -bg white} {-fill x} {} ::_entry}
    }}
    {checkbutton check {-variable ::TcheckBgMan \
			    -text "Check for running CAbgManager on startup"} {-pady 4} {}}
  }
  .prefs.nb compute_size
  pack .prefs.nb -side top -fill both -expand t
  packTree .prefs {
    {frame h1 {-bd 1 -relief sunken} {-fill x -padx 4}}
    {button ok {-text Ok -command {
      lassign [list $::T_port $::TcheckInt $::TdontCheckAtAll $::TcheckBgMan] ::_port ::checkInt ::dontCheckAtAll ::checkBgMan
      destroy .prefs
    }} {-side right -padx 8 -pady 8} {} ::_ok}
    {button can {-text Cancel -command {destroy .prefs}} {-side right -padx 8 -pady 8} {} ::_can}
  }
  .prefs.nb raise "gen"
  bind $::_entry <Return> {$::_ok invoke}
  bind $::_entry <Escape> {$::_can invoke}
  focus $::_entry

  trace variable ::TdontCheckAtAll w "endisable"
  tkwait window .prefs
  trace vdelete ::TdontCheckAtAll w "endisable"

  if {"$::_port" != "$::_oport"} {
    SaveSettings
    tk_dialog .confirm "Notice" "Changes will take effect after restart of CAbgManager!" warning 0 Close
  }
}
