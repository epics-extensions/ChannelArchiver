# EPICS R3.14 Makefile for the Channel Archiver

TOP=../..
include $(TOP)/configure/CONFIG

DIRS += Tools
DIRS += rtree
DIRS += StorageLib
DIRS += LibIO
DIRS += DataTool
DIRS += Engine
DIRS += XMLRPCServer

DIRS += Manager
DIRS += Export
DIRS += CGIExport

include $(TOP)/configure/RULES_DIRS
