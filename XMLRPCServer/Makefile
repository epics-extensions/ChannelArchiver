# EPICS R3.14 Makefile for the XML-RPC Data Server
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

SCRIPTS_HOST += ArchiveDataClient.pl

USR_CXXFLAGS += -I ../../Storage

# Compare to output of `xmlrpc-c-config cgi-server --cflags`
USR_CXXFLAGS += -I/usr/local/include

DummyDataServer_SRCS += DummyServer.cpp
ArchiveDataServer_SRCS += ServerConfig main.cpp
TESTPROD_HOST := DummyDataServer ArchiveDataServer

PROD_LIBS_DEFAULT  := Storage    Tools    ca Com
PROD_LIBS_WIN32    := StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
Storage_DIR    = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
ToolsObj_DIR   = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
ToolsObj_DIR   = $(INSTALL_LIB)

# Compare to output of `xmlrpc-c-config cgi-server --libs`
PROD_LIBS_DEFAULT  += xmlrpc_cgi xmlrpc xmlrpc_xmlparse xmlrpc_xmltok
xmlrpc_cgi_DIR      = $(XMLRPCBASE)/lib
xmlrpc_DIR          = $(XMLRPCBASE)/lib
xmlrpc_xmlparse_DIR = $(XMLRPCBASE)/lib
xmlrpc_xmltok_DIR   = $(XMLRPCBASE)/lib
xmlrpc_client_DIR   = $(XMLRPCBASE)/lib

# From xmlrpc-c-config libwww-client --libs
bench_LDFLAGS=-L/usr/lib -lxmlrpc_client -lwwwxml -lxmltok -lxmlparse -lwwwzip -lwwwinit -lwwwapp -lwwwhtml -lwwwtelnet -lwwwnews -lwwwhttp -lwwwmime -lwwwgopher -lwwwftp -lwwwfile -lwwwdir -lwwwcache -lwwwstream -lwwwmux -lwwwtrans -lwwwcore -lwwwutils -lmd5 -ldl -lz -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -Wl,--rpath -Wl,/usr/lib

include $(TOP)/configure/RULES

cgiinstall:
	cp O.$(EPICS_HOST_ARCH)/DummyDataServer /var/www/cgi-bin/xmlrpc/DummyDataServer.cgi
	cp O.$(EPICS_HOST_ARCH)/ArchiveDataServer /var/www/cgi-bin/xmlrpc/ArchiveDataServer.cgi

