// Tools
#include <MsgLogger.h>
// Engine
#include "DisableFilter.h"

#define DEBUG_DISABLE_FILT

DisableFilter::DisableFilter(ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      is_disabled(false), is_connected(false), was_connected(false)
{
}

DisableFilter::~DisableFilter()
{
}

void DisableFilter::disable(Guard &guard)
{
    is_disabled = true;
    was_connected = is_connected;
}

void DisableFilter::enable(Guard &guard, ProcessVariable &pv,
                           const epicsTime &when)
{
    is_disabled = false;
    // Bring listener up to date on connection status
    if (was_connected  &&  !is_connected)
        ProcessVariableFilter::pvDisconnected(guard, pv, when);
    else if (!was_connected  &&  is_connected)
        ProcessVariableFilter::pvConnected(guard, pv, when);
    // If there is a buffered value, send that to listener
    if (last_value)
    {
        RawValue::setTime(last_value, when);
        ProcessVariableFilter::pvValue(guard, pv, last_value);
        last_value = 0;
    }
}

void DisableFilter::pvConnected(Guard &guard, ProcessVariable &pv,
                                const epicsTime &when)
{
    is_connected = true;
    // Suppress while disabled
    if (! is_disabled)
        ProcessVariableFilter::pvConnected(guard, pv, when);
}

void DisableFilter::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                  const epicsTime &when)
{
    is_connected = false;
    // When disabled, this means that any
    if (is_disabled)
        last_value = 0;
    else
        ProcessVariableFilter::pvDisconnected(guard, pv, when);
}

void DisableFilter::pvValue(Guard &guard, ProcessVariable &pv,
                           const RawValue::Data *data)
{
    if (is_disabled)
    {   // Remember value
        DbrType type = pv.getDbrType(guard);
        DbrCount count = pv.getDbrCount(guard);
        if (! last_value)
            last_value = RawValue::allocate(type, count, 1);
        LOG_ASSERT(last_value);
        RawValue::copy(type, count, last_value, data);
    }
    else // and pass on to listener.
        ProcessVariableFilter::pvValue(guard, pv, data);
}
