namespace eval camMisc {
  variable cfg_file_d "<Registry>"
  variable force_cfg_file_d 0
  if {[catch {package require registry}]} {
    set force_cfg_file_d 1
    variable rcdir "$::env(HOME)/.CAManager"
    file mkdir "$rcdir"
    set cfg_file_d "$rcdir/archivers"
  } else {
    variable reg_stem "HKEY_CURRENT_USER\\Software\\ChannelArchiveManager"
    registry set $reg_stem
  }
  variable cfg_file $cfg_file_d
  variable force_cfg_file $force_cfg_file_d

  foreach k $::argv {
    if {[file exists "$k"]} {
      set cfg_file $k
      set force_cfg_file 1
    } elseif {[info exists camMisc::rcdir]} {
      set cfg_file "$rcdir/$k"
      set force_cfg_file 1
    }
  }

  variable _Archiver {}
  variable _newCnt 1001
  set ::_host [info hostname]
  regsub "\\..*" $::_host "" ::_host
}

set ::_msg {}

set ::weekdays {
  Sunday Monday Tuesday Wednesday Thursday Friday Saturday
}

proc camMisc::init {} {
  variable cfg_file
  variable force_cfg_file
  variable reg_stem
  variable cfg_ts
  variable _Archiver
  global Archivers ArchiversSet
 
  variable cfg_file_tail [file tail $cfg_file]
  if {$force_cfg_file} {
    catch {source $cfg_file}
    if {![info exists Archivers]} {set Archivers {}}
  } else {
    set Archivers {}
    foreach k [lsort [registry keys $reg_stem arc*]] {
      set l {}
      foreach v [registry values "$reg_stem\\$k"] {
	lappend l $v [registry get "$reg_stem\\$k" $v]
      }
      lappend Archivers $l
    }
  }
  set _Archiver {}
  foreach a $Archivers {
    array unset arcs
    array set arcs $a
    set row [camMisc::arcNew]
    foreach param [array names arcs] {
      camMisc::arcSet $row $param $arcs($param)
    }
  }
  set ArchiversSet 1
}

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

proc camMisc::Block {row {var x}} {
  write_file [file dirname [camMisc::arcGet $row cfg]]/BLOCKED ""
  set $var [lindex $::yesno 1]
}

proc camMisc::Release {row {var x}} {
  file delete -force [file dirname [camMisc::arcGet $row cfg]]/BLOCKED
  set $var  [lindex $::yesno 0]
}

proc luniq {list} {
  set res {}
  foreach i $list {
    if {[lsearch $res $i] < 0} {
      lappend res $i
    }
  }
  return $res
}
array set colormap {
  error		red
  command	blue
  schedule	green
  debug1	no
  debug2	no
  normal	normal
}

if {$::debug} {
  set ::colormap(debug1) normal
  set ::colormap(debug2) orange
}

proc camMisc::recCopyCfg {file dir} {
  set sdir [file dirname $file]
  file copy -force $file $dir/
  for_file line $file {
    if [regexp "^!group\[ 	\]*(.*)\[ 	\]*$" $line all nfile] {
      recCopyCfg $sdir/$nfile $dir
    }
  }
}
