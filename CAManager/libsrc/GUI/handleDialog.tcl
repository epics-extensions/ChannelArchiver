proc handleDialog {w {cond 0} {errmsg {"" ""}}} {
  wm protocol $w WM_DELETE_WINDOW "after 1 $w.bb.cancel invoke"
  $w.bb.cancel configure -text Close -command "set ::var($w,go) 0"  -state normal
  $w.bb.go configure -text Go -command "set ::var($w,go) 1"  -state normal
  while {1} {
    vwait ::var($w,go)
    if {$::var($w,go)} {
      if $cond {
	tk_messageBox -type ok -icon error -parent $w \
	    -title [lindex $errmsg 0] -message [lindex $errmsg 1]
      } else {
	$w.bb.cancel configure -state disabled
	$w.bb.go configure -text Abort -command "kill \[expr \$::var(xec,pid,$w.tf.t) + 1 \]" -state normal
	break
      }
    } else {
      break
    }
  }
  return $::var($w,go)
}
