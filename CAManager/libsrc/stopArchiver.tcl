proc stopArchiver {i} {
  array unset ::sched $i,stop,*
  if {[file exists [file dirname [camMisc::arcGet $i cfg]]/BLOCKED]} {
    Puts "stop of \"[camMisc::arcGet $i descr]\" blocked" error
    return
  }
  Puts "stop \"[camMisc::arcGet $i descr]\"" command
  if {![catch {set sock [socket [camMisc::arcGet $i host] [camMisc::arcGet $i port]]}]} {
    puts $sock "GET /stop HTTP/1.0"
    puts $sock ""
    close $sock
  }
}
