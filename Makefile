# EPICS R3.14 Makefile for the Channel Archiver

TOP=../..
include $(TOP)/configure/CONFIG

DIRS += Tools
DIRS += LibIO
DIRS += Storage
DIRS += Engine
DIRS += DataTool
DIRS += IndexTool
DIRS += ArchiveDaemon
DIRS += XMLRPCServer

DIRS += Manager
DIRS += Export

include $(TOP)/configure/RULES_DIRS
