// --------------------- -*- c++ -*- ----------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ARCHIVECHANNEL_H__
#define __ARCHIVECHANNEL_H__

// Base
#include <cadef.h>
// Tools
#include "Bitset.h"
#include "Guard.h"
// Storage
#include "CtrlInfo.h"
#include "CircularBuffer.h"
#include "SampleMechanism.h"

/// \ingroup Engine Used by the ArchiveEngine to define a 'Channel'.

/// An ArchiveChannel holds almost all the information
/// for an archived channel:
/// Name, CA connection info, value ring buffer.
///
/// The actual sampling details are handled by a SampleMechanism.
class ArchiveChannel
{
public:
    friend class SampleMechanism;
    friend class SampleMechanismMonitored;

    /// Create an ArchiveChannel
    ArchiveChannel(const stdString &name,
                   double period, SampleMechanism *mechanism);
    ~ArchiveChannel();
    
    /// mutex between CA, HTTPD, Engine.
    /// None of the ArchiveChannel methods take the lock,
    /// this is up to the caller.
    /// Exception: The CA callbacks.
    epicsMutex mutex;

    /// Get name of channel.
    const stdString &getName() const;

    /// Get period. Meaning depends on SampleMechanism.
    double getPeriod() const;

    /// A channel belongs to at least one group,
    /// and might disable some of the groups to which it belongs.
    stdList<class GroupInfo *> &getGroups(Guard &guard);
    
    /// Add a channel to a group & keep track of disabling.
    void addToGroup(Guard &guard, class GroupInfo *group, bool disabling);
    
    /// Get the current sample mechanism (never NULL).
    const SampleMechanism *getMechanism(Guard &guard) const;

    /// Start CA communication: Get control info, maybe subscribe, ...
    /// Can be called repeatedly, e.g. after changing the
    /// SampleMechanism or to toggle a re-get of the control information.
    void startCA(Guard &guard);

    /// Is the CA connection currently good?
    bool isConnected(Guard &guard) const;

    /// A set bit indicates a group that this channel disables.
    const BitSet &getGroupsToDisable(Guard &guard) const;

    /// Is this channel disabled?
    bool isDisabled(Guard &guard) const;

    /// Disable this channel.
    void disable(Guard &guard, const epicsTime &when);

    /// Enable this channel.
    void enable(Guard &guard, const epicsTime &when);

    /// Initialize value type etc.

    /// It's used by the engine for channels that are already in the
    /// archive, so we know ASAP what we'd otherwis only learn from the
    /// CA connection.
    /// Pass 0 to ctrl_info or last_stamp if they're unknown.
    void init(Guard &guard, DbrType dbr_time_type, DbrCount nelements,
              const CtrlInfo *ctrl_info = 0, const epicsTime *last_stamp = 0);
    
    /// Write current ring buffer content to archive.
    void write(Guard &guard, class IndexFile &index);

    /// Add an event to the buffer (special status/severity)

    /// The time might be adjusted to be >= the last stamp
    /// in the archive.
    ///
    void addEvent(Guard &guard, dbr_short_t status, dbr_short_t severity,
                  const epicsTime &time);
        
private:
    stdString   name;
    double      period; // Sample period, max period, ..(see SampleMechanism)
    SampleMechanism *mechanism;
    stdList<class GroupInfo *> groups;

    // CA Info and callbacks
    bool chid_valid;
    chid ch_id;
    static void connection_handler(struct connection_handler_args arg);
    bool setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx);
    static void control_callback(struct event_handler_args arg);

    // All from here down to '---' are only valid if connected==true
    bool            connected;
    epicsTime       connection_time;
    DbrType         dbr_time_type;
    DbrCount        nelements;  // == 0 -> data type is not known
    CtrlInfo        ctrl_info;
    // Value buffer in memory, later written to disk.
    // Should hold values arriving up to 'period' plus some
    CircularBuffer  buffer;
    // In case we are e.g. disabled, we park incoming values
    // here so that we can write the last one right after
    // being re-enabled
    bool            pending_value_set;
    RawValue::Data *pending_value;
    // ---
    
    // The mechanism: This or another channel of one of the groups
    // to which this channel belongs might disable a group.
    // The group then comes back and disables all its channels.
    int disabled_count; // See isDisabled()
    BitSet groups_to_disable; // Bit is set -> we disable that group
    bool currently_disabling; // Is this channel disabling its groups?
    void handleDisabling(Guard &guard, const RawValue::Data *value);

    // Bookkeeping and value checking stuff, used between ArchiveChannel
    // and SampleMechanism
    epicsTime last_stamp_in_archive; // for back-in-time checks
};

inline const stdString &ArchiveChannel::getName() const
{   return name; }    

inline double ArchiveChannel::getPeriod() const
{   return period; }    
    
inline stdList<class GroupInfo *> &ArchiveChannel::getGroups(Guard &guard)
{
    guard.check(mutex);
    return groups;
}

inline const SampleMechanism *ArchiveChannel::getMechanism(Guard &guard) const
{
    guard.check(mutex);
   return mechanism;
}

inline bool ArchiveChannel::isConnected(Guard &guard) const
{
    guard.check(mutex);
    return connected;
}

inline const BitSet &ArchiveChannel::getGroupsToDisable(Guard &guard) const
{
    guard.check(mutex);
    return groups_to_disable;
}

inline bool ArchiveChannel::isDisabled(Guard &guard) const
{
    guard.check(mutex);
    return disabled_count >= (int)groups.size();
}

#endif








