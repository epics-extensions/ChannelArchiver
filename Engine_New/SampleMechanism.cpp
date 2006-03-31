
// Local
#include "SampleMechanism.h"

SampleMechanism::SampleMechanism(const EngineConfig &config,
                                 ProcessVariableContext &ctx,
                                 const char *name, double period)
    : config(config), pv(ctx, name), period(period)
{
}

SampleMechanism::~SampleMechanism()
{
}

const stdString &SampleMechanism::getName() const
{
    return pv.getName();
}

epicsMutex &SampleMechanism::getMutex()
{
    return pv.getMutex();
}

void SampleMechanism::start(Guard &guard)
{
    pv.start(guard);
}   
    
void SampleMechanism::stop(Guard &guard)
{
    pv.stop(guard);
}

