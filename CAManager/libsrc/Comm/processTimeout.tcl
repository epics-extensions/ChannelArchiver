proc camComm::processTimeout {sock} {
  global fstate fsvar fsto
  if {"$fstate($sock)" != "closed"} {
    condSet $fsvar($sock) "TIMEOUT"
    set fstate($sock) closed
    camComm::Close $sock
  }
#  array unset fstate $sock
  update
}
