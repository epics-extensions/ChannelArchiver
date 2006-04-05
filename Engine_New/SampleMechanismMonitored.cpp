
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitored.h"

SampleMechanismMonitored::SampleMechanismMonitored(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate),
      time_filter(config, this)
{
}

SampleMechanismMonitored::~SampleMechanismMonitored()
{
}

void SampleMechanismMonitored::start(Guard &guard)
{
    pv.addProcessVariableListener(guard, &time_filter);
    SampleMechanism::start(guard);
}   
    
void SampleMechanismMonitored::stop(Guard &guard)
{
    pv.removeProcessVariableListener(guard, this);
    SampleMechanism::stop(guard);
}

void SampleMechanismMonitored::pvConnected(Guard &guard, ProcessVariable &pv,
                                           const epicsTime &when)
{
    LOG_MSG("SampleMechanismMonitored(%s): connected\n", pv.getName().c_str());
    SampleMechanism::pvConnected(guard, pv, when);
    if (!pv.isSubscribed(guard))
        pv.subscribe(guard);
}

