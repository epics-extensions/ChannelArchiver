// Tools
#include "MsgLogger.h"
// Storage
#include "LinearReader.h"

#undef DEBUG_LINREAD

LinearReader::LinearReader(archiver_Index &index, double delta)
        : reader(index), delta(delta)
{
    type = 0;
    count = 0;
    data = 0;
}

LinearReader::~LinearReader()
{
    RawValue::free(data);
}

const RawValue::Data *LinearReader::find(
    const stdString &channel_name,
    const epicsTime *start,
    const epicsTime *end)
{
    reader_data = reader.find(channel_name, start, end);
    if (reader_data)
    {
#ifdef DEBUG_LINREAD
        printf("Raw: ");
        RawValue::show(stdout,
                       reader.getType(), reader.getCount(),
                       reader_data,
                       &reader.getInfo());
#endif
        time_slot =
            roundTimeUp(RawValue::getTime(reader_data), delta);
        return next();
    }
    return 0;
}
    
const RawValue::Data *LinearReader::next()
{
#ifdef DEBUG_LINREAD
    stdString txt;
    printf("Next time slot: %s\n", epicsTimeTxt(time_slot, txt));
#endif
    // iterate reader until just after time_slot
    int N = 0;
    double d, d2, sum = 0;
    while (reader_data &&
           RawValue::getTime(reader_data) < time_slot)
    {
        // the last value before we cross the time_slot
        // is what we might want to extrapolate onto time_slot,
        // so keep a copy of the reader_data before calling reader.next()
        if (reader.changedInfo())
        {
            info = reader.getInfo();
            ctrl_info_changed = true;
        }        
        if (data == 0 ||
            type != reader.getType() || count != reader.getCount())
        {
            RawValue::free(data);
            type = reader.getType();
            count = reader.getCount();
            data = RawValue::allocate(type, count, 1);
            type_changed = true;
            if (!data)
            {
                LOG_MSG("LinearReader: Cannot allocate data %d/%d\n",
                        type, count);
                return 0;
            }
        }
        RawValue::copy(type, count, data, reader_data);
        if (RawValue::getDouble(type, count, data, d))
        {
            sum += d;
            ++N;
        }   
        reader_data = reader.next();
#ifdef DEBUG_LINREAD
        if (reader_data)
        {
            printf("Raw: ");
            RawValue::show(stdout,
                           reader.getType(), reader.getCount(),
                           reader_data, &reader.getInfo());
        }
#endif
    }
    if (!reader_data)
        return 0;
    if (!data)
        return 0;
    // data.time <  time_slot, reader_data.time >= time_slot
    // N >= 1  if we accumulated N values in 'sum'

    // Attempt linear interpolation if there's one
    // point available before and after the time_slot
    if (N <= 1  &&
        !RawValue::isInfo(data) &&
        RawValue::getDouble(type, count, data, d) &&
        RawValue::getDouble(reader.getType(), reader.getCount(),
                            reader_data, d2))
    {
#ifdef DEBUG_LINREAD
        printf("Interpolating %g .. %g onto time slot\n", d, d2);
#endif
        epicsTime t = RawValue::getTime(data);
        epicsTime t2 = RawValue::getTime(reader_data);
        double dT = t2-t;
        if (dT > 0)
            RawValue::setDouble(type, count, data,
                                d + (d2-d)*((time_slot-t)/dT));
        else // Use average if there's no time between d..d2?
            RawValue::setDouble(type, count, data, (d + d2)/2);
        RawValue::setTime(data, time_slot);
    }
    else if (N > 1)
    {   // If more than one point fell into the time_slot,
        // report average at middle of slot
#ifdef DEBUG_LINREAD
        printf("Using average over last %d samples\n", N);
#endif
        RawValue::setDouble(type, count, data, sum/N);
        RawValue::setTime(data, time_slot-delta/2);
    }
	else
    {   // Otherwise map last point before slot onto slot
#ifdef DEBUG_LINREAD
        printf("Staircase-mapping onto time_slot\n");
#endif
        RawValue::setTime(data, time_slot);
    }
    time_slot += delta;
    return data;
}

DbrType LinearReader::getType() const
{   return type; }
    
DbrCount LinearReader::getCount() const
{   return count; }
    
const CtrlInfo &LinearReader::getInfo() const
{   return info; }
    
bool LinearReader::changedType()
{
    bool changed = type_changed;
    type_changed = false;
    return changed;
}

bool LinearReader::changedInfo()
{
    bool changed = ctrl_info_changed;
    ctrl_info_changed = false;
    return changed;
} 
