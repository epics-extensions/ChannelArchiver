// DataFile.cpp
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif
#include <map>

#include "DataFile.h"
#include "ArchiveException.h"
#include "MsgLogger.h"
#include "Filename.h"

//#define LOG_DATAFILE

BEGIN_NAMESPACE_CHANARCH

//////////////////////////////////////////////////////////////////////
// DataHeader
//////////////////////////////////////////////////////////////////////

void DataHeader::read (FILE *fd, FileOffset offset)
{
	if (fseek (fd, offset, SEEK_SET) != 0  ||
		fread (this, sizeof (DataHeader), 1, fd) != 1)
		throwArchiveException (ReadError);
	// convert the data header into host format:
	FileOffsetFromDisk (dir_offset);
	FileOffsetFromDisk (next_offset);
	FileOffsetFromDisk (prev_offset);
	FileOffsetFromDisk (curr_offset);
	ULONGFromDisk (num_samples);
	FileOffsetFromDisk (config_offset);
	ULONGFromDisk (buf_size);
	ULONGFromDisk (buf_free);
	USHORTFromDisk (dbr_type);
	USHORTFromDisk (nelements);
	DoubleFromDisk (period);
	TS_STAMPFromDisk (begin_time);
	TS_STAMPFromDisk (next_file_time);
	TS_STAMPFromDisk (end_time);
}

void DataHeader::write (FILE *fd, FileOffset offset) const
{
	DataHeader copy (*this);
	// convert the data header into host format:
	FileOffsetToDisk (copy.dir_offset);
	FileOffsetToDisk (copy.next_offset);
	FileOffsetToDisk (copy.prev_offset);
	FileOffsetToDisk (copy.curr_offset);
	ULONGToDisk (copy.num_samples);
	FileOffsetToDisk (copy.config_offset);
	ULONGToDisk (copy.buf_size);
	ULONGToDisk (copy.buf_free);
	USHORTToDisk (copy.dbr_type);
	USHORTToDisk (copy.nelements);
	DoubleToDisk (copy.period);
	TS_STAMPToDisk (copy.begin_time);
	TS_STAMPToDisk (copy.next_file_time);
	TS_STAMPToDisk (copy.end_time);

#if 0
	{
		stdString test = prev_file;
		LOG_ASSERT (test.find ('/') == test.npos);
	}
#endif

	if (fseek (fd, offset, SEEK_SET) != 0  ||
		fwrite (&copy, sizeof (DataHeader), 1, fd) != 1)
		throwArchiveException (WriteError);
}

//////////////////////////////////////////////////////////////////////
// DataFile
//////////////////////////////////////////////////////////////////////

// Map of all DataFiles currently open
#ifdef USE_STD_MAP

typedef map<stdString, DataFile *> FileMap;
static FileMap	open_data_files;

#else

class FileMap
{
public:
	stdString			_name;
	DataFile		*_file;

	static DataFile *find (const stdString &name);
	static void insert (const stdString &name, DataFile *file);
	static void erase (const stdString &name);

private:
	FileMap			*_next;
	static FileMap	*_root;
};

FileMap	*FileMap::_root = 0;

DataFile *FileMap::find (const stdString &name)
{
	FileMap	*n = _root;

	while (n)
	{
		if (n->_name == name)
			return n->_file;
		n = n->_next;
	}
	return 0;
}

void FileMap::insert (const stdString &name, DataFile *file)
{
	LOG_ASSERT (find (name) == 0);

	FileMap *n = new FileMap;
	n->_name = name;
	n->_file = file;
	n->_next = _root;
	_root = n;
}

void FileMap::erase (const stdString &name)
{
	FileMap	**n = &_root;

	while (*n)
	{
		if ((*n)->_name == name)
		{
			FileMap	*to_del = *n;

			*n = to_del->_next;
			delete to_del;

			return;
		}
		n = &((*n)->_next);
	}
	LOG_ASSERT (false);
}

#endif

DataFile *DataFile::reference (const stdString &filename)
{
	DataFile *file;

#ifdef USE_STD_MAP
	FileMap::iterator i = open_data_files.find (filename);
	if (i == open_data_files.end ())
	{
		file = new DataFile (filename);
		open_data_files.insert (FileMap::value_type (file->getFilename(), file));
	}
	else
	{
		file = i->second;
		file->reference ();
	}
#else
	file = FileMap::find (filename);
	if (! file)
	{
		file = new DataFile (filename);
		FileMap::insert (file->getFilename(), file);
	}
	else
	{
		file->reference ();
	}
#endif

	return file;
}

// Add reference to current DataFile
DataFile *DataFile::reference ()
{
	++_ref_count;
	return this;
}

// Call instead of delete:
void DataFile::release ()
{
	if (--_ref_count <= 0)
	{
#ifdef USE_STD_MAP
		FileMap::iterator i = open_data_files.find (_filename);
		LOG_ASSERT (i != open_data_files.end ());
		open_data_files.erase (i);
#else
		FileMap::erase (_filename);
#endif
		delete this;
	}
}

DataFile::DataFile (const stdString &filename)
{
	_fd = fopen (filename.c_str(), "r+b");	// try to open
	if (! _fd)
		_fd = fopen (filename.c_str(), "rb");	// try readonly

	if (! _fd)
	{	// failed. Try to create new file
		_fd = fopen (filename.c_str(), "w+b");
		if (! _fd)
			throwDetailedArchiveException (CreateError, filename);
	}

	_ref_count = 1;
	_filename = filename;
	Filename::getDirname  (_filename, _dirname);
	Filename::getBasename (_filename, _basename);
#ifdef LOG_DATAFILE
	LOG_MSG ("DataFile " << _filename << " - new\n");
#endif
}

DataFile::~DataFile ()
{
	if (_fd)
	{
#ifdef LOG_DATAFILE
		LOG_MSG ("DataFile " << _filename << "' - deleted\n");
#endif
		fclose (_fd);
	}
}

// For synchr. with a file that's actively written
// by another prog. is might help to reopen:
void DataFile::reopen ()
{	// use freopen? Or better: use iostream !
	fclose (_fd);
	// File was open -> re-open for "r+b"
	_fd = fopen (_filename.c_str(), "r+b");	// try to open
	if (! _fd)
		_fd = fopen (_filename.c_str(), "rb");// readonly?
	if (! _fd)
		throwDetailedArchiveException (ReadError, _filename);
}

// Add a new value to a buffer.
// Returns false when buffer cannot hold any more values.
bool DataFile::addNewValue (DataHeaderIterator &header, const BinValue &value, bool update_header)
{
	// Is buffer full?
	size_t value_size = value.getRawValueSize ();
	size_t buf_free = header->getBufFree (); 
	if (value_size > buf_free)
		return false;

	unsigned long num = header->getNumSamples();
	value.write (_fd, header.getDataOffset () + header->getCurrent ());

	if (num == 0) // first entry?
	{
		header->setBeginTime (value.getTime ());
		update_header = true;
	}
	header->setCurrent (header->getCurrent () + value_size);
	header->setNumSamples (num + 1);
	header->setBufFree (buf_free - value_size);
	if (update_header)
	{
		header->setEndTime (value.getTime ());
		header.save ();
	}

	return true;
}

// Add new DataHeader to this data file.
// Will allocate space for data.
//
// DataHeader must be prepared
// except for link information (prev/next...).
//
// CtrlInfo will not be written if it's already in this DataFile.
//
// Will not update directory file.
DataHeaderIterator DataFile::addHeader (
	DataHeader &new_header,
	const BinCtrlInfo &ctrl_info,
	DataHeaderIterator *prev_header // may be 0
	)
{
	// Assume that we have to write a new CtrlInfo:
	bool need_ctrl_info = true;

	if (prev_header)
	{
		const stdString &prev_file = prev_header->getBasename ();
		new_header.setPrevFile (prev_file);
		new_header.setPrev (prev_header->getOffset ());
		if (prev_file == _basename)
		{	// check if we have the same CtrlInfo in this file
			BinCtrlInfo prev_info;
			prev_info.read (_fd, (*prev_header)->getConfigOffset ());
			if (prev_info == ctrl_info)
				need_ctrl_info = false;
		}
	}
	else
	{
		new_header.setPrevFile ("");
		new_header.setPrev (0);
	}

	if (need_ctrl_info)
	{
		if (fseek (_fd, 0, SEEK_END))
			throwArchiveException (WriteError);
		new_header.setConfigOffset (ftell (_fd));
		ctrl_info.write (_fd, new_header.getConfigOffset ());
	}
	else
		new_header.setConfigOffset ((*prev_header)->getConfigOffset ());

	// create new data header in this file
	new_header.setNextFile ("");
	new_header.setNext (0);
	if (fseek (_fd, 0, SEEK_END) != 0)
		throwArchiveException (WriteError);
	FileOffset header_offset = ftell (_fd);
	new_header.write (_fd, header_offset);

	// allocate data buffer by writing some marker at the end:
	long marker = 0x0effaced;
	if (fseek (_fd,	header_offset + new_header.getBufSize () - sizeof marker,
					SEEK_SET) != 0 ||
		fwrite (&marker, sizeof marker, 1, _fd) != 1)
		throwArchiveException (WriteError);

	// Now that the new header is complete, make prev. point to new one:
	if (prev_header)
	{
		(*prev_header)->setNextFile (_basename);
		(*prev_header)->setNext (header_offset);
		prev_header->save ();
	}

	DataHeaderIterator header;
	header.attach (this, header_offset, &new_header);
	return header;
}

//////////////////////////////////////////////////////////////////////
// DataHeaderIterator
//////////////////////////////////////////////////////////////////////

void DataHeaderIterator::init ()
{
	_file = 0;
	_header_offset = INVALID_OFFSET;
	_header.num_samples = 0;
}

void DataHeaderIterator::clear ()
{
	if (_file)
		_file->release ();
	init ();
}

DataHeaderIterator::DataHeaderIterator ()
{
	init ();
}

DataHeaderIterator::DataHeaderIterator (const DataHeaderIterator &rhs)
{
	init ();
	*this = rhs;
}

DataHeaderIterator & DataHeaderIterator::operator = (const DataHeaderIterator &rhs)
{
	if (&rhs != this)
	{
		if (rhs._file)
		{
			DataFile *tmp = _file;
			_file = rhs._file->reference ();
			if (tmp)
				tmp->release ();
			_header_offset = rhs._header_offset;
			if (_header_offset != INVALID_OFFSET)
				_header = rhs._header;
		}
		else
			clear ();	
	}
	return *this;
}

DataHeaderIterator::~DataHeaderIterator ()
{
	clear ();
}

void DataHeaderIterator::attach (DataFile *file, FileOffset offset, DataHeader *header)
{	
	if (file)
	{
		DataFile *tmp = _file;
		_file = file->reference ();
		if (tmp)
			tmp->release ();
		_header_offset = offset;
		if (_header_offset != INVALID_OFFSET)
		{
			if (header)
				_header = *header;
			else
				getHeader (offset);
		}
	}
	else
		clear ();	
}

// Re-read the current DataHeader
void DataHeaderIterator::sync ()
{
	if (_header_offset == INVALID_OFFSET)
		throwArchiveException (Invalid);
	_file->reopen ();
	getHeader (_header_offset);
}

FileOffset DataHeaderIterator::getDataOffset() const
{	
	if (_header_offset == INVALID_OFFSET)
		throwArchiveException (Invalid);
	return _header_offset + sizeof (DataHeader);
}

void DataHeaderIterator::getHeader (FileOffset position)
{
	LOG_ASSERT (_file  &&  position != INVALID_OFFSET);
	_header.read (_file->_fd, position);
	_header_offset = position;
}

// Get next header (might be in different data file).
DataHeaderIterator & DataHeaderIterator::operator ++ ()
{
	if (_header_offset == INVALID_OFFSET)
		throwArchiveException (Invalid);

	if (haveNextHeader ())	// Is there a valid next file?
	{
		if (_file->getBasename () != _header.getNextFile ())
		{	// switch to next data file
			stdString next_file;
			Filename::build (_file->getDirname (), _header.getNextFile (),
				next_file);
			_file->release ();
			_file = DataFile::reference (next_file);
		}
		getHeader (_header.getNext ());
	}
	else
	{
		_header_offset = INVALID_OFFSET;
		_header.num_samples = 0;
	}

	return *this;
}

DataHeaderIterator & DataHeaderIterator::operator -- ()
{
	if (_header_offset == INVALID_OFFSET)
		throwArchiveException (Invalid);

	if (havePrevHeader ())	// Is there a valid next file?
	{
		if (_file->getFilename () != _header.getPrevFile ())
		{	// switch to prev data file
			stdString prev_file;
			Filename::build (_file->getDirname (), _header.getPrevFile (),
				prev_file);
			_file->release ();
			_file = DataFile::reference (prev_file);
		}
		getHeader (_header.getPrev ());
	}
	else
	{
		_header_offset = INVALID_OFFSET;
		_header.num_samples = 0;
	}

	return *this;
}

END_NAMESPACE_CHANARCH

