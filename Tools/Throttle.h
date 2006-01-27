// -*- c++ -*-
#ifndef __THROTTLE_H__
#define __THROTTLE_H__

// Base
#include <epicsTime.h>
// Tools
#include <ToolsConfig.h>

/// \ingroup Tools
///
/// Timer for throttling messages.
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
    double seconds;
    epicsTime last;
};

#endif

