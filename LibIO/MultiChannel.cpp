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

MultiChannel::MultiChannel(MultiChannelIterator *channel_iterator)
{
	_channel_iterator = channel_iterator;
}

// MultiChannelIterator has current channel index,
// MultiArchive has list of channels with info
// Name is always valid,
// rest only after querying all subarchives.
const char *MultiChannel::getName() const
{
	return _channel_iterator->_multi_archive->_channels[
        _channel_iterator->_channel_index]._name.c_str();
}

osiTime MultiChannel::getFirstTime() const
{
    _channel_iterator->_multi_archive->queryAllArchives();
	return _channel_iterator->_multi_archive->_channels[
        _channel_iterator->_channel_index]._first_time;
}

osiTime MultiChannel::getLastTime() const
{
    _channel_iterator->_multi_archive->queryAllArchives();
	return _channel_iterator->_multi_archive->_channels[
        _channel_iterator->_channel_index]._last_time;
}

bool MultiChannel::getFirstValue(ValueIteratorI *values)
{
	return getValueAfterTime(getFirstTime(), values);
}

bool MultiChannel::getLastValue(ValueIteratorI *values)
{
	return getValueAfterTime(getLastTime(), values);
}

bool MultiChannel::getValueAfterTime(const osiTime &time,
                                     ValueIteratorI *values)
{
	MultiValueIterator *multi_values =
        dynamic_cast<MultiValueIterator *>(values);

	if (_channel_iterator->_multi_archive->getValueAtOrAfterTime(
        *_channel_iterator, time, true /* == is ok */,
        *multi_values))
        return true;
    multi_values->clear();
    return false;
}

bool MultiChannel::getValueBeforeTime(const osiTime &time,
                                      ValueIteratorI *values)
{
	MultiValueIterator *multi_values =
        dynamic_cast<MultiValueIterator *>(values);

	if (_channel_iterator->_multi_archive->getValueAtOrBeforeTime(
        *_channel_iterator, time, true /* == is OK */,
        *multi_values))
        return true;
    multi_values->clear();
    return false;
}

bool MultiChannel::getValueNearTime(const osiTime &time,
                                    ValueIteratorI *values)
{
	MultiValueIterator *multi_values =
        dynamic_cast<MultiValueIterator *>(values);
    
	if (_channel_iterator->_multi_archive->getValueNearTime
        (*_channel_iterator, time, *multi_values))
        return true;
    multi_values->clear();
    return false;
}

size_t MultiChannel::lockBuffer(const ValueI &value, double period)
{
	throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
	return 0;
}

void MultiChannel::addBuffer(const ValueI &value_arg,
                             double period, size_t value_count)
{
	throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
}

bool MultiChannel::addValue(const ValueI &value)
{
	throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
	return false;
}

void MultiChannel::releaseBuffer()
{
	throwDetailedArchiveException(Invalid,
                                  "Cannot write, MultiArchive is read-only");
}

