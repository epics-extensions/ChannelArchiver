proc camMisc::arcGet {index {key ""}} {
  variable _Archiver
  if {"$key" == ""} {
    return [lindex $_Archiver $index]
  } else {
    return [lindex [namespace inscope :: array get [lindex $_Archiver $index] $key] 1]
  }
}

proc camMisc::arcSet {index key value} {
  variable _Archiver
  return [namespace inscope :: array set [lindex $_Archiver $index] [list $key $value]]
}

proc camMisc::arcNew {{index end}} {
  variable _Archiver
  variable _newCnt
  if {"$index" == "end"} {
    set index [llength $_Archiver]
  }
  set index [expr max(min($index,[llength $_Archiver]),0)]
  set narc arc$_newCnt
  incr _newCnt
  namespace inscope :: array set $narc {}
  set _Archiver [linsert $_Archiver $index $narc]
  return $index
}

proc camMisc::arcDel {index} {
  variable _Archiver
  set arr [lindex $_Archiver $index]
  namespace inscope :: array unset $arr
  set _Archiver [lreplace $_Archiver $index $index]
}

proc camMisc::arcIdx {} {
  variable _Archiver
  set res {}
  for {set i 0} {$i < [llength $_Archiver]} {incr i} {
    lappend res $i
  }
  return $res
}
