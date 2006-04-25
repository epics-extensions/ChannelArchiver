#ifndef PROCESSVARIABLECONTEXT_H_
#define PROCESSVARIABLECONTEXT_H_

// Tools
#include <Guard.h>

/**\ingroup Engine
 *  Context for ProcessVariable instances.
 *  <p>
 *  ChannelAccess assigns PVs to contexts.
 *  This one helps to use the same context across threads.
 *  <p>
 *  Uses a reference count to check that
 *  all PVs which used it were cleared when the context
 *  is destroyed.
 *  <p>
 *  Also helps to avoid some deadlocks:
 *  We cannot re-configure the Engine while CA callbacks
 *  arrive, since we cannot lock and change the ArchiveChannel
 *  and GroupInfo lists while CA callbacks might change the
 *  connection status of groups or the engine,
 *  or while callbacks disable/enable groups.
 *  <p>
 *  But even stopping all the channels can result in a deadlock,
 *  since the engine is locked, looping over all channels and
 *  eventually invoking ca_clear_channel,
 *  while at the same time a "connect" callback can try to
 *  increment the engine's connect count.
 *  <p>
 *  So when the engine stops, it'll stop the ProcessVariableContext,
 *  which sets a flag to ignore all CA callbacks inside the
 *  ProcessVariable.
 */
class ProcessVariableContext : public Guardable
{
public:
    /** Create the context. */
    ProcessVariableContext();
    
    /** Destructor. */
    virtual ~ProcessVariableContext();
    
    /** @see Guardable */
    epicsMutex &getMutex();
        
    /** Attach current thread to this context.
     *  <p>
     *  Except for the thread that created the context,
     *  all other threads that wish to use the context
     *  need to 'attach'.
     *  Unclear what happens when you don't, or when
     *  you attach more than once.
     *  @exception GenericException when detecting an error.
     */
    void attach(Guard &guard);
    
    /** @return Returns 'true' if the current thread is attached.
     *  <p>
     *  @see attach()
     */
    bool isAttached(Guard &guard);
    
    /** Add another reference. */
    void incRef(Guard &guard);
        
    /** Remove a reference. */    
    void decRef(Guard &guard);

    /** @return Returns current reference count. */    
    size_t getRefs(Guard &guard);
    
    /** Request a CA 'flush'.
     *  
     *  A 'connect' or 'get' often only becomes effective
     *  after a 'flush', but we don't want to 'flush' after
     *  every such operation because that would degrade performance.
     *  So this call requests a flush at a later time.
     *  @see #flush()
     */
    void requestFlush(Guard &guard);
    
    /** @return Returns true if a flush was requested. */
    bool isFlushRequested(Guard &guard);

    /** Perform the CA flush.
     * 
     *  Somebody, somehow should monitor 'isFlushRequested'
     *  and invoke 'flush' whenever requested.
     */    
    void flush(Guard &guard);
private:
    // The mutex
    epicsMutex mutex;
    
    // The context
    struct ca_client_context *ca_context;
    
    // The reference count
    size_t refs;
    
    bool flush_requested;
};


#endif /*PROCESSVARIABLECONTEXT_H_*/
