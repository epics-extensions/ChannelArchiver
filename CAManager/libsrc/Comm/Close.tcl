proc camComm::Close {sock} {
  global fstate fsto
  fileevent $sock readable ""
  after cancel $fsto($sock)
#  array unset fstate $sock
  set fstate($sock) closed
  close $sock
}
