proc camGUI::setButtons {w} {
  set topf [file rootname [file rootname $w]]
  set row [expr [$w cget -rows] - 3]
  regexp (\[0-9\]*), [$w cursel] all row
  if {([llength [$w cursel]] == 0) || (![info exists camGUI::aEngines($row,$::iRun)]) } {
    $topf.bf.start configure -state disabled
    $topf.bf.stop configure -state disabled
    $topf.bf.edit configure -state disabled
    $topf.bf.delete configure -state disabled
    return
  }
  if {[regexp "since" $camGUI::aEngines($row,$::iRun)]} {
    $topf.bf.start configure -state disabled
#    $topf.bf.edit configure -state disabled
    $topf.bf.stop configure -state normal
  } else {
    $topf.bf.start configure -state normal
#    $topf.bf.edit configure -state normal
    $topf.bf.stop configure -state disabled
  }
  if {$camGUI::aEngines($row,$::iHost) != "$::_host"} {
    $topf.bf.start configure -state disabled
  }
  $topf.bf.edit configure -state normal
  $topf.bf.delete configure -state normal
}
