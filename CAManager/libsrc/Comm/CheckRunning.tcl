proc camComm::CheckRunning {i rvar} {
  global fstate fsvar fsto

  set start [clock seconds]
  if [catch {set sock [socket [camMisc::arcGet $i host] [camMisc::arcGet $i port]]}] {
    condSet $rvar "NO"
    return
  }

  puts $sock "GET / HTTP/1.0"
  puts $sock ""
  flush $sock

  set fsvar($sock) $rvar
  set fstate($sock) open
  fileevent $sock readable "camComm::processAnswer $sock"
  set fsto($sock) [after 5000 "camComm::processTimeout $sock"]
}
