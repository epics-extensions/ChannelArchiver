// ChannelInfo.cpp

#include "Engine.h"
#include "fdManager.h"
#include "Histogram.h"
#include <math.h>
#include <iostream>

using namespace std;
USING_NAMESPACE_CHANARCH

// Helper:
// print chid
//
static ostream & operator << (ostream &o, const chid &chid)
{
	short typ = ca_field_type(chid);
	const char *type_txt = (typ > 0  &&  typ < dbr_text_dim) ? dbr_text[typ] : dbr_text_invalid;

	o << "CHID: " << ca_name(chid) << "\n";
	o << "\ttype/count: " << type_txt << "(" << typ << ") / " << ca_element_count(chid) << "\n";
	o << "\thost: " << ca_host_name(chid) << "\n";
	o << "\tuser ptr: " << ca_puser(chid) << "\n";
	switch (ca_state(chid))
	{
	case cs_never_conn:	o << "\tcs_never_conn: valid chid, IOC not found\n";                  break;
	case cs_prev_conn:	o << "\tcs_prev_conn   valid chid, IOC was found, but unavailable\n"; break;
	case cs_conn:		o << "\tcs_conn:       valid chid, IOC was found, still available\n"; break;
	case cs_closed:		o << "\tcs_closed:     invalid chid\n";                               break;
	default:			o << "\t<undefined ca_state: " << ca_state(chid) << ">\n";
	}

	return o;
}

// Locking:
// 
// The ChannelInfo list and the Circ. Buffers in there are
// locked for multithreading.

ChannelInfo::ChannelInfo ()
{
	_currently_disabling = false;
	_disabled = 0;
	_monitored = false;
	_connected = false;
	_get_mechanism = none;
	_vals_per_buffer = INIT_VALS_PER_BUF;
	_period = 1;			// Default scan period
	_chid = 0;
	
	_new_value_set = false;
	_new_value = 0;
	_pending_value_set = false;
	_pending_value = 0;
	_previous_value_set = false;
	_previous_value = 0;
	_tmp_value = 0;
	_write_value = 0;

#	ifdef CA_STATISTICS
	_next_CA_value = -1;
#	endif

	_had_null_time = false;
}

ChannelInfo::~ChannelInfo ()
{
	_write_lock.take ();
	delete _write_value;
	_write_lock.give ();
	delete _tmp_value;
	delete _previous_value;
	delete _pending_value;
	delete _new_value;
}

void ChannelInfo::setPeriod (double sample)
{
	LOG_ASSERT (sample > 0.0);
	_period = sample;
	checkRingBuffer ();
}

// Mark channel as belonging to 'group', maybe disabling that group.
// It's OK to call this method again with the same group.
void ChannelInfo::addToGroup (GroupInfo *group, bool disabling)
{
	// bit in _disabling indicates if we could disable that group
	_disabling.grow (group->getID() + 1);
	_disabling.set (group->getID(), disabling);
	
	// Is Channel already in group?
	list<GroupInfo *>::iterator i;
	for (i=_groups.begin(); i!=_groups.end(); ++i)
		if (*i == group)
			return;
	
	_groups.push_back (group);
}

void ChannelInfo::startCaConnection (bool new_channel)
{
	if (new_channel)
	{
		if (ca_search_and_connect (_name.c_str(), &_chid,
				caLinkConnectionHandler, this)  != ECA_NORMAL)
		{
			LOG_MSG ("ca_search_and_connect (" << _name << ") failed\n");
		}
	}
	else
	{
		// re-get control information for this channel as in caLinkConnectionHandler
		if (_new_value && _connected)
		{
			int status = ca_array_get_callback (_new_value->getType ()-DBR_TIME_STRING+DBR_CTRL_STRING, 1,
				_chid, caControlHandler, this);
			if (status != ECA_NORMAL)
			{
				LOG_MSG ("CA ca_array_get_callback ('" << getName () << "') failed in startCaConnection:"
					<< ca_message (status) << "\n");
				return;
			}
		}
	}
}

// CA Callback for each channel that connects or disconnects:
// * ca_search_and_connect calls this when a channel is found
// * CA also calls this routine when a connection is broken
void ChannelInfo::caLinkConnectionHandler (struct connection_handler_args arg)
{
	list<GroupInfo *>::iterator g;
	ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);
	bool was_connected = me->_connected;

	if (ca_state(arg.chid) != cs_conn)
	{
		LOG_MSG (osiTime::getCurrent() << ", " << me->getName () << ": CA disconnect\n");
		// Flush everything until the disconnect happened
		me->_connected = false;
		me->_new_value_set = false;
		me->_connectTime = osiTime::getCurrent ();
		me->flushRepeats (me->_connectTime);
		// add a disconnect event
		me->addEvent (0, ARCH_DISCONNECT, me->_connectTime);

		if (was_connected)
		{
			// Update statics in GroupInfo:
			for (g=me->_groups.begin(); g!=me->_groups.end(); ++g)
				(*g)->decConnectedChannels ();
		}
		return;
	}
	//LOG_MSG (osiTime::getCurrent() << ", " << me->getName () << ": CA connect\n");

	// get control information for this channel
	// TODO: This is only requested on connect - similar to the previous engine or DM.
	// How do we learn about changes, since you might actually change a channel
	// without rebooting an IOC?
	int status = ca_array_get_callback (ca_field_type(arg.chid)+DBR_CTRL_STRING, 1,
			me->_chid, caControlHandler, me);

	if (status != ECA_NORMAL)
	{
		LOG_MSG (osiTime::getCurrent() << ", " << me->getName ()
			<< ": ca_array_get_callback failed in caLinkConnectionHandler: "
			<< ca_message (status) << "\n");
	}
	// ChannelInfo is not really considered 'connected' until
	// we received the control information...
}

static bool setup_CtrlInfo (DbrType type, CtrlInfoI &info, const void *raw)
{
	switch (type)
	{
	case DBR_CTRL_DOUBLE:
		{
			struct dbr_ctrl_double *ctrl = (struct dbr_ctrl_double *)raw;
			info.setNumeric (ctrl->precision, ctrl->units,
				    ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
					ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
					ctrl->upper_warning_limit, ctrl->upper_alarm_limit);
		}
		return true;
	case DBR_CTRL_SHORT:
		{
		    struct dbr_ctrl_int *ctrl = (struct dbr_ctrl_int *)raw;
			info.setNumeric (0, ctrl->units,
				    ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
					ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
					ctrl->upper_warning_limit, ctrl->upper_alarm_limit);
		}
		return true;
	case DBR_CTRL_FLOAT:
		{
			struct dbr_ctrl_float *ctrl = (struct dbr_ctrl_float *)raw;
			info.setNumeric (ctrl->precision, ctrl->units,
				    ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
					ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
					ctrl->upper_warning_limit, ctrl->upper_alarm_limit);
		}
		return true;
	case DBR_CTRL_CHAR:
		{
			struct dbr_ctrl_char *ctrl = (struct dbr_ctrl_char *)raw;
			info.setNumeric (0, ctrl->units,
				    ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
					ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
					ctrl->upper_warning_limit, ctrl->upper_alarm_limit);
		}
		return true;
	case DBR_CTRL_LONG:
		{
			struct dbr_ctrl_long *ctrl = (struct dbr_ctrl_long *)raw;
			info.setNumeric (0, ctrl->units,
				    ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
					ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
					ctrl->upper_warning_limit, ctrl->upper_alarm_limit);
		}
		return true;
	case DBR_CTRL_ENUM:
		{
			size_t i;
			struct dbr_ctrl_enum *ctrl = (struct dbr_ctrl_enum *)raw;
			info.allocEnumerated (ctrl->no_str, MAX_ENUM_STATES*MAX_ENUM_STRING_SIZE);
			for (i=0; i<(size_t)ctrl->no_str; ++i)
				info.setEnumeratedString (i, ctrl->strs[i]);
			info.calcEnumeratedSize ();
	    }
		return true;
	case DBR_CTRL_STRING:	/* no control information */
		info.setEnumerated (0, 0);
		return true;
	}
	return false;
}

// Called by CA for new control information
void ChannelInfo::caControlHandler (struct event_handler_args arg)
{
	ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);
	bool was_connected = me->_connected;

	//LOG_MSG (osiTime::getCurrent() << ", " << me->getName () << ": CA control information\n");
	if (arg.status != ECA_NORMAL)
	{
		LOG_MSG (osiTime::getCurrent() << ", " << me->_name <<
				": Bad control information, CA status " << ca_message(arg.status) << "\n");
		LOG_MSG (me->_chid << "\n");
		if (me->_new_value  &&  me->_get_mechanism != none)
		{
			LOG_MSG ("...ignored since it seems to be a re-connect\n");
			me->_connected = true;
		}
	}
	else
	{
		if (setup_CtrlInfo (arg.type, me->_ctrl_info, arg.dbr))
			me->_connected = true;
		else
		{
			LOG_MSG ("ERROR: Unknown CA control information: " << me->getName () << endl);
			me->_connected = false;
		}
		me->_connectTime = osiTime::getCurrent ();

		DbrType dbr_type = ca_field_type(arg.chid)+DBR_TIME_STRING;
		DbrCount nelements = ca_element_count(arg.chid);
		me->setValueType (dbr_type, nelements); 	// size or type might have changed...

		// Already subscribed or first connection?
		if (me->_get_mechanism == none   ||
			(me->isMonitored() && me->_get_mechanism != use_monitor)  )
		{
			if (!me->isMonitored()  &&  theEngine->addToScanList (me))
				me->_get_mechanism = use_get;
			else
			{
				// Engine will not scan this: add a monitor for this channel
				if (ca_add_array_event (dbr_type, nelements, me->_chid,
					caEventHandler, me, 0.0, 0.0, 0.0, (evid *)0) != ECA_NORMAL)
				{
					LOG_MSG ("CA ca_add_array_event ('" << me->getName () << "') failed\n");
					return;
				}
				me->_get_mechanism = use_monitor;
			}
		}
	}

	if (!was_connected  &&  me->_connected)
	{
		list<GroupInfo *>::iterator g;
		for (g=me->_groups.begin(); g!=me->_groups.end(); ++g)
			(*g)->incConnectedChannels ();
	}
}

// Callback for values (monitored)
void ChannelInfo::caEventHandler (struct event_handler_args arg)
{
	ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);
	LOG_ASSERT (me);
	LOG_ASSERT (me->_new_value);

	me->_new_value->copyIn (reinterpret_cast<const RawValueI::Type *>(arg.dbr));
	//LOG_MSG (me->getName () << " : CA monitor      " << *me->_new_value << endl);
#	ifdef CA_STATISTICS
	double dv = me->_new_value->getDouble ();
	if (me->_next_CA_value < 0)
		me->_next_CA_value = dv;
	else if (me->_next_CA_value != dv)
	{
		++me->_missing_CA_values;
		me->_next_CA_value = dv;
	}
	++me->_next_CA_value;
	if (me->_next_CA_value > 10)
		me->_next_CA_value = 0;
#	endif

	me->handleNewValue ();
}

// Issue CA get for _scanned_value, no ca_pend_io in here!
void ChannelInfo::issueCaGet ()
{
	if (!_new_value)
	{
		LOG_MSG ("... no value description in issueCaGet!\n");
		return;
	}
	if (ca_array_get (_new_value->getType (), _new_value->getCount (), _chid, _new_value->getRawValue())
		!= ECA_NORMAL)
		LOG_MSG ("ca_array_get (" << _name << ") failed\n");
}

// Define the type of value for this ChannelInfo.
// Result: has the type changed ?
bool ChannelInfo::setValueType (DbrType type, DbrCount count)
{
	bool changed = false;

	if (_new_value)
	{
		if (_new_value->getType() == type  &&  _new_value->getCount() == count)
			return false; // same type

		delete _new_value;
		delete _previous_value;
		delete _tmp_value;
		_write_lock.take ();
		delete _write_value;
		_write_lock.give ();
		changed = true;
	}
	_new_value_set = false;
	_pending_value_set = false;
	_previous_value_set = false;

	_new_value = theEngine->newValue (type, count);
	_new_value->setCtrlInfo (&_ctrl_info);
	_pending_value = _new_value->clone ();
	_previous_value = _new_value->clone ();
	_tmp_value = _new_value->clone ();
	_write_lock.take ();
	_write_value = _new_value->clone ();
	_write_lock.give ();
	checkRingBuffer ();

	return changed;
}

// Check if Ring buffer is big enough, fits _new_value etc.
// Might be called from setWritePeriod() before _value is available...
void ChannelInfo::checkRingBuffer ()
{
	if (_new_value)
		_buffer.allocate (_new_value->getType(), _new_value->getCount(), _period);
}

void ChannelInfo::addToRingBuffer (const ValueI *value)
{
	if (! value)
	{
		LOG_ASSERT (value);
		return;
	}

	size_t ow = _buffer.getOverWrites ();
	_buffer.addRawValue (value->getRawValue ());
	if (ow <= 0  &&  _buffer.getOverWrites () > 0)
		LOG_MSG (osiTime::getCurrent() << ", " << _name << ": "
			<< _buffer.getOverWrites () << " overwrites\n");
	_last_buffer_time = value->getTime ();
}

void ChannelInfo::addEvent (dbr_short_t status, dbr_short_t severity, const osiTime &time)
{
	static osiTime adjust (0.0l);

	if (!_tmp_value)
	{
		LOG_ASSERT (_tmp_value);
		return;
	}
	// Setup "event" Value. Clear, then set only common fields that archiver event uses:
	memset (_tmp_value->getRawValue (), 0, _tmp_value->getRawValueSize());
	_tmp_value->setStatus (status, severity);
	_tmp_value->setTime (time);

	if (_tmp_value->getTime() <= _last_buffer_time)
	{	// adjust time because this event has to be added to archive somehow
		_last_buffer_time += adjust;
		_tmp_value->setTime (_last_buffer_time);
	}
	addToRingBuffer (_tmp_value);
}


//#define LOG_NSV(stuff)	LOG_MSG(stuff)
#define LOG_NSV(stuff)	{}

// To make handleNewValue more readable:
// We are scanned, _new_value is OK,
// but: were there repeats? shall we really add it?
inline void ChannelInfo::handleNewScannedValue ()
{
	osiTime stamp = _new_value->getTime ();
	size_t repeat_count;

	LOG_ASSERT (_previous_value);

	LOG_NSV ("handleNewScannedValue: got " << *_new_value);

	// 1) Scanned: only saved on change, otherwise calculate repeat counts:
	if (_previous_value_set &&
		_previous_value->hasSameValue (*_new_value) &&
		_previous_value->getStat() == _new_value->getStat() &&
		_previous_value->getSevr() == _new_value->getSevr())
	{
		LOG_NSV (" - repeat\n");
		return;
	}

	if (_expected_next_time == nullTime)
		_expected_next_time = roundTimeUp (stamp, _period);
	else if (stamp < _expected_next_time)
	{
		LOG_NSV (" - made pending\n");
		_pending_value->copyValue (*_new_value);
		_pending_value_set = true;
		return;// Add only if expected_next_time exceeded
	}

	// 2) Save close to _expected_next_time.
	// _new_value is more recent than _expected_next_time,
	// but maybe something's left that was valid at the exact _expected_next_time:
	if (_pending_value_set)
	{
		LOG_NSV (" - made pending\n");
		LOG_NSV (" unpending:                " << *_pending_value << "\n");
		stamp = _pending_value->getTime ();
		repeat_count = flushRepeats (stamp);
		addToRingBuffer (_pending_value);
		// remember this value for comparison next time
		_previous_value->copyValue (*_pending_value);
		_previous_value_set = true;
		// _new_value wasn't saved, it's now pending
		_pending_value->copyValue (*_new_value);
		// _pending_value_set == true still holds
		stamp = _pending_value->getTime ();
	}
	else
	{
		repeat_count = flushRepeats (stamp);
		addToRingBuffer (_new_value);
		// remember this value for comparison next time
		_previous_value->copyValue (*_new_value);
		_previous_value_set = true;
	}

	if (repeat_count > 0)
		_expected_next_time += _period * repeat_count;
	else
		_expected_next_time += _period;
	if (_expected_next_time <= stamp)
		_expected_next_time = roundTimeUp (stamp, _period);
	LOG_NSV (" next time:                " << _expected_next_time << "\n");
}

inline bool ChannelInfo::isDisabling(const GroupInfo *group) const
{	return _disabling[group->getID()];	}

// To make handleNewValue more readable:
// _value is set, check if we should disable anything based on it
inline void ChannelInfo::handleDisabling ()
{
	if (_disabling.empty())
		return;

	bool criteria = _new_value->getDouble() > 0;
	if (criteria && !_currently_disabling)
	{
		_currently_disabling = true;
		list<GroupInfo *>::iterator g;
		for (g=_groups.begin(); g!=_groups.end(); ++g)
		{
			if (isDisabling(*g))
				(*g)->disable (this);
		}
	}
	else
	if (!criteria && _currently_disabling)
	{
		list<GroupInfo *>::iterator g;
		for (g=_groups.begin(); g!=_groups.end(); ++g)
		{
			if (isDisabling(*g))
				(*g)->enable (this);
		}
		_currently_disabling = false;
	}
}

// Called from caEventHandler or (SinglePeriod-)ScanList,
// _new_value is already set
void ChannelInfo::handleNewValue ()
{
	osiTime now = osiTime::getCurrent ();
	osiTime stamp = _new_value->getTime();

	if (! isValidTime (stamp))
	{
		if ((double(now) - double(_had_null_time)) > (60.0*60*24))
		{	// quite frequent, limit messages to once a day
			LOG_MSG (now << ", " << _name << ", IOC " << ca_host_name(_chid)
				<< " : Invalid/null time stamp\n\t"
				<< *_new_value << "\n");
			_had_null_time = now;
		}
		_new_value_set = false;
		return;
	}

	_new_value_set = true;

	if (stamp > _last_buffer_time)
	{
		if (isDisabled ())	// Ignore while disabled
			return;

		if (_monitored)	// save all monitors
		{
			addToRingBuffer (_new_value);
			_expected_next_time = roundTimeUp (stamp, _period);
		}
		else
			handleNewScannedValue ();
	}
#if 0
	// historic time stamp: cannot use
	if (stamp < _last_buffer_time)
		LOG_MSG (now << ", " << _name << ": Outdated value" << *_value << "\n");
#endif

	handleDisabling ();
}

// For scanned channels,
// handleNewValue won't put repeated values
// in the ring buffer unless there's a change.
// This call will force it to write the
// repeat count out up to 'now'.
size_t ChannelInfo::flushRepeats (const osiTime &now)
{
	if (_monitored  ||  _previous_value_set==false)
		return 0;

	double time = _expected_next_time, stamp = now;
	if (time >= stamp) 
		return 0;

	size_t repeat_count = size_t ((stamp - time) / _period);
	if (repeat_count > 1)
	{	// put a repeat event into the circular buffer
		time += repeat_count * _period;
		osiTime artificial_stamp = osiTime (time);
		if (artificial_stamp < now) // no rounding errors?
		{
			// +1 because "_expected_next_time" = last entry + period
			_previous_value->setStatus (repeat_count+1, ARCH_REPEAT);
			_previous_value->setTime (artificial_stamp);
			addToRingBuffer (_previous_value);
		}
	}
	_previous_value_set = false;

	return repeat_count;
}

void ChannelInfo::disable (ChannelInfo *cause)
{
	if (_disabling.any())	// 'Disabling' channel has to be kept alive
		return;

	if (++_disabled < _groups.size())	// Disabled by all groups?
		return;

	if (! _connected)	// might have no circ. buffer at all
		return;

	// Flush everything until the disable happened
	flushRepeats (cause->_new_value->getTime());
	addEvent (0, ARCH_DISABLED, cause->_new_value->getTime());
	_expected_next_time = nullTime;
}

void ChannelInfo::enable (ChannelInfo *cause)
{
	if (_disabling.any())	// 'Disabling' channel has to be kept alive
		return;

	// Check if enabled by all groups
	if (_disabled <= 0)
	{
		LOG_MSG (osiTime::getCurrent() << ": Channel " << _name << " is not disabled, ERROR!\n");
		return;
	}
	if (--_disabled > 0)
		return;

	if (isConnected() && _new_value_set)
	{
		// Write last value as if it came right now:
		// (value did come in, it just wasn't written)
		_new_value->setTime (cause->_new_value->getTime());
		handleNewValue ();
	}
}


// Dump circular buffer into archive
void ChannelInfo::write (Archive &archive, ChannelIterator &channel)
{
	if (! archive.findChannelByName (_name, channel))
	{
		LOG_MSG ("ChannelInfo::write: Cannot find " << _name << "\n");
		return;
	}

	size_t count = _buffer.getCount();
	if (count <= 0)
		return;

	const RawValueI::Type *raw = _buffer.removeRawValue ();
	_write_lock.take ();
	size_t avail = channel->lockBuffer (*_write_value, _period);
	while (raw)
	{
		_write_value->copyIn (raw);
		if (avail <= 0) // no buffer at all or buffer full
		{
			channel->addBuffer (*_write_value, _period, _vals_per_buffer);

			avail = _vals_per_buffer;
			if (_vals_per_buffer < MAX_VALS_PER_BUF)
				_vals_per_buffer *= BUF_GROWTH_RATE;
		}

		if (! channel->addValue (*_write_value))
		{
			LOG_MSG ("Fatal: cannot add values in writeArchive (" << _name << ")\n");
			break;
		}

		if (--count <= 0)
			break;
		--avail;
		raw = _buffer.removeRawValue ();
	}
	_write_lock.give ();
	_buffer.reset ();
}
