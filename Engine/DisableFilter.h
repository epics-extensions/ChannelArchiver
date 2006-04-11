#ifndef DISABLEFILTER_H_
#define DISABLEFILTER_H_

// Engine
#include "EngineConfig.h"
#include "ProcessVariableFilter.h"

/**\ingroup Engine
 *  A filter to block samples while disabled.
 *  <p>
 *  Meant to be installed right after the ProcessVariable:
 *  <p>
 *  @image html disable_filt.png
 *  <p>  
 *  When the SampleMechanism is disabled, it instructs the DisableFilter
 *  to block all further samples.
 *  Actually, it keeps a copy of the most recent sample,
 *  so we start out with that last good sample when 'enabled' again,
 *  without having to wait for another new sample.
 */
class DisableFilter : public ProcessVariableFilter
{
public:
    /** Construct a DisableFilter. */
    DisableFilter(ProcessVariableListener *listener);

    ~DisableFilter();
                               
    /** Temporarily disable sampling.
     *  @see enable()  */
    void disable(Guard &guard);
     
    /** Re-enable sampling.
     *  @see disable() */
    void enable(Guard &guard, ProcessVariable &pv, const epicsTime &when);                           
                                 
    // ProcessVariableListener
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when);
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);
private:
    /** Is filter disabled? */
    bool is_disabled;
    /** Is PV currently connected? */
    bool is_connected;
    /** Was PV connected when we got disabled? */
    bool was_connected;
    /** Last value received while disabled. */
    RawValueAutoPtr last_value;
};

#endif /*DISABLEFILTER_H_*/
