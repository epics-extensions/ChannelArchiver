// Storage
#include "DataFile.h"
#include "SpreadsheetReader.h"

SpreadsheetReader::SpreadsheetReader(archiver_Index &index)
        : index(index)
{
    num = 0;
    reader = 0;
    data = 0;
}

SpreadsheetReader::~SpreadsheetReader()
{
    size_t i;
    if (reader)
    {
        for (i=0; i<num; ++i)
            if (reader[i])
                delete reader[i];
        free(reader);
    }
    if (data)
        free(data);
    DataFile::close_all();
}

bool SpreadsheetReader::find(const stdVector<stdString> &channel_names,
                             const epicsTime *start)
{
    size_t i;
    num = channel_names.size();
    reader = (DataReader **)malloc(sizeof(DataReader *) * num);
    data = (const RawValue::Data **)malloc(sizeof(DataReader *) * num);
    for (i=0; i<num; ++i)
    {
        printf("%s\n", channel_names[i].c_str());
        reader[i] = new DataReader(index);
        data[i] = reader[i]->find(channel_names[i], start, 0);
    }
    return update();
}

void SpreadsheetReader::update()
{
    size_t i;

    time = nullStamp;
    for (i=0; i<num; ++i)
    {
        printf("%s\n", channel_names[i].c_str());
        reader[i] = new DataReader(index);
        data[i] = reader[i]->find(channel_names[i], start, 0);
        if (data[i]  &&  RavValue::getTime(data[i]) < time)
            time = RavValue::getTime(data[i]);
    }
}
