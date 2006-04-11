
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismGet.h"

SampleMechanismGet::SampleMechanismGet(EngineConfig &config,
                                       ProcessVariableContext &ctx,
                                       ScanList &scan_list,
                                       const char *name,
                                       double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate, &repeat_filter),
      scan_list(scan_list),
      repeat_filter(config, &time_filter),
      time_filter(config, this)    
{
}

SampleMechanismGet::~SampleMechanismGet()
{
}

stdString SampleMechanismGet::getInfo(Guard &guard) const
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);
    stdString info;
    info.reserve(200);
    info = "Get, ";
    info += per;
    info += ", PV ";
    info += pv.getStateStr(guard);
    info += ", CA ";
    info += pv.getCAStateStr(guard);
    return info;
}

void SampleMechanismGet::start(Guard &guard)
{
    SampleMechanism::start(guard);
    scan_list.add(this, period);
}   
    
void SampleMechanismGet::stop(Guard &guard)
{
    scan_list.remove(this);
    repeat_filter.stop(guard, pv);
    SampleMechanism::stop(guard);
}

// Invoked by scanner.
void SampleMechanismGet::scan()
{   // Trigger a 'get', which should result in pvValue() callback.
    Guard guard(pv);
    pv.getValue(guard);
}

