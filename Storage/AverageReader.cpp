// Tools
#include "MsgLogger.h"
// Storage
#include "AverageReader.h"

#undef DEBUG_AVGREAD

AverageReader::AverageReader(IndexFile &index, double delta)
        : reader(index), delta(delta)
{
    type = 0;
    count = 0;
    data = 0;
}

AverageReader::~AverageReader()
{
    RawValue::free(data);
}

const RawValue::Data *AverageReader::find(
    const stdString &channel_name, const epicsTime *start)
{
    this->channel_name = channel_name;
    reader_data = reader.find(channel_name, start);
    channel_found = reader_data != 0;
    if (!channel_found)
        return 0;
    if (start)
    {
        end_of_bin = *start + delta;
        while (RawValue::getTime(reader_data) < *start)
        {
            if (!(reader_data = reader.next()))
                return 0;
        }
    }
    else
        end_of_bin =
            roundTimeUp(RawValue::getTime(reader_data), delta);
    return next();
}
    
const RawValue::Data *AverageReader::next()
{
    if (!reader_data)
        return 0;
#ifdef DEBUG_AVGREAD
    stdString txt;
    printf("Next time slot: %s\n", epicsTimeTxt(end_of_bin, txt));
#endif
    size_t N = 0;
    double d, sum = 0;
    short stat = 0, sevr = 0;
    bool anything = false;
    if (RawValue::getTime(reader_data) > end_of_bin)
    {   // Continue where the data is, skip bins that have nothing anyway
        end_of_bin = roundTimeUp(RawValue::getTime(reader_data), delta);
#ifdef DEBUG_AVGREAD
        printf("Adjusted: %s\n", epicsTimeTxt(end_of_bin, txt));
#endif
    }
    while (reader_data  &&  RawValue::getTime(reader_data) < end_of_bin)
    {   // iterate reader until just after end_of_bin
#ifdef DEBUG_AVGREAD
        printf("Raw: ");
        RawValue::show(stdout, reader.getType(), reader.getCount(),
                       reader_data, &reader.getInfo());
#endif
        // copy reader_data before calling reader.next()
        if (reader.changedInfo())
        {
            info = reader.getInfo();
            ctrl_info_changed = true;
        }        
        if (data==0 || type!=reader.getType() || count!=reader.getCount())
        {
            if (data)
                RawValue::free(data);
            type_changed = true;
            type  = reader.getType();
            count = reader.getCount();
            if (!(data  = RawValue::allocate(type, count, 1)))
            {
                LOG_MSG("AverageReader: Cannot allocate data %d/%d\n",
                        type, count);
                return 0;
            }
        }
        RawValue::copy(type, count, data, reader_data);
        if (count == 1  &&  !RawValue::isInfo(data)  &&
            RawValue::getDouble(type, count, data, d))
        {   // average over numeric scalar data
            sum += d;
            ++N;
            if ((RawValue::getSevr(data) & ARCH_BASE_MASK) > sevr)
            {   // Maximize stat/severity
                sevr = RawValue::getSevr(data);
                stat = RawValue::getStat(data);
            }
            reader_data = reader.next();
        }
        else
        {   // Special values, non-scalars and non-numerics
            // are reported as is, interrupting the binning
            anything = true;
            N = 0;
            do
            {
                reader_data = reader.next();
#ifdef DEBUG_AVGREAD
                printf("Skipping: ");
                RawValue::show(stdout, reader.getType(), reader.getCount(),
                               reader_data, &reader.getInfo());
#endif
            }
            while (reader_data && RawValue::getTime(reader_data) < end_of_bin);
        }   
    }
    if (N > 0)
    {   // Sufficient points in bin, report average at middle of slot
#ifdef DEBUG_AVGREAD
        printf("Using average over last %d samples\n", N);
#endif
        RawValue::setStatus(data, stat, sevr);
        anything = RawValue::setDouble(type, count, data, sum/N);
        RawValue::setTime(data, end_of_bin-delta/2);
    }
    end_of_bin += delta;
    if (anything)
        return data;
    return next();
}

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
