// -*- c++ -*-

#ifndef __INDEX_CONFIG_H__
#define __INDEX_CONFIG_H__

// Tools
#include <ToolsConfig.h>

/// \ingroup Tools

/// Parser for indexconfig.dtd.

/// The index configuration is used by both the ArchiveIndexTool
/// and the simple file-by-file mechanism supported by the
/// ListedIndex.
class IndexConfig
{
public:
    bool parse(const stdString &config_name);
    
    stdList<stdString> subarchives;
};

#endif

