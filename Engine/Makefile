# EPICS R3.14 Makefile for the Engine
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../rtree
SRCS += CircularBuffer.cpp
SRCS += ArchiveChannel.cpp
SRCS += SampleMechanism.cpp
SRCS += ScanList.cpp
SRCS += GroupInfo.cpp
SRCS += HTMLPage.cpp
SRCS += LockFile.cpp
SRCS += HTTPServer.cpp
SRCS += ConfigFile.cpp
SRCS += EngineServer.cpp
SRCS += Engine.cpp
SRCS += main.cpp
PROD_HOST := ArchiveEngine
PROD_LIBS_DEFAULT := rtree Storage Tools ca Com
PROD_LIBS_WIN32 := rtreeObj StorageObj ToolsObj ca Com
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
rtree_DIR = $(INSTALL_LIB)
rtreeObj_DIR = $(INSTALL_LIB)
Storage_DIR = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

# Only for benchmarking the read/write routines:
TESTPROD_HOST := bench

include $(TOP)/configure/RULES


