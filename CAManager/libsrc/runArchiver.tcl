set ::master_version 2

set ::mutex 0

proc semTake {} {
  while {$::mutex} {
    after 100 {set ::pipi 1}
    vwait ::pipi
  }
  set ::mutex 1
}

proc semGive {} {
  set ::mutex 0
}

proc runArchiver {i {forceRun 0}} {

  # Each archiver has a root-directory ROOT
  # ArchiveEngine runs in ROOT
  #     ArchiveEngine -p ... -l x/y/z/log cfg a/b/c/directory
  # files/dirs:
  #            /ROOT/CVS/
  #                  cfg/
  #                  config-file
  #                  a/b/c/directory
  #                  x/y/z/log
  #                  archive_active.lck
  #                  BLOCKED
  #
  # parameters:
  #             cfg	/ROOT/config-file
  #		log	x/y/z/log
  #		archive	a/b/c/directory
  #		descr	BlaBlaBla
  #		port	4711

  array unset ::sched $i,start
  set now [clock seconds]
  foreach attr {port descr cfg cfgc start multi} {
    set $attr [camMisc::arcGet $i $attr]
  }
  foreach attr {log archive} {
    set $attr [clock format $now -format [camMisc::arcGet $i $attr]]
  }

  set ROOT "[file dirname $cfg]"

  semTake

  if {![file exists $cfg]} {
    if {![info exists ::wasError($i)] || ($::wasError($i) != 1)} {
      Puts "Config file of \"$descr\" doesn't exist!" error
    }
    set ::wasError($i) 1
    semGive
    return
  }

  if {[file exists $ROOT/archive_active.lck]} {
    if {![info exists ::wasError($i)] || ($::wasError($i) != 2)} {
      Puts "Lockfile for \"$descr\" exists - Archiver already running! (or terminated abnormally?)" error
    }
    set ::wasError($i) 2
    semGive
    return
  }

  lassign [duetime $i] starttime stoptime

  set toggle 0
  while {[regexp "%(\[0-9\]\[0-9\]*\)" $archive all mod]} {
    set toggle 1
    switch $start {
      minute  {set active [expr [clock format $now -format %s -gmt 1] / 60]}
      hour    {set active [expr [clock format $now -format %s -gmt 1] / 3600]}
      day     {set active [expr [clock format $now -format %s -gmt 1] / 86400]}
      week    {set active [expr [clock format $now -format %s -gmt 1] / (7*86400)]}
      month   {set active [expr [clock format $now -format %Y -gmt 1] * 12 + [clock format $now -format %m -gmt 1]]}
    }
    set active [expr $active % $mod]
    regsub -all " *%$mod" $archive $active archive
  }

  # if it's a toggle-archive, delete the one to overwrite
  if {$toggle && ("$::lastArc($i)" != "$archive")} {
    set files [glob -nocomplain [file dirname $archive]/*]
    if {[llength $files] > 0} {
      eval file delete -force $files
    } 
  }

  if {!$forceRun && [file exists $ROOT/BLOCKED]} {
    if { ![info exists ::wasError($i)] || ($::wasError($i) != 3) } {
      Puts "start of \"$descr\" blocked!" error
      set ::wasError($i) 3
    }
    semGive
    return
  }
  set ::wasError($i) 0

  # create all directories that are necessary
  file mkdir "$ROOT/[file dirname $log]"
  file mkdir "$ROOT/[file dirname $archive]"
  if {!$cfgc} {file mkdir "$ROOT/cfg"} else {file delete -force $ROOT/cfg}

  # Save the (eventually) modified cfg-files in ROOT/cfg/
  if {!$cfgc} {
    set cfgfiles {}
    set X 1
    foreach f [glob -nocomplain $ROOT/cfg/*] {
      if {$X} {Puts "Saving config-files of \"$descr\""}
      set X 0
      set f [file tail $f]
      if {![file exists $ROOT/$f] ||
	  ([file mtime $ROOT/cfg/$f] > [file mtime $ROOT/$f])} {
	file copy -force $ROOT/cfg/$f $ROOT/$f
	lappend cfgfiles $f
      } else {
	Puts "Conflict copying \"$f\" - $ROOT/$f is newer!" error
      }
    }
  }

  # Start the Archiver
  set cfg "[file tail $cfg]"
  Puts "start \"$descr\"" command
  cd $ROOT
  if {$cfgc} {
    exec ArchiveEngine -p $port -description "$descr" -log $log -nocfg $cfg $archive >&runlog &
  } else {
    exec ArchiveEngine -p $port -description "$descr" -log $log $cfg $archive >&runlog &
  }

  semGive

  if {!$forceRun} {scheduleStop $i}

  # Check the changed cfg-files into cvs.
  if {!$cfgc} {
    if {[file isdir $ROOT/CVS]} {
      set ee [split [read_file $ROOT/CVS/Entries]]
      foreach f $cfgfiles {
	cd $ROOT
	if {[lsearch -regex $ee "^/[file tail $f]/"] < 0} {
	  exec cvs add -m "automatic add by CAManager" [file tail $f]
	}
	exec cvs commit -m "automatic commit by CAManager" [file tail $f]
      }
    }
  }

  # Now edit the master-files accordingly
  set master_dir $ROOT/[file tail $archive]
  set master_txt "master_version=$::master_version"

  if {[file exists $master_dir] && ("[file dirname $archive]" != ".")} { 
    regsub -all "\r" [read_file $master_dir] "" master_txt
  }
  set md [split $master_txt "\n"]
  if {$toggle} {
    set j [lsearch -regex $md ".* $ROOT/$archive"]
    if {$j > 0} {
      if {[regexp "^\#" [lindex $md [expr $j - 1]]]} {incr j -1}
      set md [lrange $md 0 [expr $j - 1]]
    }
  }

  if {"[file dirname $archive]" != "."} {
    if {"[lindex $md 0]" == "master_version=$::master_version"} {
      if {[lsearch -regex $md ".*$ROOT/$archive"] < 0} {
	set ts ""
	if {$::master_version == 2} {
	  if {$starttime == 0} {
	    lassign [getTimes $ROOT/$archive] starttime stoptime
	    if {"$starttime" != "0"} {
	      set starttime [clock scan $starttime]
	      set stoptime [clock scan $stoptime]
	    }
	  }
	  set ts "[clock format $starttime -format %m/%d/%Y\ %H:%M:%S] [clock format $stoptime -format %m/%d/%Y\ %H:%M:%S] "
	  set md [linsert $md 1 "$ts$ROOT/$archive"]
	} else {
	  set md [linsert $md 1 "$ROOT/$archive"]
	}
	write_file $master_dir [join $md "\n"]
      }
    }
  }
  if {"$multi" != ""} {
    after 20000 "updateMultiArchive \"$multi\""
  }
}
