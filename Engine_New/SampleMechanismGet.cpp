
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismGet.h"

// Data pipe:
//  pv -> repeat -> time -> this
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
    puts("'Gotten' Values:");
    buffer.dump();
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

void SampleMechanismGet::scan()
{
    Guard guard(pv);
    pv.getValue(guard);
}

void SampleMechanismGet::pvConnected(Guard &guard, ProcessVariable &pv,
                                     const epicsTime &when)
{
    LOG_MSG("SampleMechanismGet(%s): connected\n", pv.getName().c_str());
    buffer.allocate(pv.getDbrType(guard), pv.getDbrCount(guard),
                    config.getSuggestedBufferSpace(period));
}
    
void SampleMechanismGet::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                        const epicsTime &when)
{
    LOG_MSG("SampleMechanismGet(%s): disconnected\n", pv.getName().c_str());
}

void SampleMechanismGet::pvValue(Guard &guard, ProcessVariable &pv,
                                 const RawValue::Data *data)
{
    LOG_MSG("SampleMechanismGet(%s): value\n", pv.getName().c_str());
    // TODO: Add repeat-count filter
    buffer.addRawValue(data);
}

