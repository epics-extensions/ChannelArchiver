// System
#include <stdio.h>
// Tools
#include <UnitTest.h>
// Storage
#include "IndexFile.h"
#include "DataFile.h"
#include "RawDataReader.h"

static void dump(const DataReader &reader, const RawValue::Data *value, const char *prefix="")
{
    stdString t, s, v;
    epicsTime2string(RawValue::getTime(value), t);
    RawValue::getStatus(value, s);
    RawValue::getValueString(
        v, reader.getType(), reader.getCount(), value);
    printf("        %s%s\t%s\t%s\n",
           prefix, t.c_str(), v.c_str(), s.c_str());
}

static size_t read_test(const stdString &index_name, const stdString &channel_name,
                        const epicsTime *start = 0, const epicsTime *end = 0)
{
    size_t num = 0;
    try
    {
        IndexFile index;
        index.open(index_name);
        RawDataReader reader(index);
        const RawValue::Data *value = reader.find(channel_name, start);
        while (value &&
               (end==0  ||  RawValue::getTime(value) < *end))
        {
            ++num;
            dump(reader, value);
            value = reader.next();
        }
        index.close();
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        return 0;
    }
    return num;
}

TEST_CASE RawDataReaderTest()
{
    TEST(read_test("../DemoData/index", "fred") == 87);

    epicsTime start;
    TEST(string2epicsTime("03/23/2004 10:50:42.400561000", start));
    TEST(read_test("../DemoData/index", "fred", &start) == 10);

    epicsTime end(start);
    end += 5;
    TEST(read_test("../DemoData/index", "fred", &start, &end) == 3);

    TEST(DataFile::clear_cache() == 0);
    TEST_OK;
}

static size_t dual_read_test(const stdString &index_name, const stdString &channel_name,
                             const epicsTime *start = 0, const epicsTime *end = 0)
{
    size_t num = 0;
    try
    {
        IndexFile index1, index2;
        index1.open(index_name);
        index2.open(index_name);
        RawDataReader reader1(index1);
        RawDataReader reader2(index2);
        const RawValue::Data *value1 = reader1.find(channel_name, start);
        const RawValue::Data *value2 = reader2.find(channel_name, start);
        while ((value1 &&
                (end==0  ||  RawValue::getTime(value1) < *end)) ||
               (value2 &&
                (end==0  ||  RawValue::getTime(value2) < *end)) )
        {
            if (value1 && (end==0  ||  RawValue::getTime(value1) < *end))
            {
                ++num;
                dump(reader1, value1, "1) ");
                value1 = reader1.next();
            }
            if (value2 && (end==0  ||  RawValue::getTime(value2) < *end))
            {
                ++num;
                dump(reader2, value2, "2) ");
                value2 = reader2.next();
            }
        }
        index1.close();
        index2.close();
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        return 0;
    }
    return num;
}

TEST_CASE DualRawDataReaderTest()
{
    TEST(dual_read_test("../DemoData/index", "fred") == 2*87);

    epicsTime start;
    TEST(string2epicsTime("03/23/2004 10:50:42.400561000", start));
    TEST(dual_read_test("../DemoData/index", "fred", &start) == 2*10);

    epicsTime end(start);
    end += 5;
    TEST(dual_read_test("../DemoData/index", "fred", &start, &end) == 2*3);

    TEST(DataFile::clear_cache() == 0);
    TEST_OK;
}

