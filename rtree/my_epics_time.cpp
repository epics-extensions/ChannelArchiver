#include "my_epics_time.h" 
#include <stdio.h>

long compareTimeStamps(const epicsTimeStamp & a, const epicsTimeStamp & b) 
{
	if(a.secPastEpoch == b.secPastEpoch && a.nsec == b.nsec) return 0;
	else if(isTimeStampZero(a))	return 1;
	else if(isTimeStampZero(b))	return -1;
	else if(a.secPastEpoch == b.secPastEpoch) return a.nsec - b.nsec;
	else return a.secPastEpoch - b.secPastEpoch;
}

bool isTimeStampZero(const epicsTimeStamp & a)	{return (a.secPastEpoch == a.nsec) && (a.secPastEpoch == 0);}

bool isTimeStampValid(const epicsTimeStamp & a) 
{
	if((a.secPastEpoch >=0) && (a.nsec >= 0))	return true;
	printf("The time %d.%d is not valid \n", a.secPastEpoch, a.nsec );	return false;

}

