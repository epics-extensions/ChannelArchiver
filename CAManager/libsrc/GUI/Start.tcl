proc camGUI::aStart {w} {
  regexp (\[0-9\]*), [$w cursel] all row

  set host [camMisc::arcGet $row host]
  if {"$host" != "$::_host"} {return}

  runArchiver $row 1
  after 5000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
  ClearSel $w
}
