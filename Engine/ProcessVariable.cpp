// Tools
#include <ToolsConfig.h>
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
#include <epicsTimeHelper.h>
// Storage
#include <RawValue.h>
// Local
#include "ProcessVariable.h"

static ThrottledMsgLogger nulltime_throttle("Null-Timestamp", 60.0);
static ThrottledMsgLogger nanosecond_throttle("Bad Nanoseconds", 60.0);

//#define DEBUG_PV

ProcessVariable::ProcessVariable(ProcessVariableContext &ctx, const char *name)
    : NamedBase(name), ctx(ctx), state(INIT),
      id(0), ev_id(0), dbr_type(0), dbr_count(0),
      outstanding_gets(0), subscribed(false)
{
#   ifdef DEBUG_PV
    printf("new ProcessVariable(%s)\n", this->getName().c_str());
#   endif
    {
        Guard ctx_guard(ctx);
        ctx.incRef(ctx_guard);
    }
}

ProcessVariable::~ProcessVariable()
{
    if (state != INIT)
        LOG_MSG("ProcessVariable(%s) destroyed without stopping!\n",
                getName().c_str());
                
    if (!state_listeners.empty())
    {
        LOG_MSG("ProcessVariable(%s) still has %zu state listeners\n",
                getName().c_str(), state_listeners.size());
        return;
    }
    if (!value_listeners.empty())
    {
        LOG_MSG("ProcessVariable(%s) still has %zu value listeners\n",
                getName().c_str(), value_listeners.size());
        return;
    }
                
    try
    {
        Guard ctx_guard(ctx);
        ctx.decRef(ctx_guard);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) destructor: %s\n",
                getName().c_str(), e.what());
    }
#   ifdef DEBUG_PV
    printf("deleted ProcessVariable(%s)\n", getName().c_str());
#   endif
}

epicsMutex &ProcessVariable::getMutex()
{
    return mutex;
}

ProcessVariable::State ProcessVariable::getState(Guard &guard) const
{
    guard.check(__FILE__, __LINE__, mutex);
    return state;
}

const char *ProcessVariable::getStateStr(Guard &guard) const
{
    guard.check(__FILE__, __LINE__, mutex);
    switch (state)
    {
    case INIT:          return "INIT";
    case DISCONNECTED:  return "DISCONNECTED";
    case GETTING_INFO:  return "GETTING_INFO";
    case CONNECTED:     return "CONNECTED";
    default:            return "<unknown>";
    }
}

const char *ProcessVariable::getCAStateStr(Guard &guard) const
{
    guard.check(__FILE__, __LINE__, mutex);
    if (id != 0)
    {
        switch (ca_state(id))
        {
        case cs_never_conn: return "Never Conn.";
        case cs_prev_conn:  return "Prev. Conn.";
        case cs_conn:       return "Connected";
        case cs_closed:     return "Closed";
        default:            return "unknown";
        }
    }
    return "Not Initialized";
}

void ProcessVariable::addStateListener(
    Guard &guard, ProcessVariableStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);    
    stdList<ProcessVariableStateListener *>::iterator l;
    for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
        if (*l == listener)
            throw GenericException(__FILE__, __LINE__,
                                   "Duplicate listener for '%s'",
                                   getName().c_str());
    state_listeners.push_back(listener);                              
    //LOG_MSG("PV '%s' adds State listener 0x%lX, total %zu\n",
    //        getName().c_str(), (unsigned long)listener,
    //        state_listeners.size());
}

void ProcessVariable::removeStateListener(
    Guard &guard, ProcessVariableStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);  
    size_t old = state_listeners.size();
    state_listeners.remove(listener);                              
    if (state_listeners.size() + 1  !=  old)
        throw GenericException(__FILE__, __LINE__,
                               "Unknown listener for '%s'",
                               getName().c_str());
    //LOG_MSG("PV '%s' removes State listener 0x%lX, total %zu\n",
    //        getName().c_str(), (unsigned long)listener,
    //        state_listeners.size());
}

void ProcessVariable::addValueListener(
    Guard &guard, ProcessVariableValueListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    stdList<ProcessVariableValueListener *>::iterator l;
    for (l = value_listeners.begin(); l != value_listeners.end(); ++l)
        if (*l == listener)
            throw GenericException(__FILE__, __LINE__,
                                   "Duplicate listener for '%s'", getName().c_str());
    value_listeners.push_back(listener);                              
}

void ProcessVariable::removeValueListener(
    Guard &guard, ProcessVariableValueListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    value_listeners.remove(listener);                              
}

void ProcessVariable::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (id != 0)
        throw GenericException(__FILE__, __LINE__,
                               "Duplicate start(%s)", getName().c_str());
#   ifdef DEBUG_PV
    printf("start ProcessVariable(%s)\n", getName().c_str());
#   endif
    state = DISCONNECTED;
    // Unlock around CA lib. calls to prevent deadlocks in callbacks!
    GuardRelease release(guard);
   	int status = ca_create_channel(getName().c_str(), connection_handler,
                                   this, CA_PRIORITY_ARCHIVE, &id);
    if (status != ECA_NORMAL)
    {
        LOG_MSG("'%s': ca_create_channel failed, status %s\n",
                getName().c_str(), ca_message(status));
        return;
    }
    {   // Lock ctx while PV is unlocked.
        Guard ctx_guard(ctx);
        ctx.requestFlush(ctx_guard);
    }
}

void ProcessVariable::getValue(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (state != CONNECTED)
        return; // Can't get
    ++outstanding_gets;
    GuardRelease release(guard);
    int status = ca_array_get_callback(dbr_type, dbr_count,
                                       id, value_callback, this);
    if (status != ECA_NORMAL)
    {
        LOG_MSG("%s: ca_array_get_callback failed: %s\n",
                getName().c_str(), ca_message(status));
        return;
    }
    {   // Lock ctx while PV is unlocked.
        Guard ctx_guard(ctx);
        ctx.requestFlush(ctx_guard);
    }    
}

void ProcessVariable::subscribe(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (dbr_type == 0)
        throw GenericException(__FILE__, __LINE__,
                               "Cannot subscribe to %s, never connected",
                               getName().c_str());
    // Prevent multiple subscriptions
    if (subscribed)
        return;
    {   // Release around CA call.
        GuardRelease release(guard);
        int status = ca_create_subscription(dbr_type, dbr_count, id,
                                            DBE_LOG | DBE_ALARM,
                                            value_callback, this, &ev_id);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("%s: ca_create_subscription failed: %s\n",
                    getName().c_str(), ca_message(status));
            return;
        }
    }
    subscribed = true;
    // At this point, PV is locked. Locking ctx is OK with locking order.
    Guard ctx_guard(ctx);
    ctx.requestFlush(ctx_guard);
}

void ProcessVariable::unsubscribe(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (subscribed)
    {
        subscribed = false;
        evid cpy = ev_id;
        ev_id = 0;
        GuardRelease release(guard);
        ca_clear_subscription(cpy);
    }    
}

void ProcessVariable::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (id == 0)
        throw GenericException(__FILE__, __LINE__,
                               "Stop without start(%s)", getName().c_str());
#   ifdef DEBUG_PV
    printf("stop ProcessVariable(%s)\n", getName().c_str());
#   endif
    unsubscribe(guard);
    // While locked, set all indicators back to INIT state
    chid to_del = id;
    id = 0;
    state = INIT;
    // Then unlock around CA lib. calls to prevent deadlocks.
    {
        GuardRelease release(guard);
        ca_clear_channel(to_del);
    }
}

void ProcessVariable::connection_handler(struct connection_handler_args arg)
{
    ProcessVariable *me = (ProcessVariable *) ca_puser(arg.chid);
    LOG_ASSERT(me != 0);
    try
    {
        Guard guard(*me);
        if (arg.op == CA_OP_CONN_DOWN)
        {   // Connection is down
            me->state = DISCONNECTED;
            if (me->state_listeners.empty())
            {
                LOG_MSG("ProcessVariable(%s) is disconnected\n",
                        me->getName().c_str());
                return;
            }
            epicsTime now = epicsTime::getCurrent();
            stdList<ProcessVariableStateListener *>::iterator l;
            for (l = me->state_listeners.begin();
                 l != me->state_listeners.end();
                 ++l)
                (*l)->pvDisconnected(guard, *me, now);
            return;
        }
        // else: Connection is 'up'
    #   ifdef DEBUG_PV
        printf("ProcessVariable(%s) getting control info\n",
               me->getName().c_str());
    #   endif
        me->state = GETTING_INFO;
        // Get control information for this channel.
        {
            GuardRelease release(guard);
            int status = ca_array_get_callback(
                ca_field_type(me->id)+DBR_CTRL_STRING,
                1 /* ca_element_count(me->ch_id) */,
                me->id, control_callback, me);
            if (status != ECA_NORMAL)
            {
                LOG_MSG("ProcessVariable('%s') connection_handler error %s\n",
                        me->getName().c_str(), ca_message (status));
                return;
            }
            {
                Guard ctx_guard(me->ctx);
                me->ctx.requestFlush(ctx_guard);
            }
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) control_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }    
}

// Fill crtl_info from raw dbr_ctrl_xx data
bool ProcessVariable::setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx)
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
    LOG_MSG("ProcessVariable(%s) setup_ctrl_info cannot handle type %d\n",
            getName().c_str(), type);
    return false;
}

void ProcessVariable::control_callback(struct event_handler_args arg)
{
    ProcessVariable *me = (ProcessVariable *) ca_puser(arg.chid);
    LOG_ASSERT(me != 0);
    if (arg.status != ECA_NORMAL)
    {
        LOG_MSG("ProcessVariable(%s) control_callback failed: %s\n",
                me->getName().c_str(),  ca_message(arg.status));
        return;
    }
    try
    {
        // Should be invoked with the control info 'get' data,
        // requested in the connection handler.
        Guard guard(*me);    
        if (me->state != GETTING_INFO)
        {
            LOG_MSG("ProcessVariable(%s) received control_callback while %s\n",
                    me->getName().c_str(), me->getStateStr(guard));
            return;
        }
        // Setup the PV info.
        if (!me->setup_ctrl_info(arg.type, arg.dbr))
            return;
        me->dbr_type  = ca_field_type(me->id)+DBR_TIME_STRING;
        me->dbr_count = ca_element_count(me->id);
        me->state     = CONNECTED;
        // Notify listeners that PV is now fully connected.
        if (me->state_listeners.empty())
        {
            LOG_MSG("ProcessVariable(%s) Connected\n", me->getName().c_str());
            return;
        }
        epicsTime now = epicsTime::getCurrent();
        stdList<ProcessVariableStateListener *>::iterator l;
        for (l = me->state_listeners.begin(); l != me->state_listeners.end(); ++l)
            (*l)->pvConnected(guard, *me, now);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) control_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

// Each subscription monitor or 'get' callback goes here:
void ProcessVariable::value_callback(struct event_handler_args arg)
{
    ProcessVariable *me = (ProcessVariable *) ca_puser(arg.chid);
    LOG_ASSERT(me != 0);
    // Check if there is a useful value at all.
    if (arg.status != ECA_NORMAL)
    {
        LOG_MSG("ProcessVariable(%s) value_callback failed: %s\n",
                me->getName().c_str(),  ca_message(arg.status));
        return;
    }
    const RawValue::Data *value = (const RawValue::Data *)arg.dbr;
    LOG_ASSERT(value != 0);
    // Catch records that never processed and give null times.
    if (value->stamp.secPastEpoch == 0)
    {
        nulltime_throttle.LOG_MSG("ProcessVariable::value_callback(%s) "
            "with invalid null timestamp\n", me->getName().c_str());
        return;
    }
    // Check the nanoseconds of each incoming value. Problems will arise later
    // unless they are normalized to less than one second.
    if (value->stamp.nsec >= 1000000000L)
    {
        epicsTimeStamp s = value->stamp;
        s.nsec = 0;
        epicsTime t = s;
        stdString txt;
        epicsTime2string(t, txt);
        nanosecond_throttle.LOG_MSG("ProcessVariable::value_callback(%s) "
            "with invalid secs/nsecs %zu, %zu: %s\n",
            me->getName().c_str(),
            (size_t) value->stamp.secPastEpoch,
            (size_t) value->stamp.nsec,
            txt.c_str());
        return;
    }
    try
    {
        // Check if we expected a value at all
        Guard guard(*me);
        if (me->outstanding_gets <= 0  &&  !me->subscribed)
            LOG_MSG("ProcessVariable(%s) received unexpected value_callback\n",
                    me->getName().c_str());
        if (me->outstanding_gets > 0)
            --me->outstanding_gets;
        if (me->state != CONNECTED)
        {
            // After a disconnect, this can happen in the GETTING_INFO state:
            // The CAC lib already sends us new monitors after the re-connect,
            // while we wait for the ctrl-info get_callback to finish.
            if (me->state == GETTING_INFO) // ignore
                return;
            LOG_MSG("ProcessVariable(%s) received value_callback while %s\n",
                    me->getName().c_str(), me->getStateStr(guard));
            return;
        }
        // Notify listeners of new value
        if (me->value_listeners.empty())
        {
            LOG_MSG("ProcessVariable(%s) value\n", me->getName().c_str());
            return;
        }
        stdList<ProcessVariableValueListener *>::iterator l;
        for (l = me->value_listeners.begin(); l != me->value_listeners.end(); ++l)
            (*l)->pvValue(guard, *me, value);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) value_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

#if 0
//  TODO: Dump CA client lib info to file on demand.
    if (info_dump_file.length() > 0)
    {
        int out = open(info_dump_file.c_str(), O_CREAT|O_WRONLY, 0x777);
        info_dump_file.assign(0, 0);
        if (out >= 0)
        {
            int oldout = dup(1);
            dup2(out, 1);
            ca_client_status(10);
            dup2(oldout, 1);
        }
    }
#endif

