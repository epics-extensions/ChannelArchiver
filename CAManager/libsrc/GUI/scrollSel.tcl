proc scrollSel {w i} {
  set sel [$w cursel]
  if {[llength $sel] == 0} {
    if {$i > 0} {
      set s 0
    } else {
      set s [expr [llength [camMisc::arcIdx]] -1]
    }
    for {set i 0} {$i < [llength [camMisc::arcIdx]]} {incr i} {
      $w selection set $s,$i
    }
  } else {
    regsub ",.*" [lindex $sel 0] "" s
    set os $s
    incr s $i
    if {($s < 0) || ($s >= [llength [camMisc::arcIdx]])} return
    $w selection clear all
    regsub -all -- "$os," $sel "$s," sel
    foreach s $sel {
      $w selection set $s
    }
  }
  camGUI::setButtons $w
}
