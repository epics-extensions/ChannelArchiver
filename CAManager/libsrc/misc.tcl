# This is to get the CVS-revision-code into the source...
set Revision ""
set Date ""
set Author ""
set CVS(Revision,Misc) "$Revision$"
set CVS(Date,Misc) "$Date$"
set CVS(Author,Misc) "$Author$"

regsub ": (.*) \\$" $CVS(Revision,Misc) "\\1" CVS(Revision,Misc)
regsub ": (.*) \\$" $CVS(Date,Misc) "\\1" CVS(Date,Misc)
regsub ": (.*) \\$" $CVS(Author,Misc) "\\1" CVS(Author,Misc)

namespace eval camMisc {
  variable cfg_file_d "<Registry>"
  variable force_cfg_file_d 0
  if {[info exists ::env(HOME)]} {
    variable rcdir "$::env(HOME)/.CAManager"
    file mkdir "$rcdir"
  } else {
    variable rcdir [pwd]
  }
  if {[catch {package require registry}]} {
    set force_cfg_file_d 1
    set cfg_file_d "$rcdir/archivers"
  } else {
    variable reg_stem "HKEY_CURRENT_USER\\Software\\ChannelArchiveManager"
    registry set $reg_stem
  }
  variable cfg_file $cfg_file_d
  variable force_cfg_file $force_cfg_file_d

  set argList {
    {log.arg "" "create logfile"}
    {log+.arg "" "append to logfile"}
    {nocmd "" "without start/stop-interface (CAbgManager only)"}
  }

  package require cmdline
  array set ::args [cmdline::getoptions ::argv $argList]
  if {($::args(log) != "-") && [regexp "^\[^/\]" $::args(log)]} {
    set ::args(log) [pwd]/$::args(log)
  }
  if {($::args(log+) != "-") && [regexp "^\[^/\]" $::args(log+)]} {
    set ::args(log+) [pwd]/$::args(log+)
  }
  regsub "^.*//" $::args(log) "/" ::args(log)
  regsub "^.*//" $::args(log+) "/" ::args(log+)
  if {$::args(log) == ""} {array unset ::args log}
  if {$::args(log+) == ""} {array unset ::args log+}

  if {[info exists ::args(log)] && ($::args(log) != "-")} {
    file delete -force "$::args(log)"
  }
  if {[info exists ::args(log+)]} {
    set $::args(log) $::args(log+)
  }
  
  foreach k $::argv {
    set d [file dirname $k]
    if {[file exists "$k"]} {
      regsub "^.*//" "[pwd]/$k" "/" cfg_file
      set force_cfg_file 1
    } elseif {![regexp "^/" $k] && [file exists "$rcdir/$k"]} {
      set cfg_file "$rcdir/$k"
      set force_cfg_file 1
    } elseif {[file isdirectory $d]} {
      set cfg_file $k
      set force_cfg_file 1
    } elseif {![regexp "^/" $k] && [file isdirectory "$rcdir/$d"]} {
      set cfg_file "$rcdir/$k"
      set force_cfg_file 1
    } else {
      puts stderr "ERROR:"
      puts stderr "File \"$k\" doesn't exist and can't be created in"
      puts stderr "    neither [pwd]"
      puts stderr "    nor     $rcdir"
      exit 1
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
Puts "camMisc::init" funcall
  variable cfg_file
  variable force_cfg_file
  variable reg_stem
  variable cfg_ts
  variable _Archiver
  global Archivers ArchiversSet
 
  variable cfg_file_tail [file tail $cfg_file]
  if {$force_cfg_file} {
    if {![file exists $cfg_file]} {
      catch {write_file $cfg_file ""}
    }
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
    # special HACK for change (+=mstr)!
    if {"[arcGet $row mstr]" == ""} {
      arcSet $row mstr [file dirname [arcGet $row cfg]]
      arcSet $row cfg  [file tail [arcGet $row cfg]]
    }
  }
  set ArchiversSet 1
  if {![info exists ::allowedIPs]} {
    set ::allowedIPs ""
  }
}

proc camMisc::arcGet {index {key ""}} {
Puts "camMisc::arcGet $index \"$key\"" funcallX
  variable _Archiver
  if {"$key" == ""} {
    return [lindex $_Archiver $index]
  } else {
    return [lindex [namespace inscope :: array get [lindex $_Archiver $index] $key] 1]
  }
}

proc camMisc::arcSet {index key value} {
Puts "camMisc::arcSet $index \"$key\" \"$value\"" funcall
  variable _Archiver
  return [namespace inscope :: array set [lindex $_Archiver $index] [list $key $value]]
}

proc camMisc::arcNew {{index end}} {
Puts "camMisc::arcNew $index" funcall
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
Puts "camMisc::arcDel $index" funcall
  variable _Archiver
  set arr [lindex $_Archiver $index]
  namespace inscope :: array unset $arr
  set _Archiver [lreplace $_Archiver $index $index]
}

proc camMisc::arcIdx {} {
Puts "camMisc::arcIdx" funcall
  variable _Archiver
  set res {}
  for {set i 0} {$i < [llength $_Archiver]} {incr i} {
    lappend res $i
  }
  return $res
}

proc camMisc::Block {row {var x}} {
Puts "camMisc::Block $row $var" funcall
  write_file [file join [camMisc::arcGet $row mstr] BLOCKED] ""
  set $var [lindex $::yesno 1]
}

proc camMisc::Release {row {var x}} {
Puts "camMisc::Release $row $var" funcall
  file delete -force [file join [camMisc::arcGet $row mstr] BLOCKED]
  set $var  [lindex $::yesno 0]
}

proc luniq {list} {
Puts "misc: luniq $list" funcall
  set res {}
  foreach i $list {
    if {[lsearch $res $i] < 0} {
      lappend res $i
    }
  }
  return $res
}

proc camMisc::recCopyCfg {file dir} {
Puts "camMisc::recCopyCfg \"$file\" \"$dir\"" funcall
  set sdir [file dirname $file]
  file copy -force $file $dir
  for_file line $file {
    if {[regexp "^!group\[ 	\]*(.*)\[ 	\]*$" $line all nfile]} {
      file mkdir [file dirname $dir/$nfile]
      recCopyCfg $sdir/$nfile $dir/$nfile
    }
  }
}

proc camMisc::isLocalhost {name} {
Puts "camMisc::isLocalhost \"$name\"" funcall
  return [expr ![string compare -nocase $name $::_host] \
	    || ![string compare -nocase $name "localhost"]]
}
