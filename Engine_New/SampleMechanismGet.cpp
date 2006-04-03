
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismGet.h"

// Data pipe:
// pv -> repeat_filter -> time_filter -> this -> base SampleMechanism
SampleMechanismGet::SampleMechanismGet(EngineConfig &config,
                                       ProcessVariableContext &ctx,
                                       ScanList &scan_list,
                                       const char *name,
                                       double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate),
      scan_list(scan_list),
      repeat_filter(config, &time_filter),
      time_filter(config, this)    
{
}

SampleMechanismGet::~SampleMechanismGet()
{
}

void SampleMechanismGet::start(Guard &guard)
{
    pv.addProcessVariableListener(guard, &repeat_filter);
    SampleMechanism::start(guard);
    scan_list.add(this, period);
}   
    
void SampleMechanismGet::stop(Guard &guard)
{
    scan_list.remove(this);
    pv.removeProcessVariableListener(guard, &repeat_filter);
    repeat_filter.stop(guard, pv);
    SampleMechanism::stop(guard);
}

// Invoked by scanner.
void SampleMechanismGet::scan()
{   // Trigger a 'get', which should result in pvValue() callback.
    Guard guard(pv);
    pv.getValue(guard);
}

// TODO: Remove these fall-through PV Listeners
void SampleMechanismGet::pvConnected(Guard &guard, ProcessVariable &pv,
                                     const epicsTime &when)
{
    SampleMechanism::pvConnected(guard, pv, when);
}
    
void SampleMechanismGet::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                        const epicsTime &when)
{
    SampleMechanism::pvDisconnected(guard, pv, when);
}

void SampleMechanismGet::pvValue(Guard &guard, ProcessVariable &pv,
                                 const RawValue::Data *data)
{
    SampleMechanism::pvValue(guard, pv, data);
}

