#include "DataServer.h"

// epicsTime -> time_t-type of seconds & nanoseconds
void epicsTime2pieces(const epicsTime &t,
                      xmlrpc_int32 &secs, xmlrpc_int32 &nano)
{   // TODO: This is lame, calling epicsTime's conversions twice
    epicsTimeStamp stamp = t;
    time_t time;
    epicsTimeToTime_t(&time, &stamp);
    secs = time;
    nano = stamp.nsec;
}

// Inverse to epicsTime2pieces
void pieces2epicsTime(xmlrpc_int32 secs, xmlrpc_int32 nano, epicsTime &t)
{   // As lame as other nearby code
    epicsTimeStamp stamp;
    time_t time = secs;
    epicsTimeFromTime_t(&stamp, time);
    stamp.nsec = nano;
    t = stamp;
}

