# EPICS R3.14 Makefile for the rtree library
TOP=../../..
include $(TOP)/configure/CONFIG
include $(TOP)/src/ChannelArchiver/make.cfg

#INC +=

# Contents of library
LIB_SRCS += archiver_index.cpp
LIB_SRCS += archiver_unit.cpp
LIB_SRCS += array_list.cpp
LIB_SRCS += au_list.cpp
LIB_SRCS += au_pointer.cpp
LIB_SRCS += au_pointer_list.cpp
LIB_SRCS += aup_iterator.cpp
LIB_SRCS += bin_io.cpp
LIB_SRCS += channel_name_iterator.cpp
LIB_SRCS += cntu.cpp
LIB_SRCS += cntu_address_iterator.cpp
LIB_SRCS += cntu_table.cpp
LIB_SRCS += file_allocator.cpp
LIB_SRCS += interval.cpp
LIB_SRCS += key_au_iterator.cpp
LIB_SRCS += key_object.cpp
LIB_SRCS += my_epics_time.cpp
LIB_SRCS += r_entry.cpp
LIB_SRCS += r_tree.cpp
LIB_SRCS += r_tree_free_space_manager.cpp
LIB_SRCS += r_tree_root.cpp

# Library to build
LIBRARY_HOST := rtree

TESTPROD_HOST+= file_allocator_test archiver_index_test
PROD_LIBS_DEFAULT := rtree Tools ca Com
PROD_LIBS_WIN32 := rtreeObj ToolsObj ca Com
rtree_DIR = $(INSTALL_LIB)
rtreeObj_DIR = $(INSTALL_LIB)
Tools_DIR = $(INSTALL_LIB)
ToolsObj_DIR = $(INSTALL_LIB)

include $(TOP)/configure/RULES
