// --------------------- -*- c++ -*- ----------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------
#ifndef __GROUPINFO_H__
#define __GROUPINFO_H__

// System
#include <stdlib.h>
// Base
#include <epicsTime.h>
/// Tools
#include <ToolsConfig.h>
#include <Guard.h>

/// Each channel, identified by an ArchiveChannel,
/// belongs to at least one group. GroupInfo handles
/// one such group.
/// A channel can disable its group.
///
/// This is double-linked:
/// Each ArchiveChannel indicates membership to several groups
/// so that a channel can disable its groups.
/// Each GroupInfo knows all it's members in order
/// to disable them.
class GroupInfo
{
public:
    GroupInfo(const stdString &name);

    /// Name of this group
    const stdString &getName() const
    { return name; }
    
    /// Unique ID (within one ArchiveEngine). 0 ... (#groups-1)

    /// ArchiveChannel maintains a bitset of groups that it disabled.
    /// This ID is used as an index into the bitset.
    ///
    size_t getID() const
    { return ID; }

    /// Add channel to this group. NOP if already group member.
    void addChannel(Guard &channel_guard, class ArchiveChannel *channel);

    /// Return current list of group members
    const stdList<class ArchiveChannel *>&getChannels () const
    { return members; }

    void disable(class ArchiveChannel *cause, const epicsTime &when);
    void enable(class ArchiveChannel *cause, const epicsTime &when);
    bool isEnabled() const
    { return disable_count <= 0; }

    /// # of channels in group that are connected
    size_t num_connected;

private:
    GroupInfo(const GroupInfo &); // not impl.
    GroupInfo & operator = (const GroupInfo &); // not impl.

    stdString              name;           // Well, guess what?
    static size_t          next_ID;        // next unused group ID
    size_t                 ID;             // ID of this group
    stdList<class ArchiveChannel *> members; 
    size_t                 disable_count;  // disabled by how many channels?
};

#endif //__GROUPINFO_H__
