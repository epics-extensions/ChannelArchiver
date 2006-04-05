#ifndef REPEATFILTER_H_
#define REPEATFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to combine successive matching samples into a 'repeat count'.
 */
class RepeatFilter : public ProcessVariableFilter
{
public:
    RepeatFilter(const EngineConfig &config,
                 ProcessVariableListener *listener);
                 
    /** @see flushRepeats() */
    virtual ~RepeatFilter();
        
    /** It's suggested to stop the filter when sampling stops,
     *  since this flushes accumulated repeats.
     */
    void stop(Guard &guard, ProcessVariable &pv);
        
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
    
private:
    const EngineConfig &config;
    bool is_previous_value_valid;
    RawValueAutoPtr previous_value; // the previous value
    size_t repeat_count; // repeat count for the previous value
    
    void flush(Guard &guard, ProcessVariable &pv, const epicsTime &when);
};

#endif /*REPEATFILTER_H_*/
