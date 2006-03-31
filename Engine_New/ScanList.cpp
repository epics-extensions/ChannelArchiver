// System
#include <math.h>
// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "ScanList.h"

// #define DEBUG_SCANLIST

/*  List of channels to scan for one period.
 *  <p>
 *  Used internally by the ScanList.
 */
class SinglePeriodScanList
{
public:
    SinglePeriodScanList(double period);
    ~SinglePeriodScanList();
    
    double getPeriod() const
    {
        return period;
    }
    
    epicsTime getNextScantime() const
    {
        return next_scan;
    }

    void add(Scannable *item);

    void remove(Scannable *item);
    
    void scan();

    bool empty() const
    {
        return items.empty();
    }

    void dump() const;
    
private:
    double               period;    // Scan period in seconds
    stdList<Scannable *> items;     // Items to scan every 'period'
    epicsTime            next_scan; // Next time this list is due
    
    void computeNextScantime();
};


SinglePeriodScanList::SinglePeriodScanList(double period)
        : period(period)
{
#ifdef DEBUG_SCANLIST
    LOG_MSG("new SinglePeriodScanList(%g seconds)\n", period);
#endif
    computeNextScantime();
}

SinglePeriodScanList::~SinglePeriodScanList()
{
#ifdef DEBUG_SCANLIST
    LOG_MSG("delete SinglePeriodScanList(%g seconds)\n", period);
#endif
}

void SinglePeriodScanList::add(Scannable *item)
{
    stdList<Scannable *>::iterator l;
    // Avoid duplicate entries.
    for (l = items.begin(); l != items.end(); ++l)
        if (*l == item)
            throw GenericException(__FILE__, __LINE__,
                "Duplicate item '%s' for %.2f s scan list",
                item->getName().c_str(), period);
    items.push_back(item);
}

void SinglePeriodScanList::remove(Scannable *item)
{
    items.remove(item);
}

void SinglePeriodScanList::scan()
{
    stdList<Scannable *>::iterator item;
    for (item = items.begin(); item != items.end(); ++item)
        (*item)->scan();
    computeNextScantime();
}

void SinglePeriodScanList::dump() const
{
    stdList<Scannable *>::const_iterator item;
    printf("Scan List %g sec\n", period);
    for (item = items.begin(); item != items.end(); ++item)
        printf("'%s'\n", (*item)->getName().c_str());
}

void SinglePeriodScanList::computeNextScantime()
{
    // Start with simple calculation of next time.
    next_scan += period;
    // Use the more elaborate rounding if that time already passed.
    epicsTime now = epicsTime::getCurrent();
    if (next_scan < now)
        next_scan = roundTimeUp(now, period);
#ifdef DEBUG_SCANLIST
    stdString time;
    epicsTime2string(next_scan, time);
    LOG_MSG("Next due time for %g sec list: %s\n", period, time.c_str());
#endif
}

// --------------------------------------

ScanList::ScanList()
{
    is_due_at_all = false;
    next_list_scan = nullTime;
}

ScanList::~ScanList()
{
    if (!lists.empty())
    {
        LOG_MSG("ScanList not empty while destructed\n");
    }
    while (!lists.empty())
    {
        SinglePeriodScanList *sl = lists.front();
        lists.pop_front();
        LOG_MSG("Removing the %g second scan list.\n", sl->getPeriod());
        delete sl;
    }
}

void ScanList::add(Scannable *item, double period)
{
    // Check it the channel is already on some
    // list where it needs to be removed
    remove(item);
    // Find a scan list with suitable period,
    // meaning: a scan period that matches the requested
    // period to some epsilon.
    stdList<SinglePeriodScanList *>::iterator li;
    SinglePeriodScanList *list = 0;
    for (li = lists.begin(); li != lists.end(); ++li)
    {
        if (fabs((*li)->getPeriod() - period) < 0.05)
        {
            list = *li; // found one!
            break;
        }
    }
    if (list == 0) // create new list for this period
    {
        list = new SinglePeriodScanList(period);
        lists.push_back(list);
    }
    list->add(item);
}

void ScanList::remove(Scannable *item)
{
    stdList<SinglePeriodScanList *>::iterator li = lists.begin();
    SinglePeriodScanList *l;
    while (li != lists.end())
    {
        l = *li;
        // Remove item from every SinglePeriodScanList
        // (should really be at most on one)
        l->remove(item);
        // In case that shrunk a S.P.S.L to zero, drop that list.
        if (l->empty())
        {   // .. which advances the li.
            li = lists.erase(li);
            delete l;
        }
        else // otherwise, advance li as usual:
            ++li;
    }
}

void ScanList::scan(const epicsTime &deadline)
{
    stdList<SinglePeriodScanList *>::iterator li;
#ifdef DEBUG_SCANLIST
    LOG_MSG("ScanList::scan\n");
#endif
    // Reset: Next time any list is due
    next_list_scan = nullTime;
    for (li = lists.begin(); li != lists.end(); ++li)
    {
        if (deadline >= (*li)->getNextScantime())
            (*li)->scan();
        // Update earliest scan time
        if (next_list_scan == nullTime ||
            next_list_scan > (*li)->getNextScantime())
            next_list_scan = (*li)->getNextScantime();
    }
#ifdef DEBUG_SCANLIST
    stdString time;
    epicsTime2string(next_list_scan, time);            
    LOG_MSG("Next due '%s'\n", time.c_str());    
#endif
}

void ScanList::dump() const
{
    stdList<SinglePeriodScanList *>::const_iterator li;
    for (li = lists.begin(); li != lists.end(); ++li)
        (*li)->dump();
}

// EOF ScanList.cpp
