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

bool SampleMechanism::isGoodTimestamp(const epicsTime &stamp,
                                      const epicsTime &now)
{
    if (! isValidTime(stamp))
    {
        stdString t;
        epicsTime2string(stamp, t);
        LOG_MSG("'%s': Invalid/null time stamp %s\n",
                channel->getName().c_str(), t.c_str());
        return false;
    }
    double future = stamp - now;
    if (future > theEngine->getIgnoredFutureSecs())
    {
        stdString t;
        epicsTime2string(stamp, t);
        LOG_MSG("'%s': Ignoring futuristic time stamp %s\n",
                channel->getName().c_str(), t.c_str());
        return false;
    }
    return true;
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

void SampleMechanismMonitored::handleConnectionChange(Guard &guard)
{
    stdList<GroupInfo *>::iterator g;
    stdList<class GroupInfo *> &groups = channel->getGroups(guard);
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
        // Tell groups that we are connected
        for (g=groups.begin(); g!=groups.end(); ++g)
            ++ (*g)->num_connected;
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        // Tell groups that we are disconnected
        for (g=groups.begin(); g!=groups.end(); ++g)
            -- (*g)->num_connected;
    }
}

void SampleMechanismMonitored::handleValue(Guard &guard,
                                           const epicsTime &now,
                                           const RawValue::Data *value)
{
    guard.check(channel->mutex);
    if (channel->isDisabled(guard))
    {   // park the value so that we can write it ASAP after being re-enabled
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       channel->pending_value, value);
        channel->pending_value_set = true;
    }
    else
    {
        LOG_MSG("SampleMechanismMonitored::handleValue %s\n",
                channel->name.c_str());
        //RawValue::show(stdout, channel->dbr_time_type,
        //               channel->nelements, value, &channel->ctrl_info);   
        // Add every monitor to the ring buffer, only check for back-in-time
        epicsTime stamp = RawValue::getTime(value);
        if (isGoodTimestamp(stamp, now))
        {
            if (isValidTime(channel->last_stamp_in_archive) &&
                stamp < channel->last_stamp_in_archive)
            {
                stdString stamp_txt;
                epicsTime2string(stamp, stamp_txt);
                LOG_MSG("'%s': received back-in-time stamp %s\n",
                        channel->getName().c_str(), stamp_txt.c_str());
            }
            else
            {
                channel->buffer.addRawValue(value);
                channel->last_stamp_in_archive = stamp;
            }
        }
    }
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

void SampleMechanismGet::handleConnectionChange(Guard &guard)
{
    stdList<class GroupInfo *> &groups = channel->getGroups(guard);
    stdList<GroupInfo *>::iterator g;
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
        // Tell groups that we are connected
        for (g=groups.begin(); g!=groups.end(); ++g)
            ++ (*g)->num_connected;
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        // Tell groups that we are disconnected
        for (g=groups.begin(); g!=groups.end(); ++g)
            -- (*g)->num_connected;
    }
}

void SampleMechanismGet::handleValue(Guard &guard,
                                     const epicsTime &now,
                                     const RawValue::Data *value)
{
    LOG_MSG("SampleMechanismGet::handleValue %s\n", channel->name.c_str());
#if 0
    if (channel->isDisabled(guard))
    {   // park the value so that we can write it ASAP after being re-enabled
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       channel->pending_value, value);
        channel->pending_value_set = true;
    }
    else
    {
        //LOG_MSG("SampleMechanismMonitored::value_callback %s\n",
        //        channel->name.c_str());
        //RawValue::show(stdout, channel->dbr_time_type,
        //               channel->nelements, value, &channel->ctrl_info);   
        // Add every monitor to the ring buffer, only check for back-in-time
        epicsTime stamp = RawValue::getTime(value);
        if (me->isGoodTimestamp(stamp, now))
        {
            if (isValidTime(channel->last_stamp_in_archive) &&
                stamp < channel->last_stamp_in_archive)
            {
                stdString stamp_txt;
                epicsTime2string(stamp, stamp_txt);
                LOG_MSG("'%s': received back-in-time stamp %s\n",
                        channel->getName().c_str(), stamp_txt.c_str());
            }
            else
            {
                channel->buffer.addRawValue(value);
                channel->last_stamp_in_archive = stamp;
            }
        }
    }
#endif
}
