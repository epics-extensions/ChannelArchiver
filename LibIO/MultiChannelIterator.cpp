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

BEGIN_NAMESPACE_CHANARCH

MultiChannelIterator::MultiChannelIterator (const MultiArchive *archive)
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
	return _base_channel_iterator->getChannel ();
}

bool MultiChannelIterator::next ()
{
	++_channel_index;
	_is_valid = _multi_archive->getChannel (_channel_index, this);
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

void MultiChannelIterator::position (ArchiveI *archive, ChannelIteratorI *channel_iterator)
{
	clear ();
	_base_archive = archive;
	_base_channel_iterator = channel_iterator;
	_is_valid = _base_channel_iterator->isValid ();
}

END_NAMESPACE_CHANARCH

