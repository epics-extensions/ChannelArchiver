// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTICHANNEL_H__
#define __MULTICHANNEL_H__

#include "ChannelI.h"

class MultiChannelIterator;

//////////////////////////////////////////////////////////////////////
//CLASS MultiChannel
//
// Implementation of CLASS ChannelI interface for the CLASS MultiArchive
class MultiChannel : public ChannelI
{
public:
	MultiChannel (MultiChannelIterator *channel_iterator);

	virtual const char *getName() const;
	virtual osiTime getFirstTime()  const;
	virtual osiTime getLastTime()   const;

	virtual bool getFirstValue(ValueIteratorI *values);
	virtual bool getLastValue(ValueIteratorI *values);
	virtual bool getValueAfterTime(const osiTime &time,
                                   ValueIteratorI *values);
	virtual bool getValueBeforeTime(const osiTime &time,
                                    ValueIteratorI *values);
	virtual bool getValueNearTime(const osiTime &time,
                                  ValueIteratorI *values);

	virtual size_t lockBuffer(const ValueI &value, double period);
	virtual void addBuffer(const ValueI &value_arg, double period, size_t value_count);
	virtual bool addValue(const ValueI &value);
	virtual void releaseBuffer();

private:
	// Implementation:
	// MultiChannel is part of the MCIterator who handles the archive access
	mutable MultiChannelIterator *_channel_iterator; // ptr back to owner
};

#endif // __MULTICHANNEL_H__
