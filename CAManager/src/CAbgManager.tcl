#!/bin/sh
# -*- tcl -*- \
    PATH=/opt/TclTk/bin:$PATH exec tclsh $0 ${1+"$@"}

########################################################################
# 
# Project:    CAManager
#
# Descr.:     backend to start/restart ArchiveEngines
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
  global INCDIR
  set pwd [pwd]

  cd [file dirname [info script]]
  set script [pwd]/[file tail [info script]]
  regsub /src/[file tail $script] $script "" INCDIR
  append INCDIR /libsrc
  if {![file isdirectory $INCDIR]} {regsub "src$" $INCDIR "/tcl/CAManager" INCDIR}

  namespace inscope :: package require Tclx
  
  foreach dir {{} /Misc /Comm /httpd} {
    set script $INCDIR$dir/_init.tcl
    cd [file dirname $script]
    namespace inscope :: source $script
  }
  cd $pwd

  set ::_host [info hostname]
  regsub "\\..*" $::_host "" ::_host

  signal trap * {}
  signal trap SIGINT {exit}

  catch {wm withdraw .}
  Puts "Channel Archiver bgManager startup"
}

::init
trace variable ArchiversSet w {after 1 checkArchivers}
readCFG
httpd::init

update

while 1 {
  after 3600000 {set setit 1}
  vwait setit
}
