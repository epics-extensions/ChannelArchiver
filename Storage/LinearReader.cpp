// Tools
#include "MsgLogger.h"
// Storage
#include "LinearReader.h"

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
        printf("Raw: ");
        RawValue::show(stdout,
                       reader.getType(), reader.getCount(),
                       reader_data,
                       &reader.getInfo());
        time_slot =
            roundTimeUp(RawValue::getTime(reader_data), delta);
        return next();
    }
    return 0;
}
    
const RawValue::Data *LinearReader::next()
{
    stdString txt;
    printf("Next time slot: %s\n", epicsTimeTxt(time_slot, txt));

    // iterate reader until just after time_slot
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
        if (data == 0 || type != reader.getType() || count != reader.getCount())
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
        
        reader_data = reader.next();
        if (reader_data)
        {
            printf("Raw: ");
            RawValue::show(stdout,
                           reader.getType(), reader.getCount(),
                           reader_data, &reader.getInfo());
        }
    }
    if (!reader_data)
        return 0;

    if (data)
    {
        printf("Extrapolating previous value onto time_slot\n");
        RawValue::setTime(data, time_slot);
    }
    time_slot =
        roundTimeUp(RawValue::getTime(reader_data), delta);
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
