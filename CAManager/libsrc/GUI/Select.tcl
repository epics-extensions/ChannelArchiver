proc camGUI::Select {w row} {
  $w selection clear all
  if {$row >= [llength [camMisc::arcIdx]]} {return}

  for {set i 0} {$i < [$w cget -cols]} {incr i} {
    $w selection set $row,$i
  }

  setButtons $w
}
