proc runArchiver {i} {
  array unset ::sched $i,start
  set now [clock seconds]
  foreach attr {port descr cfg cfgc} {
    set $attr [camMisc::arcGet $i $attr]
  }
  foreach attr {log archive} {
    set $attr [clock format $now -format [camMisc::arcGet $i $attr]]
  }
#  cd "[file dirname $cfg]"
  set cwd "[file dirname $cfg]"
  if {[file exists $cwd/BLOCKED]} {
    if { ![info exists ::wasBlocked($i)] || !$::wasBlocked($i) } {
      Puts "start of \"[camMisc::arcGet $i descr]\" blocked!" error
      set ::wasBlocked($i) 1
    }
    return
  }
  set ::wasBlocked($i) 0

  file mkdir "$cwd/[file dirname $log]"
  file mkdir "$cwd/[file dirname $archive]/cfg"
  set cfgdir [camMisc::arcGet $i lastCfg]

  set master_dir [file dirname $cfg]/[file tail $archive]
  set master_txt "master_version=1"

  if {[file exists $master_dir] && ("[file dirname $archive]" != ".")} { 
    regsub -all "\r" [read_file $master_dir] "" master_txt
  }
  set md [split $master_txt "\n"]
  if {[llength $md] > 1} {
    set cfgd [file dirname [lindex $md [lsearch -regex $md "^/"]]]/cfg
    if [file isdirectory $cfgd] {set cfgdir $cfgd}
  }
  if {!$cfgc} {
    if {"$cfgdir" == ""} {
      Puts "Not saving config-files of \"[camMisc::arcGet $i descr]\" - no previous directory found" error
    } else {
      Puts "Saving config-files of \"[camMisc::arcGet $i descr]\""
      set cvs [file isdir [file dirname $cfg]/CVS]
      if {$cvs} {
	set ee [split [read_file [file dirname $cfg]/CVS/Entries]]
      }
      foreach f [glob $cfgdir/*] {
	if {[file mtime $f] < [file mtime [file dirname $cfg]/[file tail $f]]} {
	  Puts "Conflict copying \"$f\" to \"[file dirname $cfg]/[file tail $f]\"" error
	  Puts "  the latter is newer!" error
	} else {
	  file copy -force $f [file dirname $cfg]
	}
	if {$cvs} {
	  cd [file dirname $cfg]
	  if {[lsearch -regex $ee "^/[file tail $f]/"] < 0} {
	    exec cvs add -m "automatic add by CAbgManager" [file tail $f]
	  }
	  exec cvs commit -m "automatic commit by CAbgManager" [file tail $f]
	}
      }
    }
  }
  if {"[file dirname $archive]" != "."} {
    set log "[file dirname $cfg]/$log"
    camMisc::recCopyCfg $cfg $cwd/[file dirname $archive]

    if {"[lindex $md 0]" == "master_version=1"} {
      if {[lsearch $md "[file dirname $cfg]/$archive"] < 0} {
	set md [linsert $md 1 "\# added by CAbgManager @ [clock format $now]" [file dirname $cfg]/$archive]
	write_file $master_dir [join $md "\n"]
      }
    }
  }
  camMisc::arcSet $i lastCfg $cwd/[file dirname $archive]/cfg
  set cwd $cwd/[file dirname $archive]

  set cfg "[file tail $cfg]"
  set archive "[file tail $archive]"
  file delete -force $cwd/archive_active.lck
#  Puts "ArchiveEngine -p $port -description \"$descr\" -log $log $cfg $archive" command
  Puts "start \"[camMisc::arcGet $i descr]\"" command
  cd $cwd
  if {$cfgc} {
    exec ArchiveEngine -p $port -description "$descr" -log $log -nocfg $cfg $archive >&runlog &
  } else {
    exec ArchiveEngine -p $port -description "$descr" -log $log $cfg $archive >&runlog &
  }
  scheduleStop $i
}
