#include "interval.h"
#include "bin_io.h"

interval::interval() 
:	f(0), interval_Address(-1)
{
	start.secPastEpoch = 0;
	start.nsec =  0;
	end.secPastEpoch = 0;
	end.nsec = 0;
}

interval::interval(const epicsTimeStamp & _start, const epicsTimeStamp & _end)
:	f(0), interval_Address(-1)
{
	start = _start;
	end = _end;
}

interval::interval(long start_Sec, long start_Nsec, long end_Sec, long end_Nsec)
:	f(0), interval_Address(-1)
{
	start.secPastEpoch = start_Sec;
	start.nsec =  start_Nsec;
	end.secPastEpoch = end_Sec;
	end.nsec = end_Nsec;
}

bool interval::isIntervalOver(const interval& other) const
{
	//special treatment for 0 end time values
	//intervals are treated as being "exclusive"
	if(isTimeStampZero(end))
	{
		return compareTimeStamps(other.getEnd(), start) > 0;
	}
	if(isTimeStampZero(other.getEnd()))
	{
		return compareTimeStamps(end, other.getStart()) > 0;
	}
	return	compareTimeStamps(other.getEnd(), start) > 0 && 
			compareTimeStamps(end, other.getStart()) > 0;
}

long interval::compareIntervals(const interval& other) const
{
	//sorted by start times DESC and then end times ASC 
	long result;
	if((result = compareTimeStamps(other.getStart(), start)) == 0)
	{
		return compareTimeStamps(end, other.getEnd());
	}
	return result;
}

bool interval::isIntervalValid() const
{
	if(isTimeStampZero(end))
	{
		return isTimeStampValid(start);
	}
	else
	{
		return	isTimeStampValid(start) && isTimeStampValid(end) && 
				compareTimeStamps(end, start) >= 0;
	}
}

bool interval::isNull() const
{
	return start.nsec == 0 && start.secPastEpoch == 0 && end.nsec == 0 && end.secPastEpoch == 0;
}
void interval::operator=(const interval & other)
{
	start = other.getStart();
	end = other.getEnd();
}

//////////
//IO below
//////////

void interval::attach(FILE * f, long interval_Address)
{
	this->f = f;
	this->interval_Address = interval_Address;
}

bool interval::readInterval()
{
	return
		(
			readStart() &&
			readEnd()
		);
}

bool interval::writeInterval() const
{
	return
		(
			writeStart() &&
			writeEnd()
		);
}

bool interval::readStart()
{
	if(f == 0 || interval_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	long ADDRESS = interval_Address + IV_START_SEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	long value;
	if(readLong(f, &value) == false)
	{
		printf("Failed to read the seconds of the start time from the interval from the address %ld", ADDRESS);
		return false;
	}
	start.secPastEpoch = value;

	ADDRESS = interval_Address + IV_START_NSEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, &value) == false)
	{
		printf("Failed to read the nanoseconds of the start time from the interval from the address %ld", ADDRESS);
		return false;
	}
	start.nsec = value;

	return true;
}

bool interval::readEnd()
{
	if(f == 0 || interval_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	long ADDRESS = interval_Address + IV_END_SEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	long value;
	if(readLong(f, &value) == false)
	{
		printf("Failed to read the seconds of the end time from the interval from the address %ld", ADDRESS);
		return false;
	}
	end.secPastEpoch = value;

	ADDRESS = interval_Address + IV_END_NSEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(readLong(f, &value) == false)
	{
		printf("Failed to read the nanoseconds of the end time from the interval from the address %ld", ADDRESS);
		return false;
	}
	end.nsec = value;	
	return true;
}

bool interval::writeStart() const
{
	if(f == 0 || interval_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}
	long ADDRESS = interval_Address + IV_START_SEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, start.secPastEpoch) == false)
	{
		printf("Failed to write the seconds of the start time to the address %ld", ADDRESS);
		return false;
	}

	ADDRESS = interval_Address + IV_START_NSEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, start.nsec) == false)
	{
		printf("Failed to write the nanoseconds of the start time to the address %ld", ADDRESS);
		return false;
	}
	return true;
}

bool interval::writeEnd() const
{
	if(f == 0 || interval_Address < 0) 
	{
		printf("No file specified\n");
		return false;
	}

	long ADDRESS = interval_Address + IV_END_SEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, end.secPastEpoch) == false)
	{
		printf("Failed to write the seconds of the end time to the address %ld", ADDRESS);
		return false;
	}

	ADDRESS = interval_Address + IV_END_NSEC_OFFSET;
	fseek(f, ADDRESS, SEEK_SET);
	if(writeLong(f, end.nsec) == false)
	{
		printf("Failed to write the nanoseconds of the end time to the address %ld", ADDRESS);
		return false;
	}

	return true;
}


