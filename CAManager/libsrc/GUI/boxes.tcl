proc spintime {w tvar} {
  global var
  frame $w -bd 0
  label $w.at -text at
  iwidgets::timeentry $w.te -format military -seconds off
  [$w.te component time] configure -textvariable $tvar
  pack $w.at $w.te -side left -fill both
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
