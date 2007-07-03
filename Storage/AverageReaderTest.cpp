// System
#include <stdio.h>
// Tools
#include <UnitTest.h>
#include <MsgLogger.h>
// Storage
#include "IndexFile.h"
#include "DataFile.h"
#include "AverageReader.h"

static size_t read_test(const stdString &index_name, const stdString &channel_name,
                        double delta,
                        const epicsTime *start = 0, const epicsTime *end = 0)
{
    stdString text;
    size_t num = 0;
    try
    {
        IndexFile index;
        index.open(index_name);
        AverageReader reader(index, delta);
        const RawValue::Data *value = reader.find(channel_name, start);
        while (value &&
               (end==0  ||  RawValue::getTime(value) < *end))
        {
            ++num;
            LOG_ASSERT(value == reader.get());
            reader.toString(text);
            if (reader.isRaw())
                printf("==> %s (raw sample)\n", text.c_str());
            else
                printf("==> %s (average over %g ... %g)\n",
                       text.c_str(),
                       reader.getMinimum(),
                       reader.getMaximum());
            value = reader.next();
        }
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        return 0;
    }
    return num;
}

TEST_CASE AverageReaderTest()
{
    TEST(read_test("../DemoData/index", "fred", 10.0) == 18);

    // Read start .... inf, delta 10 sec
    epicsTime start;
    TEST(string2epicsTime("03/23/2004 10:50:00", start));
    TEST(read_test("../DemoData/index", "fred", 10.0, &start) == 7);

    // .. until given end, delta 5 sec
    epicsTime end(start);
    end += 15;
    TEST(read_test("../DemoData/index", "fred", 5.0, &start, &end) == 3);

    // Delta small enough to hit 'raw' samples
    end = start;
    end += 10;
    TEST(read_test("../DemoData/index", "fred", 2.5, &start, &end) == 4);

    TEST(DataFile::clear_cache() == 0);

    TEST_OK;
}

