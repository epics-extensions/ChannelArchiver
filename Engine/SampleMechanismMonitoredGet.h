#ifndef SAMPLEMECHANISMMONITOREDGET_H_
#define SAMPLEMECHANISMMONITOREDGET_H_

// Engine
#include "SampleMechanism.h"
#include "ScanList.h"
#include "TimeSlotFilter.h"
#include "RepeatFilter.h"
#include "TimeFilter.h"

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
 *  <li>RepeatFilter (also informed about periodic scan)
 *  <li>TimeFilter
 *  <li>SampleMechanismMonitoredGet
 *  <li>base SampleMechanism
 *  </ol> 
 *  
 *  For PVs that acutally change, the monitors from the PV
 *  drive the value chain. But when the PV does not change,
 *  the lack of monitors would also never trigger the repeat filter
 *  to count repeated values. Therefore the scan list is used to
 *  periodically trigger the repeat filter, which can then add
 *  repeat counts if no new values have been received.
 */
class SampleMechanismMonitoredGet : public SampleMechanism, public Scannable
{
public:
    /** Construct mechanism for given sampling period. */
    SampleMechanismMonitoredGet(EngineConfig &config,
                                ProcessVariableContext &ctx,
                                ScanList &scan_list,
                                const char *name,
                                double period);
    virtual ~SampleMechanismMonitoredGet();

    // SampleMechanism  
    stdString getInfo(Guard &guard);
    void start(Guard &guard);    
    void stop(Guard &guard);
    
    // Scannable
    void scan(const epicsTime &now);
    
    // ProcessVariableListener
    void pvConnected(ProcessVariable &pv, const epicsTime &when);

    void addToFUX(Guard &guard, class FUX::Element *doc);

private:
    ScanList       &scan_list;
    TimeSlotFilter time_slot_filter;
    RepeatFilter   repeat_filter;
    TimeFilter     time_filter;
};

#endif /*SAMPLEMECHANISMMONITOREDGET_H_*/
