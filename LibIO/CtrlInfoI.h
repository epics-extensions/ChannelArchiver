#ifndef __CTRLINFOI_H__
#define __CTRLINFOI_H__

#include "ArchiveTypes.h"
#include "MemoryBuffer.h"
#include <stdio.h>

// ! Binary layout of CtrlInfo must be maintained !
class NumericInfo
{
public:
	float	disp_high;	// high display range
	float	disp_low;	// low display range
	float	low_warn;
	float	low_alarm;
	float	high_warn;
	float	high_alarm;
	long	prec;		// display precision
	char	units[1];	// actually as long as needed,
};

class EnumeratedInfo
{
public:
	short	num_states;		// state_strings holds num_states strings
	short	pad;			// one after the other, separated by '\0'
	char	state_strings[1];
};

// Info::size includes the "size" and "type" field.
// The original archiver read/wrote "Info" that way,
// but didn't properly inilialize it:
// size excluded size/type and was then rounded up by 8 bytes... ?!
class CtrlInfoData
{
public:
	unsigned short	size;
	unsigned short	type;
	union
	{
		NumericInfo		analog;
		EnumeratedInfo	index;
	}				value;
	// class will be as long as necessary
	// to hold the units or all the state_strings
};

//////////////////////////////////////////////////////////////////////
//CLASS CtrlInfoI
// A value is archived with control information.
// Several values might share the same control information
// for efficiency.
//
// The control information is variable in size
// because it holds units or state strings.
class CtrlInfoI
{
public:
	CtrlInfoI();
	CtrlInfoI(const CtrlInfoI &rhs);
	CtrlInfoI& operator = (const CtrlInfoI &rhs);
	virtual ~CtrlInfoI ();

	bool operator == (const CtrlInfoI &rhs) const;
	bool operator != (const CtrlInfoI &rhs) const
	{	return ! (*this == rhs);	}

	//* Type of Value:
	typedef enum
	{
		Invalid = 0,
		Numeric = 1,
		Enumerated = 2
	}	Type;
	Type getType() const;

	//* Read Control Information:
	// Numeric precision, units,
	// high/low limits for display etc.:
	long getPrecision() const;
	const char *getUnits() const;
	float getDisplayHigh() const;
	float getDisplayLow() const;
	float getHighAlarm() const;
	float getHighWarning() const;
	float getLowWarning() const;
	float getLowAlarm() const;

	//* Initialize a Numeric CtrlInfo
	// (sets Type to Numeric and then sets fields)
	void setNumeric(long prec, const stdString &units,
					float disp_low, float disp_high,
					float low_alarm, float low_warn,
                    float high_warn, float high_alarm);

	//* Initialize an Enumerated CtrlInfo
	void setEnumerated(size_t num_states, char *strings[]);

	// Alternative to setEnumerated:
	// Call with total string length, including all the '\0's !
	void allocEnumerated(size_t num_states, size_t string_len);

	// Must be called after allocEnumerated()
	// AND must be called in sequence,
	// i.e. setEnumeratedString (0, ..
	//      setEnumeratedString (1, ..
	void setEnumeratedString(size_t state, const char *string);

	// After allocEnumerated() and a sequence of setEnumeratedString ()
	// calls, this method recalcs the total size
	// and checks if the buffer is sufficient (Debug version only)
	void calcEnumeratedSize();

	//* Format a double value according to precision<BR>
	// Throws Invalid if CtrlInfo is not for Numeric
	void formatDouble(double value, stdString &result) const;

	//* Enumerated: state string
	size_t getNumStates() const;
	void getState(size_t state, stdString &result) const;

	// Like strtod, strtol: try to parse,
	// position 'next' on character following the recognized state text
	bool parseState(const char *text, const char **next, size_t &state) const;

    void show(FILE *f) const;

protected:
	const char *getState(size_t state, size_t &len) const;

	MemoryBuffer<CtrlInfoData>	_infobuf;
};

inline CtrlInfoI::Type CtrlInfoI::getType() const
{	return (CtrlInfoI::Type) (_infobuf.mem()->type);}

inline long CtrlInfoI::getPrecision() const
{
	return (getType() == Numeric) ? _infobuf.mem()->value.analog.prec : 0;
}

inline const char *CtrlInfoI::getUnits() const
{
    if (getType() == Numeric)
        return _infobuf.mem()->value.analog.units;
    return "";
}

inline float CtrlInfoI::getDisplayHigh() const
{	return _infobuf.mem()->value.analog.disp_high; }

inline float CtrlInfoI::getDisplayLow() const
{	return _infobuf.mem()->value.analog.disp_low; }

inline float CtrlInfoI::getHighAlarm() const
{	return _infobuf.mem()->value.analog.high_alarm; }

inline float CtrlInfoI::getHighWarning() const
{	return _infobuf.mem()->value.analog.high_warn; }

inline float CtrlInfoI::getLowWarning() const
{	return _infobuf.mem()->value.analog.low_warn; }

inline float CtrlInfoI::getLowAlarm() const
{	return _infobuf.mem()->value.analog.low_alarm; }

inline size_t CtrlInfoI::getNumStates() const
{
	if (getType() == Enumerated)
		return _infobuf.mem()->value.index.num_states;
	return 0;
}

#endif //__CTRLINFOI_H__


