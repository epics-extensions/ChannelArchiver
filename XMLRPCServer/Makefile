# EPICS R3.14 Makefile for the Data Server
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../Storage -I ../../rtree

# Compare to output of `xmlrpc-c-config cgi-server --cflags`
USR_CXXFLAGS += -I/usr/local/include

DummyServer_SRCS += DummyServer.cpp
ArchiveServer_SRCS += main.cpp
TESTPROD_HOST := DummyServer ArchiveServer bench xmltest

PROD_LIBS_DEFAULT := Storage Tools ca Com
PROD_LIBS_WIN32 := StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

# Compare to output of `xmlrpc-c-config cgi-server --libs`
PROD_LIBS_DEFAULT  += xmlrpc_cgi xmlrpc xmlrpc_xmlparse xmlrpc_xmltok

xmlrpc_cgi_DIR = /usr/local/lib
xmlrpc_DIR = /usr/local/lib
xmlrpc_xmlparse_DIR = /usr/local/lib
xmlrpc_xmltok_DIR = /usr/local/lib
xmlrpc_client_DIR = /usr/local/lib

# From xmlrpc-c-config libwww-client --libs
bench_LDFLAGS=-L/usr/lib -L/usr/local/lib -lxmlrpc_client -L/usr/lib -lmd5 -lwwwxml -lxmltok -lxmlparse -lwwwzip -lwwwinit -lwwwapp -lwwwhtml -lwwwtelnet -lwwwnews -lwwwhttp -lwwwmime -lwwwgopher -lwwwftp -lwwwfile -lwwwdir -lwwwcache -lwwwstream -lwwwmux -lwwwtrans -lwwwcore -lwwwutils -ldl -lz -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -Wl,--rpath -Wl,/usr/lib   

clienttest_LDFLAGS=-L/usr/lib -L/usr/local/lib -lxmlrpc_client -L/usr/lib -lmd5 -lwwwxml -lxmltok -lxmlparse -lwwwzip -lwwwinit -lwwwapp -lwwwhtml -lwwwtelnet -lwwwnews -lwwwhttp -lwwwmime -lwwwgopher -lwwwftp -lwwwfile -lwwwdir -lwwwcache -lwwwstream -lwwwmux -lwwwtrans -lwwwcore -lwwwutils -ldl -lz -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -Wl,--rpath -Wl,/usr/lib   

include $(TOP)/configure/RULES

cgiinstall:
	cp O.$(EPICS_HOST_ARCH)/DummyServer /var/www/cgi-bin/xmlrpc/DummyServer.cgi
	cp O.$(EPICS_HOST_ARCH)/ArchiveServer /var/www/cgi-bin/xmlrpc/ArchiveServer.cgi

