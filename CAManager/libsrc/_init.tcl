if {[info exists env(DEBUG)]} {
  set ::debug 1
} else {
  set ::debug 0
}

set yesno {NO YES}
set truefalse {FALSE TRUE}

set ::tl_cnt 0
set ::_port 4610
set ::dontCheckAtAll 0
set ::checkBgMan 1
set ::checkInt 10
set ::bgCheckInt 10
set ::bgUpdateInt 10

set ::multiVersion 1

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