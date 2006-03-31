
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitored.h"

SampleMechanismMonitored::SampleMechanismMonitored(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period_estimate)
    : SampleMechanism(config, ctx, name, period_estimate)
{
}

SampleMechanismMonitored::~SampleMechanismMonitored()
{
    puts("Monitored Values:");
    buffer.dump();
}

void SampleMechanismMonitored::start(Guard &guard)
{
    pv.addProcessVariableListener(guard, this);
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
    buffer.allocate(pv.getDbrType(guard), pv.getDbrCount(guard),
                    config.getSuggestedBufferSpace(period));
    pv.subscribe(guard);
}
    
void SampleMechanismMonitored::pvDisconnected(Guard &guard,
                                              ProcessVariable &pv,
                                              const epicsTime &when)
{
    LOG_MSG("SampleMechanismMonitored(%s): disconnected\n", pv.getName().c_str());
}

void SampleMechanismMonitored::pvValue(Guard &guard, ProcessVariable &pv,
                                       const RawValue::Data *data)
{
    LOG_MSG("SampleMechanismMonitored(%s): value\n", pv.getName().c_str());
    buffer.addRawValue(data);
}

