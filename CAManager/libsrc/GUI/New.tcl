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

  set camGUI::aEngines($row,$::iHost) "$::_host"
  set ::var($row,port) 4711
  set ::ports [getPorts -1]
#  spinPort $row 1
  set camGUI::aEngines($row,$::iPort) "$::var($row,port)"
  set camGUI::aEngines($row,$::iDescr) "<enter description>"
  set camGUI::aEngines($row,$::iRun) ""

  camMisc::arcSet $row host $camGUI::aEngines($row,$::iHost)
  camMisc::arcSet $row port $camGUI::aEngines($row,$::iPort)
  camMisc::arcSet $row descr $camGUI::aEngines($row,$::iDescr)
  camMisc::arcSet $row cfg [file join [pwd] "ENTER FILENAME"]
  camMisc::arcSet $row cfgc 0
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
    destroy $f
  }
  setButtons $w
}
