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
  trace variable ::var(xport,$::tl_cnt,src) w getAInfoCB
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

  trace variable ::var(xport,$::tl_cnt,ef) w switchOptions
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
