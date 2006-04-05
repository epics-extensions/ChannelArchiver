// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
#include <ThrottledMsgLogger.h>
// Engine
#include "TimeFilter.h"

// Throttle messages to once per hour
static ThrottledMsgLogger futuristic_time_throttle("Future Timestamps", 60.0*60.0);
static ThrottledMsgLogger back_in_time_throttle("Back-in-time", 60.0*60.0);

#define DEBUG_REP_FILT

TimeFilter::TimeFilter(const EngineConfig &config,
                       ProcessVariableListener *listener)
    : ProcessVariableFilter(listener), config(config)
{
}

TimeFilter::~TimeFilter()
{
}

void TimeFilter::pvConnected(Guard &guard, ProcessVariable &pv,
                             const epicsTime &when)
{
    listener->pvConnected(guard, pv, when);
}

void TimeFilter::pvDisconnected(Guard &guard, ProcessVariable &pv,
                                const epicsTime &when)
{
    listener->pvDisconnected(guard, pv, when);
}

void TimeFilter::pvValue(Guard &guard, ProcessVariable &pv,
                         const RawValue::Data *data)
{
    epicsTime stamp = RawValue::getTime(data);
    double future = stamp - epicsTime::getCurrent();
    if (future > config.getIgnoredFutureSecs())
    {
        stdString t;
        epicsTime2string(stamp, t);
        futuristic_time_throttle.LOG_MSG(
            "'%s': Ignoring futuristic time stamp %s\n",
            pv.getName().c_str(), t.c_str());
        return;
    }
    if (stamp < last_stamp)
    {
        stdString t;
        epicsTime2string(stamp, t);
        back_in_time_throttle.LOG_MSG(
            "'%s': Ignoring back-in-time time stamp %s\n",
            pv.getName().c_str(), t.c_str());
        return;
    }    
    // OK, remember this time stamp and pass value to listener.
    last_stamp = stamp;
    listener->pvValue(guard, pv, data);
}
