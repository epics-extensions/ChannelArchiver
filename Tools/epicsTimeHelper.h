// epicsTimeHelper

#ifndef __EPICSTIMEHELPER_H__
#define __EPICSTIMEHELPER_H__

#include <ToolsConfig.h>
#include <epicsTime.h>

extern const epicsTime nullTime; // uninitialized (=0) class epicsTime

// Needs to be called for initialization
void initEpicsTimeHelper();

// Check if time is non-zero, whatever that could be
bool isValidTime(const epicsTime &t);

// Convert string "mm/dd/yyyy" or "mm/dd/yyyy 00:00:00" or
// "mm/dd/yyyy 00:00:00.000000000" into epicsTime
// Result: true for OK
bool string2epicsTime(const stdString &txt, epicsTime &time);
// Convert epicsTime into "mm/dd/yyyy 00:00:00.000000000"
void epicsTime2string(const epicsTime &time, stdString &txt);

// Assemble/disassemple pieces
// year : 4-digit year like 2003
// month: 1-12
// day  : 1-31
// hour : 0-23
// min  : 0-59
// sec  : 0-59
// nano : 0- 999999999
// (Note: 'man localtime' indicates that sec could be up to 61
//        to account for leap-seconds)
void epicsTime2vals(const epicsTime &time,
                    int &year, int &month, int &day,
                    int &hour, int &min, int &sec, unsigned long &nano);

void vals2epicsTime(int year, int month, int day,
                    int hour, int min, int sec, unsigned long nano,
                    epicsTime &time);

// Some of these are magic numbers because e.g.
// the number of seconds per month of course depends on the month
#define secsPerMinute 60
#define secsPerHour   (60*60)
#define secsPerDay    (60*60*24)
#define secsPerMonth  (60*60*24*31)
#define secsPerYear   (60*60*24*31*365)

// Find timestamp near 'time' which is a multiple of 'secs'.
epicsTime roundTime (const epicsTime &time, double secs);
// Round up. (Rounding 5.5 by 0.5 will give 6.0!)
epicsTime roundTimeUp (const epicsTime &time, double secs);
// Round down (Rounding 5.5 by 0.5 will give 5.5!)
epicsTime roundTimeDown (const epicsTime &time, double secs);

#endif //__EPICSTIMEHELPER_H__
