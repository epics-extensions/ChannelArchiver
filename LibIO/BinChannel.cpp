#include "BinArchive.h"
#include "BinChannel.h"
#include "BinChannelIterator.h"
#include "BinValueIterator.h"
#include "DirectoryFile.h"
#include "DataFile.h"
#include "ArchiveException.h"

BEGIN_NAMESPACE_CHANARCH

BinChannel::BinChannel ()
{
	init ();
}

BinChannel::~BinChannel ()
{
	delete _append_buffer;
}

BinChannel & BinChannel::operator = (const BinChannel &rhs)
{
	// keep _append_buffer referencing an eventually open DataFile
	_data = rhs._data;
	_offset = rhs._offset;
	_channel_iter = rhs._channel_iter;
	return *this;
}

void BinChannel::init (const char *name)
{
	memset (&_data, 0, getDataSize ());
	if (name)
	{
		strncpy(_data.name, name, ChannelNameLength);
		_data.name[ChannelNameLength-1] = '\0';
	}
	else
		_data.name[0] = '\0';
	_channel_iter = 0;
	_append_buffer = 0;
}

BinArchive *BinChannel::getArchive ()
{
	if (!_channel_iter)
		return 0;
	return _channel_iter->getArchive ();
}

void BinChannel::read (FILE *fd, FileOffset offset)
{
	if (_append_buffer)
		throwDetailedArchiveException (Invalid, "Open append-buffer");

	if (fseek (fd, offset, SEEK_SET) != 0  ||
		fread (&_data, getDataSize (), 1, fd) != 1)
		throwArchiveException (ReadError);

	FileOffsetFromDisk (_data.next_entry_offset);
	FileOffsetFromDisk (_data.last_offset);
	FileOffsetFromDisk (_data.first_offset);
	TS_STAMPFromDisk   (_data.create_time);
	TS_STAMPFromDisk   (_data.first_save_time);
	TS_STAMPFromDisk   (_data.last_save_time);
	_offset = offset;
}

void BinChannel::write (FILE *fd, FileOffset offset) const
{
	Data copy = _data;

	FileOffsetToDisk (copy.next_entry_offset);
	FileOffsetToDisk (copy.last_offset);
	FileOffsetToDisk (copy.first_offset);
	TS_STAMPToDisk (copy.create_time);
	TS_STAMPToDisk (copy.first_save_time);
	TS_STAMPToDisk (copy.last_save_time);

	if (fseek (fd, offset, SEEK_SET) != 0  ||
		fwrite (&copy, getDataSize (), 1, fd) != 1)
		throwArchiveException (WriteError);
	_offset = offset;
}

// Implementation of ChannelI
const char *BinChannel::getName ()   const
{	return _data.name; }

osiTime BinChannel::getCreateTime () const
{	return TS_STAMP2osi(_data.create_time); }

osiTime BinChannel::getFirstTime ()  const
{	return TS_STAMP2osi(_data.first_save_time); }

osiTime BinChannel::getLastTime ()   const
{	return TS_STAMP2osi(_data.last_save_time); }

bool BinChannel::getFirstValue (ValueIteratorI *arg)
{
	BinValueIterator *value = dynamic_cast <BinValueIterator *> (arg);

	if (getArchive () && isValidFilename (getFirstFile()))
	{
		stdString full_name;
		getArchive ()->makeFullFileName (getFirstFile(), full_name);
		return value->attach (this, full_name, getFirstOffset ());
	}

	value->clear ();
	return false;
}

bool BinChannel::getLastValue (ValueIteratorI *arg)
{
	BinValueIterator *value = dynamic_cast <BinValueIterator *> (arg);

	if (getArchive () && isValidFilename (getLastFile()))
	{
		stdString full_name;
		getArchive ()->makeFullFileName (getLastFile(), full_name);
		return value->attach (this, full_name, getLastOffset (), true);
	}

	value->clear ();
	return false;
}

bool BinChannel::getValueAfterTime (const osiTime &time, ValueIteratorI *arg)
{
	// Before first value?
	if (time == nullTime  ||  time <= getFirstTime())
		return getFirstValue (arg);

	BinValueIterator *value = dynamic_cast <BinValueIterator *> (arg);

	// After last value?
	if (!getArchive ()  ||   time > getLastTime())
	{
		value->clear ();
		return false;
	}

	// Start < time <= end, determine where to start looking for time:
	double d_time = double(time);
	double from_start = d_time - double (getFirstTime());
	double from_end =  double (getLastTime()) - d_time;
	bool found_buffer = false;
	stdString full_name;

	// Find data header that brackens 'time':
	if (from_start < from_end  &&  isValidFilename (getFirstFile()) )
	{	// start at beginning of archive:
		getArchive ()->makeFullFileName (getFirstFile (), full_name);
		try
		{
			if (value->attach (this, full_name, getFirstOffset ()))
			{
				// Snoop forward in time:
				while (value->getHeader()->getEndTime() < time  &&  value->nextBuffer ())
				{}
				found_buffer = true;
			}
		}
		catch (ArchiveException &e)
		{
			LOG_MSG ("BinChannel::getValueAfterTime caught: " << e.what());
		}
	}
	// start f. beginning not suggested - or failed for some other reason like broken archive
	if (! found_buffer)
	{	// start at end of archive to retrieve latest data first:
		getArchive ()->makeFullFileName (getLastFile (), full_name);
		if (! value->attach (this, full_name, getLastOffset ()))
			return false;
		// Snoop back in time:
		while (value->getHeader()->getBeginTime() > time)
		{
			if (! value->prevBuffer ())
				return value->readValue (0); // first value we can find...
		}
	}

	// Find matching value inside that header
	// Tried binary search here, but that
	// didn't work with old archives where header's end times
	// are illdefined....
	// ->
	// linear search:
	value->readValue (0);
	while (value->isValid() && value->getValue()->getTime() < time)
		value->next();

	return value->isValid();
}

bool BinChannel::getValueBeforeTime (const osiTime &time, ValueIteratorI *value)
{
	LOG_ASSERT (value);
	if (time == nullTime)
		return getFirstValue (value);

	if (! getValueAfterTime (time, value))
		return getLastValue (value);

	// Find first value on or before "time".
	while (value->isValid() && value->getValue()->getTime() > time)
		value->prev();

	return value->isValid();
}

bool BinChannel::getValueNearTime (const osiTime &time, ValueIteratorI *value)
{	// not optimal....
	if (! getValueBeforeTime (time, value))
		return getValueAfterTime (time, value);

	double d = double (time);
	double before = fabs(double(value->getValue()->getTime()) - d);
	value->next();
	if (! value->isValid())
	{
		value->prev();
		return value->isValid();
	}
	double after = fabs(double(value->getValue()->getTime()) - d);
	if (before <= after) 
		value->prev();

	return value->isValid();
}

size_t BinChannel::lockBuffer (const ValueI &value, double period)
{
	// add value to last buffer
	if (!getArchive ()  ||  ! isValidFilename (getLastFile ()))
		return 0;

	stdString last_file_name;
	getArchive ()->makeFullFileName(getLastFile (), last_file_name);

	if (! _append_buffer)
		_append_buffer = new DataHeaderIterator();
	DataFile *file = DataFile::reference (last_file_name);
	_append_buffer->attach (file, getLastOffset());
	file->release (); // now ref'd by "_append_buffer"

	if ((*_append_buffer)->getType() == value.getType() &&
		(*_append_buffer)->getCount() == value.getCount() &&
		(*_append_buffer)->getNextTime() > value.getTime())
	{
		// Calc. number of free entries
		size_t value_size = value.getRawValueSize ();
		unsigned long buf_free = (*_append_buffer)->getBufFree ();
		if (buf_free > 0)
			return buf_free / value_size;
	}

	return 0;
}

void BinChannel::addBuffer (const ValueI &value_arg, double period, size_t value_count)
{
	// dir_entry: this BinChannel + it's offset in _archive->_dir
	LOG_ASSERT (getArchive ());
	DirectoryFileIterator *dir_entry = &_channel_iter->_dir;
	LOG_ASSERT (dir_entry->getChannel() == this);

	const BinValue *value = dynamic_cast<const BinValue *>(&value_arg);
	if (! value)
	{
		throwDetailedArchiveException (Invalid, "BinChannel::addBuffer: empty ValueIterator");
		return;
	}
	if (! value->getCtrlInfo())
	{
		throwDetailedArchiveException (Invalid, "BinChannel::addBuffer: Value w/o CtrlInfo");
		return;
	}

	// Create filename for that value's time, find when we need new file:
	stdString data_file_name;
	getArchive ()->makeDataFileName (value_arg, data_file_name);

	osiTime next_file_time;
	getArchive ()->calcNextFileTime (value_arg.getTime(), next_file_time);

	// Init new DataHeader
	DataHeader	header;
	header.clear ();
	header.setDirOffset (_offset);
	header.setBufFree (value_count * value->getRawValueSize ());
	header.setBufSize (header.getBufFree () + sizeof (DataHeader));
	header.setDbrType (value->getType ());
	header.setDbrCount (value->getCount ());
	header.setPeriod (period);
	header.setNextTime (next_file_time);

	// Add to DataFile
    // add value to last buffer
	DataHeaderIterator *prev = 0;
	if (isValidFilename (getLastFile ()))
	{
		stdString last_file_name;
		getArchive ()->makeFullFileName (getLastFile (), last_file_name);
		DataFile *file = DataFile::reference (last_file_name);
		prev = new DataHeaderIterator ();
		prev->attach (file, getLastOffset());
		file->release (); // now ref'd by "prev"
	}

	stdString data_file_path;
	getArchive ()->makeFullFileName (data_file_name, data_file_path);
	DataFile *data = DataFile::reference (data_file_path);
	if (! _append_buffer)
		_append_buffer = new DataHeaderIterator();

	// cannot use dynamic_cast because it might really be only a CtrlInfoI
	const BinCtrlInfo *ctrl_info = (const BinCtrlInfo*) value->getCtrlInfo();
	LOG_ASSERT (ctrl_info);
	*_append_buffer = data->addHeader (header, *ctrl_info, prev);
	data->release (); // now ref'ed by _append_buffer
	delete prev;

	// update DirectoryFile (note again that dir_entry holds this BinChannel)
	if (! isValidFilename (getFirstFile ()))
	{
		setFirstFile (_append_buffer->getBasename ());
		setFirstOffset (_append_buffer->getOffset ());
	}
	setLastFile (_append_buffer->getBasename ());
	setLastOffset (_append_buffer->getOffset ());
	dir_entry->save ();
}

bool BinChannel::addValue (const ValueI &value)
{
	if (! _append_buffer)
		return false;
	bool is_first_entry = (*_append_buffer)->getNumSamples() == 0;

	if (! _append_buffer->addNewValue (*dynamic_cast<const BinValue*>(&value)))
	{	// Failed...
		releaseBuffer ();
		return false;
	}

	// update DirectoryFile
	if (is_first_entry && _append_buffer->getBasename () == getFirstFile () &&
		                  _append_buffer->getOffset ()   == getFirstOffset ())
		setFirstTime (value.getTime ());
	setLastTime (value.getTime ());
	_channel_iter->_dir.save();

	return true;
}

void BinChannel::releaseBuffer ()
{
	if (_append_buffer)
	{
		delete _append_buffer;
		_append_buffer = 0;
	}
}

END_NAMESPACE_CHANARCH

