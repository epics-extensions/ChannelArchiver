/// -*- c++ -*-

#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

// Tools
#include <AutoFilePtr.h>
// Storage
#include <Index.h>
#include <NameHash.h>

/// \addtogroup Storage
/// @{

/** RTree-based index file.
 *  <p>
 *  The IndexFile combines the NameHash for channel name
 *  lookup with one RTree per channel into an index.
 *  <p>
 *  The file itself starts with the IndexFile cookie,
 *  followed by the NameHash anchor.
 *  Those two items constitute the 'reserved space'
 *  all the remaining space is handled by the FileAllocator.
 *  The ID of each NameHash entry points to an RTree anchor.
 *  <p>
 *  For a 'full' index, the RTree data blocks point to the actual
 *  data file and offset of the referenced data.
 *  For a 'shallow' index, the RTree points to further index files,
 *  offset 0.
 *  This distinction is made outside of the IndexFile class by
 *  the ArchiveIndexTool that builds the indices respectivly the DataReader
 *  classes that read data.
 */
class IndexFile : public Index
{
public:
    // == 'CAI2', Chan. Arch. Index 2
    static const uint32_t cookie = 0x43414932;

    IndexFile(int RTreeM = 50);

    ~IndexFile();

    /**  The hash table size used for new channel name tables. */
    static const uint32_t ht_size;
    
    // Index interface
    virtual void open(const stdString &filename, ReadWrite readwrite=ReadOnly);
    virtual void close();
    virtual Result *addChannel(const stdString &channel);
    virtual Result *findChannel(const stdString &channel);
    virtual NameIterator *iterator();
    
    void showStats(FILE *f);   
    bool check(int level);
    
private:
    const int RTreeM;
    AutoFilePtr f;
    FileAllocator fa;
    NameHash names;
};

/// @}

#endif
