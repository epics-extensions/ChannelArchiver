#include "epicsTimeHelper.h"
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
    disabled = false;
    currently_disabling = false;
}

ArchiveChannel::~ArchiveChannel()
{
    if (chid_valid)
    {
        ca_clear_channel(ch_id);
        chid_valid = false;
    }
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
    theEngine->needCAflush();
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
        theEngine->needCAflush();
    }
    else
    {
        me->connected = false;
        me->connection_time = epicsTime::getCurrent();
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
        me->connected = true;
        me->mechanism->handleConnectionChange();
    }
    else
    {
        LOG_MSG("%s: ERROR, control info request failed\n", me->name.c_str());
        me->connected = false;
        me->connection_time = epicsTime::getCurrent();
        me->mechanism->handleConnectionChange();
    }
    me->mutex.unlock();
}

void ArchiveChannel::handleDisabling(const RawValue::Data *value)
{
    if (disabling.empty())
        return;

    // We disable if the channel is non-zero
    bool criteria = RawValue::isZero(dbr_time_type, value) == false;
    if (criteria && !currently_disabling)
    {   // wasn't disabling -> disabling
        currently_disabling = true;
        stdList<GroupInfo *>::iterator g;
        for (g=groups.begin(); g!=groups.end(); ++g)
        {
            if (disabling.test((*g)->getID()))
                (*g)->disable(this);
        }
    }
    else if (!criteria && currently_disabling)
    {   // was disabling -> enabling
        stdList<GroupInfo *>::iterator g;
        for (g=groups.begin(); g!=groups.end(); ++g)
        {
            if (disabling.test((*g)->getID()))
                (*g)->enable(this);
        }
        currently_disabling = false;
    }
}
