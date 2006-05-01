
// Tools
#include "Guard.h"
#include "Throttle.h"

bool Throttle::isPermitted(const epicsTime &when)
{
    Guard guard(__FILE__, __LINE__, mutex);
    double time_passed = when - last;
    if (time_passed < seconds)
        return false;
    last = when;
    return true;
}

bool Throttle::isPermitted()
{
    epicsTime now(epicsTime::getCurrent());
    return isPermitted(now);
}
    
