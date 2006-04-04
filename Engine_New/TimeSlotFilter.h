#ifndef TIMESLOTFILTER_H_
#define TIMESLOTFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 * Filter that passes one sample per time slot.
 */
class TimeSlotFilter : public ProcessVariableFilter
{
public:
	TimeSlotFilter(double period, ProcessVariableListener *listener);
                 
	virtual ~TimeSlotFilter();
                
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
private:
    double period;
    epicsTime next_slot;
};

#endif /*TIMESLOTFILTER_H_*/
