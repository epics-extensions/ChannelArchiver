// DataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "DataFile.h"
#include "DataReader.h"

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
    const epicsTime *start,
    const epicsTime *end)
{
    this->channel_name = channel_name;
    // default: all that's in archive
    interval range;
    if (start==0  ||  end==0)
    {
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
    // --
    {
        stdString txt;
        epicsTime2string(range.getStart(), txt);
        printf("DataReader::find: %s ...", txt.c_str());
        epicsTime2string(range.getEnd(), txt);
        printf("%s\n", txt.c_str());
    }
    // --
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
    // --
    stdString s, e;
    epicsTime2string(valid_interval.getStart(), s);
    epicsTime2string(valid_interval.getEnd(), e);
    printf("%s @ 0x%lX: %s - %s\n",
           datablock.getPath(),
           datablock.getOffset(),
           s.c_str(), e.c_str());
    // --
    // Get the buffer for that data block
    header = getHeader("", // TODO: index.getDirname(),
                       datablock.getPath(),
                       datablock.getOffset());
    if (!header)
    {
        LOG_MSG("DataReader: No data for '%s'\n",
                channel_name.c_str());
        return 0;
    }
    dbr_type = header->data.dbr_type;
    dbr_count = header->data.dbr_count;
    if (!ctrl_info.read(header->datafile,
                        header->data.ctrl_info_offset))
    {
        LOG_MSG("DataReader(%s): header in %s @ 0x%lX"
                " has bad CtrlInfo\n",
                channel_name.c_str(),
                header->datafile->getBasename().c_str(),
                header->offset);
        delete header;
        header = 0;
        return 0;     
    }
    raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    data = RawValue::allocate(dbr_type, dbr_count, 1);
    if (!data)
    {
        LOG_MSG("DataReader: Cannot alloc data for '%s'\n",
                channel_name.c_str());
        delete header;
        header = 0;
        return 0;
    }
    type_changed = false;
    ctrl_info_changed = false;

    
    // Binary search for best matching sample.
#error somewhere in here
    epicsTime goal = valid_interval.getStart();
    epicsTime stamp;
    size_t low = 0, high = header->data.num_samples - 1;
    FileOffset offset0 =
        header->offset + sizeof(DataHeader::DataHeaderData);
    FileOffset offset;
    // --
    stdString goal_txt;
    epicsTime2string(goal, goal_txt);
    printf("Goal: %s\n", goal_txt.c_str());
    // --
    while (true)
    {
        // Pick middle value, rounded up
        val_idx = low+high;
        if (val_idx & 1)
            ++val_idx;
        val_idx /= 2;   
        offset = offset0 + val_idx * raw_value_size;
        if (! RawValue::read(this->dbr_type, this->dbr_count,
                             raw_value_size, data,
                             header->datafile, offset))
        {
            delete header;
            header = 0;
            return 0;
        }
        stamp = RawValue::getTime(data);
        // --
        stdString stamp_txt;
        epicsTime2string(stamp, stamp_txt);
        printf("Index %d: %s\n", val_idx, stamp_txt.c_str());
        // --
        if (high-low <= 1)
        {   // The intervall can't shrink further,
            // idx = (low+high)/2 == high.
            // Which value's best?
            LOG_ASSERT(val_idx == high);
            if (stamp > goal)
            {
                val_idx = low;
                return next();
            }
            // else: val_idx == high is good & already in data
            break;
        }
        if (stamp == goal)
            break;
        else if (stamp > goal)
            high = val_idx;
        else
            low = val_idx;
    }   
    ++val_idx;
    return data;
#endif
    return 0;
}    

const RawValue::Data *DataReader::next()
{
#if 0
    if (!header)
        return 0;    
    if (val_idx >= header->data.num_samples)
    {
                valid_datablock = au_iter->getNext(&datablock, &valid_interval);

        val_idx = 0;
        DataHeader *new_header =
            getHeader(header->datafile->getDirname(),
                      header->data.next_file,
                      header->data.next_offset);
        if (!new_header)
        {
            delete header;
            header = 0;
            return 0;
        }
        // See if CtrlInfo has changed
        if (new_header->data.ctrl_info_offset !=
            header->data.ctrl_info_offset          ||
            new_header->datafile->getFilename() !=
            header->datafile->getFilename())
        {
            CtrlInfo new_ctrl_info;
            if (!new_ctrl_info.read(
                    new_header->datafile,
                    new_header->data.ctrl_info_offset))
            {
                LOG_MSG("DataReader(%s): header in %s @ 0x%lX"
                        " has bad CtrlInfo\n",
                        channel_name.c_str(),
                        new_header->datafile->getBasename().c_str(),
                        new_header->offset);
                delete new_header;
                delete header;
                header = 0;
                return 0;
            }
            // When we switch files, we read a new CtrlInfo,
            // but it might contain the same values, so compare:
            if (new_ctrl_info != ctrl_info)
            {
                ctrl_info = new_ctrl_info;
                ctrl_info_changed = true;
            }
        }
        // Switch to new header
        delete header;
        header = new_header;
        // Check for type change
        if (header->data.dbr_type != dbr_type  ||
            header->data.dbr_count != dbr_count)
        {
            dbr_type = header->data.dbr_type;
            dbr_count = header->data.dbr_count;
            RawValue::free(data);
            raw_value_size = RawValue::getSize(dbr_type, dbr_count);
            data = RawValue::allocate(dbr_type, dbr_count, 1);
            if (!data)
            {
                LOG_MSG("DataReader: Cannot alloc data for '%s'\n",
                        channel_name.c_str());
                delete header;
                header = 0;
                return 0;
            }
            type_changed = true;
        }
    }
    // Read next sample        
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
#endif
    return 0;
}

// Either sets header to new dirname/basename/offset
// or returns false
DataHeader *DataReader::getHeader(const stdString &dirname,
                                  const stdString &basename,
                                  FileOffset offset)
{
    if (!Filename::isValid(basename))
        return 0;
    DataFile *datafile =
        DataFile::reference(dirname, basename, false);
    if (!datafile)
    {
        LOG_MSG("DataReader(%s) cannot open data file %s\n",
                channel_name.c_str(), basename.c_str());
        return 0;
    }
    DataHeader *new_header = datafile->getHeader(offset);
    datafile->release(); // now ref'ed by header
    if (!new_header)
    {
        LOG_MSG("DataReader(%s): cannot get header %s @ 0x%lX\n",
                channel_name.c_str(),
                basename.c_str(), offset);
        return 0;
    }
    return new_header;
}
