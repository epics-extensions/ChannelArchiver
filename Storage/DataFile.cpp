// $Id$
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

// Tools
#include "AutoPtr.h"
#include "Conversions.h"
#include "MsgLogger.h"
#include "Filename.h"
#include "epicsTimeHelper.h"
#include "string2cp.h"
// Storage
#include "DataFile.h"
#include "CtrlInfo.h"

// #define LOG_DATAFILE

// List of all DataFiles currently open
// We assume that there aren't that many open,
// so a simple list is sufficient.
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
    ref_count = 1;
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s (%c) opened\n",
            filename.c_str(), (for_write?'W':'R'));
#endif
}

DataFile::~DataFile ()
{
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s (%c) closed\n",
            filename.c_str(), (for_write?'W':'R'));
#endif
    if (file)
        fclose(file);
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
        if (file->getFilename() == filename  &&
            file->for_write == for_write)
        {
#ifdef LOG_DATAFILE
            LOG_MSG("DataFile %s (%c) is cached (%d)\n",
                    filename.c_str(),
                    (for_write?'W':'R'), file->ref_count);
#endif
            file->reference();
            // When it was put in the cache, it might
            // have been a new file.
            // But now it's one that already existed,
            // so reset is_new_file:
            file->is_new_file = false;
            return file;
        }
        ++i;
    }
    file = new DataFile(dirname, basename, filename, for_write);
    if (file->reopen())
    {
        open_data_files.push_back(file);
        return file;
    }
    delete file;
    return 0;
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

bool DataFile::getSize(FileOffset &size) const
{
    if (fseek(file, 0, SEEK_END) != 0)
    {
        LOG_MSG("DataFile::getSize(%s): Cannot seek to end\n",
                filename.c_str());
        return false;
    }
    long end = ftell(file);
    if (end < 0)
    {
        LOG_MSG("DataFile::getSize(%s): ftell failed\n",
                filename.c_str());
        return false;
    }
    size = (FileOffset) end;
    return true;
}

bool DataFile::reopen ()
{
    is_new_file = false;
    if (file)
        fclose(file);
    // Try existing
    if (for_write)
        file = fopen(filename.c_str(), "r+b");
    else
        file = fopen(filename.c_str(), "rb");
    // Try to create a new one
    if (file==0 && for_write)
    {
        file = fopen(filename.c_str(), "w+b");
        is_new_file = true;
    }
    // Got any?
    if (file == 0)
    {   
        LOG_MSG("Cannot %s DataFile '%s'\n",
                (for_write ? "create" : "open"),
                filename.c_str());
        return false;
    }
    return true;
}

bool DataFile::close_all(bool verbose)
{
    DataFile *file;
    bool all_closed = true;
    
    stdList<DataFile *>::iterator i = open_data_files.begin();
    while (i != open_data_files.end())
    {
        file = *i;
        if (file->ref_count > 0)
        {
            if (verbose)
            {
                LOG_MSG("DataFile %s still ref'ed in close_all (%d)\n",
                        file->filename.c_str(), file->ref_count);
            }
            all_closed = false;
            ++i;
        }
        else
        {
#ifdef LOG_DATAFILE
            LOG_MSG("DataFile::close_all: %s released (%d)\n",
                    file->filename.c_str(), file->ref_count);
#endif
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

size_t DataFile::getHeaderSize(DbrType dbr_type, DbrCount dbr_count,
                               size_t num_samples)
{
    size_t raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    size_t buf_free = num_samples * raw_value_size;
    return buf_free + sizeof(DataHeader::DataHeaderData);
}

DataHeader *DataFile::addHeader(DbrType dbr_type, DbrCount dbr_count,
                                double period, size_t num_samples)
{
    AutoPtr<DataHeader> header(new DataHeader(this));
    if (! header)
    {
        LOG_MSG("DataFile::addHeader(%s): Cannot alloc new header\n",
                filename.c_str());
        return 0;
    }
    if (!getSize(header->offset))
        return 0;
    size_t raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    header->data.dbr_type = dbr_type;
    header->data.dbr_count = dbr_count;
    header->data.period = period;
    header->data.buf_free = num_samples * raw_value_size;
    header->data.buf_size =
        header->data.buf_free + sizeof(DataHeader::DataHeaderData);
    if (!header->write())
    {
        LOG_MSG("DataFile::addHeader(%s): Cannot write new header\n",
                filename.c_str());
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
        return 0;
    }
    return header.release();
}

bool DataFile::addCtrlInfo(const CtrlInfo &info, FileOffset &offset)
{
    if (fseek(file, 0, SEEK_END) != 0)
        return false;
    offset = ftell(file);
    return info.write(this, offset);
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
    size_t val_size = RawValue::getSize(data.dbr_type, data.dbr_count);
    if (val_size > 0)
        return data.buf_free / val_size;
    return 0;
}

size_t DataHeader::capacity()
{
    size_t val_size = RawValue::getSize(data.dbr_type, data.dbr_count);
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
    USHORTFromDisk(data.dbr_count);
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
    USHORTToDisk(copy.dbr_count);
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
    return get_prev_next(data.next_file, data.next_offset);
}

bool DataHeader::read_prev()
{
    if (offset == INVALID_OFFSET)
        return false;
    return get_prev_next(data.prev_file, data.prev_offset);
}

bool DataHeader::get_prev_next(const char *name, FileOffset new_offset)
{
    if (new_offset == INVALID_OFFSET ||
        !Filename::isValid(name))
    {
        clear();
        return false;
    }
    // Do we need to switch data files?
    if (strcmp(datafile->basename.c_str(), name))
    {
        DataFile *next = DataFile::reference(
            datafile->dirname,
            name, datafile->for_write);
        if (! next)
        {
            clear();
            return false;
        }
        datafile->release();
        datafile = next;
    }
    return read(new_offset);
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
            data.dbr_type, data.dbr_count);
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
