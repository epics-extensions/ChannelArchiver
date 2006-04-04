
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitoredGet.h"

SampleMechanismMonitoredGet::SampleMechanismMonitoredGet(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period)
    : SampleMechanism(config, ctx, name, period),
      time_slot_filter(period, &repeat_filter),
      repeat_filter(config, &time_filter),
      time_filter(config, this)  
{
}

SampleMechanismMonitoredGet::~SampleMechanismMonitoredGet()
{
}

void SampleMechanismMonitoredGet::start(Guard &guard)
{
    pv.addProcessVariableListener(guard, &time_slot_filter);
    SampleMechanism::start(guard);
}   
    
void SampleMechanismMonitoredGet::stop(Guard &guard)
{
    pv.removeProcessVariableListener(guard, &time_slot_filter);
    repeat_filter.stop(guard, pv);
    SampleMechanism::stop(guard);
}

void SampleMechanismMonitoredGet::pvConnected(Guard &guard,
    ProcessVariable &pv, const epicsTime &when)
{   
    LOG_MSG("SampleMechanismMonitoredGet(%s): connected\n",
            pv.getName().c_str());
    SampleMechanism::pvConnected(guard, pv, when);
    if (!pv.isSubscribed(guard))
        pv.subscribe(guard);
    
}
    
// TODO: Remove these fall-through PV Listeners
void SampleMechanismMonitoredGet::pvDisconnected(Guard &guard,
    ProcessVariable &pv, const epicsTime &when)
{
    SampleMechanism::pvDisconnected(guard, pv, when);
}

void SampleMechanismMonitoredGet::pvValue(Guard &guard, ProcessVariable &pv,
                                          const RawValue::Data *data)
{
    SampleMechanism::pvValue(guard, pv, data);
}

