proc getAInfoCB {arr ind mode} {
  if {![catch {lassign [getAInfo [lindex [array get $arr $ind] 1]] num first last}]} {
    if {$num > 0} {
      $::tf.d show $first
      $::tf.t show $first
      $::tt.d show $last
      $::tt.t show $last
    }
  }
  return 1
}

proc getAInfo {fn} {
  for_file line "|ArchiveManager -info \"$fn\"" {
    regexp "Channel count : (\[0-9\]*)" $line all numChannels
    regexp "First sample  : (\[^\\.\]*)" $line all firstTime
    regexp "Last  sample  : (\[^\\.\]*)" $line all lastTime
  }
  return [list $numChannels [clock scan $firstTime] [clock scan $lastTime]]
}
