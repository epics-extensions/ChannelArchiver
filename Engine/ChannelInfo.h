// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __CHANNELINFO_H__
#define __CHANNELINFO_H__

#include <cadef.h>
#include "CircularBuffer.h"
#include "Bitset.h"
#include "ArchiveI.h"

class GroupInfo; // forward

// For testing only,
// expects all channels to be 0...10 ramps,
// checks for missing values:
// #define CA_STATISTICS

//CLASS ChannelInfo
// Information about an Engine's Channel to be archived
// (class Channel represents a channel already in the Archive)
class ChannelInfo
{
public:
    ChannelInfo();
    ~ChannelInfo();

    // Archiver configuration for this channel
    // ------------------------------------------------------------
    void setName(const stdString &name)       { _name = name; }
    const stdString &getName() const          { return _name; }

    // How is the current value fetched?
    typedef enum { none, use_monitor, use_get } Mechanism;
    Mechanism getMechanism() const            { return _mechanism; }

    const stdList<GroupInfo *> &getGroups() const { return _groups; }
    void addToGroup(GroupInfo *group, bool disabling);

    double getPeriod() const                  { return _period; }
    void setPeriod(double secs);

    // Values --------
    // Define the type of value for this ChannelInfo.
    // Result: has the type changed ?
    bool setValueType(DbrType type, DbrCount count);
    void setCtrlInfo(const CtrlInfoI *info);

    void setLastArchiveStamp(const osiTime &stamp)
                                           { _last_archive_stamp = stamp; }

    const CtrlInfoI &getCtrlInfo() const      { return _ctrl_info; }
    size_t getValsPerBuffer() const           { return _vals_per_buffer; }

    // Channel Access
    // ------------------------------------------------------------
    // monitored or scanned?
    bool isMonitored() const                  { return _monitored; }
    void setMonitored(bool monitored)         { _monitored = monitored; }
    bool isConnected() const                  { return _connected; }
    const osiTime &getConnectTime() const     { return _connectTime; }
    const char *getHost() const;

    void startCaConnection(bool new_channel);

    // Issue CA get for _value, no ca_pend_io in here!
    void issueCaGet();

    // Called by Engine
    // ------------------------------------------------------------

    // Called from caEventHandler or ScanList, _value is already set
    void handleNewValue();

    // For scanned channels,
    // handleNewValue won't put repeated values
    // in the ring buffer unless there's a change.
    // This call will force it to write the
    // repeat count out up to 'now'.
    size_t flushRepeats(const osiTime &now);

    void addEvent(dbr_short_t status, dbr_short_t severity,
                  const osiTime &time);

    // (Try to) enable/disable this channel
    void disable(ChannelInfo *cause);
    void enable(ChannelInfo *cause);
    // Info about disabling behaviour of this channel
    const BitSet &getDisabling() const { return _disabling; }
    bool isDisabling(const GroupInfo *group) const;
    bool isDisabled() const            { return _disabled >= _groups.size(); }

    void write(class Archive &archive, ChannelIterator &channel);

#ifdef CA_STATISTICS
    int _next_CA_value;
    static size_t _missing_CA_values;
#endif

    // Check if Ring buffer is big enough, fits _value etc.
    void checkRingBuffer();

private:
    enum
    {
    INIT_VALS_PER_BUF = 16,
    BUF_GROWTH_RATE = 4,
    MAX_VALS_PER_BUF = 1000
    };

    // These could be sorted in order to minimize the
    // memory footprint.
    // For now they are in some more or less meaningful order.
    stdString           _name;
    bool                _monitored;    // monitored or scanned?
    double              _period;       // (expected) period in secs
    stdList<GroupInfo *> _groups;      // Groups that this channel belongs to

    BitSet              _disabling;    // bit set->Channel disables group
    size_t              _disabled;     // disabled by how many groups?
    bool                _currently_disabling;// does it right now?

    bool                _connected;    // CA: currently connected?
    bool                _ever_written;// ever archived since connected?
    chid                _chid;
    osiTime             _connectTime;  // when did _connected change?
    Mechanism           _mechanism;    // scanned via get or monitor?

    CtrlInfoI           _ctrl_info;    // has to be copy, not * !

    bool                _new_value_set;
    bool                _pending_value_set;
    bool                _previous_value_set;
    ValueI              *_new_value;     // New value buffer for ca_get/monitor
    ValueI              *_pending_value; // monitor, arrived but wasn't due
    ValueI              *_previous_value;// for change detection
    ValueI              *_tmp_value;     // for making values in addEvent

    ThreadSemaphore     _write_lock;
    ValueI              *_write_value;  // tmp for write (different thread!)

    unsigned short      _vals_per_buffer; // see enum: INIT_VALS_PER_BUF ...
    CircularBuffer      _buffer;    // buffer in memory, later written to disk
    
    osiTime             _expected_next_time;
    osiTime             _last_archive_stamp; // last time stamp in Archie
    osiTime             _had_null_time;

    ChannelInfo(const ChannelInfo &rhs); // not defined
    ChannelInfo & operator = (const ChannelInfo &rhs); // not defined

    // CA Callback for connections
    static void caLinkConnectionHandler(struct connection_handler_args arg);

    // These are to be called from CA with ar.user == this
    // Callback for control information
    static void caControlHandler(struct event_handler_args arg);

    // Callback for values (monitored)
    static void caEventHandler(struct event_handler_args arg);

    void addToRingBuffer(const ValueI *value);
    void handleNewScannedValue(osiTime &stamp);
    void handleDisabling();
};

inline void ChannelInfo::setCtrlInfo(const CtrlInfoI *info)
{
    LOG_ASSERT(info);
    _ctrl_info = *info;
}

inline const char *ChannelInfo::getHost() const
{
    // When the channel _was_ connected before,
    // ca_host_name is actually valid and shows
    // the previous host.
    // But if the channel was never connected,
    // ca_host_name will crash
    // -> be safe here and only call it when connected
    if(_connected)
        return ca_host_name(_chid);
    return "<??>";
}


#endif //__CHANNELINFO_H__


