proc camGUI::aBlock {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  camMisc::Block $row camGUI::aEngines($row,$::iBlocked)
  camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
  setButtons $w
}

proc camGUI::aRelease {w} {
  regexp (\[0-9\]*), [$w cursel] all row
  camMisc::Release $row camGUI::aEngines($row,$::iBlocked)
  camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
  setButtons $w
}
