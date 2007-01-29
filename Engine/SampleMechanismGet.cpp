
// Tools
#include <MsgLogger.h>
#include <epicsTimeHelper.h>
// Engine
#include "SampleMechanismGet.h"

SampleMechanismGet::SampleMechanismGet(EngineConfig &config,
                                       ProcessVariableContext &ctx,
                                       ScanList &scan_list,
                                       const char *name,
                                       double period)
    : SampleMechanism(config, ctx, name, period, &repeat_filter),
      scan_list(scan_list),
      repeat_filter(config, pv, &time_filter),
      time_filter(config, this)    
{
}

SampleMechanismGet::~SampleMechanismGet()
{
}

stdString SampleMechanismGet::getInfo(Guard &guard)
{
    char per[10];
    snprintf(per, sizeof(per), "%.1f s", period);
    GuardRelease release(__FILE__, __LINE__, guard);

    stdString info;
    info.reserve(200);
    info = "Get, ";
    info += per;
    info += ", PV ";

    Guard pv_guard(__FILE__, __LINE__, pv);
    info += pv.getStateStr(pv_guard);
    info += ", CA ";
    info += pv.getCAStateStr(pv_guard);
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
    repeat_filter.stop();
    SampleMechanism::stop(guard);
}

// Invoked by scanner.
void SampleMechanismGet::scan(const epicsTime &now)
{   // Trigger a 'get', which should result in pvValue() callback.
    Guard pv_guard(__FILE__, __LINE__, pv);
    pv.getValue(pv_guard);
}

void SampleMechanismGet::addToFUX(Guard &guard, class FUX::Element *doc)
{
    new FUX::Element(doc, "period", "%s",
                     SecondParser::format(period).c_str());
    new FUX::Element(doc, "scan");
}

