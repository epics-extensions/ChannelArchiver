proc camGUI::aStart {w} {
  set now [clock seconds]

  regexp (\[0-9\]*), [$w cursel] all row

  set host [camMisc::arcGet $row host]
  if {"$host" != "$::_host"} {return}

  foreach attr {cfg archive log port descr} {
    set $attr [camMisc::arcGet $row $attr]
  }

  set dir [pwd]
  cd "[file dirname $cfg]"
  set archive [clock format $now -format $archive]
  set log [clock format $now -format $log]

  set adir [file dirname [file dirname $cfg]/$archive]
  set ldir [file dirname [file dirname $cfg]/$log]
  file mkdir $adir/cfg
  file mkdir $ldir

  camMisc::recCopyCfg $cfg $adir

  set master_dir [file dirname $cfg]/[file tail $archive]
  puts "master_dir = $master_dir"
  puts "archive = [file dirname $cfg]/$archive"

  if [regexp "Example" $descr] {
    exec ArchiveEngine -description $descr -port $port -log $log $cfg $archive >/dev/null 2>/dev/null &
  } else {
    exec ArchiveEngine -nocfg -description $descr -port $port -log $log $cfg $archive >/dev/null 2>/dev/null &
  }
  cd "$dir"

  #  ClearSel $w
  after 5000 [list camComm::CheckRunning $row camGUI::aEngines($row,$::iRun)]
}
