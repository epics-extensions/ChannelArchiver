// -*- c++ -*-

#ifndef __NAME_HASH_H__
#define __NAME_HASH_H__

// Tools
#include <stdString.h>
// Index
#include "FileAllocator.h"

/// \ingroup Storage
/// \@{

/// A file-based Hash table for strings.

/// Beginning at the 'anchor' position in the file,
/// NameHash deposits the start offset of the hash table
/// and the number of table entries.
///
/// Each hash table entry is a file offset that points
/// to the beginnig of the NameHash::Entry list for that
/// hash value.
class NameHash
{
public:
    class Entry
    {
    public:
        stdString  name;  // Channel Name.
        stdString  ID_txt;// String and numeric ID
        FileOffset ID;    // (filename and offset to RTree for the channel).
        FileOffset next;  // Offset to next entry w/ same hash value

        FileOffset offset;// Offset of this Entry
        FileOffset getSize() const;
        bool write(FILE *f) const;
        bool read(FILE *f);
    };

    static const long anchor_size = 8;
    
    /// Constructor
    
    /// \param anchor: The NameHash will deposit its root pointer there.
    ///                Caller needs to assert that there are anchor_size
    ///                bytes available at that location in the file.
    NameHash(FileAllocator &fa, FileOffset anchor);

    /// Create a new hash table of given size.

    /// \param ht_size determines the hash table size and should be prime.
    ///
    ///
    bool init(unsigned long ht_size=1009);

    /// Attach to existing hash table
    bool reattach();

    /// Insert name w/ ID
    bool insert(const stdString &name, const stdString &ID_txt, FileOffset ID);

    /// Locate name and obtain its ID. Returns true on success
    bool find(const stdString &name, stdString &ID_txt, FileOffset &ID);
    
    /// Start iterating over all entries (in table's order).

    /// \return Returns true if hashvalue & entry were set to something valid.
    ///
    ///
    bool startIteration(unsigned long &hashvalue, Entry &entry);
    
    /// Get next entry during iteration.

    /// \pre start_iteration() was successfully invoked.
    /// \return Returns true on success.
    ///
    bool nextIteration(unsigned long &hashvalue, Entry &entry);
    
    /// Get hash value (public to allow for test code)
    long hash(const stdString &name) const;  

    /// Generate info on table fill ratio and list length
    void showStats(FILE *f);
private:
    FileAllocator &fa;
    FileOffset anchor;       // Where offset gets deposited in file
    unsigned long ht_size;   // Hash Table size (entries, not bytes)
    FileOffset table_offset; // Start of HT in file
    bool read_HT_entry(unsigned long hash_value, FileOffset &offset);
    bool write_HT_entry(unsigned long hash_value, FileOffset offset) const;
};

/// \@}

#endif
