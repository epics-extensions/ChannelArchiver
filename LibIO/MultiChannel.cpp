// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ArchiveException.h"
#include "MultiChannel.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"

BEGIN_NAMESPACE_CHANARCH

MultiChannel::MultiChannel (MultiChannelIterator *owner)
{
	_channel_iterator = owner;
}

const char *MultiChannel::getName () const
{
	return _channel_iterator->getChannelInfo()._name.c_str();
}

osiTime MultiChannel::getFirstTime () const
{
	return _channel_iterator->getChannelInfo()._first_time;
}

osiTime MultiChannel::getLastTime () const
{
	return _channel_iterator->getChannelInfo()._last_time;
}

void MultiChannel::getChannelInfo (ChannelInfo &info) const
{
	info = _channel_iterator->getChannelInfo();
}

bool MultiChannel::getFirstValue (ValueIteratorI *values)
{
	return getValueAfterTime (getFirstTime(), values);
}

bool MultiChannel::getLastValue (ValueIteratorI *values)
{
	return getValueAfterTime (getLastTime(), values);
}

bool MultiChannel::getValueAfterTime (const osiTime &time, ValueIteratorI *values)
{
	MultiValueIterator *multi_values = dynamic_cast<MultiValueIterator *> (values);
	return _channel_iterator->getValueAfterTime (time, *multi_values);
}

bool MultiChannel::getValueBeforeTime (const osiTime &time, ValueIteratorI *values)
{
	// TODO: implement this
	return false;
}

bool MultiChannel::getValueNearTime (const osiTime &time, ValueIteratorI *values)
{
	// TODO: implement this
	return false;
}

size_t MultiChannel::lockBuffer (const ValueI &value, double period)
{
	throwDetailedArchiveException (Invalid, "Cannot write, MultiArchive is read-only");
	return 0;
}

void MultiChannel::addBuffer (const ValueI &value_arg, double period, size_t value_count)
{
	throwDetailedArchiveException (Invalid, "Cannot write, MultiArchive is read-only");
}

bool MultiChannel::addValue (const ValueI &value)
{
	throwDetailedArchiveException (Invalid, "Cannot write, MultiArchive is read-only");
	return false;
}

void MultiChannel::releaseBuffer ()
{
	throwDetailedArchiveException (Invalid, "Cannot write, MultiArchive is read-only");
}

END_NAMESPACE_CHANARCH

