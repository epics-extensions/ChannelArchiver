// -*- c++ -*-

#ifndef __AVERAGE_READER_H__
#define __AVERAGE_READER_H__

#include "DataReader.h"

/// \ingroup Storage
/// @{

/// Reads data from storage, averaging over the raw samples.

/// The AverageReader is an implementaion of a DataReader
/// that returns the average value of the raw values within
/// each 'bin' of 'delta' seconds on the time axis.
class AverageReader : public DataReader
{
public:
    /// Create a reader for an index.
    AverageReader(IndexFile &index, double delta);
    virtual ~AverageReader();
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start);
    const RawValue::Data *next();
    DbrType getType() const;
    DbrCount getCount() const;
    const CtrlInfo &getInfo() const;
    bool changedType();
    bool changedInfo();
protected:
    RawDataReader reader;
    double delta;

    // Current value of reader
    const RawValue::Data *reader_data;

    // Current value of this AverageReader
    epicsTime end_of_bin;
    DbrType type;
    DbrCount count;
    CtrlInfo info;
    bool type_changed;
    bool ctrl_info_changed;
    RawValue::Data *data;
};

/// @}

#endif
