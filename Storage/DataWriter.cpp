// DataWriter.cpp

// Tools
#include <MsgLogger.h>
#include <Filename.h>
#include <epicsTimeHelper.h>
// Storage
#include "DataFile.h"
#include "DataWriter.h"

FileOffset DataWriter::file_size_limit = 100*1024*1024; // 100MB Default.

DataWriter::DataWriter(IndexFile &index,
                       const stdString &channel_name,
                       const CtrlInfo &ctrl_info,
                       DbrType dbr_type,
                       DbrCount dbr_count,
                       double period,
                       size_t num_samples)
        : index(index), tree(0), channel_name(channel_name),
          ctrl_info(ctrl_info), dbr_type(dbr_type),
          dbr_count(dbr_count), period(period), header(0), available(0)
{
    DataFile *datafile;

    // Size of next buffer should at least hold num_samples
    calc_next_buffer_size(num_samples);
    raw_value_size = RawValue::getSize(dbr_type, dbr_count);

    // Find or add appropriate data buffer
    tree = index.addChannel(channel_name);
    if (!tree)
    {
        LOG_MSG("DataWriter(%s) cannot get Tree\n",
                channel_name.c_str());
        return;
    }
    RTree::Datablock block;
    RTree::Node node(tree->getM(), true);
    int idx;
    if (tree->getLastDatablock(node, idx, block))        
    {   // - There is a data file and buffer
        if (! (datafile = DataFile::reference(
                   index.getDirectory(), block.data_filename, true)))
        {
            LOG_MSG("DataWriter(%s) cannot open data file %s\n",
                    channel_name.c_str(), block.data_filename.c_str());
            return;
        } 
        header = datafile->getHeader(block.data_offset);
        datafile->release(); // now ref'ed by header
        if (!header)
        {
            LOG_MSG("DataWriter(%s): cannot get header %s @ 0x%lX\n",
                    channel_name.c_str(),
                    block.data_filename.c_str(), block.data_offset);
            return;
        }
        // See if anything has changed
        CtrlInfo prev_ctrl_info;
        if (!prev_ctrl_info.read(header->datafile,
                                 header->data.ctrl_info_offset))
        {
            LOG_MSG("DataWriter(%s): header in %s @ 0x%lX"
                    " has bad CtrlInfo\n",
                    channel_name.c_str(),
                    header->datafile->getBasename().c_str(),
                    header->offset);
            delete header;
            header = 0;
            return;            
        }
        if (prev_ctrl_info != ctrl_info)
        {   // Add new header because info has changed
            if (!addNewHeader(true))
                return;
        }
        else if (header->data.dbr_type != dbr_type  ||
                 header->data.dbr_count != dbr_count)
        {   // Add new header because type has changed
            if (!addNewHeader(false))
                return;
        }
        else
        {   // All fine, just check if we're already in bigger league
            size_t capacity = header->capacity();
            if (capacity > num_samples)
                calc_next_buffer_size(capacity);
        }
    }   
    else
    {
        if (!addNewHeader(true))
            return;
    }
    available = header->available();
}
    
DataWriter::~DataWriter()
{
    // Update index
    if (header)
    {   
        header->write();
        if (tree)
        {
            if (!tree->updateLastDatablock(
                    header->data.begin_time, header->data.end_time,
                    header->offset, header->datafile->getBasename()))
            {
                LOG_MSG("Cannot add %s @ 0x%lX to index\n",
                        header->datafile->getBasename().c_str(),
                        header->offset);
            }
            delete tree;
            tree = 0;
        }
        delete header;
        header = 0;
    }
}

bool DataWriter::add(const RawValue::Data *data)
{
    if (!header) // In here, we should always have a header
        return false;
    if (available <= 0) // though it might be full
    {
        if (!addNewHeader(false))
            return false;
        available = header->available();
    }
    // Add the value
    available -= 1;
    FileOffset offset = header->offset
        + sizeof(DataHeader::DataHeaderData)
        + header->data.curr_offset;
    RawValue::write(dbr_type, dbr_count,
                    raw_value_size, data,
                    cvt_buffer,
                    header->datafile, offset);
    // Update the header
    header->data.curr_offset += raw_value_size;
    header->data.num_samples += 1;
    header->data.buf_free    -= raw_value_size;
    epicsTime time = RawValue::getTime(data);
    if (header->data.num_samples == 1) // first entry?
        header->data.begin_time = time;
    header->data.end_time = time;
    // Note: we didn't write the header nor update the index,
    // that'll happen when we close the DataWriter!
    return true;
}

// Create name "<today>[-serial]"
void DataWriter::makeDataFileName(int serial, stdString &name)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    char buffer[30];    
    epicsTime now = epicsTime::getCurrent();    
    epicsTime2vals(now, year, month, day, hour, min, sec, nano);
    if (serial > 0)
        sprintf(buffer, "%04d%02d%02d-%d", year, month, day, serial);
    else
        sprintf(buffer, "%04d%02d%02d", year, month, day);
    name = buffer;
}

// Create new DataFile that's below file_size_limit in size.
DataFile *DataWriter::createNewDataFile(size_t headroom)
{
    DataFile *datafile;
    int serial=0;
    stdString data_file_name;
    FileOffset file_size;
    while (true)
    {
        makeDataFileName(serial, data_file_name);
        datafile = DataFile::reference(index.getDirectory(),
                                       data_file_name, true);
        if (! datafile)
        {
            LOG_MSG ("DataWriter(%s): Cannot create data file '%s'\n",
                     channel_name.c_str(), data_file_name.c_str());
            return 0;
        }
        if (!datafile->getSize(file_size))
        {
            datafile->release();
            return 0;
        }
        if (file_size+headroom < file_size_limit)
            return datafile;
        if (datafile->is_new_file)
        {
            LOG_MSG ("DataWriter(%s): "
                     "Cannot create a new data file within file size limit\n",
                     channel_name.c_str());
            return datafile; // Use anyway
        }
        ++serial;
        datafile->release();
    }
}

void DataWriter::calc_next_buffer_size(size_t start)
{
    if (start < 64)
        next_buffer_size = 64;
    else if (start > 4096)
        next_buffer_size = 4096;
    else
    {   // We want the next power of 2:
        int log2 = 0;
        while (start > 0)
        {
            start >>= 1;
            ++log2;
        }
        next_buffer_size = 1 << log2;
    }
}

// Add a new header because
// - there's none
// - data type or ctrl_info changed
// - the existing data buffer is full.
// Might switch to new DataFile.
// Will write ctrl_info out if new_ctrl_info is set,
// otherwise the new header tries to point to the existing ctrl_info.
bool DataWriter::addNewHeader(bool new_ctrl_info)
{
    DataFile  *datafile;
    FileOffset ctrl_info_offset;
    bool       new_datafile = false;    // Need to use new DataFile?
    size_t     headroom = 0;
    if (header == 0)
        new_datafile = true;            // Yes, because there's none.
    else
    {   // Check how big the current data file would get
        FileOffset file_size;
        if (! header->datafile->getSize(file_size))
            return false;
        headroom = header->datafile->getHeaderSize(dbr_type, dbr_count,
                                                     next_buffer_size);
        if (new_ctrl_info)
            headroom += ctrl_info.getSize();
        if (file_size+headroom > file_size_limit) // Yes: reached max. size.
            new_datafile = true;
    }
    if (new_datafile)
    {
        if (! (datafile = createNewDataFile(headroom)))
            return false;
        new_ctrl_info = true;
    }
    else
        datafile = header->datafile;
    if (new_ctrl_info)
    {
        if (!datafile->addCtrlInfo(ctrl_info, ctrl_info_offset))
        {
            LOG_MSG("DataWriter(%s): Cannot add CtrlInfo to file '%s'\n",
                    channel_name.c_str(), datafile->getBasename().c_str());
            if (new_datafile)
                datafile->release();
            return false;
        }        
    }
    else // use existing one
        ctrl_info_offset = header->data.ctrl_info_offset;
    DataHeader *new_header =
        datafile->addHeader(dbr_type, dbr_count, period, next_buffer_size);
    if (new_datafile)
        datafile->release(); // now ref'ed by new_header
    if (!new_header)
        return false;
    new_header->data.ctrl_info_offset = ctrl_info_offset;
    if (header)
    {   // Link old header to new one
        header->set_next(new_header->datafile->getBasename(),
                         new_header->offset);
        header->write();
        // back from new to old
        new_header->set_prev(header->datafile->getBasename(),
                             header->offset);        
        // Update index entry for the old header
        if (tree)
        {
            if (!tree->updateLastDatablock(
                    header->data.begin_time, header->data.end_time,
                    header->offset, header->datafile->getBasename()))
            {
                LOG_MSG("Cannot add %s @ 0x%lX to index\n",
                        header->datafile->getBasename().c_str(), header->offset);
            }
        }
        delete header;
    }
    // Switch to new header
    header = new_header;
    // Upp the buffer size
    calc_next_buffer_size(next_buffer_size);
    // new header will be added to index when it's closed
    return true;
}
    
