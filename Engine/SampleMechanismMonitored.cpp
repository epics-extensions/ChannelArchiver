
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

stdString SampleMechanismMonitored::getInfo(Guard &guard) const
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);
    stdString info;
    info.reserve(200);
    info = "Monitored, max per ";
    info += per;
    info += "\nPV: ";
    info += pv.getStateStr(guard);
    info += "\nCA: ";
    info += pv.getCAStateStr(guard);
    return info;
}


void SampleMechanismMonitored::start(Guard &guard)
{
    pv.addListener(guard, &time_filter);
    SampleMechanism::start(guard);
}   
    
void SampleMechanismMonitored::stop(Guard &guard)
{
    pv.removeListener(guard, this);
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


