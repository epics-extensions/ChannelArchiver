proc camGUI::aStop {w} {
  regexp (\[0-9\]*), [$w cursel] all row

  stopArchiver $row 1

  after 2000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
}
