// DataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "DataFile.h"
#include "DataReader.h"
#include "AverageReader.h"
#include "PlotReader.h"
#include "LinearReader.h"

#undef DEBUG_DATAREADER

DataReader::~DataReader()
{}

const char *ReaderFactory::toString(How how, double delta)
{
    static char buf[100];
    switch (how)
    {
        case Raw:
            return "Raw Data";
        case Plotbin:
            sprintf(buf, "Plot-Binning, %g sec bins", delta);
            return buf;
        case Average:
            sprintf(buf, "Averaging every %g seconds", delta);
            return buf;
        case Linear:
            sprintf(buf, "Linear Interpolation every %g seconds", delta);
            return buf;
        default:
            return "<unknown>";
    };  
}

DataReader *ReaderFactory::create(IndexFile &index, How how, double delta)
{
    if (how == Raw  ||  delta <= 0.0)
        return new RawDataReader(index);
    else if (how == Plotbin)
        return new PlotReader(index, delta);
    else if (how == Average)
        return new AverageReader(index, delta);
    else
        return new LinearReader(index, delta);
    return 0;
}


RawDataReader::RawDataReader(IndexFile &index)
        : index(index), tree(0), node(0), rec_idx(0), valid_datablock(false),
          dbr_type(0), dbr_count(0),
          type_changed(false), ctrl_info_changed(false),
          data(0), header(0)
{}

RawDataReader::~RawDataReader()
{
    if (tree)
        delete tree;
    if (node)
        delete node;
    if (data)
        RawValue::free(data);
    if (header)
        delete header;
    DataFile::close_all();
}

const RawValue::Data *RawDataReader::find(
    const stdString &channel_name,
    const epicsTime *start)
{
    this->channel_name = channel_name;
    // Get tree
    if (!(tree = index.getTree(channel_name)))
        return false;
    node = new RTree::Node(tree->getM(), true);
    // Get 1st data block
    if (start)
        valid_datablock = tree->searchDatablock(*start, *node,
                                                rec_idx, datablock);
    else
        valid_datablock = tree->getFirstDatablock(*node, rec_idx, datablock);
    if (! valid_datablock)  // No values for this time in index
        return 0;
#ifdef DEBUG_DATAREADER
    {
        stdString s, e;
        epicsTime2string(node->record[rec_idx].start, s);
        epicsTime2string(node->record[rec_idx].end, e);
        printf("First Block: %s @ 0x%lX: %s - %s\n",
               datablock.data_filename.c_str(),
               datablock.data_offset,
               s.c_str(), e.c_str());
    }
#endif
    // Get the buffer for that data block
    if (!getHeader(index.getDirectory(),
                   datablock.data_filename, datablock.data_offset))
        return 0;
    if (start)
        return findSample(*start);
    else
        return findSample(node->record[rec_idx].start);       
}

// Read sample at val_idx
const RawValue::Data *RawDataReader::next()
{
    if (!header)
        return 0;
    // End of current header or current RTree entry
    // (if we still have an rtree entry)?
    if (val_idx >= header->data.num_samples   ||
        (valid_datablock  &&
         RawValue::getTime(data) > node->record[rec_idx].end))
    {
        if (valid_datablock)
            valid_datablock = tree->getNextDatablock(*node, rec_idx, datablock);
        if (valid_datablock)
        {
#ifdef DEBUG_DATAREADER
            stdString s, e;
            printf("Next  Block: %s @ 0x%lX: %s - %s\n",
                   datablock.data_filename.c_str(), datablock.data_offset,
                   epicsTimeTxt(node->record[rec_idx].start, s),
                   epicsTimeTxt(node->record[rec_idx].end, e));
#endif
            if (!getHeader(index.getDirectory(),
                           datablock.data_filename, datablock.data_offset))
                return 0;
            return findSample(node->record[rec_idx].start);
        }
        else
        {   // Special case: master index might be between updates.
            // Try to get more samples from Datafile, ignoring RTree.
#ifdef DEBUG_DATAREADER
            stdString txt;
            printf("RawDataReader reached end:\n");
            printf("Sample %d of %lu\n",
                   val_idx, (unsigned long)header->data.num_samples);
#endif
            // Refresh datafile and header                
            if (!(header->datafile->reopen() && header->read(header->offset)))
            {
                delete header;
                header = 0;
                return 0;
            } 
            // Need to look for next header (w/o asking RTree) ?
            if (val_idx >= header->data.num_samples)
            {
                if (!getHeader(header->datafile->getDirname(),
                               header->data.next_file, header->data.next_offset))
                    return 0; 
#ifdef DEBUG_DATAREADER
                stdString txt;
                printf("Using new data block %s @ 0x%X:\n",
                       header->datafile->getFilename().c_str(),
                       (unsigned long)header->offset);
#endif
                return findSample(header->data.begin_time);
            }
            // else fall through and continue in current header
#ifdef DEBUG_DATAREADER 
            printf("Using sample %d of %lu\n",
                   val_idx, (unsigned long)header->data.num_samples);
#endif
        }
    }
    // Read next sample in current block       
    FileOffset offset = header->offset
        + sizeof(DataHeader::DataHeaderData) + val_idx * raw_value_size;
    if (! RawValue::read(dbr_type, dbr_count, raw_value_size, data,
                         header->datafile, offset))
    {
        delete header;
        header = 0;
        return 0;
    }
    ++val_idx;
    return data;
}

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
    
// Either sets header to new dirname/basename/offset or returns false
bool RawDataReader::getHeader(const stdString &dirname,
                              const stdString &basename,
                              FileOffset offset)
{
    DataFile *datafile;
    DataHeader *new_header;
    if (!Filename::isValid(basename))
        goto no_header;
    // Read new header
    if (basename[0] == '/')
        // Index gave us the data file with the full path
        datafile = DataFile::reference("", basename, false);
    else // Look relative to the index's directory
        datafile = DataFile::reference(dirname, basename, false);
    if (!datafile)
        goto no_header;
    new_header = datafile->getHeader(offset);
    datafile->release(); // now ref'ed by new_header
    if (!new_header)
    {
        LOG_MSG("RawDataReader(%s): cannot get header %s @ 0x%lX\n",
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
            LOG_MSG("RawDataReader(%s): header in %s @ 0x%lX"
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
    if (!data ||
        header->data.dbr_type  != dbr_type  ||
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
            LOG_MSG("RawDataReader: Cannot alloc data for '%s'\n",
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
const RawValue::Data *RawDataReader::findSample(const epicsTime &start)
{
    LOG_ASSERT(header);
    LOG_ASSERT(data); 
#ifdef DEBUG_DATAREADER
    stdString stamp_txt;
    epicsTime2string(start, stamp_txt);
    printf("Goal: %s\n", stamp_txt.c_str());
#endif
    // Speedier handling of start == header->data.begin_time
    if (start == header->data.begin_time)
    {
#ifdef DEBUG_DATAREADER
        printf("Using the first sample in the buffer\n");
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


