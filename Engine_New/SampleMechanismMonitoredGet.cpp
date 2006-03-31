
// Tools
#include <MsgLogger.h>
// Engine
#include "SampleMechanismMonitoredGet.h"

SampleMechanismMonitoredGet::SampleMechanismMonitoredGet(
    EngineConfig &config, ProcessVariableContext &ctx,
    const char *name, double period)
    : SampleMechanismMonitored(config, ctx, name, period)
{
}

SampleMechanismMonitoredGet::~SampleMechanismMonitoredGet()
{}

void SampleMechanismMonitoredGet::pvConnected(Guard &guard, ProcessVariable &pv)
{
    SampleMechanismMonitored::pvConnected(guard, pv);
}
    
void SampleMechanismMonitoredGet::pvDisconnected(Guard &guard, ProcessVariable &pv)
{
    SampleMechanismMonitored::pvDisconnected(guard, pv);
}

void SampleMechanismMonitoredGet::pvValue(Guard &guard, ProcessVariable &pv,
                                       RawValue::Data *data)
{
    LOG_MSG("SampleMechanismMonitoredGet(%s): value\n", pv.getCName());
    buffer.addRawValue(data);
}

