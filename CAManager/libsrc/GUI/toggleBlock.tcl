proc toggleBlock {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  set stat [lsearch $::yesno $camGUI::aEngines($row,$::iBlocked)]
  set stat [expr 1 - $stat]
  if {$stat} {
    camMisc::Block $row camGUI::aEngines($row,$::iBlocked)
  } else {
    camMisc::Release $row camGUI::aEngines($row,$::iBlocked)
  }
}
