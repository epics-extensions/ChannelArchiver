// -*- c++ -*-

#ifndef __LIST_INDEX_H__
#define __LIST_INDEX_H__

// Storage
#include <IndexFile.h>
#include <IndexConfig.h>

/// \ingroup Storage
/// @{

/// Support list of sub-archives.

/// Ideally, an index configuration conforming to indexfile.dtd
/// is used to create a master index via the ArchiveIndexTool.
/// Alternatively, that list of sub-archives can be used with
/// a ListIndex, which then acts like a master index by simply querying
/// the sub-archives one by one:
/// - Less efficient for retrieval
/// - Easier to set up and maintain, because
///   you only create the config file without need
///   to run the ArchiveIndexTool whenever any of
///   the sub-archives change.
class ListIndex : public Index
{
public:
    ListIndex();

    virtual bool open(const stdString &filename, bool readonly=true);

    virtual void close();
    
    virtual class RTree *addChannel(const stdString &channel,
                                    stdString &directory);

    virtual class RTree *getTree(const stdString &channel,
                                 stdString &directory);

    virtual bool getFirstChannel(NameIterator &iter);

    virtual bool getNextChannel(NameIterator &iter);

private:
    stdString filename;
    // List of all the sub-archives
    IndexConfig config;
    // The sub-index, maybe open to one of the sub-archives
    IndexFile index;
    
    // Is it currently open?
    bool is_open;
};

#endif
