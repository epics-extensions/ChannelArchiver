// ------------------------------------------- -*- c++ -*-
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __SCANLIST_H__
#define __SCANLIST_H__
#include <epicsTime.h>
#include "ArchiveChannel.h"

/// \addtogroup Engine
/// \@{

/// List of channels to scan for one period.
class SinglePeriodScanList
{
public:
    SinglePeriodScanList(double period);
    ~SinglePeriodScanList();

    void add(ArchiveChannel *channel);
    void remove(ArchiveChannel *channel);
    
    void scan();

    bool empty()
    {   return channels.empty(); }

    void dump();
    
    double                 period;    // Scan period in seconds
    epicsTime              next_scan; // Next time this list is due
private:
    stdList<class ArchiveChannel *> channels;
};

/// List of SinglePeriodScanList classes, one per period.
class ScanList
{
public:
    ScanList();
    ~ScanList();

    /// Add a channel to a ScanList.

    /// channel->getPeriod() must be valid.
    ///
    ///
    void addChannel(Guard &channel_guard, class ArchiveChannel *channel);

    /// Remove channel from ScanList.
    void removeChannel(class ArchiveChannel *channel);

    /// Scan all channels that are due at/after deadline
    void scan(const epicsTime &deadline);

    /// Does the scan list contain anyting?
    bool isDueAtAll()
    {   return is_due_at_all; }
    
    /// When should scan() be called ?
    const epicsTime &getDueTime() const
    {   return  next_list_scan; }

    void dump();

private:
    bool is_due_at_all;
    stdList<SinglePeriodScanList *> period_lists;
    epicsTime                       next_list_scan;
};

/// \@}

#endif //__SCANLIST_H__
