proc camMisc::Block {row {var x}} {
  write_file [file dirname [camMisc::arcGet $row cfg]]/BLOCKED ""
  set $var 1
}

proc camMisc::Release {row {var x}} {
  file delete -force [file dirname [camMisc::arcGet $row cfg]]/BLOCKED
  set $var 0
}
