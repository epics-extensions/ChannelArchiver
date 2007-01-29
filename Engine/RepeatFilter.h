#ifndef REPEATFILTER_H_
#define REPEATFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to combine successive matching samples into a 'repeat count'.
 *  <p>
 *  Used in two ways:
 *  Simply driven by a periodic 'get' on the PV, which results in a periodic
 *  pvValue() event.
 *  Or driven by a subscription to the PV, which results in unpredictable
 *  pvValue() events. So in addition, update() is called periodically,
 *  allowing the filter to add repeat counts in the absence of PV events.
 */
class RepeatFilter : public ProcessVariableFilter
{
public:
    /** Create new filter, using config for max. repeat count. */
    RepeatFilter(const EngineConfig &config,
                 ProcessVariable &pv,
                 ProcessVariableListener *listener);
                 
    /** @see flushRepeats() */
    virtual ~RepeatFilter();
        
    /** It's suggested to stop the filter when sampling stops,
     *  since this flushes accumulated repeats.
     */
    void stop();
        
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);
    void pvDisconnected(ProcessVariable &pv, const epicsTime &when);
    void pvValue(ProcessVariable &pv, const RawValue::Data *data);
    
    /** SampleMechanismMonitoredGet uses this to trigger repeat counts
     *  in case the ProcessVariableListener does not receive anything
     *  because the PV simply doesn't change.
     */
    void update(const epicsTime &now);
    
private:
    // RepeatFilter is usually accessed by ProcessVariable (CA),
    // but also by engine thread in 'stop()', so we need a mutex.
    OrderedMutex mutex;
    const EngineConfig &config;
    ProcessVariable &pv;
    
    bool is_previous_value_valid;
    RawValueAutoPtr previous_value; // the previous value
    size_t repeat_count; // repeat count for the previous value
    bool had_value_since_update;
    
    void inc(Guard &guard, const epicsTime &when);
    void flush(Guard &guard, const epicsTime &when);
};

#endif /*REPEATFILTER_H_*/
