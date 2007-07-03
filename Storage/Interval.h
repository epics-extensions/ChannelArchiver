#ifndef INTERVAL_H_
#define INTERVAL_H_

// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>

/** A time interval. */
class Interval
{
public:
	/** Construct empty Interval. */
	Interval()
	{
		clear();
	}
	
	/** Construct Interval with given start..end. */
	Interval(const epicsTime &start, const epicsTime &end)
	   : start(start), end(end)
	{
        LOG_ASSERT(start <= end);
    }
	
	/** @return Start time of Interval. */
	const epicsTime &getStart() const
	{   return start; }
	
	/** @return End time of Interval. */
	const epicsTime &getEnd() const
	{   return end; }
    
    /** @return time range of this interval in seconds. */
    double width() const
    {   return end - start; }

	/** Reset Interval to null start and end times. */
	void clear()
	{
	    start = nullTime;
    	end = nullTime;
	}

    /** Update start time of Interval. */
    void setStart(const epicsTime &time)
    {   start = time; }
    
    /** Update end time of Interval. */
    void setEnd(const epicsTime &time)
    {   end = time; }

	/** Union; Cause Interval to include the range of the other Interval. */
	void add(const Interval &other)
	{
        if (start > other.start)
            start = other.start;
        if (end < other.end)
            end = other.end;
        LOG_ASSERT(start <= end);
	}
	
	/** @return true if both start and end time are non-null. */
	bool isValid() const
	{
		return start != nullTime && end != nullTime;
	}
	
    /** Check if this interval 'covers' another one,
     *  meaning this interval matches the other one,
     *  or starts before and ends after the other one.
     *  @return true if this interval fully covers (or matches)
     *          other interval. */
    bool covers(const Interval &other) const
    {
        return start <= other.start  &&  end >= other.end;
    }

    /** @return true if this interval matches other interval. */
    bool operator == (const Interval &other) const
    {
        return start == other.start  &&  end == other.end;
    }

    /** @return true if this interval differs from other interval. */
    bool operator != (const Interval &other) const
    {
        return start != other.start  ||  end != other.end;
    }

    /** Check if this and other interval overlap.
     *  Note: They might touch, e.g. start1 <= end1  <= start2 <= end2,
     *        but that's not considered an overlap.
     *  @return true if they overlap.
     */
    bool overlaps(const Interval &other) const
    {
        // Same as this: 
        // return end > other.start   &&  other.end > start;
        if (end <= other.start   ||  other.end <= start)
            return false;
        return true;
    }
    
    stdString toString() const;

private:
	epicsTime start, end;
};

#endif /*INTERVAL_H_*/
