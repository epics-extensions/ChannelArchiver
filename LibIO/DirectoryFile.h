#if !defined(_DIRECTORYFILE_H_)
#define _DIRECTORYFILE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BinChannel.h"
#include "HashTable.h"

// The Directory File is a disk-based hash table:
//
// First, there is a hash table with HashTable::HashTableSize
// entries. Each entry maps a channel name to a file offset.
// At the given file offset, there follow lists of BinChannel
// entries.
//
// Find a channel:
//
// hash name, get offset from HashTable,
// read the BinChannel at the given offset.
// Does it match the name? If not, follow the "next"
// pointer in the BinChannel to the next Channel entry.
//
// DirectoryFile::_next_free_entry points to the end
// of the DirectoryFile.

//////////////////////////////////////////////////////////////////////
// DirectoryFileIterator
//////////////////////////////////////////////////////////////////////

class DirectoryFile;

// DirectoryFileIterator allows read/write access
// to individual Channels in a DirectoryFile.
//
// This is for internal use, user code should use
// Archive and related classes.
// (Low level with full access.
//  ChannelIterator is high level w/ reg. expression
//  and limited access to raw data)
class DirectoryFileIterator
{
public:
    // DirectoryFileIterator has to be bound to DirectoryFile:
    DirectoryFileIterator();
    DirectoryFileIterator(DirectoryFile *dir);
    DirectoryFileIterator(const DirectoryFileIterator &dir);
    DirectoryFileIterator & operator = (const DirectoryFileIterator &rhs);

    bool isValid() const   
    {   return _entry.getOffset() != INVALID_OFFSET; }

    BinChannel *getChannel()
    {   return &_entry; }
    const BinChannel *getChannel() const
    {   return &_entry; }

    // Move to next DirectoryEntry
    bool next();

    bool isEqual(const DirectoryFileIterator& rhs) const
    {   return (_entry.getOffset() == rhs._entry.getOffset() && _dir == rhs._dir); }

    void save();

private:
    friend class DirectoryFile;
    void clear();

    bool findValidEntry(HashTable::HashValue start);

    bool operator == (const DirectoryFileIterator& rhs) const; // not impl.
    bool operator != (const DirectoryFileIterator& rhs) const; // not impl.

    DirectoryFile           *_dir;
    BinChannel              _entry;
    HashTable::HashValue    _hash;  // ... for _entry
};

//////////////////////////////////////////////////////////////////////
// DirectoryFile
//////////////////////////////////////////////////////////////////////

class DirectoryFile
{
public:
    // Attach DirectoryFile to disk file.
    // Existing file is opened, otherwise new one is created
    // for for_write==true.
    DirectoryFile(const stdString &filename, bool for_write=false);

    // Close file.
    ~DirectoryFile();

    DirectoryFileIterator findFirst();

    // Try to locate entry with given name.
    DirectoryFileIterator find(const stdString &name);

    // Add new DirecotryEntry with given name.
    // Entry will be empty, i.e. point to no data file.
    DirectoryFileIterator add(const stdString &name);

    // Remove name from directory file.
    // Will not remove data but only "pointers" to the data!
    bool remove(const stdString &name);

    const stdString &getDirname()    {   return _dirname;  }

    bool isReadonly()                {   return _file.isReadonly(); }

private:
    friend class DirectoryFileIterator;
    enum
    {
        FirstEntryOffset = HashTable::HashTableSize * sizeof(FileOffset)
    };

    // Prohibit assignment: two DirectoryFiles cannot access the same file
    // (However, more than one iterator are OK)
    DirectoryFile(const DirectoryFile &);
    DirectoryFile &operator =(const DirectoryFile &);

    // Read (first) FileOffset for given HashValue
    FileOffset readHTEntry(HashTable::HashValue entry) const;

    // Write (first) FileOffset for given HashValue
    void writeHTEntry(HashTable::HashValue entry, FileOffset offset);

    // Search HT for the first non-empty entry:
    FileOffset lookForValidEntry(HashTable::HashValue start) const;

    stdString    _filename;
    stdString    _dirname;
    LowLevelIO   _file;

    // Offset of next unused entry for add:
    FileOffset   _next_free_entry;
};

#endif // !defined(_DIRECTORYFILE_H_)
