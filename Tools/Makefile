# EPICS R3.14 Makefile for the Tools library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

# Library to build
INC += ToolsConfig.h 
INC += AutoPtr.h
INC += ArchiveException.h
INC += ArchiverConfig.h
INC += ArgParser.h
INC += ArrayTools.h
INC += ASCIIParser.h
INC += AVLTree.h
INC += BenchTimer.h
INC += BinaryTree.h
INC += BinIO.h
INC += Bitset.h
INC += CGIDemangler.h
INC += Conversions.h
INC += epicsTimeHelper.h       
INC += Filename.h
INC += GenericException.h
INC += Guard.h
INC += Histogram.h
INC += InsertionSort.h
INC += MemoryPool.h 
INC += MemoryBuffer.h 
INC += MsgLogger.h
INC += NetTools.h       
INC += RegularExpression.h 
INC += stdString.h 
INC += string2cp.h
INC += VectorHelper.h       
INC += FUX.h       
LIB_SRCS += ArchiveException.cpp
LIB_SRCS += ArgParser.cpp
LIB_SRCS += ASCIIParser.cpp
LIB_SRCS += BenchTimer.cpp
LIB_SRCS += BinIO.cpp
LIB_SRCS += Bitset.cpp
LIB_SRCS += CGIDemangler.cpp
LIB_SRCS += epicsTimeHelper.cpp
LIB_SRCS += Filename.cpp
LIB_SRCS += GenericException.cpp
LIB_SRCS += MemoryPool.cpp
LIB_SRCS += MsgLogger.cpp
LIB_SRCS += NetTools.cpp
LIB_SRCS += RegularExpression.cpp
LIB_SRCS += stdString.cpp
LIB_SRCS += FUX.cpp
# WIN32 only...
LIB_SRCS_WIN32 := gnu_regex.c
LIBRARY_HOST := Tools
# In case of shared lib, link these:
Tools_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build locally w/o installation
PROD_LIBS_DEFAULT := Tools    $(EPICS_BASE_IOC_LIBS)
PROD_LIBS_WIN32   := ToolsObj $(EPICS_BASE_IOC_LIBS)
Tools_DIR    = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)
TESTPROD_HOST += ToolsTest

include $(TOP)/configure/RULES
