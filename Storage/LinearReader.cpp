// Storage
#include "LinearReader.h"

LinearReader::LinearReader(archiver_Index &index, double delta)
        : reader(index), delta(delta)
{
}

const RawValue::Data *LinearReader::find(
    const stdString &channel_name,
    const epicsTime *start,
    const epicsTime *end)
{
    return reader.find(channel_name, start, end);
}
    
const RawValue::Data *LinearReader::next()
{
    return reader.next();
}

DbrType LinearReader::getType() const
{   return reader.getType(); }
    
DbrCount LinearReader::getCount() const
{   return reader.getCount(); }
    
const CtrlInfo &LinearReader::getInfo() const
{   return reader.getInfo(); }
    
bool LinearReader::changedType()
{   return reader.changedType(); }
    
bool LinearReader::changedInfo()
{   return reader.changedInfo(); }
