proc luniq {list} {
  set res {}
  foreach i $list {
    if {[lsearch $res $i] < 0} {
      lappend res $i
    }
  }
  return $res
}
