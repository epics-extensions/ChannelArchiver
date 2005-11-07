# EPICS R3.14 Makefile for the Channel Archiver

TOP=../..
include $(TOP)/configure/CONFIG

DIRS += Tools
DIRS += LibIO
DIRS += Storage
DIRS += Engine
DIRS += DataTool
DIRS += IndexTool
DIRS += ArchiveDaemon
DIRS += XMLRPCServer

DIRS += Manager
DIRS += Export

include $(TOP)/configure/RULES_DIRS

stats:
	@echo -n "Lines of Code (C, C++, Perl) :"
	@wc -l */*.h */*.cpp */*.cpp | fgrep total
	@echo -n "Lines of Documentation Source:"
	@wc -l manual/*.tex | fgrep total

tests:
	cd Tools; O.$(EPICS_HOST_ARCH)/ToolsTest
	cd Storage; sh test.sh
	cd DemoData; sh test.sh
	cd XMLRPCServer; sh test.sh
	cd Engine; sh test.sh
