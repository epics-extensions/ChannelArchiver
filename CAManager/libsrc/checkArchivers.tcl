proc checkArchivers {args} {
  trace vdelete ArchiversSet w {after 1 checkArchivers}
  after 3000 checkArchivers
  foreach i [camMisc::arcIdx] {
    set ::_run($i) NO
    if {"[camMisc::arcGet $i host]" != "$::_host"} continue
#    if {"[camMisc::arcGet $i start]" == "NO"} continue
    if {![info exist ::_run($i)] || ![regexp "^since" $::_run($i)]} {set ::_run($i) -}
    trace variable ::_run($i) w checkstate
    camComm::CheckRunning $i ::_run($i)
  }
}
