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
    {  "3",  "4", "A", 1 },
    {  "1",  "2", "A", 2 }, // insert left
};

bool fill_test()
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
    int i, num = sizeof(fill_data)/sizeof(TestData);
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        if (i==(num-1))
            tree.makeDot("index0.dot");
        string2epicsTime(fill_data[i].start, start);
        string2epicsTime(fill_data[i].end, end);
        stdString filename = fill_data[i].file;
        if (!tree.insertDatablock(start, end,
                                  fill_data[i].offset, filename))
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    fill_data[i].start, fill_data[i].end, i+1);
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
    fa.detach();
    fclose(f);
    puts("OK: RTree fill");
    return true;
}

int main()
{
    initEpicsTimeHelper();
    if (!fill_test())
        return -1;
    return 0;
}
