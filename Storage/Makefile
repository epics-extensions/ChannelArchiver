# EPICS R3.14 Makefile for the Storage library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Includes to be installed
INC += StorageTypes.h
INC += HashTable.h
INC += CtrlInfo.h
INC += RawValue.h
INC += DirectoryFile.h
INC += DataFile.h
INC += DataWriter.h
INC += DataReader.h

# Contents of library
LIB_SRCS += HashTable.cpp
LIB_SRCS += CtrlInfo.cpp
LIB_SRCS += RawValue.cpp
LIB_SRCS += DirectoryFile.cpp
LIB_SRCS += DataFile.cpp
LIB_SRCS += DataWriter.cpp
LIB_SRCS += DataReader.cpp

# Library to build
LIBRARY_HOST := Storage

# Build "ToolsTest" but don't install it
PROD_LIBS_DEFAULT := Storage Tools ca Com
PROD_LIBS_WIN32 := StorageObj ToolsObj ca Com
Storage_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)
TESTPROD_HOST += StorageTest

include $(TOP)/configure/RULES
