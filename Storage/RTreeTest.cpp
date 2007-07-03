
#define INDEX_TEST

// System
#include <stdlib.h>
// Tools
#include <AutoPtr.h>
#include <BinIO.h>
#include <UnitTest.h>
// Storage
#include "RTree.h"
#include "FileAllocator.h"
#ifdef INDEX_TEST
#include "IndexFile.h"
#endif

// Test data for fill_test
typedef struct
{
    const char *start, *end;
    const char *file;
    long offset;
}  TestData;

// Used in manual
static TestData man_data1[] =
{
    { "1", "2", "FileA",  0x10 },
    { "2", "3", "FileA",  0x20 },
    { "3", "4", "FileA",  0x30 },
    { "4", "5", "FileA",  0x40 },
    { "5", "6", "FileA",  0x50 },
    { "6", "7", "FileA",  0x60 },
};
// ...
static TestData man_data2[] =
{
    { "4", "6", "FileB",  0x10 },
    { "6", "8", "FileB",  0x20 },
    { "8", "9", "FileB",  0x30 },
    { "9","10", "FileB",  0x40 },
};
// ... both
static TestData man_data3[] =
{
    { "1", "2", "FileA",  0x10 },
    { "2", "3", "FileA",  0x20 },
    { "3", "4", "FileA",  0x30 },
    { "4", "5", "FileA",  0x40 },
    { "5", "6", "FileA",  0x50 },
    { "6", "7", "FileA",  0x60 },
    { "4", "6", "FileB",  0x10 },
    { "6", "8", "FileB",  0x20 },
    { "8", "9", "FileB",  0x30 },
    { "9","10", "FileB",  0x40 },
};
// This is meant to handle all cases at least once.
static TestData fill_data[] =
{
    { "50", "60", "50-60",  0 },    // starting point
    { "70", "80", "70-80",  1 },    // way 'after'
    { "80", "85", "80-85",  2 },    // just 'after'
    { "20", "30", "20-30",  3 },    // way 'before', also node overflow
    { "40", "50", "40-50",  4 },    // just 'before'
    { "50", "60", "50-60(B)",  5 }, // exact match to existing range
    { "20", "32", "20-32",  6 },    // longer than 20-30
    { "31", "33", "31-33",  7 },    // overlap right end of 30-32
    { "65", "80", "65-80",  8 },    // starts before 70-80
    { "35", "45", "35-45",  9 },    // left overlap with 40-50
    { "62", "63", "62-63", 10 },    // New range between 50-60 and 65-70
    { "61", "64", "61-64", 11 },    // bigger than 62-63
    { "40", "42", "40-42", 12 },    // first half of 40-45
    { "25", "30", "25-30", 13 },    // last half of 20-30
    { "47", "48", "47-48", 14 },    // subsection of 45-50
    { "10", "90", "10-90", 15 },    // overlap the whole tree
};

// Array of the above tests
typedef struct
{
    const char *index_name;
    const TestData *data;
    size_t num;
    const char *dotfile;
    const char *diff;
}   TestConfig;
static TestConfig test_config[] =
{
    {
        "test/tree.tst",
        fill_data, sizeof(fill_data)/sizeof(TestData),
        "test/test_data1.dot", "test/test_data1.dot.OK"
    },
    {
        "test/man_index_a",
        man_data1, sizeof(man_data1)/sizeof(TestData),
        "test/man_index_a.dot", 0
    },
    {
        "test/man_index_b",
        man_data2, sizeof(man_data2)/sizeof(TestData),
        "test/man_index_b.dot", 0
    },
    {
        "test/man_index_ab",
        man_data3, sizeof(man_data3)/sizeof(TestData),
        "test/man_index_ab.dot", 0
    },
};

TEST_CASE fill_tests()
{
    size_t test = 0, i=0;
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    try
    {   // Loop over test configs
        for (test=0; test<sizeof(test_config)/sizeof(TestConfig); ++test)
        {
            printf("Fill Test %zu: %s, %s\n",
                   test,
                   test_config[test].index_name,
                   test_config[test].dotfile);
            stdString directory;
            // Open File
            TEST_DELETE_FILE(test_config[test].index_name);
            AutoFilePtr f(test_config[test].index_name, "w+b");
            // Attach FileAllocator
            FileAllocator fa;
            fa.attach(f, RTree::anchor_size, true);
            // Add RTree
            AutoPtr<RTree> tree(new RTree(fa, 0));
            tree->init(3);

            unsigned long nodes, records;
            TEST(tree->selfTest(nodes, records));
            epicsTime start, end;
            size_t num = test_config[test].num;
            // Loop over data points
            for (i=0; i<num; ++i)
            {
                // Create dot file of 'before' and 'after' state
                // for the last insertion.
                tree->makeDot("/tmp/index0.dot");
                // Insert new data point
                string2epicsTime(test_config[test].data[i].start, start);
                string2epicsTime(test_config[test].data[i].end, end);
                stdString filename = test_config[test].data[i].file;
                if (!tree->insertDatablock(Interval(start, end),
                                           test_config[test].data[i].offset,
                                           filename))
                {
                    printf("Insert %s..%s: %zu failed\n",
                           test_config[test].data[i].start,
                           test_config[test].data[i].end, i);
                    FAIL("insertDatablock (1)");
                }
                // When inserted again, that should be a NOP
                if (tree->insertDatablock(Interval(start, end),
                                          test_config[test].data[i].offset,
                                          filename))
                {
                    printf("Re-Insert %s..%s: %zu failed\n",
                           test_config[test].data[i].start,
                           test_config[test].data[i].end, i);
                    FAIL("insertDatablock (2)");
                }
                // ... 'after' of the last insertion
                tree->makeDot("/tmp/index1.dot");
                // Check
                if (!tree->selfTest(nodes, records))
                {
                    FAIL("RTree::selfTest");
                }
            }
            printf("%ld nodes, %ld used records, %ld records total (%.1lf %%)\n",
                   nodes, records, nodes*tree->getM(),
                   records*100.0/(nodes*tree->getM()));
            tree->makeDot(test_config[test].dotfile);
            TEST(fa.dump(0));
            
            // Remove entries
            for (i=0; i<num; ++i)
            {
                // Remove this data block
                string2epicsTime(test_config[test].data[i].start, start);
                string2epicsTime(test_config[test].data[i].end, end);
                stdString filename = test_config[test].data[i].file;
                tree->makeDot("/tmp/remove0.dot");
                if (!tree->removeDatablock(Interval(start, end),
                                           test_config[test].data[i].offset,
                                           filename))
                {
                    printf("Remove %s..%s @ 0x%lX: %zu failed\n",
                           test_config[test].data[i].start,
                           test_config[test].data[i].end,
                           (unsigned long) test_config[test].data[i].offset,
                           i);
                    FAIL("removeDatablock (1)");
                }
                tree->makeDot("/tmp/remove1.dot");
                TEST(tree->selfTest(nodes, records));
            }
            
            fa.detach();
            if (test_config[test].diff)
            {
                TEST_FILEDIFF(test_config[test].dotfile, test_config[test].diff);
            }
        }
    }
    catch (GenericException &e)
    {
        printf("Exception during sub-test %zu, item %zi:\n%s\n",
               test, i, e.what());
        FAIL("Exception");
    }
    TEST_OK;
}

// Dump all the blocks for the major test case
TEST_CASE dump_blocks()
{
    try
    {
        AutoFilePtr f("test/tree.tst", "rb");
        FileAllocator fa;
        fa.attach(f, RTree::anchor_size, true);
        RTree tree(fa, 0);
        tree.reattach();
        unsigned long nodes, records;
        TEST(tree.selfTest(nodes, records));
        stdString s, e;
        AutoPtr<RTree::Datablock> block(tree.getFirstDatablock());
        while (block)
        {
            const Interval &range = block->getInterval();
            printf("%s - %s: '%s' @ 0x%lX\n",
                   epicsTimeTxt(range.getStart(), s),
                   epicsTimeTxt(range.getEnd(), e),
                   block->getDataFilename().c_str(),
                   (unsigned long)block->getDataOffset());
            while (block->getNextChainedBlock())
                printf("---  '%s' @ 0x%lX\n",
                       block->getDataFilename().c_str(),
                       (unsigned long)block->getDataOffset());
            if (! block->getNextDatablock())
                block = 0;
        }
        fa.detach();
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        FAIL("Exception");
    }
    TEST_OK;
}

#ifdef INDEX_TEST
static TestData update_data[] =
{
    { "100", "100", "old", 1 }, // Engine adds a block
    { "100", "200", "old", 1 }, // .. grows it.
    { "200", "200", "old", 2 }, // Adds another block
    { "200", "205", "old", 2 }, // .. grows it,
    { "200", "210", "old", 2 }, // .. grows it, then quits.
    { "209", "209", "new",  3 }, // New engine's block wedges into previous
    { "209", "220", "new",  3 }, // .. growing, adding more records to end
    { "209", "230", "new",  3 }, // .. growing
    { "209", "270", "new",  3 }, //... grown up.
    { "280", "280", "new2", 4 }, // Second block of new engine, one sample 
    { "280", "281", "new2", 4 }, // .. growing
    { "280", "282", "new2", 4 }, // .. growing
    { "280", "283", "new2", 4 }, // .. growing
    { "280", "290", "new2", 4 }, // done.
};

TEST_CASE update_test()
{
    const size_t num = sizeof(update_data)/sizeof(TestData);
    size_t i = 0;

    TEST_DELETE_FILE("test/update.tst");
    try
    {
        IndexFile index(10);
        FileAllocator::minimum_size = 0;
        FileAllocator::file_size_increment = 0;
        FileAllocator fa;
        index.open("test/update.tst", Index::ReadAndWrite);
    
        AutoPtr<Index::Result> result(index.addChannel("test"));
        TEST_MSG(result, "Added Channel");
    
        unsigned long nodes, records;
        TEST_MSG(result->getRTree()->selfTest(nodes, records), "Initial Self Test");
    
        epicsTime start, end;
        for (i=0; i<num; ++i)
        {
            result->getRTree()->makeDot("/tmp/update0.dot");
            string2epicsTime(update_data[i].start, start);
            string2epicsTime(update_data[i].end, end);
            stdString filename = update_data[i].file;
            result->getRTree()->updateLastDatablock(Interval(start, end),
                                      update_data[i].offset, filename);
            result->getRTree()->makeDot("/tmp/update1.dot");
            TEST_MSG(result->getRTree()->selfTest(nodes, records), "Self test");
        }
        result->getRTree()->makeDot("test/update_data.dot");
        TEST_MSG(result->getRTree()->selfTest(nodes, records), "Final Self Test");
        TEST_FILEDIFF("test/update_data.dot", "test/update_data.dot.OK");
    }
    catch (GenericException &e)
    {
        printf("Exception while processing item %zu:\n%s\n", i, e.what());
        FAIL("Exception");
    }

    TEST_OK;
}
#endif

