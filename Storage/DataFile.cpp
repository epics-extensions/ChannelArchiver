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

#define LOG_DATAFILE

// List of all DataFiles currently open
// We assume that there aren't that many open,
// so a simple list is sufficient
static stdList<DataFile *> open_data_files;

DataFile::DataFile(const stdString &dirname,
                   const stdString &basename,
                   const stdString &filename, bool for_write)
{
    this->for_write = for_write;
    this->dirname = dirname;
    this->basename = basename;
    this->filename = filename;
    file = 0;
    reopen();
    ref_count = 1;
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s opened\n", filename.c_str());
#endif
}

DataFile::~DataFile ()
{
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s closed\n", filename.c_str());
#endif
    if (file)
        fclose(file);
}

// For synchr. with a file that's actively written
// by another prog. is might help to reopen:
bool DataFile::reopen ()
{
    if (file)
        fclose(file);
    file = fopen(filename.c_str(), "r+b");
    if (file==0  && for_write)
        file = fopen(filename.c_str(), "w+b");
    if (file == 0)
    {
        LOG_MSG ("Cannot open/create DataFile '%s'\n",
                 filename.c_str());
        return false;
    }
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
    ++ref_count;
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s referenced again (%d)\n",
            filename.c_str(), ref_count);
#endif
    return this;
}

// Call instead of delete:
void DataFile::release ()
{
    --ref_count;
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s released (%d)\n",
            filename.c_str(), ref_count);
#endif
}

bool DataFile::close_all()
{
    DataFile *file;
    bool all_closed = true;
    
    stdList<DataFile *>::iterator i = open_data_files.begin();
    while (i != open_data_files.end())
    {
        file = *i;
        if (file->ref_count > 0)
        {
            LOG_MSG("DataFile %s still ref'ed in close_all (%d)\n",
                    file->filename.c_str(), file->ref_count);
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

DataHeader *DataFile::getHeader(FileOffset offset)
{
    DataHeader *header = new DataHeader(this);
    if (header)
    {
        if (header->read(offset))
            return header;
        else
            delete header;
    }
    return 0;
}

DataHeader::DataHeader(DataFile *datafile)
{
    datafile->reference();
    this->datafile = datafile;
    clear();
}

DataHeader::~DataHeader()
{
    datafile->release();
}
    
void DataHeader::clear()
{
    memset(&data, 0, sizeof(struct DataHeaderData));
    offset = INVALID_OFFSET;
}

bool DataHeader::isValid()
{
    return offset != INVALID_OFFSET;
}

size_t DataHeader::available()
{
    if (!isValid()  ||  data.buf_free <= 0)
        return 0;
    size_t val_size = RawValue::getSize(data.dbr_type, data.nelements);
    if (val_size > 0)
        return data.buf_free / val_size;
    return 0;
}

bool DataHeader::read(FileOffset offset)
{
    this->offset = offset;
    if (fseek(datafile->file, offset, SEEK_SET) != 0   ||
        (FileOffset)ftell(datafile->file) != offset  ||
        fread(&data, sizeof(struct DataHeaderData), 1, datafile->file) != 1)
    {
        clear();
        return false;
    }
    // convert the data header into host format:
    FileOffsetFromDisk(data.dir_offset);
    FileOffsetFromDisk(data.next_offset);
    FileOffsetFromDisk(data.prev_offset);
    FileOffsetFromDisk(data.curr_offset);
    ULONGFromDisk(data.num_samples);
    FileOffsetFromDisk(data.ctrl_info_offset);
    ULONGFromDisk(data.buf_size);
    ULONGFromDisk(data.buf_free);
    USHORTFromDisk(data.dbr_type);
    USHORTFromDisk(data.nelements);
    DoubleFromDisk(data.period);
    epicsTimeStampFromDisk(data.begin_time);
    epicsTimeStampFromDisk(data.next_file_time);
    epicsTimeStampFromDisk(data.end_time);
    return true;
}

bool DataHeader::write() const
{
    DataHeaderData copy = data;
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
    return fseek(datafile->file, offset, SEEK_SET) == 0 &&
        (FileOffset) ftell(datafile->file) == offset  &&
        fwrite(&copy, sizeof(struct DataHeaderData), 1, datafile->file) == 1;
}

bool DataHeader::read_next()
{
    if (offset == INVALID_OFFSET)
        return false;
    if (data.next_offset == INVALID_OFFSET ||
        !Filename::isValid(data.next_file))
    {
        clear();
        return false;
    }
    // Do we need to switch data files?
    if (strcmp(datafile->basename.c_str(), data.next_file))
    {
        DataFile *next = DataFile::reference(
            datafile->dirname,
            data.next_file, datafile->for_write);
        if (! next)
        {
            clear();
            return false;
        }
        datafile->release();
        datafile = next;
    }
    return read(data.next_offset);
}
