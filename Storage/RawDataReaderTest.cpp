// System
#include <stdio.h>
// Tools
#include <UnitTest.h>
// Storage
#include "IndexFile.h"
#include "RawDataReader.h"

static bool read_test(const stdString &index_name, const stdString &channel_name)
{
    stdString t, s, v;

    try
    {
        IndexFile index;
        index.open(index_name);
        RawDataReader reader(index);
        const RawValue::Data *value = reader.find(channel_name, 0);
        while (value)
        {
            epicsTime2string(RawValue::getTime(value), t);
            RawValue::getStatus(value, s);
            RawValue::getValueString(
                v, reader.getType(), reader.getCount(), value);
            printf("%s\t%s\t%s\n",
                   t.c_str(), v.c_str(), s.c_str());
            value = reader.next();
        }
        index.close();
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        return false;
    }
    return true;
}

TEST_CASE RawDataReaderTest()
{
    TEST(read_test("../DemoData/index", "fred"));
    TEST_OK;
}
