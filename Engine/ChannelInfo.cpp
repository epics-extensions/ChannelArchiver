// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "Engine.h"
#include <epicsTimeHelper.h>
#include <math.h>

#define DEBUG_CI

#if 0
// Helper:
// print chid
static void show_chid(const chid &chid)
{
    short typ = ca_field_type(chid);
    const char *type_txt = (typ > 0  &&  typ < dbr_text_dim) ?
                           dbr_text[typ] : dbr_text_invalid;

    printf("CHID: %s\n", ca_name(chid));
    printf("\ttype/count: %s (%d) / %lu\n",
           type_txt, typ, ca_element_count(chid));
    printf("\thost: %s\n", ca_host_name(chid));
    printf("\tuser ptr: 0x%lX\n", (unsigned long) ca_puser(chid));
    printf("\tca state: ");
    switch (ca_state(chid))
    {
    case cs_never_conn:
        printf("cs_never_conn: valid chid, IOC not found\n");
        break;
    case cs_prev_conn:
        printf("cs_prev_conn   valid chid, IOC was found, but unavailable\n");
        break;
    case cs_conn:
        printf("cs_conn:       valid chid, IOC was found, still available\n");
        break;
    case cs_closed:
        printf("cs_closed:     invalid chid\n");
        break;
    default:
        printf("%d (undefined)\n", ca_state(chid));
    }
}
#endif

ChannelInfo::ChannelInfo()
{
    _currently_disabling = false;
    _disabled = 0;
    _monitored = false;
    _connected = false;
    _ever_written = false;
    _mechanism = none;
    _vals_per_buffer = INIT_VALS_PER_BUF;
    _period = 1;            // Default scan period
    _chid = 0;
    
    _new_value_set = false;
    _new_value = 0;
    _pending_value_set = false;
    _pending_value = 0;
    _previous_value_set = false;
    _previous_value = 0;
    _tmp_value = 0;
    _write_value = 0;
    _had_null_time = nullTime;
}

ChannelInfo::~ChannelInfo()
{
    delete _write_value;
    delete _tmp_value;
    delete _previous_value;
    delete _pending_value;
    delete _new_value;
}

void ChannelInfo::setPeriod(double secs)
{
    if (secs <= 0.0)
    {
        LOG_MSG("'%s': setPeriod called with %g, changed to 30 secs\n",
                _name.c_str(), secs);
        secs = 30.0;
    }
    _period = secs;
    checkRingBuffer();
}

// Mark channel as belonging to 'group', maybe disabling that group.
// It's OK to call this method again with the same group.
void ChannelInfo::addToGroup(GroupInfo *group, bool disabling)
{
    // bit in _disabling indicates if we could disable that group
    _disabling.grow(group->getID() + 1);
    _disabling.set(group->getID(), disabling);
    
    // Is Channel already in group?
    stdList<GroupInfo *>::iterator i;
    for (i=_groups.begin(); i!=_groups.end(); ++i)
        if (*i == group)
            return;
    
    _groups.push_back(group);
    // If channel is added and it's already connected,
    // the whole connection shebang will be skipped
    // -> tell group we're connected right away:
    if (_connected)
        group->incConnectedChannels();
}

void ChannelInfo::startCaConnection(bool new_channel)
{
    if (new_channel)
    {
#       ifdef DEBUG_CI
        printf("ca_create_channel(%s)\n", _name.c_str());
#       endif
        if (ca_create_channel(_name.c_str(),
                              caLinkConnectionHandler, this,
                              CA_PRIORITY_ARCHIVE, &_chid) != ECA_NORMAL)
        {
            LOG_MSG("'%s': ca_search_and_connect failed\n", _name.c_str());
        }
    }
    else
    {
        // re-get control information for this channel
        // as in caLinkConnectionHandler
        if (_new_value && _connected)
        {
            int status = ca_array_get_callback(_new_value->getType()
                                               -DBR_TIME_STRING
                                               +DBR_CTRL_STRING, 1,
                                               _chid,
                                               caControlHandler, this);
            if (status != ECA_NORMAL)
            {
                LOG_MSG("'%s': CA ca_array_get_callback"
                        " failed in startCaConnection: %s\n",
                        getName().c_str(), ca_message(status));
                return;
            }
        }
    }
    theEngine->needCAflush();
}

// CA Callback for each channel that connects or disconnects:
// * ca_search_and_connect calls this when a channel is found
// * CA also calls this routine when a connection is broken
void ChannelInfo::caLinkConnectionHandler(struct connection_handler_args arg)
{
    stdList<GroupInfo *>::iterator g;
    ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);

    me->lock();
    bool was_connected = me->_connected;
#   ifdef DEBUG_CI
    printf("caLinkConnectionHandler(%s), thread 0x%08X: %s, now: %s\n",
           me->getName().c_str(), (unsigned int)epicsThreadGetIdSelf(),
           (was_connected ? "was connected" : "wasn't connected"),
           (ca_state(arg.chid) == cs_conn ? "connected" : "disconnected")
           );
#   endif
    if (ca_state(arg.chid) != cs_conn)
    {
        LOG_MSG("'%s': CA disconnect\n", me->getName().c_str());
        // Flush everything until the disconnect happened
        me->_connected = false;
        // add a disconnect event
        me->addEvent(0, ARCH_DISCONNECT, me->_connect_time);
        // reset, prepare for reconnect
        me->_ever_written = false; 
        me->_new_value_set = false;
        me->_expected_next_time = nullTime;
        me->_pending_value_set = false;
        me->_previous_value_set = false;
        me->_connect_time = epicsTime::getCurrent();
        if (was_connected)
        {
            // Update statics in GroupInfo:
            for (g=me->_groups.begin(); g!=me->_groups.end(); ++g)
                (*g)->decConnectedChannels();
        }
    }
    else
    {
        // else: (re-)connected
        // Get control information for this channel
        // TODO: This is only requested on connect
        // - similar to the previous engine or DM.
        // How do we learn about changes, since you might actually change
        // a channel without rebooting an IOC?
        // Note: CA used to be broken here; DBR_CTRL... only worked with
        //       count==1 for arrays. We'll get the full value later
        int status = ca_array_get_callback(ca_field_type(arg.chid)
                                           +DBR_CTRL_STRING, 1,
                                           me->_chid, caControlHandler, me);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("'%s': ca_array_get_callback error in "
                    "caLinkConnectionHandler: %s",
                    me->getName().c_str(), ca_message (status));
        }
        // ChannelInfo is not really considered 'connected' until
        // we received the control information...
        theEngine->needCAflush();
    }
    me->unlock();
}

static bool setup_CtrlInfo(DbrType type, CtrlInfoI &info, const void *raw)
{
    switch (type)
    {
    case DBR_CTRL_DOUBLE:
        {
            struct dbr_ctrl_double *ctrl = (struct dbr_ctrl_double *)raw;
            info.setNumeric(ctrl->precision, ctrl->units,
                            ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                            ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                            ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
    case DBR_CTRL_SHORT:
        {
            struct dbr_ctrl_int *ctrl = (struct dbr_ctrl_int *)raw;
            info.setNumeric(0, ctrl->units,
                            ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                            ctrl->lower_alarm_limit,ctrl->lower_warning_limit,
                            ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
    case DBR_CTRL_FLOAT:
        {
            struct dbr_ctrl_float *ctrl = (struct dbr_ctrl_float *)raw;
            info.setNumeric(ctrl->precision, ctrl->units,
                            ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                            ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                            ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
    case DBR_CTRL_CHAR:
        {
            struct dbr_ctrl_char *ctrl = (struct dbr_ctrl_char *)raw;
            info.setNumeric(0, ctrl->units,
                            ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                            ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                            ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
    case DBR_CTRL_LONG:
        {
            struct dbr_ctrl_long *ctrl = (struct dbr_ctrl_long *)raw;
            info.setNumeric(0, ctrl->units,
                            ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                            ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                            ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
    case DBR_CTRL_ENUM:
        {
            size_t i;
            struct dbr_ctrl_enum *ctrl = (struct dbr_ctrl_enum *)raw;
            info.allocEnumerated(ctrl->no_str,
                                 MAX_ENUM_STATES*MAX_ENUM_STRING_SIZE);
            for (i=0; i<(size_t)ctrl->no_str; ++i)
                info.setEnumeratedString(i, ctrl->strs[i]);
            info.calcEnumeratedSize();
        }
        return true;
    case DBR_CTRL_STRING:   /* no control information */
        info.setEnumerated(0, 0);
        return true;
    }
    return false;
}

// Called by CA for new control information
void ChannelInfo::caControlHandler(struct event_handler_args arg)
{
    ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);
    me->lock();
    bool was_connected = me->_connected;

#   ifdef DEBUG_CI
    printf("caControlHandler(%s), thread 0x%08X: %s\n",
           me->getName().c_str(), (unsigned int)epicsThreadGetIdSelf(),
           (was_connected ? "was connected" : "wasn't connected"));
#   endif

    if (arg.status != ECA_NORMAL)
    {
        LOG_MSG("'%s': Bad control information, CA status %s\n",
                me->_name.c_str(), ca_message(arg.status));
        if (me->_new_value  &&  me->_mechanism != none)
        {
            LOG_MSG("...ignored since it seems to be a re-connect\n");
            me->_connected = true;
        }
    }
    else
    {
        if (setup_CtrlInfo(arg.type, me->_ctrl_info, arg.dbr))
            me->_connected = true;
        else
        {
            LOG_MSG("'%s' ERROR: Unknown CA control information\n",
                    me->_name.c_str());
            me->_connected = false;
        }
        me->_connect_time = epicsTime::getCurrent();

        DbrType dbr_type = DbrType(ca_field_type(arg.chid)+DBR_TIME_STRING);
        DbrCount nelements = ca_element_count(arg.chid);
        // size or type might have changed...
        me->setValueType(dbr_type, nelements);

#       ifdef DEBUG_CI
        printf("caControlHandler(%s) got CtrlInfo for %s [%d]\n",
               me->getName().c_str(),
               dbr_type_to_text(dbr_type), nelements);
#       endif
        
        // Already subscribed or first connection?
        if (me->_mechanism == none   ||
            (me->isMonitored() && me->_mechanism != use_monitor)  )
        {
            if (!me->isMonitored()  &&  theEngine->addToScanList(me))
            {
                me->_mechanism = use_get;
            }
            else
            {
                // Engine will not scan this: add a monitor for this channel
                int status;
                status = ca_add_masked_array_event(dbr_type, nelements,
                                                   me->_chid,
                                                   caEventHandler, me,
                                                   0.0, 0.0, 0.0, (evid *)0,
                                                   DBE_LOG);
                if (status != ECA_NORMAL)
                {
                    LOG_MSG("'%s' CA ca_add_array_event failed: %s\n",
                            me->_name.c_str(), ca_message(status));
                    me->unlock();
                    return;
                }
                theEngine->needCAflush();
                me->_mechanism = use_monitor;
            }
        }
    }

    if (!was_connected  &&  me->_connected)
    {
        LOG_MSG("'%s': Connected\n", me->_name.c_str());
        stdList<GroupInfo *>::iterator g;
        for (g=me->_groups.begin(); g!=me->_groups.end(); ++g)
            (*g)->incConnectedChannels();
    }
    me->unlock();
}

// Callback for values (from monitor or get_callback)
void ChannelInfo::caEventHandler(struct event_handler_args arg)
{
    ChannelInfo *me = (ChannelInfo *) ca_puser(arg.chid);
    me->lock();
    if (!me || !me->_new_value)
    {
        LOG_MSG("'%s': caEventHandler called without ChannelInfo\n",
                ca_name(arg.chid));
        me->unlock();
        return;
    }
    me->_new_value->copyIn(reinterpret_cast<const RawValueI::Type *>(arg.dbr));

#   ifdef DEBUG_CI
    printf("caEventHandler(%s), thread 0x%08X:\n",
            me->getName().c_str(), (unsigned int)epicsThreadGetIdSelf());
    me->_new_value->show(stdout);
#   endif

    me->handleNewValue();
    me->unlock();
}

// Issue CA get for _scanned_value, no ca_pend_io in here!
void ChannelInfo::issueCaGetCallback()
{
    if (!_new_value)
    {
        LOG_MSG("No value description in issueCaGetCallback '%s'!\n",
                _name.c_str());
        return;
    }
#   ifdef DEBUG_CI
    printf("issueCaGetCallback(%s), thread 0x%08X\n",
           getName().c_str(), (unsigned int)epicsThreadGetIdSelf());
#   endif
    if (ca_array_get_callback(_new_value->getType(), _new_value->getCount(),
                              _chid, caEventHandler, this) != ECA_NORMAL)
        LOG_MSG("ca_array_get_callback(%s) failed\n", _name.c_str());
    theEngine->needCAflush();
}

// Define the type of value for this ChannelInfo.
// Result: has the type changed ?
bool ChannelInfo::setValueType(DbrType type, DbrCount count)
{
    bool changed = false;

    if (_new_value)
    {
        if (_new_value->getType() == type  &&
            _new_value->getCount() == count)
            return false; // same type
        
        delete _new_value;
        delete _previous_value;
        delete _tmp_value;
        delete _write_value;
        changed = true;
    }
    _new_value_set = false;
    _pending_value_set = false;
    _previous_value_set = false;

    _new_value = theEngine->newValue(type, count);
    _new_value->setCtrlInfo(&_ctrl_info);
    _pending_value = _new_value->clone();
    _previous_value = _new_value->clone();
    _tmp_value = _new_value->clone();
    _write_value = _new_value->clone();
    checkRingBuffer();

    return changed;
}

// Check if Ring buffer is big enough, fits _new_value etc.
// Might be called from setWritePeriod() before _value is available...
void ChannelInfo::checkRingBuffer()
{
    if (_new_value)
        _buffer.allocate(_new_value->getType(), _new_value->getCount(),
                         _period);
}

void ChannelInfo::addToRingBuffer(const ValueI *value)
{
    if (! value)
    {
        LOG_MSG ("'%s': addToRingBuffer called without value\n",
                 _name.c_str());
        return;
    }
    
    if (value->getTime() < _last_archive_stamp)
    {
        stdString t;
        LOG_MSG("'%s': Ring buffer back-in-time\n", _name.c_str());
        epicsTime2string(_last_archive_stamp, t);
        LOG_MSG("Prev:  %s\n", t.c_str());
        epicsTime2string(value->getTime(), t);
        LOG_MSG("Value: %s\n", t.c_str());
    }

    size_t ow = _buffer.getOverwrites();
    _buffer.addRawValue(value->getRawValue());
    _ever_written = true;
    if (ow <= 0  &&  _buffer.getOverwrites() > 0)
        LOG_MSG ("'%s': %d overwrites\n",
                 _name.c_str(), _buffer.getOverwrites());
    setLastArchiveStamp(value->getTime());
}

// Event (value with special status/severity):
// Add unconditionally to ring buffer,
// maybe adjust time so that it can be added
void ChannelInfo::addEvent(dbr_short_t status, dbr_short_t severity,
                           const epicsTime &event_time)
{
    if (!_tmp_value)
    {
        LOG_MSG("'%s': Cannot add event because data type is unknown\n",
                _name.c_str());
        return;
    }

    if (_pending_value_set)
    {
        const epicsTime &pend_time = _pending_value->getTime();
        if (pend_time < event_time)
        {
#ifdef DEBUG_CI
            printf("Event!, unpending: ");
            _pending_value->show(stdout);
            printf("\n");
#endif
            flushRepeats(_pending_value->getTime());
            addToRingBuffer(_pending_value);
            _pending_value_set = false;
        }
    }
    else
        flushRepeats(event_time);

    // Setup "event" Value. Clear, then set only common fields
    // that archiver event uses:
    memset(_tmp_value->getRawValue(), 0, _tmp_value->getRawValueSize());
    _tmp_value->setStatus(status, severity);
    _tmp_value->setTime(event_time);
    addEvent(_tmp_value);
}

// Force Value into archive, adjusting the time stamp if necessary
void ChannelInfo::addEvent(ValueI *value)
{
    static double adjust = 0.0l;

    if (value->getTime() <= _last_archive_stamp)
    {   // adjust time, event has to be added to archive somehow!
        _last_archive_stamp += adjust;
        value->setTime(_last_archive_stamp);
    }
    addToRingBuffer(value);

    // events are not repeat-counted, _previous_value is unset
    _previous_value_set = false;
}

// To make handleNewValue more readable:
// _value is set, check if we should disable anything based on it
void ChannelInfo::handleDisabling()
{
    if (_disabling.empty())
        return;

    bool criteria = _new_value->getDouble() > 0;
    if (criteria && !_currently_disabling)
    {
        _currently_disabling = true;
        stdList<GroupInfo *>::iterator g;
        for (g=_groups.begin(); g!=_groups.end(); ++g)
        {
            if (isDisabling(*g))
                (*g)->disable(this);
        }
    }
    else
    if (!criteria && _currently_disabling)
    {
        stdList<GroupInfo *>::iterator g;
        for (g=_groups.begin(); g!=_groups.end(); ++g)
        {
            if (isDisabling(*g))
                (*g)->enable(this);
        }
        _currently_disabling = false;
    }
}

// Called from caEventHandler or (SinglePeriod-)ScanList,
// _new_value is already set
void ChannelInfo::handleNewValue()
{
    epicsTime now = epicsTime::getCurrent();
    epicsTime stamp = _new_value->getTime();

    // Check time stamp: 0/invalid?
    if (! isValidTime(stamp))
    {
        if ((now - _had_null_time) > (60.0*60*24))
        {   // quite frequent, limit messages to once a day
            LOG_MSG("'%s', , IOC %s: Invalid/null time stamp\n\t",
                    _name.c_str(), ca_host_name(_chid));
            stdString t, s;
            epicsTime2string(_new_value->getTime(), t);
            _new_value->getStatus(s);
            LOG_MSG("%s %s\n", t.c_str(), s.c_str());
            _had_null_time = now;
        }
        _new_value_set = false;
        return;
    }
    // Too far in the future?
    if ((stamp > now) &&
        ((stamp - now) > theEngine->getIgnoredFutureSecs()))
    {
        LOG_MSG("'%s', , IOC %s: Ignoring futuristic time stamp\n",
                _name.c_str(), ca_host_name(_chid));
        stdString t, s;
        epicsTime2string(_new_value->getTime(), t);
        _new_value->getStatus(s);
        LOG_MSG("%s %s\n", t.c_str(), s.c_str());
        _new_value_set = false;
        return;
    }

    if (!_ever_written)
    {
#       ifdef DEBUG_CI
        printf("%s: write first value after connection\n", _name.c_str());
#       endif
        addEvent(_new_value);
        return;
    }
    
    _new_value_set = true;
    if (isDisabled())  // Ignore while disabled
        return;
    if (stamp <= _last_archive_stamp)
    {
        // Ignore back-in-time values, except after startup:
        if (_ever_written)
        {
            _new_value_set = false;
            return;
        }
        // round up to indicate artificial value
        stamp = roundTimeUp(now, 1.0);
        if (stamp <= _last_archive_stamp)
        {
            LOG_MSG("'%s', , IOC %s: Unresolvable back-in-time value\n",
                    _name.c_str(), ca_host_name(_chid));
            stdString t, s;
            epicsTime2string(_new_value->getTime(), t);
            _new_value->getStatus(s);
            LOG_MSG("%s %s\n", t.c_str(), s.c_str());
            epicsTime2string(_last_archive_stamp, t);
            LOG_MSG("Previously archived value %s\n", t.c_str());
            return;
        }
        _new_value->setTime(stamp);
    }
    if (_monitored) // save all monitors
    {
        addToRingBuffer(_new_value);
        _expected_next_time = roundTimeUp(stamp, _period);
    }
    else
        handleNewScannedValue(stamp);
    handleDisabling ();
}

// To make handleNewValue more readable:
// Scanned operation, not monitored, _new_value is OK,
// but: were there repeats? shall we really add it?
void ChannelInfo::handleNewScannedValue(epicsTime &stamp)
{
#   ifdef DEBUG_CI
    printf("handleNewScannedValue(%s)\n", _name.c_str());
#   endif    
    if (!_previous_value)
    {
        LOG_MSG("'%s': handleNewScannedValue called "
                "without _previous_value\n", _name.c_str());
        return;
    }

    // New value, but not due to be saved? Update pending value!
    if (_expected_next_time == nullTime)
    {
        _expected_next_time = roundTimeDown(stamp, _period);

#       ifdef DEBUG_CI
        stdString time_txt;
        epicsTime2string(_expected_next_time, time_txt);
        printf("expected_next corrected to %s\n", time_txt.c_str());
#       endif
    }
    else if (stamp < _expected_next_time)
    {
#       ifdef DEBUG_CI        
        printf("stamp < expected - pending\n");
#       endif
        _pending_value->copyValue(*_new_value);
        _pending_value_set = true;
        return;// Add only if expected_next_time exceeded
    }

    // _new_value, time to take a sample.
    // Have pending value that's just before this time slot,
    // so we ought to save that one?
    size_t repeat_count = 0;
    if (_pending_value_set)
    {
        bool pending_is_repeated = isRepeated(_pending_value);
        if (pending_is_repeated)
        {
#           ifdef DEBUG_CI        
            printf("Would store the last pending value, "
                   "but it's repeated, so we keep waiting\n");
#           endif
        }
        else
        {
#           ifdef DEBUG_CI        
            printf("Storing the last pending value, "
                   "making this one pending\n");
#           endif
            stamp = _pending_value->getTime();
            repeat_count = flushRepeats(stamp);
            addToRingBuffer(_pending_value);
            // remember this value for comparison next time
            _previous_value->copyValue(*_pending_value);
            _previous_value_set = true;
        }
        // _new_value wasn't saved, it's now pending
        _pending_value->copyValue(*_new_value);
        if (pending_is_repeated)
            return;
    }
    else
    {
        if (isRepeated(_new_value))
        {
#           ifdef DEBUG_CI        
            printf("Repeat...\n");
#           endif
            return;
        }
        repeat_count = flushRepeats(stamp);
#       ifdef DEBUG_CI        
        printf("Stored\n");
#       endif
        addToRingBuffer(_new_value);
        // remember this value for comparison next time
        _previous_value->copyValue(*_new_value);
        _previous_value_set = true;
    }

    if (repeat_count > 0)
        _expected_next_time += _period * repeat_count;
    else
        _expected_next_time += _period;
    if (_expected_next_time <= stamp)
        _expected_next_time = roundTimeUp(stamp, _period);

#  ifdef DEBUG_CI
    stdString time_txt;
    epicsTime2string(_expected_next_time, time_txt);
    printf("next time: %s\n", time_txt.c_str());
#   endif
}

bool ChannelInfo::isDisabling(const GroupInfo *group) const
{
    return _disabling[group->getID()];
}

// For scanned channels,
// handleNewValue won't put repeated values
// in the ring buffer unless there's a change.
// This call will force it to write the
// repeat count out up to 'now'.
size_t ChannelInfo::flushRepeats(const epicsTime &now)
{
    if (_monitored  ||  _previous_value_set==false)
        return 0;

    double time_passed = now - _expected_next_time;
    if (time_passed <= 0.0) 
        return 0;

    size_t repeat_count = size_t(time_passed / _period);
    if (repeat_count >= 0)
    {   // put a repeat event into the circular buffer
        epicsTime artificial_stamp =
            _expected_next_time + repeat_count * _period;
        if (artificial_stamp < now) // no rounding errors?
        {
            // +1 because "_expected_next_time" = last entry + period
            _previous_value->setStatus(repeat_count+1, ARCH_REPEAT);
            _previous_value->setTime(artificial_stamp);
            addToRingBuffer(_previous_value);
        }
    }
    _previous_value_set = false;

    return repeat_count;
}

void ChannelInfo::disable(ChannelInfo *cause)
{
    if (_disabling.any())   // 'Disabling' channel has to be kept alive
        return;

    if (++_disabled < _groups.size())   // Disabled by all groups?
        return;

    if (! _connected)   // might have no circ. buffer at all
        return;

    // Flush everything until the disable happened
    addEvent(0, ARCH_DISABLED, cause->_new_value->getTime());
    _expected_next_time = nullTime;
}

void ChannelInfo::enable(ChannelInfo *cause)
{
    if (_disabling.any())   // 'Disabling' channel has to be kept alive
        return;

    // Check if enabled by all groups
    if (_disabled <= 0)
    {
        LOG_MSG("'%s' cannot be enabled since it is not disabled, ERROR!\n",
                _name.c_str());
        return;
    }
    if (-- _disabled > 0)
        return;
    
    if (isConnected() && _new_value_set)
    {
        // Write last value as if it came right now:
        // (value did come in, it just wasn't written)
        _new_value->setTime(cause->_new_value->getTime());
        handleNewValue();
    }
}

void ChannelInfo::resetBuffers()
{
    _buffer.reset();
    _new_value_set = false;
    _pending_value_set = false;
    _previous_value_set = false;
    _had_null_time = nullTime;
}

// Dump circular buffer into archive
void ChannelInfo::write(Archive &archive, ChannelIterator &channel)
{
    size_t count = _buffer.getCount();
    if (count <= 0)
        return;

    if (! archive.findChannelByName(_name, channel))
    {
        LOG_MSG ("ChannelInfo::write: Cannot find '%s'\n",
                 _name.c_str());
        return;
    }

    const RawValueI::Type *raw = _buffer.removeRawValue();
    size_t avail = channel->lockBuffer(*_write_value, _period);
    while (raw)
    {
        _write_value->copyIn(raw);
        if (avail <= 0) // no buffer at all or buffer full
        {
            channel->addBuffer(*_write_value, _period, _vals_per_buffer);

            avail = _vals_per_buffer;
            if (_vals_per_buffer < MAX_VALS_PER_BUF)
                _vals_per_buffer *= BUF_GROWTH_RATE;
        }

        if (! channel->addValue(*_write_value))
        {
            LOG_MSG("Fatal: cannot add values in writeArchive '%s'\n",
                    _name.c_str());
            break;
        }

#if defined(ENGINE_DEBUG) && ENGINE_DEBUG > 5
        stdString time, val, stat;
        _write_value->getTime(time);
        _write_value->getValue(val); 
        _write_value->getStatus(stat);
        LOG_MSG("write thread 0x%08X: %s %s %s %s\n",
                epicsThreadGetIdSelf(), channel->getName(),
                time.c_str(), val.c_str(), stat.c_str());
#endif
        
        if (--count <= 0)
            break;
        --avail;
        raw = _buffer.removeRawValue();
    }
    _buffer.resetOverwrites();
}


void ChannelInfo::shutdown(Archive &archive, ChannelIterator &channel,
                           const epicsTime &now)
{
    _buffer.reset();
    addEvent(0, ARCH_STOPPED, now);
    write(archive, channel);
}

