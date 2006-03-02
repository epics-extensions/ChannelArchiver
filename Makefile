# EPICS R3.14 Makefile for the Channel Archiver

TOP=../..
include $(TOP)/configure/CONFIG

DIRS += Tools
DIRS += LibIO
DIRS += Storage
DIRS += IndexTool
DIRS += Export
DIRS += XMLRPCServer
DIRS += Engine
DIRS += DataTool
DIRS += Manager

include $(TOP)/configure/RULES_DIRS

stats:
	@echo -n "Lines of Code (C, C++, Perl) :"
	@wc -l */*.h */*.cpp | fgrep total
	@echo -n "Lines of Documentation Source:"
	@wc -l manual/*.tex | fgrep total

tests:
	cd Tools; $(MAKE) test
	cd Storage; $(MAKE) test
	cd Export; $(MAKE) test
	cd XMLRPCServer; $(MAKE) test
	cd IndexTool; $(MAKE) test
	#cd DemoData; sh test.sh
	#cd Engine; sh test.sh
