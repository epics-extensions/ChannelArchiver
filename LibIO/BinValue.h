// BinValue.h

#if !defined(_BINVALUE_H_)
#define _BINVALUE_H_

#include "BinTypes.h"
#include "ValueI.h"
#include "BinCtrlInfo.h"
#include <iostream>

//////////////////////////////////////////////////////////////////////
//CLASS BinRawValue
class BinRawValue : public RawValueI 
{
public:
    // size: pre-calculated from type, count
    static void read  (DbrType type, DbrCount count, size_t size, Type *value,
                       LowLevelIO &file, FileOffset offset);
    // write requires a buffer for the memory-to-disk format conversions
    static void write (DbrType type, DbrCount count, size_t size, const Type *value,
                       MemoryBuffer<dbr_time_string> &cvt_buffer,
                       LowLevelIO &file, FileOffset offset);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValue
class BinValue : public ValueI
{
public:
    //* Create a value suitable for the given DbrType,
    // will return CLASS BinValueDbrDouble,
    // CLASS BinValueDbrEnum, ...
    static BinValue *create (DbrType type, DbrCount count);

	virtual ~BinValue();

    ValueI *clone () const;

    void setCtrlInfo (const CtrlInfoI *info);
    const CtrlInfoI *getCtrlInfo () const;
    
    // Read/write & convert single value/array
    void read (LowLevelIO &filefd, FileOffset offset);
    void write (LowLevelIO &filefd, FileOffset offset) const;

    void show (std::ostream &o) const;

protected:
    BinValue (DbrType type, DbrCount count);
    BinValue (const BinValue &); // not allowed
    BinValue &operator = (const BinValue &rhs);

    const CtrlInfoI *_ctrl_info;// precision, units, limits, ...
    mutable MemoryBuffer<dbr_time_string> _write_buffer;
};

inline void BinValue::read (LowLevelIO &file, FileOffset offset)
{
    BinRawValue::read (_type, _count, _size, _value, file, offset);
}

inline void BinValue::write (LowLevelIO &file, FileOffset offset) const
{
    BinRawValue::write (_type, _count, _size, _value, _write_buffer, file, offset);
}

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrShort
// Specialization of CLASS BinValue.
class BinValueDbrShort : public BinValue
{
public:
    BinValueDbrShort (DbrCount count) : BinValue (DBR_TIME_SHORT, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrLong
// Specialization of CLASS BinValue.
class BinValueDbrLong : public BinValue
{
public:
    BinValueDbrLong (DbrCount count) : BinValue (DBR_TIME_LONG, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrDouble
// Specialization of CLASS BinValue.
class BinValueDbrDouble : public BinValue
{
public:
    BinValueDbrDouble (DbrCount count) : BinValue (DBR_TIME_DOUBLE, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrFloat
// Specialization of CLASS BinValue.
class BinValueDbrFloat : public BinValue
{
public:
    BinValueDbrFloat (DbrCount count) : BinValue (DBR_TIME_FLOAT, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrEnum
// Specialization of CLASS BinValue.
class BinValueDbrEnum : public BinValue
{
public:
    BinValueDbrEnum (DbrCount count) : BinValue (DBR_TIME_ENUM, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrString
// Specialization of CLASS BinValue.
class BinValueDbrString : public BinValue
{
public:
    BinValueDbrString (DbrCount count) : BinValue (DBR_TIME_STRING, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

//////////////////////////////////////////////////////////////////////
//CLASS BinValueDbrChar
// Specialization of CLASS BinValue.
class BinValueDbrChar : public BinValue
{
public:
    BinValueDbrChar (DbrCount count) : BinValue (DBR_TIME_CHAR, count) {}
    //* Specialization of the pure virtual methods in CLASS BinValue:
    virtual double getDouble (DbrCount index) const;
    virtual void setDouble (double value, DbrCount index = 0);
    virtual void getValue (stdString &result) const;
    virtual bool parseValue (const stdString &text);
};

#endif // !defined(_VALUE_H_)
