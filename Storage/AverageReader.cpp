// Tools
#include "MsgLogger.h"
// Storage
#include "AverageReader.h"

// #define DEBUG_AVGREAD

AverageReader::AverageReader(Index &index, double delta)
  : reader(index),
    delta(delta),
    reader_data(0),
    type(0),
    count(0),
    type_changed(false),
    ctrl_info_changed(false),
    is_raw(false),
    minimum(0.0),
    maximum(0.0)
{
}

const RawValue::Data *AverageReader::find(
    const stdString &channel_name, const epicsTime *start)
{
    reader_data = reader.find(channel_name, start);
    if (!reader_data)
        return 0;
    // The reported time stamp will be end_of_bin-delta/2.
    if (start)
    {   // ... 1st sample will be at start:
        end_of_bin = *start + delta/2.0;
        while (RawValue::getTime(reader_data) < *start)
        {
            reader_data = reader.next();
            if (!reader_data)
                return 0;
        }
    }
    else // ... 1st sample will be at rounded-up stamp
        end_of_bin =
            roundTimeUp(RawValue::getTime(reader_data), delta) + delta/2.0;
    return next();
}
    
const stdString &AverageReader::getName() const
{
    return reader.getName();
}                                  
    
const RawValue::Data *AverageReader::next()
{
    if (!reader_data)
        return 0;
#   ifdef DEBUG_AVGREAD
    stdString txt;
    printf("Next time slot: %s\n", epicsTimeTxt(end_of_bin, txt));
#   endif
    size_t N = 0;
    double d, sum = 0;
    short stat = 0, sevr = 0;
    if (RawValue::getTime(reader_data) > end_of_bin)
    {   // Continue where the data is, skip bins that have nothing anyway
        end_of_bin = roundTimeUp(RawValue::getTime(reader_data), delta) + delta/2.0;
#       ifdef DEBUG_AVGREAD
        printf("Adjusted: %s\n", epicsTimeTxt(end_of_bin, txt));
#       endif
    }
    // Loop over data until reaching end of current bin.
    while (reader_data  &&  RawValue::getTime(reader_data) < end_of_bin)
    {   // iterate reader until just after end_of_bin
#       ifdef DEBUG_AVGREAD
        printf("Raw: ");
        RawValue::show(stdout, reader.getType(), reader.getCount(),
                       reader_data, &reader.getInfo());
#       endif
        // copy reader_data before calling reader.next()
        if (reader.changedInfo())
        {
            info = reader.getInfo();
            ctrl_info_changed = true;
        }        
        if (!data || type!=reader.getType() || count!=reader.getCount())
        {
            type  = reader.getType();
            count = reader.getCount();
            data  = RawValue::allocate(type, count, 1);
            type_changed = true;
        }
        RawValue::copy(type, count, data, reader_data);
        // For scalars with data, average over the
        // first element (no full waveform average)
        if (count == 1  &&  !RawValue::isInfo(data)  &&
            RawValue::getDouble(type, count, data, d))
        {
            const int severity = RawValue::getSevr(data);
            if (severity != ARCH_REPEAT  &&
                severity != ARCH_EST_REPEAT)
            {   // Unless it's a 'repeat' of previous data,
	        // average over numeric scalar data
                sum += d;
                ++N;
                if ((severity & ARCH_BASE_MASK) > sevr)
                {   // Maximize stat/severity
                    sevr = severity;
                    stat = RawValue::getStat(data);
                }
                // Update minimum/maximum
                if (N==1)
                    minimum = maximum = d;
                else
                {
                    if (minimum > d)
                        minimum = d;
                    if (maximum < d)
                        maximum = d;
                }   
            }
            reader_data = reader.next();
        }
        else
        {   // Special values, waveforms and non-numerics are reported as is,
            // and we skip to the next bin:
            N = 1;
            do
            {
                reader_data = reader.next();
#ifdef DEBUG_AVGREAD
                printf("Moving forward to: ");
                RawValue::show(stdout, reader.getType(), reader.getCount(),
                               reader_data, &reader.getInfo());
#endif
            }
            while (reader_data && RawValue::getTime(reader_data) < end_of_bin);
        }   
    }
    if (N == 1)
    {   // Single value, already in 'data', report as is
#       ifdef DEBUG_AVGREAD
        printf("Single Sample\n");
#       endif
        is_raw = true;
    }
    else if (N > 1)
    {
       // Sufficient points in bin, report average at middle of slot
#       ifdef DEBUG_AVGREAD
        printf("Using average over last %zd samples\n", N);
#       endif
        is_raw = false;
        RawValue::setStatus(data, stat, sevr);
        RawValue::setTime(data, end_of_bin-delta/2);
        if (! RawValue::setDouble(type, count, data, sum/N))
            return 0;
    }
    else
    {   // N==0
#       ifdef DEBUG_AVGREAD
        printf("No data found, trying next bin.\n");
#       endif
        return next();
    }

    end_of_bin += delta;
    return data;
}

const RawValue::Data *AverageReader::get() const
{   return data; }

DbrType AverageReader::getType() const
{   return type; }
    
DbrCount AverageReader::getCount() const
{   return count; }
    
const CtrlInfo &AverageReader::getInfo() const
{   return info; }
    
bool AverageReader::changedType()
{
    bool changed = type_changed;
    type_changed = false;
    return changed;
}

bool AverageReader::changedInfo()
{
    bool changed = ctrl_info_changed;
    ctrl_info_changed = false;
    return changed;
} 

