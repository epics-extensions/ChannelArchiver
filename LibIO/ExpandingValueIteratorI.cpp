#include "ExpandingValueIteratorI.h"
#include "ArchiveException.h"

BEGIN_NAMESPACE_CHANARCH

static inline bool isRepeat (const ValueI *value)
{ return value->getSevr() == ARCH_EST_REPEAT  ||
		 value->getSevr() == ARCH_REPEAT;
}

// This iterator will move the _base iterator
// until that one hits a repeat count.
// In that case a _repeat_value will be cloned
// and it's time stamp is patched.
ExpandingValueIteratorI::ExpandingValueIteratorI (ValueIterator &base)
{
	_repeat_value = 0;
	attach (base.getI());
}

ExpandingValueIteratorI::ExpandingValueIteratorI (ValueIteratorI *base)
{
	_repeat_value = 0;
	attach (base);
}

void ExpandingValueIteratorI::attach (ValueIteratorI *base)
{
	delete _repeat_value;
	_repeat_value = 0;
	_base = base;
	if (_base && _base->isValid())
	{
		// Special case:
		// The initial value is a repeat count already.
		// Don't know how to adjust the time stamp here,
		// so keep it and simply remove the repeat tag:
		const ValueI *value = _base->getValue();
		if (value && isRepeat (value))
		{
			_repeat_value = value->clone ();
			_repeat_value->setStatus (0, 0);
		}
	}
}

ExpandingValueIteratorI::~ExpandingValueIteratorI ()
{
	attach (0);
}

bool ExpandingValueIteratorI::isValid () const
{	return _base && _base->isValid (); }

const ValueI * ExpandingValueIteratorI::getValue () const
{	return _repeat_value ? _repeat_value : _base->getValue ();	}

double ExpandingValueIteratorI::getPeriod () const
{	return _base->getPeriod ();	}

bool ExpandingValueIteratorI::next ()
{
	bool had_useful_value = false;

	if (_repeat_value)
	{
		// expanding repeat count:
		osiTime time (_repeat_value->getTime());
		time += _base->getPeriod ();
		if (time > _base->getValue()->getTime()) // end of repetitions
		{
			delete _repeat_value;
			_repeat_value = 0;
		}
		else
		{	// return next repeated value
			_repeat_value->setTime (time);
			return true;
		}
	}

	const ValueI *value = _base->getValue();
	if (value)
	{
		// Will not expand repetitions after disconnect etc.:
		switch (value->getSevr())
		{
		case ARCH_DISCONNECT:
		case ARCH_STOPPED:
		case ARCH_DISABLED:
			break;
		default:
			had_useful_value = true;
		}
	}
	// Get next ordinary Value
	if (!_base->next ())
		return false;

	value = _base->getValue();
	if (isRepeat (value))
	{	// Do we know how to build the repeated Values?
		if (had_useful_value  &&  getPeriod () > 0.0)
		{
			_repeat_value = value->clone();
			// calculate first repetition
			osiTime time (_repeat_value->getTime());
			time -= _repeat_value->getStat() * getPeriod ();
			_repeat_value->setTime (time);
			_repeat_value->setStatus (0, 0);
		}
		else // Skip repeats that cannot be expanded
			return next (); 
	}

	return true;
}

bool ExpandingValueIteratorI::prev ()
{
	if (_repeat_value)
	{	// expanding repeat count:
		osiTime time (_repeat_value->getTime());
		time -= getPeriod ();

		// _repeat_value->getTime() is the last time stamp,
		// check when repetition started:
		osiTime start (_base->getValue()->getTime());
		start -= _base->getValue()->getStat() * getPeriod ();
		if (time >= start) // end of repetitions
		{	// return previous repeated value
			_repeat_value->setTime (time);
			return true;
		}
		delete _repeat_value;
		_repeat_value = 0;
	}

	// Get prev. ordinary Value
	if (! _base->prev ())
		return false;

	const ValueI *value = _base->getValue ();
	if (isRepeat (value))
	{
		// Does this value follow a DISCONNECT or
		// another non-expandable event?
		_base->prev ();
		if (_base->getValue()->isInfo ())
			return true; // return _base's info value
		_base->next (); // undo peek back

		// Do we know how to build the repeated Values?
		if (getPeriod () == 0.0)
			return prev (); 
		_repeat_value = _base->getValue()->clone();
		_repeat_value->setStatus (0, 0);
	}

	return true;
}

size_t ExpandingValueIteratorI::determineChunk (const osiTime &until)
{
	LOG_MSG ("Warning: ExpandingValueIteratorI::determineChunk called, will give base's return\n");
	if (! _base)
		return 0;
	return _base->determineChunk (until);
}

END_NAMESPACE_CHANARCH
