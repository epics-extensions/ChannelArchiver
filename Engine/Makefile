# EPICS R3.14 Makefile for the Engine
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

USR_CXXFLAGS += -I ../../Index

ArchiveEngine_SRCS += CircularBuffer.cpp
ArchiveEngine_SRCS += ArchiveChannel.cpp
ArchiveEngine_SRCS += SampleMechanism.cpp
ArchiveEngine_SRCS += ScanList.cpp
ArchiveEngine_SRCS += GroupInfo.cpp
ArchiveEngine_SRCS += HTMLPage.cpp
ArchiveEngine_SRCS += LockFile.cpp
ArchiveEngine_SRCS += HTTPServer.cpp
ArchiveEngine_SRCS += EngineConfig.cpp
ArchiveEngine_SRCS += EngineServer.cpp
ArchiveEngine_SRCS += Engine.cpp
ArchiveEngine_SRCS += main.cpp
PROD_HOST := ArchiveEngine
PROD_LIBS_DEFAULT := Storage    Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32   := StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
SYS_PROD_LIBS_WIN32 = ws2_32 advapi32 user32
Storage_DIR    = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
ToolsObj_DIR   = $(INSTALL_LIB)

# Only for benchmarking the read/write routines:
TESTPROD_HOST := bench

# Install script to convert old request files.
SCRIPTS += ConvertEngineConfig.pl

include $(TOP)/configure/RULES
