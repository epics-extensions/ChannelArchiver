// Local
#include "Interval.h"

stdString Interval::toString() const
{
    char result[100];
    stdString s, e;
    epicsTime2string(start, s);
    epicsTime2string(end, e);
    snprintf(result, sizeof(result), "%s - %s", s.c_str(), e.c_str());
    return stdString(result);
}