# EPICS R3.14 Makefile for the Export Tool
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../rtree

SRCS += main.cpp

PROD_HOST := ArchiveExport
PROD_LIBS_DEFAULT := rtree Storage Tools ca Com
PROD_LIBS_WIN32 := rtreeObj StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
rtree_DIR = $(INSTALL_LIB)
rtreeObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
ChanArchIO_DIR = $(INSTALL_LIB)
ChanArchIOObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

include $(TOP)/configure/RULES
