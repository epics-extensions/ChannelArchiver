// -*- c++ -*-

#ifndef __AUTO_INDEX_H__
#define __AUTO_INDEX_H__

// Tools
#include <AutoPtr.h>
// Storage
#include <Index.h>
#include <NoCopy.h>

/** \ingroup Storage
 *  General Index for reading.
 * 
 *  Index which automatically picks ListIndex or IndexFile
 *  when reading, based on looking at the first few bytes
 *  in the index file.
 */
class AutoIndex : public Index
{
public:
    AutoIndex() {}

    ~AutoIndex();

    virtual void open(const stdString &filename, ReadWrite readwrite=ReadOnly);

    virtual void close();
    
    virtual Result *addChannel(const stdString &channel);

    virtual Result *findChannel(const stdString &channel);

    virtual NameIterator *iterator();

private:
    PROHIBIT_DEFAULT_COPY(AutoIndex);
    AutoPtr<Index> index;
};

#endif
