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
class TreeDataReader
{
public:
    virtual ~TreeDataReader();
    
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
                                       const epicsTime *end) = 0;

    /// Returns next value or 0.
    virtual const RawValue::Data *next() = 0;

    /// Name of the channel, i.e. the one passed to find()
    stdString channel_name;
    
    /// The dbr_time_xxx type
    virtual DbrType getType() const = 0;
    
    /// array size
    virtual DbrCount getCount() const = 0;
    
    /// The meta information for the channel
    virtual const CtrlInfo &getInfo() const = 0;

    /// next() updates this if dbr_type/count changed.

    /// Returns whether the type changed or not AND!!
    /// resets the flag!!
    ///
    virtual bool changedType() = 0;

    /// next() updates this if ctrl_info changed.

    /// Returns whether the ctrl_info  changed or not
    /// AND(!) resets the flag!!
    ///
    virtual bool changedInfo() = 0;
};

/// @}

#endif
