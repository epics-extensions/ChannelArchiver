// $Id$
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

// Tools
#include "Conversions.h"
#include "MsgLogger.h"
#include "Filename.h"
#include "epicsTimeHelper.h"
#include "string2cp.h"
// Storage
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

DataHeader *DataFile::addHeader(DbrType dbr_type, DbrCount dbr_count,
                                size_t num_samples)
{
    if (fseek(file, 0, SEEK_END) != 0)
    {
        LOG_MSG("DataFile::addHeader(%s): Cannot add new header\n",
                filename.c_str());
        return 0;
    }
    DataHeader *header = new DataHeader(this);
    if (! header)
    {
        LOG_MSG("DataFile::addHeader(%s): Cannot alloc new header\n",
                filename.c_str());
        return 0;
    }
    header->offset = ftell(file);
    size_t raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    header->data.dbr_type = dbr_type;
    header->data.nelements = dbr_count;
    header->data.buf_free = num_samples * raw_value_size;
    header->data.buf_size = header->data.buf_free + sizeof(DataHeader::DataHeaderData);
    if (!header->write())
    {
        LOG_MSG("DataFile::addHeader(%s): Cannot write new header\n",
                filename.c_str());
        delete header;
        return 0;   
    }
    // allocate data buffer by writing some marker at the end:
    long marker = 0xfacefade;
    FileOffset pos = header->offset
        + header->data.buf_size - sizeof marker;    
    if (fseek(file, pos, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != pos ||
        fwrite(&marker, sizeof marker, 1, file) != 1)    
   {
        LOG_MSG("DataFile::addHeader(%s): Cannot mark end of new buffer\n",
                filename.c_str());
        delete header;
        return 0;
    }
    return header;
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

size_t DataHeader::capacity()
{
    size_t val_size = RawValue::getSize(data.dbr_type, data.nelements);
    if (val_size > 0)
        return (data.buf_size - sizeof(DataHeader::DataHeaderData))
            / val_size;
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

void DataHeader::set_prev(const stdString &basename, FileOffset offset)
{
    string2cp(data.prev_file, basename, FilenameLength);
    data.prev_offset = offset;
}
     
void DataHeader::set_next(const stdString &basename, FileOffset offset)
{
    string2cp(data.next_file, basename, FilenameLength);
    data.next_offset = offset;
}

void DataHeader::show(FILE *f)
{
    stdString t;
    fprintf(f, "Buffer  : '%s' @ 0x%lX\n",
           datafile->basename.c_str(), offset);               
    fprintf(f, "Prev    : '%s' @ 0x%lX\n",
            data.prev_file, data.prev_offset);
    epicsTime2string(data.begin_time, t);
    fprintf(f, "Time    : %s\n", t.c_str());
    epicsTime2string(data.end_time, t);
    fprintf(f, "...     : %s\n", t.c_str());
    epicsTime2string(data.next_file_time, t);
    fprintf(f, "New File: %s\n", t.c_str());
    fprintf(f, "DbrType : %d, %d elements\n",
            data.dbr_type, data.nelements);
    fprintf(f, "Samples : %ld\n", data.num_samples);
    fprintf(f, "Size    : %ld bytes, free: %ld bytes\n",
            data.buf_size, data.buf_free);
    fprintf(f, "Curr.   : %ld\n", data.curr_offset);
    fprintf(f, "Period  : %g\n", data.period);
    fprintf(f, "CtrlInfo@ 0x%lX\n", data.ctrl_info_offset);
    fprintf(f, "Next    : '%s' @ 0x%lX\n",
            data.next_file, data.next_offset);
    fprintf(f, "\n");
}
