proc camGUI::aConfig {w} {
  # show/edit the configuration file of the selected archiver
  # provide "!group" includes as hyperllinks
  if {![regexp (\[0-9\]*), [$w cursel] all row]} return
  set tl .c
  toplevel $tl
  wm title $tl "Archiver Config"
  wm protocol $tl WM_DELETE_WINDOW {after 1 $::w(close) invoke}
  set lnk 0
  
  packTree $tl {
    {label head {} {-fill x} {} ::w(head)}
    {frame txt {-bd 0} {-fill both -expand t} {
      {scrollbar v {-orient vertical -command {%p.t yview}} {-side right -fill y}}
      {text t {-state normal -yscrollcommand {%p.v set}} {-fill both -expand t} {} ::w(txt)}
    }}
    {button close {-text Close -command {set ::continue(%P) close}} {-side right -padx 8 -pady 8} {} ::w(close)}
    {button fwd {-text ">" -command {set ::continue(%P) fwd}} {-side right -padx 8 -pady 8} {} ::w(fwd)}
    {button rew {-text "<" -command {set ::continue(%P) rew}} {-side right -padx 8 -pady 8} {} ::w(rew)}
    {button save {-text "Save" -command {set ::continue(%P) save}} {-side right -padx 8 -pady 8} {}}
    {button reload {-text "Reload" -command {set ::continue(%P) reload}} {-side right -padx 8 -pady 8}}
  }
  $::w(txt) tag add comment end; $::w(txt) tag configure comment -foreground brown
  $::w(txt) tag add option end; $::w(txt) tag configure option -foreground magenta4

  bind $::w(txt) <Control-Key-x> {
    lassign [%W tag ranges sel] from to
    if {$from == {}} break
    set ::clip [%W get $from $to]
    %W delete $from $to
    break
  }
  bind $::w(txt) <Control-Key-c> {
    lassign [%W tag ranges sel] from to
    if {$from == {}} break
    set ::clip [%W get $from $to]
    break
  }
  bind $::w(txt) <Control-Key-v> {
    if {![info exists ::clip]} break
    lassign [%W tag ranges sel] from to
    if {$from != {}} {%W delete $from $to}
    %W insert insert $::clip
    break
  }

  set show [list [camMisc::arcGet $row cfg]]
  set sidx 0
  while 1 {
    if {[llength $show] > [expr $sidx + 1]} {
      $::w(fwd) configure -state normal
    } else {
      $::w(fwd) configure -state disabled
    }
    if {$sidx > 0} {
      $::w(rew) configure -state normal
    } else {
      $::w(rew) configure -state disabled
    }
    $::w(head) configure -text [lindex $show $sidx]
    $::w(txt) configure -state normal
    $::w(txt) delete 0.0 end
    if [file exists [lindex $show $sidx]] {
      for_file line [lindex $show $sidx] {
	if {[regexp "!group(.*)" $line all name]} {
	  incr lnk
	  set name [string trim $name]
	  $::w(txt) tag add link$lnk end
	  $::w(txt) tag configure link$lnk -foreground blue -underline 1
	  $::w(txt) tag bind link$lnk <Enter> \
	      "$::w(txt) tag configure link$lnk -background gray95"
	  $::w(txt) tag bind link$lnk <Leave> \
	      "$::w(txt) tag configure link$lnk -background \[$::w(txt) cget -background\]"
	  $::w(txt) tag bind link$lnk <ButtonRelease-1> \
	      "set ::continue($tl) \"$name\""
	  regexp "(.*)${name}(.*)" $line all before after
	  $::w(txt) insert end "$before" option
	  $::w(txt) insert end "$name" link$lnk
	  $::w(txt) insert end "$after"
	} elseif {[regexp "^!.*" $line]} {
	  $::w(txt) insert end "$line" option
	} elseif {[regexp "^#.*" $line]} {
	  $::w(txt) insert end "$line" comment
	} else {
	  $::w(txt) insert end "$line"
	}
	$::w(txt) insert end "\n"
      }
    } else {
      $::w(txt) insert end "\# File doesn't yet exist!\n" comment
      $::w(txt) insert end "\# Enter contents and press \"Save\"!" comment
      $::w(txt) insert end "\n\n"
    }
#    $::w(txt) configure -state disabled
    vwait ::continue($tl)
    switch $::continue($tl) {
      close { destroy $tl; return }
      fwd { incr sidx  1 }
      rew { incr sidx -1 }
      save {
	set s [lindex $show $sidx]
	if [file exists $s] {
	  file delete -force $s~
	  file rename $s $s~
	}
	write_file $s [string trim [$::w(txt) get 0.0 end]]
      }
      reload {}
      default { 
	set cfn [file dirname [lindex $show $sidx]]
	incr sidx
	if {$sidx < [llength $show]} {
	  set show [lreplace $show $sidx end]
	}
	set show [linsert $show end $cfn/$::continue($tl)]
      }
    }
  }
}
