proc camGUI::aStop {w} {
  regexp (\[0-9\]*), [$w cursel] all row

  if [catch {set sock [socket [camMisc::arcGet $row host] [camMisc::arcGet $row port]]}] {
    return
  }
  puts $sock "GET /stop HTTP/1.0"
  puts $sock ""
  close $sock

  after 2000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
}
