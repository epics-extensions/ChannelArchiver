#include "epicsTimeHelper.h"

// TODO: Use RawValue, Index & Datafile instead of full LibIO
#include "ArchiveException.h"
#include "ArchiveI.h"
#include "BinValue.h"

#include "ArchiveChannel.h"
#include "Engine.h"

ArchiveChannel::ArchiveChannel(const stdString &name,
                               double period, SampleMechanism *mechanism)
{
    this->name = name;
    this->period = period;
    this->mechanism = mechanism;
    mechanism->channel = this;
    chid_valid = false;
    connected = false;
    nelements = 0;
    pending_value_set = false;
    pending_value = 0;
    disabled_count = 0;
    currently_disabling = false;
}

ArchiveChannel::~ArchiveChannel()
{
    if (mechanism)
        delete mechanism;
    if (pending_value)
        RawValue::free(pending_value);
    if (chid_valid)
    {
        LOG_MSG("'%s': clearing channel\n", name.c_str());
        ca_clear_channel(ch_id);
        chid_valid = false;
    }
}

void ArchiveChannel::addToGroup(GroupInfo *group, bool disabling)
{
    // bit in _disabling indicates if we could disable that group
    groups_to_disable.grow(group->getID() + 1);
    groups_to_disable.set(group->getID(), disabling);
    
    // Is Channel already in group?
    stdList<GroupInfo *>::iterator i;
    for (i=groups.begin(); i!=groups.end(); ++i)
        if (*i == group)
            return;
    
    groups.push_back(group);
    // If channel is added and it's already connected,
    // the whole connection shebang will be skipped
    // -> tell group we're connected right away:
    if (connected)
        ++group->num_connected;
}
    
void ArchiveChannel::startCA()
{
    if (!chid_valid)
    {
#       ifdef DEBUG_CI
        printf("ca_create_channel(%s)\n", name.c_str());
#       endif
        if (ca_create_channel(name.c_str(),
                              connection_handler, this,
                              CA_PRIORITY_ARCHIVE, &ch_id) != ECA_NORMAL)
        {
            LOG_MSG("'%s': ca_search_and_connect failed\n", name.c_str());
        }
        chid_valid = true;
    }
    else
    {
        // re-get control information
        // TODO
    }
    theEngine->need_CA_flush = true;
}


void ArchiveChannel::disable(const epicsTime &when)
{
    ++disabled_count;
    if (disabled_count < (int)groups.size())
        return; // some groups still want our values
    addEvent(0, ARCH_DISABLED, when);
}

void ArchiveChannel::enable(const epicsTime &when)
{
    --disabled_count;
    // Try to write the last value we got while disabled
    if (connected && pending_value_set)
    {
        LOG_MSG("'%s': re-enabled, writing the most recent value\n",
                name.c_str());
        RawValue::setTime(pending_value, when);
        buffer.addRawValue(pending_value);
        pending_value_set = false;
    }
}

void ArchiveChannel::write(Archive &archive, ChannelIterator &channel)
{
    try
    {
        size_t count = buffer.getCount();
        if (count <= 0)
            return;
        if (! archive.findChannelByName(name, channel))
        {
            LOG_MSG ("ArchiveChannel::write: Cannot find '%s'\n",
                     name.c_str());
            return;
        }
        
        BinValue *write_value = BinValue::create(dbr_time_type, nelements);
        write_value->setCtrlInfo(&ctrl_info);
        const RawValue::Data *raw = buffer.removeRawValue();
        size_t avail = channel->lockBuffer(*write_value, period);
        while (raw)
        {
            write_value->copyIn(raw);
            if (avail <= 0) // no buffer at all or buffer full
            {
                channel->addBuffer(*write_value, period, 100);
                avail = 100;
                //            if (_vals_per_buffer < MAX_VALS_PER_BUF)
                //    _vals_per_buffer *= BUF_GROWTH_RATE;
            }
            if (! channel->addValue(*write_value))
            {
                LOG_MSG("Fatal: cannot add values in writeArchive '%s'\n",
                        name.c_str());
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
            raw = buffer.removeRawValue();
        }
        buffer.resetOverwrites();
        delete write_value;
    }
    catch (ArchiveException &e)
    {
        LOG_MSG("ArchiveChannel::write caught %s\n", e.what());
    }   
}


// CA callback for connects and disconnects
void ArchiveChannel::connection_handler(struct connection_handler_args arg)
{
    ArchiveChannel *me = (ArchiveChannel *) ca_puser(arg.chid);
    me->mutex.lock();
    if (ca_state(arg.chid) == cs_conn)
    {
        LOG_MSG("%s: cs_conn, getting control info\n", me->name.c_str());
        // Get control information for this channel
        // TODO: This is only requested on connect
        // - similar to the previous engine or DM.
        // How do we learn about changes, since you might actually change
        // a channel without rebooting an IOC?
        int status = ca_array_get_callback(
            ca_field_type(me->ch_id)+DBR_CTRL_STRING,
            ca_element_count(me->ch_id),
            me->ch_id, control_callback, me);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("'%s': ca_array_get_callback error in "
                    "caLinkConnectionHandler: %s",
                    me->name.c_str(), ca_message (status));
        }
        theEngine->need_CA_flush = true;
    }
    else
    {
        me->connected = false;
        me->connection_time = epicsTime::getCurrent();
        me->pending_value_set = false;
        me->mechanism->handleConnectionChange();
    }    
    me->mutex.unlock();
}

// Fill crtl_info from raw dbr_ctrl_xx data
bool ArchiveChannel::setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx)
{
    switch (type)
    {
        case DBR_CTRL_DOUBLE:
        {
            struct dbr_ctrl_double *ctrl = (struct dbr_ctrl_double *)dbr_ctrl_xx;
            ctrl_info.setNumeric(ctrl->precision, ctrl->units,
                                 ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_SHORT:
        {
            struct dbr_ctrl_int *ctrl = (struct dbr_ctrl_int *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_FLOAT:
        {
            struct dbr_ctrl_float *ctrl = (struct dbr_ctrl_float *)dbr_ctrl_xx;
            ctrl_info.setNumeric(ctrl->precision, ctrl->units,
                                 ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_CHAR:
        {
            struct dbr_ctrl_char *ctrl = (struct dbr_ctrl_char *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_LONG:
        {
            struct dbr_ctrl_long *ctrl = (struct dbr_ctrl_long *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit, ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit, ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_ENUM:
        {
            size_t i;
            struct dbr_ctrl_enum *ctrl = (struct dbr_ctrl_enum *)dbr_ctrl_xx;
            ctrl_info.allocEnumerated(ctrl->no_str,
                                      MAX_ENUM_STATES*MAX_ENUM_STRING_SIZE);
            for (i=0; i<(size_t)ctrl->no_str; ++i)
                ctrl_info.setEnumeratedString(i, ctrl->strs[i]);
            ctrl_info.calcEnumeratedSize();
        }
        return true;
        case DBR_CTRL_STRING:   // no control information
            ctrl_info.setEnumerated(0, 0);
            return true;
    }
    return false;
}

void ArchiveChannel::control_callback(struct event_handler_args arg)
{
    ArchiveChannel *me = (ArchiveChannel *) ca_puser(arg.chid);
    me->mutex.lock();

    if (arg.status == ECA_NORMAL &&
        me->setup_ctrl_info(arg.type, arg.dbr))
    {
        LOG_MSG("%s: received control info\n", me->name.c_str());
        me->connection_time = epicsTime::getCurrent();
        me->dbr_time_type = ca_field_type(arg.chid)+DBR_TIME_STRING;
        me->nelements = ca_element_count(arg.chid);
        me->buffer.allocate(me->dbr_time_type, me->nelements,
                            theEngine->suggestedBufferSize(me->period));
        if (me->pending_value)
            RawValue::free(me->pending_value);
        me->pending_value = RawValue::allocate(me->dbr_time_type, me->nelements, 1);
        me->pending_value_set = false;
        me->connected = true;
        me->mechanism->handleConnectionChange();
    }
    else
    {
        LOG_MSG("%s: ERROR, control info request failed\n", me->name.c_str());
        me->connection_time = epicsTime::getCurrent();
        me->connected = false;
        me->pending_value_set = false;
        me->mechanism->handleConnectionChange();
    }
    me->mutex.unlock();
}

// called by SampleMechanism, mutex is locked.
void ArchiveChannel::handleDisabling(const RawValue::Data *value)
{
    if (groups_to_disable.empty())
        return;

    // We disable if the channel is non-zero
    bool criteria = RawValue::isZero(dbr_time_type, value) == false;
    if (criteria && !currently_disabling)
    {   // wasn't disabling -> disabling
        currently_disabling = true;
        stdList<GroupInfo *>::iterator g;
        for (g=groups.begin(); g!=groups.end(); ++g)
        {
            if (groups_to_disable.test((*g)->getID()))
                (*g)->disable(this, RawValue::getTime(value));
        }
    }
    else if (!criteria && currently_disabling)
    {   // was disabling -> enabling
        stdList<GroupInfo *>::iterator g;
        for (g=groups.begin(); g!=groups.end(); ++g)
        {
            if (groups_to_disable.test((*g)->getID()))
                (*g)->enable(this, RawValue::getTime(value));
        }
        currently_disabling = false;
    }
}

// Event (value with special status/severity):
// Add unconditionally to ring buffer,
// maybe adjust time so that it can be added
void ArchiveChannel::addEvent(dbr_short_t status, dbr_short_t severity,
                              const epicsTime &event_time)
{
    if (nelements <= 0)
    {
        LOG_MSG("'%s': Cannot add event because data type is unknown\n",
                name.c_str());
        return;
    }
    RawValue::Data *value = buffer.getNextElement();
    memset(value, 0, RawValue::getSize(dbr_time_type, nelements));
    RawValue::setStatus(value, status, severity);
    RawValue::setTime(value, event_time);

#ifdef TODO
    // maybe this is nonsense?
    static double adjust = 0.0l;

    if (value->getTime() <= _last_archive_stamp)
    {   // adjust time, event has to be added to archive somehow!
        _last_archive_stamp += adjust;
        value->setTime(_last_archive_stamp);
    }
#endif
}
