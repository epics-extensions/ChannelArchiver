proc camGUI::aTest {w} {
  set initialdir [pwd]
  if [regexp (\[0-9\]*), [$w cursel] all row] {
    set initialdir [file dirname [camMisc::arcGet $row cfg]]
  }
  set f [actionDialog "Test Archive"]
  selFile $f.c "Select Archive to Test" ::var(test,$::tl_cnt,arc) $initialdir archive
  $f.c.b invoke
  while {[handleDialog $f]} {
    pXec {ArchiveManager -test $::var(test,$::tl_cnt,arc)} $f.tf.t
  }
  closeDialog $f
}
