proc stopArchiver {i {forceStop 0}} {
  array unset ::sched $i,stop,*
  if {!$forceStop && [file exists [file dirname [camMisc::arcGet $i cfg]]/BLOCKED]} {
    Puts "stop of \"[camMisc::arcGet $i descr]\" blocked" error
    return
  }
  Puts "stop \"[camMisc::arcGet $i descr]\"" command
  if {![catch {set sock [socket [camMisc::arcGet $i host] [camMisc::arcGet $i port]]}]} {
    incr ::openSocks
    puts $sock "GET /stop HTTP/1.0"
    puts $sock ""
    after 300 {set ::pipi 0}
    vwait ::pipi
    close $sock
    incr ::openSocks -1
  }
}
