// DataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "DataFile.h"
#include "DataReader.h"

#undef DEBUG_DATAREADER

DataReader::DataReader(archiver_Index &index)
        : index(index), au_iter(0), valid_datablock(false), data(0), header(0)
{
}

DataReader::~DataReader()
{
    if (au_iter)
        delete au_iter;
    if (data)
        RawValue::free(data);
    if (header)
        delete header;
    DataFile::close_all();
}

const RawValue::Data *DataReader::find(
    const stdString &channel_name,
    const epicsTime *start, const epicsTime *end)
{
    this->channel_name = channel_name;
    interval range;
    if (start==0  ||  end==0)
    {   // default: all that's in archive
        if (!index.getEntireIndexedInterval(channel_name.c_str(), &range))
        {
            LOG_MSG("Cannot get interval for '%s' in index\n",
                    channel_name.c_str());
            return 0;
        }   
    }
    // adjust time range to requested start & end
    if (start)
        range.setStart(*start);
    if (end)
        range.setEnd(*end);
#ifdef DEBUG_DATAREADER
    {
        stdString txt;
        epicsTime2string(range.getStart(), txt);
        printf("DataReader::find: %s ...", txt.c_str());
        epicsTime2string(range.getEnd(), txt);
        printf("%s\n", txt.c_str());
    }
#endif
    au_iter = index.getKeyAUIterator(channel_name.c_str());
    if (!au_iter)
    {
        LOG_MSG ("DataReader: Cannot find '%s' in index\n",
                 channel_name.c_str());
        return 0;
    }
    // Get 1st data block
    valid_datablock = au_iter->getFirst(range, &datablock, &valid_interval);
    if (! valid_datablock)
    {
        LOG_MSG ("DataReader: No values for '%s' in index\n",
                 channel_name.c_str());
        return 0;
    }
#ifdef DEBUG_DATAREADER
    {
        stdString s, e;
        epicsTime2string(valid_interval.getStart(), s);
        epicsTime2string(valid_interval.getEnd(), e);
        printf("First Block: %s @ 0x%lX: %s - %s\n",
               datablock.getPath(), datablock.getOffset(), s.c_str(),
               e.c_str());
#endif
    // Get the buffer for that data block
    if (!getHeader(index.getDirectory(),
                   datablock.getPath(), datablock.getOffset()))
    {
        LOG_MSG("DataReader: No data for '%s'\n",
                channel_name.c_str());
        return 0;
    }
    return findSample(valid_interval.getStart());
}

// Either sets header to new dirname/basename/offset or returns false
bool DataReader::getHeader(const stdString &dirname,
                           const stdString &basename,
                           FileOffset offset)
{
    DataFile *datafile;
    DataHeader *new_header;
    if (!Filename::isValid(basename))
        goto no_header;
    // Read new header
    // If index tells us the full path: OK, otherwise look
    // in index'es directory
    if (Filename::containsPath(basename))
        datafile = DataFile::reference("", basename, false);
    else
        datafile = DataFile::reference(dirname, basename, false);
    if (!datafile)
    {
        LOG_MSG("DataReader(%s) cannot open data file %s\n",
                channel_name.c_str(), basename.c_str());
        goto no_header;
    }
    new_header = datafile->getHeader(offset);
    datafile->release(); // now ref'ed by new_header
    if (!new_header)
    {
        LOG_MSG("DataReader(%s): cannot get header %s @ 0x%lX\n",
                channel_name.c_str(),
                basename.c_str(), offset);
        goto no_header;
    }
    // Need to read CtrlInfo because we don't have any or it changed?
    if (header == 0                                                         ||
        new_header->data.ctrl_info_offset != header->data.ctrl_info_offset  ||
        new_header->datafile->getFilename() !=header->datafile->getFilename())
    {
        CtrlInfo new_ctrl_info;
        if (!new_ctrl_info.read(new_header->datafile,
                                new_header->data.ctrl_info_offset))
        {
            LOG_MSG("DataReader(%s): header in %s @ 0x%lX"
                    " has bad CtrlInfo\n",
                    channel_name.c_str(),
                    new_header->datafile->getBasename().c_str(),
                    new_header->offset);
            delete new_header;
            goto no_header;
        }
        // When we switch files, we read a new CtrlInfo,
        // but it might contain the same values, so compare:
        if (header == 0  ||  new_ctrl_info != ctrl_info)
        {
            ctrl_info = new_ctrl_info;
            ctrl_info_changed = true;
        }
    }
    // Switch to new header
    if (header)
        delete header;
    header = new_header;
    // Check for type change
    if (header->data.dbr_type  != dbr_type  ||
        header->data.dbr_count != dbr_count)
    {
        dbr_type  = header->data.dbr_type;
        dbr_count = header->data.dbr_count;
        raw_value_size = RawValue::getSize(dbr_type, dbr_count);
        if (data)
            RawValue::free(data);
        raw_value_size = RawValue::getSize(dbr_type, dbr_count);
        data = RawValue::allocate(dbr_type, dbr_count, 1);
        if (!data)
        {
            LOG_MSG("DataReader: Cannot alloc data for '%s'\n",
                    channel_name.c_str());
            goto no_header;
        }
        type_changed = true;
    }
    // Have header, data, ctrl_info.
    return true;
    
  no_header:
    if (header)
    {
        delete header;
        header = 0;
    }
    return false;
}

// Based on a valid 'header' & allocated 'data',
// return sample before-or-at start,
// leaving val_idx set to the following sample
// (i.e. we return sample[val_idx-1], it's stamp <= start,
//  and sample[val_idx] should be > start)
const RawValue::Data *DataReader::findSample(const epicsTime &start)
{
    LOG_ASSERT(header);
    LOG_ASSERT(data);
    // Speedier handling of start == header->data.end_time?
    // For now, the binary search is used in any case.
    // Binary search for sample before-or-at start in current header
    epicsTime stamp;
    size_t low = 0, high = header->data.num_samples - 1;
    FileOffset offset, offset0 =
        header->offset + sizeof(DataHeader::DataHeaderData);
#ifdef DEBUG_DATAREADER
    stdString stamp_txt;
    epicsTime2string(start, stamp_txt);
    printf("Goal: %s\n", stamp_txt.c_str());
#endif
    while (true)
    {
        // Pick middle value, rounded up
        val_idx = low+high;
        if (val_idx & 1)
            ++val_idx;
        val_idx /= 2;   
        offset = offset0 + val_idx * raw_value_size;
        if (! RawValue::read(dbr_type, dbr_count, raw_value_size, data,
                             header->datafile, offset))
            return 0;
        stamp = RawValue::getTime(data);
#ifdef DEBUG_DATAREADER
        epicsTime2string(stamp, stamp_txt);
        printf("Index %d: %s\n", val_idx, stamp_txt.c_str());
#endif
        if (high-low <= 1)
        {   // The intervall can't shrink further,
            // idx = (low+high)/2 == high.
            // Which value's best?
            LOG_ASSERT(val_idx == high);
            if (stamp > start)
            {
                val_idx = low;
                return next();
            }
            // else: val_idx == high is good & already in data
            break;
        }
        if (stamp == start)
            break;
        else if (stamp > start)
            high = val_idx;
        else
            low = val_idx;
    }   
    ++val_idx;
    return data;
}

// Read sample at val_idx
const RawValue::Data *DataReader::next()
{
    if (!(header && valid_datablock))
        return 0;
    if (val_idx >= header->data.num_samples            ||
        RawValue::getTime(data) > valid_interval.getEnd())
    {   // Need to get another block
        valid_datablock = au_iter->getNext(&datablock, &valid_interval);
        if (!valid_datablock)
            return 0;
#ifdef DEBUG_DATAREADER
        stdString s, e;
        epicsTime2string(valid_interval.getStart(), s);
        epicsTime2string(valid_interval.getEnd(), e);
        printf("Next  Block: %s @ 0x%lX: %s - %s\n",
               datablock.getPath(), datablock.getOffset(),
               s.c_str(), e.c_str());
#endif
        if (!getHeader(index.getDirectory(),
                       datablock.getPath(), datablock.getOffset()))
        {
            delete header;
            header = 0;
            return 0;
        }
        return findSample(valid_interval.getStart());
    }
    // Read next sample in current block       
    FileOffset offset =
        header->offset
        + sizeof(DataHeader::DataHeaderData)
        + val_idx * raw_value_size;
    if (! RawValue::read(this->dbr_type, this->dbr_count,
                         raw_value_size, data,
                         header->datafile, offset))
    {
        delete header;
        header = 0;
        return 0;
    }
    ++val_idx;
    return data;
}

