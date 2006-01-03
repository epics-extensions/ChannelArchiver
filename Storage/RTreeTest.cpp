// System
#include <stdlib.h>
// Tools
#include <MsgLogger.h>
#include <BinIO.h>
#include <UnitTest.h>
// Storage
#include "RTree.h"
#include "FileAllocator.h"

typedef struct
{
    const char *start, *end;
    const char *file;
    long offset;
}  TestData;

static TestData man_data1[] =
{
    { "1", "2", "FileA",  0x10 },
    { "2", "3", "FileA",  0x20 },
    { "3", "4", "FileA",  0x30 },
    { "4", "5", "FileA",  0x40 },
    { "5", "6", "FileA",  0x50 },
    { "6", "7", "FileA",  0x60 },
};

static TestData man_data2[] =
{
    { "4", "6", "FileB",  0x10 },
    { "6", "8", "FileB",  0x20 },
    { "8", "9", "FileB",  0x30 },
    { "9","10", "FileB",  0x40 },
};

static TestData fill_data[] =
{
    { "20", "21", "20-21",  1 },
    { "10", "11", "10-11",  2 }, // insert left
    { "30", "31", "30-31",  3 }, // insert right
    { "40", "41", "40-41",  4 }, // overflow right
    { "10", "15", "10-15",  5 }, // left overlaps with existing
    { "39", "41", "38-41",  6 }, // right overlaps with existing
    { "26", "27", "26-27",  7 }, // overflow left
    { "26", "32", "26-32",  8 }, // left overlaps & interleaves 2 exist. recs
    { "25", "30", "25-30",  9 }, // rigth overlaps 2 exist. recs
    { "20", "21", "20-21(B)", 10 }, // fully hidden under existing record
    { "50", "51", "50-51",  11 }, // insert right
    { "60", "61", "60-61",  12 }, // overflow right
    { "70", "71", "70-71",  13 }, // insert right
    { "80", "81", "80-81",  14 }, // overflow right
    { "90", "91", "90-91",  15 }, // insert right
    { "95", "96", "95-96",  16 }, // overflow right
    { "96", "97", "96-97",  17 }, // insert right
    { "98", "99", "98-99",  18 }, // overflow right
    { "38", "39", "38-39",  19 }, // insert left in central node
    { "37", "38", "37-38",  20 }, // overflow left in central node
};

static bool fill_test(const char *index_name,
                      const TestData *data, int num, const char *dotfile)
{
    stdString directory;
    RTree *tree;
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    FileAllocator fa;
    AutoFilePtr f(index_name, "w+b");
    fa.attach(f, RTree::anchor_size);
    tree = new RTree(fa, 0);
    tree->init(3);
    unsigned long nodes, records;
    if (!tree->selfTest(nodes, records))
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    int i;
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        if (i==(num-1))
            tree->makeDot("index0.dot");
        string2epicsTime(data[i].start, start);
        string2epicsTime(data[i].end, end);
        stdString filename = data[i].file;
        if (!tree->insertDatablock(start, end, data[i].offset, filename))
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (tree->insertDatablock(start, end, data[i].offset, filename))
        {
            fprintf(stderr, "Re-Insert %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (i==(num-1))
            tree->makeDot("index1.dot");
        if (!tree->selfTest(nodes, records))
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    printf("%ld nodes, %ld used records, %ld records total (%.1lf %%)\n",
           nodes, records, nodes*tree->getM(),
           records*100.0/(nodes*tree->getM()));
    tree->makeDot(dotfile);
    delete tree;
    
    if (fa.dump(0))
        puts("OK: FileAllocator's view of the tree");
    else
    {
        fprintf(stderr, "FileAllocator is unhappy\n");
        return false;
    }
    fa.detach();
   
    puts("OK: RTree fill");
    return true;
}

bool dump_blocks()
{
    AutoFilePtr f("test/tree.tst", "rb");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.reattach();
    unsigned long nodes, records;
    if (!tree.selfTest(nodes, records))
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    stdString s, e;
    RTree::Datablock block;
    RTree::Node node(tree.getM(), true);
    int idx;
    bool ok;
    for (ok = tree.getFirstDatablock(node, idx, block);
         ok;
         ok = tree.getNextDatablock(node, idx, block))
    {
        printf("%s - %s: '%s' @ 0x%lX\n",
               epicsTimeTxt(node.record[idx].start, s),
               epicsTimeTxt(node.record[idx].end, e),
               block.data_filename.c_str(),
               (unsigned long)block.data_offset);
        while (tree.getNextChainedBlock(block))
            printf("---  '%s' @ 0x%lX\n",
                   block.data_filename.c_str(),
                   (unsigned long)block.data_offset);
    }
    fa.detach();
    puts("OK: RTree Dump");
    return true;
}

static TestData update_data[] =
{
    { "200", "210", "-last-", 1 }, // Last engine's last block
    { "209", "209", "-new-",  1 }, // New engine's block, hidden
    { "209", "220", "-new-",  1 }, // .. growing
    { "209", "230", "-new-",  1 }, // .. growing
    { "230", "270", "-new-",  1 }, // .. unreal, but handled as growing.
    { "280", "280", "-new2-", 1 }, // Second block of new engine, one sample 
    { "280", "281", "-new2-", 1 }, // .. growing
    { "280", "282", "-new2-", 1 }, // .. growing
    { "280", "283", "-new2-", 1 }, // .. growing
    { "280", "290", "-new2-", 1 }, // .. growing
};

TEST_CASE rtree_test()
{
    initEpicsTimeHelper();
    try
    {
        TEST(fill_test("test/tree.tst",
                       fill_data, sizeof(fill_data)/sizeof(TestData),
                       "test/test_data1.dot"));
        TEST(dump_blocks());
    }
    catch (GenericException &e)
    {
        printf("Exception: %s", e.what());
        FAIL("Exception");
    }
    TEST_OK;
}
