
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitoredGet.h"

SampleMechanismMonitoredGet::SampleMechanismMonitoredGet(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period)
    : SampleMechanism(config, ctx, name, period, &time_slot_filter),
      time_slot_filter(period, &repeat_filter),
      repeat_filter(config, &time_filter),
      time_filter(config, this)  
{
}

SampleMechanismMonitoredGet::~SampleMechanismMonitoredGet()
{
}

stdString SampleMechanismMonitoredGet::getInfo(Guard &guard) const
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);
    stdString info;
    info.reserve(200);
    info = "Monitored get, ";
    info += per;
    info += ", PV ";
    info += pv.getStateStr(guard);
    info += ", CA ";
    info += pv.getCAStateStr(guard);
    return info;
}

void SampleMechanismMonitoredGet::stop(Guard &guard)
{
    repeat_filter.stop(guard, pv);
    SampleMechanism::stop(guard);
}

void SampleMechanismMonitoredGet::pvConnected(Guard &guard,
    ProcessVariable &pv, const epicsTime &when)
{   
    //LOG_MSG("SampleMechanismMonitoredGet(%s): connected\n",
    //        pv.getName().c_str());
    SampleMechanism::pvConnected(guard, pv, when);
    if (!pv.isSubscribed(guard))
        pv.subscribe(guard);    
}

void SampleMechanismMonitoredGet::addToFUX(Guard &guard, FUX::Element *doc)
{
    new FUX::Element(doc, "period", "%g", period);
    new FUX::Element(doc, "scan");
}

