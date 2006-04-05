#ifndef PROCESSVARIABLELISTENER_H_
#define PROCESSVARIABLELISTENER_H_

// Storage
#include <RawValue.h>

/**\ingroup Engine
 *  Listener for ProcessVariable info
 */
class ProcessVariableListener
{
public:
    /** Invoked when the pv connects.
     * 
     *  This means: connected and received control info.
     */
    virtual void pvConnected(class Guard &guard,
                             class ProcessVariable &pv,
                             const epicsTime &when) = 0;
    
    /** Invoked when the pv disconnects. */
    virtual void pvDisconnected(class Guard &guard,
                                class ProcessVariable &pv,
                                const epicsTime &when) = 0;

    /** Invoked when the pv has a new value.
     *
     *  Can be the result of a 'getValue' or 'subscribe'.
     */
    virtual void pvValue(class Guard &guard,
                         class ProcessVariable &pv,
                         const RawValue::Data *data) = 0;
};

#endif /*PROCESSVARIABLE_H_*/
