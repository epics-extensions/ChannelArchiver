// Tools
#include "epicsTimeHelper.h"
#include "MsgLogger.h"
#include "ArchiveException.h"
// Storage
#include "DataWriter.h"
//Engine
#include "ArchiveChannel.h"
#include "Engine.h"

ArchiveChannel::ArchiveChannel(const stdString &name, double period)
{
    this->name = name;
    this->period = period;
    this->mechanism = 0;
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
    mechanism = 0;
    if (pending_value)
        RawValue::free(pending_value);
    if (chid_valid)
    {
        LOG_MSG("'%s': clearing channel\n", name.c_str());
        ca_clear_channel(ch_id);
        chid_valid = false;
    }
}

void ArchiveChannel::setMechanism(Guard &guard, SampleMechanism *mechanism)
{
    guard.check(mutex);
    if (this->mechanism)
        delete this->mechanism;
    this->mechanism = mechanism;
}

void ArchiveChannel::setPeriod(Guard &engine_guard, Guard &guard, double period)
{
    guard.check(mutex);
    this->period = period;
    if (connected)
        buffer.allocate(dbr_time_type, nelements,
                        theEngine->suggestedBufferSize(engine_guard, period));
}

void ArchiveChannel::addToGroup(Guard &guard, GroupInfo *group, bool disabling)
{
    guard.check(mutex);
    // bit in 'disabling' indicates if we could disable that group
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
    
void ArchiveChannel::startCA(Guard &guard)
{
    guard.check(mutex);
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
    {   // Re-get control information for this channel
        int status = ca_array_get_callback(
            ca_field_type(ch_id)+DBR_CTRL_STRING,
            ca_element_count(ch_id),
            ch_id, control_callback, this);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("'%s': ca_array_get_callback error in "
                    "caLinkConnectionHandler: %s",
                    name.c_str(), ca_message(status));
        }
    }
    theEngine->need_CA_flush = true;
}


void ArchiveChannel::issueCaGet(Guard &guard)
{
    guard.check(mutex);
    if (connected)
    {
        if (ca_array_get_callback(dbr_time_type, nelements,
                                  ch_id, value_callback, this)
            != ECA_NORMAL)
        {
            LOG_MSG("ca_array_get_callback (%s) failed\n", name.c_str());
            return;
        }
        theEngine->need_CA_flush = true;
    }
}

void ArchiveChannel::disable(Guard &guard, const epicsTime &when)
{
    guard.check(mutex);
    ++disabled_count;
    if (disabled_count > (int)groups.size())
    {
        LOG_MSG("Channel '%s': Disable count is messed up (%d)\n",
                name.c_str(), disabled_count);
    }
    if (isDisabled(guard))
        addEvent(guard, 0, ARCH_DISABLED, when);
}

void ArchiveChannel::enable(Guard &guard, const epicsTime &when)
{
    guard.check(mutex);
    --disabled_count;
    if (disabled_count < 0)
    {
        LOG_MSG("Channel '%s': Disable count is messed up (%d)\n",
                name.c_str(), disabled_count);
    }
    // Try to write the last value we got while disabled
    if (connected && pending_value_set)
    {   // Assert we don't go back in time
        if (when >= last_stamp_in_archive)
        {
            LOG_MSG("'%s': re-enabled, writing the most recent value\n",
                    name.c_str());
            RawValue::setTime(pending_value, when);
            buffer.addRawValue(pending_value);
            last_stamp_in_archive = when;
        }
        pending_value_set = false;
    }
}

void ArchiveChannel::init(Guard &engine_guard, Guard &guard,
                          DbrType dbr_time_type, DbrCount nelements,
                          const CtrlInfo *ctrl_info,
                          const epicsTime *last_stamp)
{
    guard.check(mutex);
    this->dbr_time_type = dbr_time_type;
    this->nelements = nelements;
    buffer.allocate(dbr_time_type, nelements,
                    theEngine->suggestedBufferSize(engine_guard, period));
    if (pending_value)
        RawValue::free(pending_value);
    pending_value = RawValue::allocate(dbr_time_type, nelements, 1);
    pending_value_set = false;
    if (ctrl_info)
        this->ctrl_info = *ctrl_info;
    if (last_stamp)
        last_stamp_in_archive = *last_stamp;
}

void ArchiveChannel::write(Guard &guard, IndexFile &index)
{
    guard.check(mutex);
    size_t num_samples = buffer.getCount();
    if (num_samples <= 0)
        return;
    DataWriter writer(index,
                      name, ctrl_info,
                      dbr_time_type, nelements,
                      period,
                      num_samples);
    const RawValue::Data *value;
    while (num_samples-- > 0)
    {
        value = buffer.removeRawValue();
        if (!value)
        {
            LOG_MSG("'%s': Circular buffer empty while writing\n",
                    name.c_str());
            break;
        }
        if (! writer.add(value))
        {
            LOG_MSG("'%s': Couldn't write value\n",
                    name.c_str());
            break;
        }
    }
    buffer.resetOverwrites();
}

// CA callback for connects and disconnects
void ArchiveChannel::connection_handler(struct connection_handler_args arg)
{
    ArchiveChannel *me = (ArchiveChannel *) ca_puser(arg.chid);
    Guard guard(me->mutex);
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
        bool was_connected = me->connected;
        me->connected = false;
        me->connection_time = epicsTime::getCurrent();
        me->pending_value_set = false;
        if (was_connected  &&  me->mechanism)
            me->mechanism->handleConnectionChange(guard);
    }    
}

// Fill crtl_info from raw dbr_ctrl_xx data
bool ArchiveChannel::setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx)
{
    switch (type)
    {
        case DBR_CTRL_DOUBLE:
        {
            struct dbr_ctrl_double *ctrl =
                (struct dbr_ctrl_double *)dbr_ctrl_xx;
            ctrl_info.setNumeric(ctrl->precision, ctrl->units,
                                 ctrl->lower_disp_limit,
                                 ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,
                                 ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,
                                 ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_SHORT:
        {
            struct dbr_ctrl_int *ctrl = (struct dbr_ctrl_int *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit,
                                 ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,
                                 ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,
                                 ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_FLOAT:
        {
            struct dbr_ctrl_float *ctrl = (struct dbr_ctrl_float *)dbr_ctrl_xx;
            ctrl_info.setNumeric(ctrl->precision, ctrl->units,
                                 ctrl->lower_disp_limit,
                                 ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,
                                 ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,
                                 ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_CHAR:
        {
            struct dbr_ctrl_char *ctrl = (struct dbr_ctrl_char *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit,
                                 ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,
                                 ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,
                                 ctrl->upper_alarm_limit);
        }
        return true;
        case DBR_CTRL_LONG:
        {
            struct dbr_ctrl_long *ctrl = (struct dbr_ctrl_long *)dbr_ctrl_xx;
            ctrl_info.setNumeric(0, ctrl->units,
                                 ctrl->lower_disp_limit,
                                 ctrl->upper_disp_limit, 
                                 ctrl->lower_alarm_limit,
                                 ctrl->lower_warning_limit,
                                 ctrl->upper_warning_limit,
                                 ctrl->upper_alarm_limit);
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
    bool was_connected = me->connected;
    Guard guard(me->mutex);
    if (arg.status == ECA_NORMAL &&
        me->setup_ctrl_info(arg.type, arg.dbr))
    {
        LOG_MSG("%s: received control info\n", me->name.c_str());
        me->connection_time = epicsTime::getCurrent();
        Guard engine_guard(theEngine->mutex);
        me->init(engine_guard, guard, ca_field_type(arg.chid)+DBR_TIME_STRING,
                 ca_element_count(arg.chid));
        me->connected = true;
        if (was_connected == false   &&   me->mechanism)
            me->mechanism->handleConnectionChange(guard);
    }
    else
    {
        LOG_MSG("%s: ERROR, control_callback info request failed\n",
                me->name.c_str());
        me->connection_time = epicsTime::getCurrent();
        me->connected = false;
        me->pending_value_set = false;
        if (was_connected == true   &&   me->mechanism)
            me->mechanism->handleConnectionChange(guard);
    }
}

void ArchiveChannel::value_callback(struct event_handler_args args)
{
    ArchiveChannel *me = (ArchiveChannel *) args.usr;
    epicsTime now = epicsTime::getCurrent();
    const RawValue::Data *value = (const RawValue::Data *)args.dbr;
    Guard guard(me->mutex);
    if (me->mechanism)
        me->mechanism->handleValue(guard, now, value);
    me->handleDisabling(guard, value);
}

// called by SampleMechanism
void ArchiveChannel::handleDisabling(Guard &guard, const RawValue::Data *value)
{
    //guard.check(mutex);
    if (groups_to_disable.empty())
        return;
    // We disable if the channel is above zero
    bool criteria = RawValue::isAboveZero(dbr_time_type, value);
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
void ArchiveChannel::addEvent(Guard &guard,
                              dbr_short_t status, dbr_short_t severity,
                              const epicsTime &event_time)
{
    guard.check(mutex);
    if (nelements <= 0)
    {
        LOG_MSG("'%s': Cannot add event because data type is unknown\n",
                name.c_str());
        return;
    }
    RawValue::Data *value = buffer.getNextElement();
    memset(value, 0, RawValue::getSize(dbr_time_type, nelements));
    RawValue::setStatus(value, status, severity);
    if (event_time < last_stamp_in_archive)
    {   // adjust time, event has to be added to archive
        RawValue::setTime(value, last_stamp_in_archive);
    }
    else
    {
        last_stamp_in_archive = event_time;
        RawValue::setTime(value, event_time);
    }
}
