set cfg_ts 0
proc readCFG {} {
  after 1000 readCFG
  if {$camMisc::force_cfg_file} {
    set ts [file mtime $camMisc::cfg_file]
  } else {
    set ts [registry get "$camMisc::reg_stem" "ts"]
  }
  if {$ts > $::cfg_ts} {
    Puts "(re-)readCFG"
    foreach i [array names ::sched *,job] {
      after cancel $::sched($i)
    }
    camMisc::init
    set ::cfg_ts $ts
    foreach i [camMisc::arcIdx] {
      if {"[camMisc::arcGet $i host]" != "$::_host"} continue
      if {"[camMisc::arcGet $i start]" == "NO"} continue
      scheduleStop $i
    }
  }
  update
}
