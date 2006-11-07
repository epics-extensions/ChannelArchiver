#ifndef SAMPLEMECHANISMMONITOREDGET_H_
#define SAMPLEMECHANISMMONITOREDGET_H_

// Engine
#include "SampleMechanism.h"
#include "TimeSlotFilter.h"
#include "RepeatFilter.h"
#include "TimeFilter.h"

/*  TODO: Rething this.
 *  It's OK for changing values:
 *  <li>ProcessVariable gets monitor
 *  <li>DisableFilter passes
 *  <li>TimeSlotFilter blocks unless scan period passed
 *  <li>RepeatFilter Counts up if value didn't change
 *  <li>TimeFilter Should pass sane time stamps
 *  <li>SampleMechanismMonitoredGet nothing
 *  <li>base SampleMechanism Should write
 * 
 *  But for a value that doesn't change for a while:
 *  <li>ProcessVariable gets no monitor
 *  ...
 *  <li>RepeatFilter Doesn't count up, since it's never called.
 * 
 *  Finally a new value:
 *  <li>ProcessVariable gets monitor
 *  ...
 *  <li>RepeatFilter Considers this the only value since the last one.
 *                   No repeat count!
 * 
 *  Should there be a repeat count?
 *  Or is this all garbage?
 */

/**\ingroup Engine
 *  Sample Mechanism that stores periodic samples using a 'monitor'.
 *  <p>
 *  For each sample period, the first sample is stored.
 *  Samples that don't change are stored via a 'repeat count',
 *  up to a maximum repeat count specified in the EngineConfig.
 *  <p>
 *  The data flows as follows:
 *  <ol>
 *  <li>ProcessVariable (monitored)
 *  <li>DisableFilter
 *  <li>TimeSlotFilter (for requested period)
 *  <li>RepeatFilter
 *  <li>TimeFilter
 *  <li>SampleMechanismMonitoredGet
 *  <li>base SampleMechanism
 *  </ol> 
 */
class SampleMechanismMonitoredGet : public SampleMechanism
{
public:
    /** Construct mechanism for given sampling period. */
    SampleMechanismMonitoredGet(EngineConfig &config,
                                ProcessVariableContext &ctx,
                                const char *name,
                                double period);
    virtual ~SampleMechanismMonitoredGet();

    // SampleMechanism  
    stdString getInfo(Guard &guard);
    void stop(Guard &guard);
    
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);

    void addToFUX(Guard &guard, class FUX::Element *doc);

private:
    TimeSlotFilter time_slot_filter;
    RepeatFilter   repeat_filter;
    TimeFilter     time_filter;
};

#endif /*SAMPLEMECHANISMMONITOREDGET_H_*/
