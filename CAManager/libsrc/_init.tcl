if {[info exists env(DEBUG)]} {
  set ::debug 1
} else {
  set ::debug 0
}

source bgerror.tcl
source checkArchivers.tcl
source checkForBgManager.tcl
source checkstate.tcl
source duetime.tcl
source readCfg.tcl
source runArchiver.tcl
source scheduleStop.tcl
source stopArchiver.tcl
source updateMultiArchive.tcl