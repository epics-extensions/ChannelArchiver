//my_epics_time.h
#ifndef _MY_EPICS_TIME_H_
#define _MY_EPICS_TIME_H_

	//for developing with visual studio
	#ifdef _MSC_VER

		typedef struct epicsTimeStamp {
			long    secPastEpoch;   /** seconds since 0000 Jan 1, 1990 */
			long    nsec;           /** nanoseconds within second */
		} epicsTimeStamp;

	#else 

		#include <epicsTime.h>

	#endif // _MSC_VER


/**
*	If not sure whether time stamps are valid, isTimeStampValid() should be called
*	@see isTimeStampValid();
*	@return 
*		 0, if a and b are identical
*		 1, if a is "later" than b
*		-1, if a is "earlier" than b
*/
long compareTimeStamps(const epicsTimeStamp & a, const epicsTimeStamp & b);


/**
*	End time stamp zero means the AU is active; treated in comparisons as "infinity"
*	@return True, if seconds AND nanoseconds of the time stamp are 0; false otherwise
*/
bool isTimeStampZero(const epicsTimeStamp & a);

/**
*	No negative time stamps allowed
*	@return True, if seconds and nanoseconds are not negative; false otherwise
*/
bool isTimeStampValid(const epicsTimeStamp & a);

#endif //my_epics_time.h

