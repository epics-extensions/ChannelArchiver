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

# Compare to output of `xmlrpc-c-config cgi-server --cflags`
USR_CXXFLAGS += -I/usr/local/include

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

# Compare to output of `xmlrpc-c-config cgi-server --libs`
PROD_LIBS_DEFAULT  += xmlrpc_cgi xmlrpc xmlrpc_xmlparse xmlrpc_xmltok
xmlrpc_cgi_DIR      = $(XMLRPCBASE)/lib
xmlrpc_DIR          = $(XMLRPCBASE)/lib
xmlrpc_xmlparse_DIR = $(XMLRPCBASE)/lib
xmlrpc_xmltok_DIR   = $(XMLRPCBASE)/lib
xmlrpc_client_DIR   = $(XMLRPCBASE)/lib

# TODO:
# xmlrpc-c-config libwww-client --libs
# tells us what flags to use for linking,
# and the result differs slightly between installations.
# This one works on Linux, but won't under Windoze.
ArchiveDataClientTest_LDFLAGS=`xmlrpc-c-config libwww-client --libs`
bench_LDFLAGS=$(ArchiveDataClientTest_LDFLAGS)

include $(TOP)/configure/RULES

cgiinstall:
	cp O.$(EPICS_HOST_ARCH)/DummyDataServer /var/www/cgi-bin/xmlrpc/DummyDataServer.cgi
	cp O.$(EPICS_HOST_ARCH)/ArchiveDataServer /var/www/cgi-bin/xmlrpc/ArchiveDataServer.cgi

