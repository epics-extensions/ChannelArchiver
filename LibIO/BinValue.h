// BinValue.h

#if !defined(_BINVALUE_H_)
#define _BINVALUE_H_

#include "BinTypes.h"
#include "ValueI.h"
#include "BinCtrlInfo.h"

BEGIN_NAMESPACE_CHANARCH

//////////////////////////////////////////////////////////////////////
//CLASS BinRawValue
class BinRawValue : public RawValueI 
{
public:
	// size: pre-calculated from type, count
	static void read  (DbrType type, DbrCount count, size_t size, Type *value, FILE *fd, FileOffset offset);
	// write requires a buffer for the memory-to-disk format conversions
	static void write (DbrType type, DbrCount count, size_t size, const Type *value,
		MemoryBuffer<dbr_time_string> &cvt_buffer, FILE *fd, FileOffset offset);
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
	
	ValueI *clone () const;

	void setCtrlInfo (const CtrlInfoI *info);
	const CtrlInfoI *getCtrlInfo () const;
	
	// Read/write & convert single value/array
	void read (FILE *fd, FileOffset offset);
	void write (FILE *fd, FileOffset offset) const;

	void show (ostream &o) const;

protected:
	BinValue (DbrType type, DbrCount count);
	BinValue (const BinValue &); // not allowed
	BinValue &operator = (const BinValue &rhs);

	const CtrlInfoI	*_ctrl_info;// precision, units, limits, ...
	mutable MemoryBuffer<dbr_time_string> _write_buffer;
};

inline void BinValue::setCtrlInfo (const CtrlInfoI *info)
{	_ctrl_info = info;	}

inline const CtrlInfoI *BinValue::getCtrlInfo () const
{	return _ctrl_info;	}

inline void BinValue::read (FILE *fd, FileOffset offset)
{	BinRawValue::read (_type, _count, _size, _value, fd, offset);	}

inline void BinValue::write (FILE *fd, FileOffset offset) const
{	BinRawValue::write (_type, _count, _size, _value, _write_buffer, fd, offset);	}

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

END_NAMESPACE_CHANARCH

#endif // !defined(_VALUE_H_)
