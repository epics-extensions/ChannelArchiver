#ifndef ARCHIVECHANNEL_H_
#define ARCHIVECHANNEL_H_

// Tools
#include <AutoPtr.h>
// Storage
#include <Index.h>
// Engine
#include "Named.h"
#include "SampleMechanism.h"
#include "EngineConfig.h"
#include "ScanList.h"
#include "ArchiveChannelStateListener.h"

/** \ingroup Engine
 *  One archived channel.
 */
class ArchiveChannel : public NamedBase,
                       public Guardable,
                       public ProcessVariableListener
{
public:
    /** Create channel with given name. */
    ArchiveChannel(EngineConfig &config, ProcessVariableContext &ctx,
                   ScanList &scan_list,
                   const char *channel_name,
                   double scan_period, bool monitor);

    virtual ~ArchiveChannel();

    /** @see Guardable */
    epicsMutex &getMutex();

    /** Configure or re-configure channel.
     *  <p>
     *  When re-configured, the scan period will be
     *  minimized, any 'monitor' overrides a 'non-monitor'
     *  configure call.
     */
    void configure(EngineConfig &config, ProcessVariableContext &ctx,
                   ScanList &scan_list,
                   double scan_period, bool monitor);
                
    /** Add channel to a group. */
    void addToGroup(Guard &group_guard, class GroupInfo *group,
                    Guard &channel_guard, bool disabling);


    /** @return Returns list of groups where this channel is a member. */
    const stdList<class GroupInfo *> getGroups(Guard &guard) const;

    /** @return Returns list of groups to disable. */
    const stdList<class GroupInfo *> getGroupsToDisable(Guard &guard) const;
                      
    /** Start the sample mechanism. */        
    void start(Guard &guard);
    
    /** @return Returns true if started and successfully connected. */
    bool isConnected(Guard &guard) const;
      
    /** @return Returns true if currently disabled. */
    bool isDisabled(Guard &guard) const;
    
    /** @return Returns string that describes the current sample mechanism
     *  and its state. */
    stdString getSampleInfo(Guard &guard);

    /** Stop sampling.
     */
    void stop(Guard &guard);
    
    /** Write samples to index.
     *  @return Returns numbe of samples written.
     */
    unsigned long write(Guard &guard, Index &index);

    /** Add a ProcessVariableStateListener. */
    void addStateListener(Guard &guard, ArchiveChannelStateListener *listener);

    /** Remove a ProcessVariableStateListener. */
    void removeStateListener(Guard &guard,
                             ArchiveChannelStateListener *listener);
                            
    /** Implements ProcessVariableStateListener by forwrding
     *  connect/disconnect info to ArchiveChannelStateListeners
     */
    void pvConnected(Guard &guard, ProcessVariable &pv, const epicsTime &when);
    
    /** Implements ProcessVariableStateListener by forwrding
     *  connect/disconnect info to ArchiveChannelStateListeners
     */
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when);
              
    /** Implements ProcessVariableValueListener for handling enable/disable */     
    void pvValue(Guard &guard, ProcessVariable &pv,
                 const RawValue::Data *data);

private:
    double scan_period;
    bool monitor;
    AutoPtr<SampleMechanism> sample_mechanism;

    // Groups that this channel disables (might be empty)
    stdList<class GroupInfo *> disable_groups;
    // Groups to which this channel belongs (at least one)
    stdList<class GroupInfo *> groups;
    
    stdList<ArchiveChannelStateListener *> state_listeners;
    
    /** @return Returns true if channel can disable any groups.
     *  That does not imply that it currently <b>does</b> disable
     *  anything.
     */
    bool canDisable() const;
    
    bool is_disabled;
    
    void reconfigure(EngineConfig &config, ProcessVariableContext &ctx,
                     ScanList &scan_list);
};

inline const stdList<class GroupInfo *>
    ArchiveChannel::getGroups(Guard &guard) const
{
    return groups;
}

inline const stdList<class GroupInfo *>
    ArchiveChannel::getGroupsToDisable(Guard &guard) const
{
    return disable_groups;
}
    
inline bool ArchiveChannel::isDisabled(Guard &guard) const
{
    return is_disabled;
}
    
inline stdString ArchiveChannel::getSampleInfo(Guard &guard)
{
    return sample_mechanism->getInfo(guard);
}

inline bool ArchiveChannel::canDisable() const
{
    return !disable_groups.empty();
}

#endif /*ARCHIVECHANNEL_H_*/
