// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// kasemir@lanl.gov
// --------------------------------------------------------

#include <float.h>
#include <epicsTimeHelper.h>
#include "ScanList.h"

#undef DEBUG_SCANLIST

SinglePeriodScanList::SinglePeriodScanList(double period)
{
    _period   = period;
}

// returns false on timeout
void SinglePeriodScanList::scan()
{
    // fetch channels
    stdList<ChannelInfo *>::iterator channel;
    for (channel = _channels.begin(); channel != _channels.end(); ++channel)
    {
        if ((*channel)->isConnected())
            (*channel)->issueCaGetCallback();
    }
}

ScanList::ScanList()
{
    _is_due_at_all = false;
    _next_list_scan = nullTime;
}

ScanList::~ScanList()
{
    while (!_period_lists.empty())
    {
        SinglePeriodScanList *sl = _period_lists.front();
        _period_lists.pop_front();
        delete sl;
    }
}

void ScanList::addChannel(ChannelInfo *channel)
{
    stdList<SinglePeriodScanList *>::iterator li;
    SinglePeriodScanList *list;

    // find a scan list with suitable period
    for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
    {
        if ((*li)->_period == channel->getPeriod())
        {
            list = *li; // found one!
            break;
        }
    }
    if (li == _period_lists.end()) // create new list
    {
        list = new SinglePeriodScanList(channel->getPeriod());
        _period_lists.push_back(list);
        // next scan time, rounded to period
        list->_next_scan = roundTimeUp(epicsTime::getCurrent(),
                                       list->_period);
    }
    list->_channels.push_back(channel);
    if (_next_list_scan == nullTime || _next_list_scan > list->_next_scan)
        _next_list_scan = list->_next_scan;
    _is_due_at_all = true;

#   ifdef DEBUG_SCANLIST
    char buf[30];
    
    list->_next_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("Channel '%s' makes list %g due %s\n",
            channel->getName().c_str(),
            list->_period,
            buf);
    _next_list_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("->Whole ScanList due %s\n", buf);
#   endif
}

// Scan all channels that are due at/after deadline
void ScanList::scan(const epicsTime &deadline)
{
    stdList<SinglePeriodScanList *>::iterator li;
    unsigned long rounded_period;
#   ifdef DEBUG_SCANLIST
    char buf[30];
#   endif

#ifdef DEBUG_SCANLIST
    LOG_MSG("ScanList::scan\n");
#endif
    // Reset: Next time any list is due
    _next_list_scan = nullTime;
    for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
    {
        if (deadline >= (*li)->_next_scan)
        {
            // Determine next scan time,
            // make sure it's in the future.
            rounded_period = (unsigned long) (*li)->_period;
            while (deadline > (*li)->_next_scan)
                (*li)->_next_scan += rounded_period;
            // Scan list list
            (*li)->scan();
#ifdef DEBUG_SCANLIST
            (*li)->_next_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
            LOG_MSG("Scanned List %g, next due %s\n",
                    (*li)->_period, buf);
#endif
        }
        // Update earliest scan time
        if (_next_list_scan == nullTime ||
            _next_list_scan > (*li)->_next_scan)
            _next_list_scan = (*li)->_next_scan;
    }
#ifdef DEBUG_SCANLIST
    _next_list_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("->Whole ScanList due %s\n", buf);
#endif
}

// EOF ScanList.cpp
