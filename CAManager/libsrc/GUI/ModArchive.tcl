proc mkChanList {w c v i m} {
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

  trace variable ::var(info,$::tl_cnt,arc) w "mkChanList $f $::tl_cnt"

  label $f.l

  TitleFrame $f.rename -text "Rename a Channel" -font [$f.l cget -font]
  set c [$f.rename getframe]
  combobox $c.from "rename channel" left ::var(info,$::tl_cnt,cpfrm) {}
  entrybox $c.to "to" left ::var(info,$::tl_cnt,cpto)
  pack $c.from $c.to -side left -padx 8 -pady 4
  pack $f.rename -side top -fill x -padx 4 -pady 4

  TitleFrame $f.del -text "Delete a Channel" -font [$f.l cget -font]
  set d [$f.del getframe]
  combobox $d.delete "delete channel" left ::var(info,$::tl_cnt,delete) {}
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
