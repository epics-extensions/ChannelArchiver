proc spintime {w hvar mvar} {
  frame $w -bd 0
  spinbox $w.h Hour left $hvar {0 23 1} {}
  $w.h.e configure -width 2 -editable 0
  spinbox $w.m Minute left $mvar {0 59 1} {}
  $w.m.e configure -width 2 -editable 0
  pack $w.m $w.h -side right -fill both -padx 4
}

proc spinbox {w label labelside var vals cmd args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  SpinBox $w.e -entrybg white -bd 1 -textvariable $var -range $vals
  if {[llength $cmd] > 0} {$w.e configure -modifycmd $cmd}
  eval pack $w.e -side top -fill both $args
}

proc combobox {w label labelside var vals args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  ComboBox $w.e -entrybg white -bd 1 -textvariable $var -values [luniq $vals]
  eval pack $w.e -side top -fill both $args
}

proc entrybox {w label labelside var args} {
  LabelFrame $w -side $labelside -anchor w -text $label
  Entry $w.e -bg white -bd 1 -textvariable $var
  eval pack $w.e -side top -fill both $args
}
