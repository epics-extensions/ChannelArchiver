proc selFile {w label var initialdir type {mode Open}} {
  frame $w -bd 0
  iwidgets::combobox $w.e -labeltext $label -labelpos nw \
      -textvariable $var
  if {[llength $::selected($type)] == 0} {
    foreach k [camMisc::arcIdx] {
      if {"$type" != "misc"} {
	set a [file dirname [camMisc::arcGet $k cfg]]/[camMisc::arcGet $k $type]
	set a [clock format [clock seconds] -format $a]
	lappend ::selected($type) $a
      }
    }
  }
  catch {eval $w.e insert list end [luniq $::selected($type)]}
  button $w.b -text "..." -bd 1 -command "
      cd \"$initialdir\"
      set fn \[tk_get${mode}File -initialdir \[file dirname \$$var\] -initialfile \[file tail \$$var\] -parent [file rootname $w] -title \"$label\"\]
      if {\"\$fn\" != \"\"} {set $var \$fn; lappend ::selected($type) \$fn}"
  pack $w.e -side left -fill x -expand t
  pack $w.b -side bottom
  pack $w -side top -fill x -padx 4 -pady 4
}
