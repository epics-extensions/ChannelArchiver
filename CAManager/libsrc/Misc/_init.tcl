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
source Puts.tcl
source arc.tcl
source block.tcl
source luniq.tcl
source recCopyCfg.tcl
