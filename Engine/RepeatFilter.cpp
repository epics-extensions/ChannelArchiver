// Tools
#include <MsgLogger.h>
// Engine
#include "EngineLocks.h"
#include "RepeatFilter.h"

// #define DEBUG_REP_FILT

RepeatFilter::RepeatFilter(const EngineConfig &config,
                           ProcessVariable &pv,
                           ProcessVariableListener *listener)
    : ProcessVariableFilter(listener),
      mutex("RepeatFilter", EngineLocks::RepeatFilter),
      config(config),
      pv(pv),
      is_previous_value_valid(false),
      repeat_count(0),
      had_value_since_update(false),
      last_sample_was_repeat(false)
{
}

RepeatFilter::~RepeatFilter()
{
}

void RepeatFilter::stop()
{
    Guard guard(__FILE__, __LINE__, mutex);
    flush(guard, epicsTime::getCurrent());
    is_previous_value_valid = false;
}

void RepeatFilter::pvConnected(ProcessVariable &pv, const epicsTime &when)
{
    if (&pv != &this->pv)
    {
        LOG_MSG("RepeatFilter::pvConnected '%s' called with wrong PV '%s'\n",
                this->pv.getName().c_str(),
                pv.getName().c_str());
        return;
    }
    // Prepare storage for copying values now that data type is known.
    DbrType type;
    DbrCount count;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }    
    {
        Guard guard(__FILE__, __LINE__, mutex);
        previous_value = RawValue::allocate(type, count, 1);
        is_previous_value_valid = false;
    }
    ProcessVariableFilter::pvConnected(pv, when);
}

void RepeatFilter::pvDisconnected(ProcessVariable &pv, const epicsTime &when)
{
    if (&pv != &this->pv)
    {
        LOG_MSG("RepeatFilter::pvDisconnected '%s' called with wrong PV '%s'\n",
                this->pv.getName().c_str(),
                pv.getName().c_str());
        return;
    }
    {
        // Before the disconnect is handled 'downstream',
        // log and clear any accumulated repeats.
        Guard guard(__FILE__, __LINE__, mutex);
        flush(guard, when);
        is_previous_value_valid = false;
    }
    ProcessVariableFilter::pvDisconnected(pv, when);
}

void RepeatFilter::pvValue(ProcessVariable &pv, const RawValue::Data *data)
{
    if (&pv != &this->pv)
    {
        LOG_MSG("RepeatFilter::pvValue '%s' called with wrong PV '%s'\n",
                this->pv.getName().c_str(),
                pv.getName().c_str());
        return;
    }
    DbrType type;
    DbrCount count;
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        type  = pv.getDbrType(pv_guard);
        count = pv.getDbrCount(pv_guard);
    }
    // Check new monitor or result of 'scan'.
    // Does it have the same value, is it a 'repeat'?     
    {
        Guard guard(__FILE__, __LINE__, mutex);
        if (is_previous_value_valid)
        {
            if (RawValue::hasSameValue(type, count, previous_value, data))
            {   // Accumulate repeats.
                inc(guard, epicsTime::getCurrent());
                return; // Do _not_ pass on to listener.
            }
            else // New data flushes repeats, then continue to handle new data.
                flush(guard, RawValue::getTime(data));
        }
        
        // Remember current value for repeat test.
        LOG_ASSERT(previous_value);
        RawValue::copy(type, count, previous_value, data);
        // If the last value was a 'repeat' sample,
        // it could carry the timestamp from an update()
        // call.
        // A split second later, real new data can arrive,
        // but with a time stamp just before the last
        // 'repeat' sample.
        // We can't go back to decrement or even remove
        // the 'repeat' samples, but we at least tweak this
        // time stamp so that the sample is logged and not
        // refused because of back-in-time issues.
        if (last_sample_was_repeat &&
            RawValue::getTime(previous_value) < repeat_stamp)
        {   RawValue::setTime(previous_value, repeat_stamp); }    
        is_previous_value_valid = true;
        repeat_count = 0;
        had_value_since_update = true;
        last_sample_was_repeat = false;
    }
    // and pass on to listener.
#   ifdef DEBUG_REP_FILT
    LOG_MSG("'%s': RepeatFilter passes new value.\n", pv.getName().c_str());
#   endif
    ProcessVariableFilter::pvValue(pv, previous_value);
}

// Called by SampleMechanismMonitoredGet::scan
void RepeatFilter::update(const epicsTime &now)
{
#   ifdef DEBUG_REP_FILT
    LOG_MSG("'%s': RepeatFilter update.\n", pv.getName().c_str());
#   endif
    Guard guard(__FILE__, __LINE__, mutex);
    if (had_value_since_update)
    {   // OK so far, but 'arm'.
        // If there are no more values until the next update(),
        // we'll count repeats
        had_value_since_update = false;
        return;
    }
    // else: No new values received, accumulate repeats.
    inc(guard, now);
}

void RepeatFilter::inc(Guard &guard, const epicsTime &when)
{   // Count one more repeat.
    ++repeat_count;
#   ifdef DEBUG_REP_FILT
    LOG_MSG("RepeatFilter '%s': repeat %zu\n",
            pv.getName().c_str(), (size_t)repeat_count);
#   endif
    if (repeat_count >= config.getMaxRepeatCount())
    {   // Forced flush, marked by host time; keep the repeat value.
#       ifdef DEBUG_REP_FILT
        LOG_MSG("'%s': Reached max. repeat count.\n", pv.getName().c_str());
#       endif
        flush(guard, when);
    }
}

// In case we have a repeat value, send to listener and reset.
void RepeatFilter::flush(Guard &guard, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (is_previous_value_valid == false || repeat_count <= 0)
        return;
#   ifdef DEBUG_REP_FILT
    LOG_MSG("RepeatFilter '%s': Flushing %zu repeats\n",
            pv.getName().c_str(), (size_t)repeat_count);
#   endif
    // Try to move time stamp of 'repeat' sample to the time
    // when the value became obsolete.
    epicsTime stamp = RawValue::getTime(previous_value);
    if (when > stamp)
        RawValue::setTime(previous_value, when);
    // Add 'repeat' sample.
    RawValue::setStatus(previous_value, repeat_count, ARCH_REPEAT);
    last_sample_was_repeat = true;
    repeat_stamp = RawValue::getTime(previous_value);
    repeat_count = 0;
    had_value_since_update = true;
    {
        GuardRelease release(__FILE__, __LINE__, guard);    
        ProcessVariableFilter::pvValue(pv, previous_value);
    }
}
