proc checkArchivers {args} {
  trace vdelete ArchiversSet w {after 1 checkArchivers}
  after [expr 1000 * $::bgCheckInt] checkArchivers
  foreach i [camMisc::arcIdx] {
    if {![info exists ::_run($i)]} {set ::_run($i) NO}
    if {"[camMisc::arcGet $i host]" != "$::_host"} continue
#    if {![regexp "^since" $::_run($i)]} {set ::_run($i) -}
    trace variable ::_run($i) w checkstate
    camComm::CheckRunning $i ::_run($i)
  }
}
