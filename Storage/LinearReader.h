// -*- c++ -*-

#ifndef __LINEAR_READER_H__
#define __LINEAR_READER_H__

#include "DataReader.h"

/// \ingroup Storage
/// @{

/// Reads data from storage w/ linear interpolation.

/// Based on a DataReader, the LinearReader interpolates
/// all values in a linear manner.
///
class LinearReader : public DataReader
{
public:
    /// Create a reader for an index.
    LinearReader(archiver_Index &index, double delta);

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
};

#endif
