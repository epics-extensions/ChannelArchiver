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
  set ::TbgCheckInt $::bgCheckInt
  set ::TbgUpdateInt $::bgUpdateInt
  set ::TcheckBgMan $::checkBgMan
  set ::T_port $::_port

  NoteBook .prefs.nb -font [.prefs.xxx cget -font]
  set w [.prefs.nb insert end "gen" -text "General"]
  packTree $w {
    {frame f {-bd 0} {} {
      {TitleFrame tf1 {-text "CAManager" -font [.prefs.xxx cget -font]} {-fill both -padx 4 -pady 4} {
	{checkbutton check {-variable ::TcheckBgMan -anchor w \
				-text "Check for running CAbgManager on startup"} {-fill x -pady 4} {}}
	{checkbutton dontcheck {-variable ::TdontCheckAtAll -onvalue 0 -offvalue 1 -anchor w \
				    -text "Check for running Archivers regularly"} {-fill x -pady 4} {}}
	{LabelFrame inter {-side left -anchor w -text "check-interval:"} {-fill x -padx 24 -pady 4} {
	  {Label suff {-text seconds} {-side right} {}}
	  {SpinBox e {-entrybg white -bd 1 -width 5 -textvariable ::TcheckInt -range {1 3600 1}} {-side right -padx 4} {} ::spb}
	}}
      }}
    }}
  }

  set w [.prefs.nb insert end "bgman" -text "bgManager"]
  packTree $w {
    {frame f {-bd 0} {} {
      {TitleFrame tf2 {-text "general" -font [.prefs.xxx cget -font]} {-fill both -padx 4 -pady 4} {
	{LabelFrame inter {-side left -anchor w -text "check-interval:"} {-fill x -padx 4 -pady 4} {
	  {Label suff {-text seconds} {-side right} {}}
	  {SpinBox e {-entrybg white -bd 1 -width 5 -textvariable ::TbgCheckInt -range {1 3600 1}} {-side right -padx 4} {} ::spb}
	}}
      }}
      {TitleFrame tf1 {-text "Web-Interface" -font [.prefs.xxx cget -font]} {-fill both -padx 4 -pady 4} {
	{LabelFrame head {-side left -anchor w -text "Port: "} {-fill x -padx 4 -pady 4} {
	  {entry port {-textvariable ::T_port -bg white -width 15} {-side right} {} ::_entry}
	}}
	{LabelFrame update {-side left -anchor w -text "update-interval:"} {-fill x -padx 4 -pady 4} {
	  {Label suff {-text seconds} {-side right} {}}
	  {SpinBox e {-entrybg white -bd 1 -width 5 -textvariable ::TbgUpdateInt -range {1 3600 1}} {-side right -padx 4} {} ::spb}
	}}
      }}
    }}
  }

  set w [.prefs.nb insert end "multi" -text "MultiArchive"]
  packTree $w {
    {frame f {-bd 0} {} {
      {TitleFrame mav {-text "MultiArchive Version" -font [.prefs.xxx cget -font]} {-fill both -padx 4 -pady 4} {
	{radiobutton one {-text "version 1 (exact, but slower)" -value 1 -anchor w -variable ::multiVersion} {-fill x -padx 4 -pady 4}}
	{radiobutton two {-text "version 2 (may be fuzzy, but is faster)" -value 2 -anchor w -variable ::multiVersion} {-fill x -padx 4 -pady 4}}
      }}
    }}
  }
  .prefs.nb compute_size
  pack .prefs.nb -side top -fill both -expand t
  packTree .prefs {
    {frame h1 {-bd 1 -relief sunken} {-fill x -padx 4}}
    {button ok {-text Ok -command {
      set ::settingsSaved 0
      lassign [list $::T_port $::TcheckInt $::TdontCheckAtAll $::TcheckBgMan $::TbgCheckInt $::TbgUpdateInt] ::_port ::checkInt ::dontCheckAtAll ::checkBgMan ::bgCheckInt ::bgUpdateInt
      destroy .prefs
    }} {-side right -padx 8 -pady 8} {} ::_ok}
    {button can {-text Cancel -command {destroy .prefs}} {-side right -padx 8 -pady 8} {} ::_can}
  }
  .prefs.nb raise "gen"
  bind $::_entry <Return> {$::_ok invoke}
  bind $::_entry <Escape> {$::_can invoke}
  focus $::_entry

#  trace variable ::TdontCheckAtAll w "endisable"
  tkwait window .prefs
#  trace vdelete ::TdontCheckAtAll w "endisable"

  if {"$::_port" != "$::_oport"} {
    SaveSettings
    tk_dialog .confirm "Notice" "Changes will take effect after restart of CAbgManager!" warning 0 Close
  }
}
