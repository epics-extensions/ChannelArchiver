// ValueI.h

#if !defined(_VALUEI_H_)
#define _VALUEI_H_

#include"ArchiveTypes.h"
#include"CtrlInfoI.h"
#include<db_access.h>

// Non-CA events to the archiver;
// some are archived - some are directives.
enum
{
    ARCH_NO_VALUE           = 0x0f00,
    ARCH_EST_REPEAT         = 0x0f80,
    ARCH_DISCONNECT         = 0x0f40,
    ARCH_STOPPED            = 0x0f20,
    ARCH_REPEAT             = 0x0f10,
    ARCH_DISABLED           = 0x0f08,
    ARCH_CHANGE_WRITE_FREQ  = 0x0f04,
    ARCH_CHANGE_PERIOD      = 0x0f02, // new period for single channel ARCH_CHANGE_FREQ
    ARCH_CHANGE_SIZE        = 0x0f01
};

//////////////////////////////////////////////////////////////////////
//CLASS RawValueI
//
// Lightweight wrapper class for a raw Type (dbr_time_xxx).
// All methods are static, they reqire the value ptr and a context hint
// of type/count.
//
// This class doesn't hold the actual memory location of the value,
// since it might be owned by e.g. the CA client library,
// it also doesn't know about it's Type.
// CLASS ValueI provides this for a single RawValue,
// the archive engine maintains circular buffers of several
// RawValues.
class RawValueI
{
public:
    //* Defined for future portability.
    // All values hold status, severity, time.
    // dbr_time_double has RISC pads
    // but it's used here because it's most common (my opinion)
    typedef dbr_time_double Type;

    //* Allocate/free space for num samples of type/count.
    static Type * allocate (DbrType type, DbrCount count, size_t num);
    static void free (Type *value);

    //* Calculate size of a single value of type/count
    static size_t getSize (DbrType type, DbrCount count);

    //* Compare the value part of two RawValues, not the time stamp or status!
    // (for full comparison, use memcmp over size given by getSize()).
    //
    // Both lhs, rhs must have the same type.
    static bool hasSameValue (DbrType type, DbrCount count, size_t size, const Type *lhs, const Type *rhs);

    //* Only valid for Values of same type:
    static void copy (DbrType type, DbrCount count, Type *lhs, const Type *rhs);

    //* Access to status...
    static short getStat (const Type *value);
    static short getSevr (const Type *value);
    static void getStatus (const Type *value, stdString &status);
    static void setStatus (Type *value, short status, short severity);

    static bool parseStatus (const stdString &text, short &stat, short &sevr);

    //* and time
    static const epicsTime getTime (const Type *value);
    static void setTime (Type *value, const epicsTime &stamp);
    static void getTime (const Type *value, stdString &time);
private:
    friend class ValueI;
    static Type * allocate (size_t size);
};

inline void RawValueI::copy(DbrType type, DbrCount count,
                            Type *lhs, const Type *rhs)
{   memcpy(lhs, rhs, getSize(type, count));   }

inline short RawValueI::getStat(const Type *value)
{ return value->status; }

inline short RawValueI::getSevr(const Type *value)
{ return value->severity; }

inline void RawValueI::setStatus(Type *value, short status, short severity)
{   value->status = status; value->severity = severity; }

inline const epicsTime RawValueI::getTime(const Type *value)
{   return epicsTime(value->stamp);  }

inline void RawValueI::setTime (Type *value, const epicsTime &stamp)
{   value->stamp = (epicsTimeStamp)stamp; }

//////////////////////////////////////////////////////////////////////
//CLASS ValueI
// Extends a CLASS RawValueI with type/count.
class ValueI
{
public:
    virtual ~ValueI ();

    //* Create/clone Value
    virtual ValueI *clone () const = 0;

    //* Get value as text.
    // (undefined if CtrlInfo is invalid)
    // Will get the whole array for getCount()>1.
    virtual void getValue (stdString &result) const = 0;

    virtual bool parseValue (const stdString &text) = 0;

    //* Access to CLASS CtrlInfoI : units, precision, limits, ...
    virtual const CtrlInfoI *getCtrlInfo () const = 0;
    virtual void setCtrlInfo (const CtrlInfoI *info);

    //* Get/set one array element as double
    virtual double getDouble (DbrCount index = 0) const = 0;
    virtual void setDouble (double value, DbrCount index = 0) = 0;

    //* Get time stamp
    const epicsTime getTime () const;
    void getTime (stdString &result) const;
    void setTime (const epicsTime &stamp);
    
    //* Get status as raw value or string
    dbr_short_t getStat () const;
    dbr_short_t getSevr () const;
    void getStatus (stdString &result) const;
    void setStatus (dbr_short_t status, dbr_short_t severity);

    bool parseStatus (const stdString &text);

    //* compare DbrType, DbrCount
    bool hasSameType (const ValueI &rhs) const;

    //* compare two Values (time stamp, status, severity, value)
    bool operator == (const ValueI &rhs) const;
    bool operator != (const ValueI &rhs) const;

    //* compare only the pure value (refer to operator ==),
    // not the time stamp or status
    bool hasSameValue (const ValueI &rhs) const;

    // Must only be used if hasSameType()==true,
    // will copy the raw value (value, status, time stamp)
    void copyValue (const ValueI &rhs);

    //* Does this Value hold Archiver status information
    // like disconnect and no 'real' value,
    // so calling e.g. getDouble() makes no sense?
    //
    // Repeat counts do hold a value, so these
    // are not recognized in here:
    bool isInfo () const;

    //* Access to contained RawValue:
    DbrType getType() const;
    void getType(stdString &type) const;
    DbrCount getCount() const;

    // Parse string for DbrType
    // (as written by getType())
    static bool parseType (const stdString &text, DbrType &type);

    void copyIn (const RawValueI::Type *data);
    void copyOut (RawValueI::Type *data) const;
    const RawValueI::Type *getRawValue () const;
    RawValueI::Type *getRawValue ();
    size_t getRawValueSize () const;

    virtual void show(FILE *f) const;

protected:
    // Hidden constuctor: Use Create!
    ValueI (DbrType type, DbrCount count);
    ValueI (const ValueI &); // not allowed
    ValueI &operator = (const ValueI &rhs); // not allowed

    DbrType         _type;      // type that _value holds
    DbrCount        _count;     // >1: value is array
    size_t          _size;      // ..to avoid calls to RawValue::getSize ()
    RawValueI::Type *_value;
};

inline bool ValueI::hasSameType (const ValueI &rhs) const
{   return _type == rhs._type && _count == rhs._count; }

inline void ValueI::copyValue (const ValueI &rhs)
{
    LOG_ASSERT (hasSameType (rhs));
    memcpy (_value, rhs._value, _size);
}

inline bool ValueI::operator == (const ValueI &rhs) const
{   return hasSameType (rhs) && memcmp (_value, rhs._value, _size) == 0; }

inline bool ValueI::operator != (const ValueI &rhs) const
{   return ! (hasSameType (rhs) && memcmp (_value, rhs._value, _size) == 0); }

inline bool ValueI::hasSameValue (const ValueI &rhs) const
{   return hasSameType (rhs) && RawValueI::hasSameValue (_type, _count, _size, _value, rhs._value); }

inline dbr_short_t ValueI::getStat () const
{   return RawValueI::getStat (_value); }

inline dbr_short_t ValueI::getSevr () const
{   return RawValueI::getSevr (_value); }

inline void ValueI::setStatus (dbr_short_t status, dbr_short_t severity)
{   RawValueI::setStatus (_value, status, severity); }

inline void ValueI::getStatus (stdString &result) const
{   RawValueI::getStatus (_value, result);  }

inline bool ValueI::isInfo () const
{
    dbr_short_t s = getSevr();
    return  s==ARCH_DISCONNECT || s==ARCH_STOPPED || s==ARCH_DISABLED;
}

inline const epicsTime ValueI::getTime () const
{   return RawValueI::getTime (_value); }

inline void ValueI::setTime (const epicsTime &stamp)
{   RawValueI::setTime (_value, stamp); }

inline void ValueI::getTime (stdString &result) const
{   RawValueI::getTime (_value, result);    }

inline DbrType ValueI::getType () const
{   return _type;   }

inline DbrCount ValueI::getCount () const
{   return _count;  }

inline void ValueI::copyIn (const RawValueI::Type *data)
{   memcpy (_value, data, _size);   }

inline void ValueI::copyOut (RawValueI::Type *data) const
{   memcpy (data, _value, _size);   }

inline const RawValueI::Type *ValueI::getRawValue () const
{   return _value; }

inline RawValueI::Type *ValueI::getRawValue ()
{   return _value; }

inline size_t ValueI::getRawValueSize () const
{   return _size;   }

#endif // !defined(_VALUEI_H_)
