proc checkJob {} {
  after [expr $::checkInt * 1000] checkJob
  if {$::dontCheckAtAll} return
  set ::busyIndicator "@"
  for {set row 0} {$row < [llength [camMisc::arcIdx]]} {incr row} {
    set camGUI::aEngines($row,$::iBlocked) \
	[file exists [file dirname [camMisc::arcGet $row cfg]]/BLOCKED]
    after 10 camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
    update
  }
  after 500 {set ::busyIndicator ""}
}