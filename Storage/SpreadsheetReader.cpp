// Tools
#include "MsgLogger.h"
// Storage
#include "DataFile.h"
#include "LinearReader.h"
#include "SpreadsheetReader.h"

SpreadsheetReader::SpreadsheetReader(IndexFile &index, double delta)
        : index(index), delta(delta)
{
    num = 0;
    reader = 0;
    read_data = 0;
    info = 0;
    type = 0;
    count = 0;
    value = 0;
}

SpreadsheetReader::~SpreadsheetReader()
{
    size_t i;
    if (value)
    {
        for (i=0; i<num; ++i)
            if (value[i])
                RawValue::free(value[i]);
        free(value);
    }
    free(count);
    free(type); 
    if (info)
    {
        for (i=0; i<num; ++i)
            if (info[i])
                delete info[i];
        free(info);
    }
    free(read_data);
    if (reader)
    {
        for (i=0; i<num; ++i)
            if (reader[i])
                delete reader[i];
        free(reader);
    }
    DataFile::close_all();
}

bool SpreadsheetReader::find(const stdVector<stdString> &channel_names,
                             const epicsTime *start)
{
    size_t i;
    num = channel_names.size();
    reader = (DataReader **)calloc(num, sizeof(DataReader *));
    read_data = (const RawValue::Data **)calloc(num, sizeof(RawValue::Data *));
    info = (CtrlInfo **)calloc(num, sizeof(CtrlInfo *));
    type =  (DbrType *)calloc(num, sizeof(DbrType));
    count =  (DbrCount *)calloc(num, sizeof(DbrCount));
    value = (RawValue::Data **)calloc(num, sizeof(RawValue::Data *));
    if (!(reader && read_data && info && type && count && value))
    {
        LOG_MSG("SpreadsheetReader::find cannot allocate mem\n");
        return false;
    }
    for (i=0; i<num; ++i)
    {
        if (delta > 0)
            reader[i] = new LinearReader(index, delta);
        else
            reader[i] = new RawDataReader(index);
        if (!reader[i])
        {
            LOG_MSG("SpreadsheetReader::find cannot allocate reader %d\n", i);
            return false;
        }
        read_data[i] = reader[i]->find(channel_names[i], start, 0);
        if (read_data[i])
        {
            info[i] = new CtrlInfo(reader[i]->getInfo());
            if (!info[i])
            {
                LOG_MSG("SpreadsheetReader::find cannot allocate info %d\n",i);
                return false;
            }
        }
    }
    return next();
}

bool SpreadsheetReader::next()
{
    bool have_any = false;
    epicsTime stamp;
    size_t i;
    // compute oldest time stamp
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
    // extrapolate current data onto time
    for (i=0; i<num; ++i)
    {
        if (read_data[i])
        {
            if (RawValue::getTime(read_data[i]) <= time)
            {
                if (reader[i]->changedInfo())
                    *info[i] = reader[i]->getInfo();
                if (value[i]==0 ||
                    type[i] != reader[i]->getType() ||
                    count[i] != reader[i]->getCount())
                {
                    RawValue::free(value[i]);
                    type[i] = reader[i]->getType();
                    count[i] = reader[i]->getCount();
                    value[i] = RawValue::allocate(type[i], count[i], 1);
                    if (!value[i])
                    {
                        LOG_MSG("SpreadsheetReader cannot allocate value\n");
                        return false;
                    }
                }
                RawValue::copy(type[i], count[i], value[i], read_data[i]);
                read_data[i] = reader[i]->next(); // advance reader
            }
            // else leave value[i] as is, which initially means: 0
        }
    }    
    return have_any;
}
