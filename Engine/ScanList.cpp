// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// kasemir@lanl.gov
// --------------------------------------------------------

// System
#include <float.h>
// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "ScanList.h"

#undef DEBUG_SCANLIST

SinglePeriodScanList::SinglePeriodScanList(double period)
        : period(period)
{
#   ifdef DEBUG_SCANLIST
    LOG_MSG("new SinglePeriodScanList(%g seconds)\n", period);
#   endif
}

SinglePeriodScanList::~SinglePeriodScanList()
{
#   ifdef DEBUG_SCANLIST
    LOG_MSG("delete SinglePeriodScanList(%g seconds)\n", period);
#   endif
}

void SinglePeriodScanList::add(ArchiveChannel *channel)
{
    channels.push_back(channel);
}

void SinglePeriodScanList::remove(ArchiveChannel *channel)
{
    stdList<ArchiveChannel *>::iterator ci;
    for (ci = channels.begin(); ci != channels.end(); ++ci)
    {
        if (*ci == channel)
        {
#           ifdef DEBUG_SCANLIST
            LOG_MSG("SinglePeriodScanList(%g s): Removed '%s'\n",
                    period, channel->getName().c_str());
#           endif
            ci = channels.erase(ci);
        }
    }
}

void SinglePeriodScanList::scan()
{
    stdList<ArchiveChannel *>::iterator channel;
    for (channel = channels.begin(); channel != channels.end(); ++channel)
    {
        ArchiveChannel *c = *channel;
        Guard guard(c->mutex);
        if (c->isConnected(guard))
            c->issueCaGet(guard);
    }
}

void SinglePeriodScanList::dump()
{
    stdList<ArchiveChannel *>::iterator ci;
    printf("Scan List %g sec\n", period);
    for (ci = channels.begin(); ci != channels.end(); ++ci)
        printf("'%s'\n", (*ci)->getName().c_str());
}

ScanList::ScanList()
{
    is_due_at_all = false;
    next_list_scan = nullTime;
#   ifdef DEBUG_SCANLIST
    LOG_MSG("Created ScanList\n");
#   endif
}

ScanList::~ScanList()
{
#   ifdef DEBUG_SCANLIST
    LOG_MSG("Destroying ScanList\n");
#   endif
    if (!period_lists.empty())
    {
        LOG_MSG("Hmmm. At this point, channels should have removed themselves from the scan lists,\n");
        LOG_MSG("and the scan lists should then have quit.\n");
    }
    while (!period_lists.empty())
    {
        SinglePeriodScanList *sl = period_lists.front();
        period_lists.pop_front();
        LOG_MSG("Removing the %g second scan list.\n", sl->period);
        delete sl;
    }
}

void ScanList::addChannel(Guard &guard, ArchiveChannel *channel)
{
    stdList<SinglePeriodScanList *>::iterator li;
    SinglePeriodScanList *period_list;
    double period = channel->getPeriod(guard);
    // Check it the channel is already on some
    // list where it needs to be removed
    removeChannel(channel);
    // Find a scan list with suitable period
    for (li = period_lists.begin(); li != period_lists.end(); ++li)
    {
        if ((*li)->period == period)
        {
            period_list = *li; // found one!
            break;
        }
    }
    if (li == period_lists.end()) // create new list
    {
        period_list = new SinglePeriodScanList(channel->getPeriod(guard));
        period_lists.push_back(period_list);
        // next scan time, rounded to period
        period_list->next_scan = roundTimeUp(epicsTime::getCurrent(),
                                             period_list->period);
    }
    period_list->add(channel);
    if (next_list_scan == nullTime || next_list_scan > period_list->next_scan)
        next_list_scan = period_list->next_scan;
    is_due_at_all = true;
#   ifdef DEBUG_SCANLIST
    char buf[30];    
    period_list->next_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("Channel '%s' makes list %g due %s\n",
            channel->getName().c_str(),
            period_list->period,
            buf);
    next_list_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("->Whole ScanList due %s\n", buf);
#   endif
}

void ScanList::removeChannel(ArchiveChannel *channel)
{
    stdList<SinglePeriodScanList *>::iterator li = period_lists.begin();
    SinglePeriodScanList *l;
    while (li != period_lists.end())
    {
        l = *li;
        l->remove(channel);
        if (l->empty())
        {
            li = period_lists.erase(li);
            delete l;
        }
        else
            ++li;
    }
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
    next_list_scan = nullTime;
    for (li = period_lists.begin(); li != period_lists.end(); ++li)
    {
        if (deadline >= (*li)->next_scan)
        {   // Determine next scan time,
            // make sure it's in the future.
            rounded_period = (unsigned long) (*li)->period;
            while (deadline > (*li)->next_scan)
                (*li)->next_scan += rounded_period;
            // Scan that list
            (*li)->scan();
#ifdef DEBUG_SCANLIST
            (*li)->next_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
            LOG_MSG("Scanned List %g, next due %s\n",
                    (*li)->period, buf);
#endif
        }
        // Update earliest scan time
        if (next_list_scan == nullTime ||
            next_list_scan > (*li)->next_scan)
            next_list_scan = (*li)->next_scan;
    }
#ifdef DEBUG_SCANLIST
    next_list_scan.strftime(buf, 30, "%Y/%m/%d %H:%M:%S");
    LOG_MSG("->Whole ScanList due %s\n", buf);
#endif
}

void ScanList::dump()
{
    stdList<SinglePeriodScanList *>::iterator li;
    for (li = period_lists.begin(); li != period_lists.end(); ++li)
        (*li)->dump();
}

// EOF ScanList.cpp
