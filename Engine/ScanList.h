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
#include <list>
#include <osiTime.h>
#include "ChannelInfo.h"

BEGIN_NAMESPACE_CHANARCH
#ifdef USE_NAMESPACE_STD
using std::list;
#endif

class SinglePeriodScanList
{
public:
    SinglePeriodScanList ();

    // returns false on timeout
    bool scan ();

    double      _period;    // Scan period in seconds
    osiTime     _next_scan; // Next time this list is due
    list<ChannelInfo *> _channels;

private:
    double  _min_wait, _max_wait;
};

class ScanList
{
public:
    ScanList ();
    ~ScanList ();

    // Add a channel to a ScanList.
    // channel->getPeriod() must be valid
    void addChannel (ChannelInfo *channel);

    // Scan all channels that are due at/after deadline
    void scan (const osiTime &deadline);

    // When should scan() be called ?
    bool isDue (const osiTime &now) const
    {   return !!(now > _next_list_scan); } // !! to avoid int->bool warning

private:
    list<SinglePeriodScanList>  _period_lists;
    osiTime                     _next_list_scan;
};

END_NAMESPACE_CHANARCH

#endif //__SCANLIST_H__
