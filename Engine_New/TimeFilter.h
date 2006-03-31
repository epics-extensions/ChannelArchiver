#ifndef TIMEFILTER_H_
#define TIMEFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to remove samples that go back in time or are too futuristic
 */
class TimeFilter : public ProcessVariableFilter
{
public:
	TimeFilter(const EngineConfig &config, ProcessVariableListener *listener);
                 
	virtual ~TimeFilter();
                
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
    
private:
    const EngineConfig &config;
    epicsTime last_stamp;
};

#endif /*TIMEFILTER_H_*/
