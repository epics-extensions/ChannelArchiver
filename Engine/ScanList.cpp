#include "ScanList.h"
#include "float.h"

using namespace std;
USING_NAMESPACE_CHANARCH

//#define LOG_SCANLIST

SinglePeriodScanList::SinglePeriodScanList ()
{
	_min_wait = DBL_MAX;
	_max_wait = 0.0;
}

// returns false on timeout
bool SinglePeriodScanList::scan ()
{
	// fetch channels
	list<ChannelInfo *>::iterator	channel;
	for (channel = _channels.begin(); channel != _channels.end(); ++channel)
	{
		if ((*channel)->isConnected())
			(*channel)->issueCaGet ();
	}

	osiTime start = osiTime::getCurrent ();

	if (ca_pend_io (10.0) != ECA_NORMAL)
		return false;

	double elapsed = double(osiTime::getCurrent ()) - double(start);
	if (elapsed > _max_wait)
		_max_wait = elapsed;
	if (elapsed < _min_wait)
		_min_wait = elapsed;

	// add values to circular buffer
	for (channel = _channels.begin(); channel != _channels.end(); ++channel)
	{
		if ((*channel)->isConnected())
			(*channel)->handleNewValue();
	}

	return true;
}

ScanList::ScanList ()
{
	_next_list_scan = nullTime;
}

ScanList::~ScanList ()
{
	list<SinglePeriodScanList>::iterator li;
	LOG_MSG ("Statistics for Time spend in ca_pend_io\n");
	LOG_MSG ("=======================================\n");
	for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
	{
		LOG_MSG ("ScanList " << li->_period << " s wait: min "
			<< li->_min_wait << ", max " << li->_max_wait << "\n");
	}
}

void ScanList::addChannel (ChannelInfo *channel)
{
	list<SinglePeriodScanList>::iterator li;
	SinglePeriodScanList *list;

	// find a scan list with suitable period
	for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
		if (li->_period == channel->getPeriod ())
		{
			list = &*li; // found one!
			break;
		}

	if (li == _period_lists.end()) // create new list
	{
		list = &*_period_lists.insert (_period_lists.end());
		list->_period = channel->getPeriod ();

		// next scan time, rounded to period
		unsigned long secs = (unsigned long) double(osiTime::getCurrent ());
		unsigned long rounded_period = (unsigned long) list->_period;
		secs += rounded_period;
		secs -= secs % rounded_period;
		list->_next_scan = secs;
	}
	list->_channels.push_back (channel);

	if (_next_list_scan == nullTime || _next_list_scan > list->_next_scan)
		_next_list_scan = list->_next_scan;

#	ifdef LOG_SCANLIST
	LOG_MSG ("Channel '" << channel->getName()
		<< "' makes list " << list->_period
		<< " due " << list->_next_scan << "\n");
	LOG_MSG ("->ScanList due " << _next_list_scan << "\n");
#	endif
}

// Scan all channels that are due at/after deadline
void ScanList::scan (const osiTime &deadline)
{
	list<SinglePeriodScanList>::iterator li;
	unsigned long rounded_period;

	// find expired list
	for (li = _period_lists.begin(); li != _period_lists.end(); ++li)
		if (deadline >= li->_next_scan)
		{
			rounded_period = (unsigned long) li->_period;
			// determine next scan time,
			// make sure it's in the future:
			while (deadline > li->_next_scan)
				li->_next_scan += rounded_period;
			if (! li->scan ())
			{
				LOG_MSG (osiTime::getCurrent() <<
					": ScanList timeout for period " << li->_period << endl);
			}
			if (_next_list_scan == nullTime || _next_list_scan > li->_next_scan)
				_next_list_scan = li->_next_scan;

#			ifdef LOG_SCANLIST
			LOG_MSG ("Scanned List " << li->_period << ", next due " << li->_next_scan << "\n");
#			endif
		}
}

