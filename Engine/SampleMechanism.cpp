// Tools
#include "epicsTimeHelper.h"
#include "MsgLogger.h"
// Engine
#include "SampleMechanism.h"
#include "ArchiveChannel.h"
#include "Engine.h"

SampleMechanism::~SampleMechanism()
{}

bool SampleMechanism::isGoodTimestamp(const epicsTime &stamp,
                                      const epicsTime &now)
{
    if (! isValidTime(stamp))
    {
        stdString t;
        epicsTime2string(stamp, t);
        LOG_MSG("'%s': Invalid/null time stamp %s\n",
                channel->name.c_str(), t.c_str());
        return false;
    }
    double future = stamp - now;
    if (future > theEngine->getIgnoredFutureSecs())
    {
        stdString t;
        epicsTime2string(stamp, t);
        LOG_MSG("'%s': Ignoring futuristic time stamp %s\n",
                channel->name.c_str(), t.c_str());
        return false;
    }
    return true;
}

SampleMechanismMonitored::SampleMechanismMonitored()
{
    have_subscribed = false;
}

SampleMechanismMonitored::~SampleMechanismMonitored()
{
    if (have_subscribed)
    {
        LOG_MSG("'%s': Unsubscribing\n", channel->name.c_str());
        ca_clear_subscription(ev_id);
    }
}

stdString SampleMechanismMonitored::getDescription() const
{
    char desc[100];
    sprintf(desc, "Monitored, max. period %g secs", channel->period);
    return stdString(desc);
}

void SampleMechanismMonitored::handleConnectionChange()
{
    stdList<GroupInfo *>::iterator g;
    if (channel->connected)
    {
        LOG_MSG("%s: fully connected\n", channel->name.c_str());
        if (!have_subscribed)
        {
            int status = ca_create_subscription(
                channel->dbr_time_type, channel->nelements, channel->ch_id,
                DBE_LOG | DBE_ALARM,
                value_callback, this, &ev_id);
            if (status != ECA_NORMAL)
            {
                LOG_MSG("'%s' ca_create_subscription failed: %s\n",
                        channel->name.c_str(), ca_message(status));
                return;
            }
            theEngine->need_CA_flush = true;
            have_subscribed = true;
        }
        // CA should automatically send an initial monitor.
        // Tell groups that we are connected
        for (g=channel->groups.begin(); g!=channel->groups.end(); ++g)
            ++ (*g)->num_connected;
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->name.c_str());
        // Add a 'disconnected' value.
        channel->addEvent(0, ARCH_DISCONNECT, channel->connection_time);
        // Tell groups that we are disconnected
        for (g=channel->groups.begin(); g!=channel->groups.end(); ++g)
            -- (*g)->num_connected;
    }
}

void SampleMechanismMonitored::value_callback(struct event_handler_args args)
{
    SampleMechanismMonitored *me = (SampleMechanismMonitored *) args.usr;
    ArchiveChannel *channel = me->channel;
    epicsTime now = epicsTime::getCurrent();
    const RawValue::Data *value = (const RawValue::Data *)args.dbr;
    channel->mutex.lock();
    
    if (channel->isDisabled())
    {   // park the value so that we can write it ASAP after begin enabled
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       channel->pending_value, value);
        channel->pending_value_set = true;
    }
    else
    {
        LOG_MSG("SampleMechanismMonitored::value_callback %s\n",
                channel->name.c_str());
        RawValue::show(stdout, channel->dbr_time_type,
                       channel->nelements, value, &channel->ctrl_info);
        
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
                        channel->name.c_str(), stamp_txt.c_str());
            }
            else
            {
                channel->buffer.addRawValue(value);
                channel->last_stamp_in_archive = stamp;
            }
        }
    }
    channel->handleDisabling(value);
    channel->mutex.unlock();
}
