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

#define DEBUG_PV

// TODO: You just cannot add/remove listeners while running.

ProcessVariable::ProcessVariable(ProcessVariableContext &ctx, const char *name)
    : NamedBase(name), ctx(ctx), state(INIT),
      id(0), ev_id(0), dbr_type(DBR_TIME_DOUBLE), dbr_count(1),
      outstanding_gets(0), subscribed(false)
{
#   ifdef DEBUG_PV
    printf("new ProcessVariable(%s)\n", getName().c_str());
#   endif
    {
        Guard ctx_guard(ctx);
        ctx.incRef(ctx_guard);
    }
    // Set some default t match the default getDbrType/Count
    ctrl_info.setNumeric(0, "?", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
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
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
    if (id == 0)
        return "Not Initialized";
    enum channel_state cs;
    chid _id = id;
    {   // Unlock while dealing with CAC.
        GuardRelease release(guard);
        cs = ca_state(_id);
    }
    switch (cs)
    {
    case cs_never_conn: return "Never Conn.";
    case cs_prev_conn:  return "Prev. Conn.";
    case cs_conn:       return "Connected";
    case cs_closed:     return "Closed";
    default:            return "unknown";
    }
}

void ProcessVariable::addStateListener(
    Guard &guard, ProcessVariableStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(!isRunning(guard));    
    stdList<ProcessVariableStateListener *>::iterator l;
    for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
    {
        if (*l == listener)
            throw GenericException(__FILE__, __LINE__,
                                   "Duplicate listener for '%s'",
                                   getName().c_str());
    }
    state_listeners.push_back(listener);                              
}

void ProcessVariable::removeStateListener(
    Guard &guard, ProcessVariableStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);  
    LOG_ASSERT(!isRunning(guard));    
    stdList<ProcessVariableStateListener *>::iterator l;
    for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
    {
        if (*l == listener)
        {
            state_listeners.erase(l);
            return;
        }
    }
    throw GenericException(__FILE__, __LINE__, "Unknown listener for '%s'",
                           getName().c_str());
}

void ProcessVariable::addValueListener(
    Guard &guard, ProcessVariableValueListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(!isRunning(guard));    
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
    LOG_ASSERT(!isRunning(guard));    
    stdList<ProcessVariableValueListener *>::iterator l;
    for (l = value_listeners.begin(); l != value_listeners.end(); ++l)
    {
        if (*l == listener)
        {
            value_listeners.erase(l);
            return;
        }
    }
    throw GenericException(__FILE__, __LINE__, "Unknown listener for '%s'",
                           getName().c_str());
}

void ProcessVariable::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
#   ifdef DEBUG_PV
    printf("start ProcessVariable(%s)\n", getName().c_str());
#   endif
    LOG_ASSERT(! isRunning(guard));
    LOG_ASSERT(state == INIT);
    state = DISCONNECTED;
    // Unlock around CA lib. calls to prevent deadlocks in callbacks.
    int status;
    chid _id;
    {
        GuardRelease release(guard);
   	    status = ca_create_channel(getName().c_str(), connection_handler,
                                   this, CA_PRIORITY_ARCHIVE, &_id);
    }
    id = _id;
    if (status != ECA_NORMAL)
    {
        LOG_MSG("'%s': ca_create_channel failed, status %s\n",
                getName().c_str(), ca_message(status));
        return;
    }
    {   // Lock Order: PV, PV ctx.
        Guard ctx_guard(ctx);
        ctx.requestFlush(ctx_guard);
    }
}

bool ProcessVariable::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    return id != 0;
}

void ProcessVariable::getValue(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
    if (state != CONNECTED)
        return; // Can't get
    ++outstanding_gets;
    chid _id = id;
    GuardRelease release(guard); // Unlock while in CAC.
    int status = ca_array_get_callback(dbr_type, dbr_count,
                                       _id, value_callback, this);
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
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
    if (dbr_type == 0)
        throw GenericException(__FILE__, __LINE__,
                               "Cannot subscribe to %s, never connected",
                               getName().c_str());
    // Prevent multiple subscriptions
    if (subscribed)
        return;
    evid _ev_id;
    DbrType _type = dbr_type;
    DbrCount _count = dbr_count;
    {   // Release around CA call.
        chid _id = id;
        GuardRelease release(guard);
        int status = ca_create_subscription(_type, _count, _id,
                                            DBE_LOG | DBE_ALARM,
                                            value_callback, this, &_ev_id);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("%s: ca_create_subscription failed: %s\n",
                    getName().c_str(), ca_message(status));
            return;
        }
    }
    ev_id = _ev_id;
    subscribed = true;
    // At this point, PV is locked. Locking ctx is OK with locking order.
    Guard ctx_guard(ctx);
    ctx.requestFlush(ctx_guard);
}

void ProcessVariable::unsubscribe(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
    if (subscribed)
    {
        subscribed = false;
        evid _ev_id = ev_id;
        ev_id = 0;
        GuardRelease release(guard);
        ca_clear_subscription(_ev_id);
    }    
}

void ProcessVariable::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    {
        Guard ctx_guard(ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
    }
#   ifdef DEBUG_PV
    printf("stop ProcessVariable(%s)\n", getName().c_str());
#   endif
    LOG_ASSERT(isRunning(guard));
    unsubscribe(guard);
    // While locked, set all indicators back to INIT state
    chid _id = id;
    id = 0;
    bool was_connected = state == CONNECTED;
    state = INIT;
    // Then unlock around CA lib. calls to prevent deadlocks.
    {
        GuardRelease release(guard);
        ca_clear_channel(_id);
    }
    // If there are listeners, tell them that we are disconnected.
    if (was_connected)
        firePvDisconnected(guard);
}

void ProcessVariable::connection_handler(struct connection_handler_args arg)
{
    ProcessVariable *me = (ProcessVariable *) ca_puser(arg.chid);
    LOG_ASSERT(me != 0);
    try
    {
        chid _id;
        {
            Guard guard(*me);
            if (arg.op == CA_OP_CONN_DOWN)
            {   // Connection is down
                me->state = DISCONNECTED;
                me->firePvDisconnected(guard);
                return;
            }
            // else: Connection is 'up'
            _id = me->id;
            if (_id == 0)
            {
                LOG_MSG("ProcessVariable(%s) received "
                        "unexpected connection_handler\n",
                        me->getName().c_str());
                return;
            }
#           ifdef DEBUG_PV
            printf("ProcessVariable(%s) getting control info\n",
                   me->getName().c_str());
#           endif
            me->state = GETTING_INFO;
        }
        // Get control information for this channel.
        // Unlocked while in CAC.
        // Bug in (at least older) CA: Works only for 1 element,
        // even for array channels.
        int status = ca_array_get_callback(ca_field_type(_id)+DBR_CTRL_STRING,
                                           1 /* ca_element_count(me->ch_id) */,
                                           _id, control_callback, me);
        if (status != ECA_NORMAL)
        {
            LOG_MSG("ProcessVariable('%s') connection_handler error %s\n",
                    me->getName().c_str(), ca_message (status));
            return;
        }
        Guard ctx_guard(me->ctx);
        me->ctx.requestFlush(ctx_guard);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) control_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }    
}

// Fill crtl_info from raw dbr_ctrl_xx data
bool ProcessVariable::setup_ctrl_info(Guard &guard,
                                      DbrType type, const void *dbr_ctrl_xx)
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

// Should be invoked with the control info 'get' data,
// requested in the connection handler.
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
    chid _id;
    {
        Guard guard(*me);
        if (me->state != GETTING_INFO)
        {
            LOG_MSG("ProcessVariable(%s) received control_callback while %s\n",
                    me->getName().c_str(), me->getStateStr(guard));
            return;
        }
        _id = me->id;
    }    
    // For native type DBR_xx, use DBR_TIME_xx, and native count:
    DbrType _type   = ca_field_type(_id) + DBR_TIME_STRING;
    DbrCount _count = ca_element_count(_id);
    try
    {
        Guard guard(*me);    
        // Setup the PV info.
        if (!me->setup_ctrl_info(guard, arg.type, arg.dbr))
            return;
        me->dbr_type  = _type;
        me->dbr_count = _count;
        me->state     = CONNECTED;
        // Notify listeners that PV is now fully connected.
        me->firePvConnected(guard);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) control_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

// TODO Check context::isRunning in all callbacks


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
        {
            LOG_MSG("ProcessVariable(%s) received unexpected value_callback\n",
                    me->getName().c_str());
            return;
        }
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
        me->firePvValue(guard, value);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) value_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

void ProcessVariable::firePvConnected(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);    
    LOG_ASSERT(isRunning(guard));
    if (state_listeners.empty())
    {
        LOG_MSG("ProcessVariable(%s) Connected\n", getName().c_str());
        return;
    }
    epicsTime now = epicsTime::getCurrent();
    stdList<ProcessVariableStateListener *>::iterator l;
    for (l = state_listeners.begin();  l != state_listeners.end();  ++l)
        (*l)->pvConnected(guard, *this, now);
}

void ProcessVariable::firePvDisconnected(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (state_listeners.empty())
    {
        LOG_MSG("ProcessVariable(%s) is disconnected\n",
                getName().c_str());
        return;
    }
    epicsTime now = epicsTime::getCurrent();
    stdList<ProcessVariableStateListener *>::iterator l;
    for (l = state_listeners.begin();  l != state_listeners.end();  ++l)
        (*l)->pvDisconnected(guard, *this, now);
}

void ProcessVariable::firePvValue(Guard &guard, const RawValue::Data *value)
{
    if (value_listeners.empty())
    {
        LOG_MSG("ProcessVariable(%s) value\n", getName().c_str());
        return;
    }
    stdList<ProcessVariableValueListener *>::iterator l;
    for (l = value_listeners.begin();  l != value_listeners.end();  ++l)
        (*l)->pvValue(guard, *this, value);
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

