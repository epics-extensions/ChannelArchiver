// Tools
#include <MsgLogger.h>
// Engine
#include "RepeatFilter.h"

#define DEBUG_REP_FILT

RepeatFilter::RepeatFilter(const EngineConfig &config,
                           ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      config(config), is_previous_value_valid(false), repeat_count(0)
{
}

RepeatFilter::~RepeatFilter()
{
}

void RepeatFilter::stop(Guard &guard, ProcessVariable &pv)
{
    flush(guard, pv, epicsTime::getCurrent());
    is_previous_value_valid = false;
}

void RepeatFilter::pvConnected(Guard &guard, ProcessVariable &pv,
                               const epicsTime &when)
{
    // Prepare storage for copying values now that data type is known.
    previous_value = RawValue::allocate(pv.getDbrType(guard),
                                        pv.getDbrCount(guard), 1);
    is_previous_value_valid = false;
    ProcessVariableFilter::pvConnected(guard, pv, when);
}

void RepeatFilter::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                  const epicsTime &when)
{
    // Before the disconnect is handled 'downstream',
    // log and clear any accumulated repeats.
    flush(guard, pv, when);
    is_previous_value_valid = false;
    ProcessVariableFilter::pvDisconnected(guard, pv, when);
}

void RepeatFilter::pvValue(Guard &guard, ProcessVariable &pv,
                           const RawValue::Data *data)
{
    DbrType type = pv.getDbrType(guard);
    DbrCount count = pv.getDbrCount(guard);
    if (is_previous_value_valid)
    {
        if (RawValue::hasSameValue(type, count, previous_value, data))
        {
            ++repeat_count;
#           ifdef DEBUG_REP_FILT
            LOG_MSG("RepeatFilter '%s': repeat %zu\n",
                    pv.getName().c_str(), repeat_count);
#           endif
            if (repeat_count >= config.getMaxRepeatCount())
            {   // Forced flush, marked by host time; keep the repeat value.
#               ifdef DEBUG_REP_FILT
                LOG_MSG("Reached max. repeat count.\n");
#               endif
                flush(guard, pv, epicsTime::getCurrent());
            }
            return;
        }
        else // New data flushes repeats, then continue to handle new data.
            flush(guard, pv, RawValue::getTime(data));
    }
    // Remember current value for repeat test..
    LOG_ASSERT(previous_value);
    RawValue::copy(type, count, previous_value, data);
    is_previous_value_valid = true;
    repeat_count = 0;
    // and pass on to listener.
    ProcessVariableFilter::pvValue(guard, pv, data);
}

// In case we have a repeat value, send to listener and reset.
void RepeatFilter::flush(Guard &guard, ProcessVariable &pv,
                         const epicsTime &when)
{
    if (is_previous_value_valid == false || repeat_count <= 0)
        return;
#   ifdef DEBUG_REP_FILT
    LOG_MSG("RepeatFilter '%s': Flushing %zu repeats\n",
            pv.getName().c_str(), repeat_count);
#   endif
    // Try to move time stamp of 'repeat' sample to the time
    // when the value became obsolete.
    epicsTime stamp = RawValue::getTime(previous_value);
    if (when > stamp)
        RawValue::setTime(previous_value, when);
    // Add 'repeat' sample.
    RawValue::setStatus(previous_value, repeat_count, ARCH_REPEAT);
    ProcessVariableFilter::pvValue(guard, pv, previous_value);
    repeat_count = 0;
}

