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

    set camGUI::aEngines($i,$::iBlocked) [file exists [file dirname [camMisc::arcGet $i cfg]]/BLOCKED]
    catch {destroy $table.f$i}
    frame $table.f$i -bd 1 -relief sunken
    label $table.f$i.l -text " "
    checkbutton $table.f$i.c -variable camGUI::aEngines($i,$::iBlocked) \
	-command "toggleBlock $i"
    $table.f$i.c config -activebackground [$table.f$i.c cget -background]
    bind $table.f$i.c <Enter> {
      set ::status "inhibit restart of ArchiveEngine"
    }
    bind $table.f$i.c <Leave> {
      set ::status ""
    }
    pack $table.f$i.l -side left
    pack $table.f$i.c -fill both -expand t
    $table window config $i,4 -sticky news -window $table.f$i

    after 1 "camComm::CheckRunning $i camGUI::aEngines($i,$::iRun)"
  }
  for {set i [llength [camMisc::arcIdx]]} {$i < $row} {incr i} {
    catch {destroy $table.f$i}
  }
  $table configure -rows $row
  setDT
  after 500 {set ::busyIndicator ""}
}
