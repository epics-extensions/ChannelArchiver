// interval.h

#ifndef _INTERVAL_H_
#define _INTERVAL_H_

#include <stdio.h>
#include "rtree_constants.h"
#include "my_epics_time.h"

/**
*	Start time {0, 0} means NULL
*	End time {0, 0} means INFINITY
*/
class interval
{
public:	
	interval();
	interval(long start_Sec, long start_Nsec,long end_Sec, long end_Nsec);
	interval(const epicsTimeStamp & _start, const epicsTimeStamp & _end);

	const epicsTimeStamp & getStart() const		{return start;} 
	const epicsTimeStamp & getEnd() const		{return end;}
	void setStart(const epicsTimeStamp & s)		{start = s;	}
	void setEnd(const epicsTimeStamp & e)		{end = e;	}

	/**
	*	If the validity of the intervals is not certain, the user should call isIntervalValid() first
	*	@see isIntervalValid()
	*	@return True, if <i>this</i> interval intersects the "other"; false, otherwise
	*/
	bool isIntervalOver(const interval & other) const;

	/**
	*	If the validity of the intervals is not certain, the user should call isIntervalValid() first
	*	@return 
	*		 0, if intervals are identical
	*		 1, if <i>this</i> interval has an earlier start time; or, if the start times
						  are identical, a later end time
	*		-1  otherwise
	*	@see isIntervalValid()
	*/
	long compareIntervals(const interval & othert) const;  // <0 ~ <, ==0 ~ ==, > 0 ~ >
	
	/**
	*	@return True if end time and start time are valid, and if end time is "later" than start time;
	*	false otherwise
	*/
	bool isIntervalValid() const;

	/**
	*	@return True, if all attributes are 0; false otherwise
	*/
	bool isNull() const;

	void operator=(const interval & other);

	/**
	*	When used in Visual Studio, prints only the long values; otherwise converts 
	*	epicsTimeStamp to a string
	*/
	void print(FILE * text_File) const;	
	
	/**
	*	Only the file pointer is copied into the object; NO memcpy or similar
	*	MUST be called before invoking any i/o methods
	*	Does not check if the file pointer is valid (see archiver_Index::open())	
	*/
	void attach(FILE * f, long interval_Address);
	
	/**
	*	Set the object attributes according to the values from the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherwise
	*/
	bool readInterval();	

	/**
	*	Write the object attributes to the file this object was attached to
	*	@see attach()	
	*	@return False if errors occured, or attach() was not called before; true otherise
	*/
	bool writeInterval() const;		

private:
	bool readStart();
	bool readEnd();
	bool writeStart() const;	
	bool writeEnd() const;
	
	struct epicsTimeStamp start;
	struct epicsTimeStamp end;

	FILE * f;
	long interval_Address;
};

#endif //interval.h




