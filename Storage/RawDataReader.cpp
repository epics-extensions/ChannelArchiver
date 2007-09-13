// RawDataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "RawDataReader.h"
#include "DataFile.h"

// #define DEBUG_DATAREADER

RawDataReader::RawDataReader(Index &index)
        : index(index),
          dbr_type(0),
          dbr_count(0),
          type_changed(false),
          ctrl_info_changed(false),
          period(0.0),
          raw_value_size(0),
          val_idx(0)
{
    LOG_ASSERT(index.getFilename().length() > 0);
}

RawDataReader::~RawDataReader()
{
    // Delete the header which might still keep a data file open.
    header = 0;
    DataFile::clear_cache();
}

const RawValue::Data *RawDataReader::find(const stdString &channel_name,
                                          const epicsTime *start)
{
#   ifdef DEBUG_DATAREADER
    printf("- RawDataReader(%s)::find(%s)\n",
           index.getName().c_str(), channel_name.c_str());
#   endif
    this->channel_name = channel_name;
    // Lookup channel in index
    index_result = index.findChannel(channel_name);
    if (! index_result)
        return 0; // Channel not found
    try
    {
        // Get 1st data block
        if (start)
            datablock = index_result->getRTree()->search(*start);
        else
            datablock = index_result->getRTree()->getFirstDatablock();
        if (! datablock)  // No values for this time in index
            return 0;
        LOG_ASSERT(datablock->isValid());
#       ifdef DEBUG_DATAREADER
        {
            stdString range = datablock->getInterval().toString();
            printf("- First Block: %s @ 0x%lX: %s\n",
                   datablock->getDataFilename().c_str(),
                   (unsigned long)datablock->getDataOffset(),
                   range.c_str());
        }
#       endif
        if (datablock->getDataOffset() == 0)
            throw GenericException(__FILE__, __LINE__,
                                   "Cannot handle shallow indices");
        // Get the buffer for that data block
        getHeader(datablock->getDataFilename(), datablock->getDataOffset());
        if (start)
            return findSample(*start);
        else
            return findSample(datablock->getInterval().getStart());
    }
    catch (GenericException &e)
    {  // Add channel name to the message
        throw GenericException(__FILE__, __LINE__,
                               "Index '%s', Channel '%s':\n%s",
                               index.getFilename().c_str(),
                               channel_name.c_str(), e.what());
    }
}

const stdString &RawDataReader::getName() const
{
    return channel_name;
}                                  

// Read next sample, the one to which val_idx points.
const RawValue::Data *RawDataReader::next()
{
    if (!header)
        throw GenericException(__FILE__, __LINE__,
                               "Data Reader called after reaching end of data");
    // Still in current header?
    if (val_idx < header->data.num_samples)
        return nextFromDatablock();
    // Exhaused the current header.
    if (datablock)
    {   // Try to get next data block
        LOG_ASSERT(datablock->isValid());
        if (datablock->getNextDatablock())
        {
#           ifdef DEBUG_DATAREADER
            printf("- Next Block: %s @ 0x%lX: %s\n",
                   datablock->getDataFilename().c_str(),
                   (unsigned long)datablock->getDataOffset(),
                   datablock->getInterval().toString().c_str());
                   
#           endif
            getHeader(datablock->getDataFilename(), datablock->getDataOffset());
            if (!findSample(datablock->getInterval().getStart()))
                return 0;
            if (RawValue::getTime(data) >= datablock->getInterval().getStart())
                return data;
#       ifdef DEBUG_DATAREADER
            printf("- findSample gave sample<start, skipping that one\n");
#       endif
            return next();  
        } 
        // indicate that there's no datablock
        datablock = 0;
    }
    // TODO: For better ListIndex functionality,
    //       ask index again with proper start time,
    //       which might switch to to another sub-archive
    // data= find(...., last known end time);
    // maybe skip one sample < proper time.

    // else: RTree indicates end of data.
    // In the special case of a master index between updates,
    // the last data file might in fact have more samples
    // than the RTree thinks there are, so try to read on.
#   ifdef DEBUG_DATAREADER
    printf("- RawDataReader(%s) reached end for '%s' in '%s': ",
           index.getName().c_str(),
           header->datafile->getBasename().c_str(),
           header->datafile->getDirname().c_str());
    printf("Sample %zd of %lu\n",
           val_idx, (unsigned long)header->data.num_samples);
#   endif
    // Refresh datafile and header.
    header->datafile->reopen();
    header->read(header->offset);
    // Need to look for next header (w/o asking RTree) ?
    if (val_idx >= header->data.num_samples)
    {
        if (!Filename::isValid(header->data.next_file))
            return 0; // Reached last sample in last buffer.
        // Found a new buffer, unknown to RTree
        getHeader(header->data.next_file, header->data.next_offset);
#       ifdef DEBUG_DATAREADER
        printf("- Using new data block %s @ 0x%X:\n",
                   header->datafile->getFilename().c_str(),
                   (unsigned int)header->offset);
#       endif
        return findSample(header->data.begin_time);
    }
    // After refresh, the last buffer had more samples.
#   ifdef DEBUG_DATAREADER 
    printf("Using sample %zd of %lu\n",
           val_idx, (unsigned long)header->data.num_samples);
#   endif
    return nextFromDatablock();
}   
    
// Read next sample, the one to which val_idx points.
const RawValue::Data *RawDataReader::nextFromDatablock()
{    
    // Read 'val_idx' sample in current block.
    FileOffset offset = header->offset
        + sizeof(DataHeader::DataHeaderData) + val_idx * raw_value_size;
    RawValue::read(dbr_type, dbr_count, raw_value_size, data,
                   header->datafile, offset);
    // If we still have an RTree entry: Are we within bounds?
    // This is because the DataFile might contain the current sample
    // in the current buffer, but the RTree already has a different
    // DataFile in mind for this time stamp.
    if (datablock  &&
        RawValue::getTime(data) > datablock->getInterval().getEnd())
    {   // Recurse after marking end of samples in the current datafile
        val_idx = header->data.num_samples;
        return next();
    }
    ++val_idx;
    return data;
}

const RawValue::Data *RawDataReader::get() const
{   return data; }

DbrType RawDataReader::getType() const
{   return dbr_type; }
    
DbrCount RawDataReader::getCount() const
{   return dbr_count; }
    
const CtrlInfo &RawDataReader::getInfo() const
{   return ctrl_info; }

bool RawDataReader::changedType()
{
    bool changed = type_changed;
    type_changed = false;
    return changed;
}

bool RawDataReader::changedInfo()
{
    bool changed = ctrl_info_changed;
    ctrl_info_changed = false;
    return changed;
} 
    
// Sets header to new dirname/basename/offset,
// or throws GenericException with details.
void RawDataReader::getHeader(const stdString &basename, FileOffset offset)
{
    if (!Filename::isValid(basename))
        throw GenericException(__FILE__, __LINE__, "'%s': Invalid basename",
                               channel_name.c_str());
    try
    {
        AutoPtr<DataHeader>  new_header;
        {   // Read new header
            DataFile *datafile;
            if (basename[0] == '/') // Index gave us the data file with the full path
                datafile = DataFile::reference("", basename, false);
            else // Look relative to the index's directory
                datafile = DataFile::reference(index_result->getDirectory(),
                                               basename, false);
            // If we keep opening data files, we'll hit the max-open-files limit.
            // Close files which are no longer referenced:
            DataFile::clear_cache();
    
            try
            {
                new_header = datafile->getHeader(offset);
            }
            catch (GenericException &e)
            {   // Something is wrong with datafile, can't read header.
                // Release the file, then send the error upwards.
                datafile->release();
                throw e;
            }
            // DataFile now ref'ed by new_header.
            datafile->release();
        }
        // Need to read CtrlInfo because we don't have any or it changed?
        if (!header                                                              ||
            new_header->data.ctrl_info_offset   != header->data.ctrl_info_offset ||
            new_header->datafile->getFilename() != header->datafile->getFilename())
        {
            CtrlInfo new_ctrl_info;
            new_ctrl_info.read(new_header->datafile,
                               new_header->data.ctrl_info_offset);
            // When we switch files, we read a new CtrlInfo,
            // but it might contain the same values, so compare:
            if (header == 0  ||  new_ctrl_info != ctrl_info)
            {
                ctrl_info = new_ctrl_info;
                ctrl_info_changed = true;
            }
        }
        // Switch to new header. AutoPtr will release previous header.
        header = new_header;
        // If we never allocated a RawValue, or the type changed...
        if (!data ||
            header->data.dbr_type  != dbr_type  ||
            header->data.dbr_count != dbr_count)
        {
            dbr_type  = header->data.dbr_type;
            dbr_count = header->data.dbr_count;
            raw_value_size = RawValue::getSize(dbr_type, dbr_count);
            data = RawValue::allocate(dbr_type, dbr_count, 1);
            type_changed = true;
        }
    }
    catch (GenericException &e)
    {
        throw GenericException(__FILE__, __LINE__,
           "Error in data header '%s', '%s' @ 0x%08lX for channel '%s'.\n%s",
           index_result->getDirectory().c_str(),
           basename.c_str(),
           (unsigned long) offset, channel_name.c_str(),
           e.what());
    }
}

// Based on a valid 'header' & allocated 'data',
// return sample before-or-at start,
// leaving val_idx set to the following sample
// (i.e. we return sample[val_idx-1],
//  its stamp is <= start,
//  and sample[val_idx] should be > start)
const RawValue::Data *RawDataReader::findSample(const epicsTime &start)
{
    LOG_ASSERT(header);
    LOG_ASSERT(data); 
#ifdef DEBUG_DATAREADER
    stdString stamp_txt;
    epicsTime2string(start, stamp_txt);
    printf("- Goal: %s\n", stamp_txt.c_str());
#endif
    // Speedier handling of start == header->data.begin_time
    if (start == header->data.begin_time)
    {
#ifdef DEBUG_DATAREADER
        printf("- Using the first sample in the buffer\n");
#endif
        val_idx = 0;
        return next();
    }
    // Binary search for sample before-or-at start in current header
    epicsTime stamp;
    size_t low = 0, high = header->data.num_samples - 1;
    FileOffset offset, offset0 =
        header->offset + sizeof(DataHeader::DataHeaderData);
    while (true)
    {   // Pick middle value, rounded up
        val_idx = (low+high+1)/2;
        offset = offset0 + val_idx * raw_value_size;
        RawValue::read(dbr_type, dbr_count, raw_value_size, data,
                       header->datafile, offset);
        stamp = RawValue::getTime(data);
#ifdef DEBUG_DATAREADER
        epicsTime2string(stamp, stamp_txt);
        printf("- Index %zd: %s\n", val_idx, stamp_txt.c_str());
#endif
        if (high-low <= 1)
        {   // The intervall can't shrink further.
            // idx == high because of up-rounding above.
            // Which value's best?
            LOG_ASSERT(val_idx == high);
            if (stamp > start)
            {
                val_idx = low;
                return next();
            }
            // else: val_idx == high is good & already read into data
            break;
        }
        if (stamp == start)
            break; // val_idx is perfect & already read into data
        else if (stamp > start)
            high = val_idx;
        else
            low = val_idx;
    }   
    ++val_idx;
    return data;
}


