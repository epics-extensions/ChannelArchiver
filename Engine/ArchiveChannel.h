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

#include <cadef.h>
#include "CtrlInfo.h"
#include "Bitset.h"
#include "CircularBuffer.h"
#include "SampleMechanism.h"

/// An ArchiveChannel holds almost all the information
/// for an archived channel:
/// Name, CA connection info, value ring buffer.
///
/// The actual sampling details are handled by a SampleMechanism
class ArchiveChannel
{
public:
    friend class SampleMechanism;
    friend class SampleMechanismMonitored;
    friend class GroupInfo;
    
    ArchiveChannel(const stdString &name,
                   double period, SampleMechanism *mechanism);
    ~ArchiveChannel();
    
    /// mutex between CA, HTTPD, Engine.
    /// None of the ArchiveChannel methods take the lock,
    /// this is up to the caller.
    /// Exception: The CA callbacks.
    epicsMutex mutex;

    const stdString &getName() const;

    double getPeriod() const;

    /// A channel belongs to at least one group,
    /// and might disable some of the groups to which it belongs
    stdList<class GroupInfo *> groups;

    /// Add a channel to a group & keep track of disabling
    void addToGroup(class GroupInfo *group, bool disabling);
    
    /// Get the current sample mechanism (never NULL)
    const SampleMechanism *getMechanism() const;

    /// Start CA communication: Get control info, maybe subscribe, ...
    /// Can be called repeatedly, e.g. after changing the
    /// SampleMechanism or to toggle a re-get of the control information
    void startCA();

    /// Is the CA connection currently good?
    bool isConnected() const;

    /// A set bit indicates a group that this channel disables.
    const BitSet &getGroupsToDisable() const;

    /// Is this channel disabled?
    bool isDisabled() const;
    
private:
    stdString       name;
    double          period; // Sample period, max period, ... up to SampleMechanism
    SampleMechanism *mechanism;

    // CA Info and callbacks
    bool chid_valid;
    chid ch_id;
    static void connection_handler(struct connection_handler_args arg);
    bool setup_ctrl_info(DbrType type, const void *dbr_ctrl_xx);
    static void control_callback(struct event_handler_args arg);

    // All from here down to '---' are only valid if connected==true
    bool          connected;
    epicsTime     connection_time;
    short         dbr_time_type;
    unsigned long nelements;
    CtrlInfo      ctrl_info;
    // Value buffer in memory, later written to disk.
    // Should hold values arriving up to 'period' plus some
    CircularBuffer  buffer;
    // ---
    
    /// The mechanism: This or another channel of one of the groups
    /// to which this channel belongs might diable a group.
    /// The group then comes back and disables all its channels.
    bool disabled;
    BitSet groups_to_disable; // Bit is set -> we disable that group
    bool currently_disabling; // is this channel currently disabling its groups?
    void handleDisabling(const RawValue::Data *value);
    
    // Bookkeeping and value checking stuff, used between ArchiveChannel
    // and SampleMechanism
    epicsTime last_stamp_in_archive; // for back-in-time checks
};

inline const stdString &ArchiveChannel::getName() const
{   return name; }    

inline double ArchiveChannel::getPeriod() const
{   return period; }    
    
inline const SampleMechanism *ArchiveChannel::getMechanism() const
{   return mechanism; }

inline bool ArchiveChannel::isConnected() const
{   return connected; }

inline const BitSet &ArchiveChannel::getGroupsToDisable() const
{   return groups_to_disable; }

inline bool ArchiveChannel::isDisabled() const
{   return disabled; }

#endif








