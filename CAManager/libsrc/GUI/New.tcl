proc camGUI::aNew {w} {
  set row end
  set dir 1
  if [regexp (\[0-9\]*), [$w cursel] all row] {set dir -1}

  set nrow [camMisc::arcNew $row]
  $w configure -state normal
  $w insert row -- $row $dir
  set row $nrow
  if {[expr [.tf.t cget -rows] - 2] > [llength [camMisc::arcIdx]]} {
    $w delete row [expr [.tf.t cget -rows] - 2]
  }
  frame $w.f$row -bd 1 -relief sunken
  label $w.f$row.l -text " "
  checkbutton $w.f$row.c -variable camGUI::aEngines($row,$::iBlocked) \
      -command "toggleBlock $row"
  $w.f$row.c config -activebackground [$w.f$row.c cget -background]
  bind $w.f$row.c <Enter> {
    set ::status "inhibit restart of ArchiveEngine"
  }
  bind $w.f$row.c <Leave> {
    set ::status ""
  }
  pack $w.f$row.l -side left
  pack $w.f$row.c -fill both -expand t
  $w window config $row,4 -sticky news -window $w.f$row
  set camGUI::aEngines($row,$::iHost) "$::_host"
  set ::var($row,port) 4710
  set ::ports [getPorts -1]
  spinPort $row 1
  set camGUI::aEngines($row,$::iPort) "$::var($row,port)"
  set camGUI::aEngines($row,$::iDescr) "<enter description>"
  set camGUI::aEngines($row,$::iRun) ""

  camMisc::arcSet $row host $camGUI::aEngines($row,$::iHost)
  camMisc::arcSet $row port $camGUI::aEngines($row,$::iPort)
  camMisc::arcSet $row descr $camGUI::aEngines($row,$::iDescr)
  camMisc::arcSet $row cfg [file join [pwd] "ENTER FILENAME"]
  camMisc::arcSet $row archive "<enter filename>"
  camMisc::arcSet $row log "<enter filename>"
  camMisc::arcSet $row start NO

  $w configure -state disabled

  update
  
  $w selection clear all
  $w selection set $row,0
  if {![aEdit $w]} {
    $w configure -state normal
    $w delete row $row
    $w configure -state disabled
    camMisc::arcDel $row
  }
  setButtons $w
}
