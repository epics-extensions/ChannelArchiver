// System
#include <stdlib.h>
// Tools
#include <MsgLogger.h>
#include <BinIO.h>
// Index
#include "RTree.h"

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

static TestData fill_data[] =
{
    { "10", "11", "10-11",  1 },
    {  "7",  "8",   "7-8",  2 }, // insert left
    { "13", "14", "13-14",  3 }, // insert right
    { "17", "18", "17-18",  4 }, // overflow right
    {  "3",  "4",   "3-4",  5 }, // overflow left
    { "16", "18", "16-18",  6 }, // right part overlaps existing entry
    { "17", "19", "17-19",  7 }, // left part overlaps existing entry
    { "13", "14", "13-14(B)",  8 }, // fully hidden under existing record
    { "11", "16", "11-16",  9 }, // fully overlaps existing entry
    {  "9", "12",  "9-12", 10 }, // fully overlaps existing entry
    {  "2", "11",  "2-11", 11 }, // fully overlaps several entries
    { "20", "22", "20-22", 12 }, // insert right
    { "19", "20", "19-20", 13 }, // insert between
    { "18", "23", "18-23", 14 }, // left overlaps 3 records
};

bool fill_test(const TestData *data, int num, const char *dotfile)
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    if (!tree.selfTest())
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    int i;
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        if (i==(num-1))
            tree.makeDot("index0.dot");
        string2epicsTime(data[i].start, start);
        string2epicsTime(data[i].end, end);
        stdString filename = data[i].file;
        if (tree.insertDatablock(start, end, data[i].offset, filename)
            != RTree::YNE_Yes)
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (tree.insertDatablock(start, end, data[i].offset, filename)
            != RTree::YNE_No)
        {
            fprintf(stderr, "Re-Insert %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (i==(num-1))
            tree.makeDot("index1.dot");
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree.makeDot(dotfile);
    if (fa.dump(0))
        puts("OK: FileAllocator's view of the tree");
    else
    {
        fprintf(stderr, "FileAllocator is unhappy\n");
        return false;
    }
    fa.detach();
    fclose(f);
    puts("OK: RTree fill");
    return true;
}

bool dump_blocks()
{
    FILE *f = fopen("test/tree.tst", "rb");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.reattach();
    if (!tree.selfTest())
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    stdString s, e;
    RTree::Datablock block;
    RTree::Node node;
    int idx;
    bool ok;
    for (ok = tree.getFirstDatablock(node, idx, block);
         ok;
         ok = tree.getNextDatablock(node, idx, block))
    {
        printf("%s - %s: '%s' @ 0x%lX\n",
               epicsTimeTxt(node.record[idx].start, s),
               epicsTimeTxt(node.record[idx].end, e),
               block.data_filename.c_str(), block.data_offset);
        while (tree.getNextChainedBlock(block))
            printf("---  '%s' @ 0x%lX\n",
                   block.data_filename.c_str(), block.data_offset);
    }
    fa.detach();
    fclose(f);
    puts("OK: RTree fill");
    return true;
}

int main()
{
    initEpicsTimeHelper();
    if (!fill_test(man_data1, sizeof(man_data1)/sizeof(TestData),
                   "man_data1.dot"))
        return -1;
    if (!fill_test(fill_data, sizeof(fill_data)/sizeof(TestData),
                   "test/test_data1.dot"))
        return -1;
    if (!dump_blocks())
        return -1;
    return 0;
}
