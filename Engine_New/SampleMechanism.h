#ifndef SAMPLEMECHANISM_H_
#define SAMPLEMECHANISM_H_

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
    
    /** Stop sampling.
     *  @see #start()
     *  <p>
     *  Base implementation stops the PV and adds a 'STOPPED (OFF)' event.
     */
    virtual void stop(Guard &guard);
    
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
     */
    virtual void pvValue(class Guard &guard,
                         class ProcessVariable &pv,
                         const RawValue::Data *data);
    
protected:
    const EngineConfig &config;
    ProcessVariable pv;
    double period; // .. in seconds
    CircularBuffer buffer;
    bool last_stamp_set;
    epicsTime last_stamp;

    /** Add a special 'event' value with given severity and time.
     *  <p>
     *  Time might actually be adjusted to be after the most recent
     *  sample in the archive.
     */
    void addEvent(Guard &guard, short severity, const epicsTime &when);
};

#endif /*SAMPLEMECHANISM_H_*/
