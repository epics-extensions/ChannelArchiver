#ifndef ENGINE_H_
#define ENGINE_H_

// Tools
#include <Guard.h>
// Engine
#include "EngineConfig.h"
#include "ScanList.h"
#include "ProcessVariableContext.h"
#include "ArchiveChannel.h"


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
class Engine : public Guardable, public EngineConfigListener
{
public:
    /** Create Engine that writes to given index and serves info at port. */
    Engine(const stdString &index_name, int port);
    
    virtual ~Engine();
    
    /** Guardable interface. */
    epicsMutex &getMutex();

    /** Set the description string. */
    void setDescription(Guard &guard, const stdString &description);
    
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
                    
private:
    epicsMutex              mutex;
    stdString               description;
    EngineConfigParser      config;
    ScanList                scan_list;
    ProcessVariableContext  pv_context;
	stdList<ArchiveChannel *> channels; // all the channels
    //stdList<GroupInfo *>  groups;   // scan-groups of channels
    
    double write_duration;
    size_t write_count;
    double process_delay_avg;
    epicsTime next_write_time;
    
    ArchiveChannel *findChannel(Guard &engine_guard, const stdString &name);
    
    unsigned long writeArchive(Guard &engine_guard);
};

#endif /*ENGINE_H_*/
