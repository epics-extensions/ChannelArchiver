#!/bin/sh
# -*- tcl -*- \
    PATH=/opt/TclTk/bin:$PATH exec wish $0 ${1+"$@"}

########################################################################
# 
# Project:    CAManager
#
# Descr.:     main GUI frontend to configure/start/stop ArchiveEngines
#
# Author(s):  Thomas Birke <birke@lanl.gov>
# 
########################################################################

# This is to get the CVS-revision-code into the source...
set Revision ""
set Date ""
set Author ""
set CVS(Revision,Manager) "$Revision$"
set CVS(Date,Manager) "$Date$"
set CVS(Author,Manager) "$Author$"

regsub ": (.*) \\$" $CVS(Revision,Manager) "\\1" CVS(Revision,Manager)
regsub ": (.*) \\$" $CVS(Date,Manager) "\\1" CVS(Date,Manager)
regsub ": (.*) \\$" $CVS(Author,Manager) "\\1" CVS(Author,Manager)

set CVS(Version) "Version: 1.3"

proc init {} {
  global INCDIR tcl_platform
#  set pwd [pwd]
#  cd [file dirname [info script]]
#  set script [pwd]/[file tail [info script]]
#  set script [info script]
  regsub /src/[file tail [info script]] [info script] "" INCDIR
  append INCDIR /libsrc
  if {![file isdirectory $INCDIR]} {regsub "src$" $INCDIR "/tcl/CAManager" INCDIR}

  lappend ::auto_path [file dirname [info nameofexecutable]]
  namespace inscope :: package require Tclx

  foreach f {util gui misc comm} {
    set script $INCDIR/$f.tcl
    namespace inscope :: source $script
  }
  foreach f [glob $INCDIR/plugins/*.tcl] {
    set script $f
    namespace inscope :: source $f
  }
}

::init
camMisc::init
camGUI::init
camGUI::mainWindow
wm title . "Channel Archive Manager"
wm geom . [wm geom .]
set ::status "Channel Archive Manager - $CVS(Version) - $CVS(Date,Manager)"
after 1 checkForBgManager
after 10000 camGUI::checkJob
after 3000 {set ::status ""}
