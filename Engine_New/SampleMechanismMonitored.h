#ifndef SAMPLEMECHANISMMONITORED_H_
#define SAMPLEMECHANISMMONITORED_H_

// Engine
#include "SampleMechanism.h"
#include "TimeFilter.h"

/**\ingroup Engine
 *  Monitored Sample Mechanism.
 * 
 *  Every received sample is stored.
 *  <p>
 *  Requires a period estimate in order to allocate a buffer
 *  for the values until they are written to storage.
 */
class SampleMechanismMonitored : public SampleMechanism
{
public:
    SampleMechanismMonitored(EngineConfig &config,
                             ProcessVariableContext &ctx,
                             const char *name,
                             double period_estimate);
	virtual ~SampleMechanismMonitored();

    // SampleMechanism  
    void start(Guard &guard);    
    void stop(Guard &guard);
    
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv, const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
                 
private:
    TimeFilter time_filter;
};

#endif /*SAMPLEMECHANISMMONITORED_H_*/
