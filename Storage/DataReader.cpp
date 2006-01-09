// DataReader.cpp

// Storage
#include "DataReader.h"

DataReader::~DataReader()
{}

void DataReader::toString(stdString &text) const
{
    const RawValue::Data *value = get();
    stdString t, s, v;
    epicsTime2string(RawValue::getTime(value), t);
    RawValue::getStatus(value, s);
    RawValue::getValueString(
        v, getType(), getCount(), value, &getInfo());

    text.reserve(t.length() + v.length() + s.length() + 2);
    text = t;
    if (RawValue::isInfo(value))
        text += "\t#N/A";
    else
    {
        text += '\t';
        text += v;
    }
    if (s.length() > 0)
    {
        text += '\t';
        text += s;
    }
}

