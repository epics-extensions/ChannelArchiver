#ifndef ENGINE_H_
#define ENGINE_H_

// Tools
#include <Guard.h>
// Engine
#include "EngineConfig.h"
#include "ScanList.h"
#include "ProcessVariableContext.h"
#include "GroupInfo.h"
#include "ArchiveChannel.h"
#include "ArchiveChannelStateListener.h"

/** \defgroup Engine Engine
 *  Classes related to the ArchiveEngine.
 *  <p>
 *  The Engine is what keeps track of the various groups of channels,
 *  periodically processes the one and only ScanList,
 *  starts the HTTPServer etc.
 *  <p>
 *  Fundamentally, the data flows from the ProcessVariable,
 *  i.e. the network client, through one or more ProcessVariableFilter
 *  types into the CircularBuffer of the SampleMechanism.
 *  The following image shows the main actors:
 *  <p>
 *  @image html engine_api.png
 *  <p>
 *  This is the lock order. When locking more than
 *  one object from the following list, they have to be taken
 *  in this order, for example: First lock the PV, then the PVCtx.
 *  <ol>
 *  <li>Engine
 *  <li>GroupInfo
 *  <li>ArchiveChannel
 *  <li>SampleMechanism (same lock as ProcessVariable)
 *  <li>ProcessVariable
 *  <li>ProcessVariableContext
 *  </ol>
 */
 
/** \ingroup Engine
 *  The archive engine.
 */
class Engine : public Guardable,
               public EngineConfigListener,
               public ArchiveChannelStateListener
{
public:
    /** Create Engine that writes to given index. */
    Engine(const stdString &index_name);
    
    virtual ~Engine();
    
    /** Guardable interface. */
    epicsMutex &getMutex();

    /** @return Returns the description. */
    const stdString &getIndexName(Guard &guard) const { return index_name; }

    /** Set the description string. */
    void setDescription(Guard &guard, const stdString &description);
    
    /** @return Returns the description. */
    const stdString &getDescription(Guard &guard) const { return description; }
    
    /** @return Returns the start time. */
    const epicsTime &getStartTime(Guard &guard) const   { return start_time; }

    /** @return Returns the next write time. */
    const epicsTime &getNextWriteTime(Guard &guard) const
    { return next_write_time; }
    
    
    
    const stdList<GroupInfo *> &getGroups(Guard &guard) const { return groups; }
    
    const stdList<ArchiveChannel *> &getChannels(Guard &guard) const
    { return channels; }

    size_t  getNumConnected(Guard &guard) const  { return num_connected; }

    ArchiveChannel *findChannel(Guard &engine_guard, const stdString &name);    

    GroupInfo *findGroup(Guard &engine_guard, const stdString &name);    
    
    bool isWriting(Guard &guard) const { return is_writing; }
    
    double getWriteDuration(Guard &guard) const    { return write_duration; }
    
    size_t  getWriteCount(Guard &guard) const      { return write_count; }
  
    double  getProcessDelayAvg(Guard &guard) const { return process_delay_avg;}
    
    const EngineConfig &getConfig(Guard &guard) const { return config; }
    
    /** Read the given config file. */
    void read_config(Guard &guard, const stdString &file_name);
    
    /** EngineConfigListener */
    void addChannel(const stdString &group_name,
                    const stdString &channel_name,
                    double scan_period,
                    bool disabling, bool monitor);
                    
    /** Start the sample mechanism.
     */        
    void start(Guard &guard);
      
    /** Stop sampling.
     */
    void stop(Guard &guard);
    
    /** Main process routine.
     *  @return Returns true if we should process again.
     */
    bool process();
                    
    /** Write all current buffers to disk.
     *  <p>
     *  Typically done within process(),
     *  also explicitly invoked when shutting down.
     */
    unsigned long write(Guard &guard);
    
    // ArchiveChannelStateListener
    void acConnected(Guard &guard, ArchiveChannel &pv, const epicsTime &when);
    
    // ArchiveChannelStateListener
    void acDisconnected(Guard &guard,ArchiveChannel &pv,const epicsTime &when);
private:
    epicsMutex                mutex;
    stdString                 index_name;
    stdString                 description;
    EngineConfigParser        config;
    ScanList                  scan_list;
    ProcessVariableContext    pv_context;
    stdList<GroupInfo *>      groups;   // scan-groups of channels
	stdList<ArchiveChannel *> channels; // all the channels
    size_t                    num_connected;
    bool                      is_writing;
    double                    write_duration;
    size_t                    write_count;
    double                    process_delay_avg;
    epicsTime                 start_time;
    epicsTime                 next_write_time;
    
};

#endif /*ENGINE_H_*/
