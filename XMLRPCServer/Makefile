# EPICS R3.14 Makefile for the XML-RPC Data Server
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

SCRIPTS_HOST += ArchiveDataClient.pl

USR_CXXFLAGS += -I ../../Storage

# Library to build
INC += ArchiveDataClient.h
INC += ArchiveDataServer.h
LIB_SRCS += DataServer.cpp
LIB_SRCS += ArchiveDataClient.cpp
LIBRARY_HOST := DataServer

TESTPROD_HOST := DummyDataServer ArchiveDataServer ArchiveDataClientTest
DummyDataServer_SRCS += DummyServer.cpp
ArchiveDataServer_SRCS += ServerConfig.cpp main.cpp

PROD_LIBS_DEFAULT  := DataServer    Storage    Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32    := DataServerObj StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
DataServer_DIR    = $(INSTALL_LIB)
DataServerObj_DIR = $(INSTALL_LIB)
Storage_DIR       = $(INSTALL_LIB)
StorageObj_DIR    = $(INSTALL_LIB)
Tools_DIR         = $(INSTALL_LIB)
ToolsObj_DIR      = $(INSTALL_LIB)

# NOTE:
# Slight problem here.
# We use xmlrpc-c-config to query what's needed.
# Unfortuantely, the _LDFLAGS are inserted towards
# the front of the linker command line,
# while at least the Mac OS X linker wants to see
# those libraries towards the end.
# So you see "unresolved ..." errors.
#
# Hack:
# cd O.darwin-ppc
# make
# .. then copy/paste the last linker line, the one that failed,
# and append the `xmlrpc-c-config ...` that's somewhere in the linker line
# to the end of the linker line.
# Run make again to continue to the next error, again patch the linker
# command etc.
USR_CXXFLAGS                 += `xmlrpc-c-config cgi-server --cflags`
DummyDataServer_LDFLAGS      += `xmlrpc-c-config cgi-server --libs`
ArchiveDataServer_LDFLAGS    += `xmlrpc-c-config cgi-server --libs`
ArchiveDataClientTest_LDFLAGS = `xmlrpc-c-config libwww-client --libs`
bench_LDFLAGS=$(ArchiveDataClientTest_LDFLAGS)

include $(TOP)/configure/RULES

cgiinstall:
	cp O.$(EPICS_HOST_ARCH)/DummyDataServer /var/www/html/archive/cgi/DummyDataServer.cgi
	cp O.$(EPICS_HOST_ARCH)/ArchiveDataServer /var/www/html/archive/cgi/ArchiveDataServer.cgi

