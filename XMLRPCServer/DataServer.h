// -*- c++ -*-

// DataServer.h

// XML-RPC does not define fault codes.
// The xml-rpc-c library uses -500, -501, ... (up to -510)
#define ARCH_DAT_SERV_FAULT -600
#define ARCH_DAT_NO_INDEX -601        

// Type codes returned by archiver.get_values
#define XML_STRING 0
#define XML_ENUM 1
#define XML_INT 2
#define XML_DOUBLE 3
