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

/** \ingroup Engine
 *  One archived channel.
 */
class ArchiveChannel : public NamedBase, public Guardable
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
           
    /** Start the sample mechanism.
     */        
    void start(Guard &guard);
      
    /** Stop sampling.
     */
    void stop(Guard &guard);
    
    /** Write samples to index. */
    unsigned long write(Guard &guard, Index &index);
    
private:
    AutoPtr<SampleMechanism> sample_mechanism;
    double scan_period;
    bool monitor;
    
    void reconfigure(EngineConfig &config, ProcessVariableContext &ctx,
                     ScanList &scan_list);
};

#endif /*ARCHIVECHANNEL_H_*/
