#!/bin/sh
# -*- tcl -*- \
    PATH=/opt/TclTk/bin:$PATH exec wish $0 ${1+"$@"}

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
after 1 checkForBgManager
after 10000 checkJob
