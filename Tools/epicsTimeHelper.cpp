// epicsTimeHelper

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "epicsTimeHelper.h"
#include "tsDefs.h"
#include "MsgLogger.h"

// Because of unknown differences in the epoch definition,
// the next two might be different!
const epicsTime nullTime; // uninitialized (=0) class epicsTime
static epicsTime nullStamp;      // epicsTime with epicsTimeStamp 0, 0

// Needs to be called for initialization
void initEpicsTimeHelper()
{
	epicsTimeStamp	stamp;
	stamp.secPastEpoch = 0;
	stamp.nsec = 0;
	nullStamp = stamp;
}

// Check if time is non-zero, whatever that could be
bool isValidTime(const epicsTime &t)
{
	return t != nullTime  &&  t != nullStamp;
}

// Convert string "mm/dd/yyyy" or "mm/dd/yyyy 00:00:00" or
// "mm/dd/yyyy 00:00:00.000000000" into epicsTime
// Result: true for OK
bool string2epicsTime(const stdString &txt, epicsTime &time)
{
    // TODO: number check ("ab/cd/efgh" not caught here)
    size_t tlen = txt.length();
    //  0123456789
    // "mm/dd/yyyy" ?
	if (tlen < 10  ||  txt[2] != '/' || txt[5] != '/')
		return false;

    struct local_tm_nano_sec tm;
    memset(&tm, 0, sizeof(struct local_tm_nano_sec));
    tm.ansi_tm.tm_isdst = -1; /* don't know if daylight saving or not */
    tm.ansi_tm.tm_mon  = (txt[0]-'0')*10 + (txt[1]-'0') - 1;
    tm.ansi_tm.tm_mday = (txt[3]-'0')*10 + (txt[4]-'0');
    tm.ansi_tm.tm_year = (txt[6]-'0')*1000 + (txt[7]-'0')*100 +
                         (txt[8]-'0')*10 + (txt[9]-'0') - 1900;

    //  0123456789012345678
    // "mm/dd/yyyy 00:00:00"
    if (tlen >= 19)
    {
        if (txt[13] != ':' || txt[16] != ':')
            return false;
        tm.ansi_tm.tm_hour = (txt[11]-'0')*10 + (txt[12]-'0');
        tm.ansi_tm.tm_min  = (txt[14]-'0')*10 + (txt[15]-'0');
        tm.ansi_tm.tm_sec  = (txt[17]-'0')*10 + (txt[18]-'0');
    }
    //  01234567890123456789012345678
    // "mm/dd/yyyy 00:00:00.000000000"
    if (tlen == 29 && txt[19] == '.')
        tm.nSec =
            (txt[20]-'0')*100000000 +
            (txt[21]-'0')*10000000 +
            (txt[22]-'0')*1000000 +
            (txt[23]-'0')*100000 +
            (txt[24]-'0')*10000 +
            (txt[25]-'0')*1000 +
            (txt[26]-'0')*100 +
            (txt[27]-'0')*10 +
            (txt[28]-'0');   
    
	time = tm;

	return true;
}

// Convert epicsTime into "mm/dd/yyyy 00:00:00.000000000"
void epicsTime2string (const epicsTime &time, stdString &txt)
{
	if (! isValidTime (time))
	{
		txt = "00:00:00";
		return;
	}
    char buffer[50];
    struct local_tm_nano_sec tm = (local_tm_nano_sec) time;
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            tm.ansi_tm.tm_mon + 1,
            tm.ansi_tm.tm_mday,
            tm.ansi_tm.tm_year + 1900,
            tm.ansi_tm.tm_hour,
            tm.ansi_tm.tm_min,
            tm.ansi_tm.tm_sec,
            tm.nSec);
    txt = buffer;
}

void epicsTime2vals(const epicsTime &time,
                    int &year, int &month, int &day,
                    int &hour, int &min, int &sec, unsigned long &nano)
{
    if (isValidTime(time))
    {
        struct local_tm_nano_sec tm = (local_tm_nano_sec) time;
        year  = tm.ansi_tm.tm_year + 1900;
        month = tm.ansi_tm.tm_mon + 1;
        day   = tm.ansi_tm.tm_mday;
        hour  = tm.ansi_tm.tm_hour;
        min   = tm.ansi_tm.tm_min;
        sec   = tm.ansi_tm.tm_sec;
        nano  = tm.nSec;
    }
    else
	{
	   year = month = day = hour = min = sec = 0;
	   nano = 0;
	}
}

void vals2epicsTime(int year, int month, int day,
                    int hour, int min, int sec, unsigned long nano,
                    epicsTime &time)
{
    struct local_tm_nano_sec tm;
    tm.ansi_tm.tm_year = year - 1900;
    tm.ansi_tm.tm_mon  = month - 1;
    tm.ansi_tm.tm_mday = day;
    tm.ansi_tm.tm_hour = hour;
    tm.ansi_tm.tm_min  = min;
    tm.ansi_tm.tm_sec  = sec;
    tm.ansi_tm.tm_isdst   = -1;
    tm.nSec = nano;
    time = tm;
}

// Round down
epicsTime roundTimeDown(const epicsTime &time, double secs)
{
	if (secs <= 0)
		return time;

    struct local_tm_nano_sec tm = (local_tm_nano_sec) time;
    unsigned long round;

    if (secs < secsPerDay)
    {
        epicsTimeStamp stamp = (epicsTimeStamp)time;
        
        double t = stamp.secPastEpoch + stamp.nsec / 1.0e9;
        t = floor(t / secs) * secs;
        
        stamp.secPastEpoch = (epicsUInt32) t;
        stamp.nsec = (epicsUInt32) ((t - stamp.secPastEpoch) * 1.0e9);
        
        return epicsTime(stamp);
    }
    else if (secs < secsPerMonth)
    {
        round = (unsigned long) (secs/secsPerDay);
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = (tm.ansi_tm.tm_mday / round) * round;
    }
    else if (secs < secsPerYear)
    {
        round = (unsigned long) (secs/secsPerMonth);
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = 1;
        tm.ansi_tm.tm_mon = (tm.ansi_tm.tm_mon / round) * round;
    }
    else
    {
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = 1;
        tm.ansi_tm.tm_mon = 0;
    }
    // TODO: round weeks, fortnights?

    return epicsTime(tm);
}

// Round up
epicsTime roundTimeUp(const epicsTime &time, double secs)
{
	if (secs <= 0)
		return time;

    struct local_tm_nano_sec tm = (local_tm_nano_sec) time;
    unsigned long round;
    if (secs < secsPerDay)
    {
        epicsTimeStamp stamp = (epicsTimeStamp)time;
        
        double t = stamp.secPastEpoch + stamp.nsec / 1.0e9;
        double div = floor(t / secs); 
        t = (div+1)*secs;
        
        stamp.secPastEpoch = (epicsUInt32) t;
        stamp.nsec = (epicsUInt32) ((t - stamp.secPastEpoch) * 1.0e9);
        
        return epicsTime(stamp);
    }
    else if (secs < secsPerMonth)
    {
        round = (unsigned long) (secs/secsPerDay);
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = (tm.ansi_tm.tm_mday / round + 1) * round;
    }
    else if (secs < secsPerYear)
    {
        round = (unsigned long) (secs/secsPerMonth);
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = 1;
        tm.ansi_tm.tm_mon = (tm.ansi_tm.tm_mon / round + 1) * round;
    }
    else
    {
        tm.nSec = 0;
        tm.ansi_tm.tm_sec = 0;
        tm.ansi_tm.tm_min = 0;
        tm.ansi_tm.tm_hour = 0;
        tm.ansi_tm.tm_mday = 1;
        tm.ansi_tm.tm_mon = 0;
        tm.ansi_tm.tm_year += 1;
    }
    // TODO: round weeks, fortnights?

    return epicsTime(tm);
}

// Find timestamp near 'time' which is a multiple
// of 'secs'.
epicsTime roundTime (const epicsTime &time, double secs)
{
    epicsTime up = roundTimeUp(time, secs);
    epicsTime down = roundTimeDown(time, secs);

    if (time - down  <  up - time)
        return down;
    return up;

#if 0
    // TODO:
    // The above implementation is expensive.
    // Can something like this made to work
    // after converting the time to a double and back?
	if (secs == 0)
		return time;
	double val = time;
	double rounded = floor (val / secs + 0.5) * secs;
	return epicsTime (rounded);
#endif
}

