// System
#include <stdlib.h>
// Tools
#include <AutoPtr.h>
#include <BinIO.h>
#include <UnitTest.h>
// Storage
#include "IndexFile.h"

// Test data for fill_test
typedef struct
{
    const char *start, *end;
    const char *file;
    long offset;
}  TestData;

// This is meant to handle all cases at least once.
static TestData fill_data[] =
{
    { "20", "21", "20-21",  1 },    // starting point
    { "10", "11", "10-11",  2 },    // insert left
    { "30", "31", "30-31",  3 },    // insert right
    { "40", "41", "40-41",  4 },    // overflow right
};

static const char *names[] =
{
    "Fred", "Freddy", "Jane", "Janet", "Allen"
};

static void add_blocks(RTree *tree)
{
    const size_t num = sizeof(fill_data)/sizeof(TestData);
    for (size_t i = 0; i<num; ++i)
    {
        epicsTime start, end;
        string2epicsTime(fill_data[i].start, start);
        string2epicsTime(fill_data[i].end, end);
        stdString filename = fill_data[i].file;
        tree->updateLastDatablock(Interval(start, end),
                                  fill_data[i].offset, filename);
    }
}

TEST_CASE index_file_test()
{
    const size_t num = sizeof(names)/sizeof(char *);

    TEST_DELETE_FILE("test/index_file.tst");
    size_t i;
    try
    {
        IndexFile index(10);
        index.open("test/index_file.tst", Index::ReadAndWrite);
   
        for (i=0; i<num; ++i)
        {
            AutoPtr<Index::Result> result(index.addChannel(names[i]));
            TEST_MSG(result, "Added Channel");
            add_blocks(result->getRTree());
            
            result = index.findChannel(names[i]);
            TEST_MSG(result, "Found Channel");
            TEST_MSG(index.check(10), "Self Test");
            
            AutoPtr<Index::NameIterator> names(index.iterator());
            size_t count = 0;
            while (names->isValid())
            {
                ++count;
                printf("Channel: '%s'\n", names->getName().c_str());
                names->next();
            }
            TEST_MSG(count == (i + 1), "Found all names");
        }
    }
    catch (GenericException &e)
    {
        printf("Exception while processing item %zu:\n%s\n", i, e.what());
        FAIL("Exception");
    }

    TEST_OK;
}
