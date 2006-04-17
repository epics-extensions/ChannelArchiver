// Tools
#include <MsgLogger.h>
// Engine
#include "ArchiveChannel.h"
#include "GroupInfo.h"
#include "SampleMechanismMonitored.h"
#include "SampleMechanismGet.h"
#include "SampleMechanismMonitoredGet.h"

#define DEBUG_ARCHIVE_CHANNEL
 
ArchiveChannel::ArchiveChannel(EngineConfig &config,
                               ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               const char *channel_name,
                               double scan_period, bool monitor)
    : NamedBase(channel_name), 
      scan_period(scan_period),
      monitor(monitor),
      currently_disabling(false),
      disable_count(0)
{
    Guard guard(*this);
    reconfigure(guard, config, ctx, scan_list);
    LOG_ASSERT(sample_mechanism);
}

ArchiveChannel::~ArchiveChannel()
{
    if (! state_listeners.empty())
        LOG_MSG("ArchiveChannel '%s' has %zu listeners left\n",
                getName().c_str(), state_listeners.size());
    if (sample_mechanism)
    {
        try
        {
            Guard guard(*sample_mechanism);
            sample_mechanism->removeStateListener(guard, this);
            if (canDisable(guard))  
                sample_mechanism->removeValueListener(guard, this);   
        }
        catch (...)
        {
        }
    } 
    sample_mechanism = 0;
}

epicsMutex &ArchiveChannel::getMutex()
{
    return mutex;
}

void ArchiveChannel::configure(Guard &guard,
                               EngineConfig &config,
                               ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               double scan_period, bool monitor)
{
    guard.check(__FILE__, __LINE__, mutex);
    // If anybody wants to monitor, use monitor
    if (monitor)
        this->monitor = true;
    // Is scan_period initialized?
    if (this->scan_period <= 0.0)
        this->scan_period = scan_period;
    else if (this->scan_period > scan_period) // minimize
        this->scan_period = scan_period;
    reconfigure(guard, config, ctx, scan_list);
}

void ArchiveChannel::reconfigure(Guard &guard,
                                 EngineConfig &config,
                                 ProcessVariableContext &ctx,
                                 ScanList &scan_list)
{
    guard.check(__FILE__, __LINE__, mutex);
    // See if this is a re-config, i.e. something's already running.
    bool was_running;
    if (sample_mechanism)
    {
        LOG_MSG("ArchiveChannel '%s' reconfig...\n",
                getName().c_str());
        Guard sample_guard(*sample_mechanism); // Lock: Channel, SampleMech.       
        was_running = sample_mechanism->isRunning(sample_guard);
        if (was_running)
            sample_mechanism->stop(sample_guard);
        sample_mechanism->removeStateListener(sample_guard, this);
        if (canDisable(guard))  
            sample_mechanism->removeValueListener(sample_guard, this);
    }
    else
        was_running = false;
    // Determine new mechanism
    sample_mechanism = 0;
    if (monitor)
        sample_mechanism = new SampleMechanismMonitored(config, ctx,
                                                        getName().c_str(),
                                                        scan_period);
    else if (scan_period >= config.getGetThreshold())
        sample_mechanism = new SampleMechanismGet(config, ctx, scan_list,
                                                  getName().c_str(),
                                                  scan_period);
    else
        sample_mechanism = new SampleMechanismMonitoredGet(config, ctx,
                                                           getName().c_str(),
                                                           scan_period);
    LOG_ASSERT(sample_mechanism);
    Guard sample_guard(*sample_mechanism); // Lock: Channel, SampleMech.      
    sample_mechanism->addStateListener(sample_guard, this);
    if (canDisable(guard))  
        sample_mechanism->addValueListener(sample_guard, this);
    // Possibly, start again
    if (was_running)
    {
        sample_mechanism->start(sample_guard);
        if (isDisabled(guard))
            sample_mechanism->disable(sample_guard, epicsTime::getCurrent());
    }
    LOG_MSG("ArchiveChannel '%s' reconfig... done\n",
            getName().c_str());        
}

void ArchiveChannel::addToGroup(Guard &group_guard, GroupInfo *group,
                                Guard &channel_guard, bool disabling)
{
    channel_guard.check(__FILE__, __LINE__, mutex);
    stdList<GroupInfo *>::iterator i;
    // Add to the group list
    bool add = true;
    for (i=groups.begin(); i!=groups.end(); ++i)
    {
        if (*i == group)
        {
            add = false;
            break;
        }
    }
    if (add)
    {
        groups.push_back(group);
        // Is the group disabled?      
        if (! group->isEnabled(group_guard))
        {  
            LOG_MSG("Channel '%s': added to disabled group '%s'\n",
                    getName().c_str(), group->getName().c_str());
            disable(channel_guard, epicsTime::getCurrent());
        }
    }       
    // Add to the 'disable' groups?
    if (disabling)
    {
        // Is this the first time we become 'disabling',
        // i.e. not diabling, yet?
        // --> monitor values!
        if (! canDisable(channel_guard))
        {
            Guard sample_guard(*sample_mechanism);                
            sample_mechanism->addValueListener(sample_guard, this);      
        }      
        // Add, but only once.
        add = true;
        for (i=disable_groups.begin(); i!=disable_groups.end(); ++i)
        {
            if (*i == group)
            {
                add = false;
                break;
            }
        }
        if (add)
        {
            disable_groups.push_back(group);
            // Is the channel already 'disabling'?
            // Then disable that new group right away.
            if (currently_disabling)
            {
                epicsTime when = epicsTime::getCurrent();
                LOG_MSG("Channel '%s' disables '%s' right away\n",
                        getName().c_str(), group->getName().c_str());  
                group->disable(group_guard, this, when);
            }
        }
    }
}

void ArchiveChannel::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    Guard sample_guard(*sample_mechanism);                    
    if (sample_mechanism->isRunning(sample_guard))
        throw GenericException(__FILE__, __LINE__,
                               "Channel '%s' started twice",
                               getName().c_str()); 
    sample_mechanism->start(sample_guard);
}
      
bool ArchiveChannel::isConnected(Guard &guard) const
{
    Guard sample_guard(*sample_mechanism);                
    return sample_mechanism->getPVState(sample_guard)
                                 == ProcessVariable::CONNECTED;
}
     
// Called by group to disable channel.
// Doesn't mean that this channel itself can disable,
// that's handled in pvValue() callback!
void ArchiveChannel::disable(Guard &guard, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    ++disable_count;
    if (disable_count > groups.size())
    {
        LOG_MSG("ERROR: Channel '%s' disabled %zu times?\n",
                getName().c_str(), disable_count);
        return;
    }
    if (isDisabled(guard))
    {
        LOG_MSG("Channel '%s' disabled\n", getName().c_str());  
        Guard sample_guard(*sample_mechanism);                
        sample_mechanism->disable(sample_guard, when);
    }
#if 0
    // TODO:
    // In case we're asked to disconnect _and_ this channel
    // doesn't need to stay connected because it disables other channels,
    // stop CA
    if (theEngine->disconnectOnDisable(engine_guard) && groups_to_disable.empty())
        stop(engine_guard, guard);
#endif
}
 
// Called by group to re-enable channel.
void ArchiveChannel::enable(Guard &guard, const epicsTime &when)
{
    guard.check(__FILE__, __LINE__, mutex);
    if (disable_count <= 0)
    {
        LOG_MSG("ERROR: Channel '%s' enabled while not disabled?\n",
                getName().c_str());
        return;
    }
    --disable_count;
    if (!isDisabled(guard))
    {
        LOG_MSG("Channel '%s' enabled\n", getName().c_str());   
        Guard sample_guard(*sample_mechanism);
        sample_mechanism->enable(sample_guard, when);
    }
#if 0
    // TODO:
    // In case we're asked to disconnect _and_ this channel
    // doesn't need to stay connected because it disables other channels,
    // stop CA
    if (theEngine->disconnectOnDisable(engine_guard) && groups_to_disable.empty())
        stop(engine_guard, guard);
#endif
}    
          
stdString ArchiveChannel::getSampleInfo(Guard &guard)
{
    Guard sample_guard(*sample_mechanism);
    return sample_mechanism->getInfo(sample_guard);
}
     
void ArchiveChannel::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    Guard sample_guard(*sample_mechanism);
    if (! sample_mechanism->isRunning(sample_guard))
        throw GenericException(__FILE__, __LINE__,
                               "Channel '%s' stopped while not running",
                               getName().c_str()); 
    sample_mechanism->stop(sample_guard);
}

unsigned long  ArchiveChannel::write(Guard &guard, Index &index)
{
    guard.check(__FILE__, __LINE__, mutex);
    Guard sample_guard(*sample_mechanism);
    return sample_mechanism->write(sample_guard, index);
}

void ArchiveChannel::addStateListener(
    Guard &guard, ArchiveChannelStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    stdList<ArchiveChannelStateListener *>::iterator l;
    for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
        if (*l == listener)
            throw GenericException(__FILE__, __LINE__,
                                   "Duplicate listener for '%s'",
                                   getName().c_str());
    state_listeners.push_back(listener);                              
}

void ArchiveChannel::removeStateListener(
    Guard &guard, ArchiveChannelStateListener *listener)
{
    guard.check(__FILE__, __LINE__, mutex);
    state_listeners.remove(listener);                              
}

// ArchiveChannel is StateListener to SampleMechanism (==PV)
void ArchiveChannel::pvConnected(Guard &pv_guard, ProcessVariable &pv,
                                 const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is connected\n", getName().c_str());
#endif
    GuardRelease release(pv_guard);
    {
        Guard guard(*this); // Lock order: only Channel
        // Notify listeners
        stdList<ArchiveChannelStateListener *>::iterator l;
            for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
              (*l)->acConnected(guard, *this, when);   
        // Notify groups
        stdList<GroupInfo *>::iterator gi;
        for (gi = groups.begin(); gi != groups.end(); ++gi)
        {
            GroupInfo *g = *gi;
            GuardRelease release(guard);
            {
                Guard group_guard(*g); // Lock Order: only Group
                g->incConnected(group_guard, *this);
            }
        }
    }
}

// ArchiveChannel is StateListener to SampleMechanism (==PV)
void ArchiveChannel::pvDisconnected(Guard &pv_guard, ProcessVariable &pv,
                    const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is disconnected\n", getName().c_str());
#endif
    GuardRelease pv_release(pv_guard);
    {
        Guard guard(*this); // Lock order: Only Channel.
        // Notify listeners
        stdList<ArchiveChannelStateListener *>::iterator l;
        {
            for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
                (*l)->acDisconnected(guard, *this, when);    
        }
        // Notify groups
        stdList<GroupInfo *>::iterator gi;
        for (gi = groups.begin(); gi != groups.end(); ++gi)
        {
            GroupInfo *g = *gi;
            GuardRelease release(guard);
            {
                Guard group_guard(*g); // Lock order: Only Group.
                g->decConnected(group_guard, *this);
            }
        }
    }
}

// ArchiveChannel is ValueListener to SampleMechanism (==PV) _IF_ disabling
void ArchiveChannel::pvValue(Guard &pv_guard, ProcessVariable &pv,
                             const RawValue::Data *data)
{
    bool should_disable = RawValue::isAboveZero(pv.getDbrType(pv_guard), data);
    GuardRelease pv_release(pv_guard);
    {
        Guard guard(*this); // Lock order: Only Channel
        if (!canDisable(guard))
        {
            LOG_MSG("ArchiveChannel '%s' got value for disable test "
                    "but not configured to disable\n",
                    getName().c_str());
            return;
        }        
        if (should_disable)
        {
            //LOG_MSG("ArchiveChannel '%s' got disabling value\n",
            //        getName().c_str());
            if (currently_disabling) // Was and still is disabling
                return;
            // Wasn't, but is now disabling.
            LOG_MSG("ArchiveChannel '%s' disables its groups\n",
                    getName().c_str());
            currently_disabling = true;
            // Notify groups
            stdList<GroupInfo *>::iterator gi;
            epicsTime when = RawValue::getTime(data);
            for (gi = disable_groups.begin(); gi != disable_groups.end(); ++gi)
            {
                GroupInfo *g = *gi;
                GuardRelease release(guard);
                {
                    Guard group_guard(*g);  // Lock order: Only Group.
                    g->disable(group_guard, this, when);
                }
            }
        }
        else
        {
            //LOG_MSG("ArchiveChannel '%s' got enabling value\n",
            //        getName().c_str());
            if (! currently_disabling) // Wasn't and isn't disabling.
                return;
            // Re-enable groups.
            LOG_MSG("ArchiveChannel '%s' enables its groups\n",
                    getName().c_str());
            currently_disabling = false;
            // Notify groups
            stdList<GroupInfo *>::iterator gi;
            epicsTime when = RawValue::getTime(data);
            for (gi = disable_groups.begin(); gi != disable_groups.end(); ++gi)
            {
                GroupInfo *g = *gi;
                GuardRelease release(guard);
                {
                    Guard group_guard(*g);  // Lock order: Only Group.
                    g->enable(group_guard, this, when);
                }
            }
        }
    }
}

void ArchiveChannel::addToFUX(Guard &guard, class FUX::Element *doc)
{
    FUX::Element *channel = new FUX::Element(doc, "channel");
    new FUX::Element(channel, "name", getName());
    sample_mechanism->addToFUX(guard, channel);
    if (canDisable(guard))
        new FUX::Element(channel, "disable");
}
