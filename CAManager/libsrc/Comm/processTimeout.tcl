proc camComm::processTimeout {sock} {
  global fstate fsvar fsto
  if {"$fstate($sock)" != "closed"} {
    fileevent $sock readable ""
    condSet $fsvar($sock) "TIMEOUT"
    Close $sock
  }
  array unset fstate $sock
  update
}
