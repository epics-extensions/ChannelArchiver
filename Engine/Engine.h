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
 *  in this order, for example: First lock the Engine, then an ArchiveChannel,
 *  then the ProcessVariableContext.
 *  <ul>
 *  <li>(10) Engine
 *  <li>(20) GroupInfo
 *  <li>(30) ArchiveChannel
 *  <li>(40) SampleMechanism (same mutex as ProcessVariable)
 *  <li>(40) ProcessVariable
 *  <li>(50) ProcessVariableContext
 *  <li>(60) HTTPD Client List
 *  <li>(100) ThrottledMsgLogger
 *  </ul>
 *
 *  Beyond the reach of this code are locks internal to the CA client library.
 *  Possible lock chains:
 *  <ul>
 *  <li>Start the engine: Engine, ArchiveChannel, ProcessVariable, ca_...<br>
 *      Has to lock the Engine to prevent concurrent stop().
 *      This initial scenario created the lock order.
 *  <li>CA connect or control info callback: CA, PV, channel, group.<br>
 *      This is basically the reversed lock order!
 *      To preserve the lock order, the channel will release the PV lock
 *      before locking itself, and unlock itself before locking the group.
 *      This does assume that the configuration (group membership etc.)
 *      does not change!<br>
 *      The engine used to maintain a connection count that was updated
 *      in here as well, but locking the engine in here results in deadlocks
 *      with case 6.
 *  <li>CA value arrives: CA,  PV, channel.<br>
 *      Maybe enable/disable group, which locks group and other channels.
 *      As in 2, the lock order must be preserved.
 *  <li>HTTP client lists group or channel info:<br>
 *      Lock Engine, group or channel, maybe channel's PV and ca_state().<br>
 *      Must lock the engine to prevent concurrent engine changes.
 *      DEADLOCK with 2 or 3.
 *      <br>
 *      TODO: Handled by not keeping any locks?
 *  <li>Stop a channel (including from HTTPClient config update):
 *      channel, PV, then ca_*<br>
 *      Handled as part of 6.
 *  <li>Stop engine (including ...): engine, channel, PV, ca_<br>
 *      DEADLOCK with 2 or 3.
 *      <br>
 *      Must lock the engine to prevent concurrent engine changes.
 *  <li>Change channel or overall config<br>
 *      Handled after stopping the Engine, reload, then start.
 *      No new problems.
 *  </ul>
 *  The basic PV/CA conflicts need to be handled by PV lock releases
 *  whenever calling CA.
 *  <p>
 */
 
/** \ingroup Engine
 *  The archive engine.
 */
class Engine : public Guardable,
               public EngineConfigListener
{
public:
    static const double MAX_DELAY = 0.5;
    
    /** Create Engine that writes to given index. */
    Engine(const stdString &index_name);
    
    virtual ~Engine();
    
    /** Guardable interface. */
    OrderedMutex &getMutex();

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

    /** Join the engine's ProcessVariableContext.
     *  <p>
     *  While the engine supports multithreaded access,
     *  that's why there are all those Guard arguments,
     *  threads other than the main thread which end up
     *  calling ProcessVariable methods must join the
     *  PV context of the engine. */    
    void attachToProcessVariableContext(Guard &guard);
    
    /** @return Returns all groups. */    
    const stdList<GroupInfo *> &getGroups(Guard &guard) const { return groups; }
    
    /** @return Returns all channels. */    
    const stdList<ArchiveChannel *> &getChannels(Guard &guard) const
    { return channels; }

    /** @return Returns number of connected channels. */    
    size_t getNumConnected(Guard &guard) const;

    /** @return Returns channel with given name or 0. */    
    ArchiveChannel *findChannel(Guard &engine_guard, const stdString &name);    

    /** @return Returns group with given name or 0. */    
    GroupInfo *findGroup(Guard &engine_guard, const stdString &name);    
    
    /** @return Returns average write duration in seconds. */    
    double getWriteDuration(Guard &guard) const    { return write_duration; }
    
    /** @return Returns average write count. */    
    size_t  getWriteCount(Guard &guard) const      { return write_count; }
  
    /** @return Returns average delay in seconds spent waiting in process().
     *  <p>
     *  This could be considered the engine's idle time.
     */    
    double  getProcessDelayAvg(Guard &guard) const { return process_delay_avg;}
    
    /** @return Returns the current configuration. */        
    const EngineConfig &getConfig(Guard &guard) const { return config; }
    
    /** Read the given config file. */
    void read_config(Guard &guard, const stdString &file_name);
    
    /** Write a new config file. */
    void write_config(Guard &guard);

    /** Add a group.
     *  <p>
     *  If the group already exists, that existing group is returned.
     *  @return Returns the group
     */
    GroupInfo *addGroup(Guard &guard, const stdString &group_name);

    /** Add a channel.
     *  <p>
     *  Also an EngineConfigListener */
    void addChannel(const stdString &group_name,
                    const stdString &channel_name,
                    double scan_period,
                    bool disabling, bool monitor);
                    
    /** Start the sample mechanism. */        
    void start(Guard &guard);
      
    /** @return Returns true if start() has been called but not stop().
     *  @see start()     */
    bool isRunning(Guard &guard);      
      
    /** Stop sampling. */
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
    
private:
    OrderedMutex              mutex;
    bool                      is_running;
    epicsTime                 start_time;
    stdString                 index_name;
    stdString                 description;
    EngineConfigParser        config;
    ScanList                  scan_list;
    ProcessVariableContext    pv_context;
    stdList<GroupInfo *>      groups;   // scan-groups of channels
	stdList<ArchiveChannel *> channels; // all the channels
    double                    write_duration; // Updated by process()
    size_t                    write_count;    // process()
    double                    process_delay_avg;// process()
    epicsTime                 next_write_time;// process()
};

inline bool Engine::isRunning(Guard &guard)
{
    return is_running;
}

#endif /*ENGINE_H_*/
