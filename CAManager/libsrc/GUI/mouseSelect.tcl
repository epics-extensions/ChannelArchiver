proc mouseSelect {w x y} {
  set oldsel [$w cursel]
  $w selection clear all
  $w selection set @$x,$y
  set cursel [$w cursel]
  if {$oldsel == $cursel} {
#    $w selection clear all
#    return 0
    return 1
  }
  regexp (\[0-9\]*), $cursel all row
  if {$row > [expr [llength [camMisc::arcIdx]] - 1]} {
    $w selection clear all
    return 0
  }
  return 1
}
