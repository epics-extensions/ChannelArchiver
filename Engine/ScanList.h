// --------------------------------------------------------
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
#include "ChannelInfo.h"

class SinglePeriodScanList
{
public:
    SinglePeriodScanList(double period);
    void scan();

    double                 _period;    // Scan period in seconds
    epicsTime              _next_scan; // Next time this list is due
    stdList<ChannelInfo *> _channels;
};

class ScanList
{
public:
    ScanList();
    ~ScanList();

    // Add a channel to a ScanList.
    // channel->getPeriod() must be valid
    void addChannel(ChannelInfo *channel);

    // Scan all channels that are due at/after deadline
    void scan(const epicsTime &deadline);

    // When should scan() be called ?
    const epicsTime &getDueTime() const
    {   return  _next_list_scan; }

private:
    stdList<SinglePeriodScanList *> _period_lists;
    epicsTime                       _next_list_scan;
};

#endif //__SCANLIST_H__
