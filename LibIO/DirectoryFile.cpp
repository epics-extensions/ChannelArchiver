// DirectoryFile.cpp
//////////////////////////////////////////////////////////////////////

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
// Existing file is opened, otherwise new one is created.
DirectoryFile::DirectoryFile (const stdString &filename, bool for_write)
{
	_readonly = false;
	_filename = filename;
	Filename::getDirname (_filename, _dirname);
	
	// Default: read and write access
	_fd = fopen (filename.c_str (), "r+b");
	if (! _fd)	// retry readonly
	{
		_fd = fopen (filename.c_str (), "rb");
		_readonly = true;
	}
	if (_fd)
	{	// Open existing file.
		// Find next free entry after end of current file.
		// Couldn't find this in the online manual
		// but tests gave that SEEK_END gives the # of bytes in file
		// which is one byte _after_ the last valid position.
		fseek (_fd, 0, SEEK_END);
		_next_free_entry = ftell (_fd);

		// Does file contain HT?
		if (_next_free_entry < FirstEntryOffset)
			throwArchiveException (Invalid);
		// Check if file size = HT + N full entries
		FileOffset rest = 
			(_next_free_entry - FirstEntryOffset) % BinChannel::getDataSize ();
		if (rest)
		{
			LOG_MSG ("Suspicious directory file:\n"
				<< filename << " has a 'tail' of " << rest << " Bytes\n");
		}
	}
	else
	{
		// create new file
		if (! for_write)
			throwDetailedArchiveException (OpenError, filename);
		_fd = fopen (filename.c_str (), "w+b");
		if (! _fd)
			throwDetailedArchiveException (CreateError, filename);

		// Initialize HT:
		for (HashTable::HashValue entry = 0;
			entry < HashTable::HashTableSize; ++entry)
			writeHTEntry (entry, INVALID_OFFSET);

		// Next free entry = first entry after HT
		_next_free_entry = FirstEntryOffset;
	}
#ifdef LOG_DIRFILE
	LOG_MSG ("DirectoryFile " << _filename << " - new\n");
#endif
}

DirectoryFile::~DirectoryFile()
{
#ifdef LOG_DIRFILE
	LOG_MSG ("~DirectoryFile " << _filename << " - new\n");
#endif
	if (_fd)
		fclose (_fd);
	_fd = 0;
}

DirectoryFileIterator DirectoryFile::findFirst ()
{
	DirectoryFileIterator	i (this);
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
		i.getChannel()->read (_fd, offset);
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
	const char *cname = name.c_str();

	i._hash = HashTable::Hash (cname);
	FileOffset offset = readHTEntry (i._hash);

	if (offset == INVALID_OFFSET) // Empty HT slot:
		writeHTEntry (i._hash, _next_free_entry);
	else
	{	// Follow the entry chain that hashed to this value:
		FileOffset next = offset;
		while (next != INVALID_OFFSET)
		{
			i.getChannel()->read (_fd, next);
			if (name == i.getChannel()->getName ()) // already there?
				return i;
			next = i.getChannel()->getNextEntryOffset ();
		}
		// i._entry: last entry in chain.
		// make that one point to new entry:
		i.getChannel()->setNextEntryOffset (_next_free_entry);
		i.getChannel()->write (_fd, i.getChannel()->getOffset());
	}

	// Last entry points now to _next_free_entry.
	// Create the new entry there:
	i.getChannel()->init (cname);
	i.getChannel()->setNextEntryOffset (INVALID_OFFSET);
	i.getChannel()->write (_fd, _next_free_entry);
	_next_free_entry += i.getChannel()->getDataSize ();

	return i;
}

FileOffset DirectoryFile::readHTEntry (HashTable::HashValue entry) const
{
	FileOffset	offset;
	if (fseek (_fd, entry * sizeof (FileOffset), SEEK_SET)  ||
		fread (&offset, sizeof (FileOffset), 1, _fd) != 1)
		throwArchiveException (ReadError);
	FileOffsetFromDisk (offset);

	return offset;
}

void DirectoryFile::writeHTEntry (HashTable::HashValue entry, FileOffset offset)
{	// offset is value parm -> safe to convert in place
	FileOffsetToDisk (offset);
	if (fseek (_fd, entry * sizeof (FileOffset), SEEK_SET)  ||
		fwrite (&offset, sizeof (FileOffset), 1, _fd) != 1)
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
{	clear ();	}

DirectoryFileIterator::DirectoryFileIterator (DirectoryFile *dir)
{
	clear ();
	_dir = dir;
}

DirectoryFileIterator::DirectoryFileIterator (DirectoryFileIterator &dir)
{
	clear ();
	*this = dir;
}

DirectoryFileIterator & DirectoryFileIterator::operator = (const DirectoryFileIterator &rhs)
{	// Right now, this is actually what the default copy op. would do:
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
		_entry.read (_dir->_fd, next);
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
			_entry.read (_dir->_fd, tmp);
			return isValid();
		}
	}
	return false;
}

void DirectoryFileIterator::save ()
{
	if (_dir)
		_entry.write (_dir->_fd, _entry.getOffset());
}

END_NAMESPACE_CHANARCH
