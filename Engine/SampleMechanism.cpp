// Tools
#include "epicsTimeHelper.h"
#include "MsgLogger.h"
// Engine
#include "SampleMechanism.h"
#include "ArchiveChannel.h"
#include "Engine.h"


SampleMechanism::SampleMechanism(class ArchiveChannel *channel)
        : channel(channel)
{}

SampleMechanism::~SampleMechanism()
{
    channel = 0;
}

// ---------------------------------------------------------------------------

SampleMechanismMonitored::SampleMechanismMonitored(ArchiveChannel *channel)
        : SampleMechanism(channel)
{
    have_subscribed = false;
}

SampleMechanismMonitored::~SampleMechanismMonitored()
{
    if (have_subscribed)
    {
        LOG_MSG("'%s': Unsubscribing\n", channel->getName().c_str());
        ca_clear_subscription(ev_id);
    }
}

stdString SampleMechanismMonitored::getDescription(Guard &guard) const
{
    char desc[100];
    sprintf(desc, "Monitored, max. period %g secs", channel->getPeriod(guard));
    return stdString(desc);
}

bool SampleMechanismMonitored::isScanning() const
{   return false; }

void SampleMechanismMonitored::handleConnectionChange(Guard &guard)
{
    if (channel->isConnected(guard))
    {
        LOG_MSG("%s: fully connected\n", channel->getName().c_str());
        if (!have_subscribed)
        {
            int status = ca_create_subscription(
                channel->dbr_time_type, channel->nelements, channel->ch_id,
                DBE_LOG | DBE_ALARM,
                ArchiveChannel::value_callback, channel, &ev_id);
            if (status != ECA_NORMAL)
            {
                LOG_MSG("'%s' ca_create_subscription failed: %s\n",
                        channel->getName().c_str(), ca_message(status));
                return;
            }
            theEngine->need_CA_flush = true;
            have_subscribed = true;
        }
        // CA should automatically send an initial monitor.
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
    }
}

void SampleMechanismMonitored::handleValue(Guard &guard,
                                           const epicsTime &now,
                                           const epicsTime &stamp,
                                           const RawValue::Data *value)
{
    guard.check(channel->mutex);
    LOG_MSG("SampleMechanismMonitored::handleValue %s\n",
            channel->getName().c_str());
    //RawValue::show(stdout, channel->dbr_time_type,
    //               channel->nelements, value, &channel->ctrl_info);   
    // Add every monitor to the ring buffer, only check for back-in-time
    channel->buffer.addRawValue(value);
    channel->last_stamp_in_archive = stamp;
}

// ---------------------------------------------------------------------------

SampleMechanismGet::SampleMechanismGet(class ArchiveChannel *channel)
        : SampleMechanism(channel)
{
    is_on_scanlist = false;
}

SampleMechanismGet::~SampleMechanismGet()
{
    if (is_on_scanlist)
    {
        Guard engine_guard(theEngine->mutex);
        ScanList &scanlist = theEngine->getScanlist(engine_guard);
        scanlist.removeChannel(channel);
    }
}

stdString SampleMechanismGet::getDescription(Guard &guard) const
{
    char desc[100];
    sprintf(desc, "Get, %g sec period", channel->getPeriod(guard));
    return stdString(desc);
}

bool SampleMechanismGet::isScanning() const
{   return true; }

void SampleMechanismGet::handleConnectionChange(Guard &guard)
{
    if (channel->isConnected(guard))
    {
        LOG_MSG("%s: fully connected\n", channel->getName().c_str());
        if (!is_on_scanlist)
        {
            Guard engine_guard(theEngine->mutex);
            ScanList &scanlist = theEngine->getScanlist(engine_guard);
            scanlist.addChannel(guard, channel);
            is_on_scanlist = true;
        }
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
    }
}

void SampleMechanismGet::handleValue(Guard &guard,
                                     const epicsTime &now,
                                     const epicsTime &stamp,
                                     const RawValue::Data *value)
{
    LOG_MSG("SampleMechanismGet::handleValue %s\n",
            channel->getName().c_str());
    //RawValue::show(stdout, channel->dbr_time_type,
    //               channel->nelements, value, &channel->ctrl_info);   
    // Add every monitor to the ring buffer, only check for back-in-time
    // TODO: compare w/ remembered value for 'repeat'
    channel->buffer.addRawValue(value);
    channel->last_stamp_in_archive = stamp;
    // TODO: remember
}
