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
