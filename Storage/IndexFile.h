/// -*- c++ -*-

#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

// Index
#include <NameHash.h>
#include <RTree.h>

class TreeCacheEntry
{
public:
    TreeCacheEntry() : tree(0) {}
    ~TreeCacheEntry()
    {
        if (tree)
            delete tree;
    }
    stdString channel;
    RTree *tree;
};

/// \ingroup Storage
/// @{
/// Defines routines for the RTree-based index.

/// The archiver's index file.

/// The IndexFile combines the NameHash for channel name
/// lookup with one RTree per channel into an index.
///
/// The file itself starts with the IndexFile cookie,
/// followed by the NameHash anchor.
/// Those two items constitute the 'reserved space'
/// all the remaining space is handled by the FileAllocator.
/// The ID of each NameHash entry points to an RTree anchor.
///
/// The IndexFile class maintains a cache of all the RTrees
/// that were used. So whenever one requests an RTree for one
/// channel in the index, that RTree stays in memory until the
/// IndexFile class gets deleted.
class IndexFile
{
public:
    static const long cookie = 0x43414931; // == 'CAI1', Chan. Arch. Index 1

    IndexFile();

    /// The hash table size used for new channel name tables.
    static long ht_size;
    
    /// Open an index.
    bool open(const stdString &filename, bool readonly=true);

    /// Close the index.
    void close();
    
    /// Add a channel to the index.

    /// A channel has to be added before data blocks get defined
    /// for the channel. When channel is already in index, existing
    /// tree gets returned.
    ///
    /// Caller must not delete the tree pointer.
    class RTree *addChannel(const stdString &channel);

    /// Obtain the RTree for a channel.

    /// Caller must not delete the tree pointer.
    ///
    ///
    class RTree *getTree(const stdString &channel);
    
    /// Used by get_first_channel(), get_next_channel().
    class NameIterator
    {
    public:
        const stdString &getName() 
        {    return entry.name; }
    private:
        friend class IndexFile;
        long hashvalue;
        NameHash::Entry entry;
    };

    /// Locate NameIterator on first channel.
    bool getFirstChannel(NameIterator &iter);

    /// Locate NameIterator on next channel.

    /// \pre Successfull call to get_first_channel().
    bool getNextChannel(NameIterator &iter);

    /// Returns directory (path) of this index.
    const stdString &getDirectory() const
    {    return dirname; }

    void showHashStats(FILE *f);
   
    mutable size_t cache_hits, cache_misses;

private:
    mutable AVLTree<TreeCacheEntry> tree_cache;

    FILE *f;
    FileAllocator fa;
    NameHash names;
    stdString filename, dirname;
};

/// @}


#endif
