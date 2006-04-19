#ifndef SAMPLEMECHANISM_H_
#define SAMPLEMECHANISM_H_

// Storage
#include <Index.h>
// Local
#include "Named.h"
#include "EngineConfig.h"
#include "ProcessVariable.h"
#include "CircularBuffer.h"
#include "DisableFilter.h"

/**\ingroup Engine
 *  Sample Mechanism base.
 *  <p>
 *  This base class for all sample mechanisms maintains the
 *  ProcessVariable start/stop.
 *  <p>
 *  Data from the PV goes to the DisableFilter, via which
 *  the enable/disable is implemented.
 *  Derived sample mechanisms will probably add more filters,
 *  until finally invoking the ProcessVariableListener interface
 *  of the base SampleMechanism.
 *  <p>
 *  The ProcessVariableListener implementation in the base
 *  SampleMechanism logs "Disconnected" on pvDisconnected(),
 *  and otherwise adds every pvValue to the CircularBuffer.
 *  <p>
 *  Uses 'virtual Named' so we can implement other
 *  'Named' interfaces in derived SampleMechanisms and still
 *  only get one 'Named' base.
 */
class SampleMechanism : public ProcessVariableListener,
                        public Guardable,
                        public virtual Named
{
public:
    /** Construct mechanism for given period.
     *  @param config The global configuration.
     *  @param ctx    The ProcessVariableContext to use for the pv.
     *  @param name   The pv name to connect to.
     *  @param period The sample period. Use differs with derived class.
     *  @param disable_filt_listener The next listener after the DisableFilter.
     */
    SampleMechanism(const EngineConfig &config,
                    ProcessVariableContext &ctx, const char *name,
                    double period,
                    ProcessVariableListener *disable_filt_listener);
    virtual ~SampleMechanism();
    
    /** Gets the ProcessVariable name.
     *  @see NamedAbstractBase
     */
    const stdString &getName() const;
    
    /** @return Returns the mutex for the SampleMechanism.
     *  <p>
     *  Actually uses the mutex of the ProcessVariable.
     *  @see Guardable */
    epicsMutex &getMutex();

    /** @return Returns a description of mechanism and current state. */
    virtual stdString getInfo(Guard &guard) const;
    
    /** Start the sample mechanism.
     *  <p>
     *  Base implementation starts the PV.
     */
    virtual void start(Guard &guard);
    
    /** @return Returns true if start has been invoked until stop(). */
    bool isRunning(Guard &guard);
    
    /** @return Returns the state of the ProcessVariable. */
    ProcessVariable::State getPVState(Guard &guard);
    
    /** Add a listener to the underlying PV. */
    void addStateListener(Guard &guard, ProcessVariableStateListener *listener);

    /** Remove a listener from the underlying PV. */
    void removeStateListener(Guard &guard,
                             ProcessVariableStateListener *listener);

    /** Add a listener to the underlying PV. */
    void addValueListener(Guard &guard, ProcessVariableValueListener *listener);

    /** Remove a listener from the underlying PV. */
    void removeValueListener(Guard &guard,
                             ProcessVariableValueListener *listener);
    
    /** Temporarily disable sampling.
     *  @see enable()  */
    void disable(Guard &guard, const epicsTime &when);
     
    /** Re-enable sampling.
     *  @see disable() */
    void enable(Guard &guard, const epicsTime &when);
    
    /** Stop sampling.
     *  @see #start()
     *  <p>
     *  Base implementation stops the PV and adds a 'STOPPED (OFF)' event.
     */
    virtual void stop(Guard &guard);
    
    /** @return Returns the number of samples in the circular buffer. */
    size_t getSampleCount(Guard &guard) const;
    
    /** ProcessVariableStateListener.
     *  <p>
     *  Base implementation allocates circular buffer
     */
    virtual void pvConnected(class Guard &guard,
                             class ProcessVariable &pv,
                             const epicsTime &when);
    
    /** ProcessVariableStateListener.
     *  <p>
     *  Base implementation adds a "DISCONNECTED" marker.
     */
    virtual void pvDisconnected(class Guard &guard,
                                class ProcessVariable &pv,
                                const epicsTime &when);

    /** ProcessVariableValueListener.
     *  <p>
     *  Base implementation adds data to buffer.
     *  In addition, the initial value after a new connection
     *  is also logged with the host time stamp.
     *  For PVs that never change, this helps because the
     *  original time stamp might be before the last 'disconnect',
     *  so this gives us an idea of when the PV connected.
     */
    virtual void pvValue(class Guard &guard,
                         class ProcessVariable &pv,
                         const RawValue::Data *data);
    
    /** Write current buffer to index.
     *  @return Returns number of samples written.
     */                     
    unsigned long write(Guard &guard, Index &index);
    
    /** Append this sample mechanism to a FUX document. */ 
    virtual void addToFUX(Guard &guard, class FUX::Element *doc) = 0;
    
protected:
    const EngineConfig &config;
    ProcessVariable pv;
    DisableFilter disable_filter;
    bool running;
    double period;         // .. in seconds
    CircularBuffer buffer; // Sample storage between disk writes.
    bool last_stamp_set;   // For catching 'back-in-time' at the
    epicsTime last_stamp;  // buffer level.
    bool have_sample_after_connection;

    /** Add a special 'event' value with given severity and time.
     *  <p>
     *  Time might actually be adjusted to be after the most recent
     *  sample in the archive.
     */
    void addEvent(Guard &guard, short severity, const epicsTime &when);
};

inline void SampleMechanism::addStateListener(Guard &guard,
                                       ProcessVariableStateListener *listener)
{
    pv.addStateListener(guard, listener);
}

inline void SampleMechanism::removeStateListener(Guard &guard,
                                       ProcessVariableStateListener *listener)
{
    pv.removeStateListener(guard, listener);
}

inline void SampleMechanism::addValueListener(Guard &guard,
                                       ProcessVariableValueListener *listener)
{
    pv.addValueListener(guard, listener);
}

inline void SampleMechanism::removeValueListener(Guard &guard,
                                       ProcessVariableValueListener *listener)
{
    pv.removeValueListener(guard, listener);
}

#endif /*SAMPLEMECHANISM_H_*/
