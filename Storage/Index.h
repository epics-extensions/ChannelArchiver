/// -*- c++ -*-

#ifndef __INDEX_H__
#define __INDEX_H__

// Storage
#include <NameHash.h>

/// \ingroup Storage
/// \@{

/// Base class for the archiver's indices.
class Index
{
public:
    virtual ~Index();
    
    /// Open an index.
    virtual bool open(const stdString &filename, bool readonly=true) = 0;

    /// Close the index.
    virtual void close() = 0;
    
    /// Add a channel to the index.

    /// A channel has to be added before data blocks get defined
    /// for the channel. When channel is already in index, existing
    /// tree gets returned.
    ///
    /// Caller must delete the tree pointer.
    virtual class RTree *addChannel(const stdString &channel,
                                    stdString &directory) = 0;

    /// Obtain the RTree for a channel.

    /// Caller must delete the tree pointer.
    ///
    /// Directory is set to the path/directory of the index,
    /// which together with the data block in the RTree will then
    /// lead to the actual data files.
    virtual class RTree *getTree(const stdString &channel,
                                 stdString &directory) = 0;
    
    /// Used by get_first_channel(), get_next_channel().
    class NameIterator
    {
    public:
        const stdString &getName() 
        {    return entry.name; }
    private:
        friend class IndexFile;
        unsigned long hashvalue;
        NameHash::Entry entry;
    };

    /// Locate NameIterator on first channel.
    virtual bool getFirstChannel(NameIterator &iter) = 0;

    /// Locate NameIterator on next channel.

    /// \pre Successfull call to get_first_channel().
    ///
    ///
    virtual bool getNextChannel(NameIterator &iter) = 0;

};

/// \@}

#endif
