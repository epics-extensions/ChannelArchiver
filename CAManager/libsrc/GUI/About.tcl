set ::Version 0.1

proc camGUI::aAbout {} {
  tk_dialog .about "About CAManager" "CAManager\nVersion $::Version\nThomas Birke <birke@lanl.gov>" info 0 ok
}
