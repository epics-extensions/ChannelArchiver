#include "epicsTimeHelper.h"
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
        ca_clear_subscription(ev_id);
}

stdString SampleMechanismMonitored::description()
{
    char desc[100];
    sprintf(desc, "Monitored, max. period %g secs", channel->period);
    return stdString(desc);
}

void SampleMechanismMonitored::handleConnectionChange()
{
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
            theEngine->needCAflush();
        }
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->name.c_str());
        // TODO: Add a 'disconnected' value.
    }
}

void SampleMechanismMonitored::value_callback(struct event_handler_args args)
{
    SampleMechanismMonitored *me = (SampleMechanismMonitored *) args.usr;
    ArchiveChannel *channel = me->channel;
    channel->mutex.lock();

    if (channel->disabled)
    {
      channel->mutex.unlock();
      return;
    }    
    epicsTime now = epicsTime::getCurrent();
    const RawValue::Data *value = (const RawValue::Data *)args.dbr;
    
    LOG_MSG("SampleMechanismMonitored::value_callback %s\n",
            channel->name.c_str());
    RawValue::show(stdout, channel->dbr_time_type,
                   channel->nelements, value, &channel->ctrl_info);
    
    // Add every monitor unconditionally to the ring buffer,
    // but need to check for back-in-time
    epicsTime stamp = RawValue::getTime(value);
    if (me->isGoodTimestamp(stamp, now))
    {
        // Check for back-in-time
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
        channel->handleDisabling(value);
    }
    channel->mutex.unlock();
}





