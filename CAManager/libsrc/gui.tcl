option add *textBackground white widgetDefault
option add *borderWidth 1 widgetDefault
option add *buttonForeground black widgetDefault
option add *outline black widgetDefault
option add *weekdayBackground white widgetDefault
option add *weekendBackground mistyrose widgetDefault
option add *selectColor red widgetDefault

if {1} {
  option add *startDay sunday widgetDefault
  option add *days {Su Mo Tu We Th Fr Sa} widgetDefault
} else {
  option add *startDay monday widgetDefault
  option add *days {Mo Tu We Th Fr Sa Su} widgetDefault
}

set ::selected(archive) {}
set ::selected(misc) {}
set ::selected(regex) {}

namespace eval camGUI {
  variable aEngines
  variable row
  variable datetime
}

proc camGUI::init {} {
  variable aEngines
  variable row

  foreach pack {BWidget Tktable Iwidgets} {
    namespace inscope :: package require $pack
  } 
 
  option add *TitleFrame.l.font {helvetica 11 bold italic}
  
  wm withdraw .

  if {"$::tcl_platform(platform)" != "unix"} {
    registry set "$camMisc::reg_stem\\Settings"
    foreach v [registry values "$camMisc::reg_stem\\Settings"] {
      set ::$v [registry get "$camMisc::reg_stem\\Settings" $v]
    }
    foreach k [registry keys "$camMisc::reg_stem\\Settings"] {
      foreach v [registry values "$camMisc::reg_stem\\Settings\\$k"] {
	set ::${k}($v) [registry get "$camMisc::reg_stem\\Settings\\$k" $v]
      }
    }
  }
}


proc camGUI::aAbout {} {
  tk_dialog .about "About CAManager" "CAManager\n$::CVS(Version)\n$::CVS(Date)\nThomas Birke <birke@lanl.gov>" info 0 ok
}

proc camGUI::actionDialog {title} {
  incr ::tl_cnt
  set f [toplevel .t$::tl_cnt]
  wm protocol $f WM_DELETE_WINDOW "after 1 $f.bb.cancel invoke"
  wm title $f $title
  
  camGUI::packTree $f {
    {frame bb {-bd 0} {-side bottom -fill x} {
      {button go {-text Go -command {set ::var(%P,go) 1}} {-side right -padx 4 -pady 4}}
      {button cancel {-text Cancel -command {set ::var(%P,go) 0}} {-side right -padx 4 -pady 4}}
    }}
    {frame tf {-bd 0} {-side bottom -expand t -fill both -padx 4 -pady 4} {
      {scrollbar h {-orient horiz -command {%p.t xview}} {-side bottom -fill x}}
      {scrollbar v {-orient vert -command {%p.t yview}} {-side right -fill y}}
      {text t {-bg white -state disabled -wrap none -width 80 -height 15 -xscrollcommand {%p.h set}  -yscrollcommand {%p.v set}} {-fill both -expand t} {} ::w(%P,txt)}
    }}
  }

  $f.tf.t tag add error end; $f.tf.t tag configure error -foreground red
  $f.tf.t tag add command end; $f.tf.t tag configure command -foreground blue
  $f.tf.t tag add normal end;
  return $f
}

proc camGUI::aBlock {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  camMisc::Block $row camGUI::aEngines($row,$::iBlocked)
  camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
  setButtons $w
}

proc camGUI::aRelease {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  camMisc::Release $row camGUI::aEngines($row,$::iBlocked)
  camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
  setButtons $w
}

proc camGUI::spintime {w tvar} {
  global var
  frame $w -bd 0
  label $w.at -text at
  iwidgets::timeentry $w.te -format military -seconds off
  [$w.te component time] configure -textvariable $tvar
  pack $w.at $w.te -side left -fill both
}

proc camGUI::spinbox {w label labelside var vals cmd args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  SpinBox $w.e -entrybg white -bd 1 -textvariable $var -range $vals
  if {[llength $cmd] > 0} {$w.e configure -modifycmd $cmd}
  eval pack $w.e -side top -fill both $args
}

proc camGUI::combobox {w label labelside var vals args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  ComboBox $w.e -entrybg white -bd 1 -textvariable $var -values [luniq $vals]
  eval pack $w.e -side top -fill both $args
}

proc camGUI::entrybox {w label labelside var args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  Entry $w.e -bg white -bd 1 -textvariable $var
  eval pack $w.e -side top -fill both $args
}


proc camGUI::checkJob {} {
  after [expr $::checkInt * 1000] camGUI::checkJob
  if {$::dontCheckAtAll} return
  set ::busyIndicator "@"
  for {set row 0} {$row < [llength [camMisc::arcIdx]]} {incr row} {
    camComm::condSet camGUI::aEngines($row,$::iBlocked) \
	[lindex $::yesno [file exists [file join [file dirname [camMisc::arcGet $row cfg]] BLOCKED]]]
	
    after 10 camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
    update
  }
  after 500 {set ::busyIndicator ""}
}



proc camGUI::aCheck {w} {
  if {[regexp (\[0-9\]*), [$w cursel] all row] && 
      ($row < [llength [camMisc::arcIdx]])} {
    after 1 camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
  } else {
    for {set row 0} {$row < [llength [camMisc::arcIdx]]} {incr row} {
      after 1 camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
    }
  }
}

proc camGUI::ClearSel {w} {
  $w selection clear all
  set topf [file rootname [file rootname $w]]
  $topf.bf.start configure -state disabled
  $topf.bf.stop configure -state disabled
  $topf.bf.edit configure -state disabled
  $topf.bf.delete configure -state disabled
}

proc camGUI::closeDialog {w {wait 1}} {
  if {([info command $w.tf.t] == "$w.tf.t") && ($::var($w,go) == 1)} {
    foreach fh $::var(xec,fh,$w.tf.t) {
      catch {close $fh}
    }
  }
  destroy $w
}

proc camGUI::aConfig {w} {
  # show/edit the configuration file of the selected archiver
  # provide "!group" includes as hyperllinks
  if {![regexp (\[0-9\]*), [$w cursel] all row]} return
  set tl .c
  toplevel $tl
  wm title $tl "Archiver Config"
  wm protocol $tl WM_DELETE_WINDOW {after 1 $::w(close) invoke}
  set lnk 0
  
  packTree $tl {
    {label head {} {-fill x} {} ::w(head)}
    {frame txt {-bd 0} {-fill both -expand t} {
      {scrollbar v {-orient vertical -command {%p.t yview}} {-side right -fill y}}
      {text t {-state normal -yscrollcommand {%p.v set}} {-fill both -expand t} {} ::w(txt)}
    }}
    {button close {-text Close -command {set ::continue(%P) close}} {-side right -padx 8 -pady 8} {} ::w(close)}
    {button fwd {-text ">" -command {set ::continue(%P) fwd}} {-side right -padx 8 -pady 8} {} ::w(fwd)}
    {button rew {-text "<" -command {set ::continue(%P) rew}} {-side right -padx 8 -pady 8} {} ::w(rew)}
    {button save {-text "Save" -command {set ::continue(%P) save}} {-side right -padx 8 -pady 8} {}}
    {button reload {-text "Reload" -command {set ::continue(%P) reload}} {-side right -padx 8 -pady 8}}
  }
  $::w(txt) tag add comment end; $::w(txt) tag configure comment -foreground brown
  $::w(txt) tag add option end; $::w(txt) tag configure option -foreground magenta4

  bind $::w(txt) <Control-Key-x> {
    lassign [%W tag ranges sel] from to
    if {$from == {}} break
    set ::clip [%W get $from $to]
    %W delete $from $to
    break
  }
  bind $::w(txt) <Control-Key-c> {
    lassign [%W tag ranges sel] from to
    if {$from == {}} break
    set ::clip [%W get $from $to]
    break
  }
  bind $::w(txt) <Control-Key-v> {
    if {![info exists ::clip]} break
    lassign [%W tag ranges sel] from to
    if {$from != {}} {%W delete $from $to}
    %W insert insert $::clip
    break
  }

  set show [list [camMisc::arcGet $row cfg]]
  set sidx 0
  while 1 {
    if {[llength $show] > [expr $sidx + 1]} {
      $::w(fwd) configure -state normal
    } else {
      $::w(fwd) configure -state disabled
    }
    if {$sidx > 0} {
      $::w(rew) configure -state normal
    } else {
      $::w(rew) configure -state disabled
    }
    $::w(head) configure -text [lindex $show $sidx]
    $::w(txt) configure -state normal
    $::w(txt) delete 0.0 end
    if [file exists [lindex $show $sidx]] {
      for_file line [lindex $show $sidx] {
	if {[regexp "!group(.*)" $line all name]} {
	  incr lnk
	  set name [string trim $name]
	  $::w(txt) tag add link$lnk end
	  $::w(txt) tag configure link$lnk -foreground blue -underline 1
	  $::w(txt) tag bind link$lnk <Enter> \
	      "$::w(txt) tag configure link$lnk -background gray95"
	  $::w(txt) tag bind link$lnk <Leave> \
	      "$::w(txt) tag configure link$lnk -background \[$::w(txt) cget -background\]"
	  $::w(txt) tag bind link$lnk <ButtonRelease-1> \
	      "set ::continue($tl) \"$name\""
	  regexp "(.*)${name}(.*)" $line all before after
	  $::w(txt) insert end "$before" option
	  $::w(txt) insert end "$name" link$lnk
	  $::w(txt) insert end "$after"
	} elseif {[regexp "^!.*" $line]} {
	  $::w(txt) insert end "$line" option
	} elseif {[regexp "^#.*" $line]} {
	  $::w(txt) insert end "$line" comment
	} else {
	  $::w(txt) insert end "$line"
	}
	$::w(txt) insert end "\n"
      }
    } else {
      $::w(txt) insert end "\# File doesn't yet exist!\n" comment
      $::w(txt) insert end "\# Enter contents and press \"Save\"!" comment
      $::w(txt) insert end "\n\n"
    }
#    $::w(txt) configure -state disabled
    vwait ::continue($tl)
    switch $::continue($tl) {
      close { destroy $tl; return }
      fwd { incr sidx  1 }
      rew { incr sidx -1 }
      save {
	set s [lindex $show $sidx]
	if [file exists $s] {
	  file delete -force $s~
	  file rename $s $s~
	}
	write_file $s [string trim [$::w(txt) get 0.0 end]]
      }
      reload {}
      default { 
	set cfn [file dirname [lindex $show $sidx]]
	incr sidx
	if {$sidx < [llength $show]} {
	  set show [lreplace $show $sidx end]
	}
	set show [linsert $show end $cfn/$::continue($tl)]
      }
    }
  }
}

proc camGUI::aDelete {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  $w configure -state normal
  $w delete row $row
  $w configure -state disabled

  camMisc::arcDel $row
  catch {$w selection set anchor}
  setButtons $w
#  ClearSel $w
}

set percentDoc "The following is a list of suggested %-rules to use:"
append percentDoc "\n\t%Y\t year"
append percentDoc "\n\t%m\t month"
append percentDoc "\n\t%V\t weeknumber"
append percentDoc "\n\t%d\t day"
append percentDoc "\n\t%H\t hour"
append percentDoc "\n\t%M\t minute"
append percentDoc "\n\t%S\t second"
append percentDoc "\nA % followed by a number <n> indicates, that <n>"
append percentDoc "\ndirectories/files should be used in a round-robin way."
append percentDoc "\nSee documentation for further details!"

array set suggested {
  minute {
    {%Y/%m/%d/%H/%M/directory %2/directory} 
    {%Y.*%m.*%d.*%H.*%M.*/ %\[1-9\].*/ -%\[1-9\].*% -%.*%\[1-9\]}
    "\"%Y\", \"%m\", \"%d\", \"%H\" and \"%M\" (in this order) or"
    "a \"%\" followed by a number and no other \"%\""
  }
  hour {
    {%Y/%m/%d/%H/directory %2/directory}
    {%Y.*%m.*%d.*%H.*/ %\[1-9\].*/ -%\[1-9\].*% -%.*%\[1-9\]}
    "\"%Y\", \"%m\", \"%d\" and \"%H\" (in this order) or"
    "a \"%\" followed by a number (>0) and no other \"%\""
  }
  day {
    {%Y/%m/%d/directory %2/directory}
    {%Y.*%m.*%d.*/ %\[1-9\].*/ -%\[1-9\].*% -%.*%\[1-9\]}
    "\"%Y\", \"%m\" and \"%d\" (in this order) or"
    "a \"%\" followed by a number (>0) and no other \"%\""
  }
  week {
    {%Y/%V/directory %2/directory}
    {%Y.*%\[UV\].*/ %\[1-9\].*/ -%\[1-9\].*% -%.*%\[1-9\]}
    "\"%Y\" and \"%V\" or \"%U\" (in this order) or"
    "a \"%\" followed by a number (>0) and no other \"%\""
  }
  month {
    {%Y/%m/directory %2/directory}
    {%Y.*%m.*/ %\[1-9\].*/ -%\[1-9\].*% -%.*%\[1-9\]}
    "\"%Y\" and \"%m\" (in this order) or"
    "a \"%\" followed by a number (>0) and no other \"%\""
  }
  NO {
    {directory}
    {^\[^%\]*$}
    "no \"%\" at all"
  }
  always {
    {directory}
    {^\[^%\]*$}
    "no \"%\" at all"
  }
}

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
      set fn \[camGUI::getCfgFile $tl $row\]
      if {\"\$fn\" != \"\"} {set var($row,cfg) \$fn}"
  checkbutton $f.olcfgc -text "disable configuration changes via Archiver's web interface" \
      -variable var($row,cfgc) -onvalue 1 -offvalue 0

  foreach k [camMisc::arcIdx] {
    lappend af [camMisc::arcGet $k archive]
  }
  combobox $f.af "Archive-file: (rel. to path of Configuration file)" top \
      var($row,archive) {} -side top -fill x
  $f.af.e configure -helptext "The filename of the archive file.\nShould not be an absolute pathname (starting with /).\nNever put more than one archive in a single directory!\nDirectories will be created.\nSubject to %-expansion.\n$::percentDoc"
  set ::arcsel($row) $f.af.e

  set ma [list [file dirname [file dirname $var($row,cfg)]]/directory]
  foreach k [camMisc::arcIdx] {
    lappend ma [camMisc::arcGet $k multi]
  }
  combobox $f.ma "Multi-Archive file: (absolute path)" top \
      var($row,multi) $ma -side top -fill x
  $f.ma.e configure -helptext "The Filename of a Multi-Archive file,\nthese archives should be appended to."

  set ll {log log/%Y/%m/%d/%H%M%S}
  foreach k [camMisc::arcIdx] {
    lappend ll [camMisc::arcGet $k log]
  }
  combobox $f.lf "Log-file: (rel. to path of Configuration file)" top \
      var($row,log) $ll -side top -fill x
  $f.lf.e configure -helptext "The filename of the Archiver's log file.\nShould not be an absolute pathname (starting with /).\nDirectories will be created.\nSubject to %-expansion.\n$::percentDoc"

  if {$::debug} {
    combobox $f.run Run/Rerun: left var($row,start) \
	{NO minute hour day week month timerange always}
  } else {
    combobox $f.run Run/Rerun: left var($row,start) \
	{NO hour day week month timerange always}
  }
  $f.run.e configure -helptext "A schedule of how this Archiver should be started/restarted.\nDirectories are changed/created on each restart."
  proc showpage($row) {args} "$f.nb raise \$::var($row,start); $::arcsel($row) configure -values \[lindex \$::suggested(\$::var($row,start)) 0\]"
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

  $::arcsel($row) configure -values [lindex $::suggested($var($row,start)) 0]

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
    set dirwarn ""
    set matches 0
    foreach re [lindex $::suggested($var($row,start)) 1] {
      if {[string range $re 0 0] == "-"} {
	if [regexp [string range $re 1 end] $var($row,archive)] {
	  incr matches -1000
	}
      } else {
	incr matches [regexp $re $var($row,archive)]
      }
      puts stderr "   $matches"
    }
    if {$matches <= 0} {
      set dirwarn "For a Run/Rerun-mode of \"$var($row,start)\"\n"
      append dirwarn "the directory of the archive-file should contain:\n"
      foreach s [lrange $::suggested($var($row,start)) 2 end] {
	append dirwarn "  - $s\n"
      }
      if {[MessageBox warning "Archive-file warning" \
	       "The Archive-file specified doesn't match\nany of the suggested patterns!" \
	       $dirwarn\
	       "Do you want to continue with your setting anyway?" {No Yes} $tl] == "No"} {
	continue
      }
    }

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
	  MessageBox error "Timespec Error" \
	      "\"From:\" is after \"To:\" !!!\nPlease review and change dates!" Ok $tl
	  continue
	} elseif {[expr $t - $f] < 60} {
	  MessageBox error "Timespec Error" \
	      "\"From:\" is too close to \"To:\" !!!\nPlease review and change dates!" Ok $tl
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

proc camGUI::Exit {} {
  SaveSettings
  exit
}

proc camGUI::aExport {w} {
  set initialdir [pwd]
  set arch ""
  if [regexp (\[0-9\]*), [$w cursel] all row] {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
    set arch [clock format [clock seconds] -format $initialdir/[camMisc::arcGet $row archive]]
    if {![file exists $arch]} {set arch ""}
  }
  set f [actionDialog "Export Archive"]
  selFile $f.s "Select source Archive" ::var(xport,$::tl_cnt,src) $initialdir archive
  trace variable ::var(xport,$::tl_cnt,src) w camGUI::getAInfoCB
  frame $f.o -bd 0
  set f $f.o
  iwidgets::labeledframe $f.ef -labeltext "Export to" -labelpos nw -ipadx 4 -ipady 4
  set cs [$f.ef childsite]
  radiobutton $cs.a -text "an Archive" \
      -variable ::var(xport,$::tl_cnt,ef) -value "Archive" -anchor w
  radiobutton $cs.s -text "a Spreadsheet (CSV)" \
      -variable ::var(xport,$::tl_cnt,ef) -value "Spreadsheet" -anchor w
  radiobutton $cs.m -text "a Matlab file" \
      -variable ::var(xport,$::tl_cnt,ef) -value "Matlab" -anchor w
  radiobutton $cs.g -text "a Gnuplot file" \
      -variable ::var(xport,$::tl_cnt,ef) -value "Gnuplot" -anchor w
  if {"$arch" != ""} {
    set ::var(info,$::tl_cnt,src) $arch
  } else {
    $cs.a invoke
  }
  pack $cs.a $cs.s $cs.m $cs.g -side top -fill x
  pack $f.ef -side left -fill x -expand t

  trace variable ::var(xport,$::tl_cnt,ef) w camGUI::switchOptions
  set ::onb $f.onb
  iwidgets::notebook $f.onb -width 2.5i -height 1.2i -background [$f.ef cget -background]
  set nb [$f.onb add -label Archive]
  iwidgets::labeledframe $nb.o -labeltext "Options" -labelpos nw -ipadx 4 -ipady 4
  set cs [$nb.o childsite]
  frame $cs.o1
  iwidgets::entryfield $cs.o1.fsize -labeltext "file size" \
      -textvariable ::var(xport,$::tl_cnt,opt,fsize) -width 6 -validate numeric
  label $cs.o1.l -text "days"
  pack $cs.o1.fsize $cs.o1.l -side left -fill x
  pack $cs.o1 -side top -fill x
  frame $cs.o2
  iwidgets::entryfield $cs.o2.repeat -labeltext "repeat limit" \
      -textvariable ::var(xport,$::tl_cnt,opt,repeat) -width 6 -validate numeric
  label $cs.o2.l -text "seconds"
  pack $cs.o2.repeat $cs.o2.l -side left -fill x
  pack $cs.o2 -side top -fill x
  
  pack $nb.o -side left -expand t -fill both

  set nb [$f.onb add -label Spreadsheet]
  iwidgets::labeledframe $nb.o -labeltext "Options" -labelpos nw -ipadx 4 -ipady 4
  set cs [$nb.o childsite]
  checkbutton $cs.fill -text "fill columns w/ repeated values" \
      -variable ::var(xport,$::tl_cnt,opt,fill) -anchor w
  checkbutton $cs.text -text "include status information" \
      -variable ::var(xport,$::tl_cnt,opt,text) -anchor w
  pack $cs.fill $cs.text -side top -fill x
  frame $cs.o2
  iwidgets::entryfield $cs.o2.interpol -labeltext "interpolate values" \
      -textvariable ::var(xport,$::tl_cnt,opt,interpol) -width 6 -validate numeric
  label $cs.o2.l -text "seconds"
  pack $cs.o2.interpol $cs.o2.l -side left -fill x -expand t
  pack $cs.o2 -side left
  pack $nb.o -side left -expand t -fill both

  set nb [$f.onb add -label Gnuplot]
  iwidgets::labeledframe $nb.o -labeltext "Options" -labelpos nw -ipadx 4 -ipady 4
  set cs [$nb.o childsite]
  frame $cs.o2
  iwidgets::entryfield $cs.o2.reduce -labeltext "reduce data to" \
      -textvariable ::var(xport,$::tl_cnt,opt,reduce) -width 6 -validate numeric
  label $cs.o2.l -text "buckets"
  pack $cs.o2.reduce $cs.o2.l -side left -fill x
  pack $cs.o2 -side top -fill x
  pack $nb.o -side left -expand t -fill both

  set nb [$f.onb add -label Matlab]
  iwidgets::labeledframe $nb.o -labeltext "Options" -labelpos nw -ipadx 4 -ipady 4
  set cs [$nb.o childsite]
  checkbutton $cs.text -text "include status information" \
      -variable ::var(xport,$::tl_cnt,opt,text) -anchor w
  pack $cs.text -side top -fill x
  pack $nb.o -side left -expand t -fill both

  $f.onb select Archive
  pack $f.onb -side left -fill x -expand t

  pack $f -side top -fill x
  set f [file rootname $f]
  selFile $f.d "Select target" ::var(xport,$::tl_cnt,target) $initialdir archive Save
  combobox $f.rx "Regular Expression to match Channelnames" top \
      ::var(xport,$::tl_cnt,regex) [luniq $::selected(regex)]
  pack $f.rx -side top -fill x -padx 4 -pady 4
  frame $f.t -bd 0
  frame $f.t.f -bd 0
  frame $f.t.t -bd 0

  set p $f.t.f
  label $p.l -text From: -width 10 -anchor e
  iwidgets::dateentry $p.d -iq high
  iwidgets::timeentry $p.t -format military
  pack $p.l $p.d $p.t -side left
  set ::tf $p

  set p $f.t.t
  label $p.l -text To: -width 10 -anchor e
  iwidgets::dateentry $p.d -iq high
  iwidgets::timeentry $p.t -format military
  pack $p.l $p.d $p.t -side left
  set ::tt $p

  grid $f.t.f $f.t.t
  pack $f.t -fill both -expand t -side top

  $f.s.b invoke
  $f.d.b invoke
  while {[set res [handleDialog $f \
	      {("$::var(xport,$::tl_cnt,ef)" == "Archive") && ([file dirname $::var(xport,$::tl_cnt,src)] == [file dirname $::var(xport,$::tl_cnt,target)])} \
		       {"same directory!" "Source and Target must not be in the same directory!"}]]} {
    if {"$::var(xport,$::tl_cnt,ef)" == "Archive"} {
      set cmd ArchiveManager
      if {($::var(xport,$::tl_cnt,opt,fsize) != "") && 
	  ($::var(xport,$::tl_cnt,opt,fsize) > 0)} {
	lappend cmd -FileSize $::var(xport,$::tl_cnt,opt,fsize)
      }
      if {($::var(xport,$::tl_cnt,opt,repeat) != "") && 
	  ($::var(xport,$::tl_cnt,opt,repeat) > 0)} {
	lappend cmd -repeat_limit $::var(xport,$::tl_cnt,opt,repeat)
      }
    } else {
      set cmd ArchiveExport
      if {("$::var(xport,$::tl_cnt,ef)" == "Spreadsheet")} {
	if {($::var(xport,$::tl_cnt,opt,interpol) != "") && 
	    ($::var(xport,$::tl_cnt,opt,interpol) > 0)} {
	  lappend cmd -interpolate $::var(xport,$::tl_cnt,opt,interpol)
	}
	if {$::var(xport,$::tl_cnt,opt,fill)} { lappend cmd -fill }
      }
      if {("$::var(xport,$::tl_cnt,ef)" == "Gnuplot") && 
	  ($::var(xport,$::tl_cnt,opt,reduce) != "") && 
	  ($::var(xport,$::tl_cnt,opt,reduce) > 0)} {
	lappend cmd -reduce $::var(xport,$::tl_cnt,opt,reduce)
      } else {
	if {$::var(xport,$::tl_cnt,opt,text)} { lappend cmd -text }
      }
    }

    if {$res} {
      if {"$::var(xport,$::tl_cnt,ef)" == "Matlab"} {
	lappend cmd -Matlab
      } elseif {"$::var(xport,$::tl_cnt,ef)" == "Gnuplot"} {
	lappend cmd -gnuplot
      }
      if {"$::var(xport,$::tl_cnt,regex)" != ""} {
	lappend cmd -match $::var(xport,$::tl_cnt,regex)
      }
      lappend cmd -start "[$::tf.d get] [$::tf.t get]"
      lappend cmd -end "[$::tt.d get] [$::tt.t get]"
      if {"$::var(xport,$::tl_cnt,ef)" == "Archive"} {
	lappend cmd -xport "$::var(xport,$::tl_cnt,target)" "$::var(xport,$::tl_cnt,src)"
      } else {
	lappend cmd -output "$::var(xport,$::tl_cnt,target)" "$::var(xport,$::tl_cnt,src)" 
      }
      pXec $cmd $f.tf.t
    }
  }
  lappend ::selected(regex) $::var(xport,$::tl_cnt,regex)
  closeDialog $f
}

proc camGUI::getAInfoCB {arr ind mode} {
  if {![catch {lassign [getAInfo [lindex [array get $arr $ind] 1]] num first last}]} {
    if {$num > 0} {
      $::tf.d show $first
      $::tf.t show $first
      $::tt.d show $last
      $::tt.t show $last
    }
  }
  return 1
}

proc camGUI::getAInfo {fn} {
  for_file line "|ArchiveManager -info \"$fn\"" {
    regexp "Channel count : (\[0-9\]*)" $line all numChannels
    regexp "First sample  : (\[^\\.\]*)" $line all firstTime
    regexp "Last  sample  : (\[^\\.\]*)" $line all lastTime
  }
  return [list $numChannels [clock scan $firstTime] [clock scan $lastTime]]
}

proc camGUI::getCfgFile {parent row} {
  global var
  tk_getOpenFile -parent $parent \
      -initialdir [file dirname $var($row,cfg)] \
      -initialfile [file tail $var($row,cfg)] \
      -title "Select Configuration File"
}

proc camGUI::getPorts {row} {
  set ports {}
  foreach i [camMisc::arcIdx] {
    if {( $i != $row ) &&
	( ( $row == -1 ) || 
	  ( [camMisc::arcGet $row host] == [camMisc::arcGet $i host] ) ) } {
      lappend ports [camMisc::arcGet $i port]
    }
  }
  return $ports
}

proc camGUI::handleDialog {w {cond 0} {errmsg {"" ""}}} {
  wm protocol $w WM_DELETE_WINDOW "after 1 $w.bb.cancel invoke"
  $w.bb.cancel configure -text Close -command "set ::var($w,go) 0"  -state normal
  $w.bb.go configure -text Go -command "set ::var($w,go) 1"  -state normal
  while {1} {
    vwait ::var($w,go)
    if {$::var($w,go)} {
      if $cond {
	tk_messageBox -type ok -icon error -parent $w \
	    -title [lindex $errmsg 0] -message [lindex $errmsg 1]
      } else {
	$w.bb.cancel configure -state disabled
	$w.bb.go configure -text Abort -command "kill \[expr \$::var(xec,pid,$w.tf.t) + 1 \]" -state normal
	break
      }
    } else {
      break
    }
  }
  return $::var($w,go)
}

proc camGUI::aInfo {w} {
  set initialdir [pwd]
  set arch ""
  if [regexp (\[0-9\]*), [$w cursel] all row] {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
    set arch [clock format [clock seconds] -format $initialdir/[camMisc::arcGet $row archive]]
    if {![file exists $arch]} {set arch ""}
  }
  set f [actionDialog "Info Archive"]
  selFile $f.c "Select Archive to get Info about" ::var(info,$::tl_cnt,arc) $initialdir archive
  if {"$arch" != ""} {
    set ::var(info,$::tl_cnt,arc) $arch
  } else {
    $f.c.b invoke
  }
  while {[handleDialog $f]} {
    pXec {ArchiveManager -info $::var(info,$::tl_cnt,arc)} $f.tf.t
  }
  closeDialog $f
}

proc camGUI::initTable {table} {
  variable row
  variable aEngines
  array unset aEngines

  set ::busyIndicator "@"
  $table width 0 15 1 8 2 50 3 25 4 5
  lassign {0 1 2 3 4 5} ::iHost ::iPort ::iDescr ::iRun ::iBlocked

  set aEngines(-1,$::iHost) Host
  set aEngines(-1,$::iPort) Port
  set aEngines(-1,$::iDescr) Description
  set aEngines(-1,$::iRun) Running?
  set aEngines(-1,$::iBlocked) Block

  for {set i 0} {$i < [llength [camMisc::arcIdx]]} {incr i} {
    set aEngines($i,$::iHost) [camMisc::arcGet $i host]
    set aEngines($i,$::iPort) [camMisc::arcGet $i port]
    set aEngines($i,$::iDescr) [camMisc::arcGet $i descr]
    set aEngines($i,$::iRun) Dunno
    set aEngines($i,$::iBlocked) Dunno

    set camGUI::aEngines($i,$::iBlocked) \
	[lindex $::yesno [file exists [file join [file dirname [camMisc::arcGet $i cfg]] BLOCKED]]]
    after 1 "camComm::CheckRunning $i camGUI::aEngines($i,$::iRun)"
  }
  for {set i [llength [camMisc::arcIdx]]} {$i < $row} {incr i} {
    catch {destroy $table.f.f$i}
  }
  $table configure -rows $row
  setDT
  after 500 {set ::busyIndicator ""}
}

proc camGUI::mainWindow {} {
  variable aEngines
  variable row
  wm protocol . WM_DELETE_WINDOW "after 1 camGUI::Exit"
  set descmenu {
    "&File" all file 0 {
      {command "&New..." {} "New Configuration" {} -command {camGUI::aOpen Save}}
      {command "&Open..." {} "Open Configuration" {} -command camGUI::aOpen}
      {command "Open D&efault" {} "Open default Configuration" {} -command camGUI::aOpenDefault}
      {separator}
      {command "&Revert" {} "Revert to last saved status" {} -command camGUI::reOpen}
      {separator}
      {command "&Save" {} "Save Configuration" {} -command camGUI::aSave}
      {command "Save &as..." {} "Save Configuration as..." {} -command camGUI::aSaveAs}
      {command "Save as &Default" {} "Save Configuration as Default" {} -command camGUI::aSaveAsDefault}
      {separator}
      {command "E&xit" {} "Exit ArchiveManager" {} -command camGUI::Exit}
    }
    "&Edit" all edit 0 {
      {command "&Preferences" {} "Edit global preferences" {Ctrl p} -command {camGUI::aPrefs}}
    }
    "&Tools" all tools 0 {
      {command "&Info" {} "Info on Archive" {Ctrl i} -command {camGUI::aInfo .tf.t}}
      {command "&Test" {} "Test an Archive" {Ctrl t} -command {camGUI::aTest .tf.t}}
      {command "E&xport" {} "Export an Archive" {Ctrl x} -command {camGUI::aExport .tf.t}}
      {command "&Modify" {} "Copy/Delete Channels in an Archive" {Ctrl m} -command {camGUI::aModArchive .tf.t}}
    }
    "&Help" all help 0 {
      {command "&About" {} "Version Info" {Ctrl a} -command {camGUI::aAbout}}
    }
  }
  set ::busyIndicator ""
  set mainframe [MainFrame .mainframe -menu $descmenu -textvariable ::status ]
  $mainframe addindicator -textvariable camGUI::datetime
  $mainframe addindicator -textvariable camMisc::cfg_file_tail
  $mainframe addindicator -textvariable ::busyIndicator -width 2 -foreground blue
  pack $mainframe -side bottom -fill both -expand no

  set f [frame .tf -bd 0]

  scrollbar $f.sy -command "$f.t yview"
  pack $f.sy -side right -fill y

  set row [expr max([llength [camMisc::arcIdx]]+2, 8)]

  set table [table $f.t -rows $row -cols 5 -titlerows 1 \
		 -rowheight -22 \
		 -colstretchmode all -rowstretchmode none -roworigin -1 \
		 -yscrollcommand "$f.sy set" -flashmode 1 \
		 -borderwidth 1 -state disabled -selecttype row]
  pack $table -side right -expand t -fill both
  frame $table.f -bd 0
  frame $table.f.b -bd 1 -relief sunken
  pack $table.f.b -side bottom -expand t -fill both


  initTable $table
  $table config -variable camGUI::aEngines 

  set brel raised
  set f [frame .bf -bd 0]
  Button $f.start -text Start -bd 1 -relief $brel \
      -command "camGUI::aStart $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "start the selected Archiver"
  Button $f.stop -text Stop -bd 1 -relief $brel \
      -command "camGUI::aStop $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "stop the selected Archiver (may restart if not blocked)"
  Button $f.new -text New -bd 1 -relief $brel \
      -command "camGUI::aNew $table" \
      -helptype variable -helpvar ::status \
      -helptext "create a new Archiver"
  Button $f.delete -text Delete -bd 1 -relief $brel \
      -command "camGUI::aDelete $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "delete the selected Archiver (without stopping it))"
  Button $f.edit -text Edit -bd 1 -relief $brel \
      -command "camGUI::aEdit $table" -state disabled \
      -helptype variable -helpvar ::status \
      -helptext "edit properties of selected Archiver (restricted if Archiver is runnung)"
  Button $f.check -text Check -bd 1 -relief $brel \
      -command "camGUI::aCheck $table" \
      -helptype variable -helpvar ::status \
      -helptext "manually check which Archivers are running"
  ArrowButton $f.up -type button -dir top -bd 1 \
      -command "camGUI::aSwap up $table" \
      -helptype variable -helpvar ::status \
      -helptext "move selected Archiver up in list" \
      -width 25 -height 25 -ipadx 4 -ipady 4
  ArrowButton $f.down -type button -dir bottom -bd 1 \
      -command "camGUI::aSwap down $table" \
      -helptype variable -helpvar ::status \
      -helptext "move selected Archiver down in list" \
      -width 25 -height 25 -ipadx 4 -ipady 4

  pack $f.delete $f.new $f.edit $f.stop $f.start $f.check $f.down $f.up -padx 8 -pady 8 -side right

#  event add <<B1>> <1>
#  event add <<B1>> <Button1-Motion>
  bind $table <<B1>> {
    camGUI::mouseSelect %W %x %y
    break
  }

  bind $table <3> {
    if {[camGUI::mouseSelect %W %x %y]} {
      catch {destroy %W.m}
      menu %W.m -tearoff 0
      %W.m add command -label Edit -command {camGUI::aEdit %W}
      %W.m add separator
      %W.m add command -label "Block/Unblock" -command {camGUI::toggleBlock %W}
      %W.m add command -label "Check if running" -command {camGUI::aCheck %W}
      %W.m add command -label "View/Edit configuration" -command {camGUI::aConfig %W}
      %W.m add separator
      %W.m add command -label Delete -command {camGUI::aDelete %W}
      after 1 {tk_popup %W.m %X %Y}
    }
    break
  }
  bind $table <Button3-Motion> {break}

  bind $table <Control-i> { camGUI::aInfo %W; break }
  bind $table <Control-t> { camGUI::aTest %W; break }
  bind $table <Control-x> { camGUI::aExport %W; break }
  bind $table <ButtonRelease-1> { camGUI::setButtons %W }
  bind $table <Up> { camGUI::scrollSel %W -1 ; break }
  bind $table <Down> { camGUI::scrollSel %W 1 ; break }
  bind $table <Right> { break }
  bind $table <Left> { break }

  pack .bf -side bottom -fill x
  pack .tf -side top -expand t -fill both

  bind $table <Double-Button-1> "$f.edit invoke"
  bind $table <Delete> "$f.delete invoke"

  BWidget::place . 0 0 center
  wm deiconify .
  raise .
  focus -force .
  wm geom . [wm geom .]
  setButtons $table
}

proc camGUI::MessageBox {icon title head txt tail buttons parent} {
  toplevel .msgbox
  wm title .msgbox $title
  label .msgbox.xxx
  entry .msgbox.yyy
  frame .msgbox.cf -bd 4
  pack .msgbox.cf -fill both -expand t
  set mb [frame .msgbox.cf.frame]
  if {$icon == "error"} {
    .msgbox.cf configure -bg red
  }
  if {$icon == "warning"} {
    .msgbox.cf configure -bg yellow
  }
  set ::msgbox(answer) [lindex $buttons 0]
  frame $mb.buttons
  set bn 0
  foreach b $buttons {
    set B $mb.buttons.b$bn
    button $B -bd 1 -text $b -command "set ::msgbox(answer) \"$b\"; destroy .msgbox"
    pack $B -side right -padx 4 -pady 4
    incr bn
  }
  pack $mb.buttons -side bottom -fill x
  pack [frame $mb.sep1 -bg black] -side bottom -fill x -padx 8 -pady 8

  frame $mb.text
  if {$head != ""} {
    foreach l [split [string trimright $head] "\n"] {
      set L $mb.text.l$bn
      label $L -text $l -anchor w
      pack $L -side top -fill x
      incr bn
    }
    pack [frame $mb.text.sep1 -bg black] -side top -fill x -padx 8 -pady 8
  }
  foreach l [split [string trimright $txt] "\n"] {
    set L $mb.text.l$bn
    label $L -text $l -anchor w -font [.msgbox.yyy cget -font]
    pack $L -side top -fill x
    incr bn
  }
  if {$tail != ""} {
    pack [frame $mb.text.sep2 -bg black] -side top -fill x -padx 8 -pady 8
    foreach l [split [string trimright $tail] "\n"] {
      set L $mb.text.l$bn
      label $L -text $l -anchor w
      pack $L -side top -fill x
      incr bn
    }
  }

  pack $mb.text -side right -fill both -expand t -padx 8 -pady 8

  label $mb.bitmap -bitmap $icon
  pack $mb.bitmap -side left -padx 4 -pady 4
  pack $mb -fill both -expand t

  wm withdraw .msgbox
  update idletasks
  regexp "(.*)x(.*)\\+(.*)\\+(.*)" [winfo geometry $parent] all pw ph px py
  set cw [winfo reqwidth $mb]
  set ch [winfo reqheight $mb]
  wm geometry .msgbox "+[expr $px + ( $pw - $cw ) / 2 ]+[expr $py + ( $ph - $ch ) / 2 ]"
  wm deiconify .msgbox

  tkwait window .msgbox
  return $::msgbox(answer)
}

proc camGUI::mkChanList {w c v i m} {
  if [file exists $::var(info,$c,arc)] {
    regsub -all "\n\n" [read_file "|ArchiveManager -match . \"$::var(info,$c,arc)\""] "\n" lst
    set l [lrmdups [split [string trim $lst] "\n"]]
    [$w.rename getframe].from.e configure -values $l
    [$w.del getframe].delete.e configure -values $l
  }
}

proc camGUI::aModArchive {w} {
  set initialdir [pwd]
  set arch ""
  if [regexp (\[0-9\]*), [$w cursel] all row] {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
    set arch [clock format [clock seconds] -format $initialdir/[camMisc::arcGet $row archive]]
    if {![file exists $arch]} {set arch ""}
  }
  set f [actionDialog "Modify Archive"]
  selFile $f.c "Select Archive to modify" ::var(info,$::tl_cnt,arc) $initialdir archive

  trace variable ::var(info,$::tl_cnt,arc) w "camGUI::mkChanList $f $::tl_cnt"

  label $f.l

  TitleFrame $f.rename -text "Rename a Channel" -font [$f.l cget -font]
  set c [$f.rename getframe]
  combobox $c.from "rename channel " left ::var(info,$::tl_cnt,cpfrm) {}
  entrybox $c.to "to" left ::var(info,$::tl_cnt,cpto)
  pack $c.from $c.to -side left -padx 8 -pady 4
  pack $f.rename -side top -fill x -padx 4 -pady 4

  TitleFrame $f.del -text "Delete a Channel" -font [$f.l cget -font]
  set d [$f.del getframe]
  combobox $d.delete "delete channel " left ::var(info,$::tl_cnt,delete) {}
  pack $d.delete -side left -padx 8 -pady 4
  pack $f.del -side top -fill x -padx 4 -pady 4

  if {"$arch" != ""} {
    set ::var(info,$::tl_cnt,arc) $arch
  } else {
    $f.c.b invoke
  }
  while {[handleDialog $f]} {
    if {"::var(info,$::tl_cnt,arc)" != ""} {
      if {("$::var(info,$::tl_cnt,cpfrm)" != "") &&
	  ("$::var(info,$::tl_cnt,cpto)" != "")} {
	pXec {ArchiveManager -Rename $::var(info,$::tl_cnt,cpto) \
		  -channel $::var(info,$::tl_cnt,cpfrm) $::var(info,$::tl_cnt,arc)} $f.tf.t
      }
      if {("$::var(info,$::tl_cnt,delete)" != "")} {
	pXec {ArchiveManager -DELETE $::var(info,$::tl_cnt,delete) \
		  $::var(info,$::tl_cnt,arc)} $f.tf.t
      }
    }
  }
  closeDialog $f
}

proc camGUI::mouseSelect {w x y} {
  set oldsel [$w cursel]
  $w selection clear all
  $w selection set @$x,$y
  set cursel [$w cursel]
  if {$oldsel == $cursel} {
#    $w selection clear all
#    return 0
    return 1
  }
  regexp (\[0-9\]*), $cursel all row
  if {$row > [expr [llength [camMisc::arcIdx]] - 1]} {
    $w selection clear all
    return 0
  }
  return 1
}

proc camGUI::aNew {w} {
  set row end
  set dir 1
  if [regexp (\[0-9\]*), [$w cursel] all row] {set dir -1}

  set nrow [camMisc::arcNew $row]
  $w configure -state normal
  $w insert row -- $row $dir
  set row $nrow
  if {[expr [.tf.t cget -rows] - 2] > [llength [camMisc::arcIdx]]} {
    $w delete row [expr [.tf.t cget -rows] - 2]
  }

  set camGUI::aEngines($row,$::iHost) "$::_host"
  set ::var($row,port) 4711
  set ::ports [getPorts -1]
  set camGUI::aEngines($row,$::iPort) "$::var($row,port)"
  set camGUI::aEngines($row,$::iDescr) "<enter description>"
  set camGUI::aEngines($row,$::iRun) ""

  camMisc::arcSet $row host $camGUI::aEngines($row,$::iHost)
  camMisc::arcSet $row port $camGUI::aEngines($row,$::iPort)
  camMisc::arcSet $row descr $camGUI::aEngines($row,$::iDescr)
  camMisc::arcSet $row cfg [file join [pwd] "ENTER FILENAME"]
  camMisc::arcSet $row cfgc 0
  camMisc::arcSet $row archive "<enter filename>"
  camMisc::arcSet $row log "<enter filename>"
  camMisc::arcSet $row start NO

  $w configure -state disabled

  update
  
  $w selection clear all
  $w selection set $row,0
  if {![aEdit $w]} {
    $w configure -state normal
    $w delete row $row
    $w configure -state disabled
    camMisc::arcDel $row
  }
  setButtons $w
}

proc camGUI::aOpen {{mode Open}} {
  # open a fileselector
  # if there's a file selected -> open it
  if {$mode == "Save"} {set ::Archivers {}}
  if {[info exists camMisc::rcdir]} {cd $camMisc::rcdir}
  set fn [tk_get${mode}File]
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

proc camGUI::packTree {w tree} {
  foreach item $tree {
    lassign $item type name options packopt sub var
    regsub "(.)\\..*" $w "\\1" P
    regsub -all "%w" $options $w.$name options
    regsub -all "%P" $options $P options
    regsub -all "%p" $options $w options
    regsub -all "%w" $var $w.$name var
    regsub -all "%P" $var $P var
    regsub -all "%p" $var $w var
    if {$var != {}} {set $var $w.$name}
    eval $type $w.$name $options
    set W $w.$name
    if [regexp "TitleFrame" $type] {
      set W [$W getframe]
    }
    packTree $W $sub
    eval pack $w.$name $packopt
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

  tkwait window .prefs

  if {"$::_port" != "$::_oport"} {
    SaveSettings
    tk_dialog .confirm "Notice" "Changes will take effect after restart of CAbgManager!" warning 0 Close
  }
}

proc camGUI::eatInput {fd w {mode normal}} {
  if {![catch {set line [read $fd]}]} {
    if {[regexp "TeRmInAtEd" $line]} {
      $w configure -state normal
      $w insert end "\nReady.\n" command
      $w configure -state disabled
      $w yview end
      set ::var(terminated,$w) 1
    } else {
      $w configure -state normal
      $w insert end "$line" $mode
      $w configure -state disabled
      $w yview end
    }
  }
}

proc camGUI::pXec {cmd w} {
  pipe rd wr
  pipe ed er
  fileevent $rd readable "camGUI::eatInput $rd $w"
  fileevent $ed readable "camGUI::eatInput $ed $w error"
  fconfigure $rd -blocking 0
  fconfigure $ed -blocking 0
  fconfigure $wr -buffering none
  fconfigure $er -buffering none
  $w configure -state normal
#  regsub -all "/\[^ \]*/" "$cmd" ".../" c
  eval $w insert end "\"$cmd\n\"" command
  $w configure -state disabled
  if [regexp ">" $cmd] {
    catch {set ::var(xec,pid,$w) [eval exec [info nameofexecutable] $::INCDIR/xec.tcl $cmd 2>@$er &]}
  } else {
    catch {set ::var(xec,pid,$w) [eval exec [info nameofexecutable] $::INCDIR/xec.tcl $cmd >@$wr 2>@$er &]}
  }
  set ::var(xec,fh,$w) [list $wr $er $rd $ed]
  vwait ::var(terminated,$w)
}
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
  if {$fh != -1} { 
    close $wh 
    file rename -force $camMisc::cfg_file.N $camMisc::cfg_file
  }
  set ::settingsSaved 1
}

proc camGUI::save {wh var val desc {p ""}} {
  if {"$::tcl_platform(platform)" == "unix"} {
    puts $wh "set $var {$val}"
  } else {
    registry set "$camMisc::reg_stem\\Settings$p" $desc $val
  }
}

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

proc camGUI::scrollSel {w i} {
  set sel [$w cursel]
  if {[llength $sel] == 0} {
    if {$i > 0} {
      set s 0
    } else {
      set s [expr [llength [camMisc::arcIdx]] -1]
    }
    for {set i 0} {$i < [llength [camMisc::arcIdx]]} {incr i} {
      $w selection set $s,$i
    }
  } else {
    regsub ",.*" [lindex $sel 0] "" s
    set os $s
    incr s $i
    if {($s < 0) || ($s >= [llength [camMisc::arcIdx]])} return
    $w selection clear all
    regsub -all -- "$os," $sel "$s," sel
    foreach s $sel {
      $w selection set $s
    }
  }
  camGUI::setButtons $w
}

proc camGUI::Select {w row} {
  $w selection clear all
  if {$row >= [llength [camMisc::arcIdx]]} {return}

  for {set i 0} {$i < [$w cget -cols]} {incr i} {
    $w selection set $row,$i
  }

  setButtons $w
}

proc camGUI::selFile {w label var initialdir type {mode Open}} {
  frame $w -bd 0
  if {[llength $::selected($type)] == 0} {
    foreach k [camMisc::arcIdx] {
      if {"$type" != "misc"} {
	set a [file dirname [camMisc::arcGet $k cfg]]/[camMisc::arcGet $k $type]
	set a [clock format [clock seconds] -format $a]
	lappend ::selected($type) $a
      }
    }
  }
  combobox $w.e "$label" top $var [luniq $::selected($type)] -side left -fill x -expand t
  button $w.b -text "..." -padx 0 -pady 0 -width 2 -bd 1 -command "
      cd \"$initialdir\"
      set fn \[tk_get${mode}File -initialdir \[file dirname \$$var\] -initialfile \[file tail \$$var\] -parent [file rootname $w] -title \"$label\"\]
      if {\"\$fn\" != \"\"} {set $var \$fn; lappend ::selected($type) \$fn}"

  pack $w.e -side left -fill x -expand t
  pack $w.b -side bottom
  pack $w -side top -fill x -padx 4 -pady 4
}

proc camGUI::setButtons {w} {
  set topf [file rootname [file rootname $w]]
  set row [expr [$w cget -rows] - 3]
  regexp (\[0-9\]*), [$w cursel] all row
  if {([llength [$w cursel]] == 0) || (![info exists camGUI::aEngines($row,$::iRun)]) } {
    $topf.bf.start configure -state disabled
    $topf.bf.stop configure -state disabled
    $topf.bf.edit configure -state disabled
    $topf.bf.delete configure -state disabled
    $topf.bf.up configure -state disabled
    $topf.bf.down configure -state disabled
    return
  }
  if {[regexp "since" $camGUI::aEngines($row,$::iRun)]} {
    $topf.bf.start configure -state disabled
#    $topf.bf.edit configure -state disabled
    $topf.bf.stop configure -state normal
  } else {
    $topf.bf.start configure -state normal
#    $topf.bf.edit configure -state normal
    $topf.bf.stop configure -state disabled
  }
  if {$camGUI::aEngines($row,$::iHost) != "$::_host"} {
    $topf.bf.start configure -state disabled
  }
  if {$row > 0} {
    $topf.bf.up configure -state normal
  } else {
    $topf.bf.up configure -state disabled
  }
  if {$row < [expr [$w cget -rows] - 4]} {
    $topf.bf.down configure -state normal
  } else {
    $topf.bf.down configure -state disabled
  }
  $topf.bf.edit configure -state normal
  $topf.bf.delete configure -state normal
}

proc camGUI::setDT {} {
  variable datetime [clock format [clock seconds] -format " %m/%d/%Y %H:%M:%S "]
  after 1000 camGUI::setDT
}

proc camGUI::aStart {w} {
  regexp (\[0-9\]*), [$w cursel] all row

  set host [camMisc::arcGet $row host]
  if {"$host" != "$::_host"} {return}

  runArchiver $row 1
  after 5000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
  ClearSel $w
}

proc camGUI::aStop {w} {
  regexp (\[0-9\]*), [$w cursel] all row

  stopArchiver $row 1

  after 2000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
}

proc camGUI::aSwap {dir w} {
  regexp (\[0-9\]*), [$w cursel] all row
  if {$dir == "up"} {
    set with [expr $row - 1]
    set sel $with
  } else {
    if {$row == 0} {
      set row 1; set with 0
      set sel 1
    } else {
      set with [expr $row + 1]
      set sel $with
    }
  }

  

  set b1 $camGUI::aEngines($row,$::iBlocked)
  set r1 $camGUI::aEngines($row,$::iRun)
  set b2 $camGUI::aEngines($with,$::iBlocked)
  set r2 $camGUI::aEngines($with,$::iRun)

  set m [lindex $camMisc::_Archiver $row]
  set a [lreplace $camMisc::_Archiver $row $row]
  set camMisc::_Archiver [linsert $a $with $m]

  $w configure -state normal -flashmode 0
  $w delete row $row
  $w insert row -- $with -1

  set camGUI::aEngines($with,$::iHost) "[camMisc::arcGet $with host]"
  set camGUI::aEngines($with,$::iPort) "[camMisc::arcGet $with port]"
  set camGUI::aEngines($with,$::iDescr) "[camMisc::arcGet $with descr]"
  set camGUI::aEngines($with,$::iRun) "[camMisc::arcGet $with run]"

  set camGUI::aEngines($with,$::iBlocked) $b1
  set camGUI::aEngines($with,$::iRun) $r1
  set camGUI::aEngines($row,$::iBlocked) $b2
  set camGUI::aEngines($row,$::iRun) $r2

  $w configure -state disabled -flashmode 1
  Select $w $sel
}

proc camGUI::switchOptions {a b c} {
  $::onb select $::var(xport,$::tl_cnt,ef)
}

proc camGUI::aTest {w} {
  set initialdir [pwd]
  set arch ""
  if {[regexp (\[0-9\]*), [$w cursel] all row]} {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
    set arch [clock format [clock seconds] -format $initialdir/[camMisc::arcGet $row archive]]
    if {![file exists $arch]} {set arch ""}
  }
  set f [actionDialog "Test Archive"]
  selFile $f.c "Select Archive to Test" ::var(test,$::tl_cnt,arc) $initialdir archive
  if {"$arch" != ""} {
    set ::var(info,$::tl_cnt,arc) $arch
  } else {
    $f.c.b invoke
  }
  while {[handleDialog $f]} {
    pXec {ArchiveManager -test $::var(test,$::tl_cnt,arc)} $f.tf.t
  }
  closeDialog $f
}

proc camGUI::toggleBlock {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  set stat [lsearch $::yesno $camGUI::aEngines($row,$::iBlocked)]
  set stat [expr 1 - $stat]
  if {$stat} {
    camMisc::Block $row camGUI::aEngines($row,$::iBlocked)
  } else {
    camMisc::Release $row camGUI::aEngines($row,$::iBlocked)
  }
}
