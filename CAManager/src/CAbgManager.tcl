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
set CVS(Revision,bgManager) "$Revision$"
set CVS(Date,bgManager) "$Date$"
set CVS(Author,bgManager) "$Author$"

regsub ": (.*) \\$" $CVS(Revision,bgManager) "\\1" CVS(Revision,bgManager)
regsub ": (.*) \\$" $CVS(Date,bgManager) "\\1" CVS(Date,bgManager)
regsub ": (.*) \\$" $CVS(Author,bgManager) "\\1" CVS(Author,bgManager)

set CVS(Version) "Version: 1.1"

proc init {} {
  global INCDIR
#  set ::pwd [pwd]
#  cd [file dirname [info script]]
#  set script [pwd]/[file tail [info script]]
  regsub /src/[file tail [info script]] [info script] "" INCDIR
  append INCDIR /libsrc
  if {![file isdirectory $INCDIR]} {regsub "src$" $INCDIR "/tcl/CAManager" INCDIR}

  namespace inscope :: package require Tclx
  
  foreach dir {util misc comm httpd} {
    set script $INCDIR/$dir.tcl
    namespace inscope :: source $script
  }
  foreach f [glob $INCDIR/plugins/*.tcl] {
    set script $f
    namespace inscope :: source $f
  }
  cd $::pwd

  set ::_host [info hostname]
  regsub "\\..*" $::_host "" ::_host

  signal ignore *
  catch {signal trap SIGTERM {puts stderr "Terminating(TERM)..."; after 1 {exit -15}}}
  catch {signal trap SIGINT {puts stderr "Terminating(INT)..."; after 1 {Exit -2}}}
  catch {signal trap SIGQUIT {puts stderr "Terminating(QUIT)..."; after 1 {Exit -3}}}
  catch {signal default SIGTSTP}
  catch {signal default SIGCONT}
  catch {signal default SIGCHLD}

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
