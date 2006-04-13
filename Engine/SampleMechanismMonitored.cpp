
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitored.h"

SampleMechanismMonitored::SampleMechanismMonitored(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate, &time_filter),
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
    info += ", PV ";
    info += pv.getStateStr(guard);
    info += ", CA ";
    info += pv.getCAStateStr(guard);
    return info;
}

void SampleMechanismMonitored::pvConnected(Guard &guard, ProcessVariable &pv,
                                           const epicsTime &when)
{
    // LOG_MSG("SampleMechanismMonitored(%s): connected\n", pv.getName().c_str());
    SampleMechanism::pvConnected(guard, pv, when);
    if (!pv.isSubscribed(guard))
        pv.subscribe(guard);
}

void SampleMechanismMonitored::addToFUX(Guard &guard, class FUX::Element *doc)
{
    new FUX::Element(doc, "period", "%g", period);
    new FUX::Element(doc, "monitor");
}

