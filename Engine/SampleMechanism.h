#ifndef SAMPLEMECHANISM_H_
#define SAMPLEMECHANISM_H_

// Storage
#include <Index.h>
// Local
#include "Named.h"
#include "EngineConfig.h"
#include "ProcessVariable.h"
#include "CircularBuffer.h"

/**\ingroup Engine
 *  Sample Mechanism base.
 *  <p>
 *  This base class for all sample mechanisms maintains the
 *  ProcessVariable start/stop.
 *  Its ProcessVariableListener implementation logs "Disconnected"
 *  on pvDisconnected() and otherwise adds every pvValue in the buffer.
 *  <p>
 *  Uses 'virtual Named' so we can implement other
 *  'Named' interfaces in derived SampleMechanisms and still
 *  only get one 'Named' base.
 */
class SampleMechanism : public ProcessVariableListener, public Guardable,
    public virtual Named
{
public:
    /** Construct mechanism for given period.
     *  @param config The global configuration.
     *  @param ctx    The ProcessVariableContext to use for the pv.
     *  @param name   The pv name to connect to.
     *  @param period The sample period. Use differs with derived class.
     */
    SampleMechanism(const EngineConfig &config,
                    ProcessVariableContext &ctx, const char *name,
                    double period);
    virtual ~SampleMechanism();
    
    /** Gets the ProcessVariable name.
     *  @see NamedAbstractBase
     */
    const stdString &getName() const;
    
    /** @return Returns the PV mutex.
     *  @see Guardable */
    epicsMutex &getMutex();
    
    /** Start the sample mechanism.
     *  <p>
     *  Base implementation starts the PV.
     */
    virtual void start(Guard &guard);
    
    /** @return Returns true if start has been invoked until stop(). */
    bool isRunning(Guard &guard);
    
    /** @return Returns the state of the ProcessVariable. */
    ProcessVariable::State getPVState(Guard &guard);
    
    /** Stop sampling.
     *  @see #start()
     *  <p>
     *  Base implementation stops the PV and adds a 'STOPPED (OFF)' event.
     */
    virtual void stop(Guard &guard);
    
    /** @return Returns the number of samples in the circular buffer. */
    size_t getSampleCount(Guard &guard) const;
    
    /** ProcessVariableListener.
     *  <p>
     *  Base implementation allocates circular buffer
     */
    virtual void pvConnected(class Guard &guard,
                             class ProcessVariable &pv,
                             const epicsTime &when);
    
    /** ProcessVariableListener.
     *  <p>
     *  Base implementation adds a "DISCONNECTED" marker.
     */
    virtual void pvDisconnected(class Guard &guard,
                                class ProcessVariable &pv,
                                const epicsTime &when);

    /** ProcessVariableListener.
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
    
protected:
    const EngineConfig &config;
    ProcessVariable pv;
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

#endif /*SAMPLEMECHANISM_H_*/
