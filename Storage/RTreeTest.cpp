// System
#include <stdlib.h>
// Tools
#include <MsgLogger.h>
#include <BinIO.h>
// Storage
#include "RawValue.h"
#include "IndexFile.h"

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

bool fill_test(bool use_index, const char *index_name,
               const TestData *data, int num, const char *dotfile)
{
    IndexFile index(3);
    FILE *f;
    RTree *tree;
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    FileAllocator fa;
    if (use_index)
    {
        index.open(index_name, false);
        tree = index.addChannel("test");
        if (!tree)
        {
            fprintf(stderr, "index.addChannel failed\n");
            return false;
        }
    }
    else
    {
        f = fopen(index_name, "w+b");
        fa.attach(f, RTree::anchor_size);
        tree = new RTree(fa, 0);
        tree->init(3);
    }
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
        if (tree->insertDatablock(start, end, data[i].offset, filename)
            != RTree::YNE_Yes)
        {
            fprintf(stderr, "Insert %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (tree->insertDatablock(start, end, data[i].offset, filename)
            != RTree::YNE_No)
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
    if (use_index)
    {
        index.close();
    }
    else
    {
        if (fa.dump(0))
            puts("OK: FileAllocator's view of the tree");
        else
        {
            fprintf(stderr, "FileAllocator is unhappy\n");
            return false;
        }
        fa.detach();
        fclose(f);
    }
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
               block.data_filename.c_str(), block.data_offset);
        while (tree.getNextChainedBlock(block))
            printf("---  '%s' @ 0x%lX\n",
                   block.data_filename.c_str(), block.data_offset);
    }
    fa.detach();
    fclose(f);
    puts("OK: RTree Dump");
    return true;
}

static TestData update_data[] =
{
    { "200", "210", "-20-",  1 },
    { "209", "209", "-21-",  1 },
    { "209", "220", "-21-",  1 },
};

bool update_test(const char *index_name,
                 const TestData *data, int num, const char *dotfile)
{
    IndexFile index(10);
    RTree *tree;
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    FileAllocator fa;
    index.open(index_name, false);
    tree = index.addChannel("test");
    if (!tree)
    {
        fprintf(stderr, "index.addChannel failed\n");
        return false;
    }    
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
            tree->makeDot("update0.dot");
        string2epicsTime(data[i].start, start);
        string2epicsTime(data[i].end, end);
        stdString filename = data[i].file;
        if (tree->updateLastDatablock(start, end, data[i].offset, filename)
            == RTree::YNE_Error)
        {
            fprintf(stderr, "Update %s..%s: %d failed\n",
                    data[i].start, data[i].end, i+1);
            return false;
        }
        if (i==(num-1))
            tree->makeDot("update1.dot");
        if (!tree->selfTest(nodes, records))
        {
            fprintf(stderr, "Self test failed\n");
            return false;
        }
    }
    tree->makeDot(dotfile);
    delete tree;
    index.close();
    puts("OK: RTree update");
    return true;
}

#include <assert.h>

void fmt(double d)
{
    char buffer[50];
    size_t l;

    puts("Format Test");
    l = RawValue::formatDouble(d, RawValue::DEFAULT, 6, buffer, sizeof(buffer));
    printf("DEFAULT   : '%s' (%d)\n", buffer, l);
    assert(strlen(buffer) == l);
    l = RawValue::formatDouble(d, RawValue::DECIMAL, 6, buffer, sizeof(buffer));
    printf("DECIMAL   : '%s' (%d)\n", buffer, l);
    assert(strlen(buffer) == l);
    l = RawValue::formatDouble(d, RawValue::ENGINEERING, 6, buffer, sizeof(buffer));
    printf("ENGINEERING: '%s' (%d)\n", buffer, l);
    assert(strlen(buffer) == l);
    l = RawValue::formatDouble(d, RawValue::EXPONENTIAL, 6, buffer, sizeof(buffer));
    printf("EXPONENTIAL: '%s' (%d)\n\n", buffer, l);
    assert(strlen(buffer) == l);
}

int main()
{
    initEpicsTimeHelper();
#ifdef DO_FORMAT_TEST
    fmt(0.0);
    fmt(-0.321);
    fmt(1.0e-12);
    fmt(-1.0e-12);
    fmt(3.14e-7);
    fmt(3.14);
    fmt(3.14e+7);
    fmt(-3.14e+7);
    fmt(-0.123456789);
#endif
    if (!update_test("test/update.tst",
                     update_data, sizeof(update_data)/sizeof(TestData),
                     "test/update_data.dot"))
        return -1;
#if 0
    if (!fill_test(false, "test/tree.tst",
                   fill_data, sizeof(fill_data)/sizeof(TestData),
                   "test/test_data1.dot"))
        return -1;
    if (!dump_blocks())
        return -1;
    if (!fill_test(true, "test/index1",
                   man_data1, sizeof(man_data1)/sizeof(TestData),
                   "man_data1.dot"))
        return -1;
    if (!fill_test(true, "test/index2",
                   man_data2, sizeof(man_data2)/sizeof(TestData),
                   "man_data2.dot"))
        return -1;
#endif
    return 0;
}
