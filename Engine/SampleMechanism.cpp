// Tools
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
#include <epicsTimeHelper.h>
// Storage
#include <DataWriter.h>
// Local
#include "SampleMechanism.h"

static ThrottledMsgLogger back_in_time_throttle("Buffer Back-in-time",
                                                60.0*60.0);

SampleMechanism::SampleMechanism(const EngineConfig &config,
                                 ProcessVariableContext &ctx,
                                 const char *name, double period)
    : config(config), pv(ctx, name), running(false), period(period),
      last_stamp_set(false), have_sample_after_connection(false)
{
}

SampleMechanism::~SampleMechanism()
{
    puts("Values in sample buffer:");
    buffer.dump();    
}

const stdString &SampleMechanism::getName() const
{
    return pv.getName();
}

epicsMutex &SampleMechanism::getMutex()
{
    return pv.getMutex();
}

stdString SampleMechanism::getInfo(Guard &guard) const
{
    stdString info;
    info.reserve(200);
    info = "Sample Mechanism";
    info += "PV: ";
    info += pv.getStateStr(guard);
    info += "\nCA: ";
    info += pv.getCAStateStr(guard);
    return info;
}

void SampleMechanism::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    pv.start(guard);
    running = true;
}   

bool SampleMechanism::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    return running;
}

ProcessVariable::State SampleMechanism::getPVState(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    return pv.getState(guard);
}
    
void SampleMechanism::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    running = false;
    pv.stop(guard);
    addEvent(guard, ARCH_STOPPED, epicsTime::getCurrent());
}

size_t SampleMechanism::getSampleCount(Guard &guard) const
{
    return buffer.getCount();
}

void SampleMechanism::pvConnected(Guard &guard, ProcessVariable &pv,
                                  const epicsTime &when)
{
    LOG_MSG("SampleMechanism(%s): connected\n", pv.getName().c_str());
    // If necessary, allocate a new circular buffer.
    // Of course that means that data in there is tossed.
    // So if you managed to stop an IOC,
    // then reboot it with new data types _before_
    // the engine could write, the last few samples before
    // the reboot are gone.
    //
    // TODO: See if this code could toggle a 'write'
    // to avoid that data loss.
    DbrType type = pv.getDbrType(guard);
    DbrCount count = pv.getDbrCount(guard);
    if (type != buffer.getDbrType()  ||
        count != buffer.getDbrCount())
        buffer.allocate(type, count, config.getSuggestedBufferSpace(period));
        
    have_sample_after_connection = false;
}
    
void SampleMechanism::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                     const epicsTime &when)
{
    LOG_MSG("SampleMechanism(%s): disconnected\n", pv.getName().c_str());
    addEvent(guard, ARCH_DISCONNECT, when);
}

void SampleMechanism::pvValue(Guard &guard, ProcessVariable &pv,
                              const RawValue::Data *data)
{
    //LOG_MSG("SampleMechanism(%s): value\n", pv.getName().c_str());
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
            LOG_MSG("SampleMechanism(%s): adding sample stamped 'now'\n",
                   pv.getName().c_str());
            RawValue::Data *value = buffer.getNextElement();
            RawValue::copy(buffer.getDbrType(), buffer.getDbrCount(),
                           value, data);
            RawValue::setTime(value, now);              
            last_stamp = now;
        }
    }
}

void SampleMechanism::addEvent(Guard &guard, short severity,
                               const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, getMutex());
    if (buffer.getCapacity() < 1)
    {
        // Message is strange but matches what's described in the manual.
        LOG_MSG("'%s': Cannot add event because data type is unknown\n",
              pv.getName().c_str());
        return;
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

