// -*- c++ -*-

#ifndef __LINEAR_READER_H__
#define __LINEAR_READER_H__

#include "AverageReader.h"

/// \addtogroup Storage
/// @{

/// Reads data from storage w/ linear interpolation.

/// The LinearReader is an implementation of a DataReader
/// that aligns data onto multiples of 'delta' seconds on the
/// time axis via linear interpolation.
class LinearReader : public AverageReader
{
public:
    /// Create a reader for an index.
    LinearReader(Index &index, double delta);
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start,
                               ErrorInfo &error_info);
    const RawValue::Data *next(ErrorInfo &error_info);
};

/// @}

#endif
