// -*- c++ -*-

#ifndef __RAW_VALUE_H__
#define __RAW_VALUE_H__

#include <stdio.h>
#include <db_access.h>
#include "MemoryBuffer.h"
#include "StorageTypes.h"

/// \addtogroup Storage
/// @{

/// \typedef DbrType is used to hold dbr_time_xxx types.
typedef unsigned short DbrType;

/// \typedef DbrCount is used to hold the array size of CA channels.
typedef unsigned short DbrCount;

/// Non-CA events to the archiver;
/// some are archived - some are directives.
enum RawValueSpecialSeverities
{
    ARCH_NO_VALUE           = 0x0f00,
    ARCH_EST_REPEAT         = 0x0f80,
    ARCH_DISCONNECT         = 0x0f40,
    ARCH_STOPPED            = 0x0f20,
    ARCH_REPEAT             = 0x0f10,
    ARCH_DISABLED           = 0x0f08
};

/// Helper class for raw dbr_time_xxx values.

/// This class has all static methods, it always requires
/// a hint for type, count and maybe CtrlInfo
/// to properly handle the underlying value.
/// The class also doesn't hold the actual memory of the
/// value, since that might be in the argument of
/// a CA event callback etc.
class RawValue
{
public:
    /// Type for accessing the raw data and its common fields.
    /// (status, severity, time) w/o compiler warnings.
    /// Had to pick one of the dbr_time_xxx
    typedef dbr_time_double Data;

    /// Allocate/free space for num samples of type/count.
    static Data * allocate(DbrType type, DbrCount count, size_t num);
    static void free(Data *value);

    /// Calculate size of a single value of type/count
    static size_t getSize(DbrType type, DbrCount count);

    /// Compare the value part of two RawValues, not the time stamp or status!
    /// (for full comparison, use memcmp over size given by getSize()).
    ///
    /// Both lhs, rhs must have the same type.
    static bool hasSameValue(DbrType type, DbrCount count,
                             size_t size, const Data *lhs, const Data *rhs);

    /// Full copy (stat, time, value). Only valid for Values of same type.
    static void copy(DbrType type, DbrCount count, Data *lhs, const Data *rhs);

    /// Get status
    static short getStat(const Data *value);

    /// Get severity
    static short getSevr(const Data *value);

    /// Get status/severity as string
    static void getStatus(const Data *value, stdString &status);

    /// Does the severity represent one of the special ARCH_xxx values
    /// that does not carry any value
    static bool isInfo(const Data *value);

    /// Check the value to see if it's above zero.

    /// For numerics, that's obvious: value>0. Enums are treated like integers,
    /// strings are 'zero' if empty (zero length).
    /// Arrays are not really handled, we only consider the first element.
    static bool isAboveZero(DbrType type, const Data *value);
    
    /// Set status and severity
    static void setStatus(Data *value, short status, short severity);

    /// Parse stat/sevr from text
    static bool parseStatus(const stdString &text, short &stat, short &sevr);

    /// Get time stamp
    static const epicsTime getTime(const Data *value);

    /// Get time stamp as text
    static void getTime(const Data *value, stdString &time);

    /// Set time stamp
    static void setTime(Data *value, const epicsTime &stamp);

    /// Get data as a double or return false

    /// Works for scalar short, int, long, float, double
    ///
    ///
    static bool getDouble(DbrType type, DbrCount count,
                          const Data *value, double &d, int i=0);

    /// Get data as a long or return false

    /// Works for scalar enum, short, int, long, float, double
    ///
    ///
    static bool getLong(DbrType type, DbrCount count,
                        const Data *value, long &l, int i=0);

    /// Set data from a double or return false

    /// Works for scalar short, int, long, float, double
    ///
    ///
    static bool setDouble(DbrType type, DbrCount count,
                          Data *value, double d);
    
    /// Convert value to txt, using CtrlInfo if available.

    /// This gives only the value. Use getTime() and getStatus()
    /// for time and status.
    static void getValueString(stdString &txt,
                               DbrType type, DbrCount count, const Data *value,
                               const class CtrlInfo *info=0);

    /// Display value, using CtrlInfo if available.
    static void show(FILE *file,
                     DbrType type, DbrCount count, const Data *value,
                     const class CtrlInfo *info=0);
    
    /// Read a value from binary file.
    
    /// size: pre-calculated from type, count.
    ///
    ///
    static bool read(DbrType type, DbrCount count,
                     size_t size, Data *value,
                     class DataFile *datafile, FileOffset offset);
    
    /// Write a value to binary file/
    
    /// Requires a buffer for the memory-to-disk format conversions.
    ///
    ///
    static bool write(DbrType type, DbrCount count,
                      size_t size, const Data *value,
                      MemoryBuffer<dbr_time_string> &cvt_buffer,
                      class DataFile *datafile, FileOffset offset);
};

/// @}

inline void RawValue::copy(DbrType type, DbrCount count,
                           Data *lhs, const Data *rhs)
{
    memcpy(lhs, rhs, getSize(type, count));
}

inline short RawValue::getStat(const Data *value)
{ return value->status; }

inline short RawValue::getSevr(const Data *value)
{ return value->severity; }

inline bool RawValue::isInfo(const Data *value)
{
    short s = value->severity;
    return  s==ARCH_DISCONNECT || s==ARCH_STOPPED || s==ARCH_DISABLED;
}

inline void RawValue::setStatus(Data *value, short status, short severity)
{
    value->status = status;
    value->severity = severity;
}

inline const epicsTime RawValue::getTime(const Data *value)
{   return epicsTime(value->stamp);  }

inline void RawValue::setTime(Data *value, const epicsTime &stamp)
{   value->stamp = (epicsTimeStamp)stamp; }

#endif
