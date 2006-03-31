#ifndef SAMPLEMECHANISM_H_
#define SAMPLEMECHANISM_H_

// Local
#include "Named.h"
#include "EngineConfig.h"
#include "ProcessVariable.h"
#include "CircularBuffer.h"

/**\ingroup Engine
 *  Sample Mechanism Interface: Scanned, monitored, ...
 *  <p>
 *  Uses 'virtual Named' so we  can implement other
 *  'Named' interfaces in derived SampleMechanisms and still
 *  only get one 'Named' base.
 */
class SampleMechanism : public virtual Named, public Guardable
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
    
    /** @see Guardable */
    epicsMutex &getMutex();
    
    /** Start the sample mechanism. */
    virtual void start(Guard &guard);    
    
    /** Stop sampling.
     *  @see #start()
     */
    virtual void stop(Guard &guard);
protected:
    const EngineConfig &config;
    ProcessVariable pv;
    double period; // .. in seconds
    CircularBuffer buffer;
};

#endif /*SAMPLEMECHANISM_H_*/
