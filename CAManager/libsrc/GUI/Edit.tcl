proc camGUI::aEdit {w} {
  global var
  regexp (\[0-9\]*), [$w cursel] all row
  set tl .t$row
  if {![catch {raise $tl}]} return

  foreach param {host port descr cfg cfgc archive log start timespec} {
    set var($row,$param) [camMisc::arcGet $row $param]
  }

  toplevel $tl
  label $tl.xxx
  wm title $tl "ArchiveEngine properties"
  set lf [TitleFrame $tl.f -text "ArchiveEngine properties" -side left -font [$tl.xxx cget -font]]
  set f [$lf getframe]
  frame $f.h -bd 0

  set hl {}
  foreach k [camMisc::arcIdx] {
    lappend hl [camMisc::arcGet $k host]
  }
  combobox $f.h.hf "Host:" left var($row,host) $hl -side top -fill x -padx 4

  set ::ports [getPorts $row]
  set var($row,oport) $var($row,port)
  spinbox $f.h.port Port: left var($row,port) {1 65535 1} {} -side right -fill x
  entrybox $f.de Description: top var($row,descr)
  frame $f.c -bd 0
  entrybox $f.c.cfg "Configuration-file: (absolute pathname)" top var($row,cfg) -expand t
  button  $f.c.fs -text "..." -padx 0 -pady 0 -width 2 -bd 1 -command "
      set fn \[getCfgFile $tl $row\]
      if {\"\$fn\" != \"\"} {set var($row,cfg) \$fn}"
  checkbutton $f.olcfgc -text "disable configuration changes via ArchiveEngine web interface" \
      -variable var($row,cfgc)

  set af {"" freq_directory dir directory catalog %Y/%V/directory %Y/%m/%d/directory}
  foreach k [camMisc::arcIdx] {
    lappend af [camMisc::arcGet $k archive]
  }
  combobox $f.af "Archive-file: (rel. to path of Configuration file)" top \
      var($row,archive) $af -side top -fill x

  set ll {log log/%Y/%m/%d/%H%M%S}
  foreach k [camMisc::arcIdx] {
    lappend ll [camMisc::arcGet $k log]
  }
  combobox $f.lf "Log-file: (rel. to path of Configuration file)" top \
      var($row,log) $ll -side top -fill x

  combobox $f.run Run/Rerun: left var($row,start) \
      {NO hourly daily weekly monthly timerange always}
  proc showpage {args} "$f.nb raise \$::var($row,start)"
  trace variable var($row,start) w camGUI::showpage
  $f.run.e configure -editable 0 \
      -insertontime 0 -selectborderwidth 0 
#      -selectbackground white -selectforeground black
  PagesManager $f.nb

  set page [$f.nb add NO]

  set page [$f.nb add hourly]
  spinbox $page.m Minute left var(minute) {0 59 1} {}
  $page.m.e configure -editable 0 -width 3
  set var(minute) 42
  pack $page.m

  set page [$f.nb add daily]
  spintime $page.t var(hour) var(minute)
  pack $page.t

  set page [$f.nb add weekly]
  combobox $page.w "Weekday" left var(day) $::weekdays
  $page.w.e configure -editable 0 -insertontime 0 -selectborderwidth 0
  spintime $page.t var(hour) var(minute)
  pack $page.w $page.t

  set page [$f.nb add monthly]
  spintime $page.t var(hour) var(minute)
  spinbox $page.t.d "Day of Month" left var(dayofmonth) {1 28 1} {}
  $page.t.d.e configure -width 3
  $page.t.d.e configure -editable 0
  pack $page.t.d.e -side bottom -fill none -expand 0
  pack $page.t.d -side left
  pack $page.t

  set page [$f.nb add timerange]
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
  pack $page.f $page.t -side left -fill both -expand t

  set page [$f.nb add always]
  $f.nb compute_size

  pack $f.h.hf -side left -fill x
  pack $f.h.port -side right -fill x
  pack $f.c.cfg -side left -fill x -expand t
  pack $f.c.fs -side bottom
  pack $f.h $f.de $f.c $f.olcfgc $f.af $f.lf -fill x -padx 4 -pady 4
  pack $f.run -side top -padx 4 -pady 4
  pack $f.nb -side top -fill x -padx 4 -pady 4
  pack $lf -side top -fill both -expand t -padx 4 -pady 4

  $f.nb raise $var($row,start)
  
#  if {"$var($row,timespec)" == "-"} {
#    set var($row,timespec) [clock seconds]
#  }

  set var(minute) 0
  set var(hour) 2
  set var(day) Monday
  set var(dayofmonth) 1

  switch $var($row,start) {
    hourly {
      set var(minute) $var($row,timespec)
    }
    daily {
      set due [clock scan $var($row,timespec)]
      set var(minute) [clock format $due -format %M]
      set var(hour) [clock format $due -format %H]
    }
    weekly {
      set due [clock scan [lindex $var($row,timespec) 1]]
      set var(day) [lindex $var($row,timespec) 0]
      set var(minute) [clock format $due -format %M]
      set var(hour) [clock format $due -format %H]
    }
    monthly {
      set var(dayofmonth) [lindex $var($row,timespec) 0]
      set due [clock scan [lindex $var($row,timespec) 1]]
      set var(minute) [clock format $due -format %M]
      set var(hour) [clock format $due -format %H]
    }
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
    $f.lf.e configure -state disabled
    $f.olcfgc configure -state disabled
  }

  while {1} {
    vwait var($row,close)
    if {$var($row,close) == 0} break
    switch $var($row,start) {
      hourly {
	set var($row,timespec) $var(minute)
      }
      daily {
	set var($row,timespec) [format "%02d:%02d" $var(hour) $var(minute)]
      }
      weekly {
	set var($row,timespec) [format "%s %02d:%02d" $var(day) $var(hour) $var(minute)]
      }
      monthly {
	set var($row,timespec) [format "%d %02d:%02d" $var(dayofmonth) $var(hour) $var(minute)]
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
  trace vdelete var($row,start) w camGUI::showpage
  destroy $tl
  if {$var($row,close) == 0} {setButtons $w; return 0}
  
  foreach param {host port descr cfg cfgc archive log start timespec} {
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
