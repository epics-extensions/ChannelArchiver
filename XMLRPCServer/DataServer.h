// -*- c++ -*-

// DataServer.h


// The version of this server
// (We use 0 as long as we didn't really release anything)
#define ARCH_VER 0

// If defined, the server writes log messages into
// this file. That helps with debugging because
// otherwise your XML-RPC clients are likely
// to only show "error" and you have no clue
// what's happening.
#define LOGFILE "/tmp/archserver.log"

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
