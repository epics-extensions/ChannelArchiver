proc camComm::Close {sock} {
  global fstate fsto
#  fileevent $sock readable ""
  catch {after cancel $fsto($sock)}
#  array unset fstate $sock
  set fstate($sock) closed
  close $sock
  incr ::openSocks -1
}
