proc camGUI::aInfo {w} {
  set initialdir [pwd]
  if [regexp (\[0-9\]*), [$w cursel] all row] {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
  }
  set f [actionDialog "Info Archive"]
  selFile $f.c "Select Archive to get Info about" ::var(info,$::tl_cnt,arc) $initialdir archive
  $f.c.b invoke
  while {[handleDialog $f]} {
    pXec {ArchiveManager -info $::var(info,$::tl_cnt,arc)} $f.tf.t
  }
  closeDialog $f
}
