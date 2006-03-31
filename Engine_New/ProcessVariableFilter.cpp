// Tools
#include "MsgLogger.h"
// Engine
#include "ProcessVariableFilter.h"

ProcessVariableFilter::ProcessVariableFilter(ProcessVariableListener *listener)
    : listener(listener)
{
    LOG_ASSERT(listener != 0);
}

ProcessVariableFilter::~ProcessVariableFilter()
{}

void ProcessVariableFilter::pvConnected(Guard &guard, ProcessVariable &pv,
                                        const epicsTime &when)
{
    listener->pvConnected(guard, pv, when);
}

void ProcessVariableFilter::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                           const epicsTime &when)
{
    listener->pvDisconnected(guard, pv, when);
}

void ProcessVariableFilter::pvValue(Guard &guard, ProcessVariable &pv,
                                    const RawValue::Data *data)
{
    listener->pvValue(guard, pv, data);
}
    
