proc Puts {msg {hilit normal}} {
  .f.t configure -state normal
  .f.t insert end $msg $hilit
#  .f.t insert end \n
  .f.t configure -state disabled
  .f.t yview end
  update
}

set ::prog 0

wm title . "Install Channel Archive Manager"
frame .f -bd 8 -relief flat
text .f.t -width 80 -height 20 -yscrollcommand {.f.v set} -wrap none
.f.t tag add error end; .f.t tag configure error -foreground red
.f.t tag add ok end; .f.t tag configure ok -foreground forestgreen
.f.t tag add warning end; .f.t tag configure warning -foreground brown
.f.t tag add action end; .f.t tag configure action -foreground blue
.f.t tag add normal end;
scrollbar .f.v -command {.f.t yview}
pack .f.v -side right -fill y
pack .f.t -side right -fill both -expand t
pack .f -side top -fill both -expand t

frame .i -bd 8 -relief flat
label .i.l -text "Installation directory:" -anchor w
entry .i.e -textvariable instdir -bd 1 -bg white
pack .i.l .i.e -side top -fill x
pack .i -side top -fill x

if {![regexp "Windows" $tcl_platform(os)]} {
  frame .t -bd 8 -relief flat
  label .t.l -text "Tclsh executable:" -anchor w
  entry .t.e -textvariable tclsh -bd 1 -bg white
  pack .t.l .t.e -side top -fill x
  pack .t -side top -fill x
  
  frame .w -bd 8 -relief flat
  label .w.l -text "Wish executable:" -anchor w
  entry .w.e -textvariable wish -bd 1 -bg white
  pack .w.l .w.e -side top -fill x
  pack .w -side top -fill x
}
button .go -command {set go 1} -text Install
button .cancel -command exit -text Close
pack .go .cancel -side right -padx 8 -pady 8
focus .go
bind .go <Key-Return> {.go invoke}

set srcfiles [glob -nocomplain src/*.tcl]
incr ::prog [llength $srcfiles]
set libfiles {}
foreach dir [concat libsrc [glob -nocomplain -types d libsrc/*]] {
  set lib [glob -nocomplain $dir/*.tcl]
  set libfiles [concat $libfiles $lib]
}
incr ::prog [llength $libfiles]

if {[regexp "Windows" $tcl_platform(os)]} {
  set path [split $env(PATH) ";"]
  set ext .exe
  set tclext .tcl
} else {
  set path [split $env(PATH) ":"]
  set ext ""
  set tclext ""
}

Puts "Searching Tcl/TK interpreters:\n" action
set wish [info nameofexecutable]
set tclsh [lindex [glob -nocomplain [file dirname $wish]/tcl*$ext] 0]
Puts "  tclsh executable is $tclsh\n"
Puts "  wish executable is $wish\n"
Puts "\n"

Puts "Searching for proper installation-root:\n" action
foreach p $path {
  if [file exists [file join $p ArchiveEngine$ext]] {
    Puts "Found \"ArchiveEngine$ext\" in [file join $p]\n"
    # try to guess a proper installation-root
    set instdir [file dirname $p]
    if [regexp "bin$" $instdir] {
      set instdir [file dirname $instdir]
    }
    regsub $instdir/ [file join $p] "" bininfix 
    Puts "using \"$instdir\" as default installation-root\n"
    Puts "  bin-infix is $bininfix\n"
    break
  }
}

if {![info exists instdir]} {
  Puts "ArchiveEngine not found in PATH!\n" error
}

Puts "\n"
Puts "Searching for required external packages:\n" action
array set required {
  Tcl       8.3
  Tk        8.3
  Tclx      8.3
  Tkx       8.3
  Iwidgets  4.0.0
  BWidget   1.3.1
  Tktable   2.7
}

set terminate 0
foreach package [array names required] {
  if [catch {package require $package} result] {
    Puts "ERROR: " error
    Puts "Package \"$package\" is not installed!\n"
    set terminate 1
  } elseif {"$result" != "$required($package)"} {
    Puts "WARNING: " warning
    Puts "Version of \"$package\" is not tested version!\n"
    Puts "  Found version $result, tested version is $required($package)\n"
    if {"$result" > "$required($package)"} {
      Puts "  Installed version is newer, so CAManager should work properly\n"
    } else {
      Puts "  Installed version is older, so CAManager may not work properly\n" warning
    }
  } else {
    Puts "Package \"$package\" found (version $result)\n"
  }
}

if {!$terminate} {
  ProgressBar .progress -maximum $::prog -variable ::prog -bd 1
  pack .progress -fill both -expand t -padx 12 -pady 12
  set ::prog 0

  vwait go
  catch {
    .i.e configure -state disabled
    .t.e configure -state disabled
    .w.e configure -state disabled
  }
  Puts "\n"
  Puts "Install programs in $instdir/$bininfix:\n" action
  foreach bin $srcfiles {
    set df $instdir/$bininfix/[file rootname [file tail $bin]]$tclext
    Puts "installing $bin in $df ..."
    set s [read_file $bin]
    regsub "PATH=.* exec wish" $s "PATH=[file dirname $wish]:\$PATH exec [file tail $wish]" d
    regsub "PATH=.* exec tclsh" $d "PATH=[file dirname $tclsh]:\$PATH exec [file tail $tclsh]" d
    set mbininfix $bininfix
    regsub "bin/.*" $mbininfix "bin/.*" mbininfix
    regsub "regsub /src/" $d "regsub /$mbininfix/" d
    if {[catch {file mkdir [file dirname $df]} res]} {
      Puts "\nCan't create directory [file dirname $df]" error
      Puts "$res" error
      set terminate 1
      break
    }
    if {[catch {write_file $df $d} res]} {
      Puts "\nCan't create file $df" error
      Puts "$res" error
      set terminate 1
      break
    }
    if {![regexp "Windows" $tcl_platform(os)]} {
      file attributes $df -permissions u=rwx,go=rx
    }
    incr ::prog
    Puts " done.\n"
  }
}
if {!$terminate} {
  Puts "OK.\n" action
  Puts "\n"
  Puts "Install libraries in $instdir/lib/tcl/CAManager:\n" action
  set files {}

  foreach lib $libfiles {
    regsub "libsrc/" $lib "" l
    set df $instdir/lib/tcl/CAManager/$l
    Puts "installing $lib ..."
    if {[catch {file mkdir [file dirname $df]} res]} {
      Puts "\nCan't create directory [file dirname $df]" error
      Puts "$res" error
      set terminate 1
      break
    }
    if {[catch {file copy -force $lib $df}]} {
      Puts "\nCan't create file $df" error
      Puts "$res" error
      set terminate 1
      break
    }
    if {![regexp "Windows" $tcl_platform(os)]} {
      file attributes $df -permissions u=rw,go=r
    }
    incr ::prog
    Puts " done.\n"
  }
}

if {!$terminate} {
  Puts "OK.\n" action
  Puts "\n"
  Puts "Programs installed in $instdir/$bininfix\n"
  Puts "Libraries installed in $instdir/lib/tcl/CAManager\n"
  if {[regexp "Windows" $tcl_platform(os)]} {
    foreach d {"All Users" $tcl_platform(user)} {
      if {![file isdirectory "C:/Documents and Settings/$d/Start Menu/Programs"]} continue
      if {[catch {file mkdir "C:/Documents and Settings/$d/Start Menu/Programs/Channel Archive Manager"}]} continue
      if [catch {exec ln -sf "$instdir/$bininfix/CAManager$tclext" "C:/Documents and Settings/$d/Start Menu/Programs/Channel Archive Manager/Channel Archive Manager"}] continue
      if [catch {exec ln -sf "$instdir/$bininfix/CAbgManager$tclext" "C:/Documents and Settings/$d/Start Menu/Programs/Channel Archive Manager/Channel Archive Background Manager"}] continue
      Puts "Menu entries added to Startmenu.\n"
      Puts "\n"
    }
  }
  Puts "\n"
  Puts "Installation successfully completed.\n" ok
  Puts "You should now be able to execute \"CAManager$tclext\" and \"CAbgManager$tclext\"\n" ok
} else {
  Puts "\n"
  Puts "Installation terminated with errors!\n" error
  Puts "Check and eliminate the above problems and rerun installation\n" error
}
.go configure -text Close -command exit
destroy .progress
destroy .cancel
