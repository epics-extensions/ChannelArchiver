// Tools
#include "MsgLogger.h"
// Storage
#include "LinearReader.h"

#undef DEBUG_LINREAD

LinearReader::LinearReader(IndexFile &index, double delta)
        : AverageReader(index, delta)
{
}

const RawValue::Data *LinearReader::find(
    const stdString &channel_name, const epicsTime *start)
{
    this->channel_name = channel_name;
    if (!(reader_data = reader.find(channel_name, start)))
        return 0;
    if (start)
        time_slot = *start;
    else
        time_slot =
            roundTimeUp(RawValue::getTime(reader_data), delta);
    return next();
}

const RawValue::Data *LinearReader::next()
{
    if (!reader_data)
        return 0;
#ifdef DEBUG_LINREAD
    stdString txt;
    printf("Next time slot: %s\n", epicsTimeTxt(time_slot, txt));
#endif
    // To interpolate onto the time slot, we need the last value before
    // and the first sample after the time slot.
    bool anything = false;
    epicsTime t0, t1;
    double d0, d1;
    while (reader_data  &&  RawValue::getTime(reader_data) < time_slot)
    {
#ifdef DEBUG_LINREAD
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
            type = reader.getType();
            count = reader.getCount();
            if (!(data = RawValue::allocate(type, count, 1)))
            {
                LOG_MSG("LinearReader: Cannot allocate data %d/%d\n",
                        type, count);
                return 0;
            }
        }
        RawValue::copy(type, count, data, reader_data);
        reader_data = reader.next();
        if (count==1  &&  !RawValue::isInfo(data) &&
            RawValue::getDouble(type, count, data, d0))
        {
            t0 = RawValue::getTime(data);
            anything = true;
        }
        else   // This sample can't be interpolated; return as is.
            return data;
    }
    if (anything)
    {
        if (reader_data && !RawValue::isInfo(reader_data) &&
            reader.getCount()==1 &&
            RawValue::getDouble(reader.getType(), reader.getCount(),
                                reader_data, d1))
        {   // Have good pre- and post time_slot sample: Interpolate.
#ifdef DEBUG_LINREAD
            printf("Interpolating %g .. %g onto time slot\n", d0, d1);
#endif
            t1 = RawValue::getTime(reader_data);
            double dT = t1-t0;
            if (dT > 0)
                RawValue::setDouble(type, count, data,
                                    d0 + (d1-d0)*((time_slot-t0)/dT));
            else // Use average if there's no time between t0..t1?
                RawValue::setDouble(type, count, data, (d0 + d1)/2);
            RawValue::setTime(data, time_slot);
        }
        // Else: Have only pre-time_slot sample: Return as is
    }
    // Else: nothing pre-time_slot. See if we find anything in following slot
    time_slot += delta;
    if (anything)
        return data;
    return next();
}

