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
public:
    friend class ArchiveChannel;
    /// Destructor.

    SampleMechanism(class ArchiveChannel *channel);

    /// The ArchiveChannel might redefine its SampleMechanism,
    /// so a SampleMechanism must assert that it can be deleted
    /// (and replaced with a new one) at any time.
    virtual ~SampleMechanism();

    /// Printable description.
    virtual stdString getDescription(Guard &guard) const = 0;

    /// Is the mechanism scanning or safe-every-monitor?
    virtual bool isScanning() const = 0;
    
    /// Invoked for connection changes.
    virtual void handleConnectionChange(Guard &guard) = 0;

    /// Invoked for new value.

    /// This is called by the ArchiveChannel's value_callback,
    /// which in turn is either invoked
    /// - from a CA monitor that the SampleMechanism initiated
    /// - from a CA callback initiated by the Engine's ScanList
    ///
    /// When called, the channel is enabled and the value's stamp is good.
    /// While disabled, values are copied to pending_value.
    /// handleValue needs to update last_stamp_in_archive.
    virtual void handleValue(Guard &guard,
                             const epicsTime &now,
                             const epicsTime &stamp,
                             const RawValue::Data *value) = 0;

protected:
    ArchiveChannel *channel;
};

/// A SampleMechanism that stores each CA event (monitor).

/// This implementation of a SampleMechanism subscribes
/// to a channel (CA monitor) and stores every incoming value.
/// A configurable max_period determines the ring buffer size.
class SampleMechanismMonitored : public SampleMechanism
{
public:
    SampleMechanismMonitored(class ArchiveChannel *channel);
    ~SampleMechanismMonitored();
    stdString getDescription(Guard &guard) const;
    bool isScanning() const;
    void handleConnectionChange(Guard &guard);
    void handleValue(Guard &guard, const epicsTime &now,
                     const epicsTime &stamp, const RawValue::Data *value);
private:
    bool   have_subscribed;
    evid   ev_id;
};

/// A SampleMechanism that uses a periodic CA 'get'.
class SampleMechanismGet : public SampleMechanism
{
public:
    SampleMechanismGet(class ArchiveChannel *channel);
    ~SampleMechanismGet();
    stdString getDescription(Guard &guard) const;
    bool isScanning() const;
    void handleConnectionChange(Guard &guard);
    void handleValue(Guard &guard, const epicsTime &now,
                     const epicsTime &stamp, const RawValue::Data *value);
private:
    bool is_on_scanlist;
    // flushRepeats();
};

/// \@}

#endif
