// $Id$
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "Conversions.h"
#include "MsgLogger.h"
#include "Filename.h"
#include "DataFile.h"

//#define LOG_DATAFILE

void DataHeader::clear()
{
    memset(this, 0, sizeof(class DataHeader));
}

bool DataHeader::read(FILE *file, FileOffset offset)
{
    if (fseek(file, offset, SEEK_SET) != 0   ||
        (FileOffset)ftell(file) != offset  ||
        fread(this, sizeof(DataHeader), 1, file) != 1)
        return false;
    // convert the data header into host format:
    FileOffsetFromDisk(dir_offset);
    FileOffsetFromDisk(next_offset);
    FileOffsetFromDisk(prev_offset);
    FileOffsetFromDisk(curr_offset);
    ULONGFromDisk(num_samples);
    FileOffsetFromDisk(ctrl_info_offset);
    ULONGFromDisk(buf_size);
    ULONGFromDisk(buf_free);
    USHORTFromDisk(dbr_type);
    USHORTFromDisk(nelements);
    DoubleFromDisk(period);
    epicsTimeStampFromDisk(begin_time);
    epicsTimeStampFromDisk(next_file_time);
    epicsTimeStampFromDisk(end_time);
    return true;
}

bool DataHeader::write(FILE *file, FileOffset offset) const
{
    DataHeader copy (*this);

    // convert the data header into host format:
    FileOffsetToDisk(copy.dir_offset);
    FileOffsetToDisk(copy.next_offset);
    FileOffsetToDisk(copy.prev_offset);
    FileOffsetToDisk(copy.curr_offset);
    ULONGToDisk(copy.num_samples);
    FileOffsetToDisk(copy.ctrl_info_offset);
    ULONGToDisk(copy.buf_size);
    ULONGToDisk(copy.buf_free);
    USHORTToDisk(copy.dbr_type);
    USHORTToDisk(copy.nelements);
    DoubleToDisk(copy.period);
    epicsTimeStampToDisk(copy.begin_time);
    epicsTimeStampToDisk(copy.next_file_time);
    epicsTimeStampToDisk(copy.end_time);

    return fseek(file, offset, SEEK_SET) == 0 &&
        (FileOffset) ftell(file) == offset  &&
        fwrite(&copy, sizeof(DataHeader), 1, file) == 1;
}

// List of all DataFiles currently open
// We assume that there aren't that many open,
// so a simple list is sufficient
static stdList<DataFile *> open_data_files;

DataFile::DataFile(const stdString &dirname,
                   const stdString &basename,
                   const stdString &filename, bool for_write)
{
    _file_for_write = for_write;
    _dirname = dirname;
    _basename = basename;
    _filename = filename;
    file = 0;
    reopen();
    _ref_count = 1;
    LOG_MSG("DataFile %s opened\n", filename.c_str());
}

DataFile::~DataFile ()
{
    LOG_MSG("DataFile %s closed\n", _filename.c_str());    
    if (file)
        fclose(file);
}

// For synchr. with a file that's actively written
// by another prog. is might help to reopen:
bool DataFile::reopen ()
{
    if (file)
        fclose(file);
    file = fopen(_filename.c_str(), "r+b");
    if (file==0  && _file_for_write)
        file = fopen(_filename.c_str(), "w+b");
    if (file == 0)
    {
        LOG_MSG ("Cannot open/create DataFile '%s'\n",
                 _filename.c_str());

        return false;
    }
#ifdef LOG_DATAFILE
    LOG_MSG ("DataFile " << _filename << "\n");
#endif
    return true;
}

DataFile *DataFile::reference(const stdString &dirname,
                              const stdString &basename, bool for_write)
{
    DataFile *file;
    stdString filename;

    Filename::build(dirname, basename, filename);
    stdList<DataFile *>::iterator i = open_data_files.begin();
    while (i != open_data_files.end ())
    {
        file = *i;
        if (file->getFilename() == filename)
        {
            file->reference();
            return file;
        }
        ++i;
    }
    file = new DataFile(dirname, basename, filename, for_write);
    open_data_files.push_back(file);
    return file;
}

// Add reference to current DataFile
DataFile *DataFile::reference ()
{
    ++_ref_count;
    LOG_MSG("DataFile %s referenced again (%d)\n",
            _filename.c_str(), _ref_count);
    return this;
}

// Call instead of delete:
void DataFile::release ()
{
    --_ref_count;
    LOG_MSG("DataFile %s released (%d)\n",
            _filename.c_str(), _ref_count);
}

bool DataFile::close_all()
{
    DataFile *file;
    bool all_closed = true;
    
    stdList<DataFile *>::iterator i = open_data_files.begin();
    while (i != open_data_files.end())
    {
        file = *i;
        if (file->_ref_count > 0)
        {
            LOG_MSG("DataFile %s still ref'ed in close_all (%d)\n",
                    file->_filename.c_str(), file->_ref_count);
            all_closed = false;
            ++i;
        }
        else
        {
            i = open_data_files.erase(i);
            delete file;
        }
    }
    return all_closed;
}

DataHeaderIterator *DataFile::getHeader(FileOffset offset)
{
    DataHeaderIterator *i = new DataHeaderIterator;
    if (i)
        i->attach(this, offset);
    return i;
}

void DataHeaderIterator::init()
{
    datafile = 0;
    header_offset = INVALID_OFFSET;
    header.num_samples = 0;
}

void DataHeaderIterator::clear()
{
    if (datafile)
        datafile->release();
    init();
}

DataHeaderIterator::DataHeaderIterator()
{
    init();
}

DataHeaderIterator::DataHeaderIterator(const DataHeaderIterator &rhs)
{
    init();
    *this = rhs;
}

DataHeaderIterator & DataHeaderIterator::operator = (const DataHeaderIterator &rhs)
{
    if (&rhs != this)
    {
        if (rhs.datafile)
        {
            DataFile *tmp = datafile;
            datafile = rhs.datafile->reference();
            if (tmp)
                tmp->release();
            header_offset = rhs.header_offset;
            if (header_offset != INVALID_OFFSET)
                header = rhs.header;
        }
        else
            clear();   
    }
    return *this;
}

DataHeaderIterator::~DataHeaderIterator()
{
    clear();
}

void DataHeaderIterator::attach(DataFile *file, FileOffset offset,
                                DataHeader *header)
{   
    if (file)
    {
        DataFile *tmp = datafile;
        datafile = file->reference ();
        if (tmp)
            tmp->release ();
        header_offset = offset;
        if (header_offset != INVALID_OFFSET)
        {
            if (header)
                this->header = *header;
            else
                getHeader(offset);
        }
    }
    else
        clear ();   
}

// Re-read the current DataHeader
void DataHeaderIterator::sync ()
{
    if (header_offset != INVALID_OFFSET &&
        datafile->reopen())
        getHeader(header_offset);
}

FileOffset DataHeaderIterator::getDataOffset() const
{
    LOG_ASSERT(header_offset != INVALID_OFFSET);
    return header_offset + sizeof(DataHeader);
}

bool DataHeaderIterator::getHeader(FileOffset position)
{
    LOG_ASSERT (datafile  &&  position != INVALID_OFFSET);
    if (header.read(datafile->file, position))
    {
        header_offset = position;
        return true;
    }
    return false;
}

// Get next header (might be in different data file).
bool DataHeaderIterator::getNext()
{
    if (header_offset == INVALID_OFFSET)
        return false;

    if (haveNextHeader())  // Is there a valid next file?
    {
        if (datafile->getBasename() != header.next_file)
        {
            // switch to next data file
            stdString dirname = datafile->getDirname();
            bool for_write = datafile->_file_for_write;
            datafile->release();
            datafile = DataFile::reference(dirname,
                                           header.next_file,
                                           for_write);
        }
        return getHeader(header.next_offset);
    }    
    header_offset = INVALID_OFFSET;
    header.num_samples = 0;
    return false;
}

bool DataHeaderIterator::getPrev()
{
    if (header_offset == INVALID_OFFSET)
        return false;
    if (havePrevHeader())  // Is there a valid next file?
    {
        if (datafile->getFilename () != header.prev_file)
        {   // switch to prev data file
            bool for_write = datafile->_file_for_write;
            stdString dirname = datafile->getDirname ();
            datafile->release();
            datafile = DataFile::reference(dirname,
                                           header.prev_file, for_write);
        }
        return getHeader(header.prev_offset);
    }
    header_offset = INVALID_OFFSET;
    header.num_samples = 0;
    return false;
}


