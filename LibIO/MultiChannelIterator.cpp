// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiArchive.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"

BEGIN_NAMESPACE_CHANARCH

MultiChannelIterator::MultiChannelIterator (const MultiArchive *archive)
	: _channel (this)
{
	_is_valid = false;
	_multi_archive = archive;
	_channel_index = 0;
	_base_archive = 0;
	_base_channel_iterator = 0;
}

MultiChannelIterator::~MultiChannelIterator ()
{
	clear ();
}

bool MultiChannelIterator::isValid () const
{
	return _is_valid;
}

ChannelI *MultiChannelIterator::getChannel ()
{
	return & _channel;
}

bool MultiChannelIterator::next ()
{
	++_channel_index;
	_is_valid = _multi_archive->getChannel (_channel_index, *this);
	return _is_valid;
}

void MultiChannelIterator::clear ()
{
	_is_valid = false;
	if (_base_channel_iterator)
	{
		delete _base_channel_iterator;
		_base_channel_iterator = 0;
	}
	if (_base_archive)
	{
		delete _base_archive;
		_base_archive = 0;
	}
}

void MultiChannelIterator::position (size_t index, ArchiveI *archive, ChannelIteratorI *channel_iterator)
{
	clear ();
	_channel_index = index;
	_base_archive = archive;
	_base_channel_iterator = channel_iterator;
	_is_valid = _base_channel_iterator->isValid ();
}

// For current channel, position MultiValueIterator on value on or after given time
bool MultiChannelIterator::getValueAfterTime (const osiTime &time, MultiValueIterator &value_iterator)
{
	return _multi_archive->getValueAfterTime (_channel_index, *this, time, value_iterator);
}

bool MultiChannelIterator::getNextValue (MultiValueIterator &value_iterator)
{
    if (_is_valid && _base_channel_iterator && _base_channel_iterator->isValid())
	{
		osiTime next_time = _base_channel_iterator->getChannel()->getLastTime ();
		return _multi_archive->getValueAfterTime (_channel_index, *this, next_time, value_iterator);
	}

	value_iterator.clear ();
	return false;  
}

bool MultiChannelIterator::getPrevValue (MultiValueIterator &value_iterator)
{
	// TODO: implement
	value_iterator.clear ();
	return false;
}

END_NAMESPACE_CHANARCH

