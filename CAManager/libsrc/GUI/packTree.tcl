proc camGUI::packTree {w tree} {
  foreach item $tree {
    lassign $item type name options packopt sub var
    regsub "(.)\\..*" $w "\\1" P
    regsub -all "%w" $options $w.$name options
    regsub -all "%P" $options $P options
    regsub -all "%p" $options $w options
    regsub -all "%w" $var $w.$name var
    regsub -all "%P" $var $P var
    regsub -all "%p" $var $w var
    if {$var != {}} {set $var $w.$name}
    eval $type $w.$name $options
    set W $w.$name
    if [regexp "TitleFrame" $type] {
      set W [$W getframe]
    }
    packTree $W $sub
    eval pack $w.$name $packopt
  }
}
