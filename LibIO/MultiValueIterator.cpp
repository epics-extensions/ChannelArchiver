// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiValueIterator.h"
#include "MultiChannelIterator.h"

MultiValueIterator::MultiValueIterator ()
{
	_is_valid = false;
	_channel_iterator = 0;
	_base_value_iterator = 0;
}

MultiValueIterator::~MultiValueIterator ()
{
	clear ();
}

bool MultiValueIterator::isValid () const
{
	return _is_valid;
}

const ValueI * MultiValueIterator::getValue () const
{
	return _base_value_iterator->getValue ();
}

bool MultiValueIterator::next ()
{
	if (_base_value_iterator->next ())
	{
		_is_valid = true;
		return true;
	}
	_is_valid =  _channel_iterator->getNextValue (*this);

	return _is_valid;
}

bool MultiValueIterator::prev ()
{
    if (_base_value_iterator->prev ())
	{
		_is_valid = true;
		return true;
	}
	_is_valid = _channel_iterator->getPrevValue (*this);

	return _is_valid;
}     

size_t MultiValueIterator::determineChunk (const osiTime &until)
{
	return _base_value_iterator->determineChunk (until);
}

double MultiValueIterator::getPeriod () const
{
	return _base_value_iterator->getPeriod ();
}

// To be called by MultiArchive classes only:
void MultiValueIterator::clear ()
{
	_is_valid = false;
	if (_base_value_iterator)
	{
		delete _base_value_iterator;
		_base_value_iterator = 0;
	}
}

void MultiValueIterator::position (MultiChannelIterator *channel, ValueIteratorI *value)
{
	clear ();
	_channel_iterator = channel;
	_base_value_iterator = value;
	_is_valid = _base_value_iterator && _base_value_iterator->isValid();
}

