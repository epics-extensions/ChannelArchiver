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
}  TestData;
static TestData test_data1[] =
{
    {  "3",  "4" },
    {  "1",  "2" },
    {  "5",  "6" },
    { "11", "12" },
    {  "7",  "8" },
    {  "9", "10" },
    { "15", "16" },
    { "17", "18" },
    { "19", "20" },
    { "13", "14" },
    { "23", "24" },
    { "21", "22" },
    { "25", "26" },
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
    int i, num = sizeof(test_data1)/sizeof(TestData);
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        if (i==(num-1))
            tree.makeDot("index0.dot");
        string2epicsTime(test_data1[i].start, start);
        string2epicsTime(test_data1[i].end, end);
        if (!tree.insert(start, end, i+1))
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    test_data1[i].start, test_data1[i].end, i+1);
            return false;
        }
        if (!tree.insert(start, end, i+1))
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    test_data1[i].start, test_data1[i].end, i+1);
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
    tree.makeDot("test/test_data1.dot");

    string2epicsTime(test_data1[num/2].start, start);
    RTree::Node node, tmp;
    if (!tree.search(start, node, i))
    {
        fprintf(stderr, "Cannot locate entry\n");
        return false;
    }
    tmp = node;
    num = i;
    printf("ID %ld\n", node.record[i].child_or_ID);
    while (tree.next(node, i))
        printf("Next ID %ld\n", node.record[i].child_or_ID);
    node = tmp;
    i = num;
    while (tree.prev(node, i))
        printf("Prev ID %ld\n", node.record[i].child_or_ID);

    fa.detach();
    fclose(f);
    puts("OK: RTree fill");
    return true;
}

bool delete_test()
{
    FILE *f = fopen("test/tree.tst", "r+b");
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);

    RTree tree(fa, 0);
    tree.reattach();
    if (!tree.selfTest())
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    int i, num = sizeof(test_data1)/sizeof(TestData);
    epicsTime start, end;
    for (i=num-1; i>=0; --i)
    {
        string2epicsTime(test_data1[i].start, start);
        string2epicsTime(test_data1[i].end, end);
        if (!tree.remove(start, end, i+1))
        {
            fprintf(stderr, "Removal %s..%s: %d failed\n",
                    test_data1[i].start, test_data1[i].end, i+1);
            return false;
        }
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree.makeDot("test/test_data1d.dot");
    fa.detach();
    fclose(f);
    puts("OK: RTree remove");
    return true;
}

static TestData test_data2[] =
{
    {  "1",  "2" },
    {  "3",  "4" },
    {  "5",  "6" },
    {  "7",  "8" },
    {  "9", "10" },
    { "11", "12" },
    { "13", "14" },
    { "15", "16" },
    { "17", "18" },
    { "19", "20" },
    { "21", "22" },
    { "23", "24" },
    { "25", "26" },
};

bool ordered_test()
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    size_t i, num = sizeof(test_data2)/sizeof(TestData);
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        string2epicsTime(test_data2[i].start, start);
        string2epicsTime(test_data2[i].end, end);
        if (!(tree.insert(start, start, i+1) &&
              tree.updateLatest(start, end, i+1)))
        {
            fprintf(stderr, "Insert %s..%s %d failed\n",
                    test_data2[i].start, test_data2[i].end, i+1);
            return false;
        }
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree.makeDot("test/test_data2.dot");
    fa.detach();
    fclose(f);
    puts("OK: RTree ordered insert");
    return true;
}

static TestData test_data3[] =
{
    { "10", "12" },
    { "11", "13" },
    { "15", "16" },
    { "14", "17" },
    { "13", "14" },
    { "14", "18" },
    {  "9", "18" },
    { "13", "14" },
    { "19", "20" },
    { "6", "7" },
    { "5", "9" },
};

bool overlap_test()
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    size_t i, num = sizeof(test_data3)/sizeof(TestData);
    epicsTime start, end;
    for (i=0; i<num; ++i)
    {
        if (i==num-1)
            tree.makeDot("index0.dot");
        string2epicsTime(test_data3[i].start, start);
        string2epicsTime(test_data3[i].end, end);
        if (!tree.insert(start, end, i+1))
        {
            fprintf(stderr, "Insert %s..%s %d failed\n",
                    test_data3[i].start, test_data3[i].end, i+1);
            return false;
        }
        if (i==num-1)
            tree.makeDot("index1.dot");
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }

    fa.detach();
    fclose(f);
    puts("OK: RTree overlap test");
    return true;
}        

bool tree_test4(int num)
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    int i;
    epicsTimeStamp stamp;
    epicsTime start, end;
    stamp.nsec = 0;
    for (i=0; i<num; ++i)
    {
        stamp.secPastEpoch = (i+1)*2;
        start = stamp;
        stamp.secPastEpoch = (i+1)*2+1;
        end = stamp;
        if (start >= end)
            continue;
        if (!tree.insert(start, end, i+1))
        {
            fprintf(stderr, "Insert %d failed\n", i+1);
            return false;
        }
    }
    if (!tree.selfTest())
    {
        fprintf(stderr, "Self test failed\n");
        return false;
    }
    if (num <=300)
    {
        tree.makeDot("order.dot");
	    stamp.secPastEpoch = (num/2+1)*2;
	    start = stamp;
	    RTree::Node node, tmp;
	    if (!tree.search(start, node, i))
	    {
            fprintf(stderr, "Cannot locate %d\n", num/2);
            return false;
	    }
        tmp = node;
        printf("ID for %d: %ld\n", (num/2+1)*2, node.record[i].child_or_ID);
	    while (tree.next(node, i))
            printf("Next ID %ld\n", node.record[i].child_or_ID);
	    node = tmp;
	    while (tree.prev(node, i))
            printf("Prev ID %ld\n", node.record[i].child_or_ID);
    }
    fa.detach();
    fclose(f);
    return true;
}

bool tree_test5(size_t num)
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    size_t i;
    epicsTimeStamp stamp;
    epicsTime start, end;
    stamp.nsec = 0;
    for (i=0; i<num; ++i)
    {
        stamp.secPastEpoch = (epicsUInt32)((rand()*999.0) / RAND_MAX) + 1;
        start = stamp;
        stamp.secPastEpoch = (epicsUInt32)((rand()*999.0) / RAND_MAX) + 1;
        end = stamp;
        if (start >= end)
            continue;
        if (!tree.insert(start, end, i+1))
        {
            fprintf(stderr, "Insert %d failed\n", i+1);
            return false;
        }
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree.makeDot("random.dot");
    fa.detach();
    fclose(f);
    return true;
}

bool datatest()
{
    FILE *f = fopen("test/tree.tst", "w+b");
    FileAllocator fa;
    fa.attach(f, RTree::anchor_size);
    RTree tree(fa, 0);
    tree.init();
    size_t i,num=10;
    epicsTimeStamp stamp;
    epicsTime start, end;
    stamp.nsec = 0;
    char buffer[80];
    for (i=0; i<num; ++i)
    {
        stamp.secPastEpoch = (i+1)*2;
        start = stamp;
        stamp.secPastEpoch += 1;
        end = stamp;
        sprintf(buffer, "/a/b/c/file%d", (i+1)*2);
        if (!tree.insertDatablock(start, end, (i+1)*2, buffer))
        {
            fprintf(stderr, "Insert %d failed\n", i+1);
            return false;
        }
        if (!tree.selfTest())
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree.makeDot("data.dot", true);
    fa.detach();
    fclose(f);
    return true;
}

int main()
{
    initEpicsTimeHelper();
    if (!fill_test())
        return -1;
    if (!delete_test())
        return -1;
    if (!ordered_test())
        return -1;
    if (!overlap_test())
        return -1;
    if (!tree_test4(1000))
        return -1;
    if (!datatest())
        return -1;
    return 0;
}
