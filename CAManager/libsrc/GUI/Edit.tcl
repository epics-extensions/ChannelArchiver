proc camGUI::aEdit {w} {
  global var
  regexp (\[0-9\]*), [$w cursel] all row
  set tl .t$row
  if {![catch {raise $tl}]} return

  foreach param {host port descr cfg cfgc archive log start timespec multi} {
    set var($row,$param) [camMisc::arcGet $row $param]
  }

  toplevel $tl
  label $tl.xxx
  wm title $tl "Archiver properties"
  set lf [TitleFrame $tl.f -text "Archiver properties" -side left -font [$tl.xxx cget -font]]
  set f [$lf getframe]
  frame $f.h -bd 0

  set hl {}
  foreach k [camMisc::arcIdx] {
    lappend hl [camMisc::arcGet $k host]
  }
  combobox $f.h.hf "Host:" left var($row,host) $hl -side top -fill x -padx 4
  $f.h.hf.e configure -helptext "A hostname where this\nArchiver is supposed to run."

  set ::ports [getPorts $row]
  set var($row,oport) $var($row,port)
  spinbox $f.h.port "Port:" left var($row,port) {1 65535 1} {} -side right -fill x
  $f.h.port.e configure -width 5 -helptext "A portnumber for the\narchiver's web-interface\nKeep in mind, that there mustn't\nbe any collisions!"
  entrybox $f.de Description: top var($row,descr)
  $f.de.e configure -helptext "A description for the Archiver.\nThe Archiver's web-interface will have this description as well."
  frame $f.c -bd 0
  entrybox $f.c.cfg "Configuration-file: (absolute pathname)" top var($row,cfg) -expand t
  $f.c.cfg.e configure -helptext "The full pathname of the\nmain configuration file\nfor this Archiver."
  button  $f.c.fs -text "..." -padx 0 -pady 0 -width 2 -bd 1 -command "
      set fn \[getCfgFile $tl $row\]
      if {\"\$fn\" != \"\"} {set var($row,cfg) \$fn}"
  checkbutton $f.olcfgc -text "disable configuration changes via Archiver's web interface" \
      -variable var($row,cfgc) -onvalue 1 -offvalue 0

  set af {"" freq_directory dir directory catalog %Y/%V/directory %Y/%m/%d/directory}
  foreach k [camMisc::arcIdx] {
    lappend af [camMisc::arcGet $k archive]
  }
  combobox $f.af "Archive-file: (rel. to path of Configuration file)" top \
      var($row,archive) $af -side top -fill x
  $f.af.e configure -helptext "The filename of the archive file.\nShould not be an absolute pathname (starting with /).\nNever put more than one archive in a single directory!\nDirectories will be created.\nSubject to %-expansion (see documentation)."

  set ma [list [file dirname [file dirname $var($row,cfg)]]/directory]
  foreach k [camMisc::arcIdx] {
    lappend ma [camMisc::arcGet $k multi]
  }
  combobox $f.ma "Multi-Archive file: (absolute path)" top \
      var($row,multi) $ma -side top -fill x
  $f.ma.e configure -helptext "The Filename of a Multi-Archive file,\nwhere these archives should be appended to."

  set ll {log log/%Y/%m/%d/%H%M%S}
  foreach k [camMisc::arcIdx] {
    lappend ll [camMisc::arcGet $k log]
  }
  combobox $f.lf "Log-file: (rel. to path of Configuration file)" top \
      var($row,log) $ll -side top -fill x
  $f.lf.e configure -helptext "The filename of the Archiver's log file.\nShould not be an absolute pathname (starting with /).\nDirectories will be created.\nSubject to %-expansion (see documentation)."

  if {$::debug} {
    combobox $f.run Run/Rerun: left var($row,start) \
	{NO minute hour day week month timerange always}
  } else {
    combobox $f.run Run/Rerun: left var($row,start) \
	{NO hour day week month timerange always}
  }
  $f.run.e configure -helptext "A schedule of how this Archiver should be started/restarted.\nDirectories are changed/created on each restart."
  proc showpage($row) {args} "$f.nb raise \$::var($row,start)"
  trace variable var($row,start) w camGUI::showpage($row)
  $f.run.e configure -editable 0 \
      -insertontime 0 -selectborderwidth 0 
  PagesManager $f.nb

  set var(minute) 0
  set var(time) 02:00:00
  set var(day) Monday
  set var(dayofmonth) 1
  set var(start) 0
  set var(every) 1

  switch $var($row,start) {
    minute {
      lassign $var($row,timespec) var(start) var(every)
    }
    hour {
      lassign $var($row,timespec) var(time) var(every)
    }
    day {
      lassign $var($row,timespec) var(time) var(every)
    }
    week {
      lassign $var($row,timespec) var(day) var(time) var(every)
    }
    month {
      lassign $var($row,timespec) var(dayofmonth) var(time)
    }
    default {
    }
  }

  set page [$f.nb add NO]

  set page [$f.nb add minute]
  frame $page.rep -bd 0
  spinbox $page.rep.st "at minute" left var(start) {0 59 1} {}
  $page.rep.st.e configure -width 3
  combobox $page.rep.ev "every" left var(every) {1 2 3 4 5 6 10 12 15 20 30}
  $page.rep.ev.e configure -editable 0 -width 3
  label $page.rep.lb -text "minutes"
  pack $page.rep.st $page.rep.ev $page.rep.lb -side left -padx 4 -pady 4
  pack $page.rep -side top

  set page [$f.nb add hour]
  frame $page.rep -bd 0
  spintime $page.rep.t var(time)
  combobox $page.rep.ev "every" left var(every) {1 2 3 4 6 8 12}
  $page.rep.ev.e configure -editable 0 -width 3
  label $page.rep.lb -text "hours,"
  pack $page.rep.t $page.rep.ev $page.rep.lb -side left -padx 4 -pady 4
  pack $page.rep -side top

  set page [$f.nb add day]
  frame $page.rep -bd 0
  spinbox $page.rep.ev "every" left var(every) {1 30 1} {}
  $page.rep.ev.e configure -width 3
  label $page.rep.lb -text "days"
  pack $page.rep.ev $page.rep.lb -side left -padx 4 -pady 4
  spintime $page.rep.t var(time)
  pack $page.rep.t -side left -padx 4 -pady 4
  pack $page.rep -side top

  set page [$f.nb add week]
  frame $page.rep -bd 0
  spinbox $page.rep.ev "every" left var(every) {1 52 1} {}
  $page.rep.ev.e configure -width 3
  label $page.rep.lb -text "weeks,"
  pack $page.rep.ev $page.rep.lb -side left -padx 4 -pady 4
  combobox $page.rep.w "" left var(day) $::weekdays
  $page.rep.w.e configure -width 10
  $page.rep.w.e configure -editable 0 -insertontime 0 -selectborderwidth 0
  spintime $page.rep.t var(time)
  pack $page.rep.w $page.rep.t -side left -padx 4 -pady 4
  pack $page.rep -side top

  set page [$f.nb add month]
  frame $page.rep -bd 0
  spinbox $page.rep.d "day" left var(dayofmonth) {1 28 1} {}
  $page.rep.d.e configure -width 3
  $page.rep.d.e configure -editable 0
  spintime $page.rep.t var(time)
  pack $page.rep.d $page.rep.t -side left -padx 4 -pady 4
  pack $page.rep -side top

  set page [$f.nb add timerange]
  set page [frame $page.tr]
  frame $page.f
  label $page.f.l -text From: -anchor w
  iwidgets::dateentry $page.f.d -iq high
  iwidgets::timeentry $page.f.t -format military -seconds off
  frame $page.t
  label $page.t.l -text To: -anchor w
  iwidgets::dateentry $page.t.d -iq high
  iwidgets::timeentry $page.t.t -format military -seconds off
  set ft $page

  pack $page.f.l $page.f.d $page.f.t -fill x
  pack $page.t.l $page.t.d $page.t.t -fill x
  pack $page.f $page.t -side left -fill both
  pack $page -side top

  set page [$f.nb add always]
  $f.nb compute_size

  pack $f.h.hf -side left -fill x
  pack $f.h.port -side right -fill x
  pack $f.c.cfg -side left -fill x -expand t
  pack $f.c.fs -side bottom
  pack $f.h $f.de $f.c $f.olcfgc $f.af $f.ma $f.lf -fill x -padx 4 -pady 4
  pack $f.run -side top -padx 4 -pady 4
  pack $f.nb -side top -fill x -padx 4 -pady 4
  pack $lf -side top -fill both -expand t -padx 4 -pady 4

  $f.nb raise $var($row,start)
  
  switch $var($row,start) {
    timerange {
      $ft.f.d show [lindex $var($row,timespec) 0]
      $ft.f.t show [lindex $var($row,timespec) 1]
      $ft.t.d show [lindex $var($row,timespec) 3]
      $ft.t.t show [lindex $var($row,timespec) 4]
    }
    default {
    }
  }

  iwidgets::buttonbox $tl.bb
  $tl.bb add Cancel -text Cancel -command [list set var($row,close) 0]
  $tl.bb add OK -text OK -command [list set var($row,close) 1]
  $tl.bb default 1
  bind $tl <Return> [list $tl.bb invoke 1]
  bind $tl <Escape> [list $tl.bb invoke 0]

  pack $tl.bb -side top -fill x -padx 4 -pady 4

  if {[regexp "^since" $camGUI::aEngines($row,$::iRun)]} {
    $f.de.e configure -state disabled
    $f.h.hf.e configure -state disabled
    $f.h.port.e configure -state disabled
    $f.c.cfg.e configure -state disabled
    $f.c.fs configure -state disabled
    $f.af.e configure -state disabled
    $f.ma.e configure -state disabled
    $f.lf.e configure -state disabled
    $f.olcfgc configure -state disabled
  }

  while {1} {
    vwait var($row,close)
    if {$var($row,close) == 0} break
    switch $var($row,start) {
      minute {
	set var($row,timespec) [list $var(start) $var(every)]
      }
      hour {
	set var($row,timespec) [list $var(time) $var(every)]
      }
      day {
	set var($row,timespec) [list $var(time) $var(every)]
      }
      week {
	set var($row,timespec) [list $var(day) $var(time) $var(every)]
      }
      month {
	set var($row,timespec) [list $var(dayofmonth) $var(time)]
      }
      timerange {
	set f [clock scan "[$ft.f.d get] [$ft.f.t get]"]
	set t [clock scan "[$ft.t.d get] [$ft.t.t get]"]
	if {$f > $t} {
	  tk_messageBox -type ok -icon error -parent $tl -title "Timespec Error" \
	      -message "\"From:\" is after \"To:\" !!!\nPlease review and change dates!"
	  continue
	} elseif {[expr $t - $f] < 60} {
	  tk_messageBox -type ok -icon error -parent $tl -title "Timespec Error" \
	      -message "\"From:\" is too close to \"To:\" !!!\nPlease review and change dates!"
	  continue
	} else {
	  set var($row,timespec) "[$ft.f.d get] [$ft.f.t get] - [$ft.t.d get] [$ft.t.t get]"
	}
      }
      default {
	set var($row,timespec) "-"
      }
    }
    break
  }
  trace vdelete var($row,start) w camGUI::showpage($row)
  destroy $tl
  if {$var($row,close) == 0} {setButtons $w; return 0}
  
  foreach param {host port descr cfg cfgc archive log start timespec multi} {
    camMisc::arcSet $row $param $var($row,$param)
  }
  set camGUI::aEngines($row,$::iHost) $var($row,host)
  set camGUI::aEngines($row,$::iPort) $var($row,port)
  set camGUI::aEngines($row,$::iDescr) $var($row,descr)
  set camGUI::aEngines($row,$::iRun) ""
  after 1 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
  ClearSel $w
  setButtons $w
  return 1
}
