// Tools
#include "MsgLogger.h"
// Storage
#include "DataFile.h"
#include "SpreadsheetReader.h"

SpreadsheetReader::SpreadsheetReader(archiver_Index &index)
        : index(index)
{
    num = 0;
    reader = 0;
    read_data = 0;
    value = 0;
}

SpreadsheetReader::~SpreadsheetReader()
{
    size_t i;
    if (value)
        free(value);
    if (reader)
    {
        for (i=0; i<num; ++i)
            if (reader[i])
                delete reader[i];
        free(reader);
    }
    if (read_data)
        free(read_data);
    DataFile::close_all();
}

bool SpreadsheetReader::find(const stdVector<stdString> &channel_names,
                             const epicsTime *start)
{
    size_t i;
    num = channel_names.size();
    reader = (DataReader **)calloc(num, sizeof(DataReader *));
    read_data = (const RawValue::Data **)calloc(num, sizeof(RawValue::Data *));
    value = (const RawValue::Data **)calloc(num, sizeof(RawValue::Data *));
    if (!(reader && read_data && value))
    {
        LOG_MSG("SpreadsheetReader::find cannot allocate mem\n");
        return false;
    }
    for (i=0; i<num; ++i)
    {
        printf("%s\n", channel_names[i].c_str());
        reader[i] = new DataReader(index);
        if (!reader[i])
        {
            LOG_MSG("SpreadsheetReader::find cannot get reader\n");
            return false;
        }
        read_data[i] = reader[i]->find(channel_names[i], start, 0);
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
        have_any = true;
        stamp = RawValue::getTime(read_data[i]);
        if (i==0)
            time = stamp;
        else if (stamp < time)
            time = stamp;
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
                value[i] = read_data[i];
                read_data[i] = reader[i]->next(); // advance reader
            }
            // else leave value[i] as is, which initially means: 0
        }
        else
            value[i] = 0;
    }
    return have_any;
}
