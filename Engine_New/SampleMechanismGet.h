#ifndef SAMPLEMECHANISMGET_H_
#define SAMPLEMECHANISMGET_H_

// Engine
#include "SampleMechanism.h"
#include "ScanList.h"
#include "RepeatFilter.h"
#include "TimeFilter.h"

/**\ingroup Engine
 *  Sample Mechanism that performs a periodic 'get'.
 * 
 *  New samples are stored.
 *  Samples that don't change are stored via a 'repeat count',
 *  up to a maximum repeat count specified in the EngineConfig.
 */
class SampleMechanismGet :
    public SampleMechanism, public Scannable, public ProcessVariableListener
{
public:
    SampleMechanismGet(EngineConfig &config,
                       ProcessVariableContext &ctx,
                       ScanList &scan_list,
                       const char *name,
                       double period);
	virtual ~SampleMechanismGet();

    // SampleMechanism  
    void start(Guard &guard);    
    void stop(Guard &guard);
    
    // Scannable
    void scan();
   
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
private:
    ScanList &scan_list;
    RepeatFilter repeat_filter;
    TimeFilter time_filter;
};

#endif /*SAMPLEMECHANISMGET_H_*/
