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
{
	_is_valid = false;
	_multi_archive = archive;
	_channel_index = 0;
	_base_archive = 0;
	_base_channel_iterator = 0;
	_channel.setMultiChannelIterator (this);
	_regex = 0;
}

MultiChannelIterator::~MultiChannelIterator ()
{
	clear ();
	if (_regex)
	{
		_regex->release ();
		_regex = 0;
	}    
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
	do
	{
		++_channel_index;
		_is_valid = _multi_archive->getChannel (_channel_index, *this);
	}
	while (_is_valid && _regex && _regex->doesMatch (_channel.getName())==false);

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
	// keep _regex because clear is also
	// called in the course of position(),
	// where ArchiveI and ChannelIteratorI change,
	// but the matching behaviour should stay
}

bool MultiChannelIterator::moveToMatchingChannel (const stdString &pattern)
{
	if (_regex)
		_regex->release ();
	_regex = RegularExpression::reference (pattern.c_str());

	if (_is_valid && _regex && _regex->doesMatch (_channel.getName())==false)
		return next ();

	return _is_valid;
}

void MultiChannelIterator::position (size_t index, ArchiveI *archive, ChannelIteratorI *channel_iterator)
{
	clear ();
	_channel_index = index;
	_base_archive = archive;
	_base_channel_iterator = channel_iterator;
	_is_valid = _base_channel_iterator->isValid ();
}

bool MultiChannelIterator::getNextValue (MultiValueIterator &value_iterator)
{
    if (_is_valid && _base_channel_iterator && _base_channel_iterator->isValid())
	{
		// last time stamp current archive has for this channel:
		osiTime next_time = _base_channel_iterator->getChannel()->getLastTime ();
		return _multi_archive->getValueAtOrAfterTime (_channel_index, *this,
			next_time, true /* has to be later */, value_iterator);
	}

	return false;  
}

bool MultiChannelIterator::getPrevValue (MultiValueIterator &value_iterator)
{
    if (_is_valid && _base_channel_iterator && _base_channel_iterator->isValid())
	{
		// first time stamp current archive has for this channel:
		osiTime next_time = _base_channel_iterator->getChannel()->getFirstTime ();
		return _multi_archive->getValueAtOrBeforeTime (_channel_index, *this,
			next_time, true /* has to be earlier */, value_iterator);
	}

	return false;
}

END_NAMESPACE_CHANARCH

