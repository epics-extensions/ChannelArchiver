// Tools
#include <MsgLogger.h>
// Engine
#include "ArchiveChannel.h"
#include "GroupInfo.h"
#include "SampleMechanismMonitored.h"
#include "SampleMechanismGet.h"
#include "SampleMechanismMonitoredGet.h"

// #define DEBUG_ARCHIVE_CHANNEL

ArchiveChannel::ArchiveChannel(EngineConfig &config,
                               ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               const char *channel_name,
                               double scan_period, bool monitor)
    : NamedBase(channel_name), 
      scan_period(scan_period),
      monitor(monitor)
{
    reconfigure(config, ctx, scan_list);
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
            Guard guard(*this);
            sample_mechanism->removeStateListener(guard, this);   
        }
        catch (...)
        {
        }
    } 
    sample_mechanism = 0;
}

// Channel uses the mutex of the sample mechanism
epicsMutex &ArchiveChannel::getMutex()
{
    LOG_ASSERT(sample_mechanism);
    return sample_mechanism->getMutex();
}

void ArchiveChannel::configure(EngineConfig &config,
                               ProcessVariableContext &ctx,
                               ScanList &scan_list,
                               double scan_period, bool monitor)
{
    // If anybody wants to monitor, use monitor
    if (monitor)
        this->monitor = true;
    // Is scan_period initialized?
    if (this->scan_period <= 0.0)
        this->scan_period = scan_period;
    else if (this->scan_period > scan_period) // minimize
        this->scan_period = scan_period;
    reconfigure(config, ctx, scan_list);
}

void ArchiveChannel::reconfigure(EngineConfig &config,
                                 ProcessVariableContext &ctx,
                                 ScanList &scan_list)
{
    // See if this is a re-config, i.e. something's already running.
    bool was_running;
    if (sample_mechanism)
    {
        Guard guard(*sample_mechanism);
        was_running = sample_mechanism->isRunning(guard);
        if (was_running)
            sample_mechanism->stop(guard);
        sample_mechanism->removeStateListener(guard, this);
    }
    else
        was_running = false;
    // Determine new mechanism
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
    Guard guard(*sample_mechanism);
    sample_mechanism->addStateListener(guard, this);
    // Possibly, start again
    if (was_running)
        sample_mechanism->start(guard);
}

void ArchiveChannel::addToGroup(Guard &group_guard, GroupInfo *group,
                                Guard &channel_guard, bool disabling)
{
    channel_guard.check(__FILE__, __LINE__, getMutex());
    // Add to the 'disable' groups?
    if (disabling)
    {   // Remove & add as quick hack to add only once.
        disable_groups.remove(group);
        disable_groups.push_back(group);
    }
    // Add to the group list
    stdList<GroupInfo *>::iterator i;
    for (i=groups.begin(); i!=groups.end(); ++i)
        if (*i == group)
            return; // Already in there
    groups.push_back(group);
}

void ArchiveChannel::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    if (sample_mechanism->isRunning(guard))
        throw GenericException(__FILE__, __LINE__,
                               "Channel '%s' started twice",
                               getName().c_str()); 
    sample_mechanism->start(guard);
}
      
bool ArchiveChannel::isConnected(Guard &guard) const
{
    return sample_mechanism->getPVState(guard)
                                 == ProcessVariable::CONNECTED;
}
     
void ArchiveChannel::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, getMutex());
    if (! sample_mechanism->isRunning(guard))
        throw GenericException(__FILE__, __LINE__,
                               "Channel '%s' stopped while not running",
                               getName().c_str()); 
    sample_mechanism->stop(guard);
}

unsigned long  ArchiveChannel::write(Guard &guard, Index &index)
{
    guard.check(__FILE__, __LINE__, getMutex());
    return sample_mechanism->write(guard, index);
}

void ArchiveChannel::addStateListener(
    Guard &guard, ArchiveChannelStateListener *listener)
{
    guard.check(__FILE__, __LINE__, getMutex());
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
    guard.check(__FILE__, __LINE__, getMutex());
    state_listeners.remove(listener);                              
}

void ArchiveChannel::pvConnected(Guard &guard, ProcessVariable &pv,
                                 const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is connected\n", getName().c_str());
#endif
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
            Guard group_guard(*g);
            g->incConnected(group_guard, *this);
        }
    }
}

void ArchiveChannel::pvDisconnected(Guard &guard, ProcessVariable &pv,
                    const epicsTime &when)
{
#ifdef DEBUG_ARCHIVE_CHANNEL
    LOG_MSG("ArchiveChannel '%s' is disconnected\n", getName().c_str());
#endif
    // Notify listeners
    stdList<ArchiveChannelStateListener *>::iterator l;
    for (l = state_listeners.begin(); l != state_listeners.end(); ++l)
        (*l)->acDisconnected(guard, *this, when);    
    // Notify groups
    stdList<GroupInfo *>::iterator gi;
    for (gi = groups.begin(); gi != groups.end(); ++gi)
    {
        GroupInfo *g = *gi;
        GuardRelease release(guard);
        {
            Guard group_guard(*g);
            g->decConnected(group_guard, *this);
        }
    }
}
    
