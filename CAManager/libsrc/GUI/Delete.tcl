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
