// -*- c++ -*-

#ifndef __DATA_READER_H__
#define __DATA_READER_H__

// Tools
#include "stdString.h"
// Storage
#include "CtrlInfo.h"
#include "RawValue.h"
// rtree
#include "archiver_index.h"

/// \ingroup Storage
/// @{

/// Reads data from storage.

/// The data reader interfaces to the Index/DataFile
/// and returns a stream of RawValue values.
///
class DataReader
{
public:
    /// Create a reader for an index.
    DataReader(archiver_Index &index);

    virtual ~DataReader();
    
    /// Locate data.

    /// Positions reader on given channel and start time.
    ///
    /// Specifically: If a value with the exact start time exists,
    /// it will be returned. Otherwise the value just before the start time
    /// is returned, so that the user can then decide if and how that value
    /// might extrapolate onto the start time.
    ///
    /// \param channel_name: Name of the channel
    /// \param start: start time or 0 for first value
    /// \param end: end time or 0 for last value
    /// \return Returns value or 0
    virtual const RawValue::Data *find(const stdString &channel_name,
                                       const epicsTime *start,
                                       const epicsTime *end);

    /// Returns next value or 0.
    virtual const RawValue::Data *next();

    /// Name of the channel, i.e. the one passed to find()
    stdString channel_name;
    
    /// The dbr_time_xxx type
    DbrType dbr_type;
    
    /// array size
    DbrCount dbr_count;
    
    /// The meta information for the channel
    CtrlInfo ctrl_info;

    /// next() updates this if dbr_type/count changed.
    bool type_changed;

    /// next() updates this if ctrl_info changed.
    bool ctrl_info_changed;
    
    double period;    

private:
    archiver_Index &index;
    key_AU_Iterator *au_iter; // iterator for index & channel_name
    bool valid_datablock; // is au_iter on valid datablock? 
    key_Object datablock; // the current datablock
    interval   valid_interval;    // the valid interval in there
    RawValue::Data *data;
    size_t raw_value_size;
    class DataHeader *header;
    size_t val_idx; // current index in data buffer

    bool getHeader(const stdString &dirname, const stdString &basename,
                   FileOffset offset);
    const RawValue::Data *findSample(const epicsTime &start);
};

/// @}

#endif
