proc camGUI::ClearSel {w} {
  $w selection clear all
  set topf [file rootname [file rootname $w]]
  $topf.bf.start configure -state disabled
  $topf.bf.stop configure -state disabled
  $topf.bf.edit configure -state disabled
  $topf.bf.delete configure -state disabled
}
