proc camGUI::aAbout {} {
  tk_dialog .about "About CAManager" "CAManager\n$::CVS(Version)\n$::CVS(Date)\nThomas Birke <birke@lanl.gov>" info 0 ok
}
