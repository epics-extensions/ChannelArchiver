
// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "DataFile.h"
#include "DataWriter.h"

DataWriter::DataWriter(DirectoryFile &index,
                       const stdString &channel_name,
                       const CtrlInfo &ctrl_info,
                       DbrType dbr_type,
                       DbrCount dbr_count,
                       size_t num_samples)
        : index(index), channel_name(channel_name),
          ctrl_info(ctrl_info), dbr_type(dbr_type),
          dbr_count(dbr_count)
{
    DataFile *datafile;

    raw_value_size = RawValue::getSize(dbr_type, dbr_count);
    header = 0;
    available = 0;    
    dfi = index.find(channel_name);
    if (!dfi.isValid())
    {
        LOG_MSG ("DataWriter: Cannot find '%s' in index\n",
                 channel_name.c_str());
        return;
    }
    if (!Filename::isValid(dfi.entry.data.last_file))
    {
        // TODO: Add new header
        LOG_MSG("%s NEVER WRITTEN. NOT YET\n", channel_name.c_str());
        return;
    }
    else
    {
        datafile = DataFile::reference(index.getDirname(),
                                       dfi.entry.data.last_file, true);
        if (!datafile)
        {
            LOG_MSG("ArchiveChannel::write(%s) cannot open data file %s\n",
                    channel_name.c_str(), dfi.entry.data.last_file);
            return;
        } 
        header = datafile->getHeader(dfi.entry.data.last_offset);
        // Header should now ref. the datafile, we no longer need it.
        datafile->release();
        if (!header)
        {
            LOG_MSG("ArchiveChannel::write(%s): cannot get header %s @ 0x%lX\n",
                    channel_name.c_str(),
                    dfi.entry.data.last_file,
                    dfi.entry.data.last_offset);
            return;
        }
        // TODO: Compare CtrlInfo
        if (header->data.dbr_type != dbr_type  ||
            header->data.nelements != dbr_count)
        {
            // TODO: Add new header because type has changed
        }
        else
            available = header->available();
    }
}
    
DataWriter::~DataWriter()
{
    // Update index
    if (header)
    {
        if (dfi.isValid())
        {
            dfi.entry.data.last_save_time = header->data.end_time;
            dfi.save();
            header->write();
        }
        else
        {
            LOG_MSG("DataWriter(%s): Cannot update index, it's invalid\n",
                    channel_name.c_str());
        }
        delete header;
    }
}

bool DataWriter::add(const RawValue::Data *data)
{
    // In here, we should always have a valid header, but
    // it might be full.
    if (!header)
        return false;
    if (available <= 0)
    {
        // TODO: Add header
        return false;
    }
    available -= 1;
    FileOffset offset = header->offset
        + sizeof(DataHeader::DataHeaderData)
        + header->data.curr_offset;
    RawValue::write(dbr_type, dbr_count,
                    raw_value_size, data,
                    cvt_buffer,
                    header->datafile, offset);
    header->data.curr_offset += raw_value_size;
    header->data.num_samples += 1;
    header->data.buf_free    -= raw_value_size;
    
    epicsTime time = RawValue::getTime(data);
    if (header->data.num_samples == 1) // first entry?
        header->data.begin_time = time;
    header->data.end_time = time;
    // Note: we don't write the header nor dfi!
    return true;
}
