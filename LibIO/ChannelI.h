// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __CHANNELI_H__
#define __CHANNELI_H__

#include "ValueIteratorI.h"

class ChannelIInfo
{
public:
	stdString	_name;			// name of the channel
	osiTime		_first_time;	// Time stamp of first value
	osiTime		_last_time;		// Time stamp of last value
};

//////////////////////////////////////////////////////////////////////
//CLASS ChannelI
//
// The Channel interface gives access to the name and basic statistical
// information for an archived channel.
//
// The Channel does not know about the type of
// archived values since those may in fact
// change.
class ChannelI
{
public:
	virtual ~ChannelI ();

	//* Name of this channel
	virtual const char *getName () const = 0;

	//* Time stamp of first value
	virtual osiTime getFirstTime ()  const = 0;

	//* Time stamp of last value
	virtual osiTime getLastTime ()   const = 0;

	//* Get the above information at once by filling a ChannelIInfo structure
	virtual void getChannelInfo (ChannelIInfo &info) const;

	//* Move CLASS ValueIterator for current Channel
	// to first, last, ... value
	bool getFirstValue (ValueIterator &values);
	bool getLastValue (ValueIterator &values);
	virtual bool getFirstValue (ValueIteratorI *values) = 0;
	virtual bool getLastValue (ValueIteratorI *values) = 0;

	//* Get value stamped >= time. time==0 results in call to getFirstValue
	virtual bool getValueAfterTime (const osiTime &time, ValueIteratorI *values) = 0;
	bool getValueAfterTime (const osiTime &time, ValueIterator &values);

	//* Get value stamped <= time
	virtual bool getValueBeforeTime (const osiTime &time, ValueIteratorI *values) = 0;
	bool getValueBeforeTime (const osiTime &time, ValueIterator &values);

	//* Get value stamped near time (whatever's next: before or after time)
	virtual bool getValueNearTime (const osiTime &time, ValueIteratorI *values) = 0;
	bool getValueNearTime (const osiTime &time, ValueIterator &values);

    // This is not copied into the HTML docs. because
    // I'm not certain about the interface:
    // A binary buffer has value type/count,
    // CtrlInfo and scan period.
    // So I could pass in
    // - ValueI *
    // - CtrlInfoI *
    // - period
    // but the ValueI already knows about the CtrlInfo,
    // so this is the interface.
    //
    // Alternative: only pass in ValueIteratorI.
    // But why would you require an iterator,
    // when in principle a single value and the other info
    // is enough?
    //
    // Tell me if you use this interface,
    // beware of possible changes!
	virtual size_t lockBuffer (const ValueI &value, double period);

	virtual void addBuffer (const ValueI &value_arg, double period, size_t value_count);

	// Add value to last buffer.
	// returns false when buffer is full
	virtual bool addValue (const ValueI &value) = 0;

	// Call after adding all values to that buffer
	virtual void releaseBuffer ();
};

inline bool ChannelI::getFirstValue (ValueIterator &values)
{ return getFirstValue (values.getI()); }

inline bool ChannelI::getLastValue (ValueIterator &values)
{ return getLastValue (values.getI()); }
	
inline bool ChannelI::getValueAfterTime (const osiTime &time, ValueIterator &values)
{ return getValueAfterTime (time, values.getI()); }

inline bool ChannelI::getValueBeforeTime (const osiTime &time, ValueIterator &values)
{ return getValueBeforeTime (time, values.getI()); }

inline bool ChannelI::getValueNearTime (const osiTime &time, ValueIterator &values)
{ return getValueNearTime (time, values.getI()); }

#endif //__CHANNELI_H__
