proc eatInput {fd w {mode normal}} {
  if {![catch {set line [read $fd]}]} {
    if {[regexp "TeRmInAtEd" $line]} {
      $w configure -state normal
      $w insert end "\nReady.\n" command
      $w configure -state disabled
      $w yview end
      set ::var(terminated,$w) 1
    } else {
      $w configure -state normal
      $w insert end "$line" $mode
      $w configure -state disabled
      $w yview end
    }
  }
}

proc pXec {cmd w} {
  pipe rd wr
  pipe ed er
  fileevent $rd readable "eatInput $rd $w"
  fileevent $ed readable "eatInput $ed $w error"
  fconfigure $rd -blocking 0
  fconfigure $ed -blocking 0
  fconfigure $wr -buffering none
  fconfigure $er -buffering none
  $w configure -state normal
#  regsub -all "/\[^ \]*/" "$cmd" ".../" c
  eval $w insert end "\"$cmd\n\"" command
  $w configure -state disabled
  if [regexp ">" $cmd] {
    catch {set ::var(xec,pid,$w) [eval exec [info nameofexecutable] $::INCDIR/xec.tcl $cmd 2>@$er &]}
  } else {
    catch {set ::var(xec,pid,$w) [eval exec [info nameofexecutable] $::INCDIR/xec.tcl $cmd >@$wr 2>@$er &]}
  }
  set ::var(xec,fh,$w) [list $wr $er $rd $ed]
  vwait ::var(terminated,$w)
}
