// --------------------- -*- c++ -*- ----------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __SCAN_MECHANISM_H__
#define __SCAN_MECHANISM_H__

// System
#include <cadef.h>
#include <stdString.h>
//Tools
#include <Guard.h>
// Storage
#include <RawValue.h>

class ArchiveChannel;

/// \addtogroup Engine
/// \@{

/// Base class for all sampling mechanisms.

/// SampleMechanism is used by the ArchiveChannel
/// to handle the actual data sampling.
/// Subscribe or add to scan list,
/// handle incoming values, repeat counts, ...
class SampleMechanism
{
    friend class nobody_really_but_we_want_to_avoid_compiler_warnings;
public:
    /// Constructor.
    SampleMechanism(class ArchiveChannel *channel);

    /// The ArchiveChannel might redefine its SampleMechanism,
    /// so a SampleMechanism must assert that it can be deleted
    /// (and replaced with a new one) at any time.
    /// We use destroy instead of a destructor so that we can
    /// pass the Guard classes.
    /// Needs to call the super class's destroy!
    virtual void destroy(Guard &engine_guard, Guard &guard);
    
    /// Printable description.
    virtual stdString getDescription(Guard &guard) const = 0;

    /// Is the mechanism scanning or safe-every-monitor?
    virtual bool isScanning() const = 0;
    
    /// Invoked for connection changes.
    virtual void handleConnectionChange(Guard &engine_guard, Guard &guard) = 0;

    /// Invoked for new value.

    /// This is called by the ArchiveChannel's value_callback,
    /// which in turn is either invoked
    /// - from a CA monitor that the SampleMechanism initiated
    /// - from a CA callback initiated by the Engine's ScanList
    ///
    /// While disabled, values are copied to pending_value.
    /// Otherwise, handleValue is invoked as long as the
    /// value's time stamp is good:
    /// - non-zero,
    /// - not too far ahead in the future.
    ///
    /// The SampleMechanism needs to deal with back-in-time
    /// issues and update last_stamp_in_archive.
    virtual void handleValue(Guard &guard,
                             const epicsTime &now,
                             const epicsTime &stamp,
                             const RawValue::Data *value) = 0;

protected:
    ArchiveChannel *channel;
    bool wasWrittenAfterConnect;
};

/// A SampleMechanism that stores each CA event (monitor).

/// This implementation of a SampleMechanism subscribes
/// to a channel (CA monitor) and stores every incoming value.
/// A configurable max_period determines the ring buffer size.
class SampleMechanismMonitored : public SampleMechanism
{
public:
    SampleMechanismMonitored(class ArchiveChannel *channel);
    virtual void destroy(Guard &engine_guard, Guard &guard);
    stdString getDescription(Guard &guard) const;
    bool isScanning() const;
    void handleConnectionChange(Guard &engine_guard, Guard &guard);
    void handleValue(Guard &guard, const epicsTime &now,
                     const epicsTime &stamp, const RawValue::Data *value);
private:
    bool   have_subscribed;
    evid   ev_id;
};

/// A SampleMechanism that uses a periodic CA 'get'.

/// This implementation of a SampleMechanism performs
/// periodic CA 'get' operations and stores the most
/// recent value that it receives with its original time stamp.
/// In addition, repeat counts are used:
/// If the value matches the previous sample, it is not written
/// again. Only after the value changes, a value that indicates
/// the repeat count gets written.
class SampleMechanismGet : public SampleMechanism
{
public:
    /// Max. counter for repeats.

    /// Even if the value does not change, it is written
    /// after max_repeat_count iterations. This way we avoid
    /// the appearance of the ArchiveEngine being dead.
    static size_t max_repeat_count;
    SampleMechanismGet(class ArchiveChannel *channel);
    virtual void destroy(Guard &engine_guard, Guard &guard);
    stdString getDescription(Guard &guard) const;
    bool isScanning() const;
    void handleConnectionChange(Guard &engine_guard, Guard &guard);
    void handleValue(Guard &guard, const epicsTime &now,
                     const epicsTime &stamp, const RawValue::Data *value);
private:
    bool is_on_scanlist; // Registered w/ Engine's Scanlist?
    // Handling of repeats:
    bool previous_value_set; // previous_value valid?
    RawValue::Data *previous_value; // the previous value
    size_t repeat_count; // repeat count for the previous value
    // Write the previous value because we're disconnected or got a new value
    void flushPreviousValue(const epicsTime &stamp);
};

/// \@}

#endif
