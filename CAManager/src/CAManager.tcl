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
set CVS(Revision) "$Revision$"
set CVS(Date) "$Date$"
set CVS(Author) "$Author$"

regsub ": (.*) \\$" $CVS(Revision) "\\1" CVS(Revision)
regsub ": (.*) \\$" $CVS(Date) "\\1" CVS(Date)
regsub ": (.*) \\$" $CVS(Author) "\\1" CVS(Author)

set CVS(Version) "Version: 0.9 (Rev. $CVS(Revision))"

proc init {} {
  global INCDIR tcl_platform
  set pwd [pwd]
  cd [file dirname [info script]]
  set script [pwd]/[file tail [info script]]
  regsub /src/[file tail $script] $script "" INCDIR
  append INCDIR /libsrc
  if {![file isdirectory $INCDIR]} {regsub "src$" $INCDIR "/tcl/CAManager" INCDIR}

  lappend ::auto_path [file dirname [info nameofexecutable]]
  namespace inscope :: package require Tclx

  foreach dir {{} /GUI /Misc /Comm} {
    set script $INCDIR$dir/_init.tcl
    cd [file dirname $script]
    namespace inscope :: source $script
  }
  cd $pwd
}

::init
camMisc::init
camGUI::init
camGUI::mainWindow
wm title . "Channel Archive Manager"
wm geom . [wm geom .]
set ::status "Channel Archive Manager - $CVS(Version) - $CVS(Date)"
after 1 checkForBgManager
after 10000 checkJob
after 3000 {set ::status ""}