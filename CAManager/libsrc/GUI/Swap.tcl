proc camGUI::aSwap {dir w} {
  regexp (\[0-9\]*), [$w cursel] all row
  if {$dir == "up"} {
    set with [expr $row - 1]
    set sel $with
  } else {
    if {$row == 0} {
      set row 1; set with 0
      set sel 1
    } else {
      set with [expr $row + 1]
      set sel $with
    }
  }

  

  set b1 $camGUI::aEngines($row,$::iBlocked)
  set r1 $camGUI::aEngines($row,$::iRun)
  set b2 $camGUI::aEngines($with,$::iBlocked)
  set r2 $camGUI::aEngines($with,$::iRun)

  set m [lindex $camMisc::_Archiver $row]
  set a [lreplace $camMisc::_Archiver $row $row]
  set camMisc::_Archiver [linsert $a $with $m]

  $w configure -state normal -flashmode 0
  $w delete row $row
  $w insert row -- $with -1

  set camGUI::aEngines($with,$::iHost) "[camMisc::arcGet $with host]"
  set camGUI::aEngines($with,$::iPort) "[camMisc::arcGet $with port]"
  set camGUI::aEngines($with,$::iDescr) "[camMisc::arcGet $with descr]"
  set camGUI::aEngines($with,$::iRun) "[camMisc::arcGet $with run]"

  set camGUI::aEngines($with,$::iBlocked) $b1
  set camGUI::aEngines($with,$::iRun) $r1
  set camGUI::aEngines($row,$::iBlocked) $b2
  set camGUI::aEngines($row,$::iRun) $r2

  $w configure -state disabled -flashmode 1
  Select $w $sel
}