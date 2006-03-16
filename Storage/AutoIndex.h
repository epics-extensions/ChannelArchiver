// -*- c++ -*-

#ifndef __AUTO_INDEX_H__
#define __AUTO_INDEX_H__

// Tools
#include <AutoPtr.h>
// Storage
#include <Index.h>

/// \addtogroup Storage
/// @{

/// Index which automatically picks ListIndex or FileIndex
/// when reading, based on looking at the first few bytes
/// in the index file.
class AutoIndex : public Index
{
public:
    ~AutoIndex();

    virtual void open(const stdString &filename, bool readonly=true);

    virtual void close();
    
    virtual class RTree *addChannel(const stdString &channel,
                                    stdString &directory);

    virtual class RTree *getTree(const stdString &channel,
                                 stdString &directory);

    virtual bool getFirstChannel(NameIterator &iter);

    virtual bool getNextChannel(NameIterator &iter);

private:
    stdString filename;
    AutoPtr<Index> index;
};

#endif
