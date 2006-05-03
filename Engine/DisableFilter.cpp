// Tools
#include <MsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "DisableFilter.h"

// #define DEBUG_DISABLE_FILT

DisableFilter::DisableFilter(ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      mutex("DisableFilter", EngineLocks::DisableFilter),
      is_disabled(false),
      is_connected(false)
{
}

DisableFilter::~DisableFilter()
{
}

void DisableFilter::disable()
{
    Guard guard(__FILE__, __LINE__, mutex);
    is_disabled = true;
}

void DisableFilter::enable(ProcessVariable &pv, const epicsTime &when)
{
    Guard guard(__FILE__, __LINE__, mutex);
    is_disabled = false;
    // If there is a buffered value, send that to listener
    if (last_value)
    {
        RawValue::setTime(last_value, when);
        
        // TODO:
        RawValueAutoPtr copy;
        copy = last_value;
        last_value = 0;
        {
            GuardRelease
            ProcessVariableFilter::pvValue(pv, copy);
        }
    }
}

void DisableFilter::pvConnected(Guard &guard, ProcessVariable &pv,
                                const epicsTime &when)
{
#ifdef DEBUG_DISABLE_FILT
    LOG_MSG("DisableFilter('%s')::pvConnected\n",
            pv.getName().c_str());
#endif    
    is_connected = true;
    ProcessVariableFilter::pvConnected(guard, pv, when);
}

void DisableFilter::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                  const epicsTime &when)
{
#ifdef DEBUG_DISABLE_FILT
    LOG_MSG("DisableFilter('%s')::pvDisconnected\n",
            pv.getName().c_str());
#endif    
    is_connected = false;
    // When disabled, this means that any last value becomes invalid.
    if (is_disabled)
        last_value = 0;
    ProcessVariableFilter::pvDisconnected(guard, pv, when);
}

void DisableFilter::pvValue(Guard &guard, ProcessVariable &pv,
                           const RawValue::Data *data)
{
#ifdef DEBUG_DISABLE_FILT
    LOG_MSG("DisableFilter('%s')::pvValue: %s\n",
            pv.getName().c_str(),
            (is_disabled ? "blocked" : "passed"));
#endif    
    if (is_disabled)
    {   // Remember value
        DbrType type = pv.getDbrType(guard);
        DbrCount count = pv.getDbrCount(guard);
        if (! last_value)
            last_value = RawValue::allocate(type, count, 1);
        LOG_ASSERT(last_value);
        RawValue::copy(type, count, last_value, data);
    }
    else // or pass on to listener.
        ProcessVariableFilter::pvValue(guard, pv, data);
}
