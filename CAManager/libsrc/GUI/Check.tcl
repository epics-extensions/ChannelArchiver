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
