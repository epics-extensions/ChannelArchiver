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
    if (have_subscribed && channel && channel->chid_valid)
    {
#       ifdef DEBUG_SAMPLING
        LOG_MSG("'%s': Unsubscribing\n", channel->getName().c_str());
#       endif
        guard.unlock();
        engine_guard.unlock();
        ca_clear_subscription(ev_id);
        engine_guard.lock();
        guard.lock();
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: fully connected\n", channel->getName().c_str());
#       endif
        if (!have_subscribed)
        {
            guard.unlock();
            engine_guard.unlock();
            int status = ca_create_subscription(
                channel->dbr_time_type, channel->nelements, channel->ch_id,
                DBE_LOG | DBE_ALARM,
                ArchiveChannel::value_callback, channel, &ev_id);
            engine_guard.lock();
            guard.lock();
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
#       endif
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        wasWrittenAfterConnect = false;
        if (!channel->chid_valid)
            have_subscribed = false;
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
    //RawValue::show(stdout, channel->dbr_time_type,
    //               channel->nelements, value, &channel->ctrl_info);   
#   endif
    // Add every incoming value to buffer
    if (! channel->isBackInTime(stamp))
    {   // Add original sample as is w/o any problems
        channel->buffer.addRawValue(value);
        channel->last_stamp_in_archive = stamp;
    }
    if (!wasWrittenAfterConnect)
    {   // First value after a (re-)connect: add w/ host time,
        // maybe in addition to the previously logged sample w/
        // orig. time stamp.
        if (!channel->pending_value)
            return; // no temp. storage for tweaking value
        if (channel->isBackInTime(now))
        {
#           ifdef DEBUG_SAMPLING
            LOG_MSG("SampleMechanismMonitored::handleValue %s: "
                    "unresolved back in time\n", channel->getName().c_str());
#           endif
            return;
        }
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       channel->pending_value, value);
        RawValue::setTime(channel->pending_value, now);
        channel->buffer.addRawValue(channel->pending_value);
        channel->last_stamp_in_archive = now;
        wasWrittenAfterConnect = true;
    }
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: fully connected\n", channel->getName().c_str());
#       endif
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
#       endif
        flushPreviousValue(guard, channel->connection_time);
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
    guard.check(channel->mutex);
#   ifdef DEBUG_SAMPLING
    LOG_MSG("SampleMechanismGet::handleValue %s\n",channel->getName().c_str());
#   endif
    if (!wasWrittenAfterConnect)
    {
        RawValue::copy(channel->dbr_time_type, channel->nelements,
                       previous_value, value);
        if (channel->isBackInTime(stamp))
        {
            if (channel->isBackInTime(now))
            {
#               ifdef DEBUG_SAMPLING
                LOG_MSG("SampleMechanismGet::handleValue %s: "
                        "unresolved back in time\n",
                        channel->getName().c_str());
#               endif                
                return;
            }
            RawValue::setTime(previous_value, now);
        }
        channel->buffer.addRawValue(previous_value);
        if (!isValidTime(RawValue::getTime(previous_value)))
        {
            LOG_MSG("'%s': SampleMechanismGet added bad time stamp!\n",
                    channel->getName().c_str());
        }
        previous_value_set = true;
        channel->last_stamp_in_archive = stamp;
        wasWrittenAfterConnect = true;
        return;
    }
    if (previous_value_set)
    {
        if (RawValue::hasSameValue(channel->dbr_time_type, channel->nelements,
                                   channel->dbr_size, previous_value, value))
        {
#           ifdef DEBUG_SAMPLING
            LOG_MSG("SampleMechanismGet: repeat value\n");
#           endif
            if (++repeat_count >= max_repeat_count)
            {   // Forced flush, keep the repeat value
                flushPreviousValue(guard, now);
                previous_value_set = true;
            }
            return;
        }
        // New value: Flush repeats, add current value.
        flushPreviousValue(guard, stamp);
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
    if (!isValidTime(RawValue::getTime(value)))
    {
        LOG_MSG("'%s': SampleMechanismGet(2) added bad time stamp!\n",
                channel->getName().c_str());
    }
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

void SampleMechanismGet::flushPreviousValue(Guard &guard,
                                            const epicsTime &stamp)
{
    guard.check(channel->mutex);
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
        if (!isValidTime(RawValue::getTime(previous_value)))
        {
            LOG_MSG("'%s': flushPreviousValue( added bad time stamp!\n",
                    channel->getName().c_str());
        }
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
    if (have_subscribed && channel && channel->chid_valid)
    {
#       ifdef DEBUG_SAMPLING
        LOG_MSG("'%s': Unsubscribing\n", channel->getName().c_str());
#       endif
        guard.unlock();
        engine_guard.unlock();
        ca_clear_subscription(ev_id);
        engine_guard.lock();
        guard.lock();
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: fully connected\n", channel->getName().c_str());
#       endif
        if (!have_subscribed)
        {
            guard.unlock();
            engine_guard.unlock();
            int status = ca_create_subscription(
                channel->dbr_time_type, channel->nelements, channel->ch_id,
                DBE_LOG | DBE_ALARM,
                ArchiveChannel::value_callback, channel, &ev_id);
            engine_guard.lock();
            guard.lock();
            if (status != ECA_NORMAL)
            {
                LOG_MSG("'%s' ca_create_subscription failed: %s\n",
                        channel->getName().c_str(), ca_message(status));
                return;
            }
            theEngine->need_CA_flush = true;
            have_subscribed = true;
#           ifdef DEBUG_SAMPLING
            LOG_MSG("%s: subscribed to CA\n", channel->getName().c_str());
#           endif
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
#       ifdef DEBUG_SAMPLING
        LOG_MSG("%s: disconnected\n", channel->getName().c_str());
#       endif
        flushPreviousValue(guard, channel->connection_time);
        // Add a 'disconnected' value.
        channel->addEvent(guard, 0, ARCH_DISCONNECT, channel->connection_time);
        wasWrittenAfterConnect = false;
        if (!channel->chid_valid)
            have_subscribed = false;
    }   
}

void SampleMechanismMonitoredGet::handleValue(
    Guard &guard, const epicsTime &now,
    const epicsTime &stamp, const RawValue::Data *value)
{
    guard.check(channel->mutex);
    // Want to use SampleMechanismGet's handling of repeat counts etc.
    // ==> need to throttle the incoming CA monitors
    // down to the scan rate of this channel.
    // Next_sample_time determines if we can use the current
    // monitor of if we should ignore it.
    // Note that the result is slightly different from scanning:
    // Scanning every 10 seconds would store the most recent value
    // every 10 seconds (stamped just before the 10 second slots),
    // while periods of 10 secs for SampleMechanismMonitoredGet
    // actually store every value just after the 10 second slots.
    // Attempts to keep a copy of the last value and then use
    // the last one before a new time slot starts created
    // bookkeeping headaches, especially when trying to handle channels
    // that only have a single value ever during the runtime of the engine.
    //
    // Data from e.g. a BPM being read at 60 Hz, archived at 10 Hz,
    // should result in data for all BPMs from the same time slice.
    // While one cannot pick the time slice
    // - it's determined by roundTimeUp(stamp, channel->period) -
    // the engine will store values from the same time slice,
    // as long as the time stamps of the BPM values match
    // across BPM channels.
#   ifdef DEBUG_SAMPLING
    LOG_MSG("SampleMechanismMonitoredGet:\n");
    RawValue::show(stdout, channel->dbr_time_type, channel->nelements, value);
#   endif
    if (stamp > next_sample_time)
    {
        next_sample_time = roundTimeUp(stamp, channel->period);
#       ifdef DEBUG_SAMPLING
        stdString txt;
        LOG_MSG("SampleMechanismMonitoredGet: passed next_sample_time\n");
        LOG_MSG("next_sample_time=%s\n", epicsTimeTxt(next_sample_time, txt));
#       endif
        SampleMechanismGet::handleValue(guard, now, stamp, value);
    }
}
