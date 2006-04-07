#ifndef PROCESSVARIABLE_H_
#define PROCESSVARIABLE_H_

// Base
#include <cadef.h>
// Tool
#include <Guard.h>
// Storage
#include <RawValue.h>
#include <CtrlInfo.h>
// local
#include "Named.h"
#include "ProcessVariableContext.h"
#include "ProcessVariableListener.h"

/**\ingroup Engine
 *  One process variable.
 *
 *  Handles the connect/disconnect,
 *  also requests the full information (DBR_CTRL_...)
 *  and from then on get/monitor will only
 *  get the DBR_TIME_...
 */
class ProcessVariable : public NamedBase, public Guardable
{
public:
    /** Create a ProcessVariable with given name. */
    ProcessVariable(ProcessVariableContext &ctx, const char *name);
    
    /** Destructor. */
    virtual ~ProcessVariable();

    /** @see Guardable */
    epicsMutex &getMutex();
    
    /** Possible states of a ProcessVariable. */
    enum State
    {
        /** Not initialized */
        INIT,
        /** Not connected, but trying to connect. */
        DISCONNECTED,
        /** Received CA connection callback, getting control info. */
        GETTING_INFO,
        /** Fully connected. */
        CONNECTED
    };
    
    /** @return Returns the current state. */
    State getState(Guard &guard) const;

    /** @return Returns the current state. */
    const char *getStateStr(Guard &guard) const;
    
    /** @return Returns the ChannelAccess state. */
    const char *getCAStateStr(Guard &guard) const;
    
    /** Get the DBR_TIME_... type of this PV.
     * 
     *  Only valid when getState() was CONNECTED
     */
    DbrType getDbrType(Guard &guard) const;
    
    /** Get the array size of this PV.
     * 
     *  Only valid when getState() was CONNECTED
     */
    DbrCount getDbrCount(Guard &guard) const;
    
    /** @return Returns the control information. */    
    const CtrlInfo &getCtrlInfo(Guard &guard) const;

    /** Add a ProcessVariableListener. */
    void addProcessVariableListener(Guard &guard, ProcessVariableListener *listener);

    /** Remove a ProcessVariableListener. */
    void removeProcessVariableListener(Guard &guard, ProcessVariableListener *listener);

    /** Start the connection mechanism. */
    void start(Guard &guard);
    
    /** Perform a single 'get'.
     *
     *  Value gets delivered to listeners.
     */
    void getValue(Guard &guard);

    /** Subscribe for value updates.
     *
     *  Values get delivered to listeners.
     */
    void subscribe(Guard &guard);

    /** @return Returns 'true' if we are subscribed. */
    bool isSubscribed(Guard &guard) const;

    /** Unsubscribe, no more updates. */
    void unsubscribe(Guard &guard);
    
    /** Disconnect.
     *  @see #start()
     */
    void stop(Guard &guard);
    
private:
    epicsMutex                         mutex;
    ProcessVariableContext             &ctx;
    State                              state;
    stdList<ProcessVariableListener *> listeners;
    chid                               id;
    evid                               ev_id;
    DbrType                            dbr_type;
    DbrCount                           dbr_count;
    CtrlInfo                           ctrl_info;
    size_t                             outstanding_gets;
    bool                               subscribed;
 
    // Channel Access callbacks   
    static void connection_handler(struct connection_handler_args arg);
    static void control_callback(struct event_handler_args arg);
    static void value_callback(struct event_handler_args);
    
    bool setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx);
};


inline DbrType ProcessVariable::getDbrType(Guard &guard) const
{
    return dbr_type;
}
    
inline DbrCount ProcessVariable::getDbrCount(Guard &guard) const
{
    return dbr_count;
}

inline const CtrlInfo &ProcessVariable::getCtrlInfo(Guard &guard) const
{
    return ctrl_info;
}

inline bool ProcessVariable::isSubscribed(Guard &guard) const
{
    return subscribed;
}

#endif /*PROCESSVARIABLE_H_*/
