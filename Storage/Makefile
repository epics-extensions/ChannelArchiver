# EPICS R3.14 Makefile for the Storage library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Library to build
INC += StorageTypes.h
# Old
INC += HashTable.h
INC += DirectoryFile.h
INC += OldDataWriter.h
INC += OldDataReader.h
# Current
INC += CtrlInfo.h
INC += RawValue.h
INC += DataFile.h
INC += FileAllocator.h
INC += NameHash.h
INC += RTree.h
INC += Index.h
INC += IndexFile.h
INC += ListIndex.h
INC += DataWriter.h
INC += DataReader.h
INC += AverageReader.h
INC += LinearReader.h
INC += PlotReader.h
INC += SpreadsheetReader.h
# Old
LIB_SRCS += HashTable.cpp
LIB_SRCS += DirectoryFile.cpp
LIB_SRCS += OldDataWriter.cpp
LIB_SRCS += OldDataReader.cpp
# Current
LIB_SRCS += CtrlInfo.cpp
LIB_SRCS += RawValue.cpp
LIB_SRCS += DataFile.cpp
LIB_SRCS += FileAllocator.cpp
LIB_SRCS += NameHash.cpp
LIB_SRCS += RTree.cpp
LIB_SRCS += Index.cpp
LIB_SRCS += IndexFile.cpp
LIB_SRCS += ListIndex.cpp
LIB_SRCS += DataWriter.cpp
LIB_SRCS += DataReader.cpp
LIB_SRCS += AverageReader.cpp
LIB_SRCS += LinearReader.cpp
LIB_SRCS += PlotReader.cpp
LIB_SRCS += SpreadsheetReader.cpp
LIBRARY_HOST := Storage

# Tools to build but not install
TESTPROD_HOST += FileAllocatorTest FileAllocatorTool
TESTPROD_HOST += NameHashTest RTreeTest
PROD_LIBS_DEFAULT := Storage    Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32   := StorageObj ToolsObj $(EPICS_BASE_IOC_LIBS)
Storage_DIR    = $(INSTALL_LIB)
StorageObj_DIR = $(INSTALL_LIB)
Tools_DIR      = $(INSTALL_LIB)
ToolsObj_DIR   = $(INSTALL_LIB)

include $(TOP)/configure/RULES



