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

// This does no longer work:
//#define LOG_SCANLIST

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

#   ifdef LOG_SCANLIST
    LOG_MSG("Channel '" << channel->getName()
            << "' makes list " << list->_period
            << " due " << list->_next_scan << "\n");
    LOG_MSG("->ScanList due " << _next_list_scan << "\n");
#   endif
}

// Scan all channels that are due at/after deadline
void ScanList::scan(const epicsTime &deadline)
{
    stdList<SinglePeriodScanList *>::iterator li;
    unsigned long rounded_period;

    // find expired list
    for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
    {
        if (deadline >= (*li)->_next_scan)
        {
            rounded_period = (unsigned long) (*li)->_period;
            // determine next scan time,
            // make sure it's in the future:
            while (deadline > (*li)->_next_scan)
                (*li)->_next_scan += rounded_period;
            (*li)->scan();
            
            if (_next_list_scan == nullTime ||
                _next_list_scan > (*li)->_next_scan)
                _next_list_scan = (*li)->_next_scan;
#           ifdef LOG_SCANLIST
            LOG_MSG("Scanned List " << li->_period << ", next due "
                    << li->_next_scan << "\n");
#           endif
        }
    }
}

// EOF ScanList.cpp
