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

class ArchiveChannel;

/// \ingroup Engine Base class for all sampling mechanisms.

/// SampleMechanism is used by the ArchiveChannel
/// to handle the actual data sampling.
/// Subscribe or add to scan list,
/// handle incoming values, repeat counts, ...
class SampleMechanism
{
public:
    friend class ArchiveChannel;
    virtual ~SampleMechanism();
    virtual stdString getDescription() const = 0;

    /// Invoked for connection changes
    virtual void handleConnectionChange(Guard &guard) = 0;
protected:
    ArchiveChannel *channel;

    // Check for 0 or too futuristic time stamps
    // Need to provide 'now' for the future test 
    bool isGoodTimestamp(const epicsTime &stamp, const epicsTime &now);
};

/// A SampleMechanism that stores each CA event (monitor).

/// This implementation of a SampleMechanism subscribes
/// to a channel (CA monitor) and stores every incoming value.
/// A configurable max_period determines the ring buffer size.
class SampleMechanismMonitored : public SampleMechanism
{
public:
    SampleMechanismMonitored();
    ~SampleMechanismMonitored();
    stdString getDescription() const;
    void handleConnectionChange(Guard &guard);
private:
    bool   have_subscribed;
    evid   ev_id;

    static void value_callback(struct event_handler_args);
};

#ifdef NOTYET
class SampleMechanismGet
{
public:
    SampleMechanismGet();
    virtual const char *description();

protected:

    // flushRepeats();
};
#endif


#endif
