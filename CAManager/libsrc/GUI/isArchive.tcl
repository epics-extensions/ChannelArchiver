proc isArchive {fn} {
  if {![file isfile $fn]} {return 0}
  return 1
}
