proc camGUI::setDT {} {
  variable datetime [clock format [clock seconds] -format " %m/%d/%Y %H:%M:%S "]
  after 1000 camGUI::setDT
}
