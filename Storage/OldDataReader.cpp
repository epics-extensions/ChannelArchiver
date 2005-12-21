// DataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "DataFile.h"
#include "OldDataReader.h"

OldDataReader::OldDataReader(OldDirectoryFile &index)
        : index(index), data(0), header(0)
{
}

OldDataReader::~OldDataReader()
{
    if (data)
        RawValue::free(data);
    if (header)
        delete header;
    DataFile::close_all();
}

const RawValue::Data *OldDataReader::find(
    const stdString &channel_name, const epicsTime *start)
{
    this->channel_name = channel_name;
    OldDirectoryFileIterator dfi = index.find(channel_name);
    if (!dfi.isValid())
    {
        LOG_MSG ("OldDataReader: Cannot find '%s' in index\n",
                 channel_name.c_str());
        return 0;
    }
    if (start)
    {   // look for time, start at end
        header = getHeader(index.getDirname(),
                           dfi.entry.data.last_file,
                           dfi.entry.data.last_offset);
        if (!header)
        {
            LOG_MSG("OldDataReader: No data for '%s'\n",
                    channel_name.c_str());
            return 0;
        }
        ///
        stdString txt;
        epicsTime2string(epicsTime(header->data.begin_time), txt);
        printf("header: %s\n", txt.c_str());
        ///
        while (epicsTime(header->data.begin_time) > *start)
        {
            if (!header->read_prev())
            {
                delete header;
                header = 0;
                LOG_MSG("OldDataReader: No data for '%s'\n",
                        channel_name.c_str());
                return 0;
            }
            epicsTime2string(epicsTime(header->data.begin_time), txt);
            printf("header: %s\n", txt.c_str());

        }
        // Have header <= *start
    }
    else
    {   // Start at first buffer
        header = getHeader(index.getDirname(),
                           dfi.entry.data.first_file,
                           dfi.entry.data.first_offset);
        if (!header)
        {
            LOG_MSG("OldDataReader: No data for '%s'\n",
                    channel_name.c_str());
            return 0;
        }
    }
    dbr_type = header->data.dbr_type;
    dbr_count = header->data.dbr_count;
    if (!ctrl_info.read(header->datafile,
                        header->data.ctrl_info_offset))
    {
        LOG_MSG("OldDataReader(%s): header in %s @ 0x%lX"
                " has bad CtrlInfo\n",
                channel_name.c_str(),
                header->datafile->getBasename().c_str(),
                (unsigned long)header->offset);
        delete header;
        header = 0;
        return 0;     
    }
    raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    data = RawValue::allocate(dbr_type, dbr_count, 1);
    if (!data)
    {
        LOG_MSG("OldDataReader: Cannot alloc data for '%s'\n",
                channel_name.c_str());
        delete header;
        header = 0;
        return 0;
    }
    val_idx = 0;
    type_changed = false;
    ctrl_info_changed = false;
    if (start)
    {
        // Binary search for best matching sample.
        size_t low = 0, high = header->data.num_samples - 1;
        FileOffset offset0 =
            header->offset + sizeof(DataHeader::DataHeaderData);
        FileOffset offset;
        epicsTime stamp;
        ///
        stdString start_txt;
        epicsTime2string(*start, start_txt);
        printf("Start: %s\n", start_txt.c_str());
        ///
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
            ///
            stdString stamp_txt;
            epicsTime2string(stamp, stamp_txt);
            printf("Index %u: %s\n", (unsigned int)val_idx, stamp_txt.c_str());
            ///
            if (high-low <= 1)
            {   // The intervall can't shrink further,
                // idx = (low+high)/2 == high.
                // Which value's best?
                LOG_ASSERT(val_idx == high);
                if (stamp > *start)
                {
                    val_idx = low;
                    return next();
                }
                // else: val_idx == high is good & already in data
                break;
            }
            if (stamp == *start)
                break;
            else if (stamp > *start)
                high = val_idx;
            else
                low = val_idx;
        }   
        ++val_idx;
        return data;
    }
    else
        return next();
}    

const RawValue::Data *OldDataReader::next()
{
    if (!header)
        return 0;    
    if (val_idx >= header->data.num_samples)
    {
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
                LOG_MSG("OldDataReader(%s): header in %s @ 0x%lX"
                        " has bad CtrlInfo\n",
                        channel_name.c_str(),
                        new_header->datafile->getBasename().c_str(),
                        (unsigned long)new_header->offset);
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
                LOG_MSG("OldDataReader: Cannot alloc data for '%s'\n",
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
}

// Either sets header to new dirname/basename/offset
// or returns false
DataHeader *OldDataReader::getHeader(const stdString &dirname,
                                     const stdString &basename,
                                     FileOffset offset)
{
    if (!Filename::isValid(basename))
        return 0;
    DataFile *datafile =
        DataFile::reference(dirname, basename, false);
    if (!datafile)
    {
        LOG_MSG("OldDataReader(%s) cannot open data file %s\n",
                channel_name.c_str(), basename.c_str());
        return 0;
    }
    DataHeader *new_header = datafile->getHeader(offset);
    datafile->release(); // now ref'ed by header
    if (!new_header)
    {
        LOG_MSG("OldDataReader(%s): cannot get header %s @ 0x%lX\n",
                channel_name.c_str(),
                basename.c_str(), (unsigned long)offset);
        return 0;
    }
    return new_header;
}


