proc getCfgFile {parent row} {
  global var
  tk_getOpenFile -parent $parent \
      -initialdir [file dirname $var($row,cfg)] \
      -initialfile [file tail $var($row,cfg)] \
      -title "Select Configuration File"
}
