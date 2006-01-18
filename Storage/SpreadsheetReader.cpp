// Tools
#include "MsgLogger.h"
// Storage
#include "DataFile.h"
#include "SpreadsheetReader.h"

SpreadsheetReader::SpreadsheetReader(Index &index,
                                     ReaderFactory::How how, double delta)
  : index(index),
    how(how),
    delta(delta),
    num(0)
{
}

SpreadsheetReader::~SpreadsheetReader()
{
    DataFile::clear_cache();
}

bool SpreadsheetReader::find(const stdVector<stdString> &channel_names,
                             const epicsTime *start)
{
    try
    {
        num       = channel_names.size();
        reader    = new AutoPtr<DataReader>[num];
        typedef const RawValue::Data * constRawValueData;
        read_data = new constRawValueData[num];
        info      = new AutoPtr<CtrlInfo>[num];
        type      = new DbrType[num];
        count     = new DbrCount[num];
        value     = new RawValueAutoPtr[num];
    }
    catch (...)
    {
        throw new GenericException(__FILE__, __LINE__,
                                   "SpreadsheetReader::find cannot allocate mem");
    }
    size_t i;
    for (i=0; i<num; ++i)
    {
        reader[i]    = ReaderFactory::create(index, how, delta);
        read_data[i] = reader[i]->find(channel_names[i], start);
        if (!read_data[i]) // Channel has no data
            continue;
        try
        {
            info[i] = new CtrlInfo(reader[i]->getInfo());
        }
        catch (...)
        {
            throw new GenericException(__FILE__, __LINE__,
                                       "SpreadsheetReader::find cannot "
                                       "allocate info %zu\n",i);
        }
    }
    return next();
}

bool SpreadsheetReader::next()
{
    bool have_any = false;
    epicsTime stamp;
    size_t i;
    // Compute oldest time stamp.
    for (i=0; i<num; ++i)
    {
        if (! read_data[i])
            continue;
        stamp = RawValue::getTime(read_data[i]);
        if (! have_any)
            time = stamp;
        else if (stamp < time)
            time = stamp;
        have_any = true;
    }
    if (!have_any)
        return false;
    // Extrapolate current data onto time.
    for (i=0; i<num; ++i)
    {
        if (!read_data[i])
        {   // No (more) values for this channel.
            if (value[i])
            {   // Invalidate all the info.
                value[i].release();
                type[i] = 0;
                count[i] = 0;
            }
            continue;
        }
        if (RawValue::getTime(read_data[i]) <= time)
        {
            if (reader[i]->changedInfo())
                *info[i] = reader[i]->getInfo();
            if (! value[i] ||
                type[i]  != reader[i]->getType() ||
                count[i] != reader[i]->getCount())
            {
                type[i]  = reader[i]->getType();
                count[i] = reader[i]->getCount();
                value[i] = RawValue::allocate(type[i], count[i], 1);
            }
            RawValue::copy(type[i], count[i], value[i], read_data[i]);
            read_data[i] = reader[i]->next(); // advance reader.
        }
        // else leave value[i] as is, which initially means: 0
    }    
    return have_any;
}

