// -*- c++ -*-

// ArchiveDataServer.h

// XML-RPC
#include <xmlrpc.h>
// EPICS Base
#include <epicsTime.h>

/// \defgroup DataServer
/// Code related to the network data server

// The version of this server
// (We use 0 as long as we didn't really release anything)
#define ARCH_VER 0

// Code numbers for 'how'
// Raw data, channel by channel
#define HOW_RAW      0
// Raw data in 'filled' spreadsheet
#define HOW_SHEET    1
// Averaged spreadsheet
#define HOW_AVERAGE  2
// Plot-binned, channel by channel
#define HOW_PLOTBIN  3
// Linear interpolation spreadsheet
#define HOW_LINEAR   4

// XML-RPC does not define fault codes.
// The xml-rpc-c library uses -500, -501, ... (up to -510)
#define ARCH_DAT_SERV_FAULT -600
#define ARCH_DAT_NO_INDEX -601        
#define ARCH_DAT_ARG_ERROR -602

// Type codes returned by archiver.get_values
#define XML_STRING 0
#define XML_ENUM 1
#define XML_INT 2
#define XML_DOUBLE 3

// meta.type as returned by archiver.get_values
#define META_TYPE_ENUM    0
#define META_TYPE_NUMERIC 1

void epicsTime2pieces(const epicsTime &t,
                      xmlrpc_int32 &secs, xmlrpc_int32 &nano);

// Inverse to epicsTime2pieces
void pieces2epicsTime(xmlrpc_int32 secs, xmlrpc_int32 nano, epicsTime &t);
