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

class SinglePeriodScanList
{
public:
    SinglePeriodScanList(double period);
    void scan();

    double                 period;    // Scan period in seconds
    epicsTime              next_scan; // Next time this list is due
    stdList<class ArchiveChannel *> channels;
};

class ScanList
{
public:
    ScanList();
    ~ScanList();

    // Add a channel to a ScanList.
    // channel->getPeriod() must be valid
    void addChannel(class ArchiveChannel *channel);

    // Scan all channels that are due at/after deadline
    void scan(const epicsTime &deadline);

    // Does the scan list contain anyting?
    bool isDueAtAll()
    {   return is_due_at_all; }
    
    // When should scan() be called ?
    const epicsTime &getDueTime() const
    {   return  next_list_scan; }

private:
    bool is_due_at_all;
    stdList<SinglePeriodScanList *> period_lists;
    epicsTime                       next_list_scan;
};

#endif //__SCANLIST_H__







