#ifndef SAMPLEMECHANISMMONITOREDGET_H_
#define SAMPLEMECHANISMMONITOREDGET_H_

// Engine
#include "SampleMechanismMonitored.h"

/**\ingroup Engine
 *  Sample Mechanism that stores periodic samples using a 'monitor'.
 * 
 *  For each sample period, the most recent sample is stored.
 */
class SampleMechanismMonitoredGet : public SampleMechanismMonitored
{
public:
    SampleMechanismMonitoredGet(EngineConfig &config,
                                ProcessVariableContext &ctx,
                                const char *name,
                                double period);
	virtual ~SampleMechanismMonitoredGet();

    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv);
    void pvDisconnected(Guard &guard, ProcessVariable &pv);
    void pvValue(Guard &guard, ProcessVariable &pv, RawValue::Data *data);
};

#endif /*SAMPLEMECHANISMMONITOREDGET_H_*/
