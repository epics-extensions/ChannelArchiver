// Tools
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
// Local
#include "SampleMechanism.h"

static ThrottledMsgLogger back_in_time_throttle("Buffer Back-in-time",
                                                60.0*60.0);

SampleMechanism::SampleMechanism(const EngineConfig &config,
                                 ProcessVariableContext &ctx,
                                 const char *name, double period)
    : config(config), pv(ctx, name), period(period), last_stamp_set(false)
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

void SampleMechanism::start(Guard &guard)
{
    pv.start(guard);
}   
    
void SampleMechanism::stop(Guard &guard)
{
    pv.stop(guard);
    addEvent(guard, ARCH_STOPPED, epicsTime::getCurrent());
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
    LOG_MSG("SampleMechanism(%s): value\n", pv.getName().c_str());
    epicsTime stamp = RawValue::getTime(data);
    if (last_stamp_set && last_stamp > stamp)
    {
        back_in_time_throttle.LOG_MSG("SampleMechanism(%s): back in time\n",
                                      pv.getName().c_str());
        return;
    }
    buffer.addRawValue(data);
    last_stamp = stamp;
    last_stamp_set = true;
}

void SampleMechanism::addEvent(Guard &guard, short severity,
                               const epicsTime &when)
{
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

