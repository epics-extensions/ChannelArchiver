// DirectoryFile.cpp
//////////////////////////////////////////////////////////////////////

#include "LowLevelIO.h"
#include "DirectoryFile.h"
#include "ArchiveException.h"
#include "MsgLogger.h"
#include "Filename.h"

BEGIN_NAMESPACE_CHANARCH

//#define LOG_DIRFILE

//////////////////////////////////////////////////////////////////////
// DirectoryFile
//////////////////////////////////////////////////////////////////////

// Attach DiskBasedHashTable to disk file of given name.
// a) new file: setup Hash Table
// b) existing file for read-only: check HT
// c) existing file for read-write: check HT
DirectoryFile::DirectoryFile (const stdString &filename, bool for_write)
{
    _filename = filename;
    Filename::getDirname (_filename, _dirname);
    
    if (for_write)
    {
        if (! _file.llopen (filename.c_str (), true))
            throwDetailedArchiveException (CreateError, filename);
    }
    else
    {
        if (! _file.llopen (filename.c_str ()))
            throwDetailedArchiveException (OpenError, filename);
    }

    // Does file contain HT?
    _next_free_entry = _file.llseek_end (0);
    if (_next_free_entry < FirstEntryOffset)
    {
        if (!for_write) // ... but it should
            throwDetailedArchiveException (Invalid, "Missing HT");

        // Initialize HT:
        for (HashTable::HashValue entry = 0;
             entry < HashTable::HashTableSize; ++entry)
            writeHTEntry (entry, INVALID_OFFSET);
    
        // Next free entry = first entry after HT
        _next_free_entry = FirstEntryOffset;
    }
    
    // Check if file size = HT + N full entries
    FileOffset rest = (_next_free_entry - FirstEntryOffset)
        % BinChannel::getDataSize ();
    if (rest)
        LOG_MSG ("Suspicious directory file:\n"
                 << filename << " has a 'tail' of " << rest << " Bytes\n");
    
#ifdef LOG_DIRFILE
    if (_file.isReadonly ())
        LOG_MSG ("(readonly) ");
    LOG_MSG ("DirectoryFile " << _filename << "\n");
#endif
}

DirectoryFile::~DirectoryFile()
{
#ifdef LOG_DIRFILE
    if (_file.isReadonly ())
        LOG_MSG ("(readonly) ");
    LOG_MSG ("~DirectoryFile " << _filename << "\n");
#endif
    _file.llclose ();
}

DirectoryFileIterator DirectoryFile::findFirst ()
{
    DirectoryFileIterator       i (this);
    i.findValidEntry (0);

    return i;
}

// Try to locate entry with given name.
DirectoryFileIterator DirectoryFile::find (const stdString &name)
{
    DirectoryFileIterator i (this);

    i._hash   = HashTable::Hash (name.c_str ());
    FileOffset offset = readHTEntry (i._hash);
    while (offset != INVALID_OFFSET)
    {
        i.getChannel()->read (_file, offset);
        if (name == i.getChannel()->getName ())
            return i;
        offset = i.getChannel()->getNextEntryOffset ();
    }
    i.getChannel()->clear();
    
    return i;
}

// Add a new entry to HT.
// Throws Invalid if that entry exists already.
//
// After calling this routine the current entry
// is undefined. It must be initialized and
// then written with saveEntry ().
DirectoryFileIterator DirectoryFile::add (const stdString &name)
{
    DirectoryFileIterator i (this);
	BinChannel *channel = i.getChannel ();
    const char *cname = name.c_str();

    i._hash = HashTable::Hash (cname);
    FileOffset offset = readHTEntry (i._hash);

    if (offset == INVALID_OFFSET) // Empty HT slot:
        writeHTEntry (i._hash, _next_free_entry);
    else
    {       // Follow the entry chain that hashed to this value:
        FileOffset next = offset;
        while (next != INVALID_OFFSET)
        {
            channel->read (_file, next);
            if (name == channel->getName ()) // already there?
                return i;
            next = channel->getNextEntryOffset ();
        }
        // i._entry: last entry in chain.
        // make that one point to new entry:
        channel->setNextEntryOffset (_next_free_entry);
        channel->write (_file, channel->getOffset());
    }

    // Last entry points now to _next_free_entry.
    // Create the new entry there:
    channel->init (cname);
    channel->setNextEntryOffset (INVALID_OFFSET);
    channel->write (_file, _next_free_entry);
    _next_free_entry += channel->getDataSize ();
    
    return i;
}

FileOffset DirectoryFile::readHTEntry (HashTable::HashValue entry) const
{
    FileOffset offset;
    FileOffset pos = entry * sizeof (FileOffset);
    
    if (_file.llseek (pos) != pos   ||
        !_file.llread (&offset, sizeof (FileOffset)))
        throwArchiveException (ReadError);
    FileOffsetFromDisk (offset);
    
    return offset;
}

void DirectoryFile::writeHTEntry (HashTable::HashValue entry, FileOffset offset)
{       // offset is value parm -> safe to convert in place
    FileOffsetToDisk (offset);
    FileOffset pos = entry * sizeof (FileOffset);
    if (_file.llseek (pos) != pos   ||
        !_file.llwrite (&offset, sizeof (FileOffset)))
        throwArchiveException (WriteError);
}

//////////////////////////////////////////////////////////////////////
// DirectoryFileIterator
//////////////////////////////////////////////////////////////////////

void DirectoryFileIterator::clear ()
{
    _dir = 0;
    _hash = HashTable::HashTableSize;
    _entry.clear ();
}

DirectoryFileIterator::DirectoryFileIterator ()
{
    clear ();
}

DirectoryFileIterator::DirectoryFileIterator (DirectoryFile *dir)
{
    clear ();
    _dir = dir;
}

DirectoryFileIterator::DirectoryFileIterator (const DirectoryFileIterator &dir)
{
    clear ();
    *this = dir;
}

DirectoryFileIterator & DirectoryFileIterator::operator = (const DirectoryFileIterator &rhs)
{
    // Right now, this is actually what the default copy op. would do:
    _dir = rhs._dir;
    _entry = rhs._entry;
    _hash = rhs._hash;
    return *this;
}

bool DirectoryFileIterator::next ()
{
    if (_hash >= HashTable::HashTableSize ||
        _entry.getOffset() == INVALID_OFFSET ||
        _dir == 0)
        return false;
    
    // Have a current entry.
    // Ask it for pointer to next entry:
    FileOffset next = _entry.getNextEntryOffset ();
    if (next != INVALID_OFFSET)
    {
        _entry.read (_dir->_file, next);
        return isValid();
    }
    // End of entries that relate to current _hash value,
    // switch to next value:
    return findValidEntry (_hash + 1);
}

// Search HT for the first non-empty entry:
bool DirectoryFileIterator::findValidEntry (HashTable::HashValue start)
{
    _entry.clear ();
    if (!_dir)
        return false;
    
        // Loop HT from 'start'
    FileOffset tmp;
    for (_hash = start; _hash < HashTable::HashTableSize; ++_hash)
    {
        // Get first entry's position from HT
        tmp = _dir->readHTEntry (_hash);
        // If valid, read that entry
        if (tmp != INVALID_OFFSET)
        {
            _entry.read (_dir->_file, tmp);
            return isValid();
        }
    }
    return false;
}

void DirectoryFileIterator::save ()
{
    if (_dir)
        _entry.write (_dir->_file, _entry.getOffset());
}

END_NAMESPACE_CHANARCH
