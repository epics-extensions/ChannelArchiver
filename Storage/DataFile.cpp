// $Id$
//////////////////////////////////////////////////////////////////////

// The map-template generates names of enormous length
// which will generate warnings because the debugger cannot display them:
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

// Tools
#include <AutoPtr.h>
#include <BinIO.h>
#include <Conversions.h>
#include <MsgLogger.h>
#include <Filename.h>
#include <epicsTimeHelper.h>
#include <string2cp.h>
// Storage
#include "DataFile.h"
#include "CtrlInfo.h"

// TODO: Convert to BinIO?

#undef LOG_DATAFILE

// List of all DataFiles currently open
// We assume that there aren't that many open,
// so a simple list is sufficient.
static stdList<DataFile *> open_data_files;

DataFile::DataFile(const stdString &dirname,
                   const stdString &basename,
                   const stdString &filename, bool for_write)
  : ref_count(1),
    for_write(for_write),
    is_tagged_file(false),
    filename(filename),
    dirname(dirname),
    basename(basename)

{
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
}

DataFile *DataFile::reference(const stdString &req_dirname,
                              const stdString &req_basename, bool for_write)
{
    DataFile *datafile;
    stdString dirname, basename, filename;

    Filename::build(req_dirname, req_basename, filename);
    Filename::getDirname(filename, dirname);
    Filename::getBasename(filename, basename);
#ifdef LOG_DATAFILE
    LOG_MSG("reference('%s', '%s', %s)\n",
            req_dirname.c_str(),
            req_basename.c_str(),
            (for_write ?  "read/write" : "read-only"));
    LOG_MSG("normalized: '%s' + '%s' = %s)\n",
            dirname.c_str(), basename.c_str(), filename.c_str());
#endif
    stdList<DataFile *>::iterator i = open_data_files.begin();
    for (/**/;  i != open_data_files.end ();  ++i)
    {
        datafile = *i;
        if (datafile->getFilename() == filename  &&
            datafile->for_write == for_write)
        {
#ifdef LOG_DATAFILE
            LOG_MSG("DataFile %s (%c) is cached (%d)\n",
                    filename.c_str(),
                    (for_write?'W':'R'), datafile->ref_count);
#endif
            datafile->reference();
            // When it was put in the cache, it might
            // have been a new file.
            // But now it's one that already existed,
            // so reset is_new_file:
            datafile->is_new_file = false;
            return datafile;
        }
    }
    try
    {
        datafile = new DataFile(dirname, basename, filename, for_write);
    }
    catch (...)
    {
        throw GenericException(__FILE__, __LINE__, "Cannot reference '%s'",
                               filename.c_str());
    }
    try
    {
        datafile->reopen();
    }
    catch (GenericException &e)
    {
        delete datafile;
        throw;
    }
    open_data_files.push_back(datafile);
    return datafile;
}

// Add reference to current DataFile
DataFile *DataFile::reference()
{
    ++ref_count;
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s referenced again (%d)\n",
            filename.c_str(), ref_count);
#endif
    return this;
}

// Call instead of delete:
void DataFile::release()
{
    --ref_count;
    // You might expect
    //   delete this;
    // in here, but we keep the files open
    // and cache them until close_all() is called.
#ifdef LOG_DATAFILE
    LOG_MSG("DataFile %s released (%d)\n",
            filename.c_str(), ref_count);
#endif
}

FileOffset DataFile::getSize() const
{
    if (fseek(file, 0, SEEK_END) != 0)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): Cannot seek to end",
                               filename.c_str());
    long end = ftell(file);
    if (end < 0)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): ftell failed",
                               filename.c_str());
    return (FileOffset) end;
}

void DataFile::reopen()
{
    is_new_file = is_tagged_file = false;
    // Try existing
    if (for_write)
        file.open(filename.c_str(), "r+b");
    else
        file.open(filename.c_str(), "rb");
    if (file)
    {   // Opened existing file. Check type
        uint32_t file_cookie;
        if (fseek(file, 0, SEEK_SET) != 0  ||
            readLong(file, &file_cookie) == false)
            throw GenericException(__FILE__, __LINE__,
                                   "DataFile(%s): Read error",
                                   filename.c_str());
        is_tagged_file = file_cookie == cookie;
        return;
    }
    // No file, yet.
    if (!for_write)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): No existing file found.",
                               filename.c_str());
    // Create a new one.
    if (!file.open(filename.c_str(), "w+b"))
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): Cannot create new file.",
                               filename.c_str());
    is_new_file = true;
    if (fseek(file, 0, SEEK_SET) != 0  ||
        writeLong(file, cookie) == false)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): Cannot write to file.",
                               filename.c_str());
    is_tagged_file = true;
}

void DataFile::close_all()
{
    stdList<DataFile *>::iterator i = open_data_files.begin();
    while (i != open_data_files.end())
    {
        DataFile *file = *i;
        if (file->ref_count > 0)
           throw GenericException(__FILE__, __LINE__,
                                  "DataFile %s still ref'ed in close_all (%d)\n",
                                  file->filename.c_str(), file->ref_count);
#ifdef LOG_DATAFILE
        LOG_MSG("DataFile::close_all: closing %s\n",
                file->filename.c_str());
#endif
        i = open_data_files.erase(i);
        delete file;
    }
}

DataHeader *DataFile::getHeader(FileOffset offset)
{
    DataHeader *header = 0;
    try
    {
        header = new DataHeader(this);
    }
    catch (...)
    {
        throw GenericException(__FILE__, __LINE__, "Cannot allocate DataHeader");
    }
    if (!header->read(offset))
        throw GenericException(__FILE__, __LINE__, "Cannot read DataHeader (%s)",
                               filename.c_str());
    return header;
}

size_t DataFile::getHeaderSize(const stdString &name,
                               DbrType dbr_type, DbrCount dbr_count,
                               size_t num_samples)
{
    size_t raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    size_t buf_free = num_samples * raw_value_size;
    // 'INFO' + name + '\0' + header info + data buffer
    return 4 + name.length() + 1 +
        sizeof(DataHeader::DataHeaderData) + buf_free;
}

DataHeader *DataFile::addHeader(const stdString &name,
                                DbrType dbr_type, DbrCount dbr_count,
                                double period, size_t num_samples)
{
    AutoPtr<DataHeader> header;
    try
    {
        header = new DataHeader(this);
    }
    catch (...)
    {
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): Cannot alloc new header",
                               filename.c_str());
    }
    FileOffset new_offset = getSize();
    if (fseek(file, new_offset, SEEK_SET))
    {
        throw GenericException(__FILE__, __LINE__,
                               "DataFile::addHeader(%s, %s): Cannot seek to 0x%X",
                               filename.c_str(), name.c_str(), new_offset);
    }
    if (fwrite("DATA", 4, 1, file) != 1  ||
        fwrite(name.c_str(), name.length() + 1, 1, file) != 1)
    {
        throw GenericException(__FILE__, __LINE__,
                               "DataFile::addHeader(%s, %s): Cannot tag at 0x%X",
                               filename.c_str(), name.c_str(), new_offset);
    }
    header->offset = new_offset + 4 + name.length() + 1;
    size_t raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    header->data.dbr_type = dbr_type;
    header->data.dbr_count = dbr_count;
    header->data.period = period;
    header->data.buf_free = num_samples * raw_value_size;
    header->data.buf_size =
        header->data.buf_free + sizeof(DataHeader::DataHeaderData);
    if (!header->write())
        throw GenericException(__FILE__, __LINE__,
                               "DataFile::addHeader(%s, %s): Cannot write new header",
                               filename.c_str(), name.c_str());
    // allocate data buffer by writing some marker at the end:
    uint32_t marker = 0xfacefade;
    FileOffset pos = header->offset
        + header->data.buf_size - sizeof marker;    
    if (fseek(file, pos, SEEK_SET) != 0 ||
        (FileOffset) ftell(file) != pos ||
        !writeLong(file, marker))    
        throw GenericException(__FILE__, __LINE__,
                               "DataFile::addHeader(%s, %s): "
                               "Cannot mark end of new buffer",
                               filename.c_str(), name.c_str());
    return header.release();
}

void DataFile::addCtrlInfo(const CtrlInfo &info, FileOffset &offset)
{
    if (fseek(file, 0, SEEK_END) != 0)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile(%s): Seek error",
                               filename.c_str());
    offset = ftell(file);
    if (fwrite("INFO", 4, 1, file) != 1)
        throw GenericException(__FILE__, __LINE__,
                               "DataFile::addCtrlInfo(%s): Cannot tag at 0x%X\n",
                               filename.c_str(), offset);
    offset += 4;
    info.write(this, offset);
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
    LOG_ASSERT(isValid());
    if (data.buf_free <= 0)
        return 0;
    size_t val_size = RawValue::getSize(data.dbr_type, data.dbr_count);
    LOG_ASSERT(val_size > 0);
    return data.buf_free / val_size;
}

size_t DataHeader::capacity()
{
    size_t val_size = RawValue::getSize(data.dbr_type, data.dbr_count);
    LOG_ASSERT(val_size > 0);
    return (data.buf_size - sizeof(DataHeader::DataHeaderData))
            / val_size;
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

void DataHeader::show(FILE *f, bool full_detail)
{
    size_t val_size = RawValue::getSize(data.dbr_type, data.dbr_count);
    size_t maxcount = (val_size > 0) ?
        (data.buf_size - sizeof(DataHeader::DataHeaderData)) / val_size : 0;
    stdString t;
    if (full_detail)
    {
        fprintf(f, "Buffer  : '%s' @ 0x%lX\n",
                datafile->basename.c_str(), (unsigned long)offset);
        fprintf(f, "Prev    : '%s' @ 0x%lX\n",
                data.prev_file, (unsigned long)data.prev_offset);
        epicsTime2string(data.begin_time, t);
        fprintf(f, "Time    : %s\n", t.c_str());
        epicsTime2string(data.end_time, t);
        fprintf(f, "...     : %s\n", t.c_str());
        epicsTime2string(data.next_file_time, t);
        fprintf(f, "New File: %s\n", t.c_str());
        fprintf(f, "DbrType : %d, %d elements (%lu bytes/sample)\n",
                data.dbr_type, data.dbr_count,
                (unsigned long) val_size);
        fprintf(f, "Samples : %ld used out of %d\n",
                (long)data.num_samples, maxcount);
        fprintf(f, "Size    : %ld bytes, free: %ld bytes"
                " (header: %zu bytes)\n",
                (long)data.buf_size, (long)data.buf_free,
                sizeof(DataHeader::DataHeaderData));
        fprintf(f, "Curr.   : %ld\n", (unsigned long)data.curr_offset);
        fprintf(f, "Period  : %g\n", data.period);
        fprintf(f, "CtrlInfo@ 0x%lX\n", (unsigned long)data.ctrl_info_offset);
        fprintf(f, "Next    : '%s' @ 0x%lX\n",
                data.next_file, (unsigned long)data.next_offset);
    }
    else
    {
        fprintf(f, "'%s' @ 0x%lX ",
                datafile->basename.c_str(), (unsigned long)offset);
        fprintf(f, "%ld/%d samples from ", (long)data.num_samples, maxcount);
        epicsTime2string(data.begin_time, t);
        fprintf(f, " %s - ", t.c_str());
        epicsTime2string(data.end_time, t);
        fprintf(f, "%s\n", t.c_str());
        epicsTime2string(data.next_file_time, t);
    }
}

