proc getPorts {row} {
  set ports {}
  foreach i [camMisc::arcIdx] {
    if {( $i != $row ) &&
	( ( $row == -1 ) || 
	  ( [camMisc::arcGet $row host] == [camMisc::arcGet $i host] ) ) } {
      lappend ports [camMisc::arcGet $i port]
    }
  }
  return $ports
}
