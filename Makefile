# EPICS R3.14 Makefile for the Channel Archiver

TOP=../..
include $(TOP)/configure/CONFIG

DIRS += Tools
DIRS += rtree
DIRS += Storage
DIRS += LibIO
DIRS += DataTool
DIRS += Engine
DIRS += XMLRPCServer

DIRS += Manager
DIRS += Export

include $(TOP)/configure/RULES_DIRS
