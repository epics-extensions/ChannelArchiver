proc camComm::processTimeout {sock} {
  global fstate fsvar fsto
  if {"$fstate($sock)" != "closed"} {
    fileevent $sock readable ""
    condSet $fsvar($sock) "TIMEOUT"
    close $sock
    set fstate($sock) closed
  }
  array unset fstate $sock
  update
}
