proc checkJob {} {
  after [expr $::checkInt * 1000] checkJob
  if {$::dontCheckAtAll} return
  set ::busyIndicator "@"
  for {set row 0} {$row < [llength [camMisc::arcIdx]]} {incr row} {
#    puts "setting camGUI::aEngines($row,$::iBlocked) to [file exists [file dirname [camMisc::arcGet $row cfg]]/BLOCKED]"
#    puts "  filename was [file dirname [camMisc::arcGet $row cfg]]/BLOCKED"
    set camGUI::aEngines($row,$::iBlocked) \
	[file exists [file dirname [camMisc::arcGet $row cfg]]/BLOCKED]
    after 10 camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)
    update
  }
#  puts ""
  after 500 {set ::busyIndicator ""}
}
