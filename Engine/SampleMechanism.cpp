// Tools
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
#include <epicsTimeHelper.h>
// Storage
#include <DataWriter.h>
// Local
#include "EngineLocks.h"
#include "SampleMechanism.h"

// #define DEBUG_SAMPLE_MECHANISM

static ThrottledMsgLogger back_in_time_throttle("Buffer Back-in-time",
                                                60.0*60.0);

SampleMechanism::SampleMechanism(
    const EngineConfig &config,
    ProcessVariableContext &ctx,
    const char *name, double period,
    ProcessVariableListener *disable_filt_listener)
    : mutex("SampleMechanism", EngineLocks::SampleMechanism),
      config(config),
      pv(ctx, name),
      disable_filter(disable_filt_listener),
      running(false),
      period(period),
      last_stamp_set(false),
      have_sample_after_connection(false)
{
}

SampleMechanism::~SampleMechanism()
{
#ifdef DEBUG_SAMPLE_MECHANISM
    puts("Values in sample buffer:");
    buffer.dump(); 
#endif
}

const stdString &SampleMechanism::getName() const
{
    return pv.getName();
}

OrderedMutex &SampleMechanism::getMutex()
{
    return mutex;
}

stdString SampleMechanism::getInfo(Guard &guard) const
{
    guard.check(__FILE__, __LINE__, getMutex());
    GuardRelease release(__FILE__, __LINE__, guard);
    stdString info;
    info.reserve(200);
    info = "PV ";
    {
        Guard pv_guard(__FILE__, __LINE__, pv);
        info += pv.getStateStr(pv_guard);
        info += ", CA ";
        info += pv.getCAStateStr(pv_guard);
    }
    return info;
}

void SampleMechanism::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    LOG_ASSERT(running == false);
    running = true;
    GuardRelease release(__FILE__, __LINE__, guard);    
    pv.addListener(&disable_filter);
    {    
        Guard pv_guard(__FILE__, __LINE__, pv);
        pv.start(pv_guard);
    }   
}

bool SampleMechanism::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    return running;
}

ProcessVariable::State SampleMechanism::getPVState(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    GuardRelease release(__FILE__, __LINE__, guard);    
    Guard pv_guard(__FILE__, __LINE__, pv);
    return pv.getState(pv_guard);        
}
    
void SampleMechanism::disable(const epicsTime &when)
{
    {
        Guard guard(__FILE__, __LINE__, getMutex());
        addEvent(guard, ARCH_DISABLED, when);
    }
    Guard dis_guard(__FILE__, __LINE__, disable_filter);
    disable_filter.disable(dis_guard);
}
 
void SampleMechanism::enable(const epicsTime &when)
{
    Guard dis_guard(__FILE__, __LINE__, disable_filter);
    disable_filter.enable(dis_guard, pv, when);
}   
    
    TODO: review from here on with SampleMechanism::mutex
    
void SampleMechanism::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    LOG_ASSERT(running == true);    
    // Remove listener so we don't get the 'disconnected' call...
    running = false;
    {
        GuardRelease release(__FILE__, __LINE__, guard);    
        Guard pv_guard(__FILE__, __LINE__, pv);    
        pv.stop(guard);
    }
    pv.removeListener(&disable_filter);
    // .. because we use 'stopped' anyway.
    addEvent(guard, ARCH_STOPPED, epicsTime::getCurrent());
}

size_t SampleMechanism::getSampleCount(Guard &guard) const
{
    return buffer.getCount();
}

void SampleMechanism::pvConnected(ProcessVariable &pv,
                                  const epicsTime &when)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): connected\n", pv.getName().c_str());
#endif
    Guard guard(__FILE__, __LINE__, getMutex());
    // If necessary, allocate a new circular buffer.
    DbrType type   = pv.getDbrType(guard);
    DbrCount count = pv.getDbrCount(guard);
    if (type != buffer.getDbrType()  ||  count != buffer.getDbrCount())
    {   // Why there might be values in the buffer on connect:
        // 1) Channel was disabled before it connected,
        //    and addEvent(disabled) used 'double' as a guess.
        // 2) A quick reboot caused a reconnect with new data types
        //    before the engine could write the data gathered before the reboot.
        //
        // Those few _data_ samples are gone, but we don't want to loose
        // some of the key _events_.
        CircularBuffer tmp;
        const RawValue::Data *val;
        if (buffer.getCount() > 0)
        {   // Copy all events into tmp.
            tmp.allocate(buffer.getDbrType(), buffer.getDbrCount(),
                         buffer.getCount());
            while ((val = buffer.removeRawValue()) != 0)
            {
                if (RawValue::isInfo(val))
                    tmp.addRawValue(val);
            }
        }
        // Get buffer which matches the current PV type.
        buffer.allocate(type, count, config.getSuggestedBufferSpace(period));
        // Copy saved info events back (usually: nothing)
        while ((val = tmp.removeRawValue()) != 0)
        {
            RawValue::Data *value = buffer.getNextElement();
            size_t size = RawValue::getSize(buffer.getDbrType(),
                                            buffer.getDbrCount());
            memset(value, 0, size);
            RawValue::setStatus(value, 0, RawValue::getSevr(val));
            RawValue::setTime(value, RawValue::getTime(val));
        }
    }   
    have_sample_after_connection = false;
}
    
void SampleMechanism::pvDisconnected(ProcessVariable &pv,
                                     const epicsTime &when)
{
    Guard guard(__FILE__, __LINE__, getMutex());
    // ignore if this arrives as a result of 'stop()'
    if (! running)
        return;
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): disconnected\n", pv.getName().c_str());
#endif
    addEvent(guard, ARCH_DISCONNECT, when);
}

// Last in the chain from PV via filters to the Circular buffer:
// One more back-in-time check, then add to buffer.
void SampleMechanism::pvValue(ProcessVariable &pv,
                              const RawValue::Data *data)
{
#ifdef DEBUG_SAMPLE_MECHANISM
    LOG_MSG("SampleMechanism(%s): value\n", pv.getName().c_str());
#endif
    Guard guard(__FILE__, __LINE__, getMutex());
    // Last back-in-time check before writing to disk
    epicsTime stamp = RawValue::getTime(data);
    if (last_stamp_set && last_stamp > stamp)
    {
        back_in_time_throttle.LOG_MSG("SampleMechanism(%s): back in time\n",
                                      pv.getName().c_str());
        return;
    }
    // Add original sample
    buffer.addRawValue(data);
    last_stamp = stamp;
    last_stamp_set = true;
    // After connection, add another copy with the host time stamp.
    if (!have_sample_after_connection)
    {
        have_sample_after_connection = true;
        epicsTime now = epicsTime::getCurrent();
        if (now > stamp)
        {
#ifdef DEBUG_SAMPLE_MECHANISM        
            LOG_MSG("SampleMechanism(%s): adding sample stamped 'now'\n",
                   pv.getName().c_str());
#endif
            RawValue::Data *value = buffer.getNextElement();
            RawValue::copy(buffer.getDbrType(), buffer.getDbrCount(),
                           value, data);
            RawValue::setTime(value, now);              
            last_stamp = now;
        }
    }
}

// Add a special sample: No value, only time stamp and severity matter.
void SampleMechanism::addEvent(Guard &guard, short severity,
                               const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, getMutex());
    if (buffer.getCapacity() < 1)
    {   // Data type is unknown, but we want to add an event.
        // PV defaults to DOUBLE, so use that:
        buffer.allocate(pv.getDbrType(guard),
                        pv.getDbrCount(guard),
                        config.getSuggestedBufferSpace(period));
    }
    RawValue::Data *value = buffer.getNextElement();
    size_t size = RawValue::getSize(buffer.getDbrType(),
                                    buffer.getDbrCount());
    memset(value, 0, size);
    RawValue::setStatus(value, 0, severity);
    
    if (last_stamp_set  &&  when < last_stamp)
        // adjust time, event has to be added to archive
        RawValue::setTime(value, last_stamp);
    else
    {
        RawValue::setTime(value, when);
        last_stamp = when;
        last_stamp_set = true;
    }
}

unsigned long SampleMechanism::write(Guard &guard, Index &index)
{
    guard.check(__FILE__, __LINE__, getMutex());
    size_t i, num_samples = buffer.getCount();
    if (num_samples <= 0)
        return 0;
        
    DataWriter writer(index, getName(),
                      pv.getCtrlInfo(guard),
                      pv.getDbrType(guard),
                      pv.getDbrCount(guard),
                      period, num_samples);
    const RawValue::Data *value;
    unsigned long count = 0;
    for (i=0; i<num_samples; ++i)
    {
        if (!(value = buffer.removeRawValue()))
        {
            LOG_MSG("'%s': Circular buffer empty while writing\n",
                    getName().c_str());
            break;
        }
        if (! writer.add(value))
        {
            stdString txt;
            epicsTime2string(RawValue::getTime(value), txt);
            LOG_MSG("'%s': back-in-time write value stamped %s\n",
                    getName().c_str(), txt.c_str());
            break;
        }
        ++count;
    }
    buffer.reset();
    return count;        
}

void SampleMechanism::addToFUX(Guard &guard, class FUX::Element *doc)
{
    guard.check(__FILE__, __LINE__, getMutex());
}
