proc updateMultiArchives {} {
  set A {}
  foreach i [camMisc::arcIdx] {
    if {[camMisc::arcGet $i multi] != ""} {
      lappend A [camMisc::arcGet $i multi]
    }
  }
  foreach a [luniq $A] {
    updateMultiArchive $a
  }
}

proc getTimes {arc} {
  if {![catch {
    for_file line "|ArchiveManager -info \"$arc\"" {
      regexp "First.*sample.*: (\[^\\.\]*)" $line all starttime
      regexp "Last.*sample.*: (\[^\\.\]*)" $line all stoptime
    }
  }]} {
    if {[info exists ::_run($i)] && [regexp "^since" $::_run($i)]} {
      set stoptime [clock format [expr [clock seconds] + 365*86400] -format "%m/%d/%Y %H:%M:%S"]
    }
    return [list $starttime $stoptime]
  } else {
    return 0
  }
}

set ::n 1
proc updateMultiArchive {arch} {
  set myN $::n
  incr ::n
  set wh [open $arch.$myN w]
  puts $wh "master_version=$::master_version"
  foreach i [camMisc::arcIdx] {
    if {[camMisc::arcGet $i multi] == $arch} {
      set archive "[file dirname [camMisc::arcGet $i cfg]]/[file tail [camMisc::arcGet $i archive]]"
      puts $wh "\# from $archive"
      if {![file exists $archive]} {
	puts $wh "\#  nothing..."
	continue
      }
      if {[file dirname [camMisc::arcGet $i archive]] == "."} {
	set ts ""
	if {$::master_version == 2} {
	  lassign [getTimes $archive] starttime stoptime
	  if {"$starttime" != "0"} {
	    set ts "$starttime $stoptime "
	  } else {
	    continue
	  }
	}
	puts $wh "$ts$archive"
      } else {
	regsub -all "\n\n" xxx "\n" f
	puts $wh [string trim [join [lrange [split [read_file $archive] "\n"] 1 end] "\n"]]
      }
    }
  }
  close $wh
  file rename -force $arch.$myN $arch
}
