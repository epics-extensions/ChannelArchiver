// Tools
#include "epicsTimeHelper.h"
#include "MsgLogger.h"
// Engine
#include "SampleMechanism.h"
#include "ArchiveChannel.h"
#include "Engine.h"

#undef DEBUG_SAMPLING

SampleMechanism::SampleMechanism(class ArchiveChannel *channel)
        : channel(channel), wasWrittenAfterConnect(false)
{}

void SampleMechanism::destroy(Guard &engine_guard, Guard &guard)
{
    channel = 0;
    delete this;
}

// ---------------------------------------------------------------------------

SampleMechanismMonitored::SampleMechanismMonitored(ArchiveChannel *channel)
        : SampleMechanism(channel)
{
    have_subscribed = false;
}

void SampleMechanismMonitored::destroy(Guard &engine_guard, Guard &guard)
{
    if (have_subscribed)
    {
#       ifdef DEBUG_SAMPLING
        LOG_MSG("'%s': Unsubscribing\n", channel->getName().c_str());
#       endif
        ca_clear_subscription(ev_id);
    }
    SampleMechanism::destroy(engine_guard, guard);
}

stdString SampleMechanismMonitored::getDescription(Guard &guard) const
{
    char desc[100];
    sprintf(desc, "Monitored, max. period %g secs", channel->getPeriod(guard));
    return stdString(desc);
}

bool SampleMechanismMonitored::isScanning() const
{   return false; }

void SampleMechanismMonitored::handleConnectionChange(Guard &engine_guard,
                                                      Guard &guard)
{
    if (channel->isConnected(guard))
    {
        // LOG_MSG("%s: fully connected\n", channel->getName().c_str());
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
        //LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        wasWrittenAfterConnect = false;
    }
}

void SampleMechanismMonitored::handleValue(Guard &guard,
                                           const epicsTime &now,
                                           const epicsTime &stamp,
                                           const RawValue::Data *value)
{
    guard.check(channel->mutex);
#   ifdef DEBUG_SAMPLING
    LOG_MSG("SampleMechanismMonitored::handleValue %s\n",
            channel->getName().c_str());
#   endif
    //RawValue::show(stdout, channel->dbr_time_type,
    //               channel->nelements, value, &channel->ctrl_info);   
    // Add every monitor to the ring buffer
    if (channel->isBackInTime(stamp))
    {
        if (wasWrittenAfterConnect)
            return;
        // This is the first value after a connect: tweak
        if (!channel->pending_value)
            return;
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       channel->pending_value, value);
        if (channel->isBackInTime(now))
        {
#   ifdef DEBUG_SAMPLING
            LOG_MSG("SampleMechanismMonitored::handleValue %s: "
                    "unresolved back in time\n", channel->getName().c_str());
#   endif
            return;
        }
        RawValue::setTime(channel->pending_value, now);
        channel->buffer.addRawValue(channel->pending_value);
    }
    else
        channel->buffer.addRawValue(value);
    channel->last_stamp_in_archive = stamp;
    wasWrittenAfterConnect = true;
}

// ---------------------------------------------------------------------------

size_t SampleMechanismGet::max_repeat_count = 100;

SampleMechanismGet::SampleMechanismGet(class ArchiveChannel *channel)
        : SampleMechanism(channel),
          is_on_scanlist(false), previous_value_set(false),
          previous_value(0), repeat_count(0)
{}

void SampleMechanismGet::destroy(Guard &engine_guard, Guard &guard)
{
    if (is_on_scanlist)
    {
        ScanList &scanlist = theEngine->getScanlist(engine_guard);
        scanlist.removeChannel(channel);
    }
    if (previous_value)
    {
        previous_value_set = false;
        RawValue::free(previous_value);
        previous_value = 0;
    }
    SampleMechanism::destroy(engine_guard, guard);
}

stdString SampleMechanismGet::getDescription(Guard &guard) const
{
    char desc[100];
    sprintf(desc, "Get, %g sec period", channel->getPeriod(guard));
    return stdString(desc);
}

bool SampleMechanismGet::isScanning() const
{   return true; }

void SampleMechanismGet::handleConnectionChange(Guard &engine_guard,
                                                Guard &guard)
{
    if (channel->isConnected(guard))
    {
        //LOG_MSG("%s: fully connected\n", channel->getName().c_str());
        if (!is_on_scanlist)
        {
            ScanList &scanlist = theEngine->getScanlist(engine_guard);
            scanlist.addChannel(guard, channel);
            is_on_scanlist = true;
        }
        if (previous_value)
            RawValue::free(previous_value);
        previous_value = RawValue::allocate(channel->dbr_time_type,
                                            channel->nelements, 1);
        previous_value_set = false;
    }
    else
    {
        //LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        flushPreviousValue(channel->connection_time);
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        wasWrittenAfterConnect = false;
    }
}

void SampleMechanismGet::handleValue(Guard &guard,
                                     const epicsTime &now,
                                     const epicsTime &stamp,
                                     const RawValue::Data *value)
{
#   ifdef DEBUG_SAMPLING
    LOG_MSG("SampleMechanismGet::handleValue %s\n",
            channel->getName().c_str());
#   endif
    if (!wasWrittenAfterConnect)
    {
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       previous_value, value);
        if (channel->isBackInTime(stamp))
        {
            if (channel->isBackInTime(now))
            {
#   ifdef DEBUG_SAMPLING
                LOG_MSG("SampleMechanismGet::handleValue %s: "
                        "unresolved back in time\n",
                        channel->getName().c_str());
#   endif                
                return;
            }
            RawValue::setTime(previous_value, now);
        }
        channel->buffer.addRawValue(previous_value);
        previous_value_set = true;
        channel->last_stamp_in_archive = stamp;
        wasWrittenAfterConnect = true;
        return;
    }
    if (previous_value_set)
    {
        if (RawValue::hasSameValue(channel->dbr_time_type,
                                   channel->nelements,
                                   channel->dbr_size,
                                   previous_value, value))
        {
#           ifdef DEBUG_SAMPLING
            LOG_MSG("SampleMechanismGet: repeat value\n");
#           endif
            if (++repeat_count >= max_repeat_count)
            {   // Forced flush, keep the repeat value
                flushPreviousValue(now);
                previous_value_set = true;
            }
            return;
        }
        // New value: Flush repeats, add current value.
        flushPreviousValue(stamp);
    }
    // Note that while we're repeating the same value
    // over and over, the incoming data is back-in-time.
    // But now that we got a new value, it needs to be current.
    if (channel->isBackInTime(stamp))
        return;
#   ifdef DEBUG_SAMPLING
    LOG_MSG("SampleMechanismGet: accepted\n");
#   endif
    channel->buffer.addRawValue(value);
    channel->last_stamp_in_archive = stamp;
    // remember
    if (previous_value)
    {
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       previous_value, value);
        previous_value_set = true;
        repeat_count = 0;
    }
}

void SampleMechanismGet::flushPreviousValue(const epicsTime &stamp)
{
    if (previous_value_set == false || repeat_count <= 0)
        return;
#   ifdef DEBUG_SAMPLING
    LOG_MSG("'%s': Flushing %lu repeats\n",
            channel->getName().c_str(), (unsigned long) repeat_count);
#   endif
    if (stamp >= channel->last_stamp_in_archive)
    {
        RawValue::setStatus(previous_value, repeat_count, ARCH_REPEAT);
        RawValue::setTime(previous_value, stamp);
        channel->buffer.addRawValue(previous_value);
        channel->last_stamp_in_archive = stamp;
    }
    else
        LOG_MSG("'%s': Cannot flush repeat values; would go back in time\n",
                channel->getName().c_str());
    previous_value_set = false;
    repeat_count = 0;
}

SampleMechanismMonitoredGet::SampleMechanismMonitoredGet(
    ArchiveChannel *channel)
        : SampleMechanismGet(channel)
{
    have_subscribed = false;
}

void SampleMechanismMonitoredGet::destroy(Guard &engine_guard, Guard &guard)
{
    if (have_subscribed)
    {
#       ifdef DEBUG_SAMPLING
        LOG_MSG("'%s': Unsubscribing\n", channel->getName().c_str());
#       endif
        ca_clear_subscription(ev_id);
    }
    SampleMechanismGet::destroy(engine_guard, guard);
}

stdString SampleMechanismMonitoredGet::getDescription(Guard &guard) const
{
    char desc[100];
    sprintf(desc, "Get via monitor, %g sec period", channel->getPeriod(guard));
    return stdString(desc);
}

void SampleMechanismMonitoredGet::handleConnectionChange(Guard &engine_guard,
                                                         Guard &guard)
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
            LOG_MSG("%s: subscribed to CA\n", channel->getName().c_str());
       }
        // CA should automatically send an initial monitor.
        if (previous_value)
            RawValue::free(previous_value);
        previous_value = RawValue::allocate(channel->dbr_time_type,
                                            channel->nelements, 1);
        previous_value_set = false;
    }
    else
    {
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
        flushPreviousValue(channel->connection_time);
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        wasWrittenAfterConnect = false;
    }   
}

void SampleMechanismMonitoredGet::handleValue(
    Guard &guard, const epicsTime &now,
    const epicsTime &stamp, const RawValue::Data *value)
{
    // Pretend to SampleMechanismGet that this is the result of a CA get.
    // For that we have to throttle the incoming CA monitors
    // down to the scan rate of this channel.
    // Incoming monitors trigger processing of this mechanism,
    // next_sample_time determines when we store another sample.
    // The stored sample is then the *previous* one, the last one
    // we received *before* crossing next_sample_time.
    LOG_MSG("SampleMechanismMonitoredGet:\n");
    RawValue::show(stdout, channel->dbr_time_type, channel->nelements, value);
    if (!channel->pending_value)
    {
        LOG_MSG("SampleMechanismMonitoredGet: no pend buffer\n");
        return;
    }
    // TODO: Use now or stamp for next_sample_time comparison?
    // now: matches what we do in SampleMechanismGet
    // stamp: originates from better clock (in theory)
    if (channel->pending_value_set  &&  stamp > next_sample_time)
    {
        LOG_MSG("SampleMechanismMonitoredGet: passed next_sample_time\n");
        epicsTime pend_stamp = RawValue::getTime(channel->pending_value);
        SampleMechanismGet::handleValue(guard, now, pend_stamp,
                                        channel->pending_value);
        next_sample_time = roundTimeUp(pend_stamp, channel->period);

        stdString txt;
        LOG_MSG("next_sample_time=%s\n", epicsTimeTxt(next_sample_time, txt));
    }
    RawValue::copy(channel->dbr_time_type, channel->nelements,
                   channel->pending_value, value);
    channel->pending_value_set = true;
}
