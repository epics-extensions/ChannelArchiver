// Tools
#include <ToolsConfig.h>
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
#include <epicsTimeHelper.h>
// Storage
#include <RawValue.h>
// Local
#include "EngineLocks.h"
#include "ProcessVariable.h"

static ThrottledMsgLogger nulltime_throttle("Null-Timestamp", 60.0);
static ThrottledMsgLogger nanosecond_throttle("Bad Nanoseconds", 60.0);

//#define DEBUG_PV

ProcessVariable::ProcessVariable(ProcessVariableContext &ctx, const char *name)
    : NamedBase(name),
      mutex(name, EngineLocks::ProcessVariable),
      ctx(ctx),
      state(INIT),
      id(0),
      ev_id(0),
      dbr_type(DBR_TIME_DOUBLE),
      dbr_count(1),
      outstanding_gets(0),
      subscribed(false)
{
#   ifdef DEBUG_PV
    printf("new ProcessVariable(%s)\n", getName().c_str());
#   endif
    {
        Guard ctx_guard(__FILE__, __LINE__, ctx);
        ctx.incRef(ctx_guard);
    }
    // Set some default info to match the default getDbrType/Count
    ctrl_info.setNumeric(0, "?", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
}                      

ProcessVariable::~ProcessVariable()
{
    if (state != INIT)
    {
        LOG_MSG("ProcessVariable(%s) destroyed without stopping!\n",
                getName().c_str());
        // Bad situation.
        // If you don't clear the channel, CA might still invoke callbacks
        // for a ProcessVariable instance that is no more.
        // On the other hand, who knows if the 'id' is valid
        // and we won't crash in ca_clear_channel?
        // We assume that the id _is_ valid, so protect against further
        // callbacks. ca_clear_channel should remove all subscriptions.
        if (id != 0)
        {
            ca_clear_channel(id);
            id = 0;
        }
    }
    if (!state_listeners.isEmpty())
    {
        LOG_MSG("ProcessVariable(%s) still has %zu state listeners\n",
                getName().c_str(), state_listeners.size());
        return;
    }
    if (!value_listeners.isEmpty())
    {
        LOG_MSG("ProcessVariable(%s) still has %zu value listeners\n",
                getName().c_str(), value_listeners.size());
        return;
    }
    try
    {
        Guard ctx_guard(__FILE__, __LINE__, ctx);
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

OrderedMutex &ProcessVariable::getMutex()
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
    chid _id = id;
    if (_id == 0)
        return "Not Initialized";
    enum channel_state cs;
    {   // Unlock while dealing with CAC.
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            LOG_ASSERT(ctx.isAttached(ctx_guard));
        }
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

void ProcessVariable::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
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
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            LOG_ASSERT(ctx.isAttached(ctx_guard));
            try
            {
       	        status = ca_create_channel(getName().c_str(),
                                           connection_handler,
                                           this, CA_PRIORITY_ARCHIVE, &_id);
            }
            catch (std::exception &e)
            {
                LOG_MSG("ProcessVariable::start(%s): %s\n",
                        getName().c_str(), e.what());
            }
            catch (...)
            {
                LOG_MSG("ProcessVariable::start(%s): Unknown Exception\n",
                        getName().c_str());
            }                          
            ctx.requestFlush(ctx_guard);
        }
    }
    id = _id;
    if (status != ECA_NORMAL)
        LOG_MSG("'%s': ca_create_channel failed, status %s\n",
                getName().c_str(), ca_message(status));
}

bool ProcessVariable::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    return id != 0;
}

void ProcessVariable::getValue(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (state != CONNECTED)
        return; // Can't get
    ++outstanding_gets;
    chid _id = id;
    GuardRelease release(__FILE__, __LINE__, guard); // Unlock while in CAC.
    {
        Guard ctx_guard(__FILE__, __LINE__, ctx);
        LOG_ASSERT(ctx.isAttached(ctx_guard));
        int status;
        try
        {
            status = ca_array_get_callback(dbr_type, dbr_count,
                                           _id, value_callback, this);
        }
        catch (std::exception &e)
        {
            LOG_MSG("ProcessVariable::getValue(%s): %s\n",
                    getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ProcessVariable::getValue(%s): Unknown Exception\n",
                    getName().c_str());
        }                          
        if (status != ECA_NORMAL)
        {
            LOG_MSG("%s: ca_array_get_callback failed: %s\n",
                    getName().c_str(), ca_message(status));
            return;
        }
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
    chid _id = id;
    if (_id == 0)
    {
        LOG_MSG("Skipped subscription to %s, already stopped\n",
                getName().c_str());
        return;
    }
    evid     _ev_id = 0;
    DbrType  _type  = dbr_type;
    DbrCount _count = dbr_count;
    {   // Release around CA call.
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            LOG_ASSERT(ctx.isAttached(ctx_guard));
            int status;
            try
            {
                status = ca_create_subscription(_type, _count, _id,
                                                DBE_LOG | DBE_ALARM,
                                                value_callback, this, &_ev_id);
            }
            catch (std::exception &e)
            {
                LOG_MSG("ProcessVariable::subscribe(%s): %s\n",
                        getName().c_str(), e.what());
            }
            catch (...)
            {
                LOG_MSG("ProcessVariable::subscribe(%s): Unknown Exception\n",
                        getName().c_str());
            } 
            if (status != ECA_NORMAL)
            {
                LOG_MSG("%s: ca_create_subscription failed: %s\n",
                        getName().c_str(), ca_message(status));
                return;
            }
            ctx.requestFlush(ctx_guard);
        }
    }
    ev_id = _ev_id;
    LOG_ASSERT(ev_id != 0);
    subscribed = true;
}

void ProcessVariable::unsubscribe(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (subscribed)
    {
        subscribed = false;
        evid _ev_id = ev_id;
        ev_id = 0;
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            LOG_ASSERT(ctx.isAttached(ctx_guard));
        }
        try
        {
            ca_clear_subscription(_ev_id);
        }
        catch (std::exception &e)
        {
            LOG_MSG("ProcessVariable::unsubscribe(%s): %s\n",
                    getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ProcessVariable::unsubscribe(%s): Unknown Exception\n",
                    getName().c_str());
        }    
    }    
}

void ProcessVariable::stop(Guard &guard)
{
#   ifdef DEBUG_PV
    printf("stop ProcessVariable(%s)\n", getName().c_str());
#   endif
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isRunning(guard));
    unsubscribe(guard);
    // Set all indicators back to INIT state
    chid _id = id;
    id = 0;
    bool was_connected = (state == CONNECTED);
    state = INIT;
    // Then unlock around CA lib. calls to prevent deadlocks.
    {
        GuardRelease release(__FILE__, __LINE__, guard);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            LOG_ASSERT(ctx.isAttached(ctx_guard));
        }
        try
        {
            ca_clear_channel(_id);
        }
        catch (std::exception &e)
        {
            LOG_MSG("ProcessVariable::stop(%s): %s\n",
                    getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ProcessVariable::stop(%s): Unknown Exception\n",
                    getName().c_str());
        }
        // If there are listeners, tell them that we are disconnected.
        if (was_connected)
            firePvDisconnected();
    }
}

// Channel Access callback
void ProcessVariable::connection_handler(struct connection_handler_args arg)
{
    ProcessVariable *me = (ProcessVariable *) ca_puser(arg.chid);
    LOG_ASSERT(me != 0);
    try
    {
        chid _id;
        {
            if (arg.op == CA_OP_CONN_DOWN)
            {   
                {
                    // Connection is down
                    Guard guard(__FILE__, __LINE__, *me);
                    me->state = DISCONNECTED;
                }
                me->firePvDisconnected();
                return;
            }
            // else: Connection is 'up'
            Guard guard(__FILE__, __LINE__, *me);
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
        int status;
        try
        {
            status = ca_array_get_callback(ca_field_type(_id)+DBR_CTRL_STRING,
                                           1 /* ca_element_count(me->ch_id) */,
                                           _id, control_callback, me);
        }
        catch (std::exception &e)
        {
            LOG_MSG("ProcessVariable::connection_handler(%s): %s\n",
                    me->getName().c_str(), e.what());
        }
        catch (...)
        {
            LOG_MSG("ProcessVariable::connection_handler(%s): "
                    "Unknown Exception\n", me->getName().c_str());
        } 
        if (status != ECA_NORMAL)
        {
            LOG_MSG("ProcessVariable('%s') connection_handler error %s\n",
                    me->getName().c_str(), ca_message (status));
            return;
        }
        Guard ctx_guard(__FILE__, __LINE__, me->ctx);
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

// Channel Access callback
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
        Guard guard(__FILE__, __LINE__, *me);
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
        {
            Guard guard(__FILE__, __LINE__, *me);    
            // Setup the PV info.
            if (!me->setup_ctrl_info(guard, arg.type, arg.dbr))
                return;
            me->dbr_type  = _type;
            me->dbr_count = _count;
            me->state     = CONNECTED;
        }
        // Notify listeners that PV is now fully connected.
        me->firePvConnected();
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) control_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

// Channel Access callback
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
        {
            // Check if we expected a value at all
            Guard guard(__FILE__, __LINE__, *me);
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
        }
        // Notify listeners of new value
        me->firePvValue(value);
    }
    catch (GenericException &e)
    {
        LOG_MSG("ProcessVariable(%s) value_callback exception:\n%s\n",
                me->getName().c_str(), e.what());
    }
}

void ProcessVariable::firePvConnected()
{
    epicsTime now = epicsTime::getCurrent();
    ConcurrentListIterator<ProcessVariableStateListener>
        l(state_listeners.iterator());
    ProcessVariableStateListener *listener;
    while ((listener = l.next()) != 0)
        listener->pvConnected(*this, now);
}

void ProcessVariable::firePvDisconnected()
{
    epicsTime now = epicsTime::getCurrent();
    ConcurrentListIterator<ProcessVariableStateListener>
        l(state_listeners.iterator());
    ProcessVariableStateListener *listener;
    while ((listener = l.next()) != 0)
        listener->pvDisconnected(*this, now);
}

void ProcessVariable::firePvValue(const RawValue::Data *value)
{
    ConcurrentListIterator<ProcessVariableValueListener>
        l(value_listeners.iterator());
    ProcessVariableValueListener *listener;
    while ((listener = l.next()) != 0)
        listener->pvValue(*this, value);
}

