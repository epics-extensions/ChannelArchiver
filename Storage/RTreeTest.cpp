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

static TestData fill_data[] =
{
    { "10", "11", "10-11",  1 },
    {  "7",  "8", " 7-8",  2 }, // insert left
    { "13", "14", "13-14",  3 }, // insert right
    { "17", "18", "17-18",  4 }, // overflow right
    {  "3",  "4", "  3-4",  5 }, // overflow left
    { "16", "18", "16-18",  6 }, // right part overlaps existing entry
    { "17", "19", "17-19",  7 }, // left part overlaps existing entry
    { "13", "14", "13-14(B)",  8 }, // fully hidden under existing record
    { "11", "16", "11-16",  9 }, // fully overlaps existing entry
};

bool fill_test(const TestData *data, int num)
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

int main()
{
    initEpicsTimeHelper();
    if (!fill_test(fill_data, sizeof(fill_data)/sizeof(TestData)))
        return -1;
    return 0;
}
