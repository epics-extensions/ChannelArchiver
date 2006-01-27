// -*- c++ -*-
#ifndef __THROTTLE_H__
#define __THROTTLE_H__

// Base
#include <epicsTime.h>
// Tools
#include <ToolsConfig.h>
#include <Guard.h>

/// \ingroup Tools
///
/// Timer for throttling messages, thread safe.
/// 
/// In order to avoid hundreds of similar messages,
/// use the Throttle to check if it's OK to print
/// the message.
class Throttle
{
public:
    Throttle(double seconds_between_messages = 10.0)
      : seconds(seconds_between_messages)
    {}

    /// @return Returns true when it's OK to print another message.
    bool isPermitted(const epicsTime &when)
    {
        Guard guard(mutex);
        double time_passed = when - last;
        if (time_passed < seconds)
            return false;
        last = when;
        return true;
    }

    /// @return Returns true when it's OK to print another message.
    bool isPermitted()
    {
        epicsTime now(epicsTime::getCurrent());
        return isPermitted(now);
    }
    
private:
    epicsMutex mutex;
    double seconds;
    epicsTime last;
};

#endif

