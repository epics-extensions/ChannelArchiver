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
/// Let's call the time range from from of these multiples
/// of delta to the next one "time slots".
///
/// Different methods are used:
/// Linear interpolation (hence the name),
/// Averaging or Staircase interpolation.
/// - If there was only one scalar value of type double,
///   long, int or short, in the current time slot,
///   linear interpolation is used to determine
///   the approximate value at the end of the time slot.
/// - Averaging is used if several values fall into the
///   current time slot and the data type allows averaging,
///   i.e. it's a scalar double, long, int or short.
/// - Staircase interpolation is used for arrays or
///   scalars of type string, char or enumerated,
///   that is for everything that doesn't allow linear
///   interpolation nor averaging.
///   While one could handle certain arrays, we don't
///   because for one that's expensive and in addition
///   EPICS arrays often contain more than just waveform
///   elements: They're used to transfer structures.
///   "Staircase" means that the last value before
///   a time slot is extrapolated onto the time slot.
///   That's usually valid because we either sampled
///   a slow changing channel, so the previous value
///   is still good enough; or we archived on change
///   and no change means the previous data is still valid.
class LinearReader : public DataReader
{
public:
    /// Create a reader for an index.
    LinearReader(IndexFile &index, double delta);
    ~LinearReader();
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start);
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

/// @}

#endif
