// -*- c++ -*-

#ifndef __LINEAR_READER_H__
#define __LINEAR_READER_H__

#include "DataReader.h"

/// \ingroup Storage
/// @{

/// Reads data from storage w/ linear interpolation.

/// The LinearReader is an implementaion of a DataReader
/// that interpolates, returning data that is aligned onto
/// multiples of 'delta' seconds on the time axis.
/// Let's call these multiples of delta "time slots". 
///
/// Different methods are used to accomplish this:
/// - Staircase interpolation, i.e. the last value before
///   a time slot is extrapolated onto the time slot.

class LinearReader : public DataReader
{
public:
    /// Create a reader for an index.
    LinearReader(archiver_Index &index, double delta);

    ~LinearReader();
    
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start,
                               const epicsTime *end);
    
    const RawValue::Data *next();

    DbrType getType() const;
    
    DbrCount getCount() const;
    
    const CtrlInfo &getInfo() const;
    
    bool changedType();
    
    bool changedInfo();

private:
    RawDataReader reader;
    double delta;

    // Current value of reader
    const RawValue::Data *reader_data;

    // Current value of this LinearReader
    epicsTime time_slot;
    DbrType type;
    DbrCount count;
    CtrlInfo info;
    bool type_changed;
    bool ctrl_info_changed;
    RawValue::Data *data;
};

#endif
