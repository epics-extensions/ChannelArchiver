proc camGUI::initTable {table} {
  variable row
  variable aEngines
  array unset aEngines

  set ::busyIndicator "@"
  $table width 0 15 1 8 2 50 3 25 4 5
  lassign {0 1 2 3 4 5} ::iHost ::iPort ::iDescr ::iRun ::iBlocked

  set aEngines(-1,$::iHost) Host
  set aEngines(-1,$::iPort) Port
  set aEngines(-1,$::iDescr) Description
  set aEngines(-1,$::iRun) Running?
  set aEngines(-1,$::iBlocked) Block

  for {set i 0} {$i < [llength [camMisc::arcIdx]]} {incr i} {
    set aEngines($i,$::iHost) [camMisc::arcGet $i host]
    set aEngines($i,$::iPort) [camMisc::arcGet $i port]
    set aEngines($i,$::iDescr) [camMisc::arcGet $i descr]
    set aEngines($i,$::iRun) Dunno
    set aEngines($i,$::iBlocked) Dunno

    set camGUI::aEngines($i,$::iBlocked) [lindex $::yesno [file exists [file dirname [camMisc::arcGet $i cfg]]/BLOCKED]]
    after 1 "camComm::CheckRunning $i camGUI::aEngines($i,$::iRun)"
  }
  for {set i [llength [camMisc::arcIdx]]} {$i < $row} {incr i} {
    catch {destroy $table.f.f$i}
  }
  $table configure -rows $row
  setDT
  after 500 {set ::busyIndicator ""}
}
